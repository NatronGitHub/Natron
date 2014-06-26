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

#define NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS 1000

#define NATRON_BACKDROP_DEFAULT_WIDTH 80
#define NATRON_BACKDROP_DEFAULT_HEIGHT 80

#define NATRON_NODE_DUPLICATE_X_OFFSET 50

using namespace Natron;
using std::cout; using std::endl;



class MoveCommand : public QUndoCommand{
public:
    MoveCommand(const boost::shared_ptr<NodeGui>& node, const QPointF &oldPos,
                QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    virtual int id() const { return kNodeGraphMoveNodeCommandCompressionID; }
    virtual bool mergeWith(const QUndoCommand *command);

    
private:
    boost::shared_ptr<NodeGui> _node;
    QPointF _oldPos;
    QPointF _newPos;
};


class AddCommand : public QUndoCommand{
public:

    AddCommand(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QUndoCommand *parent = 0);
    
    virtual ~AddCommand();
    
    virtual void undo();
    virtual void redo();

private:
    std::list<boost::shared_ptr<Natron::Node> > _outputs;
    std::vector<boost::shared_ptr<Natron::Node> > _inputs;
    boost::shared_ptr<NodeGui> _node;
    NodeGraph* _graph;
    bool _undoWasCalled;
    bool _isUndone;
};

class RemoveCommand : public QUndoCommand{
public:

    RemoveCommand(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QUndoCommand *parent = 0);
    
    virtual ~RemoveCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    std::list<boost::shared_ptr<Natron::Node> > _outputs;
    std::vector<boost::shared_ptr<Natron::Node> > _inputs;
    boost::shared_ptr<NodeGui> _node; //< mutable because we need to modify it externally
    NodeGraph* _graph;
    bool _isRedone;
};

class ConnectCommand : public QUndoCommand{
public:
    ConnectCommand(NodeGraph* graph,Edge* edge,const boost::shared_ptr<NodeGui>&oldSrc,
                   const boost::shared_ptr<NodeGui>& newSrc,QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();
    
private:
    Edge* _edge;
    boost::shared_ptr<NodeGui> _oldSrc,_newSrc;
    boost::shared_ptr<NodeGui> _dst;
    NodeGraph* _graph;
    int _inputNb;
};

class AddBackDropCommand : public QUndoCommand {
    
public:
    
    AddBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,QUndoCommand *parent = 0);
    virtual ~AddBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    
    NodeGraph* _graph;
    NodeBackDrop* _bd;
    bool _isUndone;
};


class RemoveBackDropCommand : public QUndoCommand {
    
public:
    
    RemoveBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,const std::list<boost::shared_ptr<NodeGui> >& nodes,QUndoCommand *parent = 0);
    virtual ~RemoveBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    
    NodeGraph* _graph;
    NodeBackDrop* _bd;
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    bool _isRedone;
};


class MoveBackDropCommand : public QUndoCommand {
    
public:
    
    MoveBackDropCommand(NodeBackDrop* bd,double dx,double dy,
                        const std::list<boost::shared_ptr<NodeGui> >& nodes,
                        QUndoCommand *parent = 0);
    virtual ~MoveBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    virtual int id() const { return kNodeGraphMoveNodeBackDropCommandCompressionID; }
    virtual bool mergeWith(const QUndoCommand *command);
    
    
private:
    
    void move(double dx,double dy);
    
    NodeBackDrop* _bd;
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    double _dx,_dy;

};

class ResizeBackDropCommand : public QUndoCommand {
    
public:
    
    ResizeBackDropCommand(NodeBackDrop* bd,int w,int h,QUndoCommand *parent = 0);
    virtual ~ResizeBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    virtual int id() const { return kNodeGraphResizeNodeBackDropCommandCompressionID; }
    virtual bool mergeWith(const QUndoCommand *command);
    
    
private:
    
    NodeBackDrop* _bd;
    int _w,_h;
    int _oldW,_oldH;
    
};

class DuplicateBackDropCommand : public QUndoCommand {
    
public:
    
    DuplicateBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,const std::list<boost::shared_ptr<NodeGui> >& nodes,QUndoCommand *parent = 0);
    virtual ~DuplicateBackDropCommand();
    
    virtual void undo();
    virtual void redo();
    
private:
    
    NodeGraph* _graph;
    NodeBackDrop* _bd;
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    bool _isUndone;
    bool _firstRedoCalled;
};

NodeGraph::NodeGraph(Gui* gui,QGraphicsScene* scene,QWidget *parent):
    QGraphicsView(scene,parent),
    _gui(gui),
    _evtState(DEFAULT),
    _nodeSelected(),
    _nodeSelectedScaleBeforeMagnif(1.),
    _magnifOn(false),
    _propertyBin(0),
    _refreshOverlays(true),
    _previewsTurnedOff(false),
    _nodeClipBoard(),
    _highLightedEdge(NULL),
    _hintInputEdge(NULL),
    _hintOutputEdge(NULL),
    _selectedBackDrop(NULL),
    _firstMove(true)
{
    setAcceptDrops(true);
    
    QObject::connect(_gui->getApp()->getProject().get(), SIGNAL(nodesCleared()), this, SLOT(onProjectNodesCleared()));
    
    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    scale(qreal(0.8), qreal(0.8));
    
    smartNodeCreationEnabled=true;
    _root = new QGraphicsTextItem(0);
    _nodeRoot = new QGraphicsTextItem(_root);
    // _root->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_root);
    _navigator = new NodeGraphNavigator();
    _navigatorProxy = new QGraphicsProxyWidget(0);
    _navigatorProxy->setWidget(_navigator);
     scene->addItem(_navigatorProxy);
    _navigatorProxy->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _navigatorProxy->hide();
    
    QPen p;
    p.setBrush(QColor(200,200,200));
    p.setWidth(2);
    
    _navLeftEdge = new QGraphicsLineItem(0);
    _navLeftEdge->setPen(p);
    scene->addItem(_navLeftEdge);
    _navLeftEdge->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _navLeftEdge->hide();
    
    _navBottomEdge = new QGraphicsLineItem(0);
    _navBottomEdge->setPen(p);
    scene->addItem(_navBottomEdge);
    _navBottomEdge->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _navBottomEdge->hide();
    
    _navRightEdge = new QGraphicsLineItem(0);
    _navRightEdge->setPen(p);
    scene->addItem(_navRightEdge);
    _navRightEdge->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _navRightEdge->hide();
    
    _navTopEdge = new QGraphicsLineItem(0);
    _navTopEdge->setPen(p);
    scene->addItem(_navTopEdge);
    _navTopEdge->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _navTopEdge->hide();
    
    _cacheSizeText = new QGraphicsTextItem(0);
    scene->addItem(_cacheSizeText);
    _cacheSizeText->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _cacheSizeText->setDefaultTextColor(QColor(200,200,200));
    
    QObject::connect(&_refreshCacheTextTimer,SIGNAL(timeout()),this,SLOT(updateCacheSizeText()));
    _refreshCacheTextTimer.start(NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS);
    
    _undoStack = new QUndoStack(this);
    _undoStack->setUndoLimit(appPTR->getCurrentSettings()->getMaximumUndoRedoNodeGraph());
    _gui->registerNewUndoStack(_undoStack); 
    
    _hintInputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_nodeRoot);
    _hintInputEdge->setDefaultColor(QColor(0,255,0,100));
    _hintInputEdge->hide();
    
    _hintOutputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_nodeRoot);
    _hintOutputEdge->setDefaultColor(QColor(0,255,0,100));
    _hintOutputEdge->hide();
    
    _tL = new QGraphicsTextItem(0);
    _tL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_tL);
    
    _tR = new QGraphicsTextItem(0);
    _tR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_tR);
    
    _bR = new QGraphicsTextItem(0);
    _bR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_bR);
    
    _bL = new QGraphicsTextItem(0);
    _bL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_bL);
    
    scene->setSceneRect(0,0,10000,10000);
    _tL->setPos(_tL->mapFromScene(QPointF(0,10000)));
    _tR->setPos(_tR->mapFromScene(QPointF(10000,10000)));
    _bR->setPos(_bR->mapFromScene(QPointF(10000,0)));
    _bL->setPos(_bL->mapFromScene(QPointF(0,0)));
    centerOn(5000,5000);
    
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    _menu = new QMenu(this);
    
}

NodeGraph::~NodeGraph() {
    
    QGraphicsScene* scene = _hintInputEdge->scene();
    if (scene) {
        scene->removeItem(_hintInputEdge);
    }
    _hintInputEdge->setParentItem(NULL);
    delete _hintInputEdge;
    
    scene = _hintOutputEdge->scene();
    if (scene) {
        scene->removeItem(_hintOutputEdge);
    }
    _hintOutputEdge->setParentItem(NULL);
    delete _hintOutputEdge;

    QObject::disconnect(&_refreshCacheTextTimer,SIGNAL(timeout()),this,SLOT(updateCacheSizeText()));
    _nodeCreationShortcutEnabled = false;

    onProjectNodesCleared();

}

void NodeGraph::onProjectNodesCleared() {
    
    _nodeSelected.reset();
    QMutexLocker l(&_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
        (*it)->deleteReferences();
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodesTrash.begin(); it!=_nodesTrash.end(); ++it) {
        (*it)->deleteReferences();
    }
    _nodes.clear();
    _nodesTrash.clear();
    _undoStack->clear();
}

