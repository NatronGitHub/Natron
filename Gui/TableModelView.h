/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#endif


CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
GCC_ONLY_DIAG_OFF(class-memaccess)
#include <QtCore/QVector>
GCC_ONLY_DIAG_ON(class-memaccess)
#include <QTreeView>
#include <QAbstractItemModel>
#include <QItemEditorFactory>
#include <QtCore/QMetaType>
#include <QtCore/QSize>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"
#include "Gui/LineEdit.h"


NATRON_NAMESPACE_ENTER

class TableItem
{
    friend class TableModel;
    friend class TableView;

    struct ItemData
    {
        int role;
        QVariant value;

        ItemData()
            : role(-1), value()
        {
        }

        ItemData(int role,
                 const QVariant & v)
            : role(role), value(v)
        {
        }

        inline bool operator==(const ItemData &other) const
        {
            return role == other.role && value == other.value;
        }
    };

    QVector<ItemData> values;
    TableView* view;
    int id;
    Qt::ItemFlags itemFlags;

public:

    TableItem()
        : values()
        , view(0)
        , id(-1)
        , itemFlags(Qt::ItemIsEditable
                    | Qt::ItemIsSelectable
                    | Qt::ItemIsUserCheckable
                    | Qt::ItemIsEnabled
                    | Qt::ItemIsDragEnabled
                    | Qt::ItemIsDropEnabled)
    {
    }

    TableItem(const TableItem & other);

    virtual ~TableItem();

    virtual TableItem * clone() const;
    inline TableView * tableWidget() const
    {
        return view;
    }

    int row() const;
    int column() const;
    void setSelected(bool select);
    bool isSelected() const;

    inline Qt::ItemFlags flags() const
    {
        return itemFlags;
    }

    void setFlags(Qt::ItemFlags flags);
    inline QString text() const
    {
        return data(Qt::DisplayRole).toString();
    }

    inline void setText(const QString &text);
    inline QIcon icon() const
    {
        return qvariant_cast<QIcon>( data(Qt::DecorationRole) );
    }

    inline void setIcon(const QIcon &icon);
    inline QString statusTip() const
    {
        return data(Qt::StatusTipRole).toString();
    }

    inline void setStatusTip(const QString &statusTip);
    inline QString toolTip() const
    {
        return data(Qt::ToolTipRole).toString();
    }

    inline void setToolTip(const QString &toolTip);
    inline QFont font() const
    {
        return qvariant_cast<QFont>( data(Qt::FontRole) );
    }

    inline void setFont(const QFont &font);
    inline int textAlignment() const
    {
        return data(Qt::TextAlignmentRole).toInt();
    }

    inline void setTextAlignment(int alignment)
    {
        setData(Qt::TextAlignmentRole, alignment);
    }

    inline QColor backgroundColor() const
    {
        return qvariant_cast<QColor>( data(Qt::BackgroundColorRole) );
    }

    inline void setBackgroundColor(const QColor &color)
    {
        setData(Qt::BackgroundColorRole, color);
    }

    inline QBrush background() const
    {
        return qvariant_cast<QBrush>( data(Qt::BackgroundRole) );
    }

    inline void setBackground(const QBrush &brush)
    {
        setData(Qt::BackgroundRole, brush);
    }

    inline QColor textColor() const
    {
        return qvariant_cast<QColor>( data(Qt::TextColorRole) );
    }

    inline void setTextColor(const QColor &color)
    {
        setData(Qt::TextColorRole, color);
    }

    inline QBrush foreground() const
    {
        return qvariant_cast<QBrush>( data(Qt::ForegroundRole) );
    }

    inline void setForeground(const QBrush &brush)
    {
        setData(Qt::ForegroundRole, brush);
    }

    inline Qt::CheckState checkState() const
    {
        return static_cast<Qt::CheckState>( data(Qt::CheckStateRole).toInt() );
    }

    inline void setCheckState(Qt::CheckState state)
    {
        setData(Qt::CheckStateRole, state);
    }

    inline QSize sizeHint() const
    {
        return qvariant_cast<QSize>( data(Qt::SizeHintRole) );
    }

    inline void setSizeHint(const QSize &size)
    {
        setData(Qt::SizeHintRole, size);
    }

    virtual QVariant data(int role) const;
    virtual void setData(int role, const QVariant &value);
    TableItem &operator=(const TableItem &other);
};


inline void
TableItem::setText(const QString &atext)
{
    setData(Qt::DisplayRole, atext);
}

inline void
TableItem::setIcon(const QIcon &aicon)
{
    setData(Qt::DecorationRole, aicon);
}

inline void
TableItem::setStatusTip(const QString &astatusTip)
{
    setData(Qt::StatusTipRole, astatusTip);
}

inline void
TableItem::setToolTip(const QString &atoolTip)
{
    setData(Qt::ToolTipRole, atoolTip);
}

inline void
TableItem::setFont(const QFont &afont)
{
    setData(Qt::FontRole, afont);
}

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

