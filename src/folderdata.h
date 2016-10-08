#ifndef FOLDERDATA_H
#define FOLDERDATA_H

#include <QObject>

class FolderData : public QObject
{
    Q_OBJECT
public:
    explicit FolderData(QObject *parent = 0);

    QString name() const;
    void setName(const QString&name);

    QString parentPath() const;
    void setParentPath(const QString& parentPath);

    int noteCnt() const;
    void setNoteCnt(int noteCnt);

    int id() const;
    void setId(int id);

    QString fullPath() const;

    static const QString PathSeparator;

private:
    int m_id;
    QString m_name;
    QString m_parentPath;
    QString m_fullPath;
    int m_noteCnt;

};

#endif // FOLDERDATA_H