void NodeGraph::resizeEvent(QResizeEvent* event){
    _refreshOverlays = true;
    QGraphicsView::resizeEvent(event);
}
void NodeGraph::paintEvent(QPaintEvent* event){
    if(_refreshOverlays){
        QRectF visible = visibleRect();
        //cout << visible.topLeft().x() << " " << visible.topLeft().y() << " " << visible.width() << " " << visible.height() << endl;
        _cacheSizeText->setPos(visible.topLeft());
        QSize navSize = _navigator->sizeHint();
        QPointF navPos = visible.bottomRight() - QPoint(navSize.width(),navSize.height());
        //   cout << navPos.x() << " " << navPos.y() << endl;
        _navigatorProxy->setPos(navPos);
        _navLeftEdge->setLine(navPos.x(),
                              navPos.y() + navSize.height(),
                              navPos.x(),
                              navPos.y());
        _navLeftEdge->setPos(navPos);
        _navTopEdge->setLine(navPos.x(),
                             navPos.y(),
                             navPos.x() + navSize.width(),
                             navPos.y());
        _navTopEdge->setPos(navPos);
        _navRightEdge->setLine(navPos.x() + navSize.width() ,
                               navPos.y(),
                               navPos.x() + navSize.width() ,
                               navPos.y() + navSize.height());
        _navRightEdge->setPos(navPos);
        _navBottomEdge->setLine(navPos.x() + navSize.width() ,
                                navPos.y() + navSize.height(),
                                navPos.x(),
                                navPos.y() + navSize.height());
        _navBottomEdge->setPos(navPos);
        _refreshOverlays = false;
    }
    QGraphicsView::paintEvent(event);
}
QRectF NodeGraph::visibleRect() {
    //    QPointF tl(horizontalScrollBar()->value(), verticalScrollBar()->value());
    //    QPointF br = tl + viewport()->rect().bottomRight();
    //    QMatrix mat = matrix().inverted();
    //    return mat.mapRect(QRectF(tl,br));
    
    return mapToScene(viewport()->rect()).boundingRect();
}

boost::shared_ptr<NodeGui> NodeGraph::createNodeGUI(QVBoxLayout *dockContainer,const boost::shared_ptr<Natron::Node>& node,
                                                    bool requestedByLoad){
  
    boost::shared_ptr<NodeGui> node_ui(new NodeGui(_nodeRoot));
    node_ui->initialize(this, node_ui, dockContainer, node, requestedByLoad);
    moveNodesForIdealPosition(node_ui);
    
    {
        QMutexLocker l(&_nodesMutex);
        _nodes.push_back(node_ui);
    }
    QUndoStack* nodeStack = node_ui->getUndoStack();
    if(nodeStack){
        _gui->registerNewUndoStack(nodeStack);
    }
    
    ///before we add a new node, clear exceeding entries
    _undoStack->setActive();
    _undoStack->push(new AddCommand(this,node_ui));
    _evtState = DEFAULT;
    return node_ui;
    
}

void NodeGraph::moveNodesForIdealPosition(boost::shared_ptr<NodeGui> node) {
    QRectF viewPos = visibleRect();
    
    ///3 possible values:
    /// 0 = default , i.e: we pop the node in the middle of the graph's current view
    /// 1 = pop the node above the selected node and move the inputs of the selected node a little
    /// 2 = pop the node below the selected node and move the outputs of the selected node a little
    int behavior = 0;
    
    if (!_nodeSelected) {
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
        if (_nodeSelected->getNode()->isOutputNode()) {
            
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
                if (_nodeSelected->getNode()->maximumInputs() == 0) {
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
        const std::vector<boost::shared_ptr<Natron::Node> >& inputs = _nodeSelected->getNode()->getInputs_mt_safe();
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
        QSize selectedNodeSize = _nodeSelected->getSize();
        QSize createdNodeSize = node->getSize();
        QPointF selectedNodeMiddlePos = _nodeSelected->scenePos() + QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);
        

        position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
        position.setY(selectedNodeMiddlePos.y() - selectedNodeSize.height() / 2 - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES
                      - createdNodeSize.height());
        
        QRectF createdNodeRect(position.x(),position.y(),createdNodeSize.width(),createdNodeSize.height());

        ///now that we have the position of the node, move the inputs of the selected node to make some space for this node
        const std::map<int,Edge*>& selectedNodeInputs = _nodeSelected->getInputsArrows();
        for (std::map<int,Edge*>::const_iterator it = selectedNodeInputs.begin(); it!=selectedNodeInputs.end();++it) {
            if (it->second->hasSource()) {
                it->second->getSource()->moveAbovePositionRecursively(createdNodeRect);
            }
        }
        
    }
    ///pop it below the selected node
    else {
        QSize selectedNodeSize = _nodeSelected->getSize();
        QSize createdNodeSize = node->getSize();
        QPointF selectedNodeMiddlePos = _nodeSelected->scenePos() + QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);
        
        ///actually move the created node where the selected node is
        position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
        position.setY(selectedNodeMiddlePos.y() + (selectedNodeSize.height() / 2) + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
        
        QRectF createdNodeRect(position.x(),position.y(),createdNodeSize.width(),createdNodeSize.height());
        
        ///and move the selected node below recusively
        const std::list<boost::shared_ptr<Natron::Node> >& outputs = _nodeSelected->getNode()->getOutputs();
        for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!= outputs.end(); ++it) {
            assert(*it);
            boost::shared_ptr<NodeGui> output = _gui->getApp()->getNodeGui(*it);
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
        _evtState = MOVING_AREA;
        return;
    }
    
    _lastScenePosClick = mapToScene(event->pos());
    
    boost::shared_ptr<NodeGui> selected ;
    Edge* selectedEdge = 0;
    {
        QMutexLocker l(&_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::reverse_iterator it = _nodes.rbegin();it!=_nodes.rend();++it) {
            boost::shared_ptr<NodeGui>& n = *it;
            QPointF evpt = n->mapFromScene(_lastScenePosClick);
            if (n->isActive() && n->contains(evpt)) {
                selected = n;
                break;
            } else {
                Edge* edge = n->hasEdgeNearbyPoint(_lastScenePosClick);
                if (edge) {
                    selectedEdge = edge;
                    break;
                }
            }
        }
    }
    
    if (selected) {
        selectNode(selected);
        if(event->button() == Qt::LeftButton){
            _evtState = NODE_DRAGGING;
            _lastNodeDragStartPoint = selected->pos();
        }
        else if(event->button() == Qt::RightButton){
            selected->showMenu(mapToGlobal(event->pos()));
        }
    } else if(selectedEdge) {
        _arrowSelected = selectedEdge;
        _evtState = ARROW_DRAGGING;
    }
    
    NodeBackDrop* oldSelection = _selectedBackDrop;
    _selectedBackDrop = 0;
    if (_evtState == DEFAULT) {
        ///check if nearby a backdrop
        for (std::list<NodeBackDrop*>::iterator it = _backdrops.begin(); it!=_backdrops.end(); ++it) {
            if ((*it)->isNearbyHeader(_lastScenePosClick)) {
                _selectedBackDrop = *it;
                if ((oldSelection != _selectedBackDrop && oldSelection != NULL) || !oldSelection) {
                    _selectedBackDrop->setSelected(true);
                }
                if (event->button() == Qt::LeftButton) {
                    _evtState = BACKDROP_DRAGGING;
                }
                break;
            } else if ((*it)->isNearbyResizeHandle(_lastScenePosClick)) {
                _selectedBackDrop = *it;
                if ((oldSelection != _selectedBackDrop && oldSelection != NULL) || !oldSelection) {
                    _selectedBackDrop->setSelected(true);
                }
                if (event->button() == Qt::LeftButton) {
                    _evtState = BACKDROP_RESIZING;
                }
                break;
            }
        }
    }
    
    if (_selectedBackDrop && event->button() == Qt::RightButton) {
        _selectedBackDrop->showMenu(mapToGlobal(event->pos()));
    }
    
    if (_selectedBackDrop && oldSelection && oldSelection != _selectedBackDrop) {
        oldSelection->setSelected(false);
    }
    
    if (!_selectedBackDrop && event->button() == Qt::LeftButton) {
        if (oldSelection) {
            oldSelection->setSelected(false);
        }
    }
    
    if (!selected && !selectedEdge && event->button() == Qt::LeftButton) {
        deselect();
    }
    
    if (!_selectedBackDrop && !selected && !selectedEdge) {
        if (event->button() == Qt::RightButton) {
            showMenu(mapToGlobal(event->pos()));
        } else if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier)) {
            _evtState = MOVING_AREA;
            QGraphicsView::mousePressEvent(event);
        }
    }
    
    
}

void NodeGraph::deselect() {
    {
        QMutexLocker l(&_nodesMutex);
        for(std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
            (*it)->setSelected(false);
        }
        if (_nodeSelected && _magnifOn) {
            _magnifOn = false;
            _nodeSelected->setScale_natron(_nodeSelectedScaleBeforeMagnif);
            
        }
        _nodeSelected.reset();
    }
}

