//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GUI_EDGE_H_
#define NATRON_GUI_EDGE_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGraphicsLineItem>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
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

class Edge
    : public QGraphicsLineItem
{
public:

    ///Used to make an input edge
    Edge(int inputNb,
         double angle,
         const boost::shared_ptr<NodeGui> & dest,
         QGraphicsItem *parent = 0);

    ///Used to make an output edge
    Edge(const boost::shared_ptr<NodeGui> & src,
         QGraphicsItem *parent = 0);

    virtual ~Edge() OVERRIDE;

    QPainterPath shape() const OVERRIDE WARN_UNUSED_RETURN;

    bool contains(const QPointF &point) const OVERRIDE WARN_UNUSED_RETURN;

    void setSource(const boost::shared_ptr<NodeGui> & src);
    
    void setVisibleDetails(bool visible);

    void setSourceAndDestination(const boost::shared_ptr<NodeGui> & src,const boost::shared_ptr<NodeGui> & dst);

    int getInputNumber() const
    {
        return _inputNb;
    }

    void setInputNumber(int i)
    {
        _inputNb = i;
    }

    boost::shared_ptr<NodeGui> getDest() const
    {
        return _dest.lock();
    }

    boost::shared_ptr<NodeGui> getSource() const
    {
        return _source.lock();
    }

    bool hasSource() const
    {
        return _source.lock().get() != NULL;
    }

    void dragSource(const QPointF & src);

    void dragDest(const QPointF & dst);

    void initLine();

    void setAngle(double a)
    {
        _angle = a;
    }

    void turnOnRenderingColor()
    {
        _useRenderingColor = true;
        update();
    }

    void turnOffRenderingColor()
    {
        _useRenderingColor = false;
        update();
    }

    void setUseHighlight(bool highlight);

    bool isOutputEdge() const
    {
        return _isOutputEdge;
    }

    void setDefaultColor(const QColor & color)
    {
        _defaultColor = color;
    }
    
    bool areOptionalInputsAutoHidden() const;
    
    void setOptional(bool optional);
    
    void setDashed(bool dashed);

    void setBendPointVisible(bool visible);

    bool isBendPointVisible() const
    {
        return _paintBendPoint;
    }

    bool isNearbyBendPoint(const QPointF & scenePoint);

    bool isRotoEdge() const {
        return _isRotoMask;
    }
private:

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *options,QWidget *parent = 0) OVERRIDE FINAL;
    bool _isOutputEdge;
    int _inputNb;
    double _angle;
    QGraphicsTextItem* _label;
    QPolygonF _arrowHead;
    boost::weak_ptr<NodeGui> _dest;
    boost::weak_ptr<NodeGui> _source;
    QColor _defaultColor;
    QColor _renderingColor;
    bool _useRenderingColor;
    bool _useHighlight;
    bool _paintWithDash;
    bool _optional;
    bool _paintBendPoint;
    bool _bendPointHiddenAutomatically;
    bool _enoughSpaceToShowLabel;
    bool _isRotoMask;
    bool _isMask;
    QPointF _middlePoint; //updated only when dest && source are valid
};

/**
 * @brief An arrow in the graph representing an expression between 2 nodes or that one node is a clone of another.
 **/
class LinkArrow
    : public QObject, public QGraphicsLineItem
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    LinkArrow(const NodeGui* master,
              const NodeGui* slave,
              QGraphicsItem* parent);

    virtual ~LinkArrow();

    void setColor(const QColor & color);

    void setArrowHeadColor(const QColor & headColor);

    void setWidth(int lineWidth);

public Q_SLOTS:

    /**
     * @brief Called when one of the 2 nodes is moved
     **/
    void refreshPosition();

private:

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *options,QWidget *parent = 0) OVERRIDE FINAL;
    const NodeGui* _master;
    const NodeGui* _slave;
    QPolygonF _arrowHead;
    QColor _renderColor;
    QColor _headColor;
    int _lineWidth;
};

#endif // NATRON_GUI_EDGE_H_