struct TableViewPrivate;
class TableView
    : public QTreeView
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit TableView(QWidget* parent = 0);

    virtual ~TableView();

    void setTableModel(TableModel* model);

    void setRowCount(int rows);
    int rowCount() const;

    void setColumnCount(int columns);
    int columnCount() const;

    int row(const TableItem *item) const;
    int column(const TableItem *item) const;

    TableItem * item(int row, int column) const;
    void setItem(int row, int column, TableItem *item);
    TableItem * takeItem(int row, int column);
    TableItem * horizontalHeaderItem(int column) const;
    void setHorizontalHeaderItem(int column, TableItem *item);
    TableItem * takeHorizontalHeaderItem(int column);
    void setHorizontalHeaderLabels(const QStringList &labels);

    void editItem(TableItem *item);
    void openPersistentEditor(TableItem *item);
    void closePersistentEditor(TableItem *item);

    bool isItemSelected(const TableItem *item) const;
    void setItemSelected(const TableItem *item, bool select);

    QWidget * cellWidget(int row, int column) const;
    void setCellWidget(int row, int column, QWidget *widget);
    void removeCellWidget(int row, int column);
    TableItem * itemAt(const QPoint &p) const;
    TableItem * itemAt(int x, int y) const;
    QRect visualItemRect(const TableItem *item) const;

    TableItem* editedItem() const;
    TableItem* currentItem() const;

Q_SIGNALS:

    void aboutToDrop();
    void itemDropped();
    void deleteKeyPressed();
    void itemRightClicked(TableItem* item);
    void itemDoubleClicked(TableItem* item);

private:

    virtual void startDrag(Qt::DropActions supportedActions) OVERRIDE FINAL;
    virtual void dragLeaveEvent(QDragLeaveEvent *e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent *e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual bool edit(const QModelIndex & index, QAbstractItemView::EditTrigger trigger, QEvent * event) OVERRIDE FINAL;

    void rebuildDraggedItemsFromSelection();

    boost::scoped_ptr<TableViewPrivate> _imp;
};

struct TableModelPrivate;
class TableModel
    : public QAbstractTableModel
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum ItemFlagsExtension
    {
        ItemIsHeaderItem = 128
    }; // we need this to separate header items from other items


    TableModel(int rows,
               int columns,
               TableView* view);

    virtual ~TableModel();

    virtual bool insertRows( int row, int count = 1, const QModelIndex &parent = QModelIndex() ) OVERRIDE FINAL;
    virtual bool insertColumns( int column, int count = 1, const QModelIndex &parent = QModelIndex() ) OVERRIDE FINAL;
    virtual bool removeRows( int row, int count = 1, const QModelIndex &parent = QModelIndex() ) OVERRIDE FINAL;
    virtual bool removeColumns( int column, int count = 1, const QModelIndex &parent = QModelIndex() ) OVERRIDE FINAL;

    void setTable(const std::vector<TableItem*>& items);
    void setItem(int row, int column, TableItem *item);
    TableItem * takeItem(int row, int column);
    TableItem * item(int row, int column) const;
    TableItem * item(const QModelIndex &index) const;
    void removeItem(TableItem *item);

    virtual QModelIndex index( int row,
                               int column,
                               const QModelIndex &parent = QModelIndex() ) const OVERRIDE FINAL
    {
        return QAbstractTableModel::index(row, column, parent);
    }

    QModelIndex index(const TableItem *item) const;
    Qt::ItemFlags flags(const QModelIndex &index) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    void setHorizontalHeaderItem(int section, TableItem *item);
    TableItem * takeHorizontalHeaderItem(int section);
    TableItem * horizontalHeaderItem(int section);

    void setRowCount(int rows);
    void setColumnCount(int columns);

    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const OVERRIDE FINAL;
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const OVERRIDE FINAL;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const OVERRIDE FINAL;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role)  OVERRIDE FINAL;
    virtual bool setItemData(const QModelIndex &index, const QMap<int, QVariant> &roles)  OVERRIDE FINAL;
    virtual QMap<int, QVariant> itemData(const QModelIndex &index) const OVERRIDE FINAL;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const OVERRIDE FINAL;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) OVERRIDE FINAL;
    inline long tableIndex(int row, int column) const;

    /**
     * @brief Override to implement sorting
     **/
    virtual void sort(int /*column*/,
                      Qt::SortOrder order = Qt::AscendingOrder) OVERRIDE { Q_UNUSED(order); }

    void itemChanged(TableItem *item);
    TableItem * createItem() const;

    void clear();

    bool isValid(const QModelIndex &index) const;

public Q_SLOTS:

    void onDataChanged(const QModelIndex & index);

Q_SIGNALS:

    void s_itemChanged(TableItem*);

private:

    boost::scoped_ptr<TableModelPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

Q_DECLARE_METATYPE(NATRON_NAMESPACE::TableItem*)

#endif // TABLEMODELVIEW_H
