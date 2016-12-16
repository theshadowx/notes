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
    QModelIndex index = indexAt(e->pos());
    QString dropFolderPath = index.data((int)FolderItem::FolderDataEnum::FullPath).toString();
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

        emit noteDropped(noteIndex, dropFolderPath);

        // update the note count
        QModelIndex currentIndex = this->currentIndex();
        int currentFolderCnt = currentIndex.data((int)FolderItem::FolderDataEnum::NoteCount).toInt();
        this->model()->setData(currentIndex, --currentFolderCnt, (int)FolderItem::FolderDataEnum::NoteCount);

        int cnt = index.data((int)FolderItem::FolderDataEnum::NoteCount).toInt();
        this->model()->setData(index, ++cnt, (int)FolderItem::FolderDataEnum::NoteCount);
    }

    QTreeView::dropEvent(e);
}
