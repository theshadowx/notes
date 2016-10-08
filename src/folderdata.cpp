#include "folderdata.h"

const QString FolderData::PathSeparator = "_";

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

void FolderData::setName(const QString& name)
{
    m_name = name;
    m_fullPath = m_parentPath.isEmpty()? m_name : m_parentPath + PathSeparator + m_name;
}

QString FolderData::parentPath() const
{
    return m_parentPath;
}

void FolderData::setParentPath(const QString&parentPath)
{
    m_parentPath = parentPath;
    m_fullPath = m_parentPath.isEmpty()? m_name : m_parentPath + PathSeparator + m_name;
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

QString FolderData::fullPath() const
{
    return m_fullPath;
}
