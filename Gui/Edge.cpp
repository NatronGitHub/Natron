/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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
#include <QApplication>
#include <QGraphicsScene>

#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphTextItem.h"
#include "Gui/GuiApplicationManager.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
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
#define ARROW_OFFSET 3 // offset of the arrow tip from the object border
#define ARROW_SIZE_CONNECTED 14
#define ARROW_SIZE_DISCONNECTED 10
#define ARROW_HEAD_ANGLE ((2*M_PI)/15) // 24 degrees opening angle is a nice thin arrow

// number of offset pixels from the arrow that determine if a click is contained in the arrow or not
#define kGraphicalContainerOffset 10

NATRON_NAMESPACE_ENTER;

struct EdgePrivate
{
    Edge* _publicInterface;
    bool isOutputEdge;
    int inputNb;
    double angle;
    NodeGraphSimpleTextItem* label;
    QPolygonF arrowHead;
    boost::weak_ptr<NodeGui> dest;
    boost::weak_ptr<NodeGui> source;
    QColor defaultColor;
    QColor renderingColor;
    bool useRenderingColor;
    bool useHighlight;
    bool paintWithDash;
    bool optional;
    bool paintBendPoint;
    bool bendPointHiddenAutomatically;
    bool enoughSpaceToShowLabel;
    bool isRotoMask;
    bool isMask;
    QPointF middlePoint; //updated only when dest && source are valid

    
    EdgePrivate(Edge* publicInterface, int inputNb, double angle)
    : _publicInterface(publicInterface)
    , isOutputEdge(inputNb == -1)
    , inputNb(inputNb)
    , angle(angle)
    , label(NULL)
    , arrowHead()
    , dest()
    , source()
    , defaultColor(Qt::black)
    , renderingColor(243,149,0)
    , useRenderingColor(false)
    , useHighlight(false)
    , paintWithDash(false)
    , optional(false)
    , paintBendPoint(false)
    , bendPointHiddenAutomatically(false)
    , enoughSpaceToShowLabel(true)
    , isRotoMask(false)
    , isMask(false)
    , middlePoint()
    {
        
    }
    
    void initLabel();
};

Edge::Edge(int inputNb_,
           double angle_,
           const boost::shared_ptr<NodeGui> & dest_,
           QGraphicsItem *parent)
: QGraphicsLineItem(parent)
, _imp(new EdgePrivate(this, inputNb_, angle_))
{
    _imp->dest = dest_;
    
    setPen( QPen(Qt::black, 2, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin) );
    _imp->initLabel();
    setAcceptedMouseButtons(Qt::LeftButton);
    initLine();
    //setFlag(QGraphicsItem::ItemStacksBehindParent);
    setZValue(15);
    refreshState(false);
}

void
EdgePrivate::initLabel()
{
    boost::shared_ptr<NodeGui> dst = dest.lock();
    if ((inputNb != -1) && dst) {
        label = new NodeGraphSimpleTextItem(dst->getDagGui(), _publicInterface, false);
        label->setText(QString(dst->getNode()->getInputLabel(inputNb).c_str()));
        QColor txt;
        double r,g,b;
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        txt.setRgbF(Natron::clamp(r,0.,1.),Natron::clamp(g,0.,1.), Natron::clamp(b,0.,1.));
        label->setBrush(txt);
        QFont f = qApp->font();
        bool antialias = appPTR->getCurrentSettings()->isNodeGraphAntiAliasingEnabled();
        if (!antialias) {
            f.setStyleStrategy(QFont::NoAntialias);
        }
        label->setFont(f);
        //_imp->label->setDefaultTextColor( QColor(200,200,200) );
    }
}

Edge::Edge(const boost::shared_ptr<NodeGui> & src,
           QGraphicsItem *parent)
: QGraphicsLineItem(parent)
, _imp(new EdgePrivate(this, -1, M_PI_2))
{
    _imp->source = src;
    assert(src);
    setPen( QPen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin) );
    setAcceptedMouseButtons(Qt::LeftButton);
    initLine();
    //setFlag(QGraphicsItem::ItemStacksBehindParent);
    setZValue(15);
}

Edge::~Edge()
{
    boost::shared_ptr<NodeGui> dst = _imp->dest.lock();
    if (dst) {
        dst->markInputNull(this);
    }
}

void
Edge::setSource(const boost::shared_ptr<NodeGui> & src)
{
    _imp->source = src;
    initLine();
    refreshState(false);
}

int
Edge::getInputNumber() const
{
    return _imp->inputNb;
}

void
Edge::setInputNumber(int i)
{
    _imp->inputNb = i;
}

boost::shared_ptr<NodeGui>
Edge::getDest() const
{
    return _imp->dest.lock();
}

