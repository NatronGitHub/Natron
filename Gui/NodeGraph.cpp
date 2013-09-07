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
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
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
#include "Global/NodeInstance.h"


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
NodeGui* NodeGraph::createNodeGUI(QVBoxLayout *dockContainer, NodeInstance *node){
    QGraphicsScene* sc=scene();
    QPointF selectedPos;
    QRectF viewPos = visibleRect();
    double x,y;
    
    if(_nodeSelected){
        selectedPos = _nodeSelected->scenePos();
        x = selectedPos.x();
        int yOffset = 0;
        if(node->isInputNode() && !_nodeSelected->getNodeInstance()->isInputNode()){
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
    if(_nodeSelected)
        autoConnect(_nodeSelected, node_ui);
    
    selectNode(node_ui);
    
    if(int ret = node_ui->hasPreviewImage()){
        if(ret == 2){
            OfxNode* n = dynamic_cast<OfxNode*>(node->getNode());
            n->computePreviewImage();
        }
    }
    return node_ui;
    
}
void NodeGraph::mousePressEvent(QMouseEvent *event){
    old_pos=mapToScene(event->pos());
    oldp=event->pos();
    U32 i=0;
    bool found=false;
    while(i<_nodes.size() && !found){
        NodeGui* n=_nodes[i];
        
        QPointF evpt=n->mapFromScene(old_pos);
        if(n->isActive() && n->contains(evpt)){
            
            selectNode(n);
            
            _evtState=NODE_DRAGGING;
            _lastNodeDragStartPoint = n->pos();
            found=true;
            break;
        }else{
            Edge* edge = n->hasEdgeNearbyPoint(old_pos);
            if(edge){
                _arrowSelected = edge;
                _evtState=ARROW_DRAGGING;
            }else{
                deselect();
                _evtState=MOVING_AREA;
            }
            
        }
        ++i;
    }
    if(!found){
        
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
        U32 i=0;
        bool foundSrc=false;
        while(i<_nodes.size()){
            NodeGui* n=_nodes[i];
            QPointF ep = mapToScene(event->pos());
            QPointF evpt = n->mapFromScene(ep);
            
            if(n->isActive() && n->isNearby(evpt) &&
               (n->getNodeInstance()->getName()!=_arrowSelected->getDest()->getNodeInstance()->getName())){
                if(n->getNodeInstance()->isOutputNode() && _arrowSelected->getDest()->getNodeInstance()->isOutputNode()){
                    break;
                }
                _undoStack->push(new ConnectCommand(this,_arrowSelected,_arrowSelected->getSource(),n));
                foundSrc=true;
                
                break;
            }
            
            ++i;
        }
        if(!foundSrc){
            _undoStack->push(new ConnectCommand(this,_arrowSelected,_arrowSelected->getSource(),NULL));
            scene()->update();
        }
        _arrowSelected->initLine();
        scene()->update();
        _gui->_appInstance->getModel()->clearPlaybackCache();
        
        _arrowSelected->getDest()->getNodeInstance()->checkIfViewerConnectedAndRefresh();
        
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
        _nodeSelected->setPos(_nodeSelected->pos()+QPointF(diffx,diffy));
        /*also moving node creation anchor*/
        _nodeSelected->getNodeInstance()->refreshEdgesGui();
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
        if(n->isActive() && n->contains(evpt) && n->getNodeInstance()->className() != "Viewer"){
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
        NodeInstance* reader = _gui->_appInstance->createNode("Reader");
        std::vector<KnobGui*> knobs = reader->getKnobs();
        foreach(KnobGui* k,knobs){
            if(k->name() == "InputFile"){
                File_KnobGui* fk = dynamic_cast<File_KnobGui*>(k);
                fk->open_file();
                break;
            }
        }
        
    }else if(e->key() == Qt::Key_W){
        NodeInstance* writer = _gui->_appInstance->createNode("Writer");
        std::vector<KnobGui*> knobs = writer->getKnobs();
        foreach(KnobGui* k,knobs){
            if(k->name() == "OutputFile"){
                OutputFile_KnobGui* fk = static_cast<OutputFile_KnobGui*>(k);
                fk->open_file();
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
    }else if(e->key() == Qt::Key_1){
        if(_nodeSelected){
            connectCurrentViewerToSelection();
        }
    }
}
void NodeGraph::connectCurrentViewerToSelection(){
    ViewerTab* lastSelectedViewerTab = _gui->getLastSelectedViewer();
    const NodeGui::InputEdgesMap& edges = lastSelectedViewerTab->getInternalNode()->getNodeInstance()->getNodeGui()->getInputsArrows();
    NodeGui::InputEdgesMap::const_iterator it = edges.find(1);
    if(it!=edges.end()){
        _undoStack->push(new ConnectCommand(this,it->second,it->second->getSource(),_nodeSelected));
    }
}

void NodeGraph::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);
    if(smartNodeCreationEnabled){
        
        _nodeCreationShortcutEnabled=true;
        setFocus();
        //grabMouse();
        //releaseMouse();
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
    
    _root->scale(scaleFactor,scaleFactor);
    
}

void NodeGraph::autoConnect(NodeGui* selected,NodeGui* created){
    Edge* first = 0;
    if(!selected) return;
    bool cont = false;
    if(!selected->getNodeInstance()->isOutputNode() && !created->getNodeInstance()->isInputNode()){
        /*dst is not outputnode*/
        
        /*check first if it has outputs and connect the outputs to the new node*/
        const NodeInstance::OutputMap& outputs = selected->getNodeInstance()->getOutputs();
        for (NodeInstance::OutputMap::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
            NodeInstance* output = it->second;
            const NodeGui::InputEdgesMap& outputEdges = output->getNodeGui()->getInputsArrows();
            Edge* edgeWithSelectedNode = 0;
            for (NodeGui::InputEdgesMap::const_iterator i = outputEdges.begin();i!=outputEdges.end();++i) {
                if (i->second->getSource() == selected) {
                    edgeWithSelectedNode = i->second;
                    break;
                }
            }
            if(edgeWithSelectedNode && !created->getNodeInstance()->isOutputNode()){
                _undoStack->push(new ConnectCommand(this,edgeWithSelectedNode,selected,created));
                
                /*we now try to move the created node in between the 2 previous*/
                QPointF parentPos = created->mapFromScene(selected->scenePos());
                if(selected->hasPreviewImage()){
                    parentPos.ry() += (NodeGui::NODE_HEIGHT + NodeGui::PREVIEW_HEIGHT);
                }else{
                    parentPos.ry() += (NodeGui::NODE_HEIGHT);
                }
                QPointF childPos = created->mapFromScene(output->getNodeGui()->scenePos());
                QPointF newPos = (parentPos + childPos)/2.;
                QPointF oldPos = created->mapFromScene(created->scenePos());
                QPointF diff = newPos - oldPos;
        
                created->moveBy(diff.x(), diff.y());
                
                /*now moving the output node so it is at an appropriate distance (not too close to
                 the created one)*/
                QPointF childTopLeft = output->getNodeGui()->scenePos();
                QPointF createdBottomLeft = created->scenePos()+QPointF(0,created->boundingRect().height());
                QPointF createdTopLeft = created->scenePos();
                QPointF parentBottomLeft = selected->scenePos()+QPointF(0,selected->boundingRect().height());
                
                double diffY_child_created,diffY_created_parent;
                diffY_child_created = childTopLeft.y() - createdBottomLeft.y();
                diffY_created_parent = createdTopLeft.y() - parentBottomLeft.y();
                
                double diffX_child_created,diffX_created_parent;
                diffX_child_created = childTopLeft.x() - createdBottomLeft.x();
                diffX_created_parent = createdTopLeft.x() - parentBottomLeft.x();
                
                output->getNodeGui()->moveBy(diffX_created_parent-diffX_child_created, diffY_created_parent-diffY_child_created);
                
                edgeWithSelectedNode->initLine();
            }
        }
        first = created->firstAvailableEdge();
        if(first){
            _undoStack->push(new ConnectCommand(this,first,first->getSource(),selected));
            cont = true;
        }
    }else{
        /*dst is outputnode,src isn't*/        
        first = selected->firstAvailableEdge();
        if(first && !created->getNodeInstance()->isOutputNode()){
            _undoStack->push(new ConnectCommand(this,first,first->getSource(),created));
            cont = true;
            
        }
    }
    
    if(cont){
        Node* viewer = Node::hasViewerConnected(first->getDest()->getNodeInstance()->getNode());
        if(viewer){
            _gui->_appInstance->setCurrentGraph(dynamic_cast<OutputNode*>(viewer),true);
            const VideoEngine::DAG& dag = _gui->_appInstance->getModel()->getVideoEngine()->getCurrentDAG();
            const vector<Node*>& inputs = dag.getInputs();
            bool start = false;
            for (U32 i = 0 ; i < inputs.size(); ++i) {
                if(inputs[i]->className() == "Reader"){
                    Reader* reader = dynamic_cast<Reader*>(inputs[i]);
                    if(reader->hasFrames()) start = true;
                    else{
                        start = false;
                        break;
                    }
                }else{
                    if(inputs[0]->isInputNode()){
                        start = true;
                    }
                }
            }
            if(start)
                _gui->_appInstance->startRendering(1);
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
            n->remove();
            _nodes.erase(_nodes.begin()+i);
            break;
        }
    }
    for(U32 i = 0 ; i < _nodesTrash.size();++i) {
        if (n == _nodesTrash[i]) {
            n->remove();
            _nodes.erase(_nodesTrash.begin()+i);
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
    if(n->getNodeInstance()->className() == "Viewer"){
        _gui->setLastSelectedViewer(dynamic_cast<ViewerNode*>(n->getNodeInstance()->getNode())->getUiContext());
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
        if (_nodes[i]->getNodeInstance()->className() != "Writer" && _nodes[i]->getNodeInstance()->className() != "Viewer") {
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
    
    _node->getNodeInstance()->refreshEdgesGui();
    
    if(_node->scene())
        _node->scene()->update();
    setText(QObject::tr("Move %1")
            .arg(_node->getNodeInstance()->getName().c_str()));
}
void MoveCommand::redo(){
    _node->setPos(_newPos);
    _node->getNodeInstance()->refreshEdgesGui();
    setText(QObject::tr("Move %1")
            .arg(_node->getNodeInstance()->getName().c_str()));
}
bool MoveCommand::mergeWith(const QUndoCommand *command){
    const MoveCommand *moveCommand = static_cast<const MoveCommand *>(command);
    NodeGui* node = moveCommand->_node;
    if(_node != node)
        return false;
    _newPos = node->pos();
    setText(QObject::tr("Move %1")
            .arg(node->getNodeInstance()->getName().c_str()));
    return true;
}


AddCommand::AddCommand(NodeGraph* graph,NodeGui *node,QUndoCommand *parent):QUndoCommand(parent),
_node(node),_graph(graph),_undoWasCalled(false){
    
}
void AddCommand::undo(){
    _undoWasCalled = true;
  
    
    
    _inputs = _node->getNodeInstance()->getInputs();
    _outputs = _node->getNodeInstance()->getOutputs();
    
    _node->getNodeInstance()->deactivate();

    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNodeInstance()->getName().c_str()));
    
}
void AddCommand::redo(){
    if(_undoWasCalled){
       
        _node->getNodeInstance()->activate();
    }
    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNodeInstance()->getName().c_str()));
    
    
}

RemoveCommand::RemoveCommand(NodeGraph* graph,NodeGui *node,QUndoCommand *parent):QUndoCommand(parent),
_node(node),_graph(graph){
    
}
void RemoveCommand::undo(){
    _node->getNodeInstance()->activate();
    
    _graph->scene()->update();
    setText(QObject::tr("Remove %1")
            .arg(_node->getNodeInstance()->getName().c_str()));
    
    
    
}
void RemoveCommand::redo(){
    _node->getNodeInstance()->deactivate();
    
    _inputs = _node->getNodeInstance()->getInputs();
    _outputs = _node->getNodeInstance()->getOutputs();

    
    _graph->scene()->update();
    setText(QObject::tr("Add %1")
            .arg(_node->getNodeInstance()->getName().c_str()));
    
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
        _graph->getGui()->_appInstance->connect(_edge->getInputNumber(), _oldSrc->getNodeInstance(), _edge->getDest()->getNodeInstance());
    }
    if(_newSrc){
        _graph->getGui()->_appInstance->disconnect(_newSrc->getNodeInstance(), _edge->getDest()->getNodeInstance());
    }
    
    if(_oldSrc){
        setText(QObject::tr("Connect %1 to %2")
                .arg(_edge->getDest()->getNodeInstance()->getName().c_str()).arg(_oldSrc->getNodeInstance()->getName().c_str()));
    }else{
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNodeInstance()->getName().c_str()));
    }
    _graph->getGui()->_appInstance->triggerAutoSaveOnNextEngineRun();
    _edge->getDest()->getNodeInstance()->checkIfViewerConnectedAndRefresh();
    
    
}
void ConnectCommand::redo(){
    _edge->setSource(_newSrc);
    
    if(_oldSrc){
        _graph->getGui()->_appInstance->disconnect(_oldSrc->getNodeInstance(), _edge->getDest()->getNodeInstance());
    }
    if(_newSrc){
        _graph->getGui()->_appInstance->connect(_edge->getInputNumber(), _newSrc->getNodeInstance(), _edge->getDest()->getNodeInstance());
    }
    
    _edge->initLine();
    if(_newSrc){
        setText(QObject::tr("Connect %1 to %2")
            .arg(_edge->getDest()->getNodeInstance()->getName().c_str()).arg(_newSrc->getNodeInstance()->getName().c_str()));
    }else{
        setText(QObject::tr("Disconnect %1")
                .arg(_edge->getDest()->getNodeInstance()->getName().c_str()));
    }
    _edge->getDest()->getNodeInstance()->checkIfViewerConnectedAndRefresh();
    _graph->getGui()->_appInstance->triggerAutoSaveOnNextEngineRun();

    
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

    textEdit->addItems(graph->getGui()->_appInstance->getNodeNameList());
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
        if(graph->getGui()->_appInstance->getNodeNameList().contains(res)){
            graph->getGui()->_appInstance->createNode(res);
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


