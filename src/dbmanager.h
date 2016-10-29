#ifndef DBMANAGER_H
#define DBMANAGER_H

#include "notedata.h"
#include "folderdata.h"
#include "tagdata.h"

#include <QObject>
#include <QtSql/QSqlDatabase>

class DBManager : public QObject
{
    Q_OBJECT
public:
    explicit DBManager(const QString& path, bool doCreate = false, QObject *parent = 0);

    bool noteExist(NoteData* note) const;
    bool folderExist(int id) const;

private:
    QSqlDatabase m_db;


signals:
    void notesReceived(QList<NoteData*> noteList);
    void notesInTrashReceived(QList<NoteData*> noteList);
    void foldersReceived(QList<FolderData*> folderList);
    void tagsReceived(QList<TagData*> tagList);

public slots:
    QList<NoteData*> getAllNotes();
    QList<NoteData*> getAllNotes(const QString& fullPath);
    QList<NoteData*> getNotesInTrash();
    bool addNote(const NoteData*note) const;
    bool removeNote(const NoteData* note) const;
    bool modifyNote(const NoteData* note) const;
    bool migrateNote(const NoteData* note) const;
    bool migrateTrash(const NoteData* note) const;
    int getNotesLastRowID() const;

    QList<FolderData*> getAllFolders();
    bool addFolder(FolderData* folder) const;
    bool removeFolder(int id) const;
    bool modifyFolder(const FolderData* folder) const;
    int getFoldersLastRowID() const;

    QList<TagData*> getAllTags();
    bool addTag(const TagData* tag) const;
    bool deleteTagInNote(const int noteId, const QString tagIdStr, const QString tableName) const;
    bool removeTag(const TagData* tag) const;
    bool modifyTag(const TagData* tag) const;
    int getTagsLastRowID() const;
};

#endif // DBMANAGER_H