void NodeGraph::mouseReleaseEvent(QMouseEvent *event){
    EVENT_STATE state = _evtState;
    _firstMove = true;
    _nodesToMoveWithBackDrop.clear();
    _evtState = DEFAULT;

    if (state == ARROW_DRAGGING) {
        bool foundSrc = false;
        boost::shared_ptr<NodeGui> nodeHoldingEdge = _arrowSelected->isOutputEdge() ?
        _arrowSelected->getSource() : _arrowSelected->getDest();
        assert(nodeHoldingEdge);
        
        std::list<boost::shared_ptr<NodeGui> > nodes = getAllActiveNodes_mt_safe();
        QPointF ep = mapToScene(event->pos());

        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
            boost::shared_ptr<NodeGui>& n = *it;
            
            if(n->isActive() && n->isNearby(ep) &&
                    (n->getNode()->getName() != nodeHoldingEdge->getNode()->getName())){
               
                
                if (!_arrowSelected->isOutputEdge()) {
                    ///can't connect to a viewer
                    if (n->getNode()->pluginID() == "Viewer") {
                        break;
                    }
                    
//                    std::string sequentialNodeName;
//                    if (n->getNode()->hasSequentialOnlyNodeUpstream(sequentialNodeName)) {
//                        ///check if the output has a viewer connected in which case error
//                        std::list <ViewerInstance*> connectedViewers;
//                        _arrowSelected->getDest()->getNode()->hasViewersConnected(&connectedViewers);
//
//                        if (!connectedViewers.empty()) {
//                            Natron::errorDialog("Connection", "You cannot connect a tree with a Viewer to " + sequentialNodeName +
//                                                " which only works on sequential renders (i.e with a Writer node.).");
//                            break;
//                        }
//                        
//                    }
                   
                    _arrowSelected->stackBefore(n.get());
                    _undoStack->setActive();
                    _undoStack->push(new ConnectCommand(this,_arrowSelected,_arrowSelected->getSource(),n));
                } else {
//                    
//                    std::string sequentialNodeName;
//                    if (_arrowSelected->getSource()->getNode()->hasSequentialOnlyNodeUpstream(sequentialNodeName)) {
//                        std::list <ViewerInstance*> connectedViewers;
//                        n->getNode()->hasViewersConnected(&connectedViewers);
//                        if (!connectedViewers.empty()) {
//                            Natron::errorDialog("Connection", "You cannot connect a tree with a Viewer to " + sequentialNodeName +
//                                                " which only works on sequential renders (i.e with a Writer node.).");
//                            break;
//                        }
//
//                    }
                    
                    ///Find the input edge of the node we just released the mouse over,
                    ///and use that edge to connect to the source of the selected edge.
                    int preferredInput = n->getNode()->getPreferredInputForConnection();
                    if (preferredInput != -1) {
                        const std::map<int,Edge*>& inputEdges = n->getInputsArrows();
                        std::map<int,Edge*>::const_iterator foundInput = inputEdges.find(preferredInput);
                        assert(foundInput != inputEdges.end());
                        _undoStack->setActive();
                        _undoStack->push(new ConnectCommand(this,foundInput->second,
                                                            foundInput->second->getSource(),_arrowSelected->getSource()));
                    
                    }
                }
                foundSrc = true;
                
                break;
            }
        }
        
        ///if we disconnected the input edge, use the undo/redo stack.
        ///Output edges can never be really connected, they're just there
        ///So the user understands some nodes can have output
        if (!foundSrc && !_arrowSelected->isOutputEdge() && _arrowSelected->getSource()) {
            _undoStack->setActive();
            _undoStack->push(new ConnectCommand(this,_arrowSelected,_arrowSelected->getSource(),boost::shared_ptr<NodeGui>()));
            scene()->update();
        }
        nodeHoldingEdge->refreshEdges();
        scene()->update();
    } else if (state == NODE_DRAGGING) {
        if (_nodeSelected) {
            _undoStack->setActive();
            _undoStack->push(new MoveCommand(_nodeSelected,_lastNodeDragStartPoint));
            
            ///now if there was a hint displayed, use it to actually make connections.
            if (_highLightedEdge) {
                if (_highLightedEdge->isOutputEdge()) {
                    int prefInput = _nodeSelected->getNode()->getPreferredInputForConnection();
                    if (prefInput != -1) {
                        Edge* inputEdge = _nodeSelected->getInputArrow(prefInput);
                        assert(inputEdge);
                        _undoStack->push(new ConnectCommand(this,inputEdge,inputEdge->getSource(),_highLightedEdge->getSource()));
                    }
                } else {
                    
                    boost::shared_ptr<NodeGui> src = _highLightedEdge->getSource();
                    _undoStack->push(new ConnectCommand(this,_highLightedEdge,_highLightedEdge->getSource(),_nodeSelected));
                    
                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> >& inpNodes = _nodeSelected->getNode()->getInputs_mt_safe();
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
                        int prefInput = _nodeSelected->getNode()->getPreferredInputForConnection();
                        if (prefInput != -1) {
                            Edge* inputEdge = _nodeSelected->getInputArrow(prefInput);
                            assert(inputEdge);
                            _undoStack->push(new ConnectCommand(this,inputEdge,inputEdge->getSource(),src));
                        }
                    }
                }
                _highLightedEdge->setUseHighlight(false);
                _highLightedEdge = 0;
                _hintInputEdge->hide();
                _hintOutputEdge->hide();
            }
        }
    }
    scene()->update();
    
    
    setCursor(QCursor(Qt::ArrowCursor));
}
void NodeGraph::mouseMoveEvent(QMouseEvent *event) {
    
    QPointF newPos = mapToScene(event->pos());
    double dx = _root->mapFromScene(newPos).x() - _root->mapFromScene(_lastScenePosClick).x();
    double dy = _root->mapFromScene(newPos).y() - _root->mapFromScene(_lastScenePosClick).y();
    {
        ///set cursor
        boost::shared_ptr<NodeGui> selected;
        Edge* selectedEdge = 0;
        
        {
            QMutexLocker l(&_nodesMutex);
            for(std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it){
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
    switch (_evtState) {
        case ARROW_DRAGGING: {
            QPointF np=_arrowSelected->mapFromScene(newPos);
            if (_arrowSelected->isOutputEdge()) {
                _arrowSelected->dragDest(np);
            } else {
                _arrowSelected->dragSource(np);
            }
        } break;
        case NODE_DRAGGING: {
            
            if (!_nodeSelected) {
                ///it should never happen for real
                return;
            }
            
            QSize size = _nodeSelected->getSize();
            newPos = _nodeSelected->mapToParent(_nodeSelected->mapFromScene(newPos));
            _nodeSelected->refreshPosition(newPos.x() - size.width() / 2,newPos.y() - size.height() / 2);
            
            
            ///try to find a nearby edge
            boost::shared_ptr<Natron::Node> internalNode = _nodeSelected->getNode();
            bool doHints = appPTR->getCurrentSettings()->isConnectionHintEnabled();
            
            ///for readers already connected don't show hint
            if (internalNode->maximumInputs() == 0 && internalNode->hasOutputConnected()) {
                doHints = false;
            } else if (internalNode->maximumInputs() > 0 && internalNode->hasInputConnected() && internalNode->hasOutputConnected()) {
                doHints = false;
            }
            
            if (doHints) {
                QRectF rect = _nodeSelected->mapToParent(_nodeSelected->boundingRect()).boundingRect();
                double tolerance = 20;
                rect.adjust(-tolerance, -tolerance, tolerance, tolerance);
                
                Edge* edge = 0;
                {
                    QMutexLocker l(&_nodesMutex);
                    for(std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it){
                        boost::shared_ptr<NodeGui>& n = *it;
                        if (n != _nodeSelected) {
                            edge = n->hasEdgeNearbyRect(rect);
                            
                            ///if the edge input is the selected node don't continue
                            if (edge && edge->getSource() == _nodeSelected) {
                                edge = 0;
                            }
                            
                            if (edge && edge->isOutputEdge()) {
                                ///if the edge is an output edge but the node doesn't have any inputs don't continue
                                if (_nodeSelected->getInputsArrows().empty()) {
                                    edge = 0;
                                    
                                }
                                ///if the source of that edge is already connected also skip
                                const std::vector<boost::shared_ptr<Natron::Node> >& inpNodes = _nodeSelected->getNode()->getInputs_mt_safe();
                                for (U32 i = 0; i < inpNodes.size();++i) {
                                    if (edge && inpNodes[i] == edge->getSource()->getNode()) {
                                        edge = 0;
                                        break;
                                    }
                                }
                                
                            }
                            
                            if (edge && !edge->isOutputEdge()) {
                                ///if the edge is an input edge but the selected node is a viewer don't continue (viewer can't have output)
                                if ( _nodeSelected->getNode()->pluginID() == "Viewer") {
                                    edge = 0;
                                }
                                
//                                std::string sequentialNodeName;
//                                if (_nodeSelected->getNode()->hasSequentialOnlyNodeUpstream(sequentialNodeName)) {
//                                    std::list<ViewerInstance*> connectedViewers;
//                                    edge->getDest()->getNode()->hasViewersConnected(&connectedViewers);
//                                    if (!connectedViewers.empty()) {
//                                        return;
//                                    }
//                                }

                                
                                ///if the selected node doesn't have any input but the edge has an input don't continue
                                if (edge && _nodeSelected->getInputsArrows().empty() && edge->getSource()) {
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
                if (_highLightedEdge && _highLightedEdge != edge) {
                    _highLightedEdge->setUseHighlight(false);
                    _hintInputEdge->hide();
                    _hintOutputEdge->hide();
                }
                
                _highLightedEdge = edge;
                
                if (edge && edge->getSource() && edge->getDest()) {
                    ///setup the hints edge
                    
                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> >& inpNodes = _nodeSelected->getNode()->getInputs_mt_safe();
                    for (U32 i = 0; i < inpNodes.size();++i) {
                        if (inpNodes[i] == edge->getSource()->getNode()) {
                            alreadyConnected = true;
                            break;
                        }
                    }
                    
                    if (!_hintInputEdge->isVisible()) {
                        if (!alreadyConnected) {
                            int prefInput = _nodeSelected->getNode()->getPreferredInputForConnection();
                            _hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _hintInputEdge->setSourceAndDestination(edge->getSource(), _nodeSelected);
                            _hintInputEdge->setVisible(true);
                            
                        }
                        _hintOutputEdge->setInputNumber(edge->getInputNumber());
                        _hintOutputEdge->setSourceAndDestination(_nodeSelected, edge->getDest());
                        _hintOutputEdge->setVisible(true);
                    } else {
                        if (!alreadyConnected) {
                            _hintInputEdge->initLine();
                        }
                        _hintOutputEdge->initLine();
                    }
                } else if (edge) {
                    ///setup only 1 of the hints edge
                    
                    if (_highLightedEdge && !_hintInputEdge->isVisible()) {
                        if (edge->isOutputEdge()) {
                            int prefInput = _nodeSelected->getNode()->getPreferredInputForConnection();
                            _hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _hintInputEdge->setSourceAndDestination(edge->getSource(), _nodeSelected);
                            
                        } else {
                            _hintInputEdge->setInputNumber(edge->getInputNumber());
                            _hintInputEdge->setSourceAndDestination(_nodeSelected,edge->getDest());
                            
                        }
                        _hintInputEdge->setVisible(true);
                    } else if (_highLightedEdge && _hintInputEdge->isVisible()) {
                        _hintInputEdge->initLine();
                    }
                }
                
            }
            
            setCursor(QCursor(Qt::ClosedHandCursor));

        } break;
        case MOVING_AREA: {
            _root->moveBy(dx, dy);
            setCursor(QCursor(Qt::SizeAllCursor));
        } break;
        case BACKDROP_DRAGGING: {
            assert(_selectedBackDrop);
            _undoStack->setActive();
            if (_firstMove) {
                _nodesToMoveWithBackDrop = getNodesWithinBackDrop(_selectedBackDrop);
                _firstMove = false;
            }
            bool controlHeld = event->modifiers().testFlag(Qt::ControlModifier);
            _undoStack->push(new MoveBackDropCommand(_selectedBackDrop,dx,dy,
                                                     controlHeld ? std::list<boost::shared_ptr<NodeGui> >() : _nodesToMoveWithBackDrop));
        } break;
        case BACKDROP_RESIZING: {
            assert(_selectedBackDrop);
            _undoStack->setActive();
            QPointF p = _selectedBackDrop->scenePos();
            int w = newPos.x() - p.x();
            int h = newPos.y() - p.y();
            _undoStack->push(new ResizeBackDropCommand(_selectedBackDrop,w,h));
        } break;
        default: {

        } break;
    }

    _lastScenePosClick = newPos;
    update();
    
    /*Now update navigator*/
    //updateNavigator();
}


void NodeGraph::mouseDoubleClickEvent(QMouseEvent *) {
    
    std::list<boost::shared_ptr<NodeGui> > nodes = getAllActiveNodes_mt_safe();
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodes.begin();it!=nodes.end();++it) {
        QPointF evpt = (*it)->mapFromScene(_lastScenePosClick);
        if((*it)->isActive() && (*it)->contains(evpt) && (*it)->getSettingPanel()){
            if(!(*it)->isSettingsPanelVisible()){
                (*it)->setVisibleSettingsPanel(true);
            }
            if (!(*it)->wasBeginEditCalled()) {
                (*it)->beginEditKnobs();
            }
            _gui->putSettingsPanelFirst((*it)->getSettingPanel());
            getGui()->getApp()->redrawAllViewers();
            return;
        }
    }

    for (std::list<NodeBackDrop*>::iterator it = _backdrops.begin(); it!=_backdrops.end(); ++it) {
        if ((*it)->isNearbyHeader(_lastScenePosClick)) {
            if ((*it)->isSettingsPanelClosed()) {
                (*it)->setSettingsPanelClosed(false);
            }
            _gui->putSettingsPanelFirst((*it)->getSettingsPanel());
            return;
        }
    }
}
bool NodeGraph::event(QEvent* event){
    if ( event->type() == QEvent::KeyPress ) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (ke &&  ke->key() == Qt::Key_Tab && _nodeCreationShortcutEnabled ) {
            if(smartNodeCreationEnabled){
                //releaseKeyboard();
                QPoint global = mapToGlobal(mapFromScene(_lastScenePosClick.toPoint()));
                SmartInputDialog* nodeCreation=new SmartInputDialog(this);
                nodeCreation->move(global.x(), global.y());
                QPoint position=_gui->getWorkshopPane()->pos();
                position+=QPoint(_gui->width()/2,0);
                nodeCreation->move(position);
                setMouseTracking(false);
                
                nodeCreation->show();
                nodeCreation->raise();
                nodeCreation->activateWindow();
                
                
                smartNodeCreationEnabled=false;
            }
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
    } else if (e->key() == Qt::Key_R) {
        _gui->createReader();
    } else if (e->key() == Qt::Key_W) {
        _gui->createWriter();
    } else if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete) {
        /*delete current node.*/
        if (_selectedBackDrop) {
            _undoStack->setActive();
            _undoStack->push(new RemoveBackDropCommand(this,_selectedBackDrop,getNodesWithinBackDrop(_selectedBackDrop)));
            _selectedBackDrop = 0;
        } else {
            deleteSelectedNode();
        }
    } else if (e->key() == Qt::Key_P) {
        forceRefreshAllPreviews();
    } else if (e->key() == Qt::Key_C && e->modifiers().testFlag(Qt::ControlModifier)) {
        copySelectedNode();
    } else if (e->key() == Qt::Key_V && e->modifiers().testFlag(Qt::ControlModifier)) {
        pasteNodeClipBoard();
    } else if (e->key() == Qt::Key_X && e->modifiers().testFlag(Qt::ControlModifier)) {
        cutSelectedNode();
    } else if (e->key() == Qt::Key_C && e->modifiers().testFlag(Qt::AltModifier)) {
        duplicateSelectedNode();
    } else if (e->key() == Qt::Key_K && e->modifiers().testFlag(Qt::AltModifier) && !e->modifiers().testFlag(Qt::ShiftModifier)) {
        cloneSelectedNode();
    } else if (e->key() == Qt::Key_K && e->modifiers().testFlag(Qt::AltModifier) && e->modifiers().testFlag(Qt::ShiftModifier)) {
        decloneSelectedNode();
    } else if (e->key() == Qt::Key_F) {
        centerOnAllNodes();
    } else if (e->key() == Qt::Key_H) {
        toggleConnectionHints();
    }

    
}
void NodeGraph::connectCurrentViewerToSelection(int inputNB){
    
    if (!_gui->getLastSelectedViewer()) {
        _gui->getApp()->createNode("Viewer");
    }
    
    ///get a pointer to the last user selected viewer
    boost::shared_ptr<InspectorNode> v = boost::dynamic_pointer_cast<InspectorNode>(_gui->getLastSelectedViewer()->
                                                                                    getInternalNode()->getNode());
    
    ///if the node is no longer active (i.e: it was deleted by the user), don't do anything.
    if (!v->isActivated()) {
        return;
    }
    
    ///get a ptr to the NodeGui
    boost::shared_ptr<NodeGui> gui = _gui->getApp()->getNodeGui(v);
    assert(gui);
    
    ///if there's no selected node or the viewer is selected, then try refreshing that input nb if it is connected.
    if (!_nodeSelected || _nodeSelected == gui) {
        v->setActiveInputAndRefresh(inputNB);
        gui->refreshEdges();
        return;
    }
    

    ///if the selected node is a viewer, return, we can't connect a viewer to another viewer.
    if (_nodeSelected->getNode()->pluginID() == "Viewer") {
        return;
    }
    
//    std::string sequentialNodeName;
//    if (_nodeSelected->getNode()->hasSequentialOnlyNodeUpstream(sequentialNodeName)) {
//        Natron::errorDialog("Connection", "You cannot connect a tree with a Viewer to " + sequentialNodeName +
//                            " which only works on sequential renders (i.e with a Writer node.).");
//        return;
//    }
//    

    
    ///if the node doesn't have the input 'inputNb' created yet, populate enough input
    ///so it can be created.
    NodeGui::InputEdgesMap::const_iterator it = gui->getInputsArrows().find(inputNB);
    while (it == gui->getInputsArrows().end()) {
        v->addEmptyInput();
        it = gui->getInputsArrows().find(inputNB);
    }
    
    ///set the undostack the active one before connecting
    _undoStack->setActive();
    
    ///and push a connect command to the selected node.
    _undoStack->push(new ConnectCommand(this,it->second,it->second->getSource(),_nodeSelected));
    
    ///Set the viewer as the selected node
    selectNode(gui);
    
}

void NodeGraph::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);
    if (smartNodeCreationEnabled) {
        _nodeCreationShortcutEnabled=true;
        setFocus();
    }
}
void NodeGraph::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    if(smartNodeCreationEnabled){
        _nodeCreationShortcutEnabled=false;
        setFocus();
    }
}


void NodeGraph::wheelEvent(QWheelEvent *event){
    if (event->orientation() != Qt::Vertical) {
        return;
    }

    double scaleFactor = pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if(factor < 0.07 || factor > 10)
        return;
    
    if (event->modifiers().testFlag(Qt::ControlModifier) && _nodeSelected) {
        if (!_magnifOn) {
            _magnifOn = true;
            _nodeSelectedScaleBeforeMagnif = _nodeSelected->scale();
        }
        _nodeSelected->setScale_natron(_nodeSelected->scale() * scaleFactor);
    } else {
        scale(scaleFactor,scaleFactor);
        _refreshOverlays = true;
    }
    QPointF newPos = mapToScene(event->pos());
    _lastScenePosClick = newPos;
}

void NodeGraph::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control && _magnifOn) {
        _magnifOn = false;
        _nodeSelected->setScale_natron(_nodeSelectedScaleBeforeMagnif);
    }
}

void NodeGraph::deleteSelectedNode(){
    if(_nodeSelected){
        _undoStack->setActive();
        _nodeSelected->setSelected(false);
        _undoStack->push(new RemoveCommand(this,_nodeSelected));
        _nodeSelected.reset();
    }
}

void NodeGraph::deleteSelectedBackdrop()
{
    if (_selectedBackDrop) {
        _undoStack->setActive();
        _undoStack->push(new RemoveBackDropCommand(this,_selectedBackDrop,getNodesWithinBackDrop(_selectedBackDrop)));
    }
}

void NodeGraph::deleteNode(const boost::shared_ptr<NodeGui>& n) {
    assert(n);
    _undoStack->setActive();
    n->setSelected(false);
    _undoStack->push(new RemoveCommand(this,n));
}


void NodeGraph::selectNode(const boost::shared_ptr<NodeGui>& n) {
    if (_nodeSelected && _nodeSelected != n && _magnifOn) {
        _magnifOn = false;
        _nodeSelected->setScale_natron(_nodeSelectedScaleBeforeMagnif);
        
    }
    
    assert(n);
    _nodeSelected = n;
    
    /*now remove previously selected node*/
    {
        QMutexLocker l(&_nodesMutex);
        for(std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
            (*it)->setSelected(false);
        }
    }
    if(n->getNode()->pluginID() == "Viewer") {
        OpenGLViewerI* viewer = dynamic_cast<ViewerInstance*>(n->getNode()->getLiveInstance())->getUiContext();
        const std::list<ViewerTab*>& viewerTabs = _gui->getViewersList();
        for(std::list<ViewerTab*>::const_iterator it = viewerTabs.begin();it!=viewerTabs.end();++it) {
            if((*it)->getViewer() == viewer) {
                _gui->setLastSelectedViewer((*it));
            }
        }
    }
    
    
    n->setSelected(true);
}


void NodeGraph::updateNavigator(){
    if (!areAllNodesVisible()) {
        _navigator->setImage(getFullSceneScreenShot());
        _navigator->show();
        _navLeftEdge->show();
        _navBottomEdge->show();
        _navRightEdge->show();
        _navTopEdge->show();

    }else{
        _navigator->hide();
        _navLeftEdge->hide();
        _navTopEdge->hide();
        _navRightEdge->hide();
        _navBottomEdge->hide();
    }
}
bool NodeGraph::areAllNodesVisible(){
    QRectF rect = visibleRect();
    QMutexLocker l(&_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
        if(!rect.contains((*it)->boundingRectWithEdges()))
            return false;
    }
    return true;
}

QImage NodeGraph::getFullSceneScreenShot(){
    const QTransform& currentTransform = transform();
    setTransform(currentTransform.inverted());
    QRectF sceneR = calcNodesBoundingRect();
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
    scene()->removeItem(_navLeftEdge);
    scene()->removeItem(_navBottomEdge);
    scene()->removeItem(_navTopEdge);
    scene()->removeItem(_navRightEdge);
    scene()->removeItem(_cacheSizeText);
    scene()->removeItem(_navigatorProxy);
    scene()->render(&painter,QRectF(),sceneR);
    scene()->addItem(_navigatorProxy);
    scene()->addItem(_cacheSizeText);
    scene()->addItem(_navLeftEdge);
    scene()->addItem(_navBottomEdge);
    scene()->addItem(_navTopEdge);
    scene()->addItem(_navRightEdge);
    p.setColor(QColor(200,200,200,255));
    painter.setPen(p);
    painter.fillRect(viewRect, QColor(200,200,200,100));
    setTransform(currentTransform);
    return img;
}

NodeGraph::NodeGraphNavigator::NodeGraphNavigator(QWidget* parent ):QLabel(parent),
    _w(120),_h(70){
    
}

void NodeGraph::NodeGraphNavigator::setImage(const QImage& img){
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(_w, _h);
    setPixmap(pix);
}

const std::list<boost::shared_ptr<NodeGui> >& NodeGraph::getAllActiveNodes() const {
    return _nodes;
}

std::list<boost::shared_ptr<NodeGui> > NodeGraph::getAllActiveNodes_mt_safe() const {
    QMutexLocker l(&_nodesMutex);
    return _nodes;
}

void NodeGraph::moveToTrash(NodeGui* node) {
    assert(node);
    QMutexLocker l(&_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
        if ((*it).get() == node) {
            _nodesTrash.push_back(*it);
            _nodes.erase(it);
            break;
        }
    }
}

void NodeGraph::restoreFromTrash(NodeGui* node) {
    assert(node);
    QMutexLocker l(&_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodesTrash.begin();it!=_nodesTrash.end();++it) {
        if ((*it).get() == node) {
            _nodes.push_back(*it);
            _nodesTrash.erase(it);
            break;
        }
    }
}


MoveCommand::MoveCommand(const boost::shared_ptr<NodeGui>& node, const QPointF &oldPos,
                         QUndoCommand *parent):QUndoCommand(parent),
    _node(node),
    _oldPos(oldPos),
    _newPos(node->pos()){
        assert(node);
    
}
void MoveCommand::undo(){

    _node->refreshPosition(_oldPos.x(),_oldPos.y());
    _node->refreshEdges();
    
    if(_node->scene())
        _node->scene()->update();
    setText(QObject::tr("Move %1")
            .arg(_node->getNode()->getName().c_str()));
}
void MoveCommand::redo(){

    _node->refreshPosition(_newPos.x(),_newPos.y());
    _node->refreshEdges();
    setText(QObject::tr("Move %1")
            .arg(_node->getNode()->getName().c_str()));
}
bool MoveCommand::mergeWith(const QUndoCommand *command){
    
    const MoveCommand *moveCommand = static_cast<const MoveCommand *>(command);

    const boost::shared_ptr<NodeGui>& node = moveCommand->_node;
    if(_node != node)
        return false;
    _newPos = node->pos();
    setText(QObject::tr("Move %1")
            .arg(node->getNode()->getName().c_str()));
    return true;
}


AddCommand::AddCommand(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QUndoCommand *parent):QUndoCommand(parent),
    _node(node),_graph(graph),_undoWasCalled(false), _isUndone(false){
    
}

AddCommand::~AddCommand()
{
    if (_isUndone) {
        _graph->deleteNodePermanantly(_node);
    }
}

void AddCommand::undo(){

    _isUndone = true;
    _undoWasCalled = true;
    
    
    
    _inputs = _node->getNode()->getInputs_mt_safe();
    _outputs = _node->getNode()->getOutputs();
    
    _node->getNode()->deactivate();
    
    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNode()->getName().c_str()));
    
}
void AddCommand::redo(){

    _isUndone = false;
    if(_undoWasCalled){
        ///activate will trigger an autosave
        _node->getNode()->activate();
    }
    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNode()->getName().c_str()));
    
    
}

