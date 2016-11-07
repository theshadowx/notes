#ifndef NOTEMODEL_H
#define NOTEMODEL_H

#include <QAbstractListModel>
#include "notedata.h"

class NoteModel : public QAbstractListModel
{

    friend class tst_NoteModel;

public:

    enum NoteRoles{
        NoteID = Qt::UserRole + 1,
        NoteFullTitle,
        NoteCreationDateTime,
        NoteLastModificationDateTime,
        NoteDeletionDateTime,
        NoteContent,
        NoteScrollbarPos,
        NotePath,
        NoteTagSerial,
        NoteTagIndexList
    };

    explicit NoteModel(QObject *parent = Q_NULLPTR);
    ~NoteModel();

    QModelIndex addNote(NoteData* note);
    QModelIndex insertNote(NoteData* note, int row);
    NoteData* getNote(const QModelIndex& index);
    void addListNote(QList<NoteData*> noteList);
    NoteData* removeNote(const QModelIndex& noteIndex);
    bool moveRow(int sourceRow,int destinationChild);
    void clearNotes();

    QList<QPersistentModelIndex> tagIndexes(const int noteId) const;
    bool addTagIndex(const int noteId, const QModelIndex tagIndex);
    void addTagIndexes(const int noteId, const QModelIndexList& tagindexes);
    bool removeTagIndex(const int noteId, const QModelIndex tagIndex);
    bool removeTagIndex(QModelIndex tagIndex);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles) Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void sort(int column = 0, Qt::SortOrder order = Qt::DescendingOrder) Q_DECL_OVERRIDE;

private:

    void setTagsToNote(NoteData* note, QList<QPersistentModelIndex> tagIndexes);

    QList<NoteData *> m_noteList;
    QMap<int, NoteData *> m_noteIdMap;
    QMap<int, QList<QPersistentModelIndex>> m_noteTagMap;

signals:
    void noteRemoved();
};

#endif // NOTEMODEL_H
