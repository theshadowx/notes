#include "notemodel.h"
#include "tagmodel.h"

NoteModel::NoteModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

NoteModel::~NoteModel()
{

}

QModelIndex NoteModel::addNote(NoteData* note)
{
    const int rowCnt = rowCount();
    beginInsertRows(QModelIndex(), rowCnt, rowCnt);
    m_noteList << note;
    endInsertRows();

    m_noteIdMap[note->id()] = note;
    return createIndex(rowCnt, 0);
}

QModelIndex NoteModel::insertNote(NoteData *note, int row)
{
    if(row >= rowCount()){
        return addNote(note);
    }else{
        beginInsertRows(QModelIndex(), row, row);
        m_noteList.insert(row, note);
        endInsertRows();
    }

    m_noteIdMap[note->id()] = note;
    return createIndex(row, 0);
}

NoteData* NoteModel::getNote(const QModelIndex& index)
{
    if(index.isValid()){
        return m_noteList.at(index.row());
    }else{
        return Q_NULLPTR;
    }
}

void NoteModel::addListNote(QList<NoteData *> noteList)
{
    int start = rowCount();
    int end = start + noteList.count()-1;
    beginInsertRows(QModelIndex(), start, end);
    m_noteList << noteList;
    endInsertRows();

    foreach (NoteData* note, noteList) {
        m_noteIdMap[note->id()] = note;
    }
}

NoteData* NoteModel::removeNote(const QModelIndex &noteIndex)
{
    int row = noteIndex.row();
    beginRemoveRows(QModelIndex(), row, row);
    NoteData* note = m_noteList.takeAt(row);
    endRemoveRows();

    m_noteIdMap.remove(note->id());

    return note;
}

bool NoteModel::moveRow(int sourceRow,
                        int destinationChild)
{
    if(sourceRow<0
            || sourceRow >= m_noteList.count()
            || destinationChild <0
            || destinationChild >= m_noteList.count()){

        return false;
    }

    beginMoveRows(QModelIndex(),sourceRow,sourceRow,QModelIndex(),destinationChild);
    m_noteList.move(sourceRow,destinationChild);
    endMoveRows();

    return true;
}

void NoteModel::clearNotes()
{
    QList<NoteData*> toBeDeletedList = m_noteList;

    beginResetModel();
    m_noteList.clear();
    endResetModel();

    m_noteIdMap.clear();
    m_noteTagMap.clear();

    qDeleteAll(toBeDeletedList);
}

QList<QPersistentModelIndex> NoteModel::tagIndexes(const int noteId) const
{
    return m_noteTagMap[noteId];
}

bool NoteModel::addTagIndex(const int noteId, const QModelIndex tagIndex)
{
    Q_ASSERT_X(tagIndex.isValid(), "NoteModel::addTagIndex", "tagIndex is not valid");

    if(m_noteTagMap.contains(noteId) && m_noteTagMap[noteId].contains(tagIndex))
        return false;

    int tagId = tagIndex.data(TagModel::TagID).toInt();
    m_noteTagMap[noteId].append(tagIndex);
    m_noteIdMap[noteId]->addTagId(tagId);
    return true;
}

void NoteModel::addTagIndexes(const int noteId, const QModelIndexList& tagIndexes)
{
    foreach (QModelIndex index, tagIndexes) {
        addTagIndex(noteId, index);
    }
}

bool NoteModel::removeTagIndex(const int noteId, const QModelIndex tagIndex)
{
    Q_ASSERT_X(tagIndex.isValid(), "NoteModel::removeTagIndex", "tagIndex is not valid");

    if(!m_noteTagMap.contains(noteId) ||
            (m_noteTagMap.contains(noteId) && !m_noteTagMap[noteId].contains(tagIndex)))
        return false;

    int tagId = tagIndex.data(TagModel::TagID).toInt();
    NoteData* note = m_noteIdMap[noteId];
    note->removeTagId(tagId);
    m_noteTagMap[noteId].removeOne(tagIndex);
    if(m_noteTagMap[noteId].count() == 0)
        m_noteTagMap.remove(noteId);

    int row = m_noteList.indexOf(note);
    QModelIndex noteIndex = index(row);

    emit dataChanged(noteIndex, noteIndex);

    return true;
}

bool NoteModel::removeTagIndex(QModelIndex tagIndex)
{
    Q_ASSERT_X(tagIndex.isValid(), "NoteModel::removeTagIndex", "tagIndex is not valid");

    QString noteIds = tagIndex.data(TagModel::TagNoteSerial).toString();
    if(noteIds.isEmpty())
        return false;

    QStringList noteIdList = noteIds.split(TagData::TagSeparator);
    foreach (QString idStr, noteIdList) {
        int id = idStr.toInt();
        removeTagIndex(id, tagIndex);
    }
    return true;
}

