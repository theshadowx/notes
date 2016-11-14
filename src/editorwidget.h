#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QLabel>

namespace Ui {
    class EditorWidget;
    }

class EditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorWidget(QWidget *parent = 0);
    ~EditorWidget();

    void setTextEditable(bool isTextEditable);
    void clearTextAndHeader();
    void showNoteInEditor(const QModelIndex& noteIndex);
    int scrollBarValue();

protected:
    bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;
    void focusInEvent(QFocusEvent *event) Q_DECL_OVERRIDE;

private:
    Ui::EditorWidget *ui;

    QTextEdit* m_textEdit;
    QLabel* m_editorDateLabel;

    bool m_isTextEditable;

    void setupEditor();
    void setupTextEdit();
    QString dateToLocal (QString dateString);
    QDateTime getQDateTime(QString date);

public slots:
    void onTextEditTextChanged();

signals:
    void editorTextChanged(const QString& text);
    void editorFocusedIn();
};

#endif // EDITORWIDGET_H
