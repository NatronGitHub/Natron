/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef TABLEMODELVIEW_H
#define TABLEMODELVIEW_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#endif


CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeView>
#include <QAbstractItemModel>
#include <QItemEditorFactory>
#include <QIcon>
#include <QPixmap>
#include <QStyleOption>
#include <QtCore/QMetaType>
#include <QtCore/QSize>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"
#include "Gui/LineEdit.h"


NATRON_NAMESPACE_ENTER

/**
 * @class An item in the TableModel, it represents the content of 1 row in the table
 **/
struct TableItemPrivate;
class TableItem : public boost::enable_shared_from_this<TableItem>
{
    friend class TableModel;
    friend class TableView;
    friend struct TableItemPrivate;

    struct MakeSharedEnabler;

    TableItem(const TableModelPtr& model);

public:

    enum ChildIndicatorVisibilityEnum
    {
        // Child indicator always visible
        eChildIndicatorVisibilityShowAlways,

        // Child indicator never visible
        eChildIndicatorVisibilityShowNever,

        // Child indicator visible only if the item has children
        eChildIndicatorVisibilityShowIfChildren
    };

    /**
     * @brief Creates an item
     * @param parent If not NULL, this is a pointer to the parent row, otherwise
     * this item is considered a top-level item.
     * If the parent not NULL, it must belong to a model already
     **/
    static TableItemPtr create(const TableModelPtr& model, const TableItemPtr& parent = TableItemPtr());

    virtual ~TableItem();

    /**
     * @brief A pointer to the parent item if any. If null, the item is considered a toplevel item
     **/
    TableItemPtr getParentItem() const;

    /**
     * @brief If this item is currently managed by a model, returns a pointer to this model.
     * The item may not co-exist in multiple models at once.
     **/
    TableModelPtr getModel() const;


    /**
     * @brief Controls whether the child indicator is visible or not.
     **/
    void setChildIndicatorVisibility(ChildIndicatorVisibilityEnum visibility);
    ChildIndicatorVisibilityEnum getChildIndicatorVisibility() const;

    /**
     * @brief Returns the children of this item. The ownership of the children is handled by this item.
     **/
    const std::vector<TableItemPtr>& getChildren() const;

    /**
     * @brief Returns true if the given item is a child of this item or any sub item recursively
     **/
    bool isChildRecursive(const TableItemPtr& item) const;
    
    /**
     * @brief Inserts a child at the given row.
     * It does not work if the model is not a table model.
     * If this item does not belong to a model this function will return false.
     * If row is -1 or the count of children, the child is appended
     * Returns true on success.
     **/
    bool insertChild(int row, const TableItemPtr& child);
    
    /**
     * @brief Returns the children of this item. The ownership of the children is handled by this item.
     **/
    bool addChild(const TableItemPtr& child)
    {
        return insertChild(-1, child);
    }
    
    /**
     * @brief Removes the given child if found. Returns true on success.
     **/
    bool removeChild(const TableItemPtr& child);

    /**
     * @brief Returns the index of this item in the parent children
     * If the item is not in a model, this will return -1
     **/
    int getRowInParent() const;

    /**
     * @brief Get/Set flags at the given column. If the column index is invalid an exception is thrown
     **/
    Qt::ItemFlags getFlags(int col) const;
    void setFlags(int col, Qt::ItemFlags flags);

    /**
     * @brief Get/Set data for the given role at the given column. If the column index is invalid an exception is thrown
     **/
    QVariant getData(int col, int role) const;
    void setData(int col, int role, const QVariant &value);

    //// Below are convenience calls for set/getData for specific roles
    inline QString getText(int col) const
    {
        return getData(col, Qt::DisplayRole).toString();
    }

    inline void setText(int col, const QString &text)
    {
        setData(col, Qt::DisplayRole, text);
    }

    inline QIcon getIcon(int col) const
    {
        return qvariant_cast<QIcon>( getData(col, Qt::DecorationRole) );
    }

    void setIcon(int col, const QIcon &aicon);

    inline QString getStatusTip(int col) const
    {
        return getData(col, Qt::StatusTipRole).toString();
    }


    inline void setStatusTip(int col, const QString &astatusTip)
    {
        setData(col, Qt::StatusTipRole, astatusTip);
    }

    inline QString getToolTip(int col) const
    {
        return getData(col, Qt::ToolTipRole).toString();
    }


