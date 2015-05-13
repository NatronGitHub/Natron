#include "DopeSheet.h"

#include <QDebug> //REMOVEME
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QStyledItemDelegate>
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

QRectF DSKnob::getTreeItemRect() const
{
    return _imp->nameItem->treeWidget()->visualItemRect(_imp->nameItem);
}

QRectF DSKnob::getTreeItemRectForDim(int dim) const
{
    return _imp->nameItem->treeWidget()->visualItemRect(_imp->nameItem->child(dim));
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

QTreeWidgetItem *DSKnob::getTreeItem() const
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

    TreeItemsAndDSKnobs treeItemsAndDSKnobs;

    std::pair<double, double> clipRange;

    bool isSelected;
};

DSNodePrivate::DSNodePrivate() :
    dopeSheetEditor(0),
    nodeGui(),
    nameItem(0),
    treeItemsAndDSKnobs(),
    isSelected(false)
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
               QTreeWidgetItem *nameItem,
               const boost::shared_ptr<NodeGui> &nodeGui) :
    QObject(),
    _imp(new DSNodePrivate)
{
    _imp->dopeSheetEditor = dopeSheetEditor;
    _imp->nameItem = nameItem;
    _imp->nodeGui = nodeGui;

    boost::shared_ptr<Natron::Node> node = nodeGui->getNode();

    connect(node.get(), SIGNAL(labelChanged(QString)),
            this, SLOT(onNodeNameChanged(QString)));

    connect(nodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
            dopeSheetEditor, SLOT(refreshDopeSheetView()));

    DSNode::DSNodeType nodeType = getDSNodeType();

    // Create the hierarchy
    // If it's a Read node
    if (nodeType == DSNode::ReaderNodeType) {
        // The dopesheet view must refresh if the user set some values in the settings panel
        // so we connect some signals/slots
        boost::shared_ptr<KnobSignalSlotHandler> firstFrameKnob = node->getKnobByName("firstFrame")->getSignalSlotHandler();
        boost::shared_ptr<KnobSignalSlotHandler> lastFrameKnob =  node->getKnobByName("lastFrame")->getSignalSlotHandler();
        boost::shared_ptr<KnobSignalSlotHandler> startingTimeKnob = node->getKnobByName("startingTime")->getSignalSlotHandler();

        connect(firstFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(computeReaderRange()));

        connect(lastFrameKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(computeReaderRange()));

        connect(startingTimeKnob.get(), SIGNAL(valueChanged(int, int)),
                this, SLOT(computeReaderRange()));
    }

    // If it's another node
    if (nodeType == DSNode::CommonNodeType) {
        const KnobsAndGuis &knobs = nodeGui->getKnobs();

        if (DSNode *parentGroupDSNode = dopeSheetEditor->getParentGroupDSNode(this)) {
            connect(nodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
                    parentGroupDSNode, SLOT(checkVisibleState()));

            connect(nodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
                    parentGroupDSNode, SLOT(computeGroupRange()));
        }

        for (KnobsAndGuis::const_iterator it = knobs.begin();
             it != knobs.end(); ++it) {
            boost::shared_ptr<KnobI> knob = it->first.lock();
            KnobGui *knobGui = it->second;

            if (!knob->canAnimate() || !knob->isAnimationEnabled()) {
                continue;
            }

            if (DSNode *parentGroupDSNode = dopeSheetEditor->getParentGroupDSNode(this)) {
                connect(knob->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                        parentGroupDSNode, SLOT(computeGroupRange()));

                connect(knobGui, SIGNAL(keyFrameSet()),
                        parentGroupDSNode, SLOT(computeGroupRange()));

                connect(knobGui, SIGNAL(keyFrameRemoved()),
                        parentGroupDSNode, SLOT(computeGroupRange()));
            }

            DSKnob *dsKnob = dopeSheetEditor->createDSKnob(knobGui, this);

            _imp->treeItemsAndDSKnobs.insert(TreeItemAndDSKnob(dsKnob->getTreeItem(), dsKnob));
        }

        if (DSNode *parentGroupDSNode = dopeSheetEditor->getParentGroupDSNode(this)) {
            parentGroupDSNode->computeGroupRange();
        }
    }

    // If some subnodes are already in the dope sheet, the connections must be set to update
    // the group's clip rect
    if (nodeType == DSNode::GroupNodeType) {
        NodeList subNodes = dynamic_cast<NodeGroup *>(nodeGui->getNode()->getLiveInstance())->getNodes();

        for (NodeList::const_iterator it = subNodes.begin(); it != subNodes.end(); ++it) {
            NodePtr subNode = (*it);
            boost::shared_ptr<NodeGui> subNodeGui = boost::dynamic_pointer_cast<NodeGui>(subNode->getNodeGui());

            if (!subNodeGui->getSettingPanel() || !subNodeGui->getSettingPanel()->isVisible()) {
                continue;
            }

            connect(subNodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
                    this, SLOT(checkVisibleState()));

            connect(subNodeGui->getSettingPanel(), SIGNAL(closeChanged(bool)),
                    this, SLOT(computeGroupRange()));

            const KnobsAndGuis &knobs = subNodeGui->getKnobs();

            for (KnobsAndGuis::const_iterator knobIt = knobs.begin();
                 knobIt != knobs.end(); ++knobIt) {
                boost::shared_ptr<KnobI> knob = knobIt->first.lock();
                KnobGui *knobGui = knobIt->second;

                connect(knob->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                        this, SLOT(computeGroupRange()));

                connect(knobGui, SIGNAL(keyFrameSet()),
                        this, SLOT(computeGroupRange()));

                connect(knobGui, SIGNAL(keyFrameRemoved()),
                        this, SLOT(computeGroupRange()));
            }
        }

        computeGroupRange();
    }
}

