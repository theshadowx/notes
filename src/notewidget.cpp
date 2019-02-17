#include "notewidget.h"
#include "ui_notewidget.h"
#include "notewidgetdelegate.h"

#include <QKeyEvent>
#include <QShortcut>
#include <QDebug>

NoteWidget::NoteWidget(QWidget *parent) :
    QWidget(parent),
    ui                  (new Ui::NoteWidget),
    m_searchEdit        (Q_NULLPTR),
    m_newNoteButton     (Q_NULLPTR),
    m_trashButton       (Q_NULLPTR),
    m_clearButton       (Q_NULLPTR),
    m_noteCountView     (Q_NULLPTR),
    m_noteView          (Q_NULLPTR),
    m_noteModel         (new NoteModel(this)),
    m_proxyModel        (new QSortFilterProxyModel(this)),
    m_dbManager         (Q_NULLPTR),
    m_settingsDatabase  (Q_NULLPTR),
    m_isCurrentNoteTemp (false),
    m_noteCounter       (0)
{
    ui->setupUi(this);

    setupNoteWidget();
    setupModelView();
    setupSearchEdit();
    setupConnections();
    setupStylesheet();
}

NoteWidget::~NoteWidget()
{
    delete ui;
}

void NoteWidget::initNoteWidget(QList<NoteData*>& noteList)
{
    loadNotes(noteList);
    createNewNoteIfEmpty();
    selectFirstNote();
}

QModelIndex NoteWidget::getCurrentSelectedIndex() const
{
    return m_currentSelectedNoteProxy;
}

NoteData* NoteWidget::getNoteData(const QModelIndex noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "getNoteData", "noteIndex is not valid");

    QModelIndex indexInSrc = m_proxyModel->mapToSource(noteIndex);
    NoteData* note = m_noteModel->getNote(indexInSrc);

    return note;
}

void NoteWidget::initNoteCounter(const int count)
{
    m_noteCounter = count;
}

int NoteWidget::getNotesLastIndex() const
{
    // TODO rename this attribute and the setters/getters
    return m_noteCounter;
}

int NoteWidget::getNotesCount() const
{
    return m_noteModel->rowCount();
}

QString NoteWidget::getSearchText() const
{
    return m_searchEdit->text();
}

void NoteWidget::updateContent(QString title, QString text, QDateTime modificationDateTime)
{
    Q_ASSERT_X(m_currentSelectedNoteProxy.isValid(), "updateText", "m_currentSelectedNoteProxy is not valid");

    QMap<int, QVariant> dataValue;
    dataValue[NoteModel::NoteContent] = QVariant::fromValue(text);
    dataValue[NoteModel::NoteFullTitle] = QVariant::fromValue(title);
    dataValue[NoteModel::NoteLastModificationDateTime] = QVariant::fromValue(modificationDateTime);

    QModelIndex indexInSource = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
    m_noteModel->setItemData(indexInSource, dataValue);

    m_isCurrentNoteTemp = false;

    // update the row of indicator of the selected note before searching
    if(!m_searchEdit->text().isEmpty()){
        if(indexInSource != m_selectedNoteBeforeSearchingInSource){
            int beforeSearchSelectedRow = m_selectedNoteBeforeSearchingInSource.row();
            if(indexInSource.row() > beforeSearchSelectedRow){
                m_selectedNoteBeforeSearchingInSource = m_noteModel->index(beforeSearchSelectedRow + 1);
            }
        }
    }

    // move the current note to top
    if(m_currentSelectedNoteProxy.row() != 0){
        moveNoteToTop();
    }else if(!m_searchEdit->text().isEmpty() && indexInSource.row() != 0){
        // in not filtered list
        m_noteView->setAnimationEnabled(false);
        moveNoteToTop();
        m_noteView->setAnimationEnabled(true);
    }
}

