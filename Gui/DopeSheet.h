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

#include "Gui/DopeSheetView.h"

class DopeSheetEditorPrivate;
class DSKnobPrivate;
class DopeSheetEditor;
class DSNodePrivate;
class Gui;
class HierarchyView;
class KnobI;
class KnobGui;
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


class DSKnob : public QObject
{
    Q_OBJECT

public:
    DSKnob(QTreeWidgetItem *nameItem,
           KnobGui *knobGui);
    ~DSKnob();

    QTreeWidgetItem *getNameItem() const;
    QRectF getNameItemRect() const;
    QRectF getNameItemRectForDim(int dim) const;

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
    friend class DopeSheetEditor;

    enum DSNodeType
    {
        CommonNodeType = 1001,
        ReaderNodeType,
        GroupNodeType
    };

    enum ClipColor
    {
        ClipFill,
        ClipOutline,
    };

    DSNode(DopeSheetEditor *dopeSheetEditor,
           QTreeWidgetItem *nameItem,
           const boost::shared_ptr<NodeGui> &nodeGui);
    ~DSNode();

    QTreeWidgetItem *getNameItem() const;
    QRectF getNameItemRect() const;

    boost::shared_ptr<NodeGui> getNodeGui() const;

    TreeItemsAndDSKnobs getTreeItemsAndDSKnobs() const;

    DSNode::DSNodeType getDSNodeType() const;

    DSKnob *findDSKnob(QTreeWidgetItem *item) const;

    bool isCommonNode() const;
    bool isReaderNode() const;
    bool isGroupNode() const;

    bool isWithinGroupNode() const;

    QRectF getClipRect() const;

    bool isSelected() const;
    void setSelected(bool selected);

    QColor getClipColor(ClipColor clipColor) const;

Q_SIGNALS:
    void clipRectChanged();

public Q_SLOTS:
    void onNodeNameChanged(const QString &name);
    void checkVisibleState();
    void computeClipRect();

private:
    boost::scoped_ptr<DSNodePrivate> _imp;
};

class DopeSheetEditor : public QWidget, public ScriptObject
{
    Q_OBJECT

public:
    friend class DSNode;
    friend class DopeSheetView;

    DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent = 0);
    ~DopeSheetEditor();

    HierarchyView *getHierarchyView() const;

    TreeItemsAndDSNodes getTreeItemsAndDSNodes() const;

    DSNode *findDSNode(const boost::shared_ptr<Natron::Node> &node) const;
    DSNode *findDSNode(QTreeWidgetItem *item) const;

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

public Q_SLOTS:
    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

    void refreshDopeSheetView();

private: /* functions */
    DSNode *createDSNode(const boost::shared_ptr<NodeGui> &nodeGui);
    DSKnob *createDSKnob(KnobGui *knobGui, DSNode *dsNode);

private Q_SLOTS:
    void refreshClipRects();

private:
    boost::scoped_ptr<DopeSheetEditorPrivate> _imp;
};

class HierarchyView : public QTreeWidget
{
public:
    friend class HierarchyViewItemDelegate;

    explicit HierarchyView(DopeSheetEditor *editor, QWidget *parent = 0);

protected:
    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;

private:
    DopeSheetEditor *m_editor;
};

#endif // DOPESHEET_H
