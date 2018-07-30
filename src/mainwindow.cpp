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
    m_editorDateLabel(Q_NULLPTR),
    m_splitter(Q_NULLPTR),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_restoreAction(new QAction(tr("&Hide Notes"), this)),
    m_quitAction(new QAction(tr("&Quit"), this)),
    m_trayIconMenu(new QMenu(this)),
    m_trafficLightLayout(Q_NULLPTR),
    m_titleBar(Q_NULLPTR),
    m_dbManager(Q_NULLPTR),
    m_trashCounter(0),
    m_layoutMargin(10),
    m_shadowWidth(10),
    m_noteListWidth(200),
    m_canMoveWindow(false),
    m_canStretchWindow(false),
    m_isOperationRunning(false),
    m_dontShowUpdateWindow(false),
    m_alwaysStayOnTop(false),
    m_isContentModified(false)
{
    ui->setupUi(this);

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

            // load notes to the NoteWidget
            QList<NoteData*> noteList = m_dbManager->getAllNotes();
            m_noteWidget->initNoteWidget(noteList);
        });

        QFuture<void> migration = QtConcurrent::run(this, &MainWindow::checkMigration);
        watcher->setFuture(migration);

    } else {
        QList<NoteData*> noteList = m_dbManager->getAllNotes();
        m_noteWidget->initNoteWidget(noteList);
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
        this->activateWindow();
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

    QMargins margins = ui->centralWidget->layout()->contentsMargins();
    m_titleBar->setGeometry(margins.top(),
                            margins.left(),
                            event->size().width() - (margins.left() + margins.right()),
                            24);

    QMainWindow::resizeEvent(event);
}

MainWindow::~MainWindow ()
{
    delete ui;
}

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

    m_titleBar = new QFrame(this);
    m_trafficLightLayout = new QHBoxLayout();

    m_redCloseButton = new QPushButton();
    m_greenMaximizeButton = new QPushButton();
    m_yellowMinimizeButton = new QPushButton();
    m_trafficLightLayout->addWidget(m_redCloseButton);
    m_trafficLightLayout->addWidget(m_yellowMinimizeButton);
    m_trafficLightLayout->addWidget(m_greenMaximizeButton);
    m_trafficLightLayout->addStretch();
    m_trafficLightLayout->setSpacing(0);
    m_trafficLightLayout->setContentsMargins(0,0,0,0);

    m_titleBar->setLayout(m_trafficLightLayout);
    QString ss = "QFrame{background-color:transparent;}";
    m_titleBar->setStyleSheet(ss);

#ifdef _WIN32
    m_trafficLightLayout.setSpacing(0);
    m_trafficLightLayout.setMargin(0);
    m_trafficLightLayout.setGeometry(QRect(2,2,90,16));
#endif

    m_noteWidget = ui->noteWidget;
    m_dotsButton = ui->dotsButton;
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

    m_dotsButton->setToolTip("Open Menu");
}

void MainWindow::setupFonts()
{
#ifdef __APPLE__
    m_editorDateLabel->setFont(QFont("Helvetica Neue", 12, 65));
#else
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

void MainWindow::setupKeyboardShortcuts ()
{
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_N), m_noteWidget, SLOT(onNewNoteButtonClicked()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Delete), m_noteWidget, SLOT(deleteCurrentNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), m_noteWidget, SLOT(focusOnSearch()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_E), m_noteWidget, SLOT(clearSearch()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), m_noteWidget, SLOT(setFocusOnCurrentNote()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down), m_noteWidget, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up), m_noteWidget, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::Key_Down), m_noteWidget, SLOT(selectNoteDown()));
    new QShortcut(QKeySequence(Qt::Key_Up), m_noteWidget, SLOT(selectNoteUp()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter), m_textEdit, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return), m_textEdit, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::Key_Enter), m_textEdit, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::Key_Return), m_textEdit, SLOT(setFocus()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F), this, SLOT(fullscreenWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L), this, SLOT(maximizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M), this, SLOT(minimizeWindow()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this, SLOT(QuitApplication()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_K), this, SLOT(toggleStayOnTop()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_J), this, SLOT(toggleNoteWidget()));

    QxtGlobalShortcut *shortcut = new QxtGlobalShortcut(this);
    shortcut->setShortcut(QKeySequence("META+N"));
    connect(shortcut, &QxtGlobalShortcut::activated,[=]() {

        setMainWindowVisibility(isHidden()
                                || windowState() == Qt::WindowMinimized
                                || qApp->applicationState() == Qt::ApplicationInactive);
    });
}

