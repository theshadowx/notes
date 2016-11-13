#ifndef NOTEWIDGET_H
#define NOTEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QQueue>
#include <QLineEdit>
#include <QToolButton>
#include <QTimer>

#include "noteview.h"
#include "notemodel.h"


namespace Ui {
    class NoteWidget;
    }

class NoteWidget : public QWidget
{
    Q_OBJECT

//    friend class MainWindow;

public:

    explicit NoteWidget(QWidget *parent = 0);
    ~NoteWidget();

    void initNoteCounter(int count);
    void setNoteScrollBarPosition(QModelIndex index, int pos);
    void setNoteText(const QString& text);
    void setNoteTagIndexes(QModelIndex index, QList<QPersistentModelIndex>& tagIndexes);
    void addTagIndexesToNote(const int noteId, QModelIndexList& tagIndexes);
    void setCurrentFolderPath(const QString& currentFolderPath);
    void reset();
    void updateNoteView();
    void prepareForTextEdition();
    bool isViewEmpty();
    bool isEmpty();

    void setNoteDeletionEnabled(bool isNoteDeletionEnabled);
    void setAddingNoteEnabled(bool isAddingNoteEnabled);
    void setNoteEditable(bool isNoteEditable);

    static QString getFirstLine(const QString& str);

private:
    Ui::NoteWidget *ui;

    QTimer* m_autoSaveTimer;
    QToolButton* m_clearSearchButton;
    QPushButton* m_addNoteButton;
    QPushButton* m_deleteNoteButton;
    QPushButton* m_tagNoteButton;
    QLineEdit* m_searchField;

    NoteView* m_noteView;
    NoteModel* m_noteModel;
    NoteModel* m_deletedNotesModel;

    QSortFilterProxyModel* m_proxyNoteModel;
    QModelIndex m_currentSelectedNoteProxy;
    QPersistentModelIndex m_selectedNoteBeforeSearchingInSource;
    QQueue<QString> m_searchQueue;
    QQueue<QString> m_filterTagQueue;
    QString m_currentFolderPath;

    bool m_isNoteDeletionEnabled;
    bool m_isAddingNoteEnabled;
    bool m_isNoteEditable;
    bool m_isTempNoteExist;
    bool m_isOperationRunning;
    int m_noteCounter;

    void setupNoteWidget();
    void setupSearchField();
    void setupModelView();
    void setupSignalsSlots();
    void addNewNoteIfEmpty();
    NoteData* generateNote(int id);
    void sortNotesList(QStringList &stringNotesList);
    void fillNoteModel(QList<NoteData*> noteList);
    void selectFirstNote();
    void moveNoteToTop();
    void clearSearch();
    void findNotesContaining(const QString &keyword);
    void selectNote(const QModelIndex& noteIndex);
    void removeTempNote();
    void removeTempNoteIfExists();

public slots:
    void initNotes(QList<NoteData*> noteList);
    void onAddNoteButtonClicked();
    void onRemoveNoteButtonClicked();
    void onNoteClicked(const QModelIndex &index);
    void onNoteModelRowsRemoved(const QModelIndex &parent, int first, int last);
    void onSearchFieldTextChanged(const QString& keyword);
    void onNoteDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    void onClearSearchButtonClicked();
    void updateNoteCountLabel();
    void onNoteViewViewportClicked();
    void addNewNote();
    void removeNote(const QModelIndex& noteIndex);
    void removeSelectedNote();
    void removeTagFromNotes(QModelIndex index);
    void setFocusOnCurrentNote();
    void selectNoteDown();
    void selectNoteUp();
    void showTagNoteMenu();
    void filterByTag(const QString& regexString);
    void clearFilter();
    void onNoteChangedTimeoutTriggered();

signals:
    void noteSelectionChanged(QModelIndex selected, QModelIndex deselected);
    void newNoteAdded(QModelIndex index);
    void noteRemoved(NoteData* note);
    void noteAdded(NoteData* note);
    void menuAddTagClicked(QModelIndex index);
    void noteTagMenuAboutTobeShown(const QModelIndex& index, QMenu& menu);
    void tagIndexesToBeAdded(const int noteId, const QString& tagIdString);
    void tagsInNoteChanged(const QList<QPersistentModelIndex>& tagIndexes, const int noteId);
    void noteUpdated(NoteData* note);
    void noteModelContentChanged();
    void noteSearchBegin();

};

#endif // NOTEWIDGET_H
