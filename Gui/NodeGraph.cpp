/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/Backdrop.h"
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
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraphTextItem.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER;
//using std::cout; using std::endl;


void
NodeGraph::makeFullyQualifiedLabel(Node* node,std::string* ret)
{
    boost::shared_ptr<NodeCollection> parent = node->getGroup();
    NodeGroup* isParentGrp = dynamic_cast<NodeGroup*>(parent.get());
    std::string toPreprend = node->getLabel();
    if (isParentGrp) {
        toPreprend.insert(0, "/");
    }
    ret->insert(0, toPreprend);
    if (isParentGrp) {
        makeFullyQualifiedLabel(isParentGrp->getNode().get(), ret);
    }
}

NodeGraph::NodeGraph(Gui* gui,
                     const boost::shared_ptr<NodeCollection>& group,
                     QGraphicsScene* scene,
                     QWidget *parent)
    : QGraphicsView(scene,parent)
    , NodeGraphI()
    , PanelWidget(this,gui)
      , _imp( new NodeGraphPrivate(this, group) )
{
    
    group->setNodeGraphPointer(this);
    
    setAcceptDrops(true);


    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(group.get());
    if (isGrp) {
        
        std::string newName = isGrp->getNode()->getFullyQualifiedName();
        for (std::size_t i = 0; i < newName.size(); ++i) {
            if (newName[i] == '.') {
                newName[i] = '_';
            }
        }
        setScriptName(newName);
        std::string label;
        makeFullyQualifiedLabel(isGrp->getNode().get(),&label);
        setLabel(label);
        QObject::connect(isGrp->getNode().get(), SIGNAL(labelChanged(QString)), this, SLOT( onGroupNameChanged(QString)));
        QObject::connect(isGrp->getNode().get(), SIGNAL(scriptNameChanged(QString)), this, SLOT( onGroupScriptNameChanged(QString)));
    } else {
        setScriptName(kNodeGraphObjectName);
        setLabel(QObject::tr("Node Graph").toStdString());
    }
    
    QObject::connect(&_imp->autoScrollTimer, SIGNAL(timeout()), this, SLOT(onAutoScrollTimerTriggered()));
    
    
    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    scale(0.8,0.8);

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
    _imp->_cacheSizeText->setBrush( QColor(200,200,200) );
    _imp->_cacheSizeText->setVisible(false);

    QObject::connect( &_imp->_refreshCacheTextTimer,SIGNAL( timeout() ),this,SLOT( updateCacheSizeText() ) );
    _imp->_refreshCacheTextTimer.start(NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS);

    _imp->_undoStack = new QUndoStack(this);
    _imp->_undoStack->setUndoLimit( appPTR->getCurrentSettings()->getMaximumUndoRedoNodeGraph() );
    getGui()->registerNewUndoStack(_imp->_undoStack);

    _imp->_hintInputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_imp->_nodeRoot);
    _imp->_hintInputEdge->setDefaultColor( QColor(0,255,0,100) );
    _imp->_hintInputEdge->hide();

    _imp->_hintOutputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_imp->_nodeRoot);
    _imp->_hintOutputEdge->setDefaultColor( QColor(0,255,0,100) );
    _imp->_hintOutputEdge->hide();

    _imp->_tL = new NodeGraphTextItem(this,0,false);
    _imp->_tL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_tL);

    _imp->_tR = new NodeGraphTextItem(this,0,false);
    _imp->_tR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_tR);

    _imp->_bR = new NodeGraphTextItem(this,0,false);
    _imp->_bR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_bR);

    _imp->_bL = new NodeGraphTextItem(this,0,false);
    _imp->_bL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_bL);

    _imp->_tL->setPos( _imp->_tL->mapFromScene( QPointF(NATRON_SCENE_MIN,NATRON_SCENE_MAX) ) );
    _imp->_tR->setPos( _imp->_tR->mapFromScene( QPointF(NATRON_SCENE_MAX,NATRON_SCENE_MAX) ) );
    _imp->_bR->setPos( _imp->_bR->mapFromScene( QPointF(NATRON_SCENE_MAX,NATRON_SCENE_MIN) ) );
    _imp->_bL->setPos( _imp->_bL->mapFromScene( QPointF(NATRON_SCENE_MIN,NATRON_SCENE_MIN) ) );
    centerOn(0,0);
    setSceneRect(NATRON_SCENE_MIN,NATRON_SCENE_MIN,NATRON_SCENE_MAX,NATRON_SCENE_MAX);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    _imp->_menu = new Menu(this);
    
}

