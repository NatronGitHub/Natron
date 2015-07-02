#include "DopeSheet.h"

// Qt includes
#include <QDebug> //REMOVEME
#include <QtEvents>
#include <QTreeWidget>
#include <QUndoStack>

// Natron includes
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NoOp.h"
#include "Engine/TimeLine.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditorUndoRedo.h"
#include "Gui/DopeSheetHierarchyView.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/KnobGui.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"


typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;
typedef std::pair<QTreeWidgetItem *, boost::shared_ptr<DSNode> > TreeItemAndDSNode;
typedef std::pair<QTreeWidgetItem *, DSKnob *> TreeItemAndDSKnob;

const int NO_DIM = -5;


////////////////////////// Helpers //////////////////////////

bool nodeHasAnimation(const boost::shared_ptr<NodeGui> &nodeGui)
{
    const std::vector<boost::shared_ptr<KnobI> > &knobs = nodeGui->getNode()->getKnobs();

    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin();
         it != knobs.end();
         ++it) {
        boost::shared_ptr<KnobI> knob = *it;

        if (knob->hasAnimation()) {
            return true;
        }
    }

    return false;
}


namespace {

bool nodeCanAnimate(const NodePtr &node)
{
    const std::vector<boost::shared_ptr<KnobI> > &knobs = node->getKnobs();

    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin();
         it != knobs.end();
         ++it) {
        boost::shared_ptr<KnobI> knob = *it;

        if (knob->isAnimationEnabled()) {
            return true;
        }
    }

    return false;
}

QTreeWidgetItem *createKnobNameItem(const QString &text,
                                    DopeSheet::ItemType itemType,
                                    int dimension,
                                    QTreeWidgetItem *parent)
{
    QTreeWidgetItem *ret = new QTreeWidgetItem;

    if (parent) {
        parent->addChild(ret);
    }

    ret->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);

    if (dimension != NO_DIM) {
        ret->setData(0, QT_ROLE_CONTEXT_DIM, dimension);
    }

    ret->setText(0, text);

    if (itemType == DopeSheet::ItemTypeKnobRoot
            || itemType == DopeSheet::ItemTypeKnobDim) {
        ret->setFlags(ret->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    }

    return ret;
}

} // anon namespace


////////////////////////// DopeSheet //////////////////////////

class DopeSheetPrivate
{
public:
    DopeSheetPrivate(DopeSheet *qq);
    ~DopeSheetPrivate();

    /* functions */
    Natron::Node *getNearestTimeFromOutputs_recursive(Natron::Node *node) const;
    Natron::Node *getNearestReaderFromInputs_recursive(Natron::Node *node) const;
    void getInputsConnected_recursive(Natron::Node *node, std::vector<boost::shared_ptr<DSNode> > *result) const;

    void pushUndoCommand(QUndoCommand *cmd);

    bool canTrimLeft(double newFirstFrame, double currentLastFrame) const;
    bool canTrimRight(double newLastFrame, double currentFirstFrame, double originalLastFrame) const;

    /* attributes */
    DopeSheet *q_ptr;
    DSTreeItemNodeMap treeItemNodeMap;

    DopeSheetSelectionModel *selectionModel;

    boost::scoped_ptr<QUndoStack> undoStack;

    std::vector<DopeSheetKey> keyframesClipboard;

    boost::shared_ptr<TimeLine> timeline;
};

DopeSheetPrivate::DopeSheetPrivate(DopeSheet *qq) :
    q_ptr(qq),
    treeItemNodeMap(),
    selectionModel(new DopeSheetSelectionModel(qq)),
    undoStack(new QUndoStack(qq)),
    keyframesClipboard(),
    timeline()
{

}

DopeSheetPrivate::~DopeSheetPrivate()
{
    delete selectionModel;
}

Natron::Node *DopeSheetPrivate::getNearestTimeFromOutputs_recursive(Natron::Node *node) const
{
    const std::list<Natron::Node *> &outputs = node->getOutputs();

    for (std::list<Natron::Node *>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        Natron::Node *output = (*it);

        std::string pluginID = output->getPluginID();

        if (pluginID == PLUGINID_OFX_RETIME
                || pluginID == PLUGINID_OFX_TIMEOFFSET
                || pluginID == PLUGINID_OFX_FRAMERANGE) {
            return output;
        }
        else {
            return getNearestTimeFromOutputs_recursive(output);
        }
    }

    return NULL;
}

