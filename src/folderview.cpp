#include "folderview.h"
#include "foldermodel.h"
#include "folderitemdelegate.h"

#include <QListView>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QPainter>
#include <QApplication>

FolderView::FolderView(QWidget *parent) : QTreeView(parent)
{
    setAcceptDrops(true);
    connect(this, &FolderView::collapsed, this, &FolderView::onCollapsed, Qt::QueuedConnection);
    connect(this, &FolderView::expanded, this, &FolderView::onExpended, Qt::QueuedConnection);
}

void FolderView::paintEvent(QPaintEvent* e)
{
    if(state() == QAbstractItemView::DraggingState){
        QPainter painter(viewport());
        drawTree(&painter, e->region());
        paintDropIndicator(&painter);
    }else{
        QTreeView::paintEvent(e);
    }
}

void FolderView::paintDropIndicator(QPainter* painter)
{
    if (showDropIndicator() && state() == QAbstractItemView::DraggingState
            && viewport()->cursor().shape() != Qt::ForbiddenCursor) {
        QStyleOption opt;
        opt.init(this);
        opt.rect = m_dropIndicatorRect;
        painter->save();
        painter->setBrush(QBrush(QColor(55, 71, 79, 64)));
        painter->setPen(Qt::NoPen);
        style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemDrop, &opt, painter, this);
        painter->restore();
    }
}

void FolderView::dragMoveEvent(QDragMoveEvent* e)
{
    QModelIndex index = indexAt(e->pos());

    FolderItemDelegate* folderItemDelegate = static_cast<FolderItemDelegate*>(itemDelegate());

    if (index.isValid() && index != currentIndex() && showDropIndicator()){
        QRect rect = visualRect(index);
        QAbstractItemView::DropIndicatorPosition dropIndicatorPosition = position(e->pos(), rect, index);

        if (dropIndicatorPosition == QAbstractItemView::OnItem){
            folderItemDelegate->setDraggedOnIndex(index);
            m_dropIndicatorRect = rect;
            QAbstractItemView::dragMoveEvent(e);
        }else{
            folderItemDelegate->setDraggedOnIndex(QModelIndex());
            m_dropIndicatorRect = QRect();
            viewport()->update();
            e->ignore();
        }
    }else{
        folderItemDelegate->setDraggedOnIndex(QModelIndex());
        m_dropIndicatorRect = QRect();
        viewport()->update();
        e->ignore();
    }
}

void FolderView::dragLeaveEvent(QDragLeaveEvent* e)
{
    FolderItemDelegate* folderItemDelegate = static_cast<FolderItemDelegate*>(itemDelegate());
    folderItemDelegate->setDraggedOnIndex(QModelIndex());

    QTreeView::dragLeaveEvent(e);
}

void FolderView::dropEvent(QDropEvent* e)
{
    QModelIndex dropFolderIndex = indexAt(e->pos());
    QString dropFolderPath = dropFolderIndex.data(FolderModel::FullPath).toString();
    QString currentFolderPath = this->currentIndex().data(FolderModel::FullPath).toString();

    FolderItemDelegate* folderItemDelegate = static_cast<FolderItemDelegate*>(itemDelegate());
    folderItemDelegate->setDraggedOnIndex(QModelIndex());
    setState(QAbstractItemView::NoState);
    viewport()->update();

    if(currentFolderPath != dropFolderPath){
        // extract index of the dropped note
        const QMimeData* mimeData = e->mimeData();
        QByteArray encoded = mimeData->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        QMap<int, QVariant> data;
        int r,c;
        stream >> r >> c >> data;

        QListView* listview = qobject_cast<QListView*>(e->source());
        QSortFilterProxyModel* model = static_cast<QSortFilterProxyModel*>(listview->model());
        QModelIndex noteIndexSrc = model->sourceModel()->index(r,c);
        QModelIndex noteIndex = model->mapFromSource(noteIndexSrc);

        if(noteIndex.isValid()){
            emit noteDropped(noteIndex, dropFolderPath);

            // update the note count
            QModelIndex currentFolderIndex = this->currentIndex();
            if(currentFolderIndex.isValid()){
                int currentFolderCnt = currentFolderIndex.data(FolderModel::NoteCount).toInt();
                this->model()->setData(currentFolderIndex, --currentFolderCnt, FolderModel::NoteCount);
            }

            int cnt = dropFolderIndex.data(FolderModel::NoteCount).toInt();
            this->model()->setData(dropFolderIndex, ++cnt, FolderModel::NoteCount);
        }
    }
}

void FolderView::mouseMoveEvent(QMouseEvent* e)
{
    if(e->buttons() == Qt::NoButton)
        QTreeView::mouseMoveEvent(e);
}

QAbstractItemView::DropIndicatorPosition FolderView::position(const QPoint& pos, const QRect& rect, const QModelIndex& index) const
{
    QAbstractItemView::DropIndicatorPosition r = QAbstractItemView::OnViewport;
    if (!dragDropOverwriteMode()) {
        const int margin = 2;
        if (pos.y() - rect.top() < margin) {
            r = QAbstractItemView::AboveItem;
        } else if (rect.bottom() - pos.y() < margin) {
            r = QAbstractItemView::BelowItem;
        } else if (rect.contains(pos, true)) {
            r = QAbstractItemView::OnItem;
        }
    } else {
        QRect touchingRect = rect;
        touchingRect.adjust(-1, -1, 1, 1);
        if (touchingRect.contains(pos, false)) {
            r = QAbstractItemView::OnItem;
        }
    }

    if (r == QAbstractItemView::OnItem && (!(model()->flags(index) & Qt::ItemIsDropEnabled)))
        r = pos.y() < rect.center().y() ? QAbstractItemView::AboveItem : QAbstractItemView::BelowItem;

    return r;
}

bool FolderView::isParentCollapsed(QModelIndex& index)
{
    QModelIndex parentIndex = index.parent();
    if(parentIndex.isValid()){
        return (isExpanded(parentIndex) ? isParentCollapsed(parentIndex) : true);
    }else{
        return false;
    }
}

void FolderView::onCollapsed(const QModelIndex& index)
{
    QModelIndex currentIndex = this->currentIndex();
    if(isParentCollapsed(currentIndex)){
        while(state() == QAbstractItemView::AnimatingState){
            qApp->processEvents();
        }
        setCurrentIndex(index);
    }
}

void FolderView::onExpended(const QModelIndex& index)
{
    Q_UNUSED(index)
    updateGeometries();
}

