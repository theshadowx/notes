/**************************************************************************************
* We believe in the power of notes to help us record ideas and thoughts.
* We want people to have an easy, beautiful and simple way of doing that.
* And so we have Notes.
***************************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "notewidgetdelegate.h"
#include "folderwidgetdelegate.h"
#include "tagwidgetdelegate.h"
#include "tagnotemodel.h"
#include "qxtglobalshortcut.h"

#include <QScrollBar>
#include <QShortcut>
#include <QTextStream>
#include <QScrollArea>
#include <QtConcurrent>
#include <QProgressDialog>
#include <QGraphicsDropShadowEffect>
#include <QWidgetAction>
#include <QColorDialog>
#define FIRST_LINE_MAX 80

/**
* Setting up the main window and it's content
*/
MainWindow::MainWindow (QWidget *parent) :
    QMainWindow (parent),
    ui (new Ui::MainWindow),
    m_autoSaveTimer(new QTimer(this)),
    m_settingsDatabase(Q_NULLPTR),
    m_noteWidgetsContainer(Q_NULLPTR),
    m_clearButton(Q_NULLPTR),
    m_greenMaximizeButton(Q_NULLPTR),
    m_redCloseButton(Q_NULLPTR),
    m_yellowMinimizeButton(Q_NULLPTR),
    m_addNoteButton(Q_NULLPTR),
    m_deleteNoteButton(Q_NULLPTR),
    m_tagNoteButton(Q_NULLPTR),
    m_addRootFolderButton(Q_NULLPTR),
    m_deleteRootFolderButton(Q_NULLPTR),
    m_newTagButton(Q_NULLPTR),
    m_deleteTagButton(Q_NULLPTR),
    m_clearTagSelectionButton(Q_NULLPTR),
    m_textEdit(Q_NULLPTR),
    m_lineEdit(Q_NULLPTR),
    m_editorDateLabel(Q_NULLPTR),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_trayRestoreAction(new QAction(tr("&Hide Notes"), this)),
    m_quitAction(new QAction(tr("&Quit"), this)),
    m_trayIconMenu(new QMenu(this)),
    m_folderView(Q_NULLPTR),
    m_generalListW(Q_NULLPTR),
    m_tagView(Q_NULLPTR),
    m_noteModel(new NoteModel(this)),
    m_deletedNotesModel(new NoteModel(this)),
    m_proxyNoteModel(new QSortFilterProxyModel(this)),
    m_folderModel(new FolderModel(this)),
    m_tagModel(new TagModel(this)),
    m_dbManager(Q_NULLPTR),
    m_noteCounter(0),
    m_folderCounter(0),
    m_tagCounter(0),
    m_canMoveWindow(false),
    m_isTemp(false),
    m_isContentModified(false),
    m_isOperationRunning(false),
    m_isNoteEditable(true),
    m_isNoteDeletionEnabled(true),
    m_isAddingNoteEnabled(true)
{
    ui->setupUi(this);
    setupMainWindow();
    setupTrayIcon();
    setupKeyboardShortcuts();
    setupSplitter();
    setupTitleBarButtons();
    setupLineEdit();
    setupTextEdit();
    setupDatabases();
    setupModelView();
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
        connect(watcher, &QFutureWatcher<void>::finished, this, [&, pd](){
            pd->deleteLater();
            setButtonsAndFieldsEnabled(true);
            initFolders();
            initTags();

            // select the first folder in the treeview
            m_folderView->selectionModel()->setCurrentIndex(m_folderModel->index(0,0), QItemSelectionModel::Select);
        });

        QFuture<void> migration = QtConcurrent::run(this, &MainWindow::checkMigration);
        watcher->setFuture(migration);

    }else{
        initFolders();
        initTags();

        // select the first folder in the treeview
        m_folderView->selectionModel()->setCurrentIndex(m_folderModel->index(0,0), QItemSelectionModel::Select);
    }

    ui->scrollAreaFolderTag->ensureVisible(0,0);
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
    m_addNoteButton = ui->addNoteButton;
    m_deleteNoteButton = ui->deleteNoteButton;
    m_tagNoteButton = ui->tagNoteButton;
    m_lineEdit = ui->lineEdit;
    m_textEdit = ui->textEdit;
    m_editorDateLabel = ui->editorDateLabel;
    m_splitter = ui->splitter;
    m_folderView = ui->folderTree;
    m_tagView = ui->tagListView;
    m_addRootFolderButton = ui->addRootFolderButton;
    m_deleteRootFolderButton = ui->delRootFolderButton;
    m_newTagButton = ui->newTagButton;
    m_deleteTagButton = ui->deleteTagButton;
    m_clearTagSelectionButton = ui->clearSelectionButton;
    m_generalListW = ui->generalListW;

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
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_E), m_lineEdit, SLOT(clear()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), m_lineEdit, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F), this, SLOT(fullscreenWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), this, SLOT(setFocusOnCurrentNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_M), this, SLOT(maximizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), this, SLOT(minimizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_N), this, SLOT(onAddNoteButtonClicked()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this, SLOT(QuitApplication()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Delete), this, SLOT(deleteSelectedNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), this, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), this, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::Key_Down), this, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::Key_Up), this, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter), this, SLOT(setFocusOnText()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this, SLOT(setFocusOnText()));
    //    new QShortcut(QKeySequence(Qt::Key_Enter), this, SLOT(setFocusOnText()));
    //    new QShortcut(QKeySequence(Qt::Key_Return), this, SLOT(setFocusOnText()));
    QxtGlobalShortcut *shortcut = new QxtGlobalShortcut(this);
    shortcut->setShortcut(QKeySequence("META+N"));
    connect(shortcut, &QxtGlobalShortcut::activated,[=]() {
        // workaround prevent textEdit and lineEdit
        // from taking 'N' from shortcut
        m_textEdit->setDisabled(true);
        m_lineEdit->setDisabled(true);
        setMainWindowVisibility(isHidden()
                                || windowState() == Qt::WindowMinimized
                                || qApp->applicationState() == Qt::ApplicationInactive);
        m_textEdit->setDisabled(false);
        m_lineEdit->setDisabled(false);
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
    // green button
    connect(m_greenMaximizeButton, &QPushButton::clicked, this, &MainWindow::onGreenMaximizeButtonClicked);
    // red button
    connect(m_redCloseButton, &QPushButton::clicked, this, &MainWindow::onRedCloseButtonClicked);
    // yellow button
    connect(m_yellowMinimizeButton, &QPushButton::clicked, this, &MainWindow::onYellowMinimizeButtonClicked);
    // new note button
    connect(m_addNoteButton, &QPushButton::clicked, this, &MainWindow::onAddNoteButtonClicked);
    // delete note button
    connect(m_deleteNoteButton, &QPushButton::clicked, this, &MainWindow::onDeleteNoteButtonClicked);
    connect(m_noteModel, &NoteModel::rowsRemoved, [this](){m_deleteNoteButton->setEnabled(true);});
    // tag note button
    connect(m_tagNoteButton, &QPushButton::clicked, this, &MainWindow::showTagNoteMenu);
    // add/delete folder button
    connect(m_addRootFolderButton, &QPushButton::clicked, this, &MainWindow::onAddFolderButtonClicked);
    connect(m_deleteRootFolderButton, &QPushButton::clicked, this, &MainWindow::onDeleteFolderButtonClicked);
    // add/delete/select/deselect tag button
    connect(m_newTagButton, &QPushButton::clicked, this, &MainWindow::onAddTagButtonClicked);
    connect(m_deleteTagButton, &QPushButton::clicked, this, &MainWindow::onDeleteTagButtonClicked);
    connect(m_clearTagSelectionButton, &QPushButton::clicked, this, &MainWindow::onClearTagSelectionButtonClicked);
    // text edit text changed
    connect(m_textEdit, &QTextEdit::textChanged, this, &MainWindow::onTextEditTextChanged);
    // line edit text changed
    connect(m_lineEdit, &QLineEdit::textChanged, this, &MainWindow::onLineEditTextChanged);
    // note pressed
    connect(m_noteView, &NoteView::clicked, this, &MainWindow::onNoteClicked);
    // All Notes/ Trash listWidget
    connect(m_generalListW, &QListWidget::currentRowChanged, this, &MainWindow::onGeneralListWCurrentRowChanged);
    // noteView viewport pressed
    connect(m_noteView, &NoteView::viewportClicked, this, [this](){
        if(m_isTemp && m_proxyNoteModel->rowCount() > 1){
            QModelIndex indexInProxy = m_proxyNoteModel->index(1, 0);
            selectNote(indexInProxy);
        }else if(m_isTemp && m_proxyNoteModel->rowCount() == 1){
            QModelIndex indexInProxy = m_proxyNoteModel->index(0, 0);
            m_editorDateLabel->clear();
            deleteNote(indexInProxy);
        }
    });
    // folderView / folder selected / context menu
    connect(m_folderView, &NoteView::customContextMenuRequested, this, &MainWindow::showFolderViewContextMenu);
    connect(m_folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFolderSelectionChanged);
    // tagView contextMenu / selection changed
    connect(m_tagView, &QListView::customContextMenuRequested,this, &MainWindow::showTagViewContextMenu);
    connect(m_tagView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onTagSelectionChanged);
    // Update note count label
    connect(m_proxyNoteModel, &QSortFilterProxyModel::rowsInserted,[&](){
        ui->noteCntLabel->setText(QStringLiteral("%1").arg(m_proxyNoteModel->rowCount()));
    });
    connect(m_proxyNoteModel, &QSortFilterProxyModel::rowsRemoved,[&](){
        ui->noteCntLabel->setText(QStringLiteral("%1").arg(m_proxyNoteModel->rowCount()));
    });
    connect(m_proxyNoteModel, &QSortFilterProxyModel::modelReset,[&](){
        ui->noteCntLabel->setText(QStringLiteral("%1").arg(m_proxyNoteModel->rowCount()));
    });
    // note model rows moved
    connect(m_noteModel, &NoteModel::rowsAboutToBeMoved, m_noteView, &NoteView::rowsAboutToBeMoved);
    connect(m_noteModel, &NoteModel::rowsMoved, m_noteView, &NoteView::rowsMoved);
    // folder Model data changed / rows inserted / rows removed
    connect(m_folderModel, &FolderModel::dataChanged,[&](const QModelIndex& topLeft){saveFolderToDB(topLeft);});
    connect(m_folderModel, &FolderModel::rowsInserted, this, &MainWindow::onFolderModelRowsInserted);
    connect(m_folderModel, &FolderModel::rowsRemoved, m_folderView, &QTreeView::adjustSize);
    // tag Model / data changed / rows about to be removed / rows removed / rows inserted
    connect(m_tagModel, &TagModel::dataChanged, this, &MainWindow::onTagModelDataChanged);
    connect(m_tagModel, &TagModel::rowsAboutToBeRemoved, this, &MainWindow::onTagModelRowsAboutToBeRemoved);
    connect(m_tagModel, &TagModel::rowsRemoved, this, &MainWindow::onTagModelRowsRemoved);
    connect(m_tagModel, &TagModel::rowsInserted, this, &MainWindow::onTagModelRowsInserted);
    connect(m_noteModel, &NoteModel::dataChanged, this, &MainWindow::onNoteDataChanged);
    // auto save timer
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::onTextEditTimeoutTriggered);
    // clear button
    connect(m_clearButton, &QToolButton::clicked, this, &MainWindow::onClearButtonClicked);
    // Restore Notes Action
    connect(m_trayRestoreAction, &QAction::triggered, this, [this](){
        setMainWindowVisibility(isHidden()
                                || windowState() == Qt::WindowMinimized
                                || (qApp->applicationState() == Qt::ApplicationInactive));
    });
    // Quit Action
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::QuitApplication);
    // Application state changed
    connect(qApp, &QApplication::applicationStateChanged, this,[this](){
        m_noteView->update(m_noteView->currentIndex());
    });
}

