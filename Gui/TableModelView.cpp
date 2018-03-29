/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TableModelView.h"

#include <set>
#include <algorithm> // min, max
#include <vector>
#include <stdexcept>
#include <climits>
#include <cfloat>
#include <map>

#include <QApplication>
#include <QHeaderView>
#include <QMouseEvent>
#include <QStyle>
#include <QScrollBar>
#include <QDebug>
#include <QPainter>
#include <QVariant>

#include "Gui/Gui.h"
#include "Gui/GuiMacros.h"
#include "Gui/Label.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/ComboBox.h"
#include "Gui/SpinBox.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiApplicationManager.h"

NATRON_NAMESPACE_ENTER

//////////////TableItem


NATRON_NAMESPACE_ANONYMOUS_ENTER

class MetaTypesRegistration
{
public:
    inline MetaTypesRegistration()
    {
        qRegisterMetaType<TableItemPtr>("TableItemPtr");
    }
};

typedef std::vector<TableItemPtr> TableItemVec;
NATRON_NAMESPACE_ANONYMOUS_EXIT


static MetaTypesRegistration registration;

struct TableItemPrivate
{

    struct ColumnData
    {
        // For each role index, its value as a variant
        std::map<int, QVariant> values;

        // Flags controlling the UI
        Qt::ItemFlags itemFlags;

        ColumnData()
        : values()
        , itemFlags(Qt::ItemIsEditable
                    | Qt::ItemIsSelectable
                    | Qt::ItemIsUserCheckable
                    | Qt::ItemIsEnabled
                    | Qt::ItemIsDragEnabled
                    | Qt::ItemIsDropEnabled)
        {

        }
    };

    // Per columns data. The number of columns must match the columns count of the model
    std::vector<ColumnData> columns;

    // Weak ref to the model
    TableModelWPtr model;

    // The index of the item in the parent storage for fast access.
    int indexInParent;

    // Strong ref to children
    TableItemVec children;

    // Weak ref to column 0 item of the parent
    TableItemWPtr parent;

    TableItem::ChildIndicatorVisibilityEnum childIndicatorPolicy;


    TableItemPrivate(const TableModelPtr& model)
    : columns()
    , model()
    , indexInParent(-1)
    , children()
    , parent()
    , childIndicatorPolicy(TableItem::eChildIndicatorVisibilityShowIfChildren)
    {
        if (model) {
            setModelAndInitColumns(model);
        }
    }

    void setModelAndInitColumns(const TableModelPtr& m)
    {
        model = m;
        int nCols = m ? m->columnCount() : 0;
        if ((int)columns.size() != nCols) {
            columns.resize(nCols);
        }
        for (std::size_t i = 0; i < children.size(); ++i) {
            children[i]->_imp->setModelAndInitColumns(m);
        }
    }

    // Ensure the col index is ok, otherwise throw an exc
    void ensureColumnOk(int col) {
        assert(col >= 0 && col < (int)columns.size());
        if (col < 0 || col >= (int)columns.size()) {
            throw std::invalid_argument("TableItem: wrong column index");
        }
    }

    void insertColumnsRecursively(int column, int count)
    {
        assert(column >= 0 && column <= (int)columns.size());
        if (column == (int)columns.size()) {
            columns.resize(columns.size() + count);
        } else {
            std::vector<ColumnData>::iterator it = columns.begin();
            std::advance(it, column);
            columns.insert(it, count, ColumnData());
        }

        for (std::size_t i = 0; i < children.size(); ++i) {
            children[i]->_imp->insertColumnsRecursively(column, count);
        }
    }

    void removeColumnsRecursively(int column, int count)
    {
        assert(column >= 0 && column <= (int)columns.size());

        std::vector<ColumnData>::iterator it = columns.begin();
        std::advance(it, column);

        std::vector<ColumnData>::iterator last = it;
        if (column + count < (int)columns.size()) {
            std::advance(last, count);
        } else {
            last = columns.end();
        }

        columns.erase(it, last);


        for (std::size_t i = 0; i < children.size(); ++i) {
            children[i]->_imp->removeColumnsRecursively(column, count);
        }
    }

};

struct TableModelPrivate
{
    TableModel::TableModelTypeEnum type;

    // For table type, this is all items. For trees, this is only toplevel items
    TableItemVec topLevelItems;

    // The header item
    TableItemPtr headerItem;

    int colCount;

    Gui* gui;

    TableModelPrivate(int columns, TableModel::TableModelTypeEnum type)
    : type(type)
    , topLevelItems()
    , headerItem()
    , colCount(columns)
    , gui(0)
    {
        assert(columns > 0);
        if (columns == 0) {
            throw std::invalid_argument("TableModel: Must have at least 1 column");
        }
    }

    int getTopLevelItemIndex(const TableItemConstPtr item) const
    {
        for (std::size_t i = 0; i < topLevelItems.size(); ++i) {
            if (topLevelItems[i] == item) {
                return (int)i;
            }
        }

        return -1;
    }
};


TableItem::TableItem(const TableModelPtr& model)
: _imp(new TableItemPrivate(model))
{
}


