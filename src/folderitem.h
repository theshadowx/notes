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

    int childIndex(QString childName);
    int childIndex(FolderItem* childItem);
    FolderItem* child(int number);
    int childCount() const;

    bool appendChild(FolderItem* childFolder);
    bool insertChild(FolderItem* childFolder, int position);
    bool removeChild(int position);

    QVariant data(FolderItem::FolderDataEnum data) const;
    bool setData(FolderDataEnum data, const QVariant &value);

    FolderItem* parentFolder() const;
    void setParentFolder(FolderItem* parentFolder);

    int row();
    bool contains(QString folderName);
    bool hasName(QString folderName);

    const FolderData* folderData() const;

private:
    QList<FolderItem *> m_childFolders;
    FolderData* m_folderData;
    FolderItem* m_parentFolder;

};

#endif // FOLDERITEM_H