/**
* @brief
* Set the lineedit to start a bit to the right and end a bit to the left (pedding)
*/
void MainWindow::setupLineEdit ()
{
    // clear button
    m_clearButton = new QToolButton(m_lineEdit);
    QPixmap pixmap(":images/closeButton.png");
    m_clearButton->setIcon(QIcon(pixmap));
    QSize clearSize(15, 15);
    m_clearButton->setIconSize(clearSize);
    m_clearButton->setCursor(Qt::ArrowCursor);
    m_clearButton->hide();

    // search button
    QToolButton *searchButton = new QToolButton(m_lineEdit);
    QPixmap newPixmap(":images/magnifyingGlass.png");
    searchButton->setIcon(QIcon(newPixmap));
    QSize searchSize(24, 25);
    searchButton->setIconSize(searchSize);
    searchButton->setCursor(Qt::ArrowCursor);

    // layout
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::RightToLeft, m_lineEdit);
    layout->setContentsMargins(0,0,3,0);
    layout->addWidget(m_clearButton);
    layout->addStretch();
    layout->addWidget(searchButton);
    m_lineEdit->setLayout(layout);
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
    m_noteCounter = m_dbManager->getNotesLastRowID();
    m_folderCounter = m_dbManager->getFoldersLastRowID();
    m_tagCounter = m_dbManager->getTagsLastRowID();
}

