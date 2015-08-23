//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef TABLEMODELVIEW_H
#define TABLEMODELVIEW_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeView>
#include <QAbstractItemModel>
#include <QMetaType>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

class TableView;
class TableModel;
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
        : values(), view(0), id(-1),
          itemFlags(Qt::ItemIsEditable
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

Q_DECLARE_METATYPE(TableItem*)

struct TableViewPrivate;
class TableView
    : public QTreeView
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

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
    inline void removeCellWidget(int row, int column);
    TableItem * itemAt(const QPoint &p) const;
    TableItem * itemAt(int x, int y) const;
    QRect visualItemRect(const TableItem *item) const;

Q_SIGNALS:

    void deleteKeyPressed();
    void itemRightClicked(TableItem* item);

private:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual bool edit(const QModelIndex & index, QAbstractItemView::EditTrigger trigger, QEvent * event) OVERRIDE FINAL;

    
    boost::scoped_ptr<TableViewPrivate> _imp;
};

struct TableModelPrivate;
class TableModel
    : public QAbstractTableModel
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

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


#endif // TABLEMODELVIEW_H
