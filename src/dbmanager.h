#ifndef DBMANAGER_H
#define DBMANAGER_H

#include "notedata.h"
#include "folderdata.h"
#include "tagdata.h"

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QMutexLocker>

class DBManager : public QObject
{
    Q_OBJECT
public:
    explicit DBManager(QObject *parent = 0);
    explicit DBManager(const QString& path, bool doCreate = false, QObject *parent = 0);

    void open(const QString& path, bool doCreate = false);

    bool noteExist(NoteData* note) const;
    bool folderExist(int id) const;

private:
    QSqlDatabase m_db;
    QMutex m_mutex;

    void createTables();

    QList<NoteData*> getAllNotes();
    QList<NoteData*> getAllNotes(const QString& fullPath);
    QList<NoteData*> getNotesInTrash();
    bool addNote(const NoteData*note) const;
    bool removeNote(const NoteData* note) const;
    bool modifyNote(const NoteData* note) const;
    bool migrateNote(const NoteData* note) const;
    bool migrateTrash(const NoteData* note) const;
    int getNotesLastRowID() const;
    bool restoreNote(const NoteData* note) const;

    QList<FolderData*> getAllFolders();
    bool addFolder(FolderData* folder) const;
    bool removeFolder(int id) const;
    bool modifyFolder(const FolderData* folder) const;
    int getFoldersLastRowID() const;

    QList<TagData*> getAllTags();
    bool addTag(const TagData* tag) const;
    bool deleteTagInNote(const int noteId, const QString tagIdStr, const QString tableName) const;
    bool removeTag(const TagData* tag) const;
    void removeTags(const QList<TagData*> tagList) const;
    bool modifyTag(const TagData* tag) const;
    int getTagsLastRowID() const;

signals:
    void notesReceived(QList<NoteData*> noteList);
    void notesInTrashReceived(QList<NoteData*> noteList);
    void foldersReceived(QList<FolderData*> folderList);
    void tagsReceived(QList<TagData*> tagList);
    void tablesLastRowIdReceived(int noteRowId, int tagRowId, int FolderRowId);

public slots:

    void onTablesLastRowIdRequested();

    void onFoldersRequested();
    void onAddFolderRequested(FolderData* folder);
    void onRemoveFolderRequested(const int id);
    void onUpdateFolderRequested(const FolderData* folder);

    void onTagsRequested();
    void onAddTagRequested(const TagData* tag);
    void onRemoveTagRequested(TagData* tag);
    void onRemoveTagsRequested(QList<TagData*> tagList);
    void onUpdateTagRequested(const TagData* tag);

    void onMigrateNoteInTrashResquested(NoteData* note);
    void onMigrateNoteResquested(NoteData* note);
    void onNotesInTrashRequested();
    void onAllNotesRequested();
    void onNotesRequested(const QString& path);
    void onAddNoteRequested(const NoteData* note);
    void onRemoveNoteRequested(NoteData* note);
    void onUpdateNoteRequested(const NoteData* note);
    void onRestoreNoteRequested(const NoteData* note);
};

#endif // DBMANAGER_H