QVariant NoteModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT_X(index.model() == this, "NoteModel::setData", "index must be from NoteModel");

    if (!index.isValid())
        return QVariant();

    NoteData* note = m_noteList[index.row()];

    switch (role) {
    case NoteID:
       return note->id();
    case NoteFullTitle:
        return note->fullTitle();
    case NoteCreationDateTime:
        return note->creationDateTime();
    case NoteLastModificationDateTime:
        return note->lastModificationdateTime();
    case NoteDeletionDateTime:
        return note->deletionDateTime();
    case NoteContent:
        return note->content();
    case NoteScrollbarPos:
        return note->scrollBarPosition();
    case NotePath:
        return note->fullPath();
    case NoteTagSerial:
        return note->tagIdSerial();
    case NoteTagIndexList:
        return QVariant::fromValue(tagIndexes(note->id()));
    default:
        return QVariant();
    }
}

bool NoteModel::setItemData(const QModelIndex& index, const QMap<int, QVariant>& roles)
{
    Q_ASSERT_X(index.model() == this, "NoteModel::setData", "index must be from NoteModel");

    if (!index.isValid())
        return false;

    NoteData* note = m_noteList[index.row()];

    for(auto role: roles.toStdMap()) {
        switch (role.first) {
        case NoteID:
           note->setId(role.second.toInt());
            break;
        case NoteFullTitle:
            note->setFullTitle(role.second.toString());
            break;
        case NoteCreationDateTime:
            note->setCreationDateTime(role.second.toDateTime());
            break;
        case NoteLastModificationDateTime:
            note->setLastModificationDateTime(role.second.toDateTime());
            break;
        case NoteDeletionDateTime:
            note->setDeletionDateTime(role.second.toDateTime());
            break;
        case NoteContent:
            note->setContent(role.second.toString());
            break;
        case NoteScrollbarPos:
            note->setScrollBarPosition(role.second.toInt());
            break;
        case NotePath:
            note->setFullPath(role.second.toString());
            break;
        case NoteTagIndexList:
            setTagsToNote(note, role.second.value<QList<QPersistentModelIndex>>());
            break;
        default:
            return false;
        }
    }

    QVector<int> roleList = roles.keys().toVector();

    emit dataChanged(index, index, roleList);

    return true;
}

bool NoteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_ASSERT_X(index.model() == this, "NoteModel::setData", "index must be from NoteModel");

    if (!index.isValid())
        return false;

    NoteData* note = m_noteList[index.row()];

    switch (role) {
    case NoteID:
       note->setId(value.toInt());
        break;
    case NoteFullTitle:
        note->setFullTitle(value.toString());
        break;
    case NoteCreationDateTime:
        note->setCreationDateTime(value.toDateTime());
        break;
    case NoteLastModificationDateTime:
        note->setLastModificationDateTime(value.toDateTime());
        break;
    case NoteDeletionDateTime:
        note->setDeletionDateTime(value.toDateTime());
        break;
    case NoteContent:
        note->setContent(value.toString());
        break;
    case NoteScrollbarPos:
        note->setScrollBarPosition(value.toInt());
        break;
    case NotePath:
        note->setFullPath(value.toString());
        break;
    case NoteTagIndexList:
        setTagsToNote(note, value.value<QList<QPersistentModelIndex>>());
        break;
    default:
        return false;
    }

    emit dataChanged(index, index, QVector<int>(1,role));

    return true;
}

Qt::ItemFlags NoteModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractListModel::flags(index);
}

int NoteModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_noteList.count();
}

void NoteModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column)
    Q_UNUSED(order)

    layoutAboutToBeChanged();
    std::stable_sort(m_noteList.begin(), m_noteList.end(), [](NoteData* lhs, NoteData* rhs){
        return lhs->lastModificationdateTime() > rhs->lastModificationdateTime();
    });
    emit layoutChanged();
}

void NoteModel::setTagsToNote(NoteData* note, QList<QPersistentModelIndex> tagIndexes)
{
    note->setTagIdSerial(QString());
    m_noteTagMap.remove(note->id());

    if(tagIndexes.count() > 0){
        m_noteTagMap[note->id()] = tagIndexes;
        for(auto tagIndex : tagIndexes){
            int id = tagIndex.data(TagModel::TagID).toInt();
            note->addTagId(id);
        }
    }
}
