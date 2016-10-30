#include "noteview.h"
#include "notewidgetdelegate.h"
#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <QAbstractItemView>
#include <QPaintEvent>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QScrollBar>

NoteView::NoteView(QWidget *parent)
    : QListView( parent ),
      m_isScrollBarHidden(true),
      m_isAnimationEnabled(true),
      m_isMousePressed(false)
{
    this->setAttribute(Qt::WA_MacShowFocusRect, 0);

    QTimer::singleShot(0, this, SLOT(init()));
}

NoteView::~NoteView()
{
}

void NoteView::animateAddedRow(const QModelIndex& parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(end)

    QModelIndex idx = model()->index(start,0);
    // Note: this line add flikering, seen when the animation runs slow
    selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);

    NoteWidgetDelegate* delegate = static_cast<NoteWidgetDelegate*>(itemDelegate());
    if(delegate != Q_NULLPTR){
        delegate->setState( NoteWidgetDelegate::Insert, idx);

        // TODO find a way to finish this function till the animation stops
        while(delegate->animationState() == QTimeLine::Running){
            qApp->processEvents();
        }
    }
}

/**
 * @brief Reimplemented from QAbstractItemView::rowsInserted().
 */
void NoteView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if(m_isAnimationEnabled && start == end)
        animateAddedRow(parent, start, end);

    QListView::rowsInserted(parent, start, end);
}

/**
 * @brief Reimplemented from QAbstractItemView::rowsAboutToBeRemoved().
 */
void NoteView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    if(start == end){
        NoteWidgetDelegate* delegate = static_cast<NoteWidgetDelegate*>(itemDelegate());
        if(delegate != Q_NULLPTR){
            QModelIndex idx = model()->index(start,0);

            if(m_isAnimationEnabled){
                delegate->setState( NoteWidgetDelegate::Remove, idx);
            }else{
                delegate->setState( NoteWidgetDelegate::Normal, idx);
            }

            // TODO find a way to finish this function till the animation stops
            while(delegate->animationState() == QTimeLine::Running){
                qApp->processEvents();
            }
        }
    }

    QListView::rowsAboutToBeRemoved(parent, start, end);
}

void NoteView::rowsAboutToBeMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                                  const QModelIndex &destinationParent, int destinationRow)
{
    Q_UNUSED(sourceParent)
    Q_UNUSED(sourceEnd)
    Q_UNUSED(destinationParent)
    Q_UNUSED(destinationRow)

    if(model() != Q_NULLPTR){
        QModelIndex idx = model()->index(sourceStart,0);
        NoteWidgetDelegate* delegate = static_cast<NoteWidgetDelegate*>(itemDelegate());
        if(delegate != Q_NULLPTR){
            delegate->setState( NoteWidgetDelegate::MoveOut, idx);

            // TODO find a way to finish this function till the animation stops
            while(delegate->animationState() == QTimeLine::Running){
                qApp->processEvents();
            }
        }
    }
}

void NoteView::rowsMoved(const QModelIndex &parent, int start, int end,
                         const QModelIndex &destination, int row)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(destination)

    QModelIndex idx = model()->index(row,0);
    setCurrentIndex(idx);

    NoteWidgetDelegate* delegate = static_cast<NoteWidgetDelegate*>(itemDelegate());
    if(delegate == Q_NULLPTR)
        return;

    delegate->setState( NoteWidgetDelegate::MoveIn, idx );

    // TODO find a way to finish this function till the animation stops
    while(delegate->animationState() == QTimeLine::Running){
        qApp->processEvents();
    }
}

void NoteView::init()
{
    setMouseTracking(true);
    setUpdatesEnabled(true);
    viewport()->setAttribute(Qt::WA_Hover);

    setupSignalsSlots();
}

void NoteView::mouseMoveEvent(QMouseEvent*e)
{
    if(!m_isMousePressed)
        QListView::mouseMoveEvent(e);
}

void NoteView::mousePressEvent(QMouseEvent*e)
{
    if(e->button() == Qt::LeftButton){
        m_isMousePressed = true;
        QListView::mousePressEvent(e);
    }else{
        m_isMousePressed = false;
    }
}

void NoteView::mouseReleaseEvent(QMouseEvent*e)
{
    m_isMousePressed = false;
    QListView::mouseReleaseEvent(e);
}

bool NoteView::viewportEvent(QEvent*e)
{
    if(model() != Q_NULLPTR){
        switch (e->type()) {
        case QEvent::MouseButtonRelease:{
            QPoint pt = mapFromGlobal(QCursor::pos());
            QModelIndex index = indexAt(QPoint(10, pt.y()));
            if(!index.isValid())
                emit viewportClicked();
            break;
        }
        default:
            break;
        }
    }

    return QListView::viewportEvent(e);
}

void NoteView::resizeEvent(QResizeEvent* e)
{
    if(model() != Q_NULLPTR){
       QSortFilterProxyModel *proxymodel = qobject_cast<QSortFilterProxyModel*> (model());
       proxymodel->layoutChanged();
    }

    QListView::resizeEvent(e);
}

void NoteView::setAnimationEnabled(bool isAnimationEnabled)
{
    m_isAnimationEnabled = isAnimationEnabled;
}

void NoteView::setupSignalsSlots()
{
    // remove/add separator
    // current selectected row changed
    connect(selectionModel(), &QItemSelectionModel::currentRowChanged, [this]
            (const QModelIndex& current, const QModelIndex& previous){

        if(model() != Q_NULLPTR){
            // remove the separator from the row above current row
            if(current.row() > 0){
                QModelIndex aboveCurrentIndex = model()->index(current.row()-1, 0);
                viewport()->update(visualRect(aboveCurrentIndex));
            }

            // restore the separatore to the row above previous row
            if(previous.row() > 0){
                QModelIndex abovePreviousIndex = model()->index(previous.row()-1, 0);
                viewport()->update(visualRect(abovePreviousIndex));
            }
        }
    });

    // row was entered
    connect(this, &NoteView::entered,[this](QModelIndex index){

        if(model() != Q_NULLPTR){
            // restore the separator when moving the mouse down (hovering)
            if(index.row() > 1){
                QModelIndex prevPrevIndex = model()->index(index.row()-2, 0);
                viewport()->update(visualRect(prevPrevIndex));

                QModelIndex prevIndex = model()->index(index.row()-1, 0);
                viewport()->update(visualRect(prevIndex));

            }else if(index.row() > 0){
                QModelIndex prevIndex = model()->index(index.row()-1, 0);
                viewport()->update(visualRect(prevIndex));
            }
        }
    });

    // viewport was entered
    // restore the separator to the row above the last row
    connect(this, &NoteView::viewportEntered,[this](){
        if(model() != Q_NULLPTR && model()->rowCount() > 1){
            QModelIndex lastIndex = model()->index(model()->rowCount()-2, 0);
            viewport()->update(visualRect(lastIndex));
        }
    });
}

