//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef INPUTNODE_UI_H
#define INPUTNODE_UI_H
#include "Gui/node_ui.h"
class InputNode_ui : public NodeGui
{
public:
    InputNode_ui(NodeGraph* dag,QVBoxLayout* dockContainer,
                 Node *node,qreal x, qreal y,QGraphicsItem* parent=0,QGraphicsScene* sc=0);
     bool hasOutput(){return true;}
     QRectF boundingRect() const{return NodeGui::boundingRect();}
     void paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *parent){
         NodeGui::paint(painter,options,parent);
     }
    
};

#endif // INPUTNODE_UI_H
