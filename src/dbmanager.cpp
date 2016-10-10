#include "dbmanager.h"
#include <QtSql/QSqlQuery>
#include <QTimeZone>
#include <QDateTime>
#include <QDebug>

DBManager::DBManager(const QString& path, bool doCreate, QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QList<NoteData*> >("QList<NoteData*>");

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);

    if (!m_db.open()){
        qDebug() << "Error: connection with database fail";
    }else{
        qDebug() << "Database: connection ok";
    }

    if(doCreate){
        QSqlQuery query;
        QString active = "CREATE TABLE active_notes ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                         "creation_date INTEGER NOT NULL DEFAULT (0),"
                         "modification_date INTEGER NOT NULL DEFAULT (0),"
                         "deletion_date INTEGER NOT NULL DEFAULT (0),"
                         "content TEXT, "
                         "full_title TEXT,"
                         "full_path TEXT);";

        query.exec(active);

        QString active_index = "CREATE UNIQUE INDEX active_index on active_notes (id ASC);";
        query.exec(active_index);

        QString deleted = "CREATE TABLE deleted_notes ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                          "creation_date INTEGER NOT NULL DEFAULT (0),"
                          "modification_date INTEGER NOT NULL DEFAULT (0),"
                          "deletion_date INTEGER NOT NULL DEFAULT (0),"
                          "content TEXT,"
                          "full_title TEXT,"
                          "full_path TEXT);";
        query.exec(deleted);

        QString folders = "CREATE TABLE folders ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                          "name TEXT,"
                          "parent_path TEXT,"
                          "notes_cnt INTEGER NOT NULL DEFAULT (0));";
        query.exec(folders);

        QString folder_index = "CREATE UNIQUE INDEX folder_index on folder_notes (id ASC);";
        query.exec(folder_index);
    }

}

bool DBManager::noteExist(NoteData* note) const
{
    QSqlQuery query;

    int id = note->id().split('_')[1].toInt();
    QString queryStr = QStringLiteral("SELECT EXISTS(SELECT 1 FROM active_notes WHERE id = %1 LIMIT 1 )")
                       .arg(id);
    query.exec(queryStr);
    query.next();

    return query.value(0).toInt() == 1;
}

QList<NoteData *> DBManager::getAllNotes()
{
    QList<NoteData *> noteList;

    QSqlQuery query;
    query.prepare("SELECT * FROM active_notes");
    bool status = query.exec();
    if(status){
        while(query.next()){
            NoteData* note = new NoteData(this);
            int id =  query.value(0).toInt();
            qint64 epochDateTimeCreation = query.value(1).toLongLong();
            QDateTime dateTimeCreation = QDateTime::fromMSecsSinceEpoch(epochDateTimeCreation, QTimeZone::systemTimeZone());
            qint64 epochDateTimeModification= query.value(2).toLongLong();
            QDateTime dateTimeModification = QDateTime::fromMSecsSinceEpoch(epochDateTimeModification, QTimeZone::systemTimeZone());
            QString content = query.value(4).toString();
            QString fullTitle = query.value(5).toString();
            QString fullPath = query.value(6).toString();

            note->setId(QStringLiteral("noteID_%1").arg(id));
            note->setCreationDateTime(dateTimeCreation);
            note->setLastModificationDateTime(dateTimeModification);
            note->setContent(content);
            note->setFullTitle(fullTitle);
            note->setFullPath(fullPath);

            noteList.push_back(note);
        }

        emit notesReceived(noteList);
    }

    return noteList;
}

QList<NoteData*> DBManager::getAllNotes(const QString& fullPath)
{
    if(fullPath.isEmpty())
        return getAllNotes();

    QList<NoteData *> noteList;

    QSqlQuery query;
    QString queryStr = QStringLiteral("SELECT * FROM active_notes WHERE full_path='%1'")
                       .arg(fullPath);

    bool status = query.exec(queryStr);
    if(status){
        while(query.next()){
            NoteData* note = new NoteData(this);
            int id =  query.value(0).toInt();
            qint64 epochDateTimeCreation = query.value(1).toLongLong();
            QDateTime dateTimeCreation = QDateTime::fromMSecsSinceEpoch(epochDateTimeCreation, QTimeZone::systemTimeZone());
            qint64 epochDateTimeModification= query.value(2).toLongLong();
            QDateTime dateTimeModification = QDateTime::fromMSecsSinceEpoch(epochDateTimeModification, QTimeZone::systemTimeZone());
            QString content = query.value(4).toString();
            QString fullTitle = query.value(5).toString();
            QString fullPath = query.value(6).toString();

            note->setId(QStringLiteral("noteID_%1").arg(id));
            note->setCreationDateTime(dateTimeCreation);
            note->setLastModificationDateTime(dateTimeModification);
            note->setContent(content);
            note->setFullTitle(fullTitle);
            note->setFullPath(fullPath);

            noteList.push_back(note);
        }

        emit notesReceived(noteList);
    }

    return noteList;
}

