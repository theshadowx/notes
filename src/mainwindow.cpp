/**************************************************************************************
* We believe in the power of notes to help us record ideas and thoughts.
* We want people to have an easy, beautiful and simple way of doing that.
* And so we have Notes.
***************************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qxtglobalshortcut.h"
#include "tagnotemodel.h"

#include <QShortcut>
#include <QProgressDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QWidgetAction>
#include <QtConcurrent>
#include <QThread>

MainWindow::MainWindow (QWidget *parent) :
    QMainWindow (parent),
    ui (new Ui::MainWindow),
    m_settingsDatabase(Q_NULLPTR),
    m_greenMaximizeButton(Q_NULLPTR),
    m_redCloseButton(Q_NULLPTR),
    m_yellowMinimizeButton(Q_NULLPTR),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_trayRestoreAction(new QAction(tr("&Hide Notes"), this)),
    m_quitAction(new QAction(tr("&Quit"), this)),
    m_trayIconMenu(new QMenu(this)),
    m_dbManager(Q_NULLPTR),
    m_folderTagWidget(Q_NULLPTR),
    m_noteWidget(Q_NULLPTR),
    m_editorWidget(Q_NULLPTR),
    m_canMoveWindow(false),
    m_isContentModified(false)
{
    ui->setupUi(this);

    setupMainWindow();
    setupTrayIcon();
    setupTitleBarButtons();
    setupSplitter();
    setupDatabases();
    restoreStates();
    setupKeyboardShortcuts();
    setupSignalsSlots();

    QTimer::singleShot(200,this, SLOT(InitData()));
}

MainWindow::~MainWindow ()
{
    delete ui;
}

void MainWindow::setupMainWindow ()
{
#ifdef Q_OS_LINUX
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#elif _WIN32
    this->setWindowFlags(Qt::CustomizeWindowHint);
#elif __APPLE__
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#else
#error "We don't support that version yet..."
#endif

    m_greenMaximizeButton = ui->greenMaximizeButton;
    m_redCloseButton = ui->redCloseButton;
    m_yellowMinimizeButton = ui->yellowMinimizeButton;
    m_splitter = ui->splitter;
    m_folderTagWidget = ui->folderTagWidget;
    m_noteWidget = ui->noteWidget;
    m_editorWidget = ui->editorWidget;

    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(248, 248, 248));
    this->setAutoFillBackground(true);
    this->setPalette(pal);
}

void MainWindow::setupTrayIcon()
{
    m_trayIconMenu->addAction(m_trayRestoreAction);
    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_quitAction);

    QIcon icon(":images/notes_system_tray_icon.png");
    m_trayIcon->setIcon(icon);
    m_trayIcon->setContextMenu(m_trayIconMenu);
    m_trayIcon->show();
}

void MainWindow::setupTitleBarButtons ()
{
    m_redCloseButton->installEventFilter(this);
    m_yellowMinimizeButton->installEventFilter(this);
    m_greenMaximizeButton->installEventFilter(this);
}

void MainWindow::setupSplitter()
{
    QList<int> sizes;
    int width = this->minimumWidth();
    sizes << 0.25*width << 0.25*width << 0.5*width;
    ui->splitter->setSizes(sizes);
    ui->splitter->setCollapsible(0,false);
    ui->splitter->setCollapsible(1,false);
    ui->splitter->setCollapsible(2,false);
}

void MainWindow::setupDatabases ()
{
    m_settingsDatabase = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Awesomeness", "Settings", this);
    m_settingsDatabase->setFallbacksEnabled(false);
    initializeSettingsDatabase();

    bool doCreate = false;
    QFileInfo fi(m_settingsDatabase->fileName());
    QDir dir(fi.absolutePath());
    QString noteDBFilePath(dir.path() + "/notes.db");

    // create database if it doesn't exist
    if(!QFile::exists(noteDBFilePath)){
        QFile noteDBFile(noteDBFilePath);
        if(!noteDBFile.open(QIODevice::WriteOnly)){
            qDebug() << __FILE__ << " " << __FUNCTION__ << " " << __LINE__
                     << " can't create db file";
            qApp->exit(-1);
        }
        noteDBFile.close();
        doCreate = true;
    }

    m_dbManager = new DBManager;
    QThread* dbThread = new QThread;
    dbThread->setObjectName(QStringLiteral("dbThread"));
    m_dbManager->moveToThread(dbThread);
    connect(dbThread, &QThread::started, [=](){m_dbManager->open(noteDBFilePath, doCreate);});
    dbThread->start();
}

void MainWindow::restoreStates()
{
    this->restoreGeometry(m_settingsDatabase->value("windowGeometry").toByteArray());

    m_splitter->restoreState(m_settingsDatabase->value("splitterSizes").toByteArray());
}

void MainWindow::setupKeyboardShortcuts ()
{
//    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_E), m_noteWidget->m_searchField, SLOT(clear()));
//    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), m_noteWidget->m_searchField, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F), this, SLOT(fullscreenWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), m_noteWidget, SLOT(setFocusOnCurrentNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_M), this, SLOT(maximizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), this, SLOT(minimizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_N), m_noteWidget, SLOT(onAddNoteButtonClicked()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this, SLOT(QuitApplication()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Delete), m_noteWidget, SLOT(removeSelectedNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), m_noteWidget, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), m_noteWidget, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::Key_Down), m_noteWidget, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::Key_Up), m_noteWidget, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter), m_editorWidget, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), m_editorWidget, SLOT(setFocus()));
    //    new QShortcut(QKeySequence(Qt::Key_Enter), this, SLOT(setFocusOnText()));
    //    new QShortcut(QKeySequence(Qt::Key_Return), this, SLOT(setFocusOnText()));
    QxtGlobalShortcut *shortcut = new QxtGlobalShortcut(this);
    shortcut->setShortcut(QKeySequence("META+N"));
    connect(shortcut, &QxtGlobalShortcut::activated,[=]() {
        setMainWindowVisibility(isHidden()
                                || windowState() == Qt::WindowMinimized
                                || qApp->applicationState() == Qt::ApplicationInactive);
    });
}

void MainWindow::setupSignalsSlots()
{
    // green button / red button / yellow button
    connect(m_greenMaximizeButton, &QPushButton::clicked, this, &MainWindow::onGreenMaximizeButtonClicked);
    connect(m_redCloseButton, &QPushButton::clicked, this, &MainWindow::onRedCloseButtonClicked);
    connect(m_yellowMinimizeButton, &QPushButton::clicked, this, &MainWindow::onYellowMinimizeButtonClicked);
    // Restore Notes Action
    connect(m_trayRestoreAction, &QAction::triggered, this, &MainWindow::onTrayRestoreActionTriggered);
    // Quit Action
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::QuitApplication);
    // FolderTagWidget
    connect(m_folderTagWidget, &FolderTagWidget::allNotesFolderSelected, this, &MainWindow::onAllNotesFolderSelected);
    connect(m_folderTagWidget, &FolderTagWidget::trashFolderSelected, this, &MainWindow::onTrashFolderSelected);
    connect(m_folderTagWidget, &FolderTagWidget::folderAdded, this, &MainWindow::onFolderAdded);
    connect(m_folderTagWidget, &FolderTagWidget::folderRemoved, this, &MainWindow::onFolderRemoved);
    connect(m_folderTagWidget, &FolderTagWidget::folderUpdated, this, &MainWindow::onFolderUpdated);
    connect(m_folderTagWidget, &FolderTagWidget::folderSelected, this, &MainWindow::onFolderSelected);
    connect(m_folderTagWidget, &FolderTagWidget::tagAdded, this, &MainWindow::onTagAdded);
    connect(m_folderTagWidget, &FolderTagWidget::tagRemoved, this, &MainWindow::onTagRemoved);
    connect(m_folderTagWidget, &FolderTagWidget::tagsRemoved, this, &MainWindow::onTagsRemoved);
    connect(m_folderTagWidget, &FolderTagWidget::tagUpdated, this, &MainWindow::onTagUpdated);
    connect(m_folderTagWidget, &FolderTagWidget::tagSelectionChanged, m_noteWidget, &NoteWidget::filterByTag);
    connect(m_folderTagWidget, &FolderTagWidget::tagSelectionCleared, m_noteWidget, &NoteWidget::clearTagFilter);
    connect(m_folderTagWidget, &FolderTagWidget::tagAboutToBeRemoved, m_noteWidget, &NoteWidget::removeTagFromNotes);
    // NoteWidget
    connect(m_noteWidget, &NoteWidget::noteSelectionChanged, this, &MainWindow::onNoteSelectionChanged);
    connect(m_noteWidget, &NoteWidget::newNoteAdded, this, &MainWindow::onNewNoteAdded);
    connect(m_noteWidget, &NoteWidget::noteAdded, this, &MainWindow::onNoteAdded);
    connect(m_noteWidget, &NoteWidget::noteRemoved, this, &MainWindow::onNoteRemoved);
    connect(m_noteWidget, &NoteWidget::noteTagMenuAboutTobeShown, this, &MainWindow::onNoteTagMenuAboutTobeShown);
    connect(m_noteWidget, &NoteWidget::menuAddTagClicked, this, &MainWindow::onNoteMenuAddTagClicked);
    connect(m_noteWidget, &NoteWidget::tagIndexesToBeAdded, this, &MainWindow::onTagIndexesToBeAdded);
    connect(m_noteWidget, &NoteWidget::tagsInNoteChanged, m_folderTagWidget, &FolderTagWidget::onTagsInNoteChanged);
    connect(m_noteWidget, &NoteWidget::noteUpdated, this, &MainWindow::onNoteUpdated);
    connect(m_noteWidget, &NoteWidget::noteSearchBegin, this, &MainWindow::onNoteSearchBegin);
    connect(m_noteWidget, &NoteWidget::noteModelContentChanged, this, &MainWindow::onNoteModelContentChanged);
    // EditorWidget
    connect(m_editorWidget, &EditorWidget::editorFocusedIn, this, &MainWindow::onEditorFocusedIn);
    connect(m_editorWidget, &EditorWidget::editorTextChanged, m_noteWidget, &NoteWidget::setNoteText);
    // From DataBase
    connect(m_dbManager, &DBManager::foldersReceived, m_folderTagWidget, &FolderTagWidget::initFolders);
    connect(m_dbManager, &DBManager::tagsReceived, m_folderTagWidget, &FolderTagWidget::initTags);
    connect(m_dbManager, &DBManager::notesReceived, m_noteWidget, &NoteWidget::initNotes);
    connect(m_dbManager, &DBManager::notesInTrashReceived, m_noteWidget, &NoteWidget::initNotes);
    connect(m_dbManager, &DBManager::tablesLastRowIdReceived, this, &MainWindow::onTableLastRowIdReceived);
    // To DataBase
    connect(this, &MainWindow::tablesLastRowIdRequested, m_dbManager, &DBManager::onTablesLastRowIdRequested);

    connect(this, &MainWindow::foldersRequested, m_dbManager, &DBManager::onFoldersRequested);
    connect(this, &MainWindow::addFolderRequested, m_dbManager, &DBManager::onAddFolderRequested);
    connect(this, &MainWindow::removeFolderRequested, m_dbManager, &DBManager::onRemoveFolderRequested);
    connect(this, &MainWindow::updateFolderRequested, m_dbManager, &DBManager::onUpdateFolderRequested);

    connect(this, &MainWindow::tagsRequested, m_dbManager, &DBManager::onTagsRequested);
    connect(this, &MainWindow::addTagRequested, m_dbManager, &DBManager::onAddTagRequested);
    connect(this, &MainWindow::removeTagRequested, m_dbManager, &DBManager::onRemoveTagRequested);
    connect(this, &MainWindow::removeTagsRequested, m_dbManager, &DBManager::onRemoveTagsRequested);
    connect(this, &MainWindow::updateTagRequested, m_dbManager, &DBManager::onUpdateTagRequested);

    connect(this, &MainWindow::migrateNoteInTrashResquested, m_dbManager, &DBManager::onMigrateNoteInTrashResquested);
    connect(this, &MainWindow::migrateNoteResquested, m_dbManager, &DBManager::onMigrateNoteResquested);
    connect(this, &MainWindow::allNotesRequested, m_dbManager, &DBManager::onAllNotesRequested);
    connect(this, &MainWindow::notesInTrashRequested, m_dbManager, &DBManager::onNotesInTrashRequested);
    connect(this, &MainWindow::notesRequested, m_dbManager, &DBManager::onNotesRequested);
    connect(this, &MainWindow::addNoteRequested, m_dbManager, &DBManager::onAddNoteRequested);
    connect(this, &MainWindow::removeNoteRequested, m_dbManager, &DBManager::onRemoveNoteRequested);
    connect(this, &MainWindow::updateNoteRequested, m_dbManager, &DBManager::onUpdateNoteRequested);
}


void MainWindow::InitData()
{
    QFileInfo fi(m_settingsDatabase->fileName());
    QDir dir(fi.absolutePath());
    QString oldNoteDBPath(dir.path() + "/Notes.ini");
    QString oldTrashDBPath(dir.path() + "/Trash.ini");

    bool exist = (QFile::exists(oldNoteDBPath) || QFile::exists(oldTrashDBPath));
    // migrate the old notes contained in files to  SQLite DB
    if(exist){
        QProgressDialog* pd = new QProgressDialog("Migrating database, please wait.", "", 0, 0, this);
        pd->setCancelButton(0);
        pd->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        pd->setMinimumDuration(0);
        pd->show();

        setButtonsAndFieldsEnabled(false);

        QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
        connect(watcher, &QFutureWatcher<void>::finished, this, [&](){
            pd->deleteLater();
            setButtonsAndFieldsEnabled(true);
            // get last tables id
            emit tablesLastRowIdRequested();
            // get Folder list
            emit foldersRequested();
            // get tag list
            emit tagsRequested();
          });

        QFuture<void> migration = QtConcurrent::run(this, &MainWindow::checkMigration);
        watcher->setFuture(migration);

    }else{
        // get last tables id
        emit tablesLastRowIdRequested();
        // get Folder list
        emit foldersRequested();
        // get tag list
        emit tagsRequested();
    }
}

void MainWindow::onTableLastRowIdReceived(int noteRowId, int tagRowId, int FolderRowId)
{
    m_noteWidget->initNoteCounter(noteRowId);
    m_folderTagWidget->initTagCounter(tagRowId);
    m_folderTagWidget->initFolderCounter(FolderRowId);
}

void MainWindow::onTrashFolderSelected()
{
    m_editorWidget->clearTextAndHeader();
    m_noteWidget->reset();

    m_noteWidget->setAddingNoteEnabled(false);
    m_noteWidget->setNoteDeletionEnabled(false);
    setNoteEditabled(false);

    emit notesInTrashRequested();
}

void MainWindow::onAllNotesFolderSelected()
{
    // initialize
    m_editorWidget->clearTextAndHeader();
    m_noteWidget->reset();

    // set flags
    m_noteWidget->setAddingNoteEnabled(false);
    m_noteWidget->setNoteDeletionEnabled(false);
    setNoteEditabled(false);

    // get notes from database  and them to the model
    emit allNotesRequested();
}

void MainWindow::onFolderSelected(const QString& folderPath, const int noteCount)
{
    // initialize
    m_editorWidget->clearTextAndHeader();
    m_noteWidget->reset();
    m_noteWidget->setCurrentFolderPath(folderPath);

    // enable Add/delete/edit of notes
    m_noteWidget->setAddingNoteEnabled(true);
    m_noteWidget->setNoteDeletionEnabled(true);
    setNoteEditabled(true);

    if(noteCount >0)
        emit notesRequested(folderPath);
}

void MainWindow::onFolderAdded(FolderData* folder)
{
    Q_ASSERT_X(folder != Q_NULLPTR, "MainWindow::onFolderAdded", "folder is null");

    emit addFolderRequested(folder);
}

void MainWindow::onFolderRemoved(const int folderId)
{
    emit removeFolderRequested(folderId);
}

void MainWindow::onFolderUpdated(const FolderData* folder)
{
    Q_ASSERT_X(folder != Q_NULLPTR, "MainWindow::onFolderUpdated", "folder is null");

    emit updateFolderRequested(folder);
}

void MainWindow::onTagAdded(TagData* tag)
{
    Q_ASSERT_X(tag != Q_NULLPTR, "MainWindow::onTagAdded", "tag is null");

    emit addTagRequested(tag);
}

void MainWindow::onTagRemoved(TagData* tag)
{
    Q_ASSERT_X(tag != Q_NULLPTR, "MainWindow::onTagRemoved", "tag is null");

    emit removeTagRequested(tag);
}

void MainWindow::onTagsRemoved(QList<TagData*> tagList)
{
    Q_ASSERT_X(tagList.count() > 0, "MainWindow::onTagsRemoved", "list is empty");

    emit removeTagsRequested(tagList);
}

void MainWindow::onTagUpdated(const TagData* tag)
{
    Q_ASSERT_X(tag != Q_NULLPTR, "MainWindow::onTagUpdated", "tag is null");

    m_noteWidget->updateNoteView();
    emit updateTagRequested(tag);

}

void MainWindow::onNoteSelectionChanged(QModelIndex selected, QModelIndex deselected)
{
    if(deselected.isValid()){
        // save the position of scrollbar to the settings
        int pos = m_editorWidget->scrollBarValue();
        int prevPos = deselected.data(NoteModel::NoteScrollbarPos).toInt();
        if(prevPos != pos)
            m_noteWidget->setNoteScrollBarPosition(deselected, pos);
    }

    if(!selected.isValid()){
         m_editorWidget->clearTextAndHeader();
        return;
    }

    m_editorWidget->showNoteInEditor(selected);
}

void MainWindow::onNewNoteAdded(QModelIndex index)
{
    if(!m_folderTagWidget->isTagSelectionEmpty())
        m_folderTagWidget->clearTagSelection();

    m_editorWidget->showNoteInEditor(index);
    m_editorWidget->setFocus();
}

void MainWindow::onNoteAdded(NoteData* note)
{
    Q_ASSERT_X(note != Q_NULLPTR, "MainWindow::onNoteAdded", "note is null");

    m_folderTagWidget->onNoteAdded();
    emit addNoteRequested(note);
}

void MainWindow::onNoteRemoved(NoteData* note)
{
    Q_ASSERT_X(note != Q_NULLPTR, "MainWindow::onNoteRemoved", "note is null");

    bool isEmpty = m_noteWidget->isViewEmpty();
    m_noteWidget->setNoteDeletionEnabled(!isEmpty);
    m_folderTagWidget->onNoteRemoved();

    if(m_folderTagWidget->folderType() == FolderTagWidget::AllNotes)
        setNoteEditabled(!isEmpty);

    emit removeNoteRequested(note);
}

void MainWindow::onNoteTagMenuAboutTobeShown(const QModelIndex& index, QMenu& menu)
{
    QWidgetAction* action = qobject_cast<QWidgetAction*>(menu.actions()[0]);
    QListView* view = qobject_cast<QListView*>(action->defaultWidget());
    TagNoteModel* noteTagmodel = static_cast<TagNoteModel*>(view->model());
    noteTagmodel->setTagModel(m_folderTagWidget->tagModel());

    QList<QPersistentModelIndex> tagIndexes = index.data(NoteModel::NoteTagIndexList).value<QList<QPersistentModelIndex>>();
    foreach (QPersistentModelIndex tagIndex, tagIndexes){
        QModelIndex indexInTagModel = noteTagmodel->index(tagIndex.row());
        noteTagmodel->setData(indexInTagModel, Qt::Checked, Qt::CheckStateRole);
    }
}

void MainWindow::onNoteMenuAddTagClicked(QModelIndex index)
{
    QPersistentModelIndex tagToAddIndex = m_folderTagWidget->addNewTagInEditMode();

    QList<QPersistentModelIndex> tagIndexes = index.data(NoteModel::NoteTagIndexList).value<QList<QPersistentModelIndex>>();
    tagIndexes.append(tagToAddIndex);
    m_noteWidget->setNoteTagIndexes(index, tagIndexes);
}

void MainWindow::onTagIndexesToBeAdded(const int noteId, const QString& tagIdString)
{
    QModelIndexList tagIndexes = m_folderTagWidget->tagIndexesFromIds(tagIdString);
    m_noteWidget->addTagIndexesToNote(noteId, tagIndexes);
}

void MainWindow::onNoteUpdated(NoteData* note)
{
    Q_ASSERT_X(note != Q_NULLPTR, "MainWindow::onNoteUpdated", "note is null");

    emit updateNoteRequested(note);
}

void MainWindow::onNoteSearchBegin()
{
    m_folderTagWidget->clearTagSelection();
}

void MainWindow::onNoteModelContentChanged()
{
    if(m_folderTagWidget->folderType() == FolderTagWidget::AllNotes){
        bool isViewEmpty =  m_noteWidget->isViewEmpty();
        setNoteEditabled(!isViewEmpty);
        m_noteWidget->setNoteDeletionEnabled(!isViewEmpty);
    }
}

void MainWindow::onEditorFocusedIn()
{
    switch (m_folderTagWidget->folderType()) {
    case FolderTagWidget::Normal:
        setNoteEditabled(true);
        m_folderTagWidget->addNewFolderIfNotExists();
        m_folderTagWidget->blockSignals(true);
        m_folderTagWidget->clearTagSelection();
        m_folderTagWidget->blockSignals(false);
        break;
    case FolderTagWidget::AllNotes:
        setNoteEditabled(!m_noteWidget->isEmpty());
        m_folderTagWidget->clearTagSelection();
        break;
    default:
        m_folderTagWidget->clearTagSelection();
        break;
    }

    m_noteWidget->prepareForTextEdition();
}

void MainWindow::setMainWindowVisibility(bool state)
{
    if(state){
        showNormal();
        setWindowState(Qt::WindowNoState);
        qApp->processEvents();
        setWindowState(Qt::WindowActive);
        qApp->processEvents();
        qApp->setActiveWindow(this);
        qApp->processEvents();
        m_trayRestoreAction->setText(tr("&Hide Notes"));
    }else{
        m_trayRestoreAction->setText(tr("&Show Notes"));
        hide();
    }
}

void MainWindow::initializeSettingsDatabase()
{
    if(m_settingsDatabase->value("version", "NULL") == "NULL")
        m_settingsDatabase->setValue("version", qApp->applicationVersion());

    if(m_settingsDatabase->value("defaultWindowWidth", "NULL") == "NULL")
        m_settingsDatabase->setValue("defaultWindowWidth", 757);

    if(m_settingsDatabase->value("defaultWindowHeight", "NULL") == "NULL")
        m_settingsDatabase->setValue("defaultWindowHeight", 341);

    if(m_settingsDatabase->value("windowGeometry", "NULL") == "NULL")
        m_settingsDatabase->setValue("windowGeometry", saveGeometry());

    if(m_settingsDatabase->value("splitterSizes", "NULL") == "NULL")
        m_settingsDatabase->setValue("splitterSizes", m_splitter->saveState());
}

void MainWindow::setButtonsAndFieldsEnabled(bool doEnable)
{
    m_greenMaximizeButton->setEnabled(doEnable);
    m_redCloseButton->setEnabled(doEnable);
    m_yellowMinimizeButton->setEnabled(doEnable);
    m_noteWidget->setAddingNoteEnabled(doEnable);
    m_noteWidget->setNoteDeletionEnabled(doEnable);
    m_editorWidget->setEnabled(doEnable);
}

void MainWindow::onTrayRestoreActionTriggered()
{
    setMainWindowVisibility(isHidden()
                            || windowState() == Qt::WindowMinimized
                            || (qApp->applicationState() == Qt::ApplicationInactive));
}

void MainWindow::fullscreenWindow ()
{
    switch (windowState()){
    case Qt::WindowFullScreen:
        m_greenMaximizeButton->setProperty("fullscreen",false);
        setWindowState(Qt::WindowNoState);
        break;
    default:
        m_greenMaximizeButton->setProperty("fullscreen",true);
        setWindowState(Qt::WindowFullScreen);
        break;
    }
}

void MainWindow::maximizeWindow ()
{
    m_greenMaximizeButton->setProperty("fullscreen",false);

    switch (windowState()) {
    case Qt::WindowFullScreen:
    case Qt::WindowMaximized:
        setWindowState(Qt::WindowNoState);
        break;
    default:
        setWindowState(Qt::WindowMaximized);
        break;
    }
}

void MainWindow::minimizeWindow ()
{
    this->setWindowState(Qt::WindowMinimized);
}

void MainWindow::QuitApplication ()
{
    MainWindow::close();
}

void MainWindow::onGreenMaximizeButtonClicked()
{
    fullscreenWindow();
}

void MainWindow::onYellowMinimizeButtonClicked()
{
    minimizeWindow();
    m_trayRestoreAction->setText(tr("&Show Notes"));
}

void MainWindow::onRedCloseButtonClicked()
{
    setMainWindowVisibility(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(windowState() != Qt::WindowFullScreen && windowState() != Qt::WindowMaximized)
        m_settingsDatabase->setValue("windowGeometry", saveGeometry());

    m_settingsDatabase->setValue("splitterSizes", m_splitter->saveState());
    m_settingsDatabase->sync();

    QWidget::closeEvent(event);
}

void MainWindow::mousePressEvent (QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton){
        if(event->pos().x() < this->width() - 5
                && event->pos().x() >5
                && event->pos().y() < this->height()-5
                && event->pos().y() > 5){

            m_canMoveWindow = true;
            m_mousePressX = event->x();
            m_mousePressY = event->y();
        }
    }

    event->accept();
}

void MainWindow::mouseMoveEvent (QMouseEvent* event)
{
    if(m_canMoveWindow){
        this->setCursor(Qt::ClosedHandCursor);
        int dx = event->globalX() - m_mousePressX;
        int dy = event->globalY() - m_mousePressY;
        move (dx, dy);
    }
}

void MainWindow::mouseReleaseEvent (QMouseEvent *event)
{
    m_canMoveWindow = false;
    this->unsetCursor();
    event->accept();
}

void MainWindow::checkMigration()
{
    QFileInfo fi(m_settingsDatabase->fileName());
    QDir dir(fi.absolutePath());

    QString oldNoteDBPath(dir.path() + "/Notes.ini");
    if(QFile::exists(oldNoteDBPath))
        migrateNote(oldNoteDBPath);

    QString oldTrashDBPath(dir.path() + "/Trash.ini");
    if(QFile::exists(oldTrashDBPath))
        migrateTrash(oldTrashDBPath);
}

void MainWindow::migrateNote(QString notePath)
{
    QSettings notesIni(QSettings::IniFormat, QSettings::UserScope, "Awesomeness", "Notes");
    QStringList dbKeys = notesIni.allKeys();

    auto it = dbKeys.begin();
    for(; it < dbKeys.end()-1; it += 3){
        QString noteName = it->split("/")[0];
        int id = noteName.split("_")[1].toInt();


        NoteData* newNote = new NoteData();
        newNote->setId(id);

        QString cntStr = notesIni.value("notesCounter", "NULL").toString();
        if(cntStr == "NULL"){
            m_noteWidget->initNoteCounter(0);
        }else{
            m_noteWidget->initNoteCounter(cntStr.toInt());
        }

        QString createdDateDB = notesIni.value(noteName + "/dateCreated", "Error").toString();
        newNote->setCreationDateTime(QDateTime::fromString(createdDateDB, Qt::ISODate));
        QString lastEditedDateDB = notesIni.value(noteName + "/dateEdited", "Error").toString();
        newNote->setLastModificationDateTime(QDateTime::fromString(lastEditedDateDB, Qt::ISODate));
        QString contentText = notesIni.value(noteName + "/content", "Error").toString();
        newNote->setContent(contentText);
        QString firstLine = NoteWidget::getFirstLine(contentText);
        newNote->setFullTitle(firstLine);

        emit migrateNoteResquested(newNote);
        newNote->deleteLater();
    }

    QFile oldNoteDBFile(notePath);
    oldNoteDBFile.rename(QFileInfo(notePath).dir().path() + "/oldNotes.ini");
}

void MainWindow::migrateTrash(QString trashPath)
{
    QSettings trashIni(QSettings::IniFormat, QSettings::UserScope, "Awesomeness", "Trash");

    QStringList dbKeys = trashIni.allKeys();

    auto it = dbKeys.begin();
    for(; it < dbKeys.end()-1; it += 3){
        QString noteName = it->split("/")[0];
        int id = noteName.split("_")[1].toInt();

        NoteData* newNote = new NoteData();
        newNote->setId(id);

        QString createdDateDB = trashIni.value(noteName + "/dateCreated", "Error").toString();
        newNote->setCreationDateTime(QDateTime::fromString(createdDateDB, Qt::ISODate));
        QString lastEditedDateDB = trashIni.value(noteName + "/dateEdited", "Error").toString();
        newNote->setLastModificationDateTime(QDateTime::fromString(lastEditedDateDB, Qt::ISODate));
        QString contentText = trashIni.value(noteName + "/content", "Error").toString();
        newNote->setContent(contentText);
        QString firstLine = NoteWidget::getFirstLine(contentText);
        newNote->setFullTitle(firstLine);

        emit migrateNoteInTrashResquested(newNote);
        newNote->deleteLater();
    }

    QFile oldTrashDBFile(trashPath);
    oldTrashDBFile.rename(QFileInfo(trashPath).dir().path() + "/oldTrash.ini");
}

void MainWindow::setNoteEditabled(bool state)
{
    m_noteWidget->setNoteEditable(state);
    m_editorWidget->setTextEditable(state);
}

void MainWindow::mouseDoubleClickEvent (QMouseEvent *event)
{
    maximizeWindow();
    event->accept();
}

void MainWindow::leaveEvent(QEvent *)
{
    this->unsetCursor();
}

bool MainWindow::eventFilter (QObject *object, QEvent *event)
{
    switch (event->type()){
    case QEvent::Enter:
        if(qApp->applicationState() == Qt::ApplicationActive){
            // When hovering one of the traffic light buttons (red, yellow, green),
            // set new icons to show their function
            if(object == m_redCloseButton
                    || object == m_yellowMinimizeButton
                    || object == m_greenMaximizeButton){

                m_redCloseButton->setProperty("hovered", true);
                m_yellowMinimizeButton->setProperty("hovered", true);
                m_greenMaximizeButton->setProperty("hovered", true);
                qApp->setStyleSheet(qApp->styleSheet());
            }
        }
        break;

    case QEvent::Leave:
        if(qApp->applicationState() == Qt::ApplicationActive){
            // When not hovering, change back the icons of the traffic lights to their default icon
            if(object == m_redCloseButton
                    || object == m_yellowMinimizeButton
                    || object == m_greenMaximizeButton){

                m_redCloseButton->setProperty("hovered", false);
                m_yellowMinimizeButton->setProperty("hovered", false);
                m_greenMaximizeButton->setProperty("hovered", false);

                qApp->setStyleSheet(qApp->styleSheet());
            }
        }
        break;

    case QEvent::WindowDeactivate:
        this->setEnabled(false);
        break;

    case QEvent::WindowActivate:
        this->setEnabled(true);
        break;

    default:
        break;
    }

    return QObject::eventFilter(object, event);
}
