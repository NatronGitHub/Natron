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
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QGraphicsProxyWidget>
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
#include <QDropEvent>

#include "Gui/TabWidget.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/DockablePanel.h"

#include "Gui/KnobGui.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Gui/Gui.h"
#include "Gui/TimeLineGui.h"
#include "Gui/SequenceFileDialog.h"

#include "Engine/VideoEngine.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Hash64.h"
#include "Engine/FrameEntry.h"
#include "Engine/Settings.h"

#include "Readers/Reader.h"

#include "Global/AppManager.h"


#define POWITER_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS 1000

using namespace Natron;
using std::cout; using std::endl;

NodeGraph::NodeGraph(Gui* gui,QGraphicsScene* scene,QWidget *parent):
QGraphicsView(scene,parent),
_gui(gui),
_evtState(DEFAULT),
_nodeSelected(0),
_maximized(false),
_propertyBin(0),
_refreshOverlays(true),
_previewsTurnedOff(false)
{
    setAcceptDrops(true);
    
    QObject::connect(_gui->getApp(), SIGNAL(pluginsPopulated()), this, SLOT(populateMenu()));
    
    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    scale(qreal(0.8), qreal(0.8));
    setDragMode(QGraphicsView::ScrollHandDrag);
    
    smartNodeCreationEnabled=true;
    _root = new QGraphicsTextItem(0);
    // _root->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_root);
    _navigator = new NodeGraphNavigator();
    _navigatorProxy = new QGraphicsProxyWidget(0);
    _navigatorProxy->setWidget(_navigator);
    // scene->addItem(_navigatorProxy);
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
    _refreshCacheTextTimer.start(POWITER_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS);
    
    _undoStack = new QUndoStack(this);
    
    _undoAction = _undoStack->createUndoAction(this,tr("&Undo"));
    _undoAction->setShortcuts(QKeySequence::Undo);
    _redoAction = _undoStack->createRedoAction(this,tr("&Redo"));
    _redoAction->setShortcuts(QKeySequence::Redo);
    
    _gui->addUndoRedoActions(_undoAction, _redoAction);
    
    
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

NodeGraph::~NodeGraph(){
    _nodeCreationShortcutEnabled = false;
    _nodes.clear();
    _nodesTrash.clear();
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

NodeGui* NodeGraph::createNodeGUI(QVBoxLayout *dockContainer, Natron::Node *node){
    QPointF selectedPos;
    QRectF viewPos = visibleRect();
    double x,y;
    
    if(_nodeSelected){
        selectedPos = _nodeSelected->scenePos();
        int yOffset = 0;
        int xOffset = 0;
        if(node->makePreviewByDefault() && !_nodeSelected->getNode()->makePreviewByDefault()){
            xOffset -= NodeGui::PREVIEW_LENGTH/2;
            yOffset -= NodeGui::PREVIEW_HEIGHT;
        }
        if(node->isInputNode() || _nodeSelected->getNode()->isOutputNode()){
            yOffset -=  (NodeGui::NODE_HEIGHT + 20);
        }else{
            yOffset +=  (NodeGui::NODE_HEIGHT + 20);
        }
        if(_nodeSelected->getNode()->isInputNode() && node->isInputNode())
            xOffset -= (NodeGui::NODE_LENGTH + 20);
        
        y = selectedPos.y() + yOffset;
        x = selectedPos.x() + xOffset;
    }else{
        x = (viewPos.bottomRight().x()+viewPos.topLeft().x())/2.;
        y = (viewPos.topLeft().y()+viewPos.bottomRight().y())/2.;
    }
    
    NodeGui* node_ui = new NodeGui(this,dockContainer,node,x,y,_root);
    _nodes.push_back(node_ui);
    _undoStack->push(new AddCommand(this,node_ui));
    _evtState = DEFAULT;
    return node_ui;
    
}
void NodeGraph::mousePressEvent(QMouseEvent *event) {
    
    assert(event);
    if(event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier)) {
        _evtState = MOVING_AREA;
        QGraphicsView::mousePressEvent(event);
        return;
    }
    
    _lastScenePosClick = mapToScene(event->pos());
    
    NodeGui* selected = 0;
    Edge* selectedEdge = 0;
    for(U32 i = 0;i < _nodes.size();++i){
        NodeGui* n = _nodes[i];
        QPointF evpt = n->mapFromScene(_lastScenePosClick);
        if(n->isActive() && n->contains(evpt)){
            selected = n;
            break;
        }else{
            Edge* edge = n->hasEdgeNearbyPoint(_lastScenePosClick);
            if(edge){
                selectedEdge = edge;
                break;
            }
        }
    }
    if(!selected && !selectedEdge){
        if(event->button() == Qt::RightButton){
            showMenu(mapToGlobal(event->pos()));
        }else if(event->button() == Qt::LeftButton){
            deselect();
        }else if(event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier)){
            _evtState = MOVING_AREA;
            QGraphicsView::mousePressEvent(event);
        }
        return;
    }
    
    if(selected){
        selectNode(selected);
        if(event->button() == Qt::LeftButton){
            _evtState = NODE_DRAGGING;
            _lastNodeDragStartPoint = selected->pos();
        }
        else if(event->button() == Qt::RightButton){
            selected->showMenu(mapToGlobal(event->pos()));
        }
    }else if(selectedEdge){
        _arrowSelected = selectedEdge;
        _evtState = ARROW_DRAGGING;
    }
}

