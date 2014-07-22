//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "NodeGraph.h"

#include <cstdlib>
#include <set>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QGraphicsProxyWidget>
CLANG_DIAG_ON(unused-private-field)
#include <QGraphicsTextItem>
#include <QFileSystemModel>
#include <QScrollArea>
#include <QScrollBar>
#include <QComboBox>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QGraphicsLineItem>
#include <QUndoStack>
#include <QMenu>
#include <QThread>
#include <QDropEvent>
#include <QCoreApplication>
#include <QMimeData>
#include <QLineEdit>
#include <QDebug>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QLabel>
#include <QAction>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <SequenceParsing.h>

#include "Engine/AppManager.h"

#include "Engine/VideoEngine.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Hash64.h"
#include "Engine/FrameEntry.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/Project.h"
#include "Engine/Plugin.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Node.h"

#include "Gui/TabWidget.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/DockablePanel.h"
#include "Gui/ToolButton.h"
#include "Gui/KnobGui.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Gui/Gui.h"
#include "Gui/TimeLineGui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/CurveEditor.h"
#include "Gui/NodeBackDrop.h"
#include "Gui/NodeBackDropSerialization.h"
#include "Gui/NodeGraphUndoRedo.h"

#define NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS 1000

#define NATRON_BACKDROP_DEFAULT_WIDTH 80
#define NATRON_BACKDROP_DEFAULT_HEIGHT 80

#define NATRON_NODE_DUPLICATE_X_OFFSET 50

using namespace Natron;
using std::cout; using std::endl;


namespace
{
    struct NodeClipBoard {
        boost::shared_ptr<NodeSerialization> _internal;
        boost::shared_ptr<NodeGuiSerialization> _gui;
        
        NodeClipBoard()
        : _internal()
        , _gui()
        {
        }
        
        bool isEmpty() const { return !_internal || !_gui; }
    };
    
    enum EVENT_STATE
    {
        DEFAULT,
        MOVING_AREA,
        ARROW_DRAGGING,
        NODE_DRAGGING,
        BACKDROP_DRAGGING,
        BACKDROP_RESIZING
    };
    
    class NodeGraphNavigator : public QLabel
    {
        int _w,_h;
    public:
        
        explicit NodeGraphNavigator(QWidget* parent = 0);
        
        void setImage(const QImage& img);
        
        virtual QSize sizeHint() const OVERRIDE FINAL {return QSize(_w,_h);};
        
        virtual ~NodeGraphNavigator(){}
    };

}

struct NodeGraphPrivate
{
    Gui* _gui;
    
    QPointF _lastScenePosClick;
    
    QPointF _lastNodeDragStartPoint;
    
    EVENT_STATE _evtState;
    
    boost::shared_ptr<NodeGui> _nodeSelected;
    double _nodeSelectedScaleBeforeMagnif;
    bool _magnifOn;
    
    Edge* _arrowSelected;
    
    mutable QMutex _nodesMutex;
    
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    std::list<boost::shared_ptr<NodeGui> > _nodesTrash;
    
    ///Enables the "Tab" shortcut to popup the node creation dialog.
    ///This is set to true on enterEvent and set back to false on leaveEvent
    bool _nodeCreationShortcutEnabled;
    
    QGraphicsItem* _root; ///< this is the parent of all items in the graph
    QGraphicsItem* _nodeRoot; ///< this is the parent of all nodes
    
    QScrollArea* _propertyBin;
    
    QGraphicsTextItem* _cacheSizeText;
    
    QTimer _refreshCacheTextTimer;
    
    NodeGraphNavigator* _navigator;
    
    QGraphicsLineItem* _navLeftEdge;
    QGraphicsLineItem* _navBottomEdge;
    QGraphicsLineItem* _navRightEdge;
    QGraphicsLineItem* _navTopEdge;
    
    QGraphicsProxyWidget* _navigatorProxy;
    
    QUndoStack* _undoStack;
    
    
    QMenu* _menu;
    
    QGraphicsItem *_tL,*_tR,*_bR,*_bL;
    
    bool _refreshOverlays;
    
    mutable QMutex _previewsTurnedOffMutex;
    bool _previewsTurnedOff;
    
    
    NodeClipBoard _nodeClipBoard;
    
    Edge* _highLightedEdge;
    
    ///This is a hint edge we show when _highLightedEdge is not NULL to display a possible connection.
    Edge* _hintInputEdge;
    Edge* _hintOutputEdge;
    
    std::list<NodeBackDrop*> _backdrops;
    boost::shared_ptr<NodeBackDropSerialization> _backdropClipboard;
    
    NodeBackDrop* _selectedBackDrop;
    std::list<boost::shared_ptr<NodeGui> > _nodesToMoveWithBackDrop;
    bool _firstMove;
    
    NodeGraphPrivate(Gui* gui)
    : _gui(gui)
    , _lastScenePosClick()
    , _lastNodeDragStartPoint()
    , _evtState(DEFAULT)
    , _nodeSelected()
    , _nodeSelectedScaleBeforeMagnif(1.)
    , _magnifOn(false)
    , _arrowSelected(NULL)
    , _nodesMutex()
    , _nodes()
    , _nodesTrash()
    , _nodeCreationShortcutEnabled(false)
    , _root(NULL)
    , _nodeRoot(NULL)
    , _propertyBin(NULL)
    , _cacheSizeText(NULL)
    , _refreshCacheTextTimer()
    , _navigator(NULL)
    , _navLeftEdge(NULL)
    , _navBottomEdge(NULL)
    , _navRightEdge(NULL)
    , _navTopEdge(NULL)
    , _navigatorProxy(NULL)
    , _undoStack(NULL)
    , _menu(NULL)
    , _tL(NULL)
    , _tR(NULL)
    , _bR(NULL)
    , _bL(NULL)
    , _refreshOverlays(false)
    , _previewsTurnedOffMutex()
    , _previewsTurnedOff(false)
    , _nodeClipBoard()
    , _highLightedEdge(NULL)
    , _hintInputEdge(NULL)
    , _hintOutputEdge(NULL)
    , _backdrops()
    , _backdropClipboard()
    , _selectedBackDrop(NULL)
    , _nodesToMoveWithBackDrop()
    , _firstMove(true)
    {
        
    }

    void resetAllClipboards();
    
    QRectF calcNodesBoundingRect();
    
};

NodeGraph::NodeGraph(Gui* gui,QGraphicsScene* scene,QWidget *parent)
    : QGraphicsView(scene,parent)
    , _imp(new NodeGraphPrivate(gui))
{
    setAcceptDrops(true);
    
    QObject::connect(_imp->_gui->getApp()->getProject().get(), SIGNAL(nodesCleared()), this, SLOT(onProjectNodesCleared()));
    
    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    scale(qreal(0.8), qreal(0.8));
    
    _imp->_root = new QGraphicsTextItem(0);
    _imp->_nodeRoot = new QGraphicsTextItem(_imp->_root);
    scene->addItem(_imp->_root);
    
    _imp->_navigator = new NodeGraphNavigator();
    _imp->_navigatorProxy = new QGraphicsProxyWidget(0);
    _imp->_navigatorProxy->setWidget(_imp->_navigator);
     scene->addItem(_imp->_navigatorProxy);
    _imp->_navigatorProxy->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_navigatorProxy->hide();
    
    QPen p;
    p.setBrush(QColor(200,200,200));
    p.setWidth(2);
    
    _imp->_navLeftEdge = new QGraphicsLineItem(0);
    _imp->_navLeftEdge->setPen(p);
    scene->addItem(_imp->_navLeftEdge);
    _imp->_navLeftEdge->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_navLeftEdge->hide();
    
    _imp->_navBottomEdge = new QGraphicsLineItem(0);
    _imp->_navBottomEdge->setPen(p);
    scene->addItem(_imp->_navBottomEdge);
    _imp->_navBottomEdge->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_navBottomEdge->hide();
    
    _imp->_navRightEdge = new QGraphicsLineItem(0);
    _imp->_navRightEdge->setPen(p);
    scene->addItem(_imp->_navRightEdge);
    _imp->_navRightEdge->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_navRightEdge->hide();
    
    _imp->_navTopEdge = new QGraphicsLineItem(0);
    _imp->_navTopEdge->setPen(p);
    scene->addItem(_imp->_navTopEdge);
    _imp->_navTopEdge->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_navTopEdge->hide();
    
    _imp->_cacheSizeText = new QGraphicsTextItem(0);
    scene->addItem(_imp->_cacheSizeText);
    _imp->_cacheSizeText->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_cacheSizeText->setDefaultTextColor(QColor(200,200,200));
    
    QObject::connect(&_imp->_refreshCacheTextTimer,SIGNAL(timeout()),this,SLOT(updateCacheSizeText()));
    _imp->_refreshCacheTextTimer.start(NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS);
    
    _imp->_undoStack = new QUndoStack(this);
    _imp->_undoStack->setUndoLimit(appPTR->getCurrentSettings()->getMaximumUndoRedoNodeGraph());
    _imp->_gui->registerNewUndoStack(_imp->_undoStack);
    
    _imp->_hintInputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_imp->_nodeRoot);
    _imp->_hintInputEdge->setDefaultColor(QColor(0,255,0,100));
    _imp->_hintInputEdge->hide();
    
    _imp->_hintOutputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_imp->_nodeRoot);
    _imp->_hintOutputEdge->setDefaultColor(QColor(0,255,0,100));
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
    
    scene->setSceneRect(0,0,10000,10000);
    _imp->_tL->setPos(_imp->_tL->mapFromScene(QPointF(0,10000)));
    _imp->_tR->setPos(_imp->_tR->mapFromScene(QPointF(10000,10000)));
    _imp->_bR->setPos(_imp->_bR->mapFromScene(QPointF(10000,0)));
    _imp->_bL->setPos(_imp->_bL->mapFromScene(QPointF(0,0)));
    centerOn(5000,5000);
    
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    _imp->_menu = new QMenu(this);
    _imp->_menu->setFont(QFont(NATRON_FONT, NATRON_FONT_SIZE_11));
    
    QObject::connect(_imp->_gui->getApp()->getTimeLine().get(),SIGNAL(frameChanged(SequenceTime,int)),
                     this,SLOT(onTimeChanged(SequenceTime,int)));

}