NodeGraph::~NodeGraph()
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();
         it != _imp->_nodes.end();
         ++it) {
        (*it)->discardGraphPointer();
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin();
         it != _imp->_nodesTrash.end();
         ++it) {
        (*it)->discardGraphPointer();
    }

    if (getGui()) {
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

    QObject::disconnect( &_imp->_refreshCacheTextTimer,SIGNAL( timeout() ),this,SLOT( updateCacheSizeText() ) );
    _imp->_nodeCreationShortcutEnabled = false;

}

bool
NodeGraph::isDoingNavigatorRender() const
{
    return _imp->isDoingPreviewRender;
}


const std::list< boost::shared_ptr<NodeGui> > &
NodeGraph::getSelectedNodes() const
{
    return _imp->_selection;
}

boost::shared_ptr<NodeCollection>
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
    boost::shared_ptr<NodeCollection> group = getGroup();
    if (group) {
        group->discardNodeGraphPointer();
    }
}

void
NodeGraph::onNodesCleared()
{
    _imp->_selection.clear();
    std::list<boost::shared_ptr<NodeGui> > nodesCpy;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        nodesCpy = _imp->_nodes;
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodesCpy.begin(); it != nodesCpy.end(); ++it) {
        deleteNodepluginsly( *it );
    }

    while ( !_imp->_nodesTrash.empty() ) {
        deleteNodepluginsly( *( _imp->_nodesTrash.begin() ) );
    }
    _imp->_selection.clear();
    _imp->_magnifiedNode.reset();
    _imp->_nodes.clear();
    _imp->_nodesTrash.clear();
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
    AppInstance* app = 0;
    if (getGui()) {
        app = getGui()->getApp();
    }
    if (app && app->getProject()->isLoadingProjectInternal()) {
        return;
    }
    
    boost::shared_ptr<NodeCollection> collection = getGroup();
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(collection.get());
    bool isGroupEditable = true;
    bool groupEdited = true;
    if (isGroup) {
        isGroupEditable = isGroup->isSubGraphEditable();
        groupEdited = isGroup->getNode()->hasPyPlugBeenEdited();
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
        QPoint navTopLeftWidget = btmRightWidget - QPoint(navWidth,navHeight );
        QPointF navTopLeftScene = mapToScene(navTopLeftWidget);

        _imp->_navigator->refreshPosition(navTopLeftScene,navWidth,navHeight);
        updateNavigator();
        _imp->_refreshOverlays = false;
    }
    QGraphicsView::paintEvent(e);
    
    if (drawLockedMode) {
        ///Show a semi-opaque forground indicating the PyPlug has not been edited
        QPainter p(viewport());
        p.setBrush(QColor(120,120,120));
        p.setOpacity(0.7);
        p.drawRect(rect());
        
        if (isGroupEditable) {
            ///Draw the unlock icon
            QPoint pixPos = _imp->getPyPlugUnlockPos();
            int pixW = _imp->unlockIcon.width();
            int pixH = _imp->unlockIcon.height();
            QRect pixRect(pixPos.x(),pixPos.y(),pixW,pixH);
            pixRect.adjust(-2, -2, 2, 2);
            QRect selRect(pixPos.x(),pixPos.y(),pixW,pixH);
            pixRect.adjust(-3, -3, 3, 3);
            p.setBrush(QColor(243,137,0));
            p.setOpacity(1.);
            p.drawRoundedRect(selRect,5,5);
            p.setBrush(QColor(50,50,50));
            p.drawRoundedRect(pixRect,5,5);
            p.drawPixmap(pixPos.x(), pixPos.y(), pixW, pixH, _imp->unlockIcon, 0, 0, pixW, pixH);
        }
    }
    
    if (_imp->_evtState == eEventStateSelectionRect && !isDoingNavigatorRender()) {
        QPainter p(viewport());
        const QRect r = mapFromScene(_imp->_selectionRect).boundingRect();
        QColor color(16,84,200,20);
        p.setBrush(color);
        p.drawRect(r);
        double w = p.pen().widthF();
        p.fillRect(QRect(r.x() + w,r.y() + w,r.width() - w,r.height() - w),color);
    }
}

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

