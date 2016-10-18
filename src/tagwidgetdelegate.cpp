#include "tagwidgetdelegate.h"
#include "tagmodel.h"
#include <QPainter>
#include <QColorDialog>
#include <QMouseEvent>
#include <QLineEdit>
#include <QDebug>
#include <QApplication>

TagWidgetDelegate::TagWidgetDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{

}

void TagWidgetDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    double ry = option.rect.height()*50.0/(2.0*100.0);
    double rx = ry;
    double xCenter = option.rect.x() + 8.0 + rx;
    double yCenter = option.rect.y() + option.rect.height()/2.0;

    QColor color = index.data(TagModel::TagColor).value<QColor>();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    painter->setPen(QPen(color));
    painter->setBrush(QBrush(QColor(color)));
    painter->drawEllipse(QPointF(xCenter, yCenter), rx, ry);

    painter->restore();
}

QWidget* TagWidgetDelegate::createEditor(QWidget* parent,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const {

    QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);

    QLineEdit* e = qobject_cast<QLineEdit*>(editor);
    QRegExp rx("[a-zA-Z0-9\-]{1,30}");
    QValidator *validator = new QRegExpValidator(rx, editor);
    e->setValidator(validator);

    return e;
}


bool TagWidgetDelegate::editorEvent(QEvent* event,
                                    QAbstractItemModel* model,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index)
{
    switch (event->type()) {
    case QEvent::MouseButtonRelease:{
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if(mouseEvent->button() == Qt::RightButton){

            QColor color = index.data(TagModel::TagColor).value<QColor>();
            QColorDialog d(color);
            int ret = d.exec();
            if(ret > 0){
                color = d.selectedColor();
                QVariant v = color;
                model->setData(index, v, TagModel::TagColor);
            }

        }
        break;
    }
    default:
        break;
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
