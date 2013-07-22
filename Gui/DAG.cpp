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
#include "Gui/tabwidget.h"
#include "Gui/DAG.h"
#include "Superviser/controler.h"
#include "Gui/arrowGUI.h"
#include "Core/hash.h"
#include "Core/outputnode.h"
#include "Core/inputnode.h"
#include "Core/OP.h"
#include "Gui/DAGQuickNode.h"
#include "Gui/mainGui.h"
#include "Gui/dockableSettings.h"
#include "Core/model.h"
#include "Core/viewerNode.h"
#include "Reader/Reader.h"
#include "Gui/knob.h"
#include "Gui/GLViewer.h"
#include "Gui/viewerTab.h"
#include "Core/VideoEngine.h"
#include <QScrollBar>
#include <QGraphicsLineItem>
#include "Gui/timeline.h"
#include "Gui/outputnode_ui.h"
#include "Gui/inputnode_ui.h"
#include "Gui/operatornode_ui.h"
#include <cstdlib>

using namespace std;
using namespace Powiter;
NodeGraph::NodeGraph(QGraphicsScene* scene,QWidget *parent):
QGraphicsView(scene,parent),
_lastSelectedPos(0,0),
_evtState(DEFAULT),
_nodeSelected(0),
_fullscreen(false)
{
    
    setObjectName("DAG_GUI");
    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    
    scale(qreal(0.8), qreal(0.8));
    
    smartNodeCreationEnabled=true;
    _root = new QGraphicsLineItem(0);
    scene->addItem(_root);
    oldZoom = QPointF(0,0);
    srand(2013);
}

NodeGraph::~NodeGraph(){
    _nodeCreationShortcutEnabled=false;
    _nodes.clear();
}

void NodeGraph::createNodeGUI(QVBoxLayout *dockContainer,UI_NODE_TYPE type, Node *node,double x,double y){
    QGraphicsScene* sc=scene();
    NodeGui* node_ui;
    
   
    
    int yOffset = rand() % 50 + 50;
    if(x == INT_MAX)
        x = _lastSelectedPos.x();
    if(type==OUTPUT){
        if(y == INT_MAX)
            y = _lastSelectedPos.y() + yOffset;
        node_ui=new OutputNode_ui(this,dockContainer,node,x,y,_root,sc);
    }else if(type==INPUT_NODE){
        if(y == INT_MAX)
            y = _lastSelectedPos.y() - yOffset;
        node_ui=new InputNode_ui(this,dockContainer,node,x,y,_root,sc);
    }else {
        if(y == INT_MAX)
            y = _lastSelectedPos.y() - yOffset;
        node_ui=new OperatorNode_ui(this,dockContainer,node,x,y,_root,sc);
    }
    
    _nodes.push_back(node_ui);
    
    if(_nodeSelected)
        autoConnect(_nodeSelected, node_ui);
    
    selectNode(node_ui);
    _nodeSelected = node_ui;
    _lastSelectedPos = QPoint(x,y);
    
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
            _lastSelectedPos = n->pos();
            
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
        if(n->isSelected()){
            _lastSelectedPos = n->pos();
        }
        n->setSelected(false);
        _nodeSelected = 0;
    }
}