void NodeGraph::deselect(){
    for(U32 i = 0 ; i < _nodes.size() ;++i) {
        NodeGui* n = _nodes[i];
        n->setSelected(false);
    }
    _nodeSelected = 0;
}

void NodeGraph::mouseReleaseEvent(QMouseEvent *event){
    if(_evtState == ARROW_DRAGGING){
        bool foundSrc=false;
        NodeGui* dst = _arrowSelected->getDest();
        for(U32 i = 0; i<_nodes.size() ;++i){
            NodeGui* n=_nodes[i];
            QPointF ep = mapToScene(event->pos());
            QPointF evpt = n->mapFromScene(ep);
            
            if(n->isActive() && n->isNearby(evpt) &&
               (n->getNode()->getName()!=_arrowSelected->getDest()->getNode()->getName())){
                if(n->getNode()->isOutputNode()){
                    break;
                }
                _undoStack->push(new ConnectCommand(this,_arrowSelected,_arrowSelected->getSource(),n));
                foundSrc=true;
                
                break;
            }
        }
        if(!foundSrc){
            _undoStack->push(new ConnectCommand(this,_arrowSelected,_arrowSelected->getSource(),NULL));
            scene()->update();
        }
        dst->refreshEdges();
        scene()->update();
        appPTR->clearPlaybackCache();
        _gui->getApp()->checkViewersConnection();
    }else if(_evtState == NODE_DRAGGING){
        if(_nodeSelected)
            _undoStack->push(new MoveCommand(_nodeSelected,_lastNodeDragStartPoint));
    }else if(_evtState == MOVING_AREA){
        QGraphicsView::mouseReleaseEvent(event);
    }
    scene()->update();
    
    _evtState=DEFAULT;
}
void NodeGraph::mouseMoveEvent(QMouseEvent *event){
    QPointF newPos = mapToScene(event->pos());
    
    if(_evtState == ARROW_DRAGGING){
        
        QPointF np=_arrowSelected->mapFromScene(newPos);
        _arrowSelected->updatePosition(np);
        
    }else if(_evtState == NODE_DRAGGING && _nodeSelected){
        
        QPointF op = _nodeSelected->mapFromScene(_lastScenePosClick);
        QPointF np = _nodeSelected->mapFromScene(newPos);
        qreal diffx=np.x()-op.x();
        qreal diffy=np.y()-op.y();
        QPointF p = _nodeSelected->pos()+QPointF(diffx,diffy);
        _nodeSelected->refreshPosition(p.x(),p.y());
        
    }else if(_evtState == MOVING_AREA){
        
        double dx = _root->mapFromScene(newPos).x() - _root->mapFromScene(_lastScenePosClick).x();
        double dy = _root->mapFromScene(newPos).y() - _root->mapFromScene(_lastScenePosClick).y();
//        double dx = newPos.x() - _lastScenePosClick.x();
//        double dy = newPos.y() - _lastScenePosClick.y();
        _root->moveBy(dx, dy);
//        QGraphicsView::mouseMoveEvent(event);
//        translate(dx, dy);
       
    }
    _lastScenePosClick = newPos;
    
    /*Now update navigator*/
    //updateNavigator();
}


