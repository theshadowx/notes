#include "tagmodel.h"

TagModel::TagModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

QModelIndex TagModel::addTag(TagData *tag)
{
    const int rowCnt = rowCount();
    beginInsertRows(QModelIndex(), rowCnt, rowCnt);
    m_tagList << tag;
    endInsertRows();

    return createIndex(rowCnt, 0);
}

void TagModel::addTags(QList<TagData *> tags)
{
    int start = rowCount();
    int end = start + tags.count()-1;
    beginInsertRows(QModelIndex(), start, end);
    m_tagList << tags;
    endInsertRows();
}

TagData* TagModel::removeTag(const QModelIndex &index)
{
    Q_ASSERT_X(index.isValid(), "TagModel::removeTag", "index is not valid");

    int row = index.row();
    beginRemoveRows(QModelIndex(), row, row);
    TagData* tag = m_tagList.takeAt(row);
    endRemoveRows();

    return tag;
}

TagData* TagModel::tagData(const QModelIndex& index)
{
    Q_ASSERT_X(index.isValid(), "TagModel::getTagData", "index is not valid");


    return m_tagList.at(index.row());
}

QVariant TagModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return false;

    TagData* tag = m_tagList[index.row()];

    switch (role) {
    case TagID:
        return tag->id();
    case Qt::EditRole:
    case Qt::DisplayRole:
    case TagName:
        return tag->name();
    case TagColor:
        return tag->color();
    case TagNoteCnt:
        return tag->noteCnt();
    default:
        return QVariant();
    }
}

bool TagModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    TagData* tag = m_tagList[index.row()];

    bool success = false;

    switch (role) {
    case TagID:
        success = (value.toInt() != tag->id());
        tag->setId(value.toInt());
        break;
    case Qt::EditRole:
    case TagName:
        success = (value.toString() != tag->name());
        tag->setName(value.toString());
        break;
    case TagColor:
        success = (value.value<QColor>() != tag->color());
        tag->setColor(value.value<QColor>());
        break;
    case TagNoteCnt:
        success = (value.toInt() != tag->noteCnt());
        tag->setNoteCnt(value.toInt());
        break;
    default:
        return false;
    }

    if(success)
        emit dataChanged(this->index(index.row()),
                         this->index(index.row()),
                         QVector<int>(1,role));

    return success;
}

Qt::ItemFlags TagModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

int TagModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_tagList.count();
}
