/**************************************************************************************
* We believe in the power of notes to help us record ideas and thoughts.
* We want people to have an easy, beautiful and simple way of doing that.
* And so we have Notes.
***************************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qxtglobalshortcut.h"
#include "tagnotemodel.h"

#include <QScrollBar>
#include <QShortcut>
#include <QTextStream>
#include <QScrollArea>
#include <QtConcurrent>
#include <QProgressDialog>
#include <QGraphicsDropShadowEffect>
#include <QWidgetAction>
#include <QColorDialog>

/**
* Setting up the main window and it's content
*/
MainWindow::MainWindow (QWidget *parent) :
    QMainWindow (parent),
    ui (new Ui::MainWindow),
    m_settingsDatabase(Q_NULLPTR),
    m_greenMaximizeButton(Q_NULLPTR),
    m_redCloseButton(Q_NULLPTR),
    m_yellowMinimizeButton(Q_NULLPTR),
    m_textEdit(Q_NULLPTR),
    m_editorDateLabel(Q_NULLPTR),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_trayRestoreAction(new QAction(tr("&Hide Notes"), this)),
    m_quitAction(new QAction(tr("&Quit"), this)),
    m_trayIconMenu(new QMenu(this)),
    m_dbManager(Q_NULLPTR),
    m_folderTagWidget(Q_NULLPTR),
    m_noteWidget(Q_NULLPTR),
    m_canMoveWindow(false),
    m_isContentModified(false),
    m_isTextEditable(true)
{
    ui->setupUi(this);
    setupMainWindow();
    setupTrayIcon();
    setupKeyboardShortcuts();
    setupSplitter();
    setupTitleBarButtons();
    setupTextEdit();
    setupDatabases();
    restoreStates();
    setupSignalsSlots();

    QTimer::singleShot(200,this, SLOT(InitData()));
}

/**
 * @brief Init the data from database and select the first note if there is one
 */
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
            // get Folder list
            m_dbManager->getAllFolders();
            // get tag list
            m_dbManager->getAllTags();
          });

        QFuture<void> migration = QtConcurrent::run(this, &MainWindow::checkMigration);
        watcher->setFuture(migration);

    }else{
        // get Folder list
        m_dbManager->getAllFolders();
        // get tag list
        m_dbManager->getAllTags();
    }
}


void MainWindow::onTrashFolderSelected()
{
    // initialize

    clearTextAndHeader();
    m_noteWidget->reset();

    m_noteWidget->setAddingNoteEnabled(false);
    m_noteWidget->setNoteDeletionEnabled(false);
    setNoteEditabled(false);

    m_dbManager->getNotesInTrash();
}

void MainWindow::onAllNotesFolderSelected()
{
    // initialize
    clearTextAndHeader();
    m_noteWidget->reset();

    // set flags
    m_noteWidget->setAddingNoteEnabled(false);
    m_noteWidget->setNoteDeletionEnabled(false);
    setNoteEditabled(false);

    // get notes from database  and them to the model
    m_dbManager->getAllNotes();
}

void MainWindow::onFolderSelected(const QString& folderPath, const int noteCount)
{
    // initialize
    clearTextAndHeader();
    m_noteWidget->reset();
    m_noteWidget->setCurrentFolderPath(folderPath);

    // enable Add/delete/edit of notes
    m_noteWidget->setAddingNoteEnabled(true);
    m_noteWidget->setNoteDeletionEnabled(true);
    setNoteEditabled(true);

    if(noteCount >0)
        m_dbManager->getAllNotes(folderPath);
}

void MainWindow::onFolderAdded(FolderData* folder)
{
    if(folder != Q_NULLPTR)
        QtConcurrent::run(m_dbManager, &DBManager::addFolder, folder);
}

void MainWindow::onFolderRemoved(const int folderId)
{
    QtConcurrent::run(m_dbManager, &DBManager::removeFolder, folderId);
}

void MainWindow::onFolderUpdated(const FolderData* folder)
{
    if(folder != Q_NULLPTR)
        QtConcurrent::run(m_dbManager, &DBManager::modifyFolder, folder);
}

void MainWindow::onTagAdded(TagData* tag)
{
    if(tag != Q_NULLPTR)
     QtConcurrent::run(m_dbManager, &DBManager::addTag, tag);
}

void MainWindow::onTagRemoved(TagData* tag)
{
    if(tag != Q_NULLPTR)
        QtConcurrent::run(m_dbManager, &DBManager::removeTag, tag);
}

