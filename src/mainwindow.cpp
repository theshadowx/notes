/**************************************************************************************
* We believe in the power of notes to help us record ideas and thoughts.
* We want people to have an easy, beautiful and simple way of doing that.
* And so we have Notes.
***************************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "notewidgetdelegate.h"
#include "qxtglobalshortcut.h"
#include "updaterwindow.h"

#include <QScrollBar>
#include <QShortcut>
#include <QTextStream>
#include <QScrollArea>
#include <QtConcurrent>
#include <QProgressDialog>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QList>
#include <QWidgetAction>

#define FIRST_LINE_MAX 80

/**
* Setting up the main window and it's content
*/
MainWindow::MainWindow (QWidget *parent) :
    QMainWindow (parent),
    ui (new Ui::MainWindow),
    m_noteWidget(Q_NULLPTR),
    m_autoSaveTimer(new QTimer(this)),
    m_settingsDatabase(Q_NULLPTR),
    m_greenMaximizeButton(Q_NULLPTR),
    m_redCloseButton(Q_NULLPTR),
    m_yellowMinimizeButton(Q_NULLPTR),
    m_dotsButton(Q_NULLPTR),
    m_textEdit(Q_NULLPTR),
    m_searchEdit(Q_NULLPTR),
    m_editorDateLabel(Q_NULLPTR),
    m_splitter(Q_NULLPTR),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_restoreAction(new QAction(tr("&Hide Notes"), this)),
    m_quitAction(new QAction(tr("&Quit"), this)),
    m_trayIconMenu(new QMenu(this)),
    m_trafficLightLayout(Q_NULLPTR),
    m_noteView(Q_NULLPTR),
    m_noteModel(Q_NULLPTR),
    m_proxyModel(Q_NULLPTR),
    m_dbManager(Q_NULLPTR),
    m_trashCounter(0),
    m_layoutMargin(10),
    m_shadowWidth(10),
    m_noteListWidth(200),
    m_canMoveWindow(false),
    m_canStretchWindow(false),
    m_isOperationRunning(false),
    m_dontShowUpdateWindow(false)
{
    ui->setupUi(this);

    m_noteWidget = ui->noteWidget;
    m_noteModel  = m_noteWidget->m_noteModel;
    m_proxyModel = m_noteWidget->m_proxyModel;
    m_noteView   = m_noteWidget->m_noteView;

    setupMainWindow();
    setupFonts();
    setupTrayIcon();
    setupKeyboardShortcuts();
    setupSplitter();
    setupLine();
    setupRightFrame();
    setupTitleBarButtons();
    setupTextEdit();
    setupDatabases();
    restoreStates();
    setupSignalsSlots();
    autoCheckForUpdates();

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

            m_noteWidget->loadNotes();
            m_noteWidget->createNewNoteIfEmpty();
            m_noteWidget->selectFirstNote();
        });

        QFuture<void> migration = QtConcurrent::run(this, &MainWindow::checkMigration);
        watcher->setFuture(migration);

    } else {
        m_noteWidget->loadNotes();
        m_noteWidget->createNewNoteIfEmpty();
        m_noteWidget->selectFirstNote();
    }

    /// Check if it is running with an argument (ex. hide)
    if (qApp->arguments().contains("--autostart")) {
        setMainWindowVisibility(false);
    }
}

void MainWindow::setMainWindowVisibility(bool state)
{
    if(state){
        show();
        qApp->processEvents();
        qApp->setActiveWindow(this);
        qApp->processEvents();
        m_restoreAction->setText(tr("&Hide Notes"));
    }else{
        m_restoreAction->setText(tr("&Show Notes"));
        hide();
    }
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.save();

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    dropShadow(painter, ShadowType::Linear, ShadowSide::Left  );
    dropShadow(painter, ShadowType::Linear, ShadowSide::Top   );
    dropShadow(painter, ShadowType::Linear, ShadowSide::Right );
    dropShadow(painter, ShadowType::Linear, ShadowSide::Bottom);

    dropShadow(painter, ShadowType::Radial, ShadowSide::TopLeft    );
    dropShadow(painter, ShadowType::Radial, ShadowSide::TopRight   );
    dropShadow(painter, ShadowType::Radial, ShadowSide::BottomRight);
    dropShadow(painter, ShadowType::Radial, ShadowSide::BottomLeft );

    painter.restore();
    QMainWindow::paintEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    if(m_splitter != Q_NULLPTR){
        //restore note list width
        QList<int> sizes = m_splitter->sizes();
        if(sizes.at(0) != 0){
            sizes[0] = m_noteListWidth;
            sizes[1] = m_splitter->width() - m_noteListWidth;
            m_splitter->setSizes(sizes);
        }
    }

    QMainWindow::resizeEvent(event);
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
    this->setAttribute(Qt::WA_TranslucentBackground);
#elif _WIN32
    this->setWindowFlags(Qt::CustomizeWindowHint);
#elif __APPLE__
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
#else
#error "We don't support that version yet..."
#endif

    m_greenMaximizeButton = new QPushButton(this);
    m_redCloseButton = new QPushButton(this);
    m_yellowMinimizeButton = new QPushButton(this);
    m_trafficLightLayout.addWidget(m_redCloseButton);
    m_trafficLightLayout.addWidget(m_yellowMinimizeButton);
    m_trafficLightLayout.addWidget(m_greenMaximizeButton);

#ifdef _WIN32
    m_trafficLightLayout.setSpacing(0);
    m_trafficLightLayout.setMargin(0);
    m_trafficLightLayout.setGeometry(QRect(2,2,90,16));
#endif


    m_noteWidget->m_newNoteButton = m_noteWidget->m_newNoteButton;
    m_noteWidget->m_trashButton = m_noteWidget->m_trashButton;
    m_dotsButton = ui->dotsButton;
    m_searchEdit = m_noteWidget->m_searchEdit;
    m_textEdit = ui->textEdit;
    m_editorDateLabel = ui->editorDateLabel;
    m_splitter = ui->splitter;

#ifndef _WIN32
    QMargins margins(m_layoutMargin, m_layoutMargin, m_layoutMargin, m_layoutMargin);
    setMargins(margins);
#else
    ui->verticalSpacer->changeSize(0, 7, QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->verticalSpacer_upEditorDateLabel->changeSize(0, 27, QSizePolicy::Fixed, QSizePolicy::Fixed);
#endif
    ui->frame->installEventFilter(this);
    ui->centralWidget->setMouseTracking(true);
    this->setMouseTracking(true);

    QPalette pal(palette());
    pal.setColor(QPalette::Background, QColor(248, 248, 248));
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    m_noteWidget->m_newNoteButton->setToolTip("Create New Note");
    m_noteWidget->m_trashButton->setToolTip("Delete Selected Note");
    m_dotsButton->setToolTip("Open Menu");
}

void MainWindow::setupFonts()
{
#ifdef __APPLE__
    m_searchEdit->setFont(QFont("Helvetica Neue", 12));
    m_editorDateLabel->setFont(QFont("Helvetica Neue", 12, 65));
#else
    m_searchEdit->setFont(QFont(QStringLiteral("Roboto"), 10));
    m_editorDateLabel->setFont(QFont(QStringLiteral("Roboto"), 10, QFont::Bold));
#endif
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
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_N), m_noteWidget, SLOT(onNewNoteButtonClicked()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Delete), m_noteWidget, SLOT(deleteSelectedNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), m_searchEdit, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_E), m_searchEdit, SLOT(clear()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), m_noteWidget, SLOT(setFocusOnCurrentNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), m_noteWidget, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), m_noteWidget, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::Key_Down), m_noteWidget, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::Key_Up), m_noteWidget, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter), this, SLOT(setFocusOnText()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), this, SLOT(setFocusOnText()));
    new QShortcut(QKeySequence(Qt::Key_Enter), this, SLOT(setFocusOnText()));
    new QShortcut(QKeySequence(Qt::Key_Return), this, SLOT(setFocusOnText()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F), this, SLOT(fullscreenWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L), this, SLOT(maximizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), this, SLOT(minimizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this, SLOT(QuitApplication()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_K), this, SLOT(toggleStayOnTop()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_J), this, SLOT(toggleNoteWidget()));

    QxtGlobalShortcut *shortcut = new QxtGlobalShortcut(this);
    shortcut->setShortcut(QKeySequence("META+N"));
    connect(shortcut, &QxtGlobalShortcut::activated,[=]() {
        // workaround prevent textEdit and searchEdit
        // from taking 'N' from shortcut
        m_textEdit->setDisabled(true);
        m_searchEdit->setDisabled(true);
        setMainWindowVisibility(isHidden()
                                || windowState() == Qt::WindowMinimized
                                || qApp->applicationState() == Qt::ApplicationInactive);
        m_textEdit->setDisabled(false);
        m_searchEdit->setDisabled(false);
    });
}

