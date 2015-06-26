#ifndef DOPESHEET_H
#define DOPESHEET_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Curve.h"
#include "Engine/ScriptObject.h"

#include "Global/GlobalDefines.h"
#include "Global/Macros.h"

class DopeSheetEditorPrivate;
class DopeSheetPrivate;
class DopeSheetSelectionModel;
class DopeSheetSelectionModelPrivate;
class DSKnobPrivate;
class DSNodePrivate;
class Gui;
class HierarchyView;
class KnobI;
class KnobGui;
class NodeGroup;
class NodeGui;
class QUndoCommand;
class TimeLine;

namespace Natron {
class Node;
}

class DSKnob;
class DSNode;

typedef boost::shared_ptr<DSNode> DSNodePtr;
typedef boost::shared_ptr<DSKnob> DSKnobPtr;

typedef std::map<QTreeWidgetItem *, boost::shared_ptr<DSNode> > DSTreeItemNodeMap;
typedef std::map<QTreeWidgetItem *, boost::shared_ptr<DSKnob> > DSTreeItemKnobMap;
// typedefs

const int QTREEWIDGETITEM_CONTEXT_TYPE_ROLE = Qt::UserRole + 1;
const int QTREEWIDGETITEM_DIM_ROLE = Qt::UserRole + 2;


bool nodeHasAnimation(const boost::shared_ptr<NodeGui> &nodeGui);


/**
 * @brief The DopeSheet class
 *
 *
 */
class DopeSheet: public QObject
{
    Q_OBJECT

public:
    enum ItemType
    {
        ItemTypeCommon = 1001,

        // Range-based nodes
        ItemTypeReader,
        ItemTypeRetime,
        ItemTypeTimeOffset,
        ItemTypeFrameRange,
        ItemTypeGroup,

        // Others
        ItemTypeKnobRoot,
        ItemTypeKnobDim
    };

    DopeSheet(Gui *gui, const boost::shared_ptr<TimeLine> &timeline);
    ~DopeSheet();

    DSTreeItemNodeMap getNodeRows() const;

    std::pair<double, double> getKeyframeRange() const;

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

    SequenceTime getCurrentFrame() const;

    boost::shared_ptr<DSNode> findParentDSNode(QTreeWidgetItem *treeItem) const;
    boost::shared_ptr<DSNode> findDSNode(QTreeWidgetItem *nodeTreeItem) const;
    boost::shared_ptr<DSNode> findDSNode(Natron::Node *node) const;
    boost::shared_ptr<DSNode> findDSNode(const boost::shared_ptr<KnobI> knob) const;

    boost::shared_ptr<DSNode> getDSNodeFromItem(QTreeWidgetItem *item, bool *itemIsNode = 0) const;

    boost::shared_ptr<DSKnob> findDSKnob(QTreeWidgetItem *knobTreeItem) const;
    boost::shared_ptr<DSKnob> findDSKnob(KnobGui *knobGui) const;

    int getDim(const DSKnob *dsKnob, QTreeWidgetItem *item) const;

    bool isPartOfGroup(DSNode *dsNode) const;
    boost::shared_ptr<DSNode> getGroupDSNode(DSNode *dsNode) const;
    bool groupSubNodesAreHidden(NodeGroup *group) const;

    std::vector<boost::shared_ptr<DSNode> > getImportantNodes(DSNode *dsNode) const;

    boost::shared_ptr<DSNode> getNearestTimeNodeFromOutputs(DSNode *dsNode) const;
    Natron::Node *getNearestReader(DSNode *timeNode) const;

    DopeSheetSelectionModel *getSelectionModel() const;

    // User interaction
    void deleteSelectedKeyframes();

    void moveSelectedKeys(double dt);
    void trimReaderLeft(const boost::shared_ptr<DSNode> &reader, double newFirstFrame);
    void trimReaderRight(const boost::shared_ptr<DSNode> &reader, double newLastFrame);
    void slipReader(const boost::shared_ptr<DSNode> &reader, double dt);
    void moveReader(const boost::shared_ptr<DSNode> &reader, double dt);
    void moveGroup(const boost::shared_ptr<DSNode> &group, double dt);
    void copySelectedKeys();
    void pasteKeys();
    void setSelectedKeysInterpolation(Natron::KeyframeTypeEnum keyType);

    // Undo/redo
    void setUndoStackActive();

    void emit_modelChanged();

Q_SIGNALS:
    void modelChanged();
    void nodeAdded(DSNode *dsNode);
    void nodeAboutToBeRemoved(DSNode *dsNode);
    void nodeRemoved(DSNode *dsNode);
    void keyframeSetOrRemoved(DSKnob *dsKnob);

private: /* functions */
    boost::shared_ptr<DSNode> createDSNode(const boost::shared_ptr<NodeGui> &nodeGui, ItemType itemType);

private Q_SLOTS:
    void onNodeNameChanged(const QString &name);
    void onKeyframeSetOrRemoved();

private:
    boost::scoped_ptr<DopeSheetPrivate> _imp;
};