Natron::Node *DopeSheetPrivate::getNearestReaderFromInputs_recursive(Natron::Node *node) const
{
    const std::vector<boost::shared_ptr<Natron::Node> > &inputs = node->getInputs_mt_safe();

    for (std::vector<boost::shared_ptr<Natron::Node> >::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = (*it);

        if (!input) {
            continue;
        }

        std::string pluginID = input->getPluginID();

        if (pluginID == PLUGINID_OFX_READOIIO ||
                pluginID == PLUGINID_OFX_READFFMPEG ||
                pluginID == PLUGINID_OFX_READPFM) {
            return input.get();
        }
        else {
            return getNearestReaderFromInputs_recursive(input.get());
        }
    }

    return NULL;
}

void DopeSheetPrivate::getInputsConnected_recursive(Natron::Node *node, std::vector<boost::shared_ptr<DSNode> > *result) const
{
    const std::vector<boost::shared_ptr<Natron::Node> > &inputs = node->getInputs_mt_safe();

    for (std::vector<boost::shared_ptr<Natron::Node> >::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = (*it);

        if (!input) {
            continue;
        }

        boost::shared_ptr<DSNode>dsNode = q_ptr->findDSNode(input.get());

        if (dsNode) {
            result->push_back(dsNode);
        }

        getInputsConnected_recursive(input.get(), result);
    }
}

void DopeSheetPrivate::pushUndoCommand(QUndoCommand *cmd)
{
    undoStack->setActive();
    undoStack->push(cmd);
}

bool DopeSheetPrivate::canTrimLeft(double newFirstFrame, double currentLastFrame) const
{
    if (newFirstFrame < 1) {
        return false;
    }

    if (newFirstFrame > currentLastFrame) {
        return false;
    }

    return true;
}

bool DopeSheetPrivate::canTrimRight(double newLastFrame, double currentFirstFrame, double originalLastFrame) const
{
    if (newLastFrame > originalLastFrame) {
        return false;
    }

    if (newLastFrame < currentFirstFrame) {
        return false;
    }

    return true;
}

DopeSheet::DopeSheet(Gui *gui, const boost::shared_ptr<TimeLine> &timeline) :
    _imp(new DopeSheetPrivate(this))
{
    _imp->timeline = timeline;

    gui->registerNewUndoStack(_imp->undoStack.get());
}

DopeSheet::~DopeSheet()
{
    _imp->treeItemNodeMap.clear();
}

DSTreeItemNodeMap DopeSheet::getItemNodeMap() const
{
    return _imp->treeItemNodeMap;
}

void DopeSheet::addNode(boost::shared_ptr<NodeGui> nodeGui)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    DopeSheet::ItemType nodeType = DopeSheet::ItemTypeCommon;

    NodePtr node = nodeGui->getNode();
    Natron::EffectInstance *effectInstance = node->getLiveInstance();

    std::string pluginID = node->getPluginID();

    if (pluginID == PLUGINID_OFX_READOIIO
            || pluginID == PLUGINID_OFX_READFFMPEG
            || pluginID == PLUGINID_OFX_READPFM) {
        nodeType = DopeSheet::ItemTypeReader;
    }
    else if (dynamic_cast<NodeGroup *>(effectInstance)) {
        nodeType = DopeSheet::ItemTypeGroup;
    }
    else if (pluginID == PLUGINID_OFX_RETIME) {
        nodeType = DopeSheet::ItemTypeRetime;
    }
    else if (pluginID == PLUGINID_OFX_TIMEOFFSET) {
        nodeType = DopeSheet::ItemTypeTimeOffset;
    }
    else if (pluginID == PLUGINID_OFX_FRAMERANGE) {
        nodeType = DopeSheet::ItemTypeFrameRange;
    }

    // Discard specific nodes
    if (nodeType == DopeSheet::ItemTypeCommon) {
        if (dynamic_cast<GroupInput *>(effectInstance) ||
                dynamic_cast<GroupOutput *>(effectInstance)) {
            return;
        }

        if (node->isRotoNode() || node->isRotoPaintingNode()) {
            return;
        }

        if (node->getKnobs().empty()) {
            return;
        }

        if (!nodeCanAnimate(node)) {
            return;
        }
    }

    nodeGui->ensurePanelCreated();

    boost::shared_ptr<DSNode>dsNode = createDSNode(nodeGui, nodeType);

    _imp->treeItemNodeMap.insert(TreeItemAndDSNode(dsNode->getTreeItem(), dsNode));

    Q_EMIT nodeAdded(dsNode.get());
}

void DopeSheet::removeNode(NodeGui *node)
{
    DSTreeItemNodeMap::iterator toRemove = _imp->treeItemNodeMap.end();

    for (DSTreeItemNodeMap::iterator it = _imp->treeItemNodeMap.begin();
         it != _imp->treeItemNodeMap.end();
         ++it)
    {
        if ((*it).second->getNodeGui().get() == node) {
            toRemove = it;

            break;
        }
    }

    if (toRemove == _imp->treeItemNodeMap.end()) {
        return;
    }

    boost::shared_ptr<DSNode> dsNode = (*toRemove).second;

    _imp->selectionModel->onNodeAboutToBeRemoved(dsNode);

    Q_EMIT nodeAboutToBeRemoved(dsNode.get());

    _imp->treeItemNodeMap.erase(toRemove);
}

