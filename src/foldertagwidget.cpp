#include "foldertagwidget.h"
#include "ui_foldertagwidget.h"

#include "folderitemdelegate.h"
#include "tagitemdelegate.h"

#include <QMenu>
#include <QWidgetAction>
#include <QColorDialog>
#include <QScrollBar>
#include <QDebug>

FolderTagWidget::FolderTagWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FolderTagWidget),
    m_folderCounter(0),
    m_tagCounter(0),
    m_folderType(Normal),
    m_folderModel(new FolderModel(this)),
    m_tagModel(new TagModel(this)),
    m_isFolderModelInitialized(false),
    m_isTagModelInitialized(false),
    m_folderView(Q_NULLPTR),
    m_tagView(Q_NULLPTR),
    m_generalFoldersView(Q_NULLPTR),
    m_addRootFolderButton(Q_NULLPTR),
    m_removeFolderButton(Q_NULLPTR),
    m_addTagButton(Q_NULLPTR),
    m_deleteTagButton(Q_NULLPTR),
    m_clearTagSelectionButton(Q_NULLPTR)
{
    ui->setupUi(this);

    m_folderView = ui->folderView;
    m_tagView = ui->tagView;
    m_addRootFolderButton = ui->addRootFolderButton;
    m_removeFolderButton = ui->removeFolderButton;
    m_addTagButton = ui->addTagButton;
    m_deleteTagButton = ui->deleteTagButton;
    m_clearTagSelectionButton = ui->clearSelectionButton;
    m_generalFoldersView = ui->generalFoldersView;


    m_folderView->setModel(m_folderModel);
    m_folderView->setItemDelegate(new FolderItemDelegate(m_folderView));
    m_folderView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_tagView->setModel(m_tagModel);
    m_tagView->setItemDelegate(new TagItemDelegate(m_tagView));
    m_tagView->setContextMenuPolicy(Qt::CustomContextMenu);

    // add/delete folder button
    connect(m_addRootFolderButton, &QPushButton::clicked, this, &FolderTagWidget::onAddFolderButtonClicked);
    connect(m_removeFolderButton, &QPushButton::clicked, this, &FolderTagWidget::onDeleteFolderButtonClicked);
    // add/delete/select/deselect tag button
    connect(m_addTagButton, &QPushButton::clicked, this, &FolderTagWidget::onAddTagButtonClicked);
    connect(m_deleteTagButton, &QPushButton::clicked, this, &FolderTagWidget::onDeleteTagButtonClicked);
    connect(m_clearTagSelectionButton, &QPushButton::clicked, this, &FolderTagWidget::onClearTagSelectionButtonClicked);
    // folder Model data changed / rows inserted / rows removed
    connect(m_folderModel, &FolderModel::dataChanged, this, &FolderTagWidget::onFolderModelDataChanged);
    connect(m_folderModel, &FolderModel::rowsInserted, this, &FolderTagWidget::onFolderModelRowsInserted);
    connect(m_folderModel, &FolderModel::rowsRemoved, m_folderView, &QTreeView::adjustSize);
    // tag Model / data changed / rows about to be removed / rows removed / rows inserted
    connect(m_tagModel, &TagModel::dataChanged, this, &FolderTagWidget::onTagModelDataChanged);
    connect(m_tagModel, &TagModel::rowsAboutToBeRemoved, this, &FolderTagWidget::onTagModelRowsAboutToBeRemoved);
    connect(m_tagModel, &TagModel::rowsRemoved, this, &FolderTagWidget::onTagModelRowsRemoved);
    connect(m_tagModel, &TagModel::rowsInserted, this, &FolderTagWidget::onTagModelRowsInserted);
    // folderView / folder selected / context menu
    connect(m_folderView, &FolderView::customContextMenuRequested, this, &FolderTagWidget::showFolderViewContextMenu);
    connect(m_folderView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FolderTagWidget::onFolderSelectionChanged);
    connect(m_folderView, &FolderView::noteDropped, this, &FolderTagWidget::noteDropped);
    // tagView contextMenu / selection changed
    connect(m_tagView, &QListView::customContextMenuRequested,this, &FolderTagWidget::showTagViewContextMenu);
    connect(m_tagView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FolderTagWidget::onTagSelectionChanged);
    // general List Widget / All Notes/ Trash
    connect(m_generalFoldersView, &QListWidget::currentRowChanged, this, &FolderTagWidget::onGeneralListWCurrentRowChanged);
    // inner connections
    connect(this, &FolderTagWidget::folderModelInitialized, this, &FolderTagWidget::onInitDone);
    connect(this, &FolderTagWidget::tagModelInitialized, this, &FolderTagWidget::onInitDone);
}

