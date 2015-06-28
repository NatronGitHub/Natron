#ifndef DOPESHEETHIERARCHYVIEW_H
#define DOPESHEETHIERARCHYVIEW_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtGui/QTreeWidget>
#include <QtGui/QStyledItemDelegate>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif


class DopeSheet;
class DSKnob;
class DSNode;
class Gui;
class HierarchyViewPrivate;
class QModelIndex;
class QStyleOptionViewItem;


class HierarchyViewSelectionModel : public QItemSelectionModel
{
    Q_OBJECT

public:
    explicit HierarchyViewSelectionModel(QAbstractItemModel *model, QObject *parent = 0);
    ~HierarchyViewSelectionModel();

public Q_SLOTS:
    virtual void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) OVERRIDE FINAL;

private: /* functions */
    void selectChildren(const QModelIndex &index, QItemSelection *selection) const;
    void selectParents(const QModelIndex &index, QItemSelection *selection) const;
};


class HierarchyView : public QTreeWidget
{
    Q_OBJECT

public:
    explicit HierarchyView(DopeSheet *dopeSheetModel, Gui *gui, QWidget *parent = 0);
    ~HierarchyView();

    boost::shared_ptr<DSKnob> getDSKnobAt(int y) const;

    bool itemIsVisibleFromOutside(QTreeWidgetItem *item) const;
    int firstVisibleParentCenterY(QTreeWidgetItem * item) const;
    QTreeWidgetItem *lastVisibleChild(QTreeWidgetItem *item) const;

protected:
    virtual void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const OVERRIDE FINAL;

    bool childrenAreHidden(QTreeWidgetItem *item) const;
    QTreeWidgetItem *getParentItem(QTreeWidgetItem *item) const;
    void moveItem(QTreeWidgetItem *child, QTreeWidgetItem *newParent) const;

private Q_SLOTS:
    void onNodeAdded(DSNode *dsNode);
    void onNodeAboutToBeRemoved(DSNode *dsNode);
    void onKeyframeSetOrRemoved(DSKnob *dsKnob);

    void onItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onItemSelectionChanged();

private:
    boost::scoped_ptr<HierarchyViewPrivate> _imp;
};


class HierarchyViewItemDelegate : public QStyledItemDelegate
{
public:
    explicit HierarchyViewItemDelegate(QObject *parent = 0);

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
};

#endif // DOPESHEETHIERARCHYVIEW_H
