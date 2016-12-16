#include "notewidget.h"
#include "ui_notewidget.h"

#include "noteitemdelegate.h"
#include "tagnotemodel.h"

#include <QTextStream>
#include <QMenu>
#include <QWidgetAction>
#include <QDebug>

const int FIRST_LINE_MAX=80;

NoteWidget::NoteWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NoteWidget),
    m_autoSaveTimer(new QTimer(this)),
    m_clearSearchButton(Q_NULLPTR),
    m_addNoteButton(Q_NULLPTR),
    m_deleteNoteButton(Q_NULLPTR),
    m_tagNoteButton(Q_NULLPTR),
    m_searchField(Q_NULLPTR),
    m_noteModel(new NoteModel(this)),
    m_deletedNotesModel(new NoteModel(this)),
    m_proxyNoteModel(new QSortFilterProxyModel(this)),
    m_isNoteDeletionEnabled(true),
    m_isAddingNoteEnabled(true),
    m_isTempNoteExist(false),
    m_isOperationRunning(false),
    m_doSaveImmediately(false),
    m_noteCounter(0)
{
    ui->setupUi(this);

    setupNoteWidget();
    setupSearchField();
    setupModelView();
    setupSignalsSlots();
}

NoteWidget::~NoteWidget()
{
    delete ui;
}

void NoteWidget::setupNoteWidget()
{
    m_addNoteButton = ui->addNoteButton;
    m_deleteNoteButton = ui->deleteNoteButton;
    m_tagNoteButton = ui->tagNoteButton;
    m_searchField = ui->lineEdit;
}

void NoteWidget::setupModelView()
{
    m_noteView = static_cast<NoteView*>(ui->listView);
    m_proxyNoteModel->setSourceModel(m_noteModel);
    m_proxyNoteModel->setFilterKeyColumn(0);
    m_proxyNoteModel->setFilterRole(NoteModel::NoteContent);
    m_proxyNoteModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_noteView->setItemDelegate(new NoteItemDelegate(m_noteView));
    m_noteView->setModel(m_proxyNoteModel);
}

void NoteWidget::setupSearchField ()
{
    // clear button
    m_clearSearchButton = new QToolButton(m_searchField);
    QPixmap pixmap(":images/closeButton.png");
    m_clearSearchButton->setIcon(QIcon(pixmap));
    QSize clearSize(15, 15);
    m_clearSearchButton->setIconSize(clearSize);
    m_clearSearchButton->setCursor(Qt::ArrowCursor);
    m_clearSearchButton->hide();

    // search button
    QToolButton *searchButton = new QToolButton(m_searchField);
    QPixmap newPixmap(":images/magnifyingGlass.png");
    searchButton->setIcon(QIcon(newPixmap));
    QSize searchSize(24, 25);
    searchButton->setIconSize(searchSize);
    searchButton->setCursor(Qt::ArrowCursor);

    // layout
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::RightToLeft, m_searchField);
    layout->setContentsMargins(0,0,3,0);
    layout->addWidget(m_clearSearchButton);
    layout->addStretch();
    layout->addWidget(searchButton);
     m_searchField->setLayout(layout);
}

void NoteWidget::setupSignalsSlots()
{
    // add note button / delete note button / tag note button
    connect(m_addNoteButton, &QPushButton::clicked, this, &NoteWidget::onAddNoteButtonClicked);
    connect(m_deleteNoteButton, &QPushButton::clicked, this, &NoteWidget::onRemoveNoteButtonClicked);
    connect(m_tagNoteButton, &QPushButton::clicked, this, &NoteWidget::showTagNoteMenu);
    // Search Field text changed
    connect(m_searchField, &QLineEdit::textChanged, this, &NoteWidget::onSearchFieldTextChanged);
    // noteView / viewport clicked / note clicked
    connect(m_noteView, &NoteView::viewportClicked, this, &NoteWidget::onNoteViewViewportClicked);
    connect(m_noteView, &NoteView::pressed, this, &NoteWidget::onNotePressed);
    // Update note count label
    connect(m_proxyNoteModel, &QSortFilterProxyModel::rowsInserted, this, &NoteWidget::updateNoteCountLabel);
    connect(m_proxyNoteModel, &QSortFilterProxyModel::rowsRemoved, this, &NoteWidget::updateNoteCountLabel);
    connect(m_proxyNoteModel, &QSortFilterProxyModel::modelReset, this, &NoteWidget::updateNoteCountLabel);
    // note model / data changed / rows about to be removed / rows removed / rows moved
    connect(m_noteModel, &NoteModel::dataChanged, this, &NoteWidget::onNoteDataChanged);
    connect(m_noteModel, &NoteModel::rowsAboutToBeMoved, m_noteView, &NoteView::rowsAboutToBeMoved);
    connect(m_noteModel, &NoteModel::rowsRemoved, this, &NoteWidget::onNoteModelRowsRemoved);
    connect(m_noteModel, &NoteModel::rowsMoved, m_noteView, &NoteView::rowsMoved);
    // clear button
    connect(m_clearSearchButton, &QToolButton::clicked, this, &NoteWidget::onClearSearchButtonClicked);
    // auto save timer
    connect(m_autoSaveTimer, &QTimer::timeout, this, &NoteWidget::onNoteChangedTimeoutTriggered);
}

