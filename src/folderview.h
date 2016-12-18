#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include <QTreeView>

class FolderView : public QTreeView
{
    Q_OBJECT
public:
    explicit FolderView(QWidget* parent = 0);

protected:
    void dropEvent(QDropEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* e) Q_DECL_OVERRIDE;

signals:
    void noteDropped(const QModelIndex& index, const QString& fullPath);

public slots:
};

#endif // FOLDERTAGVIEW_H
