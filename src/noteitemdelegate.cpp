#include "noteitemdelegate.h"
#include "noteview.h"
#include <QPainter>
#include <QEvent>
#include <QDebug>
#include <QApplication>
#include <QFontDatabase>
#include <QtMath>
#include "notemodel.h"
#include "tagmodel.h"

#include <iostream>
#include <QScrollBar>

const QFont TITLE_FONT = QFont("Roboto", 10, 60);
const QFont DATE_FONT = QFont("Roboto", 10);
const QFont TAG_FONT = QFont("Roboto", 9);
const QColor TITLE_COLOR = qRgb(26, 26, 26);
const QColor DATE_COLOR = qRgb(132, 132, 132);
const QColor ACTIVE_COLOR = qRgb(175, 212, 228);
const QColor NOT_ACTIVE_COLOR = qRgb(218, 233, 239);
const QColor HOVER_COLOR = qRgb(207, 207, 207);
const QColor APPLICATION_INACTIVE_COLOR = qRgb(207, 207, 207);
const QColor SEPARATOR_COLOR = qRgb(221, 221, 221);
const QColor DEFAULT_COLOR = QColor(Qt::transparent);
const int MAX_FRAME = 300;
const int TAG_HEIGHT = 16;
const int BETWEEN_TAG_SPACE_Y = 2;
const int BETWEEN_TAG_SPACE_X = 2;
const int SIDE_OFFSET = 10;
const int BASIC_HEIGHT = 44;
const QLocale LOCAL = QLocale("en_US");

NoteItemDelegate::NoteItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent),
      m_state(Normal),
      m_isActive(false),
      m_view(qobject_cast<QListView*>(parent))
{
    m_timeLine = new QTimeLine(MAX_FRAME, this);
    m_timeLine->setFrameRange(0,MAX_FRAME);
    m_timeLine->setUpdateInterval(10);
    m_timeLine->setCurveShape(QTimeLine::EaseInCurve);

    connect( m_timeLine, &QTimeLine::frameChanged, [this](){
        emit sizeHintChanged(m_animatedIndex);
    });

    connect(m_timeLine, &QTimeLine::finished, [this](){
        m_animatedIndex = QModelIndex();
        m_state = Normal;
    });
}

void NoteItemDelegate::setState(States NewState, QModelIndex index)
{
    m_animatedIndex = index;

    auto startAnimation = [this](QTimeLine::Direction diretion, int duration){
        m_timeLine->setDirection(diretion);
        m_timeLine->setDuration(duration);
        m_timeLine->start();
    };

    switch ( NewState ){
    case Insert:
        startAnimation(QTimeLine::Forward, MAX_FRAME);
        break;
    case Remove:
    case MoveOut:
    case MoveIn:
        startAnimation(QTimeLine::Backward, MAX_FRAME);
        break;
    case Normal:
        m_animatedIndex = QModelIndex();
        break;
    }

    m_state = NewState;
}

void NoteItemDelegate::setAnimationDuration(const int duration)
{
    m_timeLine->setDuration(duration);
}

void NoteItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;

    switch(m_state){
    case MoveIn:
        if(index == m_animatedIndex){
            int initHeight = initialHeightOfRow(option, index);
            int currentFrame = m_timeLine->currentFrame();
            double rate = (currentFrame/(MAX_FRAME * 1.0));
            double height = initHeight * rate;
            // be careful: this changes also the rectangle height
            opt.rect.setY(floor(height));
        }
        break;
    default:
        break;
    }

    paintBackground(painter, opt, index);
    paintLabels(painter, opt, index);
    paintTags(painter, opt, index);
}

QSize NoteItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int initHeight = initialHeightOfRow(option, index);
    int initWidth = m_view->viewport()->width();

    QSize result = QStyledItemDelegate::sizeHint(option, index);
    if(index == m_animatedIndex){
        switch (m_state) {
        case MoveIn:
            result.setHeight(initHeight);
            break;
        default:
            double rate = m_timeLine->currentFrame()/(MAX_FRAME * 1.0);
            double height = initHeight * rate;
            result.setHeight(int(height));
            break;
        }
    }else{
        result.setHeight(initHeight);
    }

    result.setWidth(initWidth);
    return result;
}

QTimeLine::State NoteItemDelegate::animationState()
{
    return m_timeLine->state();
}

