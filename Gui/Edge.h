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

#ifndef POWITER_GUI_EDGE_H_
#define POWITER_GUI_EDGE_H_

#include <QGraphicsLineItem>

class QGraphicsPolygonItem;
class QGraphicsLineItem;
class QRectF;
class QPointF;
class QPainterPath;
class QGraphicsScene;
class QGraphicsTextItem;
class QGraphicsSceneMouseEvent;
class NodeGui;
class Node;
class Edge: public QGraphicsLineItem
{
    
public:
    Edge(int inputNb,double angle,NodeGui *dest,QGraphicsItem *parent=0, QGraphicsScene *scene=0);
    Edge(int inputNb,NodeGui *src, NodeGui *dest,QGraphicsItem *parent=0, QGraphicsScene *scene=0);
    
    virtual ~Edge();
    
    QPainterPath shape() const;
    
    bool contains(const QPointF &point) const;
    
    void setSource(NodeGui* src){
        this->source=src;
        src ? has_source=true : has_source=false;
        initLine();

    }
    
    int getInputNumber() const {return inputNb;}
    
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *options,QWidget *parent = 0);
    
    void removeSource(){has_source=false; source = NULL; initLine();}
    
    NodeGui* getDest() const {return dest;}
    
    NodeGui* getSource() const {return source;}
    
    bool hasSource() const {return has_source;}
    
    void updatePosition(const QPointF& src);
    
    void initLine();
    
    QGraphicsScene* getScene() const {return scene;}

private:



    QGraphicsScene* scene;
    int inputNb;
    double angle;
    QGraphicsTextItem* label;
    bool has_source;
    QPolygonF arrowHead;
    NodeGui* dest;
    NodeGui* source;
};

#endif // POWITER_GUI_EDGE_H_
