//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/







#include <QGraphicsProxyWidget>
#include <QScrollArea>
#include <QScrollBar>
#include "Gui/tabwidget.h"
#include "Gui/DAG.h"
#include "Superviser/controler.h"
#include "Gui/edge.h"
#include "Core/hash.h"
#include "Gui/DAGQuickNode.h"
#include "Gui/mainGui.h"
#include "Gui/dockableSettings.h"
#include "Core/model.h"
#include "Core/viewerNode.h"
#include "Reader/Reader.h"
#include "Gui/knob.h"
#include "Gui/GLViewer.h"
#include "Gui/viewerTab.h"
#include "Gui/node_ui.h"
#include "Core/VideoEngine.h"
#include <QScrollBar>
#include <QGraphicsLineItem>
#include "Gui/timeline.h"
#include <cstdlib>

using namespace std;
using namespace Powiter;
NodeGraph::NodeGraph(QGraphicsScene* scene,QWidget *parent):
    QGraphicsView(scene,parent),
    _evtState(DEFAULT),
    _nodeSelected(0),
    _fullscreen(false),
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
    _navigator = new NodeGraphNavigator(this);
    _navigatorProxy = scene->addWidget(_navigator);
    _navigatorProxy->hide();
    // autoResizeScene();
}

NodeGraph::~NodeGraph(){
    _nodeCreationShortcutEnabled=false;
    _nodes.clear();
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
void NodeGraph::createNodeGUI(QVBoxLayout *dockContainer, Node *node){
    QGraphicsScene* sc=scene();        
    QPointF selectedPos;
    QRectF viewPos = visibleRect();
    double x,y;
    
    if(_nodeSelected){
        selectedPos = _nodeSelected->scenePos();
        x = selectedPos.x();
        int yOffset = 0;
        if(node->className() == "Reader" && _nodeSelected->getNode()->className()!= "Reader"){
            x -= PREVIEW_LENGTH/2;
            yOffset -= PREVIEW_HEIGHT;
        }
        
        if(!node->isOutputNode()){
            yOffset -= (NODE_HEIGHT + 50);
        }else {
            yOffset += (NODE_HEIGHT + 50);
        }
        y =  selectedPos.y() + yOffset;
    }else{
        x = (viewPos.bottomRight().x()-viewPos.topLeft().x())/2.;
        y = (viewPos.bottomRight().y()-viewPos.topLeft().y())/2.;
    }
    
    NodeGui* node_ui = new NodeGui(this,dockContainer,node,x,y,_root,sc);
    _nodes.push_back(node_ui);
    if(_nodeSelected)
        autoConnect(_nodeSelected, node_ui);
    
    selectNode(node_ui);
    _nodeSelected = node_ui;
    
    
    //   autoResizeScene();
    
}
void NodeGraph::mousePressEvent(QMouseEvent *event){
    old_pos=mapToScene(event->pos());
    oldp=event->pos();
    U32 i=0;
    bool found=false;
    while(i<_nodes.size() && !found){
        NodeGui* n=_nodes[i];
        
        QPointF evpt=n->mapFromScene(old_pos);
        if(n->contains(evpt)){
            
            _nodeSelected=n;
            selectNode(n);
            
            _evtState=NODE_DRAGGING;
            found=true;
            break;
        }else{
            const std::vector<Edge*>& arrows = n->getInputsArrows();
            U32 j=0;
            while(j<arrows.size()){
                Edge* a=arrows[j];
                
                if(a->contains(evpt)){
                    
                    _arrowSelected=a;
                    _evtState=ARROW_DRAGGING;
                    found=true;
                    break;
                }
                j++;
            }
            
        }
        i++;
    }
    if(!found){
        deselect();
        _evtState=MOVING_AREA;
    }
}
void NodeGraph::deselect(){
    for(U32 i = 0 ; i < _nodes.size() ;i++){
        NodeGui* n = _nodes[i];
        n->setSelected(false);
        _nodeSelected = 0;
    }
}

void NodeGraph::mouseReleaseEvent(QMouseEvent *event){
    if(_evtState==ARROW_DRAGGING){
        if(_arrowSelected->hasSource()){

            _arrowSelected->getSource()->getNode()->removeChild(_arrowSelected->getDest()->getNode());
            _arrowSelected->getSource()->removeChild(_arrowSelected->getDest());
            
            _arrowSelected->getDest()->getNode()->removeParent(_arrowSelected->getSource()->getNode());
            _arrowSelected->getDest()->removeParent(_arrowSelected->getSource());
            
            _arrowSelected->removeSource();
            scene()->update();
            
        }
        U32 i=0;
        bool foundSrc=false;
        while(i<_nodes.size()){
            NodeGui* n=_nodes[i];
            QPointF ep=mapToScene(event->pos());
            QPointF evpt=n->mapFromScene(ep);
            
            if(n->isNearby(evpt) && n->getNode()->getName()!=_arrowSelected->getDest()->getNode()->getName()){
                if(n->getNode()->isOutputNode() && _arrowSelected->getDest()->getNode()->isOutputNode()){
                    break;
                }
                
                _arrowSelected->getDest()->getNode()->addParent(n->getNode());
                _arrowSelected->getDest()->addParent(n);
                n->getNode()->addChild(_arrowSelected->getDest()->getNode());
                n->addChild(_arrowSelected->getDest());
                _arrowSelected->setSource(n);
                foundSrc=true;

                break;
            }

            i++;
        }
        if(!foundSrc){
            _arrowSelected->removeSource();
        }
        _arrowSelected->initLine();
        scene()->update();
        ctrlPTR->getModel()->clearPlaybackCache();
        
        checkIfViewerConnectedAndRefresh(_arrowSelected->getDest());
        
    }
    scene()->update();
    
    _evtState=DEFAULT;
    //    setCursor(QCursor(Qt::ArrowCursor));
    //viewport()->setCursor(QCursor(Qt::ArrowCursor));
    
    
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
        _nodeSelected->moveBy(diffx,diffy);
        /*also moving node creation anchor*/
        foreach(Edge* arrow,_nodeSelected->getInputsArrows()){
            arrow->initLine();
        }
        
        foreach(NodeGui* child,_nodeSelected->getChildren()){
            foreach(Edge* arrow,child->getInputsArrows()){
                arrow->initLine();
            }
        }
        
    }else if(_evtState==MOVING_AREA){
         double dx = _root->mapFromScene(newPos).x() - _root->mapFromScene(old_pos).x();
         double dy = _root->mapFromScene(newPos).y() - _root->mapFromScene(old_pos).y();
        _root->moveBy(dx, dy);
    }
    old_pos=newPos;
    oldp=event->pos();
    
    /*Now update navigator*/
    //  autoResizeScene();
    updateNavigator();
    
    
}


