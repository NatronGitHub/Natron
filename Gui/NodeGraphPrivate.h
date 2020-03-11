/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include <set>

#include <set>
#include <list>
#include <map>
#include <utility>
#include <string>

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
#include <QtCore/QPointF>
#include <QColor>
#include <QPen>
#include <QStyleOptionGraphicsItem>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/NodeGraphUndoRedo.h" // NodeGuiPtr
#include "Gui/GuiFwd.h"


#define NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS 1000
#define NATRON_NODES_RENDER_STATE_REFRESH_INTERVAL_MS 300

#define NATRON_NODE_DUPLICATE_X_OFFSET 50

///These are percentages of the size of the NodeGraph in widget coordinates.
#define NATRON_NAVIGATOR_BASE_HEIGHT 0.2
#define NATRON_NAVIGATOR_BASE_WIDTH 0.2

#define NATRON_SCENE_MAX 1e6
#define NATRON_SCENE_MIN 0

NATRON_NAMESPACE_ENTER

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

        p.setBrush( QColor(200, 200, 200) );
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
    NodeGraph* _publicInterface; // can not be a smart ptr
    boost::weak_ptr<NodeCollection> group;
    QPoint _lastMousePos;
    QPointF _lastSelectionStartPointScene;
    EventStateEnum _evtState;
    NodeGuiWPtr _magnifiedNode;
    double _nodeSelectedScaleBeforeMagnif;
    bool _magnifOn;
    Edge* _arrowSelected;
    mutable QMutex _nodesMutex;
    NodesGuiList _nodes;

    ///Enables the "Tab" shortcut to popup the node creation dialog.
    ///This is set to true on enterEvent and set back to false on leaveEvent
    bool _nodeCreationShortcutEnabled;
    QString _lastPluginCreatedID;
    QGraphicsItem* _root; ///< this is the parent of all items in the graph
    QGraphicsItem* _nodeRoot; ///< this is the parent of all nodes
    QGraphicsSimpleTextItem* _cacheSizeText;
    bool cacheSizeHidden;
    QTimer _refreshCacheTextTimer;
    Navigator* _navigator;
    boost::shared_ptr<QUndoStack> _undoStack;
    QMenu* _menu;
    QGraphicsItem *_tL, *_tR, *_bR, *_bL;
    bool _refreshOverlays;
    Edge* _highLightedEdge;
    NodeGuiWPtr _mergeHintNode;

    ///This is a hint edge we show when _highLightedEdge is not NULL to display a possible connection.
    Edge* _hintInputEdge;
    Edge* _hintOutputEdge;
    NodeGuiWPtr _backdropResized; //< the backdrop being resized
    NodesGuiWList _selection;
    NodesGuiWList _selectionBeforeNodeCreated;

    //To avoid calling unsetCursor too much
    bool cursorSet;
    std::map<NodeGuiPtr, NodesGuiList> _nodesWithinBDAtPenDown;
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
    QTimer refreshRenderStateTimer;

    struct LinkedNodes
    {
        NodeWPtr nodes[2];
        bool isCloneLink;
        LinkArrow* arrow;

        LinkedNodes()
        : nodes()
        , isCloneLink(false)
        , arrow(0)
        {

        }
    };

    typedef std::list<LinkedNodes> LinkedNodesList;

    LinkedNodesList linkedNodes;

    // used to concatenate refreshing of links
    int refreshNodesLinkRequest;

    NodeGraphPrivate(NodeGraph* p,
                     const NodeCollectionPtr& group);

    QPoint getPyPlugUnlockPos() const;

    QRectF calcNodesBoundingRect();

    /**
     * @brief Serialize the given node list 
     **/
    void copyNodesInternal(const NodesGuiList& selection, SERIALIZATION_NAMESPACE::NodeSerializationList* clipboard);
    void copyNodesInternal(const NodesGuiList& selection, std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr> >* clipboard);

    enum PasteNodesFlagEnum
    {
        // Used to specify none of the operations below
        ePasteNodesFlagNone = 0x0,

        // The newly created nodes will be offset relative to their new centroid where the offset
        // is computed from the old centroid of the node positions in the serialization
        ePasteNodesFlagRelativeToCentroid = 0x1,

        // If set this function will push and undo/redo command to create the nodes
        ePasteNodesFlagUseUndoCommand = 0x2,

        // If set each original node in the list must contain a pointer to the original node as well as its serialization:
        // In this mode the newly created node is then linked to the original node.
        // If not specified, the node pointer of the original is not necessary and can be left to NULL.
        ePasteNodesFlagCloneNodes = 0x4
    };

    DECLARE_FLAGS(PasteNodesFlags, PasteNodesFlagEnum)

    /**
     * @brief Paste the given nodes with flags. This will create new copies of the nodes
     **/
    void pasteNodesInternal(const std::list<std::pair<NodePtr, SERIALIZATION_NAMESPACE::NodeSerializationPtr> >& originalNodes,
                            const QPointF& newCentroidScenePos,
                            PasteNodesFlags flags);

    
    
    /**
     * @brief This is called once all nodes of a clipboard have been pasted to try to restore connections between them
     * The new nodes are mapped against their old script-name
     **/
    struct ConnectionToRestore
    {
        NodePtr newNode;
        SERIALIZATION_NAMESPACE::NodeSerializationPtr nodeSerialization;
    };

    NodesGuiWList::iterator findSelectedNode(const NodeGuiPtr& node)
    {
        for (NodesGuiWList::iterator it = _selection.begin(); it != _selection.end(); ++it) {
            NodeGuiPtr n = it->lock();
            if ( n == node ) {
                return it;
            }
        }
        return _selection.end();
    }

    void editSelectionFromSelectionRectangle(bool addToSelection);

    void resetSelection();

    void setNodesBendPointsVisible(bool visible);

    bool rearrangeSelectedNodes();

    void toggleSelectedNodesEnabled();

    void getNodeSet(const NodesGuiList& nodeList, std::set<NodeGuiPtr>& nodeSet);
};

NATRON_NAMESPACE_EXIT

DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::NodeGraphPrivate::PasteNodesFlags);


#endif // Gui_NodeGraphPrivate_h
