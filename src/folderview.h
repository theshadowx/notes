#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include <QTreeView>

class FolderView : public QTreeView
{
    Q_OBJECT
public:
    explicit FolderView(QWidget* parent = 0);

protected:
    void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;
    void paintDropIndicator(QPainter *painter);
    void dragMoveEvent(QDragMoveEvent* e) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent* e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* e) Q_DECL_OVERRIDE;
    QAbstractItemView::DropIndicatorPosition position(const QPoint &pos, const QRect &rect, const QModelIndex &index) const;

private:
    QRect m_dropIndicatorRect;
    bool isParentCollapsed(QModelIndex& index);

signals:
    void noteDropped(const QModelIndex& index, const QString& fullPath);

protected slots:
    void onCollapsed(const QModelIndex& index);
    void onExpended(const QModelIndex& index);
};

#endif // FOLDERTAGVIEW_H
