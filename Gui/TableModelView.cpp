/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include <QApplication>
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>
#include "Gui/GuiMacros.h"

NATRON_NAMESPACE_ENTER;

//////////////TableItem

namespace {
class MetaTypesRegistration
{
public:
    inline MetaTypesRegistration()
    {
        qRegisterMetaType<TableItem*>("TableItem*");
    }
};
}

static MetaTypesRegistration registration;
TableItem::TableItem(const TableItem & other)
    : values(other.values)
      , view(0)
      , id(-1)
      , itemFlags(other.itemFlags)
{
}

TableItem::~TableItem()
{
    if ( TableModel * model = (view ? qobject_cast<TableModel*>( view->model() ) : 0) ) {
        model->removeItem(this);
    }
    view = 0;
}

TableItem *
TableItem::clone() const
{
    return new TableItem(*this);
}

int
TableItem::row() const
{
    return view ? view->row(this) : -1;
}

int
TableItem::column() const
{
    return view ? view->column(this) : -1;
}

void
TableItem::setSelected(bool aselect)
{
    if (view) {
        view->setItemSelected(this, aselect);
    }
}

bool
TableItem::isSelected() const
{
    return view ? view->isItemSelected(this) : false;
}

void
TableItem::setFlags(Qt::ItemFlags flags)
{
    itemFlags = flags;
    if ( TableModel * model = (view ? qobject_cast<TableModel*>( view->model() ) : 0) ) {
        model->itemChanged(this);
    }
}

QVariant
TableItem::data(int role) const
{
    role = (role == Qt::EditRole ? Qt::DisplayRole : role);
    for (int i = 0; i < values.count(); ++i) {
        if (values.at(i).role == role) {
            return values.at(i).value;
        }
    }

    return QVariant();
}

void
TableItem::setData(int role,
                   const QVariant &value)
{
    bool found = false;

    role = (role == Qt::EditRole ? Qt::DisplayRole : role);
    for (int i = 0; i < values.count(); ++i) {
        if (values.at(i).role == role) {
            if (values[i].value == value) {
                return;
            }

            values[i].value = value;
            found = true;
            break;
        }
    }
    if (!found) {
        values.append( ItemData(role, value) );
    }
    if ( TableModel * model = (view ? qobject_cast<TableModel*>( view->model() ) : 0) ) {
        model->itemChanged(this);
    }
}

TableItem &
TableItem::operator=(const TableItem &other)
{
    values = other.values;
    itemFlags = other.itemFlags;

    return *this;
}

///////////////TableModel
struct TableModelPrivate
{
    std::vector<TableItem*> tableItems;
    std::vector<TableItem*> horizontalHeaderItems;
    int rowCount;
    
    TableModelPrivate(int rows,
                      int columns)
    : tableItems(rows * columns,(TableItem*)0)
    , horizontalHeaderItems(columns,(TableItem*)0)
    , rowCount(rows)
    {
    }
    
    int indexOfItem(const TableItem* item) const
    {
        for (std::size_t i = 0; i < tableItems.size(); ++i) {
            if (tableItems[i] == item) {
                return (int)i;
            }
        }
        return -1;
    }
    
    int indexOfHeaderItem(const TableItem* item) const
    {
        for (std::size_t i = 0; i < horizontalHeaderItems.size(); ++i) {
            if (horizontalHeaderItems[i] == item) {
                return (int)i;
            }
        }
        return -1;
    }
};

TableModel::TableModel(int rows,
                       int columns,
                       TableView* view)
    : QAbstractTableModel(view)
      , _imp( new TableModelPrivate(rows,columns) )
{
    QObject::connect( this, SIGNAL( dataChanged(QModelIndex,QModelIndex) ), this, SLOT( onDataChanged(QModelIndex) ) );
}

TableModel::~TableModel()
{
    for (int i = 0; i < (int)_imp->tableItems.size(); ++i) {
        delete _imp->tableItems[i];
    }
    
    
    for (int i = 0; i < (int)_imp->horizontalHeaderItems.size(); ++i) {
        delete _imp->horizontalHeaderItems[i];
    }

}

void
TableModel::onDataChanged(const QModelIndex & index)
{
    if ( TableItem * i = item(index) ) {
        Q_EMIT s_itemChanged(i);
    }
}

