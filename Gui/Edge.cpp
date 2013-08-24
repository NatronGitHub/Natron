//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#include "Edge.h"

#include <cmath>
#include <QPainter>
#include <QGraphicsScene>

#include "Gui/NodeGui.h"
#include "Engine/Node.h"

using namespace std;
const qreal pi= 3.14159265358979323846264338327950288419717;
static const qreal UNATTACHED_ARROW_LENGTH=60.;
const int graphicalContainerOffset=5; //number of offset pixels from the arrow that determine if a click is contained in the arrow or not

Edge::Edge(int inputNb,double angle,NodeGui *dest, QGraphicsItem *parent, QGraphicsScene *scene):QGraphicsLineItem(parent) {

    source = NULL;
    this->scene=scene;
    this->inputNb=inputNb;
    this->angle=angle;
    this->dest=dest;
    has_source=false;
    setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    label=scene->addText(QString(dest->getNode()->getInputLabel(inputNb).c_str()));
    label->setParentItem(this);
    setAcceptedMouseButtons(Qt::LeftButton);
    initLine();
    setFlag(QGraphicsItem::ItemStacksBehindParent);
    setZValue(-1.1);
}
Edge::Edge(int inputNb,NodeGui *src,NodeGui* dest,QGraphicsItem *parent, QGraphicsScene *scene):QGraphicsLineItem(parent)
{

    this->inputNb=inputNb;
    has_source=true;
    this->source=src;
    this->dest=dest;
    setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    label=scene->addText(QString(dest->getNode()->getInputLabel(inputNb).c_str()));
    label->setParentItem(this);
    initLine();


}
void Edge::initLine(){
    int h = NODE_HEIGHT;
    int w = NODE_LENGTH;
    if(dest->getNode()->className() == std::string("Reader")){
        h += PREVIEW_HEIGHT;
        w += PREVIEW_LENGTH;
    }
    QPointF dst = mapFromItem(dest,QPointF(dest->boundingRect().x(),dest->boundingRect().y())
                                           + QPointF(w/2.,h/2.));
    QPointF srcpt;
    if(has_source){
        h = NODE_HEIGHT;
        w = NODE_LENGTH;

        if(source->getNode()->className() == std::string("Reader")){
            h += PREVIEW_HEIGHT;
            w += PREVIEW_LENGTH;
        }
        srcpt= mapFromItem(source,QPointF(source->boundingRect().x(),source->boundingRect().y()))
            + QPointF(w/2.,h/2.);
        setLine(dst.x(),dst.y(),srcpt.x(),srcpt.y());
        
        
        /*adjusting src and dst to show label at the middle of the line*/
        QPointF labelSrcpt= mapFromItem(source,QPointF(source->boundingRect().x(),source->boundingRect().y()))
        + QPointF(w/2.,h);

        h = NODE_HEIGHT;
        w = NODE_LENGTH;
        
        if(dest->getNode()->className() == std::string("Reader")){
            h += PREVIEW_HEIGHT;
            w += PREVIEW_LENGTH;
        }
        QPointF labelDst = mapFromItem(dest,QPointF(dest->boundingRect().x(),dest->boundingRect().y())
                          + QPointF(w/2.,0));
               double norm = sqrt(pow(labelDst.x() - labelSrcpt.x(),2) + pow(labelDst.y() - labelSrcpt.y(),2));
        if(norm > 20.){
            label->setPos((labelDst.x()+labelSrcpt.x())/2.-5.,
                          (labelDst.y()+labelSrcpt.y())/2.-10);
            label->show();
        }else{
            label->hide();
        }
        
    }else{        
        srcpt = QPointF(dst.x() + (cos(angle)*UNATTACHED_ARROW_LENGTH),
          dst.y() - (sin(angle) * UNATTACHED_ARROW_LENGTH));
        
		setLine(dst.x(),dst.y(),srcpt.x(),srcpt.y());
        double cosinus = cos(angle);
        int yOffset = 0;
        if(cosinus < 0){
            yOffset = -40;
        }else if(cosinus >= -0.01 && cosinus <= 0.01){
            
            yOffset = +5;
        }else{
            
            yOffset = +10;
        }
        
        /*adjusting dst to show label at the middle of the line*/
        h = NODE_HEIGHT;
        w = NODE_LENGTH;
        
        if(dest->getNode()->className() == std::string("Reader")){
            h += PREVIEW_HEIGHT;
            w += PREVIEW_LENGTH;
        }
        QPointF labelDst = mapFromItem(dest,QPointF(dest->boundingRect().x(),dest->boundingRect().y())
                          + QPointF(w/2.,0));
               
        label->setPos(((labelDst.x()+srcpt.x())/2.)+yOffset,(labelDst.y()+srcpt.y())/2.-20);
        
    }
    QPointF dstPost = mapFromItem(dest,QPointF(dest->boundingRect().x(),dest->boundingRect().y()));
    QLineF edges[] = { QLineF(dstPost.x()+w,dstPost.y(),dstPost.x()+w,dstPost.y()+h), // right
        QLineF(dstPost.x()+w,dstPost.y()+h,dstPost.x(),dstPost.y()+h), // bottom
        QLineF(dstPost.x(),dstPost.y()+h,dstPost.x(),dstPost.y()), // left
        QLineF(dstPost.x(),dstPost.y(),dstPost.x()+w,dstPost.y())}; // top
    
    for(int i = 0 ; i < 4 ; i++){
        QPointF intersection;
        QLineF::IntersectType type = edges[i].intersect(line(), &intersection);
        if(type == QLineF::BoundedIntersection){
            setLine(QLineF(intersection,line().p2()));
            break;
        }
    }
    qreal a;
    a = acos(line().dx() / line().length());
    if (line().dy() >= 0)
        a = 2*pi - a;
    qreal arrowSize = 5;
    QPointF arrowP1 = line().p1() + QPointF(sin(a + pi / 3) * arrowSize,
                                            cos(a + pi / 3) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(sin(a + pi - pi / 3) * arrowSize,
                                            cos(a + pi - pi / 3) * arrowSize);

    arrowHead.clear();
    arrowHead << dst << arrowP1 << arrowP2;
}


QPainterPath Edge::shape() const
 {
     QPainterPath path = QGraphicsLineItem::shape();
     path.addPolygon(arrowHead);


     return path;
 }

bool Edge::contains(const QPointF &point) const{
    QPointF ULeft,LRight;
    qreal a = acos(line().dx() / line().length());
    if (line().dy() >= 0)
        a = 2*pi - a;
    ULeft = line().p1() + QPointF(cos(a + pi / 2) * graphicalContainerOffset,sin(a + pi / 2) * graphicalContainerOffset);
    LRight=line().p2() + QPointF(cos(a  - pi / 2) * graphicalContainerOffset,sin(a  - pi / 2) * graphicalContainerOffset);

    QRectF rect(ULeft,LRight);
    //cout << "x1 " << line().x1() << " y1 " << line().y1() << " x2 " << line().x2() << " y2 " << line().y2() << endl;
   // cout << "rect x1 " << rect.x() << " rect y1 " << rect.y() << " rect x2 " << rect.x()+rect.width() << " rect y2 " << rect.y()+rect.height() << endl;
    return rect.contains(point);
}
void Edge::updatePosition(QPointF pos){


    qreal x1,y1;
    x1=dest->boundingRect().x();
    y1=dest->boundingRect().y();  
    qreal x2,y2;
    x2=pos.x();
    y2=pos.y();
    qreal a;
    a = acos(line().dx() / line().length());
    if (line().dy() >= 0)
        a = 2*pi - a;


    qreal arrowSize = 5;
    QPointF arrowP1 = line().p1() + QPointF(sin(a + pi / 3) * arrowSize,
                                            cos(a + pi / 3) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(sin(a + pi - pi / 3) * arrowSize,
                                            cos(a + pi - pi / 3) * arrowSize);

    arrowHead.clear();
	arrowHead << line().p1() << arrowP1 << arrowP2;


	setLine(x1+NODE_LENGTH/2,y1,x2,y2);
	label->setPos(((x1+(NODE_LENGTH/2)+x2)/2.)-5,((y1+y2)/2.)-5);



}
void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem * options,
           QWidget * parent)
 {

     (void)parent;
     (void)options;
     QPen myPen = pen();

     myPen.setColor(Qt::black);

     painter->setPen(myPen);
     painter->setBrush(Qt::black);   
     painter->drawLine(line());
     painter->drawPolygon(arrowHead);

  }

