//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <cstdlib>
#include <set>
#include <map>
#include <vector>
#include <locale>
#include <algorithm> // min, max

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QGraphicsProxyWidget>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QGraphicsTextItem>
#include <QFileSystemModel>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QDialogButtonBox>
#include <QUndoStack>
#include <QToolButton>
#include <QThread>
#include <QDropEvent>
#include <QApplication>
#include <QCheckBox>
#include <QMimeData>
#include <QLineEdit>
#include <QDebug>
#include <QtCore/QRectF>
#include <QRegExp>
#include <QtCore/QTimer>
#include <QAction>
#include <QPainter>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <SequenceParsing.h>

#include "Engine/AppManager.h"
#include "Engine/BackDrop.h"
#include "Engine/Dot.h"
#include "Engine/FrameEntry.h"
#include "Engine/Hash64.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/BackDropGui.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/Edge.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Gui.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/KnobGui.h"
#include "Gui/Label.h"
#include "Gui/Menu.h"
#include "Gui/NodeBackDropSerialization.h"
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ToolButton.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h"

using namespace Natron;
using std::cout; using std::endl;


static void makeFullyQualifiedLabel(Natron::Node* node,std::string* ret)
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
    , ScriptObject()
      , _imp( new NodeGraphPrivate(gui,this, group) )
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
    
    
    
    
    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    //setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    scale(0.8,0.8);

    _imp->_root = new QGraphicsTextItem(0);
    _imp->_nodeRoot = new QGraphicsTextItem(_imp->_root);
    scene->addItem(_imp->_root);

    _imp->_selectionRect = new SelectionRectangle(_imp->_root);
    _imp->_selectionRect->setZValue(1);
    _imp->_selectionRect->hide();

    _imp->_navigator = new Navigator(0);
    scene->addItem(_imp->_navigator);
    _imp->_navigator->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_navigator->hide();


    _imp->_cacheSizeText = new QGraphicsTextItem(0);
    scene->addItem(_imp->_cacheSizeText);
    _imp->_cacheSizeText->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_cacheSizeText->setDefaultTextColor( QColor(200,200,200) );

    QObject::connect( &_imp->_refreshCacheTextTimer,SIGNAL( timeout() ),this,SLOT( updateCacheSizeText() ) );
    _imp->_refreshCacheTextTimer.start(NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS);

    _imp->_undoStack = new QUndoStack(this);
    _imp->_undoStack->setUndoLimit( appPTR->getCurrentSettings()->getMaximumUndoRedoNodeGraph() );
    _imp->_gui->registerNewUndoStack(_imp->_undoStack);

    _imp->_hintInputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_imp->_nodeRoot);
    _imp->_hintInputEdge->setDefaultColor( QColor(0,255,0,100) );
    _imp->_hintInputEdge->hide();

    _imp->_hintOutputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_imp->_nodeRoot);
    _imp->_hintOutputEdge->setDefaultColor( QColor(0,255,0,100) );
    _imp->_hintOutputEdge->hide();

    _imp->_tL = new QGraphicsTextItem(0);
    _imp->_tL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_tL);

    _imp->_tR = new QGraphicsTextItem(0);
    _imp->_tR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_tR);

    _imp->_bR = new QGraphicsTextItem(0);
    _imp->_bR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_bR);

    _imp->_bL = new QGraphicsTextItem(0);
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

    _imp->_menu = new Natron::Menu(this);
    //_imp->_menu->setFont( QFont(appFont,appFontSize) );

    boost::shared_ptr<TimeLine> timeline = _imp->_gui->getApp()->getTimeLine();
    QObject::connect( timeline.get(),SIGNAL( frameChanged(SequenceTime,int) ), this,SLOT( onTimeChanged(SequenceTime,int) ) );
    QObject::connect( timeline.get(),SIGNAL( frameAboutToChange() ), this,SLOT( onTimelineTimeAboutToChange() ) );
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

    if (_imp->_gui) {
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

Gui*
NodeGraph::getGui() const
{
    return _imp->_gui;
}

void
NodeGraph::discardGuiPointer()
{
    _imp->_gui = 0;
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
NodeGraph::createNodeGUI(const boost::shared_ptr<Natron::Node> & node,
                         bool requestedByLoad,
                         double xPosHint,
                         double yPosHint,
                         bool pushUndoRedoCommand,
                         bool autoConnect)
{
    boost::shared_ptr<NodeGui> node_ui;
    Dot* isDot = dynamic_cast<Dot*>( node->getLiveInstance() );
    BackDrop* isBd = dynamic_cast<BackDrop*>(node->getLiveInstance());
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(node->getLiveInstance());
    
    
    ///prevent multiple render requests while creating node and connecting it
    getGui()->getApp()->setCreatingNode(true);
    
    if (isDot) {
        node_ui.reset( new DotGui(_imp->_nodeRoot) );
    } else if (isBd) {
        node_ui.reset(new BackDropGui(_imp->_nodeRoot));
    } else {
        node_ui.reset( new NodeGui(_imp->_nodeRoot) );
    }
    assert(node_ui);
    node_ui->initialize(this, node);

    if (isBd) {
        BackDropGui* bd = dynamic_cast<BackDropGui*>(node_ui.get());
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
    ///only move main instances
    if ( node->getParentMultiInstanceName().empty() ) {
        if (_imp->_selection.empty()) {
            autoConnect = false;
        }
        if ( (xPosHint != INT_MIN) && (yPosHint != INT_MIN) && !autoConnect ) {
            QPointF pos = node_ui->mapToParent( node_ui->mapFromScene( QPointF(xPosHint,yPosHint) ) );
            node_ui->refreshPosition( pos.x(),pos.y(), true );
        } else {
            if (!isBd && !isGrp) {
                moveNodesForIdealPosition(node_ui,autoConnect);
            }
        }
    }
    
    if (!requestedByLoad && (!getGui()->getApp()->isCreatingPythonGroup() || dynamic_cast<NodeGroup*>(node->getLiveInstance()))) {
        node_ui->ensurePanelCreated();
    }
    
    getGui()->getApp()->setCreatingNode(false);

    boost::shared_ptr<QUndoStack> nodeStack = node_ui->getUndoStack();
    if (nodeStack) {
        _imp->_gui->registerNewUndoStack(nodeStack.get());
    }
    
    if (pushUndoRedoCommand) {
        pushUndoCommand( new AddMultipleNodesCommand(this,node_ui) );
    } else if (!requestedByLoad) {
        if (!isGrp) {
            selectNode(node_ui, false);
        }
    }

    _imp->_evtState = eEventStateNone;

    return node_ui;
}

void
NodeGraph::moveNodesForIdealPosition(boost::shared_ptr<NodeGui> node,bool autoConnect)
{
    BackDropGui* isBd = dynamic_cast<BackDropGui*>(node.get());
    if (isBd) {
        return;
    }
    
    QRectF viewPos = visibleSceneRect();

    ///3 possible values:
    /// 0 = default , i.e: we pop the node in the middle of the graph's current view
    /// 1 = pop the node above the selected node and move the inputs of the selected node a little
    /// 2 = pop the node below the selected node and move the outputs of the selected node a little
    int behavior = 0;
    boost::shared_ptr<NodeGui> selected;

    if (_imp->_selection.size() == 1) {
        selected = _imp->_selection.front();
        BackDropGui* isBd = dynamic_cast<BackDropGui*>(selected.get());
        if (isBd) {
            selected.reset();
        }
    }

    if (!selected || !autoConnect) {
        behavior = 0;
    } else {
        ///this function is redundant with Project::autoConnect, depending on the node selected
        ///and this node we make some assumptions on to where we could put the node.

        //        1) selected is output
        //          a) created is output --> fail
        //          b) created is input --> connect input
        //          c) created is regular --> connect input
        //        2) selected is input
        //          a) created is output --> connect output
        //          b) created is input --> fail
        //          c) created is regular --> connect output
        //        3) selected is regular
        //          a) created is output--> connect output
        //          b) created is input --> connect input
        //          c) created is regular --> connect output

        ///1)
        if ( selected->getNode()->isOutputNode() ) {
            ///case 1-a) just do default we don't know what else to do
            if ( node->getNode()->isOutputNode() ) {
                behavior = 0;
            } else {
                ///for either cases 1-b) or 1-c) we just connect the created node as input of the selected node.
                behavior = 1;
            }
        }
        ///2) and 3) are similar except for case b)
        else {
            ///case 2 or 3- a): connect the created node as output of the selected node.
            if ( node->getNode()->isOutputNode() ) {
                behavior = 2;
            }
            ///case b)
            else if (node->getNode()->isInputNode()) {
                if (selected->getNode()->getLiveInstance()->isReader()) {
                    ///case 2-b) just do default we don't know what else to do
                    behavior = 0;
                } else {
                    ///case 3-b): connect the created node as input of the selected node
                    behavior = 1;
                }
            }
            ///case c) connect created as output of the selected node
            else {
                behavior = 2;
            }
        }
    }
    
    boost::shared_ptr<Natron::Node> createdNodeInternal = node->getNode();
    boost::shared_ptr<Natron::Node> selectedNodeInternal;
    if (selected) {
       selectedNodeInternal = selected->getNode();
    }


    ///if behaviour is 1 , just check that we can effectively connect the node to avoid moving them for nothing
    ///otherwise fallback on behaviour 0
    if (behavior == 1) {
        const std::vector<boost::shared_ptr<Natron::Node> > & inputs = selected->getNode()->getGuiInputs();
        bool oneInputEmpty = false;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (!inputs[i]) {
                oneInputEmpty = true;
                break;
            }
        }
        if (!oneInputEmpty) {
            behavior = 0;
        }
    }
    
   
    boost::shared_ptr<Natron::Project> proj = getGui()->getApp()->getProject();


    ///default
    QPointF position;
    if (behavior == 0) {
        position.setX( ( viewPos.bottomRight().x() + viewPos.topLeft().x() ) / 2. );
        position.setY( ( viewPos.topLeft().y() + viewPos.bottomRight().y() ) / 2. );
    }
    ///pop it above the selected node
    else if (behavior == 1) {
        
        ///If this is the first connected input, insert it in a "linear" way so the tree remains vertical
        int nbConnectedInput = 0;
        
        const std::vector<Edge*> & selectedNodeInputs = selected->getInputsArrows();
        for (std::vector<Edge*>::const_iterator it = selectedNodeInputs.begin() ; it != selectedNodeInputs.end() ; ++it) {
            boost::shared_ptr<NodeGui> input;
            if (*it) {
                input = (*it)->getSource();
            }
            if (input) {
                ++nbConnectedInput;
            }
        }
        
        ///connect it to the first input
        QSize selectedNodeSize = selected->getSize();
        QSize createdNodeSize = node->getSize();

        
        if (nbConnectedInput == 0) {
            
            QPointF selectedNodeMiddlePos = selected->scenePos() +
            QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);
            
            
            position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
            position.setY( selectedNodeMiddlePos.y() - selectedNodeSize.height() / 2 - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES
                          - createdNodeSize.height() );
            
            QRectF createdNodeRect( position.x(),position.y(),createdNodeSize.width(),createdNodeSize.height() );
            
            ///now that we have the position of the node, move the inputs of the selected node to make some space for this node
            
            for (std::vector<Edge*>::const_iterator it = selectedNodeInputs.begin(); it != selectedNodeInputs.end(); ++it) {
                if ( (*it)->hasSource() ) {
                    (*it)->getSource()->moveAbovePositionRecursively(createdNodeRect);
                }
            }
            
            int selectedInput = selectedNodeInternal->getPreferredInputForConnection();
            if (selectedInput != -1) {
                bool ok = proj->connectNodes(selectedInput, createdNodeInternal, selectedNodeInternal.get(),true);
                assert(ok);
            }
            
        } else {
            
            ViewerInstance* isSelectedViewer = dynamic_cast<ViewerInstance*>(selectedNodeInternal->getLiveInstance());
            if (isSelectedViewer) {
                //Don't pop a dot, it will most likely annoy the user, just fallback on behavior 0
                position.setX( ( viewPos.bottomRight().x() + viewPos.topLeft().x() ) / 2. );
                position.setY( ( viewPos.topLeft().y() + viewPos.bottomRight().y() ) / 2. );
            } else {
                
                QRectF selectedBbox = selected->mapToScene(selected->boundingRect()).boundingRect();
                QPointF selectedCenter = selectedBbox.center();
                
                double y = selectedCenter.y() - selectedNodeSize.height() / 2.
                - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES - createdNodeSize.height();
                double x = selectedCenter.x() + nbConnectedInput * 150;
                
                position.setX(x  - createdNodeSize.width() / 2.);
                position.setY(y);
                
                int index = selectedNodeInternal->getPreferredInputForConnection();
                if (index != -1) {
                    
                    ///Create a dot to make things nicer
                    CreateNodeArgs args(PLUGINID_NATRON_DOT,
                                        std::string(),
                                        -1,
                                        -1,
                                        false, //< don't autoconnect
                                        INT_MIN,
                                        INT_MIN,
                                        true,
                                        true,
                                        true,
                                        QString(),
                                        CreateNodeArgs::DefaultValuesList(),
                                        createdNodeInternal->getGroup());
                    boost::shared_ptr<Natron::Node> dotNode = _imp->_gui->getApp()->createNode(args);
                    assert(dotNode);
                    boost::shared_ptr<NodeGuiI> dotNodeGui_i = dotNode->getNodeGui();
                    assert(dotNodeGui_i);
                    NodeGui* dotGui = dynamic_cast<NodeGui*>(dotNodeGui_i.get());
                    assert(dotGui);
                    
                    double dotW,dotH;
                    dotGui->getSize(&dotW,&dotH);
                    QPointF dotPos(x - dotW / 2., selectedCenter.y() - dotH / 2.);
                    dotPos = dotGui->mapToParent(dotGui->mapFromScene(dotPos));
                    dotNodeGui_i->setPosition(dotPos.x(),dotPos.y());
                    
                    ///connect the nodes
                    
                    int index = selectedNodeInternal->getPreferredInputForConnection();
                    
                    bool ok = proj->connectNodes(index, dotNode, selectedNodeInternal.get(), true);
                    assert(ok);
                    
                    proj->connectNodes(0, createdNodeInternal, dotNode.get());
                    
                }
            } // if (isSelectedViewer) {
        } // if (nbConnectedInput == 0) {
    }
    ///pop it below the selected node
    else {

        const std::list<Natron::Node*>& outputs = selectedNodeInternal->getGuiOutputs();
        if (!createdNodeInternal->isOutputNode() || outputs.empty()) {
            QSize selectedNodeSize = selected->getSize();
            QSize createdNodeSize = node->getSize();
            QPointF selectedNodeMiddlePos = selected->scenePos() +
            QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);
            
            ///actually move the created node where the selected node is
            position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
            position.setY(selectedNodeMiddlePos.y() + (selectedNodeSize.height() / 2) + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
            
            QRectF createdNodeRect( position.x(),position.y(),createdNodeSize.width(),createdNodeSize.height() );
            
            ///and move the selected node below recusively
            const std::list<Natron::Node* > & outputs = selected->getNode()->getGuiOutputs();
            for (std::list<Natron::Node* >::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
                assert(*it);
                boost::shared_ptr<NodeGuiI> output_i = (*it)->getNodeGui();
                if (!output_i) {
                    continue;
                }
                NodeGui* output = dynamic_cast<NodeGui*>(output_i.get());
                assert(output);
                if (output) {
                    output->moveBelowPositionRecursively(createdNodeRect);
                }
            }
            
            if ( !createdNodeInternal->isOutputNode() ) {
                ///we find all the nodes that were previously connected to the selected node,
                ///and connect them to the created node instead.
                std::map<Natron::Node*,int> outputsConnectedToSelectedNode;
                selectedNodeInternal->getOutputsConnectedToThisNode(&outputsConnectedToSelectedNode);
                for (std::map<Natron::Node*,int>::iterator it = outputsConnectedToSelectedNode.begin();
                     it != outputsConnectedToSelectedNode.end(); ++it) {
                    if (it->first->getParentMultiInstanceName().empty()) {
                        bool ok = proj->disconnectNodes(selectedNodeInternal.get(), it->first);
                        if (ok) {
                            ignore_result(proj->connectNodes(it->second, createdNodeInternal, it->first));
                        }
                        //assert(ok); Might not be ok if the disconnectNodes() action above was queued
                    }
                }
            }
            
            ///finally we connect the created node to the selected node
            int createdInput = createdNodeInternal->getPreferredInputForConnection();
            if (createdInput != -1) {
                bool ok = proj->connectNodes(createdInput, selectedNodeInternal, createdNodeInternal.get());
                assert(ok);
            }
            
        } else {
            ///the created node is an output node and the selected node already has several outputs, create it aside
            QSize createdNodeSize = node->getSize();
            QRectF selectedBbox = selected->mapToScene(selected->boundingRect()).boundingRect();
            QPointF selectedCenter = selectedBbox.center();
            
            double y = selectedCenter.y() + selectedBbox.height() / 2.
            + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES;
            double x = selectedCenter.x() + (int)outputs.size() * 150;
            
            position.setX(x  - createdNodeSize.width() / 2.);
            position.setY(y);
            
            int index = createdNodeInternal->getPreferredInputForConnection();
            if (index != -1) {
                ///Create a dot to make things nicer
                CreateNodeArgs args(PLUGINID_NATRON_DOT,
                                    std::string(),
                                    -1,
                                    -1,
                                    false, //< don't autoconnect
                                    INT_MIN,
                                    INT_MIN,
                                    true,
                                    true,
                                    true,
                                    QString(),
                                    CreateNodeArgs::DefaultValuesList(),
                                    createdNodeInternal->getGroup());
                boost::shared_ptr<Natron::Node> dotNode = _imp->_gui->getApp()->createNode(args);
                assert(dotNode);
                boost::shared_ptr<NodeGuiI> dotNodeGui_i = dotNode->getNodeGui();
                assert(dotNodeGui_i);
                NodeGui* dotGui = dynamic_cast<NodeGui*>(dotNodeGui_i.get());
                assert(dotGui);
                
                double dotW,dotH;
                dotGui->getSize(&dotW,&dotH);
                QPointF dotPos(x - dotW / 2., selectedCenter.y() - dotH / 2.);
                dotPos = dotGui->mapToParent(dotGui->mapFromScene(dotPos));
                dotNodeGui_i->setPosition(dotPos.x(),dotPos.y());
    
                
                ///connect the nodes
                
                int index = createdNodeInternal->getPreferredInputForConnection();
                
                bool ok = proj->connectNodes(index, dotNode, createdNodeInternal.get(), true);
                assert(ok);
                
                proj->connectNodes(0, selectedNodeInternal, dotNode.get());
                
            }
        }
        
        
    }
    position = node->mapFromScene(position);
    position = node->mapToParent(position);
    node->setPosition( position.x(), position.y() );
} // moveNodesForIdealPosition

void
NodeGraph::mousePressEvent(QMouseEvent* e)
{
    assert(e);
    _imp->_hasMovedOnce = false;
    _imp->_deltaSinceMousePress = QPointF(0,0);
    if ( buttonDownIsMiddle(e) ) {
        _imp->_evtState = eEventStateMovingArea;

        return;
    }

    bool didSomething = false;

    _imp->_lastMousePos = e->pos();
    QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos.x(),_imp->_lastMousePos.y());

    if (((e->buttons() & Qt::MiddleButton) &&
         (buttonMetaAlt(e) == Qt::AltModifier || (e->buttons() & Qt::LeftButton))) ||
        ((e->buttons() & Qt::LeftButton) &&
         (buttonMetaAlt(e) == (Qt::AltModifier|Qt::MetaModifier)))) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->_evtState = eEventStateZoomingArea;
        return;
    }
    
    NodeGuiPtr selected;
    Edge* selectedEdge = 0;
    Edge* selectedBendPoint = 0;
    {
        
        QMutexLocker l(&_imp->_nodesMutex);
        
        ///Find matches, sorted by depth
        std::map<double,NodeGuiPtr> matches;
        for (NodeGuiList::reverse_iterator it = _imp->_nodes.rbegin(); it != _imp->_nodes.rend(); ++it) {
            QPointF evpt = (*it)->mapFromScene(lastMousePosScene);
            if ( (*it)->isVisible() && (*it)->isActive() ) {
                
                BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
                if (isBd) {
                    if (isBd->isNearbyNameFrame(evpt)) {
                        matches.insert(std::make_pair((*it)->zValue(),*it));
                    } else if (isBd->isNearbyResizeHandle(evpt)) {
                        selected = *it;
                        _imp->_backdropResized = *it;
                        _imp->_evtState = eEventStateResizingBackdrop;
                        break;
                    }
                } else {
                    if ((*it)->contains(evpt)) {
                        matches.insert(std::make_pair((*it)->zValue(),*it));
                    }
                }
                
            }
        }
        if (!matches.empty() && _imp->_evtState != eEventStateResizingBackdrop) {
            selected = matches.rbegin()->second;
        }
        if (!selected) {
            ///try to find a selected edge
            for (NodeGuiList::reverse_iterator it = _imp->_nodes.rbegin(); it != _imp->_nodes.rend(); ++it) {
                Edge* bendPointEdge = (*it)->hasBendPointNearbyPoint(lastMousePosScene);

                if (bendPointEdge) {
                    selectedBendPoint = bendPointEdge;
                    break;
                }
                Edge* edge = (*it)->hasEdgeNearbyPoint(lastMousePosScene);
                if (edge) {
                    selectedEdge = edge;
                }
                
            }
        }
    }
    
    if (selected) {
        didSomething = true;
        if ( buttonDownIsLeft(e) ) {
            
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(selected.get());
            if (!isBd) {
                _imp->_magnifiedNode = selected;
            }
            
            if ( !selected->getIsSelected() ) {
                selectNode( selected, modCASIsShift(e) );
            } else if ( modCASIsShift(e) ) {
                NodeGuiList::iterator it = std::find(_imp->_selection.begin(),
                                                     _imp->_selection.end(),selected);
                if ( it != _imp->_selection.end() ) {
                    (*it)->setUserSelected(false);
                    _imp->_selection.erase(it);
                }
            }
            if (_imp->_evtState != eEventStateResizingBackdrop) {
                _imp->_evtState = eEventStateDraggingNode;
            }
            ///build the _nodesWithinBDAtPenDown map
            _imp->_nodesWithinBDAtPenDown.clear();
            for (NodeGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
                BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
                if (isBd) {
                    NodeGuiList nodesWithin = getNodesWithinBackDrop(*it);
                    _imp->_nodesWithinBDAtPenDown.insert(std::make_pair(*it,nodesWithin));
                }
            }

            _imp->_lastNodeDragStartPoint = selected->getPos_mt_safe();
        } else if ( buttonDownIsRight(e) ) {
            if ( !selected->getIsSelected() ) {
                selectNode(selected,true); ///< don't wipe the selection
            }
        }
    } else if (selectedBendPoint) {
        _imp->setNodesBendPointsVisible(false);
        
        CreateNodeArgs args(PLUGINID_NATRON_DOT,
                            std::string(),
                            -1,
                            -1,
                            false, //< don't autoconnect
                            INT_MIN,
                            INT_MIN,
                            false, //<< don't push an undo command
                            true,
                            true,
                            QString(),
                            CreateNodeArgs::DefaultValuesList(),
                            _imp->group.lock());
        boost::shared_ptr<Natron::Node> dotNode = _imp->_gui->getApp()->createNode(args);
        assert(dotNode);
        boost::shared_ptr<NodeGuiI> dotNodeGui_i = dotNode->getNodeGui();
        boost::shared_ptr<NodeGui> dotNodeGui = boost::dynamic_pointer_cast<NodeGui>(dotNodeGui_i);
        assert(dotNodeGui);
        
        std::list<boost::shared_ptr<NodeGui> > nodesList;
        nodesList.push_back(dotNodeGui);
        
        ///Now connect the node to the edge input
        boost::shared_ptr<Natron::Node> inputNode = selectedBendPoint->getSource()->getNode();
        assert(inputNode);
        ///disconnect previous connection
        boost::shared_ptr<Natron::Node> outputNode = selectedBendPoint->getDest()->getNode();
        assert(outputNode);
        
        int inputNb = outputNode->inputIndex( inputNode.get() );
        assert(inputNb != -1);
        bool ok = _imp->_gui->getApp()->getProject()->disconnectNodes(inputNode.get(), outputNode.get());
        assert(ok);
        
        ok = _imp->_gui->getApp()->getProject()->connectNodes(0, inputNode, dotNode.get());
        assert(ok);
        
        _imp->_gui->getApp()->getProject()->connectNodes(inputNb,dotNode,outputNode.get());
        
        QPointF pos = dotNodeGui->mapToParent( dotNodeGui->mapFromScene(lastMousePosScene) );

        dotNodeGui->refreshPosition( pos.x(), pos.y() );
        if ( !dotNodeGui->getIsSelected() ) {
            selectNode( dotNodeGui, modCASIsShift(e) );
        }
        pushUndoCommand( new AddMultipleNodesCommand( this,nodesList) );
        
        
        _imp->_evtState = eEventStateDraggingNode;
        _imp->_lastNodeDragStartPoint = dotNodeGui->getPos_mt_safe();
        didSomething = true;
    } else if (selectedEdge) {
        _imp->_arrowSelected = selectedEdge;
        didSomething = true;
        _imp->_evtState = eEventStateDraggingArrow;
    }
    
    ///Test if mouse is inside the navigator
    {
        QPointF mousePosSceneCoordinates;
        bool insideNavigator = isNearbyNavigator(e->pos(), mousePosSceneCoordinates);
        if (insideNavigator) {
            updateNavigator();
            _imp->_refreshOverlays = true;
            centerOn(mousePosSceneCoordinates);
            _imp->_evtState = eEventStateDraggingNavigator;
            didSomething = true;
        }
    }

    ///Don't forget to reset back to null the _backdropResized pointer
    if (_imp->_evtState != eEventStateResizingBackdrop) {
        _imp->_backdropResized.reset();
    }

    if ( buttonDownIsRight(e) ) {
        showMenu( mapToGlobal( e->pos() ) );
        didSomething = true;
    }
    if (!didSomething) {
        if ( buttonDownIsLeft(e) ) {
            if ( !modCASIsShift(e) ) {
                deselect();
            }
            _imp->_evtState = eEventStateSelectionRect;
            _imp->_lastSelectionStartPoint = _imp->_lastMousePos;
            QPointF clickPos = _imp->_selectionRect->mapFromScene(lastMousePosScene);
            _imp->_selectionRect->setRect(clickPos.x(), clickPos.y(), 0, 0);
            //_imp->_selectionRect->show();
        } else if ( buttonDownIsMiddle(e) ) {
            _imp->_evtState = eEventStateMovingArea;
            QGraphicsView::mousePressEvent(e);
        }
    }
} // mousePressEvent