void NoteWidget::updateScrollBarPos(QModelIndex noteIndex, int pos)
{
    Q_ASSERT_X(noteIndex.isValid(), "updateText", "m_currentSelectedNoteProxy is not valid");
    QModelIndex indexInSource = m_proxyModel->mapToSource(noteIndex);
    m_noteModel->setData(indexInSource, QVariant::fromValue(pos), NoteModel::NoteScrollbarPos);
}

void NoteWidget::setupNoteWidget()
{
    m_newNoteButton = ui->newNoteButton;
    m_trashButton   = ui->trashButton;
    m_searchEdit    = ui->searchEdit;
    m_noteCountView = ui->noteCountView;

    m_newNoteButton->setToolTip("Create New Note");
    m_trashButton->setToolTip("Delete Selected Note");

#ifdef __APPLE__
    m_searchEdit->setFont(QFont("Helvetica Neue", 12));
#else
    m_searchEdit->setFont(QFont(QStringLiteral("Roboto"), 10));
#endif

}

void NoteWidget::setupModelView()
{
    m_noteView = ui->noteView;
    m_proxyModel->setSourceModel(m_noteModel);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setFilterRole(NoteModel::NoteContent);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_noteView->setItemDelegate(new NoteWidgetDelegate(m_noteView));
    m_noteView->setModel(m_proxyModel);
}

void NoteWidget::setupSearchEdit()
{
    // clear button
    m_clearButton = new QToolButton(m_searchEdit);
    QPixmap pixmap(":images/closeButton.png");
    m_clearButton->setIcon(QIcon(pixmap));
    QSize clearSize(15, 15);
    m_clearButton->setIconSize(clearSize);
    m_clearButton->setCursor(Qt::ArrowCursor);
    m_clearButton->hide();

    // search button
    QToolButton *searchButton = new QToolButton(m_searchEdit);
    QPixmap newPixmap(":images/magnifyingGlass.png");
    searchButton->setIcon(QIcon(newPixmap));
    QSize searchSize(24, 25);
    searchButton->setIconSize(searchSize);
    searchButton->setCursor(Qt::ArrowCursor);

    // layout
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::RightToLeft, m_searchEdit);
    layout->setContentsMargins(0,0,3,0);
    layout->addWidget(m_clearButton);
    layout->addStretch();
    layout->addWidget(searchButton);
    m_searchEdit->setLayout(layout);

    m_searchEdit->installEventFilter(this);
}

void NoteWidget::setupNewNoteButtonAndTrahButton()
{
    m_newNoteButton->installEventFilter(this);
    m_trashButton->installEventFilter(this);
}

void NoteWidget::setupConnections()
{
    // clear button
    connect(m_clearButton, &QToolButton::clicked, this, &NoteWidget::onClearButtonClicked);
    // noteView viewport pressed
    connect(m_noteView, &NoteView::viewportPressed, this, [this](){
        if(m_isCurrentNoteTemp){
            if(m_proxyModel->rowCount() > 1){
                QModelIndex indexInProxy = m_proxyModel->index(1, 0);
                selectNote(indexInProxy);
            }
        }
    });

    connect(m_searchEdit, &QLineEdit::textChanged, this, &NoteWidget::onSearchEditTextChanged);

    // noteView pressed
    connect(m_noteView, &NoteView::pressed, this, &NoteWidget::onNotePressed);
    // note model rows moved/removed
    connect(m_noteModel, &NoteModel::rowsAboutToBeMoved, m_noteView, &NoteView::rowsAboutToBeMoved);
    connect(m_noteModel, &NoteModel::rowsMoved, m_noteView, &NoteView::rowsMoved);
    connect(m_noteModel, &NoteModel::rowsRemoved, [this](){m_trashButton->setEnabled(true);});
    // delete note button
    connect(m_trashButton, &QPushButton::clicked, this, &NoteWidget::onTrashButtonClicked);
    // new note button
    connect(m_newNoteButton, &QPushButton::clicked, this, &NoteWidget::onNewNoteButtonClicked);
    // update note count
    connect(m_proxyModel, &QSortFilterProxyModel::rowsRemoved, this, &NoteWidget::updateNoteCount);
    connect(m_proxyModel, &QSortFilterProxyModel::rowsInserted, this, &NoteWidget::updateNoteCount);
}