boost::shared_ptr<NodeGui>
Edge::getSource() const
{
    return _imp->source.lock();
}

bool
Edge::hasSource() const
{
    return _imp->source.lock().get() != NULL;
}

void
Edge::setAngle(double a)
{
    _imp->angle = a;
}

void
Edge::turnOnRenderingColor()
{
    _imp->useRenderingColor = true;
    update();
}

void
Edge::turnOffRenderingColor()
{
    _imp->useRenderingColor = false;
    update();
}

bool
Edge::isOutputEdge() const
{
    return _imp->isOutputEdge;
}

void
Edge::setDefaultColor(const QColor & color)
{
    _imp->defaultColor = color;
}

bool
Edge::isBendPointVisible() const
{
    return _imp->paintBendPoint;
}

bool
Edge::isRotoEdge() const {
    return _imp->isRotoMask;
}

bool
Edge::isMask() const
{
    return _imp->isMask;
}

bool
Edge::areOptionalInputsAutoHidden() const
{
    boost::shared_ptr<NodeGui> dst = _imp->dest.lock();
    return dst ? dst->getDagGui()->areOptionalInputsAutoHidden() : false;
}

void
Edge::setSourceAndDestination(const boost::shared_ptr<NodeGui> & src,
                              const boost::shared_ptr<NodeGui> & dst)
{
    _imp->source = src;
    _imp->dest = dst;
    
    if (!_imp->label) {
        _imp->initLabel();
    } else {
        _imp->label->setText(QString(dst->getNode()->getInputLabel(_imp->inputNb).c_str()));
        //_imp->label->setPlainText( QString( dst->getNode()->getInputLabel(_imp->inputNb).c_str() ) );
    }
    refreshState(false);
    initLine();
}

void
Edge::refreshState(bool hovered)
{
    boost::shared_ptr<NodeGui> dst = _imp->dest.lock();
    
    EffectInstance* effect = dst ? dst->getNode()->getLiveInstance() : 0;
    
    if (effect) {
        
        ///Refresh properties
        _imp->isRotoMask = effect->isInputRotoBrush(_imp->inputNb);
        _imp->isMask = effect->isInputMask(_imp->inputNb);
        _imp->optional = _imp->isMask || effect->isInputOptional(_imp->inputNb);
        if (_imp->isMask) {
            _imp->paintWithDash = true;
        }

        ///Determine whether the edge should be visible or not
        bool hideInputsKnobValue = dst ? dst->getNode()->getHideInputsKnobValue() : false;
        if ((_imp->isRotoMask || hideInputsKnobValue) && !_imp->isOutputEdge) {
            hide();
        } else {
            
            if (_imp->isOutputEdge) {
                show();
            } else {
                
                boost::shared_ptr<NodeGui> src = _imp->source.lock();
                
                //The viewer does not hide its optional edges
                bool isViewer = effect ? dynamic_cast<ViewerInstance*>(effect) != 0 : false;
                bool isReader = effect ? effect->isReader() : false;
                bool autoHide = areOptionalInputsAutoHidden();
                bool isSelected = dst->getIsSelected();
                
                /*
                 * Hide the inputs if it is NOT hovered and NOT selected and auto-hide is enabled and if the node is either
                 * a Reader OR the input is optional and it doesn't have an input node
                 */
                if ( !hovered && !isSelected && autoHide  && !isViewer &&
                    ((_imp->optional && !src) || isReader)) {
                    hide();
                } else {
                    show();
                }
            }
        }
    }
}

void
Edge::setOptional(bool optional)
{
    _imp->optional = optional;
}

void
Edge::setDashed(bool dashed)
{
    _imp->paintWithDash = dashed;
}

