#ifndef TAGWIDGETDELEGATE_H
#define TAGWIDGETDELEGATE_H

#include <QStyledItemDelegate>

class TagWidgetDelegate : public QStyledItemDelegate
{

public:
    explicit TagWidgetDelegate(QObject* parent = Q_NULLPTR);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const Q_DECL_OVERRIDE;

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const Q_DECL_OVERRIDE;
};

#endif // TAGWIDGETDELEGATE_H
