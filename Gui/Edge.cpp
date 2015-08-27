/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Edge.h"

#include <algorithm> // min, max
#include <cmath>

#include <QPainter>
#include <QGraphicsScene>

#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif
#ifndef M_PI_4
#define M_PI_4      0.785398163397448309615660845819875721  /* pi/4           */
#endif

#define EDGE_LENGTH_MIN 0.1

// number of offset pixels from the arrow that determine if a click is contained in the arrow or not
#define kGraphicalContainerOffset 10


Edge::Edge(int inputNb_,
           double angle_,
           const boost::shared_ptr<NodeGui> & dest_,
           QGraphicsItem *parent)
: QGraphicsLineItem(parent)
, _isOutputEdge(false)
, _inputNb(inputNb_)
, _angle(angle_)
, _label(NULL)
, _arrowHead()
, _dest(dest_)
, _source()
, _defaultColor(Qt::black)
, _renderingColor(243,149,0)
, _useRenderingColor(false)
, _useHighlight(false)
, _paintWithDash(false)
, _optional(false)
, _paintBendPoint(false)
, _bendPointHiddenAutomatically(false)
, _enoughSpaceToShowLabel(true)
, _isRotoMask(false)
, _isMask(false)
, _middlePoint()
{
    setPen( QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin) );
    if ( (_inputNb != -1) && dest_ ) {
        _label = new QGraphicsTextItem(QString( dest_->getNode()->getInputLabel(_inputNb).c_str() ),this);
        _label->setDefaultTextColor( QColor(200,200,200) );
    }
    setAcceptedMouseButtons(Qt::LeftButton);
    initLine();
    //setFlag(QGraphicsItem::ItemStacksBehindParent);
    setZValue(15);
    Natron::EffectInstance* effect = dest_ ? dest_->getNode()->getLiveInstance() : 0;
    if (effect) {
        
        _isRotoMask = effect->isInputRotoBrush(_inputNb);
        _isMask = effect->isInputMask(_inputNb);
        bool autoHide = areOptionalInputsAutoHidden();
        bool isSelected = dest_->getIsSelected();
        if (_isMask) {
            setDashed(true);
            setOptional(true);
            if (!isSelected && autoHide && _isMask) {
                hide();
            }
        } else if (effect->isInputOptional(_inputNb)) {
            setOptional(true);
            if (!isSelected && autoHide  && _isMask) {
                hide();
            }
        }
    }
}

Edge::Edge(const boost::shared_ptr<NodeGui> & src,
           QGraphicsItem *parent)
: QGraphicsLineItem(parent)
, _isOutputEdge(true)
, _inputNb(-1)
, _angle(M_PI_2)
, _label(NULL)
, _arrowHead()
, _dest()
, _source(src)
, _defaultColor(Qt::black)
, _renderingColor(243,149,0)
, _useRenderingColor(false)
, _useHighlight(false)
, _paintWithDash(false)
, _optional(false)
, _paintBendPoint(false)
, _bendPointHiddenAutomatically(false)
, _enoughSpaceToShowLabel(true)
, _isRotoMask(false)
, _isMask(false)
, _middlePoint()
{
    assert(src);
    setPen( QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin) );
    setAcceptedMouseButtons(Qt::LeftButton);
    initLine();
    //setFlag(QGraphicsItem::ItemStacksBehindParent);
    setZValue(15);
}

Edge::~Edge()
{
    boost::shared_ptr<NodeGui> dst = _dest.lock();
    if (dst) {
        dst->markInputNull(this);
    }
}

void
Edge::setSource(const boost::shared_ptr<NodeGui> & src)
{
    _source = src;
    bool autoHide = areOptionalInputsAutoHidden();

    boost::shared_ptr<NodeGui> dst = _dest.lock();
    assert(dst);
    bool isSelected = dst->getIsSelected();
    
    bool isViewer = false;
    if (src) {
        boost::shared_ptr<Natron::Node> internalNode = src->getNode();
        isViewer = dynamic_cast<InspectorNode*>(internalNode.get());
    }
    if (autoHide && _optional  && !_isRotoMask && !isViewer) {

        if (src || isSelected || !_isMask) {
            show();
        } else {
            hide();
        }
    }
    initLine();
}

