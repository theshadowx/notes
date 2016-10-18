#ifndef TAGMODEL_H
#define TAGMODEL_H

#include <QAbstractListModel>
#include "tagdata.h"


class TagModel : public QAbstractListModel
{
public:

    enum TafRoles{
        TagID = Qt::UserRole + 1,
        TagName,
        TagColor,
        TagNoteCnt
    };

    explicit TagModel(QObject *parent = Q_NULLPTR);

    QModelIndex addTag(TagData* tag);
    void addTags(QList<TagData *> tags);
    TagData* removeTag(const QModelIndex& index);
    TagData* tagData(const QModelIndex& index);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;


private:
    QList<TagData*> m_tagList;
};

#endif // TAGMODEL_H
