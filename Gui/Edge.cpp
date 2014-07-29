//  Natron
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
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif

const int graphicalContainerOffset=10; //number of offset pixels from the arrow that determine if a click is contained in the arrow or not

Edge::Edge(int inputNb_, double angle_,const boost::shared_ptr<NodeGui>& dest_, QGraphicsItem *parent)
: QGraphicsLineItem(parent)
, _isOutputEdge(false)
, inputNb(inputNb_)
, angle(angle_)
, label(NULL)
, arrowHead()
, dest(dest_)
, source()
, _defaultColor(Qt::black)
, _renderingColor(243,149,0)
, _useRenderingColor(false)
, _useHighlight(false)
, _paintWithDash(false)
, _paintBendPoint(false)
, _bendPointHiddenAutomatically(false)
, _middlePoint()
{
    setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (inputNb != -1 && dest) {
        label = new QGraphicsTextItem(QString(dest->getNode()->getInputLabel(inputNb).c_str()),this);
        label->setDefaultTextColor(QColor(200,200,200));
    }
    setAcceptedMouseButtons(Qt::LeftButton);
    initLine();
    setFlag(QGraphicsItem::ItemStacksBehindParent);
    setZValue(4);
    if (dest && dest->getNode()->getLiveInstance() && dest->getNode()->getLiveInstance()->isInputOptional(inputNb))
    {
        _paintWithDash = true;
    }
}

Edge::Edge(const boost::shared_ptr<NodeGui>& src,QGraphicsItem *parent)
: QGraphicsLineItem(parent)
, _isOutputEdge(true)
, inputNb(-1)
, angle(M_PI_2)
, label(NULL)
, arrowHead()
, dest()
, source(src)
, _defaultColor(Qt::black)
, _renderingColor(243,149,0)
, _useRenderingColor(false)
, _useHighlight(false)
, _paintWithDash(false)
{
    assert(src);
    setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setAcceptedMouseButtons(Qt::LeftButton);
    initLine();
    setFlag(QGraphicsItem::ItemStacksBehindParent);
    setZValue(4);
   
}

Edge::~Edge()
{
    if (dest) {
        dest->markInputNull(this);
    }
}

void Edge::setSource(const boost::shared_ptr<NodeGui>& src) {
    this->source = src;
    initLine();
}


void Edge::setSourceAndDestination(const boost::shared_ptr<NodeGui>& src,const boost::shared_ptr<NodeGui>& dst) {
    this->source = src;
    this->dest = dst;
    if (!label) {
        label = new QGraphicsTextItem(QString(dest->getNode()->getInputLabel(inputNb).c_str()),this);
        label->setDefaultTextColor(QColor(200,200,200));
    } else {
        label->setPlainText(QString(dest->getNode()->getInputLabel(inputNb).c_str()));
    }
    if (dest && dest->getNode()->getLiveInstance() && dest->getNode()->getLiveInstance()->isInputOptional(inputNb))
    {
        _paintWithDash = true;
    }
    initLine();
}


void Edge::setUseHighlight(bool highlight)
{
    _useHighlight = highlight;
    update();
}

