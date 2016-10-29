#include "tagdata.h"

const QString TagData::TagSeparator = QStringLiteral("_");

TagData::TagData(QObject*parent):
     QObject(parent),
     m_id(-1)
{

}

int TagData::id() const
{
    return m_id;
}

void TagData::setId(int id)
{
    m_id = id;
}

QString TagData::name() const
{
    return m_name;
}

void TagData::setName(const QString &name)
{
    m_name = name;
}

QColor TagData::color() const
{
    return m_color;
}

void TagData::setColor(const QColor &color)
{
    m_color = color;
}

QString TagData::noteIdSerial() const
{
    return m_noteIdSerial;
}

void TagData::setNoteIdSerial(const QString& noteIdSerial)
{
    m_noteIdSerial = noteIdSerial;
}

bool TagData::removeNoteId(const int id)
{
    QString idStr = QStringLiteral("%1").arg(id);

    QStringList idList = m_noteIdSerial.split(TagSeparator);
    if(!idList.contains(idStr))
        return false;

    int index = idList.indexOf(idStr);
    idList.takeAt(index);

    if(!idList.isEmpty()){
        if(idList.count() > 1){
            m_noteIdSerial = idList.join(TagSeparator);
        }else{
            m_noteIdSerial = idList.at(0);
        }
    }else{
        m_noteIdSerial.clear();
    }

    return true;
}

bool TagData::addNoteId(const int id)
{
     QString idStr = QStringLiteral("%1").arg(id);

    if(!m_noteIdSerial.isEmpty()){
        QStringList idList = m_noteIdSerial.split(TagSeparator);
        if(idList.contains(idStr))
            return false;

        idList.append(idStr);
        m_noteIdSerial = idList.join(TagSeparator);
    }else{
        m_noteIdSerial = idStr;
    }

    return true;
}

bool TagData::hasNoteId(const int id) const
{
    QStringList noteIdList = m_noteIdSerial.split(TagSeparator);
    return noteIdList.contains(QStringLiteral("%1").arg(id));
}