TableItem::~TableItem()
{
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct TableItem::MakeSharedEnabler: public TableItem
{
    MakeSharedEnabler(const TableModelPtr& model) : TableItem(model) {
    }
};


TableItemPtr
TableItem::create(const TableModelPtr& model, const TableItemPtr& parent)
{
    TableItemPtr ret = boost::make_shared<TableItem::MakeSharedEnabler>(model);
    if (parent) {
        // Inserting the item in the parent may not succeed if parent is not in a model already.
        parent->insertChild(-1, ret);
    }
    return ret;
}


TableItemPtr
TableItem::getParentItem() const
{
    return _imp->parent.lock();
}

const TableItemVec&
TableItem::getChildren() const
{
    return _imp->children;
}

bool
TableItem::isChildRecursive(const TableItemPtr& item) const
{
    for (std::size_t i = 0; i < _imp->children.size(); ++i) {
        if (_imp->children[i] == item) {
            return true;
        }
        bool recursiveRet = _imp->children[i]->isChildRecursive(item);
        if (recursiveRet) {
            return true;
        }
    }
    return false;
}

bool
TableItem::insertChild(int row, const TableItemPtr& child)
{
    TableModelPtr model = getModel();
    if (!model) {
        // Item is not in a model
        return false;
    } else if (model->getType() != TableModel::eTableModelTypeTree) {
        return false;
    }
    
    if (row > (int)_imp->children.size()) {
        return false;
    }
    
    // Remove the child from its original parent/model
    {
        TableModelPtr childModel = child->getModel();
        if (childModel) {
            childModel->removeItem(child);
        }
    }

    TableItemPtr thisShared = shared_from_this();
    child->_imp->parent = thisShared;
    child->_imp->setModelAndInitColumns(model);
    
    QModelIndex thisIdx = model->getItemIndex(thisShared);
    if (row == -1 || row >= (int)_imp->children.size()) {
        model->beginInsertRows(thisIdx, _imp->children.size(), _imp->children.size());
        child->_imp->indexInParent = _imp->children.size();
        _imp->children.push_back(child);
        model->endInsertRows();
    } else {
        model->beginInsertRows(thisIdx, row, row);
        TableItemVec::iterator it = _imp->children.begin();
        if (row > 0) {
            std::advance(it, row);
        }
        it = _imp->children.insert(it, child);
        int index = row;
        for (TableItemVec::iterator p = it; p != _imp->children.end(); ++p, ++index) {
            (*p)->_imp->indexInParent = index;
        }
        model->endInsertRows();
    }
    return true;
}

bool
TableItem::removeChild(const TableItemPtr& child)
{
    if (!child) {
        return false;
    }
    if (child->getParentItem().get() != this) {
        return false;
    }
    
    TableModelPtr model = getModel();
    
    
    int i = 0;
    for (TableItemVec::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it, ++i) {
        if (*it == child) {
            
            if (model) {
                TableItemPtr thisShared = shared_from_this();
                QModelIndex thisIdx = model->getItemIndex(thisShared);
                model->beginRemoveRows(thisIdx, i, i);
            }
            
            child->_imp->parent.reset();
            child->_imp->model.reset();
            child->_imp->indexInParent = - 1;
            
            it = _imp->children.erase(it);
            for (; it != _imp->children.end(); ++it) {
                --(*it)->_imp->indexInParent;
            }
            if (model) {
                model->endRemoveRows();
            }

            
            return true;
        }
    }
    return false;
}

TableModelPtr
TableItem::getModel() const
{
    return _imp->model.lock();
}

TableItem::ChildIndicatorVisibilityEnum
TableItem::getChildIndicatorVisibility() const
{
    return _imp->childIndicatorPolicy;
}

void
TableItem::setChildIndicatorVisibility(ChildIndicatorVisibilityEnum visibility)
{
    _imp->childIndicatorPolicy = visibility;
}

Qt::ItemFlags
TableItem::getFlags(int col) const
{
    _imp->ensureColumnOk(col);
    return _imp->columns[col].itemFlags;
}

int
TableItem::getRowInParent() const
{
    return _imp->indexInParent;
}

void
TableItem::setIcon(int col, const QIcon &aicon)
{
    setData(col, Qt::DecorationRole, aicon);
}


void
TableItem::setFlags(int col, Qt::ItemFlags flags)
{
    _imp->ensureColumnOk(col);
    _imp->columns[col].itemFlags = flags;
    TableModelPtr model = getModel();
    if (model) {
        model->onItemChanged(shared_from_this(), col, Qt::DisplayRole, false);
    }
}

QVariant
TableItem::getData(int col, int role) const
{
    _imp->ensureColumnOk(col);
    role = (role == Qt::EditRole ? Qt::DisplayRole : role);

    std::map<int, QVariant>::const_iterator foundRole = _imp->columns[col].values.find(role);
    if (foundRole == _imp->columns[col].values.end()) {
        return QVariant();
    }
    return foundRole->second;
}

void
TableItem::setData(int col, int role, const QVariant &value)
{
    _imp->ensureColumnOk(col);
    
    role = (role == Qt::EditRole ? Qt::DisplayRole : role);

    QVariant& curValue = _imp->columns[col].values[role];
    if (curValue == value) {
        // Nothing changed
        return;
    }
    curValue = value;

    TableModelPtr model = getModel();
    if (model) {
        model->onItemChanged(shared_from_this(), col, role, true);
    }
}


///////////////TableModel


TableModel::TableModel(int columns, TableModelTypeEnum type)
    : QAbstractTableModel()
    , _imp( new TableModelPrivate(columns, type) )
{
    QObject::connect( this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(onDataChanged(QModelIndex, QModelIndex)) );
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct TableModel::MakeSharedEnabler: public TableModel
{
    MakeSharedEnabler(int columns, TableModelTypeEnum type) : TableModel(columns, type) {
    }
};


TableModelPtr
TableModel::create(int columns, TableModelTypeEnum type)
{
    return boost::make_shared<TableModel::MakeSharedEnabler>(columns, type);
}


TableModel::~TableModel()
{

}

TableModel::TableModelTypeEnum
TableModel::getType() const
{
    return _imp->type;
}


void
TableItem::refreshChildrenIndices()
{
    // Update table indices
    for (std::size_t i = 0; i < _imp->children.size(); ++i) {
        if (_imp->children[i]) {
            _imp->children[i]->_imp->indexInParent = i;
        }
    }
}

void
TableModel::refreshTopLevelItemIndices()
{
    // Update table indices
    for (std::size_t i = 0; i < _imp->topLevelItems.size(); ++i) {
        if (_imp->topLevelItems[i]) {
            _imp->topLevelItems[i]->_imp->indexInParent = i;
        }
    }
}


void
TableModel::onDataChanged(const QModelIndex & tl, const QModelIndex& br)
{
    for (int row = tl.row(); row <= br.row(); ++row) {
        TableItemPtr i = getItem(row);
        if (i) {
            Q_EMIT s_itemChanged(i);
        }

    }
}

bool
TableModel::insertRows(int row, int count, const QModelIndex & parent)
{
    // Only valid if:
    // - at least 1 row must be added
    // - the index of the row corresponds to a valid index or to rowCount in which case we append rows
    // - the parent column is 0 or the parent is invalid
    if ( (count < 1) || (row < 0) || (row > rowCount(parent) || parent.column() > 0) ) {
        return false;
    }

    beginInsertRows(parent, row, row + count - 1);

    if (_imp->type == eTableModelTypeTable) {

        if (_imp->topLevelItems.empty()) {
            _imp->topLevelItems.resize(_imp->colCount * count);
        } else {

            // Find the item corresponding to the row index

            if ( row < (int)_imp->topLevelItems.size() ) {
                // The index is valid in the table, insert the appropriate number of items
                TableItemVec::iterator it = _imp->topLevelItems.begin();
                std::advance(it, row);
                _imp->topLevelItems.insert(it, count, TableItemPtr());
            } else {
                // The index does not exist, append rows
                _imp->topLevelItems.insert(_imp->topLevelItems.end(), count, TableItemPtr());
            }
        }

        // Refresh indices to speed up the table type
        refreshTopLevelItemIndices();

    } else {
        TableModelPtr thisShared = shared_from_this();

        // If there's a parent item, this is a pointer to it
        TableItemPtr parentItem = getItem(parent);

        int row_i = row;
        int rowsToAdd = count;
        while (rowsToAdd > 0) {
            TableItemPtr newItem = TableItem::create(thisShared);
            newItem->_imp->parent = parentItem;

            if (parentItem) {
                if (row_i > (int)parentItem->_imp->children.size()) {
                    parentItem->_imp->children.push_back(newItem);
                } else {
                    TableItemVec::iterator it = parentItem->_imp->children.begin();
                    std::advance(it, row_i);
                    parentItem->_imp->children.insert(it, newItem);
                }
                ++row_i;
            } else {
                if (row_i > (int)_imp->topLevelItems.size()) {
                    _imp->topLevelItems.push_back(newItem);
                } else {
                    TableItemVec::iterator it = _imp->topLevelItems.begin();
                    std::advance(it, row_i);
                    _imp->topLevelItems.insert(it, newItem);
                }
                ++row_i;

            }
            --rowsToAdd;
        }
        if (parentItem) {
            parentItem->refreshChildrenIndices();
        } else {
            refreshTopLevelItemIndices();
        }
    }

    endInsertRows();

    return true;
}

bool
TableModel::insertColumns(int column,
                          int count,
                          const QModelIndex & parent)
{
    // Only Valid if:
    // - column is in colsCount range or it is at colsCount
    // - count is >= 1
    // - parent is invalid, because in our model all rows are expected to have the same number of columns
    if ( (count < 1) || (column < 0) || ( column > (int)_imp->colCount ) || parent.isValid() ) {
        return false;
    }

    beginInsertColumns(parent, column, column +  count - 1);

    for (std::size_t i = 0; i < _imp->topLevelItems.size(); ++i) {
        _imp->topLevelItems[i]->_imp->insertColumnsRecursively(column, count);
    }

    _imp->headerItem->_imp->insertColumnsRecursively(column, count);

    endInsertColumns();

    return true;
}

bool
TableModel::removeRows(int row, int count, const QModelIndex & parent)
{
    // Check for invalid row or count
    if ( (count < 1) || (row < 0) || (row + count > rowCount(parent)) ) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);

    TableItemPtr parentItem = getItem(parent);
    if (parentItem) {
        TableItemVec::iterator pos = parentItem->_imp->children.begin();
        std::advance(pos, row);
        TableItemVec::iterator last = pos;
        if ( (row + count) < (int)parentItem->_imp->children.size() ) {
            std::advance(last, count);
        } else {
            last = parentItem->_imp->children.end();
        }
        for (TableItemVec::iterator it = pos; it != last; ++it) {
            if ((*it)) {
                (*it)->_imp->model.reset();
            }
        }
        parentItem->_imp->children.erase(pos, last);

        parentItem->refreshChildrenIndices();
    } else {
        TableItemVec::iterator pos = _imp->topLevelItems.begin();
        std::advance(pos, row);
        TableItemVec::iterator last = pos;
        if ( (row + count) < (int)_imp->topLevelItems.size() ) {
            std::advance(last, count);
        } else {
            last = _imp->topLevelItems.end();
        }
        for (TableItemVec::iterator it = pos; it != last; ++it) {
            if ((*it)) {
                // Reset index in parent and model ptr in case someone else holds a ref to this item
                (*it)->_imp->indexInParent = -1;
                (*it)->_imp->model.reset();
            }
        }
        _imp->topLevelItems.erase(pos, last);

        refreshTopLevelItemIndices();
    }


    endRemoveRows();

    return true;
}

bool
TableModel::removeColumns(int column,
                          int count,
                          const QModelIndex & parent)
{
    if ( (count < 1) || (column < 0) || ( column + count > _imp->colCount ) ) {
        return false;
    }

    beginRemoveColumns(parent, column, column + count - 1);
    TableItemPtr oldItem;

    for (std::size_t i = 0; i < _imp->topLevelItems.size(); ++i) {
        _imp->topLevelItems[i]->_imp->removeColumnsRecursively(column, count);
    }

    _imp->headerItem->_imp->removeColumnsRecursively(column, count);

    endRemoveColumns();

    return true;
} // removeColumns

void
TableModel::setTable(const TableItemVec& items)
{
    if (_imp->type != eTableModelTypeTable) {
        throw std::logic_error("TableModel::setRowCount: call to setRowCount for a tree model");
    }

    beginRemoveRows( QModelIndex(), 0, std::max(0, (int)_imp->topLevelItems.size() - 1) );
    for (std::size_t i = 0; i < _imp->topLevelItems.size(); ++i) {
        if (_imp->topLevelItems[i]) {
            // Reset index in parent and model ptr in case someone else holds a ref to this item
            _imp->topLevelItems[i]->_imp->indexInParent = -1;
            _imp->topLevelItems[i]->_imp->model.reset();
            _imp->topLevelItems[i].reset();
        }
    }
    endRemoveRows();

    _imp->topLevelItems = items;

    TableModelPtr thisShared = shared_from_this();


    beginInsertRows( QModelIndex(), 0, std::max(0, (int)_imp->topLevelItems.size() - 1) );
    for (int i = 0; i < (int)_imp->topLevelItems.size(); ++i) {
        if (_imp->topLevelItems[i]) {
            _imp->topLevelItems[i]->_imp->indexInParent = i;
            _imp->topLevelItems[i]->_imp->setModelAndInitColumns(thisShared);
        }
    }

    endInsertRows();

    TableItemPtr firstItem, lastItem;
    if (!items.empty()) {
        firstItem = items.front();
        lastItem = items.back();
    }
    // Notify the view that we changed
    QModelIndex tl = createIndex(0, 0, firstItem.get());
    QModelIndex br = createIndex(std::max(0, (int)items.size() - 1), 0, lastItem.get());
    Q_EMIT dataChanged(tl, br);

}

void
TableModel::setRowCount(int rows)
{
    if (_imp->type != eTableModelTypeTable) {
        throw std::logic_error("TableModel::setRowCount: call to setRowCount for a tree model");
    }
    if ( (rows < 0) || ((int)_imp->topLevelItems.size() == rows) ) {
        return;
    }
    int curItems = (int)_imp->topLevelItems.size();
    if (curItems < rows) {
        insertRows(curItems, rows - curItems);
    } else {
        removeRows(rows, curItems - rows);
    }
}

void
TableModel::setRow(int row, const TableItemPtr& item)
{
    if (_imp->type != eTableModelTypeTable) {
        throw std::logic_error("TableModel::setRowCount: call to setRowCount for a tree model");
    }
    if (row < 0 || row >= (int)_imp->topLevelItems.size()) {
        return;
    }
    QModelIndex first, last;
    if (!item) {
        // Delete item
        if (_imp->topLevelItems[row]) {

            // Reset index in parent and model ptr in case someone else holds a ref to this item
            _imp->topLevelItems[row]->_imp->indexInParent = -1;
            _imp->topLevelItems[row]->_imp->model.reset();
            first = createIndex(row, 0, 0);
            last = createIndex(row, _imp->colCount - 1, 0);
            _imp->topLevelItems[row].reset();
        }
    } else {
        item->_imp->setModelAndInitColumns(shared_from_this());
        item->_imp->indexInParent = row;
        _imp->topLevelItems[row] = item;
        first = createIndex(row, 0, item.get());
        last = createIndex(row, _imp->colCount - 1, item.get());
    }

    // Refresh the view
    Q_EMIT dataChanged(first, last);
}

bool
TableModel::insertTopLevelItem(int row, const TableItemPtr& item)
{
   
    if (!item) {
        return false;
    }
    if (row > (int)_imp->topLevelItems.size()) {
        return false;
    }

    item->_imp->setModelAndInitColumns(shared_from_this());

    
    if (row == -1 || row == (int)_imp->topLevelItems.size()) {
        beginInsertRows(QModelIndex(), _imp->topLevelItems.size(), _imp->topLevelItems.size());
        _imp->topLevelItems.push_back(item);
        item->_imp->indexInParent = _imp->topLevelItems.size() - 1;
        endInsertRows();
    } else {
        beginInsertRows(QModelIndex(), row, row);
        TableItemVec::iterator it = _imp->topLevelItems.begin();
        std::advance(it, row);
        it = _imp->topLevelItems.insert(it, item);
        
        int index = row;
        for (TableItemVec::iterator p = it; p != _imp->topLevelItems.end(); ++p, ++index) {
            (*p)->_imp->indexInParent = index;
        }
        endInsertRows();
        
    }


    /*QModelIndex idx = createIndex(item->_imp->indexInParent, 0, item.get());

    // Refresh the view
    Q_EMIT dataChanged(idx, idx);*/
    return true;
}

bool
TableModel::removeItem(const TableItemPtr& item)
{
  
    if (!item) {
        return false;
    }

    TableItemPtr parent = item->getParentItem();
    if (parent) {
        return parent->removeChild(item);
    } else {

        int i = 0;
        for (TableItemVec::iterator it = _imp->topLevelItems.begin(); it!=_imp->topLevelItems.end(); ++it, ++i) {
            if (*it == item) {

                beginRemoveRows(QModelIndex(), i, i);
                // Item no longer belongs to model
                assert(!item->_imp->parent.lock());
                item->_imp->indexInParent = -1;
                item->_imp->model.reset();
                it = _imp->topLevelItems.erase(it);
                for (; it != _imp->topLevelItems.end(); ++it) {
                    --item->_imp->indexInParent;
                }

                endRemoveRows();
                /*QModelIndex first = createIndex(i, 0, 0);
                 QModelIndex last = createIndex(i, _imp->colCount - 1, 0);

                 // Refresh the view
                 Q_EMIT dataChanged(first, last);*/
                
                return true;
            }
        }
        return false;

    }
    
}


TableItemPtr
TableModel::getItem(int row, const QModelIndex& parent) const
{
    if (_imp->type == eTableModelTypeTable || !parent.isValid()) {
        if (row < 0 || row >= (int)_imp->topLevelItems.size()) {
            return TableItemPtr();
        }
        return _imp->topLevelItems[row];
    } else {
        TableItem *parentItem = static_cast<TableItem*>(parent.internalPointer());
        assert(parentItem);
        if (!parentItem) {
            return TableItemPtr();
        }
        if (row < 0 || row >= (int)parentItem->_imp->children.size()) {
            return TableItemPtr();
        }
        return parentItem->_imp->children[row];

    }
}

TableItemPtr
TableModel::getItem(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return TableItemPtr();
    }
    return getItem(index.row(), index.parent());
}

