#include "DopeSheet.h"

#include <QDebug> //REMOVEME
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemDelegate>
#include <QSplitter>
#include <QTreeWidget>

#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"

#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NoOp.h"

typedef std::map<boost::shared_ptr<KnobI>, KnobGui *> KnobsAndGuis;


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

        if (nodeHasAnimation(boost::dynamic_pointer_cast<NodeGui>(n))) {
            return true;
        }
    }

    return false;
}

/**
 * @brief noChildIsVisible
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

/**
 * @brief recursiveSelect
 *
 * Performs a recursive selection on 'item' 's childs.
 */
void recursiveSelect(QTreeWidgetItem *item)
{
    if (item->childCount() > 0 && !itemHasNoChildVisible(item)) {
        for (int i = 0; i < item->childCount(); ++i) {
            QTreeWidgetItem *childItem = item->child(i);
            childItem->setSelected(true);

            // /!\ recursion
            recursiveSelect(childItem);
        }
    }
}

} // anon namespace

////////////////////////// DSKnob //////////////////////////

class DSKnobPrivate
{
public:
    DSKnobPrivate();
    ~DSKnobPrivate();

    /* functions */
    bool isMultiDimRoot() const;

    /* attributes */
    DopeSheetEditor *dopeSheetEditor;
    QTreeWidgetItem *nameItem;
    boost::shared_ptr<KnobI> knob;
    KnobGui *knobGui;
    int dimension;
};

DSKnobPrivate::DSKnobPrivate() :
    nameItem(0),
    knob(),
    knobGui(0),
    dimension()
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
DSKnob::DSKnob(DopeSheetEditor *dopeSheetEditor,
               QTreeWidgetItem *nameItem,
               KnobGui *knobGui,
               int dimension) :
    QObject(),
    _imp(new DSKnobPrivate)
{
    _imp->dopeSheetEditor = dopeSheetEditor;
    _imp->nameItem = nameItem;
    _imp->knobGui = knobGui;
    _imp->dimension = dimension;

    connect(knobGui, SIGNAL(keyFrameSet()),
            this, SLOT(checkVisibleState()));

    connect(knobGui, SIGNAL(keyFrameRemoved()),
            this, SLOT(checkVisibleState()));

    checkVisibleState();
}

DSKnob::~DSKnob()
{}

