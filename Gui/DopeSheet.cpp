#include "DopeSheet.h"

// Qt includes
#include <QDebug> //REMOVEME
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
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
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"


typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;
typedef std::pair<QTreeWidgetItem *, DSNode *> TreeItemAndDSNode;
typedef std::pair<QTreeWidgetItem *, DSKnob *> TreeItemAndDSKnob;

const int QTREEWIDGETITEM_DIM_ROLE = Qt::UserRole + 1;


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
    void getInputsConnected_recursive(Natron::Node *node, std::vector<DSNode *> *result) const;

    void pushUndoCommand(QUndoCommand *cmd);

    bool canTrimLeft(double newFirstFrame, double currentLastFrame) const;
    bool canTrimRight(double newLastFrame, double currentFirstFrame, double originalLastFrame) const;

    /* attributes */
    DopeSheet *q_ptr;
    DSNodeRows nodeRows;

    DopeSheetSelectionModel *selectionModel;

    boost::scoped_ptr<QUndoStack> undoStack;

    std::vector<DSSelectedKey> keyframesClipboard;

    boost::shared_ptr<TimeLine> timeline;
};

DopeSheetPrivate::DopeSheetPrivate(DopeSheet *qq) :
    q_ptr(qq),
    nodeRows(),
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

void DopeSheetPrivate::getInputsConnected_recursive(Natron::Node *node, std::vector<DSNode *> *result) const
{
    const std::vector<boost::shared_ptr<Natron::Node> > &inputs = node->getInputs_mt_safe();

    for (std::vector<boost::shared_ptr<Natron::Node> >::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        NodePtr input = (*it);

        if (!input) {
            continue;
        }

        DSNode *dsNode = q_ptr->findDSNode(input.get());

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
    for (DSNodeRows::iterator it = _imp->nodeRows.begin();
         it != _imp->nodeRows.end(); ++it) {
        delete (*it).second;
    }

    _imp->nodeRows.clear();
}

DSNodeRows DopeSheet::getNodeRows() const
{
    return _imp->nodeRows;
}

std::pair<double, double> DopeSheet::getKeyframeRange() const
{
    std::pair<double, double> ret;

    std::vector<double> dimFirstKeys;
    std::vector<double> dimLastKeys;

    DSNodeRows dsNodeItems = _imp->nodeRows;

    for (DSNodeRows::const_iterator it = dsNodeItems.begin(); it != dsNodeItems.end(); ++it) {
        if ((*it).first->isHidden()) {
            continue;
        }

        DSNode *dsNode = (*it).second;

        DSKnobRows dsKnobItems = dsNode->getChildData();

        for (DSKnobRows::const_iterator itKnob = dsKnobItems.begin(); itKnob != dsKnobItems.end(); ++itKnob) {
            if ((*itKnob).first->isHidden()) {
                continue;
            }

            DSKnob *dsKnob = (*itKnob).second;

            for (int i = 0; i < dsKnob->getKnobGui()->getKnob()->getDimension(); ++i) {
                KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(i)->getKeyFrames_mt_safe();

                if (keyframes.empty()) {
                    continue;
                }

                dimFirstKeys.push_back(keyframes.begin()->getTime());
                dimLastKeys.push_back(keyframes.rbegin()->getTime());
            }
        }
    }

    if (dimFirstKeys.empty() || dimLastKeys.empty()) {
        ret.first = 0;
        ret.second = 0;
    }
    else {
        ret.first = *std::min_element(dimFirstKeys.begin(), dimFirstKeys.end());
        ret.second = *std::max_element(dimLastKeys.begin(), dimLastKeys.end());
    }

    return ret;
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

    DSNode *dsNode = createDSNode(nodeGui, nodeType);

    _imp->nodeRows.insert(TreeItemAndDSNode(dsNode->getTreeItem(), dsNode));

    Q_EMIT nodeAdded(dsNode);
}

void DopeSheet::removeNode(NodeGui *node)
{
    DSNodeRows::iterator toRemove = _imp->nodeRows.end();

    for (DSNodeRows::iterator it = _imp->nodeRows.begin();
         it != _imp->nodeRows.end();
         ++it)
    {
        if ((*it).second->getNodeGui().get() == node) {
            toRemove = it;

            break;
        }
    }

    if (toRemove == _imp->nodeRows.end()) {
        return;
    }

    DSNode *dsNode = (*toRemove).second;

    _imp->selectionModel->onNodeAboutToBeRemoved(dsNode);

    Q_EMIT nodeAboutToBeRemoved(dsNode);

    _imp->nodeRows.erase(toRemove);

    delete (dsNode);
}

SequenceTime DopeSheet::getCurrentFrame() const
{
    return _imp->timeline->currentFrame();
}

DSNode *DopeSheet::findParentDSNode(QTreeWidgetItem *treeItem) const
{
    QTreeWidgetItem *itemIt = treeItem;

    DSNodeRows::const_iterator clickedDSNode = _imp->nodeRows.find(itemIt);
    while (clickedDSNode == _imp->nodeRows.end()) {
        if (!itemIt) {
            return NULL;
        }

        if (itemIt->parent()) {
            itemIt = itemIt->parent();
            clickedDSNode = _imp->nodeRows.find(itemIt);
        }
    }

    return (*clickedDSNode).second;
}

DSNode *DopeSheet::findDSNode(QTreeWidgetItem *nodeTreeItem) const
{
    DSNodeRows::const_iterator dsNodeIt = _imp->nodeRows.find(nodeTreeItem);

    if (dsNodeIt != _imp->nodeRows.end()) {
        return (*dsNodeIt).second;
    }

    return NULL;
}

DSNode *DopeSheet::findDSNode(Natron::Node *node) const
{
    for (DSNodeRows::const_iterator it = _imp->nodeRows.begin();
         it != _imp->nodeRows.end();
         ++it) {
        DSNode *dsNode = (*it).second;

        if (dsNode->getInternalNode().get() == node) {
            return dsNode;
        }
    }

    return NULL;
}

DSNode *DopeSheet::findDSNode(const boost::shared_ptr<KnobI> knob) const
{
    for (DSNodeRows::const_iterator it = _imp->nodeRows.begin(); it != _imp->nodeRows.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSKnobRows knobRows = dsNode->getChildData();

        for (DSKnobRows::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            DSKnob *dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui()->getKnob() == knob) {
                return dsNode;
            }
        }
    }

    return NULL;
}