NodeGraph::~NodeGraph() {
    
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

    QObject::disconnect(&_imp->_refreshCacheTextTimer,SIGNAL(timeout()),this,SLOT(updateCacheSizeText()));
    _imp->_nodeCreationShortcutEnabled = false;

    onProjectNodesCleared();

}

void NodeGraph::setPropertyBinPtr(QScrollArea* propertyBin) { _imp->_propertyBin = propertyBin; }

boost::shared_ptr<NodeGui> NodeGraph::getSelectedNode() const { return _imp->_nodeSelected; }

QGraphicsItem* NodeGraph::getRootItem() const { return _imp->_root; }

Gui* NodeGraph::getGui() const { return _imp->_gui; }

void NodeGraph::discardGuiPointer() { _imp->_gui = 0; }

bool NodeGraph::areAllPreviewTurnedOff() const {
    QMutexLocker l(&_imp->_previewsTurnedOffMutex);
    return _imp->_previewsTurnedOff;
}


void NodeGraph::onProjectNodesCleared() {
    
    _imp->_nodeSelected.reset();
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it!=_imp->_nodes.end(); ++it) {
        (*it)->deleteReferences();
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it!=_imp->_nodesTrash.end(); ++it) {
        (*it)->deleteReferences();
    }
    _imp->_nodes.clear();
    _imp->_nodesTrash.clear();
    _imp->_undoStack->clear();
    
    for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it!= _imp->_backdrops.end(); ++it) {
        delete *it;
    }
    _imp->_backdrops.clear();
}

void NodeGraph::resizeEvent(QResizeEvent* event){
    _imp->_refreshOverlays = true;
    QGraphicsView::resizeEvent(event);
}
void NodeGraph::paintEvent(QPaintEvent* event){
    if (_imp->_refreshOverlays) {
        QRectF visible = visibleRect();
        //cout << visible.topLeft().x() << " " << visible.topLeft().y() << " " << visible.width() << " " << visible.height() << endl;
        _imp->_cacheSizeText->setPos(visible.topLeft());
        QSize navSize = _imp->_navigator->sizeHint();
        QPointF navPos = visible.bottomRight() - QPoint(navSize.width(),navSize.height());
        //   cout << navPos.x() << " " << navPos.y() << endl;
        _imp->_navigatorProxy->setPos(navPos);
        _imp->_navLeftEdge->setLine(navPos.x(),
                              navPos.y() + navSize.height(),
                              navPos.x(),
                              navPos.y());
        _imp->_navLeftEdge->setPos(navPos);
        _imp->_navTopEdge->setLine(navPos.x(),
                             navPos.y(),
                             navPos.x() + navSize.width(),
                             navPos.y());
        _imp->_navTopEdge->setPos(navPos);
        _imp->_navRightEdge->setLine(navPos.x() + navSize.width() ,
                               navPos.y(),
                               navPos.x() + navSize.width() ,
                               navPos.y() + navSize.height());
        _imp->_navRightEdge->setPos(navPos);
        _imp->_navBottomEdge->setLine(navPos.x() + navSize.width() ,
                                navPos.y() + navSize.height(),
                                navPos.x(),
                                navPos.y() + navSize.height());
        _imp->_navBottomEdge->setPos(navPos);
        _imp->_refreshOverlays = false;
    }
    QGraphicsView::paintEvent(event);
}

QRectF NodeGraph::visibleRect() {
    return mapToScene(viewport()->rect()).boundingRect();
}

boost::shared_ptr<NodeGui> NodeGraph::createNodeGUI(QVBoxLayout *dockContainer,const boost::shared_ptr<Natron::Node>& node,
                                                    bool requestedByLoad){
  
    boost::shared_ptr<NodeGui> node_ui(new NodeGui(_imp->_nodeRoot));
    node_ui->initialize(this, node_ui, dockContainer, node, requestedByLoad);
    
    ///only move main instances
    if (node->getParentMultiInstanceName().empty()) {
        moveNodesForIdealPosition(node_ui);
    }
    
    {
        QMutexLocker l(&_imp->_nodesMutex);
        _imp->_nodes.push_back(node_ui);
    }
    QUndoStack* nodeStack = node_ui->getUndoStack();
    if (nodeStack) {
        _imp->_gui->registerNewUndoStack(nodeStack);
    }
    
    ///before we add a new node, clear exceeding entries
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(new AddCommand(this,node_ui));
    _imp->_evtState = DEFAULT;
    return node_ui;
    
}

void NodeGraph::moveNodesForIdealPosition(boost::shared_ptr<NodeGui> node) {
    QRectF viewPos = visibleRect();
    
    ///3 possible values:
    /// 0 = default , i.e: we pop the node in the middle of the graph's current view
    /// 1 = pop the node above the selected node and move the inputs of the selected node a little
    /// 2 = pop the node below the selected node and move the outputs of the selected node a little
    int behavior = 0;
    
    if (!_imp->_nodeSelected) {
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
        if (_imp->_nodeSelected->getNode()->isOutputNode()) {
            
            ///case 1-a) just do default we don't know what else to do
            if (node->getNode()->isOutputNode()) {
                behavior = 0;
            } else {
                ///for either cases 1-b) or 1-c) we just connect the created node as input of the selected node.
                behavior = 1;
            }
           
        }
        ///2) and 3) are similar except for case b)
        else {
            
            ///case 2 or 3- a): connect the created node as output of the selected node.
            if (node->getNode()->isOutputNode()) {
                behavior = 2;
            }
            ///case b)
            else if (node->getNode()->maximumInputs() == 0) {
                if (_imp->_nodeSelected->getNode()->maximumInputs() == 0) {
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
    
    ///if behaviour is 1 , just check that we can effectively connect the node to avoid moving them for nothing
    ///otherwise fallback on behaviour 0
    if (behavior == 1) {
        const std::vector<boost::shared_ptr<Natron::Node> >& inputs = _imp->_nodeSelected->getNode()->getInputs_mt_safe();
        bool oneInputEmpty = false;
        for (U32 i = 0; i < inputs.size() ;++i) {
            if (!inputs[i]) {
                oneInputEmpty = true;
                break;
            }
        }
        if (!oneInputEmpty) {
            behavior = 0;
        }
    }
    
    ///default
    QPointF position;
    if (behavior == 0) {
        position.setX((viewPos.bottomRight().x() + viewPos.topLeft().x()) / 2.);
        position.setY((viewPos.topLeft().y() + viewPos.bottomRight().y()) / 2.);
    }
    ///pop it above the selected node
    else if(behavior == 1) {
        QSize selectedNodeSize = _imp->_nodeSelected->getSize();
        QSize createdNodeSize = node->getSize();
        QPointF selectedNodeMiddlePos = _imp->_nodeSelected->scenePos() +
        QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);
        

        position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
        position.setY(selectedNodeMiddlePos.y() - selectedNodeSize.height() / 2 - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES
                      - createdNodeSize.height());
        
        QRectF createdNodeRect(position.x(),position.y(),createdNodeSize.width(),createdNodeSize.height());

        ///now that we have the position of the node, move the inputs of the selected node to make some space for this node
        const std::map<int,Edge*>& selectedNodeInputs = _imp->_nodeSelected->getInputsArrows();
        for (std::map<int,Edge*>::const_iterator it = selectedNodeInputs.begin(); it!=selectedNodeInputs.end();++it) {
            if (it->second->hasSource()) {
                it->second->getSource()->moveAbovePositionRecursively(createdNodeRect);
            }
        }
        
    }
    ///pop it below the selected node
    else {
        QSize selectedNodeSize = _imp->_nodeSelected->getSize();
        QSize createdNodeSize = node->getSize();
        QPointF selectedNodeMiddlePos = _imp->_nodeSelected->scenePos() +
        QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);
        
        ///actually move the created node where the selected node is
        position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
        position.setY(selectedNodeMiddlePos.y() + (selectedNodeSize.height() / 2) + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
        
        QRectF createdNodeRect(position.x(),position.y(),createdNodeSize.width(),createdNodeSize.height());
        
        ///and move the selected node below recusively
        const std::list<boost::shared_ptr<Natron::Node> >& outputs = _imp->_nodeSelected->getNode()->getOutputs();
        for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!= outputs.end(); ++it) {
            assert(*it);
            boost::shared_ptr<NodeGui> output = _imp->_gui->getApp()->getNodeGui(*it);
            assert(output);
            output->moveBelowPositionRecursively(createdNodeRect);
            
        }
    }
    position = node->mapFromScene(position);
    position = node->mapToParent(position);
    node->setPos(position.x(), position.y());
    
}



void NodeGraph::mousePressEvent(QMouseEvent *event) {
    
    assert(event);
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier)) {
        _imp->_evtState = MOVING_AREA;
        return;
    }
    
    _imp->_lastScenePosClick = mapToScene(event->pos());
    
    boost::shared_ptr<NodeGui> selected ;
    Edge* selectedEdge = 0;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::reverse_iterator it = _imp->_nodes.rbegin();it!=_imp->_nodes.rend();++it) {
            boost::shared_ptr<NodeGui>& n = *it;
            QPointF evpt = n->mapFromScene(_imp->_lastScenePosClick);
            if (n->isActive() && n->contains(evpt)) {
                selected = n;
                break;
            } else {
                Edge* edge = n->hasEdgeNearbyPoint(_imp->_lastScenePosClick);
                if (edge) {
                    selectedEdge = edge;
                    break;
                }
            }
        }
    }
    
    if (selected) {
        selectNode(selected);
        if (event->button() == Qt::LeftButton) {
            _imp->_evtState = NODE_DRAGGING;
            _imp->_lastNodeDragStartPoint = selected->pos();
        }
        else if (event->button() == Qt::RightButton) {
            selected->showMenu(mapToGlobal(event->pos()));
        }
    } else if (selectedEdge) {
        _imp->_arrowSelected = selectedEdge;
        _imp->_evtState = ARROW_DRAGGING;
    }
    
    NodeBackDrop* oldSelection = _imp->_selectedBackDrop;
    _imp->_selectedBackDrop = 0;
    if (_imp->_evtState == DEFAULT) {
        ///check if nearby a backdrop
        for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it!=_imp->_backdrops.end(); ++it) {
            if ((*it)->isNearbyHeader(_imp->_lastScenePosClick)) {
                _imp->_selectedBackDrop = *it;
                if ((oldSelection != _imp->_selectedBackDrop && oldSelection != NULL) || !oldSelection) {
                    _imp->_selectedBackDrop->setSelected(true);
                }
                if (event->button() == Qt::LeftButton) {
                    _imp->_evtState = BACKDROP_DRAGGING;
                }
                break;
            } else if ((*it)->isNearbyResizeHandle(_imp->_lastScenePosClick)) {
                _imp->_selectedBackDrop = *it;
                if ((oldSelection != _imp->_selectedBackDrop && oldSelection != NULL) || !oldSelection) {
                    _imp->_selectedBackDrop->setSelected(true);
                }
                if (event->button() == Qt::LeftButton) {
                    _imp->_evtState = BACKDROP_RESIZING;
                }
                break;
            }
        }
    }
    
    if (_imp->_selectedBackDrop && event->button() == Qt::RightButton) {
        _imp->_selectedBackDrop->showMenu(mapToGlobal(event->pos()));
    }
    
    if (_imp->_selectedBackDrop && oldSelection && oldSelection != _imp->_selectedBackDrop) {
        oldSelection->setSelected(false);
    }
    
    if (!_imp->_selectedBackDrop && event->button() == Qt::LeftButton) {
        if (oldSelection) {
            oldSelection->setSelected(false);
        }
    }
    
    if (!selected && !selectedEdge && event->button() == Qt::LeftButton) {
        deselect();
    }
    
    if (!_imp->_selectedBackDrop && !selected && !selectedEdge) {
        if (event->button() == Qt::RightButton) {
            showMenu(mapToGlobal(event->pos()));
        } else if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier)) {
            _imp->_evtState = MOVING_AREA;
            QGraphicsView::mousePressEvent(event);
        }
    }
    
    
}