bool
TableModel::insertRows(int row,
                       int count,
                       const QModelIndex &)
{
    if ( (count < 1) || (row < 0) || (row > _imp->rowCount) ) {
        return false;
    }
    beginInsertRows(QModelIndex(), row, row + count - 1);
    _imp->rowCount += count;
    int cc = _imp->horizontalHeaderItems.size();
    if (_imp->rowCount == 0) {
        _imp->tableItems.resize(cc * count);
    } else {
        std::vector<TableItem*>::iterator pos = _imp->tableItems.begin();
        int idx = tableIndex(row, 0);
        if (idx < (int)_imp->tableItems.size()) {
            std::advance(pos, idx);
            _imp->tableItems.insert(pos, cc * count,(TableItem*)0);
        } else {
            _imp->tableItems.insert(_imp->tableItems.end(), cc * count,(TableItem*)0);
        }
        
    }
    endInsertRows();

    return true;
}

bool
TableModel::insertColumns(int column,
                          int count,
                          const QModelIndex &)
{
    if ( (count < 1) || (column < 0) || ( column > (int)_imp->horizontalHeaderItems.size() ) ) {
        return false;
    }
    beginInsertColumns(QModelIndex(), column, column +  count - 1);
    int cc = _imp->horizontalHeaderItems.size();
    
    
    {
        std::vector<TableItem*>::iterator pos = _imp->horizontalHeaderItems.begin();
        if (column < (int)_imp->horizontalHeaderItems.size()) {
            std::advance(pos, column);
            _imp->horizontalHeaderItems.insert(pos, count,(TableItem*)0);
        } else {
            _imp->horizontalHeaderItems.insert(_imp->horizontalHeaderItems.end(), count,(TableItem*)0);
        }
    }
    if (cc == 0) {
        _imp->tableItems.resize(_imp->rowCount * count);
    } else {
        for (int row = 0; row < _imp->rowCount; ++row) {
            std::vector<TableItem*>::iterator pos = _imp->tableItems.begin();
            int idx = tableIndex(row, column);
            if (idx < (int)_imp->tableItems.size()) {
                std::advance(pos, idx);
                _imp->tableItems.insert(pos, count,(TableItem*)0);
            } else {
                _imp->tableItems.insert(_imp->tableItems.end(), count,(TableItem*)0);
            }
        }
    }
    endInsertColumns();

    return true;
}