DSKnob *DopeSheet::findDSKnob(QTreeWidgetItem *knobTreeItem) const
{
    DSKnob *ret = 0;

    DSNode *dsNode = findParentDSNode(knobTreeItem);
    DSKnobRows knobRows = dsNode->getChildData();

    DSKnobRows::const_iterator clickedDSKnob = knobRows.find(knobTreeItem);

    if (clickedDSKnob == knobRows.end()) {
        ret = NULL;
    }
    else {
        ret = clickedDSKnob->second;
    }

    return ret;
}

DSKnob *DopeSheet::findDSKnob(KnobGui *knobGui) const
{
    for (DSNodeRows::const_iterator it = _imp->nodeRows.begin(); it != _imp->nodeRows.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSKnobRows knobRows = dsNode->getChildData();

        for (DSKnobRows::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            DSKnob *dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui() == knobGui) {
                return dsKnob;
            }
        }
    }

    return NULL;
}

bool DopeSheet::isPartOfGroup(DSNode *dsNode) const
{
    boost::shared_ptr<NodeGroup> parentGroup = boost::dynamic_pointer_cast<NodeGroup>(dsNode->getInternalNode()->getGroup());

    return (parentGroup);
}

DSNode *DopeSheet::getGroupDSNode(DSNode *dsNode) const
{
    boost::shared_ptr<NodeGroup> parentGroup = boost::dynamic_pointer_cast<NodeGroup>(dsNode->getInternalNode()->getGroup());

    DSNode *parentGroupDSNode = 0;

    if (parentGroup) {
        parentGroupDSNode = findDSNode(parentGroup->getNode().get());
    }

    return parentGroupDSNode;
}

