#ifndef FOLDERWIDGETDELEGATE_H
#define FOLDERWIDGETDELEGATE_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QTreeView>

class FolderItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit FolderItemDelegate(QObject* parent = Q_NULLPTR);

    QWidget* createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const Q_DECL_OVERRIDE;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;

    void setDraggedOnIndex(const QModelIndex& draggedOnIndex);

private:
    QTreeView* m_view;
    QColor m_lineColor;
    QModelIndex m_draggedOnIndex;
};

#endif // FOLDERWIDGETDELEGATE_H
