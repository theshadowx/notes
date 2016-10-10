#ifndef DBMANAGER_H
#define DBMANAGER_H

#include "notedata.h"
#include "folderdata.h"

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
    void foldersReceived(QList<FolderData*> folderList);

public slots:
    QList<NoteData*> getAllNotes();
    QList<NoteData*> getAllNotes(const QString& fullPath);
    bool addNote(const NoteData*note) const;
    bool removeNote(const NoteData* note) const;
    bool modifyNote(const NoteData* note) const;
    bool migrateNote(const NoteData* note) const;
    bool migrateTrash(const NoteData* note) const;
    int getLastRowID() const;

    QList<FolderData*> getAllFolders();
    bool addFolder(FolderData*folder) const;
    bool removeFolder(const FolderData* folder) const;
    bool modifyFolder(const FolderData*folder) const;

};

#endif // DBMANAGER_H