/**
* @brief
* Set up the splitter that control the size of the scrollArea and the textEdit
*/
void MainWindow::setupSplitter()
{
    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(1, false);
}

/**
* @brief
* Set up the vertical line that seperate between the scrollArea to the textEdit
*/
void MainWindow::setupLine ()
{
#ifdef __APPLE__
//    ui->line->setStyleSheet("border: 0px solid rgb(221, 221, 221)");
#else
//    ui->line->setStyleSheet("border: 1px solid rgb(221, 221, 221)");
#endif
}

/**
* @brief
* Set up a frame above textEdit and behind the other widgets for a unifed background in thet editor section
*/
void MainWindow::setupRightFrame ()
{
    QString ss = "QFrame{ "
                 "  background-image: url(:images/textEdit_background_pattern.png); "
                 "  border: none;"
                 "}";
    ui->frameRight->setStyleSheet(ss);
}

/**
* @brief
* Setting up the red (close), yellow (minimize), and green (maximize) buttons
* Make only the buttons icon visible
* And install this class event filter to them, to act when hovering on one of them
*/
void MainWindow::setupTitleBarButtons ()
{
    QString ss = "QPushButton { "
                 "  border: none; "
                 "  padding: 0px; "
                 "}";

    m_redCloseButton->setStyleSheet(ss);
    m_yellowMinimizeButton->setStyleSheet(ss);
    m_greenMaximizeButton->setStyleSheet(ss);

#ifdef _WIN32
    m_redCloseButton->setIcon(QIcon(":images/windows_close_regular.png"));
    m_yellowMinimizeButton->setIcon(QIcon(":images/windows_maximize_regular.png"));
    m_greenMaximizeButton->setIcon(QIcon(":images/windows_minimize_regular.png"));

    m_redCloseButton->setIconSize(QSize(34, 16));
    m_yellowMinimizeButton->setIconSize(QSize(28, 16));
    m_greenMaximizeButton->setIconSize(QSize(28, 16));
#endif

    m_redCloseButton->installEventFilter(this);
    m_yellowMinimizeButton->installEventFilter(this);
    m_greenMaximizeButton->installEventFilter(this);
}

/**
 * @brief connect between signals and slots
 */
void MainWindow::setupSignalsSlots()
{
    connect(&m_updater, &UpdaterWindow::dontShowUpdateWindowChanged, [=](bool state){m_dontShowUpdateWindow = state;});
    // actions
    // connect(rightToLeftActionion, &QAction::triggered, this, );
    //connect(checkForUpdatesAction, &QAction::triggered, this, );
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
    connect(m_noteWidget->m_newNoteButton, &QPushButton::pressed, m_noteWidget, &NoteWidget::onNewNoteButtonPressed);
    connect(m_noteWidget->m_newNoteButton, &QPushButton::clicked, m_noteWidget, &NoteWidget::onNewNoteButtonClicked);
    // delete note button
    connect(m_noteWidget->m_trashButton, &QPushButton::pressed, m_noteWidget, &NoteWidget::onTrashButtonPressed);
    connect(m_noteWidget->m_trashButton, &QPushButton::clicked, m_noteWidget, &NoteWidget::onTrashButtonClicked);
    connect(m_noteModel, &NoteModel::rowsRemoved, [this](){m_noteWidget->m_trashButton->setEnabled(true);});
    // 3 dots button
    connect(m_dotsButton, &QPushButton::pressed, this, &MainWindow::onDotsButtonPressed);
    connect(m_dotsButton, &QPushButton::clicked, this, &MainWindow::onDotsButtonClicked);
    // text edit text changed
    connect(m_textEdit, &QTextEdit::textChanged, this, &MainWindow::onTextEditTextChanged);
    // line edit text changed
    connect(m_searchEdit, &QLineEdit::textChanged, m_noteWidget, &NoteWidget::onSearchEditTextChanged);
    // note pressed
    connect(m_noteView, &NoteView::pressed, m_noteWidget, &NoteWidget::onNotePressed);
    // note model rows moved
    connect(m_noteModel, &NoteModel::rowsAboutToBeMoved, m_noteView, &NoteView::rowsAboutToBeMoved);
    connect(m_noteModel, &NoteModel::rowsMoved, m_noteView, &NoteView::rowsMoved);
    // auto save timer
    connect(m_autoSaveTimer, &QTimer::timeout, [this](){
        m_autoSaveTimer->stop();
        saveNoteToDB(m_noteWidget->m_currentSelectedNoteProxy);
    });
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
 * Checks for updates, if an update is found, then the updater dialog will show
 * up, otherwise, no notification shall be showed
 */
void MainWindow::autoCheckForUpdates()
{
    m_updater.installEventFilter(this);
    m_updater.setShowWindowDisable(m_dontShowUpdateWindow);
    m_updater.checkForUpdates(false);
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
    QString ss = QString("QTextEdit {background-image: url(:images/textEdit_background_pattern.png); padding-left: %1px; padding-right: %2px; padding-bottom:2px;} "
                         "QScrollBar::handle:vertical:hover { background: rgb(170, 170, 171); } "
                         "QScrollBar::handle:vertical:pressed { background: rgb(149, 149, 149); } "
                         "QScrollBar::handle:vertical { border-radius: 4px; background: rgb(188, 188, 188); min-height: 20px; }  "
                         "QScrollBar::vertical {border-radius: 4px; width: 8px; color: rgba(255, 255, 255,0);} "
                         "QScrollBar {margin: 0; background: transparent;} "
                         "QScrollBar:hover { background-color: rgb(217, 217, 217);}"
                         "QScrollBar::add-line:vertical { width:0px; height: 0px; subcontrol-position: bottom; subcontrol-origin: margin; }  "
                         "QScrollBar::sub-line:vertical { width:0px; height: 0px; subcontrol-position: top; subcontrol-origin: margin; }"
                         ).arg("27", "27");

    m_textEdit->setStyleSheet(ss);

    m_textEdit->installEventFilter(this);
    m_textEdit->verticalScrollBar()->installEventFilter(this);
    m_textEdit->setFont(QFont(QStringLiteral("Arimo"), 11, QFont::Normal));

    // This is done because for now where we're only handling plain text,
    // and we don't want people to past rich text and get something wrong.
    // In future versions, where we'll support rich text, we'll need to change that.
    m_textEdit->setAcceptRichText(false);

#ifdef __APPLE__
    m_textEdit->setFont(QFont("Helvetica Neue", 14));
#endif
}

void MainWindow::initializeSettingsDatabase()
{
    if(m_settingsDatabase->value("version", "NULL") == "NULL")
        m_settingsDatabase->setValue("version", qApp->applicationVersion());

    if(m_settingsDatabase->value("dontShowUpdateWindow", "NULL") == "NULL")
        m_settingsDatabase->setValue("dontShowUpdateWindow", m_dontShowUpdateWindow);

    if(m_settingsDatabase->value("windowGeometry", "NULL") == "NULL"){
        int initWidth = 733;
        int initHeight = 336;
        QPoint center = qApp->desktop()->geometry().center();
        QRect rect(center.x() - initWidth/2, center.y() - initHeight/2, initWidth, initHeight);
        setGeometry(rect);
        m_settingsDatabase->setValue("windowGeometry", saveGeometry());
    }

    if(m_settingsDatabase->value("splitterSizes", "NULL") == "NULL"){
        m_splitter->resize(width()-2*m_layoutMargin, height()-2*m_layoutMargin);
        QList<int> sizes = m_splitter->sizes();
        m_noteListWidth = ui->frameLeft->minimumWidth() != 0 ? ui->frameLeft->minimumWidth() : m_noteListWidth;
        sizes[0] = m_noteListWidth;
        sizes[1] = m_splitter->width() - m_noteListWidth;
        m_splitter->setSizes(sizes);
        m_settingsDatabase->setValue("splitterSizes", m_splitter->saveState());
    }
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
    m_settingsDatabase = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                       QStringLiteral("Awesomeness"), QStringLiteral("Settings"), this);
    m_settingsDatabase->setFallbacksEnabled(false);
    initializeSettingsDatabase();

    bool doCreate = false;
    QFileInfo fi(m_settingsDatabase->fileName());
    QDir dir(fi.absolutePath());
    bool folderCreated = dir.mkpath(QStringLiteral("."));
    if(!folderCreated)
         qFatal("ERROR: Can't create settings folder : %s", dir.absolutePath().toStdString().c_str());

    QString noteDBFilePath(dir.path() + QDir::separator() + QStringLiteral("notes.db"));

    if(!QFile::exists(noteDBFilePath)){
        QFile noteDBFile(noteDBFilePath);
        if(!noteDBFile.open(QIODevice::WriteOnly))
            qFatal("ERROR : Can't create database file");

        noteDBFile.close();
        doCreate = true;
    }

    m_dbManager = new DBManager(noteDBFilePath, doCreate, this);
    m_noteWidget->m_noteCounter = m_dbManager->getLastRowID();
}