void NodeGraph::deselect() {
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for(std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
            (*it)->setSelected(false);
        }
        if (_imp->_nodeSelected && _imp->_magnifOn) {
            _imp->_magnifOn = false;
            _imp->_nodeSelected->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
            
        }
        _imp->_nodeSelected.reset();
    }
}

void NodeGraph::mouseReleaseEvent(QMouseEvent *event){
    EVENT_STATE state = _imp->_evtState;
    _imp->_firstMove = true;
    _imp->_nodesToMoveWithBackDrop.clear();
    _imp->_evtState = DEFAULT;

    if (state == ARROW_DRAGGING) {
        bool foundSrc = false;
        assert(_imp->_arrowSelected);
        boost::shared_ptr<NodeGui> nodeHoldingEdge = _imp->_arrowSelected->isOutputEdge() ?
        _imp->_arrowSelected->getSource() : _imp->_arrowSelected->getDest();
        assert(nodeHoldingEdge);
        
        std::list<boost::shared_ptr<NodeGui> > nodes = getAllActiveNodes_mt_safe();
        QPointF ep = mapToScene(event->pos());

        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
            boost::shared_ptr<NodeGui>& n = *it;
            
            if(n->isActive() && n->isNearby(ep) &&
                    (n->getNode()->getName() != nodeHoldingEdge->getNode()->getName())){
               
                
                if (!_imp->_arrowSelected->isOutputEdge()) {
                    ///can't connect to a viewer
                    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(n->getNode()->getLiveInstance());
                    if (isViewer) {
                        break;
                    }
                   
                    _imp->_arrowSelected->stackBefore(n.get());
                    _imp->_undoStack->setActive();
                    _imp->_undoStack->push(new ConnectCommand(this,_imp->_arrowSelected,_imp->_arrowSelected->getSource(),n));
                } else {
                    
                    ///Find the input edge of the node we just released the mouse over,
                    ///and use that edge to connect to the source of the selected edge.
                    int preferredInput = n->getNode()->getPreferredInputForConnection();
                    if (preferredInput != -1) {
                        const std::map<int,Edge*>& inputEdges = n->getInputsArrows();
                        std::map<int,Edge*>::const_iterator foundInput = inputEdges.find(preferredInput);
                        assert(foundInput != inputEdges.end());
                        _imp->_undoStack->setActive();
                        _imp->_undoStack->push(new ConnectCommand(this,foundInput->second,
                                                            foundInput->second->getSource(),_imp->_arrowSelected->getSource()));
                    
                    }
                }
                foundSrc = true;
                
                break;
            }
        }
        
        ///if we disconnected the input edge, use the undo/redo stack.
        ///Output edges can never be really connected, they're just there
        ///So the user understands some nodes can have output
        if (!foundSrc && !_imp->_arrowSelected->isOutputEdge() && _imp->_arrowSelected->getSource()) {
            _imp->_undoStack->setActive();
            _imp->_undoStack->push(new ConnectCommand(this,_imp->_arrowSelected,_imp->_arrowSelected->getSource(),
                                                      boost::shared_ptr<NodeGui>()));
            scene()->update();
        }
        nodeHoldingEdge->refreshEdges();
        scene()->update();
    } else if (state == NODE_DRAGGING) {
        if (_imp->_nodeSelected) {
            _imp->_undoStack->setActive();
            _imp->_undoStack->push(new MoveCommand(_imp->_nodeSelected,_imp->_lastNodeDragStartPoint));
            
            ///now if there was a hint displayed, use it to actually make connections.
            if (_imp->_highLightedEdge) {
                if (_imp->_highLightedEdge->isOutputEdge()) {
                    int prefInput = _imp->_nodeSelected->getNode()->getPreferredInputForConnection();
                    if (prefInput != -1) {
                        Edge* inputEdge = _imp->_nodeSelected->getInputArrow(prefInput);
                        assert(inputEdge);
                        _imp->_undoStack->push(new ConnectCommand(this,inputEdge,inputEdge->getSource(),
                                                                  _imp->_highLightedEdge->getSource()));
                    }
                } else {
                    
                    boost::shared_ptr<NodeGui> src = _imp->_highLightedEdge->getSource();
                    _imp->_undoStack->push(new ConnectCommand(this,_imp->_highLightedEdge,_imp->_highLightedEdge->getSource(),
                                                        _imp->_nodeSelected));
                    
                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> >& inpNodes = _imp->_nodeSelected->getNode()->getInputs_mt_safe();
                    if (src) {
                        for (U32 i = 0; i < inpNodes.size();++i) {
                            if (inpNodes[i] == src->getNode()) {
                                alreadyConnected = true;
                                break;
                            }
                        }
                    }
                    
                    if (src && !alreadyConnected) {
                        ///push a second command... this is a bit dirty but I don't have time to add a whole new command just for this
                        int prefInput = _imp->_nodeSelected->getNode()->getPreferredInputForConnection();
                        if (prefInput != -1) {
                            Edge* inputEdge = _imp->_nodeSelected->getInputArrow(prefInput);
                            assert(inputEdge);
                            _imp->_undoStack->push(new ConnectCommand(this,inputEdge,inputEdge->getSource(),src));
                        }
                    }
                }
                _imp->_highLightedEdge->setUseHighlight(false);
                _imp->_highLightedEdge = 0;
                _imp->_hintInputEdge->hide();
                _imp->_hintOutputEdge->hide();
            }
        }
    }
    scene()->update();
    
    
    setCursor(QCursor(Qt::ArrowCursor));
}
void NodeGraph::mouseMoveEvent(QMouseEvent *event) {
    
    QPointF newPos = mapToScene(event->pos());
    double dx = _imp->_root->mapFromScene(newPos).x() - _imp->_root->mapFromScene(_imp->_lastScenePosClick).x();
    double dy = _imp->_root->mapFromScene(newPos).y() - _imp->_root->mapFromScene(_imp->_lastScenePosClick).y();
    {
        ///set cursor
        boost::shared_ptr<NodeGui> selected;
        Edge* selectedEdge = 0;
        
        {
            QMutexLocker l(&_imp->_nodesMutex);
            for(std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it){
                boost::shared_ptr<NodeGui>& n = *it;
                QPointF evpt = n->mapFromScene(newPos);
                if(n->isActive() && n->contains(evpt)){
                    selected = n;
                    break;
                }else{
                    Edge* edge = n->hasEdgeNearbyPoint(newPos);
                    if(edge){
                        selectedEdge = edge;
                        break;
                    }
                }
            }
        }
        if (selected) {
            setCursor(QCursor(Qt::OpenHandCursor));
        } else if(selectedEdge) {
            
        } else if(!selectedEdge && !selected) {
            setCursor(QCursor(Qt::ArrowCursor));
        }
    }
    
    ///Apply actions
    switch (_imp->_evtState) {
        case ARROW_DRAGGING: {
            QPointF np = _imp->_arrowSelected->mapFromScene(newPos);
            if (_imp->_arrowSelected->isOutputEdge()) {
                _imp->_arrowSelected->dragDest(np);
            } else {
                _imp->_arrowSelected->dragSource(np);
            }
        } break;
        case NODE_DRAGGING: {
            
            if (!_imp->_nodeSelected) {
                ///it should never happen for real
                return;
            }
            
            QSize size = _imp->_nodeSelected->getSize();
            newPos = _imp->_nodeSelected->mapToParent(_imp->_nodeSelected->mapFromScene(newPos));
            _imp->_nodeSelected->refreshPosition(newPos.x() - size.width() / 2,newPos.y() - size.height() / 2);
            
            
            ///try to find a nearby edge
            boost::shared_ptr<Natron::Node> internalNode = _imp->_nodeSelected->getNode();
            bool doHints = appPTR->getCurrentSettings()->isConnectionHintEnabled();
            
            ///for readers already connected don't show hint
            if (internalNode->maximumInputs() == 0 && internalNode->hasOutputConnected()) {
                doHints = false;
            } else if (internalNode->maximumInputs() > 0 && internalNode->hasInputConnected() && internalNode->hasOutputConnected()) {
                doHints = false;
            }
            
            if (doHints) {
                QRectF rect = _imp->_nodeSelected->mapToParent(_imp->_nodeSelected->boundingRect()).boundingRect();
                double tolerance = 20;
                rect.adjust(-tolerance, -tolerance, tolerance, tolerance);
                
                Edge* edge = 0;
                {
                    QMutexLocker l(&_imp->_nodesMutex);
                    for(std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it){
                        boost::shared_ptr<NodeGui>& n = *it;
                        if (n != _imp->_nodeSelected) {
                            edge = n->hasEdgeNearbyRect(rect);
                            
                            ///if the edge input is the selected node don't continue
                            if (edge && edge->getSource() == _imp->_nodeSelected) {
                                edge = 0;
                            }
                            
                            if (edge && edge->isOutputEdge()) {
                                ///if the edge is an output edge but the node doesn't have any inputs don't continue
                                if (_imp->_nodeSelected->getInputsArrows().empty()) {
                                    edge = 0;
                                    
                                }
                                ///if the source of that edge is already connected also skip
                                const std::vector<boost::shared_ptr<Natron::Node> >& inpNodes =
                                _imp->_nodeSelected->getNode()->getInputs_mt_safe();
                                for (U32 i = 0; i < inpNodes.size();++i) {
                                    if (edge && inpNodes[i] == edge->getSource()->getNode()) {
                                        edge = 0;
                                        break;
                                    }
                                }
                                
                            }
                            
                            if (edge && !edge->isOutputEdge()) {
                                ///if the edge is an input edge but the selected node is a viewer don't continue (viewer can't have output)
                                if (_imp->_nodeSelected->getNode()->pluginID() == "Viewer") {
                                    edge = 0;
                                }
                                
                                ///if the selected node doesn't have any input but the edge has an input don't continue
                                if (edge && _imp->_nodeSelected->getInputsArrows().empty() && edge->getSource()) {
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
                if (_imp->_highLightedEdge && _imp->_highLightedEdge != edge) {
                    _imp->_highLightedEdge->setUseHighlight(false);
                    _imp->_hintInputEdge->hide();
                    _imp->_hintOutputEdge->hide();
                }
                
                _imp->_highLightedEdge = edge;
                
                if (edge && edge->getSource() && edge->getDest()) {
                    ///setup the hints edge
                    
                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> >& inpNodes = _imp->_nodeSelected->getNode()->getInputs_mt_safe();
                    for (U32 i = 0; i < inpNodes.size();++i) {
                        if (inpNodes[i] == edge->getSource()->getNode()) {
                            alreadyConnected = true;
                            break;
                        }
                    }
                    
                    if (!_imp->_hintInputEdge->isVisible()) {
                        if (!alreadyConnected) {
                            int prefInput = _imp->_nodeSelected->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), _imp->_nodeSelected);
                            _imp->_hintInputEdge->setVisible(true);
                            
                        }
                        _imp->_hintOutputEdge->setInputNumber(edge->getInputNumber());
                        _imp->_hintOutputEdge->setSourceAndDestination(_imp->_nodeSelected, edge->getDest());
                        _imp->_hintOutputEdge->setVisible(true);
                    } else {
                        if (!alreadyConnected) {
                            _imp->_hintInputEdge->initLine();
                        }
                        _imp->_hintOutputEdge->initLine();
                    }
                } else if (edge) {
                    ///setup only 1 of the hints edge
                    
                    if (_imp->_highLightedEdge && !_imp->_hintInputEdge->isVisible()) {
                        if (edge->isOutputEdge()) {
                            int prefInput = _imp->_nodeSelected->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), _imp->_nodeSelected);
                            
                        } else {
                            _imp->_hintInputEdge->setInputNumber(edge->getInputNumber());
                            _imp->_hintInputEdge->setSourceAndDestination(_imp->_nodeSelected,edge->getDest());
                            
                        }
                        _imp->_hintInputEdge->setVisible(true);
                    } else if (_imp->_highLightedEdge && _imp->_hintInputEdge->isVisible()) {
                        _imp->_hintInputEdge->initLine();
                    }
                }
                
            }
            
            setCursor(QCursor(Qt::ClosedHandCursor));

        } break;
        case MOVING_AREA: {
            _imp->_root->moveBy(dx, dy);
            setCursor(QCursor(Qt::SizeAllCursor));
        } break;
        case BACKDROP_DRAGGING: {
            assert(_imp->_selectedBackDrop);
            _imp->_undoStack->setActive();
            if (_imp->_firstMove) {
                _imp->_nodesToMoveWithBackDrop = getNodesWithinBackDrop(_imp->_selectedBackDrop);
                _imp->_firstMove = false;
            }
            bool controlHeld = event->modifiers().testFlag(Qt::ControlModifier);
            _imp->_undoStack->push(new MoveBackDropCommand(_imp->_selectedBackDrop,dx,dy,
                                                     controlHeld ?
                                                           std::list<boost::shared_ptr<NodeGui> >() : _imp->_nodesToMoveWithBackDrop));
        } break;
        case BACKDROP_RESIZING: {
            assert(_imp->_selectedBackDrop);
            _imp->_undoStack->setActive();
            QPointF p = _imp->_selectedBackDrop->scenePos();
            int w = newPos.x() - p.x();
            int h = newPos.y() - p.y();
            _imp->_undoStack->push(new ResizeBackDropCommand(_imp->_selectedBackDrop,w,h));
        } break;
        default:
            break;
    }

    _imp->_lastScenePosClick = newPos;
    update();
    
    /*Now update navigator*/
    //updateNavigator();
}


void NodeGraph::mouseDoubleClickEvent(QMouseEvent *) {
    
    std::list<boost::shared_ptr<NodeGui> > nodes = getAllActiveNodes_mt_safe();
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodes.begin();it!=nodes.end();++it) {
        QPointF evpt = (*it)->mapFromScene(_imp->_lastScenePosClick);
        if((*it)->isActive() && (*it)->contains(evpt) && (*it)->getSettingPanel()){
            if(!(*it)->isSettingsPanelVisible()){
                (*it)->setVisibleSettingsPanel(true);
            }
            if (!(*it)->wasBeginEditCalled()) {
                (*it)->beginEditKnobs();
            }
            _imp->_gui->putSettingsPanelFirst((*it)->getSettingPanel());
            getGui()->getApp()->redrawAllViewers();
            return;
        }
    }

    for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it!=_imp->_backdrops.end(); ++it) {
        if ((*it)->isNearbyHeader(_imp->_lastScenePosClick)) {
            if ((*it)->isSettingsPanelClosed()) {
                (*it)->setSettingsPanelClosed(false);
            }
            _imp->_gui->putSettingsPanelFirst((*it)->getSettingsPanel());
            return;
        }
    }
}