boost::shared_ptr<DSNode> DopeSheet::mapNameItemToDSNode(QTreeWidgetItem *nodeTreeItem) const
{
    DSTreeItemNodeMap::const_iterator dsNodeIt = _imp->treeItemNodeMap.find(nodeTreeItem);

    if (dsNodeIt != _imp->treeItemNodeMap.end()) {
        return (*dsNodeIt).second;
    }

    return boost::shared_ptr<DSNode>();
}

boost::shared_ptr<DSKnob> DopeSheet::mapNameItemToDSKnob(QTreeWidgetItem *knobTreeItem) const
{
    boost::shared_ptr<DSKnob> ret;

    boost::shared_ptr<DSNode>dsNode = findParentDSNode(knobTreeItem);
    DSTreeItemKnobMap knobRows = dsNode->getItemKnobMap();

    DSTreeItemKnobMap::const_iterator clickedDSKnob = knobRows.find(knobTreeItem);

    if (clickedDSKnob == knobRows.end()) {
        ret.reset();
    }
    else {
        ret = clickedDSKnob->second;
    }

    return ret;
}

boost::shared_ptr<DSNode> DopeSheet::findParentDSNode(QTreeWidgetItem *treeItem) const
{
    QTreeWidgetItem *itemIt = treeItem;

    DSTreeItemNodeMap::const_iterator clickedDSNode = _imp->treeItemNodeMap.find(itemIt);
    while (clickedDSNode == _imp->treeItemNodeMap.end()) {
        if (!itemIt) {
            return boost::shared_ptr<DSNode>();
        }

        if (itemIt->parent()) {
            itemIt = itemIt->parent();
            clickedDSNode = _imp->treeItemNodeMap.find(itemIt);
        }
    }

    return (*clickedDSNode).second;
}

boost::shared_ptr<DSNode> DopeSheet::findDSNode(Natron::Node *node) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin();
         it != _imp->treeItemNodeMap.end();
         ++it) {
        boost::shared_ptr<DSNode> dsNode = (*it).second;

        if (dsNode->getInternalNode().get() == node) {
            return dsNode;
        }
    }

    return boost::shared_ptr<DSNode>();
}

boost::shared_ptr<DSNode> DopeSheet::findDSNode(const boost::shared_ptr<KnobI> &knob) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin(); it != _imp->treeItemNodeMap.end(); ++it) {
        boost::shared_ptr<DSNode>dsNode = (*it).second;

        DSTreeItemKnobMap knobRows = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            boost::shared_ptr<DSKnob> dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui()->getKnob() == knob) {
                return dsNode;
            }
        }
    }

    return boost::shared_ptr<DSNode>();
}

boost::shared_ptr<DSKnob> DopeSheet::findDSKnob(KnobGui *knobGui) const
{
    for (DSTreeItemNodeMap::const_iterator it = _imp->treeItemNodeMap.begin(); it != _imp->treeItemNodeMap.end(); ++it) {
        boost::shared_ptr<DSNode>dsNode = (*it).second;

        DSTreeItemKnobMap knobRows = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            boost::shared_ptr<DSKnob> dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui() == knobGui) {
                return dsKnob;
            }
        }
    }

    return boost::shared_ptr<DSKnob>();
}

bool DopeSheet::isPartOfGroup(DSNode *dsNode) const
{
    boost::shared_ptr<NodeGroup> parentGroup = boost::dynamic_pointer_cast<NodeGroup>(dsNode->getInternalNode()->getGroup());

    return (parentGroup);
}

boost::shared_ptr<DSNode> DopeSheet::getGroupDSNode(DSNode *dsNode) const
{
    boost::shared_ptr<NodeGroup> parentGroup = boost::dynamic_pointer_cast<NodeGroup>(dsNode->getInternalNode()->getGroup());

    boost::shared_ptr<DSNode> parentGroupDSNode;

    if (parentGroup) {
        parentGroupDSNode = findDSNode(parentGroup->getNode().get());
    }

    return parentGroupDSNode;
}