QModelIndex
TableModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }
    TableItem *itm = static_cast<TableItem*>(child.internalPointer());
    if (!itm) {
        return QModelIndex();
    }
    TableItemPtr parent = itm->getParentItem();
    return getItemIndex(parent);

}

bool
TableModel::hasChildren(const QModelIndex & parent) const
{

    if (_imp->type != eTableModelTypeTree) {
        return QAbstractItemModel::hasChildren();
    }
    if (!parent.isValid()) {
        return _imp->topLevelItems.size() > 0;
    }
    TableItemPtr itm = getItem(parent);
    if (!itm) {
        return false;
    }
    switch (itm->getChildIndicatorVisibility()) {
        case TableItem::eChildIndicatorVisibilityShowAlways:
            return true;
        case TableItem::eChildIndicatorVisibilityShowIfChildren: {

            return itm->getChildren().size() > 0;
        }
        case TableItem::eChildIndicatorVisibilityShowNever:
            return false;
    }
    return false;
}

QModelIndex
TableModel::getItemIndex(const TableItemConstPtr& item) const
{
    // Item is not part of the model...
    if (!item || item->getModel().get() != this) {
        return QModelIndex();
    }

    // Use the index already computed on the item
    TableItemPtr parent = item->getParentItem();
    if (parent) {
        assert(item->_imp->indexInParent >= 0 && item->_imp->indexInParent < (int)parent->_imp->children.size());
        if (item->_imp->indexInParent >= 0 && item->_imp->indexInParent < (int)parent->_imp->children.size()) {
            return createIndex(item->_imp->indexInParent, 0, parent->_imp->children[item->_imp->indexInParent].get());
        } else {
            throw std::runtime_error("TableModel::getItemIndex: Item internal index in storage was wrong");
        }
    } else {
        assert(item->_imp->indexInParent >= 0 && item->_imp->indexInParent < (int)_imp->topLevelItems.size());
        if (item->_imp->indexInParent >= 0 && item->_imp->indexInParent < (int)_imp->topLevelItems.size()) {
            return createIndex(item->_imp->indexInParent, 0, _imp->topLevelItems[item->_imp->indexInParent].get());
        } else {
            throw std::runtime_error("TableModel::getItemIndex: Item internal index in storage was wrong");
        }
    }
}


