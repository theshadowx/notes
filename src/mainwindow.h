/*********************************************************************************************
* Mozila License
* Just a meantime project to see the ability of qt, the framework that my OS might be based on
* And for those linux users that beleive in the power of notes
*********************************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QtCore>
#include <QGroupBox>
#include <QPushButton>
#include <vector>
#include <QToolButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QProgressDialog>
#include <QAction>
#include <QAutostart>
#include "notedata.h"
#include "notemodel.h"
#include "noteview.h"
#include "updaterwindow.h"
#include "dbmanager.h"
#include "notewidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class tst_MainWindow;

public:

    enum class ShadowType{
        Linear = 0,
        Radial
        };

    enum class ShadowSide{
        Left = 0,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
        };

    enum class StretchSide{
        None = 0,
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
        };

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    Q_ENUM(ShadowType)
    Q_ENUM(ShadowSide)
    Q_ENUM(StretchSide)
#endif

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setMainWindowVisibility(bool state);

protected:
    void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent*) Q_DECL_OVERRIDE;
    bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;

private:

    Ui::MainWindow*     ui;

    NoteWidget*         m_noteWidget;

    QTimer*             m_autoSaveTimer;
    QSettings*          m_settingsDatabase;

    QPushButton*        m_greenMaximizeButton;
    QPushButton*        m_redCloseButton;
    QPushButton*        m_yellowMinimizeButton;
    QPushButton*        m_dotsButton;
    QTextEdit*          m_textEdit;
    QLabel*             m_editorDateLabel;
    QSplitter*          m_splitter;
    QSystemTrayIcon*    m_trayIcon;
    QAction*            m_restoreAction;
    QAction*            m_quitAction;
    QMenu*              m_trayIconMenu;
    QHBoxLayout*        m_trafficLightLayout;
    QFrame*             m_titleBar;

    NoteModel*          m_deletedNotesModel;
    DBManager*          m_dbManager;

    UpdaterWindow       m_updater;
    StretchSide         m_stretchSide;
    Autostart           m_autostart;


    int                 m_mousePressX;
    int                 m_mousePressY;
    int                 m_trashCounter;
    int                 m_layoutMargin;
    int                 m_shadowWidth;
    int                 m_noteListWidth;
    bool                m_canMoveWindow;
    bool                m_canStretchWindow;
    bool                m_isOperationRunning;
    bool                m_dontShowUpdateWindow;
    bool                m_alwaysStayOnTop;
    bool                m_isContentModified;

    void setupMainWindow();
    void setupFonts();
    void setupTrayIcon();
    void setupKeyboardShortcuts();
    void setupSplitter();
    void setupLine();
    void setupRightFrame();
    void setupTitleBarButtons();
    void setupSignalsSlots();
    void setupDotesButton();
    void autoCheckForUpdates();
    void setupTextEdit();
    void setupDatabases();
    void initializeSettingsDatabase();
    void setLayoutForScrollArea();
    void setButtonsAndFieldsEnabled(bool doEnable);
    void restoreStates();
    QString getFirstLine(const QString& str);
    QString getNoteDateEditor (QString dateEdited);
    QDateTime getQDateTime(QString date);
    void showNoteInEditor(const QModelIndex& noteIndex);
    void clearTextAndHeader();
    void highlightSearch() const;
    void checkMigration();
    void executeImport(const bool replace);
    void migrateNote(QString notePath);
    void migrateTrash(QString trashPath);

    void dropShadow(QPainter& painter, ShadowType type, ShadowSide side);
    void fillRectWithGradient(QPainter& painter, const QRect& rect, QGradient& gradient);
    double gaussianDist(double x, const double center, double sigma) const;

    void setMargins(QMargins margins);

private slots:
    void InitData();
    void onDotsButtonPressed();
    void onDotsButtonClicked();
    void onTextEditTextChanged();
    void onNoteSelectionChanged(QModelIndex selectedNoteIndex, QModelIndex deselectedNoteIndex);
    void onGreenMaximizeButtonPressed ();
    void onYellowMinimizeButtonPressed ();
    void onRedCloseButtonPressed ();
    void onGreenMaximizeButtonClicked();
    void onYellowMinimizeButtonClicked();
    void onRedCloseButtonClicked();
    void fullscreenWindow();
    void maximizeWindow();
    void minimizeWindow();
    void QuitApplication();
    void checkForUpdates (const bool clicked);
    void collapseNoteList();
    void expandNoteWidget();
    void toggleNoteWidget();
    void importNotesFile(const bool clicked);
    void exportNotesFile(const bool clicked);
    void restoreNotesFile (const bool clicked);
    void stayOnTop(bool checked);
    void toggleStayOnTop();
    void saveNoteToDB(QModelIndex noteIndex);
    void removeNoteFromDB(NoteData* noteData);
};

#endif // MAINWINDOW_H