/**
 * @brief The DSNode class
 *
 *
 */
class DSNode
{
public:
    DSNode(DopeSheet *model,
           DopeSheet::ItemType itemType,
           const boost::shared_ptr<NodeGui> &nodeGui,
           QTreeWidgetItem *nameItem);
    ~DSNode();

    QTreeWidgetItem *getTreeItem() const;

    boost::shared_ptr<NodeGui> getNodeGui() const;
    boost::shared_ptr<Natron::Node> getInternalNode() const;

    DSTreeItemKnobMap getChildData() const;

    DopeSheet::ItemType getItemType() const;

    bool isTimeNode() const;

    bool isRangeDrawingEnabled() const;

    bool canContainOtherNodeContexts() const;
    bool containsNodeContext() const;

private:
    boost::scoped_ptr<DSNodePrivate> _imp;
};


/**
 * @brief The DSKnob class
 *
 *
 */
class DSKnob
{
public:
    DSKnob(int dimension,
           QTreeWidgetItem *nameItem,
           KnobGui *knobGui);
    ~DSKnob();

    QTreeWidgetItem *getTreeItem() const;
    QTreeWidgetItem *findDimTreeItem(int dimension) const;

    KnobGui *getKnobGui() const;
    boost::shared_ptr<KnobI> getInternalKnob() const;

    bool isMultiDimRoot() const;
    int getDimension() const;

private:
    boost::scoped_ptr<DSKnobPrivate> _imp;
};

/**
 * @brief The DSSelectedKey struct
 *
 *
 */
struct DopeSheetKey
{
    DopeSheetKey(const boost::shared_ptr<DSKnob> &knob, KeyFrame kf) :
        context(knob),
        key(kf)
    {
        boost::shared_ptr<DSKnob> knobContext = context.lock();
        assert(knobContext);

        assert(knobContext->getDimension() != -1);
    }

    DopeSheetKey(const DopeSheetKey &other) :
        context(other.context),
        key(other.key)
    {}

    friend bool operator==(const DopeSheetKey &key, const DopeSheetKey &other)
    {
        boost::shared_ptr<DSKnob> knobContext = key.context.lock();
        boost::shared_ptr<DSKnob> otherKnobContext = other.context.lock();
        if (!knobContext || !otherKnobContext) {
            return false;
        }

        if (knobContext != otherKnobContext) {
            return false;
        }

        if (key.key != other.key) {
            return false;
        }

        if (knobContext->getTreeItem() != otherKnobContext->getTreeItem()) {
            return false;
        }

        if (knobContext->getDimension() != otherKnobContext->getDimension()) {
            return false;
        }

        return true;
    }

    boost::weak_ptr<DSKnob> context;
    KeyFrame key;
};

typedef boost::shared_ptr<DopeSheetKey> DSKeyPtr;
typedef std::list<DSKeyPtr> DSKeyPtrList;

class DopeSheetSelectionModel : public QObject
{
    Q_OBJECT

public:
    enum SelectionType {
        SelectionTypeOneByOne,
        SelectionTypeAdd,
        SelectionTypeToggle
    };

    DopeSheetSelectionModel(DopeSheet *dopeSheet);
    ~DopeSheetSelectionModel();

    void selectAllKeyframes();
    void selectKeyframes(const boost::shared_ptr<DSKnob> &dsKnob, std::vector<DopeSheetKey> *result);

    void clearKeyframeSelection();
    void makeSelection(const std::vector<DopeSheetKey> &keys, SelectionType selectionType);

    bool isEmpty() const;

    DSKeyPtrList getSelectedKeyframes() const;
    std::vector<DopeSheetKey> getSelectionCopy() const;

    int getSelectedKeyframesCount() const;

    bool keyframeIsSelected(const boost::shared_ptr<DSKnob> &dsKnob, const KeyFrame &keyframe) const;
    DSKeyPtrList::iterator keyframeIsSelected(const DopeSheetKey &key) const;

    void emit_keyframeSelectionChanged();

    void onNodeAboutToBeRemoved(const boost::shared_ptr<DSNode> &removed);

Q_SIGNALS:
    void keyframeSelectionAboutToBeCleared();
    void keyframeSelectionChanged();

private:
    boost::scoped_ptr<DopeSheetSelectionModelPrivate> _imp;
};


/**
 * @brief The DopeSheetEditor class
 *
 *
 */
class DopeSheetEditor : public QWidget, public ScriptObject
{
    Q_OBJECT

public:
    DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent = 0);
    ~DopeSheetEditor();

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

    void centerOn(double xMin, double xMax);

public Q_SLOTS:
    void toggleTripleSync(bool enabled);

private:
    boost::scoped_ptr<DopeSheetEditorPrivate> _imp;
};

#endif // DOPESHEET_H