void MainWindow::setupModelView()
{
    m_noteView = static_cast<NoteView*>(ui->listView);
    m_proxyNoteModel->setSourceModel(m_noteModel);
    m_proxyNoteModel->setFilterKeyColumn(0);
    m_proxyNoteModel->setFilterRole(NoteModel::NoteContent);
    m_proxyNoteModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_noteView->setItemDelegate(new NoteWidgetDelegate(m_noteView));
    m_noteView->setModel(m_proxyNoteModel);

    m_folderView->setModel(m_folderModel);
    m_folderView->setItemDelegate(new FolderWidgetDelegate(m_folderView));
    m_folderView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_tagView->setModel(m_tagModel);
    m_tagView->setItemDelegate(new TagWidgetDelegate(m_tagView));
    m_tagView->setContextMenuPolicy(Qt::CustomContextMenu);

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
* @brief
* Get a string 'str' and return only the first line of it
* If the string contain no text, return "New Note"
* TODO: We might make it more efficient by not loading the entire string into the memory
*/
QString MainWindow::getFirstLine (const QString& str)
{
    if(str.simplified().isEmpty())
        return "New Note";

    QString text = str.trimmed();
    QTextStream ts(&text);
    return std::move(ts.readLine(FIRST_LINE_MAX));
}

/**
* @brief
* Get a date string of a note from database and put it's data into a QDateTime object
* This function is not efficient
* If QVariant would include toStdString() it might help, aka: notesDatabase->value().toStdString
*/
QDateTime MainWindow::getQDateTime (QString date)
{
    QDateTime dateTime = QDateTime::fromString(date, Qt::ISODate);
    return dateTime;
}

/**
* @brief
* Get the full date of the selected note from the database and return it as a string in form for editorDateLabel
*/
QString MainWindow::getNoteDateEditor (QString dateEdited)
{
    QDateTime dateTimeEdited(getQDateTime(dateEdited));
    QLocale usLocale(QLocale("en_US"));

    return usLocale.toString(dateTimeEdited, "MMMM d, yyyy, h:mm A");
}

/**
* @brief
* @brief generate a new note
*/
NoteData *MainWindow::generateNote(int id)
{
    NoteData* newNote = new NoteData(this);
    newNote->setId(id);

    QDateTime noteDate = QDateTime::currentDateTime();
    newNote->setCreationDateTime(noteDate);
    newNote->setLastModificationDateTime(noteDate);
    newNote->setFullTitle(QStringLiteral("New Note"));
    newNote->setFullPath(m_currentFolderPath);

    return newNote;
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
    QString noteDateEditor = getNoteDateEditor(noteDate);
    m_editorDateLabel->setText(noteDateEditor);
    // set scrollbar position
    m_textEdit->verticalScrollBar()->setValue(scrollbarPos);
    m_textEdit->blockSignals(false);
}


void MainWindow::initFolders ()
{
    // get Folder list
    QList<FolderData *> folderDataList = m_dbManager->getAllFolders();
    // push folders to the treeview
    m_folderModel->setupModelData(folderDataList);

    // create new Folder if no folder exists
    if(folderDataList.isEmpty()){
        ++m_folderCounter;
        FolderData* folderData = new FolderData;
        folderData->setId(m_folderCounter);
        folderData->setName(QStringLiteral("Folder0"));
        FolderItem* folderItem = new FolderItem(folderData, this);
        m_folderModel->insertFolder(folderItem, 0);

        m_dbManager->addFolder(folderData);
    }

    m_folderView->setCurrentIndex(QModelIndex());
}

void MainWindow::initTags()
{
    QList<TagData*> tagList = m_dbManager->getAllTags();
    m_tagModel->addTags(tagList);
}

void MainWindow::fillNoteModel(QList<NoteData*> noteList)
{
    if(!noteList.isEmpty()){
        m_noteModel->addListNote(noteList);
        m_noteModel->sort();

        // add tags indexes to each correspondent note
        foreach(NoteData* note, noteList){
            QString tagStr = note->tagIdSerial();
            if(!tagStr.isEmpty()){
                foreach (QString tagIDStr, tagStr.split(TagData::TagSeparator)) {
                    int id = tagIDStr.toInt();
                    QModelIndex index = m_tagModel->indexFromId(id);
                    m_noteModel->addTagIndex(note->id(), index);
                }
            }
        }

        selectFirstNote();
    }
}

/**
* @brief
* save the current note to database
*/
void MainWindow::saveNoteToDB(const QModelIndex &noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "MainWindow::saveNoteToDB", "noteIndex is not valid");
    Q_ASSERT_X(noteIndex.model() == m_proxyNoteModel, "MainWindow::saveNoteToDB", "noteIndex must be from ProxyModel");

    if(!m_isTemp && m_isContentModified){
        m_isContentModified = false;
        QModelIndex indexInSrc = m_proxyNoteModel->mapToSource(noteIndex);
        NoteData* note = m_noteModel->getNote(indexInSrc);
        if(note != Q_NULLPTR){
            bool doExist = m_dbManager->noteExist(note);
            if(doExist){
                QtConcurrent::run(m_dbManager, &DBManager::modifyNote, note);
            }else{
                QtConcurrent::run(m_dbManager, &DBManager::addNote, note);
            }
        }
    }
}

void MainWindow::saveFolderToDB(const QModelIndex& folderIndex)
{
    Q_ASSERT_X(folderIndex.isValid(), "MainWindow::saveFolderToDB", "noteIndex is not valid");

    const FolderData* folderData = m_folderModel->folderData(folderIndex);
    if(folderData != Q_NULLPTR)
        QtConcurrent::run(m_dbManager, &DBManager::modifyFolder, folderData);
}

void MainWindow::saveTagToDB(const QModelIndex& tagIndex)
{
    Q_ASSERT_X(tagIndex.isValid(), "MainWindow::saveFolderToDB", "noteIndex is not valid");

    const TagData* tagData = m_tagModel->tagData(tagIndex);

    if(tagData != Q_NULLPTR)
        QtConcurrent::run(m_dbManager, &DBManager::modifyTag, tagData);
}

void MainWindow::removeNoteFromDB(const QModelIndex& noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "MainWindow::removeNoteFromDB", "noteIndex is not valid");

    if(noteIndex.isValid()){
        QModelIndex indexInSrc = m_proxyNoteModel->mapToSource(noteIndex);
        NoteData* note = m_noteModel->getNote(indexInSrc);
        m_dbManager->removeNote(note);
    }
}

/**
* @brief
* Select the first note in the notes list
*/
void MainWindow::selectFirstNote ()
{
    if(m_proxyNoteModel->rowCount() > 0){
        QModelIndex index = m_proxyNoteModel->index(0,0);
        m_noteView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        m_noteView->setCurrentIndex(index);

        m_currentSelectedNoteProxy = index;
        showNoteInEditor(index);

        if(m_lineEdit->text().isEmpty())
            m_noteView->setFocus();
    }else{
        m_currentSelectedNoteProxy = QModelIndex();
    }
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

/**
* @brief
* create a new note if there are no notes
*/
void MainWindow::createNewNoteIfEmpty ()
{
    if(m_proxyNoteModel->rowCount() == 0)
        createNewNote();
}

void MainWindow::setButtonsAndFieldsEnabled(bool doEnable)
{
    m_greenMaximizeButton->setEnabled(doEnable);
    m_redCloseButton->setEnabled(doEnable);
    m_yellowMinimizeButton->setEnabled(doEnable);
    m_addNoteButton->setEnabled(doEnable);
    m_deleteNoteButton->setEnabled(doEnable);
    m_lineEdit->setEnabled(doEnable);
    m_textEdit->setEnabled(doEnable);
}

/**
* @brief
* Create a new note when clicking the 'new note' button
*/
void MainWindow::onAddNoteButtonClicked()
{
    if(m_isAddingNoteEnabled){
        m_noteView->setAnimationEnabled(false);
        if(!m_lineEdit->text().isEmpty()){
            clearSearchAndText();
            m_selectedNoteBeforeSearchingInSource = QModelIndex();
        }else if(m_tagView->selectionModel()->selectedIndexes().count() > 0){
            clearTagSelection();
        }
        m_noteView->setAnimationEnabled(true);

        // save the data of the previous selected
        if(m_currentSelectedNoteProxy.isValid())
            saveNoteToDB(m_currentSelectedNoteProxy);

        createNewNote();
    }
}

void MainWindow::onAddFolderButtonClicked()
{
    QModelIndex index = addNewFolder();
    m_folderView->edit(index);
}

void MainWindow::onAddTagButtonClicked()
{
    QModelIndex index = addNewTag();
    qApp->processEvents();

    QPoint p = m_tagView->visualRect(index).bottomLeft();
    p = m_tagView->mapTo(ui->scrollAreaWidgetContents, p);
    ui->scrollAreaFolderTag->ensureVisible(p.x(), p.y());

    m_tagView->edit(index);
}

/**
* @brief
* Delete selected note when clicking the 'delete note' button
*/
void MainWindow::onDeleteNoteButtonClicked()
{
    m_deleteNoteButton->blockSignals(true);
    this->deleteSelectedNote();
    m_deleteNoteButton->blockSignals(false);
}

QModelIndex MainWindow::addNewFolder(QModelIndex index)
{
    // getting folder data for the new child folder
    // building the item and insert it to the model
    int row = m_folderModel->rowCount(index);
    QString folderName = QStringLiteral("Folder%1").arg(row);
    QString parentPath = m_folderModel->data(index, (int) FolderItem::FolderDataEnum::FullPath).toString();

    ++m_folderCounter;
    FolderData* folderData = new FolderData;
    folderData->setName(folderName);
    folderData->setParentPath(parentPath);
    folderData->setId(m_folderCounter);

    // save the new folder to the database and set id
    QtConcurrent::run(m_dbManager, &DBManager::addFolder, folderData);

    // create folderItem and insert it to the model
    FolderItem* folderItem = new FolderItem(folderData, this);
    m_folderModel->insertFolder(folderItem, row, index);

    m_folderView->expand(index);

    // Set the current index to the newly crealted folder
    QModelIndex newFolderIndex = m_folderModel->index(row,0,index);
    m_folderView->selectionModel()->setCurrentIndex(newFolderIndex, QItemSelectionModel::ClearAndSelect);

    return newFolderIndex;
}

void MainWindow::onNoteClicked (const QModelIndex& index)
{
    if(sender() != Q_NULLPTR){
        QModelIndex indexInProxy = m_proxyNoteModel->index(index.row(), 0);
        selectNote(indexInProxy);
        m_noteView->setFocus();
    }
}

void MainWindow::onFolderSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)

    m_currentFolderPath.clear();

    // initialize
    m_noteView->setAnimationEnabled(false);
    clearSearchAndText();
    clearTagSelection();
    m_noteView->setAnimationEnabled(true);

    m_noteModel->clearNotes();
    m_currentSelectedNoteProxy = QModelIndex();
    m_selectedNoteBeforeSearchingInSource = QModelIndex();
    m_isTemp = false;
    m_isContentModified = false;

    m_folderView->setFocus();

    if(!selected.indexes().isEmpty()){

        // enable Add/delete/edit of notes
        setAddingNoteEnabled(true);
        setNoteDeletionEnabled(true);
        setNoteEditabled(true);

        QModelIndex selectedFolderIndex = selected.indexes().at(0);

        // clear the selection in the All Notes/ Trash listWidget
        m_generalListW->clearSelection();
        m_generalListW->setCurrentRow(-1);

        // update the current Folder Path
        m_currentFolderPath = m_folderModel->data(selectedFolderIndex,
                                                  (int) FolderItem::FolderDataEnum::FullPath).toString();

        // check the number of notes contained in the current selected folder
        // if the folder contains notes, grab them from database
        int noteCnt =  m_folderModel->data(selectedFolderIndex,
                                           (int) FolderItem::FolderDataEnum::NoteCount).toInt();

        if(noteCnt > 0){
            // get all notes contained in the selected path
            QList<NoteData*> noteList = m_dbManager->getAllNotes(m_currentFolderPath);
            fillNoteModel(noteList);
        }
    }
}