void NoteWidget::setNoteScrollBarPosition(QModelIndex index, int pos)
{
    Q_ASSERT_X(index.isValid(), "NoteWidget::saveNoteTextPosition", "index is not valid");
    Q_ASSERT_X(index.model() == m_noteModel, "NoteWidget::saveNoteTextPosition", "index model must be NoteModel");

    m_noteModel->setData(index, QVariant::fromValue(pos), NoteModel::NoteScrollbarPos);
}

void NoteWidget::setNoteText(const QString& text)
{
    QString content = m_currentSelectedNoteProxy.data(NoteModel::NoteContent).toString();
    if(text != content){

        // Get the new data
        QString firstline = getFirstLine(text);
        QDateTime dateTime = QDateTime::currentDateTime();

        // update model
        QMap<int, QVariant> dataValue;
        dataValue[NoteModel::NoteContent] = QVariant::fromValue(text);
        dataValue[NoteModel::NoteFullTitle] = QVariant::fromValue(firstline);
        dataValue[NoteModel::NoteLastModificationDateTime] = QVariant::fromValue(dateTime);


        // move note to the top of the list
        if(m_currentSelectedNoteProxy.row() != 0)
            moveNoteToTop();

        QModelIndex index = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
        m_noteModel->setItemData(index, dataValue);
    }
}

void NoteWidget::setNoteTagIndexes(QModelIndex index, QList<QPersistentModelIndex>& tagIndexes)
{
   m_noteModel->setData(index, QVariant::fromValue(tagIndexes), NoteModel::NoteTagIndexList);
}

void NoteWidget::addTagIndexesToNote(const int noteId, QModelIndexList& tagIndexes)
{
    m_noteModel->addTagIndexes(noteId, tagIndexes);
}

void NoteWidget::removeTagFromNotes(QModelIndex index)
{
    if(index.isValid())
        m_noteModel->removeTagIndex(index);
}

void NoteWidget::initNoteCounter(int count)
{
    m_noteCounter = count;
}

void NoteWidget::reset()
{
    m_noteView->setAnimationEnabled(false);
    clearSearch();
    m_noteView->setAnimationEnabled(true);

    m_noteModel->clearNotes();
    m_currentSelectedNoteProxy = QModelIndex();
    m_selectedNoteBeforeSearchingInSource = QModelIndex();
    m_isTempNoteExist = false;

    m_currentFolderPath.clear();

    m_noteView->setFocus();
}

void NoteWidget::prepareForTextEdition()
{
    bool isNoteListFilteredByTag = !m_proxyNoteModel->filterRegExp().pattern().isEmpty();
    bool isNoteListFilteredByKeyword = !m_searchField->text().isEmpty();

    auto resetProxyModel = [&](){
        m_noteView->setAnimationEnabled(false);
        if(isNoteListFilteredByKeyword){
            clearSearch();
        }else if(isNoteListFilteredByTag){
            clearFilter();
        }
        m_noteView->setAnimationEnabled(true);
    };

    if(isNoteListFilteredByTag || isNoteListFilteredByKeyword){
        m_selectedNoteBeforeSearchingInSource = QModelIndex();

        if(m_currentSelectedNoteProxy.isValid()){
            QModelIndex indexInSource = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
            resetProxyModel();
            m_currentSelectedNoteProxy = m_proxyNoteModel->mapFromSource(indexInSource);
            selectNote(m_currentSelectedNoteProxy);

        }else{
            resetProxyModel();

            if(m_isAddingNoteEnabled)
                addNewNote();

            selectFirstNote();
        }

    }else if(m_proxyNoteModel->rowCount() == 0 && m_isAddingNoteEnabled){
        addNewNote();
    }else if(m_currentSelectedNoteProxy.isValid()){
        selectNote(m_currentSelectedNoteProxy);
    }else {
        selectFirstNote();
    }
}