void NodeGraph::mouseDoubleClickEvent(QMouseEvent *){
    U32 i=0;
    while(i<_nodes.size()){
        NodeGui* n=_nodes[i];
        
        QPointF evpt=n->mapFromScene(old_pos);
        if(n->contains(evpt) && n->getNode()->className() != std::string("Viewer")){
            if(!n->isThisPanelEnabled()){
                /*building settings panel*/
                n->setSettingsPanelEnabled(true);
                n->getSettingPanel()->setVisible(true);
                
            }
            n->putSettingsPanelFirst();
            /*scrolling back to the top the selected node's property tab is visible*/
            _propertyBin->verticalScrollBar()->setValue(0);
            break;
        }
        i++;
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
                QPoint position=ctrlPTR->getGui()->_workshopPane->pos();
                position+=QPoint(ctrlPTR->getGui()->width()/2,0);
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
        ctrlPTR->createNode("Reader");
        Node* reader = _nodes[_nodes.size()-1]->getNode();
        std::vector<Knob*> knobs = reader->getKnobs();
        foreach(Knob* k,knobs){
            if(k->name() == "InputFile"){
                File_Knob* fk = static_cast<File_Knob*>(k);
                fk->open_file();
                break;
            }
        }
        
    }else if(e->key() == Qt::Key_W){
        ctrlPTR->createNode("Writer");
        Node* writer = _nodes[_nodes.size()-1]->getNode();
        std::vector<Knob*> knobs = writer->getKnobs();
        foreach(Knob* k,knobs){
            if(k->name() == "OutputFile"){
                OutputFile_Knob* fk = static_cast<OutputFile_Knob*>(k);
                fk->open_file();
                break;
            }
        }
    }else if(e->key() == Qt::Key_Space){
        
        if(!_fullscreen){
            _fullscreen = true;
            ctrlPTR->getGui()->setFullScreen(dynamic_cast<TabWidget*>(parentWidget()));
        }else{
            _fullscreen = false;
            ctrlPTR->getGui()->exitFullScreen();
        }
        
    }else if(e->key() == Qt::Key_Backspace){
        /*delete current node.*/
        if(_nodeSelected)
            deleteSelectedNode();
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
        //releaseMouse();
        releaseKeyboard();
    }
}