bool
TableModel::removeRows(int row,
                       int count,
                       const QModelIndex &)
{
    if ( (count < 1) || (row < 0) || (row + count > _imp->rowCount) ) {
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    int i = tableIndex(row, 0);
    int n = count * columnCount();
    TableItem *oldItem = 0;
    for (int j = i; j < n + i; ++j) {
        oldItem = _imp->tableItems.at(j);
        if (oldItem) {
            oldItem->view = 0;
        }
        delete oldItem;
    }
    _imp->rowCount -= count;
    
    std::vector<TableItem*>::iterator pos = _imp->tableItems.begin();
    int idx = std::max(i, 0);
    if (idx < (int)_imp->tableItems.size()) {
        std::advance(pos, idx);
        std::vector<TableItem*>::iterator last = pos;
        if ((idx + n) < (int)_imp->tableItems.size()) {
            std::advance(last, n);
        } else {
            last = _imp->tableItems.end();
        }
        _imp->tableItems.erase(pos ,last);
    }
    
    endRemoveRows();

    return true;
}

bool
TableModel::removeColumns(int column,
                          int count,
                          const QModelIndex &)
{
    if ( (count < 1) || (column < 0) || ( column + count >  (int)_imp->horizontalHeaderItems.size() ) ) {
        return false;
    }

    beginRemoveColumns(QModelIndex(), column, column + count - 1);
    TableItem* oldItem = 0;
    for (int row = rowCount() - 1; row >= 0; --row) {
        int i = tableIndex(row, column);
        for (int j = i; j < i + count; ++j) {
            oldItem = _imp->tableItems.at(j);
            if (oldItem) {
                oldItem->view = 0;
            }
            delete oldItem;
        }
        
        
        std::vector<TableItem*>::iterator pos = _imp->tableItems.begin();
        if (i < (int)_imp->tableItems.size()) {
            std::advance(pos, i);
            std::vector<TableItem*>::iterator last = pos;
            if ((i + count) < (int)_imp->tableItems.size()) {
                std::advance(last, count);
            } else {
                last = _imp->tableItems.end();
            }
            _imp->tableItems.erase(pos ,last);
        }
    }
    for (int h = column; h < column + count; ++h) {
        oldItem = _imp->horizontalHeaderItems.at(h);
        if (oldItem) {
            oldItem->view = 0;
        }
        delete oldItem;
    }
    
    std::vector<TableItem*>::iterator pos = _imp->horizontalHeaderItems.begin();
    if (column < (int)_imp->horizontalHeaderItems.size()) {
        std::advance(pos, column);
        std::vector<TableItem*>::iterator last = pos;
        if ((column + count) < (int)_imp->horizontalHeaderItems.size()) {
            std::advance(last, count);
        } else {
            last = _imp->horizontalHeaderItems.end();
        }
        _imp->horizontalHeaderItems.erase(pos ,last);
    }
    
    endRemoveColumns();

    return true;
}

void
TableModel::setTable(const std::vector<TableItem*>& items)
{
    beginRemoveRows(QModelIndex(), 0, _imp->rowCount - 1);
    for (std::size_t i = 0; i < _imp->tableItems.size(); ++i) {
        if ( _imp->tableItems.at(i) ) {
            _imp->tableItems[i]->view = 0;
            delete _imp->tableItems.at(i);
            _imp->tableItems[i] = 0;
        }
    }
    endRemoveRows();
    
    _imp->rowCount = items.size() / _imp->horizontalHeaderItems.size();

    _imp->tableItems = items;
    
    TableView *view = qobject_cast<TableView*>( QObject::parent() );

    
    beginInsertRows(QModelIndex(), 0, _imp->rowCount -1);

    
    for (int i = 0; i < (int)_imp->tableItems.size(); ++i) {
        _imp->tableItems[i]->id = i;
        _imp->tableItems[i]->view = view;
    }
    
    endInsertRows();
    
    if (!items.empty()) {
        QModelIndex tl = QAbstractTableModel::index(0, 0);
        int cols = columnCount();
        int lastTableIndex = items.size() - 1;
        int lastIndexRow = cols == 0 ? lastTableIndex : (lastTableIndex - (cols - 1)) / cols;
        QModelIndex br = QAbstractTableModel::index(lastIndexRow, cols -1);
        Q_EMIT dataChanged(tl, br);
    }
}

void
TableModel::setItem(int row,
                    int column,
                    TableItem *item)
{
    int i = tableIndex(row, column);

    if ( (i < 0) || ( i >= (int)_imp->tableItems.size() ) ) {
        return;
    }
    TableItem *oldItem = _imp->tableItems.at(i);
    if (item == oldItem) {
        return;
    }

    // remove old
    if (oldItem) {
        oldItem->view = 0;
    }
    delete _imp->tableItems.at(i);

    // set new
    if (item) {
        item->id = i;
    }
    _imp->tableItems[i] = item;

    QModelIndex idx = QAbstractTableModel::index(row, column);
    Q_EMIT dataChanged(idx, idx);
}

TableItem *
TableModel::takeItem(int row,
                     int column)
{
    long i = tableIndex(row, column);
    TableItem *itm = _imp->tableItems[i];

    if (itm) {
        itm->view = 0;
        itm->id = -1;
        _imp->tableItems[i] = 0;
        QModelIndex ind = index(itm);
        Q_EMIT dataChanged(ind, ind);
    }

    return itm;
}

QModelIndex
TableModel::index(const TableItem *item) const
{
    if (!item) {
        return QModelIndex();
    }
    int i = -1;
    const int id = item->id;
    if ( (id >= 0) && ( id < (int)_imp->tableItems.size() ) && (_imp->tableItems.at(id) == item) ) {
        i = id;
    } else { // we need to search for the item
        i = _imp->indexOfItem(item);
        if (i == -1) { // not found
            return QModelIndex();
        }
    }
    int ncols = columnCount();
    if (ncols == 0) {
        return QModelIndex();
    } else {
        int row = i / ncols;
        int col = i % ncols;

        return QAbstractTableModel::index(row, col);
    }
}

TableItem *
TableModel::item(int row,
                 int column) const
{
    return item( index(row, column) );
}

TableItem *
TableModel::item(const QModelIndex &index) const
{
    if ( !isValid(index) ) {
        return 0;
    }

    int idx = tableIndex(index.row(), index.column());
    return idx < (int)_imp->tableItems.size() ? _imp->tableItems[idx] : 0;
}

void
TableModel::removeItem(TableItem *item)
{
    int i = _imp->indexOfItem(item);

    if (i != -1) {
        _imp->tableItems[i] = 0;
        QModelIndex idx = index(item);
        Q_EMIT dataChanged(idx, idx);

        return;
    }

    i = _imp->indexOfHeaderItem(item);
    if (i != -1) {
        _imp->horizontalHeaderItems[i] = 0;
        Q_EMIT headerDataChanged(Qt::Horizontal, i, i);

        return;
    }
}

void
TableModel::setHorizontalHeaderItem(int section,
                                    TableItem *item)
{
    if ( (section < 0) || ( section >= (int)_imp->horizontalHeaderItems.size() ) ) {
        return;
    }

    TableItem *oldItem = _imp->horizontalHeaderItems.at(section);
    if (item == oldItem) {
        return;
    }

    if (oldItem) {
        oldItem->view = 0;
    }
    delete oldItem;

    TableView *view = qobject_cast<TableView*>( QObject::parent() );

    if (item) {
        item->view = view;
        item->itemFlags = Qt::ItemFlags(int(item->itemFlags) | ItemIsHeaderItem);
    }
    _imp->horizontalHeaderItems[section] = item;
    Q_EMIT headerDataChanged(Qt::Horizontal, section, section);
}

TableItem *
TableModel::takeHorizontalHeaderItem(int section)
{
    if ( (section < 0) || ( section >= (int)_imp->horizontalHeaderItems.size() ) ) {
        return 0;
    }
    TableItem *itm = _imp->horizontalHeaderItems.at(section);
    if (itm) {
        itm->view = 0;
        itm->itemFlags &= ~ItemIsHeaderItem;
        _imp->horizontalHeaderItems[section] = 0;
    }

    return itm;
}

TableItem *
TableModel::horizontalHeaderItem(int section)
{
    assert(section >= 0 && section < (int)_imp->horizontalHeaderItems.size());
    return _imp->horizontalHeaderItems[section];
}

void
TableModel::setRowCount(int rows)
{
    if ( (rows < 0) || (_imp->rowCount == rows) ) {
        return;
    }
    if (_imp->rowCount < rows) {
        insertRows(std::max(_imp->rowCount, 0), rows - _imp->rowCount);
    } else {
        removeRows(std::max(rows, 0), _imp->rowCount - rows);
    }
}

void
TableModel::setColumnCount(int columns)
{
    int cc = (int)_imp->horizontalHeaderItems.size();

    if ( (columns < 0) || (cc == columns) ) {
        return;
    }
    if (cc < columns) {
        insertColumns(std::max(cc, 0), columns - cc);
    } else {
        removeColumns(std::max(columns, 0), cc - columns);
    }
}

int
TableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _imp->rowCount;
}

