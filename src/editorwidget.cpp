#include "editorwidget.h"
#include "ui_editorwidget.h"
#include "notemodel.h"

#include <QScrollBar>
#include <QDateTime>
#include <QDebug>

EditorWidget::EditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EditorWidget),
    m_textEdit(Q_NULLPTR),
    m_editorDateLabel(Q_NULLPTR),
    m_isTextEditable(true)
{
    ui->setupUi(this);

    setupEditor();
    setupTextEdit();

    // text edit text changed / lineEdit text changed
    connect(m_textEdit, &QTextEdit::textChanged, this, &EditorWidget::onTextEditTextChanged);
}

EditorWidget::~EditorWidget()
{
    delete ui;
}

bool EditorWidget::eventFilter(QObject* object, QEvent* event)
{
    switch (event->type()) {
    case QEvent::HoverEnter:
        if(object == m_textEdit->verticalScrollBar()){
            if(!m_textEdit->hasFocus()){
                m_textEdit->setFocusPolicy(Qt::NoFocus);
            }
        }
        break;

    case QEvent::HoverLeave:
        if(object == m_textEdit->verticalScrollBar()){
            bool isNoButtonClicked = qApp->mouseButtons() == Qt::NoButton;
            if(isNoButtonClicked){
                m_textEdit->setFocusPolicy(Qt::StrongFocus);
            }
        }
        break;

    case QEvent::MouseButtonRelease:
        if(object == m_textEdit->verticalScrollBar()){
            QWidget* w = qApp->widgetAt(QCursor::pos());
            bool isMouseOnScrollBar = (w != m_textEdit->verticalScrollBar());
            if(isMouseOnScrollBar){
                m_textEdit->setFocusPolicy(Qt::StrongFocus);
            }
        }
        break;

    case QEvent::FocusIn:
        if(object == m_textEdit){
            emit editorFocusedIn();
            m_textEdit->setFocus();
        }
        break;

    case QEvent::FocusOut:
        if(object == m_textEdit)
            m_textEdit->setReadOnly(true);

        break;
    default:
        break;
    }

    return QObject::eventFilter(object, event);
}

void EditorWidget::focusInEvent(QFocusEvent* event)
{
   m_textEdit->setFocus();
   QWidget::focusInEvent(event);
}

void EditorWidget::setupEditor()
{
    m_textEdit = ui->textEdit;
    m_editorDateLabel = ui->editorDateLabel;
}

void EditorWidget::setTextEditable(bool isTextEditable)
{
    m_isTextEditable = isTextEditable;
    m_textEdit->setReadOnly(!isTextEditable);
}

void EditorWidget::setupTextEdit()
{
    m_textEdit->installEventFilter(this);
    m_textEdit->verticalScrollBar()->installEventFilter(this);
}

void EditorWidget::clearTextAndHeader()
{
    // clear text edit and time date label
    m_editorDateLabel->clear();
    m_textEdit->blockSignals(true);
    m_textEdit->clear();
    m_textEdit->clearFocus();
    m_textEdit->blockSignals(false);
}

/**
 * @brief show the specified note content text in the text editor
 * Set editorDateLabel text to the the selected note date
 * And restore the scrollBar position if it changed before.
 */
void EditorWidget::showNoteInEditor(const QModelIndex &noteIndex)
{
    Q_ASSERT_X(noteIndex.isValid(), "MainWindow::showNoteInEditor", "noteIndex is not valid");

    m_textEdit->blockSignals(true);
    QString content = noteIndex.data(NoteModel::NoteContent).toString();
    QDateTime dateTime = noteIndex.data(NoteModel::NoteLastModificationDateTime).toDateTime();
    int scrollbarPos = noteIndex.data(NoteModel::NoteScrollbarPos).toInt();

    // set text and date
    m_textEdit->setText(content);
    QString noteDate = dateTime.toString(Qt::ISODate);
    QString noteDateEditor = dateToLocal(noteDate);
    m_editorDateLabel->setText(noteDateEditor);
    // set scrollbar position
    m_textEdit->verticalScrollBar()->setValue(scrollbarPos);
    m_textEdit->blockSignals(false);
}

int EditorWidget::scrollBarValue()
{
    return m_textEdit->verticalScrollBar()->value();
}

QDateTime EditorWidget::getQDateTime (QString date)
{
    QDateTime dateTime = QDateTime::fromString(date, Qt::ISODate);
    return dateTime;
}

QString EditorWidget::dateToLocal (QString dateString)
{
    QDateTime dateTimeEdited(getQDateTime(dateString));
    QLocale usLocale(QLocale("en_US"));

    return usLocale.toString(dateTimeEdited, "MMMM d, yyyy, h:mm A");
}

void EditorWidget::onTextEditTextChanged ()
{
    m_textEdit->blockSignals(true);

    QDateTime dateTime = QDateTime::currentDateTime();
    QString noteDate = dateTime.toString(Qt::ISODate);
    m_editorDateLabel->setText(dateToLocal(noteDate));

    emit editorTextChanged(m_textEdit->toPlainText());

    m_textEdit->blockSignals(false);
}
