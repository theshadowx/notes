#include "noteitemdelegate.h"
#include "noteitem.h"

#include <QPainter>
#include <QEvent>
#include <QDebug>
#include <QtMath>

const QColor SEPARATOR_COLOR = qRgb(221, 221, 221);
const int MAX_FRAME = 300;
const int SIDE_OFFSET = 10;

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

    QPixmap px;
    NoteItem item(index, m_view->viewport()->width());

    if(index == m_animatedIndex){
        int initHeight = item.sizeHint().height();
        int currentFrame = m_timeLine->currentFrame();

        switch(m_state){
        case MoveIn:{
            double rate = (currentFrame/(MAX_FRAME * 1.0));
            double posY = floor(initHeight * rate);

            opt.rect.setY(posY);
            item.setGeometry(opt.rect);
            px = item.grab();

            break;
        }
        default:
            double rate = ((MAX_FRAME - currentFrame)/(MAX_FRAME * 1.0));
            double posY = opt.rect.y() - ceil(initHeight * rate);

            item.setGeometry(QRect(QPoint(opt.rect.x(), posY), QSize(opt.rect.width(), initHeight)));
            px = item.grab();
            px = px.copy(QRect(QPoint(px.rect().x(), px.rect().bottomRight().y() - opt.rect.height() + 1), px.rect().bottomRight()));

            break;
        }
    }else{
        item.setGeometry(opt.rect);
        px = item.grab();
    }

    paintBackground(painter, opt, index);
    painter->drawPixmap(opt.rect, px);
}

QSize NoteItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    NoteItem item(index, m_view->viewport()->width());

    int initHeight = item.sizeHint().height();
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
            result.setHeight(floor(height));
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

    if(((option.state & QStyle::State_Selected) == QStyle::State_Selected)
            || ((option.state & QStyle::State_MouseOver) == QStyle::State_MouseOver)){

        QStyledItemDelegate::paint(painter, option, index);
    }else if((index.row() !=  selectedIndex.row() - 1)
               && (index.row() !=  hoveredIndex.row() - 1)){

        paintSeparator(painter, option, index);
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