namespace {
class NodeCreationDialog : public QDialog
{
    
public:
    
    explicit NodeCreationDialog(NodeGraph* graph);
    
    virtual ~NodeCreationDialog() OVERRIDE {}
    
    QString getNodeName() const;
    
private:
    
    virtual void keyPressEvent(QKeyEvent *e) OVERRIDE FINAL;
    
    
    /*NodeGraph* graph*/;
    QVBoxLayout* layout;
    QLabel* textLabel;
    QComboBox* textEdit;
    
};

NodeCreationDialog::NodeCreationDialog(NodeGraph* /*graph_*/)
: QDialog(/*graph_*/) //< uncomment to inherit from the stylesheet of the application.
/*, graph(graph_)*/
, layout(NULL)
, textLabel(NULL)
, textEdit(NULL)
{
    setWindowTitle(tr("Node creation tool"));
    setWindowFlags(Qt::Popup);
    setObjectName(QString("SmartDialog"));
    setStyleSheet("SmartInputDialog#SmartDialog"
                  "{"
                  "border-style:outset;"
                  "border-width: 2px;"
                  "border-color: black;"
                  "background-color:silver;"
                  "}"
                  );
    layout = new QVBoxLayout(this);
    textLabel = new QLabel(tr("Input a node name:"),this);
    textEdit = new QComboBox(this);
    textEdit->setEditable(true);
    
    textEdit->addItems(appPTR->getNodeNameList());
    layout->addWidget(textLabel);
    layout->addWidget(textEdit);
    textEdit->lineEdit()->selectAll();
    textEdit->setFocus();
}
}

QString NodeCreationDialog::getNodeName() const
{
    return textEdit->lineEdit()->text();
}

void NodeCreationDialog::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    }
}


bool NodeGraph::event(QEvent* event){
    if ( event->type() == QEvent::KeyPress ) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (ke &&  ke->key() == Qt::Key_Tab && _imp->_nodeCreationShortcutEnabled ) {
            QPoint global = QCursor::pos();
            NodeCreationDialog nodeCreation(this);
            QSize sizeH = nodeCreation.sizeHint();
            global.rx() -= sizeH.width() / 2;
            global.ry() -= sizeH.height() / 2;
            nodeCreation.move(global.x(), global.y());
            if (nodeCreation.exec()) {
                QString res = nodeCreation.getNodeName();
                if (appPTR->getNodeNameList().contains(res)) {
                    getGui()->getApp()->createNode(res);
                }
            }
            setFocus(Qt::ActiveWindowFocusReason);
            ke->accept();
            return true;
        }
    }
    return QGraphicsView::event(event);
}