bool NoteWidget::isViewEmpty()
{
    return m_proxyNoteModel->rowCount() == 0;
}

bool NoteWidget::isEmpty()
{
    return m_noteModel->rowCount() == 0;
}

void NoteWidget::filterByTag(const QString& regexString)
{
    m_filterTagQueue.enqueue(regexString);

    if(!m_isOperationRunning){
        m_isOperationRunning = true;

        m_noteView->setAnimationEnabled(false);
        removeTempNoteIfExists();

        if(!m_searchField->text().isEmpty())
            clearSearch();

        int previousNoteCnt = m_proxyNoteModel->rowCount();

        while(!m_filterTagQueue.isEmpty()){
            QString regExp =  m_filterTagQueue.dequeue();
            m_proxyNoteModel->setFilterRole(NoteModel::NoteTagSerial);
            m_proxyNoteModel->setFilterRegExp(regExp);
        }
        m_noteView->setAnimationEnabled(true);

        selectFirstNote();
        m_isOperationRunning = false;

        int curerntNoteCnt = m_proxyNoteModel->rowCount();
        if((previousNoteCnt==0 && curerntNoteCnt >0) ||
                (previousNoteCnt > 0 && curerntNoteCnt == 0))
            emit noteModelContentChanged();
    }
}

void NoteWidget::clearFilter()
{
    int prevNoteCount = m_proxyNoteModel->rowCount();
    m_proxyNoteModel->setFilterFixedString(QStringLiteral(""));

    int currentNoteCount = m_proxyNoteModel->rowCount();
    if(prevNoteCount == 0 && currentNoteCount > 0)
        emit noteModelContentChanged();
}

void NoteWidget::clearTagFilter()
{
    m_noteView->setAnimationEnabled(false);
    clearFilter();
    m_noteView->setAnimationEnabled(true);
}

void NoteWidget::onNoteChangedTimeoutTriggered()
{
    m_autoSaveTimer->stop();
    QModelIndex index = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
    Q_ASSERT_X(index.isValid(), "NoteWidget::onNoteChangedTimeoutTriggered", "index is not valid");

    NoteData* note = m_noteModel->getNote(index);

    if(m_isTempNoteExist){
        m_isTempNoteExist = false;
        emit noteAdded(note);
    }else{
        emit noteUpdated(note);
    }
}

void NoteWidget::updateNoteView()
{
    m_noteView->update();
}

QString NoteWidget::getFirstLine(const QString& str)
{
    if(str.simplified().isEmpty())
        return "New Note";

    QString text = str.trimmed();
    QTextStream ts(&text);
    return std::move(ts.readLine(FIRST_LINE_MAX));
}

void NoteWidget::setNoteDeletionEnabled(bool isNoteDeletionEnabled)
{
    m_isNoteDeletionEnabled = isNoteDeletionEnabled;
    m_deleteNoteButton->setEnabled(isNoteDeletionEnabled);
}

void NoteWidget::setAddingNoteEnabled(bool isAddingNoteEnabled)
{
    m_isAddingNoteEnabled = isAddingNoteEnabled;
    m_addNoteButton->setEnabled(isAddingNoteEnabled);
}

void NoteWidget::setNoteEditable(bool isNoteEditable)
{
    m_isNoteEditable = isNoteEditable;
    m_tagNoteButton->setEnabled(isNoteEditable);
}

void NoteWidget::setCurrentFolderPath(const QString& currentFolderPath)
{
    m_currentFolderPath = currentFolderPath;
}

void NoteWidget::setCurrentFolderName(const QString& folderName)
{
    ui->labelNotes->setText(folderName);
    bool enableDrag = !(folderName == QStringLiteral("Trash") || folderName == QStringLiteral("All Notes"));
    m_noteView->setDragEnabled(enableDrag);
}

