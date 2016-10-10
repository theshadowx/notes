#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include "folderitem.h"

class FolderModel : public QAbstractItemModel
{
public:
    FolderModel(QObject *parent = Q_NULLPTR);

        QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
        bool setData(const QModelIndex &index, const QVariant &value,
                     int role = Qt::EditRole) Q_DECL_OVERRIDE;

        QModelIndex index(int row, int column,
                          const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
        QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

        int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
        int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

        Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

        bool insertFolder(FolderItem* folderItem, int row, const QModelIndex& parent = QModelIndex());
        bool removeFolder(int row, const QModelIndex &parent = QModelIndex());

        QModelIndex parentIndexFromPath(const QString&path, const QModelIndex parentIndex = QModelIndex());
        void setupModelData(QList<FolderData*>& folderDataList);

        const FolderData* folderData(const QModelIndex& folderIndex) const;

    private:
        FolderItem* getFolderItem(const QModelIndex &index) const;

        QList<FolderItem*> m_rootFolders;
};

#endif // FOLDERMODEL_H