int
TableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : (int)_imp->horizontalHeaderItems.size();
}

QVariant
TableModel::data(const QModelIndex &index,
                 int role) const
{
    TableItem *itm = item(index);

    if (itm) {
        return itm->data(role);
    }

    return QVariant();
}

TableItem *
TableModel::createItem() const
{
    return new TableItem;
}

bool
TableModel::setData(const QModelIndex &index,
                    const QVariant &value,
                    int role)
{
    if ( !index.isValid() ) {
        return false;
    }

    TableItem *itm = item(index);
    if (itm) {
        itm->setData(role, value);

        return true;
    }

    // don't create dummy table items for empty values
    if ( !value.isValid() ) {
        return false;
    }

    TableView *view = qobject_cast<TableView*>( QObject::parent() );
    if (!view) {
        return false;
    }

    itm = createItem();
    itm->setData(role, value);
    view->setItem(index.row(), index.column(), itm);

    return true;
}

bool
TableModel::setItemData(const QModelIndex &index,
                        const QMap<int, QVariant> &roles)
{
    if ( !index.isValid() ) {
        return false;
    }

    TableView *view = qobject_cast<TableView*>( QObject::parent() );
    TableItem *itm = item(index);
    if (itm) {
        itm->view = 0; // prohibits item from calling itemChanged()
        bool changed = false;
        for (QMap<int, QVariant>::ConstIterator it = roles.constBegin(); it != roles.constEnd(); ++it) {
            if ( itm->data( it.key() ) != it.value() ) {
                itm->setData( it.key(), it.value() );
                changed = true;
            }
        }
        itm->view = view;
        if (changed) {
            itemChanged(itm);
        }

        return true;
    }

    if (!view) {
        return false;
    }

    itm = createItem();
    for (QMap<int, QVariant>::ConstIterator it = roles.constBegin(); it != roles.constEnd(); ++it) {
        itm->setData( it.key(), it.value() );
    }
    view->setItem(index.row(), index.column(), itm);

    return true;
}

