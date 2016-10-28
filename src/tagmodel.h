#ifndef TAGMODEL_H
#define TAGMODEL_H

#include <QAbstractListModel>
#include "tagdata.h"


class TagModel : public QAbstractListModel
{
public:

    enum TagRoles{
        TagID = Qt::UserRole + 1,
        TagName,
        TagColor,
        TagNoteSerial
    };

    explicit TagModel(QObject *parent = Q_NULLPTR);

    QModelIndex addTag(TagData* tag);
    void addTags(QList<TagData *> tags);
    TagData* removeTag(const QModelIndex& index);
    TagData* tagData(const QModelIndex& index);

    QModelIndex indexFromId(int id);
    QMap<int, QModelIndex> indexesFromIds(QList<int> idList);

    void removeNoteId(QModelIndex index, int noteId);
    void addNoteId(QModelIndex index, int noteId);
    void updateNoteInTags(QList<QPersistentModelIndex> tagIndexes, int noteId);

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;


private:
    QList<TagData*> m_tagList;
    QMap<int, TagData*> m_idTagMap;
    QMap<int,QList<int>> m_noteTagMap;

};

#endif // TAGMODEL_H