/**
* @brief
* Restore the latest sates (if there are any) of the window and the splitter from the settings database
*/
void MainWindow::restoreStates()
{
    if(m_settingsDatabase->value("windowGeometry", "NULL") != "NULL")
        this->restoreGeometry(m_settingsDatabase->value("windowGeometry").toByteArray());

#ifndef _WIN32
    /// Set margin to zero if the window is maximized
    if (isMaximized()) {
        setMargins(QMargins(0,0,0,0));
    }
#endif

    if(m_settingsDatabase->value("dontShowUpdateWindow", "NULL") != "NULL")
        m_dontShowUpdateWindow = m_settingsDatabase->value("dontShowUpdateWindow").toBool();

    m_splitter->setCollapsible(0, true);
    m_splitter->resize(width() - m_layoutMargin, height() - m_layoutMargin);
    if(m_settingsDatabase->value("splitterSizes", "NULL") != "NULL")
        m_splitter->restoreState(m_settingsDatabase->value("splitterSizes").toByteArray());
    m_noteListWidth = m_splitter->sizes().at(0);
    m_splitter->setCollapsible(0, false);
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
 * @brief show the specified note content text in the text editor
 * Set editorDateLabel text to the the selected note date
 * And restore the scrollBar position if it changed before.
 */
void MainWindow::showNoteInEditor(const QModelIndex &noteIndex)
{
    m_textEdit->blockSignals(true);

    /// fixing bug #202
    m_textEdit->setTextBackgroundColor(QColor(255,255,255, 0));


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

    highlightSearch();
}

/**
* @brief
* save the current note to database
*/
void MainWindow::saveNoteToDB(const QModelIndex &noteIndex)
{
    if(noteIndex.isValid() && m_noteWidget->m_isContentModified){
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

        m_noteWidget->m_isContentModified = false;
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

void MainWindow::setButtonsAndFieldsEnabled(bool doEnable)
{
    m_greenMaximizeButton->setEnabled(doEnable);
    m_redCloseButton->setEnabled(doEnable);
    m_yellowMinimizeButton->setEnabled(doEnable);
    m_noteWidget->m_newNoteButton->setEnabled(doEnable);
    m_noteWidget->m_trashButton->setEnabled(doEnable);
    m_searchEdit->setEnabled(doEnable);
    m_textEdit->setEnabled(doEnable);
    m_dotsButton->setEnabled(doEnable);
}

/**
* @brief
* When the 3 dots button is pressed, set it's icon accordingly
*/
void MainWindow::onDotsButtonPressed()
{
    m_dotsButton->setIcon(QIcon(":/images/3dots_Pressed.png"));
}

/**
* @brief
* Open up the menu when clicking the 3 dots button
*/
void MainWindow::onDotsButtonClicked()
{
    m_dotsButton->setIcon(QIcon(":/images/3dots_Regular.png"));

    QMenu mainMenu;
    QMenu* viewMenu = mainMenu.addMenu("View");
    QMenu* importExportNotesMenu = mainMenu.addMenu("Import/Export Notes");
    importExportNotesMenu->setToolTipsVisible(true);
    viewMenu->setToolTipsVisible(true);
    mainMenu.setToolTipsVisible(true);

    mainMenu.setStyleSheet(QStringLiteral(
                               "QMenu { "
                               "  background-color: rgb(255, 255, 255); "
                               "  border: 1px solid #C7C7C7; "
                               "  }"
                               "QMenu::item:selected { "
                               "  background: 1px solid #308CC6; "
                               "}")
                           );

#ifdef __APPLE__
    mainMenu.setFont(QFont("Helvetica Neue", 13));
    viewMenu->setFont(QFont("Helvetica Neue", 13));
    importExportNotesMenu->setFont(QFont("Helvetica Neue", 13));
#else
    mainMenu.setFont(QFont(QStringLiteral("Roboto"), 10, QFont::Normal));
    viewMenu->setFont(QFont(QStringLiteral("Roboto"), 10, QFont::Normal));
    importExportNotesMenu->setFont(QFont(QStringLiteral("Roboto"), 10, QFont::Normal));
#endif

    // note list visiblity action
    bool isCollapsed = (m_splitter->sizes().at(0) == 0);
    QString actionLabel = isCollapsed? tr("Show notes list (Ctrl+J)")
                                     : tr("Hide notes list (Ctrl+J)");

    QAction* noteListVisbilityAction = viewMenu->addAction(actionLabel);
    if(isCollapsed){
        connect(noteListVisbilityAction, &QAction::triggered, this, &MainWindow::expandNoteWidget);
    }else{
        connect(noteListVisbilityAction, &QAction::triggered, this, &MainWindow::collapseNoteList);
    }

    // Check for update action
    QAction* checkForUpdatesAction = mainMenu.addAction(tr("Check For Updates"));
    connect (checkForUpdatesAction, &QAction::triggered, this, &MainWindow::checkForUpdates);

    // Autostart
    QAction* autostartAction = mainMenu.addAction(tr("Start automatically"));
    connect (autostartAction, &QAction::triggered, this, [&]() {
        m_autostart.setAutostart(autostartAction->isChecked());
    });
    autostartAction->setCheckable(true);
    autostartAction->setChecked(m_autostart.isAutostart());

    mainMenu.addSeparator();

    // Close the app
    QAction* quitAppAction = mainMenu.addAction(tr("Quit"));
    connect (quitAppAction, &QAction::triggered, this, &MainWindow::QuitApplication);

    // Export notes action
    QAction* exportNotesFileAction = importExportNotesMenu->addAction (tr("Export"));
    exportNotesFileAction->setToolTip(tr("Save notes to a file"));
    connect (exportNotesFileAction, SIGNAL (triggered (bool)),
             this, SLOT (exportNotesFile (bool)));

    // Import notes action
    QAction* importNotesFileAction = importExportNotesMenu->addAction (tr("Import"));
    importNotesFileAction->setToolTip(tr("Add notes from a file"));
    connect (importNotesFileAction, SIGNAL (triggered (bool)),
             this, SLOT (importNotesFile (bool)));

    // Restore notes action
    QAction* restoreNotesFileAction = importExportNotesMenu->addAction (tr("Restore"));
    restoreNotesFileAction->setToolTip(tr("Replace all notes with notes from a file"));
    connect (restoreNotesFileAction, SIGNAL (triggered (bool)),
             this, SLOT (restoreNotesFile (bool)));

    // Export disabled if no notes exist
    if(m_noteModel->rowCount() < 1){
        exportNotesFileAction->setDisabled(true);
    }

    // Stay on top action
    QAction* stayOnTopAction = viewMenu->addAction(tr("Always stay on top"));
    stayOnTopAction->setToolTip(tr("Always keep the notes application on top of all windows"));
    stayOnTopAction->setCheckable(true);
    stayOnTopAction->setChecked(m_alwaysStayOnTop);
    connect(stayOnTopAction, SIGNAL(triggered(bool)), this, SLOT(stayOnTop(bool)));

    mainMenu.exec(m_dotsButton->mapToGlobal(QPoint(0, m_dotsButton->height())));
}

/**
* @brief
* When the text on textEdit change:
* if the note edited is not on top of the list, we will make that happen
* If the text changed is of a new (empty) note, reset temp values
*/
void MainWindow::onTextEditTextChanged ()
{
    if(m_noteWidget->m_currentSelectedNoteProxy.isValid()){
        m_textEdit->blockSignals(true);
        QString content = m_noteWidget->m_currentSelectedNoteProxy.data(NoteModel::NoteContent).toString();
        if(m_textEdit->toPlainText() != content){

            m_autoSaveTimer->start(500);

            // move note to the top of the list
            QModelIndex sourceIndex = m_proxyModel->mapToSource(m_noteWidget->m_currentSelectedNoteProxy);
            if(m_noteWidget->m_currentSelectedNoteProxy.row() != 0){
                m_noteWidget->moveNoteToTop();
            }else if(!m_searchEdit->text().isEmpty() && sourceIndex.row() != 0){
                m_noteView->setAnimationEnabled(false);
                m_noteWidget->moveNoteToTop();
                m_noteView->setAnimationEnabled(true);
            }

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

            QModelIndex index = m_proxyModel->mapToSource(m_noteWidget->m_currentSelectedNoteProxy);
            m_noteModel->setItemData(index, dataValue);

            m_noteWidget->m_isContentModified = true;
        }

        m_textEdit->blockSignals(false);

        m_noteWidget->m_isTemp = false;
    }else{
        qDebug() << "MainWindow::onTextEditTextChanged() : m_currentSelectedNoteProxy is not valid";
    }
}

/**
* @brief
* Set focus on textEdit
*/
void MainWindow::setFocusOnText ()
{
    if(m_noteWidget->m_currentSelectedNoteProxy.isValid() && !m_textEdit->hasFocus())
        m_textEdit->setFocus();
}

/**
* @brief
* Switch to fullscreen mode
*/
void MainWindow::fullscreenWindow ()
{
#ifndef _WIN32
    if(isFullScreen()){
        if(!isMaximized()) {
            m_noteListWidth = m_splitter->sizes().at(0) != 0 ? m_splitter->sizes().at(0) : m_noteListWidth;
            QMargins margins(m_layoutMargin,m_layoutMargin,m_layoutMargin,m_layoutMargin);

            setMargins(margins);
        }

        setWindowState(windowState() & ~Qt::WindowFullScreen);
    }else{
        setWindowState(windowState() | Qt::WindowFullScreen);
        setMargins(QMargins(0,0,0,0));
    }

#else
    if(isFullScreen()){
        showNormal();
    }else{
        showFullScreen();
    }
#endif
}

/**
* @brief
* Maximize the window
*/
void MainWindow::maximizeWindow ()
{
#ifndef _WIN32
    if(isMaximized()){
        if(!isFullScreen()){
            m_noteListWidth = m_splitter->sizes().at(0) != 0 ? m_splitter->sizes().at(0) : m_noteListWidth;
            QMargins margins(m_layoutMargin,m_layoutMargin,m_layoutMargin,m_layoutMargin);

            setMargins(margins);
            setWindowState(windowState() & ~Qt::WindowMaximized);
        }else{
            setWindowState(windowState() & ~Qt::WindowFullScreen);
        }

    }else{
        setWindowState(windowState() | Qt::WindowMaximized);
        setMargins(QMargins(0,0,0,0));
    }
#else
    if(isMaximized()){
        setWindowState(windowState() & ~Qt::WindowMaximized);
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    }else if(isFullScreen()){
        setWindowState((windowState() | Qt::WindowMaximized) & ~Qt::WindowFullScreen);
        setGeometry(qApp->primaryScreen()->availableGeometry());
    }else{
        setWindowState(windowState() | Qt::WindowMaximized);
        setGeometry(qApp->primaryScreen()->availableGeometry());
    }
#endif
}

/**
* @brief
* Minimize the window
*/
void MainWindow::minimizeWindow ()
{
#ifndef _WIN32
    QMargins margins(m_layoutMargin,m_layoutMargin,m_layoutMargin,m_layoutMargin);
    setMargins(margins);
#endif

    // BUG : QTBUG-57902 minimize doesn't store the window state before minimizing
    showMinimized();
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
 * Called when the "Check for Updates" menu item is clicked, this function
 * instructs the updater window to check if there are any updates available
 *
 * \param clicked required by the signal/slot connection, the value is ignored
 */
void MainWindow::checkForUpdates(const bool clicked) {
    Q_UNUSED (clicked);
    m_updater.checkForUpdates(true);
}

/**
 * Called when the "Import Notes" menu button is clicked. this function will
 * prompt the user to select a file, attempt to load the file, and update the DB
 * if valid.
 * The user is presented with a dialog box if the upload/import fails for any reason.
 *
 * @brief MainWindow::importNotesFile
 * @param clicked
 */
void MainWindow::importNotesFile (const bool clicked) {
    Q_UNUSED (clicked);
    executeImport(false);
}

/**
 * Called when the "Restore Notes" menu button is clicked. this function will
 * prompt the user to select a file, attempt to load the file, and update the DB
 * if valid.
 * The user is presented with a dialog box if the upload/import/restore fails for any reason.
 *
 * @brief MainWindow::restoreNotesFile
 * @param clicked
 */
void MainWindow::restoreNotesFile (const bool clicked) {
    Q_UNUSED (clicked);

    if (m_noteModel->rowCount() > 0) {
        QMessageBox msgBox;
        msgBox.setText("Warning: All current notes will be lost. Make sure to create a backup copy before proceeding.");
        msgBox.setInformativeText("Would you like to continue?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        if  (msgBox.exec() != QMessageBox::Yes) {
            return;
        }
    }
    executeImport(true);
}

/**
 * Executes the note import process. if replace is true all current notes will be
 * removed otherwise current notes will be kept.
 * @brief MainWindow::importNotes
 * @param replace
 */
void MainWindow::executeImport(const bool replace) {
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open Notes Backup File"), "",
            tr("Notes Backup File (*.nbk)"));

    if (fileName.isEmpty()) {
        return;
    } else {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }
        QList<NoteData*> noteList;
        QDataStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        in.setVersion(QDataStream::Qt_5_6);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
        in.setVersion(QDataStream::Qt_5_4);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
        in.setVersion(QDataStream::Qt_5_2);
#endif

        try {
            in >> noteList;
        } catch (...) {
            // Any exception deserializing will result in an empty note list and  the user will be notified
        }
        file.close();

        if (noteList.isEmpty()) {
            QMessageBox::information(this, tr("Invalid file"), "Please select a valid notes export file");
            return;
        }

        QProgressDialog* pd = new QProgressDialog(replace ? "Restoring Notes..." : "Importing Notes...", "", 0, 0, this);
        pd->setCancelButton(0);
        pd->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        pd->setMinimumDuration(0);
        pd->show();
        pd->setValue(1);

        setButtonsAndFieldsEnabled(false);

        QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
        connect(watcher, &QFutureWatcher<void>::finished, this, [&, pd](){
            pd->deleteLater();

            setButtonsAndFieldsEnabled(true);

            m_noteModel->clearNotes();
            m_noteWidget->loadNotes();
            m_noteWidget->createNewNoteIfEmpty();
            m_noteWidget->selectFirstNote();
        });

        watcher->setFuture(QtConcurrent::run(m_dbManager, replace ? &DBManager::restoreNotes : &DBManager::importNotes, noteList));
    }
}

/**
 * Called when the "Export Notes" menu button is clicked. this function will
 * prompt the user to select a location for the export file, and then builds
 * the file.
 * The user is presented with a dialog box if the file cannot be opened for any reason.
 *
 * @brief MainWindow::exportNotesFile
 * @param clicked
 */
void MainWindow::exportNotesFile (const bool clicked) {
    Q_UNUSED (clicked);
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Notes"), "notes.nbk",
            tr("Notes Backup File (*.nbk)"));
    if (fileName.isEmpty()) {
        return;
    } else {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }
        QDataStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        out.setVersion(QDataStream::Qt_5_6);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
        out.setVersion(QDataStream::Qt_5_4);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
        out.setVersion(QDataStream::Qt_5_2);
#endif
        out << m_dbManager->getAllNotes();
        file.close();
    }
}

