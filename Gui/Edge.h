//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_EDGE_H_
#define NATRON_GUI_EDGE_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QGraphicsLineItem>
CLANG_DIAG_ON(deprecated)
#include <boost/shared_ptr.hpp>
#include "Global/Macros.h"

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
    
    ///Used to make an input edge
    Edge(int inputNb,double angle,const boost::shared_ptr<NodeGui>& dest,QGraphicsItem *parent = 0);
    
    ///Used to make an output edge
    Edge(const boost::shared_ptr<NodeGui>& src,QGraphicsItem *parent = 0);
    
    virtual ~Edge() OVERRIDE;
    
    QPainterPath shape() const;
    
    bool contains(const QPointF &point) const;
    
    void setSource(const boost::shared_ptr<NodeGui>& src){
        this->source=src;
        initLine();
    }
    
    int getInputNumber() const {return inputNb;}
    
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *options,QWidget *parent = 0);
    
    boost::shared_ptr<NodeGui> getDest() const {return dest;}
    
    boost::shared_ptr<NodeGui> getSource() const {return source;}
    
    bool hasSource() const { return this->source != NULL; }
    
    void dragSource(const QPointF& src);
    
    void dragDest(const QPointF& dst);
    
    void initLine();
    
    void setAngle(double a){angle = a;}
    
    void turnOnRenderingColor() { _useRenderingColor = true; update(); }
    
    void turnOffRenderingColor() { _useRenderingColor = false; update(); }
    
    bool isOutputEdge() const { return _isOutputEdge; }
    
private:

    bool _isOutputEdge;
    int inputNb;
    double angle;
    QGraphicsTextItem* label;
    QPolygonF arrowHead;
    boost::shared_ptr<NodeGui> dest;
    boost::shared_ptr<NodeGui> source;
    QColor _defaultColor;
    QColor _renderingColor;
    bool _useRenderingColor;
};

#endif // NATRON_GUI_EDGE_H_
