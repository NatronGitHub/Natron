//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com
#ifndef INPUTNODE_UI_H
#define INPUTNODE_UI_H
#include "Gui/node_ui.h"
class InputNode_ui : public NodeGui
{
public:
    InputNode_ui(NodeGraph* dag,QVBoxLayout* dockContainer,
                 Node *node,qreal x, qreal y,QGraphicsItem* parent=0,QGraphicsScene* sc=0);
    virtual ~InputNode_ui(){}
     bool hasOutput(){return true;}
     QRectF boundingRect() const{return NodeGui::boundingRect();}
     void paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *parent){
         NodeGui::paint(painter,options,parent);
     }
    
};

#endif // INPUTNODE_UI_H
