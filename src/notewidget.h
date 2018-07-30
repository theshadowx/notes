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

    void initNoteWidget(QList<NoteData*>& noteList);
    QModelIndex getCurrentSelectedIndex() const;
    NoteData* getNoteData(const QModelIndex noteIndex);
    void initNoteCounter(const int count);
    int getNotesLastIndex() const;
    int getNotesCount() const;
    QString getSearchText() const;
    void updateContent(QString title, QString text, QDateTime modificationDateTime);
    void updateScrollBarPos(QModelIndex noteIndex, int pos);



protected:
    bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;

private:
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

    bool                   m_isCurrentNoteTemp;
    int                    m_noteCounter;

    void setupNoteWidget();
    void setupModelView();
    void setupSearchEdit();
    void setupNewNoteButtonAndTrahButton();
    void setupConnections();

    void selectNote(const QModelIndex& noteIndex);
    void selectFirstNote();
    void findNotesContain(const QString &keyword);
    void moveNoteToTop();
    void deleteNote(const QModelIndex& noteIndex);

    void createNewNoteIfEmpty();
    NoteData* generateNote(const int noteID);


public slots:
    void setFocusOnCurrentNote();
    void selectNoteUp();
    void selectNoteDown();
    void focusOnSearch();
    void clearSearch();
    void createNewNote();
    void deleteCurrentNote();
    void onClearButtonClicked();
    void onTrashButtonPressed();
    void onTrashButtonClicked();
    void onNewNoteButtonPressed();
    void onNewNoteButtonClicked();
    void onNotePressed(const QModelIndex &index);
    void onSearchEditTextChanged(const QString& keyword);
    void loadNotes(QList<NoteData*>& noteList);
    void resetNoteWidget();

signals:
    void searchCleared();
    void noteSelectionChanged(QModelIndex selected, QModelIndex deselected);
    void noteAdded(const NoteData* noteData);
    void noteUpdated(const NoteData* noteData);
    void noteDeleted(NoteData* noteData);

};

#endif // NOTEWIDGET_H