QModelIndex
TableModel::index( int row, int column, const QModelIndex &parent) const
{
    TableItemPtr i = getItem(row, parent);
    return createIndex(row, column, i.get());
}


Qt::ItemFlags
TableModel::flags(const QModelIndex &index) const
{
    if ( !index.isValid() ) {
        return Qt::ItemIsDropEnabled;
    }
    TableItemPtr itm = getItem(index);
    if (itm) {
        return itm->getFlags(index.column());
    }

    return Qt::ItemIsEditable
    | Qt::ItemIsSelectable
    | Qt::ItemIsUserCheckable
    | Qt::ItemIsEnabled
    | Qt::ItemIsDragEnabled
    | Qt::ItemIsDropEnabled;
}


void
TableModel::setHorizontalHeaderItem(const TableItemPtr &item)
{

    if (!item) {
        Q_EMIT headerDataChanged(Qt::Horizontal, 0, _imp->colCount - 1);
        return;
    }
    if (item) {
        item->_imp->setModelAndInitColumns(shared_from_this());
    }
    _imp->headerItem = item;
    Q_EMIT headerDataChanged(Qt::Horizontal, 0, _imp->colCount - 1);
}

void
TableModel::setHorizontalHeaderData(const std::vector<std::pair<QString, QIcon> >& sections)
{
    if ((int)sections.size() != _imp->colCount) {
        throw std::invalid_argument("TableModel::setHorizontalHeaderData: invalid number of columns");
    }
    TableModelPtr thisShared = shared_from_this();
    TableItemPtr item = _imp->headerItem;
    if (!item) {
        item = TableItem::create(thisShared);
    }
    for (std::size_t i = 0; i < sections.size(); ++i) {
        if (!sections[i].first.isEmpty()) {
            item->setText(i, sections[i].first);
        }
        if (!sections[i].second.isNull()) {
            item->setIcon(i, sections[i].second);
        }
    }
    setHorizontalHeaderItem(item);
}