void NodeGraph::keyPressEvent(QKeyEvent *e){
    
    if (e->key() == Qt::Key_Space) {
        QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);
        QCoreApplication::postEvent(parentWidget(),ev);
    } else if (e->key() == Qt::Key_R && !e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        _imp->_gui->createReader();
    } else if (e->key() == Qt::Key_W && !e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        _imp->_gui->createWriter();
    } else if ((e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete) && !e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        /*delete current node.*/
        if (_imp->_selectedBackDrop) {
            _imp->_undoStack->setActive();
            _imp->_undoStack->push(new RemoveBackDropCommand(this,_imp->_selectedBackDrop,getNodesWithinBackDrop(_imp->_selectedBackDrop)));
            _imp->_selectedBackDrop = 0;
        } else {
            deleteSelectedNode();
        }
    } else if (e->key() == Qt::Key_P && !e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        forceRefreshAllPreviews();
    } else if (e->key() == Qt::Key_C && e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        copySelectedNode();
    } else if (e->key() == Qt::Key_V && e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        pasteNodeClipBoard();
    } else if (e->key() == Qt::Key_X && e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        cutSelectedNode();
    } else if (e->key() == Qt::Key_C && e->modifiers().testFlag(Qt::AltModifier)
               && !e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)) {
        duplicateSelectedNode();
    } else if (e->key() == Qt::Key_K && e->modifiers().testFlag(Qt::AltModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::ControlModifier)) {
        cloneSelectedNode();
    } else if (e->key() == Qt::Key_K && e->modifiers().testFlag(Qt::AltModifier)
               && e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::ControlModifier)) {
        decloneSelectedNode();
    } else if (e->key() == Qt::Key_F && !e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        centerOnAllNodes();
    } else if (e->key() == Qt::Key_H && !e->modifiers().testFlag(Qt::ControlModifier)
               && !e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)) {
        toggleConnectionHints();
    } else if (e->key() == Qt::Key_X && e->modifiers().testFlag(Qt::ShiftModifier)
               && !e->modifiers().testFlag(Qt::AltModifier)
               && !e->modifiers().testFlag(Qt::ControlModifier)) {
        
        ///No need to make an undo command for this, the user just have to do it a second time to reverse the effect
        if (_imp->_nodeSelected) {
            _imp->_nodeSelected->onSwitchInputActionTriggered();
        }
    }  else if (e->key() == Qt::Key_J && !e->modifiers().testFlag(Qt::ShiftModifier)
                && !e->modifiers().testFlag(Qt::ControlModifier)
                && !e->modifiers().testFlag(Qt::AltModifier)) {
        ViewerTab* lastSelectedViewer = _imp->_gui->getLastSelectedViewer();
        if (lastSelectedViewer) {
            lastSelectedViewer->startBackward(!lastSelectedViewer->isPlayingBackward());
        }
    }
    else if (e->key() == Qt::Key_K && !e->modifiers().testFlag(Qt::ShiftModifier)
             && !e->modifiers().testFlag(Qt::ControlModifier)
             && !e->modifiers().testFlag(Qt::AltModifier)) {
        ViewerTab* lastSelectedViewer = _imp->_gui->getLastSelectedViewer();
        if (lastSelectedViewer) {
           lastSelectedViewer-> abortRendering();
        }
    }
    else if (e->key() == Qt::Key_L && !e->modifiers().testFlag(Qt::ShiftModifier)
             && !e->modifiers().testFlag(Qt::ControlModifier)
             && !e->modifiers().testFlag(Qt::AltModifier)) {
        ViewerTab* lastSelectedViewer = _imp->_gui->getLastSelectedViewer();
        if (lastSelectedViewer) {
            lastSelectedViewer->startPause(!lastSelectedViewer->isPlayingForward());
        }
        
    }

    
}
void NodeGraph::connectCurrentViewerToSelection(int inputNB){
    
    if (!_imp->_gui->getLastSelectedViewer()) {
        _imp->_gui->getApp()->createNode(CreateNodeArgs("Viewer"));
    }
    
    ///get a pointer to the last user selected viewer
    boost::shared_ptr<InspectorNode> v = boost::dynamic_pointer_cast<InspectorNode>(_imp->_gui->getLastSelectedViewer()->
                                                                                    getInternalNode()->getNode());
    
    ///if the node is no longer active (i.e: it was deleted by the user), don't do anything.
    if (!v->isActivated()) {
        return;
    }
    
    ///get a ptr to the NodeGui
    boost::shared_ptr<NodeGui> gui = _imp->_gui->getApp()->getNodeGui(v);
    assert(gui);
    
    ///if there's no selected node or the viewer is selected, then try refreshing that input nb if it is connected.
    if (!_imp->_nodeSelected || _imp->_nodeSelected == gui) {
        v->setActiveInputAndRefresh(inputNB);
        gui->refreshEdges();
        return;
    }
    

    ///if the selected node is a viewer, return, we can't connect a viewer to another viewer.
    ViewerInstance* isSelectedAViewer = dynamic_cast<ViewerInstance*>(_imp->_nodeSelected->getNode()->getLiveInstance());
    if (isSelectedAViewer) {
        return;
    }
    
    ///if the node doesn't have the input 'inputNb' created yet, populate enough input
    ///so it can be created.
    NodeGui::InputEdgesMap::const_iterator it = gui->getInputsArrows().find(inputNB);
    while (it == gui->getInputsArrows().end()) {
        v->addEmptyInput();
        it = gui->getInputsArrows().find(inputNB);
    }
    
    ///set the undostack the active one before connecting
    _imp->_undoStack->setActive();
    
    ///and push a connect command to the selected node.
    _imp->_undoStack->push(new ConnectCommand(this,it->second,it->second->getSource(),_imp->_nodeSelected));
    
    ///Set the viewer as the selected node
    selectNode(gui);
    
}

void NodeGraph::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);
    _imp->_nodeCreationShortcutEnabled = true;
    setFocus();
}
void NodeGraph::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    _imp->_nodeCreationShortcutEnabled = false;
    setFocus();
}


void NodeGraph::wheelEvent(QWheelEvent *event){
    if (event->orientation() != Qt::Vertical) {
        return;
    }

    double scaleFactor = pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if(factor < 0.07 || factor > 10)
        return;
    
    if (event->modifiers().testFlag(Qt::ControlModifier) && _imp->_nodeSelected) {
        if (!_imp->_magnifOn) {
            _imp->_magnifOn = true;
            _imp->_nodeSelectedScaleBeforeMagnif = _imp->_nodeSelected->scale();
        }
        _imp->_nodeSelected->setScale_natron(_imp->_nodeSelected->scale() * scaleFactor);
    } else {
        scale(scaleFactor,scaleFactor);
        _imp->_refreshOverlays = true;
    }
    QPointF newPos = mapToScene(event->pos());
    _imp->_lastScenePosClick = newPos;
}

void NodeGraph::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_nodeSelected->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}

void NodeGraph::deleteSelectedNode(){
    if (_imp->_nodeSelected) {
        _imp->_undoStack->setActive();
        _imp->_nodeSelected->setSelected(false);
        _imp->_undoStack->push(new RemoveCommand(this,_imp->_nodeSelected));
        _imp->_nodeSelected.reset();
    }
}

void NodeGraph::deleteSelectedBackdrop()
{
    if (_imp->_selectedBackDrop) {
        _imp->_undoStack->setActive();
        _imp->_undoStack->push(new RemoveBackDropCommand(this,_imp->_selectedBackDrop,getNodesWithinBackDrop(_imp->_selectedBackDrop)));
    }
}

void NodeGraph::deleteNode(const boost::shared_ptr<NodeGui>& n) {
    assert(n);
    _imp->_undoStack->setActive();
    n->setSelected(false);
    _imp->_undoStack->push(new RemoveCommand(this,n));
}


void NodeGraph::selectNode(const boost::shared_ptr<NodeGui>& n) {
    if (_imp->_nodeSelected && _imp->_nodeSelected != n && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_nodeSelected->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
        
    }
    
    assert(n);
    _imp->_nodeSelected = n;
    
    /*now remove previously selected node*/
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for(std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
            (*it)->setSelected(false);
        }
    }
    
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(n->getNode()->getLiveInstance());
    if (isViewer) {
        OpenGLViewerI* viewer = isViewer->getUiContext();
        const std::list<ViewerTab*>& viewerTabs = _imp->_gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin();it!=viewerTabs.end();++it) {
            if ((*it)->getViewer() == viewer) {
                _imp->_gui->setLastSelectedViewer((*it));
            }
        }
    }
    
    
    n->setSelected(true);
}


void NodeGraph::updateNavigator(){
    if (!areAllNodesVisible()) {
        _imp->_navigator->setImage(getFullSceneScreenShot());
        _imp->_navigator->show();
        _imp->_navLeftEdge->show();
        _imp->_navBottomEdge->show();
        _imp->_navRightEdge->show();
        _imp->_navTopEdge->show();

    } else {
        _imp->_navigator->hide();
        _imp->_navLeftEdge->hide();
        _imp->_navTopEdge->hide();
        _imp->_navRightEdge->hide();
        _imp->_navBottomEdge->hide();
    }
}
bool NodeGraph::areAllNodesVisible(){
    QRectF rect = visibleRect();
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
        if(!rect.contains((*it)->boundingRectWithEdges()))
            return false;
    }
    return true;
}

QImage NodeGraph::getFullSceneScreenShot(){
    const QTransform& currentTransform = transform();
    setTransform(currentTransform.inverted());
    QRectF sceneR = _imp->calcNodesBoundingRect();
    QRectF viewRect = visibleRect();
    sceneR = sceneR.united(viewRect);
    QImage img((int)sceneR.width(), (int)sceneR.height(), QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(71,71,71,255));
    viewRect.setX(viewRect.x() - sceneR.x());
    viewRect.setY(viewRect.y() - sceneR.y());
    QPainter painter(&img);
    painter.save();
    QPen p;
    p.setColor(Qt::yellow);
    p.setWidth(10);
    painter.setPen(p);
    painter.drawRect(viewRect);
    painter.restore();
    scene()->removeItem(_imp->_navLeftEdge);
    scene()->removeItem(_imp->_navBottomEdge);
    scene()->removeItem(_imp->_navTopEdge);
    scene()->removeItem(_imp->_navRightEdge);
    scene()->removeItem(_imp->_cacheSizeText);
    scene()->removeItem(_imp->_navigatorProxy);
    scene()->render(&painter,QRectF(),sceneR);
    scene()->addItem(_imp->_navigatorProxy);
    scene()->addItem(_imp->_cacheSizeText);
    scene()->addItem(_imp->_navLeftEdge);
    scene()->addItem(_imp->_navBottomEdge);
    scene()->addItem(_imp->_navTopEdge);
    scene()->addItem(_imp->_navRightEdge);
    p.setColor(QColor(200,200,200,255));
    painter.setPen(p);
    painter.fillRect(viewRect, QColor(200,200,200,100));
    setTransform(currentTransform);
    return img;
}

NodeGraphNavigator::NodeGraphNavigator(QWidget* parent )
: QLabel(parent)
, _w(120)
, _h(70)
{
    
}

void NodeGraphNavigator::setImage(const QImage& img)
{
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(_w, _h);
    setPixmap(pix);
}

const std::list<boost::shared_ptr<NodeGui> >& NodeGraph::getAllActiveNodes() const {
    return _imp->_nodes;
}

std::list<boost::shared_ptr<NodeGui> > NodeGraph::getAllActiveNodes_mt_safe() const {
    QMutexLocker l(&_imp->_nodesMutex);
    return _imp->_nodes;
}

void NodeGraph::moveToTrash(NodeGui* node) {
    assert(node);
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
        if ((*it).get() == node) {
            _imp->_nodesTrash.push_back(*it);
            _imp->_nodes.erase(it);
            break;
        }
    }
}

