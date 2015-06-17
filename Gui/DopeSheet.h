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

typedef std::map<QTreeWidgetItem *, DSNode *> DSNodeRows;
typedef std::map<QTreeWidgetItem *, DSKnob *> DSKnobRows;
// typedefs


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

    DSNodeRows getNodeRows() const;

    std::pair<double, double> getKeyframeRange() const;

    void addNode(boost::shared_ptr<NodeGui> nodeGui);
    void removeNode(NodeGui *node);

    SequenceTime getCurrentFrame() const;

    DSNode *findParentDSNode(QTreeWidgetItem *treeItem) const;
    DSNode *findDSNode(QTreeWidgetItem *nodeTreeItem) const;
    DSNode *findDSNode(Natron::Node *node) const;
    DSNode *findDSNode(const boost::shared_ptr<KnobI> knob) const;

    DSKnob *findDSKnob(QTreeWidgetItem *knobTreeItem) const;
    DSKnob *findDSKnob(KnobGui *knobGui) const;

    int getDim(const DSKnob *dsKnob, QTreeWidgetItem *item) const;

    bool isPartOfGroup(DSNode *dsNode) const;
    DSNode *getGroupDSNode(DSNode *dsNode) const;
    std::vector<DSNode *> getNodesFromGroup(DSNode *dsGroup) const;

    bool groupSubNodesAreHidden(NodeGroup *group) const;

    DSNode *getNearestTimeNodeFromOutputs(DSNode *dsNode) const;
    std::vector<DSNode *> getInputsConnected(DSNode *dsNode) const;
    Natron::Node *getNearestReader(DSNode *timeNode) const;

    DopeSheetSelectionModel *getSelectionModel() const;

    // User interaction
    void deleteSelectedKeyframes();

    void moveSelectedKeys(double dt);
    void trimReaderLeft(DSNode *reader, double newFirstFrame);
    void trimReaderRight(DSNode *reader, double newLastFrame);
    void slipReader(DSNode *reader, double dt);
    void moveReader(DSNode *reader, double dt);
    void moveGroup(DSNode *group, double dt);
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
    DSNode *createDSNode(const boost::shared_ptr<NodeGui> &nodeGui, ItemType itemType);

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

    DSKnobRows getChildData() const;

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
struct DSSelectedKey
{
    DSSelectedKey(DSKnob *knob, KeyFrame kf) :
        context(knob),
        key(kf)
    {
        assert(context->getDimension() != -1);
    }

    DSSelectedKey(const DSSelectedKey &other) :
        context(other.context),
        key(other.key)
    {}

    friend bool operator==(const DSSelectedKey &key, const DSSelectedKey &other)
    {
        if (key.context != other.context) {
            return false;
        }

        if (key.key != other.key) {
            return false;
        }

        if (key.context->getTreeItem() != other.context->getTreeItem()) {
            return false;
        }

        if (key.context->getDimension() != other.context->getDimension()) {
            return false;
        }

        return true;
    }

    DSKnob *context;
    KeyFrame key;
};

typedef boost::shared_ptr<DSSelectedKey> DSKeyPtr;
typedef std::list<DSKeyPtr> DSKeyPtrList;

class DopeSheetSelectionModel : public QObject
{
    Q_OBJECT

public:
    DopeSheetSelectionModel(DopeSheet *dopeSheet);
    ~DopeSheetSelectionModel();

    void selectAllKeyframes();
    void selectKeyframes(DSKnob *dsKnob, std::vector<DSSelectedKey> *result);

    void clearKeyframeSelection();
    void makeSelection(const std::vector<DSSelectedKey> &keys);

    bool isEmpty() const;

    DSKeyPtrList getSelectedKeyframes() const;
    std::vector<DSSelectedKey> getSelectionCopy() const;

    int getSelectedKeyframesCount() const;

    bool keyframeIsSelected(DSKnob *dsKnob, const KeyFrame &keyframe) const;
    DSKeyPtrList::iterator keyframeIsSelected(const DSSelectedKey &key) const;

    void emit_keyframeSelectionChanged();

    void onNodeAboutToBeRemoved(DSNode *removed);

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