FolderTagWidget::~FolderTagWidget()
{
    delete ui;
}

void FolderTagWidget::initFolderCounter(int folderCounter)
{
    m_folderCounter = folderCounter;
}

void FolderTagWidget::initTagCounter(int tagCounter)
{
    m_tagCounter = tagCounter;
}

QString FolderTagWidget::currentFolderPath() const
{
    return m_currentFolderPath;
}

void FolderTagWidget::addNewFolderIfNotExists()
{
    if(m_folderModel->rowCount() == 0)
        addNewFolder();
}

void FolderTagWidget::initFolders(QList<FolderData*> folderDataList)
{
    // push folders to the treeview
    m_folderModel->setupModelData(folderDataList);

    // create new Folder if no folder exists
    if(folderDataList.isEmpty()){
        ++m_folderCounter;
        FolderData* folderData = new FolderData;
        folderData->setId(m_folderCounter);
        folderData->setName(QStringLiteral("Folder0"));
        FolderItem* folderItem = new FolderItem(folderData, this);
        m_folderModel->insertFolder(folderItem, 0);

        emit folderAdded(folderData);
    }

    m_isFolderModelInitialized = true;
    emit folderModelInitialized(QPrivateSignal());
}

void FolderTagWidget::initTags(QList<TagData*> tagList)
{
    m_tagModel->addTags(tagList);

    m_isTagModelInitialized = true;
    emit tagModelInitialized(QPrivateSignal());
}


void FolderTagWidget::onAddFolderButtonClicked()
{
    QModelIndex index = addNewFolder();
    m_folderView->edit(index);
}

void FolderTagWidget::onDeleteFolderButtonClicked()
{
    deleteFolder();
}

void FolderTagWidget::onAddTagButtonClicked()
{
    QModelIndex index = addNewTag();
    qApp->processEvents();

    QPoint p = m_tagView->visualRect(index).bottomLeft();
    p = m_tagView->mapTo(ui->scrollAreaWidgetContents, p);
    ui->scrollAreaFolderTag->ensureVisible(p.x(), p.y());

    m_tagView->edit(index);
}

QModelIndex FolderTagWidget::addNewFolder(QModelIndex index)
{
    // getting folder data for the new child folder
    // building the item and insert it to the model
    int row = m_folderModel->rowCount(index);
    QString folderName = QStringLiteral("Folder%1").arg(row);
    QString parentPath = m_folderModel->data(index, (int) FolderItem::FolderDataEnum::FullPath).toString();

    ++m_folderCounter;
    FolderData* folderData = new FolderData;
    folderData->setName(folderName);
    folderData->setParentPath(parentPath);
    folderData->setId(m_folderCounter);

    // create folderItem and insert it to the model
    FolderItem* folderItem = new FolderItem(folderData, this);
    m_folderModel->insertFolder(folderItem, row, index);

    m_folderView->expand(index);

    // Set the current index to the newly crealted folder
    QModelIndex newFolderIndex = m_folderModel->index(row,0,index);
    m_folderView->selectionModel()->setCurrentIndex(newFolderIndex, QItemSelectionModel::ClearAndSelect);

    emit folderAdded(folderData);

    return newFolderIndex;
}

