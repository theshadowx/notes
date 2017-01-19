#include "notetagitem.h"

#include <QFontMetrics>
#include <QStylePainter>
#include <QDebug>

NoteTagItem::NoteTagItem(QString text, QColor color, Qt::AlignmentFlag alignment, QWidget *parent) :
    QWidget(parent),
    m_tagText(text),
    m_bgColor(color),
    m_alignment(alignment)
{
    QPalette p = palette();
    p.setColor(QPalette::Background, Qt::transparent);
    setAutoFillBackground(true);
    setPalette(p);
}

void NoteTagItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(m_bgColor));
    painter.drawRoundedRect(rect(),4,4);
    painter.restore();

    painter.drawText(rect(), m_alignment | Qt::AlignHCenter, m_tagText);
}
