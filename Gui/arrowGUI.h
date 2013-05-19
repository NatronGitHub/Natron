//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
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
class Node_ui;
class Node;
static const qreal UNATTACHED_ARROW_LENGTH=30.;
class Arrow: public QGraphicsLineItem
{
    
public:
    Arrow(int inputNb,double angle,Node_ui *dest,QGraphicsItem *parent=0, QGraphicsScene *scene=0);
    Arrow(int inputNb,Node_ui *src, Node_ui *dest,QGraphicsItem *parent=0, QGraphicsScene *scene=0);
    QPainterPath shape() const;
    bool contains(const QPointF &point) const;
    void setSource(Node_ui* src){
        this->source=src;
        has_source=true;

    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *options,
               QWidget *parent = 0);
    void removeSource(){has_source=false; initLine();}
    Node_ui* getDest(){return dest;}
    Node_ui* getSource(){return source;}
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
    Node_ui* dest;
    Node_ui* source;
};

#endif // ARROW_H
