#include "notewidget.h"
#include "ui_notewidget.h"
#include "notewidgetdelegate.h"

#include <QDebug>

NoteWidget::NoteWidget(QWidget *parent) :
    QWidget(parent),
    ui                  (new Ui::NoteWidget),
    m_searchEdit        (Q_NULLPTR),
    m_newNoteButton     (Q_NULLPTR),
    m_trashButton       (Q_NULLPTR),
    m_clearButton       (Q_NULLPTR),
    m_noteView          (Q_NULLPTR),
    m_noteModel         (new NoteModel(this)),
    m_proxyModel        (new QSortFilterProxyModel(this)),
    m_dbManager         (Q_NULLPTR),
    m_settingsDatabase  (Q_NULLPTR),
    m_isTemp            (false),
    m_isContentModified (false),
    m_noteCounter       (0)
{
    ui->setupUi(this);

    setupNoteWidget();
    setupModelView();
    setupSearchEdit();

    // clear button
    connect(m_clearButton, &QToolButton::clicked, this, &NoteWidget::onClearButtonClicked);
    // noteView viewport pressed
    connect(m_noteView, &NoteView::viewportPressed, this, [this](){
        if(m_isTemp && m_proxyModel->rowCount() > 1){
            QModelIndex indexInProxy = m_proxyModel->index(1, 0);
            selectNote(indexInProxy);
        }else if(m_isTemp && m_proxyModel->rowCount() == 1){
            QModelIndex indexInProxy = m_proxyModel->index(0, 0);
            // TODO m_editorDateLabel->clear();
            deleteNote(indexInProxy, false);
        }
    });
}

NoteWidget::~NoteWidget()
{
    delete ui;
}

void NoteWidget::setupNoteWidget()
{
    m_newNoteButton = ui->newNoteButton;
    m_trashButton   = ui->trashButton;
    m_searchEdit    = ui->searchEdit;
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
    int frameWidth = m_searchEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QString ss = QString("QLineEdit{ "
                         "  padding-right: %1px; "
                         "  padding-left: 21px;"
                         "  padding-right: 19px;"
                         "  border: 1px solid rgb(205, 205, 205);"
                         "  border-radius: 3px;"
                         "  background: rgb(255, 255, 255);"
                         "  selection-background-color: rgb(61, 155, 218);"
                         "} "
                         "QToolButton { "
                         "  border: none; "
                         "  padding: 0px;"
                         "}"
                         ).arg(frameWidth + 1);

    m_searchEdit->setStyleSheet(ss);

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
    QString ss = "QPushButton { "
                 "  border: none; "
                 "  padding: 0px; "
                 "}";

    m_newNoteButton->setStyleSheet(ss);
    m_trashButton->setStyleSheet(ss);
    // TODO m_dotsButton->setStyleSheet(ss);

    m_newNoteButton->installEventFilter(this);
    m_trashButton->installEventFilter(this);
    //TODO m_dotsButton->installEventFilter(this);
}

void NoteWidget::clearSearch()
{
    m_noteView->setFocusPolicy(Qt::StrongFocus);

    m_searchEdit->blockSignals(true);
    m_searchEdit->clear();
    m_searchEdit->blockSignals(false);

    // TODO m_textEdit->blockSignals(true);
    // TODO m_textEdit->clear();
    // TODO m_textEdit->clearFocus();
    // TODO m_editorDateLabel->clear();
    // TODO m_textEdit->blockSignals(false);

    m_proxyModel->setFilterFixedString(QStringLiteral(""));

    m_clearButton->hide();
    m_searchEdit->setFocus();
}

void NoteWidget::selectNote(const QModelIndex& noteIndex)
{
    if(noteIndex.isValid()){
        // save the position of text edit scrollbar
        if(!m_isTemp && m_currentSelectedNoteProxy.isValid()){
            // TODO int pos = m_textEdit->verticalScrollBar()->value();
            // TODO QModelIndex indexSrc = m_proxyModel->mapToSource(m_noteWidget->m_currentSelectedNoteProxy);
            // TODO m_noteModel->setData(indexSrc, QVariant::fromValue(pos), NoteModel::NoteScrollbarPos);
        }

        // show the content of the pressed note in the text editor
        // TODO showNoteInEditor(noteIndex);

        if(m_isTemp && noteIndex.row() != 0){
            // delete the unmodified new note
            deleteNote(m_currentSelectedNoteProxy, false);
            m_currentSelectedNoteProxy = m_proxyModel->index(noteIndex.row()-1, 0);
        }else if(!m_isTemp
                 && m_currentSelectedNoteProxy.isValid()
                 && noteIndex != m_currentSelectedNoteProxy
                 && m_isContentModified){
            // save if the previous selected note was modified
            // TODO saveNoteToDB(m_noteWidget->m_currentSelectedNoteProxy);
            m_currentSelectedNoteProxy = noteIndex;
        }else{
            m_currentSelectedNoteProxy = noteIndex;
        }

        m_noteView->selectionModel()->select(m_currentSelectedNoteProxy, QItemSelectionModel::ClearAndSelect);
        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
        m_noteView->scrollTo(m_currentSelectedNoteProxy);
    }else{
        qDebug() << "MainWindow::selectNote() : noteIndex is not valid";
    }
}