void MainWindow::setupSplitter()
{
    m_splitter->setCollapsible(0, false);
    m_splitter->setCollapsible(1, false);
}

void MainWindow::setupLine ()
{
#ifdef __APPLE__
    //    ui->line->setStyleSheet("border: 0px solid rgb(221, 221, 221)");
#else
    //    ui->line->setStyleSheet("border: 1px solid rgb(221, 221, 221)");
#endif
}

void MainWindow::setupRightFrame ()
{
    QString ss = "QFrame{ "
                 "  background-image: url(:images/textEdit_background_pattern.png); "
                 "  border: none;"
                 "}";
    ui->frameRight->setStyleSheet(ss);
}

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

void MainWindow::setupSignalsSlots()
{
    connect(&m_updater, &UpdaterWindow::dontShowUpdateWindowChanged, [=](bool state){m_dontShowUpdateWindow = state;});
    // green button
    connect(m_greenMaximizeButton, &QPushButton::pressed, this, &MainWindow::onGreenMaximizeButtonPressed);
    connect(m_greenMaximizeButton, &QPushButton::clicked, this, &MainWindow::onGreenMaximizeButtonClicked);
    // red button
    connect(m_redCloseButton, &QPushButton::pressed, this, &MainWindow::onRedCloseButtonPressed);
    connect(m_redCloseButton, &QPushButton::clicked, this, &MainWindow::onRedCloseButtonClicked);
    // yellow button
    connect(m_yellowMinimizeButton, &QPushButton::pressed, this, &MainWindow::onYellowMinimizeButtonPressed);
    connect(m_yellowMinimizeButton, &QPushButton::clicked, this, &MainWindow::onYellowMinimizeButtonClicked);
    // 3 dots button
    connect(m_dotsButton, &QPushButton::pressed, this, &MainWindow::onDotsButtonPressed);
    connect(m_dotsButton, &QPushButton::clicked, this, &MainWindow::onDotsButtonClicked);
    // text edit text changed
    connect(m_textEdit, &QTextEdit::textChanged, this, &MainWindow::onTextEditTextChanged);
    // auto save timer
    connect(m_autoSaveTimer, &QTimer::timeout, [this](){
        m_autoSaveTimer->stop();
        if(m_isContentModified)
            saveNoteToDB(m_noteWidget->getCurrentSelectedIndex());
    });
    // NoteWidget
    connect(m_noteWidget, &NoteWidget::noteSelectionChanged, this, &MainWindow::onNoteSelectionChanged, Qt::DirectConnection);
    connect(m_noteWidget, &NoteWidget::noteDeleted, this, &MainWindow::removeNoteFromDB, Qt::DirectConnection);
    // Restore Notes Action
    connect(m_restoreAction, &QAction::triggered, this, [this](){
        setMainWindowVisibility(isHidden()
                                || windowState() == Qt::WindowMinimized
                                || (qApp->applicationState() == Qt::ApplicationInactive));
    });
    // Quit Action
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::QuitApplication);
}

void MainWindow::setupDotesButton()
{
    QString ss = "QPushButton { "
                 "  border: none; "
                 "  padding: 0px; "
                 "}";
    m_dotsButton->setStyleSheet(ss);
    m_dotsButton->installEventFilter(this);
}