void MainWindow::onGeneralListWCurrentRowChanged(int currentRow)
{
    m_isNoteEditable = true;

    if(currentRow!=-1){
        m_folderView->setCurrentIndex(QModelIndex());

        // initialize
        m_noteView->setAnimationEnabled(false);
        clearSearchAndText();
        clearTagSelection();
        m_noteView->setAnimationEnabled(true);

        m_noteModel->clearNotes();
        m_currentSelectedNoteProxy = QModelIndex();
        m_selectedNoteBeforeSearchingInSource = QModelIndex();
        m_isTemp = false;
        m_isContentModified = false;

        m_noteView->setAnimationEnabled(false);

        setAddingNoteEnabled(false);
        setNoteDeletionEnabled(true);

        if(currentRow == 0){ // All Notes
            QList<NoteData*> noteList = m_dbManager->getAllNotes();
            // add notes to the model
            fillNoteModel(noteList);

            if(noteList.isEmpty()){
                m_noteView->setFocus();
                setNoteEditabled(false);
            }else{
                setNoteEditabled(true);
            }

        }else if(currentRow == 1){ // Trash

            setAddingNoteEnabled(false);
            setNoteDeletionEnabled(false);
            setNoteEditabled(false);

            QList<NoteData*> trashNotes = m_dbManager->getNotesInTrash();
            // add notes to the model
            fillNoteModel(trashNotes);
        }

        m_noteView->setAnimationEnabled(true);
    }
}

/**
* @brief
* When the text on textEdit change:
* if the note edited is not on top of the list, we will make that happen
* If the text changed is of a new (empty) note, reset temp values
*/
void MainWindow::onTextEditTextChanged ()
{
    Q_ASSERT_X(m_currentSelectedNoteProxy.isValid(), "MainWindow::onTextEditTextChanged", "m_currentSelectedNoteProxy is not valid");

    m_textEdit->blockSignals(true);

    // if it's a new note, incremente the number of notes in the correspondent folder
    if(m_isTemp){
        m_isTemp = false;

        // update the number of note contained in the folder
        int noteCnt = m_noteModel->rowCount();
        QModelIndex folderIndex = m_folderView->selectionModel()->currentIndex();
        m_folderModel->setData(folderIndex, QVariant::fromValue(noteCnt), (int) FolderItem::FolderDataEnum::NoteCount);
    }

    QString content = m_currentSelectedNoteProxy.data(NoteModel::NoteContent).toString();
    if(m_textEdit->toPlainText() != content){
        m_isContentModified = true;
        // start/restart the timer
        m_autoSaveTimer->start(500);
        // move note to the top of the list
        if(m_currentSelectedNoteProxy.row() != 0)
            moveNoteToTop();
    }

    m_textEdit->blockSignals(false);
}

void MainWindow::onTextEditTimeoutTriggered()
{
    m_autoSaveTimer->stop();
    // Get the new data
    QString firstline = getFirstLine(m_textEdit->toPlainText());
    QDateTime dateTime = QDateTime::currentDateTime();
    QString noteDate = dateTime.toString(Qt::ISODate);
    m_editorDateLabel->setText(getNoteDateEditor(noteDate));

    // update model
    QMap<int, QVariant> dataValue;
    dataValue[NoteModel::NoteContent] = QVariant::fromValue(m_textEdit->toPlainText());
    dataValue[NoteModel::NoteFullTitle] = QVariant::fromValue(firstline);
    dataValue[NoteModel::NoteLastModificationDateTime] = QVariant::fromValue(dateTime);

    QModelIndex index = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
    m_noteModel->setItemData(index, dataValue);
}

/**
* @brief
* When text on lineEdit change:
* If there is a temp note "New Note" while searching, we delete it
* Saving the last selected note for recovery after searching
* Clear all the notes from scrollArea and
* If text is empty, reload all the notes from database
* Else, load all the notes contain the string in lineEdit from database
*/
void MainWindow::onLineEditTextChanged (const QString &keyword)
{
    m_textEdit->clearFocus();
    m_searchQueue.enqueue(keyword);

    m_noteView->setAnimationEnabled(false);
    clearTagSelection();
    m_noteView->setAnimationEnabled(true);

    if(!m_isOperationRunning){
        m_isOperationRunning = true;

        // disable the animation
        m_noteView->setAnimationEnabled(false);

        if(m_isTemp){
            m_isTemp = false;
            // prevent the line edit from emitting signal
            // while animation for deleting the new note is running
            m_lineEdit->blockSignals(true);
            m_currentSelectedNoteProxy = QModelIndex();
            QModelIndex index = m_noteModel->index(0);
            m_noteModel->removeNote(index);
            m_lineEdit->blockSignals(false);

            if(m_noteModel->rowCount() > 0){
                m_selectedNoteBeforeSearchingInSource = m_noteModel->index(0);
            }else{
                m_selectedNoteBeforeSearchingInSource = QModelIndex();
            }

        }else if(!m_selectedNoteBeforeSearchingInSource.isValid()
                 && m_currentSelectedNoteProxy.isValid()){

            m_selectedNoteBeforeSearchingInSource = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
        }

        if(m_currentSelectedNoteProxy.isValid()
                && m_isContentModified){

            saveNoteToDB(m_currentSelectedNoteProxy);
        }

        while(!m_searchQueue.isEmpty()){
            qApp->processEvents();
            QString str = m_searchQueue.dequeue();
            if(str.isEmpty()){
                clearSearchAndText();
                QModelIndex indexInProxy = m_proxyNoteModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
                selectNote(indexInProxy);
                m_selectedNoteBeforeSearchingInSource = QModelIndex();
            }else{
                findNotesContaining(str);
            }
        }

        m_noteView->setAnimationEnabled(true);

        m_isOperationRunning = false;
    }
}

