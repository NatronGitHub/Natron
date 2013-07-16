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

 

 



#ifndef ARROW_H
#define ARROW_H
#include <QtWidgets/QGraphicsLineItem>
#include <QtCore/QPoint>
class QGraphicsPolygonItem;
class QGraphicsLineItem;
class QRectF;
class QPainterPath;
class QGraphicsScene;
class QGraphicsTextItem;
class QGraphicsSceneMouseEvent;
class NodeGui;
class Node;
static const qreal UNATTACHED_ARROW_LENGTH=30.;
class Edge: public QGraphicsLineItem
{
    
public:
    Edge(int inputNb,double angle,NodeGui *dest,QGraphicsItem *parent=0, QGraphicsScene *scene=0);
    Edge(int inputNb,NodeGui *src, NodeGui *dest,QGraphicsItem *parent=0, QGraphicsScene *scene=0);
    QPainterPath shape() const;
    bool contains(const QPointF &point) const;
    void setSource(NodeGui* src){
        this->source=src;
        has_source=true;

    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *options,
               QWidget *parent = 0);
    void removeSource(){has_source=false; initLine();}
    NodeGui* getDest(){return dest;}
    NodeGui* getSource(){return source;}
    bool hasSource(){return has_source;}
    void updatePosition(QPointF pos);
    void initLine();

protected:



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

#endif // ARROW_H
