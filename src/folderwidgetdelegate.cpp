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
    QRegExp rx("[a-zA-Z0-9]{1,12}");
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

bool FolderWidgetDelegate::editorEvent(QEvent* event,
                                       QAbstractItemModel* model,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index)
{

    switch (event->type()) {
    case QEvent::MouseButtonRelease:{
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if(mouseEvent->button() == Qt::RightButton){
            showContextMenu(model, option, index);
        }
        break;
    }
    default:
        break;
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void FolderWidgetDelegate::showContextMenu(QAbstractItemModel* model,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index)
{
    Q_UNUSED(model)
    Q_UNUSED(option)

    QMenu optionMenu;

    QAction addSubFolder(tr("Add Subfolder"),&optionMenu);
    addSubFolder.setIcon(QIcon(QStringLiteral(":/images/newNote_Regular.png")));

    QAction deleteSubFolder(tr("Delete folder"),&optionMenu);
    deleteSubFolder.setIcon(QIcon(QStringLiteral(":/images/trashCan_Regular.png")));

    optionMenu.addAction(&addSubFolder);
    optionMenu.addAction(&deleteSubFolder);
    QAction* selectedAction = optionMenu.exec(QCursor::pos());

    if(selectedAction == &addSubFolder){
        emit addSubFolderClicked(index);
    }else if(selectedAction == &deleteSubFolder){
        emit deleteSubFolderButtonClicked(index);
    }
}
