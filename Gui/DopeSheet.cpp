#include "DopeSheet.h"

// Qt includes
#include <QDebug> //REMOVEME
#include <QHBoxLayout>
#include <QSplitter>
#include <QtEvents>
#include <QTreeWidget>

// Natron includes
#include "Gui/ActionShortcuts.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditorUndoRedo.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"

#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NoOp.h"

typedef std::map<boost::weak_ptr<KnobI>, KnobGui *> KnobsAndGuis;
typedef std::pair<QTreeWidgetItem *, DSNode *> TreeItemAndDSNode;
typedef std::pair<QTreeWidgetItem *, DSKnob *> TreeItemAndDSKnob;


////////////////////////// Helpers //////////////////////////

namespace {

/**
 * @brief nodeHasAnimation
 *
 * Returns true if 'node' contains at least one animated knob, otherwise
 * returns false.
 */
bool nodeHasAnimation(const boost::shared_ptr<NodeGui> &node)
{
    const std::vector<boost::shared_ptr<KnobI> > &knobs = node->getNode()->getKnobs();

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

bool nodeCanAnimate(const boost::shared_ptr<NodeGui> &node)
{
    const std::vector<boost::shared_ptr<KnobI> > &knobs = node->getNode()->getKnobs();

    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin();
         it != knobs.end();
         ++it) {
        boost::shared_ptr<KnobI> knob = *it;

        if (knob->canAnimate()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief groupHasAnimation
 *
 *
 */
bool groupHasAnimation(NodeGroup *nodeGroup)
{
    NodeList nodes = nodeGroup->getNodes();

    for (NodeList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        NodePtr n = (*it);

        if (nodeHasAnimation(boost::dynamic_pointer_cast<NodeGui>(n->getNodeGui()))) {
            return true;
        }
    }

    return false;
}

/**
 * @brief itemHasNoChildVisible
 *
 * Returns true if all childs of 'item' are hidden, otherwise returns
 * false.
 */
bool itemHasNoChildVisible(QTreeWidgetItem *item)
{
    for (int i = 0; i < item->childCount(); ++i) {
        if (!item->child(i)->isHidden())
            return false;
    }

    return true;
}

} // anon namespace


////////////////////////// DopeSheet //////////////////////////

class DopeSheetPrivate
{
public:
    DopeSheetPrivate(DopeSheet *qq);
    ~DopeSheetPrivate();

    /* functions */
    Natron::Node *getNearestRetimeFromOutputs_recursive(Natron::Node *node) const;
    void getInputsConnected_recursive(Natron::Node *node, std::vector<DSNode *> *result) const;

    /* attributes */
    DopeSheet *q_ptr;
    DSNodesRowsData nodesRows;
};

DopeSheetPrivate::DopeSheetPrivate(DopeSheet *qq) :
    q_ptr(qq),
    nodesRows()
{

}

DopeSheetPrivate::~DopeSheetPrivate()
{

}

Natron::Node *DopeSheetPrivate::getNearestRetimeFromOutputs_recursive(Natron::Node *node) const
{
    const std::list<Natron::Node *> &outputs = node->getOutputs();

    for (std::list<Natron::Node *>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        Natron::Node *output = (*it);

        if (output->getPluginLabel() == "RetimeOFX") {
            return output;
        }
        else {
            return getNearestRetimeFromOutputs_recursive(output);
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

            return;
        }
        else {
            getInputsConnected_recursive(input.get(), result);
        }
    }
}


DopeSheet::DopeSheet() :
    _imp(new DopeSheetPrivate(this))
{}

DopeSheet::~DopeSheet()
{
    for (DSNodesRowsData::iterator it = _imp->nodesRows.begin();
         it != _imp->nodesRows.end(); ++it) {
        delete (*it).second;
    }

    _imp->nodesRows.clear();
}

DSNodesRowsData DopeSheet::getTopLevelData() const
{
    return _imp->nodesRows;
}

std::pair<double, double> DopeSheet::getKeyframeRange() const
{
    std::pair<double, double> ret;

    std::vector<double> dimFirstKeys;
    std::vector<double> dimLastKeys;

    DSNodesRowsData dsNodeItems = _imp->nodesRows;

    for (DSNodesRowsData::const_iterator it = dsNodeItems.begin(); it != dsNodeItems.end(); ++it) {
        if ((*it).first->isHidden()) {
            continue;
        }

        DSNode *dsNode = (*it).second;

        DSKnobsRowsData dsKnobItems = dsNode->getChildData();

        for (DSKnobsRowsData::const_iterator itKnob = dsKnobItems.begin(); itKnob != dsKnobItems.end(); ++itKnob) {
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
    nodeGui->ensurePanelCreated();

    // Don't show the group nodes' input & output
    if (dynamic_cast<GroupInput *>(nodeGui->getNode()->getLiveInstance()) ||
            dynamic_cast<GroupOutput *>(nodeGui->getNode()->getLiveInstance())) {
        return;
    }

    if (nodeGui->getNode()->getKnobs().empty()) {
        return;
    }

    if (!nodeCanAnimate(nodeGui)) {
        return;
    }

    DSNode *dsNode = createDSNode(nodeGui);

    _imp->nodesRows.insert(TreeItemAndDSNode(dsNode->getTreeItem(), dsNode));

    Q_EMIT dsNodeCreated(dsNode);

    if (DSNode *parentGroupDSNode = getGroupDSNode(dsNode)) {
        parentGroupDSNode->computeGroupRange();
    }
}

void DopeSheet::removeNode(NodeGui *node)
{
    DSNodesRowsData::iterator toRemove = _imp->nodesRows.end();

    for (DSNodesRowsData::iterator it = _imp->nodesRows.begin();
         it != _imp->nodesRows.end();
         ++it)
    {
        if ((*it).second->getNodeGui().get() == node) {
            toRemove = it;

            break;
        }
    }

    if (toRemove == _imp->nodesRows.end()) {
        return;
    }

    DSNode *dsNode = (*toRemove).second;

    if (DSNode *parentGroupDSNode = getGroupDSNode(dsNode)) {
        parentGroupDSNode->computeGroupRange();
    }

    Q_EMIT nodeAboutToBeRemoved(dsNode);

    _imp->nodesRows.erase(toRemove);

    delete (dsNode);
}

DSNode *DopeSheet::findParentDSNode(QTreeWidgetItem *treeItem) const
{
    DSNodesRowsData::const_iterator clickedDSNode;
    QTreeWidgetItem *itemIt = treeItem;

    while ( (clickedDSNode = _imp->nodesRows.find(itemIt)) == _imp->nodesRows.end() ) {
        if (itemIt->parent()) {
            itemIt = itemIt->parent();
        }
    }

    return (*clickedDSNode).second;
}

DSNode *DopeSheet::findDSNode(QTreeWidgetItem *nodeTreeItem) const
{
    DSNodesRowsData::const_iterator dsNodeIt = _imp->nodesRows.find(nodeTreeItem);

    if (dsNodeIt != _imp->nodesRows.end()) {
        return (*dsNodeIt).second;
    }

    return NULL;
}

DSNode *DopeSheet::findDSNode(Natron::Node *node) const
{
    for (DSNodesRowsData::const_iterator it = _imp->nodesRows.begin();
         it != _imp->nodesRows.end();
         ++it) {
        DSNode *dsNode = (*it).second;

        if (dsNode->getNodeGui()->getNode().get() == node) {
            return dsNode;
        }
    }

    return 0;
}

DSKnob *DopeSheet::findDSKnob(QTreeWidgetItem *knobTreeItem, int *dimension) const
{
    assert(dimension);

    DSKnob *ret = 0;

    DSNode *dsNode = findParentDSNode(knobTreeItem);

    DSKnobsRowsData knobsRows = dsNode->getChildData();

    DSKnobsRowsData::const_iterator clickedDSKnob;
    QTreeWidgetItem *itemIt = knobTreeItem;

    while ( (clickedDSKnob = knobsRows.find(itemIt)) == knobsRows.end()) {
        if (itemIt->parent()) {
            itemIt = itemIt->parent();
        }
    }

    ret = clickedDSKnob->second;

    if (ret->isMultiDim()) {
        if (itemIt != knobTreeItem) {
            *dimension = knobTreeItem->indexOfChild(knobTreeItem);
        }
        else {
            *dimension = -1;
        }
    }
    else {
        *dimension = 0;
    }

    return ret;
}

DSNode *DopeSheet::getGroupDSNode(DSNode *dsNode) const
{
    boost::shared_ptr<NodeGroup> parentGroup = boost::dynamic_pointer_cast<NodeGroup>(dsNode->getNodeGui()->getNode()->getGroup());

    DSNode *parentGroupDSNode = 0;

    if (parentGroup) {
        parentGroupDSNode = findDSNode(parentGroup->getNode().get());
    }

    return parentGroupDSNode;
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
DSNode *DopeSheet::getNearestRetimeFromOutputs(DSNode *dsNode) const
{
    if (dsNode->getDSNodeType() == DSNode::RetimeNodeType) {
        return NULL;
    }

    Natron::Node *retimer = _imp->getNearestRetimeFromOutputs_recursive(dsNode->getNodeGui()->getNode().get());

    return findDSNode(retimer);
}

std::vector<DSNode *> DopeSheet::getInputsConnected(DSNode *dsNode) const
{
    std::vector<DSNode *> ret;

    _imp->getInputsConnected_recursive(dsNode->getNodeGui()->getNode().get(), &ret);

    return ret;
}

void DopeSheet::onNodeNameChanged(const QString &name)
{
    Natron::Node *node = qobject_cast<Natron::Node *>(sender());
    DSNode *dsNode = findDSNode(node);

    dsNode->getTreeItem()->setText(0, name);
}

DSNode *DopeSheet::createDSNode(const boost::shared_ptr<NodeGui> &nodeGui)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    DSNode::DSNodeType nodeType = DSNode::CommonNodeType;

    NodePtr node = nodeGui->getNode();

    if (node->getPlugin()->isReader()) {
        nodeType = DSNode::ReaderNodeType;
    }
    else if (dynamic_cast<NodeGroup *>(node->getLiveInstance())) {
        nodeType = DSNode::GroupNodeType;
    }
    else if (node->getPluginLabel() == "RetimeOFX") {
        nodeType = DSNode::RetimeNodeType;
    }

    QTreeWidgetItem *nameItem = new QTreeWidgetItem(nodeType);
    nameItem->setText(0, node->getLabel().c_str());

    DSNode *dsNode = new DSNode(this, nodeType, nameItem, nodeGui);

    if (nodeType != DSNode::CommonNodeType) {
        connect(dsNode, SIGNAL(clipRangeChanged()),
                this, SIGNAL(modelChanged()));
    }

    connect(node.get(), SIGNAL(labelChanged(QString)),
            this, SLOT(onNodeNameChanged(QString)));

    return dsNode;
}

DSKnob *DopeSheet::createDSKnob(KnobGui *knobGui, DSNode *dsNode)
{
    DSKnob *dsKnob = 0;

    boost::shared_ptr<KnobI> knob = knobGui->getKnob();

    if (knob->getDimension() <= 1) {
        QTreeWidgetItem * nameItem = new QTreeWidgetItem(dsNode->getTreeItem());
        nameItem->setText(0, knob->getDescription().c_str());

        dsKnob = new DSKnob(nameItem, knobGui);
    }
    else {
        QTreeWidgetItem *multiDimRootNameItem = new QTreeWidgetItem(dsNode->getTreeItem());
        multiDimRootNameItem->setText(0, knob->getDescription().c_str());

        for (int i = 0; i < knob->getDimension(); ++i) {
            QTreeWidgetItem *dimItem = new QTreeWidgetItem(multiDimRootNameItem);
            dimItem->setText(0, knob->getDimensionName(i).c_str());
        }

        dsKnob = new DSKnob(multiDimRootNameItem, knobGui);
    }

    connect(knobGui, SIGNAL(keyFrameSet()),
            dsKnob, SLOT(checkVisibleState()));

    connect(knobGui, SIGNAL(keyFrameRemoved()),
            dsKnob, SLOT(checkVisibleState()));

    connect(knobGui, SIGNAL(keyFrameSet()),
            this, SIGNAL(modelChanged()));

    connect(knobGui, SIGNAL(keyFrameRemoved()),
            this, SIGNAL(modelChanged()));

    connect(dsKnob, SIGNAL(needNodesVisibleStateChecking()),
            dsNode, SLOT(checkVisibleState()));

    return dsKnob;
}

void DopeSheet::refreshClipRects()
{
    for (DSNodesRowsData::const_iterator it = _imp->nodesRows.begin();
         it != _imp->nodesRows.end();
         ++it) {
        DSNode *dsNode = (*it).second;
        if (dsNode->getDSNodeType() == DSNode::ReaderNodeType) {
            dsNode->computeReaderRange();
        }
        else if (dsNode->getDSNodeType() == DSNode::GroupNodeType) {
            dsNode->computeGroupRange();
        }
    }
}


////////////////////////// DSKnob //////////////////////////

class DSKnobPrivate
{
public:
    DSKnobPrivate();
    ~DSKnobPrivate();

    /* attributes */
    QTreeWidgetItem *nameItem;
    KnobGui *knobGui;
};

DSKnobPrivate::DSKnobPrivate() :
    nameItem(0),
    knobGui(0)
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
DSKnob::DSKnob(QTreeWidgetItem *nameItem,
               KnobGui *knobGui) :
    QObject(),
    _imp(new DSKnobPrivate)
{
    _imp->nameItem = nameItem;
    _imp->knobGui = knobGui;

    checkVisibleState();
}

DSKnob::~DSKnob()
{}

QTreeWidgetItem *DSKnob::getTreeItem() const
{
    return _imp->nameItem;
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

/**
 * @brief DSKnob::isMultiDimRoot
 *
 *
 */
bool DSKnob::isMultiDim() const
{
    return (_imp->knobGui->getKnob()->getDimension() > 1);
}

/**
 * @brief DSKnob::checkVisibleState
 *
 * Handles the visibility of the item and its parent(s).
 *
 * This slot is automatically called each time a keyframe is
 * set or removed for this knob.
 */
void DSKnob::checkVisibleState()
{
    QTreeWidgetItem *nodeItem = _imp->nameItem->parent();

    if (isMultiDim()) {
        for (int i = 0; i < _imp->knobGui->getKnob()->getDimension(); ++i) {
            if (_imp->knobGui->getCurve(i)->isAnimated()) {
                if(_imp->nameItem->child(i)->isHidden()) {
                    _imp->nameItem->child(i)->setHidden(false);
                }
            }
            else {
                if (!_imp->nameItem->child(i)->isHidden()) {
                    _imp->nameItem->child(i)->setHidden(true);
                }
            }
        }

        if (itemHasNoChildVisible(_imp->nameItem)) {
            _imp->nameItem->setHidden(true);
        }
        else {
            _imp->nameItem->setHidden(false);
        }
    }
    else {
        if (_imp->knobGui->getCurve(0)->isAnimated()) {
            _imp->nameItem->setHidden(false);
        }
        else {
            _imp->nameItem->setHidden(true);
        }
    }

    if (itemHasNoChildVisible(nodeItem)) {
        nodeItem->setHidden(true);
    }
    else if (nodeItem->isHidden()) {
        nodeItem->setHidden(false);
    }

    Q_EMIT needNodesVisibleStateChecking();
}


////////////////////////// DSNode //////////////////////////

class DSNodePrivate
{
public:
    DSNodePrivate(DSNode *qq);
    ~DSNodePrivate();

    /* functions */
    void createDSKnobs();

    void initReaderNode();
    void initGroupNode();

    /* attributes */
    DSNode *q_ptr;

    DopeSheet *dopeSheetModel;

    DSNode::DSNodeType nodeType;

    boost::shared_ptr<NodeGui> nodeGui;

    QTreeWidgetItem *nameItem;

    DSKnobsRowsData knobsRows;

    std::pair<double, double> clipRange;

    bool isSelected;
};

DSNodePrivate::DSNodePrivate(DSNode *qq) :
    q_ptr(qq),
    dopeSheetModel(0),
    nodeType(),
    nodeGui(),
    nameItem(0),
    knobsRows(),
    isSelected(false)
{}

DSNodePrivate::~DSNodePrivate()
{}

void DSNodePrivate::createDSKnobs()
{
    const KnobsAndGuis &knobs = nodeGui->getKnobs();

    if (DSNode *parentGroupDSNode = dopeSheetModel->getGroupDSNode(q_ptr)) {
        QObject::connect(nodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
                         parentGroupDSNode, SLOT(checkVisibleState()));

        QObject::connect(nodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
                         parentGroupDSNode, SLOT(computeGroupRange()));
    }

    for (KnobsAndGuis::const_iterator it = knobs.begin();
         it != knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first.lock();
        KnobGui *knobGui = it->second;

        if (!knob->canAnimate() || !knob->isAnimationEnabled()) {
            continue;
        }

        if (DSNode *parentGroupDSNode = dopeSheetModel->getGroupDSNode(q_ptr)) {
            QObject::connect(knob->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                             parentGroupDSNode, SLOT(computeGroupRange()));

            QObject::connect(knobGui, SIGNAL(keyFrameSet()),
                             parentGroupDSNode, SLOT(computeGroupRange()));

            QObject::connect(knobGui, SIGNAL(keyFrameRemoved()),
                             parentGroupDSNode, SLOT(computeGroupRange()));
        }

        DSKnob *dsKnob = dopeSheetModel->createDSKnob(knobGui, q_ptr);

        knobsRows.insert(TreeItemAndDSKnob(dsKnob->getTreeItem(), dsKnob));
    }

    if (DSNode *parentGroupDSNode = dopeSheetModel->getGroupDSNode(q_ptr)) {
        parentGroupDSNode->computeGroupRange();
    }
}

void DSNodePrivate::initReaderNode()
{
    NodePtr node = nodeGui->getNode();
    // The dopesheet view must refresh if the user set some values in the settings panel
    // so we connect some signals/slots
    boost::shared_ptr<KnobSignalSlotHandler> firstFrameKnob = node->getKnobByName("firstFrame")->getSignalSlotHandler();
    boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnob =  node->getKnobByName("lastFrame")->getSignalSlotHandler();
    boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = node->getKnobByName("startingTime")->getSignalSlotHandler();

    QObject::connect(firstFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                     q_ptr, SLOT(computeReaderRange()));

    QObject::connect(lastFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                     q_ptr, SLOT(computeReaderRange()));

    QObject::connect(startingTimeKnob.get(), SIGNAL(valueChanged(int, int)),
                     q_ptr, SLOT(computeReaderRange()));

    q_ptr->computeReaderRange();
}

void DSNodePrivate::initGroupNode()
{
    NodeList subNodes = dynamic_cast<NodeGroup *>(nodeGui->getNode()->getLiveInstance())->getNodes();

    for (NodeList::const_iterator it = subNodes.begin(); it != subNodes.end(); ++it) {
        NodePtr subNode = (*it);
        boost::shared_ptr<NodeGui> subNodeGui = boost::dynamic_pointer_cast<NodeGui>(subNode->getNodeGui());

        if (!subNodeGui->getSettingPanel() || !subNodeGui->getSettingPanel()->isVisible()) {
            continue;
        }

        QObject::connect(subNodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
                         q_ptr, SLOT(checkVisibleState()));

        QObject::connect(subNodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
                         q_ptr, SLOT(computeGroupRange()));

        const KnobsAndGuis &knobs = subNodeGui->getKnobs();

        for (KnobsAndGuis::const_iterator knobIt = knobs.begin();
             knobIt != knobs.end(); ++knobIt) {
            boost::shared_ptr<KnobI> knob = knobIt->first.lock();
            KnobGui *knobGui = knobIt->second;

            QObject::connect(knob->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                             q_ptr, SLOT(computeGroupRange()));

            QObject::connect(knobGui, SIGNAL(keyFrameSet()),
                             q_ptr, SLOT(computeGroupRange()));

            QObject::connect(knobGui, SIGNAL(keyFrameRemoved()),
                             q_ptr, SLOT(computeGroupRange()));
        }
    }

    q_ptr->computeGroupRange();
}

DSNode::DSNode(DopeSheet *model,
               DSNodeType nodeType,
               QTreeWidgetItem *nameItem,
               const boost::shared_ptr<NodeGui> &nodeGui) :
    QObject(),
    _imp(new DSNodePrivate(this))
{
    _imp->dopeSheetModel = model;
    _imp->nodeType = nodeType;
    _imp->nameItem = nameItem;
    _imp->nodeGui = nodeGui;

    connect(nodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
            model, SIGNAL(modelChanged()));

    // Create the hierarchy
    if (_imp->nodeType == DSNode::CommonNodeType) {
        _imp->createDSKnobs();
    }
    else if (_imp->nodeType == DSNode::ReaderNodeType) {
        _imp->initReaderNode();
        _imp->createDSKnobs();
    }
    else if (_imp->nodeType == DSNode::RetimeNodeType) {
        _imp->createDSKnobs();
    }
    // If some subnodes are already in the dope sheet, the connections must be set to update
    // the group's clip rect
    else if (_imp->nodeType == DSNode::GroupNodeType) {
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
    for (DSKnobsRowsData::iterator it = _imp->knobsRows.begin();
         it != _imp->knobsRows.end();
         ++it) {
        delete (*it).second;
    }

    delete _imp->nameItem;

    _imp->knobsRows.clear();
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

/**
 * @brief DSNode::getTreeItemsAndDSKnobs
 *
 *
 */
DSKnobsRowsData DSNode::getChildData() const
{
    return _imp->knobsRows;
}

DSNode::DSNodeType DSNode::getDSNodeType() const
{
    return _imp->nodeType;
}

std::pair<double, double> DSNode::getClipRange() const
{
    return _imp->clipRange;
}

/**
 * @brief DSNode::checkVisibleState
 *
 * If the item and its set of keyframes must be hidden or not,
 * hides or shows them.
 *
 * This slot is automatically called when
 */
void DSNode::checkVisibleState()
{
    _imp->nodeGui->setVisibleSettingsPanel(true);

    bool showItem = _imp->nodeGui->isSettingsPanelVisible();

    if (_imp->nodeType == DSNode::CommonNodeType) {
        showItem = nodeHasAnimation(_imp->nodeGui);
    }
    else if (_imp->nodeType == DSNode::GroupNodeType) {
        NodeGroup *group = dynamic_cast<NodeGroup *>(_imp->nodeGui->getNode()->getLiveInstance());

        showItem = showItem && !_imp->dopeSheetModel->groupSubNodesAreHidden(group);
    }

    _imp->nameItem->setHidden(!showItem);

    // Hide the parent group item if there's no subnodes displayed
    if (DSNode *parentGroupDSNode = _imp->dopeSheetModel->getGroupDSNode(this)) {
        parentGroupDSNode->checkVisibleState();
    }
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

void DSNode::computeReaderRange()
{
    assert(_imp->nodeType == DSNode::ReaderNodeType);

    NodePtr node = _imp->nodeGui->getNode();

    int startingTime = dynamic_cast<Knob<int> *>(node->getKnobByName("startingTime").get())->getValue();
    int firstFrame = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get())->getValue();
    int lastFrame = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get())->getValue();

    _imp->clipRange.first = startingTime;
    _imp->clipRange.second = (startingTime + (lastFrame - firstFrame));

    Q_EMIT clipRangeChanged();
}

void DSNode::computeGroupRange()
{
    assert(_imp->nodeType == DSNode::GroupNodeType);

    std::vector<double> dimFirstKeys;
    std::vector<double> dimLastKeys;

    NodeList nodes = dynamic_cast<NodeGroup *>(_imp->nodeGui->getNode()->getLiveInstance())->getNodes();

    for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr node = (*it);

        boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());


        if (!nodeGui->getSettingPanel() || !nodeGui->getSettingPanel()->isVisible()) {
            continue;
        }

        const std::vector<boost::shared_ptr<KnobI> > &knobs = node->getKnobs();

        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin();
             it != knobs.end();
             ++it) {
            boost::shared_ptr<KnobI> knob = (*it);

            if (!knob->canAnimate() || !knob->hasAnimation()) {
                continue;
            }
            else {
                for (int i = 0; i < knob->getDimension(); ++i) {
                    KeyFrameSet keyframes = knob->getCurve(i)->getKeyFrames_mt_safe();

                    if (keyframes.empty()) {
                        continue;
                    }

                    dimFirstKeys.push_back(keyframes.begin()->getTime());
                    dimLastKeys.push_back(keyframes.rbegin()->getTime());
                }
            }
        }
    }

    if (dimFirstKeys.empty() || dimLastKeys.empty()) {
        _imp->clipRange.first = 0;
        _imp->clipRange.second = 0;
    }
    else {
        _imp->clipRange.first = *std::min_element(dimFirstKeys.begin(), dimFirstKeys.end());
        _imp->clipRange.second = *std::max_element(dimLastKeys.begin(), dimLastKeys.end());
    }

    Q_EMIT clipRangeChanged();
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

    DopeSheet *model;

    QSplitter *splitter;
    HierarchyView *hierarchyView;
    DopeSheetView *dopeSheetView;
};

DopeSheetEditorPrivate::DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui)  :
    q_ptr(qq),
    gui(gui),
    mainLayout(0),
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

    _imp->splitter = new QSplitter(Qt::Horizontal, this);

    _imp->model = new DopeSheet;

    _imp->hierarchyView = new HierarchyView(_imp->model, gui, _imp->splitter);

    _imp->splitter->addWidget(_imp->hierarchyView);
    _imp->splitter->setStretchFactor(0, 1);

    _imp->dopeSheetView = new DopeSheetView(_imp->model, _imp->hierarchyView, gui, timeline, _imp->splitter);

    _imp->splitter->addWidget(_imp->dopeSheetView);
    _imp->splitter->setStretchFactor(1, 5);

    _imp->mainLayout->addWidget(_imp->splitter);
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