void MainWindow::onTagsRemoved(QList<TagData*> tagList)
{
    QtConcurrent::run(m_dbManager, &DBManager::removeTags, tagList);
}

void MainWindow::onTagUpdated(const TagData* tag)
{
    if(tag != Q_NULLPTR){
        m_noteWidget->updateNoteView();
        QtConcurrent::run(m_dbManager, &DBManager::modifyTag, tag);
    }
}

void MainWindow::onNoteSelectionChanged(QModelIndex selected, QModelIndex deselected)
{
    if(deselected.isValid()){
        // save the position of scrollbar to the settings
        int pos = m_textEdit->verticalScrollBar()->value();
        int prevPos = deselected.data(NoteModel::NoteScrollbarPos).toInt();
        if(prevPos != pos)
            m_noteWidget->setNoteScrollBarPosition(deselected, pos);
    }

    if(!selected.isValid()){
        clearTextAndHeader();
        return;
    }

    showNoteInEditor(selected);
}

void MainWindow::onNewNoteAdded(QModelIndex index)
{
    if(!m_folderTagWidget->isTagSelectionEmpty())
        m_folderTagWidget->clearTagSelection();

    showNoteInEditor(index);

    m_textEdit->setFocus();
}

void MainWindow::onNoteAdded(NoteData* note)
{
    if(note != Q_NULLPTR){
        m_folderTagWidget->onNoteAdded();

        QtConcurrent::run([=](){
            QMutexLocker locker(&m_mutex);
            m_dbManager->addNote(note);
        });
    }
}

void MainWindow::onNoteRemoved(NoteData* note)
{
    if(note != Q_NULLPTR){
        bool isEmpty = m_noteWidget->isViewEmpty();
        m_noteWidget->setNoteDeletionEnabled(!isEmpty);
        m_folderTagWidget->onNoteRemoved();

        if(m_folderTagWidget->folderType() == FolderTagWidget::AllNotes)
            setNoteEditabled(!isEmpty);


        QtConcurrent::run(m_dbManager, &DBManager::removeNote, note);
    }
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
    if(note != Q_NULLPTR)
        QtConcurrent::run(m_dbManager, &DBManager::modifyNote, note);
}

void MainWindow::onNoteSearchBeing()
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

/**
* @brief
* Deconstructor of the class
*/
MainWindow::~MainWindow ()
{
    delete ui;
}

/**
* @brief
* Setting up main window prefrences like frameless window and the minimum size of the window
* Setting the window background color to be white
*/
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
    m_textEdit = ui->textEdit;
    m_editorDateLabel = ui->editorDateLabel;
    m_splitter = ui->splitter;
    m_folderTagWidget = ui->folderTagWidget;
    m_noteWidget = ui->noteWidget;

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

/**
* @brief
* Setting up the keyboard shortcuts
*/
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
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter), this, SLOT(setFocusOnText()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this, SLOT(setFocusOnText()));
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

/**
* @brief
* Set up the splitter that control the size of the scrollArea and the textEdit
*/
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

/**
* @brief
* Setting up the red (close), yellow (minimize), and green (maximize) buttons
* Make only the buttons icon visible
* And install this class event filter to them, to act when hovering on one of them
*/
void MainWindow::setupTitleBarButtons ()
{
    m_redCloseButton->installEventFilter(this);
    m_yellowMinimizeButton->installEventFilter(this);
    m_greenMaximizeButton->installEventFilter(this);
}

/**
 * @brief connect between signals and slots
 */