void NodeGraph::wheelEvent(QWheelEvent *event){
     scaleView(pow((double)2, event->delta() / 240.0), mapToScene(event->pos()));
    updateNavigator();
}



void NodeGraph::scaleView(qreal scaleFactor,QPointF){
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;
    
    scale(scaleFactor,scaleFactor);
    
}

void NodeGraph::autoConnect(NodeGui* selected,NodeGui* created){
    Edge* first = 0;
    if(!selected) return;
    bool cont = false;
    if(!selected->getNode()->isOutputNode() && !created->getNode()->isInputNode()){
        /*dst is not outputnode*/
        
        /*check first if it has a child and connect the child to the new node*/
        const vector<Node*>& children = selected->getNode()->getChildren();
        if (children.size() > 0) {
            Node* child = children[0];
            const vector<Edge*>& childEdges = child->getNodeUi()->getInputsArrows();
            Edge* edgeWithSelectedNode = 0;
            for (U32 i = 0; i < childEdges.size(); i++) {
                if (childEdges[i]->getSource() == selected) {
                    edgeWithSelectedNode = childEdges[i];
                    break;
                }
            }
            if(edgeWithSelectedNode && !created->getNode()->isOutputNode()){
                
             
                child->removeParent(selected->getNode());
                child->getNodeUi()->removeParent(selected);
            
                selected->getNode()->removeChild(child);
                selected->removeChild(child->getNodeUi());
                
                created->addChild(child->getNodeUi());
                created->getNode()->addChild(child);
                
                child->addParent(created->getNode());
                child->getNodeUi()->addParent(created);
                
                edgeWithSelectedNode->setSource(created);
                
                /*we now try to move the created node in between the 2 previous*/
                QPointF parentPos = created->mapFromScene(selected->scenePos());
                if(selected->getNode()->className() == "Reader"){
                    parentPos.ry() += (NODE_HEIGHT + PREVIEW_HEIGHT);
                }else{
                    parentPos.ry() += (NODE_HEIGHT);
                }
                QPointF childPos = created->mapFromScene(child->getNodeUi()->scenePos());
                
                QPointF newPos = (parentPos + childPos)/2.;
                
                QPointF oldPos = created->mapFromScene(created->scenePos());
                
                QPointF diff = newPos - oldPos;
                
                created->moveBy(diff.x(), diff.y());
                
                /*now moving the child node so it is at an appropriate distance (not too close to
                 the created one)*/
                QPointF childTopLeft = child->getNodeUi()->scenePos();
                QPointF createdBottomLeft = created->scenePos()+QPointF(0,created->boundingRect().height());
                QPointF createdTopLeft = created->scenePos();
                QPointF parentBottomLeft = selected->scenePos()+QPointF(0,selected->boundingRect().height());
                
                double diffY_child_created,diffY_created_parent;
                diffY_child_created = childTopLeft.y() - createdBottomLeft.y();
                diffY_created_parent = createdTopLeft.y() - parentBottomLeft.y();
                
                double diffX_child_created,diffX_created_parent;
                diffX_child_created = childTopLeft.x() - createdBottomLeft.x();
                diffX_created_parent = createdTopLeft.x() - parentBottomLeft.x();
                
                child->getNodeUi()->moveBy(diffX_created_parent-diffX_child_created, diffY_created_parent-diffY_child_created);
                
                edgeWithSelectedNode->initLine();

            }
        }
        first = created->firstAvailableEdge();
        if(first){
            first->getDest()->getNode()->addParent(selected->getNode());
            first->getDest()->addParent(selected);
            selected->getNode()->addChild(first->getDest()->getNode());
            selected->addChild(first->getDest());
            first->setSource(selected);
            first->initLine();
            cont = true;
        }
        
       
    }else{
        /*dst is outputnode,src isn't*/
        first = selected->firstAvailableEdge();
        if(first && !created->getNode()->isOutputNode()){
            first->getDest()->getNode()->addParent(created->getNode());
            first->getDest()->addParent(created);
            created->getNode()->addChild(first->getDest()->getNode());
            created->addChild(first->getDest());
            first->setSource(created);
            first->initLine();
            cont = true;
            
        }
    }
    
    if(cont){
        NodeGui* viewer = NodeGui::hasViewerConnected(first->getDest());
        if(viewer){
            ctrlPTR->getModel()->setVideoEngineRequirements(viewer->getNode(),true);
            const VideoEngine::DAG& dag = ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG();
            const vector<Node*>& inputs = dag.getInputs();
            bool start = false;
            for (U32 i = 0 ; i < inputs.size(); i++) {
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
                ctrlPTR->getModel()->startVideoEngine(1);
        }
    }
}

void NodeGraph::deleteSelectedNode(){
    Node* n = _nodeSelected->getNode();
    const vector<Node*>& children = n->getChildren();
    Node* child = 0;
    if(children.size() > 0){
        child = children[0];
    }
    ctrlPTR->getModel()->removeNode(_nodeSelected->getNode());
    removeNode(_nodeSelected);
    _nodeSelected = 0;
    ctrlPTR->getModel()->resetInternalDAG();
    if(child)
        checkIfViewerConnectedAndRefresh(child->getNodeUi());
}


void NodeGraph::removeNode(NodeGui* n){
    for(U32 i =0 ; i < _nodes.size();i++){
        if (n == _nodes[i]) {
            
            n->remove();
            _nodes.erase(_nodes.begin()+i);
            break;
        }
    }
    //autoResizeScene();
}
void NodeGraph::selectNode(NodeGui* n){
    /*now remove previously selected node*/
    for(U32 i = 0 ; i < _nodes.size() ;i++){
        _nodes[i]->setSelected(false);
    }
    n->setSelected(true);
}

void NodeGraph::checkIfViewerConnectedAndRefresh(NodeGui* n){
    NodeGui* viewer = NodeGui::hasViewerConnected(n);
    if(viewer){
        //if(foundSrc){
        if(ctrlPTR->getModel()->getVideoEngine()->isWorking()){
            ctrlPTR->getModel()->getVideoEngine()->changeDAGAndStartEngine(viewer->getNode());
        }else{
            std::pair<int,bool> ret = ctrlPTR->getModel()->setVideoEngineRequirements(viewer->getNode(),true);
            if(ret.second){
                ctrlPTR->getModel()->startVideoEngine(1);
            }
            else if(ret.first == 0){ // no inputs, disconnect viewer
                Viewer* v = static_cast<Viewer*>(viewer->getNode());
                if(v){
                    ViewerTab* tab = v->getUiContext();
                    tab->viewer->disconnectViewer();
                }
            }
        }
    }

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
    for (U32 i = 0; i < _nodes.size(); i++) {
        QRectF itemSceneRect = _nodes[i]->mapRectFromScene(rect);
        if(!itemSceneRect.contains(_nodes[i]->boundingRect()))
            return false;
    }
    return true;
}
void NodeGraph::autoResizeScene(){
    QRectF rect(0,0,1,1);
    for (U32 i = 0; i < _nodes.size(); i++) {
        NodeGui* item = _nodes[i];
        rect = rect.united(item->mapToItem(_root,item->boundingRect()).boundingRect());
    }
    setSceneRect(rect);
}
QImage NodeGraph::getFullSceneScreenShot(){
    QImage img(scene()->width(),scene()->height(), QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(0,0,0,0));
    QRectF viewRect = visibleRect();
    QPainter painter(&img);
    painter.save();
    QPen p;
    p.setColor(Qt::yellow);
    p.setWidth(10);
    painter.setPen(p);
    painter.drawRect(viewRect);
    painter.restore();
    scene()->render(&painter);
    p.setColor(QColor(200,200,200,255));
    p.setWidth(10);
    QRect border(0,0,img.width()-1,img.height()-1);
    painter.setPen(p);
    painter.drawRect(border);
    painter.fillRect(viewRect, QColor(200,200,200,100));
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