bool
NodeGraph::isNearbyNavigator(const QPoint& widgetPos,QPointF& scenePos) const
{
    if (!_imp->_navigator->isVisible()) {
        return false;
    }
    
    QRect visibleWidget = visibleWidgetRect();
    
    int navWidth = std::ceil(width() * NATRON_NAVIGATOR_BASE_WIDTH);
    int navHeight = std::ceil(height() * NATRON_NAVIGATOR_BASE_HEIGHT);

    QPoint btmRightWidget = visibleWidget.bottomRight();
    QPoint navTopLeftWidget = btmRightWidget - QPoint(navWidth,navHeight );
    
    if (widgetPos.x() >= navTopLeftWidget.x() && widgetPos.x() < btmRightWidget.x() &&
        widgetPos.y() >= navTopLeftWidget.y() && widgetPos.y() <= btmRightWidget.y()) {
        
        ///The bbox of all nodes in the nodegraph
        QRectF sceneR = _imp->calcNodesBoundingRect();
        
        ///The visible portion of the nodegraph
        QRectF viewRect = visibleSceneRect();
        sceneR = sceneR.united(viewRect);
        
        ///Make sceneR and viewRect keep the same aspect ratio as the navigator
        double xScale = navWidth / sceneR.width();
        double yScale =  navHeight / sceneR.height();
        double scaleFactor = std::max(0.001,std::min(xScale,yScale));

        ///Make the widgetPos relative to the navTopLeftWidget
        QPoint clickNavPos(widgetPos.x() - navTopLeftWidget.x(), widgetPos.y() - navTopLeftWidget.y());
        
        scenePos.rx() = clickNavPos.x() / scaleFactor;
        scenePos.ry() = clickNavPos.y() / scaleFactor;
        
        ///Now scenePos is in scene coordinates, but relative to the sceneR top left
        scenePos.rx() += sceneR.x();
        scenePos.ry() += sceneR.y();
        return true;
    }
    
    return false;
    
}

void
NodeGraph::pushUndoCommand(QUndoCommand* command)
{
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(command);
}

bool
NodeGraph::areOptionalInputsAutoHidden() const
{
    return appPTR->getCurrentSettings()->areOptionalInputsAutoHidden();
}

void
NodeGraph::deselect()
{
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            (*it)->setUserSelected(false);
        }
    }
  
    _imp->_selection.clear();

    if (_imp->_magnifiedNode && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_magnifiedNode->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}