RemoveCommand::RemoveCommand(NodeGraph* graph,const boost::shared_ptr<NodeGui>& node,QUndoCommand *parent):QUndoCommand(parent),
    _node(node),_graph(graph) , _isRedone(false){
        assert(node);
}

RemoveCommand::~RemoveCommand()
{
    if (_isRedone) {
        _graph->deleteNodePermanantly(_node);
    }
}

void RemoveCommand::undo() {

    _node->getNode()->activate();
    _isRedone = false;
    _graph->scene()->update();
    setText(QObject::tr("Remove %1")
            .arg(_node->getNode()->getName().c_str()));
    
    
    
}
void RemoveCommand::redo() {

    _isRedone = true;
    _inputs = _node->getNode()->getInputs_mt_safe();
    _outputs = _node->getNode()->getOutputs();
    
    _node->getNode()->deactivate();
    
    
    for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        assert(*it);
        boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>(*it);
        ///if the node is an inspector, when disconnecting the active input just activate another input instead
        if (inspector) {
            const std::vector<boost::shared_ptr<Natron::Node> >& inputs = inspector->getInputs_mt_safe();
            ///set as active input the first non null input
            for (U32 i = 0; i < inputs.size() ;++i) {
                if (inputs[i]) {
                    inspector->setActiveInputAndRefresh(i);
                    break;
                }
            }
        }
    }
    
    std::list<SequenceTime> keyframes;
    _node->getNode()->getAllKnobsKeyframes(&keyframes);
    _graph->getGui()->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(keyframes);
    
    _graph->scene()->update();
    setText(QObject::tr("Remove %1")
            .arg(_node->getNode()->getName().c_str()));
    
}


