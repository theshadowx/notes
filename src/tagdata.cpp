#include "tagdata.h"

TagData::TagData(QObject*parent):
     QObject(parent),
     m_noteCnt(0),
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

int TagData::noteCnt() const
{
    return m_noteCnt;
}

void TagData::setNoteCnt(int noteCnt)
{
    m_noteCnt = noteCnt;
}