std::vector<boost::shared_ptr<DSNode> > DopeSheet::getImportantNodes(DSNode *dsNode) const
{
    std::vector<boost::shared_ptr<DSNode> > ret;

    DopeSheet::ItemType nodeType = dsNode->getItemType();

    if (nodeType == DopeSheet::ItemTypeGroup) {
        NodeGroup *nodeGroup = dynamic_cast<NodeGroup *>(dsNode->getInternalNode()->getLiveInstance());
        assert(nodeGroup);

        NodeList nodes = nodeGroup->getNodes();
        for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            NodePtr childNode = (*it);

            if (boost::shared_ptr<DSNode>isInDopeSheet = findDSNode(childNode.get())) {
                ret.push_back(isInDopeSheet);
            }
        }
    }
    else if (dsNode->isTimeNode()) {
        _imp->getInputsConnected_recursive(dsNode->getInternalNode().get(), &ret);
    }


    return ret;
}

/**
 * @brief DopeSheet::getNearestRetimeFromOutputs
 *
 * Returns the first Retime node connected in output with 'dsNode' in the node graph.
 */
boost::shared_ptr<DSNode> DopeSheet::getNearestTimeNodeFromOutputs(DSNode *dsNode) const
{
    Natron::Node *timeNode = _imp->getNearestTimeFromOutputs_recursive(dsNode->getInternalNode().get());

    return findDSNode(timeNode);
}

Natron::Node *DopeSheet::getNearestReader(DSNode *timeNode) const
{
    assert(timeNode->isTimeNode());

    Natron::Node *nearestReader = _imp->getNearestReaderFromInputs_recursive(timeNode->getInternalNode().get());

    return nearestReader;
}

DopeSheetSelectionModel *DopeSheet::getSelectionModel() const
{
    return _imp->selectionModel;
}

void DopeSheet::deleteSelectedKeyframes()
{
    if (_imp->selectionModel->isEmpty()) {
        return;
    }

    std::vector<DopeSheetKey> toRemove =_imp->selectionModel->getSelectionCopy();

    _imp->selectionModel->clearKeyframeSelection();

    _imp->pushUndoCommand(new DSRemoveKeysCommand(toRemove, this));
}

void DopeSheet::moveSelectedKeys(double dt)
{
    _imp->pushUndoCommand(new DSMoveKeysCommand(_imp->selectionModel->getSelectedKeyframes(), dt, this));
}

void DopeSheet::trimReaderLeft(const boost::shared_ptr<DSNode> &reader, double newFirstFrame)
{
    NodePtr node = reader->getInternalNode();

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get());
    assert(lastFrameKnob);

    double firstFrame = firstFrameKnob->getValue();

    if (newFirstFrame == firstFrame) {
        return;
    }

    if (_imp->canTrimLeft(newFirstFrame, lastFrameKnob->getValue())) {
        _imp->pushUndoCommand(new DSLeftTrimReaderCommand(reader, firstFrame, newFirstFrame));
    }
}

void DopeSheet::trimReaderRight(const boost::shared_ptr<DSNode> &reader, double newLastFrame)
{
    NodePtr node = reader->getInternalNode();

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get());
    assert(lastFrameKnob);
    Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("originalFrameRange").get());
    assert(originalFrameRangeKnob);

    double lastFrame = lastFrameKnob->getValue();

    if (newLastFrame == lastFrame) {
        return;
    }

    if (_imp->canTrimRight(newLastFrame, firstFrameKnob->getValue(), originalFrameRangeKnob->getValue(1))) {
        _imp->pushUndoCommand(new DSRightTrimReaderCommand(reader, lastFrame, newLastFrame, this));
    }
}

void DopeSheet::slipReader(const boost::shared_ptr<DSNode> &reader, double dt)
{
    NodePtr node = reader->getInternalNode();

    Knob<int> *firstFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get());
    assert(firstFrameKnob);
    Knob<int> *lastFrameKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get());
    assert(lastFrameKnob);
    Knob<int> *originalFrameRangeKnob = dynamic_cast<Knob<int> *>(node->getKnobByName("originalFrameRange").get());
    assert(originalFrameRangeKnob);

    int currentFirstFrame = firstFrameKnob->getValue();
    int currentLastFrame = lastFrameKnob->getValue();
    int originalLastFrame = originalFrameRangeKnob->getValue(1);

    bool canSlip = ( _imp->canTrimLeft(currentFirstFrame + dt, currentLastFrame)
                     && _imp->canTrimRight(currentLastFrame + dt, currentFirstFrame, originalLastFrame) );

    if (canSlip) {
        _imp->pushUndoCommand(new DSSlipReaderCommand(reader, dt, this));
    }
}

void DopeSheet::moveReader(const boost::shared_ptr<DSNode> &reader, double dt)
{
    _imp->pushUndoCommand(new DSMoveReaderCommand(reader, dt, this));
}

void DopeSheet::moveGroup(const boost::shared_ptr<DSNode> &group, double dt)
{
    _imp->pushUndoCommand(new DSMoveGroupCommand(group, dt, this));
}