void MainWindow::onFolderModelRowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(last)

    m_folderView->adjustSize();
}

void MainWindow::onTagSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if(m_isTemp){
        m_isTemp = false;
        m_currentSelectedNoteProxy = QModelIndex();
        QModelIndex index = m_noteModel->index(0);
        m_noteModel->removeNote(index);
    }

    m_noteView->setAnimationEnabled(false);
    clearTextAndHeader();
    if(!m_lineEdit->text().isEmpty())
        clearSearch();

    m_proxyNoteModel->setFilterRole(NoteModel::NoteTagSerial);

    QString regexp = m_proxyNoteModel->filterRegExp().pattern();

    QModelIndexList indexes = deselected.indexes();
    foreach (QModelIndex index, indexes) {
        int idToRemove = index.data(TagModel::TagID).toInt();
        QString regExpToBeRemoved = QStringLiteral("(?=.*((^(%1)\\b)|^(%1_)|_%1_|(_%1)\\b))").arg(idToRemove);
        regexp.replace(regExpToBeRemoved,"");
    }

    if(selected.indexes().count() > 0){
        int idToAdd = selected.indexes().at(0).data(TagModel::TagID).toInt();
        QString regExpToBeAdded = QStringLiteral("(?=.*((^(%1)\\b)|^(%1_)|_%1_|(_%1)\\b))").arg(idToAdd);
        regexp.append(regExpToBeAdded);
    }

    m_proxyNoteModel->setFilterRegExp(regexp);

    selectFirstNote();
    m_noteView->setAnimationEnabled(true);
}

void MainWindow::onTagModelRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(last)
    QModelIndex removedTagIndex = m_tagModel->index(first);
    m_noteModel->removeTagIndex(removedTagIndex);
}

void MainWindow::onTagModelRowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    int height =  m_tagView->sizeHintForIndex(m_tagModel->index(0)).height() * m_tagModel->rowCount();
    m_tagView->setFixedHeight(height);
    ui->scrollAreaWidgetContents->updateGeometry();
}

void MainWindow::onTagModelRowsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
    if(m_tagModel->rowCount() > 0){
            int rowHeight = m_tagView->sizeHintForIndex(m_tagModel->index(0)).height();
            int height =  rowHeight * m_tagModel->rowCount();
            m_tagView->setFixedHeight(height);
    }else{
        m_tagView->setFixedHeight(0);
    }
}

void MainWindow::onTagModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    m_noteView->update();
    saveTagToDB(topLeft);
}

void MainWindow::onClearTagSelectionButtonClicked()
{
    m_tagView->clearSelection();
    m_tagView->setCurrentIndex(QModelIndex());
}

void MainWindow::onNoteDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(bottomRight)

    m_isContentModified = true;
    QModelIndex indexInProxy = m_proxyNoteModel->mapFromSource(topLeft);
    saveNoteToDB(indexInProxy);

    if(roles.contains(NoteModel::NoteTagIndexList)){
        int noteId = indexInProxy.data(NoteModel::NoteID).toInt();
        QList<QPersistentModelIndex> tagIndexes = indexInProxy.data(NoteModel::NoteTagIndexList).value<QList<QPersistentModelIndex>>();
        m_tagModel->updateNoteInTags(tagIndexes, noteId);
    }
}

/**
 * @brief MainWindow::onClearButtonClicked clears the search and
 * select the note that was selected before searching if it is still valid.
 */
void MainWindow::onClearButtonClicked()
{
    if(!m_isOperationRunning){

        clearSearchAndText();

        if(m_noteModel->rowCount() > 0){
            QModelIndex indexInProxy = m_proxyNoteModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
            int row = m_selectedNoteBeforeSearchingInSource.row();
            if(row == m_noteModel->rowCount())
                indexInProxy = m_proxyNoteModel->index(m_proxyNoteModel->rowCount()-1,0);

            selectNote(indexInProxy);
        }else{
            m_currentSelectedNoteProxy = QModelIndex();
        }

        m_selectedNoteBeforeSearchingInSource = QModelIndex();

    }
}

/**
 * @brief create a new note
 * add it to the database
 * add it to the scrollArea
 */
void MainWindow::createNewNote ()
{
    if(m_isAddingNoteEnabled){
        if(!m_isOperationRunning){
            m_isOperationRunning = true;

            m_noteView->scrollToTop();

            clearTextAndHeader();

            if(!m_isTemp){
                ++m_noteCounter;
                NoteData* tmpNote = generateNote(m_noteCounter);
                m_isTemp = true;

                // insert the new note to NoteModel
                QModelIndex indexSrc = m_noteModel->insertNote(tmpNote, 0);

                // update the editor header date label
                QString dateTimeFromDB = tmpNote->lastModificationdateTime().toString(Qt::ISODate);
                QString dateTimeForEditor = getNoteDateEditor(dateTimeFromDB);
                m_editorDateLabel->setText(dateTimeForEditor);

                // update the current selected index
                m_currentSelectedNoteProxy = m_proxyNoteModel->mapFromSource(indexSrc);

            }else{
                int row = m_currentSelectedNoteProxy.row();
                m_noteView->animateAddedRow(QModelIndex(),row, row);
            }

            m_textEdit->setFocus();
            m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
            m_isOperationRunning = false;
        }
    }
}

/**
 * @brief MainWindow::deleteNote deletes the specified note
 * @param note  : note to delete
 * @param isFromUser :  true if the user clicked on trash button
 */
void MainWindow::deleteNote(const QModelIndex &noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "MainWindow::deleteNote", "noteIndex is not valid");

    // delete from model
    QModelIndex indexToBeRemoved = m_proxyNoteModel->mapToSource(noteIndex);
    NoteData* noteTobeRemoved = m_noteModel->removeNote(indexToBeRemoved);

    if(m_isTemp){
        m_isTemp = false;
    }else{
        // remove from database
        noteTobeRemoved->setDeletionDateTime(QDateTime::currentDateTime());
        QtConcurrent::run(m_dbManager, &DBManager::removeNote, noteTobeRemoved);

        // decrement the number of note stored in the current folder and saving to database
        int noteCnt = m_noteModel->rowCount();
        QModelIndex folderIndex = m_folderView->selectionModel()->currentIndex();
        m_folderModel->setData(folderIndex, QVariant::fromValue(noteCnt), (int) FolderItem::FolderDataEnum::NoteCount);
    }

    if(m_currentFolderPath.isEmpty() && m_noteModel->rowCount() == 0)
        setNoteEditabled(false);

    m_noteView->setFocus();
}

void MainWindow::deleteSelectedNote ()
{
    if(m_isNoteDeletionEnabled){
        if(!m_isOperationRunning){
            m_isOperationRunning = true;

            if(m_currentSelectedNoteProxy.isValid()){

                // update the index of the selected note before searching
                if(!m_lineEdit->text().isEmpty()){
                    QModelIndex currentIndexInSource = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
                    int beforeSearchSelectedRow = m_selectedNoteBeforeSearchingInSource.row();
                    if(currentIndexInSource.row() < beforeSearchSelectedRow){
                        m_selectedNoteBeforeSearchingInSource = m_noteModel->index(beforeSearchSelectedRow-1);
                    }
                }

                // clear text edit and time date label
                clearTextAndHeader();

                // delete the note
                deleteNote(m_currentSelectedNoteProxy);

                // update the the current selected note
                m_currentSelectedNoteProxy = m_noteView->currentIndex();
                if(m_currentSelectedNoteProxy.isValid())
                    showNoteInEditor(m_currentSelectedNoteProxy);
            }
            m_isOperationRunning = false;
        }
    }
}

