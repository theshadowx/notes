#ifndef FOLDERTAGWIDGET_H
#define FOLDERTAGWIDGET_H

#include <QWidget>
#include <QListView>
#include <QListWidget>
#include <QPushButton>

#include "foldermodel.h"
#include "tagmodel.h"
#include "folderview.h"

namespace Ui {
    class FolderTagWidget;
    }

class FolderTagWidget : public QWidget
{
    Q_OBJECT

public:
    enum FolderType{
        Normal=0,
        AllNotes,
        Trash
        };

    Q_ENUM(FolderType)

    explicit FolderTagWidget(QWidget *parent = 0);
    ~FolderTagWidget();

    void initFolderCounter(int folderCounter);
    void initTagCounter(int tagCounter);

    QString currentFolderPath() const;

    void addNewFolderIfNotExists();
    void clearTagSelection();
    bool isTagSelectionEmpty();
    QModelIndex addNewTagInEditMode();

    TagModel* tagModel();
    QModelIndexList tagIndexesFromIds(const QString& tagIdSerial);

    FolderType folderType() const;

private:
    Ui::FolderTagWidget *ui;

    int m_folderCounter;
    int m_tagCounter;
    QString m_currentFolderPath;
    FolderType m_folderType;

    FolderModel* m_folderModel;
    TagModel* m_tagModel;
    FolderView* m_folderView;

    bool m_isFolderModelInitialized;
    bool m_isTagModelInitialized;

    QListView* m_tagView;
    QListWidget* m_generalFoldersView;
    QPushButton* m_addRootFolderButton;
    QPushButton* m_removeFolderButton;
    QPushButton* m_addTagButton;
    QPushButton* m_deleteTagButton;
    QPushButton* m_clearTagSelectionButton;

    QModelIndex addNewFolder(QModelIndex index = QModelIndex());
    void deleteFolder(QModelIndex index = QModelIndex());
    QModelIndex addNewTag();
    void removeTag();
    void removeTag(const QModelIndex index);
    void removeTags(const QList<QPersistentModelIndex> indexList);


public slots:

    void initFolders(QList<FolderData*> folderDataList);
    void initTags(QList<TagData*> tagList);

    void onAddFolderButtonClicked();
    void onDeleteFolderButtonClicked();
    void onAddTagButtonClicked();
    void onDeleteTagButtonClicked();
    void onClearTagSelectionButtonClicked();

    void onGeneralListWCurrentRowChanged(int currentRow);

    void onFolderSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onFolderModelRowsInserted(const QModelIndex &parent, int first, int last);
    void onFolderModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int> ());
    void showFolderViewContextMenu(const QPoint &pos);

    void onTagModelRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last);
    void onTagModelRowsInserted(const QModelIndex &parent, int first, int last);
    void onTagModelRowsRemoved(const QModelIndex &parent, int first, int last);
    void onTagModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int> ());
    void onTagSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void showTagViewContextMenu(const QPoint &pos);

    void onNoteAdded();
    void onNoteRemoved();
    void onTagsInNoteChanged(const QList<QPersistentModelIndex>& tagIndexes, const int noteId);

    void onInitDone();

Q_SIGNALS:
    void folderAdded(FolderData* folder);
    void folderRemoved(const int id);
    void folderUpdated(const FolderData* folder);
    void noteDropped(const QModelIndex& index, const QString& fullPath);

    void folderSelected(const QString& folderName, const QString& folderPath, const int noteCnt);
    void allNotesFolderSelected();
    void trashFolderSelected();

    void tagAdded(TagData* tag);
    void tagAboutToBeRemoved(QModelIndex index);
    void tagRemoved(TagData* tag);
    void tagsRemoved(QList<TagData*> tagList);
    void tagUpdated(const TagData* tag);

    void tagSelectionChanged(const QString& filterRegexpString);
    void tagSelectionCleared();

    void tagModelInitialized(QPrivateSignal);
    void folderModelInitialized(QPrivateSignal);

};

#endif // FOLDERTAGWIDGET_H
