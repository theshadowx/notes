/*********************************************************************************************
* Mozila License
* Just a meantime project to see the ability of qt, the framework that my OS might be based on
* And for those linux users that beleive in the power of notes
*********************************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "dbmanager.h"
#include "foldertagwidget.h"
#include "notewidget.h"
#include "editorwidget.h"

#include <QMainWindow>
#include <QSettings>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QMutexLocker>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class tst_MainWindow;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setMainWindowVisibility(bool state);

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

    Q_ENUM(StretchSide)

protected:
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent*) Q_DECL_OVERRIDE;
    bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;

private:

    Ui::MainWindow* ui;

    QSettings* m_settingsDatabase;
    QPushButton* m_greenMaximizeButton;
    QPushButton* m_redCloseButton;
    QPushButton* m_yellowMinimizeButton;
    QSplitter *m_splitter;
    QSystemTrayIcon* m_trayIcon;
    QAction* m_trayRestoreAction;
    QAction* m_quitAction;
    QMenu* m_trayIconMenu;
    QMutex m_mutex;
    DBManager* m_dbManager;
    FolderTagWidget* m_folderTagWidget;
    NoteWidget* m_noteWidget;
    EditorWidget* m_editorWidget;

    int m_currentVerticalScrollAreaRange;
    int m_mousePressX;
    int m_mousePressY;
    int m_textEditLeftPadding;
    StretchSide m_stretchSide;
    bool m_canMoveWindow;
    bool m_canStretchWindow;
    bool m_isContentModified;

    void setupMainWindow();
    void setupTrayIcon();
    void setupKeyboardShortcuts();
    void setupSplitter();
    void setupTitleBarButtons();
    void setupSignalsSlots();
    void setupDatabases();
    void initializeSettingsDatabase();
    void setButtonsAndFieldsEnabled(bool doEnable);
    void restoreStates();

    void checkMigration();
    void migrateNote(QString notePath);
    void migrateTrash(QString trashPath);
    void setNoteEditabled(bool state);

private slots:
    void InitData();

    void onTableLastRowIdReceived(int noteRowId, int tagRowId, int FolderRowId);

    void onTrashFolderSelected();
    void onAllNotesFolderSelected();

    void onFolderSelected(const QString folderName, const QString& folderPath, const int noteCount);
    void onFolderAdded(FolderData* folder);
    void onFolderRemoved(const int folderId);
    void onFolderUpdated(const FolderData* folder);

    void onTagAdded(TagData* tag);
    void onTagRemoved(TagData* tag);
    void onTagsRemoved(QList<TagData*> tagList);
    void onTagUpdated(const TagData* tag);

    void onNoteSelectionChanged(QModelIndex selected, QModelIndex deselected);
    void onNewNoteToBeAdded();
    void onNewNoteAdded(QModelIndex index);
    void onNoteAdded(NoteData* note);
    void onNoteRemoved(NoteData* note);
    void onNoteTagMenuAboutTobeShown(const QModelIndex& index, QMenu& menu);
    void onNoteMenuAddTagClicked(QModelIndex index);
    void onTagIndexesToBeAdded(const int noteId, const QString& tagIdString);
    void onNoteUpdated(NoteData* note);
    void onNoteSearchBegin();
    void onNoteModelContentChanged();

    void onEditorFocusedIn();

    void onTrayRestoreActionTriggered();
    void fullscreenWindow();
    void maximizeWindow();
    void minimizeWindow();
    void QuitApplication();
    void onGreenMaximizeButtonClicked();
    void onYellowMinimizeButtonClicked();
    void onRedCloseButtonClicked();

signals:
    void noteAdded();
    void noteRemoved();

    void tablesLastRowIdRequested();

    void foldersRequested();
    void addFolderRequested(FolderData* folder);
    void removeFolderRequested(const int id);
    void updateFolderRequested(const FolderData* folder);

    void tagsRequested();
    void addTagRequested(const TagData* tag);
    void removeTagRequested(TagData* tag);
    void removeTagsRequested(QList<TagData*> tag);
    void updateTagRequested(const TagData* tag);

    void migrateNoteInTrashResquested(NoteData* note);
    void migrateNoteResquested(NoteData* note);
    void notesInTrashRequested();
    void allNotesRequested();
    void notesRequested(const QString& path);
    void addNoteRequested(const NoteData* note);
    void removeNoteRequested(NoteData* note);
    void updateNoteRequested(const NoteData* note);
};

#endif // MAINWINDOW_H