std::vector<DSNode *> DopeSheet::getNodesFromGroup(DSNode *dsGroup) const
{
    assert(dsGroup->getItemType() == DopeSheet::ItemTypeGroup);

    NodeGroup *nodeGroup = dynamic_cast<NodeGroup *>(dsGroup->getInternalNode()->getLiveInstance());
    assert(nodeGroup);

    std::vector<DSNode *> ret;

    NodeList nodes = nodeGroup->getNodes();
    for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr childNode = (*it);

        if (DSNode *isInDopeSheet = findDSNode(childNode.get())) {
            ret.push_back(isInDopeSheet);
        }
    }

    return ret;
}

bool DopeSheet::groupSubNodesAreHidden(NodeGroup *group) const
{
    bool ret = true;

    NodeList subNodes = group->getNodes();

    for (NodeList::const_iterator it = subNodes.begin(); it != subNodes.end(); ++it) {
        NodePtr node = (*it);

        DSNode *dsNode = findDSNode(node.get());

        if (!dsNode) {
            continue;
        }

        if (!dsNode->getTreeItem()->isHidden()) {
            ret = false;

            break;
        }
    }

    return ret;
}

/**
 * @brief DopeSheet::getNearestRetimeFromOutputs
 *
 * Returns the first Retime node connected in output with 'dsNode' in the node graph.
 */
DSNode *DopeSheet::getNearestTimeNodeFromOutputs(DSNode *dsNode) const
{
    Natron::Node *timeNode = _imp->getNearestTimeFromOutputs_recursive(dsNode->getInternalNode().get());

    return findDSNode(timeNode);
}

std::vector<DSNode *> DopeSheet::getInputsConnected(DSNode *dsNode) const
{
    std::vector<DSNode *> ret;

    _imp->getInputsConnected_recursive(dsNode->getInternalNode().get(), &ret);

    return ret;
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

    std::vector<DSSelectedKey> toRemove =_imp->selectionModel->getSelectionCopy();

    _imp->selectionModel->clearKeyframeSelection();

    _imp->pushUndoCommand(new DSRemoveKeysCommand(toRemove, this));
}

void DopeSheet::moveSelectedKeys(double dt)
{
    _imp->pushUndoCommand(new DSMoveKeysCommand(_imp->selectionModel->getSelectedKeyframes(), dt, this));
}

void DopeSheet::trimReaderLeft(DSNode *reader, double newFirstFrame)
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
        _imp->pushUndoCommand(new DSLeftTrimReaderCommand(node.get(), firstFrame, newFirstFrame, this));
    }
}

void DopeSheet::trimReaderRight(DSNode *reader, double newLastFrame)
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
        _imp->pushUndoCommand(new DSRightTrimReaderCommand(node.get(), lastFrame, newLastFrame, this));
    }
}

void DopeSheet::slipReader(DSNode *reader, double dt)
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
        _imp->pushUndoCommand(new DSSlipReaderCommand(node.get(), dt, this));
    }
}

void DopeSheet::moveReader(DSNode *reader, double dt)
{
    _imp->pushUndoCommand(new DSMoveReaderCommand(reader->getNodeGui()->getNode().get(), dt, this));
}