bool
Edge::areOptionalInputsAutoHidden() const
{
    boost::shared_ptr<NodeGui> dst = _dest.lock();
    return dst ? dst->getDagGui()->areOptionalInputsAutoHidden() : false;
}

void
Edge::setSourceAndDestination(const boost::shared_ptr<NodeGui> & src,
                              const boost::shared_ptr<NodeGui> & dst)
{
    _source = src;
    _dest = dst;
    
    Natron::EffectInstance* effect = dst ? dst->getNode()->getLiveInstance() : 0;

    if (effect) {
        _isRotoMask = effect->isInputRotoBrush(_inputNb);
    }
    
    if (!_label) {
        _label = new QGraphicsTextItem(QString( dst->getNode()->getInputLabel(_inputNb).c_str() ),this);
        _label->setDefaultTextColor( QColor(200,200,200) );
    } else {
        _label->setPlainText( QString( dst->getNode()->getInputLabel(_inputNb).c_str() ) );
    }
    if (effect) {
        bool autoHide = areOptionalInputsAutoHidden();
        bool isSelected = dst->getIsSelected();
        if (effect->isInputMask(_inputNb) && !_isRotoMask) {
            setDashed(true);
            setOptional(true);
            if (autoHide) {
                if (!isSelected && !src) {
                    hide();
                } else {
                    show();
                }
            }
        } else if (effect->isInputOptional(_inputNb)  && !_isRotoMask) {
            setOptional(true);
            if (!isSelected && autoHide) {
                if (!src) {
                    hide();
                } else {
                    show();
                }
            }
        }
    }
    initLine();
}

void
Edge::setOptional(bool optional)
{
    _optional = optional;
}

void
Edge::setDashed(bool dashed)
{
    _paintWithDash = dashed;
}

void
Edge::setUseHighlight(bool highlight)
{
    _useHighlight = highlight;
    update();
}

static void
makeEdges(const QRectF & bbox,
          std::vector<QLineF> & edges)
{
    QPointF topLeft = bbox.topLeft();

    edges.push_back( QLineF( topLeft.x() + bbox.width(), // right
                             topLeft.y(),
                             topLeft.x() + bbox.width(),
                             topLeft.y() + bbox.height() ) );

    edges.push_back( QLineF( topLeft.x() + bbox.width(), // bottom
                             topLeft.y() + bbox.height(),
                             topLeft.x(),
                             topLeft.y() + bbox.height() ) );

    edges.push_back( QLineF( topLeft.x(),  // left
                             topLeft.y() + bbox.height(),
                             topLeft.x(),
                             topLeft.y() ) );

    edges.push_back( QLineF( topLeft.x(), // top
                             topLeft.y(),
                             topLeft.x() + bbox.width(),
                             topLeft.y() ) );
}