void MainWindow::toggleNoteWidget()
{
    bool isCollapsed = (m_splitter->sizes().at(0) == 0);
    if(isCollapsed) {
        expandNoteWidget();
    } else {
        collapseNoteList();
    }
}

void MainWindow::collapseNoteList()
{
    m_splitter->setCollapsible(0, true);
    QList<int> sizes = m_splitter->sizes();
    m_noteListWidth = sizes.at(0);
    sizes[0] = 0;
    m_splitter->setSizes(sizes);
    m_splitter->setCollapsible(0, false);
}

void MainWindow::expandNoteWidget()
{
    int minWidth = ui->frameLeft->minimumWidth();
    int leftWidth = m_noteListWidth < minWidth ? minWidth : m_noteListWidth;

    QList<int> sizes = m_splitter->sizes();
    sizes[0] = leftWidth;
    sizes[1] = m_splitter->width() - leftWidth;
    m_splitter->setSizes(sizes);
}

/**
* @brief
* When the green button is pressed set it's icon accordingly
*/
void MainWindow::onGreenMaximizeButtonPressed ()
{
#ifdef _WIN32
    m_greenMaximizeButton->setIcon(QIcon(":images/windows_minimize_pressed.png"));
#else
    if(this->windowState() == Qt::WindowFullScreen){
        m_greenMaximizeButton->setIcon(QIcon(":images/greenInPressed.png"));
    }else{
        m_greenMaximizeButton->setIcon(QIcon(":images/greenPressed.png"));
    }
#endif
}