void NodeGraph::mouseReleaseEvent(QMouseEvent *event){
    if(_evtState==ARROW_DRAGGING){
        if(_arrowSelected->hasSource()){
            
            _arrowSelected->getSource()->getNode()->releaseSocket();
            
            _arrowSelected->getSource()->getNode()->removeChild(_arrowSelected->getDest()->getNode());
            _arrowSelected->getSource()->substractChild(_arrowSelected->getDest());
            
            _arrowSelected->getDest()->getNode()->removeParent(_arrowSelected->getSource()->getNode());
            _arrowSelected->getDest()->substractParent(_arrowSelected->getSource());
            
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
                
                if(n->getNode()->getFreeSocketCount()>0){
                    _arrowSelected->getDest()->getNode()->addParent(n->getNode());
                    _arrowSelected->getDest()->addParent(n);
                    n->getNode()->addChild(_arrowSelected->getDest()->getNode());
                    n->addChild(_arrowSelected->getDest());
                    n->getNode()->lockSocket();
                    _arrowSelected->setSource(n);
                    foundSrc=true;
                    
                    break;
                }
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
    setCursor(QCursor(Qt::ArrowCursor));
    viewport()->setCursor(QCursor(Qt::ArrowCursor));
    
}
void NodeGraph::mouseMoveEvent(QMouseEvent *event){
    QPointF newPos=mapToScene(event->pos());
    if(_evtState==ARROW_DRAGGING){
        QPointF np=_arrowSelected->mapFromScene(newPos);
        _arrowSelected->updatePosition(np);
    }else if(_evtState==NODE_DRAGGING){
        QPointF op=_nodeSelected->mapFromScene(old_pos);
        QPointF np=_nodeSelected->mapFromScene(newPos);
        qreal diffx=np.x()-op.x();
        qreal diffy=np.y()-op.y();
        _nodeSelected->moveBy(diffx,diffy);
        /*also moving node creation anchor*/
        _lastSelectedPos+= QPointF(newPos.x()-old_pos.x() ,newPos.y()-old_pos.y());
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
        /*also moving node creation anchor*/
        _lastSelectedPos+= QPointF(newPos.x()-old_pos.x() ,newPos.y()-old_pos.y());
        
    }
    old_pos=newPos;
    oldp=event->pos();
    
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
                // needed for the layout to work correctly
               // QWidget* pr=n->getDockContainer()->parentWidget();
               // pr->setMinimumSize(n->getDockContainer()->sizeHint());
                
            }
            n->putSettingsPanelFirst();

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
        try{
            
            ctrlPTR->createNode("Reader");
        }catch(...){
            std::cout << "(NodeGraph::keyPressEvent) Couldn't create reader. " << std::endl;
            
        }
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
        try{
            
            ctrlPTR->createNode("Writer");
        }catch(...){
            std::cout << "(NodeGraph::keyPressEvent) Couldn't create writer. " << std::endl;
            
        }
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
    /*dst is outputnode,src isn't*/
    if(selected->getNode()->isOutputNode()){
        first = selected->firstAvailableEdge();
        if(first){
            if(created->getNode()->getFreeSocketCount() > 0){
                first->getDest()->getNode()->addParent(created->getNode());
                first->getDest()->addParent(created);
                created->getNode()->addChild(first->getDest()->getNode());
                created->addChild(first->getDest());
                created->getNode()->lockSocket();
                first->setSource(created);
                first->initLine();
                cont = true;
            }
        }
    }else{
        /*dst is not outputnode*/
        if (selected->getNode()->getFreeSocketCount() > 0) {
            first = created->firstAvailableEdge();
            if(first){
                first->getDest()->getNode()->addParent(selected->getNode());
                first->getDest()->addParent(selected);
                selected->getNode()->addChild(first->getDest()->getNode());
                selected->addChild(first->getDest());
                selected->getNode()->lockSocket();
                first->setSource(selected);
                first->initLine();
                cont = true;
            }
        }
    }
    
    if(cont){
        NodeGui* viewer = NodeGui::hasViewerConnected(first->getDest());
        if(viewer){
            ctrlPTR->getModel()->setVideoEngineRequirements(dynamic_cast<OutputNode*>(viewer->getNode()),true);
            const VideoEngine::DAG& dag = ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG();
            const vector<InputNode*>& inputs = dag.getInputs();
            bool start = false;
            for (U32 i = 0 ; i < inputs.size(); i++) {
                if (inputs[0]->className() == "Reader") {
                    if(static_cast<Reader*>(inputs[0])->hasFrames())
                        start = true;
                }
            }
            if(start)
                ctrlPTR->getModel()->startVideoEngine(1);
        }
    }
}

void NodeGraph::deleteSelectedNode(){
    ctrlPTR->getModel()->removeNode(_nodeSelected->getNode());
    
    removeNode(_nodeSelected);
    _nodeSelected = 0;
    ctrlPTR->getModel()->resetInternalDAG();
}


void NodeGraph::removeNode(NodeGui* n){
    for(U32 i =0 ; i < _nodes.size();i++){
        if (n == _nodes[i]) {
            
            n->remove();
            _nodes.erase(_nodes.begin()+i);
            break;
        }
    }
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
            ctrlPTR->getModel()->getVideoEngine()->changeDAGAndStartEngine(dynamic_cast<OutputNode*>(viewer));
        }else{
            std::pair<int,bool> ret = ctrlPTR->getModel()->setVideoEngineRequirements(dynamic_cast<OutputNode*>(viewer->getNode()),true);
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