void MainWindow::setupSignalsSlots()
{
    // green button / red button / yellow button
    connect(m_greenMaximizeButton, &QPushButton::clicked, this, &MainWindow::onGreenMaximizeButtonClicked);
    connect(m_redCloseButton, &QPushButton::clicked, this, &MainWindow::onRedCloseButtonClicked);
    connect(m_yellowMinimizeButton, &QPushButton::clicked, this, &MainWindow::onYellowMinimizeButtonClicked);
    // text edit text changed / lineEdit text changed
    connect(m_textEdit, &QTextEdit::textChanged, this, &MainWindow::onTextEditTextChanged);
    // Restore Notes Action
    connect(m_trayRestoreAction, &QAction::triggered, this, &MainWindow::onTrayRestoreActionTriggered);
    // Quit Action
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::QuitApplication);

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
    connect(m_folderTagWidget, &FolderTagWidget::tagSelectionCleared, m_noteWidget, &NoteWidget::clearFilter);
    connect(m_folderTagWidget, &FolderTagWidget::tagAboutToBeRemoved, m_noteWidget, &NoteWidget::removeTagFromNotes);

    connect(m_dbManager, &DBManager::foldersReceived, m_folderTagWidget, &FolderTagWidget::initFolders);
    connect(m_dbManager, &DBManager::tagsReceived, m_folderTagWidget, &FolderTagWidget::initTags);
    connect(m_dbManager, &DBManager::notesReceived, m_noteWidget, &NoteWidget::initNotes);
    connect(m_dbManager, &DBManager::notesInTrashReceived, m_noteWidget, &NoteWidget::initNotes);

    connect(m_noteWidget, &NoteWidget::noteSelectionChanged, this, &MainWindow::onNoteSelectionChanged);
    connect(m_noteWidget, &NoteWidget::newNoteAdded, this, &MainWindow::onNewNoteAdded);
    connect(m_noteWidget, &NoteWidget::noteAdded, this, &MainWindow::onNoteAdded);
    connect(m_noteWidget, &NoteWidget::noteRemoved, this, &MainWindow::onNoteRemoved);
    connect(m_noteWidget, &NoteWidget::noteTagMenuAboutTobeShown, this, &MainWindow::onNoteTagMenuAboutTobeShown);
    connect(m_noteWidget, &NoteWidget::menuAddTagClicked, this, &MainWindow::onNoteMenuAddTagClicked);
    connect(m_noteWidget, &NoteWidget::tagIndexesToBeAdded, this, &MainWindow::onTagIndexesToBeAdded);
    connect(m_noteWidget, &NoteWidget::tagsInNoteChanged, m_folderTagWidget, &FolderTagWidget::onTagsInNoteChanged);
    connect(m_noteWidget, &NoteWidget::noteUpdated, this, &MainWindow::onNoteUpdated);
    connect(m_noteWidget, &NoteWidget::noteSearchBegin, this, &MainWindow::onNoteSearchBeing);
    connect(m_noteWidget, &NoteWidget::noteModelContentChanged, this, &MainWindow::onNoteModelContentChanged);
}

/**
* @brief
* Setting up textEdit:
* Setup the style of the scrollBar and set textEdit background to an image
* Make the textEdit pedding few pixels right and left, to compel with a beautiful proportional grid
* And install this class event filter to catch when text edit is having focus
*/
void MainWindow::setupTextEdit ()
{
    m_textEdit->installEventFilter(this);
    m_textEdit->verticalScrollBar()->installEventFilter(this);
}

void MainWindow::initializeSettingsDatabase()
{
    if(m_settingsDatabase->value("version", "NULL") == "NULL")
        m_settingsDatabase->setValue("version", "0.8.0");

    if(m_settingsDatabase->value("defaultWindowWidth", "NULL") == "NULL")
        m_settingsDatabase->setValue("defaultWindowWidth", 757);

    if(m_settingsDatabase->value("defaultWindowHeight", "NULL") == "NULL")
        m_settingsDatabase->setValue("defaultWindowHeight", 341);

    if(m_settingsDatabase->value("windowGeometry", "NULL") == "NULL")
        m_settingsDatabase->setValue("windowGeometry", saveGeometry());

    if(m_settingsDatabase->value("splitterSizes", "NULL") == "NULL")
        m_settingsDatabase->setValue("splitterSizes", m_splitter->saveState());
}

/**
* @brief
* Setting up the database:
* The "Company" name is: 'Awesomeness' (So you don't have to scroll when getting to the .config folder)
* The Application name is: 'Notes'
* If it's the first run (or the database is deleted) , create the database and the notesCounter key
* (notesCounter increases it's value everytime a new note is created)
* We chose the Ini format for all operating systems because of future reasons - it might be easier to
* sync databases between diffrent os's when you have one consistent file format. We also think that this
* format, in the way Qt is handling it, is very good for are needs.
* Also because the native format on windows - the registery is very limited.
* The databases are stored in the user scope of the computer. That's mostly in (Qt takes care of this automatically):
* Linux: /home/user/.config/Awesomeness/
* Windows: C:\Users\user\AppData\Roaming\Awesomeness
* Mac OS X:
*/
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

    m_dbManager = new DBManager(noteDBFilePath, doCreate, this);

    int noteCounter = m_dbManager->getNotesLastRowID();
    m_noteWidget->initNoteCounter(noteCounter);

    int folderCounter = m_dbManager->getFoldersLastRowID();
    m_folderTagWidget->initFolderCounter(folderCounter);

    int tagCounter = m_dbManager->getTagsLastRowID();
    m_folderTagWidget->initTagCounter(tagCounter);
}

