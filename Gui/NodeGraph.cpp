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

