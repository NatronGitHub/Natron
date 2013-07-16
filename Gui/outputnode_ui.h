//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef OUTPUTNODE_UI_H
#define OUTPUTNODE_UI_H
#include "Gui/node_ui.h"
class NodeGraph;
class OutputNode_ui : public NodeGui
{
public:
    OutputNode_ui(NodeGraph* dag,QVBoxLayout* dockContainer,
                  Node* node,qreal x, qreal y,QGraphicsItem* parent=0,QGraphicsScene* sc=0);
    QRectF boundingRect() const{return NodeGui::boundingRect();}
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *parent){
        NodeGui::paint(painter,options,parent);
    }
    
    
    virtual bool hasOutput(){return false;}
};

#endif // OUTPUTNODE_UI_H
