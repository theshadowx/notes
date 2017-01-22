#include "notedatetimeitem.h"

#include <QLocale>
#include <QPainter>

const QLocale LOCAL = QLocale("en_US");

NoteDateTimeItem::NoteDateTimeItem(QString dateTime, Qt::AlignmentFlag alignment, QWidget *parent)
    : QWidget(parent),
      m_dateTime(dateTime),
      m_alignment(alignment)
{
    QPalette p = palette();
    p.setColor(QPalette::Background, Qt::transparent);
    setAutoFillBackground(true);
    setPalette(p);
}

QSize NoteDateTimeItem::initSize() const
{
    QFontMetrics fmDateTime(font());
    return fmDateTime.boundingRect(m_dateTime).size();
}

void NoteDateTimeItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.drawText(this->rect(), m_alignment, m_dateTime);
}