void NodeGraph::mouseDoubleClickEvent(QMouseEvent *){
    U32 i=0;
    while(i<_nodes.size()){
        NodeGui* n=_nodes[i];
        
        QPointF evpt = n->mapFromScene(_lastScenePosClick);
        if(n->isActive() && n->contains(evpt) && n->getSettingPanel()){
            if(!n->isSettingsPanelVisible()){
                n->setVisibleSettingsPanel(true);
            }
            _gui->putSettingsPanelFirst(n->getSettingPanel());
            break;
        }
        ++i;
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
                QPoint position=_gui->_workshopPane->pos();
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
    
    if(e->key() == Qt::Key_R){
        _gui->getApp()->createNode("Reader");
    }else if(e->key() == Qt::Key_W){
        _gui->getApp()->createNode("Writer");
    }else if(e->key() == Qt::Key_Space){
        
        if(!_maximized){
            _maximized = true;
            _gui->maximize(dynamic_cast<TabWidget*>(parentWidget()),false);
        }else{
            _maximized = false;
            _gui->minimize();
        }
        
    }else if(e->key() == Qt::Key_Backspace){
        /*delete current node.*/
        if(_nodeSelected){
            deleteSelectedNode();
        }
    }
}
void NodeGraph::connectCurrentViewerToSelection(int inputNB){
    
    InspectorNode* v = dynamic_cast<InspectorNode*>(_gui->getLastSelectedViewer()->getInternalNode()->getNode());
    if(!_nodeSelected){
        v->setActiveInputAndRefresh(inputNB);
        return;
    }
    NodeGui* gui = _gui->getApp()->getNodeGui(v);
    if(gui){
        NodeGui::InputEdgesMap::const_iterator it = gui->getInputsArrows().find(inputNB);
        while(it == gui->getInputsArrows().end()){
            v->addEmptyInput();
            it = gui->getInputsArrows().find(inputNB);
        }
        _undoStack->push(new ConnectCommand(this,it->second,it->second->getSource(),_nodeSelected));
    }
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
    scale(scaleFactor,scaleFactor);
    _refreshOverlays = true;

}


void NodeGraph::autoConnect(NodeGui* selected,NodeGui* created){
    
    /*Corner cases*/
    if(selected->getNode()->isInputNode() && created->getNode()->isInputNode())
        return;
    if(selected->getNode()->isOutputNode() && created->getNode()->isOutputNode())
        return;
    
    Edge* first = 0;
    if(!selected)
        return;
    /*If the node selected isn't an output node and the node created isn't an output
     node we want to connect the node created to the output of the node selected*/
    if(!selected->getNode()->isOutputNode() && !created->getNode()->isInputNode()){
        
        /*check first if the node selected has outputs and connect the outputs to the new node*/
        if(!created->getNode()->isOutputNode()){
            while(selected->getNode()->hasOutputConnected()){
                Natron::Node* outputNode = selected->getNode()->getOutputs().begin()->second;
                assert(outputNode);
                
                /*Find which edge is connected to the selected node */
                NodeGui* output = _gui->getApp()->getNodeGui(outputNode);
                assert(output);
                const NodeGui::InputEdgesMap& outputEdges = output->getInputsArrows();
                Edge* edgeWithSelectedNode = 0;
                for (NodeGui::InputEdgesMap::const_iterator i = outputEdges.begin();i!=outputEdges.end();++i) {
                    if (i->second->getSource() == selected) {
                        edgeWithSelectedNode = i->second;
                        break;
                    }
                }
                /*Now connecting the selected node's output to the output of the created node.
                 If the node created is an output node we don't connect it's output
                 with the outputs of the selected node since it has no output.*/
                if(edgeWithSelectedNode){
                    InspectorNode* v = dynamic_cast<InspectorNode*>(outputNode);
                    if(v){
                        if(selected){
                            selected->getNode()->disconnectOutput(outputNode);
                        }
                        if(v->connectInput(created->getNode(), edgeWithSelectedNode->getInputNumber(),true)){
                            created->getNode()->connectOutput(v);
                        }
                        
                    }else{
                        _undoStack->push(new ConnectCommand(this,edgeWithSelectedNode,selected,created));
                    }
                    
                    /*we now try to move the created node in between the selected node and its
                     old output.*/
                    QPointF parentPos = created->mapFromScene(selected->scenePos());
                    if(selected->getNode()->makePreviewByDefault()){
                        parentPos.ry() += (NodeGui::NODE_HEIGHT + NodeGui::PREVIEW_HEIGHT);
                    }else{
                        parentPos.ry() += (NodeGui::NODE_HEIGHT);
                    }
                    QPointF childPos = created->mapFromScene(output->scenePos());
                    QPointF newPos = (parentPos + childPos)/2.;
                    QPointF oldPos = created->mapFromScene(created->scenePos());
                    QPointF diff = newPos - oldPos;
                    
                    created->moveBy(diff.x(), diff.y());
                    
                    /*now moving the output node so it is at an appropriate distance (not too close to
                     the created one)*/
                    QPointF childTopLeft = output->scenePos();
                    QPointF createdBottomLeft = created->scenePos()+QPointF(0,created->boundingRect().height());
                    QPointF createdTopLeft = created->scenePos();
                    QPointF parentBottomLeft = selected->scenePos()+QPointF(0,selected->boundingRect().height());
                    
                    double diffY_child_created,diffY_created_parent;
                    diffY_child_created = childTopLeft.y() - createdBottomLeft.y();
                    diffY_created_parent = createdTopLeft.y() - parentBottomLeft.y();
                    
                    double diffX_child_created,diffX_created_parent;
                    diffX_child_created = childTopLeft.x() - createdBottomLeft.x();
                    diffX_created_parent = createdTopLeft.x() - parentBottomLeft.x();
                    
                    output->moveBy(diffX_created_parent-diffX_child_created, diffY_created_parent-diffY_child_created);
                    
                    selected->refreshEdges();
                    created->refreshEdges();
                    refreshAllEdges();
                }else{
                    break;
                }
            }
        }
        first = created->firstAvailableEdge();
        if(first){
            _undoStack->push(new ConnectCommand(this,first,first->getSource(),selected));
        }
    }else{
        /*selected is an output node or the created node is an input node. We want to connect the created node
         as input of the selected node.*/
        first = selected->firstAvailableEdge();
        if(first && !created->getNode()->isOutputNode()){
            _undoStack->push(new ConnectCommand(this,first,first->getSource(),created));
        }
    }
}

void NodeGraph::deleteSelectedNode(){
    
    _undoStack->push(new RemoveCommand(this,_nodeSelected));
    _nodeSelected = 0;
    
}


void NodeGraph::removeNode(NodeGui* n) {
    assert(n);
    if(_nodeSelected == n)
        _nodeSelected = NULL;
    std::vector<NodeGui*>::iterator it = std::find(_nodes.begin(), _nodes.end(), n);
    if (it != _nodes.end()) {
        _nodes.erase(it);
    }
    it = std::find(_nodesTrash.begin(), _nodesTrash.end(), n);
    if (it != _nodesTrash.end()) {
        _nodesTrash.erase(it);
    }
}

void NodeGraph::selectNode(NodeGui* n) {
    assert(n);
    _nodeSelected = n;
    /*now remove previously selected node*/
    for(U32 i = 0 ; i < _nodes.size() ;++i) {
        _nodes[i]->setSelected(false);
    }
    if(n->getNode()->className() == "Viewer"){
        _gui->setLastSelectedViewer(dynamic_cast<ViewerInstance*>(n->getNode()->getLiveInstance())->getUiContext());
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
    for (U32 i = 0; i < _nodes.size(); ++i) {
//        QRectF itemSceneRect = _nodes[i]->mapRectFromScene(rect);
//        if(!itemSceneRect.contains(_nodes[i]->boundingRect()))
        if(!rect.contains(_nodes[i]->boundingRectWithEdges()))
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
bool NodeGraph::isGraphWorthLess() const{
    bool worthLess = true;
    for (U32 i = 0; i < _nodes.size(); ++i) {
        if (!_nodes[i]->getNode()->isOutputNode()) {
            worthLess = false;
            break;
        }
    }
    return worthLess;
}

NodeGraph::NodeGraphNavigator::NodeGraphNavigator(QWidget* parent ):QLabel(parent),
_w(120),_h(70){
    
}

void NodeGraph::NodeGraphNavigator::setImage(const QImage& img){
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(_w, _h);
    setPixmap(pix);
}

const std::vector<NodeGui*>& NodeGraph::getAllActiveNodes() const{
    return _nodes;
}
void NodeGraph::moveToTrash(NodeGui* node) {
    assert(node);
    std::vector<NodeGui*>::iterator it = std::find(_nodes.begin(), _nodes.end(), node);
    if (it != _nodes.end()) {
        _nodesTrash.push_back(*it);
        _nodes.erase(it);
    }
}

void NodeGraph::restoreFromTrash(NodeGui* node) {
    assert(node);
    std::vector<NodeGui*>::iterator it = std::find(_nodesTrash.begin(), _nodesTrash.end(), node);
    if (it != _nodesTrash.end()) {
        _nodes.push_back(*it);
        _nodesTrash.erase(it);
    }
}


MoveCommand::MoveCommand(NodeGui *node, const QPointF &oldPos,
                         QUndoCommand *parent):QUndoCommand(parent),
_node(node),
_oldPos(oldPos),
_newPos(node->pos()){
    
}
void MoveCommand::undo(){
    _node->setPos(_oldPos);
    
    _node->refreshEdges();
    
    if(_node->scene())
        _node->scene()->update();
    setText(QObject::tr("Move %1")
            .arg(_node->getNode()->getName().c_str()));
}
void MoveCommand::redo(){
    _node->setPos(_newPos);
    _node->refreshEdges();
    setText(QObject::tr("Move %1")
            .arg(_node->getNode()->getName().c_str()));
}
bool MoveCommand::mergeWith(const QUndoCommand *command){
    const MoveCommand *moveCommand = static_cast<const MoveCommand *>(command);
    NodeGui* node = moveCommand->_node;
    if(_node != node)
        return false;
    _newPos = node->pos();
    setText(QObject::tr("Move %1")
            .arg(node->getNode()->getName().c_str()));
    return true;
}


AddCommand::AddCommand(NodeGraph* graph,NodeGui *node,QUndoCommand *parent):QUndoCommand(parent),
_node(node),_graph(graph),_undoWasCalled(false){
    
}
void AddCommand::undo(){
    _undoWasCalled = true;
    
    
    
    _inputs = _node->getNode()->getInputs();
    _outputs = _node->getNode()->getOutputs();
    
    QMutexLocker l(_graph->getGui()->getApp()->getAutoSaveMutex());
    _node->getNode()->deactivate();
    
    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNode()->getName().c_str()));
    
}
void AddCommand::redo(){
    if(_undoWasCalled){
        QMutexLocker l(_graph->getGui()->getApp()->getAutoSaveMutex());
        _node->getNode()->activate();
    }
    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNode()->getName().c_str()));
    
    
}

