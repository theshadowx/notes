#ifndef NOTEDATETIMEITEM_H
#define NOTEDATETIMEITEM_H

#include <QWidget>
#include <QDateTime>

class NoteDateTimeItem : public QWidget
{
    Q_OBJECT
public:
    explicit NoteDateTimeItem(QString dateTime, Qt::AlignmentFlag alignment, QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QString m_dateTime;
    Qt::AlignmentFlag m_alignment;

signals:

public slots:
};

#endif // NOTEDATETIMEITEM_H