void
NodeGraph::mouseReleaseEvent(QMouseEvent* e)
{
    EventStateEnum state = _imp->_evtState;
    
    _imp->_evtState = eEventStateNone;
    
    bool hasMovedOnce = modCASIsControl(e) || _imp->_hasMovedOnce;
    if (state == eEventStateDraggingArrow && hasMovedOnce) {
        
        QRectF sceneR = visibleSceneRect();
        
        bool foundSrc = false;
        assert(_imp->_arrowSelected);
        boost::shared_ptr<NodeGui> nodeHoldingEdge = _imp->_arrowSelected->isOutputEdge() ?
                                                     _imp->_arrowSelected->getSource() : _imp->_arrowSelected->getDest();
        assert(nodeHoldingEdge);
        
        std::list<boost::shared_ptr<NodeGui> > nodes = getAllActiveNodes_mt_safe();
        QPointF ep = mapToScene( e->pos() );
        
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            boost::shared_ptr<NodeGui> & n = *it;
            
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(n.get());
            if (isBd) {
                continue;
            }
            
            QRectF bbox = n->mapToScene(n->boundingRect()).boundingRect();
            
            if (n->isActive() && n->isVisible() && bbox.intersects(sceneR) &&
                n->isNearby(ep) &&
                n->getNode()->getScriptName() != nodeHoldingEdge->getNode()->getScriptName()) {
                
                if ( !_imp->_arrowSelected->isOutputEdge() ) {
                    
                    Natron::Node::CanConnectInputReturnValue linkRetCode =
                    nodeHoldingEdge->getNode()->canConnectInput(n->getNode(), _imp->_arrowSelected->getInputNumber());
                    if (linkRetCode != Natron::Node::eCanConnectInput_ok && linkRetCode != Natron::Node::eCanConnectInput_inputAlreadyConnected) {
                        if (linkRetCode == Natron::Node::eCanConnectInput_differentPars) {
                            
                            QString error = QString(tr("You cannot connect ") +  "%1" + tr(" to ") + "%2"  + tr(" because they don't have the same pixel aspect ratio (")
                                                    + "%3 / %4 " +  tr(") and ") + "%1 " + " doesn't support inputs with different pixel aspect ratio.")
                            .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                            .arg(n->getNode()->getLabel().c_str())
                            .arg(nodeHoldingEdge->getNode()->getLiveInstance()->getPreferredAspectRatio())
                            .arg(n->getNode()->getLiveInstance()->getPreferredAspectRatio());
                            Natron::errorDialog(tr("Different pixel aspect").toStdString(),
                                                error.toStdString());
                        } else if (linkRetCode == Natron::Node::eCanConnectInput_differentFPS) {

                            QString error = QString(tr("You cannot connect ") +  "%1" + tr(" to ") + "%2"  + tr(" because they don't have the same frame rate (") + "%3 / %4). Either change the FPS from the Read node parameters or change the settings of the project.")
                            .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                            .arg(n->getNode()->getLabel().c_str())
                            .arg(nodeHoldingEdge->getNode()->getLiveInstance()->getPreferredFrameRate())
                            .arg(n->getNode()->getLiveInstance()->getPreferredFrameRate());
                            Natron::errorDialog(tr("Different frame rate").toStdString(),
                                                error.toStdString());

                        } else if (linkRetCode == Natron::Node::eCanConnectInput_groupHasNoOutput) {
                            QString error = QString(tr("You cannot connect ") + "%1 " + tr(" to ") + " %2 " + tr("because it is a group which does "
                                                                                                                 "not have an Output node."))
                            .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                            .arg(n->getNode()->getLabel().c_str());
                            Natron::errorDialog(tr("Different frame rate").toStdString(),
                                                error.toStdString());

                        }
                        break;
                    }
         
                    if (linkRetCode == Natron::Node::eCanConnectInput_ok && nodeHoldingEdge->getNode()->getLiveInstance()->isReader() &&
                        n->getNode()->getPluginID() != PLUGINID_OFX_RUNSCRIPT) {
                        Natron::warningDialog(tr("Reader input").toStdString(), tr("Connecting an input to a Reader node "
                                                                                   "is only useful when using the RunScript node "
                                                                                   "so that the Reader automatically reads an image "
                                                                                   "when the render of the RunScript is done.").toStdString());
                    }
                    _imp->_arrowSelected->stackBefore( n.get() );
                    pushUndoCommand( new ConnectCommand(this,_imp->_arrowSelected,_imp->_arrowSelected->getSource(),n) );
                } else {
                    ///Find the input edge of the node we just released the mouse over,
                    ///and use that edge to connect to the source of the selected edge.
                    int preferredInput = n->getNode()->getPreferredInputForConnection();
                    if (preferredInput != -1) {
                        
                        Natron::Node::CanConnectInputReturnValue linkRetCode = n->getNode()->canConnectInput(nodeHoldingEdge->getNode(), preferredInput);
                        if (linkRetCode != Natron::Node::eCanConnectInput_ok  && linkRetCode != Natron::Node::eCanConnectInput_inputAlreadyConnected) {
                            
                            if (linkRetCode == Natron::Node::eCanConnectInput_differentPars) {
                                
                                QString error = QString(tr("You cannot connect ") +  "%1" + " to " + "%2"  + tr(" because they don't have the same pixel aspect ratio (")
                                                        + "%3 / %4 " +  tr(") and ") + "%1 " + " doesn't support inputs with different pixel aspect ratio.")
                                .arg(n->getNode()->getLabel().c_str())
                                .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                                .arg(n->getNode()->getLiveInstance()->getPreferredAspectRatio())
                                .arg(nodeHoldingEdge->getNode()->getLiveInstance()->getPreferredAspectRatio());
                                Natron::errorDialog(tr("Different pixel aspect").toStdString(),
                                                    error.toStdString());
                            } else if (linkRetCode == Natron::Node::eCanConnectInput_differentFPS) {

                                QString error = QString(tr("You cannot connect ") +  "%1" + " to " + "%2"  + tr(" because they don't have the same frame rate (") + "%3 / %4). Either change the FPS from the Read node parameters or change the settings of the project.")
                                .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                                .arg(n->getNode()->getLabel().c_str())
                                .arg(nodeHoldingEdge->getNode()->getLiveInstance()->getPreferredFrameRate())
                                .arg(n->getNode()->getLiveInstance()->getPreferredFrameRate());
                                Natron::errorDialog(tr("Different frame rate").toStdString(),
                                                    error.toStdString());

                            } else if (linkRetCode == Natron::Node::eCanConnectInput_groupHasNoOutput) {
                                QString error = QString(tr("You cannot connect ") + "%1 " + tr(" to ") + " %2 " + tr("because it is a group which does "
                                                                                                                     "not have an Output node."))
                                .arg(nodeHoldingEdge->getNode()->getLabel().c_str())
                                .arg(n->getNode()->getLabel().c_str());
                                Natron::errorDialog(tr("Different frame rate").toStdString(),
                                                    error.toStdString());
                                
                            }


                            
                            break;
                        }
                        if (linkRetCode == Natron::Node::eCanConnectInput_ok && n->getNode()->getLiveInstance()->isReader() &&
                            nodeHoldingEdge->getNode()->getPluginID() != PLUGINID_OFX_RUNSCRIPT) {
                            Natron::warningDialog(tr("Reader input").toStdString(), tr("Connecting an input to a Reader node "
                                                                                       "is only useful when using the RunScript node "
                                                                                       "so that the Reader automatically reads an image "
                                                                                       "when the render of the RunScript is done.").toStdString());
                        }
                        Edge* foundInput = n->getInputArrow(preferredInput);
                        assert(foundInput);
                        pushUndoCommand( new ConnectCommand( this,foundInput,
                                                                  foundInput->getSource(),_imp->_arrowSelected->getSource() ) );
                    }
                }
                foundSrc = true;
                
                break;
            }
        }
        ///if we disconnected the input edge, use the undo/redo stack.
        ///Output edges can never be really connected, they're just there
        ///So the user understands some nodes can have output
        if ( !foundSrc && !_imp->_arrowSelected->isOutputEdge() && _imp->_arrowSelected->getSource() ) {
            pushUndoCommand( new ConnectCommand( this,_imp->_arrowSelected,_imp->_arrowSelected->getSource(),
                                                      boost::shared_ptr<NodeGui>() ) );
        }
        
        
        
        nodeHoldingEdge->refreshEdges();
        scene()->update();
    } else if (state == eEventStateDraggingNode) {
        if ( !_imp->_selection.empty() ) {
            
            std::list<NodeGuiPtr> nodesToMove;
            for (std::list<NodeGuiPtr>::iterator it = _imp->_selection.begin();
                 it != _imp->_selection.end(); ++it) {
                
                const NodeGuiPtr& node = *it;
                nodesToMove.push_back(node);
                
                std::map<NodeGuiPtr,NodeGuiList>::iterator foundBd = _imp->_nodesWithinBDAtPenDown.find(*it);
                if (!modCASIsControl(e) && foundBd != _imp->_nodesWithinBDAtPenDown.end()) {
                    for (NodeGuiList::iterator it2 = foundBd->second.begin();
                         it2 != foundBd->second.end(); ++it2) {
                        ///add it only if it's not already in the list
                        bool found = false;
                        for (std::list<NodeGuiPtr>::iterator it3 = nodesToMove.begin();
                             it3 != nodesToMove.end(); ++it3) {
                            if (*it3 == *it2) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            nodesToMove.push_back(*it2);
                        }
                    }
                    
                }
            }
            if (_imp->_deltaSinceMousePress.x() != 0 || _imp->_deltaSinceMousePress.y() != 0) {
                pushUndoCommand(new MoveMultipleNodesCommand(nodesToMove,_imp->_deltaSinceMousePress.x(),_imp->_deltaSinceMousePress.y()));
            }
            
            ///now if there was a hint displayed, use it to actually make connections.
            if (_imp->_highLightedEdge) {
                
                _imp->_highLightedEdge->setUseHighlight(false);

                boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.front();
                
                _imp->_highLightedEdge->setUseHighlight(false);
                if ( _imp->_highLightedEdge->isOutputEdge() ) {
                    int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                    if (prefInput != -1) {
                        Edge* inputEdge = selectedNode->getInputArrow(prefInput);
                        assert(inputEdge);
                        pushUndoCommand( new ConnectCommand( this,inputEdge,inputEdge->getSource(),
                                                                    _imp->_highLightedEdge->getSource() ) );
                    }
                } else {
                    boost::shared_ptr<NodeGui> src = _imp->_highLightedEdge->getSource();
                    pushUndoCommand( new ConnectCommand(this,_imp->_highLightedEdge,_imp->_highLightedEdge->getSource(),
                                                               selectedNode) );

                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> > & inpNodes = selectedNode->getNode()->getGuiInputs();
                    if (src) {
                        for (U32 i = 0; i < inpNodes.size(); ++i) {
                            if ( inpNodes[i] == src->getNode() ) {
                                alreadyConnected = true;
                                break;
                            }
                        }
                    }

                    if (src && !alreadyConnected) {
                        ///push a second command... this is a bit dirty but I don't have time to add a whole new command just for this
                        int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                        if (prefInput != -1) {
                            Edge* inputEdge = selectedNode->getInputArrow(prefInput);
                            assert(inputEdge);
                            pushUndoCommand( new ConnectCommand(this,inputEdge,inputEdge->getSource(),src) );
                        }
                    }
                }

                _imp->_highLightedEdge = 0;
                _imp->_hintInputEdge->hide();
                _imp->_hintOutputEdge->hide();
            } else if (_imp->_mergeHintNode) {
                _imp->_mergeHintNode->setMergeHintActive(false);
                boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.front();
                selectedNode->setMergeHintActive(false);
                
                if (getGui()) {
                    
                    QRectF selectedNodeBbox = selectedNode->mapToScene(selectedNode->boundingRect()).boundingRect();
                    QRectF mergeHintNodeBbox = _imp->_mergeHintNode->mapToScene(_imp->_mergeHintNode->boundingRect()).boundingRect();
                    QPointF mergeHintCenter = mergeHintNodeBbox.center();
                    
                    ///Place the selected node on the right of the hint node
                    selectedNode->setPosition(mergeHintCenter.x() + mergeHintNodeBbox.width() / 2. + NATRON_NODE_DUPLICATE_X_OFFSET,
                                              mergeHintCenter.y() - selectedNodeBbox.height() / 2.);
                    
                    selectedNodeBbox = selectedNode->mapToScene(selectedNode->boundingRect()).boundingRect();
                    
                    QPointF selectedNodeCenter = selectedNodeBbox.center();
                    ///Place the new merge node exactly in the middle of the 2, with an Y offset
                    QPointF newNodePos((mergeHintCenter.x() + selectedNodeCenter.x()) / 2. - 40,
                                       std::max((mergeHintCenter.y() + mergeHintNodeBbox.height() / 2.),
                                                selectedNodeCenter.y() + selectedNodeBbox.height() / 2.) + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
                    
                    
                    CreateNodeArgs args(PLUGINID_OFX_MERGE,
                                        "",
                                        -1,
                                        -1,
                                        false,
                                        newNodePos.x(),newNodePos.y(),
                                        true,
                                        true,
                                        true,
                                        QString(),
                                        CreateNodeArgs::DefaultValuesList(),
                                        getGroup());

                
                    
                    boost::shared_ptr<Natron::Node> mergeNode = getGui()->getApp()->createNode(args);
                    
                    if (mergeNode) {
                        
                        int aIndex = mergeNode->getInputNumberFromLabel("A");
                        int bIndex = mergeNode->getInputNumberFromLabel("B");
                        assert(aIndex != -1 && bIndex != -1);
                        mergeNode->connectInput(selectedNode->getNode(), aIndex);
                        mergeNode->connectInput(_imp->_mergeHintNode->getNode(), bIndex);
                    }
                    
                   
                }
                
                _imp->_mergeHintNode.reset();
            }
        }
    } else if (state == eEventStateSelectionRect) {
        _imp->_selectionRect->hide();
        _imp->editSelectionFromSelectionRectangle( modCASIsShift(e) );
    }
    _imp->_nodesWithinBDAtPenDown.clear();

    unsetCursor();
} // mouseReleaseEvent

void
NodeGraph::scrollViewIfNeeded(const QPointF& scenePos)
{

    //Doesn't work for now
    return;
#if 0
    QRectF visibleRect = visibleSceneRect();
    
   
    
    static const int delta = 50;
    
    int deltaX = 0;
    int deltaY = 0;
    if (scenePos.x() < visibleRect.left()) {
        deltaX = -delta;
    } else if (scenePos.x() > visibleRect.right()) {
        deltaX = delta;
    }
    if (scenePos.y() < visibleRect.top()) {
        deltaY = -delta;
    } else if (scenePos.y() > visibleRect.bottom()) {
        deltaY = delta;
    }
    if (deltaX != 0 || deltaY != 0) {
        QPointF newCenter = visibleRect.center();
        newCenter.rx() += deltaX;
        newCenter.ry() += deltaY;
        centerOn(newCenter);
    }
#endif
}