QRectF DSKnob::getNameItemRect() const
{
    return _imp->dopeSheetEditor->getNameItemRect(_imp->nameItem);
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
bool DSKnob::isMultiDimRoot() const
{
    return (_imp->nameItem->childCount() > 0);
}

bool DSKnob::isExpanded() const
{
    return (_imp->nameItem->isExpanded());
}

bool DSKnob::isHidden() const
{
    return (_imp->nameItem->isHidden());
}

bool DSKnob::parentIsCollapsed() const
{
    assert(_imp->nameItem->parent());

    return !(_imp->nameItem->parent()->isExpanded());
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
    QTreeWidgetItem *parentInHierarchyView = _imp->nameItem->parent();

    if (isMultiDimRoot()) {
        if (itemHasNoChildVisible(_imp->nameItem)) {
            _imp->nameItem->setHidden(true);
        }
        else {
            _imp->nameItem->setHidden(false);
        }
    }
    else {
        if (!_imp->knobGui->getKnob()->isAnimated(_imp->dimension)) {
            _imp->nameItem->setHidden(true);

            // Hide the multidim root if it's "empty"
            if (itemHasNoChildVisible(parentInHierarchyView))
                parentInHierarchyView->setHidden(true);
        }
        else {
            _imp->nameItem->setHidden(false);
            parentInHierarchyView->setHidden(false);

            // Show the node item if necessary
            if (parentInHierarchyView->parent()) {
                parentInHierarchyView->parent()->setHidden(false);
            }
        }
    }

    Q_EMIT needNodesVisibleStateChecking();
}

QTreeWidgetItem *DSKnob::getNameItem() const
{
    return _imp->nameItem;
}


////////////////////////// DSNode //////////////////////////

class DSNodePrivate
{
public:
    DSNodePrivate();
    ~DSNodePrivate();

    /* attributes */
    DopeSheetEditor *dopeSheetEditor;

    boost::shared_ptr<NodeGui> nodeGui;

    QTreeWidgetItem *nameItem;

    DSNode::DSNodeType nodeType;

    DSKnobList dsKnobs;
    DSNodeList dsNodes;
};

DSNodePrivate::DSNodePrivate() :
    nodeGui(),
    nameItem(0),
    nodeType(DSNode::CommonNodeType),
    dsKnobs()
{}

DSNodePrivate::~DSNodePrivate()
{}


/**
 * @class DSNode
 *
 * The DSNode class describes a node and it's animated knobs in
 * the DopeSheet.
 *
 * A DSNode contains a list of DSKnob.
 */

/**
* @brief DSNode::DSNode
*
* Constructs a DSNode.
* A hierarchy view item is created in 'hierarchyView'.
*
* 'node' is used to fill this item with all its knobs. If 'node' contains
* no animated knob, the item is hidden in the hierarchy view and the keyframe
* set is hidden in the dope sheet view.
*
* Note that 'item' is already created when this ctor is called.
* /!\ We should improve the classes design.
*/
DSNode::DSNode(DopeSheetEditor *dopeSheetEditor,
               const boost::shared_ptr<NodeGui> &nodeGui) :
    QObject(),
    _imp(new DSNodePrivate)
{
    _imp->dopeSheetEditor = dopeSheetEditor;

    _imp->nodeGui = nodeGui;

    connect(nodeGui->getNode().get(), SIGNAL(labelChanged(QString)),
            this, SLOT(onNodeNameChanged(QString)));

    connect(nodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
            dopeSheetEditor, SLOT(refreshDopeSheetView()));

    // Create the name item
    _imp->nameItem = new QTreeWidgetItem(dopeSheetEditor->getHierarchyView());
    _imp->nameItem->setText(0, nodeGui->getNode()->getLabel().c_str());
    _imp->nameItem->setExpanded(true);

    // Determinate the node type and create the hierarchy
    // If it's a Read node
    if (nodeGui->getNode()->getPlugin()->isReader()) {
        _imp->nodeType = DSNode::ReaderNodeType;

        // The dopesheet view must refresh if the user set some values in the settings panel
        // so we connect some signals/slots
        // TODO disconnect them
        boost::shared_ptr<Natron::Node> nodePtr = nodeGui->getNode();

        boost::shared_ptr<KnobSignalSlotHandler> firstFrameKnob = nodePtr->getKnobByName("firstFrame")->getSignalSlotHandler();
        boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnob =  nodePtr->getKnobByName("lastFrame")->getSignalSlotHandler();
        boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = nodePtr->getKnobByName("startingTime")->getSignalSlotHandler();

        connect(firstFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                dopeSheetEditor, SLOT(refreshDopeSheetView()));

        connect(lastFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                dopeSheetEditor, SLOT(refreshDopeSheetView()));

        connect(startingTimeKnob.get(), SIGNAL(valueChanged(int, int)),
                dopeSheetEditor, SLOT(refreshDopeSheetView()));
    }

    //     If it's a Group node
    NodeGroup *isGroup = dynamic_cast<NodeGroup *>(nodeGui->getNode()->getLiveInstance());
    if (isGroup) {
        _imp->nodeType = DSNode::GroupNodeType;
    }

    // Useful for increase the size of Reader and Group items
    _imp->nameItem->setData(0, DSNODE_TYPE_USERROLE, _imp->nodeType);

    // If it's another node
    if (_imp->nodeType == DSNode::CommonNodeType) {
        const KnobsAndGuis &knobs = nodeGui->getKnobs();

        for (KnobsAndGuis::const_iterator it = knobs.begin();
             it != knobs.end(); ++it) {
            boost::shared_ptr<KnobI> knob = it->first;
            KnobGui *knobGui = it->second;

            connect(knobGui, SIGNAL(keyFrameSet()),
                    dopeSheetEditor, SLOT(refreshDopeSheetView()));

            connect(knobGui, SIGNAL(keyFrameRemoved()),
                    dopeSheetEditor, SLOT(refreshDopeSheetView()));

            if ( !knob->canAnimate() || !knob->isAnimationEnabled() ) {
                continue;
            }

            QTreeWidgetItem *childItem = new QTreeWidgetItem(_imp->nameItem);
            childItem->setExpanded(true);
            childItem->setText(0, knob->getDescription().c_str());

            DSKnob *multiDimRootItem = new DSKnob(dopeSheetEditor, childItem,
                                                  knobGui, 0);

            connect(multiDimRootItem, SIGNAL(needNodesVisibleStateChecking()),
                    this, SLOT(checkVisibleState()));

            _imp->dsKnobs.push_back(multiDimRootItem);

            if (knob->getDimension() > 1) {
                for (int i = 0; i < knob->getDimension(); ++i) {
                    QTreeWidgetItem *knobItem = new QTreeWidgetItem(childItem);
                    knobItem->setExpanded(true);
                    knobItem->setText(0, knob->getDimensionName(i).c_str());

                    DSKnob *dsKnob = new DSKnob(dopeSheetEditor, knobItem,
                                                knobGui, i);

                    connect(dsKnob, SIGNAL(needNodesVisibleStateChecking()),
                            this, SLOT(checkVisibleState()));

                    _imp->dsKnobs.push_back(dsKnob);
                }
            }
        }
    }

    //    checkVisibleState();
}

/**
 * @brief DSNode::~DSNode
 *
 * Deletes all this node's params.
 */
DSNode::~DSNode()
{
    for (DSKnobList::iterator it = _imp->dsKnobs.begin();
         it != _imp->dsKnobs.end();
         ++it) {
        delete (*it);
    }

    delete _imp->nameItem;

    _imp->dsKnobs.clear();
}

/**
 * @brief DSNode::getNameItem
 *
 * Returns the hierarchy view item associated with this node.
 */
QTreeWidgetItem *DSNode::getNameItem() const
{
    return _imp->nameItem;
}

QRectF DSNode::getNameItemRect() const
{
    return _imp->dopeSheetEditor->getNameItemRect(_imp->nameItem);
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
 * @brief DSNode::getKeyframeSets
 *
 *
 */
DSKnobList DSNode::getDSKnobs() const
{
    return _imp->dsKnobs;
}

/**
 * @brief DSNode::isExpanded
 *
 *
 */
bool DSNode::isExpanded() const
{
    return (_imp->nameItem->isExpanded());
}

bool DSNode::isHidden() const
{
    return (_imp->nameItem->isHidden());
}

/**
 * @brief DSNode::isParentNode
 *
 * Returns true if the associated node is a node containing
 * animated knobs.
 */
bool DSNode::isCommonNode() const
{
    return (_imp->nodeType == DSNode::CommonNodeType);
}

/**
 * @brief DSNode::isClipNode
 *
 * Returns true if the associated node is a Read node
 */
bool DSNode::isReaderNode() const
{
    return (_imp->nodeType == DSNode::ReaderNodeType);
}

/**
 * @brief DSNode::isTimeNode
 *
 * Returns true if the associated node's type is between the following :
 */
//bool DSNode::isTimeNode() const
//{
//    return (_imp->nodeType == DSNode::TimeNodeType);
//}

bool DSNode::isGroupNode() const
{
    return (_imp->nodeType == DSNode::GroupNodeType);
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
    bool showItem = _imp->nodeGui->getSettingPanel()->isVisible();

    if (_imp->nodeType == DSNode::CommonNodeType) {
        showItem = showItem && nodeHasAnimation(_imp->nodeGui);
    }
    else if (_imp->nodeType == DSNode::GroupNodeType) {
        showItem = showItem && groupHasAnimation(dynamic_cast<NodeGroup *>(_imp->nodeGui->getNode()->getLiveInstance()));
    }

    _imp->nameItem->setHidden(!showItem);
}

/**
* @brief DSNode::onNodeNameChanged
*
* Sets the text of the associated item in the hierarchy view by 'name'.
*
* This slot is automatically called when the text of the
* internal NodeGui change.
*/
void DSNode::onNodeNameChanged(const QString &name)
{
    _imp->nameItem->setText(0, name);
}


////////////////////////// HierarchyView //////////////////////////

/**
 * @brief The HierarchyViewItemDelegate class
 *
 *
 */

class HierarchyViewItemDelegate : public QItemDelegate
{
public:
    explicit HierarchyViewItemDelegate(HierarchyView *hierarchyView);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    HierarchyView *m_hierarchyView;
};

/**
 * @brief HierarchyViewItemDelegate::HierarchyViewItemDelegate
 *
 *
 */
HierarchyViewItemDelegate::HierarchyViewItemDelegate(HierarchyView *hierarchyView) :
    QItemDelegate(hierarchyView),
    m_hierarchyView(hierarchyView)
{}

/**
 * @brief HierarchyViewItemDelegate::sizeHint
 *
 *
 */
QSize HierarchyViewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    QTreeWidgetItem *item = m_hierarchyView->itemFromIndex(index);

    QSize itemSize = QItemDelegate::sizeHint(option, index);

    int dsNodeType = item->data(0, DSNODE_TYPE_USERROLE).toInt();

    if (dsNodeType == DSNode::ReaderNodeType || dsNodeType == DSNode::GroupNodeType) {
        itemSize.rheight() += 10;
    }

    return itemSize;
}

/**
 * @brief The HierarchyView class
 *
 *
 */

/**
 * @brief HierarchyView::HierarchyView
 *
 *
 */
HierarchyView::HierarchyView(QWidget *parent) :
    QTreeWidget(parent)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setColumnCount(1);
    header()->close();

    setItemDelegateForColumn(0, new HierarchyViewItemDelegate(this));
}