void
TableModel::setHorizontalHeaderData(const QStringList& sections)
{
    if ((int)sections.size() != _imp->colCount) {
        throw std::invalid_argument("TableModel::setHorizontalHeaderData: invalid number of columns");
    }
    TableModelPtr thisShared = shared_from_this();
    TableItemPtr item = _imp->headerItem;
    if (!item) {
        item = TableItem::create(thisShared);

    }
    for (int i = 0; i < sections.size(); ++i) {
        if (!sections[i].isEmpty()) {
            item->setText(i, sections[i]);
        }
    }
    setHorizontalHeaderItem(item);

}

int
TableModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return (int)_imp->topLevelItems.size();
    } else {
        // Returns the row count in the item
        TableItemPtr i = getItem(parent);
        // Check if it has children
        if (!i) {
            return 0;
        }
        return i->getChildren().size();
        
    }

}

int
TableModel::columnCount(const QModelIndex & /*parent*/) const
{
    // All rows have the same number of columns
    return _imp->colCount;
}

QVariant
TableModel::data(const QModelIndex &index, int role) const
{
    TableItemPtr itm = getItem(index);
    if (itm) {
        return itm->getData(index.column(), role);
    }

    return QVariant();
}

bool
TableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if ( !index.isValid() ) {
        return false;
    }

    TableItemPtr itm = getItem(index);
    if (itm) {
        itm->setData(index.column(), role, value);
        return true;
    }

    // For tables, create a dummy item if it doesn't exist already.
    // (only if the value is non null)
    if ( _imp->type == eTableModelTypeTree || !value.isValid() ) {
        return false;
    }

    itm = TableItem::create(shared_from_this());
    itm->setData(index.column(), role, value);
    setRow(index.row(), itm);

    return true;
}

bool
TableModel::setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)
{
    if ( !index.isValid() ) {
        return false;
    }

    TableItemPtr itm = getItem(index);
    if (itm) {
        itm->_imp->model.reset(); // prohibits item from calling itemChanged()
        bool changed = false;
        for (QMap<int, QVariant>::ConstIterator it = roles.constBegin(); it != roles.constEnd(); ++it) {
            if ( itm->getData( index.column(), it.key() ) != it.value() ) {
                itm->setData( index.column(), it.key(), it.value() );
                changed = true;
            }
        }
        itm->_imp->setModelAndInitColumns(shared_from_this());
        if (changed) {
            onItemChanged(itm, index.column(), Qt::DisplayRole, true);
        }
    } else {
        // For tables, create a dummy item if it doesn't exist already.
        if (_imp->type == eTableModelTypeTree) {
            return false;
        }
        itm = TableItem::create(shared_from_this());
        for (QMap<int, QVariant>::ConstIterator it = roles.constBegin(); it != roles.constEnd(); ++it) {
            itm->setData( index.column(), it.key(), it.value() );
        }
        setRow(index.row(), itm);

    }


    return true;
}

QMap<int, QVariant> TableModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles;
    TableItemPtr itm = getItem(index);
    if (itm) {
        if (index.column() < (int)itm->_imp->columns.size()) {
            for (std::map<int, QVariant>::const_iterator it= itm->_imp->columns[index.column()].values.begin(); it!=itm->_imp->columns[index.column()].values.end(); ++it) {
                roles.insert(it->first, it->second);
            }
        }
    }
    return roles;
}

QVariant
TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= _imp->colCount || !_imp->headerItem) {
        return QVariant();
    }
    if (orientation == Qt::Horizontal) {
        return _imp->headerItem->getData(section, role);
    } else {
        return QVariant(); // section is out of bounds
    }
}

bool
TableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if ( section < 0 || orientation == Qt::Vertical || (section >= _imp->colCount) || !_imp->headerItem) {
        return false;
    }
    _imp->headerItem->setData(section, role, value);
    return true;
}

void
TableModel::clear()
{
    beginResetModel();
    _imp->topLevelItems.clear();
    endResetModel();
}

void
TableModel::onItemChanged(const TableItemPtr& item, int col, int role, bool valuesChanged)
{
    if (!item) {
        return;
    }
    if (item == _imp->headerItem) {
        Q_EMIT headerDataChanged(Qt::Horizontal, col, col);
    } else {
        int rowIndex = item->getRowInParent();
        if (rowIndex != -1) {
            QModelIndex idx = createIndex(rowIndex, col, item.get());

            // Data changed is needed because we need to refresh the abstract item view
            Q_EMIT dataChanged(idx, idx);

            // Emit our convenience signal if values have changed
            if (valuesChanged) {
                Q_EMIT itemDataChanged(item, col, role);
            }
        }
    }
}


////////////////TableViewPrivae

struct TableViewPrivate
{
    TableModelWPtr model;
    Gui* gui;
    std::list<TableItemPtr> draggedItems;
    
    QPoint lastMousePressPosition;
    
    TableViewPrivate(Gui* gui)
    : model()
    , gui(gui)
    , draggedItems()
    , lastMousePressPosition()
    {
    }
};

ExpandingLineEdit::ExpandingLineEdit(QWidget *parent)
    : LineEdit(parent), originalWidth(-1), widgetOwnsGeometry(false)
{
    connect( this, SIGNAL(textChanged(QString)), this, SLOT(resizeToContents()) );
    updateMinimumWidth();
}

void
ExpandingLineEdit::changeEvent(QEvent *e)
{
    switch ( e->type() ) {
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::ContentsRectChange:
        updateMinimumWidth();
        break;
    default:
        break;
    }

    QLineEdit::changeEvent(e);
}

void
ExpandingLineEdit::updateMinimumWidth()
{
    int left, right;

    getTextMargins(&left, 0, &right, 0);
    int width = left + right + 4 /*horizontalMargin in qlineedit.cpp*/;
    getContentsMargins(&left, 0, &right, 0);
    width += left + right;

    QStyleOptionFrameV2 opt;
    initStyleOption(&opt);

    int minWidth = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(width, 0).
                                             expandedTo( QApplication::globalStrut() ), this).width();
    setMinimumWidth(minWidth);
}

