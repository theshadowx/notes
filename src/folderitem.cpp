#include "folderitem.h"

FolderItem::FolderItem(FolderData* folderData, QObject *parent)
    : QObject(parent),
      m_folderData(folderData),
      m_parentFolder(Q_NULLPTR)
{
    m_folderData->setParent(this);
}

int FolderItem::childIndex(QString childName)
{
    int row = 0;
    foreach (FolderItem* item, m_childFolders) {
        if(item->hasName(childName))
            return row;
        ++row;
    }

    return -1;
}

int FolderItem::childIndex(FolderItem* childItem)
{
    Q_ASSERT(childItem != Q_NULLPTR);

    return m_childFolders.indexOf(childItem);
}

FolderItem* FolderItem::child(int number)
{
    return m_childFolders.value(number);
}

int FolderItem::childCount() const
{
    return m_childFolders.count();
}

bool FolderItem::appendChild(FolderItem*childFolder)
{
    childFolder->setParentFolder(this);
    childFolder->setParent(this);
    m_childFolders.append(childFolder);
    return true;
}

bool FolderItem::insertChild(FolderItem* childFolder, int position)
{
    if(position < 0 || position > m_childFolders.size())
        return false;

    childFolder->setParentFolder(this);
    childFolder->setParent(this);
    m_childFolders.insert(position, childFolder);

    return true;
}

bool FolderItem::removeChild(int position)
{
    if(position < 0 || position > m_childFolders.size())
        return false;

    delete m_childFolders.takeAt(position);

    return true;
}

QVariant FolderItem::data(FolderDataEnum data) const
{
    QVariant value;

    switch (data) {
    case FolderDataEnum::ID:
        value = QVariant::fromValue(m_folderData->id());
        break;
    case FolderDataEnum::Name:
        value = QVariant::fromValue(m_folderData->name());
        break;
    case FolderDataEnum::ParentPath:
        value = QVariant::fromValue(m_folderData->parentPath());
        break;
    case FolderDataEnum::FullPath:
        value = QVariant::fromValue(m_folderData->fullPath());
        break;
    case FolderDataEnum::NoteCount:
        value = QVariant::fromValue(m_folderData->noteCnt());
        break;
    default:
        break;
    }

    return value;
}

bool FolderItem::setData(FolderItem::FolderDataEnum data, const QVariant& value)
{
    switch (data) {
    case FolderDataEnum::ID:
        m_folderData->setId(value.toInt());
        break;
    case FolderDataEnum::Name:
        m_folderData->setName(value.toString());
        break;
    case FolderDataEnum::ParentPath:
        m_folderData->setParentPath(value.toString());
        break;
    case FolderDataEnum::NoteCount:
        m_folderData->setNoteCnt(value.toInt());
        break;
    default:
        return false;
    }

    return true;
}

FolderItem* FolderItem::parentFolder() const
{
    return m_parentFolder;
}

void FolderItem::setParentFolder(FolderItem* parentFolder)
{
    m_parentFolder = parentFolder;
}

int FolderItem::row()
{
    if(m_parentFolder == Q_NULLPTR)
        return -1;

    return m_parentFolder->m_childFolders.indexOf(this);
}

bool FolderItem::contains(QString folderName)
{
    bool contained = false;
    foreach (FolderItem* item, m_childFolders) {
        contained = (item->m_folderData->name() == folderName);
        if(contained)
            break;
    }

    return contained;
}

bool FolderItem::hasName(QString folderName)
{
    return m_folderData->name() == folderName;
}

const FolderData*FolderItem::folderData() const
{
    return m_folderData;
}

