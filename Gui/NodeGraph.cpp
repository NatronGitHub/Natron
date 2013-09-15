//  Powiter
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
#include <QScrollArea>
#include <QScrollBar>
#include <QComboBox>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QGraphicsLineItem>
#include <QUndoStack>

#include "Gui/TabWidget.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/SettingsPanel.h"

#include "Gui/KnobGui.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Gui/Gui.h"
#include "Gui/Timeline.h"

#include "Engine/VideoEngine.h"
#include "Engine/OfxNode.h"
#include "Engine/ViewerNode.h"
#include "Engine/Model.h"
#include "Engine/ViewerNode.h"
#include "Engine/Hash64.h"

#include "Readers/Reader.h"


#include "Global/AppManager.h"


using namespace std;
using namespace Powiter;

NodeGraph::NodeGraph(Gui* gui,QGraphicsScene* scene,QWidget *parent):
QGraphicsView(scene,parent),
_gui(gui),
_evtState(DEFAULT),
_nodeSelected(0),
_maximized(false),
_propertyBin(0)
{
    
    setObjectName("DAG_GUI");
    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    
    scale(qreal(0.8), qreal(0.8));
    
    smartNodeCreationEnabled=true;
    _root = new QGraphicsLineItem(0);
    scene->addItem(_root);
    oldZoom = QPointF(0,0);
    // _navigator = new NodeGraphNavigator();
    //  _navigatorProxy = scene->addWidget(_navigator);
    //_navigatorProxy->hide();
    _undoStack = new QUndoStack(this);
    
    _undoAction = _undoStack->createUndoAction(this,tr("&Undo"));
    _undoAction->setShortcuts(QKeySequence::Undo);
    _redoAction = _undoStack->createRedoAction(this,tr("&Redo"));
    _redoAction->setShortcuts(QKeySequence::Redo);
    
    _gui->addUndoRedoActions(_undoAction, _redoAction);
}

NodeGraph::~NodeGraph(){
    _nodeCreationShortcutEnabled=false;
    _nodes.clear();
    _nodesTrash.clear();
    //delete _navigator;
}