void DopeSheet::moveGroup(DSNode *group, double dt)
{
    _imp->pushUndoCommand(new DSMoveGroupCommand(group->getInternalNode().get(), dt, this));
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
    std::vector<DSSelectedKey> toPaste;

    for (std::vector<DSSelectedKey>::const_iterator it = _imp->keyframesClipboard.begin(); it != _imp->keyframesClipboard.end(); ++it) {
        DSSelectedKey key = (*it);

//        // Retrieve the tree item associated with the key dimension
//        QTreeWidgetItem *dimTreeItem = key.context->getTreeItem();

//        if (dimTreeItem->isSelected()) {
        toPaste.push_back(key);
//        }
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

DSNode *DopeSheet::createDSNode(const boost::shared_ptr<NodeGui> &nodeGui, DopeSheet::ItemType itemType)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    NodePtr node = nodeGui->getNode();

    QTreeWidgetItem *nameItem = new QTreeWidgetItem(itemType);
    nameItem->setText(0, node->getLabel().c_str());

    DSNode *dsNode = new DSNode(this, itemType, nodeGui, nameItem);

    connect(node.get(), SIGNAL(labelChanged(QString)),
            this, SLOT(onNodeNameChanged(QString)));

    return dsNode;
}
void DopeSheet::onNodeNameChanged(const QString &name)
{
    Natron::Node *node = qobject_cast<Natron::Node *>(sender());
    DSNode *dsNode = findDSNode(node);

    dsNode->getTreeItem()->setText(0, name);
}

void DopeSheet::onKeyframeSetOrRemoved()
{
    if (DSKnob *dsKnob = findDSKnob(qobject_cast<KnobGui *>(sender()))) {
        Q_EMIT keyframeSetOrRemoved(dsKnob);
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

        if (dimension == child->data(0, QTREEWIDGETITEM_DIM_ROLE).toInt()) {
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
    std::vector<DSSelectedKey> result;

    DSNodeRows nodeRows = _imp->dopeSheet->getNodeRows();

    for (DSNodeRows::const_iterator it = nodeRows.begin(); it != nodeRows.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSKnobRows dsKnobItems = dsNode->getChildData();

        for (DSKnobRows::const_iterator itKnob = dsKnobItems.begin();
             itKnob != dsKnobItems.end();
             ++itKnob) {
            selectKeyframes((*itKnob).second, &result);
        }
    }

    makeSelection(result);
}

void DopeSheetSelectionModel::selectKeyframes(DSKnob *dsKnob, std::vector<DSSelectedKey> *result)
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

            DSKnob *context = 0;
            if (dim == -1) {
                QTreeWidgetItem *childItem = dsKnob->findDimTreeItem(i);
                context = _imp->dopeSheet->findDSKnob(childItem);
            }
            else {
                context = dsKnob;
            }

            result->push_back(DSSelectedKey(context, kf));
        }
    }
}

void DopeSheetSelectionModel::clearKeyframeSelection()
{
    if (_imp->selectedKeyframes.empty()) {
        return;
    }

    Q_EMIT keyframeSelectionAboutToBeCleared();

    _imp->selectedKeyframes.clear();

    Q_EMIT keyframeSelectionChanged();
}

void DopeSheetSelectionModel::makeSelection(const std::vector<DSSelectedKey> &keys)
{
    Q_EMIT keyframeSelectionAboutToBeCleared();

    _imp->selectedKeyframes.clear();

    for (std::vector<DSSelectedKey>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        DSSelectedKey key = (*it);

        DSKeyPtrList::iterator isAlreadySelected = keyframeIsSelected(key);

        if (isAlreadySelected == _imp->selectedKeyframes.end()) {
            DSKeyPtr selected(new DSSelectedKey(key));

            _imp->selectedKeyframes.push_back(selected);
        }
    }

    Q_EMIT keyframeSelectionChanged();
}

bool DopeSheetSelectionModel::isEmpty() const
{
    return _imp->selectedKeyframes.empty();
}

DSKeyPtrList DopeSheetSelectionModel::getSelectedKeyframes() const
{
    return _imp->selectedKeyframes;
}

std::vector<DSSelectedKey> DopeSheetSelectionModel::getSelectionCopy() const
{
    std::vector<DSSelectedKey> ret;

    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end();
         ++it) {
        ret.push_back(DSSelectedKey(**it));
    }

    return ret;
}

int DopeSheetSelectionModel::getSelectedKeyframesCount() const
{
    return _imp->selectedKeyframes.size();
}

bool DopeSheetSelectionModel::keyframeIsSelected(DSKnob *dsKnob, const KeyFrame &keyframe) const
{
    DSKeyPtrList::const_iterator isSelected = _imp->selectedKeyframes.end();

    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        DSKeyPtr selectedKey = (*it);

        if (selectedKey->context == dsKnob && selectedKey->key == keyframe) {
            isSelected = it;
            break;
        }
    }

    return (isSelected != _imp->selectedKeyframes.end());
}

DSKeyPtrList::iterator DopeSheetSelectionModel::keyframeIsSelected(const DSSelectedKey &key) const
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