/**
 * @brief DSNode::~DSNode
 *
 * Deletes all this node's params.
 */
DSNode::~DSNode()
{
    for (TreeItemsAndDSKnobs::iterator it = _imp->treeItemsAndDSKnobs.begin();
         it != _imp->treeItemsAndDSKnobs.end();
         ++it) {
        delete (*it).second;
    }

    delete _imp->nameItem;

    _imp->treeItemsAndDSKnobs.clear();
}

QRectF DSNode::getTreeItemRect() const
{
    return _imp->nameItem->treeWidget()->visualItemRect(_imp->nameItem);
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
TreeItemsAndDSKnobs DSNode::getTreeItemsAndDSKnobs() const
{
    return _imp->treeItemsAndDSKnobs;
}

DSNode::DSNodeType DSNode::getDSNodeType() const
{
    return DSNodeType(_imp->nameItem->type());
}

/**
 * @brief DSNode::isParentNode
 *
 * Returns true if the associated node is a node containing
 * animated knobs.
 */
bool DSNode::isCommonNode() const
{
    return (getDSNodeType() == DSNode::CommonNodeType);
}

/**
 * @brief DSNode::isClipNode
 *
 * Returns true if the associated node is a Read node
 */
bool DSNode::isReaderNode() const
{
    return (getDSNodeType() == DSNode::ReaderNodeType);
}

bool DSNode::isGroupNode() const
{
    return (getDSNodeType() == DSNode::GroupNodeType);
}

std::pair<double, double> DSNode::getClipRange() const
{
    return _imp->clipRange;
}

bool DSNode::isSelected() const
{
    return _imp->isSelected;
}

void DSNode::setSelected(bool selected)
{
    if (_imp->isSelected != selected) {
        _imp->isSelected = selected;
    }
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

    DSNode::DSNodeType nodeType = getDSNodeType();

    if (nodeType == DSNode::CommonNodeType) {
        showItem = nodeHasAnimation(_imp->nodeGui);
    }
    else if (nodeType == DSNode::GroupNodeType) {
        NodeGroup *group = dynamic_cast<NodeGroup *>(_imp->nodeGui->getNode()->getLiveInstance());

        showItem = showItem && !_imp->dopeSheetEditor->groupSubNodesAreHidden(group);
    }

    _imp->nameItem->setHidden(!showItem);

    // Hide the parent group item if there's no subnodes displayed
    if (DSNode *parentGroupDSNode = _imp->dopeSheetEditor->getParentGroupDSNode(this)) {
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
    assert(isReaderNode());

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
    assert(isGroupNode());

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


////////////////////////// HierarchyView //////////////////////////

/**
 * @brief The HierarchyViewItemDelegate class
 *
 *
 */

class HierarchyViewItemDelegate : public QStyledItemDelegate
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
    QStyledItemDelegate(hierarchyView),
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

    QSize itemSize = QStyledItemDelegate::sizeHint(option, index);

    DSNode::DSNodeType nodeType = DSNode::DSNodeType(item->type());

    if (nodeType == DSNode::ReaderNodeType || nodeType == DSNode::GroupNodeType) {
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
HierarchyView::HierarchyView(DopeSheetEditor *editor, QWidget *parent) :
    QTreeWidget(parent),
    m_editor(editor)
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setColumnCount(1);
    header()->close();

    setItemDelegate(new HierarchyViewItemDelegate(this));
}

void HierarchyView::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);
    DSNode *itemDSNode = m_editor->findParentDSNode(item);

    double r, g, b;
    itemDSNode->getNodeGui()->getColor(&r, &g, &b);

    const int verticalBarWitdh = 2;

    QRect rowRect = option.rect;

    QPen pen;
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(verticalBarWitdh);
    pen.setColor(QColor::fromRgbF(r, g, b));

    painter->setPen(pen);
    painter->drawLine(rowRect.left() + 3, rowRect.bottom(),
                      rowRect.left() + 3, rowRect.top());

    QTreeWidget::drawRow(painter, option, index);
}


////////////////////////// DopeSheetEditor //////////////////////////

class DopeSheetEditorPrivate
{
public:
    DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui);

    /* attributes */
    DopeSheetEditor *parent;
    Gui *gui;

    QVBoxLayout *mainLayout;

    QSplitter *splitter;
    HierarchyView *hierarchyView;
    DopeSheetView *dopeSheetView;

    TreeItemsAndDSNodes treeItemsAndDSNodes;
};