void NoteItemDelegate::paintBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QModelIndex selectedIndex = m_view->selectionModel()->currentIndex();
    QModelIndex hoveredIndex = m_view->indexAt(m_view->mapFromGlobal(QCursor::pos()));

    painter->save();
    if((option.state & QStyle::State_Selected) == QStyle::State_Selected){
        if(qApp->applicationState() == Qt::ApplicationActive){
            if((option.state & QStyle::State_HasFocus) == QStyle::State_HasFocus){
                painter->fillRect(option.rect, QBrush(ACTIVE_COLOR));
            }else{
                painter->fillRect(option.rect, QBrush(NOT_ACTIVE_COLOR));
            }
        }else if(qApp->applicationState() == Qt::ApplicationInactive){
            painter->fillRect(option.rect, QBrush(APPLICATION_INACTIVE_COLOR));
        }
    }else if((option.state & QStyle::State_MouseOver) == QStyle::State_MouseOver){
        painter->fillRect(option.rect, QBrush(HOVER_COLOR));
    }else if((index.row() !=  selectedIndex.row() - 1)
             && (index.row() !=  hoveredIndex.row() - 1)){

        painter->fillRect(option.rect, QBrush(DEFAULT_COLOR));
        paintSeparator(painter, option, index);
    }
    painter->restore();
}

void NoteItemDelegate::paintLabels(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    const int topOffsetY = 5;   // space on top of title
    const int spaceY = 1;       // space between title and date

    QString title{index.data(NoteModel::NoteFullTitle).toString()};
    QFontMetrics fmTitle(TITLE_FONT);
    QRect fmRectTitle = fmTitle.boundingRect(title);

    QString date = parseDateTime(index.data(NoteModel::NoteLastModificationDateTime).toDateTime());
    QFontMetrics fmDate(DATE_FONT);
    QRect fmRectDate = fmDate.boundingRect(title);

    double rowPosX = option.rect.x();
    double rowPosY = option.rect.y();
    double rowWidth = option.rect.width();

    double titleRectPosX = rowPosX + SIDE_OFFSET;
    double titleRectPosY = rowPosY;
    double titleRectWidth = rowWidth - 2 * SIDE_OFFSET;
    double titleRectHeight = fmRectTitle.height() + topOffsetY;

    double dateRectPosX = rowPosX + SIDE_OFFSET;
    double dateRectPosY = rowPosY + titleRectHeight + 1.0;
    double dateRectWidth = rowWidth - 2.0 * SIDE_OFFSET;
    double dateRectHeight = fmRectDate.height() + spaceY;

    Qt::AlignmentFlag textAlignment = Qt::AlignBottom;

    auto drawStr = [painter](double posX, double posY, double width, double height,
                   QColor color, QFont font, QString str, Qt::AlignmentFlag alignment){
        QRectF rect(posX, posY, width, height);
        painter->setPen(color);
        painter->setFont(font);
        painter->drawText(rect, alignment, str);
    };

    // set the bounding Rect of title and date string
    if(index.row() == m_animatedIndex.row()){

        int fullRowHeight = initialHeightOfRow(option, index);
        double rowRate = m_timeLine->currentFrame()/(MAX_FRAME * 1.0);
        double currRowHeight = fullRowHeight * rowRate;

        switch (m_state) {
        case MoveIn:
            textAlignment = Qt::AlignTop;
            // title
            if(ceil(currRowHeight) < fullRowHeight - topOffsetY){
                titleRectPosY = ceil(currRowHeight) + topOffsetY;
                titleRectHeight = fullRowHeight - ceil(currRowHeight) - topOffsetY;

                if(titleRectHeight > topOffsetY + fmRectTitle.height())
                    titleRectHeight = fmRectTitle.height() + topOffsetY;
            }else{
                titleRectHeight = 0;
            }
            // date
            if(ceil(currRowHeight) < fullRowHeight - titleRectHeight - spaceY){
                dateRectPosY = ceil(currRowHeight) + titleRectHeight + spaceY;
                dateRectHeight = fullRowHeight - ceil(currRowHeight) - titleRectHeight - spaceY;

                if(dateRectHeight > fmRectDate.height() + spaceY)
                    dateRectHeight = fmRectDate.height() + spaceY;
            }else{
                dateRectHeight = 0;
            }

            break;

        default:{
            // title
            titleRectHeight = (topOffsetY + fmRectTitle.height()) - (fullRowHeight - currRowHeight) ;
            if(currRowHeight == 0 || titleRectHeight<0)
                titleRectHeight = 0;

            // date
            dateRectPosY = rowPosY + titleRectHeight + 1;
            if(dateRectPosY <= rowPosY + 1 || currRowHeight == 0)
                dateRectPosY = rowPosY;

            if(dateRectPosY == rowPosY){
                double totalLabelHeight = fmRectTitle.height() + topOffsetY + fmRectDate.height() + spaceY + 1;
                double bottomSpace = fullRowHeight - totalLabelHeight;
                dateRectHeight = floor(currRowHeight) == 0 ? 0 : currRowHeight - bottomSpace;
            }
            break;
        }
        }
    }

    // draw title & date
    // TODO: find a way to remove the space between the end of the text and textRect right border when text elided
    title = fmTitle.elidedText(title, Qt::ElideRight, titleRectWidth, Qt::TextSingleLine);
    drawStr(titleRectPosX, titleRectPosY, titleRectWidth, titleRectHeight, TITLE_COLOR, TITLE_FONT, title, textAlignment);
    drawStr(dateRectPosX, dateRectPosY, dateRectWidth, dateRectHeight, DATE_COLOR, DATE_FONT, date, textAlignment);
    painter->restore();
}

