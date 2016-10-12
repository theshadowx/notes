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
#include <qaction.h>
#include "notedata.h"
#include "notemodel.h"
#include "noteview.h"
#include "foldermodel.h"
#include "dbmanager.h"

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
    QVBoxLayout* m_noteWidgetsContainer;
    QToolButton* m_clearButton;
    QPushButton* m_greenMaximizeButton;
    QPushButton* m_redCloseButton;
    QPushButton* m_yellowMinimizeButton;
    QPushButton* m_newNoteButton;
    QPushButton* m_trashButton;
    QPushButton* m_addRootFolderButton;
    QPushButton* m_deleteRootFolderButton;
    QPushButton* m_newTagButton;
    QTextEdit* m_textEdit;
    QLineEdit* m_lineEdit;
    QLabel* m_editorDateLabel;
    QSplitter *m_splitter;
    QSystemTrayIcon* m_trayIcon;
    QAction* m_restoreAction;
    QAction* m_quitAction;
    QMenu* m_trayIconMenu;
    QTreeView* m_folderTreeView;
    QListWidget* m_tagListW;

    NoteView* m_noteView;
    NoteModel* m_noteModel;
    NoteModel* m_deletedNotesModel;
    QSortFilterProxyModel* m_proxyModel;
    FolderModel* m_folderModel;
    QModelIndex m_currentSelectedNoteProxy;
    QModelIndex m_selectedNoteBeforeSearchingInSource;
    QQueue<QString> m_searchQueue;
    DBManager* m_dbManager;

    int m_currentVerticalScrollAreaRange;
    int m_mousePressX;
    int m_mousePressY;
    int m_textEditLeftPadding;
    int m_noteCounter;
    int m_folderCounter;
    bool m_canMoveWindow;
    bool m_isTemp;
    bool m_isContentModified;
    bool m_isOperationRunning;
    QString m_currentFolderPath;

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
    NoteData* generateNote(QString noteName);
    QDateTime getQDateTime(QString date);
    void showNoteInEditor(const QModelIndex& noteIndex);
    void sortNotesList(QStringList &stringNotesList);
    void initFolders();
    void saveNoteToDB(const QModelIndex& noteIndex);
    void saveFolderToDB(const QModelIndex& folderIndex);
    void removeNoteFromDB(const QModelIndex& noteIndex);
    void selectFirstNote();
    void clearTextAndHeader();
    void moveNoteToTop();
    void clearSearch();
    void findNotesContain(const QString &keyword);
    void selectNote(const QModelIndex& noteIndex);
    void checkMigration();
    void migrateNote(QString notePath);
    void migrateTrash(QString trashPath);

private slots:
    void InitData();
    void onNewNoteButtonClicked();
    void onTrashButtonClicked();
    void addNewFolder(QModelIndex index = QModelIndex());
    void deleteFolder(QModelIndex index = QModelIndex());
    void onNotePressed(const QModelIndex &index);
    void onFolderSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onTextEditTextChanged();
    void onLineEditTextChanged(const QString& keyword);
    void onClearButtonClicked();
    void onGreenMaximizeButtonPressed ();
    void onYellowMinimizeButtonPressed ();
    void onRedCloseButtonPressed ();
    void onGreenMaximizeButtonClicked();
    void onYellowMinimizeButtonClicked();
    void onRedCloseButtonClicked();
    void createNewNote();
    void deleteNote(const QModelIndex& noteIndex);
    void deleteSelectedNote();
    void setFocusOnCurrentNote();
    void selectNoteDown();
    void selectNoteUp();
    void setFocusOnText();
    void fullscreenWindow();
    void maximizeWindow();
    void minimizeWindow();
    void QuitApplication();
};

#endif // MAINWINDOW_H