/**
* @brief
* Restore the latest sates (if there are any) of the window and the splitter from the settings database
*/
void MainWindow::restoreStates()
{
    this->restoreGeometry(m_settingsDatabase->value("windowGeometry").toByteArray());

    m_splitter->restoreState(m_settingsDatabase->value("splitterSizes").toByteArray());
}

/**
 * @brief show the specified note content text in the text editor
 * Set editorDateLabel text to the the selected note date
 * And restore the scrollBar position if it changed before.
 */
void MainWindow::showNoteInEditor(const QModelIndex &noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "MainWindow::showNoteInEditor", "noteIndex is not valid");

    m_textEdit->blockSignals(true);
    QString content = noteIndex.data(NoteModel::NoteContent).toString();
    QDateTime dateTime = noteIndex.data(NoteModel::NoteLastModificationDateTime).toDateTime();
    int scrollbarPos = noteIndex.data(NoteModel::NoteScrollbarPos).toInt();

    // set text and date
    m_textEdit->setText(content);
    QString noteDate = dateTime.toString(Qt::ISODate);
    QString noteDateEditor = dateToLocal(noteDate);
    m_editorDateLabel->setText(noteDateEditor);
    // set scrollbar position
    m_textEdit->verticalScrollBar()->setValue(scrollbarPos);
    m_textEdit->blockSignals(false);
}

void MainWindow::clearTextAndHeader()
{
    // clear text edit and time date label
    m_editorDateLabel->clear();
    m_textEdit->blockSignals(true);
    m_textEdit->clear();
    m_textEdit->clearFocus();
    m_textEdit->blockSignals(false);
}

void MainWindow::setButtonsAndFieldsEnabled(bool doEnable)
{
    m_greenMaximizeButton->setEnabled(doEnable);
    m_redCloseButton->setEnabled(doEnable);
    m_yellowMinimizeButton->setEnabled(doEnable);
    m_noteWidget->setAddingNoteEnabled(doEnable);
    m_noteWidget->setNoteDeletionEnabled(doEnable);
    m_textEdit->setEnabled(doEnable);
}

/**
* @brief
* When the text on textEdit change:
* if the note edited is not on top of the list, we will make that happen
* If the text changed is of a new (empty) note, reset temp values
*/
void MainWindow::onTextEditTextChanged ()
{
    m_textEdit->blockSignals(true);
    QDateTime dateTime = QDateTime::currentDateTime();
    QString noteDate = dateTime.toString(Qt::ISODate);
    m_editorDateLabel->setText(dateToLocal(noteDate));
    m_noteWidget->setNoteText(m_textEdit->toPlainText());
    m_textEdit->blockSignals(false);
}

void MainWindow::onTrayRestoreActionTriggered()
{
    setMainWindowVisibility(isHidden()
                            || windowState() == Qt::WindowMinimized
                            || (qApp->applicationState() == Qt::ApplicationInactive));
}


/**
* @brief
* Set focus on textEdit
*/
void MainWindow::setFocusOnText ()
{
//    if(m_currentSelectedNoteProxy.isValid() && !m_textEdit->hasFocus())
//        m_textEdit->setFocus();
}

/**
* @brief
* Switch to fullscreen mode
*/
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

/**
* @brief
* Maximize the window
*/
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

/**
* @brief
* Minimize the window
*/
void MainWindow::minimizeWindow ()
{
    this->setWindowState(Qt::WindowMinimized);
}

/**
* @brief
* Exit the application
* If a new note created but wasn't edited, delete it from the database
*/
void MainWindow::QuitApplication ()
{
    MainWindow::close();
}

/**
* @brief
* When the green button is released the window goes fullscrren
*/
void MainWindow::onGreenMaximizeButtonClicked()
{
    fullscreenWindow();
}

/**
* @brief
* When yellow button is released the window is minimized
*/
void MainWindow::onYellowMinimizeButtonClicked()
{
    minimizeWindow();
    m_trayRestoreAction->setText(tr("&Show Notes"));
}

/**
* @brief
* When red button is released the window get closed
* If a new note created but wasn't edited, delete it from the database
*/
void MainWindow::onRedCloseButtonClicked()
{
    setMainWindowVisibility(false);
}

/**
 * @brief Called when the app is about the close
 * save the geometry of the app to the settings
 * save the current note if it's note temporary one otherwise remove it from DB
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(windowState() != Qt::WindowFullScreen && windowState() != Qt::WindowMaximized)
        m_settingsDatabase->setValue("windowGeometry", saveGeometry());

    m_settingsDatabase->setValue("splitterSizes", m_splitter->saveState());
    m_settingsDatabase->sync();

    QWidget::closeEvent(event);
}

/**
* @brief
* Set variables to the position of the window when the mouse is pressed
*/
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

