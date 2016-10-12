#include "folderwidgetdelegate.h"
#include <QKeyEvent>
#include <QLineEdit>
#include <QApplication>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>

FolderWidgetDelegate::FolderWidgetDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{

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