void
Edge::initLine()
{
    if (!_source.lock() && !_dest.lock()) {
        return;
    }
    
    boost::shared_ptr<NodeGui> source = _source.lock();
    boost::shared_ptr<NodeGui> dest = _dest.lock();

    double sc = scale();
    QRectF sourceBBOX = source ? mapFromItem( source.get(), source->boundingRect() ).boundingRect() : QRectF(0,0,1,1);
    QRectF destBBOX = dest ? mapFromItem( dest.get(), dest->boundingRect() ).boundingRect()  : QRectF(0,0,1,1);
    QSize dstNodeSize;
    QSize srcNodeSize;
    if (dest) {
        dstNodeSize = QSize( destBBOX.width(),destBBOX.height() );
    }
    if (source) {
        srcNodeSize = QSize( sourceBBOX.width(),sourceBBOX.height() );
    }

    QPointF dst;

    if (dest) {
        dst = destBBOX.center();
    } else if (source && !dest) {
        dst = QPointF( sourceBBOX.x(),sourceBBOX.y() ) + QPointF(srcNodeSize.width() / 2., srcNodeSize.height() + 10);
    }
    
    std::vector<QLineF> dstEdges;
    std::vector<QLineF> srcEdges;
    if (dest) {
        makeEdges(destBBOX, dstEdges);
    }
    if (source) {
        makeEdges(sourceBBOX, srcEdges);
    }
    
    
    QPointF srcpt;
    
    if (source && dest) {
        /// This is a connected edge, either input or output
        srcpt = sourceBBOX.center();
        setLine( dst.x(),dst.y(),srcpt.x(),srcpt.y() );
    } else if (!source && dest) {
        /// The edge is an input edge which is unconnected
        srcpt = QPointF( dst.x() + (std::cos(_angle) * 100000 * sc),
                        dst.y() - (std::sin(_angle) * 100000 * sc) );
        setLine( dst.x(),dst.y(),srcpt.x(),srcpt.y() );
    } else if (source && !dest) {
        /// The edge is an output edge which is unconnected
        srcpt = QPointF( sourceBBOX.x(),sourceBBOX.y() ) + QPointF(srcNodeSize.width() / 2.,srcNodeSize.height() / 2.);
        setLine( dst.x(),dst.y(),srcpt.x(),srcpt.y() );
        
    }
    
    bool foundDstIntersection = false;
    
    QPointF dstIntersection;
    
    if (dest) {
        for (int i = 0; i < 4; ++i) {
            QLineF::IntersectType type = dstEdges[i].intersect(line(), &dstIntersection);
            if (type == QLineF::BoundedIntersection) {
                setLine( QLineF( dstIntersection,line().p2() ) );
                foundDstIntersection = true;
                break;
            }
        }
    }
    
    if (source && dest) {

        QPointF srcInteresect;
        bool foundSrcIntersection = false;
        if (foundDstIntersection) {
            ///Find the intersection with the source bbox
            for (int i = 0; i < 4; ++i) {
                QLineF::IntersectType type = srcEdges[i].intersect(line(), &srcInteresect);
                if (type == QLineF::BoundedIntersection) {
                    foundSrcIntersection = true;
                    break;
                }
            }
        }
        if (foundSrcIntersection) {
            _middlePoint = (srcInteresect + dstIntersection) / 2;
            ///Hide bend point for short edges
            double visibleLength  = QLineF(srcInteresect,dstIntersection).length();
            if ( (visibleLength < 50) && _paintBendPoint ) {
                _paintBendPoint = false;
                _bendPointHiddenAutomatically = true;
            } else if ( (visibleLength >= 50) && _bendPointHiddenAutomatically ) {
                _bendPointHiddenAutomatically = false;
                _paintBendPoint = true;
            }


            if ( _label ) {
                _label->setPos( _middlePoint + QPointF(-5,-10) );
                QFontMetrics fm( _label->font() );
                int fontHeight = fm.height();
                double txtWidth = fm.width( _label->toPlainText() );
                if ( (visibleLength < fontHeight * 2) || (visibleLength < txtWidth) ) {
                    _label->hide();
                    _enoughSpaceToShowLabel = false;
                } else {
                    _label->show();
                    _enoughSpaceToShowLabel = true;
                }
            }
        }
    } else if (!source && dest) {

        ///ok now that we have the direction between dst and srcPt we can get the distance between the center of the node
        ///and the intersection with the bbox. We add UNATTECHED_ARROW_LENGTH to that distance to position srcPt correctly.
        QPointF intersection;
        bool foundIntersection = false;
        for (int i = 0; i < 4; ++i) {
            QLineF::IntersectType type = dstEdges[i].intersect(line(), &intersection);
            if (type == QLineF::BoundedIntersection) {
                setLine( QLineF( intersection,line().p2() ) );
                foundIntersection = true;
                break;
            }
        }

        assert(foundIntersection);
        double distToCenter = std::sqrt( ( intersection.x() - dst.x() ) * ( intersection.x() - dst.x() ) +
                                         ( intersection.y() - dst.y() ) * ( intersection.y() - dst.y() ) );
        distToCenter += appPTR->getCurrentSettings()->getDisconnectedArrowLength();

        srcpt = QPointF( dst.x() + (std::cos(_angle) * distToCenter * sc),
                         dst.y() - (std::sin(_angle) * distToCenter * sc) );
        setLine( dst.x(),dst.y(),srcpt.x(),srcpt.y() );

        if (_label) {
            QFontMetrics fm(_label->font());
            double cosinus = std::cos(_angle);
            int yOffset = 0;
            if (cosinus < 0) {
                yOffset = -fm.width(_label->toPlainText());
            } else if ( (cosinus >= -0.01) && (cosinus <= 0.01) ) {
                yOffset = +5;
            } else {
                yOffset = +10;
            }

            /*adjusting dst to show label at the middle of the line*/

            QPointF labelDst = dstIntersection;//QPointF( destBBOX.x(),destBBOX.y() ) + QPointF(dstNodeSize.width() / 2.,0);

            _label->setPos( ( ( labelDst.x() + srcpt.x() ) / 2. ) + yOffset,( labelDst.y() + srcpt.y() ) / 2. - 20 );
        }
    } 


    double length = std::max(EDGE_LENGTH_MIN, line().length());


    ///This is the angle the edge forms with the X axis
    qreal a = std::acos(line().dx() / length);

    if (line().dy() >= 0) {
        a = 2 * M_PI - a;
    }
    
    QPointF arrowIntersect = foundDstIntersection ? dstIntersection : dst;

    qreal arrowSize;
    if (source && dest) {
        arrowSize = 10. * sc;
    } else {
        arrowSize = 7. * sc;
    }
    double headAngle = 3. * M_PI_4;
    QPointF arrowP1 = arrowIntersect + QPointF(std::sin(a + headAngle) * arrowSize,
                                            std::cos(a + headAngle) * arrowSize);
    QPointF arrowP2 = arrowIntersect + QPointF(std::sin(a + M_PI - headAngle) * arrowSize,
                                            std::cos(a + M_PI - headAngle) * arrowSize);

    _arrowHead.clear();
    _arrowHead << arrowIntersect << arrowP1 << arrowP2;
} // initLine

