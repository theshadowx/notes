#include "tagmodel.h"
#include <QDebug>

TagModel::TagModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

QModelIndex TagModel::addTag(TagData *tag)
{
    const int rowCnt = rowCount();
    beginInsertRows(QModelIndex(), rowCnt, rowCnt);
    m_tagList << tag;
    m_idTagMap[tag->id()] = tag;
    endInsertRows();

    return createIndex(rowCnt, 0);
}

void TagModel::addTags(QList<TagData *> tags)
{
    int start = rowCount();
    int end = start + tags.count()-1;

    beginInsertRows(QModelIndex(), start, end);

    m_tagList << tags;
    foreach (TagData* tag, tags)
        m_idTagMap[tag->id()] = tag;

    endInsertRows();
}

TagData* TagModel::removeTag(const QModelIndex& index)
{
    Q_ASSERT_X(index.isValid(), "TagModel::removeTag", "index is not valid");

    int row = index.row();

    beginRemoveRows(QModelIndex(), row, row);

    TagData* tag = m_tagList[row];
    int tagId = tag->id();

    // update map
    QString noteIdSerial = index.data(TagModel::TagNoteSerial).toString();
    QStringList noteIdList = noteIdSerial.split(TagData::TagSeparator);
    foreach (QString noteIdStr, noteIdList) {
        int noteId = noteIdStr.toInt();
        if(!m_noteTagMap.contains(noteId) || !m_noteTagMap[noteId].contains(tagId))
            continue;

        tag->removeNoteId(noteId);
        m_noteTagMap[noteId].removeOne(tagId);

        if(m_noteTagMap[noteId].isEmpty())
            m_noteTagMap.remove(noteId);
    }

    m_tagList.removeOne(tag);
    m_idTagMap.remove(tagId);
    endRemoveRows();

    return tag;
}

QList<TagData*> TagModel::removeTags(const QList<QPersistentModelIndex> modelList)
{
    QList<TagData*> tagsDeleted;
    foreach (QPersistentModelIndex index, modelList) {
        TagData* tag = removeTag(index);
        tagsDeleted << tag;
    }

    return tagsDeleted;
}

TagData* TagModel::tagData(const QModelIndex& index)
{
    Q_ASSERT_X(index.isValid(), "TagModel::getTagData", "index is not valid");


    return m_tagList.at(index.row());
}

QModelIndex TagModel::indexFromId(int id)
{
    TagData* tag = m_idTagMap[id];
    int row = m_tagList.indexOf(tag);
    if(row == -1)
        return QModelIndex();

    return index(row);
}

QMap<int, QModelIndex> TagModel::indexesFromIds(QList<int> idList)
{
    QMap<int, QModelIndex> indexList;

    foreach (int id, idList) {
        QModelIndex index = indexFromId(id);
        if(index.isValid())
            indexList[id] = index;
    }
    return indexList;
}

bool TagModel::removeNoteId(QModelIndex index, int noteId)
{
    Q_ASSERT_X(index.isValid(), "TagModel::removeNoteId", "index is not valid");

    TagData* tag = m_tagList[index.row()];
    int tagId = tag->id();

    if(!m_noteTagMap.contains(tagId) || !m_noteTagMap[tagId].contains(tagId))
        return false;

    tag->removeNoteId(noteId);
    m_noteTagMap[noteId].removeOne(tagId);

    if(m_noteTagMap[noteId].isEmpty())
        m_noteTagMap.remove(noteId);

    emit dataChanged(index, index);

    return true;
}

bool TagModel::addNoteId(QModelIndex index, int noteId)
{
    Q_ASSERT_X(index.isValid(), "TagModel::addNoteId", "index is not valid");


    TagData* tag = m_tagList[index.row()];
    int tagId = tag->id();

    if(m_noteTagMap[tagId].contains(tagId))
        return false;

    tag->addNoteId(noteId);
    m_noteTagMap[noteId].append(tagId);

    emit dataChanged(index, index);

    return true;
}

void TagModel::updateNoteInTags(QList<QPersistentModelIndex> tagIndexes, int noteId)
{
    QList<int> storedTagIds = m_noteTagMap[noteId];
    QMap<int, QModelIndex> storedTagIndexes = indexesFromIds(storedTagIds);

    foreach (QModelIndex storedTagIndex, storedTagIndexes) {
        if(tagIndexes.contains(storedTagIndex))
            continue;

        removeNoteId(storedTagIndex, noteId);
    }

    foreach (QPersistentModelIndex tagIndex, tagIndexes) {
        if(storedTagIndexes.values().contains(tagIndex))
            continue;

        addNoteId(tagIndex, noteId);
    }
}

QVariant TagModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

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
    case TagNoteSerial:
        return tag->noteIdSerial();
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
    case Qt::EditRole:
    case TagName:
        success = (value.toString() != tag->name());
        tag->setName(value.toString());
        break;
    case TagColor:
        success = (value.value<QColor>() != tag->color());
        tag->setColor(value.value<QColor>());
        break;
    case TagNoteSerial:
        success = (value.toString() != tag->noteIdSerial());
        tag->setNoteIdSerial(value.toString());
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