void NodeGraph::restoreFromTrash(NodeGui* node) {
    assert(node);
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin();it!=_imp->_nodesTrash.end();++it) {
        if ((*it).get() == node) {
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
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
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
    if (bytes >= tb)
        return QFileSystemModel::tr("%1 TB").arg(QLocale().toString(qreal(bytes) / tb, 'f', 3));
    if (bytes >= gb)
        return QFileSystemModel::tr("%1 GB").arg(QLocale().toString(qreal(bytes) / gb, 'f', 2));
    if (bytes >= mb)
        return QFileSystemModel::tr("%1 MB").arg(QLocale().toString(qreal(bytes) / mb, 'f', 1));
    if (bytes >= kb)
        return QFileSystemModel::tr("%1 KB").arg(QLocale().toString(bytes / kb));
    return QFileSystemModel::tr("%1 byte(s)").arg(QLocale().toString(bytes));
}


void
NodeGraph::updateCacheSizeText()
{
    _imp->_cacheSizeText->setPlainText(tr("Memory cache size: %1")
                                 .arg(QDirModelPrivate_size(appPTR->getCachesTotalMemorySize())));
}

QRectF
NodeGraphPrivate::calcNodesBoundingRect()
{
    QRectF ret;
    QMutexLocker l(&_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
        ret = ret.united((*it)->boundingRectWithEdges());
    }
    return ret;
}

void
NodeGraph::toggleCacheInfos()
{
    if (_imp->_cacheSizeText->isVisible()) {
        _imp->_cacheSizeText->hide();
    } else {
        _imp->_cacheSizeText->show();
    }
}

void
NodeGraph::populateMenu()
{
    _imp->_menu->clear();
    
    
    QMenu* editMenu = new QMenu(tr("Edit"),_imp->_menu);
    editMenu->setFont(QFont(NATRON_FONT, NATRON_FONT_SIZE_11));
    _imp->_menu->addAction(editMenu->menuAction());
    
    QAction* copyAction = new QAction(tr("Copy"),editMenu);
    copyAction->setShortcut(QKeySequence::Copy);
    QObject::connect(copyAction,SIGNAL(triggered()),this,SLOT(copySelectedNode()));
    editMenu->addAction(copyAction);
    
    QAction* cutAction = new QAction(tr("Cut"),editMenu);
    cutAction->setShortcut(QKeySequence::Cut);
    QObject::connect(cutAction,SIGNAL(triggered()),this,SLOT(cutSelectedNode()));
    editMenu->addAction(cutAction);
    
    
    QAction* pasteAction = new QAction(tr("Paste"),editMenu);
    pasteAction->setShortcut(QKeySequence::Paste);
    pasteAction->setEnabled(!_imp->_nodeClipBoard.isEmpty());
    QObject::connect(pasteAction,SIGNAL(triggered()),this,SLOT(pasteNodeClipBoard()));
    editMenu->addAction(pasteAction);
    
    QAction* duplicateAction = new QAction(tr("Duplicate"),editMenu);
    duplicateAction->setShortcut(QKeySequence(Qt::AltModifier + Qt::Key_C));
    QObject::connect(duplicateAction,SIGNAL(triggered()),this,SLOT(duplicateSelectedNode()));
    editMenu->addAction(duplicateAction);
    
    QAction* cloneAction = new QAction(tr("Clone"),editMenu);
    cloneAction->setShortcut(QKeySequence(Qt::AltModifier + Qt::Key_K));
    QObject::connect(cloneAction,SIGNAL(triggered()),this,SLOT(cloneSelectedNode()));
    editMenu->addAction(cloneAction);
    
    QAction* decloneAction = new QAction(tr("Declone"),editMenu);
    decloneAction->setShortcut(QKeySequence(Qt::AltModifier + Qt::ShiftModifier + Qt::Key_K));
    QObject::connect(decloneAction,SIGNAL(triggered()),this,SLOT(decloneSelectedNode()));
    editMenu->addAction(decloneAction);
    
    QAction* displayCacheInfoAction = new QAction(tr("Display memory consumption"),this);
    displayCacheInfoAction->setCheckable(true);
    displayCacheInfoAction->setChecked(_imp->_cacheSizeText->isVisible());
    QObject::connect(displayCacheInfoAction,SIGNAL(triggered()),this,SLOT(toggleCacheInfos()));
    _imp->_menu->addAction(displayCacheInfoAction);
    
    QAction* turnOffPreviewAction = new QAction(tr("Turn off all previews"),this);
    turnOffPreviewAction->setCheckable(true);
    turnOffPreviewAction->setChecked(false);
    QObject::connect(turnOffPreviewAction,SIGNAL(triggered()),this,SLOT(turnOffPreviewForAllNodes()));
    _imp->_menu->addAction(turnOffPreviewAction);
    
    QAction* connectionHints = new QAction(tr("Use connections hint"),this);
    connectionHints->setCheckable(true);
    connectionHints->setChecked(appPTR->getCurrentSettings()->isConnectionHintEnabled());
    QObject::connect(connectionHints,SIGNAL(triggered()),this,SLOT(toggleConnectionHints()));
    _imp->_menu->addAction(connectionHints);

    
    QAction* autoPreview = new QAction(tr("Auto preview"),this);
    autoPreview->setCheckable(true);
    autoPreview->setChecked(_imp->_gui->getApp()->getProject()->isAutoPreviewEnabled());
    QObject::connect(autoPreview,SIGNAL(triggered()),this,SLOT(toggleAutoPreview()));
    QObject::connect(_imp->_gui->getApp()->getProject().get(),SIGNAL(autoPreviewChanged(bool)),autoPreview,SLOT(setChecked(bool)));
    _imp->_menu->addAction(autoPreview);
    
    QAction* forceRefreshPreviews = new QAction(tr("Refresh previews"),this);
    forceRefreshPreviews->setShortcut(QKeySequence(Qt::Key_P));
    QObject::connect(forceRefreshPreviews,SIGNAL(triggered()),this,SLOT(forceRefreshAllPreviews()));
    _imp->_menu->addAction(forceRefreshPreviews);

    QAction* frameAllNodes = new QAction(tr("Frame nodes"),this);
    frameAllNodes->setShortcut(QKeySequence(Qt::Key_F));
    QObject::connect(frameAllNodes,SIGNAL(triggered()),this,SLOT(centerOnAllNodes()));
    _imp->_menu->addAction(frameAllNodes);

    _imp->_menu->addSeparator();
    
    std::list<ToolButton*> orederedToolButtons = _imp->_gui->getToolButtonsOrdered();
    for (std::list<ToolButton*>::iterator it = orederedToolButtons.begin(); it!=orederedToolButtons.end(); ++it) {
        (*it)->getMenu()->setIcon((*it)->getIcon());
        _imp->_menu->addAction((*it)->getMenu()->menuAction());
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
NodeGraph::showMenu(const QPoint& pos)
{
    populateMenu();
    _imp->_menu->exec(pos);
}



void
NodeGraph::dropEvent(QDropEvent* event)
{
    if(!event->mimeData()->hasUrls())
        return;
    
    QStringList filesList;
    QList<QUrl> urls = event->mimeData()->urls();
    for(int i = 0; i < urls.size() ; ++i){
        const QUrl& rl = urls.at(i);
        QString path = rl.path();

#ifdef __NATRON_WIN32__
		if (!path.isEmpty() && path.at(0) == QChar('/') || path.at(0) == QChar('\\')) {
			path = path.remove(0,1);
		}	
#endif
        QDir dir(path);
        
        //if the path dropped is not a directory append it
        if(!dir.exists()){
            filesList << path;
        }else{
            //otherwise append everything inside the dir recursively
            SequenceFileDialog::appendFilesFromDirRecursively(&dir,&filesList);
        }

    }
    
    QStringList supportedExtensions;
    std::map<std::string,std::string> writersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    for (std::map<std::string,std::string>::const_iterator it = writersForFormat.begin(); it!=writersForFormat.end(); ++it) {
        supportedExtensions.push_back(it->first.c_str());
    }
    
    std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> > files = SequenceFileDialog::fileSequencesFromFilesList(filesList,supportedExtensions);
    
    for (U32 i = 0 ; i < files.size();++i) {
        
        ///get all the decoders
        std::map<std::string,std::string> readersForFormat;
        appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
        
        boost::shared_ptr<SequenceParsing::SequenceFromFiles>& sequence = files[i];
        
        ///find a decoder for this file type
        QString first = sequence->getFilesList()[0].c_str();
        std::string ext = Natron::removeFileExtension(first).toLower().toStdString();
        
        std::map<std::string,std::string>::iterator found = readersForFormat.find(ext);
        if (found == readersForFormat.end()) {
            errorDialog("Reader", "No plugin capable of decoding " + ext + " was found.");
        } else {
            boost::shared_ptr<Natron::Node>  n = getGui()->getApp()->createNode(CreateNodeArgs(found->second.c_str(),"",-1,-1,false));
            const std::vector<boost::shared_ptr<KnobI> >& knobs = n->getKnobs();
            for (U32 i = 0; i < knobs.size(); ++i) {
                if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
                    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
                    assert(fk);
                    
                    if(!fk->isAnimationEnabled() && sequence->count() > 1){
                        errorDialog(tr("Reader").toStdString(), tr("This plug-in doesn't support image sequences, please select only 1 file.").toStdString());
                        break;
                    } else {
                        fk->setFiles(sequence->getFilesList());
                        if (n->isPreviewEnabled()) {
                            n->computePreviewImage(_imp->_gui->getApp()->getTimeLine()->currentFrame());
                        }
                        break;
                    }
                    
                }
            }

        }
        
        
    }
    
}

void
NodeGraph::dragEnterEvent(QDragEnterEvent *ev)
{
    ev->accept();
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
NodeGraph::turnOffPreviewForAllNodes()
{
    bool pTurnedOff;
    {
        QMutexLocker l(&_imp->_previewsTurnedOffMutex);
        _imp->_previewsTurnedOff = !_imp->_previewsTurnedOff;
        pTurnedOff = _imp->_previewsTurnedOff;
    }
    
    if (pTurnedOff) {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
            if ((*it)->getNode()->isPreviewEnabled()) {
                (*it)->togglePreview();
            }
        }
    } else {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
            if (!(*it)->getNode()->isPreviewEnabled() && (*it)->getNode()->makePreviewByDefault()) {
                (*it)->togglePreview();
            }
        }
    }
}

void
NodeGraph::centerOnNode(const boost::shared_ptr<NodeGui>& n)
{
    _imp->_refreshOverlays = true;
    centerOn(n.get());
}


void
NodeGraph::copySelectedNode()
{
    if (_imp->_nodeSelected) {
        _imp->resetAllClipboards();
        copyNode(_imp->_nodeSelected);
    } else if (_imp->_selectedBackDrop) {
        _imp->resetAllClipboards();
        copyBackdrop(_imp->_selectedBackDrop);
    } else {
        Natron::warningDialog(tr("Copy").toStdString(), tr("You must select a node to copy first.").toStdString());

    }
    
    
}

void
NodeGraphPrivate::resetAllClipboards()
{
    _nodeClipBoard._gui.reset();
    _nodeClipBoard._internal.reset();
    _backdropClipboard.reset();
}

void
NodeGraph::cutSelectedNode()
{
    if (_imp->_nodeSelected) {
        cutNode(_imp->_nodeSelected);
    } else if (_imp->_selectedBackDrop) {
        cutBackdrop(_imp->_selectedBackDrop);
    } else {
        Natron::warningDialog(tr("Cut").toStdString(), tr("You must select a node to cut first.").toStdString());
    }
    
}

void
NodeGraph::pasteNodeClipBoard()
{
    if (!_imp->_nodeClipBoard.isEmpty()) {
        pasteNode(*_imp->_nodeClipBoard._internal,*_imp->_nodeClipBoard._gui);
    } else if (_imp->_backdropClipboard) {
        NodeBackDrop* bd = pasteBackdrop(*_imp->_backdropClipboard,false);
        int w,h;
        _imp->_backdropClipboard->getSize(w, h);
        ///also copy the nodes within the  backdrop
        std::list<boost::shared_ptr<NodeGui> > nodes = getNodesWithinBackDrop(bd);
        std::list<boost::shared_ptr<NodeGui> > duplicates;
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodes.begin();it!=nodes.end();++it) {
            boost::shared_ptr<NodeGui> d = duplicateNode(*it);
            if (d) {
                ///cancel the duplicate offset that was applied and add the same offset that we applied to the backdrop
                d->refreshPosition(d->pos().x() -NATRON_NODE_DUPLICATE_X_OFFSET + w,d->pos().y());
                duplicates.push_back(d);
            }
        }
        bd->setPos_mt_safe(bd->pos() + QPointF(w,0));
        _imp->_undoStack->setActive();
        _imp->_undoStack->push(new DuplicateBackDropCommand(this,bd,duplicates));
        
    }
}

