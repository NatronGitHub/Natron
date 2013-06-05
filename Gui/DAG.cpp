//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <QtWidgets/QGraphicsProxyWidget>
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
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QGraphicsLineItem>
#include <cstdlib>

NodeGraph::NodeGraph(QGraphicsScene* scene,QWidget *parent):QGraphicsView(scene,parent),
_fullscreen(false),
state(DEFAULT),
_lastSelectedPos(0,0),
node_dragged(0){
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
    nodeCreation_shortcut_enabled=false;
    nodes.clear();
}

void NodeGraph::addNode_ui(QVBoxLayout *dockContainer,UI_NODE_TYPE type, Node *node,double x,double y){
    QGraphicsScene* sc=scene();
    NodeGui* node_ui;
    
    /*remove previously selected node*/
    for(U32 i = 0 ; i < nodes.size() ;i++){
        NodeGui* n = nodes[i];
        n->setSelected(false);
    }
    
    int yOffset = rand() % 100 + 100;
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
    
    nodes.push_back(node_ui);
    
    if(node_dragged)
        autoConnect(node_dragged, node_ui);
    
    node_ui->setSelected(true);
    node_dragged = node_ui;
    _lastSelectedPos = QPoint(x,y);
    
}
void NodeGraph::mousePressEvent(QMouseEvent *event){
    old_pos=mapToScene(event->pos());
    oldp=event->pos();
    int i=0;
    bool found=false;
    while(i<nodes.size() && !found){
        NodeGui* n=nodes[i];
        
        QPointF evpt=n->mapFromScene(old_pos);
        if(n->contains(evpt)){
            
            node_dragged=n;
            /*now remove previously selected node*/
            for(U32 i = 0 ; i < nodes.size() ;i++){
                nodes[i]->setSelected(false);
            }
            n->setSelected(true);
            _lastSelectedPos = n->pos();
            
            state=NODE_DRAGGING;
            found=true;
            break;
        }else{
            std::vector<Arrow*>& arrows = n->getInputsArrows();
            int j=0;
            while(j<arrows.size()){
                Arrow* a=arrows[j];
                
                if(a->contains(evpt)){
                    
                    arrow_dragged=a;
                    state=ARROW_DRAGGING;
                    found=true;
                    break;
                }
                j++;
            }
            
        }
        i++;
    }
    if(!found){
        for(U32 i = 0 ; i < nodes.size() ;i++){
            NodeGui* n = nodes[i];
            if(n->isSelected()){
                _lastSelectedPos = n->pos();
            }
            n->setSelected(false);
        }
        state=MOVING_AREA;
    }
}
void NodeGraph::mouseReleaseEvent(QMouseEvent *event){
    if(state==ARROW_DRAGGING){
        if(arrow_dragged->hasSource()){
            
            arrow_dragged->getSource()->getNode()->incrementFreeOutputNb();
            
            arrow_dragged->getSource()->getNode()->removeChild(arrow_dragged->getDest()->getNode());
            arrow_dragged->getSource()->substractChild(arrow_dragged->getDest());
            
            arrow_dragged->getDest()->getNode()->removeParent(arrow_dragged->getSource()->getNode());
            arrow_dragged->getDest()->substractParent(arrow_dragged->getSource());
            
            arrow_dragged->removeSource();
            scene()->update();
            if(arrow_dragged->getDest()->getNode()->className() == std::string("Viewer")){
                ViewerGL* gl_viewer = ctrlPTR->getGui()->viewer_tab->viewer;
                ctrlPTR->getModel()->getVideoEngine()->abort(); // aborting current work
                gl_viewer->drawing(false);
                gl_viewer->blankInfoForViewer();
                gl_viewer->fitToFormat(gl_viewer->displayWindow());
                ctrlPTR->getModel()->getVideoEngine()->clearInfos(arrow_dragged->getDest()->getNode());
                ctrlPTR->getModel()->setVideoEngineRequirements(NULL);
                gl_viewer->clearViewer();
                
            }
        }
        int i=0;
        bool foundSrc=false;
        while(i<nodes.size()){
            NodeGui* n=nodes[i];
            QPointF ep=mapToScene(event->pos());
            QPointF evpt=n->mapFromScene(ep);
            
            if(n->isNearby(evpt) && n->getNode()->getName()!=arrow_dragged->getDest()->getNode()->getName()){
                if(n->getNode()->isOutputNode() && arrow_dragged->getDest()->getNode()->isOutputNode()){
                    break;
                }
                
                if(n->getNode()->getFreeOutputCount()>0){
                    arrow_dragged->getDest()->getNode()->addParent(n->getNode());
                    arrow_dragged->getDest()->addParent(n);
                    n->getNode()->addChild(arrow_dragged->getDest()->getNode());
                    n->addChild(arrow_dragged->getDest());
                    n->getNode()->decrementFreeOutputNb();
                    arrow_dragged->setSource(n);
                    foundSrc=true;
                    
                    break;
                }
            }
            i++;
        }
        if(!foundSrc){
            
            arrow_dragged->removeSource();
        }
        arrow_dragged->initLine();
        scene()->update();
        ctrlPTR->getModel()->getVideoEngine()->clearRowCache();
        ctrlPTR->getModel()->getVideoEngine()->clearPlayBackCache();
        if(foundSrc){
            NodeGui* viewer = NodeGui::hasViewerConnected(arrow_dragged->getDest());
            if(viewer){
                ctrlPTR->getModel()->setVideoEngineRequirements(dynamic_cast<OutputNode*>(viewer->getNode()));
                ctrlPTR->getModel()->startVideoEngine(1);
            }
            
        }
        scene()->update();
        
    }
    state=DEFAULT;
    setCursor(QCursor(Qt::ArrowCursor));
    viewport()->setCursor(QCursor(Qt::ArrowCursor));
    
}
void NodeGraph::mouseMoveEvent(QMouseEvent *event){
    QPointF newPos=mapToScene(event->pos());
    if(state==ARROW_DRAGGING){
        QPointF np=arrow_dragged->mapFromScene(newPos);
        arrow_dragged->updatePosition(np);
    }else if(state==NODE_DRAGGING){
        QPointF op=node_dragged->mapFromScene(old_pos);
        QPointF np=node_dragged->mapFromScene(newPos);
        qreal diffx=np.x()-op.x();
        qreal diffy=np.y()-op.y();
        node_dragged->moveBy(diffx,diffy);
        /*also moving node creation anchor*/
        _lastSelectedPos+= QPointF(newPos.x()-old_pos.x() ,newPos.y()-old_pos.y());
        foreach(Arrow* arrow,node_dragged->getInputsArrows()){
            arrow->initLine();
        }
        
        foreach(NodeGui* child,node_dragged->getChildren()){
            foreach(Arrow* arrow,child->getInputsArrows()){
                arrow->initLine();
            }
        }
        
    }else if(state==MOVING_AREA){
        double dx = _root->mapFromScene(newPos).x() - _root->mapFromScene(old_pos).x();
        double dy = _root->mapFromScene(newPos).y() - _root->mapFromScene(old_pos).y();
        _root->moveBy(dx, dy);
        /*also moving node creation anchor*/
        _lastSelectedPos+= QPointF(newPos.x()-old_pos.x() ,newPos.y()-old_pos.y());
        
    }
    old_pos=newPos;
    oldp=event->pos();
    
}
void NodeGraph::mouseDoubleClickEvent(QMouseEvent *event){
    int i=0;
    while(i<nodes.size()){
        NodeGui* n=nodes[i];
        
        QPointF evpt=n->mapFromScene(old_pos);
        if(n->contains(evpt) && n->getNode()->className() != std::string("Viewer")){
            if(!n->isThisPanelEnabled()){
                /*building settings panel*/
                n->setSettingsPanelEnabled(true);
                n->getSettingPanel()->setVisible(true);
                // needed for the layout to work correctly
                QWidget* pr=n->getDockContainer()->parentWidget();
                pr->setMinimumSize(n->getDockContainer()->sizeHint());
                
            }
            break;
        }
        i++;
    }
    
}