DopeSheetEditorPrivate::DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui)  :
    parent(qq),
    gui(gui),
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
    _imp(new DopeSheetEditorPrivate(this, gui))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->splitter = new QSplitter(Qt::Horizontal, this);

    _imp->hierarchyView = new HierarchyView(this, _imp->splitter);

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
            this, SLOT(refreshClipRects()));

    connect(_imp->hierarchyView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            this, SLOT(refreshClipRects()));

    connect(_imp->hierarchyView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            _imp->dopeSheetView, SLOT(computeSelectedKeysBRect()));

    connect(_imp->hierarchyView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            _imp->dopeSheetView, SLOT(computeSelectedKeysBRect()));
}

/**
 * @brief DopeSheetEditor::~DopeSheetEditor
 *
 * Deletes all the nodes from the DopeSheetEditor.
 */
DopeSheetEditor::~DopeSheetEditor()
{
    for (TreeItemsAndDSNodes::iterator it = _imp->treeItemsAndDSNodes.begin();
         it != _imp->treeItemsAndDSNodes.end(); ++it) {
        delete (*it).second;
    }

    _imp->treeItemsAndDSNodes.clear();
}

/**
 * @brief DopeSheetEditor::getHierarchyView
 *
 *
 */
HierarchyView *DopeSheetEditor::getHierarchyView() const
{
    return _imp->hierarchyView;
}


/**
 * @brief DopeSheetEditor::getNodes
 *
 * Returns the node ranges from the dope sheet view.
 */
TreeItemsAndDSNodes DopeSheetEditor::getTreeItemsAndDSNodes() const
{
    return _imp->treeItemsAndDSNodes;
}

DSNode *DopeSheetEditor::findDSNode(const boost::shared_ptr<Natron::Node> &node) const
{
    for (TreeItemsAndDSNodes::const_iterator it = _imp->treeItemsAndDSNodes.begin();
         it != _imp->treeItemsAndDSNodes.end();
         ++it) {
        DSNode *dsNode = (*it).second;

        if (dsNode->getNodeGui()->getNode() == node) {
            return dsNode;
        }
    }

    return 0;
}

DSNode *DopeSheetEditor::findDSNode(QTreeWidgetItem *item) const
{
    TreeItemsAndDSNodes::const_iterator dsNodeIt = _imp->treeItemsAndDSNodes.find(item);

    // Okay, the user not clicked on a top level item (which is associated with a DSNode)
    if (dsNodeIt != _imp->treeItemsAndDSNodes.end()) {
        return (*dsNodeIt).second;
    }

    return NULL;
}