boost::shared_ptr<NodeGui>
NodeGraph::pasteNode(const NodeSerialization& internalSerialization,
                     const NodeGuiSerialization& guiSerialization)
{
    boost::shared_ptr<Natron::Node> n = _imp->_gui->getApp()->loadNode(LoadNodeArgs(internalSerialization.getPluginID().c_str(),
                                                                 "",
                                               internalSerialization.getPluginMajorVersion(),
                                               internalSerialization.getPluginMinorVersion(),&internalSerialization,false));
    assert(n);
    const std::string& masterNodeName = internalSerialization.getMasterNodeName();
    if (masterNodeName.empty()) {
        std::vector<boost::shared_ptr<Natron::Node> > allNodes;
        getGui()->getApp()->getActiveNodes(&allNodes);
        std::list<Double_Knob*> tracks;
        n->restoreKnobsLinks(internalSerialization,allNodes,&tracks);
        for (std::list<Double_Knob*>::iterator it = tracks.begin(); it!=tracks.end(); ++it) {
            (*it)->restoreFeatherRelatives();
        }
    } else {
        boost::shared_ptr<Natron::Node> masterNode = _imp->_gui->getApp()->getProject()->getNodeByName(masterNodeName);
        
        ///the node could not exist any longer if the user deleted it in the meantime
        if (masterNode) {
            n->getLiveInstance()->slaveAllKnobs(masterNode->getLiveInstance());
        }
    }
    boost::shared_ptr<NodeGui> gui = _imp->_gui->getApp()->getNodeGui(n);
    assert(gui);
    
    gui->copyFrom(guiSerialization);
    QPointF newPos = gui->pos() + QPointF(NATRON_NODE_DUPLICATE_X_OFFSET,0);
    gui->refreshPosition(newPos.x(), newPos.y());
    gui->forceComputePreview(_imp->_gui->getApp()->getProject()->currentFrame());
    _imp->_gui->getApp()->getProject()->triggerAutoSave();
    return gui;
}

void
NodeGraph::duplicateSelectedNode()
{
    if (_imp->_nodeSelected) {
        duplicateNode(_imp->_nodeSelected);
    } else if (_imp->_selectedBackDrop) {
        duplicateBackdrop(_imp->_selectedBackDrop);
    } else {
        Natron::warningDialog(tr("Duplicate").toStdString(), tr("You must select a node to duplicate first.").toStdString());
    }
    
}

void
NodeGraph::cloneSelectedNode()
{
    if (_imp->_nodeSelected) {
        cloneNode(_imp->_nodeSelected);
    } else if (_imp->_selectedBackDrop) {
        cloneBackdrop(_imp->_selectedBackDrop);
    } else {
        Natron::warningDialog(tr("Clone").toStdString(), tr("You must select a node to clone first.").toStdString());
    }

}

void
NodeGraph::decloneSelectedNode()
{
    if (_imp->_nodeSelected) {
        decloneNode(_imp->_nodeSelected);
    } else if (_imp->_selectedBackDrop) {
        decloneBackdrop(_imp->_selectedBackDrop);
    } else {
        Natron::warningDialog(tr("Declone").toStdString(), tr("You must select a node to declone first.").toStdString());
    }
}

void
NodeGraph::copyNode(const boost::shared_ptr<NodeGui>& n)
{
    _imp->_nodeClipBoard._internal.reset(new NodeSerialization(n->getNode()));
    _imp->_nodeClipBoard._gui.reset(new NodeGuiSerialization);
    n->serialize(_imp->_nodeClipBoard._gui.get());
}

void
NodeGraph::cutNode(const boost::shared_ptr<NodeGui>& n)
{
    copyNode(n);
    deleteNode(n);
}

boost::shared_ptr<NodeGui>
NodeGraph::duplicateNode(const boost::shared_ptr<NodeGui>& n)
{
    NodeSerialization internalSerialization(n->getNode());
    NodeGuiSerialization guiSerialization;
    n->serialize(&guiSerialization);
    
    return pasteNode(internalSerialization, guiSerialization);
}

boost::shared_ptr<NodeGui>
NodeGraph::cloneNode(const boost::shared_ptr<NodeGui>& node)
{
    if (node->getNode()->getLiveInstance()->isSlave()) {
        Natron::errorDialog(tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString());
        return boost::shared_ptr<NodeGui>();
    }
    if (node->getNode()->pluginID() == "Viewer") {
        Natron::errorDialog(tr("Clone").toStdString(), tr("Cloning a viewer is not a valid operation.").toStdString());
        return boost::shared_ptr<NodeGui>();
    }
    if (node->getNode()->isMultiInstance()) {
        Natron::errorDialog(tr("Clone").toStdString(), tr("This node cannot be cloned.").toStdString());
        return boost::shared_ptr<NodeGui>();
    }
    
    NodeSerialization internalSerialization(node->getNode());
    
    NodeGuiSerialization guiSerialization;
    node->serialize(&guiSerialization);
    
    boost::shared_ptr<Natron::Node> clone = _imp->_gui->getApp()->loadNode(LoadNodeArgs(internalSerialization.getPluginID().c_str(),
                                                                     "",
                                               internalSerialization.getPluginMajorVersion(),
                                               internalSerialization.getPluginMinorVersion(),&internalSerialization,true));
    assert(clone);
    const std::string& masterNodeName = internalSerialization.getMasterNodeName();
    
    ///the master node cannot be a clone
    assert(masterNodeName.empty());
    
    boost::shared_ptr<NodeGui> cloneGui = _imp->_gui->getApp()->getNodeGui(clone);
    assert(cloneGui);
    
    cloneGui->copyFrom(guiSerialization);
    QPointF newPos = cloneGui->pos() + QPointF(NATRON_NODE_DUPLICATE_X_OFFSET,0);
    cloneGui->refreshPosition(newPos.x(),newPos.y());
    cloneGui->forceComputePreview(_imp->_gui->getApp()->getProject()->currentFrame());
    
    clone->getLiveInstance()->slaveAllKnobs(node->getNode()->getLiveInstance());
    
    _imp->_gui->getApp()->getProject()->triggerAutoSave();
    return cloneGui;
}

void
NodeGraph::decloneNode(const boost::shared_ptr<NodeGui>& n) {
    assert(n->getNode()->getLiveInstance()->isSlave());
    n->getNode()->getLiveInstance()->unslaveAllKnobs();
    _imp->_gui->getApp()->getProject()->triggerAutoSave();
}

void
NodeGraph::deleteBackdrop(NodeBackDrop* n)
{
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(new RemoveBackDropCommand(this,n,getNodesWithinBackDrop(n)));
}

void
NodeGraph::copyBackdrop(NodeBackDrop* n)
{
    _imp->_backdropClipboard.reset(new NodeBackDropSerialization());
    _imp->_backdropClipboard->initialize(n);
}

void
NodeGraph::cutBackdrop(NodeBackDrop* n)
{
    copyBackdrop(n);
    deleteBackdrop(n);
}

NodeBackDrop*
NodeGraph::pasteBackdrop(const NodeBackDropSerialization& serialization,bool offset)
{
    NodeBackDrop* bd = new NodeBackDrop(this,_imp->_root);
    QString name(serialization.getName().c_str());
    name.append(" - copy");
    QString bearName = name;
    int no = 0;
    while (checkIfBackDropNameExists(name,NULL)) {
        ++no;
        name = bearName;
        name.append(QString::number(no));
    }
    
    bd->initialize(name, true, _imp->_gui->getPropertiesLayout());
    insertNewBackDrop(bd);
    float r,g,b;
    serialization.getColor(r,g,b);
    QColor color;
    color.setRgbF(r, g, b);
    double x,y;
    int w,h;
    serialization.getPos(x, y);
    serialization.getSize(w, h);
    bd->setCurrentColor(color);
    if (offset) {
        bd->setPos_mt_safe(QPointF(x + w,y));
    } else {
        bd->setPos_mt_safe(QPointF(x,y));
    }
    bd->resize(w, h);
    return bd;
}

