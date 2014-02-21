
#include <QFileSystemModel>
#include "directorymodel.h"

namespace Coquillo {
    DirectoryModel::DirectoryModel(QObject * parent)
    : QSortFilterProxyModel(parent), _recursive(false) {
        QFileSystemModel * filesystem = new QFileSystemModel(this);
        filesystem->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
        setSourceModel(filesystem);
    }

    QFileSystemModel * DirectoryModel::sourceModel() const {
        return qobject_cast<QFileSystemModel *>(QSortFilterProxyModel::sourceModel());
    }

    QModelIndex DirectoryModel::index(const QString & path, int column) const {
        return mapFromSource(sourceModel()->index(path, column));
    }

    Qt::ItemFlags DirectoryModel::flags(const QModelIndex & idx) const {
        Qt::ItemFlags f = QSortFilterProxyModel::flags(idx);
        if (idx.column() == 0 && !(_recursive && isAncestorChecked(idx))) {
            f |= Qt::ItemIsUserCheckable;
        }
        return f;
    }

    QVariant DirectoryModel::data(const QModelIndex & idx, int role) const {
        if (!idx.isValid()) {
            return QVariant();
        }

        if (role == Qt::CheckStateRole) {
            bool contains = _checked.contains(idx.data(QFileSystemModel::FilePathRole).toString());

            if (contains) {
                return Qt::Checked;
            } else {
                if (isDescendantChecked(idx) || (_recursive && isAncestorChecked(idx))) {
                    return Qt::PartiallyChecked;
                } else {
                    return Qt::Unchecked;
                }
            }
        }

        return QSortFilterProxyModel::data(idx, role);
    }

    bool DirectoryModel::setData(const QModelIndex & idx, const QVariant & value, int role) {
        if (role == Qt::CheckStateRole) {
            QString path = idx.data(QFileSystemModel::FilePathRole).toString();

            if (value.toBool()) {
                _checked << path;
                emit pathChecked(path, _recursive);
            } else {
                _checked.removeAll(path);
                emit pathUnchecked(path, _recursive);
            }

            emit dataChanged(idx.sibling(idx.row(), 0), idx.sibling(idx.row(), columnCount() - 1));

            if (recursiveScanEnabled()) {
                recursiveDataChanged(idx);
            }
        }

        return QSortFilterProxyModel::setData(idx, value, role);
    }

    void DirectoryModel::uncheckAll() {
        while (!_checked.isEmpty()) {
            const QString dir = _checked.takeFirst();
            const QModelIndex idx = mapFromSource(sourceModel()->index(dir));
            emit dataChanged(idx.sibling(idx.row(), 0), idx.sibling(idx.row(), columnCount() - 1));
            emit pathUnchecked(dir, _recursive);
        }
    }

    bool DirectoryModel::isAncestorChecked(const QModelIndex & idx) const {
        QModelIndex tmp(idx);

        while (tmp.parent().isValid()) {
            if (tmp.parent().data(Qt::CheckStateRole).toInt() == Qt::Checked) {
                return true;
            }

            tmp = tmp.parent();
        }

        return false;
    }

    bool DirectoryModel::isDescendantChecked(const QModelIndex & idx) const {
        QString path = idx.data(QFileSystemModel::FilePathRole).toString() + QDir::separator();

        foreach (QString p, _checked) {
            if (p.startsWith(path)) {
                return true;
            }
        }
        return false;
    }

    void DirectoryModel::recursiveDataChanged(const QModelIndex & parent) {
        int row_count = parent.model()->rowCount(parent);

        if (row_count > 0) {
            int col_count = parent.model()->columnCount(parent);
            emit dataChanged(parent.child(0, 0), parent.child(row_count - 1, col_count - 1));

            for (int i = 0; i < row_count; i++) {
                recursiveDataChanged(parent.child(i, 0));
            }
        }
    }
}
