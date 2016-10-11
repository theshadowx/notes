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
}

QString FolderData::parentPath() const
{
    return m_parentPath;
}

void FolderData::setParentPath(const QString&parentPath)
{
    m_parentPath = parentPath;
    m_fullPath = m_parentPath.isEmpty()? QString("%1").arg(m_id) : m_parentPath + PathSeparator + QString("%1").arg(m_id);
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
    m_fullPath = m_parentPath.isEmpty()? QString("%1").arg(m_id) : m_parentPath + PathSeparator + QString("%1").arg(m_id);
}

QString FolderData::fullPath() const
{
    return m_fullPath;
}