void DopeSheet::copySelectedKeys()
{
    if (_imp->selectionModel->isEmpty()) {
        return;
    }

    _imp->keyframesClipboard.clear();

    DSKeyPtrList selectedKeyframes = _imp->selectionModel->getSelectedKeyframes();

    for (DSKeyPtrList::const_iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        DSKeyPtr selectedKey = (*it);

        _imp->keyframesClipboard.push_back(*selectedKey);
    }
}

void DopeSheet::pasteKeys()
{
    std::vector<DopeSheetKey> toPaste;

    for (std::vector<DopeSheetKey>::const_iterator it = _imp->keyframesClipboard.begin(); it != _imp->keyframesClipboard.end(); ++it) {
        DopeSheetKey key = (*it);

        toPaste.push_back(key);
    }

    _imp->pushUndoCommand(new DSPasteKeysCommand(toPaste, this));
}

void DopeSheet::setSelectedKeysInterpolation(Natron::KeyframeTypeEnum keyType)
{

    if (_imp->selectionModel->isEmpty()) {
        return;
    }

    DSKeyPtrList selectedKeyframes = _imp->selectionModel->getSelectedKeyframes();
    std::list<DSKeyInterpolationChange> changes;

    for (DSKeyPtrList::iterator it = selectedKeyframes.begin();
         it != selectedKeyframes.end();
         ++it) {
        DSKeyPtr keyPtr = (*it);
        DSKeyInterpolationChange change(keyPtr->key.getInterpolation(), keyType, keyPtr);

        changes.push_back(change);
    }

    _imp->pushUndoCommand(new DSSetSelectedKeysInterpolationCommand(changes, this));
}

void DopeSheet::setUndoStackActive()
{
    _imp->undoStack->setActive();
}

void DopeSheet::emit_modelChanged()
{
    Q_EMIT modelChanged();
}

SequenceTime DopeSheet::getCurrentFrame() const
{
    return _imp->timeline->currentFrame();
}

boost::shared_ptr<DSNode> DopeSheet::createDSNode(const boost::shared_ptr<NodeGui> &nodeGui, DopeSheet::ItemType itemType)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    NodePtr node = nodeGui->getNode();

    QTreeWidgetItem *nameItem = new QTreeWidgetItem;
    nameItem->setText(0, node->getLabel().c_str());
    nameItem->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);
    nameItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, true);

    boost::shared_ptr<DSNode> dsNode(new DSNode(this, itemType, nodeGui, nameItem));

    connect(node.get(), SIGNAL(labelChanged(QString)),
            this, SLOT(onNodeNameChanged(QString)));

    return dsNode;
}
void DopeSheet::onNodeNameChanged(const QString &name)
{
    Natron::Node *node = qobject_cast<Natron::Node *>(sender());
    boost::shared_ptr<DSNode>dsNode = findDSNode(node);

    dsNode->getTreeItem()->setText(0, name);
}

void DopeSheet::onKeyframeSetOrRemoved()
{
    if (boost::shared_ptr<DSKnob> dsKnob = findDSKnob(qobject_cast<KnobGui *>(sender()))) {
        Q_EMIT keyframeSetOrRemoved(dsKnob.get());
    }

    Q_EMIT modelChanged();
}


////////////////////////// DSKnob //////////////////////////

class DSKnobPrivate
{
public:
    DSKnobPrivate();
    ~DSKnobPrivate();

    /* attributes */
    int dimension;
    QTreeWidgetItem *nameItem;
    KnobGui *knobGui;
    boost::shared_ptr<KnobI> knob;
};

DSKnobPrivate::DSKnobPrivate() :
    dimension(-2),
    nameItem(0),
    knobGui(0),
    knob()
{}

DSKnobPrivate::~DSKnobPrivate()
{}


/**
 * @class DSKnob
 *
 * The DSKnob class describes a knob' set of keyframes in the
 * DopeSheet.
 */

/**
 * @brief DSKnob::DSKnob
 *
 * Constructs a DSKnob.
 * Adds an item in the hierarchy view with 'parentItem' as parent item.
 *
 * 'knob', 'dimension' and 'isMultiDim' areused to name this item.
 *
 *' knobGui' is used to ensure that the DopeSheet graphical elements will
 * properly react to any keyframe modification.
 *
 * /!\ We should improve the classes design.
 */
DSKnob::DSKnob(int dimension,
               QTreeWidgetItem *nameItem,
               KnobGui *knobGui) :
    _imp(new DSKnobPrivate)
{
    assert(knobGui);

    _imp->dimension = dimension;
    _imp->nameItem = nameItem;
    _imp->knobGui = knobGui;
    _imp->knob = knobGui->getKnob();
}