void NoteWidget::setupStylesheet()
{
    QString noteWidgetStyle =
        // Search Edit
        "#searchEdit{padding-left: 21px; padding-right: 19px; border: 1px solid rgb(205, 205, 205);"
        "            border-radius: 3px; background: rgb(255, 255, 255); selection-background-color: rgb(61, 155, 218);}"
        "#searchEdit:focus{border: 2px solid rgb(61, 155, 218);}"
        "QToolButton{border: none; padding: 0px;}"
        // Delete Note pushButton
        "#trashButton{border: none;padding: 0px; margin: 2px 2px 2px 2px;image:url(:/images/trashCan_Regular.png);}"
        "#trashButton:pressed{margin: 3px 2px 2px 3px;image:url(:/images/trashCan_Pressed.png);}"
        "#trashButton:hover{image:url(:/images/trashCan_Hovered.png);}"
        "#trashButton:disabled{image:url(:/images/trashCan_Hovered.png);}"
        // Add Note pushButton
        "#newNoteButton{border: none;padding: 0px; margin: 2px 2px 2px 2px;image:url(:/images/newNote_Regular.png);}"
        "#newNoteButton:pressed{margin: 3px 2px 2px 3px;image:url(:/images/newNote_Pressed.png);}"
        "#newNoteButton:hover{image:url(:/images/newNote_Hovered.png);}"
        "#newNoteButton:disabled{image:url(:/images/newNote_Hovered.png);}"
        // NoteCountView
        "#noteCountView{font-size:8pt;}"
        // NoteView
        "#noteView QListView {background-color: rgb(255, 255, 255);}"
        "#noteView QScrollBar                          {margin-right: 2px; background: transparent;}"
        "#noteView QScrollBar:hover                    {background-color: rgb(217, 217, 217);}"
        "#noteView QScrollBar::handle:vertical:hover   {background: rgb(170, 170, 171);}"
        "#noteView QScrollBar::handle:vertical:pressed {background: rgb(149, 149, 149);}"
        "#noteView QScrollBar::vertical                {border: none; width: 10px; border-radius: 4px;}"
        "#noteView QScrollBar::handle:vertical         {border-radius: 4px; background: rgb(188, 188, 188); min-height: 20px;}"
        "#noteView QScrollBar::add-line:vertical       {height: 0px; subcontrol-position: bottom; subcontrol-origin: margin;}"
        "#noteView QScrollBar::sub-line:vertical       {height: 0px; subcontrol-position: top; subcontrol-origin: margin;}";

    this->setStyleSheet(noteWidgetStyle);
}

void NoteWidget::clearSearch()
{
    m_noteView->setFocusPolicy(Qt::StrongFocus);

    m_searchEdit->blockSignals(true);
    m_searchEdit->clear();
    m_searchEdit->blockSignals(false);

    emit searchCleared();

    m_proxyModel->setFilterFixedString(QStringLiteral(""));

    m_clearButton->hide();
    m_searchEdit->setFocus();
}

void NoteWidget::selectNote(const QModelIndex& noteIndex)
{
    QModelIndex deselecteNoteIndex;

    if(noteIndex.isValid()){
        if(m_isCurrentNoteTemp && noteIndex.row() != 0){

            deleteNote(m_currentSelectedNoteProxy);
            m_currentSelectedNoteProxy = m_proxyModel->index(noteIndex.row()-1, 0);
            deselecteNoteIndex = QModelIndex();

        }else{
            deselecteNoteIndex = m_currentSelectedNoteProxy;
            m_currentSelectedNoteProxy = noteIndex;
        }

        m_noteView->selectionModel()->select(m_currentSelectedNoteProxy, QItemSelectionModel::ClearAndSelect);
        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
        m_noteView->scrollTo(m_currentSelectedNoteProxy);

        emit noteSelectionChanged(m_currentSelectedNoteProxy, deselecteNoteIndex);

    }else{
        deselecteNoteIndex = m_currentSelectedNoteProxy;
        m_currentSelectedNoteProxy = QModelIndex();
        emit noteSelectionChanged(m_currentSelectedNoteProxy, deselecteNoteIndex);
    }
}