void
NodeGraph::mouseMoveEvent(QMouseEvent* e)
{
    QPointF newPos = mapToScene( e->pos() );
    
    QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos.x(),_imp->_lastMousePos.y());
    
    double dx = _imp->_root->mapFromScene(newPos).x() - _imp->_root->mapFromScene(lastMousePosScene).x();
    double dy = _imp->_root->mapFromScene(newPos).y() - _imp->_root->mapFromScene(lastMousePosScene).y();

    _imp->_hasMovedOnce = true;
    
    bool mustUpdate = true;
    
    QRectF sceneR = visibleSceneRect();
    if (_imp->_evtState != eEventStateSelectionRect && _imp->_evtState != eEventStateDraggingArrow) {
        ///set cursor
        boost::shared_ptr<NodeGui> selected;
        Edge* selectedEdge = 0;
        {
            bool optionalInputsAutoHidden = areOptionalInputsAutoHidden();
            QMutexLocker l(&_imp->_nodesMutex);
            for (NodeGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
                QPointF evpt = (*it)->mapFromScene(newPos);
                
                QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
                if ((*it)->isActive() && bbox.intersects(sceneR)) {
                    if ((*it)->contains(evpt)) {
                        selected = (*it);
                        if (optionalInputsAutoHidden) {
                            (*it)->setOptionalInputsVisible(true);
                        } else {
                            break;
                        }
                    } else {
                        Edge* edge = (*it)->hasEdgeNearbyPoint(newPos);
                        if (edge) {
                            selectedEdge = edge;
                            if (!optionalInputsAutoHidden) {
                                break;
                            }
                        } else if (optionalInputsAutoHidden && !(*it)->getIsSelected()) {
                            (*it)->setOptionalInputsVisible(false);
                        }
                    }
                }
                
            }
        }
        if (selected) {
            setCursor( QCursor(Qt::OpenHandCursor) );
        } else if (selectedEdge) {
        } else if (!selectedEdge && !selected) {
            unsetCursor();
        }
    }

    bool mustUpdateNavigator = false;
    ///Apply actions
    switch (_imp->_evtState) {
    case eEventStateDraggingArrow: {
        QPointF np = _imp->_arrowSelected->mapFromScene(newPos);
        if ( _imp->_arrowSelected->isOutputEdge() ) {
            _imp->_arrowSelected->dragDest(np);
        } else {
            _imp->_arrowSelected->dragSource(np);
        }
        scrollViewIfNeeded(newPos);
        mustUpdate = true;
        break;
    }
    case eEventStateDraggingNode: {
        mustUpdate = true;
        if ( !_imp->_selection.empty() ) {
            
            bool controlDown = modCASIsControl(e);

            std::list<std::pair<NodeGuiPtr,bool> > nodesToMove;
            for (std::list<NodeGuiPtr>::iterator it = _imp->_selection.begin();
                 it != _imp->_selection.end(); ++it) {
            
                const NodeGuiPtr& node = *it;
                nodesToMove.push_back(std::make_pair(node,false));
                
                std::map<NodeGuiPtr,NodeGuiList>::iterator foundBd = _imp->_nodesWithinBDAtPenDown.find(*it);
                if (!controlDown && foundBd != _imp->_nodesWithinBDAtPenDown.end()) {
                    for (NodeGuiList::iterator it2 = foundBd->second.begin();
                         it2 != foundBd->second.end(); ++it2) {
                        ///add it only if it's not already in the list
                        bool found = false;
                        for (std::list<std::pair<NodeGuiPtr,bool> >::iterator it3 = nodesToMove.begin();
                             it3 != nodesToMove.end(); ++it3) {
                            if (it3->first == *it2) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            nodesToMove.push_back(std::make_pair(*it2,true));
                        }
                    }

                }
            }
            
            double dxScene = newPos.x() - lastMousePosScene.x();
            double dyScene = newPos.y() - lastMousePosScene.y();
            
            QPointF newNodesCenter;
            
            bool deltaSet = false;
            for (std::list<std::pair<NodeGuiPtr,bool> >::iterator it = nodesToMove.begin();
                 it != nodesToMove.end(); ++it) {
                QPointF pos = it->first->getPos_mt_safe();
                bool ignoreMagnet = it->second || nodesToMove.size() > 1;
                it->first->refreshPosition(pos.x() + dxScene, pos.y() + dyScene,ignoreMagnet,newPos);
                QPointF newNodePos = it->first->getPos_mt_safe();
                if (!ignoreMagnet) {
                    assert(nodesToMove.size() == 1);
                    _imp->_deltaSinceMousePress.rx() += newNodePos.x() - pos.x();
                    _imp->_deltaSinceMousePress.ry() += newNodePos.y() - pos.y();
                    deltaSet = true;
                }
                newNodePos = it->first->mapToScene(it->first->mapFromParent(newNodePos));
                newNodesCenter.rx() += newNodePos.x();
                newNodesCenter.ry() += newNodePos.y();
                
            }
            size_t c = nodesToMove.size();
            if (c) {
                newNodesCenter.rx() /= c;
                newNodesCenter.ry() /= c;
            }
            
            scrollViewIfNeeded(newNodesCenter);
            
            if (!deltaSet) {
                _imp->_deltaSinceMousePress.rx() += dxScene;
                _imp->_deltaSinceMousePress.ry() += dyScene;
            }


            mustUpdateNavigator = true;
            
        }
        
        if (_imp->_selection.size() == 1) {
            ///try to find a nearby edge
            boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.front();
            
            boost::shared_ptr<Natron::Node> internalNode = selectedNode->getNode();
            
            bool doMergeHints = e->modifiers().testFlag(Qt::ControlModifier) && e->modifiers().testFlag(Qt::ShiftModifier);
            bool doHints = appPTR->getCurrentSettings()->isConnectionHintEnabled();
            
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(selectedNode.get());
            if (isBd) {
                doMergeHints = false;
                doHints = false;
            }
            
            if (!doMergeHints) {
                ///for nodes already connected don't show hint
                if ( ( internalNode->getMaxInputCount() == 0) && internalNode->hasOutputConnected() ) {
                    doHints = false;
                } else if ( ( internalNode->getMaxInputCount() > 0) && internalNode->hasAllInputsConnected() && internalNode->hasOutputConnected() ) {
                    doHints = false;
                }
            }
            
            if (doHints) {
                QRectF selectedNodeBbox = selectedNode->boundingRectWithEdges();//selectedNode->mapToParent( selectedNode->boundingRect() ).boundingRect();
                double tolerance = 10;
                selectedNodeBbox.adjust(-tolerance, -tolerance, tolerance, tolerance);
                
                boost::shared_ptr<NodeGui> nodeToShowMergeRect;
                
                boost::shared_ptr<Natron::Node> selectedNodeInternalNode = selectedNode->getNode();
                bool selectedNodeIsReader = selectedNodeInternalNode->getLiveInstance()->isReader();
                Edge* edge = 0;
                {
                    QMutexLocker l(&_imp->_nodesMutex);
                    for (NodeGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
                        
                        bool isAlreadyAnOutput = false;
                        const std::list<Natron::Node*>& outputs = internalNode->getGuiOutputs();
                        for (std::list<Natron::Node*>::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                            if (*it2 == (*it)->getNode().get()) {
                                isAlreadyAnOutput = true;
                                break;
                            }
                        }
                        if (isAlreadyAnOutput) {
                            continue;
                        }
                        QRectF nodeBbox = (*it)->boundingRectWithEdges();
                        if ( (*it) != selectedNode && (*it)->isVisible() && nodeBbox.intersects(sceneR)) {
                            
                            if (doMergeHints) {
                                
                                //QRectF nodeRect = (*it)->mapToParent((*it)->boundingRect()).boundingRect();
                                
                                boost::shared_ptr<Natron::Node> internalNode = (*it)->getNode();
                                
                                
                                if (!internalNode->isOutputNode() && nodeBbox.intersects(selectedNodeBbox)) {
                                    
                                    bool nHasInput = internalNode->hasInputConnected();
                                    int nMaxInput = internalNode->getMaxInputCount();
                                    bool selectedHasInput = selectedNodeInternalNode->hasInputConnected();
                                    int selectedMaxInput = selectedNodeInternalNode->getMaxInputCount();
                                    double nPAR = internalNode->getLiveInstance()->getPreferredAspectRatio();
                                    double selectedPAR = selectedNodeInternalNode->getLiveInstance()->getPreferredAspectRatio();
                                    double nFPS = internalNode->getLiveInstance()->getPreferredFrameRate();
                                    double selectedFPS = selectedNodeInternalNode->getLiveInstance()->getPreferredFrameRate();
                                    
                                    bool isValid = true;
                                    
                                    if (selectedPAR != nPAR || std::abs(nFPS - selectedFPS) > 0.01) {
                                        if (nHasInput || selectedHasInput) {
                                            isValid = false;
                                        } else if (!nHasInput && nMaxInput == 0 && !selectedHasInput && selectedMaxInput == 0) {
                                            isValid = false;
                                        }
                                    }
                                    if (isValid) {
                                        nodeToShowMergeRect = *it;
                                    }
                                } else {
                                    (*it)->setMergeHintActive(false);
                                }
                                
                            } else {
                                
                                
                                
                                edge = (*it)->hasEdgeNearbyRect(selectedNodeBbox);
                                
                                ///if the edge input is the selected node don't continue
                                if ( edge && ( edge->getSource() == selectedNode) ) {
                                    edge = 0;
                                }
                                
                                if ( edge && edge->isOutputEdge() ) {
                                    
                                    if (selectedNodeIsReader) {
                                        continue;
                                    }
                                    int prefInput = selectedNodeInternalNode->getPreferredInputForConnection();
                                    if (prefInput == -1) {
                                        edge = 0;
                                    } else {
                                        Natron::Node::CanConnectInputReturnValue ret = selectedNodeInternalNode->canConnectInput(edge->getSource()->getNode(),
                                                                                                                                 prefInput);
                                        if (ret != Natron::Node::eCanConnectInput_ok) {
                                            edge = 0;
                                        }
                                    }
                                }
                                
                                if ( edge && !edge->isOutputEdge() ) {
                                    
                                    if ((*it)->getNode()->getLiveInstance()->isReader()) {
                                        edge = 0;
                                        continue;
                                    }
                                    
                                    if ((*it)->getNode()->getLiveInstance()->isInputRotoBrush(edge->getInputNumber())) {
                                        edge = 0;
                                        continue;
                                    }
                                    
                                    Natron::Node::CanConnectInputReturnValue ret = edge->getDest()->getNode()->canConnectInput(selectedNodeInternalNode, edge->getInputNumber());
                                    if (ret == Natron::Node::eCanConnectInput_inputAlreadyConnected &&
                                        !selectedNodeInternalNode->getLiveInstance()->isReader()) {
                                        ret = Natron::Node::eCanConnectInput_ok;
                                    }
                                    
                                    if (ret != Natron::Node::eCanConnectInput_ok) {
                                        edge = 0;
                                    }
                                    
                                }
                                
                                if (edge) {
                                    edge->setUseHighlight(true);
                                    break;
                                }
                            }
                        }
                    }
                } // QMutexLocker l(&_imp->_nodesMutex);
                
                if ( _imp->_highLightedEdge && ( _imp->_highLightedEdge != edge) ) {
                    _imp->_highLightedEdge->setUseHighlight(false);
                    _imp->_hintInputEdge->hide();
                    _imp->_hintOutputEdge->hide();
                }

                _imp->_highLightedEdge = edge;

                if ( edge && edge->getSource() && edge->getDest() ) {
                    ///setup the hints edge

                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> > & inpNodes = selectedNode->getNode()->getGuiInputs();
                    for (U32 i = 0; i < inpNodes.size(); ++i) {
                        if ( inpNodes[i] == edge->getSource()->getNode() ) {
                            alreadyConnected = true;
                            break;
                        }
                    }

                    if ( !_imp->_hintInputEdge->isVisible() ) {
                        if (!alreadyConnected) {
                            int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), selectedNode);
                            _imp->_hintInputEdge->setVisible(true);
                        }
                        _imp->_hintOutputEdge->setInputNumber( edge->getInputNumber() );
                        _imp->_hintOutputEdge->setSourceAndDestination( selectedNode, edge->getDest() );
                        _imp->_hintOutputEdge->setVisible(true);
                    } else {
                        if (!alreadyConnected) {
                            _imp->_hintInputEdge->initLine();
                        }
                        _imp->_hintOutputEdge->initLine();
                    }
                } else if (edge) {
                    ///setup only 1 of the hints edge

                    if ( _imp->_highLightedEdge && !_imp->_hintInputEdge->isVisible() ) {
                        if ( edge->isOutputEdge() ) {
                            int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), selectedNode);
                        } else {
                            _imp->_hintInputEdge->setInputNumber( edge->getInputNumber() );
                            _imp->_hintInputEdge->setSourceAndDestination( selectedNode,edge->getDest() );
                        }
                        _imp->_hintInputEdge->setVisible(true);
                    } else if ( _imp->_highLightedEdge && _imp->_hintInputEdge->isVisible() ) {
                        _imp->_hintInputEdge->initLine();
                    }
                } else if (nodeToShowMergeRect) {
                    nodeToShowMergeRect->setMergeHintActive(true);
                    selectedNode->setMergeHintActive(true);
                    _imp->_mergeHintNode = nodeToShowMergeRect;
                } else {
                    selectedNode->setMergeHintActive(false);
                    _imp->_mergeHintNode.reset();
                }
                
                
            } // if (doHints) {
        } //  if (_imp->_selection.nodes.size() == 1) {
        setCursor( QCursor(Qt::ClosedHandCursor) );
        break;
    }
    case eEventStateMovingArea: {
        mustUpdateNavigator = true;
        _imp->_root->moveBy(dx, dy);
        setCursor( QCursor(Qt::SizeAllCursor) );
        mustUpdate = true;
        break;
    }
    case eEventStateResizingBackdrop: {
        mustUpdateNavigator = true;
        assert(_imp->_backdropResized);
        QPointF p = _imp->_backdropResized->scenePos();
        int w = newPos.x() - p.x();
        int h = newPos.y() - p.y();
        scrollViewIfNeeded(newPos);
        mustUpdate = true;
        pushUndoCommand( new ResizeBackDropCommand(_imp->_backdropResized,w,h) );
        break;
    }
    case eEventStateSelectionRect: {
        QPointF lastSelectionScene = mapToScene(_imp->_lastSelectionStartPoint);
        QPointF startDrag = _imp->_selectionRect->mapFromScene(lastSelectionScene);
        QPointF cur = _imp->_selectionRect->mapFromScene(newPos);
        double xmin = std::min( cur.x(),startDrag.x() );
        double xmax = std::max( cur.x(),startDrag.x() );
        double ymin = std::min( cur.y(),startDrag.y() );
        double ymax = std::max( cur.y(),startDrag.y() );
        scrollViewIfNeeded(newPos);
        _imp->_selectionRect->setRect(xmin,ymin,xmax - xmin,ymax - ymin);
        _imp->_selectionRect->show();
        mustUpdate = true;
        break;
    }
    case eEventStateDraggingNavigator: {
        QPointF mousePosSceneCoordinates;
        bool insideNavigator = isNearbyNavigator(e->pos(), mousePosSceneCoordinates);
        if (insideNavigator) {
            _imp->_refreshOverlays = true;
            centerOn(mousePosSceneCoordinates);
            _imp->_lastMousePos = e->pos();
            update();
            return;
        }
        
    } break;
    case eEventStateZoomingArea: {
        int delta = 2*((e->x() - _imp->_lastMousePos.x()) - (e->y() - _imp->_lastMousePos.y()));
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        wheelEventInternal(modCASIsControl(e),delta);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        mustUpdate = true;
    } break;
    default:
            mustUpdate = false;
        break;
    } // switch

    
    _imp->_lastMousePos = e->pos();

    if (mustUpdateNavigator) {
        _imp->_refreshOverlays = true;
        mustUpdate = true;
    }
    
    if (mustUpdate) {
        update();
    }
    QGraphicsView::mouseMoveEvent(e);
} // mouseMoveEvent


void
NodeGraph::mouseDoubleClickEvent(QMouseEvent* e)
{
    
    QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos);

    NodeGuiList nodes = getAllActiveNodes_mt_safe();
    
    ///Matches sorted by depth
    std::map<double,NodeGuiPtr> matches;
    for (NodeGuiList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        QPointF evpt = (*it)->mapFromScene(lastMousePosScene);
        if ( (*it)->isVisible() && (*it)->isActive() && (*it)->contains(evpt) ) {
            matches.insert(std::make_pair((*it)->zValue(), *it));
        }
    }
    if (!matches.empty()) {
        const NodeGuiPtr& node = matches.rbegin()->second;
        if (modCASIsControl(e)) {
            node->ensurePanelCreated();
            if (node->getSettingPanel()) {
                node->getSettingPanel()->floatPanel();
            }
        } else {
            node->setVisibleSettingsPanel(true);
            if (node->getSettingPanel()) {
                _imp->_gui->putSettingsPanelFirst( node->getSettingPanel() );
            } else {
                ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(node->getNode()->getLiveInstance());
                if (isViewer) {
                    ViewerGL* viewer = dynamic_cast<ViewerGL*>(isViewer->getUiContext());
                    assert(viewer);
                    ViewerTab* tab = viewer->getViewerTab();
                    assert(tab);
                    
                    TabWidget* foundTab = 0;
                    QWidget* parent = tab->parentWidget();
                    while (parent) {
                        foundTab = dynamic_cast<TabWidget*>(parent);
                        if (foundTab) {
                            break;
                        }
                        parent = parent->parentWidget();
                    }
                    if (foundTab) {
                        foundTab->setCurrentWidget(tab);
                    } else {
                        
                        //try to find a floating window
                        FloatingWidget* floating = 0;
                        parent = tab->parentWidget();
                        while (parent) {
                            floating = dynamic_cast<FloatingWidget*>(parent);
                            if (floating) {
                                break;
                            }
                            parent = parent->parentWidget();
                        }
                        if (floating) {
                            floating->activateWindow();
                        }
                    }
                }
            }
        }
        if ( !node->wasBeginEditCalled() ) {
            node->beginEditKnobs();
        }
        
        if (modCASIsShift(e)) {
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(node->getNode()->getLiveInstance());
            if (isGrp && isGrp->isSubGraphEditable()) {
                NodeGraphI* graph_i = isGrp->getNodeGraph();
                assert(graph_i);
                NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
                if (graph) {
                    TabWidget* isParentTab = dynamic_cast<TabWidget*>(graph->parentWidget());
                    if (isParentTab) {
                        isParentTab->setCurrentWidget(graph);
                    } else {
                        NodeGraph* lastSelectedGraph = _imp->_gui->getLastSelectedGraph();
                        ///We're in the double click event, it should've entered the focus in event beforehand!
                        assert(lastSelectedGraph == this);
                        
                        isParentTab = dynamic_cast<TabWidget*>(lastSelectedGraph->parentWidget());
                        assert(isParentTab);
                        isParentTab->appendTab(graph,graph);
                        
                    }
                    QTimer::singleShot(25, graph, SLOT(centerOnAllNodes()));
                }
            }
        }
        
        getGui()->getApp()->redrawAllViewers();
    }

}

bool
NodeGraph::event(QEvent* e)
{
    if (!_imp->_gui) {
        return false;
    }
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = dynamic_cast<QKeyEvent*>(e);
        assert(ke);
        if (ke && (ke->key() == Qt::Key_Tab) && _imp->_nodeCreationShortcutEnabled) {
            NodeCreationDialog* nodeCreation = new NodeCreationDialog(_imp->_lastNodeCreatedName,this);

            ///This allows us to have a non-modal dialog: when the user clicks outside of the dialog,
            ///it closes it.
            QObject::connect( nodeCreation,SIGNAL( accepted() ),this,SLOT( onNodeCreationDialogFinished() ) );
            QObject::connect( nodeCreation,SIGNAL( rejected() ),this,SLOT( onNodeCreationDialogFinished() ) );
            nodeCreation->show();


            ke->accept();

            return true;
        }
    }

    return QGraphicsView::event(e);
}

void
NodeGraph::onNodeCreationDialogFinished()
{
    NodeCreationDialog* dialog = qobject_cast<NodeCreationDialog*>( sender() );

    if (dialog) {
        QDialog::DialogCode ret = (QDialog::DialogCode)dialog->result();
        int major;
        QString res = dialog->getNodeName(&major);
        _imp->_lastNodeCreatedName = res;
        dialog->deleteLater();

        switch (ret) {
        case QDialog::Accepted: {
            
            const Natron::PluginsMap & allPlugins = appPTR->getPluginsList();
            Natron::PluginsMap::const_iterator found = allPlugins.find(res.toStdString());
            if (found != allPlugins.end()) {
                QPointF posHint = mapToScene( mapFromGlobal( QCursor::pos() ) );
                getGui()->getApp()->createNode( CreateNodeArgs( res,
                                                               "",
                                                               major,
                                                               -1,
                                                               true,
                                                               posHint.x(),
                                                               posHint.y(),
                                                               true,
                                                               true,
                                                               true,
                                                               QString(),
                                                               CreateNodeArgs::DefaultValuesList(),
                                                               getGroup()) );
            }
            break;
        }
        case QDialog::Rejected:
        default:
            break;
        }
    }
}