/**
* @brief
* Set focus on textEdit
*/
void MainWindow::setFocusOnText ()
{
    if(m_currentSelectedNoteProxy.isValid() && !m_textEdit->hasFocus())
        m_textEdit->setFocus();
}

/**
* @brief
* Set focus on current selected note
*/
void MainWindow::setFocusOnCurrentNote ()
{
    if(m_currentSelectedNoteProxy.isValid())
        m_noteView->setFocus();
}

/**
* @brief
* Select the note above the currentSelectedNote
*/
void MainWindow::selectNoteUp ()
{
    if(m_currentSelectedNoteProxy.isValid()){
        int currentRow = m_noteView->currentIndex().row();
        QModelIndex aboveIndex = m_noteView->model()->index(currentRow - 1, 0);
        if(aboveIndex.isValid()){
            m_noteView->setCurrentIndex(aboveIndex);
            m_currentSelectedNoteProxy = aboveIndex;
            showNoteInEditor(m_currentSelectedNoteProxy);
        }
        m_noteView->setFocus();
    }
}

void MainWindow::showTagNoteMenu()
{
    if(m_noteModel->rowCount() >0
            && !m_isTemp
            && m_currentSelectedNoteProxy.isValid()){

        QMenu menu;
        menu.setObjectName("tagMenu");

        TagNoteModel tagNoteModel;
        tagNoteModel.setTagModel(m_tagModel);
        QModelIndex indexSrc = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
        QList<QPersistentModelIndex> tagIndexes = indexSrc.data(NoteModel::NoteTagIndexList).value<QList<QPersistentModelIndex>>();
        foreach (QPersistentModelIndex index, tagIndexes)
            tagNoteModel.setData(index, Qt::Checked, Qt::CheckStateRole);

        QWidgetAction tagviewWidgetAction(&menu);
        QListView listView;
        listView.setObjectName("tagNoteView");
        listView.setModel(&tagNoteModel);
        int listViewMaxHeight = 24*7+4;
        int listViewHeight = (24*tagNoteModel.rowCount()) + 4;
        listViewHeight = tagNoteModel.rowCount() < 8 ? listViewHeight : listViewMaxHeight;
        listView.setFixedHeight(listViewHeight);
        listView.setFrameShape(QFrame::NoFrame);
        listView.setSelectionMode(QAbstractItemView::NoSelection);
        tagviewWidgetAction.setDefaultWidget(&listView);
        menu.addAction(&tagviewWidgetAction);

        QWidgetAction addtagNoteWidgetAction(&menu);
        QPushButton addTagPb;
        addTagPb.setObjectName(QStringLiteral("createTagButton"));
        addTagPb.setText("Create new tag");
        addTagPb.setFlat(true);
        addtagNoteWidgetAction.setDefaultWidget(&addTagPb);
        menu.addAction(&addtagNoteWidgetAction);

        QWidgetAction validateWidgetAction(&menu);
        QPushButton validatePb;
        validatePb.setObjectName(QStringLiteral("validateButton"));
        validatePb.setText("Validate");
        validatePb.setFlat(true);
        validateWidgetAction.setDefaultWidget(&validatePb);
        menu.addAction(&validateWidgetAction);

        // check / uncheck when the item is clicked
        connect(&listView, &QListView::pressed, [&](const QModelIndex& index){
            bool isChecked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
            if(isChecked){
                tagNoteModel.setData(index, Qt::Unchecked, Qt::CheckStateRole);
            }else{
                tagNoteModel.setData(index, Qt::Checked, Qt::CheckStateRole);
            }
        });

        connect(&addTagPb, &QPushButton::clicked, [&](){
            menu.hide();
            // TODO: show dialog to add tag
        });

        // update the tags for the selected note
        connect(&validatePb, &QPushButton::clicked, [&](){
            QList<QPersistentModelIndex> indexes;
            // store the checked tags to the notes
            for(int i=0; i<tagNoteModel.rowCount(); i++){
                QModelIndex index = tagNoteModel.index(i);
                bool isChecked = index.data(Qt::CheckStateRole).toBool();
                if(isChecked){
                    int tagId = index.data(TagModel::TagID).toInt();
                    QPersistentModelIndex tagModelIndex = m_tagModel->indexFromId(tagId);
                    indexes << tagModelIndex;
                }
            }

            m_noteModel->setData(indexSrc, QVariant::fromValue(indexes), NoteModel::NoteTagIndexList);
            menu.hide();
        });

        QPoint gPos = m_tagNoteButton->parentWidget()->mapToGlobal(m_tagNoteButton->geometry().bottomLeft());
        menu.exec(gPos);
    }
}

void MainWindow::showTagViewContextMenu(const QPoint& pos)
{
    QMenu menu;
    menu.setObjectName(QStringLiteral("tagViewContextMenu"));
    menu.show();
    qApp->processEvents();

    int contentWidth = menu.contentsRect().width() - 2*5;

    QWidgetAction changeTagColorWidgetAction(&menu);
    QPushButton changeTagColorPb;
    changeTagColorPb.setEnabled(m_tagModel->rowCount() > 0);
    changeTagColorPb.setObjectName(QStringLiteral("changeColorTagMenuButton"));
    changeTagColorPb.setText(tr("Change color"));
    changeTagColorPb.setFlat(true);
    changeTagColorPb.setFixedWidth(contentWidth);
    changeTagColorWidgetAction.setDefaultWidget(&changeTagColorPb);
    menu.addAction(&changeTagColorWidgetAction);

    QWidgetAction deleteTagWidgetAction(&menu);
    QPushButton deleteTagPb;
    deleteTagPb.setEnabled(m_tagModel->rowCount() > 0);
    deleteTagPb.setObjectName(QStringLiteral("DeleteTagMenuButton"));
    deleteTagPb.setText(tr("Delete tag"));
    deleteTagPb.setFlat(true);
    deleteTagPb.setEnabled(m_tagView->indexAt(pos).isValid());
    deleteTagPb.setFixedWidth(contentWidth);
    deleteTagWidgetAction.setDefaultWidget(&deleteTagPb);
    menu.addAction(&deleteTagWidgetAction);

    menu.addSeparator();

    QWidgetAction clearSelectionWidgetAction(&menu);
    QPushButton clearTagSelectionPb;
    clearTagSelectionPb.setObjectName(QStringLiteral("clearTagSelectionMenuButton"));
    clearTagSelectionPb.setText(tr("Clear selection"));
    clearTagSelectionPb.setFlat(true);
    clearTagSelectionPb.setEnabled(m_tagView->selectionModel()->selectedIndexes().count() > 0);
    clearTagSelectionPb.setFixedWidth(contentWidth);
    clearSelectionWidgetAction.setDefaultWidget(&clearTagSelectionPb);
    menu.addAction(&clearSelectionWidgetAction);

    QWidgetAction addtagWidgetAction(&menu);
    QPushButton addTagPb;
    addTagPb.setObjectName(QStringLiteral("createTagMenuButton"));
    addTagPb.setText(tr("Add new tag"));
    addTagPb.setFlat(true);
    addTagPb.setFixedWidth(contentWidth);
    addtagWidgetAction.setDefaultWidget(&addTagPb);
    menu.addAction(&addtagWidgetAction);

    connect(&changeTagColorPb, &QPushButton::clicked, [&](){
        QModelIndex index = m_tagView->indexAt(pos);
        QColor previousColor = index.data(TagModel::TagColor).value<QColor>();
        QColorDialog d(previousColor);
        int ret = d.exec();
        if(ret > 0){
            QColor color = d.selectedColor();
            if(color != previousColor){
                QVariant v = color;
                m_tagModel->setData(index, v, TagModel::TagColor);
            }
        }
    });

    connect(&deleteTagPb, &QPushButton::clicked, [&](){
        menu.hide();
        QModelIndex index = m_tagView->indexAt(pos);
        deleteTag(index);
    });

    connect(&clearTagSelectionPb, &QPushButton::clicked, [&](){
        menu.hide();
        m_tagView->clearSelection();
        m_tagView->setCurrentIndex(QModelIndex());
    });

    connect(&addTagPb, &QPushButton::clicked, [&](){
        menu.hide();
        QModelIndex index = addNewTag();
        qApp->processEvents();

        QPoint p = m_tagView->visualRect(index).bottomLeft();
        p = m_tagView->mapTo(ui->scrollAreaWidgetContents, p);
        ui->scrollAreaFolderTag->ensureVisible(p.x(), p.y());

        m_tagView->edit(index);
    });

    QPoint gPos = m_tagView->mapToGlobal(pos);
    menu.exec(gPos);
}