void
NodeGraph::duplicateBackdrop(NodeBackDrop* n)
{
    NodeBackDropSerialization s;
    s.initialize(n);
    int w,h;
    s.getSize(w,h);
    NodeBackDrop* bd = pasteBackdrop(s);
    std::list<boost::shared_ptr<NodeGui> > nodes = getNodesWithinBackDrop(n);
    std::list<boost::shared_ptr<NodeGui> > duplicates;
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodes.begin();it!=nodes.end();++it) {
        boost::shared_ptr<NodeGui> d = duplicateNode(*it);
        if (d) {
            ///cancel the duplicate offset that was applied and add the same offset that we applied to the backdrop
            d->refreshPosition(d->pos().x(), -NATRON_NODE_DUPLICATE_X_OFFSET + w,d->pos().y());
            duplicates.push_back(d);
        }
    }
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(new DuplicateBackDropCommand(this,bd,duplicates));
}

void
NodeGraph::cloneBackdrop(NodeBackDrop* n)
{
    NodeBackDropSerialization s;
    s.initialize(n);
    int w,h;
    s.getSize(w,h);
    NodeBackDrop* bd = pasteBackdrop(s);
    bd->slaveTo(n);
    
    std::list<boost::shared_ptr<NodeGui> > nodes = getNodesWithinBackDrop(n);
    std::list<boost::shared_ptr<NodeGui> > clones;
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodes.begin();it!=nodes.end();++it) {
        boost::shared_ptr<NodeGui> c = cloneNode(*it);
        if (c) {
            c->refreshPosition(c->pos().x(), -NATRON_NODE_DUPLICATE_X_OFFSET + w,c->pos().y());
            clones.push_back(c);
        }
    }
    
    ////Note that here we use the DuplicateBackDropCommand because it has the same behaviour for clones as well.
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(new DuplicateBackDropCommand(this,bd,clones));
}

void
NodeGraph::decloneBackdrop(NodeBackDrop* n)
{
    assert(n->isSlave());
    n->unslave();
}

boost::shared_ptr<NodeGui>
NodeGraph::getNodeGuiSharedPtr(const NodeGui* n) const
{
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodes.begin();it!=_imp->_nodes.end();++it) {
        if ((*it).get() == n) {
            return *it;
        }
    }
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodesTrash.begin();it!=_imp->_nodesTrash.end();++it) {
        if ((*it).get() == n) {
            return *it;
        }
    }
    ///it must either be in the trash or in the active nodes
    assert(false);
}

void
NodeGraph::setUndoRedoStackLimit(int limit)
{
    _imp->_undoStack->clear();
    _imp->_undoStack->setUndoLimit(limit);
}

void
NodeGraph::deleteNodePermanantly(boost::shared_ptr<NodeGui> n)
{
    std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_imp->_nodesTrash.begin(),_imp->_nodesTrash.end(),n);
    if (it != _imp->_nodesTrash.end()) {
        _imp->_nodesTrash.erase(it);
    }
    boost::shared_ptr<Natron::Node> internalNode = n->getNode();
    assert(internalNode);

    if (getGui()) {
        if (internalNode->hasEffect() && internalNode->isRotoNode()) {
            getGui()->removeRotoInterface(n.get(),true);
        }
        ///now that we made the command dirty, delete the node everywhere in Natron
        getGui()->getApp()->deleteNode(n);
        
        
        getGui()->getCurveEditor()->removeNode(n.get());
        n->deleteReferences();
        if (_imp->_nodeSelected == n) {
            deselect();
        }
        
    }
    if (_imp->_nodeClipBoard._internal && _imp->_nodeClipBoard._internal->getNode() == internalNode) {
        _imp->_nodeClipBoard._internal.reset();
        _imp->_nodeClipBoard._gui.reset();
    }
    
}

void
NodeGraph::centerOnAllNodes()
{
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->_nodesMutex);
    double xmin = INT_MAX;
    double xmax = INT_MIN;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it!=_imp->_nodes.end(); ++it) {
        QSize size = (*it)->getSize();
        QPointF pos = (*it)->scenePos();
        xmin = std::min(xmin, pos.x());
        xmax = std::max(xmax,pos.x() + size.width());
        ymin = std::min(ymin,pos.y());
        ymax = std::max(ymax,pos.y() + size.height());
    }
    
    for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it!=_imp->_backdrops.end(); ++it) {
        QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
        xmin = std::min(xmin,bbox.x());
        ymin = std::min(ymin,bbox.y());
        xmax = std::max(xmax,bbox.x() + bbox.width());
        ymax = std::max(ymax,bbox.y() + bbox.height());
    }
    
    QRect rect(xmin,ymin,(xmax - xmin),(ymax - ymin));
    fitInView(rect,Qt::KeepAspectRatio);
    _imp->_refreshOverlays = true;
    repaint();
}

void
NodeGraph::toggleConnectionHints()
{
    appPTR->getCurrentSettings()->setConnectionHintsEnabled(!appPTR->getCurrentSettings()->isConnectionHintEnabled());
}

NodeBackDrop* NodeGraph::createBackDrop(QVBoxLayout *dockContainer,bool requestedByLoad)
{
    QString name(NATRON_BACKDROP_NODE_NAME);
    int no = _imp->_backdrops.size() + 1;
    name += QString::number(no);
    while (checkIfBackDropNameExists(name,NULL)) {
        ++no;
        name = QString(NATRON_BACKDROP_NODE_NAME);
        name += QString::number(no);
    }
    NodeBackDrop* bd = new NodeBackDrop(this,_imp->_root);
    bd->initialize(name, requestedByLoad, dockContainer);
    _imp->_backdrops.push_back(bd);
    _imp->_undoStack->setActive();
    if (!requestedByLoad) {
        _imp->_undoStack->push(new AddBackDropCommand(this,bd));
        if (_imp->_nodeSelected) {
            ///make the backdrop large enough to contain the node and position it correctly
            QRectF nodeBbox = _imp->_nodeSelected->mapToParent(_imp->_nodeSelected->boundingRect()).boundingRect();
            int nodeW = nodeBbox.width();
            int nodeH = nodeBbox.height();
            bd->setPos(nodeBbox.x() - nodeW / 2., nodeBbox.y() - nodeH * 2);
            bd->resize(nodeW * 2, nodeH * 4);
        } else {
            QRectF viewPos = visibleRect();
            QPointF mapped = bd->mapFromScene(QPointF((viewPos.bottomRight().x() + viewPos.topLeft().x()) / 2.,
                                        (viewPos.topLeft().y() + viewPos.bottomRight().y()) / 2.));
            mapped = bd->mapToParent(mapped);
            bd->setPos(mapped);
            bd->resize(NATRON_BACKDROP_DEFAULT_WIDTH,NATRON_BACKDROP_DEFAULT_HEIGHT);
        }
    }

    return bd;
}


void
NodeGraph::pushRemoveBackDropCommand(NodeBackDrop* bd)
{
    _imp->_undoStack->setActive();
    _imp->_undoStack->push(new RemoveBackDropCommand(this,bd,getNodesWithinBackDrop(bd)));
}

bool
NodeGraph::checkIfBackDropNameExists(const QString& n,const NodeBackDrop* bd) const
{
    for (std::list<NodeBackDrop*>::const_iterator it = _imp->_backdrops.begin(); it!=_imp->_backdrops.end(); ++it) {
        if ((*it)->getName() == n && (*it) != bd) {
            return true;
        }
    }
    return false;
}

std::list<NodeBackDrop*>
NodeGraph::getBackDrops() const
{
    return _imp->_backdrops;
}

std::list<NodeBackDrop*>
NodeGraph::getActiveBackDrops() const
{
    std::list<NodeBackDrop*> ret;
    for (std::list<NodeBackDrop*>::const_iterator it = _imp->_backdrops.begin() ; it!=_imp->_backdrops.end();++it) {
        if ((*it)->isVisible()) {
            ret.push_back(*it);
        }
    }
    return ret;
}



std::list<boost::shared_ptr<NodeGui> > NodeGraph::getNodesWithinBackDrop(const NodeBackDrop* bd) const
{
    QRectF bbox = bd->mapToScene(bd->boundingRect()).boundingRect();
    std::list<boost::shared_ptr<NodeGui> > ret;
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end();++it) {
        QRectF nodeBbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
        if (bbox.contains(nodeBbox)) {
            ret.push_back(*it);
        }
    }
    return ret;
}

void NodeGraph::insertNewBackDrop(NodeBackDrop* bd)
{
    _imp->_backdrops.push_back(bd);
}

void NodeGraph::removeBackDrop(NodeBackDrop* bd)
{
    std::list<NodeBackDrop*>::iterator it = std::find(_imp->_backdrops.begin(),_imp->_backdrops.end(),bd);
    if (it != _imp->_backdrops.end()) {
        _imp->_backdrops.erase(it);
    }
}

void NodeGraph::onTimeChanged(SequenceTime time,int reason)
{
    std::vector<ViewerInstance* > viewers;
    
    boost::shared_ptr<Natron::Project> project = _imp->_gui->getApp()->getProject();
    
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin() ;it!=_imp->_nodes.end();++it) {
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>((*it)->getNode()->getLiveInstance());
        if (isViewer) {
            viewers.push_back(isViewer);
        }
        (*it)->refreshKnobsAfterTimeChange(time);
    }
    Natron::OutputEffectInstance* lastTimelineSeekCaller = project->getLastTimelineSeekCaller();
    for(U32 i = 0; i < viewers.size();++i){
        if(viewers[i] != lastTimelineSeekCaller || reason == USER_SEEK) {
            boost::shared_ptr<VideoEngine> engine = viewers[i]->getVideoEngine();
            engine->render(1, //< frame count
                           false, //< seek timeline
                           false, //<refresh tree
                           true, //< forward
                           false, // <same frame
                           false); //< force preview
        }
    }
    project->setLastTimelineSeekCaller(NULL);
}