void
NodeGraph::keyPressEvent(QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    if (key == Qt::Key_Escape) {
        return QGraphicsView::keyPressEvent(e);
    }
    
    if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, modifiers, key) ) {
        QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
        QCoreApplication::postEvent(parentWidget(),ev);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateReader, modifiers, key) ) {
        _imp->_gui->createReader();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateWriter, modifiers, key) ) {
        _imp->_gui->createWriter();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRemoveNodes, modifiers, key) ) {
        deleteSelection();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphForcePreview, modifiers, key) ) {
        forceRefreshAllPreviews();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCopy, modifiers, key) ) {
        copySelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphPaste, modifiers, key) ) {
        pasteNodeClipBoards();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCut, modifiers, key) ) {
        cutSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDuplicate, modifiers, key) ) {
        duplicateSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphClone, modifiers, key) ) {
        cloneSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDeclone, modifiers, key) ) {
        decloneSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFrameNodes, modifiers, key) ) {
        centerOnAllNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphEnableHints, modifiers, key) ) {
        toggleConnectionHints();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSwitchInputs, modifiers, key) ) {
        ///No need to make an undo command for this, the user just have to do it a second time to reverse the effect
        switchInputs1and2ForSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAll, modifiers, key) ) {
        selectAllNodes(false);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAllVisible, modifiers, key) ) {
        selectAllNodes(true);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphMakeGroup, modifiers, key) ) {
        createGroupFromSelection();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExpandGroup, modifiers, key) ) {
        expandSelectedGroups();
    } else if (key == Qt::Key_Control) {
        _imp->setNodesBendPointsVisible(true);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectUp, modifiers, key) ||
                isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateUpstream, modifiers, key) ) {
        ///We try to find if the last selected node has an input, if so move selection (or add to selection)
        ///the first valid input node
        if ( !_imp->_selection.empty() ) {
            boost::shared_ptr<NodeGui> lastSelected = ( *_imp->_selection.rbegin() );
            const std::vector<Edge*> & inputs = lastSelected->getInputsArrows();
            for (U32 i = 0; i < inputs.size() ; ++i) {
                if ( inputs[i]->hasSource() ) {
                    boost::shared_ptr<NodeGui> input = inputs[i]->getSource();
                    if ( input->getIsSelected() && modCASIsShift(e) ) {
                        std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.begin(),
                                                                                           _imp->_selection.end(),lastSelected);
                        if ( found != _imp->_selection.end() ) {
                            lastSelected->setUserSelected(false);
                            _imp->_selection.erase(found);
                        }
                    } else {
                        selectNode( inputs[i]->getSource(), modCASIsShift(e) );
                    }
                    break;
                }
            }
        }
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectDown, modifiers, key) ||
                isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateDownstream, modifiers, key) ) {
        ///We try to find if the last selected node has an output, if so move selection (or add to selection)
        ///the first valid output node
        if ( !_imp->_selection.empty() ) {
            boost::shared_ptr<NodeGui> lastSelected = ( *_imp->_selection.rbegin() );
            const std::list<Natron::Node* > & outputs = lastSelected->getNode()->getGuiOutputs();
            if ( !outputs.empty() ) {
                boost::shared_ptr<NodeGuiI> output_i = outputs.front()->getNodeGui();
                boost::shared_ptr<NodeGui> output = boost::dynamic_pointer_cast<NodeGui>(output_i);
                if (output) {
                    if ( output->getIsSelected() && modCASIsShift(e) ) {
                        std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.begin(),
                                                                                           _imp->_selection.end(),lastSelected);
                        if ( found != _imp->_selection.end() ) {
                            lastSelected->setUserSelected(false);
                            _imp->_selection.erase(found);
                        }
                    } else {
                        selectNode( output, modCASIsShift(e) );
                    }
                }
            }
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->firstFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->lastFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->previousIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->nextIncrement();
        }
    }else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->nextFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getLastSelectedViewer() ) {
            getLastSelectedViewer()->previousFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        getGui()->getApp()->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        getGui()->getApp()->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRearrangeNodes, modifiers, key) ) {
        _imp->rearrangeSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, modifiers, key) ) {
        _imp->toggleSelectedNodesEnabled();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphShowExpressions, modifiers, key) ) {
        toggleKnobLinksVisible();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoPreview, modifiers, key) ) {
        toggleAutoPreview();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoTurbo, modifiers, key) ) {
        toggleAutoTurbo();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphAutoHideInputs, modifiers, key) ) {
        toggleAutoHideInputs(true);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, modifiers, key) ) {
        popFindDialog(QCursor::pos());
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRenameNode, modifiers, key) ) {
        popRenameDialog(QCursor::pos());
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExtractNode, modifiers, key) ) {
        pushUndoCommand(new ExtractNodeUndoRedoCommand(this,_imp->_selection));
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphTogglePreview, modifiers, key) ) {
        togglePreviewsForSelectedNodes();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomIn, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal(QCursor::pos()), 120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionZoomOut, Qt::NoModifier, key) ) { // zoom in/out doesn't care about modifiers
        QWheelEvent e(mapFromGlobal(QCursor::pos()), -120, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
        wheelEvent(&e);
    } else {
        bool intercepted = false;
        
        if ( modifiers.testFlag(Qt::ControlModifier) && (key == Qt::Key_Up || key == Qt::Key_Down)) {
            ///These shortcuts pans the graphics view but we don't want it
            intercepted = true;
        }
        
        if (!intercepted) {
            /// Search for a node which has a shortcut bound
            const Natron::PluginsMap & allPlugins = appPTR->getPluginsList();
            for (Natron::PluginsMap::const_iterator it = allPlugins.begin() ; it != allPlugins.end() ; ++it) {
                
                assert(!it->second.empty());
                Natron::Plugin* plugin = *it->second.rbegin();
                
                if ( plugin->getHasShortcut() ) {
                    QString group(kShortcutGroupNodes);
                    QStringList groupingSplit = plugin->getGrouping();
                    for (int j = 0; j < groupingSplit.size(); ++j) {
                        group.push_back('/');
                        group.push_back(groupingSplit[j]);
                    }
                    if ( isKeybind(group.toStdString().c_str(), plugin->getPluginID(), modifiers, key) ) {
                        QPointF hint = mapToScene( mapFromGlobal( QCursor::pos() ) );
                        getGui()->getApp()->createNode( CreateNodeArgs( plugin->getPluginID(),
                                                                       "",
                                                                       -1,-1,
                                                                       true,
                                                                       hint.x(),hint.y(),
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       QString(),
                                                                       CreateNodeArgs::DefaultValuesList(),
                                                                       getGroup()) );
                        intercepted = true;
                        break;
                    }
                }
            }
        }
        
        
        if (!intercepted) {
            QGraphicsView::keyPressEvent(e);
        }
    }
} // keyPressEvent

void
NodeGraph::toggleAutoTurbo()
{
    appPTR->getCurrentSettings()->setAutoTurboModeEnabled(!appPTR->getCurrentSettings()->isAutoTurboEnabled());
}


void
NodeGraph::selectAllNodes(bool onlyInVisiblePortion)
{
    _imp->resetSelection();
    if (onlyInVisiblePortion) {
        QRectF r = visibleSceneRect();
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
            if ( r.intersects(bbox) && (*it)->isActive() && (*it)->isVisible() ) {
                (*it)->setUserSelected(true);
                _imp->_selection.push_back(*it);
            }
        }
  
    } else {
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( (*it)->isActive() && (*it)->isVisible() ) {
                (*it)->setUserSelected(true);
                _imp->_selection.push_back(*it);
            }
        }
    }
}

void
NodeGraph::connectCurrentViewerToSelection(int inputNB)
{
    if ( !getLastSelectedViewer() ) {
        _imp->_gui->getApp()->createNode(  CreateNodeArgs(PLUGINID_NATRON_VIEWER,
                                                          "",
                                                          -1,-1,
                                                          true,
                                                          INT_MIN,INT_MIN,
                                                          true,
                                                          true,
                                                          true,
                                                          QString(),
                                                          CreateNodeArgs::DefaultValuesList(),
                                                          getGroup()) );
    }

    ///get a pointer to the last user selected viewer
    boost::shared_ptr<InspectorNode> v = boost::dynamic_pointer_cast<InspectorNode>( getLastSelectedViewer()->
                                                                                     getInternalNode()->getNode() );

    ///if the node is no longer active (i.e: it was deleted by the user), don't do anything.
    if ( !v->isActivated() ) {
        return;
    }

    ///get a ptr to the NodeGui
    boost::shared_ptr<NodeGuiI> gui_i = v->getNodeGui();
    boost::shared_ptr<NodeGui> gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
    assert(gui);

    ///if there's no selected node or the viewer is selected, then try refreshing that input nb if it is connected.
    bool viewerAlreadySelected = std::find(_imp->_selection.begin(),_imp->_selection.end(),gui) != _imp->_selection.end();
    if (_imp->_selection.empty() || (_imp->_selection.size() > 1) || viewerAlreadySelected) {
        v->setActiveInputAndRefresh(inputNB);
        gui->refreshEdges();

        return;
    }

    boost::shared_ptr<NodeGui> selected = _imp->_selection.front();


    if ( !selected->getNode()->canOthersConnectToThisNode() ) {
        return;
    }

    ///if the node doesn't have the input 'inputNb' created yet, populate enough input
    ///so it can be created.
    Edge* foundInput = gui->getInputArrow(inputNB);
    assert(foundInput);
  
    ///and push a connect command to the selected node.
    pushUndoCommand( new ConnectCommand(this,foundInput,foundInput->getSource(),selected) );

    ///Set the viewer as the selected node (also wipe the current selection)
    selectNode(gui,false);
} // connectCurrentViewerToSelection

void
NodeGraph::enterEvent(QEvent* e)
{
    QGraphicsView::enterEvent(e);
    
    QWidget* currentFocus = qApp->focusWidget();
    
    bool canSetFocus = !currentFocus ||
    dynamic_cast<ViewerGL*>(currentFocus) ||
    dynamic_cast<CurveWidget*>(currentFocus) ||
    dynamic_cast<Histogram*>(currentFocus) ||
    dynamic_cast<NodeGraph*>(currentFocus) ||
    dynamic_cast<QToolButton*>(currentFocus) ||
    currentFocus->objectName() == "Properties" ||
    currentFocus->objectName() == "SettingsPanel" ||
    currentFocus->objectName() == "qt_tabwidget_tabbar";
    
    if (canSetFocus) {
        setFocus();
    }

    _imp->_nodeCreationShortcutEnabled = true;
   
}

void
NodeGraph::leaveEvent(QEvent* e)
{
    QGraphicsView::leaveEvent(e);

    _imp->_nodeCreationShortcutEnabled = false;
   // setFocus();
}

void
NodeGraph::setVisibleNodeDetails(bool visible)
{
    if (visible == _imp->_detailsVisible) {
        return;
    }
    _imp->_detailsVisible = visible;
    QMutexLocker k(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->setVisibleDetails(visible);
    }
}

void
NodeGraph::wheelEventInternal(bool ctrlDown,double delta)
{
    double scaleFactor = pow( NATRON_WHEEL_ZOOM_PER_DELTA, delta);
    QTransform transfo = transform();
    
    double currentZoomFactor = transfo.mapRect( QRectF(0, 0, 1, 1) ).width();
    double newZoomfactor = currentZoomFactor * scaleFactor;
    if ((newZoomfactor < 0.01 && scaleFactor < 1.) || (newZoomfactor > 50 && scaleFactor > 1.)) {
        return;
    }
    if (newZoomfactor < 0.4) {
        setVisibleNodeDetails(false);
    } else if (newZoomfactor >= 0.4) {
        setVisibleNodeDetails(true);
    }
    
    if (ctrlDown && _imp->_magnifiedNode) {
        if (!_imp->_magnifOn) {
            _imp->_magnifOn = true;
            _imp->_nodeSelectedScaleBeforeMagnif = _imp->_magnifiedNode->scale();
        }
        _imp->_magnifiedNode->setScale_natron(_imp->_magnifiedNode->scale() * scaleFactor);
    } else {

        _imp->_accumDelta += delta;
        if (std::abs(_imp->_accumDelta) > 60) {
            scaleFactor = pow( NATRON_WHEEL_ZOOM_PER_DELTA, _imp->_accumDelta );
           // setSceneRect(NATRON_SCENE_MIN,NATRON_SCENE_MIN,NATRON_SCENE_MAX,NATRON_SCENE_MAX);
            scale(scaleFactor,scaleFactor);
            _imp->_accumDelta = 0;
        }
        _imp->_refreshOverlays = true;
        
    }

}

void
NodeGraph::wheelEvent(QWheelEvent* e)
{
    if (e->orientation() != Qt::Vertical) {
        return;
    }
    wheelEventInternal(modCASIsControl(e), e->delta());
    _imp->_lastMousePos = e->pos();
    update();
}

void
NodeGraph::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        if (_imp->_magnifOn) {
            _imp->_magnifOn = false;
            _imp->_magnifiedNode->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
        }
        if (_imp->_bendPointsVisible) {
            _imp->setNodesBendPointsVisible(false);
        }
    }
}

void
NodeGraph::removeNode(const boost::shared_ptr<NodeGui> & node)
{
 
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(node->getNode()->getLiveInstance());
    const std::vector<boost::shared_ptr<KnobI> > & knobs = node->getNode()->getKnobs();

    
    for (U32 i = 0; i < knobs.size(); ++i) {
        std::list<boost::shared_ptr<KnobI> > listeners;
        knobs[i]->getListeners(listeners);
        ///For all listeners make sure they belong to a node
        bool foundEffect = false;
        for (std::list<boost::shared_ptr<KnobI> >::iterator it2 = listeners.begin(); it2 != listeners.end(); ++it2) {
            EffectInstance* isEffect = dynamic_cast<EffectInstance*>( (*it2)->getHolder() );
            if (!isEffect) {
                continue;
            }
            if (isGrp && isEffect->getNode()->getGroup().get() == isGrp) {
                continue;
            }
            
            if ( isEffect && ( isEffect != node->getNode()->getLiveInstance() ) ) {
                foundEffect = true;
                break;
            }
        }
        if (foundEffect) {
            Natron::StandardButtonEnum reply = Natron::questionDialog( tr("Delete").toStdString(), tr("This node has one or several "
                                                                                                  "parameters from which other parameters "
                                                                                                  "of the project rely on through expressions "
                                                                                                  "or links. Deleting this node will "
                                                                                                  "remove these expressions  "
                                                                                                  "and undoing the action will not recover "
                                                                                                  "them. Do you wish to continue ?")
                                                                   .toStdString(), false );
            if (reply == Natron::eStandardButtonNo) {
                return;
            }
            break;
        }
    }

    node->setUserSelected(false);
    std::list<boost::shared_ptr<NodeGui> > nodesToRemove;
    nodesToRemove.push_back(node);
    pushUndoCommand( new RemoveMultipleNodesCommand(this,nodesToRemove) );
}

void
NodeGraph::deleteSelection()
{
    if ( !_imp->_selection.empty()) {
        NodeGuiList nodesToRemove = _imp->_selection;

        
        ///For all backdrops also move all the nodes contained within it
        for (NodeGuiList::iterator it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it) {
            NodeGuiList nodesWithinBD = getNodesWithinBackDrop(*it);
            for (NodeGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodeGuiList::iterator found = std::find(nodesToRemove.begin(),nodesToRemove.end(),*it2);
                if ( found == nodesToRemove.end()) {
                    nodesToRemove.push_back(*it2);
                }
            }
        }


        for (NodeGuiList::iterator it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it) {
            
            const std::vector<boost::shared_ptr<KnobI> > & knobs = (*it)->getNode()->getKnobs();
            bool mustBreak = false;
            
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getNode()->getLiveInstance());
            
            for (U32 i = 0; i < knobs.size(); ++i) {
                std::list<boost::shared_ptr<KnobI> > listeners;
                knobs[i]->getListeners(listeners);

                ///For all listeners make sure they belong to a node
                bool foundEffect = false;
                for (std::list<boost::shared_ptr<KnobI> >::iterator it2 = listeners.begin(); it2 != listeners.end(); ++it2) {
                    EffectInstance* isEffect = dynamic_cast<EffectInstance*>( (*it2)->getHolder() );
                    
                    if (!isEffect) {
                        continue;
                    }
                    if (isGrp && isEffect->getNode()->getGroup().get() == isGrp) {
                        continue;
                    }
                    
                    if ( isEffect && ( isEffect != (*it)->getNode()->getLiveInstance() ) ) {
                        foundEffect = true;
                        break;
                    }
                }
                if (foundEffect) {
                    Natron::StandardButtonEnum reply = Natron::questionDialog( tr("Delete").toStdString(),
                                                                           tr("This node has one or several "
                                                                              "parameters from which other parameters "
                                                                              "of the project rely on through expressions "
                                                                              "or links. Deleting this node will "
                                                                              "remove these expressions. "
                                                                              ". Undoing the action will not recover "
                                                                              "them. \nContinue anyway ?")
                                                                           .toStdString(), false );
                    if (reply == Natron::eStandardButtonNo) {
                        return;
                    }
                    mustBreak = true;
                    break;
                }
            }
            if (mustBreak) {
                break;
            }
        }


        for (NodeGuiList::iterator it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it) {
            (*it)->setUserSelected(false);
        }


        pushUndoCommand( new RemoveMultipleNodesCommand(this,nodesToRemove) );
        _imp->_selection.clear();
    }
} // deleteSelection

