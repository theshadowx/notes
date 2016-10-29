#ifndef NOTEDATA_H
#define NOTEDATA_H

#include <QObject>
#include <QDateTime>

class NoteData : public QObject
{
    Q_OBJECT

    friend class tst_NoteData;

public:
    explicit NoteData(QObject *parent = Q_NULLPTR);

    int id() const;
    void setId(const int &id);

    QString fullTitle() const;
    void setFullTitle(const QString &fullTitle);

    QDateTime lastModificationdateTime() const;
    void setLastModificationDateTime(const QDateTime &lastModificationdateTime);

    QDateTime creationDateTime() const;
    void setCreationDateTime(const QDateTime& creationDateTime);

    QString content() const;
    void setContent(const QString &content);

    bool isModified() const;
    void setModified(bool isModified);

    bool isSelected() const;
    void setSelected(bool isSelected);

    int scrollBarPosition() const;
    void setScrollBarPosition(int scrollBarPosition);

    QDateTime deletionDateTime() const;
    void setDeletionDateTime(const QDateTime& deletionDateTime);

    QString fullPath() const;
    void setFullPath(const QString& fullPath);

    QString tagIdSerial() const;
    void setTagIdSerial(const QString& tagIdSerial);

    bool addTagId(const int id);
    bool removeTagId(const int id);
    bool containsTagId(const int id) const;

    static const QString TagSeparator;

private:
    int m_id;
    QString m_fullTitle;
    QString m_fullPath;
    QString m_tagIdSerial;
    QDateTime m_lastModificationDateTime;
    QDateTime m_creationDateTime;
    QDateTime m_deletionDateTime;
    QString m_content;
    bool m_isModified;
    bool m_isSelected;
    int m_scrollBarPosition;
};


#endif // NOTEDATA_H