void NoteWidget::selectFirstNote()
{
    if(m_proxyModel->rowCount() > 0){
        QModelIndex index = m_proxyModel->index(0,0);
        m_noteView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        m_noteView->setCurrentIndex(index);

        m_currentSelectedNoteProxy = index;
        // TODO showNoteInEditor(index);
    }
}

void NoteWidget::findNotesContain(const QString& keyword)
{
    m_proxyModel->setFilterFixedString(keyword);
    m_clearButton->show();

    // TODO m_textEdit->blockSignals(true);
    // TODO m_textEdit->clear();
    // TODO m_editorDateLabel->clear();
    // TODO m_textEdit->blockSignals(false);

    if(m_proxyModel->rowCount() > 0){
        selectFirstNote();
    }else{
        m_currentSelectedNoteProxy = QModelIndex();
    }
}

void NoteWidget::moveNoteToTop()
{
    // check if the current note is note on the top of the list
    // if true move the note to the top
    if(m_currentSelectedNoteProxy.isValid()){

        m_noteView->scrollToTop();

        // move the current selected note to the top
        QModelIndex sourceIndex = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
        QModelIndex destinationIndex = m_noteModel->index(0);
        m_noteModel->moveRow(sourceIndex, sourceIndex.row(), destinationIndex, 0);

        // update the current item
        m_currentSelectedNoteProxy = m_proxyModel->mapFromSource(destinationIndex);
        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
    }else{
        qDebug() << QStringLiteral("MainWindow::moveNoteTop : m_currentSelectedNoteProxy not valid");
    }
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

void NoteWidget::loadNotes()
{
    QList<NoteData*> noteList; // TODO = m_dbManager->getAllNotes();

    if(!noteList.isEmpty()){
        m_noteModel->addListNote(noteList);
        m_noteModel->sort(0,Qt::AscendingOrder);
    }
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
        if(aboveIndex.isValid()){
            m_noteView->setCurrentIndex(aboveIndex);
            m_currentSelectedNoteProxy = aboveIndex;
            //TODO showNoteInEditor(m_currentSelectedNoteProxy);
        }
        m_noteView->setFocus();
    }
}

void NoteWidget::selectNoteDown()
{
    if(m_currentSelectedNoteProxy.isValid()){
        if(m_isTemp){
            deleteNote(m_currentSelectedNoteProxy, false);
            m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
            //TODO showNoteInEditor(m_currentSelectedNoteProxy);
        }else{
            int currentRow = m_noteView->currentIndex().row();
            QModelIndex belowIndex = m_noteView->model()->index(currentRow + 1, 0);
            if(belowIndex.isValid()){
                m_noteView->setCurrentIndex(belowIndex);
                m_currentSelectedNoteProxy = belowIndex;
                //TODO showNoteInEditor(m_currentSelectedNoteProxy);
            }
        }
        m_noteView->setFocus();
    }
}

void NoteWidget::createNewNote()
{
    // TODO if(!m_isOperationRunning){
        // TODO m_isOperationRunning = true;

        m_noteView->scrollToTop();

        // clear the textEdit
        // TODO m_textEdit->blockSignals(true);
        // TODO m_textEdit->clear();
        // TODO m_textEdit->setFocus();
        // TODO m_textEdit->blockSignals(false);

        if(!m_isTemp){
            ++m_noteCounter;
            NoteData* tmpNote = generateNote(m_noteCounter);
            m_isTemp = true;

            // insert the new note to NoteModel
            QModelIndex indexSrc = m_noteModel->insertNote(tmpNote, 0);

            // update the editor header date label
            // TODO QString dateTimeFromDB = tmpNote->lastModificationdateTime().toString(Qt::ISODate);
            // TODO QString dateTimeForEditor = getNoteDateEditor(dateTimeFromDB);
            // TODO m_editorDateLabel->setText(dateTimeForEditor);

            // update the current selected index
            m_currentSelectedNoteProxy = m_proxyModel->mapFromSource(indexSrc);

        }else{
            int row = m_currentSelectedNoteProxy.row();
            m_noteView->animateAddedRow(QModelIndex(),row, row);
        }

        m_noteView->setCurrentIndex(m_currentSelectedNoteProxy);
        // TODO m_isOperationRunning = false;
        // TODO }
}