void
ExpandingLineEdit::resizeToContents()
{
    int oldWidth = width();

    if (originalWidth == -1) {
        originalWidth = oldWidth;
    }
    if ( QWidget * parent = parentWidget() ) {
        QPoint position = pos();
        int hintWidth = minimumWidth() + fontMetrics().width( displayText() );
        int parentWidth = parent->width();
        int maxWidth = isRightToLeft() ? position.x() + oldWidth : parentWidth - position.x();
        int newWidth = qBound(originalWidth, hintWidth, maxWidth);
        if (widgetOwnsGeometry) {
            setMaximumWidth(newWidth);
        }
        if ( isRightToLeft() ) {
            move( position.x() - newWidth + oldWidth, position.y() );
        }
        resize( newWidth, height() );
    }
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
QWidget*
TableItemEditorFactory::createEditor(QVariant::Type userType,
                                     QWidget *parent) const
#else
QWidget*
TableItemEditorFactory::createEditor(int userType,
                                     QWidget *parent) const
#endif
{
    switch (userType) {
    case QVariant::UInt: {
        SpinBox *sb = new SpinBox(parent, SpinBox::eSpinBoxTypeInt);
        sb->setFrame(false);
        sb->setMaximum(INT_MAX);

        return sb;
    }
    case QVariant::Int: {
        SpinBox *sb = new SpinBox(parent, SpinBox::eSpinBoxTypeInt);
        sb->setFrame(false);
        sb->setMinimum(INT_MIN);
        sb->setMaximum(INT_MAX);

        return sb;
    }
    case QVariant::Pixmap:

        return new Label(parent);
    case QVariant::Double: {
        SpinBox *sb = new SpinBox(parent, SpinBox::eSpinBoxTypeDouble);
        sb->setFrame(false);
        sb->setMinimum(-DBL_MAX);
        sb->setMaximum(DBL_MAX);

        return sb;
    }
    case QVariant::String:
    default: {
        // the default editor is a lineedit
        ExpandingLineEdit *le = new ExpandingLineEdit(parent);
        le->setFrame( le->style()->styleHint(QStyle::SH_ItemView_DrawDelegateFrame, 0, le) );
        if ( !le->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, 0, le) ) {
            le->setWidgetOwnsGeometry(true);
        }

        return le;
    }
    }

    return 0;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
QByteArray
TableItemEditorFactory::valuePropertyName(QVariant::Type userType) const
#else
QByteArray
TableItemEditorFactory::valuePropertyName(int userType) const
#endif
{
    switch (userType) {
    case QVariant::UInt:
    case QVariant::Int:
    case QVariant::Double:

        return "value";
    case QVariant::String:
    default:

        // the default editor is a lineedit
        return "text";
    }
}


/////////////// TableView
TableView::TableView(Gui* gui, QWidget* parent)
    : QTreeView(parent)
    , _imp( new TableViewPrivate(gui) )
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setItemsExpandable(false);

    ///The table model here doesn't support sorting
    setSortingEnabled(false);
    setMouseTracking(true);
    header()->setStretchLastSection(false);
    header()->setFont( QApplication::font() ); // necessary, or the header font will have the default size, not the application font size
    setTextElideMode(Qt::ElideMiddle);
    setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setDragDropMode(QAbstractItemView::NoDragDrop);
    setAttribute(Qt::WA_MacShowFocusRect, 0);
    setAcceptDrops(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
}

TableView::~TableView()
{
}

Gui*
TableView::getGui() const
{
    return _imp->gui;
}

void
TableView::setTableModel(const TableModelPtr& model)
{
    _imp->model = model;
    model->_imp->gui = _imp->gui;
    setModel(model.get());
}

TableModelPtr
TableView::getTableModel() const
{
    return _imp->model.lock();
}



void
TableView::editItem(const TableItemPtr &item)
{
    if (!item) {
        return;
    }
    QTreeView::edit( getTableModel()->getItemIndex(item) );
}

bool
TableView::edit(const QModelIndex & index,
                QAbstractItemView::EditTrigger trigger,
                QEvent * event)
{
    scrollTo(index);

    return QTreeView::edit(index, trigger, event);
}

void
TableView::openPersistentEditor(const TableItemPtr &item)
{
    if (!item) {
        return;
    }
    QModelIndex index = getTableModel()->getItemIndex(item);
    QAbstractItemView::openPersistentEditor(index);
}

void
TableView::closePersistentEditor(const TableItemPtr &item)
{
    if (!item) {
        return;
    }
    QModelIndex index = getTableModel()->getItemIndex(item);
    QAbstractItemView::closePersistentEditor(index);
}

QWidget*
TableView::cellWidget(int row,
                      int column,
                      QModelIndex parent) const
{
    QModelIndex index = model()->index( row, column, parent );
    return QAbstractItemView::indexWidget(index);
}

void
TableView::setCellWidget(int row,
                         int column,
                         QModelIndex parent,
                         QWidget *widget)
{
    QModelIndex index = model()->index( row, column, parent );
    QAbstractItemView::setIndexWidget(index, widget);
}

void
TableView::removeCellWidget(int row,
                            int column)
{
    setCellWidget(row, column, QModelIndex(), 0);
}

TableItemPtr
TableView::itemAt(const QPoint &p) const
{
    return getTableModel()->getItem( indexAt(p) );
}

TableItemPtr
TableView::itemAt(int x,
                  int y) const
{
    return itemAt( QPoint(x, y) );
}

QRect
TableView::visualItemRect(const TableItemConstPtr& item) const
{
    if (!item) {
        return QRect();
    }
    QModelIndex index = getTableModel()->getItemIndex(item);
    assert( index.isValid() );

    return visualRect(index);
}

TableItemPtr
TableView::editedItem() const
{
    if (state() == QAbstractItemView::EditingState) {
        return currentItem();
    } else {
        return TableItemPtr();
    }
}

TableItemPtr
TableView::currentItem() const
{
    return getTableModel()->getItem( currentIndex() );
}

void
TableView::mousePressEvent(QMouseEvent* e)
{
    QPoint p = e->pos();
    _imp->lastMousePressPosition = p + getOffset();

    TableItemPtr item = itemAt(p);

    if (!item) {
        selectionModel()->clear();
    } else {

        QTreeView::mousePressEvent(e);
    }
}

void TableView::mouseMoveEvent(QMouseEvent *event)
{
    /*QPoint topLeft;
    QPoint bottomRight = event->pos();
    if (state() == ExpandingState || state() == CollapsingState)
        return;
    
    if (state() == DraggingState) {
        topLeft = _imp->lastMousePressPosition - getOffset();
        if ((topLeft - bottomRight).manhattanLength() > QApplication::startDragDistance()) {
            setupDragObject(model()->supportedDragActions());
            setState(NoState); // the startDrag will return when the dnd operation is done
            stopAutoScroll();
        }
        return;
    }*/
    QTreeView::mouseMoveEvent(event);
}


void
TableView::mouseDoubleClickEvent(QMouseEvent* e)
{
    QModelIndex index = indexAt( e->pos() );
    TableItemPtr item = getTableModel()->getItem(index);

    if (item) {
        setCurrentIndex(index);
        Q_EMIT itemDoubleClicked(item);
    }
    QTreeView::mouseDoubleClickEvent(e);
}

void
TableView::mouseReleaseEvent(QMouseEvent* e)
{
    QModelIndex index = indexAt( e->pos() );
    TableItemPtr item = itemAt( e->pos() );

    if ( triggerButtonIsRight(e) && index.isValid() ) {
        Q_EMIT itemRightClicked(e->globalPos(), item);
    } else {
        QTreeView::mouseReleaseEvent(e);
    }
}

void
TableView::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Delete) || (e->key() == Qt::Key_Backspace) ) {
        Q_EMIT deleteKeyPressed();
        e->accept();
    }

    QTreeView::keyPressEvent(e);
}

