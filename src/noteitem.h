#ifndef NOTEITEM_H
#define NOTEITEM_H

#include "notetitleitem.h"
#include "notedatetimeitem.h"
#include "notetagitem.h"

#include <QWidget>
#include <QDateTime>
#include <QResizeEvent>

class NoteItem : public QWidget
{
    Q_OBJECT
public:
    explicit NoteItem(QModelIndex index, int width, QWidget *parent = 0);

    QSize sizeHint() const Q_DECL_OVERRIDE;

protected:
    void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

private:
    NoteTitleItem* m_noteTitleItem;
    NoteDateTimeItem* m_noteDateTimeItem;
    QList<NoteTagItem*> m_tagList;


    QString parseDateTime(const QDateTime& dateTime) const;
signals:

public slots:
};

#endif // NOTEITEM_H
