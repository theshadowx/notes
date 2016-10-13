#ifndef FOLDERWIDGETDELEGATE_H
#define FOLDERWIDGETDELEGATE_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QTreeView>

class FolderWidgetDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit FolderWidgetDelegate(QObject* parent = Q_NULLPTR);

    QWidget* createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const Q_DECL_OVERRIDE;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) Q_DECL_OVERRIDE;

    void showContextMenu(QAbstractItemModel*model,
                         const QStyleOptionViewItem&option,
                         const QModelIndex&index);

private:
    QTreeView* m_view;

signals:
    void addSubFolderClicked(QModelIndex index);
    void deleteSubFolderButtonClicked(QModelIndex index);
};

#endif // FOLDERWIDGETDELEGATE_H