DSNode *DopeSheetEditor::findParentDSNode(QTreeWidgetItem *item) const
{
    TreeItemsAndDSNodes::const_iterator clickedDSNode = _imp->treeItemsAndDSNodes.find(item);

    // Okay, the user not clicked on a top level item (which is associated with a DSNode)
    if (clickedDSNode == _imp->treeItemsAndDSNodes.end()) {
        // So we find this top level item
        QTreeWidgetItem *itemRoot = item;

        while (itemRoot->parent()) {
            itemRoot = itemRoot->parent();
        }

        // And we find the node
        clickedDSNode = _imp->treeItemsAndDSNodes.find(itemRoot);
    }

    return (*clickedDSNode).second;
}

DSKnob *DopeSheetEditor::findDSKnob(QTreeWidgetItem *item, int *dimension) const
{
    DSKnob *ret = 0;

    DSNode *dsNode = findParentDSNode(item);

    TreeItemsAndDSKnobs treeItemsAndKnobs = dsNode->getTreeItemsAndDSKnobs();
    TreeItemsAndDSKnobs::const_iterator knobIt = treeItemsAndKnobs.find(item);

    if (knobIt == treeItemsAndKnobs.end()) {
        QTreeWidgetItem *knobTreeItem = item->parent();
        knobIt = treeItemsAndKnobs.find(knobTreeItem);

        if (knobIt != treeItemsAndKnobs.end()) {
            ret = knobIt->second;
        }

        if (dimension) {
            if (ret->isMultiDim()) {
                *dimension = knobTreeItem->indexOfChild(item);
            }
        }
    }
    else {
        ret = knobIt->second;

        if (ret->getKnobGui()->getKnob()->getDimension() > 1) {
            *dimension = -1;
        }
        else {
            *dimension = 0;
        }
    }

    return ret;
}

DSKnob *DopeSheetEditor::findDSKnob(const QPoint &point, int *dimension) const
{
    QTreeWidgetItem *treeItemAt = _imp->hierarchyView->itemAt(0, point.y());

    return findDSKnob(treeItemAt, dimension);
}

DSNode *DopeSheetEditor::getParentGroupDSNode(DSNode *dsNode) const
{
    boost::shared_ptr<NodeGroup> parentGroup = boost::dynamic_pointer_cast<NodeGroup>(dsNode->getNodeGui()->getNode()->getGroup());

    DSNode *parentGroupDSNode = 0;

    if (parentGroup) {
        parentGroupDSNode = findDSNode(parentGroup->getNode());
    }

    return parentGroupDSNode;
}