DSKnob::~DSKnob()
{}

QTreeWidgetItem *DSKnob::getTreeItem() const
{
    return _imp->nameItem;
}

QTreeWidgetItem *DSKnob::findDimTreeItem(int dimension) const
{
    if ( (isMultiDimRoot() && (dimension == -1) )  || (!isMultiDimRoot() && !dimension) ) {
        return _imp->nameItem;
    }

    QTreeWidgetItem *ret = 0;

    for (int i = 0; i < _imp->nameItem->childCount(); ++i) {
        QTreeWidgetItem *child = _imp->nameItem->child(i);

        if (dimension == child->data(0, QT_ROLE_CONTEXT_DIM).toInt()) {
            ret = child;

            break;
        }
    }

    return ret;
}

/**
 * @brief DSKnob::getKnobGui
 *
 *
 */
KnobGui *DSKnob::getKnobGui() const
{
    return _imp->knobGui;
}

boost::shared_ptr<KnobI> DSKnob::getInternalKnob() const
{
    return _imp->knob;
}

/**
 * @brief DSKnob::isMultiDimRoot
 *
 *
 */
bool DSKnob::isMultiDimRoot() const
{
    return (_imp->dimension == -1);
}

int DSKnob::getDimension() const
{
    return _imp->dimension;
}


////////////////////////// DopeSheetSelectionModel //////////////////////////

class DopeSheetSelectionModelPrivate
{
public:
    DopeSheetSelectionModelPrivate() :
        selectedKeyframes()
    {

    }

    ~DopeSheetSelectionModelPrivate()
    {
        selectedKeyframes.clear();
    }

    /* attributes */
    DopeSheet *dopeSheet;
    DSKeyPtrList selectedKeyframes;
};

DopeSheetSelectionModel::DopeSheetSelectionModel(DopeSheet *dopeSheet) :
    _imp(new DopeSheetSelectionModelPrivate)
{
    _imp->dopeSheet = dopeSheet;
}

DopeSheetSelectionModel::~DopeSheetSelectionModel()
{

}

void DopeSheetSelectionModel::selectAllKeyframes()
{
    std::vector<DopeSheetKey> result;

    DSTreeItemNodeMap nodeRows = _imp->dopeSheet->getItemNodeMap();

    for (DSTreeItemNodeMap::const_iterator it = nodeRows.begin(); it != nodeRows.end(); ++it) {
        boost::shared_ptr<DSNode>dsNode = (*it).second;

        DSTreeItemKnobMap dsKnobItems = dsNode->getItemKnobMap();

        for (DSTreeItemKnobMap::const_iterator itKnob = dsKnobItems.begin();
             itKnob != dsKnobItems.end();
             ++itKnob) {
            selectKeyframes((*itKnob).second, &result);
        }
    }

    makeSelection(result, (DopeSheetSelectionModel::SelectionTypeAdd | DopeSheetSelectionModel::SelectionTypeClear));
}

void DopeSheetSelectionModel::selectKeyframes(const boost::shared_ptr<DSKnob> &dsKnob, std::vector<DopeSheetKey> *result)
{
    assert(dsKnob);

    int dim = dsKnob->getDimension();

    int startDim = 0;
    int endDim = dsKnob->getInternalKnob()->getDimension();

    if (dim > -1) {
        startDim = dim;
        endDim = dim + 1;
    }

    for (int i = startDim; i < endDim; ++i) {
        KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(i)->getKeyFrames_mt_safe();

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            KeyFrame kf = (*kIt);

            boost::shared_ptr<DSKnob> context;
            if (dim == -1) {
                QTreeWidgetItem *childItem = dsKnob->findDimTreeItem(i);
                context = _imp->dopeSheet->mapNameItemToDSKnob(childItem);
            }
            else {
                context = dsKnob;
            }

            result->push_back(DopeSheetKey(context, kf));
        }
    }
}

void DopeSheetSelectionModel::clearKeyframeSelection()
{
    if (_imp->selectedKeyframes.empty()) {
        return;
    }

    makeSelection(std::vector<DopeSheetKey>(), DopeSheetSelectionModel::SelectionTypeClear);
}