void MainWindow::showFolderViewContextMenu(const QPoint& pos)
{
    QMenu menu;
    menu.setObjectName(QStringLiteral("folderViewContextMenu"));
    menu.show();
    qApp->processEvents();

    int contentWidth = menu.contentsRect().width() - 2*5;

    QWidgetAction addSubFolderWidgetAction(&menu);
    QPushButton addSubFolderPb;
    addSubFolderPb.setEnabled(m_tagModel->rowCount() > 0);
    addSubFolderPb.setObjectName(QStringLiteral("addSubFolderMenuButton"));
    addSubFolderPb.setText(tr("Add subfolder"));
    addSubFolderPb.setFlat(true);
    addSubFolderPb.setFixedWidth(contentWidth);
    addSubFolderWidgetAction.setDefaultWidget(&addSubFolderPb);
    menu.addAction(&addSubFolderWidgetAction);

    QWidgetAction deleteFolderWidgetAction(&menu);
    QPushButton deleteFolderPb;
    deleteFolderPb.setEnabled(m_tagModel->rowCount() > 0);
    deleteFolderPb.setObjectName(QStringLiteral("deleteFolderMenuButton"));
    deleteFolderPb.setText(tr("Delete folder"));
    deleteFolderPb.setFlat(true);
    deleteFolderPb.setFixedWidth(contentWidth);
    deleteFolderWidgetAction.setDefaultWidget(&deleteFolderPb);
    menu.addAction(&deleteFolderWidgetAction);

    menu.addSeparator();

    QWidgetAction addFolderWidgetAction(&menu);
    QPushButton addFolderPb;
    addFolderPb.setObjectName(QStringLiteral("addFolderMenuButton"));
    addFolderPb.setText(tr("Add folder"));
    addFolderPb.setFlat(true);
    addFolderPb.setFixedWidth(contentWidth);
    addFolderWidgetAction.setDefaultWidget(&addFolderPb);
    menu.addAction(&addFolderWidgetAction);

    connect(&addSubFolderPb, &QPushButton::clicked, [&](){
        QModelIndex index = m_folderView->indexAt(pos);
        menu.hide();

        if(index.isValid()){
            QModelIndex subFolderIndex = addNewFolder(index);
            m_folderView->edit(subFolderIndex);
        }
    });

    connect(&deleteFolderPb, &QPushButton::clicked, [&](){
        QModelIndex index = m_folderView->indexAt(pos);
        menu.hide();

        if(index.isValid())
            deleteFolder(index);
    });

    connect(&addFolderPb, &QPushButton::clicked, [&](){
        menu.hide();
        QModelIndex index = addNewFolder();
        qApp->processEvents();

        QPoint p = m_folderView->visualRect(index).bottomLeft();
        p = m_folderView->mapTo(ui->scrollAreaWidgetContents, p);
        if(!ui->scrollAreaFolderTag->visibleRegion().contains(p))
            ui->scrollAreaFolderTag->ensureVisible(p.x(), p.y());

        m_folderView->edit(index);
    });

    QPoint gPos = m_folderView->mapToGlobal(pos);
    menu.exec(gPos);
}

