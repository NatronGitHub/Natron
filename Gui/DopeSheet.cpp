#include "DopeSheet.h"

#include <QDebug> //REMOVEME
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QTreeWidget>

#include "Gui/Gui.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"

#include "Engine/Knob.h"
#include "Engine/Node.h"


typedef std::map<boost::shared_ptr<KnobI>, KnobGui *> KnobsAndGuis;
typedef std::list<DopeSheetNode *> DopeSheetNodeList;
typedef std::list<DopeSheetKeyframeSet *> DopeSheetKeyframeSetList;


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
         it < knobs.end();
         ++it) {
        boost::shared_ptr<KnobI> knob = *it;

        if (knob->hasAnimation()) {
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
bool noChildIsVisible(QTreeWidgetItem *item)
{
    for (int i = 0; i < item->childCount(); ++i) {
        if (!item->child(i)->isHidden())
            return false;
    }

    return true;
}

} // anon namespace


////////////////////////// DopeSheetKeyframeSet //////////////////////////

class DopeSheetKeyframeSetPrivate
{
public:
    DopeSheetKeyframeSetPrivate() :
        hierarchyViewItem(0),
        knob(),
        knobGui(0)
    {}

    /* functions */
    bool isMultiDimRoot() const
    {
        return (dimension > 1) && (hierarchyViewItem->childCount() > 0);
    }

    /* attributes */
    QTreeWidgetItem *hierarchyViewItem;
    boost::shared_ptr<KnobI> knob;
    KnobGui *knobGui;
    int dimension;
};


/**
 * @class DopeSheetKeyframeSet
 *
 * The DopeSheetKeyframeSet class describes a knob' set of keyframes in the
 * DopeSheet.
 */

/**
 * @brief DopeSheetKeyframeSet::DopeSheetKeyframeSet
 *
 * Constructs a DopeSheetKeyframeSet.
 * Adds an item in the hierarchy view with 'parentItem' as parent item.
 *
 * 'knob', 'dimension' and 'isMultiDim' areused to name this item.
 *
 *' knobGui' is used to ensure that the DopeSheet graphical elements will
 * properly react to any keyframe modification.
 */
DopeSheetKeyframeSet::DopeSheetKeyframeSet(QTreeWidgetItem *parentItem,
                               const boost::shared_ptr<KnobI> knob,
                               KnobGui *knobGui,
                               int dimension,
                               bool isMultiDim) :
    _imp(new DopeSheetKeyframeSetPrivate)
{
    _imp->hierarchyViewItem = new QTreeWidgetItem(parentItem);
    _imp->hierarchyViewItem->setText(0, (isMultiDim) ? knob->getDimensionName(dimension).c_str() : knob->getDescription().c_str());

    _imp->knob = knob;
    _imp->knobGui = knobGui;
    _imp->dimension = dimension;

    connect(knobGui, SIGNAL(keyFrameSet()),
            this, SLOT(checkVisibleState()));

    connect(knobGui, SIGNAL(keyFrameRemoved()),
            this, SLOT(checkVisibleState()));

    checkVisibleState();
}

DopeSheetKeyframeSet::~DopeSheetKeyframeSet()
{}

/**
 * @brief DopeSheetKeyframeSet::getItem
 *
 * Returns the associated item in the hierarchy view.
 */
QTreeWidgetItem *DopeSheetKeyframeSet::getItem() const
{
    return _imp->hierarchyViewItem;
}

/**
 * @brief DopeSheetKeyframeSet::checkVisibleState
 *
 * Handle the visibility of the item and its parent(s).
 *
 * This slot is automatically called each time a keyframe is
 * set or removed for this knob.
 */
void DopeSheetKeyframeSet::checkVisibleState()
{
    QTreeWidgetItem *parentInHierarchyView = _imp->hierarchyViewItem->parent();

    if (_imp->isMultiDimRoot()) {
        if (noChildIsVisible(_imp->hierarchyViewItem)) {
            _imp->hierarchyViewItem->setHidden(true);
        }
        else {
            _imp->hierarchyViewItem->setHidden(false);
        }
    }
    else {
        if (!_imp->knob->isAnimated(_imp->dimension)) {
            _imp->hierarchyViewItem->setHidden(true);

            // Hide the multidim root if it's "empty"
            if (noChildIsVisible(parentInHierarchyView))
                parentInHierarchyView->setHidden(true);
        }
        else {
            _imp->hierarchyViewItem->setHidden(false);
            parentInHierarchyView->setHidden(false);

            // Show the node item if necessary
            if (parentInHierarchyView->parent()) {
                parentInHierarchyView->parent()->setHidden(false);
            }
        }
    }
}


////////////////////////// DopeSheetNode //////////////////////////

class DopeSheetNodePrivate
{
public:
    DopeSheetNodePrivate() :
        node(),
        nameItem(0)
    {}

    /* attributes */
    boost::shared_ptr<NodeGui> node;

    QTreeWidgetItem *nameItem;

    DopeSheetKeyframeSetList params;
};

/**
 * @class DopeSheetNode
 *
 * The DopeSheetNode class describes a node and it's animated knobs in
 * the DopeSheet.
 *
 * A DopeSheetNode contains a list of DopeSheetKeyframeSet.
 */

/**
* @brief DopeSheetNode::DopeSheetNode
*
* Constructs a DopeSheetNode.
* A hierarchy view item is created in 'hierarchyView'.
*
* 'node' is used to fill this item with all its knobs. If 'node' contains
* no animated knob, the item is hidden in the hierarchy view and the keyframe
* set is hidden in the dope sheet view.
*/
DopeSheetNode::DopeSheetNode(QTreeWidget *hierarchyView,
                             const boost::shared_ptr<NodeGui> &node) :
    _imp(new DopeSheetNodePrivate)
{
    _imp->nameItem = new QTreeWidgetItem(hierarchyView);
    _imp->nameItem->setText(0, node->getNode()->getLabel().c_str());

    _imp->node = node;

    connect(_imp->node->getNode().get(), SIGNAL(labelChanged(QString)),
            this, SLOT(onNodeNameChanged(QString)));

    const KnobsAndGuis &knobs = node->getKnobs();

    for (KnobsAndGuis::const_iterator it = knobs.begin();
         it != knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first;
        KnobGui *knobGui = it->second;

        if ( !knob->canAnimate() || !knob->isAnimationEnabled() ) {
            continue;
        }

        DopeSheetKeyframeSet *elem = new DopeSheetKeyframeSet(_imp->nameItem, knob,
                                                  knobGui, 0, false);

        _imp->params.push_back(elem);

        if (knob->getDimension() > 1) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                _imp->params.push_back(new DopeSheetKeyframeSet(elem->getItem(), knob,
                                                          knobGui, i, true));
            }
        }

        checkVisibleState();
    }
}