void MainWindow::autoCheckForUpdates()
{
    m_updater.installEventFilter(this);
    m_updater.setShowWindowDisable(m_dontShowUpdateWindow);
    m_updater.checkForUpdates(false);
}

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
    int noteCount = m_dbManager->getLastRowID();
    m_noteWidget->initNoteCounter(noteCount);
}

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

QString MainWindow::getFirstLine (const QString& str)
{
    if(str.simplified().isEmpty())
        return "New Note";

    QString text = str.trimmed();
    QTextStream ts(&text);
    return std::move(ts.readLine(FIRST_LINE_MAX));
}

QDateTime MainWindow::getQDateTime (QString date)
{
    QDateTime dateTime = QDateTime::fromString(date, Qt::ISODate);
    return dateTime;
}

QString MainWindow::getNoteDateEditor (QString dateEdited)
{
    QDateTime dateTimeEdited(getQDateTime(dateEdited));
    QLocale usLocale(QLocale("en_US"));

    return usLocale.toString(dateTimeEdited, "MMMM d, yyyy, h:mm A");
}

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

void MainWindow::clearTextAndHeader()
{
    m_editorDateLabel->clear();
    m_textEdit->blockSignals(true);
    m_textEdit->clear();
    m_textEdit->clearFocus();
    m_textEdit->blockSignals(false);
}

void MainWindow::removeNoteFromDB(NoteData* noteData)
{
    Q_ASSERT_X(noteData != Q_NULLPTR, "removeNoteFromDB", "noteData is nullptr");

    m_dbManager->removeNote(noteData);
}

void MainWindow::setButtonsAndFieldsEnabled(bool doEnable)
{
    m_greenMaximizeButton->setEnabled(doEnable);
    m_redCloseButton->setEnabled(doEnable);
    m_yellowMinimizeButton->setEnabled(doEnable);

    m_noteWidget->setEnabled(doEnable);
    m_textEdit->setEnabled(doEnable);
    m_dotsButton->setEnabled(doEnable);
}

void MainWindow::onDotsButtonPressed()
{
    m_dotsButton->setIcon(QIcon(":/images/3dots_Pressed.png"));
}

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
    if(m_noteWidget->getNotesCount() < 1){
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

void MainWindow::onTextEditTextChanged ()
{
    QModelIndex currentIndex = m_noteWidget->getCurrentSelectedIndex();
    Q_ASSERT_X(currentIndex.isValid(), "updateText", "currentIndex is not valid");

    m_textEdit->blockSignals(true);
    QString noteContent = currentIndex.data(NoteModel::NoteContent).toString();
    if(m_textEdit->toPlainText() != noteContent){

        QString firstline = getFirstLine(m_textEdit->toPlainText());
        QDateTime dateTime = QDateTime::currentDateTime();
        QString noteDate = dateTime.toString(Qt::ISODate);
        m_editorDateLabel->setText(getNoteDateEditor(noteDate));

        m_noteWidget->updateContent(firstline, m_textEdit->toPlainText(), dateTime);

        m_autoSaveTimer->start(500);

        m_isContentModified = true;
    }

    m_textEdit->blockSignals(false);
}

void MainWindow::onNoteSelectionChanged(QModelIndex selectedNoteIndex, QModelIndex deselectedNoteIndex)
{
    if(deselectedNoteIndex.isValid()){
        // save textEdit scrollbar pos to note if not the same
        int pos = m_textEdit->verticalScrollBar()->value();
        m_noteWidget->updateScrollBarPos(deselectedNoteIndex, pos);
    }

    clearTextAndHeader();

    if(selectedNoteIndex.isValid()){
        showNoteInEditor(selectedNoteIndex);
    }

    highlightSearch();
}

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

void MainWindow::minimizeWindow ()
{
#ifndef _WIN32
    QMargins margins(m_layoutMargin,m_layoutMargin,m_layoutMargin,m_layoutMargin);
    setMargins(margins);
#endif

    // BUG : QTBUG-57902 minimize doesn't store the window state before minimizing
    showMinimized();
}

void MainWindow::QuitApplication ()
{
    MainWindow::close();
}

void MainWindow::checkForUpdates(const bool clicked) {
    Q_UNUSED (clicked);
    m_updater.checkForUpdates(true);
}

void MainWindow::importNotesFile (const bool clicked) {
    Q_UNUSED (clicked);
    executeImport(false);
}

void MainWindow::restoreNotesFile (const bool clicked) {
    Q_UNUSED (clicked);

    if (m_noteWidget->getNotesCount() > 0) {
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

            m_noteWidget->resetNoteWidget();

            QList<NoteData*> noteList = m_dbManager->getAllNotes();
            m_noteWidget->initNoteWidget(noteList);

            int noteCount = m_dbManager->getLastRowID();
            m_noteWidget->initNoteCounter(noteCount);
        });

        watcher->setFuture(QtConcurrent::run(m_dbManager, replace ? &DBManager::restoreNotes : &DBManager::importNotes, noteList));
    }
}

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