void NoteWidget::fillNoteModel(QList<NoteData*> noteList)
{
    if(!noteList.isEmpty()){
        m_noteModel->addListNote(noteList);
        m_noteModel->sort();

        // add tags indexes to each correspondent note
        foreach(NoteData* note, noteList){
            QString tagStr = note->tagIdSerial();
            if(!tagStr.isEmpty())
                emit tagIndexesToBeAdded(note->id(), tagStr);
        }
    }

    selectFirstNote();
    m_noteView->setFocus();
    emit noteModelContentChanged();
}

NoteData *NoteWidget::generateNote(int id)
{
    Q_ASSERT_X(!m_currentFolderPath.isEmpty(), "NoteWidget::generateNote", "currentFolderPath is empty.");

    NoteData* newNote = new NoteData(this);
    newNote->setId(id);

    QDateTime noteDate = QDateTime::currentDateTime();
    newNote->setCreationDateTime(noteDate);
    newNote->setLastModificationDateTime(noteDate);
    newNote->setFullTitle(QStringLiteral("New Note"));
    newNote->setFullPath(m_currentFolderPath);

    return newNote;
}

void NoteWidget::selectFirstNote ()
{
    if(m_proxyNoteModel->rowCount() > 0){
        QModelIndex index = m_proxyNoteModel->index(0, 0);
        m_noteView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        m_noteView->setCurrentIndex(index);

        m_currentSelectedNoteProxy = index;
    }else{
        m_currentSelectedNoteProxy = QModelIndex();
        m_noteView->setFocus();
    }

    QModelIndex index = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
    emit noteSelectionChanged(index, QModelIndex());
}

void NoteWidget::addNewNoteIfEmpty ()
{
    if(m_proxyNoteModel->rowCount() == 0)
        addNewNote();
}

void NoteWidget::modifyNoteFolder(const QModelIndex& index, const QString& fullPath)
{
    QModelIndex indexInSrc = m_proxyNoteModel->mapToSource(index);
    qDebug() << indexInSrc;
    m_doSaveImmediately = true;
    m_noteModel->setData(indexInSrc, fullPath, NoteModel::NotePath);
    m_doSaveImmediately = false;

    NoteData* note = m_noteModel->removeNote(indexInSrc);
    note->deleteLater();
}

void NoteWidget::onAddNoteButtonClicked()
{
    if(!m_isOperationRunning && !m_autoSaveTimer->isActive()){
        m_isOperationRunning = true;

        m_noteView->setAnimationEnabled(false);

        if(!m_searchField->text().isEmpty()){
            clearSearch();
            m_selectedNoteBeforeSearchingInSource = QModelIndex();
        }else if(!m_proxyNoteModel->filterRegExp().pattern().isEmpty()){
            blockSignals(true);
            clearTagFilter();
            blockSignals(false);
        }

        m_noteView->setAnimationEnabled(true);

        addNewNote();

        m_isOperationRunning = false;
    }
}

void NoteWidget::onRemoveNoteButtonClicked()
{
    m_deleteNoteButton->blockSignals(true);
    this->removeSelectedNote();
    m_deleteNoteButton->blockSignals(false);
}

void NoteWidget::onNotePressed (const QModelIndex& index)
{
    if(sender() != Q_NULLPTR){
        selectNote(index);
        m_noteView->setFocus();
    }
}

void NoteWidget::onNoteModelRowsRemoved(const QModelIndex& parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    m_deleteNoteButton->setEnabled(true);
}

void NoteWidget::onSearchFieldTextChanged (const QString &keyword)
{
    if(!keyword.isEmpty() && m_searchQueue.isEmpty())
        emit noteSearchBegin();

    m_searchQueue.enqueue(keyword);

    if(!m_isOperationRunning){
        m_isOperationRunning = true;

        m_noteView->setAnimationEnabled(false);

        if(m_isTempNoteExist){
            removeTempNote();
            m_selectedNoteBeforeSearchingInSource = m_noteModel->rowCount() > 0
                                                    ? m_noteModel->index(0)
                                                    : m_noteModel->index(0);

        }else if(!m_selectedNoteBeforeSearchingInSource.isValid()
                 && m_currentSelectedNoteProxy.isValid()){

            m_selectedNoteBeforeSearchingInSource = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
        }

        int previousNoteCnt = m_proxyNoteModel->rowCount();

        while(!m_searchQueue.isEmpty()){
            qApp->processEvents();
            QString str = m_searchQueue.dequeue();
            if(str.isEmpty()){
                clearSearch();
                QModelIndex indexInProxy = m_proxyNoteModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
                indexInProxy.isValid()
                        ? selectNote(indexInProxy)
                        : selectFirstNote();
                m_selectedNoteBeforeSearchingInSource = QModelIndex();
            }else{
                findNotesContaining(str);
            }
        }

        m_searchField->setFocus();
        m_noteView->setAnimationEnabled(true);

        m_isOperationRunning = false;

        int curerntNoteCnt = m_proxyNoteModel->rowCount();
        if((previousNoteCnt==0 && curerntNoteCnt >0) ||
                (previousNoteCnt > 0 && curerntNoteCnt == 0))
            emit noteModelContentChanged();
    }
}

