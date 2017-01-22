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

QSize NoteTitleItem::initSize() const
{
    QFontMetrics fmTitle(font());
    return fmTitle.boundingRect(m_title).size();
}

void NoteTitleItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QFontMetrics fmTitle(font());
    QString title = fmTitle.elidedText(m_title, Qt::ElideRight, rect().width(), Qt::TextSingleLine);

    QPainter painter(this);
    painter.drawText(this->rect(), m_alignment, title);
}
