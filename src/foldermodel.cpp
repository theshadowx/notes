#include "foldermodel.h"
#include <QDebug>

FolderModel::FolderModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

QVariant FolderModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();

    FolderItem* item = getFolderItem(index);

    switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
        return item->data(FolderItem::FolderDataEnum::Name);
    default:
        return item->data((FolderItem::FolderDataEnum) role);
    }
}

bool FolderModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid())
        return false;

    FolderItem* item = getFolderItem(index);

    switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
        return item->setData(FolderItem::FolderDataEnum::Name, value);
    default:
        return item->setData((FolderItem::FolderDataEnum) role, value);
    }
}

QModelIndex FolderModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, const_cast<FolderItem*>(m_rootFolders[row]));

    FolderItem *parentFolder = getFolderItem(parent);

    FolderItem *childFolder = parentFolder->child(row);
    if (childFolder)
        return createIndex(row, column, childFolder);
    else
        return QModelIndex();
}

QModelIndex FolderModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    FolderItem *childItem = static_cast<FolderItem*>(index.internalPointer());
    FolderItem *parentItem = childItem->parentFolder();

    if (parentItem == Q_NULLPTR)
        return QModelIndex();

    int parentRow = parentItem->row();
    if(parentRow == -1)
        parentRow = m_rootFolders.indexOf(parentItem);
    return createIndex(parentRow, 0, parentItem);
}

int FolderModel::rowCount(const QModelIndex& parent) const
{
    if(!parent.isValid())
        return m_rootFolders.count();

    FolderItem* parentFolder = getFolderItem(parent);

    return parentFolder->childCount();
}

int FolderModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

Qt::ItemFlags FolderModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool FolderModel::removeFolder(int row, const QModelIndex& parent)
{
    bool success = false;
    FolderItem* parentFolder = getFolderItem(parent);

    beginRemoveRows(parent, row, row);
    success = parentFolder->removeChild(row);
    endRemoveRows();

    return success;
}

QModelIndex FolderModel::parentIndexFromPath(const QString& path, const QModelIndex parentIndex)
{
    if(path.isEmpty())
        return parentIndex;

    QStringList parentFoldersList = path.split(FolderData::PathSeparator);

    QString parentFolderName = parentFoldersList.takeAt(0);
    QString newPath = parentFoldersList.join(FolderData::PathSeparator);

    QModelIndex newParentIndex;

    if(!parentIndex.isValid()){
        int row = 0;
        foreach (FolderItem* folder, m_rootFolders) {
            bool exist =  folder->hasName(parentFolderName);
            if(exist)
                break;

            ++row;
        }

        newParentIndex = index(row, 0);

    }else{
        FolderItem *parentItem = static_cast<FolderItem*>(parentIndex.internalPointer());
        int row = parentItem->childIndex(parentFolderName);
        newParentIndex = index(row, 0, parentIndex);
    }

    return parentIndexFromPath(newPath, newParentIndex);
}

void FolderModel::setupModelData(QList<FolderData*>& folderDataList)
{
    foreach (FolderData* folderData, folderDataList) {
        if(!folderData->parentPath().isEmpty()){
            QModelIndex parentIndex = parentIndexFromPath(folderData->parentPath());

            int row = rowCount(parentIndex);
            FolderItem* newFolderItem = new FolderItem(folderData, this);
            insertFolder(newFolderItem, row, parentIndex);

        }else{
            FolderItem* newFolderItem = new FolderItem(folderData, this);
            int rowCnt = m_rootFolders.count();
            insertFolder(newFolderItem, rowCnt);
        }
    }
}

const FolderData* FolderModel::folderData(const QModelIndex& folderIndex) const
{
    if (folderIndex.isValid()) {
        FolderItem *item = static_cast<FolderItem*>(index.internalPointer());
        if (item != Q_NULLPTR)
            return item->folderData();
    }

    return Q_NULLPTR;
}

bool FolderModel::insertFolder(FolderItem* folderItem, int row, const QModelIndex& parent)
{
    bool success = false;

    if(parent.isValid()){
        FolderItem* parentFolder = getFolderItem(parent);

        if(parentFolder != Q_NULLPTR){
            if(row < parentFolder->childCount()){
                beginInsertRows(parent, row, row);
                success = parentFolder->insertChild(folderItem, row);
                endInsertRows();
            }else{
                beginInsertRows(parent, row, row);
                success = parentFolder->appendChild(folderItem);
                endInsertRows();
            }
        }
    }else{
        beginInsertRows(QModelIndex(), row, row);
        m_rootFolders.append(folderItem);
        success = true;
        endInsertRows();
    }

    return success;
}

FolderItem* FolderModel::getFolderItem(const QModelIndex& index) const
{
    if (index.isValid()) {
        FolderItem *item = static_cast<FolderItem*>(index.internalPointer());
        if (item != Q_NULLPTR)
            return item;
    }

    return Q_NULLPTR;
}