/**
* @brief
* When the yellow button is pressed set it's icon accordingly
*/
void MainWindow::onYellowMinimizeButtonPressed ()
{
#ifdef _WIN32
    if(this->windowState() == Qt::WindowFullScreen){
        m_yellowMinimizeButton->setIcon(QIcon(":images/windows_de-maximize_pressed.png"));
    }else{
        m_yellowMinimizeButton->setIcon(QIcon(":images/windows_maximize_pressed.png"));
    }
#else
    m_yellowMinimizeButton->setIcon(QIcon(":images/yellowPressed.png"));
#endif
}

/**
* @brief
* When the red button is pressed set it's icon accordingly
*/
void MainWindow::onRedCloseButtonPressed ()
{
#ifdef _WIN32
    m_redCloseButton->setIcon(QIcon(":images/windows_close_pressed.png"));
#else
    m_redCloseButton->setIcon(QIcon(":images/redPressed.png"));
#endif
}

/**
* @brief
* When the green button is released the window goes fullscrren
*/
void MainWindow::onGreenMaximizeButtonClicked()
{
#ifdef _WIN32
    m_greenMaximizeButton->setIcon(QIcon(":images/windows_minimize_regular.png"));

    minimizeWindow();
    m_restoreAction->setText(tr("&Show Notes"));
#else
    m_greenMaximizeButton->setIcon(QIcon(":images/green.png"));

    fullscreenWindow();
#endif
}

/**
* @brief
* When yellow button is released the window is minimized
*/
void MainWindow::onYellowMinimizeButtonClicked()
{
#ifdef _WIN32
    m_yellowMinimizeButton->setIcon(QIcon(":images/windows_de-maximize_regular.png"));

    fullscreenWindow();
#else
    m_yellowMinimizeButton->setIcon(QIcon(":images/yellow.png"));

    minimizeWindow();
    m_restoreAction->setText(tr("&Show Notes"));
#endif
}

/**
* @brief
* When red button is released the window get closed
* If a new note created but wasn't edited, delete it from the database
*/
void MainWindow::onRedCloseButtonClicked()
{
#ifdef _WIN32
    m_redCloseButton->setIcon(QIcon(":images/windows_close_regular.png"));
#else
    m_redCloseButton->setIcon(QIcon(":images/red.png"));
#endif

    setMainWindowVisibility(false);
}

/**
 * @brief Called when the app is about the close
 * save the geometry of the app to the settings
 * save the current note if it's note temporary one otherwise remove it from DB
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(windowState() != Qt::WindowFullScreen)
        m_settingsDatabase->setValue("windowGeometry", saveGeometry());

    if(m_noteWidget->m_currentSelectedNoteProxy.isValid()
            &&  m_noteWidget->m_isContentModified
            && !m_noteWidget->m_isTemp){

        saveNoteToDB(m_noteWidget->m_currentSelectedNoteProxy);
    }

    m_settingsDatabase->setValue("dontShowUpdateWindow", m_dontShowUpdateWindow);

    m_settingsDatabase->setValue("splitterSizes", m_splitter->saveState());
    m_settingsDatabase->sync();

    QWidget::closeEvent(event);
}
#ifndef _WIN32
/**
* @brief
* Set variables to the position of the window when the mouse is pressed
*/
void MainWindow::mousePressEvent(QMouseEvent* event)
{
    m_mousePressX = event->x();
    m_mousePressY = event->y();

    if(event->buttons() == Qt::LeftButton){
        if(m_mousePressX < this->width() - m_layoutMargin
                && m_mousePressX >m_layoutMargin
                && m_mousePressY < this->height() - m_layoutMargin
                && m_mousePressY > m_layoutMargin){

                m_canMoveWindow = true;
                QApplication::setOverrideCursor(QCursor(Qt::ClosedHandCursor));

        }else{
            m_canStretchWindow = true;

            int currentWidth = m_splitter->sizes().at(0);
            if(currentWidth != 0)
                m_noteListWidth = currentWidth;

            if((m_mousePressX < this->width() && m_mousePressX > this->width() - m_layoutMargin)
                    && (m_mousePressY < m_layoutMargin && m_mousePressY > 0)){
                m_stretchSide = StretchSide::TopRight;
            }else if((m_mousePressX < this->width() && m_mousePressX > this->width() - m_layoutMargin)
                     && (m_mousePressY < this->height() && m_mousePressY > this->height() - m_layoutMargin)){
                m_stretchSide = StretchSide::BottomRight;
            }else if((m_mousePressX < m_layoutMargin && m_mousePressX > 0)
                     && (m_mousePressY < m_layoutMargin && m_mousePressY > 0)){
                m_stretchSide = StretchSide::TopLeft;
            }else if((m_mousePressX < m_layoutMargin && m_mousePressX > 0)
                     && (m_mousePressY < this->height() && m_mousePressY > this->height() - m_layoutMargin)){
                m_stretchSide = StretchSide::BottomLeft;
            }else if(m_mousePressX < this->width() && m_mousePressX > this->width() - m_layoutMargin){
                m_stretchSide = StretchSide::Right;
            }else if(m_mousePressX < m_layoutMargin && m_mousePressX > 0){
                m_stretchSide = StretchSide::Left;
            }else if(m_mousePressY < this->height() && m_mousePressY > this->height() - m_layoutMargin){
                m_stretchSide = StretchSide::Bottom;
            }else if(m_mousePressY < m_layoutMargin && m_mousePressY > 0){
                m_stretchSide = StretchSide::Top;
            }else{
                m_stretchSide = StretchSide::None;
            }
        }

        event->accept();

    }else{
        QMainWindow::mousePressEvent(event);
    }
}

