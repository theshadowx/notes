#ifndef FOLDERITEM_H
#define FOLDERITEM_H

#include <QObject>
#include <QVariant>
#include "folderdata.h"

class FolderItem : public QObject
{
    Q_OBJECT
public:

    explicit FolderItem(FolderData* folderData, QObject *parent = 0);

    int childIndex(int childID) const;
    int childIndex(FolderItem* childItem) const;
    FolderItem* child(int number) const;
    int childCount() const;

    bool appendChild(FolderItem* childFolder);
    bool insertChild(FolderItem* childFolder, int position);
    bool removeChild(int position);

    int id() const;
    QString name() const;
    QString parentPath() const;
    QString fullPath() const;
    int noteCnt() const;

    bool setID(const int id);
    bool setName(const QString& name);
    bool setParentPath(const QString& parentName);
    bool setNoteCnt(const int noteCnt);

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
