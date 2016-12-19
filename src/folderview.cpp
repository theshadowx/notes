#include "folderview.h"

#include "foldermodel.h"

#include <QListView>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
#include <QSortFilterProxyModel>

FolderView::FolderView(QWidget *parent) : QTreeView(parent)
{
    setAcceptDrops(true);
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