RemoveCommand::RemoveCommand(NodeGraph* graph,NodeGui *node,QUndoCommand *parent):QUndoCommand(parent),
_node(node),_graph(graph){
    
}
void RemoveCommand::undo(){
    QMutexLocker l(_graph->getGui()->getApp()->getAutoSaveMutex());
    _node->getNode()->activate();
    
    _graph->scene()->update();
    setText(QObject::tr("Remove %1")
            .arg(_node->getNode()->getName().c_str()));
    
    
    
}
void RemoveCommand::redo(){
    _inputs = _node->getNode()->getInputs();
    _outputs = _node->getNode()->getOutputs();
    
    QMutexLocker l(_graph->getGui()->getApp()->getAutoSaveMutex());
    _node->getNode()->deactivate();
    
    _graph->scene()->update();
    setText(QObject::tr("Remove %1")
            .arg(_node->getNode()->getName().c_str()));
    
}


ConnectCommand::ConnectCommand(NodeGraph* graph,Edge* edge,NodeGui *oldSrc,NodeGui* newSrc,QUndoCommand *parent):QUndoCommand(parent),
_edge(edge),
_oldSrc(oldSrc),
_newSrc(newSrc),
_graph(graph){
    
}

void ConnectCommand::undo(){
    QMutexLocker l(_graph->getGui()->getApp()->getAutoSaveMutex());
    _edge->setSource(_oldSrc);
    
    if(_oldSrc){
        _graph->getGui()->getApp()->connect(_edge->getInputNumber(), _oldSrc->getNode(), _edge->getDest()->getNode());
    }
    if(_newSrc){
        _graph->getGui()->getApp()->disconnect(_newSrc->getNode(), _edge->getDest()->getNode());
    }
    
    if(_oldSrc){
        setText(QObject::tr("Connect %1 to %2")
                .arg(_edge->getDest()->getNode()->getName().c_str()).arg(_oldSrc->getNode()->getName().c_str()));
    }else{
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNode()->getName().c_str()));
    }
    
    _graph->getGui()->getApp()->triggerAutoSave();
    std::list<ViewerInstance*> viewers;
    _edge->getDest()->getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->updateTreeAndRender();
    }
}
void ConnectCommand::redo(){
    QMutexLocker l(_graph->getGui()->getApp()->getAutoSaveMutex());
    NodeGui* dst = _edge->getDest();
    
    InspectorNode* inspector = dynamic_cast<InspectorNode*>(dst->getNode());
    if(inspector){
        if(!_newSrc){
            inspector->disconnectInput(_edge->getInputNumber());
        }else{
            if(inspector->connectInput(_newSrc->getNode(), _edge->getInputNumber(),false)){
                _edge->setSource(_newSrc);
                _newSrc->getNode()->connectOutput(inspector);
                
            }
        }
    }else{
        _edge->setSource(_newSrc);
        if(_oldSrc){
            if(!_graph->getGui()->getApp()->disconnect(_oldSrc->getNode(), dst->getNode())){
                cout << "Failed to disconnect (input) " << _oldSrc->getNode()->getName()
                << " to (output) " << dst->getNode()->getName() << endl;
            }
        }
        if(_newSrc){
            if(!_graph->getGui()->getApp()->connect(_edge->getInputNumber(), _newSrc->getNode(), dst->getNode())){
                cout << "Failed to connect (input) " << _newSrc->getNode()->getName()
                << " to (output) " << dst->getNode()->getName() << endl;
            }
        }
    }
    dst->refreshEdges();
    
    if(_newSrc){
        setText(QObject::tr("Connect %1 to %2")
                .arg(_edge->getDest()->getNode()->getName().c_str()).arg(_newSrc->getNode()->getName().c_str()));
        std::list<ViewerInstance*> viewers;
        _edge->getDest()->getNode()->hasViewersConnected(&viewers);
        for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
            (*it)->updateTreeAndRender();
        }
    }else{
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNode()->getName().c_str()));
    }
    _graph->getGui()->getApp()->triggerAutoSave();
    
    
    
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
    textEdit->setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(textEdit->lineEdit());
    textEdit->lineEdit()->setFocus(Qt::ActiveWindowFocusReason);
    textEdit->setFocus(); // textEdit->grabKeyboard();
    installEventFilter(this);
    
    
}
void SmartInputDialog::keyPressEvent(QKeyEvent *e){
    if(e->key() == Qt::Key_Return){
        QString res=textEdit->lineEdit()->text();
        if(appPTR->getNodeNameList().contains(res)){
            graph->getGui()->getApp()->createNode(res);
            graph->setSmartNodeCreationEnabled(true);
            graph->setMouseTracking(true);
            //textEdit->releaseKeyboard();
            
            graph->setFocus(Qt::ActiveWindowFocusReason);
            delete this;
            
            
        }else{
            
        }
    }else if(e->key()== Qt::Key_Escape){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        //textEdit->releaseKeyboard();
        
        graph->setFocus(Qt::ActiveWindowFocusReason);
        
        
        delete this;
        
        
    }
}
bool SmartInputDialog::eventFilter(QObject *obj, QEvent *e){
    Q_UNUSED(obj);
    
    if(e->type()==QEvent::Close){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        //textEdit->releaseKeyboard();
        
        graph->setFocus(Qt::ActiveWindowFocusReason);
        
        
    }
    return false;
}
void NodeGraph::refreshAllEdges(){
    for (U32 i=0; i < _nodes.size(); ++i) {
        _nodes[i]->refreshEdges();
    }
}
// grabbed from QDirModelPrivate::size() in qtbase/src/widgets/itemviews/qdirmodel.cpp
static QString QDirModelPrivate_size(quint64 bytes)
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