/**
 * @brief DopeSheetNode::~DopeSheetNode
 *
 * Delete all this node's params.
 */
DopeSheetNode::~DopeSheetNode()
{
    delete _imp->nameItem;

    for (DopeSheetKeyframeSetList::iterator it = _imp->params.begin();
         it != _imp->params.end();
         ++it) {
        delete *it;
    }

    _imp->params.clear();
}

/**
 * @brief DopeSheetNode::getNode
 *
 * Returns the associated node.
 */
boost::shared_ptr<NodeGui> DopeSheetNode::getNode() const
{
    return _imp->node;
}

/**
 * @brief DopeSheetNode::checkVisibleState
 *
 * If the item and its set of keyframes must be hidden or not,
 * hide or show them.
 */
void DopeSheetNode::checkVisibleState()
{
    if (!nodeHasAnimation(_imp->node))
        _imp->nameItem->setHidden(true);
    else if (_imp->nameItem->isHidden())
        _imp->nameItem->setHidden(false);
}

/**
* @brief DopeSheetNode::onNodeNameChanged
*
* Set the text of the associated item in the hierarchy view by 'name'.
*
* This slot is automatically called when the text of the
* internal NodeGui change.
*/
void DopeSheetNode::onNodeNameChanged(const QString &name)
{
    _imp->nameItem->setText(0, name);
}


