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
                         "full_path TEXT,"
                         "tags TEXT);";

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
                          "full_path TEXT,"
                          "tags TEXT);";
        query.exec(deleted);

        QString folders = "CREATE TABLE folders ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                          "name TEXT,"
                          "parent_path TEXT,"
                          "notes_cnt INTEGER NOT NULL DEFAULT (0));";
        query.exec(folders);

        QString folder_index = "CREATE UNIQUE INDEX folder_index on folders (id ASC);";
        query.exec(folder_index);

        QString tags  = "CREATE TABLE tags ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
                        "name TEXT,"
                        "color TEXT,"
                        "notes TEXT);";
        query.exec(tags);

        QString tag_index = "CREATE UNIQUE INDEX tag_index on tags (id ASC);";
        query.exec(tag_index);
    }

}

bool DBManager::noteExist(NoteData* note) const
{
    QSqlQuery query;

    int id = note->id();
    QString queryStr = QStringLiteral("SELECT EXISTS(SELECT 1 FROM active_notes WHERE id = %1 LIMIT 1 )")
                       .arg(id);
    query.exec(queryStr);
    query.next();

    return query.value(0).toInt() == 1;
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

QList<NoteData *> DBManager::getAllNotes()
{
    QList<NoteData *> noteList;

    QSqlQuery query;
    QString queryStr = QStringLiteral("SELECT * FROM active_notes");

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
            QString tags = query.value(7).toString();

            note->setId(id);
            note->setCreationDateTime(dateTimeCreation);
            note->setLastModificationDateTime(dateTimeModification);
            note->setContent(content);
            note->setFullTitle(fullTitle);
            note->setFullPath(fullPath);
            note->setTagIdSerial(tags);

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
            QString tags = query.value(7).toString();

            note->setId(id);
            note->setCreationDateTime(dateTimeCreation);
            note->setLastModificationDateTime(dateTimeModification);
            note->setContent(content);
            note->setFullTitle(fullTitle);
            note->setFullPath(fullPath);
            note->setTagIdSerial(tags);

            noteList.push_back(note);
        }

        emit notesReceived(noteList);
    }

    return noteList;
}