void FolderTagWidget::onClearTagSelectionButtonClicked()
{
    m_tagView->clearSelection();
    m_tagView->setCurrentIndex(QModelIndex());
}

void FolderTagWidget::clearTagSelection()
{
    if(m_tagView->selectionModel()->selectedIndexes().count() > 0){
        m_tagView->selectionModel()->blockSignals(true);

        m_tagView->clearSelection();
        m_tagView->setCurrentIndex(QModelIndex());
        m_tagView->update();

        m_tagView->selectionModel()->blockSignals(false);

        emit tagSelectionCleared();
    }
}

bool FolderTagWidget::isTagSelectionEmpty()
{
    return m_tagView->selectionModel()->selectedIndexes().isEmpty();
}

QModelIndex FolderTagWidget::addNewTagInEditMode()
{
    QModelIndex tagAddedIndex = addNewTag();
    qApp->processEvents();

    QPoint p = m_tagView->visualRect(tagAddedIndex).bottomLeft();
    p = m_tagView->mapTo(ui->scrollAreaWidgetContents, p);
    ui->scrollAreaFolderTag->ensureVisible(p.x(), p.y());

    m_tagView->edit(tagAddedIndex);

    return tagAddedIndex;
}

TagModel* FolderTagWidget::tagModel()
{
    return m_tagModel;
}

QModelIndexList FolderTagWidget::tagIndexesFromIds(const QString& tagIdSerial)
{
    QList<QModelIndex> tagIndexes;
    if(tagIdSerial.isEmpty())
        return tagIndexes;

    QStringList tagIdList = tagIdSerial.split(TagData::TagSeparator);
    return m_tagModel->indexesFromIds(tagIdList).values();
}

FolderTagWidget::FolderType FolderTagWidget::folderType() const
{
    return m_folderType;
}

void FolderTagWidget::deleteFolder(QModelIndex index)
{
    if(m_folderModel->rowCount() > 0){
        if(!index.isValid()){
            m_folderView->setFocus();
            index =  m_folderView->selectionModel()->currentIndex();
        }

        int id = index.data((int) FolderItem::FolderDataEnum::ID).toInt();
        m_folderModel->removeFolder(index.row(), m_folderModel->parent(index));

        emit folderRemoved(id);
    }
}

QModelIndex FolderTagWidget::addNewTag()
{
    ++m_tagCounter;
    TagData* tag = new TagData(m_tagModel);
    tag->setId(m_tagCounter);
    int cnt = m_tagModel->rowCount();
    tag->setName(QStringLiteral("Tag%1").arg(cnt));

    QModelIndex index = m_tagModel->addTag(tag);
    qApp->processEvents();

    emit tagAdded(tag);

    return index;
}

void FolderTagWidget::removeTag()
{
    if(m_tagView->selectionModel()->currentIndex().isValid()){
        QModelIndex index = m_tagView->currentIndex();
        TagData* tag = m_tagModel->removeTag(index);

        emit tagRemoved(tag);
        // TODO: Delete Tag
    }
}

void FolderTagWidget::removeTag(const QModelIndex index)
{
    Q_ASSERT_X(index.isValid(), "MainWindow::deleteTag", "index is not valid");

    TagData* tag = m_tagModel->removeTag(index);

    emit tagRemoved(tag);
    // TODO: Delete Tag
}

void FolderTagWidget::removeTags(const QList<QPersistentModelIndex> indexList)
{
    QList<TagData*> removedTagList = m_tagModel->removeTags(indexList);

    emit tagsRemoved(removedTagList);
    // TODO: Delete Tags
}

void FolderTagWidget::onDeleteTagButtonClicked()
{
    QList<QPersistentModelIndex> persistantIndexes;
    QModelIndexList selectedIndexes = m_tagView->selectionModel()->selectedIndexes();

    foreach (QModelIndex index, selectedIndexes){
        persistantIndexes << index;
    }

    removeTags(persistantIndexes);
}