QMap<int, QVariant> TableModel::itemData(const QModelIndex &index) const
{
    QMap<int, QVariant> roles;
    TableItem *itm = item(index);
    if (itm) {
        for (int i = 0; i < itm->values.count(); ++i) {
            roles.insert(itm->values.at(i).role,
                         itm->values.at(i).value);
        }
    }

    return roles;
}

QVariant
TableModel::headerData(int section,
                       Qt::Orientation orientation,
                       int role) const
{
    if (section < 0) {
        return QVariant();
    }

    TableItem *itm = 0;
    if ( (orientation == Qt::Horizontal) && ( section < (int)_imp->horizontalHeaderItems.size() ) ) {
        itm = _imp->horizontalHeaderItems.at(section);
    } else {
        return QVariant(); // section is out of bounds
    }
    if (itm) {
        return itm->data(role);
    }
    if (role == Qt::DisplayRole) {
        return section + 1;
    }

    return QVariant();
}

bool
TableModel::setHeaderData(int section,
                          Qt::Orientation orientation,
                          const QVariant &value,
                          int role)
{
    if ( (section < 0) ||
         ((orientation == Qt::Horizontal) && ((int)_imp->horizontalHeaderItems.size() <= section))) {
        return false;
    }

    TableItem *itm = 0;
    if (orientation == Qt::Horizontal) {
        itm = _imp->horizontalHeaderItems.at(section);
    }

    if (itm) {
        itm->setData(role, value);

        return true;
    }

    return false;
}

long
TableModel::tableIndex(int row,
                       int column) const
{
    // y * width + x
    return ( row * (int)_imp->horizontalHeaderItems.size() ) + column;
}

void
TableModel::clear()
{
    beginResetModel();
    for (std::size_t i = 0; i < _imp->tableItems.size(); ++i) {
        if ( _imp->tableItems.at(i) ) {
            _imp->tableItems[i]->view = 0;
            delete _imp->tableItems.at(i);
            _imp->tableItems[i] = 0;
        }
    }
    endResetModel();
}

bool
TableModel::isValid(const QModelIndex &index) const
{
    return index.isValid()
           && index.row() < _imp->rowCount
           && index.column() < (int)_imp->horizontalHeaderItems.size();
}

void
TableModel::itemChanged(TableItem *item)
{
    if (!item) {
        return;
    }

    if (item->flags() & ItemIsHeaderItem) {
        int column = _imp->indexOfHeaderItem(item);
        if (column >= 0) {
            Q_EMIT headerDataChanged(Qt::Horizontal, column, column);
        }
    } else {
        QModelIndex idx = index(item);
        if ( idx.isValid() ) {
            Q_EMIT dataChanged(idx, idx);
        }
    }
}

Qt::ItemFlags
TableModel::flags(const QModelIndex &index) const
{
    if ( !index.isValid() ) {
        return Qt::ItemIsDropEnabled;
    }
    if ( TableItem * itm = item(index) ) {
        return itm->flags();
    }

    return Qt::ItemIsEditable
           | Qt::ItemIsSelectable
           | Qt::ItemIsUserCheckable
           | Qt::ItemIsEnabled
           | Qt::ItemIsDragEnabled
           | Qt::ItemIsDropEnabled;
}


////////////////TableViewPrivae

struct TableViewPrivate
{
    TableModel* model;
    std::list<TableItem*> draggedItems;
    
    TableViewPrivate()
        : model(0)
        , draggedItems()
    {
    }
};


/////////////// TableView
TableView::TableView(QWidget* parent)
    : QTreeView(parent)
      , _imp( new TableViewPrivate() )
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);

    ///The table model here doesn't support sorting
    setSortingEnabled(false);

    header()->setStretchLastSection(false);
    header()->setFont(QApplication::font()); // necessary, or the header font will have the default size, not the application font size
    setTextElideMode(Qt::ElideMiddle);
    setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setDragDropMode(QAbstractItemView::NoDragDrop);
    setAttribute(Qt::WA_MacShowFocusRect,0);
    setAcceptDrops(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
}