////////////////////////// DopeSheetEditor //////////////////////////

class DopeSheetEditorPrivate
{
public:
    DopeSheetEditorPrivate(Gui *gui);

    /* functions */

    /* attributes */
    Gui *gui;

    boost::shared_ptr<TimeLine> timeline;

    QVBoxLayout *mainLayout;

    QSplitter *splitter;
    HierarchyView *hierarchyView;
    DopeSheetView *dopeSheetView;

    DSNodeList dsNodes;
};

DopeSheetEditorPrivate::DopeSheetEditorPrivate(Gui *gui)  :
    gui(gui),
    timeline(),
    mainLayout(0),
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
    _imp(new DopeSheetEditorPrivate(gui))
{
    _imp->timeline = timeline;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->splitter = new QSplitter(Qt::Horizontal, this);

    _imp->hierarchyView = new HierarchyView(_imp->splitter);

    _imp->splitter->addWidget(_imp->hierarchyView);
    _imp->splitter->setStretchFactor(0, 1);

    _imp->dopeSheetView = new DopeSheetView(this, gui, timeline, _imp->splitter);

    _imp->splitter->addWidget(_imp->dopeSheetView);
    _imp->splitter->setStretchFactor(1, 5);

    _imp->mainLayout->addWidget(_imp->splitter);

    connect(_imp->hierarchyView, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));

    connect(_imp->hierarchyView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)));

    connect(_imp->hierarchyView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(refreshDopeSheetView()));

    connect(_imp->hierarchyView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(refreshDopeSheetView()));
}