bool DopeSheetEditor::groupSubNodesAreHidden(NodeGroup *group) const
{
    bool ret = true;

    NodeList subNodes = group->getNodes();

    for (NodeList::const_iterator it = subNodes.begin(); it != subNodes.end(); ++it) {
        NodePtr node = (*it);

        DSNode *dsNode = findDSNode(node);

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

    if (nodeGui->getNode()->getKnobs().empty()) {
        return;
    }

    if (!nodeCanAnimate(nodeGui)) {
        return;
    }

    // Create the name item
    DSNode *dsNode = createDSNode(nodeGui);

    _imp->treeItemsAndDSNodes.insert(TreeItemAndDSNode(dsNode->getTreeItem(), dsNode));

    dsNode->checkVisibleState();

    if (DSNode *parentGroupDSNode = getParentGroupDSNode(dsNode)) {
        parentGroupDSNode->computeGroupRange();
    }
}

/**
 * @brief DopeSheetEditor::removeNode
 *
 * Removes 'node' from the dope sheet.
 * Its associated items are removed from the hierarchy view as its keyframe rows.
 */
void DopeSheetEditor::removeNode(NodeGui *node)
{
    for (TreeItemsAndDSNodes::iterator it = _imp->treeItemsAndDSNodes.begin();
         it != _imp->treeItemsAndDSNodes.end();
         ++it)
    {
        DSNode *currentDSNode = (*it).second;

        if (currentDSNode->getNodeGui().get() == node) {
            if (DSNode *parentGroupDSNode = getParentGroupDSNode(currentDSNode)) {
                parentGroupDSNode->computeGroupRange();
            }

            _imp->treeItemsAndDSNodes.erase(it);

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
 * in the Properties panel.
 *
 * This slot is automatically called when an item is double clicked in the
 * hierarchy view.
 */
void DopeSheetEditor::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    DSNode *itemDSNode = findParentDSNode(item);

    boost::shared_ptr<NodeGui> nodeGui = itemDSNode->getNodeGui();

    // Move the nodeGui's settings panel on top
    DockablePanel *panel = 0;

    if (nodeGui) {
        nodeGui->ensurePanelCreated();
    }

    if (nodeGui && nodeGui->getParentMultiInstance()) {
        panel = nodeGui->getParentMultiInstance()->getSettingPanel();
    } else {
        panel = nodeGui->getSettingPanel();
    }

    if (nodeGui && panel && nodeGui->isVisible()) {
        if ( !nodeGui->isSettingsPanelVisible() ) {
            nodeGui->setVisibleSettingsPanel(true);
        }

        if ( !nodeGui->wasBeginEditCalled() ) {
            nodeGui->beginEditKnobs();
        }

        _imp->gui->putSettingsPanelFirst( nodeGui->getSettingPanel() );
        _imp->gui->getApp()->redrawAllViewers();
    }
}

/**
 * @brief DopeSheetEditor::refreshDopeSheetView
 *
 * This slot is automatically called when :
 * -
 * -+
 * -
 */
void DopeSheetEditor::refreshDopeSheetView()
{
    _imp->dopeSheetView->update();
}

DSNode *DopeSheetEditor::createDSNode(const boost::shared_ptr<NodeGui> &nodeGui)
{
    // Determinate the node type
    // It will be useful to identify and sort tree items
    DSNode::DSNodeType nodeType = DSNode::CommonNodeType;

    if (nodeGui->getNode()->getPlugin()->isReader()) {
        nodeType = DSNode::ReaderNodeType;
    }
    else {
        NodeGroup *isGroup = dynamic_cast<NodeGroup *>(nodeGui->getNode()->getLiveInstance());
        if (isGroup) {
            nodeType = DSNode::GroupNodeType;
        }
    }

    QTreeWidgetItem *nameItem = new QTreeWidgetItem(_imp->hierarchyView, nodeType);
    nameItem->setText(0, nodeGui->getNode()->getLabel().c_str());
    nameItem->setExpanded(true);

    DSNode *dsNode = new DSNode(this, nameItem, nodeGui);

    connect(dsNode, SIGNAL(clipRangeChanged()),
            this, SLOT(refreshDopeSheetView()));

    return dsNode;
}

DSKnob *DopeSheetEditor::createDSKnob(KnobGui *knobGui, DSNode *dsNode)
{
    DSKnob *dsKnob = 0;

    boost::shared_ptr<KnobI> knob = knobGui->getKnob();

    if (knob->getDimension() <= 1) {
        QTreeWidgetItem * nameItem = new QTreeWidgetItem(dsNode->getTreeItem());
        nameItem->setExpanded(true);
        nameItem->setText(0, knob->getDescription().c_str());

        dsKnob = new DSKnob(nameItem, knobGui);
    }
    else {
        QTreeWidgetItem *multiDimRootNameItem = new QTreeWidgetItem(dsNode->getTreeItem());
        multiDimRootNameItem->setExpanded(true);
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
            this, SLOT(refreshDopeSheetView()));

    connect(knobGui, SIGNAL(keyFrameRemoved()),
            this, SLOT(refreshDopeSheetView()));

    connect(dsKnob, SIGNAL(needNodesVisibleStateChecking()),
            dsNode, SLOT(checkVisibleState()));

    return dsKnob;
}

void DopeSheetEditor::refreshClipRects()
{
    for (TreeItemsAndDSNodes::const_iterator it = _imp->treeItemsAndDSNodes.begin();
         it != _imp->treeItemsAndDSNodes.end();
         ++it) {
        DSNode *dsNode = (*it).second;
        if (dsNode->isReaderNode()) {
            dsNode->computeReaderRange();
        }
        else if (dsNode->isGroupNode()) {
            dsNode->computeGroupRange();
        }
    }
}
