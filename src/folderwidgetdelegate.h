#ifndef FOLDERWIDGETDELEGATE_H
#define FOLDERWIDGETDELEGATE_H

#include <QStyledItemDelegate>

class FolderWidgetDelegate : public QStyledItemDelegate
{
public:
    explicit FolderWidgetDelegate(QObject* parent = 0);

    QWidget* createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif // FOLDERWIDGETDELEGATE_H