void NoteWidget::deleteSelectedNote()
{
    // TODO if(!m_isOperationRunning){
    // TODO    m_isOperationRunning = true;
        if(m_currentSelectedNoteProxy.isValid()){

            // update the index of the selected note before searching
            if(!m_searchEdit->text().isEmpty()){
                QModelIndex currentIndexInSource = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
                int beforeSearchSelectedRow = m_selectedNoteBeforeSearchingInSource.row();
                if(currentIndexInSource.row() < beforeSearchSelectedRow){
                    m_selectedNoteBeforeSearchingInSource = m_noteModel->index(beforeSearchSelectedRow-1);
                }
            }

            deleteNote(m_currentSelectedNoteProxy, true);
            // TODO showNoteInEditor(m_noteWidget->m_currentSelectedNoteProxy);
        }
    // TODO   m_isOperationRunning = false;
    // TODO }
}

void NoteWidget::deleteNote(const QModelIndex& noteIndex, bool isFromUser)
{
    if(noteIndex.isValid()){
        // delete from model
        QModelIndex indexToBeRemoved = m_proxyModel->mapToSource(m_currentSelectedNoteProxy);
        NoteData* noteTobeRemoved = m_noteModel->removeNote(indexToBeRemoved);

        if(m_isTemp){
            m_isTemp = false;
            --m_noteCounter;
        }else{
            noteTobeRemoved->setDeletionDateTime(QDateTime::currentDateTime());
            // TODO QtConcurrent::run(m_dbManager, &DBManager::removeNote, noteTobeRemoved);
        }

        if(isFromUser){
            // clear text edit and time date label
            // TODO m_editorDateLabel->clear();
            // TODO m_textEdit->blockSignals(true);
            // TODO m_textEdit->clear();
            // TODO m_textEdit->clearFocus();
            // TODO m_textEdit->blockSignals(false);

            if(m_noteModel->rowCount() > 0){
                QModelIndex index = m_noteView->currentIndex();
                m_currentSelectedNoteProxy = index;
            }else{
                m_currentSelectedNoteProxy = QModelIndex();
            }
        }
    }else{
        qDebug() << "MainWindow::deleteNote noteIndex is not valid";
    }

    m_noteView->setFocus();
}

void NoteWidget::onClearButtonClicked()
{
    // TODO if(!m_isOperationRunning){

        clearSearch();

        if(m_noteModel->rowCount() > 0){
            QModelIndex indexInProxy = m_proxyModel->mapFromSource(m_selectedNoteBeforeSearchingInSource);
            int row = m_selectedNoteBeforeSearchingInSource.row();
            if(row == m_noteModel->rowCount())
                indexInProxy = m_proxyModel->index(m_proxyModel->rowCount()-1,0);

            selectNote(indexInProxy);
        }else{
            m_currentSelectedNoteProxy = QModelIndex();
        }

        m_selectedNoteBeforeSearchingInSource = QModelIndex();

    // TODO }
}

void NoteWidget::onTrashButtonPressed()
{
    m_trashButton->setIcon(QIcon(":/images/trashCan_Pressed.png"));
}

void NoteWidget::onTrashButtonClicked()
{
    m_trashButton->setIcon(QIcon(":/images/trashCan_Regular.png"));

    m_trashButton->blockSignals(true);
    deleteSelectedNote();
    m_trashButton->blockSignals(false);
}

void NoteWidget::onNewNoteButtonPressed()
{
    m_newNoteButton->setIcon(QIcon(":/images/newNote_Pressed.png"));
}

void NoteWidget::onNewNoteButtonClicked()
{
    m_newNoteButton->setIcon(QIcon(":/images/newNote_Regular.png"));

    if(!m_searchEdit->text().isEmpty()){
        clearSearch();
        m_selectedNoteBeforeSearchingInSource = QModelIndex();
    }

    // save the data of the previous selected
    if(m_currentSelectedNoteProxy.isValid()
            && m_isContentModified){

        // TODO saveNoteToDB(m_currentSelectedNoteProxy);
        m_isContentModified = false;
    }

    createNewNote();
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
    // TODO m_textEdit->clearFocus();
    m_searchQueue.enqueue(keyword);

    // TODO if(!m_isOperationRunning){
    // TODO    m_isOperationRunning = true;
        if(m_isTemp){
            m_isTemp = false;
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

        if(m_currentSelectedNoteProxy.isValid()
                && m_isContentModified){

            // TODO saveNoteToDB(m_noteWidget->m_currentSelectedNoteProxy);
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
        // TODO m_isOperationRunning = false;
    // TODO }

    // TODO highlightSearch();
}