void NoteWidget::selectFirstNote()
{
    QModelIndex deselectedIndex = m_currentSelectedNoteProxy;
    QModelIndex index = m_proxyModel->index(0,0);
    m_noteView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    m_noteView->setCurrentIndex(index);

    m_currentSelectedNoteProxy = index;
    emit noteSelectionChanged(m_currentSelectedNoteProxy, deselectedIndex);
}

void NoteWidget::findNotesContain(const QString& keyword)
{
    m_proxyModel->setFilterFixedString(keyword);
    m_clearButton->show();

    if(m_proxyModel->rowCount() > 0){
        selectFirstNote();
    }else{
        selectNote(QModelIndex());
    }
}

void NoteWidget::moveNoteToTop()
{
    Q_ASSERT_X(m_currentSelectedNoteProxy.isValid(), "moveNoteToTop", "m_currentSelectedNoteProxy is not valid.");

    m_noteView->scrollToTop();

    // move the current selected note to the top
    QModelIndex sourceIndex = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
    QModelIndex destinationIndex = m_noteModel->index(0);
    m_noteModel->moveRow(sourceIndex, sourceIndex.row(), destinationIndex, 0);

    // update the current item
    m_currentSelectedNoteProxy = m_proxyModel->mapFromSource(destinationIndex);
    m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
}

void NoteWidget::createNewNoteIfEmpty()
{
    if(m_proxyModel->rowCount() == 0)
        createNewNote();
}

NoteData*NoteWidget::generateNote(const int noteID)
{
    NoteData* newNote = new NoteData(this);
    newNote->setId(noteID);

    QDateTime noteDate = QDateTime::currentDateTime();
    newNote->setCreationDateTime(noteDate);
    newNote->setLastModificationDateTime(noteDate);
    newNote->setFullTitle(QStringLiteral("New Note"));

    return newNote;
}

bool NoteWidget::eventFilter(QObject* object, QEvent* event)
{
    if(event->type() == QEvent::KeyPress){
        if(object == m_searchEdit){
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            // prevent searchEdit to capture the meta + key event
            // as it's used in the mainwindow shortcut
            if((keyEvent->modifiers() & Qt::MetaModifier) == Qt::MetaModifier)
                return true;
        }
    }else if(event->type() == QEvent::Enter){
        if(qApp->applicationState() == Qt::ApplicationActive){
            if(object == m_newNoteButton){
                this->setCursor(Qt::PointingHandCursor);
            }else if(object == m_trashButton){
                this->setCursor(Qt::PointingHandCursor);
            }
        }
    }else if(event->type() == QEvent::Leave){
        if(qApp->applicationState() == Qt::ApplicationActive){
            if(object == m_newNoteButton){
                this->unsetCursor();
            }else if(object == m_trashButton){
                this->unsetCursor();
            }
        }
    }else if(event->type() == QEvent::WindowDeactivate){
        m_newNoteButton->setDisabled(true);
        m_trashButton->setDisabled(true);
    }else if(event->type() == QEvent::WindowActivate){
        m_newNoteButton->setEnabled(true);
        m_trashButton->setEnabled(true);
    }

    return QWidget::eventFilter(object, event);
}

void NoteWidget::loadNotes(QList<NoteData*>& noteList)
{
    if(!noteList.isEmpty()){
        m_noteModel->addListNote(noteList);
        m_noteModel->sort(0, Qt::AscendingOrder);
    }
}

void NoteWidget::resetNoteWidget()
{
    m_noteModel->clearNotes();

    m_currentSelectedNoteProxy = QModelIndex();
    m_isCurrentNoteTemp = false;
    m_noteCounter = 0;

    emit noteSelectionChanged(QModelIndex(), QModelIndex());
}