/**
* @brief
* Move the window according to the mouse positions
*/
void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if(!m_canStretchWindow && !m_canMoveWindow){
        m_mousePressX = event->x();
        m_mousePressY = event->y();

        if((m_mousePressX < this->width() && m_mousePressX > this->width() - m_layoutMargin)
                && (m_mousePressY < m_layoutMargin && m_mousePressY > 0)){
            m_stretchSide = StretchSide::TopRight;
        }else if((m_mousePressX < this->width() && m_mousePressX > this->width() - m_layoutMargin)
                 && (m_mousePressY < this->height() && m_mousePressY > this->height() - m_layoutMargin)){
            m_stretchSide = StretchSide::BottomRight;
        }else if((m_mousePressX < m_layoutMargin && m_mousePressX > 0)
                 && (m_mousePressY < m_layoutMargin && m_mousePressY > 0)){
            m_stretchSide = StretchSide::TopLeft;
        }else if((m_mousePressX < m_layoutMargin && m_mousePressX > 0)
                 && (m_mousePressY < this->height() && m_mousePressY > this->height() - m_layoutMargin)){
            m_stretchSide = StretchSide::BottomLeft;
        }else if(m_mousePressX < this->width() && m_mousePressX > this->width() - m_layoutMargin){
            m_stretchSide = StretchSide::Right;
        }else if(m_mousePressX < m_layoutMargin && m_mousePressX > 0){
            m_stretchSide = StretchSide::Left;
        }else if(m_mousePressY < this->height() && m_mousePressY > this->height() - m_layoutMargin){
            m_stretchSide = StretchSide::Bottom;
        }else if(m_mousePressY < m_layoutMargin && m_mousePressY > 0){
            m_stretchSide = StretchSide::Top;
        }else{
            m_stretchSide = StretchSide::None;
        }
    }

    if(!m_canMoveWindow){
        switch (m_stretchSide) {
        case StretchSide::Right:
        case StretchSide::Left:
            ui->centralWidget->setCursor(Qt::SizeHorCursor);
            break;
        case StretchSide::Top:
        case StretchSide::Bottom:
            ui->centralWidget->setCursor(Qt::SizeVerCursor);
            break;
        case StretchSide::TopRight:
        case StretchSide::BottomLeft:
            ui->centralWidget->setCursor(Qt::SizeBDiagCursor);
            break;
        case StretchSide::TopLeft:
        case StretchSide::BottomRight:
            ui->centralWidget->setCursor(Qt::SizeFDiagCursor);
            break;
        default:
            if(!m_canStretchWindow)
                ui->centralWidget->setCursor(Qt::ArrowCursor);
            break;
        }
    }

    if(m_canMoveWindow){
        int dx = event->globalX() - m_mousePressX;
        int dy = event->globalY() - m_mousePressY;
        move (dx, dy);

    }else if(m_canStretchWindow && !isMaximized() && !isFullScreen()){
        int newX = x();
        int newY = y();
        int newWidth = width();
        int newHeight = height();

        int minY =  QApplication::desktop()->availableGeometry().y();

        switch (m_stretchSide) {
        case StretchSide::Right:
            newWidth = abs(event->globalX()-this->x()+1);
            newWidth = newWidth < minimumWidth() ? minimumWidth() : newWidth;
            break;
        case StretchSide::Left:
            newX = event->globalX() - m_mousePressX;
            newX = newX > 0 ? newX : 0;
            newX = newX > geometry().bottomRight().x() - minimumWidth() ? geometry().bottomRight().x() - minimumWidth() : newX;
            newWidth = geometry().topRight().x() - newX + 1;
            newWidth = newWidth < minimumWidth() ? minimumWidth() : newWidth;
            break;
        case StretchSide::Top:
            newY = event->globalY() - m_mousePressY;
            newY = newY < minY ? minY : newY;
            newY = newY > geometry().bottomRight().y() - minimumHeight() ? geometry().bottomRight().y() - minimumHeight() : newY;
            newHeight = geometry().bottomLeft().y() - newY + 1;
            newHeight = newHeight < minimumHeight() ? minimumHeight() : newHeight ;

            break;
        case StretchSide::Bottom:
            newHeight = abs(event->globalY()-y()+1);
            newHeight = newHeight < minimumHeight() ? minimumHeight() : newHeight;

            break;
        case StretchSide::TopLeft:
            newX = event->globalX() - m_mousePressX;
            newX = newX < 0 ? 0: newX;
            newX = newX > geometry().bottomRight().x() - minimumWidth() ? geometry().bottomRight().x()-minimumWidth() : newX;

            newY = event->globalY() - m_mousePressY;
            newY = newY < minY ? minY : newY;
            newY = newY > geometry().bottomRight().y() - minimumHeight() ? geometry().bottomRight().y() - minimumHeight() : newY;

            newWidth = geometry().bottomRight().x() - newX + 1;
            newWidth = newWidth < minimumWidth() ? minimumWidth() : newWidth;

            newHeight = geometry().bottomRight().y() - newY + 1;
            newHeight = newHeight < minimumHeight() ? minimumHeight() : newHeight;

            break;
        case StretchSide::BottomLeft:
            newX = event->globalX() - m_mousePressX;
            newX = newX < 0 ? 0: newX;
            newX = newX > geometry().bottomRight().x() - minimumWidth() ? geometry().bottomRight().x()-minimumWidth() : newX;

            newWidth = geometry().bottomRight().x() - newX + 1;
            newWidth = newWidth < minimumWidth() ? minimumWidth() : newWidth;

            newHeight = event->globalY() - y() + 1;
            newHeight = newHeight < minimumHeight() ? minimumHeight() : newHeight;

            break;
        case StretchSide::TopRight:
            newY = event->globalY() - m_mousePressY;
            newY = newY > geometry().bottomRight().y() - minimumHeight() ? geometry().bottomRight().y() - minimumHeight() : newY;
            newY = newY < minY ? minY : newY;

            newWidth = event->globalX() - x() + 1;
            newWidth = newWidth < minimumWidth() ? minimumWidth() : newWidth;

            newHeight = geometry().bottomRight().y() - newY + 1;
            newHeight = newHeight < minimumHeight() ? minimumHeight() : newHeight;

            break;
        case StretchSide::BottomRight:
            newWidth = event->globalX() - x() + 1;
            newWidth = newWidth < minimumWidth() ? minimumWidth() : newWidth;

            newHeight = event->globalY() - y() + 1;
            newHeight = newHeight < minimumHeight() ? minimumHeight() : newHeight;

            break;
        default:
            break;
        }

        setGeometry(newX, newY, newWidth, newHeight);
        QList<int> sizes = m_splitter->sizes();
        if(sizes[0] != 0){
            sizes[0] = m_noteListWidth;
            sizes[1] = m_splitter->width() - m_noteListWidth;
            m_splitter->setSizes(sizes);
        }
    }
    event->accept();
}

