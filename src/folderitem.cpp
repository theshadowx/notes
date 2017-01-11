#include "folderitem.h"

FolderItem::FolderItem(FolderData* folderData, QObject *parent)
    : QObject(parent),
      m_folderData(folderData),
      m_parentFolder(Q_NULLPTR)
{
    m_folderData->setParent(this);
}

int FolderItem::childIndex(int childID) const
{
    int row = 0;
    foreach (FolderItem* item, m_childFolders) {
        if(item->hasID(childID))
            return row;
        ++row;
    }

    return -1;
}

int FolderItem::childIndex(FolderItem* childItem) const
{
    Q_ASSERT(childItem != Q_NULLPTR);

    return m_childFolders.indexOf(childItem);
}

FolderItem* FolderItem::child(int number) const
{
    return m_childFolders.value(number);
}

int FolderItem::childCount() const
{
    return m_childFolders.count();
}

bool FolderItem::appendChild(FolderItem* childFolder)
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

int FolderItem::id() const
{
    return m_folderData->id();
}

QString FolderItem::name() const
{
    return m_folderData->name();
}

QString FolderItem::parentPath() const
{
    return m_folderData->parentPath();
}

QString FolderItem::fullPath() const
{
    return m_folderData->fullPath();
}

int FolderItem::noteCnt() const
{
    return m_folderData->noteCnt();
}

bool FolderItem::setID(const int id)
{
    bool changed = (m_folderData->id() == id);
    m_folderData->setId(id);
    return changed ;
}

bool FolderItem::setName(const QString& name)
{
    bool changed = (m_folderData->name() == name);
    m_folderData->setName(name);
    return changed;
}

bool FolderItem::setParentPath(const QString& parentPath)
{
    bool changed = (m_folderData->parentPath() == parentPath);
    m_folderData->setParentPath(parentPath);
    return changed;
}

bool FolderItem::setNoteCnt(const int noteCnt)
{
    bool changed = (m_folderData->noteCnt() == noteCnt);
    m_folderData->setNoteCnt(noteCnt);
    return changed;
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

bool FolderItem::contains(const QString& folderName) const
{
    bool contained = false;
    foreach (FolderItem* item, m_childFolders) {
        contained = (item->m_folderData->name() == folderName);
        if(contained)
            break;
    }

    return contained;
}

bool FolderItem::hasName(const QString& folderName) const
{
    return m_folderData->name() == folderName;
}

bool FolderItem::hasID(int id) const
{
    return m_folderData->id() == id;
}

const FolderData* FolderItem::folderData() const
{
    return m_folderData;
}

