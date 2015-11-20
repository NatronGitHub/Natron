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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeGraphTextItem.h"

#include <QDebug>
#include <QStyleOption>

#include "Gui/NodeGraph.h"

#define NODEGRAPH_TEXT_ITEM_MIN_HEIGHT_PX 4
#define NODEGRAPH_SIMPLE_TEXT_ITEM_MIN_HEIGHT_PX 6
#define NODEGRAPH_PIXMAP_ITEM_MIN_HEIGHT_PX 10

NodeGraphTextItem::NodeGraphTextItem(NodeGraph* graph,QGraphicsItem* parent,bool alwaysDrawText)
: QGraphicsTextItem(parent)
, _graph(graph)
, _alwaysDrawText(alwaysDrawText)
{
    QFont f = font();
    f.setStyleStrategy(QFont::NoAntialias);
    setFont(f);
}

NodeGraphTextItem::~NodeGraphTextItem()
{
    
}

void
NodeGraphTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    bool isTooSmall = false;
    if (!_alwaysDrawText) {
        if (_graph->isDoingNavigatorRender()) {
            isTooSmall = true;
        } else {
            
            QFontMetrics fm(font());
            QPoint topLeft = _graph->mapFromScene(mapToScene(QPointF(0,0)));
            QPoint btmLeft = _graph->mapFromScene(mapToScene(QPointF(0, fm.height())));

            int height = std::abs(btmLeft.y() - topLeft.y());
            isTooSmall = height < NODEGRAPH_TEXT_ITEM_MIN_HEIGHT_PX;
        }
    }
    if (isTooSmall) {
        return;
    }
    QGraphicsTextItem::paint(painter, option, widget);
}


NodeGraphSimpleTextItem::NodeGraphSimpleTextItem(NodeGraph* graph,QGraphicsItem* parent,bool alwaysDrawText)
: QGraphicsSimpleTextItem(parent)
, _graph(graph)
, _alwaysDrawText(alwaysDrawText)
{
    QFont f = font();
    f.setStyleStrategy(QFont::NoAntialias);
    setFont(f);
}

NodeGraphSimpleTextItem::~NodeGraphSimpleTextItem()
{
    
}

void
NodeGraphSimpleTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    bool isTooSmall = false;
    if (!_alwaysDrawText) {
        if (_graph->isDoingNavigatorRender()) {
            isTooSmall = true;
        } else {
            QFontMetrics fm(font());
            QPoint topLeft = _graph->mapFromScene(mapToScene(QPointF(0,0)));
            QPointF btmLeft = _graph->mapFromScene(mapToScene(QPointF(0, fm.height())));
            
            int height = std::abs(btmLeft.y() - topLeft.y());

            isTooSmall = height < NODEGRAPH_SIMPLE_TEXT_ITEM_MIN_HEIGHT_PX;

        }
    }
    if (isTooSmall) {
        return;
    }
    QGraphicsSimpleTextItem::paint(painter, option, widget);
}


NodeGraphPixmapItem::NodeGraphPixmapItem(NodeGraph* graph,QGraphicsItem* parent)
: QGraphicsPixmapItem(parent)
, _graph(graph)
{
 
}

NodeGraphPixmapItem::~NodeGraphPixmapItem()
{
    
}

void
NodeGraphPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (_graph->isDoingNavigatorRender()) {
        return;
    }
    QRect br = _graph->mapFromScene(mapToScene(boundingRect()).boundingRect()).boundingRect();
    int height = br.height();
    if (height < NODEGRAPH_PIXMAP_ITEM_MIN_HEIGHT_PX) {
        return;
    }
    QGraphicsPixmapItem::paint(painter, option, widget);
}

