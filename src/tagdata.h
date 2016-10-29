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

    QString noteIdSerial() const;
    void setNoteIdSerial(const QString& noteIdSerial);

    bool removeNoteId(const int id);
    bool addNoteId(const int id);
    bool hasNoteId(const int id) const;

    static const QString TagSeparator;

private:
    int m_id;
    QString m_name;
    QColor m_color;
    QString m_noteIdSerial;
};

#endif // TAGDATA_H