void NodeGraph::updateCacheSizeText(){
    _cacheSizeText->setPlainText(QString("Memory cache size: %1")
                                 .arg(QDirModelPrivate_size(appPTR->getViewerCache().getMemoryCacheSize()
                                                            + appPTR->getNodeCache().getMemoryCacheSize())));
}
QRectF NodeGraph::calcNodesBoundingRect(){
    QRectF ret;
    for (U32 i = 0; i < _nodes.size(); ++i) {
        ret = ret.united(_nodes[i]->boundingRectWithEdges());
    }
    return ret;
}
void NodeGraph::toggleCacheInfos(){
    if(_cacheSizeText->isVisible()){
        _cacheSizeText->hide();
    }else{
        _cacheSizeText->show();
    }
}
void NodeGraph::populateMenu(){
    _menu->clear();
    QAction* displayCacheInfoAction = new QAction("Display memory consumption",this);
    displayCacheInfoAction->setCheckable(true);
    displayCacheInfoAction->setChecked(true);
    QObject::connect(displayCacheInfoAction,SIGNAL(triggered()),this,SLOT(toggleCacheInfos()));
    _menu->addAction(displayCacheInfoAction);
    
    QAction* turnOffPreviewAction = new QAction("Turn off all previews",this);
    turnOffPreviewAction->setCheckable(true);
    turnOffPreviewAction->setChecked(false);
    QObject::connect(turnOffPreviewAction,SIGNAL(triggered()),this,SLOT(turnOffPreviewForAllNodes()));
    _menu->addAction(turnOffPreviewAction);
    
    const std::vector<ToolButton*>& toolButtons = _gui->getToolButtons();
    for(U32 i = 0; i < toolButtons.size();++i){
        //if the toolbutton is a root (no parent), add it in the toolbox
        if(toolButtons[i]->hasChildren() && !toolButtons[i]->getPluginToolButton()->hasParent()){
            toolButtons[i]->getMenu()->setIcon(toolButtons[i]->getIcon());
            _menu->addAction(toolButtons[i]->getMenu()->menuAction());
        }

    }
    
}