    inline void setToolTip(int col, const QString &atoolTip)
    {
        setData(col, Qt::ToolTipRole, atoolTip);
    }


    inline QFont getFont(int col) const
    {
        return qvariant_cast<QFont>( getData(col, Qt::FontRole) );
    }

    inline void setFont(int col, const QFont &afont)
    {
        setData(col, Qt::FontRole, afont);
    }


    inline int getTextAlignment(int col) const
    {
        return getData(col, Qt::TextAlignmentRole).toInt();
    }

    inline void setTextAlignment(int col, int alignment)
    {
        setData(col, Qt::TextAlignmentRole, alignment);
    }

    inline QColor getBackgroundColor(int col) const
    {
        return qvariant_cast<QColor>( getData(col, Qt::BackgroundColorRole) );
    }

    inline void setBackgroundColor(int col, const QColor &color)
    {
        setData(col, Qt::BackgroundColorRole, color);
    }

    inline QBrush getBackground(int col) const
    {
        return qvariant_cast<QBrush>( getData(col, Qt::BackgroundRole) );
    }

    inline void setBackground(int col, const QBrush &brush)
    {
        setData(col, Qt::BackgroundRole, brush);
    }

    inline QColor getTextColor(int col) const
    {
        return qvariant_cast<QColor>( getData(col, Qt::TextColorRole) );
    }

    inline void setTextColor(int col, const QColor &color)
    {
        setData(col, Qt::TextColorRole, color);
    }

    inline QBrush getForeground(int col) const
    {
        return qvariant_cast<QBrush>( getData(col, Qt::ForegroundRole) );
    }

    inline void setForeground(int col, const QBrush &brush)
    {
        setData(col, Qt::ForegroundRole, brush);
    }

    inline Qt::CheckState getCheckState(int col) const
    {
        return static_cast<Qt::CheckState>( getData(col, Qt::CheckStateRole).toInt() );
    }

    inline void setCheckState(int col, Qt::CheckState state)
    {
        setData(col, Qt::CheckStateRole, state);
    }

    inline QSize getSizeHint(int col) const
    {
        return qvariant_cast<QSize>( getData(col, Qt::SizeHintRole) );
    }

    inline void setSizeHint(int col, const QSize &size)
    {
        setData(col, Qt::SizeHintRole, size);
    }


private:

    void refreshChildrenIndices();

    boost::scoped_ptr<TableItemPrivate> _imp;
};


/**
 * @brief A custom factory to create editors using our widgets
 **/
class TableItemEditorFactory
    : public QItemEditorFactory
{
public:

    TableItemEditorFactory()
        : QItemEditorFactory() {}

    virtual ~TableItemEditorFactory()
    {
    }

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    virtual QWidget * createEditor(QVariant::Type type, QWidget *parent) const OVERRIDE FINAL;
    virtual QByteArray valuePropertyName(QVariant::Type type) const OVERRIDE FINAL;
#else
    virtual QWidget * createEditor(int userType, QWidget *parent) const OVERRIDE FINAL;
    virtual QByteArray valuePropertyName(int userType) const OVERRIDE FINAL;
#endif
};


class ExpandingLineEdit
    : public LineEdit
{
    Q_OBJECT

public:
    ExpandingLineEdit(QWidget *parent);

    virtual ~ExpandingLineEdit() {}

    void setWidgetOwnsGeometry(bool value)
    {
        widgetOwnsGeometry = value;
    }

protected:
    void changeEvent(QEvent *e);

public Q_SLOTS:
    void resizeToContents();

private:
    void updateMinimumWidth();

    int originalWidth;
    bool widgetOwnsGeometry;
};

/**
 * @brief The view of a TableModel. It can be either a tree or a table
 **/
struct TableViewPrivate;
class TableView
    : public QTreeView
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    TableView(Gui* gui, QWidget* parent = 0);

    virtual ~TableView();

    /**
     * @brief Set the model. The view does not take ownership of the model.
     **/
    void setTableModel(const TableModelPtr& model);
    TableModelPtr getTableModel() const;

    void editItem(const TableItemPtr& item);
    void openPersistentEditor(const TableItemPtr& item);
    void closePersistentEditor(const TableItemPtr& item);

    bool isItemSelected(const TableItemConstPtr& item) const;
    void setItemSelected(const TableItemPtr& item, bool select);

    QWidget * cellWidget(int row, int column, QModelIndex parent) const;
    void setCellWidget(int row, int column, QModelIndex parent, QWidget *widget);
    void removeCellWidget(int row, int column);

    TableItemPtr itemAt(const QPoint &p) const;
    TableItemPtr itemAt(int x, int y) const;

    QRect visualItemRect(const TableItemConstPtr& item) const;

    TableItemPtr editedItem() const;
    TableItemPtr currentItem() const;

    Gui* getGui() const;