void NoteWidget::onNoteDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(bottomRight)

    // start/restart the timer
    if(m_isTempNoteExist || m_doSaveImmediately){
       m_autoSaveTimer->start(0);
    }else{
        m_autoSaveTimer->start(500);
    }

    if(roles.contains(NoteModel::NoteTagIndexList)){
        int noteId = topLeft.data(NoteModel::NoteID).toInt();
        QList<QPersistentModelIndex> tagIndexes = topLeft.data(NoteModel::NoteTagIndexList).value<QList<QPersistentModelIndex>>();
        emit tagsInNoteChanged(tagIndexes, noteId);
    }
}

void NoteWidget::onClearSearchButtonClicked()
{
    if(!m_isOperationRunning){

        m_noteView->setAnimationEnabled(false);
        clearSearch();
        m_noteView->setAnimationEnabled(true);

        if(m_noteModel->rowCount() > 0){
            QModelIndex indexInProxy = m_proxyNoteModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
            indexInProxy.isValid()
                    ? selectNote(indexInProxy)
                    : selectFirstNote();
        }else{
            m_currentSelectedNoteProxy = QModelIndex();
            emit noteSelectionChanged(QModelIndex(), QModelIndex());
        }

        m_selectedNoteBeforeSearchingInSource = QModelIndex();
    }
}

void NoteWidget::updateNoteCountLabel()
{
    int cnt = m_proxyNoteModel->rowCount();
    ui->noteCntLabel->setText(QStringLiteral("%1").arg(cnt));
}

void NoteWidget::addNewNote ()
{
    if(m_isAddingNoteEnabled){

        m_noteView->scrollToTop();
        int previousNoteCount = m_proxyNoteModel->rowCount();

        if(m_isTempNoteExist){
            int row = m_currentSelectedNoteProxy.row();
            m_noteView->animateAddedRow(QModelIndex(),row, row);
        }else{
            emit newNoteToBeAdded();

            ++m_noteCounter;
            NoteData* tmpNote = generateNote(m_noteCounter);
            m_isTempNoteExist = true;

            // insert the new note to NoteModel
            QModelIndex indexSrc = m_noteModel->insertNote(tmpNote, 0);

            // update the current selected index
            m_currentSelectedNoteProxy = m_proxyNoteModel->mapFromSource(indexSrc);
        }

        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);

        QModelIndex index = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
        emit newNoteAdded(index);

        if(previousNoteCount == 0)
            emit noteModelContentChanged();
    }
}

void NoteWidget::removeNote(const QModelIndex &noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "MainWindow::removeNote", "noteIndex is not valid");

    if(m_isTempNoteExist && noteIndex.row() == 0){
        removeTempNote();
    }else{
        QModelIndex indexToBeRemoved = m_proxyNoteModel->mapToSource(noteIndex);
        NoteData* note = m_noteModel->removeNote(indexToBeRemoved);
        note->setDeletionDateTime(QDateTime::currentDateTime());
        emit noteRemoved(note);
    }

    m_noteView->setFocus();

    int currentNoteCount = m_proxyNoteModel->rowCount();
    if(currentNoteCount == 0)
        emit noteModelContentChanged();
}

void NoteWidget::removeSelectedNote ()
{
    if(m_isNoteDeletionEnabled){
        if(!m_isOperationRunning){
            m_isOperationRunning = true;

            if(m_currentSelectedNoteProxy.isValid()){

                // delete the note
                removeNote(m_currentSelectedNoteProxy);

                // update the the current selected note
                m_currentSelectedNoteProxy = m_noteView->currentIndex();
                QModelIndex index = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
                emit noteSelectionChanged(index, QModelIndex());
            }
            m_isOperationRunning = false;
        }
    }
}

