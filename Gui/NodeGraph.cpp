/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <cstdlib>
#include <set>
#include <map>
#include <vector>
#include <locale>
#include <algorithm> // min, max
#include <stdexcept>

#include "Engine/Backdrop.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Dot.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/BackdropGui.h"
#include "Gui/DotGui.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraphTextItem.h"

#include "Serialization/NodeSerialization.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER
//using std::cout; using std::endl;


void
NodeGraph::makeFullyQualifiedLabel(const NodePtr& node,
                                   std::string* ret)
{
    NodeCollectionPtr parent = node->getGroup();
    NodeGroupPtr isParentGrp = toNodeGroup(parent);
    std::string toPreprend = node->getLabel();

    if (isParentGrp) {
        toPreprend.insert(0, "/");
    }
    ret->insert(0, toPreprend);
    if (isParentGrp) {
        makeFullyQualifiedLabel(isParentGrp->getNode(), ret);
    }
}

NodeGraph::NodeGraph(Gui* gui,
                     const NodeCollectionPtr& group,
                     const std::string& scriptName,
                     QGraphicsScene* scene,
                     QWidget *parent)
    : QGraphicsView(scene, parent)
    , NodeGraphI()
    , PanelWidget(scriptName, this, gui)
    , _imp( new NodeGraphPrivate(this, group) )
{

    QObject::connect(this, SIGNAL(mustRefreshNodeLinksLater()), this, SLOT(onMustRefreshNodeLinksLaterReceived()), Qt::QueuedConnection);

    group->setNodeGraphPointer(this);

    setAttribute(Qt::WA_MacShowFocusRect, 0);

    NodeGroupPtr isGrp = toNodeGroup( group );
    if (isGrp) {
        QObject::connect( isGrp->getNode().get(), SIGNAL(labelChanged(QString, QString)), this, SLOT(onGroupNameChanged(QString, QString)) );
        QObject::connect( isGrp->getNode().get(), SIGNAL(scriptNameChanged(QString)), this, SLOT(onGroupScriptNameChanged(QString)) );
    }

    QObject::connect( &_imp->autoScrollTimer, SIGNAL(timeout()), this, SLOT(onAutoScrollTimerTriggered()) );


    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    scale(0.8, 0.8);

    _imp->_root = new NodeGraphTextItem(this, 0, false);
    _imp->_nodeRoot = new NodeGraphTextItem(this, _imp->_root, false);
    scene->addItem(_imp->_root);

    _imp->_navigator = new Navigator(0);
    scene->addItem(_imp->_navigator);
    _imp->_navigator->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_navigator->hide();


    _imp->_cacheSizeText = new NodeGraphSimpleTextItem(this, 0, true);
    scene->addItem(_imp->_cacheSizeText);
    _imp->_cacheSizeText->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_cacheSizeText->setBrush( QColor(200, 200, 200) );
    _imp->_cacheSizeText->setVisible(false);

    QObject::connect( &_imp->refreshRenderStateTimer, SIGNAL(timeout()), this, SLOT(onRefreshNodesRenderStateTimerTimeout()) );
    _imp->refreshRenderStateTimer.start(NATRON_NODES_RENDER_STATE_REFRESH_INTERVAL_MS);

    QObject::connect( &_imp->_refreshCacheTextTimer, SIGNAL(timeout()), this, SLOT(updateCacheSizeText()) );
    _imp->_refreshCacheTextTimer.start(NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS);

    _imp->_undoStack = boost::make_shared<QUndoStack>(this);
    _imp->_undoStack->setUndoLimit( appPTR->getCurrentSettings()->getMaximumUndoRedoNodeGraph() );
    getGui()->registerNewUndoStack(_imp->_undoStack);

    _imp->_hintInputEdge = new Edge(0, 0, NodeGuiPtr(), _imp->_nodeRoot);
    _imp->_hintInputEdge->setDefaultColor( QColor(0, 255, 0, 100) );
    _imp->_hintInputEdge->hide();

    _imp->_hintOutputEdge = new Edge(0, 0, NodeGuiPtr(), _imp->_nodeRoot);
    _imp->_hintOutputEdge->setDefaultColor( QColor(0, 255, 0, 100) );
    _imp->_hintOutputEdge->hide();

    _imp->_tL = new NodeGraphTextItem(this, 0, false);
    _imp->_tL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_tL);

    _imp->_tR = new NodeGraphTextItem(this, 0, false);
    _imp->_tR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_tR);

    _imp->_bR = new NodeGraphTextItem(this, 0, false);
    _imp->_bR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_bR);

    _imp->_bL = new NodeGraphTextItem(this, 0, false);
    _imp->_bL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_bL);

    _imp->_tL->setPos( _imp->_tL->mapFromScene( QPointF(NATRON_SCENE_MIN, NATRON_SCENE_MAX) ) );
    _imp->_tR->setPos( _imp->_tR->mapFromScene( QPointF(NATRON_SCENE_MAX, NATRON_SCENE_MAX) ) );
    _imp->_bR->setPos( _imp->_bR->mapFromScene( QPointF(NATRON_SCENE_MAX, NATRON_SCENE_MIN) ) );
    _imp->_bL->setPos( _imp->_bL->mapFromScene( QPointF(NATRON_SCENE_MIN, NATRON_SCENE_MIN) ) );
    centerOn(0, 0);
    setSceneRect(NATRON_SCENE_MIN, NATRON_SCENE_MIN, NATRON_SCENE_MAX, NATRON_SCENE_MAX);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setAcceptDrops(false);

    _imp->_menu = new Menu(this);
}