Q_SIGNALS:

    void aboutToDrop();
    void itemDropped();
    void deleteKeyPressed();
    void itemRightClicked(QPoint globalPos, TableItemPtr item);
    void itemDoubleClicked(TableItemPtr item);

protected:
    
    virtual void dragLeaveEvent(QDragLeaveEvent *e) OVERRIDE ;
    virtual void dragEnterEvent(QDragEnterEvent *e) OVERRIDE ;
    virtual void dropEvent(QDropEvent* e) OVERRIDE ;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE;
    virtual void mouseMoveEvent(QMouseEvent *event) OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;

    virtual void setupAndExecDragObject(QDrag* drag,
                                        const std::list<TableItemPtr>& rows,
                                        Qt::DropActions supportedActions,
                                        Qt::DropAction defaultAction);
private:
    
    virtual void startDrag(Qt::DropActions supportedActions) OVERRIDE FINAL;

    void setupDragObject(Qt::DropActions supportedActions);
    
    inline QPoint getOffset() const {
        return QPoint(isRightToLeft() ? -horizontalOffset() : horizontalOffset(), verticalOffset());
    }

    void calcLogicalIndices(QVector<int> *logicalIndices, QVector<QStyleOptionViewItemV4::ViewItemPosition> *itemPositions, int left, int right) const;

    void adjustViewOptionsForIndex(QStyleOptionViewItemV4 *option, const TableItemPtr &item, const QModelIndex& index) const;
    QStyleOptionViewItemV4 viewOptionsV4() const;
    QPixmap renderToPixmap(const std::list<TableItemPtr>& rows, QRect *r) const;


    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual bool edit(const QModelIndex & index, QAbstractItemView::EditTrigger trigger, QEvent * event) OVERRIDE FINAL;

    void rebuildDraggedItemsFromSelection();

    boost::scoped_ptr<TableViewPrivate> _imp;
};

/**
 * @class TableModel is a model that goes into a view. It can be used either as a tree model or a table model. In each case
 * each row is 1 item. Each item holds data for all columns.
 * For each function taking a row index, the index is expressed in the parent children list, if a parent is supplied, otherwise
 * this will be the row index in the top level items.
 **/
struct TableModelPrivate;
class TableModel
    : public QAbstractTableModel
    , public boost::enable_shared_from_this<TableModel>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    friend class TableItem;
    friend class TableView;

    struct MakeSharedEnabler;

public:
    enum TableModelTypeEnum
    {
        // Each row is a top-level item
        eTableModelTypeTable,

        // Each item may have children
        eTableModelTypeTree
    };

protected:
    // used by boost::make_shared
    TableModel(int cols, TableModelTypeEnum type);