/**
 * @brief DopeSheetEditor::~DopeSheetEditor
 *
 * Deletes all the nodes from the DopeSheetEditor.
 */
DopeSheetEditor::~DopeSheetEditor()
{
    for (DSNodeList::iterator it = _imp->dsNodes.begin();
         it != _imp->dsNodes.end(); ++it) {
        delete (*it)->getNameItem();
    }

    _imp->dsNodes.clear();
}

/**
 * @brief DopeSheetEditor::getHierarchyView
 *
 *
 */
QTreeWidget *DopeSheetEditor::getHierarchyView() const
{
    return _imp->hierarchyView;
}

/**
 * @brief DopeSheetEditor::getNameItemRect
 *
 *
 */
QRect DopeSheetEditor::getNameItemRect(const QTreeWidgetItem *item) const
{
    return (_imp->hierarchyView->visualItemRect(item));
}

/**
 * @brief DopeSheetEditor::getSectionRect
 *
 *
 */
QRect DopeSheetEditor::getSectionRect(const QTreeWidgetItem *item) const
{

}

boost::shared_ptr<TimeLine> DopeSheetEditor::getTimeline() const
{
    return _imp->timeline;
}

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
    nodeGui->ensurePanelCreated();

    // Don't show the group nodes' input & output
    if (dynamic_cast<GroupInput *>(nodeGui->getNode()->getLiveInstance()) ||
            dynamic_cast<GroupOutput *>(nodeGui->getNode()->getLiveInstance())) {
        return;
    }

    const std::vector<boost::shared_ptr<KnobI> > &knobs = nodeGui->getNode()->getKnobs();

    if (/*!node->getSettingPanel() || */knobs.empty()) {
        return;
    }

    _imp->dsNodes.push_back(new DSNode(this, nodeGui));
}