void NodeGraph::keyPressEvent(QKeyEvent *e){
    
    if(e->key() == Qt::Key_N  && nodeCreation_shortcut_enabled){
        
        
        if(smartNodeCreationEnabled){
            releaseKeyboard();
            SmartInputDialog* nodeCreation=new SmartInputDialog(this);
            
            QPoint position=ctrlPTR->getGui()->WorkShop->pos();
            position+=QPoint(ctrlPTR->getGui()->width()/2,0);
            nodeCreation->move(position);
            setMouseTracking(false);
            
            nodeCreation->show();
            nodeCreation->raise();
            nodeCreation->activateWindow();
            
            
            smartNodeCreationEnabled=false;
        }
        
        
    }else if(e->key() == Qt::Key_R){
        try{
            
            ctrlPTR->createNode("Reader");
        }catch(...){
            std::cout << "(NodeGraph::keyPressEvent) Couldn't create reader. " << std::endl;
            
        }
        Node* reader = nodes[nodes.size()-1]->getNode();
        std::vector<Knob*> knobs = reader->getKnobs();
        foreach(Knob* k,knobs){
            if(k->getType() == FILE_KNOB){
                File_Knob* fk = static_cast<File_Knob*>(k);
                fk->open_file();
                break;
            }
        }
        
    }else if(e->key() == Qt::Key_Space){
        
        if(!_fullscreen){
            _fullscreen = true;
            ctrlPTR->getGui()->viewer_tab->hide();
        }else{
            _fullscreen = false;
            ctrlPTR->getGui()->viewer_tab->show();
        }
        
    }
    
    
    
}


