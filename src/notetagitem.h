#ifndef NOTETAGITEM_H
#define NOTETAGITEM_H

#include <QWidget>

class NoteTagItem : public QWidget
{
    Q_OBJECT

public:
    explicit NoteTagItem(QString text, QColor color, Qt::AlignmentFlag alignment, QWidget *parent = 0);

    QSize initSize() const;

protected:
    void paintEvent(QPaintEvent *event);

private:
    QString m_tagText;
    QColor m_bgColor;
    Qt::AlignmentFlag m_alignment;
};

#endif // NOTETAGITEM_H