void FolderTagWidget::onGeneralListWCurrentRowChanged(int currentRow)
{
    if(currentRow!=-1){
        m_folderView->setCurrentIndex(QModelIndex());

        //clear tags selection
        clearTagSelection();

        if(currentRow == 0){ // All Notes
            m_folderType = AllNotes;
            emit allNotesFolderSelected();

        }else if(currentRow == 1){ // Trash
            m_folderType = Trash;
            emit trashFolderSelected();
        }
    }
}

void FolderTagWidget::onNoteAdded()
{
    // update the number of note contained in the folder
    QModelIndex folderIndex = m_folderView->currentIndex();
    int noteCnt = folderIndex.data((int) FolderItem::FolderDataEnum::NoteCount).toInt();
    m_folderModel->setData(folderIndex, QVariant::fromValue(++noteCnt), (int) FolderItem::FolderDataEnum::NoteCount);
}

void FolderTagWidget::onNoteRemoved()
{
    // update the number of note contained in the folder
    QModelIndex folderIndex = m_folderView->currentIndex();
    int noteCnt = folderIndex.data((int) FolderItem::FolderDataEnum::NoteCount).toInt();
    m_folderModel->setData(folderIndex, QVariant::fromValue(--noteCnt<0?0:noteCnt), (int) FolderItem::FolderDataEnum::NoteCount);
}

void FolderTagWidget::onTagsInNoteChanged(const QList<QPersistentModelIndex>& tagIndexes, const int noteId)
{
    m_tagModel->updateNoteInTags(tagIndexes, noteId);
}

void FolderTagWidget::onInitDone()
{
    if(m_isFolderModelInitialized && m_isTagModelInitialized){
        // select the first folder in the treeview
        QModelIndex index = m_folderModel->index(0,0);
        m_folderView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
    }
}

void FolderTagWidget::onFolderSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)

    m_currentFolderPath.clear();
    m_folderView->setFocus();

    if(!selected.indexes().isEmpty()){
        m_folderType = Normal;

        QModelIndex selectedFolderIndex = selected.indexes().at(0);

        //clear tags selection
        clearTagSelection();

        // clear the selection in the All Notes/ Trash listWidget
        m_generalFoldersView->clearSelection();
        m_generalFoldersView->setCurrentRow(-1);

        // update the current Folder Path
        m_currentFolderPath = m_folderModel->data(selectedFolderIndex,
                                                  (int) FolderItem::FolderDataEnum::FullPath).toString();

        // check the number of notes contained in the current selected folder
        // if the folder contains notes, grab them from database
        int noteCnt = m_folderModel->data(selectedFolderIndex,
                                           (int) FolderItem::FolderDataEnum::NoteCount).toInt();

        QString folderName = m_folderModel->data(selectedFolderIndex,
                                                 (int) FolderItem::FolderDataEnum::Name).toString();

        emit folderSelected(folderName, m_currentFolderPath, noteCnt);
    }
}

void FolderTagWidget::onFolderModelRowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    m_folderView->adjustSize();
}

void FolderTagWidget::onFolderModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    const FolderData* folderData = m_folderModel->folderData(topLeft);
    emit folderUpdated(folderData);
}

void FolderTagWidget::onTagModelRowsAboutToBeRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(last)
    QModelIndex removedTagIndex = m_tagModel->index(first);
    emit tagAboutToBeRemoved(removedTagIndex);
}

void FolderTagWidget::onTagModelRowsInserted(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    int height =  m_tagView->sizeHintForIndex(m_tagModel->index(0)).height() * m_tagModel->rowCount();
    m_tagView->setFixedHeight(height);
    ui->scrollAreaWidgetContents->updateGeometry();
}

void FolderTagWidget::onTagModelRowsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
    if(m_tagModel->rowCount() > 0){
            int rowHeight = m_tagView->sizeHintForIndex(m_tagModel->index(0)).height();
            int height =  rowHeight * m_tagModel->rowCount();
            m_tagView->setFixedHeight(height);
    }else{
        m_tagView->setFixedHeight(0);
    }
}