public:
    static TableModelPtr create(int columns, TableModelTypeEnum type);

    virtual ~TableModel();
    
    /**
     @brief Returns the type of the model, @see TableModelTypeEnum
     **/
    TableModelTypeEnum getType() const;

    /**
     * @brief Return a list of top level items, or all items for a model of type eTableModelTypeTable
     **/
    const std::vector<TableItemPtr>& getTopLevelItems() const;

    /**
     * @brief Return an item from the table.
     * @param row The index of the row to return. If parent is a valid model index, the returned item
     * will be the one at the given row in the parent's children list (if it is valid in that list).
     * If the parent is not a valid model index, the top-level item at the given row will be returned.
     **/
    TableItemPtr getItem(int row, const QModelIndex& parent = QModelIndex()) const;

    /**
     * @brief Convenience function, same as item(index.rox(), index.parent()). The column component of the 
     * model index is not relevant
     **/
    TableItemPtr getItem(const QModelIndex& index) const;

    /**
     * @brief Returns the parent index of the given child
     **/
    virtual QModelIndex parent(const QModelIndex &child) const OVERRIDE FINAL;

    /**
     * @brief Returns true if parent has any children; otherwise returns false.
     * Use rowCount() on the parent to find out the number of children.
     * See also parent() and index().
     **/
    virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const OVERRIDE FINAL;

    /**
     * @brief Returns the model index of the item
     **/
    QModelIndex getItemIndex(const TableItemConstPtr& item) const;

    /**
     * @brief Returns the model index of the item at the given row. This is the same as
     * index(getItem(row,parent)) but with the column being the one passed in parameter.
     **/
    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const OVERRIDE FINAL;

    /**
     * @brief Returns the flags of the given item
     **/
    virtual Qt::ItemFlags flags(const QModelIndex &index) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Set the item used as header. It is not part of the model
     **/
    void setHorizontalHeaderItem(const TableItemPtr& item);

    /**
     * @brief Convenience function wrapping setHorizontalHeaderItem calls for each column to provide an a text and/or icon
     **/
    void setHorizontalHeaderData(const std::vector<std::pair<QString, QIcon> >& sections);
    void setHorizontalHeaderData(const QStringList& sections);

    /**
     * @brief For table type only: this pre-allocates the table with empty items.
     * This is a convenience wrapper over insertRows
     **/
    void setRowCount(int rows);

    /**
     * @brief For a table type only: 
     * Set the table from another one
     **/
    void setTable(const std::vector<TableItemPtr>& items);

    /**
     * @brief For a table type only: 
     * Set the item at given row
     **/
    void setRow(int row, const TableItemPtr& item);
    
    /**
     * @brief Insert count empty rows at the given index. If row is equals to the rowCount of children in parent
     * then the rows are appended. For tree model type, new items are created. For the table model type, the
     * storage only is allocated.
     **/
    virtual bool insertRows( int row, int count = 1, const QModelIndex &parent = QModelIndex() ) OVERRIDE FINAL;
    
    /**
     * @brief Removes
     **/
    virtual bool removeRows( int row, int count = 1, const QModelIndex &parent = QModelIndex() ) OVERRIDE FINAL;
    
    /**
     * @brief same as insertRows except that the new item is provided by caller
     **/
    bool insertTopLevelItem(int row, const TableItemPtr& item);
    
    /**
     * @brief Convenience function: calls insertTopLevelItem with row = -1
     **/
    bool addTopLevelItem(const TableItemPtr& item)
    {
        return insertTopLevelItem(-1, item);
    }
    


    /**
     * @brief For a tree, this removes an item from the model. If the item belongs to a parent, it removes it from the parent children list.
     * For a table, this is the same as calling setRow with a NULL pointer and also calling removeRows on that same row
     * @returns true on success
     **/
    bool removeItem(const TableItemPtr& item);

 
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const OVERRIDE FINAL;
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const OVERRIDE FINAL;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const OVERRIDE FINAL;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role)  OVERRIDE FINAL;
    virtual bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)  OVERRIDE FINAL;
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const OVERRIDE FINAL;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const OVERRIDE FINAL;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) OVERRIDE FINAL;
    /**
     * @brief Override to implement sorting
     **/
    virtual void sort(int /*column*/,
                      Qt::SortOrder order = Qt::AscendingOrder) OVERRIDE { Q_UNUSED(order); }


    /**
     * @brief Clears the model: Nothing in the model will contain references to TableItem's. 
     * The caller should make sure it no longers refers to items in the table
     **/
    void clear();

public Q_SLOTS:

    void onDataChanged(const QModelIndex & tl, const QModelIndex& br);

Q_SIGNALS:

    // Wrapper over the dataChanged signal that just gives the pointer to the table item directly
    void s_itemChanged(TableItemPtr);

    // This hacks the dataChanged by making it more convenient, but essentially does the same
    void itemDataChanged(TableItemPtr item, int col, int role);


private:

    virtual bool insertColumns( int column, int count = 1, const QModelIndex &parent = QModelIndex() ) OVERRIDE FINAL;

    virtual bool removeColumns( int column, int count = 1, const QModelIndex &parent = QModelIndex() ) OVERRIDE FINAL;

    void onItemChanged(const TableItemPtr& item, int col, int role, bool valuesChanged);

    void refreshTopLevelItemIndices();

    boost::scoped_ptr<TableModelPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

Q_DECLARE_METATYPE(NATRON_NAMESPACE::TableItemPtr)

#endif // TABLEMODELVIEW_H