boost::shared_ptr<NodeGui>
NodeGraph::createNodeGUI(const boost::shared_ptr<Node> & node,
                         const CreateNodeArgs& args)
{
    boost::shared_ptr<NodeGui> node_ui;
    Dot* isDot = dynamic_cast<Dot*>( node->getLiveInstance() );
    Backdrop* isBd = dynamic_cast<Backdrop*>(node->getLiveInstance());
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(node->getLiveInstance());
    
    
    ///prevent multiple render requests while creating node and connecting it
    getGui()->getApp()->setCreatingNode(true);
    
    if (isDot) {
        node_ui.reset( new DotGui(_imp->_nodeRoot) );
    } else if (isBd) {
        node_ui.reset(new BackdropGui(_imp->_nodeRoot));
    } else {
        node_ui.reset( new NodeGui(_imp->_nodeRoot) );
    }
    assert(node_ui);
    node_ui->initialize(this, node);

    if (isBd) {
        BackdropGui* bd = dynamic_cast<BackdropGui*>(node_ui.get());
        assert(bd);
        NodeGuiList selectedNodes = _imp->_selection;
        if ( bd && !selectedNodes.empty() ) {
            ///make the backdrop large enough to contain the selected nodes and position it correctly
            QRectF bbox;
            for (std::list<boost::shared_ptr<NodeGui> >::iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
                QRectF nodeBbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
                bbox = bbox.united(nodeBbox);
            }
            
            double border50 = mapToScene(QPoint(50,0)).x();
            double border0 = mapToScene(QPoint(0,0)).x();
            double border = border50 - border0;
            double headerHeight = bd->getFrameNameHeight();
            QPointF scenePos(bbox.x() - border, bbox.y() - border);
            
            bd->setPos(bd->mapToParent(bd->mapFromScene(scenePos)));
            bd->resize(bbox.width() + 2 * border, bbox.height() + 2 * border - headerHeight);
        }

    }

    
    {
        QMutexLocker l(&_imp->_nodesMutex);
        _imp->_nodes.push_back(node_ui);
    }

    NodeGroup* parentIsGroup = dynamic_cast<NodeGroup*>(node->getGroup().get());;
    
    if (args.reason != eCreateNodeReasonProjectLoad && (!getGui()->getApp()->isCreatingPythonGroup() || dynamic_cast<NodeGroup*>(node->getLiveInstance())) && !parentIsGroup) {
        node_ui->ensurePanelCreated();
    }
    
    getGui()->getApp()->setCreatingNode(false);

    boost::shared_ptr<QUndoStack> nodeStack = node_ui->getUndoStack();
    if (nodeStack) {
        getGui()->registerNewUndoStack(nodeStack.get());
    }
    
    if (args.reason == eCreateNodeReasonUserCreate) {
        pushUndoCommand( new AddMultipleNodesCommand(this,node_ui) );
    } else if (args.reason != eCreateNodeReasonProjectLoad && !isGrp) {
        selectNode(node_ui, false);
    }

    _imp->_evtState = eEventStateNone;

    return node_ui;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_NodeGraph.cpp"