void NoteWidget::updateNoteCount()
{
   m_noteCountView->setText(QString("Number of Notes : %1").arg(m_proxyModel->rowCount()));
}

void NoteWidget::setFocusOnCurrentNote()
{
    if(m_currentSelectedNoteProxy.isValid())
        m_noteView->setFocus();
}

void NoteWidget::selectNoteUp()
{
    if(m_currentSelectedNoteProxy.isValid()){
        int currentRow = m_noteView->currentIndex().row();
        QModelIndex aboveIndex = m_noteView->model()->index(currentRow - 1, 0);
        QModelIndex deselectedNote = m_currentSelectedNoteProxy;
        if(aboveIndex.isValid()){
            QModelIndex selectedNote = aboveIndex;
            m_noteView->setCurrentIndex(aboveIndex);
            m_currentSelectedNoteProxy = aboveIndex;
            emit noteSelectionChanged(selectedNote, deselectedNote);
        }
        m_noteView->setFocus();
    }
}

void NoteWidget::selectNoteDown()
{
    if(m_currentSelectedNoteProxy.isValid()){
        if(m_isCurrentNoteTemp){

            if(m_proxyModel->rowCount() == 1)
                return;

            deleteNote(m_currentSelectedNoteProxy);
            m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
            emit noteSelectionChanged(m_currentSelectedNoteProxy, QModelIndex());
        }else{
            int currentRow = m_noteView->currentIndex().row();
            QModelIndex deselectedNoteIndex = m_currentSelectedNoteProxy;
            QModelIndex belowIndex = m_noteView->model()->index(currentRow + 1, 0);
            if(belowIndex.isValid()){
                QModelIndex selectedNoteIndex = belowIndex;
                m_noteView->setCurrentIndex(belowIndex);
                m_currentSelectedNoteProxy = belowIndex;
                emit noteSelectionChanged(selectedNoteIndex, deselectedNoteIndex);
            }
        }
        m_noteView->setFocus();
    }
}

void NoteWidget::focusOnSearch()
{
    m_searchEdit->setFocus();
}

void NoteWidget::createNewNote()
{
    m_noteView->scrollToTop();

    if(!m_isCurrentNoteTemp){
        ++m_noteCounter;
        NoteData* tmpNote = generateNote(m_noteCounter);
        m_isCurrentNoteTemp = true;

        // insert the new note to NoteModel
        m_noteModel->insertNote(tmpNote, 0);

    }else{
        int row = 0;
        m_noteView->animateAddedRow(QModelIndex(), row, row);
    }
}

void NoteWidget::deleteNote(const QModelIndex& noteIndex)
{
    Q_ASSERT_X(m_proxyModel->rowCount() > 0, "deleteNote", "deleteNote called while the list is empty");
    Q_ASSERT_X(noteIndex.isValid(), "deleteNote", "noteIndex is not valid");

    // delete from model
    QModelIndex indexToBeDeleted = m_proxyModel->mapToSource(noteIndex);
    NoteData* noteTobeDeleted = m_noteModel->removeNote(indexToBeDeleted);

    if(m_isCurrentNoteTemp && noteIndex.row() == 0){
        m_isCurrentNoteTemp = false;
        --m_noteCounter;
        delete noteTobeDeleted;
    }else{
        noteTobeDeleted->setDeletionDateTime(QDateTime::currentDateTime());
        emit noteDeleted(noteTobeDeleted);
    }

    m_noteView->setFocus();
}