void NodeGraph::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);
    if(smartNodeCreationEnabled){
        
        nodeCreation_shortcut_enabled=true;
        setFocus();
        grabMouse();
        releaseMouse();
        grabKeyboard();
    }
}
void NodeGraph::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    if(smartNodeCreationEnabled){
        nodeCreation_shortcut_enabled=false;
        setFocus();
        releaseMouse();
        releaseKeyboard();
    }
}


void NodeGraph::wheelEvent(QWheelEvent *event){
    scaleView(pow((double)2, event->delta() / 240.0), mapToScene(event->pos()));
}



void NodeGraph::scaleView(qreal scaleFactor,QPointF center){
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;
    
    scale(scaleFactor,scaleFactor);
    
}

void NodeGraph::autoConnect(NodeGui* selected,NodeGui* created){
    Arrow* first = 0;
    /*dst is outputnode,src isn't*/
    if(selected->getNode()->isOutputNode()){
        first = selected->firstAvailableEdge();
        if(first){
            if(created->getNode()->getFreeOutputCount() > 0){
                first->getDest()->getNode()->addParent(created->getNode());
                first->getDest()->addParent(created);
                created->getNode()->addChild(first->getDest()->getNode());
                created->addChild(first->getDest());
                created->getNode()->decrementFreeOutputNb();
                first->setSource(created);
                first->initLine();
            }
        }
    }else{
        /*dst is not outputnode*/
        if (selected->getNode()->getFreeOutputCount() > 0) {
            first = created->firstAvailableEdge();
            if(first){
                first->getDest()->getNode()->addParent(selected->getNode());
                first->getDest()->addParent(selected);
                selected->getNode()->addChild(first->getDest()->getNode());
                selected->addChild(first->getDest());
                selected->getNode()->decrementFreeOutputNb();
                first->setSource(selected);
                first->initLine();
            }
        }
    }
    if(first){
        NodeGui* viewer = NodeGui::hasViewerConnected(first->getDest());
        if(viewer){
            ctrlPTR->getModel()->setVideoEngineRequirements(dynamic_cast<OutputNode*>(viewer->getNode()));
            ctrlPTR->getModel()->startVideoEngine(1);
        }
    }
}