void NoteItemDelegate::paintTags(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int nLine = 0;
    int rowWidth = option.rect.width();
    int xAcc = option.rect.x();
    int currentNbTagInLine = 0;

    painter->save();
    painter->setFont(TAG_FONT);
    painter->setRenderHint(QPainter::Antialiasing);

    QFontMetrics fmTag(TAG_FONT);

    QVariant data = index.data(NoteModel::NoteTagIndexList);
    QList<QPersistentModelIndex> tagsIndexList = data.value<QList<QPersistentModelIndex>>();
    foreach (QPersistentModelIndex tagIndex, tagsIndexList) {
        QString name = tagIndex.data(TagModel::TagName).toString();
        QColor color = tagIndex.data(TagModel::TagColor).value<QColor>();
        color.setAlpha(128);

        int tagRectWidth = fmTag.boundingRect(name).width() + 10;
        int futureUsedWidth = xAcc + tagRectWidth + BETWEEN_TAG_SPACE_X * currentNbTagInLine + 2 * SIDE_OFFSET;
        if( futureUsedWidth > rowWidth){
            xAcc = option.rect.x();
            ++nLine;
            currentNbTagInLine = 0;
        }else if(xAcc != option.rect.x()){
            ++currentNbTagInLine;
        }

        int xRect = xAcc + BETWEEN_TAG_SPACE_X * currentNbTagInLine + SIDE_OFFSET;
        int yRect = option.rect.y() + BASIC_HEIGHT + nLine * (BETWEEN_TAG_SPACE_Y + TAG_HEIGHT);
        xAcc += tagRectWidth;

        QRectF bgRectF(xRect, yRect, tagRectWidth, TAG_HEIGHT);
        QRectF textRectF(xRect+4, yRect, tagRectWidth-8, TAG_HEIGHT);

        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(color));
        painter->drawRoundedRect(bgRectF,4,4);

        painter->setPen(Qt::black);
        painter->drawText(textRectF, name);
    }

    painter->restore();
}

void NoteItemDelegate::paintSeparator(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    Q_UNUSED(index)

    painter->setPen(QPen(SEPARATOR_COLOR));
    int posX1 = option.rect.x() + SIDE_OFFSET;
    int posX2 = option.rect.x() + option.rect.width() - SIDE_OFFSET - 1;
    int posY = option.rect.y() + option.rect.height() - 1;

    painter->drawLine(QPoint(posX1, posY),
                      QPoint(posX2, posY));
    painter->restore();
}

int NoteItemDelegate::initialHeightOfRow(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int height = BASIC_HEIGHT;
    int nLine = 0;
    int bottomSpace = 0;
    int rowWidth =  m_view->viewport()->width();
    int currentNbTagInLine = 0;
    int xAcc = option.rect.x();

    QFontMetrics fmTag(TAG_FONT);

    QVariant data = index.data(NoteModel::NoteTagIndexList);
    QList<QPersistentModelIndex> tagsIndexList = data.value<QList<QPersistentModelIndex>>();
    foreach (QPersistentModelIndex tagIndex, tagsIndexList) {
        bottomSpace = 3;
        nLine = nLine == 0 ? 1: nLine;
        QString name = tagIndex.data(TagModel::TagName).toString();

        int tagRectWidth = fmTag.boundingRect(name).width() + 10;
        int futureUsedWidth = xAcc + tagRectWidth + BETWEEN_TAG_SPACE_X * currentNbTagInLine + 2 * SIDE_OFFSET;
        if( futureUsedWidth > rowWidth){
            ++nLine;
            xAcc = option.rect.x();
            currentNbTagInLine = 0;
        }else if(xAcc != option.rect.x()){
            ++currentNbTagInLine;
        }

        xAcc += tagRectWidth;
    }

    height = BASIC_HEIGHT + nLine * (TAG_HEIGHT)  + (nLine-1 < 0? 0 : nLine-1) * BETWEEN_TAG_SPACE_Y + bottomSpace;

    return height;
}

QString NoteItemDelegate::parseDateTime(const QDateTime& dateTime) const
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
