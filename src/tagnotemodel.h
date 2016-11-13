#ifndef TAGNOTEMODEL_H
#define TAGNOTEMODEL_H

#include "tagmodel.h"
#include <QAbstractListModel>
#include <QPersistentModelIndex>
#include <QSet>

class TagNoteModel: public QAbstractListModel
{
public:
    explicit TagNoteModel(QObject* parent = Q_NULLPTR);

    QModelIndex mapToSource(QModelIndex index);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void setTagModel(TagModel* tagModel);

private:
    TagModel* m_tagModel;
    QSet<int> m_checkedTags;

};

#endif // TAGNOTEMODEL_H