QPainterPath
Edge::shape() const
{
    QPainterPath path = QGraphicsLineItem::shape();

    path.addPolygon(_arrowHead);


    return path;
}

static inline double
sqr(double x)
{
    return x * x;
}

static double
dist2(const QPointF & p1,
      const QPointF & p2)
{
    return sqr( p2.x() - p1.x() ) +  sqr( p2.y() - p1.y() );
}

static double
dist2ToSegment(const QLineF & line,
               const QPointF & p)
{
    double length2 = line.length() * line.length();
    const QPointF & p1 = line.p1();
    const QPointF &p2 = line.p2();

    if (length2 == 0.) {
        return dist2(p, p1);
    }
    // Consider the line extending the segment, parameterized as p1 + t (p2 - p1).
    // We find projection of point p onto the line.
    // It falls where t = [(p-p1) . (p2-p1)] / |p2-p1|^2
    double t = ( ( p.x() - p1.x() ) * ( p2.x() - p1.x() ) + ( p.y() - p1.y() ) * ( p2.y() - p1.y() ) ) / length2;
    if (t < 0) {
        return dist2(p, p1);
    }
    if (t > 1) {
        return dist2(p, p2);
    }

    return dist2( p, QPointF( p1.x() + t * ( p2.x() - p1.x() ),
                              p1.y() + t * ( p2.y() - p1.y() ) ) );
}

bool
Edge::contains(const QPointF &point) const
{
    double d2 = dist2ToSegment(line(), point);

    return d2 <= kGraphicalContainerOffset * kGraphicalContainerOffset;
}