QRectF NodeGraph::visibleRect() {
    QPointF tl(horizontalScrollBar()->value(), verticalScrollBar()->value());
    QPointF br = tl + viewport()->rect().bottomRight();
    QMatrix mat = matrix().inverted();
    return mat.mapRect(QRectF(tl,br));
}
QRectF NodeGraph::visibleRect_v2(){
    return mapToScene(viewport()->rect()).boundingRect();
}
NodeGui* NodeGraph::createNodeGUI(QVBoxLayout *dockContainer, Node *node){
    QGraphicsScene* sc=scene();
    QPointF selectedPos;
    QRectF viewPos = visibleRect();
    double x,y;
    
    if(_nodeSelected){
        selectedPos = _nodeSelected->scenePos();
        x = selectedPos.x();
        int yOffset = 0;
        if(node->isInputNode() && !_nodeSelected->getNode()->isInputNode()){
            x -= NodeGui::PREVIEW_LENGTH/2;
            yOffset -= NodeGui::PREVIEW_HEIGHT;
        }
        
        if(!node->isOutputNode()){
            yOffset -= (NodeGui::NODE_HEIGHT + 50);
        }else {
            yOffset += (NodeGui::NODE_HEIGHT + 50);
        }
        y =  selectedPos.y() + yOffset;
    }else{
        x = (viewPos.bottomRight().x()-viewPos.topLeft().x())/2.;
        y = (viewPos.bottomRight().y()-viewPos.topLeft().y())/2.;
    }
    
    NodeGui* node_ui = new NodeGui(this,dockContainer,node,x,y,_root,sc);
    _nodes.push_back(node_ui);
    _undoStack->push(new AddCommand(this,node_ui));
    _evtState = DEFAULT;
    return node_ui;
    
}
void NodeGraph::mousePressEvent(QMouseEvent *event){
    old_pos=mapToScene(event->pos());
    oldp=event->pos();
    for(U32 i = 0;i<_nodes.size();++i){
        NodeGui* n=_nodes[i];
        
        QPointF evpt=n->mapFromScene(old_pos);
        if(n->isActive() && n->contains(evpt)){
            
            selectNode(n);
            
            _evtState=NODE_DRAGGING;
            _lastNodeDragStartPoint = n->pos();
            break;
        }else{
            Edge* edge = n->hasEdgeNearbyPoint(old_pos);
            if(edge){
                _arrowSelected = edge;
                _evtState=ARROW_DRAGGING;
                break;
            }else{
                deselect();
                _evtState=MOVING_AREA;
            }
            
        }
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
    if(_evtState==ARROW_DRAGGING){
        bool foundSrc=false;
        for(U32 i = 0; i<_nodes.size() ;++i){
            NodeGui* n=_nodes[i];
            QPointF ep = mapToScene(event->pos());
            QPointF evpt = n->mapFromScene(ep);
            
            if(n->isActive() && n->isNearby(evpt) &&
               (n->getNode()->getName()!=_arrowSelected->getDest()->getNode()->getName())){
                if(n->getNode()->isOutputNode() && _arrowSelected->getDest()->getNode()->isOutputNode()){
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
        _arrowSelected->initLine();
        scene()->update();
        _gui->getApp()->clearPlaybackCache();
        _gui->getApp()->checkViewersConnection();
    }else if(_evtState == NODE_DRAGGING){
        if(_nodeSelected)
            _undoStack->push(new MoveCommand(_nodeSelected,_lastNodeDragStartPoint));
    }
    scene()->update();
    
    _evtState=DEFAULT;
}
void NodeGraph::mouseMoveEvent(QMouseEvent *event){
    QPointF newPos=mapToScene(event->pos());
    if(_evtState==ARROW_DRAGGING){
        QPointF np=_arrowSelected->mapFromScene(newPos);
        _arrowSelected->updatePosition(np);
    }else if(_evtState==NODE_DRAGGING && _nodeSelected){
        QPointF op=_nodeSelected->mapFromScene(old_pos);
        QPointF np=_nodeSelected->mapFromScene(newPos);
        qreal diffx=np.x()-op.x();
        qreal diffy=np.y()-op.y();
        QPointF p = _nodeSelected->pos()+QPointF(diffx,diffy);
        _nodeSelected->refreshPosition(p.x(),p.y());
    }else if(_evtState==MOVING_AREA){
        double dx = _root->mapFromScene(newPos).x() - _root->mapFromScene(old_pos).x();
        double dy = _root->mapFromScene(newPos).y() - _root->mapFromScene(old_pos).y();
        _root->moveBy(dx, dy);
    }
    old_pos=newPos;
    oldp=event->pos();
    
    /*Now update navigator*/
    //  autoResizeScene();
    //     updateNavigator();
}


void NodeGraph::mouseDoubleClickEvent(QMouseEvent *){
    U32 i=0;
    while(i<_nodes.size()){
        NodeGui* n=_nodes[i];
        
        QPointF evpt = n->mapFromScene(old_pos);
        if(n->isActive() && n->contains(evpt) && n->getNode()->className() != "Viewer"){
            if(!n->isThisPanelEnabled()){
                n->setSettingsPanelEnabled(true);
                n->getSettingPanel()->setVisible(true);                
            }
            n->putSettingsPanelFirst();
            /*scrolling back to the top the selected node's property tab is visible*/
            _propertyBin->verticalScrollBar()->setValue(0);
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
                releaseKeyboard();
                QPoint global = mapToGlobal(oldp.toPoint());
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
        Node* reader = _gui->getApp()->createNode("Reader");
        const std::vector<Knob*>& knobs = reader->getKnobs();
        foreach(Knob* k,knobs){
            if(k->name() == "InputFile"){
                File_Knob* fk = dynamic_cast<File_Knob*>(k);
                fk->openFile();
                break;
            }
        }
        
    }else if(e->key() == Qt::Key_W){
        Node* writer = _gui->getApp()->createNode("Writer");
        const std::vector<Knob*>& knobs = writer->getKnobs();
        foreach(Knob* k,knobs){
            if(k->name() == "OutputFile"){
                OutputFile_Knob* fk = dynamic_cast<OutputFile_Knob*>(k);
                fk->openFile();
                break;
            }
        }
    }else if(e->key() == Qt::Key_Space){
        
        if(!_maximized){
            _maximized = true;
            _gui->maximize(dynamic_cast<TabWidget*>(parentWidget()));
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
    if(!_nodeSelected)
        return;
    ViewerNode* v = _gui->getLastSelectedViewer()->getInternalNode();
    NodeGui* gui = _gui->getApp()->getNodeGui(v);
    if(gui){
        NodeGui::InputEdgesMap::const_iterator it = gui->getInputsArrows().find(inputNB);
        while(it==gui->getInputsArrows().end()){
            v->tryAddEmptyInput();
            it = gui->getInputsArrows().find(inputNB);
        }
        _undoStack->push(new ConnectCommand(this,it->second,it->second->getSource(),_nodeSelected));
    }
}

void NodeGraph::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);
    if(smartNodeCreationEnabled){
        
        _nodeCreationShortcutEnabled=true;
        setFocus();
        grabKeyboard();
    }
}
void NodeGraph::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    if(smartNodeCreationEnabled){
        _nodeCreationShortcutEnabled=false;
        setFocus();
        releaseKeyboard();
    }
}


void NodeGraph::wheelEvent(QWheelEvent *event){
    scaleView(pow((double)2, event->delta() / 240.0), mapToScene(event->pos()));
    //   updateNavigator();
}



void NodeGraph::scaleView(qreal scaleFactor,QPointF){
    
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;
    
    scale(scaleFactor,scaleFactor);
    
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
    bool cont = false;
    /*If the node selected isn't an output node and the node created isn't an output
     node we want to connect the node created to the output of the node selected*/
    if(!selected->getNode()->isOutputNode() && !created->getNode()->isInputNode()){
        
        /*check first if the node selected has outputs and connect the outputs to the new node*/
        if(!created->getNode()->isOutputNode()){
            while(selected->getNode()->hasOutputConnected()){
                Node* outputNode = selected->getNode()->getOutputs().begin()->second;
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
                    if(outputNode->className() == "Viewer"){
                        ViewerNode* v = dynamic_cast<ViewerNode*>(outputNode);
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
                    if(selected->getNode()->canMakePreviewImage()){
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
                }
            }
        }
        first = created->firstAvailableEdge();
        if(first){
            _undoStack->push(new ConnectCommand(this,first,first->getSource(),selected));
            cont = true;
        }
    }else{
        /*selected is an output node or the created node is an input node. We want to connect the created node
         as input of the selected node.*/
        first = selected->firstAvailableEdge();
        if(first && !created->getNode()->isOutputNode()){
            _undoStack->push(new ConnectCommand(this,first,first->getSource(),created));
            cont = true;
        }
    }
    
    
    if(cont){
        Node* viewer = Node::hasViewerConnected(first->getDest()->getNode());
        if(viewer){
            _gui->getApp()->updateDAG(dynamic_cast<OutputNode*>(viewer),false);
        }
    }
}

void NodeGraph::deleteSelectedNode(){
    
    _undoStack->push(new RemoveCommand(this,_nodeSelected));
    _nodeSelected = 0;
    
}


void NodeGraph::removeNode(NodeGui* n){
    for(U32 i = 0 ; i < _nodes.size();++i) {
        if (n == _nodes[i]) {
            _nodes.erase(_nodes.begin()+i);
            break;
        }
    }
    for(U32 i = 0 ; i < _nodesTrash.size();++i) {
        if (n == _nodesTrash[i]) {
            _nodesTrash.erase(_nodesTrash.begin()+i);
            break;
        }
    }
}
void NodeGraph::selectNode(NodeGui* n){
    _nodeSelected = n;
    /*now remove previously selected node*/
    for(U32 i = 0 ; i < _nodes.size() ;++i) {
        _nodes[i]->setSelected(false);
    }
    if(n->getNode()->className() == "Viewer"){
        _gui->setLastSelectedViewer(dynamic_cast<ViewerNode*>(n->getNode())->getUiContext());
    }
    n->setSelected(true);
}


void NodeGraph::updateNavigator(){
    if (!areAllNodesVisible()) {
        _navigator->setImage(getFullSceneScreenShot());
        QRectF rect = visibleRect();
        _navigatorProxy->setPos(rect.width()-_navigator->sizeHint().width(),
                                rect.height()-_navigator->sizeHint().height());
         _navigator->show();
    }else{
        _navigator->hide();
    }
}
bool NodeGraph::areAllNodesVisible(){
    QRectF rect = visibleRect();
    for (U32 i = 0; i < _nodes.size(); ++i) {
        QRectF itemSceneRect = _nodes[i]->mapRectFromScene(rect);
        if(!itemSceneRect.contains(_nodes[i]->boundingRect()))
            return false;
    }
    return true;
}
void NodeGraph::autoResizeScene(){
    QRectF rect(0,0,1,1);
    for (U32 i = 0; i < _nodes.size(); ++i) {
        NodeGui* item = _nodes[i];
        rect = rect.united(item->mapToItem(_root,item->boundingRect()).boundingRect());
    }
    setSceneRect(rect);
}
QImage NodeGraph::getFullSceneScreenShot(){
    QImage img(scene()->width(),scene()->height(), QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(71,71,71,255));
    QRectF viewRect = visibleRect();
    QPainter painter(&img);
    painter.save();
    QPen p;
    p.setColor(Qt::yellow);
    p.setWidth(10);
    painter.setPen(p);
    painter.drawRect(viewRect);
    painter.restore();
    scene()->removeItem(_navigatorProxy);
    scene()->render(&painter);
    scene()->addItem(_navigatorProxy);
    p.setColor(QColor(200,200,200,255));
    p.setWidth(10);
    QRect border(0,0,img.width()-1,img.height()-1);
    painter.setPen(p);
    painter.drawRect(border);
    painter.fillRect(viewRect, QColor(200,200,200,100));
    return img;
}
bool NodeGraph::isGraphWorthLess() const{
    bool worthLess = true;
    for (U32 i = 0; i < _nodes.size(); ++i) {
        if (_nodes[i]->getNode()->className() != "Writer" && _nodes[i]->getNode()->className() != "Viewer") {
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
void NodeGraph::moveToTrash(NodeGui* node){
    for(U32 i = 0; i < _nodes.size() ; ++i) {
        if(node == _nodes[i]){
            _nodesTrash.push_back(_nodes[i]);
            _nodes.erase(_nodes.begin()+i);
            break;
        }
    }
}
void NodeGraph::restoreFromTrash(NodeGui* node){
    for(U32 i = 0; i < _nodesTrash.size() ; ++i) {
        if(node == _nodesTrash[i]){
            _nodes.push_back(_nodesTrash[i]);
            _nodesTrash.erase(_nodesTrash.begin()+i);
            break;
        }
    }
}

void NodeGraph::clear(){
    foreach(NodeGui* n,_nodes){
        scene()->removeItem(n);
        if(n->getSettingPanel())
            n->getSettingPanel()->hide();
        delete n;
    }
    _nodes.clear();
    _nodesTrash.clear();
    _nodeSelected = 0;
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
    
    _node->getNode()->deactivate();

    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNode()->getName().c_str()));
    
}
void AddCommand::redo(){
    if(_undoWasCalled){
       
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
    _node->getNode()->activate();
    
    _graph->scene()->update();
    setText(QObject::tr("Remove %1")
            .arg(_node->getNode()->getName().c_str()));
    
    
    
}
void RemoveCommand::redo(){
    _inputs = _node->getNode()->getInputs();
    _outputs = _node->getNode()->getOutputs();
    
    _node->getNode()->deactivate();

    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNode()->getName().c_str()));
    
}


ConnectCommand::ConnectCommand(NodeGraph* graph,Edge* edge,NodeGui *oldSrc,NodeGui* newSrc,QUndoCommand *parent):QUndoCommand(parent),
_edge(edge),
_oldSrc(oldSrc),
_newSrc(newSrc),
_graph(graph){
    
}

void ConnectCommand::undo(){
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
    _graph->getGui()->getApp()->triggerAutoSaveOnNextEngineRun();
    Node* viewer = Node::hasViewerConnected(_edge->getDest()->getNode());
    if(viewer){
        dynamic_cast<OutputNode*>(viewer)->updateDAG(false);
    }
    
    
}
void ConnectCommand::redo(){
    
    if (_edge->getDest()->getNode()->className() == "Viewer") {
        ViewerNode* v = dynamic_cast<ViewerNode*>(_edge->getDest()->getNode());
        if(!_newSrc){
            v->disconnectInput(_edge->getInputNumber());
        }else{
            if(v->connectInput(_newSrc->getNode(), _edge->getInputNumber(),false)){
                _edge->setSource(_newSrc);
                _newSrc->getNode()->connectOutput(v);
            }
        }
    }else{
        _edge->setSource(_newSrc);
        if(_oldSrc){
            if(!_graph->getGui()->getApp()->disconnect(_oldSrc->getNode(), _edge->getDest()->getNode())){
                cout << "Failed to disconnect (input) " << _oldSrc->getNode()->getName()
                << " to (output) " << _edge->getDest()->getNode()->getName() << endl;
            }
        }
        if(_newSrc){
            if(!_graph->getGui()->getApp()->connect(_edge->getInputNumber(), _newSrc->getNode(), _edge->getDest()->getNode())){
                cout << "Failed to connect (input) " << _newSrc->getNode()->getName()
                << " to (output) " << _edge->getDest()->getNode()->getName() << endl;
            }
        }
    }
    
    _edge->initLine();
    if(_newSrc){
        setText(QObject::tr("Connect %1 to %2")
            .arg(_edge->getDest()->getNode()->getName().c_str()).arg(_newSrc->getNode()->getName().c_str()));
    }else{
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNode()->getName().c_str()));
    }
    _graph->getGui()->getApp()->triggerAutoSaveOnNextEngineRun();
    Node* viewer = Node::hasViewerConnected(_edge->getDest()->getNode());
    if(viewer){
        dynamic_cast<OutputNode*>(viewer)->updateDAG(false);
    }

    
}



SmartInputDialog::SmartInputDialog(NodeGraph* graph):QDialog()
{
    this->graph=graph;
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
    textEdit->grabKeyboard();
    installEventFilter(this);


}
void SmartInputDialog::keyPressEvent(QKeyEvent *e){
    if(e->key() == Qt::Key_Return){
        QString res=textEdit->lineEdit()->text();
        if(appPTR->getNodeNameList().contains(res)){
            graph->getGui()->getApp()->createNode(res);
            graph->setSmartNodeCreationEnabled(true);
            graph->setMouseTracking(true);
            textEdit->releaseKeyboard();

            graph->setFocus(Qt::ActiveWindowFocusReason);
            delete this;


        }else{

        }
    }else if(e->key()== Qt::Key_Escape){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        textEdit->releaseKeyboard();

        graph->setFocus(Qt::ActiveWindowFocusReason);


        delete this;


    }
}
bool SmartInputDialog::eventFilter(QObject *obj, QEvent *e){
    Q_UNUSED(obj);

    if(e->type()==QEvent::Close){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        textEdit->releaseKeyboard();

        graph->setFocus(Qt::ActiveWindowFocusReason);


    }
    return false;
}
void NodeGraph::refreshAllEdges(){
    for (U32 i=0; i < _nodes.size(); ++i) {
        _nodes[i]->refreshEdges();
    }
}
