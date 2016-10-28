#include "notedata.h"
#include "tagdata.h"

const QString NoteData::TagSeparator = "_";

NoteData::NoteData(QObject *parent)
    : QObject(parent),
      m_isModified(false),
      m_isSelected(false),
      m_scrollBarPosition(0)
{

}

int NoteData::id() const
{
    return m_id;
}

void NoteData::setId(const int& id)
{
    m_id = id;
}

QString NoteData::fullTitle() const
{
    return m_fullTitle;
}

void NoteData::setFullTitle(const QString &fullTitle)
{
    m_fullTitle = fullTitle;
}

QDateTime NoteData::lastModificationdateTime() const
{
    return m_lastModificationDateTime;
}

void NoteData::setLastModificationDateTime(const QDateTime &lastModificationdateTime)
{
    m_lastModificationDateTime = lastModificationdateTime;
}

QString NoteData::content() const
{
    return m_content;
}

void NoteData::setContent(const QString &content)
{
    m_content = content;
}

bool NoteData::isModified() const
{
    return m_isModified;
}

void NoteData::setModified(bool isModified)
{
    m_isModified = isModified;
}

bool NoteData::isSelected() const
{
    return m_isSelected;
}

void NoteData::setSelected(bool isSelected)
{
    m_isSelected = isSelected;
}

int NoteData::scrollBarPosition() const
{
    return m_scrollBarPosition;
}

void NoteData::setScrollBarPosition(int scrollBarPosition)
{
    m_scrollBarPosition = scrollBarPosition;
}

QDateTime NoteData::deletionDateTime() const
{
    return m_deletionDateTime;
}

void NoteData::setDeletionDateTime(const QDateTime& deletionDateTime)
{
    m_deletionDateTime = deletionDateTime;
}

QString NoteData::fullPath() const
{
    return m_fullPath;
}

void NoteData::setFullPath(const QString& fullPath)
{
    m_fullPath = fullPath;
}

QString NoteData::tagIdSerial() const
{
    return m_tagIdSerial;
}

void NoteData::setTagIdSerial(const QString& tagIdSerial)
{
    m_tagIdSerial = tagIdSerial;
}

bool NoteData::addTagId(const int id)
{
    QString idStr = QStringLiteral("%1").arg(id);

    if(m_tagIdSerial.isEmpty()){
        m_tagIdSerial = idStr;
    }else{
        QStringList idList = m_tagIdSerial.split(TagSeparator);
        if(idList.contains(idStr))
            return false;

        idList.append(idStr);
        m_tagIdSerial = idList.join(TagSeparator);
    }

    return true;
}

bool NoteData::removeTagId(const int id)
{
    QString idStr = QStringLiteral("%1").arg(id);

    QStringList idList = m_tagIdSerial.split(TagSeparator);
    if(!idList.contains(idStr))
        return false;

    int index = idList.indexOf(idStr);
    idList.takeAt(index);

    if(!idList.isEmpty()){
        if(idList.count() > 1){
            m_tagIdSerial = idList.join(TagSeparator);
        }else{
            m_tagIdSerial = idList.at(0);
        }
    }else{
        m_tagIdSerial.clear();
    }

    return true;
}

bool NoteData::containsTagId(const int id) const
{
    QStringList idList = m_tagIdSerial.split(TagSeparator);
    return idList.contains(QStringLiteral("%1").arg(id));
}

QDateTime NoteData::creationDateTime() const
{
    return m_creationDateTime;
}

void NoteData::setCreationDateTime(const QDateTime& creationDateTime)
{
    m_creationDateTime = creationDateTime;
}
