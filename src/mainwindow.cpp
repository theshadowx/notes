/**************************************************************************************
* We believe in the power of notes to help us record ideas and thoughts.
* We want people to have an easy, beautiful and simple way of doing that.
* And so we have Notes.
***************************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "notewidgetdelegate.h"
#include "folderwidgetdelegate.h"
#include "qxtglobalshortcut.h"

#include <QScrollBar>
#include <QShortcut>
#include <QTextStream>
#include <QScrollArea>
#include <QtConcurrent>
#include <QProgressDialog>
#include <QGraphicsDropShadowEffect>
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
    m_newNoteButton(Q_NULLPTR),
    m_trashButton(Q_NULLPTR),
    m_newFolderButton(Q_NULLPTR),
    m_newTagButton(Q_NULLPTR),
    m_textEdit(Q_NULLPTR),
    m_lineEdit(Q_NULLPTR),
    m_editorDateLabel(Q_NULLPTR),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_restoreAction(new QAction(tr("&Hide Notes"), this)),
    m_quitAction(new QAction(tr("&Quit"), this)),
    m_trayIconMenu(new QMenu(this)),
    m_folderTreeView(Q_NULLPTR),
    m_tagListW(Q_NULLPTR),
    m_noteModel(new NoteModel(this)),
    m_deletedNotesModel(new NoteModel(this)),
    m_proxyModel(new QSortFilterProxyModel(this)),
    m_folderModel(new FolderModel(this)),
    m_dbManager(Q_NULLPTR),
    m_noteCounter(0),
    m_trashCounter(0),
    m_canMoveWindow(false),
    m_isTemp(false),
    m_isContentModified(false),
    m_isOperationRunning(false)
{
    ui->setupUi(this);
    setupMainWindow();
    setupFonts();
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
        });

        QFuture<void> migration = QtConcurrent::run(this, &MainWindow::checkMigration);
        watcher->setFuture(migration);

    }else{
        initFolders();
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
        m_restoreAction->setText(tr("&Hide Notes"));
    }else{
        m_restoreAction->setText(tr("&Show Notes"));
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
    m_newNoteButton = ui->newNoteButton;
    m_trashButton = ui->trashButton;
    m_lineEdit = ui->lineEdit;
    m_textEdit = ui->textEdit;
    m_editorDateLabel = ui->editorDateLabel;
    m_splitter = ui->splitter;
    m_folderTreeView = ui->folderTree;
    m_tagListW = ui->tagsList;
    m_newFolderButton = ui->newFolderButton;
    m_newTagButton = ui->newTagButton;

    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(248, 248, 248));
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    m_newNoteButton->setToolTip("Create New Note");
    m_trashButton->setToolTip("Delete Selected Note");
}

void MainWindow::setupFonts()
{
    int id = QFontDatabase::addApplicationFont(":/fonts/roboto-hinted/Roboto-Regular.ttf");
    QString robotoFontRegular = QFontDatabase::applicationFontFamilies(id).at(0);

    id = QFontDatabase::addApplicationFont(":/fonts/roboto-hinted/Roboto-Bold.ttf");
    QString robotoFontBold = QFontDatabase::applicationFontFamilies(id).at(0);

    this->setFont(QFont(robotoFontRegular, 10));
    m_lineEdit->setFont(QFont(robotoFontRegular, 10));
    m_editorDateLabel->setFont(QFont(robotoFontBold, 10, QFont::Bold));
}

void MainWindow::setupTrayIcon()
{
    m_trayIconMenu->addAction(m_restoreAction);
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
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_N), this, SLOT(onNewNoteButtonClicked()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Delete), this, SLOT(deleteSelectedNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), m_lineEdit, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_E), m_lineEdit, SLOT(clear()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), this, SLOT(setFocusOnCurrentNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), this, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), this, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::Key_Down), this, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::Key_Up), this, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter), this, SLOT(setFocusOnText()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this, SLOT(setFocusOnText()));
//    new QShortcut(QKeySequence(Qt::Key_Enter), this, SLOT(setFocusOnText()));
//    new QShortcut(QKeySequence(Qt::Key_Return), this, SLOT(setFocusOnText()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F), this, SLOT(fullscreenWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L), this, SLOT(maximizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), this, SLOT(minimizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this, SLOT(QuitApplication()));

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
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
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
    connect(m_greenMaximizeButton, &QPushButton::pressed, this, &MainWindow::onGreenMaximizeButtonPressed);
    connect(m_greenMaximizeButton, &QPushButton::clicked, this, &MainWindow::onGreenMaximizeButtonClicked);
    // red button
    connect(m_redCloseButton, &QPushButton::pressed, this, &MainWindow::onRedCloseButtonPressed);
    connect(m_redCloseButton, &QPushButton::clicked, this, &MainWindow::onRedCloseButtonClicked);
    // yellow button
    connect(m_yellowMinimizeButton, &QPushButton::pressed, this, &MainWindow::onYellowMinimizeButtonPressed);
    connect(m_yellowMinimizeButton, &QPushButton::clicked, this, &MainWindow::onYellowMinimizeButtonClicked);
    // new note button
    connect(m_newNoteButton, &QPushButton::clicked, this, &MainWindow::onNewNoteButtonClicked);
    // new folder button
    connect(m_newFolderButton, &QPushButton::clicked, this, &MainWindow::onNewFolderButtonClicked);
    // delete note button
    connect(m_trashButton, &QPushButton::clicked, this, &MainWindow::onTrashButtonClicked);
    connect(m_noteModel, &NoteModel::rowsRemoved, [this](){m_trashButton->setEnabled(true);});
    // text edit text changed
    connect(m_textEdit, &QTextEdit::textChanged, this, &MainWindow::onTextEditTextChanged);
    // line edit text changed
    connect(m_lineEdit, &QLineEdit::textChanged, this, &MainWindow::onLineEditTextChanged);
    // note pressed
    connect(m_noteView, &NoteView::pressed, this, &MainWindow::onNotePressed);
    // folder selected
    connect(m_folderTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFolderSelectionChanged);
    // noteView viewport pressed
    connect(m_noteView, &NoteView::viewportPressed, this, [this](){
        if(m_isTemp && m_proxyModel->rowCount() > 1){
            QModelIndex indexInProxy = m_proxyModel->index(1, 0);
            selectNote(indexInProxy);
        }else if(m_isTemp && m_proxyModel->rowCount() == 1){
            QModelIndex indexInProxy = m_proxyModel->index(0, 0);
            m_editorDateLabel->clear();
            deleteNote(indexInProxy, false);
        }
    });
    connect(m_proxyModel, &QSortFilterProxyModel::rowsInserted,[&](){
        ui->noteCntLabel->setText(QStringLiteral("%1 Notes").arg(m_proxyModel->rowCount()));
    });
    connect(m_proxyModel, &QSortFilterProxyModel::rowsRemoved,[&](){
        ui->noteCntLabel->setText(QStringLiteral("%1 Notes").arg(m_proxyModel->rowCount()));
    });
    // note model rows moved
    connect(m_noteModel, &NoteModel::rowsAboutToBeMoved, m_noteView, &NoteView::rowsAboutToBeMoved);
    connect(m_noteModel, &NoteModel::rowsMoved, m_noteView, &NoteView::rowsMoved);
    // auto save timer
    connect(m_autoSaveTimer, &QTimer::timeout, [this](){
        m_autoSaveTimer->stop();
        saveNoteToDB(m_currentSelectedNoteProxy);
    });
    // clear button
    connect(m_clearButton, &QToolButton::clicked, this, &MainWindow::onClearButtonClicked);
    // Restore Notes Action
    connect(m_restoreAction, &QAction::triggered, this, [this](){
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

    int id = QFontDatabase::addApplicationFont(":/fonts/arimo/Arimo-Regular.ttf");
    QString arimoFont = QFontDatabase::applicationFontFamilies(id).at(0);
    m_textEdit->setFont(QFont(arimoFont, 11));

    m_textEdit->setTextColor(QColor(51, 51, 51));
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
    m_noteCounter = m_dbManager->getLastRowID();
}

void MainWindow::setupModelView()
{
    m_noteView = static_cast<NoteView*>(ui->listView);
    m_proxyModel->setSourceModel(m_noteModel);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setFilterRole(NoteModel::NoteContent);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_noteView->setItemDelegate(new NoteWidgetDelegate(m_noteView));
    m_noteView->setModel(m_proxyModel);

    m_folderTreeView->setModel(m_folderModel);
    m_folderTreeView->setItemDelegate(new FolderWidgetDelegate(m_folderTreeView));
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
NoteData *MainWindow::generateNote(QString noteName)
{
    NoteData* newNote = new NoteData(this);
    newNote->setId(noteName);

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
        FolderData* folderData = new FolderData;
        folderData->setName(QStringLiteral("Notes0"));
        FolderItem* folderItem = new FolderItem(folderData, this);
        m_folderModel->insertFolder(folderItem, 0);

        m_dbManager->addFolder(folderData);

        m_folderTreeView->setEditTriggers(m_folderTreeView->editTriggers() | QAbstractItemView::CurrentChanged);
    }

    // select the first folder in the treeview
    m_folderTreeView->selectionModel()->setCurrentIndex(m_folderModel->index(0,0), QItemSelectionModel::Select);
}

/**
* @brief
* save the current note to database
*/
void MainWindow::saveNoteToDB(const QModelIndex &noteIndex)
{
    if(noteIndex.isValid() && m_isContentModified){
        QModelIndex indexInSrc = m_proxyModel->mapToSource(noteIndex);
        NoteData* note = m_noteModel->getNote(indexInSrc);
        if(note != Q_NULLPTR){
            bool doExist = m_dbManager->isNoteExist(note);
            if(doExist){
                QtConcurrent::run(m_dbManager, &DBManager::modifyNote, note);
            }else{
                QtConcurrent::run(m_dbManager, &DBManager::addNote, note);
            }
        }
        m_isContentModified = false;
    }
}

