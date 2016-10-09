#include "folderwidgetdelegate.h"
#include <QKeyEvent>
#include <QLineEdit>
#include <QDebug>
#include <QApplication>

FolderWidgetDelegate::FolderWidgetDelegate(QObject*parent)
    : QStyledItemDelegate(parent)
{

}

QWidget* FolderWidgetDelegate::createEditor(QWidget *parent,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const {

    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);

    QLineEdit* e = qobject_cast<QLineEdit*>(editor);
    QRegExp rx("[a-zA-Z0-9]{1,12}");
    QValidator *validator = new QRegExpValidator(rx, (QObject*)this);
    e->setValidator(validator);

    return e;
}