ConnectCommand::ConnectCommand(NodeGraph* graph,Edge* edge,const boost::shared_ptr<NodeGui>& oldSrc,
                               const boost::shared_ptr<NodeGui>& newSrc,QUndoCommand *parent):QUndoCommand(parent),
    _edge(edge),
    _oldSrc(oldSrc),
    _newSrc(newSrc),
    _dst(edge->getDest()),
    _graph(graph),
    _inputNb(edge->getInputNumber())
{
    assert(_dst);
}

void ConnectCommand::undo() {
    
    boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>(_dst->getNode());
    
    if (inspector) {
        ///if the node is an inspector, the redo() action might have disconnect the dst and src nodes
        ///hence the _edge ptr might have been invalidated, recreate it
        NodeGui::InputEdgesMap::const_iterator it = _dst->getInputsArrows().find(_inputNb);
        while (it == _dst->getInputsArrows().end()) {
            inspector->addEmptyInput();
            it = _dst->getInputsArrows().find(_inputNb);
        }
        _edge = it->second;
    }
    
    if(_oldSrc){
        _graph->getGui()->getApp()->getProject()->connectNodes(_edge->getInputNumber(), _oldSrc->getNode(), _edge->getDest()->getNode());
        _oldSrc->refreshOutputEdgeVisibility();
    }
    if(_newSrc){
        _graph->getGui()->getApp()->getProject()->disconnectNodes(_newSrc->getNode(), _edge->getDest()->getNode());
        _newSrc->refreshOutputEdgeVisibility();
        ///if the node is an inspector, when disconnecting the active input just activate another input instead
        if (inspector) {
            const std::vector<boost::shared_ptr<Natron::Node> >& inputs = inspector->getInputs_mt_safe();
            ///set as active input the first non null input
            for (U32 i = 0; i < inputs.size() ;++i) {
                if (inputs[i]) {
                    inspector->setActiveInputAndRefresh(i);
                    break;
                }
            }
        }
    }
    
    if(_oldSrc){
        setText(QObject::tr("Connect %1 to %2")
                .arg(_edge->getDest()->getNode()->getName().c_str()).arg(_oldSrc->getNode()->getName().c_str()));
    }else{
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNode()->getName().c_str()));
    }
    
    _graph->getGui()->getApp()->triggerAutoSave();
    std::list<ViewerInstance* > viewers;
    _edge->getDest()->getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->updateTreeAndRender();
    }    
}
void ConnectCommand::redo() {
    
    boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>(_dst->getNode());
    _inputNb = _edge->getInputNumber();
    if (inspector) {
        ///if the node is an inspector we have to do things differently
        
        if (!_newSrc) {
            if (_oldSrc) {
                ///we want to connect to nothing, hence disconnect
                _graph->getGui()->getApp()->getProject()->disconnectNodes(_oldSrc->getNode(),inspector);
                _oldSrc->refreshOutputEdgeVisibility();
            }
        } else {
            ///disconnect any connection already existing with the _oldSrc
            if (_oldSrc) {
                _graph->getGui()->getApp()->getProject()->disconnectNodes(_oldSrc->getNode(),inspector);
                _oldSrc->refreshOutputEdgeVisibility();
            }
            ///also disconnect any current connection between the inspector and the _newSrc
            _graph->getGui()->getApp()->getProject()->disconnectNodes(_newSrc->getNode(),inspector);
            
            
            ///after disconnect calls the _edge pointer might be invalid since the edges might have been destroyed.
            NodeGui::InputEdgesMap::const_iterator it = _dst->getInputsArrows().find(_inputNb);
            while (it == _dst->getInputsArrows().end()) {
                inspector->addEmptyInput();
                it = _dst->getInputsArrows().find(_inputNb);
            }
            _edge = it->second;
            
            ///and connect the inspector to the _newSrc
            _graph->getGui()->getApp()->getProject()->connectNodes(_inputNb, _newSrc->getNode(), inspector);
            _newSrc->refreshOutputEdgeVisibility();
        }
       
    } else {
        _edge->setSource(_newSrc);
        if (_oldSrc) {
            if(!_graph->getGui()->getApp()->getProject()->disconnectNodes(_oldSrc->getNode(), _dst->getNode())){
                cout << "Failed to disconnect (input) " << _oldSrc->getNode()->getName()
                     << " to (output) " << _dst->getNode()->getName() << endl;
            }
            _oldSrc->refreshOutputEdgeVisibility();
        }
        if (_newSrc) {
            if(!_graph->getGui()->getApp()->getProject()->connectNodes(_inputNb, _newSrc->getNode(), _dst->getNode())){
                cout << "Failed to connect (input) " << _newSrc->getNode()->getName()
                     << " to (output) " << _dst->getNode()->getName() << endl;

            }
            _newSrc->refreshOutputEdgeVisibility();
        }
        
    }

    assert(_dst);
    _dst->refreshEdges();
    
    if (_newSrc) {
        setText(QObject::tr("Connect %1 to %2")
                .arg(_edge->getDest()->getNode()->getName().c_str()).arg(_newSrc->getNode()->getName().c_str()));
       

    } else {
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNode()->getName().c_str()));
    }
    
    ///if the node has no inputs, all the viewers attached to that node should get disconnected. This will be done
    ///in VideoEngine::startEngine
    std::list<ViewerInstance* > viewers;
    _edge->getDest()->getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->updateTreeAndRender();
    }
    
    ViewerInstance* isDstAViewer = dynamic_cast<ViewerInstance*>(_edge->getDest()->getNode()->getLiveInstance());
    if (!isDstAViewer) {
        _graph->getGui()->getApp()->triggerAutoSave();
    }
    
    
    
}