/**
* @brief
* Move the window according to the mouse positions
*/
void MainWindow::mouseMoveEvent (QMouseEvent* event)
{
    if(m_canMoveWindow){
        this->setCursor(Qt::ClosedHandCursor);
        int dx = event->globalX() - m_mousePressX;
        int dy = event->globalY() - m_mousePressY;
        move (dx, dy);
    }
}

/**
* @brief
  * Initialize flags
 */
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

        m_dbManager->migrateNote(newNote);
        delete newNote;
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

        m_dbManager->migrateTrash(newNote);
        delete newNote;
    }

    QFile oldTrashDBFile(trashPath);
    oldTrashDBFile.rename(QFileInfo(trashPath).dir().path() + "/oldTrash.ini");
}

void MainWindow::setNoteEditabled(bool state)
{
    m_textEdit->setReadOnly(!state);
    m_noteWidget->setNoteEditable(state);
    m_isTextEditable = state;
}

QDateTime MainWindow::getQDateTime (QString date)
{
    QDateTime dateTime = QDateTime::fromString(date, Qt::ISODate);
    return dateTime;
}

QString MainWindow::dateToLocal (QString dateString)
{
    QDateTime dateTimeEdited(getQDateTime(dateString));
    QLocale usLocale(QLocale("en_US"));

    return usLocale.toString(dateTimeEdited, "MMMM d, yyyy, h:mm A");
}

/**
* @brief
* When the blank area at the top of window is double-clicked the window get maximized
*/
void MainWindow::mouseDoubleClickEvent (QMouseEvent *event)
{
    maximizeWindow();
    event->accept();
}

void MainWindow::leaveEvent(QEvent *)
{
    this->unsetCursor();
}

/**
* @brief
* Mostly take care on the event happened on widget whose filter installed to tht mainwindow
*/
bool MainWindow::eventFilter (QObject *object, QEvent *event)
{
    switch (event->type()){
    case QEvent::Enter:{
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
    }
    case QEvent::Leave:{
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
    }
    case QEvent::WindowDeactivate:{
        this->setEnabled(false);
        break;
    }
    case QEvent::WindowActivate:{
        this->setEnabled(true);
        break;
    }
    case QEvent::HoverEnter:{
        if(object == m_textEdit->verticalScrollBar()){
            if(!m_textEdit->hasFocus()){
                m_textEdit->setFocusPolicy(Qt::NoFocus);
            }
        }
        break;
    }
    case QEvent::HoverLeave:{
        if(object == m_textEdit->verticalScrollBar()){
            bool isNoButtonClicked = qApp->mouseButtons() == Qt::NoButton;
            if(isNoButtonClicked){
                m_textEdit->setFocusPolicy(Qt::StrongFocus);
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease:{
        if(object == m_textEdit->verticalScrollBar()){
            bool isMouseOnScrollBar = qApp->widgetAt(QCursor::pos()) != m_textEdit->verticalScrollBar();
            if(isMouseOnScrollBar){
                m_textEdit->setFocusPolicy(Qt::StrongFocus);
            }
        }
        break;
    }
    case QEvent::FocusIn:{
        if(object == m_textEdit){

            if(m_isTextEditable){
                m_textEdit->setReadOnly(false);

                if(m_folderTagWidget->folderType() == FolderTagWidget::Normal){
                    m_folderTagWidget->addNewFolderIfNotExists();
                    m_folderTagWidget->blockSignals(true);
                    m_folderTagWidget->clearTagSelection();
                    m_folderTagWidget->blockSignals(false);
                    m_noteWidget->prepareForTextEdition();
                    m_textEdit->setFocus();
                }else if(m_folderTagWidget->folderType() == FolderTagWidget::AllNotes){
                    if(!m_noteWidget->isEmpty()){
                        m_folderTagWidget->clearTagSelection();
                        m_noteWidget->prepareForTextEdition();
                        m_textEdit->setFocus();
                    }else{
                        setNoteEditabled(false);
                        m_folderTagWidget->clearTagSelection();
                        m_noteWidget->prepareForTextEdition();
                    }
                }
            }else{
                m_folderTagWidget->clearTagSelection();
                m_noteWidget->prepareForTextEdition();
            }
        }

        break;
    }
    case QEvent::FocusOut:
        if(object == m_textEdit)
            m_textEdit->setReadOnly(true);

        break;
    default:
        break;
    }

    return QObject::eventFilter(object, event);
}
