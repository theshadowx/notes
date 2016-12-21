#include "folderview.h"
#include "foldermodel.h"

#include <QListView>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QPainter>

FolderView::FolderView(QWidget *parent) : QTreeView(parent)
{
    setAcceptDrops(true);
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
    if (index.isValid() && index != currentIndex() && showDropIndicator()){
        QRect rect = visualRect(index);
        QAbstractItemView::DropIndicatorPosition dropIndicatorPosition = position(e->pos(), rect, index);

        if (dropIndicatorPosition == QAbstractItemView::OnItem){
            m_dropIndicatorRect = rect;
            QAbstractItemView::dragMoveEvent(e);
        }else{
            m_dropIndicatorRect = QRect();
            viewport()->update();
            e->ignore();
        }
    }else{
        m_dropIndicatorRect = QRect();
        viewport()->update();
        e->ignore();
    }
}

void FolderView::dropEvent(QDropEvent* e)
{
    QModelIndex dropFolderIndex = indexAt(e->pos());
    QString dropFolderPath = dropFolderIndex.data((int)FolderItem::FolderDataEnum::FullPath).toString();
    QString currentFolderPath = this->currentIndex().data((int)FolderItem::FolderDataEnum::FullPath).toString();

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
                int currentFolderCnt = currentFolderIndex.data((int)FolderItem::FolderDataEnum::NoteCount).toInt();
                this->model()->setData(currentFolderIndex, --currentFolderCnt, (int)FolderItem::FolderDataEnum::NoteCount);
            }

            int cnt = dropFolderIndex.data((int)FolderItem::FolderDataEnum::NoteCount).toInt();
            this->model()->setData(dropFolderIndex, ++cnt, (int)FolderItem::FolderDataEnum::NoteCount);
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