void
NodeGraph::selectNode(const boost::shared_ptr<NodeGui> & n,
                      bool addToSelection)
{
    if ( !n->isVisible() ) {
        return;
    }
    bool alreadyInSelection = std::find(_imp->_selection.begin(),_imp->_selection.end(),n) != _imp->_selection.end();


    assert(n);
    if (addToSelection && !alreadyInSelection) {
        _imp->_selection.push_back(n);
    } else if (!addToSelection) {
        clearSelection();
        _imp->_selection.push_back(n);
    }

    n->setUserSelected(true);

    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( n->getNode()->getLiveInstance() );
    if (isViewer) {
        OpenGLViewerI* viewer = isViewer->getUiContext();
        const std::list<ViewerTab*> & viewerTabs = _imp->_gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewer ) {
                setLastSelectedViewer( (*it) );
            }
        }
    }

    bool magnifiedNodeSelected = false;
    if (_imp->_magnifiedNode) {
        magnifiedNodeSelected = std::find(_imp->_selection.begin(),_imp->_selection.end(),_imp->_magnifiedNode)
                                != _imp->_selection.end();
    }
    if (magnifiedNodeSelected && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_magnifiedNode->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}

void
NodeGraph::setLastSelectedViewer(ViewerTab* tab)
{
    _imp->lastSelectedViewer = tab;
}

ViewerTab*
NodeGraph::getLastSelectedViewer() const
{
    return _imp->lastSelectedViewer;
}

void
NodeGraph::setSelection(const std::list<boost::shared_ptr<NodeGui> >& nodes)
{
    clearSelection();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        selectNode(*it, true);
    }
}

void
NodeGraph::clearSelection()
{
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            (*it)->setUserSelected(false);
        }
    }

    _imp->_selection.clear();

}

void
NodeGraph::updateNavigator()
{
    if ( !areAllNodesVisible() ) {
        _imp->_navigator->setPixmap( QPixmap::fromImage( getFullSceneScreenShot() ) );
        _imp->_navigator->show();
    } else {
        _imp->_navigator->hide();
    }
}

bool
NodeGraph::areAllNodesVisible()
{
    QRectF rect = visibleSceneRect();
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ( (*it)->isVisible() ) {
            if ( !rect.contains( (*it)->boundingRectWithEdges() ) ) {
                return false;
            }
        }
    }
    return true;
}

QImage
NodeGraph::getFullSceneScreenShot()
{
    ///The bbox of all nodes in the nodegraph
    QRectF sceneR = _imp->calcNodesBoundingRect();

    ///The visible portion of the nodegraph
    QRectF viewRect = visibleSceneRect();

    ///Make sure the visible rect is included in the scene rect
    sceneR = sceneR.united(viewRect);

    int navWidth = std::ceil(width() * NATRON_NAVIGATOR_BASE_WIDTH);
    int navHeight = std::ceil(height() * NATRON_NAVIGATOR_BASE_HEIGHT);

    ///Make sceneR and viewRect keep the same aspect ratio as the navigator
    double xScale = navWidth / sceneR.width();
    double yScale =  navHeight / sceneR.height();
    double scaleFactor = std::max(0.001,std::min(xScale,yScale));
    
    int sceneW_navPixelCoord = std::floor(sceneR.width() * scaleFactor);
    int sceneH_navPixelCoord = std::floor(sceneR.height() * scaleFactor);

    ///Render the scene in an image with the same aspect ratio  as the scene rect
    QImage renderImage(sceneW_navPixelCoord,sceneH_navPixelCoord,QImage::Format_ARGB32_Premultiplied);
    
    ///Fill the background
    renderImage.fill( QColor(71,71,71,255) );

    ///Offset the visible rect corner as an offset relative to the scene rect corner
    viewRect.setX( viewRect.x() - sceneR.x() );
    viewRect.setY( viewRect.y() - sceneR.y() );
    viewRect.setWidth( viewRect.width() - sceneR.x() );
    viewRect.setHeight( viewRect.height() - sceneR.y() );

    QRectF viewRect_navCoordinates = viewRect;
    viewRect_navCoordinates.setLeft(viewRect.left() * scaleFactor);
    viewRect_navCoordinates.setBottom(viewRect.bottom() * scaleFactor);
    viewRect_navCoordinates.setRight(viewRect.right() * scaleFactor);
    viewRect_navCoordinates.setTop(viewRect.top() * scaleFactor);

    ///Paint the visible portion with a highlight
    QPainter painter(&renderImage);

    ///Remove the overlays from the scene before rendering it
    scene()->removeItem(_imp->_cacheSizeText);
    scene()->removeItem(_imp->_navigator);

    ///Render into the QImage with downscaling
    scene()->render(&painter,renderImage.rect(),sceneR,Qt::KeepAspectRatio);

    ///Add the overlays back
    scene()->addItem(_imp->_navigator);
    scene()->addItem(_imp->_cacheSizeText);

    ///Fill the highlight with a semi transparant whitish grey
    painter.fillRect( viewRect_navCoordinates, QColor(200,200,200,100) );
    
    ///Draw a border surrounding the
    QPen p;
    p.setWidth(2);
    p.setBrush(Qt::yellow);
    painter.setPen(p);
    ///Make sure the border is visible
    viewRect_navCoordinates.adjust(2, 2, -2, -2);
    painter.drawRect(viewRect_navCoordinates);

    ///Now make an image of the requested size of the navigator and center the render image into it
    QImage img(navWidth, navHeight, QImage::Format_ARGB32_Premultiplied);
    img.fill( QColor(71,71,71,255) );

    int xOffset = ( img.width() - renderImage.width() ) / 2;
    int yOffset = ( img.height() - renderImage.height() ) / 2;

    int yDest = yOffset;
    for (int y = 0; y < renderImage.height(); ++y, ++yDest) {
       
        if (yDest >= img.height()) {
            break;
        }
        QRgb* dst_pixels = (QRgb*)img.scanLine(yDest);
        const QRgb* src_pixels = (const QRgb*)renderImage.scanLine(y);
        int xDest = xOffset;
        for (int x = 0; x < renderImage.width(); ++x, ++xDest) {
            if (xDest >= img.width()) {
                dst_pixels[xDest] = qRgba(0, 0, 0, 0);
            } else {
                dst_pixels[xDest] = src_pixels[x];
            }
        }
    }

    return img;
} // getFullSceneScreenShot

const std::list<boost::shared_ptr<NodeGui> > &
NodeGraph::getAllActiveNodes() const
{
    return _imp->_nodes;
}

std::list<boost::shared_ptr<NodeGui> >
NodeGraph::getAllActiveNodes_mt_safe() const
{
    QMutexLocker l(&_imp->_nodesMutex);

    return _imp->_nodes;
}

void
NodeGraph::moveToTrash(NodeGui* node)
{
    assert(node);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin();
         it != _imp->_selection.end(); ++it) {
        if (it->get() == node) {
            (*it)->setUserSelected(false);
            _imp->_selection.erase(it);
            break;
        }
    }
    
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ( (*it).get() == node ) {
            _imp->_nodesTrash.push_back(*it);
            _imp->_nodes.erase(it);
            break;
        }
    }
}

void
NodeGraph::restoreFromTrash(NodeGui* node)
{
    assert(node);
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
        if ( (*it).get() == node ) {
            _imp->_nodes.push_back(*it);
            _imp->_nodesTrash.erase(it);
            break;
        }
    }
}

void
NodeGraph::refreshAllEdges()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->refreshEdges();
    }
}

// grabbed from QDirModelPrivate::size() in qtbase/src/widgets/itemviews/qdirmodel.cpp
static
QString
QDirModelPrivate_size(quint64 bytes)
{
    // According to the Si standard KB is 1000 bytes, KiB is 1024
    // but on windows sizes are calulated by dividing by 1024 so we do what they do.
    const quint64 kb = 1024;
    const quint64 mb = 1024 * kb;
    const quint64 gb = 1024 * mb;
    const quint64 tb = 1024 * gb;

    if (bytes >= tb) {
        return QFileSystemModel::tr("%1 TB").arg( QLocale().toString(qreal(bytes) / tb, 'f', 3) );
    }
    if (bytes >= gb) {
        return QFileSystemModel::tr("%1 GB").arg( QLocale().toString(qreal(bytes) / gb, 'f', 2) );
    }
    if (bytes >= mb) {
        return QFileSystemModel::tr("%1 MB").arg( QLocale().toString(qreal(bytes) / mb, 'f', 1) );
    }
    if (bytes >= kb) {
        return QFileSystemModel::tr("%1 KB").arg( QLocale().toString(bytes / kb) );
    }

    return QFileSystemModel::tr("%1 byte(s)").arg( QLocale().toString(bytes) );
}

void
NodeGraph::updateCacheSizeText()
{
    if (!getGui() || getGui()->isGUIFrozen()) {
        return;
    }
    QString oldText = _imp->_cacheSizeText->toPlainText();
    quint64 cacheSize = appPTR->getCachesTotalMemorySize();
    QString cacheSizeStr = QDirModelPrivate_size(cacheSize);
    QString newText = tr("Memory cache size: ") + cacheSizeStr;
    if (newText != oldText) {
        _imp->_cacheSizeText->setPlainText(newText);
    }
}


void
NodeGraph::toggleCacheInfo()
{
    if ( _imp->_cacheSizeText->isVisible() ) {
        _imp->_cacheSizeText->hide();
    } else {
        _imp->_cacheSizeText->show();
    }
}

void
NodeGraph::toggleKnobLinksVisible()
{
    _imp->_knobLinksVisible = !_imp->_knobLinksVisible;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            (*it)->setKnobLinksVisible(_imp->_knobLinksVisible);
        }
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
            (*it)->setKnobLinksVisible(_imp->_knobLinksVisible);
        }
    }
}

void
NodeGraph::toggleAutoPreview()
{
    _imp->_gui->getApp()->getProject()->toggleAutoPreview();
}

void
NodeGraph::forceRefreshAllPreviews()
{
    _imp->_gui->forceRefreshAllPreviews();
}

void
NodeGraph::showMenu(const QPoint & pos)
{
    _imp->_menu->clear();
    
    QAction* findAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphFindNode,
                                                 kShortcutDescActionGraphFindNode,_imp->_menu);
    _imp->_menu->addAction(findAction);
    _imp->_menu->addSeparator();
    
    //QFont font(appFont,appFontSize);
    Natron::Menu* editMenu = new Natron::Menu(tr("Edit"),_imp->_menu);
    //editMenu->setFont(font);
    _imp->_menu->addAction( editMenu->menuAction() );
    
    QAction* copyAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphCopy,
                                                 kShortcutDescActionGraphCopy,editMenu);
    QObject::connect( copyAction,SIGNAL( triggered() ),this,SLOT( copySelectedNodes() ) );
    editMenu->addAction(copyAction);
    
    QAction* cutAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphCut,
                                                kShortcutDescActionGraphCut,editMenu);
    QObject::connect( cutAction,SIGNAL( triggered() ),this,SLOT( cutSelectedNodes() ) );
    editMenu->addAction(cutAction);
    
    
    QAction* pasteAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphPaste,
                                                  kShortcutDescActionGraphPaste,editMenu);
    pasteAction->setEnabled( !appPTR->isNodeClipBoardEmpty() );
    editMenu->addAction(pasteAction);
    
    QAction* deleteAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphRemoveNodes,
                                                   kShortcutDescActionGraphRemoveNodes,editMenu);
    QObject::connect( deleteAction,SIGNAL( triggered() ),this,SLOT( deleteSelection() ) );
    editMenu->addAction(deleteAction);
    
    QAction* duplicateAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphDuplicate,
                                                      kShortcutDescActionGraphDuplicate,editMenu);
    editMenu->addAction(duplicateAction);
    
    QAction* cloneAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphClone,
                                                  kShortcutDescActionGraphClone,editMenu);
    editMenu->addAction(cloneAction);
    
    QAction* decloneAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphDeclone,
                                                    kShortcutDescActionGraphDeclone,editMenu);
    QObject::connect( decloneAction,SIGNAL( triggered() ),this,SLOT( decloneSelectedNodes() ) );
    editMenu->addAction(decloneAction);
    
    QAction* switchInputs = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphExtractNode,
                                                   kShortcutDescActionGraphExtractNode,editMenu);
    QObject::connect( switchInputs, SIGNAL( triggered() ), this, SLOT( extractSelectedNode() ) );
    editMenu->addAction(switchInputs);
    
    QAction* extractNode = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphSwitchInputs,
                                                   kShortcutDescActionGraphSwitchInputs,editMenu);
    QObject::connect( extractNode, SIGNAL( triggered() ), this, SLOT( switchInputs1and2ForSelectedNodes() ) );
    editMenu->addAction(extractNode);
    
    QAction* disableNodes = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphDisableNodes,
                                                   kShortcutDescActionGraphDisableNodes,editMenu);
    QObject::connect( disableNodes, SIGNAL( triggered() ), this, SLOT( toggleSelectedNodesEnabled() ) );
    editMenu->addAction(disableNodes);
    
    QAction* groupFromSel = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutIDActionGraphMakeGroup,
                                                   kShortcutDescActionGraphMakeGroup,editMenu);
    QObject::connect( groupFromSel, SIGNAL( triggered() ), this, SLOT( createGroupFromSelection() ) );
    editMenu->addAction(groupFromSel);
    
    QAction* expandGroup = new ActionWithShortcut(kShortcutGroupNodegraph, kShortcutIDActionGraphExpandGroup,
                                                   kShortcutDescActionGraphExpandGroup,editMenu);
    QObject::connect( expandGroup, SIGNAL( triggered() ), this, SLOT( expandSelectedGroups() ) );
    editMenu->addAction(expandGroup);
    
    QAction* displayCacheInfoAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphShowCacheSize,
                                                             kShortcutDescActionGraphShowCacheSize,_imp->_menu);
    displayCacheInfoAction->setCheckable(true);
    displayCacheInfoAction->setChecked( _imp->_cacheSizeText->isVisible() );
    QObject::connect( displayCacheInfoAction,SIGNAL( triggered() ),this,SLOT( toggleCacheInfo() ) );
    _imp->_menu->addAction(displayCacheInfoAction);
    
    QAction* turnOffPreviewAction = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphTogglePreview,
                                                           kShortcutDescActionGraphTogglePreview,_imp->_menu);
    turnOffPreviewAction->setCheckable(true);
    turnOffPreviewAction->setChecked(false);
    QObject::connect( turnOffPreviewAction,SIGNAL( triggered() ),this,SLOT( togglePreviewsForSelectedNodes() ) );
    _imp->_menu->addAction(turnOffPreviewAction);
    
    QAction* connectionHints = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphEnableHints,
                                                      kShortcutDescActionGraphEnableHints,_imp->_menu);
    connectionHints->setCheckable(true);
    connectionHints->setChecked( appPTR->getCurrentSettings()->isConnectionHintEnabled() );
    QObject::connect( connectionHints,SIGNAL( triggered() ),this,SLOT( toggleConnectionHints() ) );
    _imp->_menu->addAction(connectionHints);
    
    QAction* autoHideInputs = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphAutoHideInputs,
                                                      kShortcutDescActionGraphAutoHideInputs,_imp->_menu);
    autoHideInputs->setCheckable(true);
    autoHideInputs->setChecked( appPTR->getCurrentSettings()->areOptionalInputsAutoHidden() );
    QObject::connect( autoHideInputs,SIGNAL( triggered() ),this,SLOT( toggleAutoHideInputs() ) );
    _imp->_menu->addAction(autoHideInputs);
    
    QAction* knobLinks = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphShowExpressions,
                                                kShortcutDescActionGraphShowExpressions,_imp->_menu);
    knobLinks->setCheckable(true);
    knobLinks->setChecked( areKnobLinksVisible() );
    QObject::connect( knobLinks,SIGNAL( triggered() ),this,SLOT( toggleKnobLinksVisible() ) );
    _imp->_menu->addAction(knobLinks);
    
    QAction* autoPreview = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphToggleAutoPreview,
                                                  kShortcutDescActionGraphToggleAutoPreview,_imp->_menu);
    autoPreview->setCheckable(true);
    autoPreview->setChecked( _imp->_gui->getApp()->getProject()->isAutoPreviewEnabled() );
    QObject::connect( autoPreview,SIGNAL( triggered() ),this,SLOT( toggleAutoPreview() ) );
    QObject::connect( _imp->_gui->getApp()->getProject().get(),SIGNAL( autoPreviewChanged(bool) ),autoPreview,SLOT( setChecked(bool) ) );
    _imp->_menu->addAction(autoPreview);
    
    QAction* autoTurbo = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphToggleAutoTurbo,
                                               kShortcutDescActionGraphToggleAutoTurbo,_imp->_menu);
    autoTurbo->setCheckable(true);
    autoTurbo->setChecked( appPTR->getCurrentSettings()->isAutoTurboEnabled() );
    QObject::connect( autoTurbo,SIGNAL( triggered() ),this,SLOT( toggleAutoTurbo() ) );
    _imp->_menu->addAction(autoTurbo);

    
    QAction* forceRefreshPreviews = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphForcePreview,
                                                           kShortcutDescActionGraphForcePreview,_imp->_menu);
    QObject::connect( forceRefreshPreviews,SIGNAL( triggered() ),this,SLOT( forceRefreshAllPreviews() ) );
    _imp->_menu->addAction(forceRefreshPreviews);
    
    QAction* frameAllNodes = new ActionWithShortcut(kShortcutGroupNodegraph,kShortcutIDActionGraphFrameNodes,
                                                    kShortcutDescActionGraphFrameNodes,_imp->_menu);
    QObject::connect( frameAllNodes,SIGNAL( triggered() ),this,SLOT( centerOnAllNodes() ) );
    _imp->_menu->addAction(frameAllNodes);
    
    _imp->_menu->addSeparator();
    
    std::list<ToolButton*> orederedToolButtons = _imp->_gui->getToolButtonsOrdered();
    for (std::list<ToolButton*>::iterator it = orederedToolButtons.begin(); it != orederedToolButtons.end(); ++it) {
        (*it)->getMenu()->setIcon( (*it)->getIcon() );
        _imp->_menu->addAction( (*it)->getMenu()->menuAction() );
    }
    
    QAction* ret = _imp->_menu->exec(pos);
    if (ret == findAction) {
        popFindDialog();
    } else if (ret == duplicateAction) {
        QRectF rect = visibleSceneRect();
        duplicateSelectedNodes(rect.center());
    } else if (ret == cloneAction) {
        QRectF rect = visibleSceneRect();
        cloneSelectedNodes(rect.center());
    } else if (ret == pasteAction) {
        QRectF rect = visibleSceneRect();
        pasteNodeClipBoards(rect.center());
    }
}