/**
* @brief
  * Initialize flags
 */
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_canMoveWindow = false;
    m_canStretchWindow = false;
    QApplication::restoreOverrideCursor();
    event->accept();
}
#else
/**
* @brief
* Set variables to the position of the window when the mouse is pressed
*/
void MainWindow::mousePressEvent(QMouseEvent* event)
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
void MainWindow::mouseMoveEvent(QMouseEvent* event)
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
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_canMoveWindow = false;
    this->unsetCursor();
    event->accept();
}
#endif

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

    m_dbManager->forceLastRowIndexValue(m_noteWidget->m_noteCounter);
}

void MainWindow::migrateNote(QString notePath)
{
    QSettings notesIni(notePath, QSettings::IniFormat);
    QStringList dbKeys = notesIni.allKeys();

    m_noteWidget->m_noteCounter = notesIni.value("notesCounter", "0").toInt();

    auto it = dbKeys.begin();
    for(; it < dbKeys.end()-1; it += 3){
        QString noteName = it->split("/")[0];
        int id = noteName.split("_")[1].toInt();

        // sync db index with biggest notes id
        m_noteWidget->m_noteCounter = m_noteWidget->m_noteCounter < id ? id : m_noteWidget->m_noteCounter;

        NoteData* newNote = new NoteData();
        newNote->setId(id);

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
    QSettings trashIni(trashPath, QSettings::IniFormat);
    QStringList dbKeys = trashIni.allKeys();

    auto it = dbKeys.begin();
    for(; it < dbKeys.end()-1; it += 3){
        QString noteName = it->split("/")[0];
        int id = noteName.split("_")[1].toInt();

        // sync db index with biggest notes id
        m_noteWidget->m_noteCounter = m_noteWidget->m_noteCounter < id ? id : m_noteWidget->m_noteCounter;

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

void MainWindow::dropShadow(QPainter& painter, ShadowType type, MainWindow::ShadowSide side)
{

    int resizedShadowWidth = m_shadowWidth > m_layoutMargin ? m_layoutMargin : m_shadowWidth;

    QRect mainRect   = rect();

    QRect innerRect(m_layoutMargin,
                    m_layoutMargin,
                    mainRect.width() - 2 * resizedShadowWidth + 1,
                    mainRect.height() - 2 * resizedShadowWidth + 1);
    QRect outerRect(innerRect.x() - resizedShadowWidth,
                    innerRect.y() - resizedShadowWidth,
                    innerRect.width() + 2* resizedShadowWidth,
                    innerRect.height() + 2* resizedShadowWidth);

    QPoint center;
    QPoint topLeft;
    QPoint bottomRight;
    QPoint shadowStart;
    QPoint shadowStop;
    QRadialGradient radialGradient;
    QLinearGradient linearGradient;

    switch (side) {
    case ShadowSide::Left :
        topLeft     = QPoint(outerRect.left(), innerRect.top() + 1);
        bottomRight = QPoint(innerRect.left(), innerRect.bottom() - 1);
        shadowStart = QPoint(innerRect.left(), innerRect.top() + 1);
        shadowStop  = QPoint(outerRect.left(), innerRect.top() + 1);
        break;
    case ShadowSide::Top :
        topLeft     = QPoint(innerRect.left() + 1, outerRect.top());
        bottomRight = QPoint(innerRect.right() - 1, innerRect.top());
        shadowStart = QPoint(innerRect.left() + 1, innerRect.top());
        shadowStop  = QPoint(innerRect.left() + 1, outerRect.top());
        break;
    case ShadowSide::Right :
        topLeft     = QPoint(innerRect.right(), innerRect.top() + 1);
        bottomRight = QPoint(outerRect.right(), innerRect.bottom() - 1);
        shadowStart = QPoint(innerRect.right(), innerRect.top() + 1);
        shadowStop  = QPoint(outerRect.right(), innerRect.top() + 1);
        break;
    case ShadowSide::Bottom :
        topLeft     = QPoint(innerRect.left() + 1, innerRect.bottom());
        bottomRight = QPoint(innerRect.right() - 1, outerRect.bottom());
        shadowStart = QPoint(innerRect.left() + 1, innerRect.bottom());
        shadowStop  = QPoint(innerRect.left() + 1, outerRect.bottom());
        break;
    case ShadowSide::TopLeft:
        topLeft     = outerRect.topLeft();
        bottomRight = innerRect.topLeft();
        center      = innerRect.topLeft();
        break;
    case ShadowSide::TopRight:
        topLeft     = QPoint(innerRect.right(), outerRect.top());
        bottomRight = QPoint(outerRect.right(), innerRect.top());
        center      = innerRect.topRight();
        break;
    case ShadowSide::BottomRight:
        topLeft     = innerRect.bottomRight();
        bottomRight = outerRect.bottomRight();
        center      = innerRect.bottomRight();
        break;
    case ShadowSide::BottomLeft:
        topLeft     = QPoint(outerRect.left(), innerRect.bottom());
        bottomRight = QPoint(innerRect.left(), outerRect.bottom());
        center      = innerRect.bottomLeft();
        break;
    default:
        break;
    }


    QRect zone(topLeft, bottomRight);
    radialGradient = QRadialGradient(center, resizedShadowWidth, center);

    linearGradient.setStart(shadowStart);
    linearGradient.setFinalStop(shadowStop);

    switch (type) {
    case ShadowType::Radial :
        fillRectWithGradient(painter, zone, radialGradient);
        break;
    case ShadowType::Linear :
        fillRectWithGradient(painter, zone, linearGradient);
        break;
    default:
        break;
    }
}

void MainWindow::fillRectWithGradient(QPainter& painter, const QRect& rect, QGradient& gradient)
{
    double variance = 0.2;
    double xMax = 1.10;
    double q = 70/gaussianDist(0, 0, sqrt(variance));
    double nPt = 100.0;

    for(int i=0; i<=nPt; i++){
        double v = gaussianDist(i*xMax/nPt, 0, sqrt(variance));

        QColor c(168, 168, 168, q*v);
        gradient.setColorAt(i/nPt, c);
    }

    painter.fillRect(rect, gradient);
}

double MainWindow::gaussianDist(double x, const double center, double sigma) const
{
    return (1.0 / (2 * M_PI * pow(sigma, 2)) * exp( - pow(x - center, 2) / (2 * pow(sigma, 2))));
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
#ifdef _WIN32
            if(object == m_redCloseButton){
                m_redCloseButton->setIcon(QIcon(":images/windows_close_hovered.png"));
            }

            if(object == m_yellowMinimizeButton){
                if(this->windowState() == Qt::WindowFullScreen){
                    m_yellowMinimizeButton->setIcon(QIcon(":images/windows_de-maximize_hovered.png"));
                }else{
                    m_yellowMinimizeButton->setIcon(QIcon (":images/windows_maximize_hovered.png"));
                }
            }

            if(object == m_greenMaximizeButton){
                m_greenMaximizeButton->setIcon(QIcon(":images/windows_minimize_hovered.png"));
            }
#else
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
#endif

            if(object == m_noteWidget->m_newNoteButton){
                this->setCursor(Qt::PointingHandCursor);
                m_noteWidget->m_newNoteButton->setIcon(QIcon(":/images/newNote_Hovered.png"));
            }

            if(object == m_noteWidget->m_trashButton){
                this->setCursor(Qt::PointingHandCursor);
                m_noteWidget->m_trashButton->setIcon(QIcon(":/images/trashCan_Hovered.png"));
            }

            if(object == m_dotsButton){
                this->setCursor(Qt::PointingHandCursor);
                m_dotsButton->setIcon(QIcon(":/images/3dots_Hovered.png"));
            }
        }

        if(object == ui->frame){
            ui->centralWidget->setCursor(Qt::ArrowCursor);
        }

        break;
    }
    case QEvent::Leave:{
        if(qApp->applicationState() == Qt::ApplicationActive){
            // When not hovering, change back the icons of the traffic lights to their default icon
            if(object == m_redCloseButton
                    || object == m_yellowMinimizeButton
                    || object == m_greenMaximizeButton){

#ifdef _WIN32
                m_redCloseButton->setIcon(QIcon(":images/windows_close_regular.png"));
                m_greenMaximizeButton->setIcon(QIcon(":images/windows_minimize_regular.png"));

                if(this->windowState() == Qt::WindowFullScreen){
                    m_yellowMinimizeButton->setIcon(QIcon(":images/windows_de-maximize_regular.png"));
                }else{
                    m_yellowMinimizeButton->setIcon(QIcon (":images/windows_maximize_regular.png"));
                }
#else
                m_redCloseButton->setIcon(QIcon(":images/red.png"));
                m_yellowMinimizeButton->setIcon(QIcon(":images/yellow.png"));
                m_greenMaximizeButton->setIcon(QIcon(":images/green.png"));
#endif
            }

            if(object == m_noteWidget->m_newNoteButton){
                this->unsetCursor();
                m_noteWidget->m_newNoteButton->setIcon(QIcon(":/images/newNote_Regular.png"));
            }

            if(object == m_noteWidget->m_trashButton){
                this->unsetCursor();
                m_noteWidget->m_trashButton->setIcon(QIcon(":/images/trashCan_Regular.png"));
            }

            if(object == m_dotsButton){
                this->unsetCursor();
                m_dotsButton->setIcon(QIcon(":/images/3dots_Regular.png"));
            }
        }
        break;
    }
    case QEvent::WindowDeactivate:{

        m_canMoveWindow = false;
        m_canStretchWindow = false;
        QApplication::restoreOverrideCursor();

#ifndef _WIN32
        m_redCloseButton->setIcon(QIcon(":images/unfocusedButton"));
        m_yellowMinimizeButton->setIcon(QIcon(":images/unfocusedButton"));
        m_greenMaximizeButton->setIcon(QIcon(":images/unfocusedButton"));
#endif
        m_noteWidget->m_newNoteButton->setIcon(QIcon(":/images/newNote_Regular.png"));
        m_noteWidget->m_trashButton->setIcon(QIcon(":/images/trashCan_Regular.png"));
        m_dotsButton->setIcon(QIcon(":/images/3dots_Regular.png"));
        break;
    }
    case QEvent::WindowActivate:{
#ifdef _WIN32
        m_redCloseButton->setIcon(QIcon(":images/windows_close_regular.png"));
        m_greenMaximizeButton->setIcon(QIcon(":images/windows_minimize_regular.png"));

        if(this->windowState() == Qt::WindowFullScreen){
            m_yellowMinimizeButton->setIcon(QIcon(":images/windows_de-maximize_regular.png"));
        }else{
            m_yellowMinimizeButton->setIcon(QIcon (":images/windows_maximize_regular.png"));
        }
#else
        m_redCloseButton->setIcon(QIcon(":images/red.png"));
        m_yellowMinimizeButton->setIcon(QIcon(":images/yellow.png"));
        m_greenMaximizeButton->setIcon(QIcon(":images/green.png"));
#endif
        m_noteWidget->m_newNoteButton->setIcon(QIcon(":/images/newNote_Regular.png"));
        m_noteWidget->m_trashButton->setIcon(QIcon(":/images/trashCan_Regular.png"));
        m_dotsButton->setIcon(QIcon(":/images/3dots_Regular.png"));
        break;
    }
    case QEvent::HoverEnter:{
        if(object == m_textEdit->verticalScrollBar()){
            bool isSearching = !m_searchEdit->text().isEmpty();
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

            if(!m_isOperationRunning){

                if(!m_searchEdit->text().isEmpty()){
                    if(!m_noteWidget->m_currentSelectedNoteProxy.isValid()){
                        m_noteWidget->clearSearch();
                        m_noteWidget->createNewNote();
                    }

                    m_textEdit->setFocus();

                }else if(m_proxyModel->rowCount() == 0){
                    m_noteWidget->createNewNote();
                }
            }
        }

        if(object == m_searchEdit){
            QString ss = QString("QLineEdit{ "
                                 "  padding-left: 21px;"
                                 "  padding-right: 19px;"
                                 "  border: 2px solid rgb(61, 155, 218);"
                                 "  border-radius: 3px;"
                                 "  background: rgb(255, 255, 255);"
                                 "  selection-background-color: rgb(61, 155, 218);"
                                 "} "
                                 "QToolButton { "
                                 "  border: none; "
                                 "  padding: 0px;"
                                 "}"
                                 );

            m_searchEdit->setStyleSheet(ss);
        }
        break;
    }
    case QEvent::FocusOut:{

        if(object == m_searchEdit){
            QString ss = QString("QLineEdit{ "
                                 "  padding-left: 21px;"
                                 "  padding-right: 19px;"
                                 "  border: 1px solid rgb(205, 205, 205);"
                                 "  border-radius: 3px;"
                                 "  background: rgb(255, 255, 255);"
                                 "  selection-background-color: rgb(61, 155, 218);"
                                 "} "
                                 "QToolButton { "
                                 "  border: none; "
                                 "  padding: 0px;"
                                 "}"
                                 );

            m_searchEdit->setStyleSheet(ss);
        }
        break;
    }
    case QEvent::Show:
        if(object == &m_updater){

            QRect rect = m_updater.geometry();
            QRect appRect = geometry();
            int titleBarHeight = 28 ;

            int x = appRect.x() + (appRect.width() - rect.width())/2.0;
            int y = appRect.y() + titleBarHeight  + (appRect.height() - rect.height())/2.0;

            m_updater.setGeometry(QRect(x, y, rect.width(), rect.height()));
        }
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
        // Only allow double click (maximise/minimise) or dragging (move)
        // from the top part of the window
        if(object == ui->frame){
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->y() >= m_searchEdit->y()){
                return true;
            }
        }
        break;
    default:
        break;
    }

    return QObject::eventFilter(object, event);
}