SmartInputDialog::SmartInputDialog(NodeGraph* graph_)
    : QDialog()
    , graph(graph_)
    , layout(NULL)
    , textLabel(NULL)
    , textEdit(NULL)
{
    setWindowTitle(QString("Node creation tool"));
    setWindowFlags(Qt::Popup);
    setObjectName(QString("SmartDialog"));
    setStyleSheet(QString("SmartInputDialog#SmartDialog{border-style:outset;border-width: 2px; border-color: black; background-color:silver;}"));
    layout=new QVBoxLayout(this);
    textLabel=new QLabel(QString("Input a node name:"),this);
    textEdit=new QComboBox(this);
    textEdit->setEditable(true);
    
    textEdit->addItems(appPTR->getNodeNameList());
    layout->addWidget(textLabel);
    layout->addWidget(textEdit);
    textEdit->lineEdit()->selectAll();
    //textEdit->setFocusPolicy(Qt::StrongFocus);
    // setFocusProxy(textEdit->lineEdit());
    //textEdit->lineEdit()->setFocus(Qt::ActiveWindowFocusReason);
    textEdit->setFocus();
    installEventFilter(this);
    
    
}
void SmartInputDialog::keyPressEvent(QKeyEvent *e){
    if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QString res=textEdit->lineEdit()->text();
        if(appPTR->getNodeNameList().contains(res)){
            graph->getGui()->getApp()->createNode(res);
            graph->setSmartNodeCreationEnabled(true);
            graph->setMouseTracking(true);
            //textEdit->releaseKeyboard();
            
            graph->setFocus(Qt::ActiveWindowFocusReason);
            delete this;
            
            
        }
    }else if(e->key()== Qt::Key_Escape){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        //textEdit->releaseKeyboard();
        
        graph->setFocus(Qt::ActiveWindowFocusReason);
        
        
        delete this;
        
        
    }
}

bool
SmartInputDialog::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj);
    
    if(e->type()==QEvent::Close){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        //textEdit->releaseKeyboard();
        
        graph->setFocus(Qt::ActiveWindowFocusReason);
        
        
    }
    return false;
}

void
NodeGraph::refreshAllEdges()
{
    QMutexLocker l(&_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
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
    _cacheSizeText->setPlainText(QString("Memory cache size: %1")
                                 .arg(QDirModelPrivate_size(appPTR->getCachesTotalMemorySize())));
}

QRectF
NodeGraph::calcNodesBoundingRect()
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
    if(_cacheSizeText->isVisible()){
        _cacheSizeText->hide();
    }else{
        _cacheSizeText->show();
    }
}

void
NodeGraph::populateMenu()
{
    _menu->clear();
    
    
    QMenu* editMenu = new QMenu(tr("Edit"),_menu);
    _menu->addAction(editMenu->menuAction());
    
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
    pasteAction->setEnabled(!_nodeClipBoard.isEmpty());
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
    displayCacheInfoAction->setChecked(_cacheSizeText->isVisible());
    QObject::connect(displayCacheInfoAction,SIGNAL(triggered()),this,SLOT(toggleCacheInfos()));
    _menu->addAction(displayCacheInfoAction);
    
    QAction* turnOffPreviewAction = new QAction(tr("Turn off all previews"),this);
    turnOffPreviewAction->setCheckable(true);
    turnOffPreviewAction->setChecked(false);
    QObject::connect(turnOffPreviewAction,SIGNAL(triggered()),this,SLOT(turnOffPreviewForAllNodes()));
    _menu->addAction(turnOffPreviewAction);
    
    QAction* connectionHints = new QAction(tr("Use connections hint"),this);
    connectionHints->setCheckable(true);
    connectionHints->setChecked(appPTR->getCurrentSettings()->isConnectionHintEnabled());
    QObject::connect(connectionHints,SIGNAL(triggered()),this,SLOT(toggleConnectionHints()));
    _menu->addAction(connectionHints);

    
    QAction* autoPreview = new QAction(tr("Auto preview"),this);
    autoPreview->setCheckable(true);
    autoPreview->setChecked(_gui->getApp()->getProject()->isAutoPreviewEnabled());
    QObject::connect(autoPreview,SIGNAL(triggered()),this,SLOT(toggleAutoPreview()));
    QObject::connect(_gui->getApp()->getProject().get(),SIGNAL(autoPreviewChanged(bool)),autoPreview,SLOT(setChecked(bool)));
    _menu->addAction(autoPreview);
    
    QAction* forceRefreshPreviews = new QAction(tr("Refresh previews"),this);
    forceRefreshPreviews->setShortcut(QKeySequence(Qt::Key_P));
    QObject::connect(forceRefreshPreviews,SIGNAL(triggered()),this,SLOT(forceRefreshAllPreviews()));
    _menu->addAction(forceRefreshPreviews);

    QAction* frameAllNodes = new QAction(tr("Frame nodes"),this);
    frameAllNodes->setShortcut(QKeySequence(Qt::Key_F));
    QObject::connect(frameAllNodes,SIGNAL(triggered()),this,SLOT(centerOnAllNodes()));
    _menu->addAction(frameAllNodes);

    _menu->addSeparator();
    
    std::list<ToolButton*> orederedToolButtons = _gui->getToolButtonsOrdered();
    for (std::list<ToolButton*>::iterator it = orederedToolButtons.begin(); it!=orederedToolButtons.end(); ++it) {
        (*it)->getMenu()->setIcon((*it)->getIcon());
        _menu->addAction((*it)->getMenu()->menuAction());
    }
   
    
}

void
NodeGraph::toggleAutoPreview()
{
    _gui->getApp()->getProject()->toggleAutoPreview();
}

void
NodeGraph::forceRefreshAllPreviews()
{
    _gui->forceRefreshAllPreviews();
}