NodeGraph::~NodeGraph()
{

    for (NodesGuiList::iterator it = _imp->_nodes.begin();
         it != _imp->_nodes.end();
         ++it) {
        (*it)->discardGraphPointer();
    }
 
    if ( getGui() ) {

        if (!getGui()->getApp()->isClosing()) {
            getGui()->removeUndoStack(_imp->_undoStack);
        }
        QGraphicsScene* scene = _imp->_hintInputEdge->scene();
        if (scene) {
            scene->removeItem(_imp->_hintInputEdge);
        }
        _imp->_hintInputEdge->setParentItem(NULL);
        delete _imp->_hintInputEdge;

        scene = _imp->_hintOutputEdge->scene();
        if (scene) {
            scene->removeItem(_imp->_hintOutputEdge);
        }
        _imp->_hintOutputEdge->setParentItem(NULL);
        delete _imp->_hintOutputEdge;
    }

    QObject::disconnect( &_imp->_refreshCacheTextTimer, SIGNAL(timeout()), this, SLOT(updateCacheSizeText()) );
    _imp->_nodeCreationShortcutEnabled = false;
}

bool
NodeGraph::isDoingNavigatorRender() const
{
    return _imp->isDoingPreviewRender;
}

NodesGuiList
NodeGraph::getSelectedNodes() const
{
    NodesGuiList ret;
    for (NodesGuiWList::const_iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        NodeGuiPtr n = it->lock();
        if (!n) {
            continue;
        }
        ret.push_back(n);
    }
    return ret;
}

NodeCollectionPtr
NodeGraph::getGroup() const
{
    return _imp->group.lock();
}

QGraphicsItem*
NodeGraph::getRootItem() const
{
    return _imp->_root;
}

void
NodeGraph::notifyGuiClosing()
{
    NodeCollectionPtr group = getGroup();

    if (group) {
        group->discardNodeGraphPointer();
    }
}

void
NodeGraph::onNodesCleared()
{
    _imp->_selection.clear();
    _imp->_magnifiedNode.reset();
    _imp->_undoStack->clear();
}

void
NodeGraph::resizeEvent(QResizeEvent* e)
{
    _imp->_refreshOverlays = true;
    QGraphicsView::resizeEvent(e);
}

void
NodeGraph::paintEvent(QPaintEvent* e)
{
    AppInstancePtr app;

    if ( getGui() ) {
        app = getGui()->getApp();
    }
    if ( app && app->getProject()->isLoadingProjectInternal() ) {
        return;
    }

    NodeCollectionPtr collection = getGroup();
    NodeGroupPtr isGroup = toNodeGroup(collection);
    bool isGroupEditable = true;
    bool groupEdited = true;
    if (isGroup) {
        isGroupEditable = isGroup->isSubGraphEditable();
        groupEdited = isGroup->isSubGraphEditedByUser();
    }

    bool drawLockedMode = !isGroupEditable || !groupEdited;

    if (_imp->_refreshOverlays) {
        ///The visible portion of the scene, in scene coordinates
        QRectF visibleScene = visibleSceneRect();
        QRect visibleWidget = visibleWidgetRect();

        ///Set the cache size overlay to be in the top left corner of the view
        _imp->_cacheSizeText->setPos( visibleScene.topLeft() );

        double navWidth = NATRON_NAVIGATOR_BASE_WIDTH * width();
        double navHeight = NATRON_NAVIGATOR_BASE_HEIGHT * height();
        QPoint btmRightWidget = visibleWidget.bottomRight();
        QPoint navTopLeftWidget = btmRightWidget - QPoint(navWidth, navHeight );
        QPointF navTopLeftScene = mapToScene(navTopLeftWidget);

        _imp->_navigator->refreshPosition(navTopLeftScene, navWidth, navHeight);
        updateNavigator();
        _imp->_refreshOverlays = false;
    }
    QGraphicsView::paintEvent(e);

    if (drawLockedMode) {
        ///Show a semi-opaque forground indicating the PyPlug has not been edited
        QPainter p( viewport() );
        p.setBrush( QColor(120, 120, 120) );
        p.setOpacity(0.7);
        p.drawRect( rect() );

        if (isGroupEditable) {
            ///Draw the unlock icon
            QPoint pixPos = _imp->getPyPlugUnlockPos();
            int pixW = _imp->unlockIcon.width();
            int pixH = _imp->unlockIcon.height();
            QRect pixRect(pixPos.x(), pixPos.y(), pixW, pixH);
            pixRect.adjust(-2, -2, 2, 2);
            QRect selRect = pixRect;
            selRect.adjust(-3, -3, 3, 3);
            p.setBrush( QColor(243, 137, 0) );
            p.setOpacity(1.);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(selRect, 5, 5);
            p.setBrush( QColor(100, 100, 100) );
            p.drawRoundedRect(pixRect, 5, 5);
            p.drawPixmap(pixPos.x(), pixPos.y(), pixW, pixH, _imp->unlockIcon, 0, 0, pixW, pixH);
        }
    }

    if ( (_imp->_evtState == eEventStateSelectionRect) && !isDoingNavigatorRender() ) {
        QPainter p( viewport() );
        const QRect r = mapFromScene(_imp->_selectionRect).boundingRect();
        QColor color(16, 84, 200, 20);
        p.setBrush(color);
        p.drawRect(r);
        double w = p.pen().widthF();
        p.fillRect(QRect(r.x() + w, r.y() + w, r.width() - w, r.height() - w), color);
    }
} // NodeGraph::paintEvent

