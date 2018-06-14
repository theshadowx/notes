#ifndef NOTEWIDGET_H
#define NOTEWIDGET_H

#include <QWidget>
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QPushButton>
#include <QQueue>
#include <QLineEdit>
#include <QToolButton>

#include "notemodel.h"
#include "noteview.h"
#include "notemodel.h"
#include "dbmanager.h"

namespace Ui {
    class NoteWidget;
    }

class NoteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NoteWidget(QWidget *parent = 0);
    ~NoteWidget();

//private:
    Ui::NoteWidget*        ui;

    QLineEdit*             m_searchEdit;
    QPushButton*           m_newNoteButton;
    QPushButton*           m_trashButton;
    QToolButton*           m_clearButton;

    NoteView*              m_noteView;
    NoteModel*             m_noteModel;
    NoteModel*             m_deletedNotesModel;
    QSortFilterProxyModel* m_proxyModel;
    QModelIndex            m_currentSelectedNoteProxy;
    QModelIndex            m_selectedNoteBeforeSearchingInSource;

    QQueue<QString>        m_searchQueue;

    DBManager*             m_dbManager;
    QSettings*             m_settingsDatabase;

    bool                   m_isTemp;
    bool                   m_isContentModified;
    int                    m_noteCounter;

    void setupNoteWidget();
    void setupModelView();
    void setupSearchEdit();
    void setupNewNoteButtonAndTrahButton();

    void clearSearch();
    void selectNote(const QModelIndex& noteIndex);
    void selectFirstNote();
    void findNotesContain(const QString &keyword);
    void moveNoteToTop();

    void createNewNoteIfEmpty();
    NoteData* generateNote(const int noteID);
    void loadNotes();

public slots:
    void setFocusOnCurrentNote();
    void selectNoteUp();
    void selectNoteDown();
    void createNewNote();
    void deleteSelectedNote();
    void deleteNote(const QModelIndex& noteIndex, bool isFromUser=true);
    void onClearButtonClicked();
    void onTrashButtonPressed();
    void onTrashButtonClicked();
    void onNewNoteButtonPressed();
    void onNewNoteButtonClicked();
    void onNotePressed(const QModelIndex &index);
    void onSearchEditTextChanged(const QString& keyword);

};

#endif // NOTEWIDGET_H