void
NodeGraph::showMenu(const QPoint& pos)
{
    populateMenu();
    _menu->exec(pos);
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
            boost::shared_ptr<Natron::Node>  n = getGui()->getApp()->createNode(found->second.c_str(),-1,-1,false);
            const std::vector<boost::shared_ptr<KnobI> >& knobs = n->getKnobs();
            for (U32 i = 0; i < knobs.size(); ++i) {
                if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
                    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
                    assert(fk);
                    
                    if(!fk->isAnimationEnabled() && sequence->count() > 1){
                        errorDialog("Reader", "This plug-in doesn't support image sequences, please select only 1 file.");
                        break;
                    } else {
                        fk->setFiles(sequence->getFilesList());
                        if (n->isPreviewEnabled()) {
                            n->computePreviewImage(_gui->getApp()->getTimeLine()->currentFrame());
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
        QMutexLocker l(&_previewsTurnedOffMutex);
        _previewsTurnedOff = !_previewsTurnedOff;
        pTurnedOff = _previewsTurnedOff;
    }
    
    if (pTurnedOff) {
        QMutexLocker l(&_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
            if ((*it)->getNode()->isPreviewEnabled()) {
                (*it)->togglePreview();
            }
        }
    } else {
        QMutexLocker l(&_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin();it!=_nodes.end();++it) {
            if (!(*it)->getNode()->isPreviewEnabled() && (*it)->getNode()->makePreviewByDefault()) {
                (*it)->togglePreview();
            }
        }
    }
}

void
NodeGraph::centerOnNode(const boost::shared_ptr<NodeGui>& n)
{
    _refreshOverlays = true;
    centerOn(n.get());
}


void
NodeGraph::copySelectedNode()
{
    if (_nodeSelected) {
        resetAllClipboards();
        copyNode(_nodeSelected);
    } else if (_selectedBackDrop) {
        resetAllClipboards();
        copyBackdrop(_selectedBackDrop);
    } else {
        Natron::warningDialog("Copy", "You must select a node to copy first.");

    }
    
    
}

void
NodeGraph::resetAllClipboards()
{
    _nodeClipBoard._gui.reset();
    _nodeClipBoard._internal.reset();
    _backdropClipboard.reset();
}

void
NodeGraph::cutSelectedNode()
{
    if (_nodeSelected) {
        cutNode(_nodeSelected);
    } else if (_selectedBackDrop) {
        cutBackdrop(_selectedBackDrop);
    } else {
        Natron::warningDialog("Cut", "You must select a node to cut first.");
    }
    
}

void
NodeGraph::pasteNodeClipBoard()
{
    if (!_nodeClipBoard.isEmpty()) {
        pasteNode(*_nodeClipBoard._internal,*_nodeClipBoard._gui);
    } else if (_backdropClipboard) {
        NodeBackDrop* bd = pasteBackdrop(*_backdropClipboard,false);
        int w,h;
        _backdropClipboard->getSize(w, h);
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
        _undoStack->setActive();
        _undoStack->push(new DuplicateBackDropCommand(this,bd,duplicates));
        
    }
}

boost::shared_ptr<NodeGui>
NodeGraph::pasteNode(const NodeSerialization& internalSerialization,
                     const NodeGuiSerialization& guiSerialization)
{
    boost::shared_ptr<Natron::Node> n = _gui->getApp()->loadNode(internalSerialization.getPluginID().c_str(),
                                               internalSerialization.getPluginMajorVersion(),
                                               internalSerialization.getPluginMinorVersion(),internalSerialization,false);
    assert(n);
    const std::string& masterNodeName = internalSerialization.getMasterNodeName();
    if (masterNodeName.empty()) {
        std::vector<boost::shared_ptr<Natron::Node> > allNodes;
        getGui()->getApp()->getActiveNodes(&allNodes);
        n->restoreKnobsLinks(internalSerialization,allNodes);
    } else {
        boost::shared_ptr<Natron::Node> masterNode = _gui->getApp()->getProject()->getNodeByName(masterNodeName);
        
        ///the node could not exist any longer if the user deleted it in the meantime
        if (masterNode) {
            n->getLiveInstance()->slaveAllKnobs(masterNode->getLiveInstance());
        }
    }
    boost::shared_ptr<NodeGui> gui = _gui->getApp()->getNodeGui(n);
    assert(gui);
    
    gui->copyFrom(guiSerialization);
    QPointF newPos = gui->pos() + QPointF(NATRON_NODE_DUPLICATE_X_OFFSET,0);
    gui->refreshPosition(newPos.x(), newPos.y());
    gui->forceComputePreview(_gui->getApp()->getProject()->currentFrame());
    _gui->getApp()->getProject()->triggerAutoSave();
    return gui;
}

void
NodeGraph::duplicateSelectedNode()
{
    if (_nodeSelected) {
        duplicateNode(_nodeSelected);
    } else if (_selectedBackDrop) {
        duplicateBackdrop(_selectedBackDrop);
    } else {
        Natron::warningDialog("Duplicate", "You must select a node to duplicate first.");
    }
    
}

void
NodeGraph::cloneSelectedNode()
{
    if (_nodeSelected) {
        cloneNode(_nodeSelected);
    } else if (_selectedBackDrop) {
        cloneBackdrop(_selectedBackDrop);
    } else {
        Natron::warningDialog("Clone", "You must select a node to clone first.");
    }

}

void
NodeGraph::decloneSelectedNode()
{
    if (_nodeSelected) {
        decloneNode(_nodeSelected);
    } else if (_selectedBackDrop) {
        decloneBackdrop(_selectedBackDrop);
    } else {
        Natron::warningDialog("Declone", "You must select a node to declone first.");
    }
}

void
NodeGraph::copyNode(const boost::shared_ptr<NodeGui>& n)
{
    _nodeClipBoard._internal.reset(new NodeSerialization(n->getNode()));
    _nodeClipBoard._gui.reset(new NodeGuiSerialization);
    n->serialize(_nodeClipBoard._gui.get());
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
        Natron::warningDialog("Clone", "You cannot clone a node whose already a clone.");
        return boost::shared_ptr<NodeGui>();
    }
    if (node->getNode()->pluginID() == "Viewer") {
        Natron::warningDialog("Clone", "Cloning a viewer is not a valid operation.");
        return boost::shared_ptr<NodeGui>();
    }
    
    NodeSerialization internalSerialization(node->getNode());
    
    NodeGuiSerialization guiSerialization;
    node->serialize(&guiSerialization);
    
    boost::shared_ptr<Natron::Node> clone = _gui->getApp()->loadNode(internalSerialization.getPluginID().c_str(),
                                               internalSerialization.getPluginMajorVersion(),
                                               internalSerialization.getPluginMinorVersion(),internalSerialization,true);
    assert(clone);
    const std::string& masterNodeName = internalSerialization.getMasterNodeName();
    
    ///the master node cannot be a clone
    assert(masterNodeName.empty());
    
    boost::shared_ptr<NodeGui> cloneGui = _gui->getApp()->getNodeGui(clone);
    assert(cloneGui);
    
    cloneGui->copyFrom(guiSerialization);
    QPointF newPos = cloneGui->pos() + QPointF(NATRON_NODE_DUPLICATE_X_OFFSET,0);
    cloneGui->refreshPosition(newPos.x(),newPos.y());
    cloneGui->forceComputePreview(_gui->getApp()->getProject()->currentFrame());
    
    clone->getLiveInstance()->slaveAllKnobs(node->getNode()->getLiveInstance());
    
    _gui->getApp()->getProject()->triggerAutoSave();
    return cloneGui;
}

void
NodeGraph::decloneNode(const boost::shared_ptr<NodeGui>& n) {
    assert(n->getNode()->getLiveInstance()->isSlave());
    n->getNode()->getLiveInstance()->unslaveAllKnobs();
    _gui->getApp()->getProject()->triggerAutoSave();
}

void
NodeGraph::deleteBackdrop(NodeBackDrop* n)
{
    _undoStack->setActive();
    _undoStack->push(new RemoveBackDropCommand(this,n,getNodesWithinBackDrop(n)));
}

void
NodeGraph::copyBackdrop(NodeBackDrop* n)
{
    _backdropClipboard.reset(new NodeBackDropSerialization());
    _backdropClipboard->initialize(n);
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
    NodeBackDrop* bd = new NodeBackDrop(this,_root);
    QString name(serialization.getName().c_str());
    name.append(" - copy");
    QString bearName = name;
    int no = 0;
    while (checkIfBackDropNameExists(name,NULL)) {
        ++no;
        name = bearName;
        name.append(QString::number(no));
    }
    
    bd->initialize(name, true, _gui->getPropertiesLayout());
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
    _undoStack->setActive();
    _undoStack->push(new DuplicateBackDropCommand(this,bd,duplicates));
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
    _undoStack->setActive();
    _undoStack->push(new DuplicateBackDropCommand(this,bd,clones));
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
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin();it!=_nodes.end();++it) {
        if ((*it).get() == n) {
            return *it;
        }
    }
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodesTrash.begin();it!=_nodesTrash.end();++it) {
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
    _undoStack->clear();
    _undoStack->setUndoLimit(limit);
}

void
NodeGraph::deleteNodePermanantly(boost::shared_ptr<NodeGui> n)
{
    std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_nodesTrash.begin(),_nodesTrash.end(),n);
    if (it != _nodesTrash.end()) {
        _nodesTrash.erase(it);
    }
    boost::shared_ptr<Natron::Node> internalNode = n->getNode();
    assert(internalNode);

    if (getGui()) {
        if (internalNode->hasEffect() && internalNode->isRotoNode()) {
            getGui()->removeRotoInterface(n.get(),true);
        }
        ///now that we made the command dirty, delete the node everywhere in Natron
        getGui()->getApp()->deleteNode(n);
        
        
        getGui()->getCurveEditor()->removeNode(n);
        n->deleteReferences();
        if (_nodeSelected == n) {
            deselect();
        }
        
    }
    if (_nodeClipBoard._internal && _nodeClipBoard._internal->getNode() == internalNode) {
        _nodeClipBoard._internal.reset();
        _nodeClipBoard._gui.reset();
    }
    
}