void DopeSheetSelectionModel::onNodeAboutToBeRemoved(DSNode *removed)
{
    for (DSKeyPtrList::iterator it = _imp->selectedKeyframes.begin();
         it != _imp->selectedKeyframes.end();) {
        DSKeyPtr key = (*it);

        if (_imp->dopeSheet->findDSNode(key->context->getInternalKnob()) == removed) {
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
    DSNodePrivate(DSNode *qq);
    ~DSNodePrivate();

    /* functions */
    void createDSKnobs(KnobGui *knobGui, DSNode *dsNode);

    void initGroupNode();

    /* attributes */
    DSNode *q_ptr;

    DopeSheet *dopeSheetModel;

    DopeSheet::ItemType nodeType;

    boost::shared_ptr<NodeGui> nodeGui;

    QTreeWidgetItem *nameItem;

    DSKnobRows knobRows;

    bool isSelected;
};

DSNodePrivate::DSNodePrivate(DSNode *qq) :
    q_ptr(qq),
    dopeSheetModel(0),
    nodeType(),
    nodeGui(),
    nameItem(0),
    knobRows(),
    isSelected(false)
{}

DSNodePrivate::~DSNodePrivate()
{}

void DSNodePrivate::createDSKnobs(KnobGui *knobGui, DSNode *dsNode)
{
    boost::shared_ptr<KnobI> knob = knobGui->getKnob();

    if (knob->getDimension() <= 1) {
        QTreeWidgetItem * nameItem = new QTreeWidgetItem(dsNode->getTreeItem(), DopeSheet::ItemTypeKnobDim);
        nameItem->setData(0, QTREEWIDGETITEM_DIM_ROLE, 0);
        nameItem->setText(0, knob->getDescription().c_str());
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);

        DSKnob *dsKnob = new DSKnob(0, nameItem, knobGui);
        knobRows.insert(TreeItemAndDSKnob(nameItem, dsKnob));
    }
    else {
        QTreeWidgetItem *multiDimRootItem = new QTreeWidgetItem(dsNode->getTreeItem(), DopeSheet::ItemTypeKnobRoot);
        multiDimRootItem->setData(0, QTREEWIDGETITEM_DIM_ROLE, -1);
        multiDimRootItem->setText(0, knob->getDescription().c_str());
        multiDimRootItem->setFlags(nameItem->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);

        DSKnob *rootDSKnob = new DSKnob(-1, multiDimRootItem, knobGui);
        knobRows.insert(TreeItemAndDSKnob(multiDimRootItem, rootDSKnob));

        for (int i = 0; i < knob->getDimension(); ++i) {
            QTreeWidgetItem *dimItem = new QTreeWidgetItem(multiDimRootItem, DopeSheet::ItemTypeKnobDim);
            dimItem->setData(0, QTREEWIDGETITEM_DIM_ROLE, i);
            dimItem->setText(0, knob->getDimensionName(i).c_str());
            dimItem->setFlags(nameItem->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);

            DSKnob *dimDSKnob = new DSKnob(i, dimItem, knobGui);
            knobRows.insert(TreeItemAndDSKnob(dimItem, dimDSKnob));
        }
    }

    QObject::connect(knobGui, SIGNAL(keyFrameSet()),
                     dopeSheetModel, SLOT(onKeyframeSetOrRemoved()));

    QObject::connect(knobGui, SIGNAL(keyFrameRemoved()),
                     dopeSheetModel, SLOT(onKeyframeSetOrRemoved()));
}

void DSNodePrivate::initGroupNode()
{
    NodeList subNodes = dynamic_cast<NodeGroup *>(nodeGui->getNode()->getLiveInstance())->getNodes();

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
    _imp(new DSNodePrivate(this))
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

        _imp->createDSKnobs(knobGui, this);
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
    for (DSKnobRows::iterator it = _imp->knobRows.begin();
         it != _imp->knobRows.end();
         ++it) {
        delete (*it).second;
    }

    delete _imp->nameItem;

    _imp->knobRows.clear();
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
    return _imp->nodeGui;
}

boost::shared_ptr<Natron::Node> DSNode::getInternalNode() const
{
    return _imp->nodeGui->getNode();
}

/**
 * @brief DSNode::getTreeItemsAndDSKnobs
 *
 *
 */
DSKnobRows DSNode::getChildData() const
{
    return _imp->knobRows;
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
        int childType = _imp->nameItem->child(i)->type();

        if (childType != DopeSheet::ItemTypeKnobDim
                && childType != DopeSheet::ItemTypeKnobRoot) {
            return true;
        }

    }
    return false;
}