void NodeGraph::showMenu(const QPoint& pos){
    _menu->exec(pos);
}



void NodeGraph::dropEvent(QDropEvent* event){
    if(!event->mimeData()->hasUrls())
        return;
    
    QStringList filesList;
    QList<QUrl> urls = event->mimeData()->urls();
    for(int i = 0; i < urls.size() ; ++i){
        const QUrl& rl = urls.at(i);
        QString path = rl.path();
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
    std::vector<std::string> supportedFileTypes = appPTR->getCurrentSettings()._readersSettings.supportedFileTypes();
    for(U32 i = 0 ; i < supportedFileTypes.size();++i){
        supportedExtensions.append(QString(supportedFileTypes[i].c_str()));
    }
    
    std::vector<QStringList> files = SequenceFileDialog::fileSequencesFromFilesList(filesList,supportedExtensions);
    
    for(U32 i = 0 ; i < files.size();++i){
        
        QStringList list = files[i];
        Natron::Node* reader = _gui->getApp()->createNode("Reader",true);
        const std::vector<Knob*>& knobs = reader->getKnobs();
        for(U32 j = 0 ; j < knobs.size();++j){
            if(knobs[j]->typeName() == "InputFile"){
                File_Knob* fileKnob = dynamic_cast<File_Knob*>(knobs[j]);
                assert(fileKnob);
                fileKnob->setValue(files[i]);
                reader->refreshPreviewImage(fileKnob->firstFrame());
                break;
            }
        }
    }
    
}

void NodeGraph::dragEnterEvent(QDragEnterEvent *ev){
    ev->accept();
}
void NodeGraph::dragLeaveEvent(QDragLeaveEvent* e){
    e->accept();
}
void NodeGraph::dragMoveEvent(QDragMoveEvent* e){
    e->accept();
}

void NodeGraph::turnOffPreviewForAllNodes(){
    _previewsTurnedOff = !_previewsTurnedOff;
    if(_previewsTurnedOff){
        for(U32 i = 0; i < _nodes.size() ; ++i){
            if(_nodes[i]->getNode()->isPreviewEnabled()){
                _nodes[i]->togglePreview();
            }
        }
    }else{
        for(U32 i = 0; i < _nodes.size() ; ++i){
            if(!_nodes[i]->getNode()->isPreviewEnabled() && _nodes[i]->getNode()->makePreviewByDefault()){
                _nodes[i]->togglePreview();
            }
        }
    }
}