void MainWindow::onRedCloseButtonPressed ()
{
#ifdef _WIN32
    m_redCloseButton->setIcon(QIcon(":images/windows_close_pressed.png"));
#else
    m_redCloseButton->setIcon(QIcon(":images/redPressed.png"));
#endif
}

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

void MainWindow::onRedCloseButtonClicked()
{
#ifdef _WIN32
    m_redCloseButton->setIcon(QIcon(":images/windows_close_regular.png"));
#else
    m_redCloseButton->setIcon(QIcon(":images/red.png"));
#endif

    setMainWindowVisibility(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(windowState() != Qt::WindowFullScreen)
        m_settingsDatabase->setValue("windowGeometry", saveGeometry());

    m_settingsDatabase->setValue("dontShowUpdateWindow", m_dontShowUpdateWindow);

    m_settingsDatabase->setValue("splitterSizes", m_splitter->saveState());
    m_settingsDatabase->sync();

    QWidget::closeEvent(event);
}
#ifndef _WIN32

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

    int noteLastIndex = m_noteWidget->getNotesLastIndex();
    m_dbManager->forceLastRowIndexValue(noteLastIndex);
}

void MainWindow::migrateNote(QString notePath)
{
    QSettings notesIni(notePath, QSettings::IniFormat);
    QStringList dbKeys = notesIni.allKeys();

    int noteLastIndex = notesIni.value("notesCounter", "0").toInt();
    m_noteWidget->initNoteCounter(noteLastIndex);

    auto it = dbKeys.begin();
    for(; it < dbKeys.end()-1; it += 3){
        QString noteName = it->split("/")[0];
        int id = noteName.split("_")[1].toInt();

        // sync db index with biggest notes id
        if(m_noteWidget->getNotesLastIndex() < id)
            m_noteWidget->initNoteCounter(id);

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
        if(m_noteWidget->getNotesLastIndex() < id)
            m_noteWidget->initNoteCounter(id);

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
        m_dotsButton->setIcon(QIcon(":/images/3dots_Regular.png"));
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
            // TODO : add invisible frame that will contain the traffic button and delimite
            // the mouse dragging
            if(mouseEvent->y() >= 24){
                return true;
            }
        }
        break;
    case QEvent::KeyPress:
        if(object == m_textEdit){
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            // prevent text edit to capture the meta + key event
            // as it's used in the mainwindow shortcut
            if((keyEvent->modifiers() & Qt::MetaModifier) == Qt::MetaModifier)
                return true;
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

void MainWindow::saveNoteToDB(QModelIndex noteIndex)
{
     Q_ASSERT_X(noteIndex.isValid(), "saveNoteToDB", "noteIndex is not valid");

    NoteData* note = m_noteWidget->getNoteData(noteIndex);

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

void MainWindow::setMargins(QMargins margins) {
    ui->centralWidget->layout()->setContentsMargins(margins);
    m_trafficLightLayout->setGeometry(QRect(4+margins.left(),4+margins.top(),56,16));
}

void MainWindow::highlightSearch() const {
    QString searchString = m_noteWidget->getSearchText();

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