void NoteWidget::setFocusOnCurrentNote ()
{
    if(m_currentSelectedNoteProxy.isValid())
        m_noteView->setFocus();
}

void NoteWidget::showTagNoteMenu()
{
    if(m_noteModel->rowCount() >0
            && !m_isTempNoteExist
            && m_currentSelectedNoteProxy.isValid()){

        QPoint gPos = m_tagNoteButton->parentWidget()->mapToGlobal(m_tagNoteButton->geometry().bottomLeft());

        bool isTagSelectionModified = false;

        QMenu menu;
        menu.setObjectName(QStringLiteral("tagNoteMenu"));
        menu.show();
        QSize sz = menu.size();
        menu.setGeometry(gPos.x(), gPos.y(), sz.width(), sz.height());

        int contentWidth = menu.contentsRect().width() - 2*5;

        QList<QPersistentModelIndex> tagIndexes;
        QModelIndex indexSrc = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);

        TagNoteModel noteTagModel;

        QWidgetAction tagviewWidgetAction(&menu);
        QListView listView;
        listView.setObjectName(QStringLiteral("tagNoteView"));
        listView.setModel(&noteTagModel);
        listView.setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        listView.show();

        listView.setFixedWidth(contentWidth);
        listView.setFrameShape(QFrame::NoFrame);
        listView.setSelectionMode(QAbstractItemView::NoSelection);
        tagviewWidgetAction.setDefaultWidget(&listView);
        menu.addAction(&tagviewWidgetAction);

        QWidgetAction addtagNoteWidgetAction(&menu);
        QPushButton addTagPb;
        addTagPb.setObjectName(QStringLiteral("createTagButton"));
        addTagPb.setText(tr("Create new tag"));
        addTagPb.setFlat(true);
        addTagPb.setEnabled(m_proxyNoteModel->rowCount() > 0);
        addTagPb.setFixedWidth(contentWidth);
        addtagNoteWidgetAction.setDefaultWidget(&addTagPb);
        menu.addAction(&addtagNoteWidgetAction);

        // check / uncheck when the item is clicked
        connect(&listView, &QListView::pressed, [&](const QModelIndex& index){
            bool isChecked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
            if(isChecked){
                noteTagModel.setData(index, Qt::Unchecked, Qt::CheckStateRole);
            }else{
                noteTagModel.setData(index, Qt::Checked, Qt::CheckStateRole);
            }
        });

        // init listview geometry
        connect(&noteTagModel, &TagNoteModel::layoutChanged, [&](){
            int bottomMargin = 6;
            int maxRowsInVisibleRegion = 7;
            int rowHeight = listView.sizeHintForIndex(noteTagModel.index(0)).height();
            int listViewMaxHeight =  (rowHeight * maxRowsInVisibleRegion) + bottomMargin;
            int listViewHeight = (rowHeight * noteTagModel.rowCount()) + bottomMargin;

            listViewHeight = noteTagModel.rowCount() <= maxRowsInVisibleRegion ? listViewHeight : listViewMaxHeight;
            listView.setFixedHeight(listViewHeight);

            if(noteTagModel.rowCount() > 0)
                menu.insertSeparator(&addtagNoteWidgetAction);
        });

        connect(&noteTagModel, &TagNoteModel::dataChanged, [&](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles){
            Q_UNUSED(bottomRight)

            if(roles.contains(Qt::CheckStateRole)){
                QPersistentModelIndex indexInTagModel = noteTagModel.mapToSource(topLeft);
                bool isChecked = topLeft.data(Qt::CheckStateRole).toInt() == Qt::Checked;
                if(isChecked){
                    tagIndexes.append(indexInTagModel);
                }else{
                    tagIndexes.removeOne(indexInTagModel);
                }
                isTagSelectionModified = true;
            }
        });

        connect(&addTagPb, &QPushButton::clicked, [&](){
            menu.hide();

            if(indexSrc.isValid())
                emit menuAddTagClicked(indexSrc);
        });

        connect(&menu, &QMenu::aboutToHide, [&](){
            if(isTagSelectionModified)
                m_noteModel->setData(indexSrc, QVariant::fromValue(tagIndexes), NoteModel::NoteTagIndexList);
        });

        emit noteTagMenuAboutTobeShown(indexSrc, menu);
        menu.exec();
    }
}

