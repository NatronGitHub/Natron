//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef NODEGRAPH_H
#define NODEGRAPH_H
#include <cmath>
#include <QtWidgets/QGraphicsView>
#include <iostream>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QInputDialog>
#include "Gui/outputnode_ui.h"
#include "Gui/inputnode_ui.h"
#include "Gui/operatornode_ui.h"
#include "Superviser/powiterFn.h"
using namespace Powiter_Enums;
class Controler;
class Arrow;
class SmartInputDialog;
class SettingsPanel;

class NodeGraph: public QGraphicsView{
    enum EVENT_STATE{DEFAULT,MOVING_AREA,ARROW_DRAGGING,NODE_DRAGGING};
    Q_OBJECT

public:

    NodeGraph(QGraphicsScene* scene=0,QWidget *parent=0);

    virtual ~NodeGraph();
 
    void addNode_ui(QVBoxLayout *dockContainer,UI_NODE_TYPE type,Node *node,double x,double y);

    virtual void enterEvent(QEvent *event);
    
    virtual void leaveEvent(QEvent *event);

    virtual void keyPressEvent(QKeyEvent *e);
    
    void autoConnect(NodeGui* selected,NodeGui* created);
    
    void setSmartNodeCreationEnabled(bool enabled){smartNodeCreationEnabled=enabled;}

	
protected:

    void mousePressEvent(QMouseEvent *event);
    
    void mouseReleaseEvent(QMouseEvent *event);
    
    void mouseMoveEvent(QMouseEvent *event);
    
    void mouseDoubleClickEvent(QMouseEvent *event);

    void wheelEvent(QWheelEvent *event);

    void scaleView(qreal scaleFactor,QPointF center);

private:
    bool smartNodeCreationEnabled;
    QPointF old_pos;
    QPointF oldp;
    QPointF oldZoom;
    QPointF _lastSelectedPos;
    EVENT_STATE state;
    NodeGui* node_dragged;
    Arrow* arrow_dragged;
    std::vector<NodeGui*> nodes;
    int timerId;
    bool nodeCreation_shortcut_enabled;
    bool _fullscreen;
    QGraphicsItem* _root;
    
};


#endif // NODEGRAPH_H
