#include "tagnotemodel.h"
#include <QDebug>

TagNoteModel::TagNoteModel(QObject* parent):
    QAbstractListModel(parent)
{

}

QVariant TagNoteModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || m_tagModel == Q_NULLPTR)
        return QVariant();

    QModelIndex indexInTagModel = m_tagModel->index(index.row());

    switch (role) {
    case Qt::CheckStateRole:{
        int id = index.data(TagModel::TagID).toInt();
        return m_checkedTags.contains(id)? Qt::Checked : Qt::Unchecked;
    }
    default:
        return indexInTagModel.data(role);
    }
}

bool TagNoteModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || m_tagModel == Q_NULLPTR)
        return false;

    bool success = false;
    QModelIndex indexInTagModel = m_tagModel->index(index.row());

    switch (role) {
    case Qt::CheckStateRole:{
        int id = indexInTagModel.data(TagModel::TagID).toInt();
        if(value.toBool()){
            m_checkedTags.insert(id);
        }else{
            m_checkedTags.remove(id);
        }

        success = true;
        break;
    }
    default:
        m_tagModel->setData(indexInTagModel, value, role);
        break;
    }

    if(success)
        emit dataChanged(index, index, QVector<int>(1,role));

    return success;
}

Qt::ItemFlags TagNoteModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
}

int TagNoteModel::rowCount(const QModelIndex& parent) const
{
    if(m_tagModel == Q_NULLPTR)
        return 0;

    return m_tagModel->rowCount(parent);
}

void TagNoteModel::setTagModel(TagModel* tagModel)
{
    m_tagModel = tagModel;
}