/**
* @brief
* Select the note below the currentSelectedNote
*/
void MainWindow::selectNoteDown ()
{
    if(m_currentSelectedNoteProxy.isValid()){
        if(m_isTemp){
            deleteNote(m_currentSelectedNoteProxy);
            m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
            showNoteInEditor(m_currentSelectedNoteProxy);
        }else{
            int currentRow = m_noteView->currentIndex().row();
            QModelIndex belowIndex = m_noteView->model()->index(currentRow + 1, 0);
            if(belowIndex.isValid()){
                m_noteView->setCurrentIndex(belowIndex);
                m_currentSelectedNoteProxy = belowIndex;
                showNoteInEditor(m_currentSelectedNoteProxy);
            }
        }
        m_noteView->setFocus();
    }
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

void MainWindow::deleteFolder(QModelIndex index)
{
    if(m_folderModel->rowCount() > 0){
        if(!index.isValid()){
            m_folderView->setFocus();
            index =  m_folderView->selectionModel()->currentIndex();
        }

        int id = index.data((int) FolderItem::FolderDataEnum::ID).toInt();
        m_folderModel->removeFolder(index.row(), m_folderModel->parent(index));

        QtConcurrent::run(m_dbManager, &DBManager::removeFolder, id);
    }
}

QModelIndex MainWindow::addNewTag()
{
    ++m_tagCounter;
    TagData* tag = new TagData(m_tagModel);
    tag->setId(m_tagCounter);
    int cnt = m_tagModel->rowCount();
    tag->setName(QStringLiteral("Tag%1").arg(cnt));
    tag->setColor(QColor(26,26,26));

    QModelIndex index = m_tagModel->addTag(tag);
    qApp->processEvents();

    QtConcurrent::run(m_dbManager, &DBManager::addTag, tag);

    return index;
}

void MainWindow::deleteTag()
{
    if(m_tagView->selectionModel()->currentIndex().isValid()){
        QModelIndex index = m_tagView->currentIndex();
        TagData* tag = m_tagModel->removeTag(index);
        QtConcurrent::run(m_dbManager, &DBManager::removeTag, tag);
        // TODO: Delete Tag
    }
}

void MainWindow::deleteTag(const QModelIndex index)
{
    Q_ASSERT_X(index.isValid(), "MainWindow::deleteTag", "index is not valid");

    TagData* tag = m_tagModel->removeTag(index);
    QtConcurrent::run(m_dbManager, &DBManager::removeTag, tag);
    // TODO: Delete Tag
}

void MainWindow::deleteTags(const QList<QPersistentModelIndex> indexList)
{
    QList<TagData*> removedTagList = m_tagModel->removeTags(indexList);

    // TODO : emit signal to the db thread
    foreach (TagData* tag, removedTagList) {
        QtConcurrent::run([=](){
            QMutexLocker mutexLocker(&m_mutex);
            m_dbManager->removeTag(tag);
        });
    }
}

void MainWindow::onDeleteTagButtonClicked()
{
    QList<QPersistentModelIndex> persistantIndexes;
    QModelIndexList selectedIndexes = m_tagView->selectionModel()->selectedIndexes();

    foreach (QModelIndex index, selectedIndexes) {
        persistantIndexes << index;
    }

    deleteTags(persistantIndexes);
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

    if(m_currentSelectedNoteProxy.isValid())
        saveNoteToDB(m_currentSelectedNoteProxy);

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

/**
 * @brief MainWindow::moveNoteToTop : moves the current note Widget
 * to the top of the layout
 */
void MainWindow::moveNoteToTop()
{
    // check if the current note is note on the top of the list
    // if true move the note to the top
    if(m_currentSelectedNoteProxy.isValid()
            && m_noteView->currentIndex().row() != 0){

        m_noteView->scrollToTop();

        // move the current selected note to the top
        m_noteModel->moveRow(m_currentSelectedNoteProxy.row(), 0);

        // update the current item
        m_currentSelectedNoteProxy = m_proxyNoteModel->index(0,0);
        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
    }
}

void MainWindow::clearSearchAndText()
{
    m_lineEdit->blockSignals(true);
    m_lineEdit->clear();
    m_lineEdit->blockSignals(false);

    m_textEdit->blockSignals(true);
    m_textEdit->clear();
    m_textEdit->clearFocus();
    m_editorDateLabel->clear();
    m_textEdit->blockSignals(false);

    m_proxyNoteModel->setFilterFixedString(QStringLiteral(""));

    m_clearButton->hide();
    m_lineEdit->setFocus();
}

void MainWindow::clearSearch()
{
    m_lineEdit->blockSignals(true);
    m_lineEdit->clear();
    m_lineEdit->blockSignals(false);

    m_proxyNoteModel->setFilterFixedString(QStringLiteral(""));

    m_clearButton->hide();
    m_lineEdit->setFocus();
}

void MainWindow::clearTagSelection()
{
    m_tagView->selectionModel()->blockSignals(true);

    m_tagView->clearSelection();
    m_tagView->setCurrentIndex(QModelIndex());
    m_tagView->update();
    m_proxyNoteModel->setFilterFixedString("");

    m_tagView->selectionModel()->blockSignals(false);
}

void MainWindow::findNotesContaining(const QString& keyword)
{
    m_proxyNoteModel->setFilterRole(NoteModel::NoteContent);
    m_proxyNoteModel->setFilterFixedString(keyword);
    m_clearButton->show();
    clearTextAndHeader();

    if(m_proxyNoteModel->rowCount() > 0){
        selectFirstNote();
    }else{
        m_currentSelectedNoteProxy = QModelIndex();
    }
}

void MainWindow::selectNote(const QModelIndex &noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "MainWindow::selectNote", "noteIndex is not valid");

    if(noteIndex == m_currentSelectedNoteProxy){
        return;
    }else if(m_currentSelectedNoteProxy.isValid()){
        if(m_isTemp){
            m_isTemp = false;
            // delete the unmodified new note from model
            QModelIndex indexToBeRemoved = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
            m_noteModel->removeNote(indexToBeRemoved);
            // update the current selected note
            m_currentSelectedNoteProxy = m_proxyNoteModel->index(noteIndex.row()-1, 0);

        }else{
            // save the position of scrollbar to the settings
            int pos = m_textEdit->verticalScrollBar()->value();
            QModelIndex indexSrc = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
            int prevPos = m_noteModel->data(indexSrc, NoteModel::NoteScrollbarPos).toInt();
            if(prevPos != pos)
                m_noteModel->setData(indexSrc, QVariant::fromValue(pos), NoteModel::NoteScrollbarPos);
            // save if the previous selected note was modified
            saveNoteToDB(m_currentSelectedNoteProxy);
            // update the current selected note
            m_currentSelectedNoteProxy = noteIndex;
        }
    }else{
        m_currentSelectedNoteProxy = noteIndex;
    }

    // show the content of the pressed note in the text editor
    showNoteInEditor(m_currentSelectedNoteProxy);

    m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
    m_noteView->scrollTo(m_currentSelectedNoteProxy);
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
            m_noteCounter = 0;
        }else{
            m_noteCounter = cntStr.toInt();
        }

        QString createdDateDB = notesIni.value(noteName + "/dateCreated", "Error").toString();
        newNote->setCreationDateTime(QDateTime::fromString(createdDateDB, Qt::ISODate));
        QString lastEditedDateDB = notesIni.value(noteName + "/dateEdited", "Error").toString();
        newNote->setLastModificationDateTime(QDateTime::fromString(lastEditedDateDB, Qt::ISODate));
        QString contentText = notesIni.value(noteName + "/content", "Error").toString();
        newNote->setContent(contentText);
        QString firstLine = getFirstLine(contentText);
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
        QString firstLine = getFirstLine(contentText);
        newNote->setFullTitle(firstLine);

        m_dbManager->migrateTrash(newNote);
        delete newNote;
    }

    QFile oldTrashDBFile(trashPath);
    oldTrashDBFile.rename(QFileInfo(trashPath).dir().path() + "/oldTrash.ini");
}

void MainWindow::setAddingNoteEnabled(bool state)
{
    m_addNoteButton->setEnabled(state);
    m_isAddingNoteEnabled = state;
}

void MainWindow::setNoteDeletionEnabled(bool state)
{
    m_deleteNoteButton->setEnabled(state);
    m_isNoteDeletionEnabled = state;
}

void MainWindow::setNoteEditabled(bool state)
{
    ui->frameRight->setEnabled(state);
    m_tagNoteButton->setEnabled(state);
    m_isNoteEditable = state;
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
            bool isSearching = !m_lineEdit->text().isEmpty();
            if(isSearching)
                m_textEdit->setFocusPolicy(Qt::NoFocus);
        }
        break;
    }
    case QEvent::HoverLeave:{
        bool isNoButtonClicked = qApp->mouseButtons() == Qt::NoButton;
        if(isNoButtonClicked){
            if(object == m_textEdit->verticalScrollBar()){
                m_textEdit->setFocusPolicy(Qt::StrongFocus);
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease:{
        bool isMouseOnScrollBar = qApp->widgetAt(QCursor::pos()) != m_textEdit->verticalScrollBar();
        if(isMouseOnScrollBar){
            if(object == m_textEdit->verticalScrollBar()){
                m_textEdit->setFocusPolicy(Qt::StrongFocus);
            }
        }
        break;
    }
    case QEvent::FocusIn:{
        if(object == m_textEdit){

            if(m_isNoteEditable){
                if(!m_isOperationRunning){

                    m_textEdit->setReadOnly(false);

                    // When clicking in a note's content while searching,
                    // reload all the notes and go and select that note

                    bool isNoteListFilteredByTag = m_tagListView->selectionModel()->selectedRows().count() > 0;
                    bool isNoteListFilteredByKeyword = !m_lineEdit->text().isEmpty();

                    if(isNoteListFilteredByTag || isNoteListFilteredByKeyword){
                        m_selectedNoteBeforeSearchingInSource = QModelIndex();

                        if(m_currentSelectedNoteProxy.isValid()){
                            QModelIndex indexInSource = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);

                            m_noteView->setAnimationEnabled(false);
                            if(isNoteListFilteredByTag) clearTagSelection();
                            if(isNoteListFilteredByKeyword) clearSearch();
                            m_noteView->setAnimationEnabled(true);

                            m_currentSelectedNoteProxy = m_proxyNoteModel->mapFromSource(indexInSource);
                            selectNote(m_currentSelectedNoteProxy);
                            m_textEdit->setFocus();
                        }else{

                            if(isNoteListFilteredByTag) clearTagSelection();
                            if(isNoteListFilteredByKeyword) clearSearch();

                            if(m_isAddingNoteEnabled){
                                if(m_folderModel->rowCount() == 0)
                                    addNewFolder();
                                createNewNote();
                            }

                            selectFirstNote();
                            m_textEdit->setFocus();
                        }

                    }else if(m_proxyNoteModel->rowCount() == 0 && m_isAddingNoteEnabled){
                        if(m_folderModel->rowCount() == 0)
                            addNewFolder();
                        createNewNote();
                    }
                }
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
