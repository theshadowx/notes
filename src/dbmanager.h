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

    bool noteExist(NoteData* note);
    bool folderExist(int id);

private:
    QSqlDatabase m_db;


signals:
    void notesReceived(QList<NoteData*> noteList);
    void foldersReceived(QList<FolderData*> folderList);

public slots:
    QList<NoteData*> getAllNotes();
    QList<NoteData*> getAllNotes(QString fullPath);
    bool addNote(NoteData* note);
    bool removeNote(NoteData* note);
    bool modifyNote(NoteData* note);
    bool migrateNote(NoteData* note);
    bool migrateTrash(NoteData* note);
    int getLastRowID();

    QList<FolderData*> getAllFolders();
    bool addFolder(FolderData* folder);
    bool removeFolder(FolderData* folder);
    bool modifyFolder(FolderData* folder);

};

#endif // DBMANAGER_H