void MainWindow::removeNoteFromDB(const QModelIndex& noteIndex)
{
    if(noteIndex.isValid()){
        QModelIndex indexInSrc = m_proxyModel->mapToSource(noteIndex);
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
    if(m_proxyModel->rowCount() > 0){
        QModelIndex index = m_proxyModel->index(0,0);
        m_noteView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        m_noteView->setCurrentIndex(index);

        m_currentSelectedNoteProxy = index;
        showNoteInEditor(index);
    }
}

/**
* @brief
* create a new note if there are no notes
*/
void MainWindow::createNewNoteIfEmpty ()
{
    if(m_proxyModel->rowCount() == 0)
        createNewNote();
}

void MainWindow::setButtonsAndFieldsEnabled(bool doEnable)
{
    m_greenMaximizeButton->setEnabled(doEnable);
    m_redCloseButton->setEnabled(doEnable);
    m_yellowMinimizeButton->setEnabled(doEnable);
    m_newNoteButton->setEnabled(doEnable);
    m_trashButton->setEnabled(doEnable);
    m_lineEdit->setEnabled(doEnable);
    m_textEdit->setEnabled(doEnable);
}

/**
* @brief
* Create a new note when clicking the 'new note' button
*/
void MainWindow::onNewNoteButtonClicked()
{
    if(!m_lineEdit->text().isEmpty()){
        clearSearch();
        m_selectedNoteBeforeSearchingInSource = QModelIndex();
    }

    // save the data of the previous selected
    if(m_currentSelectedNoteProxy.isValid()
            && m_isContentModified){

        saveNoteToDB(m_currentSelectedNoteProxy);
        m_isContentModified = false;
    }

    this->createNewNote();
}

/**
* @brief
* Delete selected note when clicking the 'delete note' button
*/
void MainWindow::onTrashButtonClicked()
{
    m_trashButton->blockSignals(true);
    this->deleteSelectedNote();
    m_trashButton->blockSignals(false);
}

void MainWindow::onNewFolderButtonClicked()
{
    // getting the current selected folder (parent)
    m_folderTreeView->setFocus();
    QModelIndex index =  m_folderTreeView->selectionModel()->currentIndex();

    // getting folder data for the new child folder
    // building the item and insert it to the model
    int row = m_folderModel->rowCount(index);
    QString folderName = QStringLiteral("Folder%1").arg(row);
    QString parentPath = m_folderModel->data(index, (int) FolderItem::FolderDataEnum::FullPath).toString();

    FolderData* folderData = new FolderData;
    folderData->setName(folderName);
    folderData->setParentPath(parentPath);

    FolderItem* folderItem = new FolderItem(folderData, this);
    m_folderModel->insertFolder(folderItem, row, index);

    m_folderTreeView->expand(index);

    // Set the current index to the newly crealted folder
    QModelIndex newFolderIndex = m_folderModel->index(row,0,index);
    m_folderTreeView->selectionModel()->setCurrentIndex(newFolderIndex, QItemSelectionModel::ClearAndSelect);

    // save the new folder to the database
    m_dbManager->addFolder(folderData);
}

/**
* @brief
* When clicking on a note in the scrollArea:
* Unhighlight the previous selected note
* If selecting a note when temporery note exist, delete the temp note
* Highlight the selected note
* Load the selected note content into textedit
*/
void MainWindow::onNotePressed (const QModelIndex& index)
{
    if(sender() != Q_NULLPTR){
        QModelIndex indexInProxy = m_proxyModel->index(index.row(), 0);
        selectNote(indexInProxy);
    }
}

void MainWindow::onFolderSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)

    // init Note List variables
    clearSearch();

    m_textEdit->blockSignals(true);
    m_editorDateLabel->clear();
    m_textEdit->clear();
    m_textEdit->blockSignals(false);

    m_noteModel->clearNotes();
    m_currentSelectedNoteProxy = QModelIndex();
    m_selectedNoteBeforeSearchingInSource = QModelIndex();
    m_isTemp = false;
    m_isContentModified = false;

    m_folderTreeView->setFocus();

    if(!selected.indexes().isEmpty()){
        QModelIndex selectedFolderIndex = selected.indexes().at(0);

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
            // add notes to the model
            if(!noteList.isEmpty()){
                m_noteModel->addListNote(noteList);
                m_noteModel->sort(0,Qt::AscendingOrder);
                selectFirstNote();
            }
        }
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
    if(m_currentSelectedNoteProxy.isValid()){
        m_textEdit->blockSignals(true);

        // if it's a new note, incremente the number of notes in the correspondent folder
        if(m_isTemp){
            QModelIndex folderIndex = m_folderTreeView->selectionModel()->currentIndex();
            qDebug() << folderIndex;
            int noteCnt = m_folderModel->data(folderIndex, (int) FolderItem::FolderDataEnum::NoteCount).toInt();
            m_folderModel->setData(folderIndex, QVariant::fromValue(++noteCnt), (int) FolderItem::FolderDataEnum::NoteCount);
            // TODO: save to FolderData to database
        }

        QString content = m_currentSelectedNoteProxy.data(NoteModel::NoteContent).toString();
        if(m_textEdit->toPlainText() != content){
            // start/restart the timer
            m_autoSaveTimer->start(500);

            // move note to the top of the list
            if(m_currentSelectedNoteProxy.row() != 0)
                moveNoteToTop();

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

            QModelIndex index = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
            m_noteModel->setItemData(index, dataValue);

            m_isContentModified = true;
        }

        m_textEdit->blockSignals(false);

        m_isTemp = false;
    }else{
        qDebug() << __FILE__ << " " << __FUNCTION__ << " " << __LINE__
                 << " MainWindow::onTextEditTextChanged() : m_currentSelectedNoteProxy is not valid";
    }
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

    if(!m_isOperationRunning){
        m_isOperationRunning = true;
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

            m_selectedNoteBeforeSearchingInSource = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
        }

        if(m_currentSelectedNoteProxy.isValid()
                && m_isContentModified){

            saveNoteToDB(m_currentSelectedNoteProxy);
        }

        // tell the noteView that we are searching to disable the animation
        m_noteView->setSearching(true);

        while(!m_searchQueue.isEmpty()){
            qApp->processEvents();
            QString str = m_searchQueue.dequeue();
            if(str.isEmpty()){
                m_noteView->setFocusPolicy(Qt::StrongFocus);
                clearSearch();
                QModelIndex indexInProxy = m_proxyModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
                selectNote(indexInProxy);
                m_selectedNoteBeforeSearchingInSource = QModelIndex();
            }else{
                m_noteView->setFocusPolicy(Qt::NoFocus);
                findNotesContain(str);
            }
        }

        m_isOperationRunning = false;
    }
}
/**
 * @brief MainWindow::onClearButtonClicked clears the search and
 * select the note that was selected before searching if it is still valid.
 */
