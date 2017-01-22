#include "noteitem.h"
#include "notemodel.h"
#include "tagmodel.h"

#include <QLocale>
#include <QPainter>
#include <QDebug>

const QLocale LOCAL = QLocale("en_US");
const int TOPOFFSETY = 5;
const int SPACEY = 1;
const int BOTTOMOFFSETY = 4;
const int SIDE_OFFSET = 10;
const int BETWEEN_TAG_SPACE_Y = 2;
const int BETWEEN_TAG_SPACE_X = 2;

NoteItem::NoteItem(QModelIndex index, int width, QWidget *parent)
    : QWidget(parent),
      m_noteTitleItem(Q_NULLPTR),
      m_noteDateTimeItem(Q_NULLPTR)
{
    this->setFixedWidth(width);

    Qt::AlignmentFlag textAlignment = Qt::AlignBottom;
    QString title = index.data(NoteModel::NoteFullTitle).toString();
    QString date = parseDateTime(index.data(NoteModel::NoteLastModificationDateTime).toDateTime());

    m_noteTitleItem = new NoteTitleItem(title, textAlignment, this);
    m_noteDateTimeItem = new NoteDateTimeItem(date, textAlignment, this);

    QVariant data = index.data(NoteModel::NoteTagIndexList);
    QList<QPersistentModelIndex> tagsIndexList = data.value<QList<QPersistentModelIndex>>();
    foreach (QPersistentModelIndex tagIndex, tagsIndexList) {
        QString name = tagIndex.data(TagModel::TagName).toString();
        QColor color = tagIndex.data(TagModel::TagColor).value<QColor>();
        color.setAlpha(128);

        NoteTagItem* tagItem = new NoteTagItem(name, color, Qt::AlignCenter, this);
        m_tagList.append(tagItem);
    }
}

void NoteItem::resizeEvent(QResizeEvent* event)
{
    QSize titleInitSize = m_noteTitleItem->initSize();
    titleInitSize.setWidth(event->size().width() - 2*SIDE_OFFSET);
    m_noteTitleItem->setGeometry(QRect(QPoint(SIDE_OFFSET, TOPOFFSETY), titleInitSize));

    int datePosY = m_noteTitleItem->geometry().bottomLeft().y() + SPACEY;
    QSize dateTimeSize = m_noteDateTimeItem->initSize();
    dateTimeSize.setWidth(event->size().width() - 2*SIDE_OFFSET);
    m_noteDateTimeItem->setGeometry(QRect(QPoint(SIDE_OFFSET, datePosY), dateTimeSize));

    int nLine = 0;
    int rowWidth = event->size().width();
    int xAcc = geometry().x();
    int currentNbTagInLine = 0;

    foreach (NoteTagItem* tagItem, m_tagList) {
        int tagRectWidth = tagItem->initSize().width() + 10;
        int futureUsedWidth = xAcc + tagRectWidth + BETWEEN_TAG_SPACE_X * currentNbTagInLine + 2 * SIDE_OFFSET;
        if( futureUsedWidth > rowWidth){
            xAcc = geometry().x();
            ++nLine;
            currentNbTagInLine = 0;
        }else if(xAcc != geometry().x()){
            ++currentNbTagInLine;
        }

        int xRect = xAcc + BETWEEN_TAG_SPACE_X * currentNbTagInLine + SIDE_OFFSET;
        int yRectIn = nLine * (BETWEEN_TAG_SPACE_Y + tagItem->initSize().height());
        int yRect = m_noteDateTimeItem->geometry().bottomLeft().y() + yRectIn;
        int tagRectHeight = tagItem->initSize().height();

        tagItem->setGeometry(xRect, yRect, tagRectWidth, tagRectHeight);

        xAcc += tagRectWidth;
    }
}

QSize NoteItem::sizeHint() const
{
    int height = TOPOFFSETY
                 + m_noteTitleItem->initSize().height()
                 + SPACEY
                 + m_noteDateTimeItem->initSize().height()
                 + BOTTOMOFFSETY;

    int nLine = 0;
    int bottomSpace = 0;
    int rowWidth = this->width();
    int currentNbTagInLine = 0;
    int xAcc = geometry().x();
    int tagHeight = 0;

    foreach (NoteTagItem* tagItem, m_tagList) {
        bottomSpace = 3;
        nLine = nLine == 0 ? 1: nLine;
        tagHeight = tagItem->initSize().height();

        int tagRectWidth = tagItem->initSize().width() + 10;
        int futureUsedWidth = xAcc + tagRectWidth + BETWEEN_TAG_SPACE_X * currentNbTagInLine + 2 * SIDE_OFFSET;
        if( futureUsedWidth > rowWidth){
            ++nLine;
            xAcc = geometry().x();
            currentNbTagInLine = 0;
        }else if(xAcc != geometry().x()){
            ++currentNbTagInLine;
        }

        xAcc += tagRectWidth;
    }

    height += nLine * tagHeight  + (nLine-1 < 0? 0 : nLine-1) * BETWEEN_TAG_SPACE_Y + bottomSpace;

    return QSize(this->width(), height);
}

QString NoteItem::parseDateTime(const QDateTime& dateTime) const
{
    auto currDateTime = QDateTime::currentDateTime();

    if(dateTime.date() == currDateTime.date()){
        return LOCAL.toString(dateTime.time(),"h:mm A");
    }else if(dateTime.daysTo(currDateTime) == 1){
        return tr("Yesterday");
    }else if(dateTime.daysTo(currDateTime) >= 2 &&
             dateTime.daysTo(currDateTime) <= 7){
        return LOCAL.toString(dateTime.date(), "dddd");
    }

    return dateTime.date().toString("M/d/yy");
}