/**
 * @brief DopeSheetEditor::removeNode
 *
 * Removes 'node' from the dope sheet.
 * Its associated items are removed from the hierarchy view as its keyframe rows.
 */
void DopeSheetEditor::removeNode(NodeGui *node)
{
    for (DSNodeList::iterator it = _imp->dsNodes.begin();
         it != _imp->dsNodes.end();
         ++it)
    {
        DSNode *currentDSNode = (*it);

        if (currentDSNode->getNodeGui().get() == node) {
            _imp->dsNodes.erase(it);

            delete (currentDSNode);

            break;
        }
    }
}

/**
 * @brief DopeSheetEditor::onItemSelectionChanged
 *
 * Selects recursively the current selected items of the hierarchy view.
 *
 * This slot is automatically called when this current selection has changed.
 */
void DopeSheetEditor::onItemSelectionChanged()
{
    QList<QTreeWidgetItem *> selectedItems = _imp->hierarchyView->selectedItems();

    Q_FOREACH (QTreeWidgetItem *item, selectedItems) {
        recursiveSelect(item);
    }
}

/**
 * @brief DopeSheetEditor::onItemDoubleClicked
 *
 * Ensures that the node panel associated with 'item' is the top-most displayed
 * Sin the Properties panel.
 *
 * This slot is automatically called when an item is double clicked in the
 * hierarchy view.
 */
void DopeSheetEditor::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    boost::shared_ptr<NodeGui> node;

    for (DSNodeList::const_iterator it = _imp->dsNodes.begin();
         it != _imp->dsNodes.end();
         ++it) {

        if ((*it)->getNameItem() == item) {
            node = (*it)->getNodeGui();
            break;
        }

        DSKnobList dsknobItems = (*it)->getDSKnobs();

        for (DSKnobList::const_iterator it2 = dsknobItems.begin();
             it2 != dsknobItems.end();
             ++it2) {
            if ((*it2)->getNameItem() == item)
                node = (*it)->getNodeGui();
        }

        if (node) {
            break;
        }
    }

    DockablePanel *panel = 0;

    if (node) {
        node->ensurePanelCreated();
    }

    if (node && node->getParentMultiInstance()) {
        panel = node->getParentMultiInstance()->getSettingPanel();
    } else {
        panel = node->getSettingPanel();
    }

    if (node && panel && node->isVisible()) {
        if ( !node->isSettingsPanelVisible() ) {
            node->setVisibleSettingsPanel(true);
        }

        if ( !node->wasBeginEditCalled() ) {
            node->beginEditKnobs();
        }

        _imp->gui->putSettingsPanelFirst( node->getSettingPanel() );
        _imp->gui->getApp()->redrawAllViewers();
    }
}

/**
 * @brief DopeSheetEditor::refreshDopeSheetView
 *
 * This slot is automatically called when :
 * -
 * -
 * -
 */
void DopeSheetEditor::refreshDopeSheetView()
{
    _imp->dopeSheetView->update();
}

/**
 * @brief DopeSheetEditor::getNodes
 *
 * Returns the node ranges from the dope sheet view.
 */
const DSNodeList &DopeSheetEditor::getDSNodeItems() const
{
    return _imp->dsNodes;
}
