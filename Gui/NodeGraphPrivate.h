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

#ifndef Gui_NodeGraphPrivate_h
#define Gui_NodeGraphPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QTimer>
#include <QtCore/QMutex>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QPointF>
#include <QColor>
#include <QPen>
#include <QStyleOptionGraphicsItem>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/NodeGraphUndoRedo.h" // NodeGuiPtr
#include "Gui/GuiFwd.h"


#define NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS 1000



#define NATRON_NODE_DUPLICATE_X_OFFSET 50

///These are percentages of the size of the NodeGraph in widget coordinates.
#define NATRON_NAVIGATOR_BASE_HEIGHT 0.2
#define NATRON_NAVIGATOR_BASE_WIDTH 0.2

#define NATRON_SCENE_MAX 1e6
#define NATRON_SCENE_MIN 0

enum EventStateEnum
{
    eEventStateNone,
    eEventStateMovingArea,
    eEventStateZoomingArea,
    eEventStateDraggingArrow,
    eEventStateDraggingNavigator,
    eEventStateDraggingNode,
    eEventStateResizingBackdrop,
    eEventStateSelectionRect,
};

class Navigator
    : public QGraphicsPixmapItem
{
    QGraphicsLineItem* _navLeftEdge;
    QGraphicsLineItem* _navBottomEdge;
    QGraphicsLineItem* _navRightEdge;
    QGraphicsLineItem* _navTopEdge;

public:

    Navigator(QGraphicsItem* parent = 0)
        : QGraphicsPixmapItem(parent)
          , _navLeftEdge(NULL)
          , _navBottomEdge(NULL)
          , _navRightEdge(NULL)
          , _navTopEdge(NULL)
    {
        QPen p;

        p.setBrush( QColor(200,200,200) );
        p.setWidth(2);

        _navLeftEdge = new QGraphicsLineItem(this);
        _navLeftEdge->setPen(p);

        _navBottomEdge = new QGraphicsLineItem(this);
        _navBottomEdge->setPen(p);

        _navRightEdge = new QGraphicsLineItem(this);
        _navRightEdge->setPen(p);

        _navTopEdge = new QGraphicsLineItem(this);
        _navTopEdge->setPen(p);
    }

    virtual ~Navigator()
    {
    }

    int getLineWidth() const
    {
        return _navLeftEdge->pen().width();
    }

    void refreshPosition(const QPointF & navTopLeftScene,
                         double width,
                         double height)
    {
        setPos(navTopLeftScene);

        _navLeftEdge->setLine(0,
                              height,
                              0,
                              0);

        _navTopEdge->setLine(0,
                             0,
                             width,
                             0);

        _navRightEdge->setLine(width,
                               0,
                               width,
                               height);

        _navBottomEdge->setLine(width,
                                height,
                                0,
                                height);
    }
};




class NodeGraphPrivate
{
public:
    NodeGraph* _publicInterface;
    
    boost::weak_ptr<NodeCollection> group;
    
    QPoint _lastMousePos;
    QPointF _lastSelectionStartPointScene;
    EventStateEnum _evtState;
    NodeGuiPtr _magnifiedNode;
    double _nodeSelectedScaleBeforeMagnif;
    bool _magnifOn;
    Edge* _arrowSelected;
    mutable QMutex _nodesMutex;
    NodeGuiList _nodes;
    NodeGuiList _nodesTrash;

    ///Enables the "Tab" shortcut to popup the node creation dialog.
    ///This is set to true on enterEvent and set back to false on leaveEvent
    bool _nodeCreationShortcutEnabled;
    QString _lastNodeCreatedName;
    QGraphicsItem* _root; ///< this is the parent of all items in the graph
    QGraphicsItem* _nodeRoot; ///< this is the parent of all nodes
    QGraphicsSimpleTextItem* _cacheSizeText;
    bool cacheSizeHidden;
    QTimer _refreshCacheTextTimer;
    Navigator* _navigator;
    QUndoStack* _undoStack;
    QMenu* _menu;
    QGraphicsItem *_tL,*_tR,*_bR,*_bL;
    bool _refreshOverlays;

    Edge* _highLightedEdge;
    NodeGuiPtr _mergeHintNode;

    ///This is a hint edge we show when _highLightedEdge is not NULL to display a possible connection.
    Edge* _hintInputEdge;
    Edge* _hintOutputEdge;
    
    NodeGuiPtr _backdropResized; //< the backdrop being resized
    
    NodeGuiList _selection;
    
    std::map<NodeGuiPtr,NodeGuiList> _nodesWithinBDAtPenDown;
    
    QRectF _selectionRect;
    bool _bendPointsVisible;
    bool _knobLinksVisible;
    double _accumDelta;
    bool _detailsVisible;
    QPointF _deltaSinceMousePress; //< mouse delta since last press
    bool _hasMovedOnce;
    
    ViewerTab* lastSelectedViewer;
    
    QPixmap unlockIcon;
    
    ///True when the graph is rendered from the getFullSceneScreenShot() function
    bool isDoingPreviewRender;
    
    QTimer autoScrollTimer;
    
    NodeGraphPrivate(NodeGraph* p,
                     const boost::shared_ptr<NodeCollection>& group);
    
    QPoint getPyPlugUnlockPos() const;
    
    void resetAllClipboards();

    QRectF calcNodesBoundingRect();

    void copyNodesInternal(const NodeGuiList& selection,NodeClipBoard & clipboard);
    void pasteNodesInternal(const NodeClipBoard & clipboard,const QPointF& scenPos,
                            bool useUndoCommand,
                            std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > *newNodes);

    /**
     * @brief Create a new node given the serialization of another one
     * @param offset[in] The offset applied to the new node position relative to the serialized node's position.
     **/
    boost::shared_ptr<NodeGui> pasteNode(const NodeSerialization & internalSerialization,
                                         const NodeGuiSerialization & guiSerialization,
                                         const QPointF & offset,
                                         const boost::shared_ptr<NodeCollection>& group,
                                         const std::string& parentName,
                                         bool clone);


    /**
     * @brief This is called once all nodes of a clipboard have been pasted to try to restore connections between them
     * WARNING: The 2 lists must be ordered the same: each item in serializations corresponds to the same item in the newNodes
     * list. We're not using 2 lists to avoid a copy from the paste function.
     **/
    void restoreConnections(const std::list<boost::shared_ptr<NodeSerialization> > & serializations,
                            const std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > & newNodes);

    void editSelectionFromSelectionRectangle(bool addToSelection);

    void resetSelection();

    void setNodesBendPointsVisible(bool visible);

    void rearrangeSelectedNodes();

    void toggleSelectedNodesEnabled();
    
};

#endif // Gui_NodeGraphPrivate_h
