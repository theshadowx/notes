#include "folderwidgetdelegate.h"
#include <QKeyEvent>
#include <QLineEdit>
#include <QApplication>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QDebug>
#include "foldermodel.h"

FolderWidgetDelegate::FolderWidgetDelegate(QObject* parent)
    : QStyledItemDelegate(parent),
      m_lineColor(QColor(221, 221, 221))
{
    m_view = qobject_cast<QTreeView*>(parent);
}

QWidget* FolderWidgetDelegate::createEditor(QWidget *parent,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const {

    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);

    QLineEdit* e = qobject_cast<QLineEdit*>(editor);
    QRegExp rx("^(?!\\s*$).{1,30}");
    QValidator *validator = new QRegExpValidator(rx, editor);
    e->setValidator(validator);

    return e;
}

void FolderWidgetDelegate::paint(QPainter* painter,
                                 const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    QModelIndex indexBelow = m_view->indexBelow(index);
    bool isBelowSelected = m_view->selectionModel()->isSelected(indexBelow);
    bool isSelected = (option.state & QStyle::State_Selected) == QStyle::State_Selected;

    if(!isSelected && !isBelowSelected){
        QPoint bottomLeft = option.rect.bottomLeft();
        QPoint bottomRight = QPoint(m_view->width(), bottomLeft.y());

        QPen pen(m_lineColor);
        pen.setWidth(1);
        painter->save();
        painter->setPen(pen);
        painter->drawLine(bottomLeft, bottomRight);
        painter->restore();
    }
}