void NoteWidget::selectNoteUp()
{
    if(m_proxyNoteModel->rowCount() > 1){
        if(m_currentSelectedNoteProxy.isValid() && m_currentSelectedNoteProxy.row() != 0){
            QModelIndex deselected = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);

            int currentRow = m_currentSelectedNoteProxy.row();
            QModelIndex aboveIndex = m_proxyNoteModel->index(currentRow - 1, 0);
            m_noteView->setCurrentIndex(aboveIndex);
            m_currentSelectedNoteProxy = aboveIndex;

            QModelIndex selected = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
            emit noteSelectionChanged(selected, deselected);

            m_noteView->setFocus();
        }
    }
}

void NoteWidget::selectNoteDown ()
{
    if(!m_isOperationRunning){
        m_isOperationRunning = true;

        if(m_proxyNoteModel->rowCount() > 1 &&
                (m_currentSelectedNoteProxy.row() != m_proxyNoteModel->rowCount()-1)){

            QModelIndex deselected = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);

            if(m_currentSelectedNoteProxy.isValid()){
                if(m_isTempNoteExist){
                    removeTempNote();
                    m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);

                }else{
                    int currentRow = m_currentSelectedNoteProxy.row();
                    QModelIndex belowIndex = m_proxyNoteModel->index(currentRow + 1, 0);
                    m_noteView->setCurrentIndex(belowIndex);
                    m_currentSelectedNoteProxy = belowIndex;
                }

                QModelIndex selected = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);
                emit noteSelectionChanged(selected, deselected);

                m_noteView->setFocus();
            }
        }
        m_isOperationRunning = false;
    }
}

void NoteWidget::onNoteViewViewportClicked()
{
    if(m_isTempNoteExist){
        removeTempNote();
        selectFirstNote();
    }
}

void NoteWidget::moveNoteToTop()
{
    if(m_currentSelectedNoteProxy.isValid()
            && m_noteView->currentIndex().row() != 0){

        m_noteView->scrollToTop();
        m_noteModel->moveRow(m_currentSelectedNoteProxy.row(), 0);

        m_currentSelectedNoteProxy = m_proxyNoteModel->index(0,0);
        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
    }
}

void NoteWidget::clearSearch()
{
    if(!m_searchField->text().isEmpty()){
        m_searchField->blockSignals(true);
        m_searchField->clear();
        m_searchField->blockSignals(false);

        clearFilter();

        m_clearSearchButton->hide();
        m_searchField->setFocus();
    }
}

void NoteWidget::findNotesContaining(const QString& keyword)
{
    m_proxyNoteModel->setFilterRole(NoteModel::NoteContent);
    m_proxyNoteModel->setFilterFixedString(keyword);
    m_clearSearchButton->show();

    selectFirstNote();
}

void NoteWidget::selectNote(const QModelIndex& noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "MainWindow::selectNote", "noteIndex is not valid");
    QModelIndex deselectedIndexInProxy;

    if(noteIndex == m_currentSelectedNoteProxy){
        return;
    }else if(m_currentSelectedNoteProxy.isValid()){
        if(m_isTempNoteExist){
            removeTempNote();
            m_currentSelectedNoteProxy = noteIndex;
        }else{
            deselectedIndexInProxy = m_currentSelectedNoteProxy;
            m_currentSelectedNoteProxy = noteIndex;
        }
    }else{
        m_currentSelectedNoteProxy = noteIndex;
    }

    m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
    m_noteView->scrollTo(m_currentSelectedNoteProxy);

    QModelIndex deselected = m_proxyNoteModel->mapToSource(deselectedIndexInProxy);
    QModelIndex selected = m_proxyNoteModel->mapToSource(m_currentSelectedNoteProxy);

    emit noteSelectionChanged(selected, deselected);
}

void NoteWidget::removeTempNote()
{
    QModelIndex index = m_noteModel->index(0);
    m_noteModel->removeNote(index);
    m_isTempNoteExist = false;
    --m_noteCounter;

    if(m_proxyNoteModel->rowCount() == 0)
        emit noteModelContentChanged();
}

void NoteWidget::removeTempNoteIfExists()
{
    if(m_isTempNoteExist)
        removeTempNote();
}

void NoteWidget::initNotes(QList<NoteData*> noteList)
{
    m_noteView->setAnimationEnabled(false);
    fillNoteModel(noteList);
    m_noteView->setAnimationEnabled(true);
}