void MainWindow::stayOnTop(bool checked) {
    Qt::WindowFlags flags;
#ifdef Q_OS_LINUX
    flags = Qt::Window | Qt::FramelessWindowHint;
#elif _WIN32
    flags = Qt::CustomizeWindowHint;
#elif __APPLE__
    flags = Qt::Window | Qt::FramelessWindowHint;
#else
#error "We don't support that version yet..."
#endif
    if (checked) {
        flags |= Qt::WindowStaysOnTopHint;
        m_alwaysStayOnTop = true;
    } else {
        m_alwaysStayOnTop = false;
    }
    this->setWindowFlags(flags);
    setMainWindowVisibility(true);
}

void MainWindow::toggleStayOnTop() {
    this->stayOnTop(!m_alwaysStayOnTop);
}

void MainWindow::setMargins(QMargins margins) {
    ui->centralWidget->layout()->setContentsMargins(margins);
    m_trafficLightLayout.setGeometry(QRect(4+margins.left(),4+margins.top(),56,16));
}

void MainWindow::highlightSearch() const {
    QString searchString = m_searchEdit->text();

    if(!searchString.isEmpty()){
        m_textEdit->blockSignals(true);

        QTextDocument *document = m_textEdit->document();

        QTextCursor highlightCursor(document);
        QTextCursor cursor(document);

        cursor.beginEditBlock();

        QTextCharFormat colorFormat(highlightCursor.charFormat());
        colorFormat.setBackground(Qt::yellow);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
            highlightCursor = document->find(searchString, highlightCursor);

            if (!highlightCursor.isNull()) {

                highlightCursor.mergeCharFormat(colorFormat);
            }
        }

        cursor.endEditBlock();

        m_textEdit->blockSignals(false);
    }
}