////////////////////////// DopeSheetEditor //////////////////////////

class DopeSheetEditorPrivate
{
public:
    DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui);

    /* attributes */
    DopeSheetEditor *q_ptr;
    Gui *gui;

    QVBoxLayout *mainLayout;
    QHBoxLayout *helpersLayout;

    DopeSheet *model;

    QSplitter *splitter;
    HierarchyView *hierarchyView;
    DopeSheetView *dopeSheetView;
};

DopeSheetEditorPrivate::DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui)  :
    q_ptr(qq),
    gui(gui),
    mainLayout(0),
    helpersLayout(0),
    model(0),
    splitter(0),
    hierarchyView(0),
    dopeSheetView(0)
{}


/**
 * @class DopeSheetEditor
 *
 * The DopeSheetEditor class provides several widgets to edit keyframe animations in
 * a more user-friendly way than the Curve Editor.
 *
 * It contains two main widgets : at left, the hierarchy view provides a tree
 * representation of the animated parameters (knobs) of each opened node.
 * At right, the dope sheet view is an OpenGL widget displaying horizontally the
 * keyframes of these knobs. The user can select, move and delete the keyframes.
 */


/**
 * @brief DopeSheetEditor::DopeSheetEditor
 *
 * Creates a DopeSheetEditor.
 */
DopeSheetEditor::DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent) :
    QWidget(parent),
    ScriptObject(),
    _imp(new DopeSheetEditorPrivate(this, gui))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->helpersLayout = new QHBoxLayout();
    _imp->helpersLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton *toggleTripleSyncBtn = new QPushButton(tr("Sync"), this);
    toggleTripleSyncBtn->setToolTip(tr("Toggle triple synchronization"));
    toggleTripleSyncBtn->setCheckable(true);

    connect(toggleTripleSyncBtn, SIGNAL(toggled(bool)),
            this, SLOT(toggleTripleSync(bool)));

    _imp->helpersLayout->addWidget(toggleTripleSyncBtn);
    _imp->helpersLayout->addStretch();

    _imp->splitter = new QSplitter(Qt::Horizontal, this);
    _imp->splitter->setHandleWidth(1);

    _imp->model = new DopeSheet(gui, timeline);

    _imp->hierarchyView = new HierarchyView(_imp->model, gui, _imp->splitter);

    _imp->splitter->addWidget(_imp->hierarchyView);
    _imp->splitter->setStretchFactor(0, 1);

    _imp->dopeSheetView = new DopeSheetView(_imp->model, _imp->hierarchyView, gui, timeline, _imp->splitter);

    _imp->splitter->addWidget(_imp->dopeSheetView);
    _imp->splitter->setStretchFactor(1, 5);

    _imp->mainLayout->addWidget(_imp->splitter);
    _imp->mainLayout->addLayout(_imp->helpersLayout);
}

/**
 * @brief DopeSheetEditor::~DopeSheetEditor
 *
 * Deletes all the nodes from the DopeSheetEditor.
 */
DopeSheetEditor::~DopeSheetEditor()
{}

/**
 * @brief DopeSheetEditor::addNode
 *
 * Adds 'node' to the hierarchy view, except if :
 * - the node haven't an existing setting panel ;
 * - the node haven't knobs ;
 * - any knob of the node can't be animated or have no animation.
 */
void DopeSheetEditor::addNode(boost::shared_ptr<NodeGui> nodeGui)
{
    _imp->model->addNode(nodeGui);
}

/**
 * @brief DopeSheetEditor::removeNode
 *
 * Removes 'node' from the dope sheet.
 * Its associated items are removed from the hierarchy view as its keyframe rows.
 */
void DopeSheetEditor::removeNode(NodeGui *node)
{
    _imp->model->removeNode(node);
}

void DopeSheetEditor::centerOn(double xMin, double xMax)
{
    _imp->dopeSheetView->centerOn(xMin, xMax);
}

void DopeSheetEditor::toggleTripleSync(bool enabled)
{
    _imp->gui->setTripleSyncEnabled(enabled);
}