void Edge::initLine()
{
    if (!source && !dest) {
        return;
    }
    
    double sc = scale();

    QRectF sourceBBOX = source ? mapFromItem(source.get(),source->boundingRect()).boundingRect() : QRectF(0,0,1,1);
    QRectF destBBOX = dest ? mapFromItem(dest.get(),dest->boundingRect()).boundingRect()  : QRectF(0,0,1,1);
    
    QSize dstNodeSize;
    QSize srcNodeSize;
    if (dest) {
        dstNodeSize = QSize(destBBOX.width(),destBBOX.height());
    }
    if (source) {
        srcNodeSize = QSize(sourceBBOX.width(),sourceBBOX.height());
    }
    
    QPointF dst;
    
    if (dest) {
        dst = destBBOX.center();
    } else if (source && !dest) {
        dst = QPointF(sourceBBOX.x(),sourceBBOX.y()) + QPointF(srcNodeSize.width() / 2., srcNodeSize.height() + 10);
    }
    
    QLineF dstEdges[4];
    QLineF srcEdges[4];
    if (dest) {
        QPointF dstBBoxTopLeft = destBBOX.topLeft();
        dstEdges[0] = QLineF(dstBBoxTopLeft.x()+dstNodeSize.width(), // right
                          dstBBoxTopLeft.y(),
                          dstBBoxTopLeft.x()+dstNodeSize.width(),
                          dstBBoxTopLeft.y()+dstNodeSize.height());
        dstEdges[1] = QLineF(dstBBoxTopLeft.x()+dstNodeSize.width(), // bottom
                          dstBBoxTopLeft.y()+dstNodeSize.height(),
                          dstBBoxTopLeft.x(),
                          dstBBoxTopLeft.y()+dstNodeSize.height());
        dstEdges[2] = QLineF(dstBBoxTopLeft.x(),  // left
                          dstBBoxTopLeft.y()+dstNodeSize.height(),
                          dstBBoxTopLeft.x(),
                          dstBBoxTopLeft.y());
        dstEdges[3] = QLineF(dstBBoxTopLeft.x(), // top
                          dstBBoxTopLeft.y(),
                          dstBBoxTopLeft.x()+dstNodeSize.width(),
                          dstBBoxTopLeft.y());
    }
    if (source) {
        QPointF srcBBoxTopLeft = sourceBBOX.topLeft();
        srcEdges[0] = QLineF(srcBBoxTopLeft.x()+srcNodeSize.width(), // right
                             srcBBoxTopLeft.y(),
                             srcBBoxTopLeft.x()+srcNodeSize.width(),
                             srcBBoxTopLeft.y()+srcNodeSize.height());
        srcEdges[1] = QLineF(srcBBoxTopLeft.x()+srcNodeSize.width(), // bottom
                             srcBBoxTopLeft.y()+srcNodeSize.height(),
                             srcBBoxTopLeft.x(),
                             srcBBoxTopLeft.y()+srcNodeSize.height());
        srcEdges[2] = QLineF(srcBBoxTopLeft.x(),  // left
                             srcBBoxTopLeft.y()+srcNodeSize.height(),
                             srcBBoxTopLeft.x(),
                             srcBBoxTopLeft.y());
        srcEdges[3] = QLineF(srcBBoxTopLeft.x(), // top
                             srcBBoxTopLeft.y(),
                             srcBBoxTopLeft.x()+srcNodeSize.width(),
                             srcBBoxTopLeft.y());

    }
    
    QPointF srcpt;
    if (source && dest) {
        /////// This is a connected edge, either input or output
        srcpt = sourceBBOX.center();
        
        /////// Only input edges have a label
        if (label) {
            /*adjusting src and dst to show label at the middle of the line*/
            QPointF labelSrcpt= QPointF(sourceBBOX.x(),sourceBBOX.y()) + QPointF(srcNodeSize.width() / 2.,srcNodeSize.height());
            
            
            QPointF labelDst = QPointF(destBBOX.x(),destBBOX.y()) + QPointF(dstNodeSize.width() / 2.,0);
            double norm = sqrt(pow(labelDst.x() - labelSrcpt.x(),2) + pow(labelDst.y() - labelSrcpt.y(),2));
            if(norm > 20.){
                label->setPos((labelDst.x()+labelSrcpt.x())/2.-5.,
                              (labelDst.y()+labelSrcpt.y())/2.-10);
                label->show();
            }else{
                label->hide();
            }
        }
        setLine(dst.x(),dst.y(),srcpt.x(),srcpt.y());
        
        bool foundIntersection = false;

        QPointF dstIntersection;
        for (int i = 0; i < 4; ++i) {
            QLineF::IntersectType type = dstEdges[i].intersect(line(), &dstIntersection);
            if(type == QLineF::BoundedIntersection){
                setLine(QLineF(dstIntersection,line().p2()));
                foundIntersection = true;
                break;
            }
        }
        QPointF srcInteresect;

        if (foundIntersection) {
            ///Find the intersection with the source bbox
            foundIntersection = false;
            for (int i = 0; i < 4; ++i) {
                QLineF::IntersectType type = srcEdges[i].intersect(line(), &srcInteresect);
                if(type == QLineF::BoundedIntersection){
                    foundIntersection = true;
                    break;
                }
            }
        }
        if (foundIntersection) {
            _middlePoint = (srcInteresect + dstIntersection) / 2;
            ///Hide bend point for short edges
            double visibleLength  = QLineF(srcInteresect,dstIntersection).length();
            if (visibleLength < 50 && _paintBendPoint) {
                _paintBendPoint = false;
                _bendPointHiddenAutomatically = true;
            } else if (visibleLength >= 50 && _bendPointHiddenAutomatically) {
                _bendPointHiddenAutomatically = false;
                _paintBendPoint = true;
            }
        }
    
    } else if (!source && dest) {
        ///// The edge is an input edge which is unconnected
        srcpt = QPointF(dst.x() + (std::cos(angle) * 100000 * sc),
                        dst.y() - (std::sin(angle) * 100000 * sc));
        setLine(dst.x(),dst.y(),srcpt.x(),srcpt.y());

        ///ok now that we have the direction between dst and srcPt we can get the distance between the center of the node
        ///and the intersection with the bbox. We add UNATTECHED_ARROW_LENGTH to that distance to position srcPt correctly.
        QPointF intersection;
        bool foundIntersection = false;
        for (int i = 0; i < 4; ++i) {
            QLineF::IntersectType type = dstEdges[i].intersect(line(), &intersection);
            if(type == QLineF::BoundedIntersection){
                setLine(QLineF(intersection,line().p2()));
                foundIntersection = true;
                break;
            }
        }
        
        assert(foundIntersection);
        double distToCenter = std::sqrt((intersection.x() - dst.x()) * (intersection.x() - dst.x()) +
                                        (intersection.y() - dst.y()) * (intersection.y() - dst.y()));
        distToCenter += appPTR->getCurrentSettings()->getDisconnectedArrowLength();
        
        srcpt = QPointF(dst.x() + (std::cos(angle) * distToCenter * sc),
                        dst.y() - (std::sin(angle) * distToCenter * sc));
        setLine(dst.x(),dst.y(),srcpt.x(),srcpt.y());
        
        if (label) {
            double cosinus = std::cos(angle);
            int yOffset = 0;
            if(cosinus < 0){
                yOffset = -40;
            }else if(cosinus >= -0.01 && cosinus <= 0.01){
                
                yOffset = +5;
            }else{
                
                yOffset = +10;
            }
            
            /*adjusting dst to show label at the middle of the line*/
            
            QPointF labelDst = QPointF(destBBOX.x(),destBBOX.y()) + QPointF(dstNodeSize.width() / 2.,0);
            
            label->setPos(((labelDst.x()+srcpt.x())/2.)+yOffset,(labelDst.y()+srcpt.y())/2.-20);
        }
    } else if (source && !dest) {
        ///// The edge is an output edge which is unconnected
        srcpt = QPointF(sourceBBOX.x(),sourceBBOX.y()) + QPointF(srcNodeSize.width() / 2.,srcNodeSize.height() / 2.);
        setLine(dst.x(),dst.y(),srcpt.x(),srcpt.y());

        ///output edges don't have labels
    }
    
    
    double length = line().length();
    
    
    ///This is the angle the edge forms with the X axis
    qreal a = std::acos(line().dx() / length);
    
    if (line().dy() >= 0) {
        a = 2 * M_PI - a;
    }
    
    qreal arrowSize = 5. * sc;
    QPointF arrowP1 = line().p1() + QPointF(std::sin(a + M_PI / 3) * arrowSize,
                                            std::cos(a + M_PI / 3) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(std::sin(a + M_PI - M_PI / 3) * arrowSize,
                                            std::cos(a + M_PI - M_PI / 3) * arrowSize);

    arrowHead.clear();
    arrowHead << dst << arrowP1 << arrowP2;
}


QPainterPath Edge::shape() const
 {
     QPainterPath path = QGraphicsLineItem::shape();
     path.addPolygon(arrowHead);


     return path;
 }
static double dist2(const QPointF& p1,const QPointF& p2){
    return  pow(p2.x() - p1.x(),2) +  pow(p2.y() - p1.y(),2);
}

static double distToSegment(const QLineF& line,const QPointF& p){
    double length = pow(line.length(),2);
    const QPointF& p1 = line.p1();
    const QPointF &p2 = line.p2();
    if(length == 0.)
        dist2(p, p1);
    // Consider the line extending the segment, parameterized as p1 + t (p2 - p1).
    // We find projection of point p onto the line.
    // It falls where t = [(p-p1) . (p2-p1)] / |p2-p1|^2
    double t = ((p.x() - p1.x()) * (p2.x() - p1.x()) + (p.y() - p1.y()) * (p2.y() - p1.y())) / length;
    if (t < 0) return dist2(p, p1);
    if (t > 1) return dist2(p, p2);
    return sqrt(dist2(p, QPointF(p1.x() + t * (p2.x() - p1.x()),
         p1.y() + t * (p2.y() - p1.y()))));
}

bool Edge::contains(const QPointF &point) const{
    double dist = distToSegment(line(), point);
    return  dist <= graphicalContainerOffset;
}
void Edge::dragSource(const QPointF& src)
{
    
    setLine(QLineF(line().p1(),src));

    double a = std::acos(line().dx() / line().length());
    if (line().dy() >= 0)
        a = 2 * M_PI - a;

    double arrowSize = 5;
    QPointF arrowP1 = line().p1() + QPointF(std::sin(a + M_PI / 3) * arrowSize,
                                            std::cos(a + M_PI / 3) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(std::sin(a + M_PI - M_PI / 3) * arrowSize,
                                            std::cos(a + M_PI - M_PI / 3) * arrowSize);
    arrowHead.clear();
	arrowHead << line().p1() << arrowP1 << arrowP2;

    
    if (label) {
        label->setPos(QPointF(((line().p1().x()+src.x())/2.)-5,((line().p1().y()+src.y())/2.)-5));
    }
}

void Edge::dragDest(const QPointF& dst)
{
    setLine(QLineF(dst,line().p2()));

    double a = std::acos(line().dx() / line().length());
    if (line().dy() >= 0)
        a = 2 * M_PI - a;
    
    double arrowSize = 5;
    QPointF arrowP1 = line().p1() + QPointF(std::sin(a + M_PI / 3) * arrowSize,
                                            std::cos(a + M_PI / 3) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(std::sin(a + M_PI - M_PI / 3) * arrowSize,
                                            std::cos(a + M_PI - M_PI / 3) * arrowSize);
    arrowHead.clear();
	arrowHead << line().p1() << arrowP1 << arrowP2;
    
}

void Edge::setBendPointVisible(bool visible)
{
    _paintBendPoint = visible;
    if (!visible) {
        _bendPointHiddenAutomatically = false;
    }
    update();
}

bool Edge::isNearbyBendPoint(const QPointF& scenePoint)
{
    assert(source && dest);
    QPointF pos =mapFromScene(scenePoint);
    if (pos.x() >= (_middlePoint.x() - 10) && pos.x() <= (_middlePoint.x() + 10) &&
        pos.y() >= (_middlePoint.y() - 10) && pos.y() <= (_middlePoint.y() + 10)) {
        return true;
    }
    return false;
}

void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*options*/,
           QWidget * /*parent*/)
 {

     QPen myPen = pen();
     
     if (_paintWithDash) {
         QVector<qreal> dashStyle;
         qreal space = 4;
         dashStyle << 3 << space;
         myPen.setDashPattern(dashStyle);
     }else{
         myPen.setStyle(Qt::SolidLine);
     }
     
     QColor color;
     if (_useHighlight) {
         color = Qt::green;
     } else if (_useRenderingColor) {
         color = _renderingColor;
     } else {
         color = _defaultColor;
     }
     myPen.setColor(color);
     painter->setPen(myPen);
     QLineF l = line();
     painter->drawLine(l);

     myPen.setStyle(Qt::SolidLine);
     painter->setPen(myPen);
     
     QPainterPath headPath;
     headPath.addPolygon(arrowHead);
     headPath.closeSubpath();
     painter->fillPath(headPath, color);
     
     if (_paintBendPoint) {
         QRectF arcRect(_middlePoint.x() - 5,_middlePoint.y() - 5,10,10);
         QPainterPath bendPointPath;
         bendPointPath.addEllipse(arcRect);
         bendPointPath.closeSubpath();
         painter->fillPath(bendPointPath,Qt::yellow);
     }

  }