void
Edge::setUseHighlight(bool highlight)
{
    _imp->useHighlight = highlight;
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

    boost::shared_ptr<NodeGui> source = _imp->source.lock();
    boost::shared_ptr<NodeGui> dest = _imp->dest.lock();
    if (!source && !dest) {
        return;
    }

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
        srcpt = QPointF( dst.x() + (std::cos(_imp->angle) * 100000 * sc),
                        dst.y() - (std::sin(_imp->angle) * 100000 * sc) );
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
            _imp->middlePoint = (srcInteresect + dstIntersection) / 2;
            ///Hide bend point for short edges
            double visibleLength  = QLineF(srcInteresect,dstIntersection).length();
            if ( (visibleLength < 50) && _imp->paintBendPoint ) {
                _imp->paintBendPoint = false;
                _imp->bendPointHiddenAutomatically = true;
            } else if ( (visibleLength >= 50) && _imp->bendPointHiddenAutomatically ) {
                _imp->bendPointHiddenAutomatically = false;
                _imp->paintBendPoint = true;
            }


            if ( _imp->label ) {
                _imp->label->setPos( _imp->middlePoint + QPointF(-5,-10) );
                QFontMetrics fm( _imp->label->font() );
                int fontHeight = fm.height();
                double txtWidth = fm.width( _imp->label->text() );
                if ( (visibleLength < fontHeight * 2) || (visibleLength < txtWidth) ) {
                    _imp->label->hide();
                    _imp->enoughSpaceToShowLabel = false;
                } else {
                    _imp->label->show();
                    _imp->enoughSpaceToShowLabel = true;
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
        if (foundIntersection) {
            double distToCenter = std::sqrt( ( intersection.x() - dst.x() ) * ( intersection.x() - dst.x() ) +
                                            ( intersection.y() - dst.y() ) * ( intersection.y() - dst.y() ) );
            distToCenter += appPTR->getCurrentSettings()->getDisconnectedArrowLength();

            srcpt = QPointF( dst.x() + (std::cos(_imp->angle) * distToCenter * sc),
                            dst.y() - (std::sin(_imp->angle) * distToCenter * sc) );
            setLine( dst.x(),dst.y(),srcpt.x(),srcpt.y() );

            if (_imp->label) {
                QFontMetrics fm(_imp->label->font());
                double cosinus = std::cos(_imp->angle);
                int yOffset = 0;
                if (cosinus < 0) {
                    yOffset = -fm.width(_imp->label->text());
                } else if ( (cosinus >= -0.01) && (cosinus <= 0.01) ) {
                    yOffset = +5;
                } else {
                    yOffset = +10;
                }

                /*adjusting dst to show label at the middle of the line*/

                QPointF labelDst = dstIntersection;//QPointF( destBBOX.x(),destBBOX.y() ) + QPointF(dstNodeSize.width() / 2.,0);

                _imp->label->setPos( ( ( labelDst.x() + srcpt.x() ) / 2. ) + yOffset,( labelDst.y() + srcpt.y() ) / 2. - 20 );
            }
        }
    }


    double length = std::max(EDGE_LENGTH_MIN, line().length());


    ///This is the angle the edge forms with the X axis
    qreal a = std::acos(line().dx() / length);

    if (line().dy() < 0) {
        a = 2 * M_PI - a;
    }

    QPointF arrowIntersect;
    if (foundDstIntersection) {
        arrowIntersect = dstIntersection + QPointF(std::cos(a) * ARROW_OFFSET * sc,
                                                   std::sin(a) * ARROW_OFFSET * sc);
    } else {
        arrowIntersect = dst;
    }

    qreal arrowSize;
    if (source && dest) {
        arrowSize = TO_DPIX(ARROW_SIZE_CONNECTED) * sc;
    } else {
        arrowSize = TO_DPIX(ARROW_SIZE_DISCONNECTED) * sc;
    }
    QPointF arrowP1 = arrowIntersect + QPointF(std::cos(a + ARROW_HEAD_ANGLE/2) * arrowSize,
                                               std::sin(a + ARROW_HEAD_ANGLE/2) * arrowSize);
    QPointF arrowP2 = arrowIntersect + QPointF(std::cos(a - ARROW_HEAD_ANGLE/2) * arrowSize,
                                               std::sin(a - ARROW_HEAD_ANGLE/2) * arrowSize);
    //_drawLine = QLineF((arrowP1+arrowP2)/2, line().p2());

    _imp->arrowHead.clear();
    _imp->arrowHead << arrowIntersect << arrowP1 << arrowP2;
} // initLine

QPainterPath
Edge::shape() const
{
    QPainterPath path = QGraphicsLineItem::shape();

    path.addPolygon(_imp->arrowHead);


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
    if (line().dy() < 0) {
        a = 2 * M_PI - a;
    }

    double arrowSize = TO_DPIX(ARROW_SIZE_DISCONNECTED);
    QPointF arrowP1 = line().p1() + QPointF(std::cos(a + ARROW_HEAD_ANGLE/2) * arrowSize,
                                            std::sin(a + ARROW_HEAD_ANGLE/2) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(std::cos(a - ARROW_HEAD_ANGLE/2) * arrowSize,
                                            std::sin(a - ARROW_HEAD_ANGLE/2) * arrowSize);
    _imp->arrowHead.clear();
    _imp->arrowHead << line().p1() << arrowP1 << arrowP2;


    if (_imp->label) {
        _imp->label->setPos( QPointF( ( ( line().p1().x() + src.x() ) / 2. ) - 5,( ( line().p1().y() + src.y() ) / 2. ) - 5 ) );
    }
}

void
Edge::dragDest(const QPointF & dst)
{
    setLine( QLineF( dst,line().p2() ) );

    double a = std::acos( line().dx() / std::max(EDGE_LENGTH_MIN, line().length()) );
    if (line().dy() < 0) {
        a = 2 * M_PI - a;
    }

    double arrowSize = TO_DPIX(ARROW_SIZE_DISCONNECTED);
    QPointF arrowP1 = line().p1() + QPointF(std::cos(a + ARROW_HEAD_ANGLE/2) * arrowSize,
                                            std::sin(a + ARROW_HEAD_ANGLE/2) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(std::cos(a - ARROW_HEAD_ANGLE/2) * arrowSize,
                                            std::sin(a - ARROW_HEAD_ANGLE/2) * arrowSize);
    _imp->arrowHead.clear();
    _imp->arrowHead << line().p1() << arrowP1 << arrowP2;
}

void
Edge::setBendPointVisible(bool visible)
{
    _imp->paintBendPoint = visible;
    if (!visible) {
        _imp->bendPointHiddenAutomatically = false;
    }
    update();
}

bool
Edge::isNearbyBendPoint(const QPointF & scenePoint)
{
    assert(_imp->source.lock() && _imp->dest.lock());
    QPointF pos = mapFromScene(scenePoint);
    if ( ( pos.x() >= (_imp->middlePoint.x() - 10) ) && ( pos.x() <= (_imp->middlePoint.x() + 10) ) &&
         ( pos.y() >= (_imp->middlePoint.y() - 10) ) && ( pos.y() <= (_imp->middlePoint.y() + 10) ) ) {
        return true;
    }

    return false;
}


void
Edge::paint(QPainter *painter,
            const QStyleOptionGraphicsItem * /*options*/,
            QWidget * /*parent*/)
{
    bool antialias = appPTR->getCurrentSettings()->isNodeGraphAntiAliasingEnabled();
    if (!antialias) {
        painter->setRenderHint(QPainter::Antialiasing, false);
    }
    
    QPen myPen = pen();
    
    boost::shared_ptr<NodeGui> dst = _imp->dest.lock();
    if (dst) {
        if (dst->getDagGui()->isDoingNavigatorRender()) {
            return;
        }
    }

    if (_imp->paintWithDash) {
        QVector<qreal> dashStyle;
        qreal space = 4;
        dashStyle << 3 << space;
        myPen.setDashPattern(dashStyle);
    } else {
        myPen.setStyle(Qt::SolidLine);
    }

    QColor color, arrowColor;
    if (_imp->useHighlight) {
        color = arrowColor = Qt::green;
    } else if (_imp->useRenderingColor) {
        color = arrowColor = _imp->renderingColor;
    } else {
        color = arrowColor = _imp->defaultColor;
        if (_imp->optional && !_imp->paintWithDash) {
            color.setAlphaF(0.4);
        }
    }
    myPen.setColor(color);
    painter->setPen(myPen);
    
  
    painter->drawLine(line());

    myPen.setStyle(Qt::SolidLine);
    painter->setPen(myPen);

    QPainterPath headPath;
    headPath.addPolygon(_imp->arrowHead);
    headPath.closeSubpath();
    myPen.setColor(arrowColor);
    myPen.setJoinStyle(Qt::MiterJoin);
    painter->fillPath(headPath, color);
    painter->strokePath(headPath, myPen); // also draw the outline, or arrows are too small

    if (_imp->paintBendPoint) {
        QRectF arcRect(_imp->middlePoint.x() - 5,_imp->middlePoint.y() - 5,10,10);
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

    if (line().dy() < 0) {
        a = 2 * M_PI - a;
    }

    qreal arrowSize = 10. * scale();
    QPointF arrowP1 = middle + QPointF(std::cos(a + ARROW_HEAD_ANGLE/2) * arrowSize,
                                       std::sin(a + ARROW_HEAD_ANGLE/2) * arrowSize);
    QPointF arrowP2 = middle + QPointF(std::cos(a - ARROW_HEAD_ANGLE/2) * arrowSize,
                                       std::sin(a - ARROW_HEAD_ANGLE/2) * arrowSize);

    _arrowHead.clear();
    _arrowHead << middle << arrowP1 << arrowP2;
} // refreshPosition

void
LinkArrow::paint(QPainter *painter,
                 const QStyleOptionGraphicsItem* /*options*/,
                 QWidget* /*parent*/)
{
    bool antialias = appPTR->getCurrentSettings()->isNodeGraphAntiAliasingEnabled();
    if (!antialias) {
        painter->setRenderHint(QPainter::Antialiasing, false);
    }
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

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Edge.cpp"