void
NodeGraph::dropEvent(QDropEvent* e)
{
    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    QStringList filesList;
    QList<QUrl> urls = e->mimeData()->urls();
    for (int i = 0; i < urls.size(); ++i) {
        const QUrl rl = Natron::toLocalFileUrlFixed(urls.at(i));
        QString path = rl.toLocalFile();

#ifdef __NATRON_WIN32__
        if ( !path.isEmpty() && ( path.at(0) == QChar('/') ) || ( path.at(0) == QChar('\\') ) ) {
            path = path.remove(0,1);
        }

#endif
        
        QDir dir(path);

        //if the path dropped is not a directory append it
        if ( !dir.exists() ) {
            filesList << path;
        } else {
            //otherwise append everything inside the dir recursively
            SequenceFileDialog::appendFilesFromDirRecursively(&dir,&filesList);
        }
    }

    QStringList supportedExtensions;
    std::map<std::string,std::string> writersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    for (std::map<std::string,std::string>::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it) {
        supportedExtensions.push_back( it->first.c_str() );
    }
    QPointF scenePos = mapToScene(e->pos());
    std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> > files = SequenceFileDialog::fileSequencesFromFilesList(filesList,supportedExtensions);
    std::locale local;
    for (U32 i = 0; i < files.size(); ++i) {
        ///get all the decoders
        std::map<std::string,std::string> readersForFormat;
        appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);

        boost::shared_ptr<SequenceParsing::SequenceFromFiles> & sequence = files[i];

        ///find a decoder for this file type
        std::string ext = sequence->fileExtension();
        std::string extLower;
        for (size_t j = 0; j < ext.size(); ++j) {
            extLower.append( 1,std::tolower( ext.at(j),local ) );
        }
        std::map<std::string,std::string>::iterator found = readersForFormat.find(extLower);
        if ( found == readersForFormat.end() ) {
            errorDialog("Reader", "No plugin capable of decoding " + extLower + " was found.");
        } else {
            
            std::string pattern = sequence->generateValidSequencePattern();
            CreateNodeArgs::DefaultValuesList defaultValues;
            defaultValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, pattern));
            
            CreateNodeArgs args(found->second.c_str(),
                                "",
                                -1,
                                -1,
                                false,
                                scenePos.x(),scenePos.y(),
                                true,
                                true,
                                false,
                                QString(),
                                defaultValues,
                                getGroup());
            boost::shared_ptr<Natron::Node>  n = getGui()->getApp()->createNode(args);
            
            //And offset scenePos by the Width of the previous node created if several nodes are created
            double w,h;
            n->getSize(&w, &h);
            scenePos.rx() += (w + 10);
        }
    }
} // dropEvent

void
NodeGraph::dragEnterEvent(QDragEnterEvent* e)
{
    e->accept();
}

void
NodeGraph::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}

void
NodeGraph::dragMoveEvent(QDragMoveEvent* e)
{
    e->accept();
}

void
NodeGraph::togglePreviewsForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin();
         it != _imp->_selection.end();
         ++it) {
        (*it)->togglePreview();
    }
}

void
NodeGraph::switchInputs1and2ForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin();
         it != _imp->_selection.end();
         ++it) {
        (*it)->onSwitchInputActionTriggered();
    }
}

void
NodeGraph::centerOnItem(QGraphicsItem* item)
{
    _imp->_refreshOverlays = true;
    centerOn(item);
}

void
NodeGraph::copyNodes(const std::list<boost::shared_ptr<NodeGui> >& nodes,NodeClipBoard& clipboard)
{
    _imp->copyNodesInternal(nodes, clipboard);
}

void
NodeGraph::copySelectedNodes()
{
    if ( _imp->_selection.empty()) {
        Natron::warningDialog( tr("Copy").toStdString(), tr("You must select at least a node to copy first.").toStdString() );

        return;
    }

    _imp->copyNodesInternal(_imp->_selection,appPTR->getNodeClipBoard());
}


void
NodeGraph::cutSelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Natron::warningDialog( tr("Cut").toStdString(), tr("You must select at least a node to cut first.").toStdString() );

        return;
    }
    copySelectedNodes();
    deleteSelection();
}

void
NodeGraph::pasteCliboard(const NodeClipBoard& clipboard,std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > >* newNodes)
{
    QPointF position = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    _imp->pasteNodesInternal(clipboard,position, false, newNodes);
}

void
NodeGraph::pasteNodeClipBoards(const QPointF& pos)
{
    std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
    _imp->pasteNodesInternal(appPTR->getNodeClipBoard(),pos, true, &newNodes);
}

void
NodeGraph::pasteNodeClipBoards()
{
    QPointF position = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    pasteNodeClipBoards(position);
}


void
NodeGraph::duplicateSelectedNodes(const QPointF& pos)
{
    if ( _imp->_selection.empty() && _imp->_selection.empty() ) {
        Natron::warningDialog( tr("Duplicate").toStdString(), tr("You must select at least a node to duplicate first.").toStdString() );
        
        return;
    }
    
    ///Don't use the member clipboard as the user might have something copied
    NodeClipBoard tmpClipboard;
    _imp->copyNodesInternal(_imp->_selection,tmpClipboard);
    std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
    _imp->pasteNodesInternal(tmpClipboard,pos,true,&newNodes);

}


void
NodeGraph::duplicateSelectedNodes()
{
    QPointF scenePos = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    duplicateSelectedNodes(scenePos);
}

void
NodeGraph::cloneSelectedNodes(const QPointF& scenePos)
{
    if (_imp->_selection.empty()) {
        Natron::warningDialog( tr("Clone").toStdString(), tr("You must select at least a node to clone first.").toStdString() );
        return;
    }
    
    double xmax = INT_MIN;
    double xmin = INT_MAX;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    NodeGuiList nodesToCopy = _imp->_selection;
    for (NodeGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        if ( (*it)->getNode()->getMasterNode()) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString() );
            return;
        }
        QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
        if ( ( bbox.x() + bbox.width() ) > xmax ) {
            xmax = ( bbox.x() + bbox.width() );
        }
        if (bbox.x() < xmin) {
            xmin = bbox.x();
        }
        
        if ( ( bbox.y() + bbox.height() ) > ymax ) {
            ymax = ( bbox.y() + bbox.height() );
        }
        if (bbox.y() < ymin) {
            ymin = bbox.y();
        }
        
        ///Also copy all nodes within the backdrop
        BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
        if (isBd) {
            NodeGuiList nodesWithinBD = getNodesWithinBackDrop(*it);
            for (NodeGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodeGuiList::iterator found = std::find(nodesToCopy.begin(),nodesToCopy.end(),*it2);
                if ( found == nodesToCopy.end() ) {
                    nodesToCopy.push_back(*it2);
                }
            }
        }
    }
    
    for (NodeGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        if ( (*it)->getNode()->getLiveInstance()->isSlave() ) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString() );
            
            return;
        }
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>((*it)->getNode()->getLiveInstance());
        if (isViewer) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("Cloning a viewer is not a valid operation.").toStdString() );
            
            return;
        }
        if ( (*it)->getNode()->isMultiInstance() ) {
            QString err = QString("%1 cannot be cloned.").arg( (*it)->getNode()->getLabel().c_str() );
            Natron::errorDialog( tr("Clone").toStdString(),
                                tr( err.toStdString().c_str() ).toStdString() );
            
            return;
        }
    }
    
    QPointF offset(scenePos.x() - ((xmax + xmin) / 2.), scenePos.y() -  ((ymax + ymin) / 2.));
    std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
    std::list <boost::shared_ptr<NodeSerialization> > serializations;
    
    std::list <boost::shared_ptr<NodeGui> > newNodesList;
    
    for (NodeGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        boost::shared_ptr<NodeSerialization>  internalSerialization( new NodeSerialization( (*it)->getNode() ) );
        NodeGuiSerialization guiSerialization;
        (*it)->serialize(&guiSerialization);
        boost::shared_ptr<NodeGui> clone = _imp->pasteNode( *internalSerialization, guiSerialization, offset,
                                                           _imp->group.lock(),std::string(),true );
        
        newNodes.push_back(std::make_pair(internalSerialization->getNodeScriptName(),clone));
        newNodesList.push_back(clone);
        serializations.push_back(internalSerialization);
        
        ///The script-name of the copy node is different than the one of the original one, update all input connections in the serialization
        for (std::list<boost::shared_ptr<NodeSerialization> >::iterator it2 = serializations.begin(); it2!=serializations.end(); ++it2) {
            (*it2)->switchInput(internalSerialization->getNodeScriptName(), clone->getNode()->getScriptName());
        }
        
        
    }
    
    
    assert( serializations.size() == newNodes.size() );
    ///restore connections
    _imp->restoreConnections(serializations, newNodes);
    
    
    pushUndoCommand( new AddMultipleNodesCommand(this,newNodesList) );
}

void
NodeGraph::cloneSelectedNodes()
{
    QPointF scenePos = _imp->_root->mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
    cloneSelectedNodes(scenePos);
    
} // cloneSelectedNodes

void
NodeGraph::decloneSelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Natron::warningDialog( tr("Declone").toStdString(), tr("You must select at least a node to declone first.").toStdString() );

        return;
    }
    std::list<boost::shared_ptr<NodeGui> > nodesToDeclone;


    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        
        BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
        if (isBd) {
            ///Also copy all nodes within the backdrop
            NodeGuiList nodesWithinBD = getNodesWithinBackDrop(*it);
            for (NodeGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodeGuiList::iterator found = std::find(nodesToDeclone.begin(),nodesToDeclone.end(),*it2);
                if ( found == nodesToDeclone.end() ) {
                    nodesToDeclone.push_back(*it2);
                }
            }
        }
        if ( (*it)->getNode()->getLiveInstance()->isSlave() ) {
            nodesToDeclone.push_back(*it);
        }
    }

    pushUndoCommand( new DecloneMultipleNodesCommand(this,nodesToDeclone) );
}

void
NodeGraph::setUndoRedoStackLimit(int limit)
{
    _imp->_undoStack->clear();
    _imp->_undoStack->setUndoLimit(limit);
}

void
NodeGraph::deleteNodepluginsly(boost::shared_ptr<NodeGui> n)
{
    assert(n);
    boost::shared_ptr<Natron::Node> internalNode = n->getNode();

    if (internalNode) {
        internalNode->deactivate(std::list< Natron::Node* >(),false,false,true,false);
    }
    std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_imp->_nodesTrash.begin(),_imp->_nodesTrash.end(),n);

    if ( it != _imp->_nodesTrash.end() ) {
        _imp->_nodesTrash.erase(it);
    }

    {
        QMutexLocker l(&_imp->_nodesMutex);
        std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_imp->_nodes.begin(),_imp->_nodes.end(),n);
        if ( it != _imp->_nodes.end() ) {
            _imp->_nodes.erase(it);
        }
    }


    n->deleteReferences();
    n->discardGraphPointer();

    if ( getGui() ) {
        
        if (internalNode->isRotoPaintingNode()) {
            getGui()->removeRotoInterface(n.get(),true);
        }
        
        if (internalNode->isPointTrackerNode()) {
            getGui()->removeTrackerInterface(n.get(), true);
        }

        ///now that we made the command dirty, delete the node everywhere in Natron
        getGui()->getApp()->deleteNode(n);


        getGui()->getCurveEditor()->removeNode( n.get() );
        std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.begin(),_imp->_selection.end(),n);
        if ( found != _imp->_selection.end() ) {
            n->setUserSelected(false);
            _imp->_selection.erase(found);
        }
        
        if (internalNode && internalNode->getLiveInstance()) {
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(internalNode->getLiveInstance());
            if (isGrp) {
                NodeGraphI* graph_i = isGrp->getNodeGraph();
                if (graph_i) {
                    NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
                    assert(graph);
                    if (graph) {
                        getGui()->removeGroupGui(graph, true);
                    }
                }
            }
        }
    }
    
    if (internalNode) {
        ///remove the node from the clipboard if it is
        NodeClipBoard &cb = appPTR->getNodeClipBoard();
        for (std::list< boost::shared_ptr<NodeSerialization> >::iterator it = cb.nodes.begin();
             it != cb.nodes.end(); ++it) {
            if ( (*it)->getNode() == internalNode ) {
                cb.nodes.erase(it);
                break;
            }
        }
        
        for (std::list<boost::shared_ptr<NodeGuiSerialization> >::iterator it = cb.nodesUI.begin();
             it != cb.nodesUI.end(); ++it) {
            if ( (*it)->getFullySpecifiedName() == internalNode->getFullyQualifiedName() ) {
                cb.nodesUI.erase(it);
                break;
            }
        }
    }
} // deleteNodepluginsly

void
NodeGraph::invalidateAllNodesParenting()
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( it->get() );
        }
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( it->get() );
        }
    }

}

void
NodeGraph::centerOnAllNodes()
{
    assert( QThread::currentThread() == qApp->thread() );
    double xmin = INT_MAX;
    double xmax = INT_MIN;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    //_imp->_root->setPos(0,0);

    if (_imp->_selection.empty()) {
        QMutexLocker l(&_imp->_nodesMutex);

        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( /*(*it)->isActive() &&*/ (*it)->isVisible() ) {
                QSize size = (*it)->getSize();
                QPointF pos = (*it)->mapToScene((*it)->mapFromParent((*it)->getPos_mt_safe()));
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax,pos.x() + size.width() );
                ymin = std::min( ymin,pos.y() );
                ymax = std::max( ymax,pos.y() + size.height() );
            }
        }
        
    } else {
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            if ( /*(*it)->isActive() && */(*it)->isVisible() ) {
                QSize size = (*it)->getSize();
                QPointF pos = (*it)->mapToScene((*it)->mapFromParent((*it)->getPos_mt_safe()));
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax,pos.x() + size.width() );
                ymin = std::min( ymin,pos.y() );
                ymax = std::max( ymax,pos.y() + size.height() ); 
            }
        }

    }
    QRectF bbox( xmin,ymin,(xmax - xmin),(ymax - ymin) );
    fitInView(bbox,Qt::KeepAspectRatio);
    
    double currentZoomFactor = transform().mapRect( QRectF(0, 0, 1, 1) ).width();
    assert(currentZoomFactor != 0);
    //we want to fit at scale 1 at most
    if (currentZoomFactor > 1.) {
        double scaleFactor = 1. / currentZoomFactor;
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        scale(scaleFactor,scaleFactor);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }
    
    currentZoomFactor = transform().mapRect( QRectF(0, 0, 1, 1) ).width();
    if (currentZoomFactor < 0.4) {
        setVisibleNodeDetails(false);
    } else if (currentZoomFactor >= 0.4) {
        setVisibleNodeDetails(true);
    }

    _imp->_refreshOverlays = true;
    update();
}

void
NodeGraph::toggleConnectionHints()
{
    appPTR->getCurrentSettings()->setConnectionHintsEnabled( !appPTR->getCurrentSettings()->isConnectionHintEnabled() );
}

