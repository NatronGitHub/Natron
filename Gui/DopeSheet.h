#ifndef DOPESHEET_H
#define DOPESHEET_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Global/Macros.h"

#include "Engine/ScriptObject.h"


class DopeSheetEditorPrivate;
class DSKnobPrivate;
class DopeSheetEditor;
class DopeSheetPrivate;
class DSNodePrivate;
class Gui;
class HierarchyView;
class HierarchyViewPrivate;
class KnobI;
class KnobGui;
class NodeGroup;
class NodeGui;
class TimeLine;

namespace Natron {
class Node;
}


//
class DSNode;
class DSKnob;

typedef std::map<QTreeWidgetItem *, DSNode *> TreeItemsAndDSNodes;
typedef std::map<QTreeWidgetItem *, DSKnob *> TreeItemsAndDSKnobs;
// typedefs


class DopeSheet: public QObject
{
    Q_OBJECT

public:
    friend class DSNode;

    DopeSheet();
    ~DopeSheet();

    HierarchyView *getHierarchyView() const;
    void setHierarchyView(HierarchyView *hierarchyView);

    TreeItemsAndDSNodes getData() const;

    std::pair<double, double> getKeyframeRange() const;

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

    DSNode *findParentDSNode(QTreeWidgetItem *item) const;
    DSNode *findDSNode(QTreeWidgetItem *item) const;
    DSNode *findDSNode(Natron::Node *node) const;

    DSKnob *findDSKnob(QTreeWidgetItem *item, int *dimension) const;
    DSKnob *findDSKnob(const QPoint &point, int *dimension) const;

    DSNode *getParentGroupDSNode(DSNode *dsNode) const;

    bool groupSubNodesAreHidden(NodeGroup *group) const;

Q_SIGNALS:
    void modelChanged();

public Q_SLOTS:
    void onNodeNameChanged(const QString &name);

private: /* functions */
    DSNode *createDSNode(const boost::shared_ptr<NodeGui> &nodeGui);
    DSKnob *createDSKnob(KnobGui *knobGui, DSNode *dsNode);

private Q_SLOTS:
    void refreshClipRects();

private:
    boost::scoped_ptr<DopeSheetPrivate> _imp;
};

class DSKnob : public QObject
{
    Q_OBJECT

public:
    DSKnob(QTreeWidgetItem *nameItem,
           KnobGui *knobGui);
    ~DSKnob();

    QTreeWidgetItem *getTreeItem() const;
    QRectF getTreeItemRect() const;
    QRectF getTreeItemRectForDim(int dim) const;

    KnobGui *getKnobGui() const;

    bool isMultiDim() const;

Q_SIGNALS:
    void needNodesVisibleStateChecking();

public Q_SLOTS:
    void checkVisibleState();

private:
    boost::scoped_ptr<DSKnobPrivate> _imp;
};

class DSNode : public QObject
{
    Q_OBJECT

public:

    enum DSNodeType
    {
        CommonNodeType = 1001,
        ReaderNodeType,
        GroupNodeType
    };

    DSNode(DopeSheet *model,
           DSNode::DSNodeType nodeType,
           QTreeWidgetItem *nameItem,
           const boost::shared_ptr<NodeGui> &nodeGui);
    ~DSNode();

    QTreeWidgetItem *getTreeItem() const;
    QRectF getTreeItemRect() const;

    boost::shared_ptr<NodeGui> getNodeGui() const;

    TreeItemsAndDSKnobs getTreeItemsAndDSKnobs() const;

    DSNode::DSNodeType getDSNodeType() const;

    std::pair<double, double> getClipRange() const;

Q_SIGNALS:
    void clipRangeChanged();

public Q_SLOTS:
    void checkVisibleState();
    void computeReaderRange();
    void computeGroupRange();

private:
    boost::scoped_ptr<DSNodePrivate> _imp;
};

class DopeSheetEditor : public QWidget, public ScriptObject
{
    Q_OBJECT

public:
    friend class DopeSheetView;

    DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent = 0);
    ~DopeSheetEditor();

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

public Q_SLOTS:
    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    boost::scoped_ptr<DopeSheetEditorPrivate> _imp;
};

class HierarchyView : public QTreeWidget
{
    Q_OBJECT

public:
    friend class HierarchyViewItemDelegate;

    explicit HierarchyView(DopeSheet *model, QWidget *parent = 0);
    ~HierarchyView();

protected:
    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;

private:
    boost::scoped_ptr<HierarchyViewPrivate> _imp;
};

#endif // DOPESHEET_H