void MainWindow::onClearButtonClicked()
{
    if(!m_isOperationRunning){

        clearSearch();

        if(m_noteModel->rowCount() > 0){
            QModelIndex indexInProxy = m_proxyModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
            int row = m_selectedNoteBeforeSearchingInSource.row();
            if(row == m_noteModel->rowCount())
                indexInProxy = m_proxyModel->index(m_proxyModel->rowCount()-1,0);

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
    if(!m_isOperationRunning){
        m_isOperationRunning = true;

        m_noteView->scrollToTop();

        // clear the textEdit
        m_textEdit->blockSignals(true);
        m_textEdit->clear();
        m_textEdit->setFocus();
        m_textEdit->blockSignals(false);

        if(!m_isTemp){
            ++m_noteCounter;
            QString noteID = QString("noteID_%1").arg(m_noteCounter);
            NoteData* tmpNote = generateNote(noteID);
            m_isTemp = true;

            // insert the new note to NoteModel
            QModelIndex indexSrc = m_noteModel->insertNote(tmpNote, 0);

            // update the editor header date label
            QString dateTimeFromDB = tmpNote->lastModificationdateTime().toString(Qt::ISODate);
            QString dateTimeForEditor = getNoteDateEditor(dateTimeFromDB);
            m_editorDateLabel->setText(dateTimeForEditor);

            // update the current selected index
            m_currentSelectedNoteProxy = m_proxyModel->mapFromSource(indexSrc);

        }else{
            int row = m_currentSelectedNoteProxy.row();
            m_noteView->animateAddedRow(QModelIndex(),row, row);
        }

        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
        m_isOperationRunning = false;
    }
}

/**
 * @brief MainWindow::deleteNote deletes the specified note
 * @param note  : note to delete
 * @param isFromUser :  true if the user clicked on trash button
 */
void MainWindow::deleteNote(const QModelIndex &noteIndex, bool isFromUser)
{
    if(noteIndex.isValid()){
        // delete from model
        QModelIndex indexToBeRemoved = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
        NoteData* noteTobeRemoved = m_noteModel->removeNote(indexToBeRemoved);

        if(m_isTemp){
            m_isTemp = false;
        }else{
            noteTobeRemoved->setDeletionDateTime(QDateTime::currentDateTime());
            QtConcurrent::run(m_dbManager, &DBManager::removeNote, noteTobeRemoved);
        }

        if(isFromUser){
            // clear text edit and time date label
            m_editorDateLabel->clear();
            m_textEdit->blockSignals(true);
            m_textEdit->clear();
            m_textEdit->clearFocus();
            m_textEdit->blockSignals(false);

            if(m_noteModel->rowCount() > 0){
                QModelIndex index = m_noteView->currentIndex();
                m_currentSelectedNoteProxy = index;
            }else{
                m_currentSelectedNoteProxy = QModelIndex();
            }
        }
    }else{
        qDebug() << __FILE__ << " " << __FUNCTION__ << " " << __LINE__
                 << " MainWindow::deleteNote noteIndex is not valid";
    }

    m_noteView->setFocus();
}

/**
* @brief
* Delete the selected note
*/
void MainWindow::deleteSelectedNote ()
{
    if(!m_isOperationRunning){
        m_isOperationRunning = true;
        if(m_currentSelectedNoteProxy.isValid()){

            // update the index of the selected note before searching
            if(!m_lineEdit->text().isEmpty()){
                QModelIndex currentIndexInSource = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
                int beforeSearchSelectedRow = m_selectedNoteBeforeSearchingInSource.row();
                if(currentIndexInSource.row() < beforeSearchSelectedRow){
                    m_selectedNoteBeforeSearchingInSource = m_noteModel->index(beforeSearchSelectedRow-1);
                }
            }

            deleteNote(m_currentSelectedNoteProxy, true);
            showNoteInEditor(m_currentSelectedNoteProxy);
        }
        m_isOperationRunning = false;
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

/**
* @brief
* Select the note below the currentSelectedNote
*/
void MainWindow::selectNoteDown ()
{
    if(m_currentSelectedNoteProxy.isValid()){
        if(m_isTemp){
            deleteNote(m_currentSelectedNoteProxy, false);
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
    if(this->windowState() == Qt::WindowFullScreen){
        this->setWindowState(Qt::WindowNoState);
    }else{
        this->setWindowState(Qt::WindowFullScreen);
    }
}

/**
* @brief
* Maximize the window
*/
void MainWindow::maximizeWindow ()
{
    if(this->windowState() == Qt::WindowMaximized || this->windowState() == Qt::WindowFullScreen){
        this->setWindowState(Qt::WindowNoState);
    }else{
        this->setWindowState(Qt::WindowMaximized);
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
* When the green button is pressed set it's icon accordingly
*/
void MainWindow::onGreenMaximizeButtonPressed ()
{
    if(this->windowState() == Qt::WindowFullScreen){
        m_greenMaximizeButton->setIcon(QIcon(":images/greenInPressed.png"));
    }else{
        m_greenMaximizeButton->setIcon(QIcon(":images/greenPressed.png"));
    }
}

/**
* @brief
* When the yellow button is pressed set it's icon accordingly
*/
void MainWindow::onYellowMinimizeButtonPressed ()
{
    m_yellowMinimizeButton->setIcon(QIcon(":images/yellowPressed.png"));
}

/**
* @brief
* When the red button is pressed set it's icon accordingly
*/
void MainWindow::onRedCloseButtonPressed ()
{
    m_redCloseButton->setIcon(QIcon(":images/redPressed.png"));
}

/**
* @brief
* When the green button is released the window goes fullscrren
*/
void MainWindow::onGreenMaximizeButtonClicked()
{
    m_greenMaximizeButton->setIcon(QIcon(":images/green.png"));

    fullscreenWindow();
}

/**
* @brief
* When yellow button is released the window is minimized
*/
void MainWindow::onYellowMinimizeButtonClicked()
{
    m_yellowMinimizeButton->setIcon(QIcon(":images/yellow.png"));

    minimizeWindow();
    m_restoreAction->setText(tr("&Show Notes"));
}

/**
* @brief
* When red button is released the window get closed
* If a new note created but wasn't edited, delete it from the database
*/
void MainWindow::onRedCloseButtonClicked()
{
    m_redCloseButton->setIcon(QIcon(":images/red.png"));
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

    if(m_currentSelectedNoteProxy.isValid()
            &&  m_isContentModified
            && !m_isTemp){

        saveNoteToDB(m_currentSelectedNoteProxy);
    }

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
        QModelIndex sourceIndex = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
        QModelIndex destinationIndex = m_noteModel->index(0);
        m_noteModel->moveRow(sourceIndex, sourceIndex.row(), destinationIndex, 0);

        // update the current item
        m_currentSelectedNoteProxy = m_proxyModel->mapFromSource(destinationIndex);
        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
    }
}

void MainWindow::clearSearch()
{
    m_noteView->setFocusPolicy(Qt::StrongFocus);

    m_lineEdit->blockSignals(true);
    m_lineEdit->clear();
    m_lineEdit->blockSignals(false);

    m_textEdit->blockSignals(true);
    m_textEdit->clear();
    m_textEdit->clearFocus();
    m_editorDateLabel->clear();
    m_textEdit->blockSignals(false);

    m_proxyModel->setFilterFixedString(QStringLiteral(""));

    m_clearButton->hide();
    m_lineEdit->setFocus();

    m_noteView->setSearching(false);
}

void MainWindow::findNotesContain(const QString& keyword)
{
    m_proxyModel->setFilterFixedString(keyword);
    m_clearButton->show();

    m_textEdit->blockSignals(true);
    m_textEdit->clear();
    m_editorDateLabel->clear();
    m_textEdit->blockSignals(false);

    if(m_proxyModel->rowCount() > 0){
        selectFirstNote();
    }else{
        m_currentSelectedNoteProxy = QModelIndex();
    }
}

void MainWindow::selectNote(const QModelIndex &noteIndex)
{
    if(noteIndex.isValid()){
        // save the position of text edit scrollbar
        if(!m_isTemp && m_currentSelectedNoteProxy.isValid()){
            int pos = m_textEdit->verticalScrollBar()->value();
            QModelIndex indexSrc = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
            m_noteModel->setData(indexSrc, QVariant::fromValue(pos), NoteModel::NoteScrollbarPos);
        }

        // show the content of the pressed note in the text editor
        showNoteInEditor(noteIndex);

        if(m_isTemp && noteIndex.row() != 0){
            // delete the unmodified new note
            deleteNote(m_currentSelectedNoteProxy, false);
            m_currentSelectedNoteProxy = m_proxyModel->index(noteIndex.row()-1, 0);
        }else if(!m_isTemp
                 && m_currentSelectedNoteProxy.isValid()
                 && noteIndex != m_currentSelectedNoteProxy
                 && m_isContentModified){
            // save if the previous selected note was modified
            saveNoteToDB(m_currentSelectedNoteProxy);
            m_currentSelectedNoteProxy = noteIndex;
        }else{
            m_currentSelectedNoteProxy = noteIndex;
        }

        m_noteView->selectionModel()->select(m_currentSelectedNoteProxy, QItemSelectionModel::ClearAndSelect);
        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
        m_noteView->scrollTo(m_currentSelectedNoteProxy);
    }else{
        qDebug() << __FILE__ << " " << __FUNCTION__ << " " << __LINE__
                 << " MainWindow::selectNote() : noteIndex is not valid " << __LINE__;
    }
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

        NoteData* newNote = new NoteData();
        newNote->setId(noteName);

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

        NoteData* newNote = new NoteData();
        newNote->setId(noteName);

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

                m_redCloseButton->setIcon(QIcon(":images/redHovered.png"));
                m_yellowMinimizeButton->setIcon(QIcon(":images/yellowHovered.png"));
                if(this->windowState() == Qt::WindowFullScreen){
                    m_greenMaximizeButton->setIcon(QIcon(":images/greenInHovered.png"));
                }else{
                    m_greenMaximizeButton->setIcon(QIcon(":images/greenHovered.png"));
                }
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

                m_redCloseButton->setIcon(QIcon(":images/red.png"));
                m_yellowMinimizeButton->setIcon(QIcon(":images/yellow.png"));
                m_greenMaximizeButton->setIcon(QIcon(":images/green.png"));
            }
        }
        break;
    }
    case QEvent::WindowDeactivate:{
        m_redCloseButton->setIcon(QIcon(":images/unfocusedButton"));
        m_yellowMinimizeButton->setIcon(QIcon(":images/unfocusedButton"));
        m_greenMaximizeButton->setIcon(QIcon(":images/unfocusedButton"));
//        m_newNoteButton->setIcon(QIcon(":/images/newNote_Regular.png"));
//        m_trashButton->setIcon(QIcon(":/images/trashCan_Regular.png"));
        break;
    }
    case QEvent::WindowActivate:{
        m_redCloseButton->setIcon(QIcon(":images/red.png"));
        m_yellowMinimizeButton->setIcon(QIcon(":images/yellow.png"));
        m_greenMaximizeButton->setIcon(QIcon(":images/green.png"));
//        m_newNoteButton->setIcon(QIcon(":/images/newNote_Regular.png"));
//        m_trashButton->setIcon(QIcon(":/images/trashCan_Regular.png"));
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

            m_noteView->setCurrentRowActive(true);

            if(!m_isOperationRunning){
                // When clicking in a note's content while searching,
                // reload all the notes and go and select that note
                if(!m_lineEdit->text().isEmpty()){
                    m_selectedNoteBeforeSearchingInSource = QModelIndex();

                    if(m_currentSelectedNoteProxy.isValid()){
                        QModelIndex indexInSource = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
                        clearSearch();
                        m_currentSelectedNoteProxy = m_proxyModel->mapFromSource(indexInSource);
                        selectNote(m_currentSelectedNoteProxy);

                    }else{
                        clearSearch();
                        createNewNote();
                    }

                    m_textEdit->setFocus();

                }else if(m_proxyModel->rowCount() == 0){
                    createNewNote();
                }
            }
        }

        break;
    }
    case QEvent::FocusOut:{
        if(object == m_textEdit){
            m_noteView->setCurrentRowActive(false);
        }

        break;
    }
    default:
        break;
    }

    return QObject::eventFilter(object, event);
}
