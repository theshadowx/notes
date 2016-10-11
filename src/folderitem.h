#ifndef FOLDERITEM_H
#define FOLDERITEM_H

#include <QObject>
#include <QVariant>
#include "folderdata.h"

class FolderItem : public QObject
{
    Q_OBJECT
public:

    enum class FolderDataEnum{
            ID = Qt::UserRole + 1,
            Name,
            ParentPath,
            FullPath,
            NoteCount
        };

    Q_ENUM(FolderDataEnum)

    explicit FolderItem(FolderData* folderData, QObject *parent = 0);

    int childIndex(int childID) const;
    int childIndex(FolderItem* childItem) const;
    FolderItem* child(int number) const;
    int childCount() const;

    bool appendChild(FolderItem* childFolder);
    bool insertChild(FolderItem* childFolder, int position);
    bool removeChild(int position);

    QVariant data(FolderItem::FolderDataEnum data) const;
    bool setData(FolderDataEnum data, const QVariant &value);

    FolderItem* parentFolder() const;
    void setParentFolder(FolderItem* parentFolder);

    int row();
    bool contains(const QString& folderName) const;
    bool hasName(const QString& folderName) const;
    bool hasID(int id) const;

    const FolderData* folderData() const;

private:
    QList<FolderItem *> m_childFolders;
    FolderData* m_folderData;
    FolderItem* m_parentFolder;

};

#endif // FOLDERITEM_H
