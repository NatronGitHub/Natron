//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#include "arrowGUI.h"

#include <cmath>
#include <QPainter>
#include <QGraphicsScene>

#include "Gui/node_ui.h"
#include "Core/node.h"

using namespace std;
const qreal pi= 3.14159265358979323846264338327950288419717;
const int graphicalContainerOffset=5;

Edge::Edge(int inputNb,double angle,NodeGui *dest, QGraphicsItem *parent, QGraphicsScene *scene):QGraphicsLineItem(parent) {

    this->scene=scene;
    this->inputNb=inputNb;
    this->angle=angle;
    this->dest=dest;
    has_source=false;
    setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    label=scene->addText(QString(dest->getNode()->getLabel(inputNb).c_str()));
    label->setParentItem(this);
    setAcceptedMouseButtons(Qt::LeftButton);
    initLine();
}
Edge::Edge(int inputNb,NodeGui *src,NodeGui* dest,QGraphicsItem *parent, QGraphicsScene *scene):QGraphicsLineItem(parent)
{

    this->inputNb=inputNb;
    has_source=true;
    this->source=src;
    this->dest=dest;
    setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    label=scene->addText(QString(dest->getNode()->getLabel(inputNb).c_str()));
    label->setParentItem(this);
    initLine();


}
void Edge::initLine(){
    qreal x1,y1;
    x1=dest->boundingRect().x();
    y1=dest->boundingRect().y();
    if(has_source){
        QPointF srcpt;
        
        srcpt=source->mapToItem(this,QPointF(source->boundingRect().x(),source->boundingRect().y()));
		
		if(source->getNode()->className() == std::string("Reader")){
			setLine(x1+(NODE_LENGTH)/2,y1,srcpt.x()+(NODE_LENGTH+PREVIEW_LENGTH)/2,srcpt.y()+NODE_HEIGHT+PREVIEW_HEIGHT);
			label->setPos(((x1+((NODE_LENGTH)/2)+srcpt.x())/2.)-5,((y1+srcpt.y())/2.)-5);
		}
		else{
			setLine(x1+NODE_LENGTH/2,y1,srcpt.x()+NODE_LENGTH/2,srcpt.y()+NODE_HEIGHT);
			label->setPos(((x1+(NODE_LENGTH/2)+srcpt.x())/2.)-5,((y1+srcpt.y())/2.)-5);
		}
        
    }else{
        qreal sourcex,sourcey;

        sourcex=cos(angle)*UNATTACHED_ARROW_LENGTH;
        sourcey=sin(angle)*UNATTACHED_ARROW_LENGTH;


		setLine(x1+NODE_LENGTH/2,y1,x1+(NODE_LENGTH/2)+sourcex,y1-sourcey);
		label->setPos(((x1+(NODE_LENGTH/2)+sourcex)-5),(y1-sourcey)/2.-5);

        

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
    arrowHead << line().p1() << arrowP1 << arrowP2;
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

