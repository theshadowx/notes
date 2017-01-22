#ifndef NOTETITLEITEM_H
#define NOTETITLEITEM_H

#include <QWidget>

class NoteTitleItem : public QWidget
{
    Q_OBJECT
public:
    explicit NoteTitleItem(QString title, Qt::AlignmentFlag alignment, QWidget *parent = 0);

    QSize initSize() const;

protected:
    void paintEvent(QPaintEvent *event);

private:
    QString m_title;
    Qt::AlignmentFlag m_alignment;

signals:

public slots:
};

#endif // NOTETITLEITEM_H