////////////////////////// DopeSheet //////////////////////////

class DopeSheetPrivate
{
public:
    DopeSheetPrivate(Gui *gui) :
        gui(gui),
        mainLayout(0),
        splitter(0),
        hierarchyTree(0)
    {}

    /* attributes */
    Gui *gui;

    QVBoxLayout *mainLayout;

    QSplitter *splitter;
    QTreeWidget *hierarchyTree;

    DopeSheetNodeList nodes;
};

/**
 * @class DopeSheet
 *
 * The DopeSheet class provides several widgets to edit keyframe animations in
 * a more user-friendly way than the Curve Editor.
 *
 * It contains two main widgets : at left, the hierarchy view provides a tree
 * representation of the animated parameters (knobs) of each opened node.
 * The dopesheet itself is an OpenGL widget displaying horizontally the
 * keyframes of these knobs. The user can select, move and delete the keyframes.
 */


/**
 * @brief DopeSheet::DopeSheet
 *
 * Creates a DopeSheet widget.
 */
DopeSheet::DopeSheet(Gui *gui, QWidget *parent) :
    QWidget(parent),
    ScriptObject(),
    _imp(new DopeSheetPrivate(gui))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->splitter = new QSplitter(Qt::Horizontal, this);

    _imp->hierarchyTree = new QTreeWidget(_imp->splitter);
    _imp->hierarchyTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _imp->hierarchyTree->setColumnCount(1);
    _imp->hierarchyTree->header()->close();

    _imp->splitter->addWidget(_imp->hierarchyTree);
    _imp->splitter->setStretchFactor(0, 1);

    _imp->mainLayout->addWidget(_imp->splitter);
}

/**
 * @brief DopeSheet::~DopeSheet
 *
 * Delete all the nodes from the DopeSheet.
 */
DopeSheet::~DopeSheet()
{
    for (DopeSheetNodeList::iterator it = _imp->nodes.begin();
         it != _imp->nodes.end(); ++it) {
        delete (*it);
    }

    _imp->nodes.clear();
}

/**
 * @brief DopeSheet::frame
 *
 * Centers the dopesheet view on the selected keyframes. If no keyframe is
 * selected, centers on all existing keyframes.
 *
 * The keyboard shortcut is 'F'.
 */
void DopeSheet::frame()
{

}

/**
 * @brief DopeSheet::addNode
 *
 * Add 'node' to the hierarchy view, except if :
 * - the node haven't an existing setting panel ;
 * - the node haven't knobs ;
 * - any knob of the node can't be animated or have no animation.
 */
void DopeSheet::addNode(boost::shared_ptr<NodeGui> node)
{
    const std::vector<boost::shared_ptr<KnobI> > &knobs = node->getNode()->getKnobs();

    if (!node->getSettingPanel() || knobs.empty())
        return;

    _imp->nodes.push_back(new DopeSheetNode(_imp->hierarchyTree, node));
}

/**
 * @brief DopeSheet::removeNode
 *
 * Remove 'node' from the dope sheet.
 * Its associated items are removed from the hierarchy view as its keyframe rows.
 */
void DopeSheet::removeNode(NodeGui *node)
{
    for (DopeSheetNodeList::iterator it = _imp->nodes.begin();
         it != _imp->nodes.end();
         ++it)
    {
        DopeSheetNode *currenDopeSheetNode = (*it);

        if (currenDopeSheetNode->getNode().get() == node) {
            delete (currenDopeSheetNode);

            _imp->nodes.erase(it);

            break;
        }
    }
}