void FolderTagWidget::onTagModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    TagData* tag = m_tagModel->tagData(topLeft);
    emit tagUpdated(tag);
}

void FolderTagWidget::showFolderViewContextMenu(const QPoint& pos)
{
    QMenu menu;
    menu.setObjectName(QStringLiteral("folderViewContextMenu"));
    menu.show();
    qApp->processEvents();

    int contentWidth = menu.contentsRect().width() - 2*5;

    QWidgetAction addSubFolderWidgetAction(&menu);
    QPushButton addSubFolderPb;
    addSubFolderPb.setObjectName(QStringLiteral("addSubFolderMenuButton"));
    addSubFolderPb.setText(tr("Add subfolder"));
    addSubFolderPb.setFlat(true);
    addSubFolderPb.setFixedWidth(contentWidth);
    addSubFolderWidgetAction.setDefaultWidget(&addSubFolderPb);
    menu.addAction(&addSubFolderWidgetAction);

    QWidgetAction deleteFolderWidgetAction(&menu);
    QPushButton deleteFolderPb;
    deleteFolderPb.setObjectName(QStringLiteral("deleteFolderMenuButton"));
    deleteFolderPb.setText(tr("Delete folder"));
    deleteFolderPb.setFlat(true);
    deleteFolderPb.setFixedWidth(contentWidth);
    deleteFolderWidgetAction.setDefaultWidget(&deleteFolderPb);
    menu.addAction(&deleteFolderWidgetAction);

    menu.addSeparator();

    QWidgetAction addFolderWidgetAction(&menu);
    QPushButton addFolderPb;
    addFolderPb.setObjectName(QStringLiteral("addFolderMenuButton"));
    addFolderPb.setText(tr("Add folder"));
    addFolderPb.setFlat(true);
    addFolderPb.setFixedWidth(contentWidth);
    addFolderWidgetAction.setDefaultWidget(&addFolderPb);
    menu.addAction(&addFolderWidgetAction);

    connect(&addSubFolderPb, &QPushButton::clicked, [&](){
        QModelIndex index = m_folderView->indexAt(pos);
        menu.hide();

        if(index.isValid()){
            QModelIndex subFolderIndex = addNewFolder(index);
            m_folderView->edit(subFolderIndex);
        }
    });

    connect(&deleteFolderPb, &QPushButton::clicked, [&](){
        QModelIndex index = m_folderView->indexAt(pos);
        menu.hide();

        if(index.isValid())
            deleteFolder(index);
    });

    connect(&addFolderPb, &QPushButton::clicked, [&](){
        menu.hide();
        QModelIndex index = addNewFolder();
        qApp->processEvents();

        QPoint p = m_folderView->visualRect(index).bottomLeft();
        p = m_folderView->mapTo(ui->scrollAreaWidgetContents, p);
        if(!ui->scrollAreaFolderTag->visibleRegion().contains(p))
            ui->scrollAreaFolderTag->ensureVisible(p.x(), p.y());

        m_folderView->edit(index);
    });

    QPoint gPos = m_folderView->mapToGlobal(pos);
    menu.exec(gPos);
}
void FolderTagWidget::showTagViewContextMenu(const QPoint& pos)
{
    QMenu menu;
    menu.setObjectName(QStringLiteral("tagViewContextMenu"));
    menu.show();
    qApp->processEvents();

    int contentWidth = menu.contentsRect().width() - 2*5;

    QWidgetAction changeTagColorWidgetAction(&menu);
    QPushButton changeTagColorPb;
    changeTagColorPb.setEnabled(m_tagModel->rowCount() > 0);
    changeTagColorPb.setObjectName(QStringLiteral("changeColorTagMenuButton"));
    changeTagColorPb.setText(tr("Change color"));
    changeTagColorPb.setFlat(true);
    changeTagColorPb.setFixedWidth(contentWidth);
    changeTagColorWidgetAction.setDefaultWidget(&changeTagColorPb);
    menu.addAction(&changeTagColorWidgetAction);

    QWidgetAction deleteTagWidgetAction(&menu);
    QPushButton deleteTagPb;
    deleteTagPb.setEnabled(m_tagModel->rowCount() > 0);
    deleteTagPb.setObjectName(QStringLiteral("DeleteTagMenuButton"));
    deleteTagPb.setText(tr("Delete tag"));
    deleteTagPb.setFlat(true);
    deleteTagPb.setEnabled(m_tagView->indexAt(pos).isValid());
    deleteTagPb.setFixedWidth(contentWidth);
    deleteTagWidgetAction.setDefaultWidget(&deleteTagPb);
    menu.addAction(&deleteTagWidgetAction);

    menu.addSeparator();

    QWidgetAction clearSelectionWidgetAction(&menu);
    QPushButton clearTagSelectionPb;
    clearTagSelectionPb.setObjectName(QStringLiteral("clearTagSelectionMenuButton"));
    clearTagSelectionPb.setText(tr("Clear selection"));
    clearTagSelectionPb.setFlat(true);
    clearTagSelectionPb.setEnabled(m_tagView->selectionModel()->selectedIndexes().count() > 0);
    clearTagSelectionPb.setFixedWidth(contentWidth);
    clearSelectionWidgetAction.setDefaultWidget(&clearTagSelectionPb);
    menu.addAction(&clearSelectionWidgetAction);

    QWidgetAction addtagWidgetAction(&menu);
    QPushButton addTagPb;
    addTagPb.setObjectName(QStringLiteral("createTagMenuButton"));
    addTagPb.setText(tr("Add new tag"));
    addTagPb.setFlat(true);
    addTagPb.setFixedWidth(contentWidth);
    addtagWidgetAction.setDefaultWidget(&addTagPb);
    menu.addAction(&addtagWidgetAction);

    connect(&changeTagColorPb, &QPushButton::clicked, [&](){
        QModelIndex index = m_tagView->indexAt(pos);
        QColor previousColor = index.data(TagModel::TagColor).value<QColor>();
        QColorDialog d(previousColor);
        int ret = d.exec();
        if(ret > 0){
            QColor color = d.selectedColor();
            if(color != previousColor){
                QVariant v = color;
                m_tagModel->setData(index, v, TagModel::TagColor);
            }
        }
        activateWindow();
        // prevent row background to be colored as if the row was hovered
        QEvent e(QEvent::HoverLeave);
        QApplication::sendEvent(m_tagView->viewport(), &e);
    });

    connect(&deleteTagPb, &QPushButton::clicked, [&](){
        menu.hide();
        QModelIndex index = m_tagView->indexAt(pos);
        removeTag(index);
    });

    connect(&clearTagSelectionPb, &QPushButton::clicked, [&](){
        menu.hide();
        m_tagView->clearSelection();
        m_tagView->setCurrentIndex(QModelIndex());
    });

    connect(&addTagPb, &QPushButton::clicked, [&](){
        menu.hide();
        QModelIndex index = addNewTag();
        qApp->processEvents();

        QPoint p = m_tagView->visualRect(index).bottomLeft();
        p = m_tagView->mapTo(ui->scrollAreaWidgetContents, p);
        ui->scrollAreaFolderTag->ensureVisible(p.x(), p.y());

        m_tagView->edit(index);
    });

    QPoint gPos = m_tagView->mapToGlobal(pos);
    menu.exec(gPos);
}

void FolderTagWidget::onTagSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    QString regexp;
    QModelIndexList selectedIndexes = m_tagView->selectionModel()->selectedIndexes();
    foreach (QModelIndex index, selectedIndexes) {
        int tagId = index.data(TagModel::TagID).toInt();
        regexp.append(QStringLiteral("(?=.*((^(%1)\\b)|^(%1_)|_%1_|(_%1)\\b))").arg(tagId));
    }

    emit tagSelectionChanged(regexp);
}