bool DBManager::addNote(const NoteData* note) const
{
    QSqlQuery query;
    QString emptyStr;

    qint64 epochTimeDateCreated = note->creationDateTime()
                                  .toMSecsSinceEpoch();
    QString content = note->content()
                      .replace("'","''")
                      .replace(QChar('\x0'), emptyStr);
    QString fullTitle = note->fullTitle()
                        .replace("'","''")
                        .replace(QChar('\x0'), emptyStr);
    QString fullPath = note->fullPath();

    QString queryStr = QString("INSERT INTO active_notes (creation_date, modification_date, deletion_date, content, full_title, full_path) "
                               "VALUES (%1, %1, -1, '%2', '%3', '%4');")
                       .arg(epochTimeDateCreated)
                       .arg(content)
                       .arg(fullTitle)
                       .arg(fullPath);

    query.exec(queryStr);
    return (query.numRowsAffected() == 1);

}

bool DBManager::removeNote(const NoteData* note) const
{
    QSqlQuery query;
    QString emptyStr;

    int id = note->id().split('_')[1].toInt();

    QString queryStr = QStringLiteral("DELETE FROM active_notes "
                                      "WHERE id=%1")
                       .arg(id);
    query.exec(queryStr);
    bool removed = (query.numRowsAffected() == 1);

    qint64 epochTimeDateCreated = note->creationDateTime().toMSecsSinceEpoch();
    qint64 epochTimeDateModified = note->lastModificationdateTime().toMSecsSinceEpoch();
    qint64 epochTimeDateDeleted = note->deletionDateTime().toMSecsSinceEpoch();
    QString content = note->content()
                      .replace("'","''")
                      .replace(QChar('\x0'), emptyStr);
    QString fullTitle = note->fullTitle()
                        .replace("'","''")
                        .replace(QChar('\x0'), emptyStr);
    QString fullPath = note->fullPath();

    queryStr = QString("INSERT INTO deleted_notes "
                       "VALUES (%1, %2, %3, %4, '%5', '%6', '%7');")
               .arg(id)
               .arg(epochTimeDateCreated)
               .arg(epochTimeDateModified)
               .arg(epochTimeDateDeleted)
               .arg(content)
               .arg(fullTitle)
               .arg(fullPath);

    query.exec(queryStr);
    bool addedToTrashDB = (query.numRowsAffected() == 1);

    return (removed && addedToTrashDB);
}

bool DBManager::modifyNote(const NoteData* note) const
{
    QSqlQuery query;
    QString emptyStr;

    int id = note->id().split('_')[1].toInt();
    qint64 epochTimeDateModified = note->lastModificationdateTime().toMSecsSinceEpoch();
    QString content = note->content()
                      .replace("'","''")
                      .replace(QChar('\x0'), emptyStr);
    QString fullTitle = note->fullTitle()
                        .replace("'","''")
                        .replace(QChar('\x0'), emptyStr);
    QString fullPath = note->fullPath();

    QString queryStr = QStringLiteral("UPDATE active_notes "
                                      "SET modification_date=%1, content='%2', full_title='%3', full_path='%4' "
                                      "WHERE id=%5")
                       .arg(epochTimeDateModified)
                       .arg(content)
                       .arg(fullTitle)
                       .arg(fullPath)
                       .arg(id);
    query.exec(queryStr);
    return (query.numRowsAffected() == 1);
}

bool DBManager::migrateNote(const NoteData* note) const
{
    QSqlQuery query;
    QString emptyStr;

    int id = note->id().split('_')[1].toInt();
    qint64 epochTimeDateCreated = note->creationDateTime().toMSecsSinceEpoch();
    qint64 epochTimeDateModified = note->lastModificationdateTime().toMSecsSinceEpoch();
    QString content = note->content()
                      .replace("'","''")
                      .replace(QChar('\x0'), emptyStr);
    QString fullTitle = note->fullTitle()
                        .replace("'","''")
                        .replace(QChar('\x0'), emptyStr);

    QString queryStr = QString("INSERT INTO active_notes "
                               "VALUES (%1, %2, %3, -1, '%4', '%5');")
                       .arg(id)
                       .arg(epochTimeDateCreated)
                       .arg(epochTimeDateModified)
                       .arg(content)
                       .arg(fullTitle);

    query.exec(queryStr);
    return (query.numRowsAffected() == 1);
}

