#ifndef TAGDATA_H
#define TAGDATA_H

#include <QObject>
#include <QColor>

class TagData: public QObject
{
    Q_OBJECT

public:
    explicit TagData(QObject *parent = 0);

    int id() const;
    void setId(int id);

    QString name() const;
    void setName(const QString &name);

    QColor color() const;
    void setColor(const QColor &color);

    int noteCnt() const;
    void setNoteCnt(int noteCnt);

private:
    int m_id;
    QString m_name;
    QColor m_color;
    int m_noteCnt;
};

#endif // TAGDATA_H
