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
    Natron::Node *getNearestTimeFromOutputs_recursive(Natron::Node *node) const;
    void getInputsConnected_recursive(Natron::Node *node, std::vector<DSNode *> *result) const;

    /* attributes */
    DopeSheet *q_ptr;
    DSNodeRows nodeRows;
};

DopeSheetPrivate::DopeSheetPrivate(DopeSheet *qq) :
    q_ptr(qq),
    nodeRows()
{

}

DopeSheetPrivate::~DopeSheetPrivate()
{

}

Natron::Node *DopeSheetPrivate::getNearestTimeFromOutputs_recursive(Natron::Node *node) const
{
    const std::list<Natron::Node *> &outputs = node->getOutputs();

    for (std::list<Natron::Node *>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        Natron::Node *output = (*it);

        if (output->getPluginLabel() == "RetimeOFX"
                || output->getPluginLabel() == "TimeOffsetOFX"
                || output->getPluginLabel() == "FrameRangeOFX") {
            return output;
        }
        else {
            return getNearestTimeFromOutputs_recursive(output);
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


DopeSheet::DopeSheet() :
    _imp(new DopeSheetPrivate(this))
{}

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

        DSKnobRow dsKnobItems = dsNode->getChildData();

        for (DSKnobRow::const_iterator itKnob = dsKnobItems.begin(); itKnob != dsKnobItems.end(); ++itKnob) {
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

    if (nodeGui->getNode()->isRotoNode() || nodeGui->getNode()->isRotoPaintingNode()) {
        return;
    }

    if (nodeGui->getNode()->getKnobs().empty()) {
        return;
    }

    if (!nodeCanAnimate(nodeGui)) {
        return;
    }

    DSNode *dsNode = createDSNode(nodeGui);

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

    Q_EMIT nodeAboutToBeRemoved(dsNode);

    _imp->nodeRows.erase(toRemove);

    delete (dsNode);
}

DSNode *DopeSheet::findParentDSNode(QTreeWidgetItem *treeItem) const
{
    DSNodeRows::const_iterator clickedDSNode;
    QTreeWidgetItem *itemIt = treeItem;

    while ( (clickedDSNode = _imp->nodeRows.find(itemIt)) == _imp->nodeRows.end() ) {
        if (itemIt->parent()) {
            itemIt = itemIt->parent();
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

        if (dsNode->getNodeGui()->getNode().get() == node) {
            return dsNode;
        }
    }

    return 0;
}

DSNode *DopeSheet::findDSNode(const boost::shared_ptr<KnobI> knob) const
{
    for (DSNodeRows::const_iterator it = _imp->nodeRows.begin(); it != _imp->nodeRows.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSKnobRow knobRows = dsNode->getChildData();

        for (DSKnobRow::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            DSKnob *dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui()->getKnob() == knob) {
                return dsNode;
            }
        }
    }

    return NULL;
}

DSKnob *DopeSheet::findDSKnob(QTreeWidgetItem *knobTreeItem, int *dimension) const
{
    assert(dimension);

    DSKnob *ret = 0;

    DSNode *dsNode = findParentDSNode(knobTreeItem);

    DSKnobRow knobRows = dsNode->getChildData();

    DSKnobRow::const_iterator clickedDSKnob;
    QTreeWidgetItem *itemIt = knobTreeItem;

    while ( (clickedDSKnob = knobRows.find(itemIt)) == knobRows.end()) {
        if (itemIt->parent()) {
            itemIt = itemIt->parent();
        }
    }

    ret = clickedDSKnob->second;

    if (ret->isMultiDim()) {
        if (itemIt != knobTreeItem) {
            *dimension = itemIt->indexOfChild(knobTreeItem);
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

DSKnob *DopeSheet::findDSKnob(KnobGui *knobGui) const
{
    for (DSNodeRows::const_iterator it = _imp->nodeRows.begin(); it != _imp->nodeRows.end(); ++it) {
        DSNode *dsNode = (*it).second;

        DSKnobRow knobRows = dsNode->getChildData();

        for (DSKnobRow::const_iterator knobIt = knobRows.begin(); knobIt != knobRows.end(); ++knobIt) {
            DSKnob *dsKnob = (*knobIt).second;

            if (dsKnob->getKnobGui() == knobGui) {
                return dsKnob;
            }
        }
    }

    return NULL;
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
DSNode *DopeSheet::getNearestTimeNodeFromOutputs(DSNode *dsNode) const
{
    if (dsNode->getDSNodeType() == DSNode::RetimeNodeType
            || dsNode->getDSNodeType() == DSNode::TimeOffsetNodeType
            || dsNode->getDSNodeType() == DSNode::FrameRangeNodeType) {
        return NULL;
    }

    Natron::Node *timeNode = _imp->getNearestTimeFromOutputs_recursive(dsNode->getNodeGui()->getNode().get());

    return findDSNode(timeNode);
}

std::vector<DSNode *> DopeSheet::getInputsConnected(DSNode *dsNode) const
{
    std::vector<DSNode *> ret;

    _imp->getInputsConnected_recursive(dsNode->getNodeGui()->getNode().get(), &ret);

    return ret;
}

bool DopeSheet::nodeHasAnimation(const boost::shared_ptr<NodeGui> &nodeGui) const
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
    else if (node->getPluginLabel() == "TimeOffsetOFX") {
        nodeType = DSNode::TimeOffsetNodeType;
    }
    else if (node->getPluginLabel() == "FrameRangeOFX") {
        nodeType = DSNode::FrameRangeNodeType;
    }

    QTreeWidgetItem *nameItem = new QTreeWidgetItem(nodeType);
    nameItem->setText(0, node->getLabel().c_str());

    DSNode *dsNode = new DSNode(this, nodeType, nameItem, nodeGui);

    connect(nodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
            this, SLOT(onSettingsPanelCloseChanged(bool)));

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
            this, SLOT(onKeyframeSetOrRemoved()));

    connect(knobGui, SIGNAL(keyFrameRemoved()),
            this, SLOT(onKeyframeSetOrRemoved()));

    return dsKnob;
}

void DopeSheet::onSettingsPanelCloseChanged(bool closed)
{
    Q_UNUSED(closed);

    DSNode *dsNode = findDSNode(qobject_cast<Natron::Node *>(sender()));

    if (!dsNode) {
        return;
    }

    if (DSNode *parentGroupDSNode = getGroupDSNode(dsNode)) {
        Q_EMIT groupNodeSettingsPanelCloseChanged(parentGroupDSNode);
    }
    else {
        Q_EMIT nodeSettingsPanelOpened(dsNode);
    }
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

////////////////////////// DSNode //////////////////////////

class DSNodePrivate
{
public:
    DSNodePrivate(DSNode *qq);
    ~DSNodePrivate();

    /* functions */
    void createDSKnobs();

    void initGroupNode();

    /* attributes */
    DSNode *q_ptr;

    DopeSheet *dopeSheetModel;

    DSNode::DSNodeType nodeType;

    boost::shared_ptr<NodeGui> nodeGui;

    QTreeWidgetItem *nameItem;

    DSKnobRow knobRows;

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

void DSNodePrivate::createDSKnobs()
{
    const KnobsAndGuis &knobs = nodeGui->getKnobs();

    for (KnobsAndGuis::const_iterator it = knobs.begin();
         it != knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first.lock();
        KnobGui *knobGui = it->second;

        if (!knob->canAnimate() || !knob->isAnimationEnabled()) {
            continue;
        }

        DSKnob *dsKnob = dopeSheetModel->createDSKnob(knobGui, q_ptr);

        knobRows.insert(TreeItemAndDSKnob(dsKnob->getTreeItem(), dsKnob));
    }
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
                         dopeSheetModel, SLOT(onSettingsPanelCloseChanged(bool)));
    }
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

    _imp->createDSKnobs();

    // If some subnodes are already in the dope sheet, the connections must be set to update
    // the group's clip rect
    if (_imp->nodeType == DSNode::GroupNodeType) {
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
    for (DSKnobRow::iterator it = _imp->knobRows.begin();
         it != _imp->knobRows.end();
         ++it) {
        delete (*it).second;
    }

    delete _imp->nameItem;

    _imp->knobRows.clear();
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
DSKnobRow DSNode::getChildData() const
{
    return _imp->knobRows;
}

DSNode::DSNodeType DSNode::getDSNodeType() const
{
    return _imp->nodeType;
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