bool DBManager::migrateTrash(const NoteData* note) const
{
    QSqlQuery query;
    QString emptyStr;

    int id = note->id().split('_')[1].toInt();
    qint64 epochTimeDateCreated = note->creationDateTime().toMSecsSinceEpoch();
    qint64 epochTimeDateModified = note->lastModificationdateTime().toMSecsSinceEpoch();
    qint64 epochTimeDateDeleted = note->deletionDateTime().toMSecsSinceEpoch();
    QString content = note->content()
                      .replace("'","''")
                      .replace(QChar('\x0'), emptyStr);
    QString fullTitle = note->fullTitle()
                        .replace("'","''")
                        .replace(QChar('\x0'), emptyStr);

    QString queryStr = QString("INSERT INTO deleted_notes "
                       "VALUES (%1, %2, %3, %4, '%5', '%6');")
               .arg(id)
               .arg(epochTimeDateCreated)
               .arg(epochTimeDateModified)
               .arg(epochTimeDateDeleted)
               .arg(content)
               .arg(fullTitle);

    query.exec(queryStr);
    return (query.numRowsAffected() == 1);
}

int DBManager::getLastRowID() const
{
    QSqlQuery query;
    query.exec("SELECT seq from SQLITE_SEQUENCE WHERE name='active_notes';");
    query.next();
    return query.value(0).toInt();
}

QList<FolderData*> DBManager::getAllFolders()
{
    QList<FolderData*> folderList;
    QSqlQuery query;
    query.prepare("SELECT * FROM folders");
    bool status = query.exec();
    if(status){
        while(query.next()){
            FolderData* folder = new FolderData(this);
            int id =  query.value(0).toInt();
            QString folderName = query.value(1).toString();
            QString folderPath = query.value(2).toString();
            int noteCnt = query.value(3).toInt();

            folder->setId(id);
            folder->setName(folderName);
            folder->setParentPath(folderPath);
            folder->setNoteCnt(noteCnt);

            folderList.append(folder);
        }

        emit foldersReceived(folderList);
    }

    return folderList;
}

bool DBManager::addFolder(FolderData* folder) const
{
    QSqlQuery query;

    QString name = folder->name();
    QString parentPath = folder->parentPath();
    int NoteCnt = folder->noteCnt();

    QString queryStr = QString("INSERT INTO folders (name, parent_path, notes_cnt) "
                               "VALUES ('%1', '%2', %3);")
                       .arg(name)
                       .arg(parentPath)
                       .arg(NoteCnt);

    query.exec(queryStr);

    bool isInserted = query.numRowsAffected() == 1;

    int insertedFolderID = isInserted ? query.lastInsertId().toInt() : -1;
    folder->setId(insertedFolderID);

    return isInserted;
}

bool DBManager::removeFolder(const FolderData* folder) const
{
    QSqlQuery query;

    int id = folder->id();
    QString queryStr = QStringLiteral("DELETE FROM folders "
                                      "WHERE id=%1")
                       .arg(id);
    query.exec(queryStr);
    bool isRemoved = (query.numRowsAffected() == 1);

    return isRemoved;
}

bool DBManager::modifyFolder(const FolderData* folder) const
{
    QSqlQuery query;

    int id = folder->id();
    QString name = folder->name();
    QString parentPath = folder->parentPath();
    int NoteCnt = folder->noteCnt();

    QString queryStr = QStringLiteral("UPDATE folders "
                                      "SET name='%1', parent_path='%2', notes_cnt=%3 "
                                      "WHERE id=%4")
                       .arg(name)
                       .arg(parentPath)
                       .arg(NoteCnt)
                       .arg(id);
    query.exec(queryStr);

    return (query.numRowsAffected() == 1);
}

bool DBManager::folderExist(int id) const
{
    QSqlQuery query;
    QString queryStr = QStringLiteral("SELECT EXISTS(SELECT 1 FROM folders WHERE id = %1 LIMIT 1 )")
                       .arg(id);
    query.exec(queryStr);
    query.next();

    return query.value(0).toInt() == 1;
}