void NoteWidget::deleteCurrentNote()
{
    // prevent using the shorcut while the previous
    // deletion is not yet done.
    if(qobject_cast<QShortcut*>(sender()))
        sender()->blockSignals(true);

    if(m_proxyModel->rowCount() > 0){
        if(m_searchEdit->text().isEmpty()){
            if(m_proxyModel->rowCount() == 1 && m_isCurrentNoteTemp){
                return;
            }else{
                deleteNote(m_currentSelectedNoteProxy);

                if(m_proxyModel->rowCount() == 0){
                    createNewNote();
                    selectFirstNote();
                }else{
                    if(m_currentSelectedNoteProxy.row() >= m_proxyModel->rowCount())
                        m_currentSelectedNoteProxy = m_proxyModel->index(m_proxyModel->rowCount() - 1, 0);

                    selectNote(m_currentSelectedNoteProxy);
                }
            }
        }else{
            if(m_noteModel->rowCount() > 1){
                // update the index of the selected note before searching
                QModelIndex currentIndexInSource = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
                int beforeSearchSelectedRow = m_selectedNoteBeforeSearchingInSource.row();
                if(currentIndexInSource.row() < beforeSearchSelectedRow){
                    m_selectedNoteBeforeSearchingInSource = m_noteModel->index(beforeSearchSelectedRow-1);
                }
            }else{
                m_selectedNoteBeforeSearchingInSource = QModelIndex();
            }

            deleteNote(m_currentSelectedNoteProxy);
        }
    }

    if(qobject_cast<QShortcut*>(sender()))
        sender()->blockSignals(false);
}

void NoteWidget::onClearButtonClicked()
{
    m_noteView->setAnimationEnabled(false);
    clearSearch();
    m_noteView->setAnimationEnabled(true);

    if(m_noteModel->rowCount() > 0){
        QModelIndex indexInProxy = m_proxyModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
        selectNote(indexInProxy);
    }else{
        createNewNote();
        selectFirstNote();
    }

    m_selectedNoteBeforeSearchingInSource = QModelIndex();
}

void NoteWidget::onTrashButtonClicked()
{
    m_trashButton->blockSignals(true);
    deleteCurrentNote();
    m_trashButton->blockSignals(false);
}

void NoteWidget::onNewNoteButtonClicked()
{
    if(!m_searchEdit->text().isEmpty()){
        m_noteView->setAnimationEnabled(false);
        clearSearch();
        m_noteView->setAnimationEnabled(true);
        m_selectedNoteBeforeSearchingInSource = QModelIndex();
    }

    createNewNote();
    selectFirstNote();
}

void NoteWidget::onNotePressed(const QModelIndex& index)
{
    if(sender() != Q_NULLPTR){
        QModelIndex indexInProxy = m_proxyModel->index(index.row(), 0);
        selectNote(indexInProxy);
    }
}

void NoteWidget::onSearchEditTextChanged(const QString& keyword)
{
    m_searchQueue.enqueue(keyword);

    if(m_isCurrentNoteTemp){
        m_isCurrentNoteTemp = false;
        --m_noteCounter;
        // prevent the line edit from emitting signal
        // while animation for deleting the new note is running
        m_searchEdit->blockSignals(true);
        m_currentSelectedNoteProxy = QModelIndex();
        QModelIndex index = m_noteModel->index(0);
        m_noteModel->removeNote(index);
        m_searchEdit->blockSignals(false);

        if(m_noteModel->rowCount() > 0){
            m_selectedNoteBeforeSearchingInSource = m_noteModel->index(0);
        }else{
            m_selectedNoteBeforeSearchingInSource = QModelIndex();
        }

    }else if(!m_selectedNoteBeforeSearchingInSource.isValid()
             && m_currentSelectedNoteProxy.isValid()){

        m_selectedNoteBeforeSearchingInSource = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
    }

    // disable animation while searching
    m_noteView->setAnimationEnabled(false);

    while(!m_searchQueue.isEmpty()){
        qApp->processEvents();
        QString str = m_searchQueue.dequeue();
        if(str.isEmpty()){
            m_noteView->setFocusPolicy(Qt::StrongFocus);
            clearSearch();
            QModelIndex indexInProxy = m_proxyModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
            selectNote(indexInProxy);
            m_selectedNoteBeforeSearchingInSource = QModelIndex();
        }else{
            m_noteView->setFocusPolicy(Qt::NoFocus);
            findNotesContain(str);
        }
    }

    m_noteView->setAnimationEnabled(true);
}