void
Edge::dragSource(const QPointF & src)
{
    setLine( QLineF(line().p1(),src) );

    double a = std::acos( line().dx() / std::max(EDGE_LENGTH_MIN, line().length()) );
    if (line().dy() >= 0) {
        a = 2 * M_PI - a;
    }

    double arrowSize = 5;
    QPointF arrowP1 = line().p1() + QPointF(std::sin(a + M_PI / 3) * arrowSize,
                                            std::cos(a + M_PI / 3) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(std::sin(a + 2 * M_PI / 3) * arrowSize,
                                            std::cos(a + 2 * M_PI / 3) * arrowSize);
    _arrowHead.clear();
    _arrowHead << line().p1() << arrowP1 << arrowP2;


    if (_label) {
        _label->setPos( QPointF( ( ( line().p1().x() + src.x() ) / 2. ) - 5,( ( line().p1().y() + src.y() ) / 2. ) - 5 ) );
    }
}

void
Edge::dragDest(const QPointF & dst)
{
    setLine( QLineF( dst,line().p2() ) );

    double a = std::acos( line().dx() / std::max(EDGE_LENGTH_MIN, line().length()) );
    if (line().dy() >= 0) {
        a = 2 * M_PI - a;
    }

    double arrowSize = 5;
    QPointF arrowP1 = line().p1() + QPointF(std::sin(a + M_PI / 3) * arrowSize,
                                            std::cos(a + M_PI / 3) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(std::sin(a + 2 * M_PI / 3) * arrowSize,
                                            std::cos(a + 2 * M_PI / 3) * arrowSize);
    _arrowHead.clear();
    _arrowHead << line().p1() << arrowP1 << arrowP2;
}

void
Edge::setBendPointVisible(bool visible)
{
    _paintBendPoint = visible;
    if (!visible) {
        _bendPointHiddenAutomatically = false;
    }
    update();
}

bool
Edge::isNearbyBendPoint(const QPointF & scenePoint)
{
    assert(_source.lock() && _dest.lock());
    QPointF pos = mapFromScene(scenePoint);
    if ( ( pos.x() >= (_middlePoint.x() - 10) ) && ( pos.x() <= (_middlePoint.x() + 10) ) &&
         ( pos.y() >= (_middlePoint.y() - 10) ) && ( pos.y() <= (_middlePoint.y() + 10) ) ) {
        return true;
    }

    return false;
}

void
Edge::setVisibleDetails(bool visible)
{
    if (!visible) {
        _label->hide();
    } else {
        if (_enoughSpaceToShowLabel) {
            _label->show();
        }
    }
}

void
Edge::paint(QPainter *painter,
            const QStyleOptionGraphicsItem * /*options*/,
            QWidget * /*parent*/)
{
    QPen myPen = pen();

    if (_paintWithDash) {
        QVector<qreal> dashStyle;
        qreal space = 4;
        dashStyle << 3 << space;
        myPen.setDashPattern(dashStyle);
    } else {
        myPen.setStyle(Qt::SolidLine);
    }

    QColor color;
    if (_useHighlight) {
        color = Qt::green;
    } else if (_useRenderingColor) {
        color = _renderingColor;
    } else {
        color = _defaultColor;
        if (_optional && !_paintWithDash) {
            color.setAlphaF(0.4);
        }
    }
    myPen.setColor(color);
    painter->setPen(myPen);
    
  
    painter->drawLine(line());

    myPen.setStyle(Qt::SolidLine);
    painter->setPen(myPen);

    QPainterPath headPath;
    headPath.addPolygon(_arrowHead);
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

LinkArrow::LinkArrow(const NodeGui* master,
                     const NodeGui* slave,
                     QGraphicsItem* parent)
    : QObject(), QGraphicsLineItem(parent)
      , _master(master)
      , _slave(slave)
      , _arrowHead()
      , _renderColor(Qt::black)
      , _headColor(Qt::white)
      , _lineWidth(1)
{
    assert(master && slave);
    QObject::connect( master,SIGNAL( positionChanged(int,int) ),this,SLOT( refreshPosition() ) );
    QObject::connect( slave,SIGNAL( positionChanged(int,int) ),this,SLOT( refreshPosition() ) );

    refreshPosition();
    setZValue(master->zValue() - 5);
}

LinkArrow::~LinkArrow()
{
}

void
LinkArrow::setColor(const QColor & color)
{
    _renderColor = color;
}

void
LinkArrow::setArrowHeadColor(const QColor & headColor)
{
    _headColor = headColor;
}

void
LinkArrow::setWidth(int lineWidth)
{
    _lineWidth = lineWidth;
}

void
LinkArrow::refreshPosition()
{
    QRectF bboxSlave = mapFromItem( _slave,_slave->boundingRect() ).boundingRect();

    ///like the box master in kfc! was bound to name it so I'm hungry atm
    QRectF boxMaster = mapFromItem( _master,_master->boundingRect() ).boundingRect();
    QPointF dst = boxMaster.center();
    QPointF src = bboxSlave.center();

    setLine( QLineF(src,dst) );

    ///Get the intersections of the line with the nodes
    std::vector<QLineF> masterEdges;
    std::vector<QLineF> slaveEdges;
    makeEdges(bboxSlave, slaveEdges);
    makeEdges(boxMaster, masterEdges);


    QPointF masterIntersect;
    QPointF slaveIntersect;
    bool foundIntersection = false;
    for (int i = 0; i < 4; ++i) {
        QLineF::IntersectType type = slaveEdges[i].intersect(line(), &slaveIntersect);
        if (type == QLineF::BoundedIntersection) {
            foundIntersection = true;
            break;
        }
    }

    if (!foundIntersection) {
        ///Don't bother continuing, there's no intersection that means the line is contained in the node bbox

        ///hence that the 2 nodes are overlapping on the nodegraph so probably we wouldn't see the link anyway.
        return;
    }

    foundIntersection = false;
    for (int i = 0; i < 4; ++i) {
        QLineF::IntersectType type = masterEdges[i].intersect(line(), &masterIntersect);
        if (type == QLineF::BoundedIntersection) {
            foundIntersection = true;
            break;
        }
    }
    if (!foundIntersection) {
        ///Don't bother continuing, there's no intersection that means the line is contained in the node bbox

        ///hence that the 2 nodes are overlapping on the nodegraph so probably we wouldn't see the link anyway.
        return;
    }

    ///Now get the middle of the visible portion of the link
    QPointF middle = (masterIntersect + slaveIntersect) / 2.;
    double length = std::max(EDGE_LENGTH_MIN, line().length());
    ///This is the angle the edge forms with the X axis
    qreal a = std::acos(line().dx() / length);

    if (line().dy() >= 0) {
        a = 2 * M_PI - a;
    }

    qreal arrowSize = 10. * scale();
    QPointF arrowP1 = middle + QPointF(std::sin(a + M_PI / 3) * arrowSize,
                                       std::cos(a + M_PI / 3) * arrowSize);
    QPointF arrowP2 = middle + QPointF(std::sin(a + 2 * M_PI / 3) * arrowSize,
                                       std::cos(a + 2 * M_PI / 3) * arrowSize);

    _arrowHead.clear();
    _arrowHead << middle << arrowP1 << arrowP2;
} // refreshPosition

void
LinkArrow::paint(QPainter *painter,
                 const QStyleOptionGraphicsItem* /*options*/,
                 QWidget* /*parent*/)
{
    QPen myPen = pen();

    myPen.setColor(_renderColor);
    myPen.setWidth(_lineWidth);
    painter->setPen(myPen);
    QLineF l = line();
    painter->drawLine(l);

    myPen.setStyle(Qt::SolidLine);
    painter->setPen(myPen);

    QPainterPath headPath;
    headPath.addPolygon(_arrowHead);
    headPath.closeSubpath();
    painter->fillPath(headPath, _headColor);
}

