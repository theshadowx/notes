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

    bool isNoteExist(NoteData* note);

private:
    QSqlDatabase m_db;


signals:
    void notesReceived(QList<NoteData*> noteList);
    void foldersReceived(QList<FolderData*> folderList);

public slots:
    QList<NoteData*> getAllNotes();
    QList<FolderData*> getAllFolders();
    bool addNote(NoteData* note);
    bool removeNote(NoteData* note);
    bool modifyNote(NoteData* note);
    bool migrateNote(NoteData* note);
    bool migrateTrash(NoteData* note);
    int getLastRowID();

};

#endif // DBMANAGER_H