TableView::~TableView()
{
}

void
TableView::setTableModel(TableModel* model)
{
    _imp->model = model;
    setModel(model);
}

void
TableView::setRowCount(int rows)
{
    _imp->model->setRowCount(rows);
}

int
TableView::rowCount() const
{
    return _imp->model->rowCount();
}

void
TableView::setColumnCount(int columns)
{
    _imp->model->setColumnCount(columns);
}

int
TableView::columnCount() const
{
    return _imp->model->columnCount();
}

int
TableView::row(const TableItem *item) const
{
    return _imp->model->index(item).row();
}

int
TableView::column(const TableItem *item) const
{
    return _imp->model->index(item).column();
}

TableItem*
TableView::item(int row,
                int column) const
{
    return _imp->model->item(row, column);
}

void
TableView::setItem(int row,
                   int column,
                   TableItem *item)
{
    if (item) {
        if (item->view != 0) {
            qWarning("TableView: cannot insert an item that is already owned by another TableView");
        } else {
            item->view = this;
            _imp->model->setItem(row, column, item);
        }
    } else {
        delete takeItem(row, column);
    }
}

TableItem*
TableView::takeItem(int row,
                    int column)
{
    TableItem* item =  _imp->model->takeItem(row, column);

    if (item) {
        item->view = 0;
    }

    return item;
}

void
TableView::setHorizontalHeaderLabels(const QStringList &labels)
{
    TableItem *item = 0;

    for (int i = 0; i < _imp->model->columnCount() && i < labels.count(); ++i) {
        item = _imp->model->horizontalHeaderItem(i);
        if (!item) {
            item = _imp->model->createItem();
            setHorizontalHeaderItem(i, item);
        }
        item->setText( labels.at(i) );
    }
}

TableItem*
TableView::horizontalHeaderItem(int column) const
{
    return _imp->model->horizontalHeaderItem(column);
}

TableItem*
TableView::takeHorizontalHeaderItem(int column)
{
    TableItem *itm = _imp->model->takeHorizontalHeaderItem(column);

    if (itm) {
        itm->view = 0;
    }

    return itm;
}

void
TableView::setHorizontalHeaderItem(int column,
                                   TableItem *item)
{
    if (item) {
        item->view = this;
        _imp->model->setHorizontalHeaderItem(column, item);
    } else {
        delete takeHorizontalHeaderItem(column);
    }
}

void
TableView::editItem(TableItem *item)
{
    if (!item) {
        return;
    }
    QTreeView::edit( _imp->model->index(item) );
}

bool
TableView::edit(const QModelIndex & index, QAbstractItemView::EditTrigger trigger, QEvent * event)
{
    
    scrollTo(index);
    return QTreeView::edit(index,trigger,event);
}


void
TableView::openPersistentEditor(TableItem *item)
{
    if (!item) {
        return;
    }
    QModelIndex index = _imp->model->index(item);
    QAbstractItemView::openPersistentEditor(index);
}

void
TableView::closePersistentEditor(TableItem *item)
{
    if (!item) {
        return;
    }
    QModelIndex index = _imp->model->index(item);
    QAbstractItemView::closePersistentEditor(index);
}

QWidget*
TableView::cellWidget(int row,
                      int column) const
{
    QModelIndex index = model()->index( row, column, QModelIndex() );

    return QAbstractItemView::indexWidget(index);
}

void
TableView::setCellWidget(int row,
                         int column,
                         QWidget *widget)
{
    QModelIndex index = model()->index( row, column, QModelIndex() );
    QAbstractItemView::setIndexWidget(index, widget);
}

void
TableView::removeCellWidget(int row,
                            int column)
{
    setCellWidget(row, column, 0);
}

TableItem*
TableView::itemAt(const QPoint &p) const
{
    return _imp->model->item( indexAt(p) );
}

TableItem*
TableView::itemAt(int x,
                  int y) const
{
    return itemAt( QPoint(x,y) );
}

QRect
TableView::visualItemRect(const TableItem *item) const
{
    if (!item) {
        return QRect();
    }
    QModelIndex index = _imp->model->index( const_cast<TableItem*>(item) );
    assert( index.isValid() );

    return visualRect(index);
}