QRectF
NodeGraph::visibleSceneRect() const
{
    return mapToScene( visibleWidgetRect() ).boundingRect();
}

QRect
NodeGraph::visibleWidgetRect() const
{
    return viewport()->rect();
}

void
NodeGraph::createNodeGui(const NodePtr & node, const CreateNodeArgs& args)
{
    // Save current selection
    NodesGuiList selectedNodes = getSelectedNodes();

    // Create the correct node gui class according to type
    NodeGuiPtr node_ui;
    {
        DotPtr isDotNode = toDot( node->getEffectInstance() );
        BackdropPtr isBd = toBackdrop( node->getEffectInstance() );
        NodeGroupPtr isGrp = toNodeGroup( node->getEffectInstance() );

        if (isDotNode) {
            node_ui = DotGui::create(_imp->_nodeRoot);
        } else if (isBd) {
            node_ui = BackdropGui::create(_imp->_nodeRoot);
        } else {
            node_ui = NodeGui::create(_imp->_nodeRoot);
        }
    }
    assert(node_ui);

    // Add the node to the list. The shared pointer is held as a strong ref on the NodeGraph object.
    {
        QMutexLocker l(&_imp->_nodesMutex);
        _imp->_nodes.push_back(node_ui);
    }

    // This will create the node GUI across all Natron
    node_ui->initialize(this, node, args);

    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization = args.getPropertyUnsafe<SERIALIZATION_NAMESPACE::NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    bool addUndoRedo = args.getPropertyUnsafe<bool>(kCreateNodeArgsPropAddUndoRedoCommand);
    if (addUndoRedo) {
        pushUndoCommand( new AddMultipleNodesCommand(this, node) );
    } else if (!serialization ) {

        selectNode(node_ui, false);


        getGui()->getApp()->triggerAutoSave();
    }

    

    _imp->_evtState = eEventStateNone;

} // NodeGraph::createNodeGUI

void
NodeGraph::onNodeAboutToBeCreated(const NodePtr& /*node*/, const CreateNodeArgsPtr& /*args*/)
{
    // Save the current node selection
    _imp->_selectionBeforeNodeCreated = _imp->_selection;
}

void
NodeGraph::onNodeCreated(const NodePtr& node, const CreateNodeArgsPtr& args)
{

    NodeGuiPtr node_ui = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());
    if (!node_ui) {
        return;
    }

    NodesGuiList selectedNodes;
    for (NodesGuiWList::const_iterator it = _imp->_selectionBeforeNodeCreated.begin(); it != _imp->_selectionBeforeNodeCreated.end(); ++it) {
        NodeGuiPtr n = it->lock();
        if (n) {
            selectedNodes.push_back(n);
        }
    }
    _imp->_selectionBeforeNodeCreated.clear();

    setNodeToDefaultPosition(node_ui, selectedNodes, *args);

}

QIcon
NodeGraph::getIcon() const
{
    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap p;
    appPTR->getIcon(NATRON_PIXMAP_NODE_GRAPH, iconSize, &p);
    return QIcon(p);
}


boost::shared_ptr<QUndoStack>
NodeGraph::getUndoStack() const
{
    return _imp->_undoStack;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_NodeGraph.cpp"