void
NodeGraph::toggleAutoHideInputs(bool setSettings)
{
    bool autoHide ;
    if (setSettings) {
        autoHide = !appPTR->getCurrentSettings()->areOptionalInputsAutoHidden();
        appPTR->getCurrentSettings()->setOptionalInputsAutoHidden(autoHide);
    } else {
        autoHide = appPTR->getCurrentSettings()->areOptionalInputsAutoHidden();
    }
    if (!autoHide) {
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            (*it)->setOptionalInputsVisible(true);
        }
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
            (*it)->setOptionalInputsVisible(true);
        }
    } else {
        
        QPointF evpt = mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            
            QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
            if (!(*it)->getIsSelected() && !bbox.contains(evpt)) {
                (*it)->setOptionalInputsVisible(false);
            }
            
        }
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
            (*it)->setOptionalInputsVisible(false);
        }
    }
}

std::list<boost::shared_ptr<NodeGui> > NodeGraph::getNodesWithinBackDrop(const boost::shared_ptr<NodeGui>& bd) const
{
    BackDropGui* isBd = dynamic_cast<BackDropGui*>(bd.get());
    if (!isBd) {
        return std::list<boost::shared_ptr<NodeGui> >();
    }
    
    QRectF bbox = bd->mapToScene( bd->boundingRect() ).boundingRect();
    std::list<boost::shared_ptr<NodeGui> > ret;
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        QRectF nodeBbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( bbox.contains(nodeBbox) ) {
            ret.push_back(*it);
        }
    }

    return ret;
}

void
NodeGraph::refreshNodesKnobsAtTime(SequenceTime time)
{
    ///Refresh all knobs at the current time
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->refreshKnobsAfterTimeChange(time);
    }
}

void
NodeGraph::onTimelineTimeAboutToChange()
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->wasLaskUserSeekDuringPlayback = false;
    const std::list<ViewerTab*>& viewers = _imp->_gui->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        RenderEngine* engine = (*it)->getInternalNode()->getRenderEngine();
        _imp->wasLaskUserSeekDuringPlayback |= engine->abortRendering(true);
    }
}

void
NodeGraph::onTimeChanged(SequenceTime time,
                         int reason)
{
    assert(QThread::currentThread() == qApp->thread());
    std::vector<ViewerInstance* > viewers;

    if (!_imp->_gui) {
        return;
    }
    boost::shared_ptr<Natron::Project> project = _imp->_gui->getApp()->getProject();

    ///Refresh all knobs at the current time
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( (*it)->getNode()->getLiveInstance() );
        if (isViewer) {
            viewers.push_back(isViewer);
        }
        (*it)->refreshKnobsAfterTimeChange(time);
    }
    
    ViewerInstance* leadViewer = getGui()->getApp()->getLastViewerUsingTimeline();

    bool isUserEdited = reason == eTimelineChangeReasonUserSeek ||
    reason == eTimelineChangeReasonDopeSheetEditorSeek ||
    reason == eTimelineChangeReasonCurveEditorSeek;
    
    bool startPlayback = isUserEdited && _imp->wasLaskUserSeekDuringPlayback;
    
    ///Syncrhronize viewers
    for (U32 i = 0; i < viewers.size(); ++i) {
        if (!startPlayback) {
            if (viewers[i] != leadViewer || isUserEdited) {
                viewers[i]->renderCurrentFrame(reason != eTimelineChangeReasonPlaybackSeek);
            }
        } else {
            viewers[i]->renderFromCurrentFrameUsingCurrentDirection();
        }
    }
}

void
NodeGraph::onGuiFrozenChanged(bool frozen)
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->onGuiFrozenChanged(frozen);
    }
}

void
NodeGraph::refreshAllKnobsGui()
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ((*it)->isSettingsPanelVisible()) {
            const std::map<boost::weak_ptr<KnobI>,KnobGui*> & knobs = (*it)->getKnobs();
            
            for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it2 = knobs.begin(); it2!=knobs.end(); ++it2) {
                boost::shared_ptr<KnobI> knob = it2->first.lock();
                if (!knob->getIsSecret()) {
                    for (int i = 0; i < knob->getDimension(); ++i) {
                        if (knob->isAnimated(i)) {
                            it2->second->onInternalValueChanged(i, Natron::eValueChangedReasonPluginEdited);
                            it2->second->onAnimationLevelChanged(i, Natron::eValueChangedReasonPluginEdited);
                        }
                    }
                }
            }
        }
    }
}

void
NodeGraph::focusInEvent(QFocusEvent* e)
{
    QGraphicsView::focusInEvent(e);
    if (_imp->_gui) {
        _imp->_gui->setLastSelectedGraph(this);
    }
}

void
NodeGraph::focusOutEvent(QFocusEvent* e)
{
    if (_imp->_bendPointsVisible) {
        _imp->setNodesBendPointsVisible(false);
    }
    QGraphicsView::focusOutEvent(e);
}

void
NodeGraph::toggleSelectedNodesEnabled()
{
    _imp->toggleSelectedNodesEnabled();
}


bool
NodeGraph::areKnobLinksVisible() const
{
    return _imp->_knobLinksVisible;
}

void
NodeGraph::popFindDialog(const QPoint& p)
{
    QPoint realPos = p;

    FindNodeDialog* dialog = new FindNodeDialog(this,this);
    
    if (realPos.x() == 0 && realPos.y() == 0) {
        QPoint global = QCursor::pos();
        QSize sizeH = dialog->sizeHint();
        global.rx() -= sizeH.width() / 2;
        global.ry() -= sizeH.height() / 2;
        realPos = global;
        
    }
    
    QObject::connect(dialog ,SIGNAL(rejected()), this, SLOT(onFindNodeDialogFinished()));
    QObject::connect(dialog ,SIGNAL(accepted()), this, SLOT(onFindNodeDialogFinished()));
    dialog->move( realPos.x(), realPos.y() );
    dialog->raise();
    dialog->show();
    
}

void
NodeGraph::popRenameDialog(const QPoint& pos)
{
    boost::shared_ptr<NodeGui> node;
    if (_imp->_selection.size() == 1 ) {
        node = _imp->_selection.front();
    } else {
        return;
    }
    
    assert(node);

    
    QPoint realPos = pos;
    
    EditNodeNameDialog* dialog = new EditNodeNameDialog(this,node,this);
    
    if (realPos.x() == 0 && realPos.y() == 0) {
        QPoint global = QCursor::pos();
        QSize sizeH = dialog->sizeHint();
        global.rx() -= sizeH.width() / 2;
        global.ry() -= sizeH.height() / 2;
        realPos = global;
        
    }
    
    QObject::connect(dialog ,SIGNAL(rejected()), this, SLOT(onNodeNameEditDialogFinished()));
    QObject::connect(dialog ,SIGNAL(accepted()), this, SLOT(onNodeNameEditDialogFinished()));
    dialog->move( realPos.x(), realPos.y() );
    dialog->raise();
    dialog->show();
  
}

void
NodeGraph::onFindNodeDialogFinished()
{
    FindNodeDialog* dialog = qobject_cast<FindNodeDialog*>( sender() );
    
    if (dialog) {
        dialog->deleteLater();
    }
}

struct FindNodeDialogPrivate
{
    NodeGraph* graph;
    
    QString currentFilter;
    std::list<boost::shared_ptr<NodeGui> > nodeResults;
    int currentFindIndex;
    
    QVBoxLayout* mainLayout;
    Natron::Label* label;
    

    QCheckBox* unixWildcards;
    QCheckBox* caseSensitivity;

    Natron::Label* resultLabel;
    LineEdit* filter;
    QDialogButtonBox* buttons;
    
    
    FindNodeDialogPrivate(NodeGraph* graph)
    : graph(graph)
    , currentFilter()
    , nodeResults()
    , currentFindIndex(-1)
    , mainLayout(0)
    , label(0)
    , unixWildcards(0)
    , caseSensitivity(0)
    , resultLabel(0)
    , filter(0)
    , buttons(0)
    {
        
    }
};

FindNodeDialog::FindNodeDialog(NodeGraph* graph,QWidget* parent)
: QDialog(parent)
, _imp(new FindNodeDialogPrivate(graph))
{
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->label = new Natron::Label(tr("Select all nodes containing this text:"),this);
    //_imp->label->setFont(QFont(appFont,appFontSize));
    _imp->mainLayout->addWidget(_imp->label);

    _imp->filter = new LineEdit(this);
    QObject::connect(_imp->filter, SIGNAL(editingFinished()), this, SLOT(updateFindResultsWithCurrentFilter()));
    QObject::connect(_imp->filter, SIGNAL(textEdited(QString)), this, SLOT(updateFindResults(QString)));
    
    _imp->mainLayout->addWidget(_imp->filter);
    
    
    _imp->unixWildcards = new QCheckBox(tr("Use Unix wildcards (*, ?, etc..)"),this);
    _imp->unixWildcards->setChecked(false);
    QObject::connect(_imp->unixWildcards, SIGNAL(toggled(bool)), this, SLOT(forceUpdateFindResults()));
    _imp->mainLayout->addWidget(_imp->unixWildcards);
    
    _imp->caseSensitivity = new QCheckBox(tr("Case sensitive"),this);
    _imp->caseSensitivity->setChecked(false);
    QObject::connect(_imp->caseSensitivity, SIGNAL(toggled(bool)), this, SLOT(forceUpdateFindResults()));
    _imp->mainLayout->addWidget(_imp->caseSensitivity);
    
    
    _imp->resultLabel = new Natron::Label(this);
    _imp->mainLayout->addWidget(_imp->resultLabel);
    //_imp->resultLabel->setFont(QFont(appFont,appFontSize));
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal,this);
    QObject::connect(_imp->buttons, SIGNAL(accepted()), this, SLOT(onOkClicked()));
    QObject::connect(_imp->buttons, SIGNAL(rejected()), this, SLOT(onCancelClicked()));
    
    _imp->mainLayout->addWidget(_imp->buttons);
    _imp->filter->setFocus();
}

FindNodeDialog::~FindNodeDialog()
{
    
}

void
FindNodeDialog::updateFindResults(const QString& filter)
{
    if (filter == _imp->currentFilter) {
        return;
    }

    _imp->currentFilter = filter;
    _imp->currentFindIndex = 0;
    _imp->nodeResults.clear();
    
    _imp->graph->deselect();
    
    if (_imp->currentFilter.isEmpty()) {
        _imp->resultLabel->setText("");
        return;
    }
    Qt::CaseSensitivity sensitivity = _imp->caseSensitivity->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    
    const std::list<boost::shared_ptr<NodeGui> >& activeNodes = _imp->graph->getAllActiveNodes();
    
    if (_imp->unixWildcards->isChecked()) {
        QRegExp exp(filter,sensitivity,QRegExp::Wildcard);
        if (!exp.isValid()) {
            return;
        }
        
        
        
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
            if ((*it)->isVisible() && exp.exactMatch((*it)->getNode()->getLabel().c_str())) {
                _imp->nodeResults.push_back(*it);
            }
        }
    } else {
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
            if ((*it)->isVisible() && QString((*it)->getNode()->getLabel().c_str()).contains(filter,sensitivity)) {
                _imp->nodeResults.push_back(*it);
            }
        }

    }
    
    if ((_imp->nodeResults.size()) == 0) {
        _imp->resultLabel->setText("");
    }

    
    selectNextResult();
}

void
FindNodeDialog::selectNextResult()
{
    if (_imp->currentFindIndex >= (int)(_imp->nodeResults.size())) {
        _imp->currentFindIndex = 0;
    }
    
    if (_imp->nodeResults.empty()) {
        return;
    }
    
    std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->nodeResults.begin();
    std::advance(it,_imp->currentFindIndex);
    
    _imp->graph->selectNode(*it, false);
    _imp->graph->centerOnItem(it->get());
    
    
    QString text = QString("Selecting result %1 of %2").arg(_imp->currentFindIndex + 1).arg(_imp->nodeResults.size());
    _imp->resultLabel->setText(text);

    
    ++_imp->currentFindIndex;
    
}



void
FindNodeDialog::updateFindResultsWithCurrentFilter()
{
    updateFindResults(_imp->filter->text());
    
}

void
FindNodeDialog::forceUpdateFindResults()
{
    _imp->currentFilter.clear();
    updateFindResultsWithCurrentFilter();
}


void
FindNodeDialog::onOkClicked()
{
    QString filterText = _imp->filter->text();
    if (_imp->currentFilter != filterText) {
        updateFindResults(filterText);
    } else {
        selectNextResult();
    }
}

void
FindNodeDialog::onCancelClicked()
{
    reject();
}

void
FindNodeDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        selectNextResult();
        _imp->filter->setFocus();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
FindNodeDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange) {
        if ( !isActiveWindow() ) {
            reject();
            
            return;
        }
    }
    QDialog::changeEvent(e);
}


struct EditNodeNameDialogPrivate
{
    
    LineEdit* field;
    boost::shared_ptr<NodeGui> node;
    NodeGraph* graph;
    
    EditNodeNameDialogPrivate(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node)
    : field(0)
    , node(node)
    , graph(graph)
    {
        
    }
};

EditNodeNameDialog::EditNodeNameDialog(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QWidget* parent)
: QDialog(parent)
, _imp(new EditNodeNameDialogPrivate(graph,node))
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    _imp->field = new LineEdit(this);
    _imp->field->setPlaceholderText(tr("Edit node name"));
    mainLayout->addWidget(_imp->field);
}

EditNodeNameDialog::~EditNodeNameDialog()
{
    
}


void
EditNodeNameDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange) {
        if ( !isActiveWindow() ) {
            reject();
            
            return;
        }
    }
    QDialog::changeEvent(e);
}

void
EditNodeNameDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        QString newName = _imp->field->text();
        QString oldName = QString(_imp->node->getNode()->getLabel().c_str());
        _imp->graph->pushUndoCommand(new RenameNodeUndoRedoCommand(_imp->node,oldName,newName));
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
NodeGraph::onNodeNameEditDialogFinished()
{
    EditNodeNameDialog* dialog = qobject_cast<EditNodeNameDialog*>(sender());
    if (dialog) {
        dialog->deleteLater();
    }
}

void
NodeGraph::extractSelectedNode()
{
    pushUndoCommand(new ExtractNodeUndoRedoCommand(this,_imp->_selection));
}

void
NodeGraph::createGroupFromSelection()
{
    pushUndoCommand(new GroupFromSelectionCommand(this,_imp->_selection));
}

void
NodeGraph::expandSelectedGroups()
{
    NodeGuiList nodes;
    for (NodeGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>((*it)->getNode()->getLiveInstance());
        if (isGroup) {
            nodes.push_back(*it);
        }
    }
    if (!nodes.empty()) {
        pushUndoCommand(new InlineGroupCommand(this,nodes));
    } else {
        Natron::warningDialog(tr("Expand group").toStdString(), tr("You must select a group to expand first").toStdString());
    }
}

void
NodeGraph::onGroupNameChanged(const QString& name)
{
    
    setLabel(name.toStdString());
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget() );
    if (parent) {
        parent->setTabLabel(this, name);
    }
}

void
NodeGraph::onGroupScriptNameChanged(const QString& /*name*/)
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    boost::shared_ptr<NodeCollection> group = getGroup();
    if (!group) {
        return;
    }
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(group.get());
    if (!isGrp) {
        return;
    }
    std::string newName = isGrp->getNode()->getFullyQualifiedName();
    for (std::size_t i = 0; i < newName.size(); ++i) {
        if (newName[i] == '.') {
            newName[i] = '_';
        }
    }
    std::string oldName = getScriptName();
    for (std::size_t i = 0; i < oldName.size(); ++i) {
        if (oldName[i] == '.') {
            oldName[i] = '_';
        }
    }
    getGui()->unregisterTab(this);
    setScriptName(newName);
    getGui()->registerTab(this,this);
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget() );
    if (parent) {
        parent->onTabScriptNameChanged(this, oldName, newName);
    }

}

void
NodeGraph::copyNodesAndCreateInGroup(const std::list<boost::shared_ptr<NodeGui> >& nodes,
                                     const boost::shared_ptr<NodeCollection>& group)
{
    NodeClipBoard clipboard;
    _imp->copyNodesInternal(nodes,clipboard);
    
    std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
    std::list<boost::shared_ptr<NodeSerialization> >::const_iterator itOther = clipboard.nodes.begin();
    for (std::list<boost::shared_ptr<NodeGuiSerialization> >::const_iterator it = clipboard.nodesUI.begin();
         it != clipboard.nodesUI.end(); ++it, ++itOther) {
        boost::shared_ptr<NodeGui> node = _imp->pasteNode( **itOther,**it,QPointF(0,0),group,std::string(), false);
        newNodes.push_back(std::make_pair((*itOther)->getNodeScriptName(),node));
    }
    assert( clipboard.nodes.size() == newNodes.size() );
    
    ///Now that all nodes have been duplicated, try to restore nodes connections
    _imp->restoreConnections(clipboard.nodes, newNodes);

}

QPointF
NodeGraph::getRootPos() const
{
    return _imp->_root->pos();
}