void
TableView::mousePressEvent(QMouseEvent* e)
{
    TableItem* item = itemAt( e->pos() );

    if (!item) {
        selectionModel()->clear();
    } else {
        QTreeView::mousePressEvent(e);
    }
}

void
TableView::mouseDoubleClickEvent(QMouseEvent* e)
{
    TableItem* item = itemAt( e->pos() );
    if (item) {
        Q_EMIT itemDoubleClicked(item);
    }
    QTreeView::mouseDoubleClickEvent(e);
    
}

void
TableView::mouseReleaseEvent(QMouseEvent* e)
{
    QModelIndex index = indexAt( e->pos() );
    TableItem* item = itemAt( e->pos() );

    if ( triggerButtonIsRight(e) && index.isValid() ) {
        Q_EMIT itemRightClicked(item);
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
TableView::isItemSelected(const TableItem *item) const
{
    QModelIndex index = _imp->model->index(item);

    return selectionModel()->isSelected(index);
}

void
TableView::setItemSelected(const TableItem *item,
                           bool select)
{
    QModelIndex index = _imp->model->index(item);

    selectionModel()->select(index, select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

void
TableView::rebuildDraggedItemsFromSelection()
{
    _imp->draggedItems.clear();
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    for (QModelIndexList::Iterator it = indexes.begin(); it!=indexes.end(); ++it) {
        TableItem* i = item(it->row(), it->column());
        if (i) {
            _imp->draggedItems.push_back(i);
        }
    }
}

void
TableView::startDrag(Qt::DropActions supportedActions)
{
    rebuildDraggedItemsFromSelection();
    QTreeView::startDrag(supportedActions);
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
    rebuildDraggedItemsFromSelection();
    QTreeView::dragEnterEvent(e);
}

void
TableView::dropEvent(QDropEvent* e)
{
    
  //  QTreeView::dropEvent(e);
    
    DropIndicatorPosition position = dropIndicatorPosition();
    switch (position) {
        case QAbstractItemView::OnItem:
        case QAbstractItemView::OnViewport:
        default:
            return;
        
        case QAbstractItemView::AboveItem:
        case QAbstractItemView::BelowItem:
            break;
    }
    TableItem* into = itemAt(e->pos());

    if (!into || _imp->draggedItems.empty()) {
        return ;
    }
    Q_EMIT aboutToDrop();
    int targetRow = into->row();
    
    //We only support full rows
    assert(selectionBehavior() == QAbstractItemView::SelectRows);
    
    ///Remove the items
    std::map<int,std::map<int,TableItem*> > rowMoved;
    for (std::list<TableItem*>::iterator it = _imp->draggedItems.begin(); it!= _imp->draggedItems.end(); ++it) {
        rowMoved[(*it)->row()][(*it)->column()] = *it;
        TableItem* taken = _imp->model->takeItem((*it)->row(),(*it)->column());
        assert(taken == *it);
    }
    /// remove the rows in reverse order so that indexes are still valid
    for (std::map<int,std::map<int,TableItem*> >::reverse_iterator it = rowMoved.rbegin(); it != rowMoved.rend(); ++it) {
        _imp->model->removeRows(it->first);
        if (it->first <= targetRow) {
            --targetRow;
        }
    }
    _imp->draggedItems.clear();
    ///insert back at the correct position
    
    int nRows = _imp->model->rowCount();
    switch (position) {
        case QAbstractItemView::AboveItem: {
            _imp->model->insertRows(targetRow, rowMoved.size());
            break;
        }
        case QAbstractItemView::BelowItem: {
            ++targetRow;
            if (targetRow > nRows) {
                targetRow = nRows;
            }
            _imp->model->insertRows(targetRow, rowMoved.size());
            break;
        }
        default:
            assert(false);
            return;
    };
    
    int rowIndex = targetRow;
    for (std::map<int,std::map<int,TableItem*> >::iterator it = rowMoved.begin(); it!=rowMoved.end(); ++it,++rowIndex) {
        for (std::map<int,TableItem*>::iterator it2 = it->second.begin();it2!=it->second.end();++it2) {
            _imp->model->setItem(rowIndex, it2->first, it2->second);
        }
    }
    
    Q_EMIT itemDropped();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_TableModelView.cpp"
