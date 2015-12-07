/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_EDGE_H
#define NATRON_GUI_EDGE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGraphicsLineItem>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"


struct EdgePrivate;
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
    
    void setSourceAndDestination(const boost::shared_ptr<NodeGui> & src,const boost::shared_ptr<NodeGui> & dst);

    int getInputNumber() const;

    void setInputNumber(int i);

    boost::shared_ptr<NodeGui> getDest() const;
    boost::shared_ptr<NodeGui> getSource() const;

    bool hasSource() const;

    void dragSource(const QPointF & src);

    void dragDest(const QPointF & dst);

    void initLine();

    void setAngle(double a);

    void turnOnRenderingColor();

    void turnOffRenderingColor();

    void setUseHighlight(bool highlight);

    bool isOutputEdge() const;

    void setDefaultColor(const QColor & color);
    
    bool isBendPointVisible() const;
    
    bool areOptionalInputsAutoHidden() const;
    
    void setOptional(bool optional);
    
    void setDashed(bool dashed);

    void setBendPointVisible(bool visible);
    
    bool isRotoEdge() const;
    
    bool isMask() const;

    bool isNearbyBendPoint(const QPointF & scenePoint);
    
    /**
     * @brief Refresh the Edge properties such as dashed state, optional state, visibility etc...
     * @param hovered True if the mouse is hovering the dst node (the node holding this edge)
     **/
    void refreshState(bool hovered);

    
private:

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *options,QWidget *parent = 0) OVERRIDE FINAL;
    
        
    boost::scoped_ptr<EdgePrivate> _imp;
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

#endif // NATRON_GUI_EDGE_H