QList<NoteData*> DBManager::getNotesInTrash()
{
    QList<NoteData *> noteList;

    QSqlQuery query;
    query.prepare("SELECT * FROM deleted_notes");
    bool status = query.exec();
    if(status){
        while(query.next()){
            NoteData* note = new NoteData(this);
            int id =  query.value(0).toInt();
            qint64 epochDateTimeCreation = query.value(1).toLongLong();
            QDateTime dateTimeCreation = QDateTime::fromMSecsSinceEpoch(epochDateTimeCreation, QTimeZone::systemTimeZone());
            qint64 epochDateTimeModification= query.value(2).toLongLong();
            QDateTime dateTimeModification = QDateTime::fromMSecsSinceEpoch(epochDateTimeModification, QTimeZone::systemTimeZone());
            qint64 epochDateTimeDeletion= query.value(3).toLongLong();
            QDateTime dateTimeDeletion = QDateTime::fromMSecsSinceEpoch(epochDateTimeDeletion, QTimeZone::systemTimeZone());
            QString content = query.value(4).toString();
            QString fullTitle = query.value(5).toString();
            QString fullPath = query.value(6).toString();
            QString tags = query.value(7).toString();

            note->setId(id);
            note->setCreationDateTime(dateTimeCreation);
            note->setLastModificationDateTime(dateTimeModification);
            note->setDeletionDateTime(dateTimeDeletion);
            note->setContent(content);
            note->setFullTitle(fullTitle);
            note->setFullPath(fullPath);
            note->setTagIdSerial(tags);

            noteList.push_back(note);
        }

        emit notesInTrashReceived(noteList);
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
    QString tags = note->tagIdSerial();

    QString queryStr = QString("INSERT INTO active_notes (creation_date, modification_date, deletion_date, content, full_title, full_path, tags) "
                               "VALUES (%1, %1, -1, '%2', '%3', '%4', '%5');")
                       .arg(epochTimeDateCreated)
                       .arg(content)
                       .arg(fullTitle)
                       .arg(fullPath)
                       .arg(tags);

    query.exec(queryStr);
    return (query.numRowsAffected() == 1);

}

bool DBManager::removeNote(const NoteData* note) const
{
    QSqlQuery query;
    QString emptyStr;

    int id = note->id();

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
    QString tags = note->tagIdSerial();

    queryStr = QString("INSERT INTO deleted_notes "
                       "VALUES (%1, %2, %3, %4, '%5', '%6', '%7', '%8');")
               .arg(id)
               .arg(epochTimeDateCreated)
               .arg(epochTimeDateModified)
               .arg(epochTimeDateDeleted)
               .arg(content)
               .arg(fullTitle)
               .arg(fullPath)
               .arg(tags);

    query.exec(queryStr);
    bool addedToTrashDB = (query.numRowsAffected() == 1);

    return (removed && addedToTrashDB);
}

bool DBManager::modifyNote(const NoteData* note) const
{
    QSqlQuery query;
    QString emptyStr;

    int id = note->id();
    qint64 epochTimeDateModified = note->lastModificationdateTime().toMSecsSinceEpoch();
    QString content = note->content()
                      .replace("'","''")
                      .replace(QChar('\x0'), emptyStr);
    QString fullTitle = note->fullTitle()
                        .replace("'","''")
                        .replace(QChar('\x0'), emptyStr);
    QString fullPath = note->fullPath();
    QString tags = note->tagIdSerial();

    QString queryStr = QStringLiteral("UPDATE active_notes "
                                      "SET modification_date=%1, content='%2', full_title='%3', full_path='%4', tags='%5' "
                                      "WHERE id=%6")
                       .arg(epochTimeDateModified)
                       .arg(content)
                       .arg(fullTitle)
                       .arg(fullPath)
                       .arg(tags)
                       .arg(id);

    query.exec(queryStr);
    return (query.numRowsAffected() == 1);
}

bool DBManager::migrateNote(const NoteData* note) const
{
    QSqlQuery query;
    QString emptyStr;

    int id = note->id();
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

    int id = note->id();
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

int DBManager::getNotesLastRowID() const
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

    QString name = folder->name().replace("'","''");
    QString parentPath = folder->parentPath();
    int NoteCnt = folder->noteCnt();

    QString queryStr = QString("INSERT INTO folders (name, parent_path, notes_cnt) "
                               "VALUES ('%1', '%2', %3);")
                       .arg(name)
                       .arg(parentPath)
                       .arg(NoteCnt);

    query.exec(queryStr);
    return (query.numRowsAffected() == 1);
}

bool DBManager::removeFolder(int id) const
{
    QSqlQuery queryGetNotes;
    QSqlQuery queryDeleteNotes;
    QSqlQuery queryAddTrashNotes;
    QSqlQuery queryDeleteFolders;

    bool success = true;

    // Get all notes having a path that contains the id
    QString queryStr = QStringLiteral("SELECT * FROM active_notes "
                                      "WHERE (full_path LIKE '%%1%') "
                                      "OR (full_path LIKE '%_%1%') "
                                      "OR (full_path LIKE '%_%1_%') "
                                      "OR (full_path LIKE '%_%1%')").arg(id);

    queryGetNotes.exec(queryStr);
    while(queryGetNotes.next()){
        int noteID = queryGetNotes.value(0).toInt();
        qint64 epochDateTimeCreation = queryGetNotes.value(1).toLongLong();
        qint64 epochDateTimeModification = queryGetNotes.value(2).toLongLong();
        qint64 epochTimeDateDeleted = QDateTime::currentDateTime().toMSecsSinceEpoch();
        QString content = queryGetNotes.value(4).toString();
        QString fullTitle = queryGetNotes.value(5).toString();
        QString fullPath = queryGetNotes.value(6).toString();

        // remove each note from active_notes table
        queryStr = QStringLiteral("DELETE FROM active_notes "
                                  "WHERE id=%1")
                   .arg(noteID);

        queryDeleteNotes.exec(queryStr);
        success &= (queryDeleteNotes.numRowsAffected() == 1);

        // add removed note to deleted_notes table
        queryStr = QString("INSERT INTO deleted_notes "
                           "VALUES (%1, %2, %3, %4, '%5', '%6', '%7');")
                   .arg(noteID)
                   .arg(epochDateTimeCreation)
                   .arg(epochDateTimeModification)
                   .arg(epochTimeDateDeleted)
                   .arg(content)
                   .arg(fullTitle)
                   .arg(fullPath);

        queryAddTrashNotes.exec(queryStr);
        success &= (queryAddTrashNotes.numRowsAffected() == 1);
    }

    // delete directory having its id = id
    queryStr = QStringLiteral("DELETE FROM folders "
                              "WHERE id=%1")
               .arg(id);

    queryDeleteFolders.exec(queryStr);
    success &= (queryDeleteFolders.numRowsAffected() == 1);

    // delete sub directories where their parentPath contains id
    queryStr = QStringLiteral("DELETE FROM folders "
                              "WHERE (parent_path LIKE '%%1%') "
                              "OR (parent_path LIKE '%_%1%') "
                              "OR (parent_path LIKE '%_%1_%') "
                              "OR (parent_path LIKE '%_%1%')").arg(id);
    queryDeleteFolders.exec(queryStr);

    return success;
}

bool DBManager::modifyFolder(const FolderData* folder) const
{
    QSqlQuery query;

    int id = folder->id();
    QString name = folder->name().replace("'","''");
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

int DBManager::getFoldersLastRowID() const
{
    QSqlQuery query;
    query.exec("SELECT seq from SQLITE_SEQUENCE WHERE name='folders';");
    query.next();
    return query.value(0).toInt();
}

QList<TagData *> DBManager::getAllTags()
{
    QList<TagData*> tagList;
    QSqlQuery query;
    QString queryStr;

    queryStr = QStringLiteral("SELECT * FROM tags");

    bool status = query.exec(queryStr);
    if(status){
        while(query.next()){
            TagData* tag = new TagData(this);
            int id =  query.value(0).toInt();
            QString tagName = query.value(1).toString();
            QColor tagColor = QColor::fromRgb(query.value(2).toUInt());
            QString notes = query.value(3).toString();

            tag->setId(id);
            tag->setName(tagName);
            tag->setColor(tagColor);
            tag->setNoteIdSerial(notes);

            tagList.append(tag);
        }

        emit tagsReceived(tagList);
    }

    return tagList;
}

bool DBManager::addTag(const TagData *tag) const
{
    QSqlQuery query;

    QString name = tag->name().replace("'","''");
    uint color = QVariant::fromValue(tag->color().rgb()).toUInt();
    QString notes = tag->noteIdSerial();

    QString queryStr = QString("INSERT INTO tags (name, color, notes) "
                               "VALUES ('%1', '%2', '%3');")
                       .arg(name)
                       .arg(color)
                       .arg(notes);

    query.exec(queryStr);
    return (query.numRowsAffected() == 1);
}

bool DBManager::deleteTagInNote(const int noteId, const QString tagIdStr, const QString tableName) const
{
    QSqlQuery selectQuery;
    QSqlQuery updateQuery;

    QString queryStr = QStringLiteral("SELECT tags FROM %1 "
                              "WHERE id=%2")
                       .arg(tableName)
                       .arg(noteId);

    selectQuery.exec(queryStr);
    if(selectQuery.next()){
        // delete tag from active table
        QString notetags = selectQuery.value(0).toString();
        QStringList tagList = notetags.split(TagData::TagSeparator);
        tagList.removeOne(tagIdStr);
        notetags = tagList.join(TagData::TagSeparator);

        queryStr = QStringLiteral("UPDATE %1 "
                                  "SET tags='%2' "
                                  "WHERE id=%3")
                   .arg(tableName)
                   .arg(notetags)
                   .arg(noteId);

        updateQuery.exec(queryStr);
        return (updateQuery.numRowsAffected() == 1);
    }

    return false;
}

bool DBManager::removeTag(const TagData* tag) const
{
    QSqlQuery query;
    QString queryStr;

    int tagToRemoveId = tag->id();
    QString tagToRemoveIdStr = QStringLiteral("%1").arg(tagToRemoveId);
    QString noteIdSerial = tag->noteIdSerial();
    QStringList noteIdList = noteIdSerial.split(TagData::TagSeparator);
    foreach (QString idStr, noteIdList) {
        int noteId = idStr.toInt();

        bool deletedFromActiveNotes = deleteTagInNote(noteId, tagToRemoveIdStr, "active_notes");
        if(!deletedFromActiveNotes)
            deleteTagInNote(noteId, tagToRemoveIdStr, "deleted_notes");
    }

    // delete tag
    queryStr = QStringLiteral("DELETE FROM tags "
                              "WHERE id=%1")
               .arg(tag->id());

    query.exec(queryStr);
    return (query.numRowsAffected() == 1);
}

bool DBManager::modifyTag(const TagData* tag) const
{
    QSqlQuery query;

    int id = tag->id();
    QString name = tag->name().replace("'","''");
    uint color =  QVariant::fromValue(tag->color().rgb()).toUInt();
    QString notes = tag->noteIdSerial();

    QString queryStr = QStringLiteral("UPDATE tags "
                                      "SET name='%1', color='%2', notes='%3' "
                                      "WHERE id=%5")
                       .arg(name)
                       .arg(color)
                       .arg(notes)
                       .arg(id);

    query.exec(queryStr);

    return (query.numRowsAffected() == 1);
}

int DBManager::getTagsLastRowID() const
{
    QSqlQuery query;
    query.exec("SELECT seq from SQLITE_SEQUENCE WHERE name='tags';");
    query.next();
    return query.value(0).toInt();
}

