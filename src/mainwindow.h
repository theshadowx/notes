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
#include <QTreeView>
#include <QListWidget>
#include <QAction>
#include <QMutexLocker>
#include "notedata.h"
#include "notemodel.h"
#include "noteview.h"
#include "foldermodel.h"
#include "tagmodel.h"
#include "dbmanager.h"
#include "foldertagwidget.h"

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

    QTimer* m_autoSaveTimer;
    QSettings* m_settingsDatabase;
    QToolButton* m_clearSearchButton;
    QPushButton* m_greenMaximizeButton;
    QPushButton* m_redCloseButton;
    QPushButton* m_yellowMinimizeButton;
    QPushButton* m_addNoteButton;
    QPushButton* m_deleteNoteButton;
    QPushButton* m_tagNoteButton;
    QTextEdit* m_textEdit;
    QLineEdit* m_lineEdit;
    QLabel* m_editorDateLabel;
    QSplitter *m_splitter;
    QSystemTrayIcon* m_trayIcon;
    QAction* m_trayRestoreAction;
    QAction* m_quitAction;
    QMenu* m_trayIconMenu;
    QMutex m_mutex;

    NoteView* m_noteView;
    NoteModel* m_noteModel;
    NoteModel* m_deletedNotesModel;
    QSortFilterProxyModel* m_proxyNoteModel;
    QModelIndex m_currentSelectedNoteProxy;
    QModelIndex m_selectedNoteBeforeSearchingInSource;
    QQueue<QString> m_searchQueue;
    DBManager* m_dbManager;
    FolderTagWidget* m_folderTagWidget;

    int m_currentVerticalScrollAreaRange;
    int m_mousePressX;
    int m_mousePressY;
    int m_textEditLeftPadding;
    int m_noteCounter;
    bool m_canMoveWindow;
    bool m_isTemp;
    bool m_isContentModified;
    bool m_isOperationRunning;
    bool m_isNoteEditable;
    bool m_isNoteDeletionEnabled;
    bool m_isAddingNoteEnabled;

    void setupMainWindow();
    void setupTrayIcon();
    void setupKeyboardShortcuts();
    void setupSplitter();
    void setupTitleBarButtons();
    void setupSignalsSlots();
    void setupLineEdit();
    void setupTextEdit();
    void setupDatabases();
    void setupModelView();
    void initializeSettingsDatabase();
    void createNewNoteIfEmpty();
    void setLayoutForScrollArea();
    void setButtonsAndFieldsEnabled(bool doEnable);
    void restoreStates();
    QString getFirstLine(const QString& str);
    QString getNoteDateEditor (QString dateEdited);
    NoteData* generateNote(int id);
    QDateTime getQDateTime(QString date);
    void showNoteInEditor(const QModelIndex& noteIndex);
    void sortNotesList(QStringList &stringNotesList);
    void fillNoteModel(QList<NoteData*> noteList);
    void saveNoteToDB(const QModelIndex& noteIndex);
    void removeNoteFromDB(const QModelIndex& noteIndex);
    void selectFirstNote();
    void clearTextAndHeader();
    void moveNoteToTop();
    void clearSearchAndText();
    void clearSearch();
    void findNotesContaining(const QString &keyword);
    void selectNote(const QModelIndex& noteIndex);
    void checkMigration();
    void migrateNote(QString notePath);
    void migrateTrash(QString trashPath);
    void setAddingNoteEnabled(bool state);
    void setNoteDeletionEnabled(bool state);
    void setNoteEditabled(bool state);


private slots:
    void InitData();

    void onTrashFolderSelected();
    void onAllNotesFolderSelected();

    void onFolderSelected(const QString& folderPath, const int noteCount);
    void onFolderAdded(FolderData* folder);
    void onFolderRemoved(const int folderId);
    void onFolderUpdated(const FolderData* folder);


    void onTagAdded(TagData* tag);
    void onTagSelectionChanged(const QString& filterRegexpString);
    void onTagAboutToBeRemoved(QModelIndex index);
    void onTagRemoved(TagData* tag);
    void onTagsRemoved(QList<TagData*> tagList);

    void onTagUpdated(const TagData* tag);


    void onAddNoteButtonClicked();
    void onDeleteNoteButtonClicked();
    void onNoteClicked(const QModelIndex &index);
    void onNoteModelRowsRemoved(const QModelIndex &parent, int first, int last);
    void onTextEditTextChanged();
    void onTextEditTimeoutTriggered();
    void onLineEditTextChanged(const QString& keyword);
    void onNoteDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void onTrayRestoreActionTriggered();
    void onClearSearchButtonClicked();
    void updateNoteCountLabel();
    void onGreenMaximizeButtonClicked();
    void onYellowMinimizeButtonClicked();
    void onRedCloseButtonClicked();
    void onNoteViewViewportClicked();
    void createNewNote();
    void deleteNote(const QModelIndex& noteIndex);
    void deleteSelectedNote();
    void setFocusOnCurrentNote();
    void selectNoteDown();
    void selectNoteUp();
    void showTagNoteMenu();
    void setFocusOnText();
    void fullscreenWindow();
    void maximizeWindow();
    void minimizeWindow();
    void QuitApplication();

signals:
    void noteAdded();
    void noteRemoved();
    void tagsInNoteChanged(const QList<QPersistentModelIndex>& tagIndexes, const int noteId);
};

#endif // MAINWINDOW_H