void DopeSheetSelectionModel::makeSelection(const std::vector<DopeSheetKey> &keys, SelectionTypeFlags selectionFlags)
{
    // Don't allow unsupported combinations
    assert(! (selectionFlags & DopeSheetSelectionModel::SelectionTypeAdd
              &&
              selectionFlags & DopeSheetSelectionModel::SelectionTypeToggle));

    DSKeyPtrList oldSelection = _imp->selectedKeyframes;

    if (selectionFlags & DopeSheetSelectionModel::SelectionTypeClear) {
        Q_EMIT keyframeSelectionAboutToBeCleared();

        _imp->selectedKeyframes.clear();
    }

    for (std::vector<DopeSheetKey>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        DopeSheetKey key = (*it);

        DSKeyPtrList::iterator isAlreadySelected = keyframeIsSelected(key);

        if (isAlreadySelected == _imp->selectedKeyframes.end()) {
            DSKeyPtr dsKey(new DopeSheetKey(key));

            _imp->selectedKeyframes.push_back(dsKey);
        }
        else if (selectionFlags & DopeSheetSelectionModel::SelectionTypeToggle) {
            _imp->selectedKeyframes.erase(isAlreadySelected);
        }
    }

    if (_imp->selectedKeyframes != oldSelection) {
        emit_keyframeSelectionChanged();
    }
}

bool DopeSheetSelectionModel::isEmpty() const
{
    return _imp->selectedKeyframes.empty();
}

DSKeyPtrList DopeSheetSelectionModel::getSelectedKeyframes() const
{
    return _imp->selectedKeyframes;
}

std::vector<DopeSheetKey> DopeSheetSelectionModel::getSelectionCopy() const
{
    std::vector<DopeSheetKey> ret;

    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end();
         ++it) {
        ret.push_back(DopeSheetKey(**it));
    }

    return ret;
}

int DopeSheetSelectionModel::getSelectedKeyframesCount() const
{
    return _imp->selectedKeyframes.size();
}

bool DopeSheetSelectionModel::keyframeIsSelected(const boost::shared_ptr<DSKnob> &dsKnob, const KeyFrame &keyframe) const
{
    DSKeyPtrList::const_iterator isSelected = _imp->selectedKeyframes.end();

    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        DSKeyPtr selectedKey = (*it);
        boost::shared_ptr<DSKnob> knobContext = selectedKey->context.lock();
        assert(knobContext);

        if (knobContext == dsKnob && selectedKey->key == keyframe) {
            isSelected = it;
            break;
        }
    }

    return (isSelected != _imp->selectedKeyframes.end());
}

DSKeyPtrList::iterator DopeSheetSelectionModel::keyframeIsSelected(const DopeSheetKey &key) const
{
    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        DSKeyPtr selectedKey = (*it);

        if (*(selectedKey.get()) == key) {
            return it;
        }
    }

    return _imp->selectedKeyframes.end();
}

void DopeSheetSelectionModel::emit_keyframeSelectionChanged()
{
    Q_EMIT keyframeSelectionChanged();
}

void DopeSheetSelectionModel::onNodeAboutToBeRemoved(const boost::shared_ptr<DSNode> &removed)
{
    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end();) {
        DSKeyPtr key = (*it);

        boost::shared_ptr<DSKnob> knobContext = key->context.lock();
        assert(knobContext);

        if (_imp->dopeSheet->findDSNode(knobContext->getInternalKnob()) == removed) {
            it = _imp->selectedKeyframes.erase(it);
        }
        else {
            ++it;
        }
    }
}


////////////////////////// DSNode //////////////////////////

class DSNodePrivate
{
public:
    DSNodePrivate();
    ~DSNodePrivate();

    /* functions */
    void initGroupNode();

    /* attributes */
    DopeSheet *dopeSheetModel;

    DopeSheet::ItemType nodeType;

    boost::weak_ptr<NodeGui> nodeGui;

    QTreeWidgetItem *nameItem;

    DSTreeItemKnobMap itemKnobMap;

    bool isSelected;
};

DSNodePrivate::DSNodePrivate() :
    dopeSheetModel(0),
    nodeType(),
    nodeGui(),
    nameItem(0),
    itemKnobMap(),
    isSelected(false)
{}

DSNodePrivate::~DSNodePrivate()
{}

void DSNodePrivate::initGroupNode()
{
    boost::shared_ptr<NodeGui> node = nodeGui.lock();
    if (!node) {
        return;
    }
    NodeList subNodes = dynamic_cast<NodeGroup *>(node->getNode()->getLiveInstance())->getNodes();

    for (NodeList::const_iterator it = subNodes.begin(); it != subNodes.end(); ++it) {
        NodePtr subNode = (*it);
        boost::shared_ptr<NodeGui> subNodeGui = boost::dynamic_pointer_cast<NodeGui>(subNode->getNodeGui());

        if (!subNodeGui->getSettingPanel() || !subNodeGui->isSettingsPanelVisible()) {
            continue;
        }
    }
}

