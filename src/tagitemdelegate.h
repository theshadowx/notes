#ifndef TAGITEMDELEGATE_H
#define TAGITEMDELEGATE_H

#include <QStyledItemDelegate>

class TagItemDelegate : public QStyledItemDelegate
{

public:
    explicit TagItemDelegate(QObject* parent = Q_NULLPTR);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const Q_DECL_OVERRIDE;

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const Q_DECL_OVERRIDE;
};

#endif // TAGITEMDELEGATE_H
