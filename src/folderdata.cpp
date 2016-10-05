#include "folderdata.h"

FolderData::FolderData(QObject *parent) :
    QObject(parent),
    m_name(QStringLiteral("Notes")),
    m_noteCnt(0)
{

}

QString FolderData::name() const
{
    return m_name;
}

void FolderData::setName(const QString&name)
{
    m_name = name;
}

QString FolderData::parentPath() const
{
    return m_parentPath;
}

void FolderData::setParentPath(const QString&parentPath)
{
    m_parentPath = parentPath;
}

int FolderData::noteCnt() const
{
    return m_noteCnt;
}

void FolderData::setNoteCnt(int noteCnt)
{
    m_noteCnt = noteCnt;
}

int FolderData::id() const
{
    return m_id;
}

void FolderData::setId(int id)
{
    m_id = id;
}
