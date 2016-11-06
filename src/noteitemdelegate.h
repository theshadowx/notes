#ifndef NOTEITEMDELEGATE_H
#define NOTEITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTimeLine>
#include <QListView>

class NoteItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit NoteItemDelegate(QObject* parent = Q_NULLPTR);

    enum States{
        Normal,
        Insert,
        Remove,
        MoveOut,
        MoveIn
    };

    void setState(States NewState , QModelIndex index);
    void setAnimationDuration(const int duration);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const Q_DECL_OVERRIDE;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const Q_DECL_OVERRIDE;

    QTimeLine::State animationState();

private:
    void paintBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index)const;
    void paintLabels(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paintTags(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paintSeparator(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    int initialHeightOfRow(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    QString parseDateTime(const QDateTime& dateTime) const;

    States m_state;
    bool m_isActive;

    QTimeLine* m_timeLine;
    QModelIndex m_animatedIndex;
    QModelIndex m_currentSelectedIndex;
    QModelIndex m_hoveredIndex;

    QListView* m_view;

signals:
    void update(const QModelIndex &index);
};

#endif // NOTEITEMDELEGATE_H