void
NodeGraph::centerOnAllNodes()
{
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_nodesMutex);
    double xmin = INT_MAX;
    double xmax = INT_MIN;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
        QSize size = (*it)->getSize();
        QPointF pos = (*it)->scenePos();
        xmin = std::min(xmin, pos.x());
        xmax = std::max(xmax,pos.x() + size.width());
        ymin = std::min(ymin,pos.y());
        ymax = std::max(ymax,pos.y() + size.height());
    }
    
    for (std::list<NodeBackDrop*>::iterator it = _backdrops.begin(); it!=_backdrops.end(); ++it) {
        QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
        xmin = std::min(xmin,bbox.x());
        ymin = std::min(ymin,bbox.y());
        xmax = std::max(xmax,bbox.x() + bbox.width());
        ymax = std::max(ymax,bbox.y() + bbox.height());
    }
    
    QRect rect(xmin,ymin,(xmax - xmin),(ymax - ymin));
    fitInView(rect,Qt::KeepAspectRatio);
    _refreshOverlays = true;
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
    int no = _backdrops.size() + 1;
    name += QString::number(no);
    while (checkIfBackDropNameExists(name,NULL)) {
        ++no;
        name = QString(NATRON_BACKDROP_NODE_NAME);
        name += QString::number(no);
    }
    NodeBackDrop* bd = new NodeBackDrop(this,_root);
    bd->initialize(name, requestedByLoad, dockContainer);
    _backdrops.push_back(bd);
    _undoStack->setActive();
    if (!requestedByLoad) {
        _undoStack->push(new AddBackDropCommand(this,bd));
        if (_nodeSelected) {
            ///make the backdrop large enough to contain the node and position it correctly
            QRectF nodeBbox = _nodeSelected->mapToParent(_nodeSelected->boundingRect()).boundingRect();
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
     _undoStack->setActive();
    _undoStack->push(new RemoveBackDropCommand(this,bd,getNodesWithinBackDrop(bd)));
}

bool
NodeGraph::checkIfBackDropNameExists(const QString& n,const NodeBackDrop* bd) const
{
    for (std::list<NodeBackDrop*>::const_iterator it = _backdrops.begin(); it!=_backdrops.end(); ++it) {
        if ((*it)->getName() == n && (*it) != bd) {
            return true;
        }
    }
    return false;
}

std::list<NodeBackDrop*>
NodeGraph::getBackDrops() const
{
    return _backdrops;
}

std::list<NodeBackDrop*>
NodeGraph::getActiveBackDrops() const
{
    std::list<NodeBackDrop*> ret;
    for (std::list<NodeBackDrop*>::const_iterator it = _backdrops.begin() ; it!=_backdrops.end();++it) {
        if ((*it)->isActive() && (*it)->isVisible()) {
            ret.push_back(*it);
        }
    }
    return ret;
}


//////// Implementation of Backdrops undo/redo commands
AddBackDropCommand::AddBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,QUndoCommand *parent)
: QUndoCommand(parent)
, _graph(graph)
, _bd(bd)
, _isUndone(false)
{
    
}

AddBackDropCommand::~AddBackDropCommand()
{
    if (_isUndone) {
        _bd->setParentItem(NULL);
        _graph->removeBackDrop(_bd);
        delete _bd;
    }
}
    
void AddBackDropCommand::undo()
{
    _bd->setActive(false);
    _bd->setVisible(false);
    _bd->setSettingsPanelClosed(true);
    _isUndone = true;
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Add %1").arg(_bd->getName()));
}

void AddBackDropCommand::redo()
{
    _bd->setActive(true);
    _bd->setVisible(true);
    _isUndone = false;
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Add %1").arg(_bd->getName()));
}


RemoveBackDropCommand::RemoveBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,const std::list<boost::shared_ptr<NodeGui> >& nodes,
                                             QUndoCommand *parent)
: QUndoCommand(parent)
, _graph(graph)
, _bd(bd)
, _nodes(nodes)
, _isRedone(false)
{
    
}

RemoveBackDropCommand::~RemoveBackDropCommand()
{
    if (_isRedone) {
        _bd->setParentItem(NULL);
        _graph->removeBackDrop(_bd);
        delete _bd;
    }
}
    

void RemoveBackDropCommand::undo()
{
    _bd->activate();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
        (*it)->getNode()->activate();
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _isRedone = false;
    setText(QObject::tr("Remove %1").arg(_bd->getName()));
}

void RemoveBackDropCommand::redo()
{
    _bd->deactivate();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
        (*it)->getNode()->deactivate();
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    _isRedone = true;
    setText(QObject::tr("Remove %1").arg(_bd->getName()));
}

MoveBackDropCommand::MoveBackDropCommand(NodeBackDrop* bd,double dx,double dy,
                                         const std::list<boost::shared_ptr<NodeGui> >& nodes,
                                         QUndoCommand *parent )
: QUndoCommand(parent)
, _bd(bd)
, _nodes(nodes)
, _dx(dx)
, _dy(dy)
{
    
}

MoveBackDropCommand::~MoveBackDropCommand()
{
    
}
    

void MoveBackDropCommand::undo()
{
    move(-_dx,-_dy);
    setText(QObject::tr("Move %1").arg(_bd->getName()));
}

void MoveBackDropCommand::move(double dx,double dy)
{
    QPointF delta(dx,dy);
    QPointF bdPos = _bd->pos();
    _bd->setPos_mt_safe(bdPos + delta);
    
    ///Also move all the nodes
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
        QPointF nodePos = (*it)->pos();
        nodePos += delta;
        (*it)->refreshPosition(nodePos.x(), nodePos.y(),true);
    }
    

}

void MoveBackDropCommand::redo()
{
    move(_dx,_dy);
    setText(QObject::tr("Move %1").arg(_bd->getName()));
}

bool MoveBackDropCommand::mergeWith(const QUndoCommand *command)
{
    const MoveBackDropCommand* mvCmd = dynamic_cast<const MoveBackDropCommand*>(command);
    if (!mvCmd) {
        return false;
    }
    if (mvCmd->_bd != _bd || mvCmd->_nodes.size() != _nodes.size()) {
        return false;
    }
    
    ///check nodes
    std::list<boost::shared_ptr<NodeGui> >::const_iterator itOther = mvCmd->_nodes.begin();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it,++itOther) {
        if (*itOther != *it) {
            return false;
        }
    }
    _dx += mvCmd->_dx;
    _dy += mvCmd->_dy;
    return true;
}


ResizeBackDropCommand::ResizeBackDropCommand(NodeBackDrop* bd,int w,int h,QUndoCommand *parent)
: QUndoCommand(parent)
, _bd(bd)
, _w(w)
, _h(h)
, _oldW(0)
, _oldH(0)
{
    QRectF bbox = bd->boundingRect();
    _oldW = bbox.width();
    _oldH = bbox.height();
}

ResizeBackDropCommand::~ResizeBackDropCommand()
{
    
}

void ResizeBackDropCommand::undo()
{
    _bd->resize(_oldW, _oldH);
    setText(QObject::tr("Resize %1").arg(_bd->getName()));
}

void ResizeBackDropCommand::redo()
{
    _bd->resize(_w, _h);
    setText(QObject::tr("Resize %1").arg(_bd->getName()));
}

bool ResizeBackDropCommand::mergeWith(const QUndoCommand *command)
{
    const ResizeBackDropCommand* rCmd = dynamic_cast<const ResizeBackDropCommand*>(command);
    if (!rCmd) {
        return false;
    }
    if (rCmd->_bd != _bd) {
        return false;
    }
    _w = rCmd->_w;
    _h = rCmd->_h;
    return true;
}


DuplicateBackDropCommand::DuplicateBackDropCommand(NodeGraph* graph,NodeBackDrop* bd,
                                                   const std::list<boost::shared_ptr<NodeGui> >& nodes,QUndoCommand *parent)
: QUndoCommand(parent)
, _graph(graph)
, _bd(bd)
, _nodes(nodes)
, _isUndone(false)
, _firstRedoCalled(false)
{
    
}

DuplicateBackDropCommand::~DuplicateBackDropCommand()
{
    if (_isUndone) {
        _bd->setParentItem(NULL);
        _graph->removeBackDrop(_bd);
        delete _bd;
    }
}

void DuplicateBackDropCommand::undo()
{
    _bd->deactivate();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
        (*it)->getNode()->deactivate();
    }
    _isUndone = true;
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Duplicate %1").arg(_bd->getName()));
}

void DuplicateBackDropCommand::redo()
{
    if (_firstRedoCalled) {
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it!= _nodes.end();++it) {
            (*it)->getNode()->activate();
        }
        _bd->activate();
    }
    _isUndone = false;
    _graph->getGui()->getApp()->triggerAutoSave();
    setText(QObject::tr("Duplicate %1").arg(_bd->getName()));
}


std::list<boost::shared_ptr<NodeGui> > NodeGraph::getNodesWithinBackDrop(const NodeBackDrop* bd) const
{
    QRectF bbox = bd->mapToScene(bd->boundingRect()).boundingRect();
    std::list<boost::shared_ptr<NodeGui> > ret;
    QMutexLocker l(&_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _nodes.begin(); it != _nodes.end();++it) {
        QRectF nodeBbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
        if (bbox.contains(nodeBbox)) {
            ret.push_back(*it);
        }
    }
    return ret;
}

void NodeGraph::insertNewBackDrop(NodeBackDrop* bd)
{
    _backdrops.push_back(bd);
}

void NodeGraph::removeBackDrop(NodeBackDrop* bd)
{
    std::list<NodeBackDrop*>::iterator it = std::find(_backdrops.begin(),_backdrops.end(),bd);
    if (it != _backdrops.end()) {
        _backdrops.erase(it);
    }
}

