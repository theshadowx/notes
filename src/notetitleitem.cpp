#include "notetitleitem.h"

#include <QFontMetrics>
#include <QPainter>

NoteTitleItem::NoteTitleItem(QString title, Qt::AlignmentFlag alignment, QWidget *parent)
    : QWidget(parent),
      m_title(title),
      m_alignment(alignment)
{
    QPalette p = palette();
    p.setColor(QPalette::Background, Qt::transparent);
    setAutoFillBackground(true);
    setPalette(p);
}

void NoteTitleItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.drawText(this->rect(), m_alignment, m_title);
}