bool
TableView::isItemSelected(const TableItemConstPtr &item) const
{
    QModelIndex index = getTableModel()->getItemIndex(item);
    if (!index.isValid()) {
        return false;
    }
    return selectionModel()->isSelected(index);
}

void
TableView::setItemSelected(const TableItemPtr& item,
                           bool select)
{
    QModelIndex index = getTableModel()->getItemIndex(item);

    selectionModel()->select(index, select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}



void TableView::calcLogicalIndices(QVector<int> *logicalIndices, QVector<QStyleOptionViewItemV4::ViewItemPosition> *itemPositions, int left, int right) const
{
    const int columnCount = header()->count();
    /* 'left' and 'right' are the left-most and right-most visible visual indices.
     Compute the first visible logical indices before and after the left and right.
     We will use these values to determine the QStyleOptionViewItemV4::viewItemPosition. */
    int logicalIndexBeforeLeft = -1, logicalIndexAfterRight = -1;
    for (int visualIndex = left - 1; visualIndex >= 0; --visualIndex) {
        int logicalIndex = header()->logicalIndex(visualIndex);
        if (!header()->isSectionHidden(logicalIndex)) {
            logicalIndexBeforeLeft = logicalIndex;
            break;
        }
    }

    for (int visualIndex = left; visualIndex < columnCount; ++visualIndex) {
        int logicalIndex = header()->logicalIndex(visualIndex);
        if (!header()->isSectionHidden(logicalIndex)) {
            if (visualIndex > right) {
                logicalIndexAfterRight = logicalIndex;
                break;
            }
            logicalIndices->append(logicalIndex);
        }
    }

    itemPositions->resize(logicalIndices->count());
    for (int currentLogicalSection = 0; currentLogicalSection < logicalIndices->count(); ++currentLogicalSection) {
        const int headerSection = logicalIndices->at(currentLogicalSection);
        // determine the viewItemPosition depending on the position of column 0
        int nextLogicalSection = currentLogicalSection + 1 >= logicalIndices->count()
        ? logicalIndexAfterRight
        : logicalIndices->at(currentLogicalSection + 1);
        int prevLogicalSection = currentLogicalSection - 1 < 0
        ? logicalIndexBeforeLeft
        : logicalIndices->at(currentLogicalSection - 1);
        QStyleOptionViewItemV4::ViewItemPosition pos;
        if (columnCount == 1 || (nextLogicalSection == 0 && prevLogicalSection == -1)
            || (headerSection == 0 && nextLogicalSection == -1)/* || spanning*/)
            pos = QStyleOptionViewItemV4::OnlyOne;
        else if (headerSection == 0 || (nextLogicalSection != 0 && prevLogicalSection == -1))
            pos = QStyleOptionViewItemV4::Beginning;
        else if (nextLogicalSection == 0 || nextLogicalSection == -1)
            pos = QStyleOptionViewItemV4::End;
        else
            pos = QStyleOptionViewItemV4::Middle;
        (*itemPositions)[currentLogicalSection] = pos;
    }
} // calcLogicalIndices

void TableView::adjustViewOptionsForIndex(QStyleOptionViewItemV4 *option,
                                          const TableItemPtr &item,
                                          const QModelIndex& index) const
{
    assert(item);
    if (isExpanded(index)) {
        option->state |= QStyle::State_Open;
    } else {
        option->state |= QStyle::State_None;
    }
    if (item->getChildren().empty()) {
        option->state |= QStyle::State_Children;
    } else {
        option->state |= QStyle::State_None;
    }
    TableItemPtr parentItem = item->getParentItem();
    if (parentItem && parentItem->getChildren().size() > 1) {
        option->state |= QStyle::State_Sibling;
    } else {
        option->state |= QStyle::State_None;
    }

    option->showDecorationSelected = (selectionBehavior() & QTreeView::SelectRows) || option->showDecorationSelected;

    // index = visual index of visible columns only. data = logical index.
    QVector<int> logicalIndices;

    // vector of left/middle/end for each logicalIndex, visible columns only.
    QVector<QStyleOptionViewItemV4::ViewItemPosition> viewItemPosList;


    const bool spanning = false;//viewItems.at(row).spanning;

    const int left = (spanning ? header()->visualIndex(0) : 0);
    const int right = (spanning ? header()->visualIndex(0) : header()->count() - 1 );
    calcLogicalIndices(&logicalIndices, &viewItemPosList, left, right);

    const int visualIndex = logicalIndices.indexOf(index.column());
    option->viewItemPosition = viewItemPosList.at(visualIndex);
} // adjustViewOptionsForIndex



QStyleOptionViewItemV4 TableView::viewOptionsV4() const
{
    QStyleOptionViewItemV4 option = viewOptions();
    //if (wrapItemText)
    //   option.features = QStyleOptionViewItemV2::WrapText;
    option.locale = locale();
    option.locale.setNumberOptions(QLocale::OmitGroupSeparator);
    option.widget = this;
    return option;
}
struct DraggablePaintItem
{
    QModelIndex index;
    int col;
    QRect rect;
    TableItemPtr tableItem;
};

QPixmap TableView::renderToPixmap(const std::list<TableItemPtr>& rows, QRect *r) const
{
    assert(r);

    TableModelPtr model = getTableModel();

    const QRect viewportRect = viewport()->rect();
    const int colsCount = model->columnCount();;

    std::vector<DraggablePaintItem> paintItems;
    for (std::list<TableItemPtr>::const_iterator it = rows.begin(); it!=rows.end(); ++it) {

        const TableItemPtr& modelItem = *it;
        QModelIndex firstColIndex = model->getItemIndex(modelItem);

        for (int c = 0; c < colsCount; ++c) {
            QModelIndex idx = c == 0 ? firstColIndex : model->index(firstColIndex.row(), c, firstColIndex.parent());
            const QRect itemRect = visualRect(idx);
            if (itemRect.intersects(viewportRect)) {
                DraggablePaintItem d;
                d.rect = itemRect;
                d.index = idx;
                d.tableItem = modelItem;
                *r = r->unite(itemRect);
                paintItems.push_back(d);
            }
        }


    }
    *r = r->intersect(viewportRect);

    if (paintItems.empty()) {
        return QPixmap();
    }


    QPixmap pixmap(r->size());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QStyleOptionViewItemV4 option = viewOptionsV4();
    option.state |= QStyle::State_Selected;
    for (std::size_t i = 0; i < paintItems.size(); ++i) {
        option.rect = paintItems[i].rect.translated(-r->topLeft());
        adjustViewOptionsForIndex(&option, paintItems[i].tableItem, paintItems[i].index);
        itemDelegate(paintItems[i].index)->paint(&painter, option, paintItems[i].index);
    }
    return pixmap;
} // renderToPixmap

void
TableView::rebuildDraggedItemsFromSelection()
{
    _imp->draggedItems.clear();

    std::set<TableItemPtr> itemsSet;
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    for (QModelIndexList::Iterator it = indexes.begin(); it != indexes.end(); ++it) {
        TableItemPtr i = getTableModel()->getItem(*it);
        if (i) {

            // If any child of this item is already selected for drag, remove the child
            {
                std::set<TableItemPtr> newItems;
                bool hasChanged = false;
                for (std::set<TableItemPtr>::const_iterator it = itemsSet.begin(); it != itemsSet.end(); ++it) {
                    if ((*it)->isChildRecursive(i)) {
                        hasChanged = true;
                        continue;
                    }
                    newItems.insert(*it);
                }
                if (hasChanged) {
                    itemsSet = newItems;
                }
            }

            // If a parent of the item is already selected for drag, don't insert it
            TableItemPtr parent = i->getParentItem();
            bool parentSelected = false;
            while (parent) {
                std::set<TableItemPtr>::iterator foundParent = itemsSet.find(parent);
                if (foundParent != itemsSet.end()) {
                    parentSelected = true;
                    break;
                }
                parent = parent->getParentItem();
            }
            if (!parentSelected) {
                itemsSet.insert(i);
            }
        }
    }
    for (std::set<TableItemPtr>::const_iterator it = itemsSet.begin(); it != itemsSet.end(); ++it) {
        _imp->draggedItems.push_back(*it);
    }
}

void
TableView::startDrag(Qt::DropActions supportedActions)
{
    setupDragObject(supportedActions);
}

void
TableView::setupDragObject(Qt::DropActions supportedActions)
{
    
    rebuildDraggedItemsFromSelection();
    
    if (_imp->draggedItems.empty()) {
        return;
    }
    QDrag *drag = new QDrag(this);
    QRect rect;
    QPixmap pixmap = renderToPixmap(_imp->draggedItems, &rect);
    rect.adjust(horizontalOffset(), verticalOffset(), 0, 0);
    drag->setPixmap(pixmap);
    drag->setHotSpot(_imp->lastMousePressPosition - rect.topLeft());
    
    Qt::DropAction action = Qt::IgnoreAction;

    // Use the default drop action if it is supported
    // Otherwise fallback on copy
    if (defaultDropAction() != Qt::IgnoreAction && (supportedActions & defaultDropAction())) {
        action = defaultDropAction();
    } else if (supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove) {
        action = Qt::CopyAction;
    }
    
    // Call implementation dependent drag mimedata setup
    setupAndExecDragObject(drag, _imp->draggedItems, supportedActions, action);
}

void
TableView::setupAndExecDragObject(QDrag* drag,
                                  const std::list<TableItemPtr>& rows,
                                  Qt::DropActions supportedActions,
                                  Qt::DropAction defaultAction)
{
    QMimeData *data = new QMimeData;//getTableModel()->mimeData(rows);
    if (!data) {
        return;
    }
    drag->setMimeData(data);
    if (drag->exec(supportedActions, defaultAction) == Qt::MoveAction) {
        
        TableModelPtr model = getTableModel();
        // If the target table is not this one, we have no choice but to remove from this table the items out of undo/redo operation
        for (std::list<TableItemPtr>::const_iterator it = rows.begin(); it != rows.end(); ++it) {
            model->removeItem(*it);
        }
        
    }
    
    
}

void
TableView::dragLeaveEvent(QDragLeaveEvent *e)
{
    _imp->draggedItems.clear();
    QTreeView::dragLeaveEvent(e);
}

void
TableView::dragEnterEvent(QDragEnterEvent *e)
{
    
    if (_imp->draggedItems.empty()) {
        e->ignore();
        return;
    }
    rebuildDraggedItemsFromSelection();
    QTreeView::dragEnterEvent(e);
}

void
TableView::dropEvent(QDropEvent* e)
{

    if (getTableModel()->getType() == TableModel::eTableModelTypeTree) {
        QTreeView::dropEvent(e);
        return;
    }
    // We only support drag & drop that are internal to this table
    TableItemPtr into = itemAt( e->pos() );
    if ( !into || _imp->draggedItems.empty() ) {
        e->ignore();
        return;
    }


    // We only support full rows
    assert(selectionBehavior() == QAbstractItemView::SelectRows);


    DropIndicatorPosition position = dropIndicatorPosition();

    // Only allow dragging on items if we can make a hierarchy (i.e: this is a tree)
    if (position == QAbstractItemView::OnViewport ||
        position == QAbstractItemView::OnItem) {
        return;
    }

    // Prepare for changes
    Q_EMIT aboutToDrop();


    TableModelPtr model = getTableModel();

    // Remove the items but do not delete them
    std::map<int, TableItemPtr> rowMoved;
    for (std::list<TableItemPtr>::iterator it = _imp->draggedItems.begin(); it != _imp->draggedItems.end(); ++it) {
        rowMoved[(*it)->getRowInParent()] = *it;
        model->removeItem(*it);
    }

    _imp->draggedItems.clear();

    int targetRow = into->getRowInParent();

    // Insert back at the correct position depending on the drop indicator
    switch (position) {
    case QAbstractItemView::AboveItem: {
        model->insertRows( targetRow, rowMoved.size() );
        break;
    }
    case QAbstractItemView::BelowItem: {
        int nRows = model->rowCount();
        ++targetRow;
        if (targetRow > nRows) {
            targetRow = nRows;
        }
        model->insertRows( targetRow, rowMoved.size() );
        break;
    }
    default:
        assert(false);
        return;
    }

    int rowIndex = targetRow;
    for (std::map<int, TableItemPtr>::iterator it = rowMoved.begin(); it != rowMoved.end(); ++it, ++rowIndex) {
        model->setRow(it->first, it->second);
    }

    Q_EMIT itemDropped();
} // TableView::dropEvent

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TableModelView.cpp"
