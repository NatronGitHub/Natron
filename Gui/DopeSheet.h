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
class DSKnobPrivate;
class DopeSheetEditor;
class DopeSheetPrivate;
class DopeSheetView;
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
typedef std::map<QTreeWidgetItem *, DSKnob *> DSKnobRow;
// typedefs


/**
 * @brief The DSSelectedKey struct
 *
 *
 */
struct DSSelectedKey
{
    DSSelectedKey(DSKnob *knob, KeyFrame kf, QTreeWidgetItem *treeItem, int dim) :
        dsKnob(knob),
        key(kf),
        dimTreeItem(treeItem),
        dimension(dim)
    {}

    DSSelectedKey(const DSSelectedKey &other) :
        dsKnob(other.dsKnob),
        key(other.key),
        dimTreeItem(other.dimTreeItem),
        dimension(other.dimension)
    {}

    friend bool operator==(const DSSelectedKey &key1, const DSSelectedKey &key2)
    {
        if (key1.dsKnob != key2.dsKnob) {
            return false;
        }

        if (key1.key != key2.key) {
            return false;
        }

        if (key1.dimTreeItem != key2.dimTreeItem) {
            return false;
        }

        if (key1.dimension != key2.dimension) {
            return false;
        }

        return true;
    }

    DSKnob *dsKnob;
    KeyFrame key;
    QTreeWidgetItem *dimTreeItem;
    int dimension;
};


typedef boost::shared_ptr<DSSelectedKey> DSKeyPtr;
typedef std::list<DSKeyPtr> DSKeyPtrList;


/**
 * @brief The DopeSheet class
 *
 *
 */
class DopeSheet: public QObject
{
    Q_OBJECT

public:
    typedef QTreeWidgetItem TreeItem;

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

    DSKnob *findDSKnob(QTreeWidgetItem *knobTreeItem, int *dimension) const;
    DSKnob *findDSKnob(KnobGui *knobGui) const;

    int getDim(const DSKnob *dsKnob, QTreeWidgetItem *item) const;

    bool isPartOfGroup(DSNode *dsNode) const;
    DSNode *getGroupDSNode(DSNode *dsNode) const;
    std::vector<DSNode *> getNodesFromGroup(DSNode *dsGroup) const;

    bool isRangeBasedNode(DopeSheet::ItemType nodeType) const;
    bool canContainOtherNodes(DopeSheet::ItemType nodeType) const;

    bool groupSubNodesAreHidden(NodeGroup *group) const;

    DSNode *getNearestTimeNodeFromOutputs(DSNode *dsNode) const;
    std::vector<DSNode *> getInputsConnected(DSNode *dsNode) const;
    Natron::Node *getNearestReader(DSNode *timeNode) const;

    bool isBundle(DSNode *dsNode) const;

    // Keyframe selection logic
    DSKeyPtrList getSelectedKeyframes() const;
    int getSelectedKeyframesCount() const;

    void makeSelection(const std::vector<DSSelectedKey> &keys);
    void makeBooleanSelection(const std::vector<DSSelectedKey> &keys);

    bool keyframeIsSelected(int dimension, DSKnob *dsKnob, const KeyFrame &keyframe) const;
    DSKeyPtrList::iterator keyframeIsSelected(const DSSelectedKey &key) const;

    // User interaction
    void clearKeyframeSelection();
    void deleteSelectedKeyframes();
    void selectAllKeyframes();
    void selectKeyframes(QTreeWidgetItem *item, std::vector<DSSelectedKey> *result);

    void moveSelectedKeys(double dt);
    void trimReaderLeft(DSNode *reader, double newFirstFrame);
    void trimReaderRight(DSNode *reader, double newLastFrame);
    void slipReader(DSNode *reader, double dt);
    void moveReader(DSNode *reader, double time);
    void moveGroup(DSNode *group, double dt);
    void copySelectedKeys();
    void pasteKeys();
    void setSelectedKeysInterpolation(Natron::KeyframeTypeEnum keyType);

    // Undo/redo
    void setUndoStackActive();

    void emit_modelChanged();
    void emit_keyframeSelectionChanged();

Q_SIGNALS:
    void modelChanged();
    void nodeAdded(DSNode *dsNode);
    void nodeSettingsPanelOpened(DSNode *dsNode);
    void groupNodeSettingsPanelCloseChanged(DSNode *dsNode);
    void nodeAboutToBeRemoved(DSNode *dsNode);
    void keyframeSetOrRemoved(DSKnob *dsKnob);
    void keyframeSelectionAboutToBeCleared();
    void keyframeSelectionChanged();

private: /* functions */
    DSNode *createDSNode(const boost::shared_ptr<NodeGui> &nodeGui, ItemType nodeType);

private Q_SLOTS:
    void onSettingsPanelCloseChanged(bool closed);
    void onNodeNameChanged(const QString &name);
    void onKeyframeSetOrRemoved();

private:
    boost::scoped_ptr<DopeSheetPrivate> _imp;
};


/**
 * @brief The DSKnob class
 *
 *
 */
class DSKnob : public QObject
{
    Q_OBJECT

public:
    DSKnob(QTreeWidgetItem *nameItem,
           KnobGui *knobGui);
    ~DSKnob();

    QTreeWidgetItem *getTreeItem() const;
    QTreeWidgetItem *findDimTreeItem(int dimension) const;

    KnobGui *getKnobGui() const;
    boost::shared_ptr<KnobI> getInternalKnob() const;

    bool isMultiDim() const;

private:
    boost::scoped_ptr<DSKnobPrivate> _imp;
};

/**
 * @brief The DSNode class
 *
 *
 */
class DSNode : public QObject
{
    Q_OBJECT

public:
    DSNode(DopeSheet *model,
           DopeSheet::ItemType nodeType,
           const boost::shared_ptr<NodeGui> &nodeGui,
           QTreeWidgetItem *nameItem);
    ~DSNode();

    QTreeWidgetItem *getTreeItem() const;

    boost::shared_ptr<NodeGui> getNodeGui() const;
    boost::shared_ptr<Natron::Node> getNode() const;

    DSKnobRow getChildData() const;

    DopeSheet::ItemType getItemType() const;

    bool isTimeNode() const;

private:
    boost::scoped_ptr<DSNodePrivate> _imp;
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

    void frame(double xMin, double xMax);

public Q_SLOTS:
    void toggleTripleSync(bool enabled);

private:
    boost::scoped_ptr<DopeSheetEditorPrivate> _imp;
};

#endif // DOPESHEET_H