DSNode::DSNode(DopeSheet *model,
               DopeSheet::ItemType itemType,
               const boost::shared_ptr<NodeGui> &nodeGui,
               QTreeWidgetItem *nameItem) :
    _imp(new DSNodePrivate)
{
    _imp->dopeSheetModel = model;
    _imp->nodeType = itemType;
    _imp->nameItem = nameItem;
    _imp->nodeGui = nodeGui;

    // Create dope sheet knobs
    const KnobsAndGuis &knobs = nodeGui->getKnobs();

    for (KnobsAndGuis::const_iterator it = knobs.begin();
         it != knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first.lock();
        KnobGui *knobGui = it->second;

        if (!knob->canAnimate() || !knob->isAnimationEnabled()) {
            continue;
        }

        if (knob->getDimension() <= 1) {
            QTreeWidgetItem * nameItem = createKnobNameItem(knob->getDescription().c_str(),
                                                            DopeSheet::ItemTypeKnobDim,
                                                            0,
                                                            _imp->nameItem);

            DSKnob *dsKnob = new DSKnob(0, nameItem, knobGui);
            _imp->itemKnobMap.insert(TreeItemAndDSKnob(nameItem, dsKnob));
        }
        else {
            QTreeWidgetItem *multiDimRootItem = createKnobNameItem(knob->getDescription().c_str(),
                                                                   DopeSheet::ItemTypeKnobRoot,
                                                                   -1,
                                                                   _imp->nameItem);

            DSKnob *rootDSKnob = new DSKnob(-1, multiDimRootItem, knobGui);
            _imp->itemKnobMap.insert(TreeItemAndDSKnob(multiDimRootItem, rootDSKnob));

            for (int i = 0; i < knob->getDimension(); ++i) {
                QTreeWidgetItem *dimItem = createKnobNameItem(knob->getDimensionName(i).c_str(),
                                                              DopeSheet::ItemTypeKnobDim,
                                                              i,
                                                              multiDimRootItem);

                DSKnob *dimDSKnob = new DSKnob(i, dimItem, knobGui);
                _imp->itemKnobMap.insert(TreeItemAndDSKnob(dimItem, dimDSKnob));
            }
        }

        QObject::connect(knobGui, SIGNAL(keyFrameSet()),
                         _imp->dopeSheetModel, SLOT(onKeyframeSetOrRemoved()));

        QObject::connect(knobGui, SIGNAL(keyFrameRemoved()),
                         _imp->dopeSheetModel, SLOT(onKeyframeSetOrRemoved()));
    }

    // If some subnodes are already in the dope sheet, the connections must be set to update
    // the group's clip rect
    if (_imp->nodeType == DopeSheet::ItemTypeGroup) {
        _imp->initGroupNode();
    }
}

/**
 * @brief DSNode::~DSNode
 *
 * Deletes all this node's params.
 */
DSNode::~DSNode()
{
    delete _imp->nameItem;

    _imp->itemKnobMap.clear();
}

/**
 * @brief DSNode::getNameItem
 *
 * Returns the hierarchy view item associated with this node.
 */
QTreeWidgetItem *DSNode::getTreeItem() const
{
    return _imp->nameItem;
}

/**
 * @brief DSNode::getNode
 *
 * Returns the associated node.
 */
boost::shared_ptr<NodeGui> DSNode::getNodeGui() const
{
    return _imp->nodeGui.lock();
}

boost::shared_ptr<Natron::Node> DSNode::getInternalNode() const
{
    boost::shared_ptr<NodeGui> node = getNodeGui();
    return node ? node->getNode() : NodePtr();
}

/**
 * @brief DSNode::getTreeItemsAndDSKnobs
 *
 *
 */
DSTreeItemKnobMap DSNode::getItemKnobMap() const
{
    return _imp->itemKnobMap;
}

DopeSheet::ItemType DSNode::getItemType() const
{
    return _imp->nodeType;
}

bool DSNode::isTimeNode() const
{
    return (_imp->nodeType >= DopeSheet::ItemTypeRetime)
            && (_imp->nodeType < DopeSheet::ItemTypeGroup);
}

bool DSNode::isRangeDrawingEnabled() const
{
    return (_imp->nodeType >= DopeSheet::ItemTypeReader &&
            _imp->nodeType <= DopeSheet::ItemTypeGroup);
}

bool DSNode::canContainOtherNodeContexts() const
{
    return (_imp->nodeType >= (DopeSheet::ItemTypeReader + 1)
            && _imp->nodeType <= DopeSheet::ItemTypeGroup);
}

bool DSNode::containsNodeContext() const
{
    for (int i = 0; i < _imp->nameItem->childCount(); ++i) {
        int childType = _imp->nameItem->child(i)->data(0, QT_ROLE_CONTEXT_TYPE).toInt();

        if (childType != DopeSheet::ItemTypeKnobDim
                && childType != DopeSheet::ItemTypeKnobRoot) {
            return true;
        }

    }

    return false;
}
