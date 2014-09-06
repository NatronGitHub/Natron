//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "NodeGraph.h"

#include <cstdlib>
#include <set>
#include <map>
#include <vector>
#include <locale>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QGraphicsProxyWidget>
CLANG_DIAG_ON(unused-private-field)
#include <QGraphicsTextItem>
#include <QFileSystemModel>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QUndoStack>
#include <QMenu>
#include <QThread>
#include <QDropEvent>
#include <QCoreApplication>
#include <QMimeData>
#include <QLineEdit>
#include <QDebug>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QLabel>
#include <QAction>
#include <QPainter>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <SequenceParsing.h>

#include "Engine/AppManager.h"

#include "Engine/VideoEngine.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Hash64.h"
#include "Engine/FrameEntry.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/Project.h"
#include "Engine/Plugin.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Node.h"
#include "Engine/NoOp.h"

#include "Gui/TabWidget.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/DockablePanel.h"
#include "Gui/ToolButton.h"
#include "Gui/KnobGui.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Gui/Gui.h"
#include "Gui/TimeLineGui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/CurveEditor.h"
#include "Gui/NodeBackDrop.h"
#include "Gui/NodeBackDropSerialization.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/GuiMacros.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/GuiApplicationManager.h"

#define NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS 1000

#define NATRON_BACKDROP_DEFAULT_WIDTH 80
#define NATRON_BACKDROP_DEFAULT_HEIGHT 80

#define NATRON_NODE_DUPLICATE_X_OFFSET 50

///These are percentages of the size of the NodeGraph in widget coordinates.
#define NATRON_NAVIGATOR_BASE_HEIGHT 0.2
#define NATRON_NAVIGATOR_BASE_WIDTH 0.2

using namespace Natron;
using std::cout; using std::endl;


namespace {
struct NodeClipBoard
{
    std::list<boost::shared_ptr<NodeSerialization> > nodes;
    std::list<boost::shared_ptr<NodeGuiSerialization> > nodesUI;
    std::list<boost::shared_ptr<NodeBackDropSerialization> > bds;

    NodeClipBoard()
        : nodes()
          , nodesUI()
          , bds()
    {
    }

    bool isEmpty() const
    {
        return nodes.empty() && nodesUI.empty() && bds.empty();
    }
};

enum EVENT_STATE
{
    DEFAULT,
    MOVING_AREA,
    ARROW_DRAGGING,
    NODE_DRAGGING,
    BACKDROP_DRAGGING,
    BACKDROP_RESIZING,
    SELECTION_RECT,
};

struct NodeSelection
{
    std::list< boost::shared_ptr<NodeGui> > nodes;
    std::list<NodeBackDrop*> bds;
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

class SelectionRectangle
    : public QGraphicsRectItem
{
public:

    SelectionRectangle(QGraphicsItem* parent = 0)
        : QGraphicsRectItem(parent)
    {
    }

    virtual void paint(QPainter *painter,
                       const QStyleOptionGraphicsItem */*option*/,
                       QWidget */*widget*/) OVERRIDE FINAL
    {
        QRectF r = rect();
        QColor color(16,84,200,20);

        painter->setBrush(color);
        painter->drawRect(r);
        double w = painter->pen().widthF();
        painter->fillRect(QRect(r.x() + w,r.y() + w,r.width() - w,r.height() - w),color);
    }
};
}

struct NodeGraphPrivate
{
    NodeGraph* _publicInterface;
    Gui* _gui;
    QPointF _lastScenePosClick;
    QPointF _lastNodeDragStartPoint;
    QPointF _lastSelectionStartPoint;
    EVENT_STATE _evtState;
    boost::shared_ptr<NodeGui> _magnifiedNode;
    double _nodeSelectedScaleBeforeMagnif;
    bool _magnifOn;
    Edge* _arrowSelected;
    mutable QMutex _nodesMutex;
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    std::list<boost::shared_ptr<NodeGui> > _nodesTrash;

    ///Enables the "Tab" shortcut to popup the node creation dialog.
    ///This is set to true on enterEvent and set back to false on leaveEvent
    bool _nodeCreationShortcutEnabled;
    QGraphicsItem* _root; ///< this is the parent of all items in the graph
    QGraphicsItem* _nodeRoot; ///< this is the parent of all nodes
    QScrollArea* _propertyBin;
    QGraphicsTextItem* _cacheSizeText;
    QTimer _refreshCacheTextTimer;
    Navigator* _navigator;
    QUndoStack* _undoStack;
    QMenu* _menu;
    QGraphicsItem *_tL,*_tR,*_bR,*_bL;
    bool _refreshOverlays;
    NodeClipBoard _nodeClipBoard;
    Edge* _highLightedEdge;

    ///This is a hint edge we show when _highLightedEdge is not NULL to display a possible connection.
    Edge* _hintInputEdge;
    Edge* _hintOutputEdge;
    std::list<NodeBackDrop*> _backdrops;
    NodeBackDrop* _backdropResized; //< the backdrop being resized
    bool _firstMove;
    NodeSelection _selection;
    QGraphicsRectItem* _selectionRect;
    bool _bendPointsVisible;
    bool _knobLinksVisible;

    NodeGraphPrivate(Gui* gui,
                     NodeGraph* p)
        : _publicInterface(p)
          , _gui(gui)
          , _lastScenePosClick()
          , _lastNodeDragStartPoint()
          , _lastSelectionStartPoint()
          , _evtState(DEFAULT)
          , _magnifiedNode()
          , _nodeSelectedScaleBeforeMagnif(1.)
          , _magnifOn(false)
          , _arrowSelected(NULL)
          , _nodesMutex()
          , _nodes()
          , _nodesTrash()
          , _nodeCreationShortcutEnabled(false)
          , _root(NULL)
          , _nodeRoot(NULL)
          , _propertyBin(NULL)
          , _cacheSizeText(NULL)
          , _refreshCacheTextTimer()
          , _navigator(NULL)
          , _undoStack(NULL)
          , _menu(NULL)
          , _tL(NULL)
          , _tR(NULL)
          , _bR(NULL)
          , _bL(NULL)
          , _refreshOverlays(false)
          , _nodeClipBoard()
          , _highLightedEdge(NULL)
          , _hintInputEdge(NULL)
          , _hintOutputEdge(NULL)
          , _backdrops()
          , _backdropResized(NULL)
          , _firstMove(true)
          , _selection()
          , _selectionRect(NULL)
          , _bendPointsVisible(false)
          , _knobLinksVisible(true)
    {
    }

    void resetAllClipboards();

    QRectF calcNodesBoundingRect();

    void copyNodesInternal(NodeClipBoard & clipboard);
    void pasteNodesInternal(const NodeClipBoard & clipboard);

    /**
     * @brief Create a new node given the serialization of another one
     * @param offset[in] The offset applied to the new node position relative to the serialized node's position.
     **/
    boost::shared_ptr<NodeGui> pasteNode(const NodeSerialization & internalSerialization,
                                         const NodeGuiSerialization & guiSerialization,
                                         const QPointF & offset);

    /**
     * @brief Create a new node backdrop given the serialization of another one
     * @param offset[in] The offset applied to the new backdrop position relative to the serialized backdrop's position.
     **/
    NodeBackDrop* pasteBackdrop(const NodeBackDropSerialization & serialization,
                                const QPointF & offset);

    /**
     * @brief This is called once all nodes of a clipboard have been pasted to try to restore connections between them
     * WARNING: The 2 lists must be ordered the same: each item in serializations corresponds to the same item in the newNodes
     * list. We're not using 2 lists to avoid a copy from the paste function.
     **/
    void restoreConnections(const std::list<boost::shared_ptr<NodeSerialization> > & serializations,
                            const std::list<boost::shared_ptr<NodeGui> > & newNodes);

    void editSelectionFromSelectionRectangle(bool addToSelection);

    void resetSelection();

    void setNodesBendPointsVisible(bool visible);

    void rearrangeSelectedNodes();

    void toggleSelectedNodesEnabled();
};

NodeGraph::NodeGraph(Gui* gui,
                     QGraphicsScene* scene,
                     QWidget *parent)
    : QGraphicsView(scene,parent)
      , _imp( new NodeGraphPrivate(gui,this) )
{
    setAcceptDrops(true);

    QObject::connect( _imp->_gui->getApp()->getProject().get(), SIGNAL( nodesCleared() ), this, SLOT( onProjectNodesCleared() ) );

    setMouseTracking(true);
    setCacheMode(CacheBackground);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    scale( qreal(0.8), qreal(0.8) );

    _imp->_root = new QGraphicsTextItem(0);
    _imp->_nodeRoot = new QGraphicsTextItem(_imp->_root);
    scene->addItem(_imp->_root);

    _imp->_selectionRect = new SelectionRectangle(_imp->_root);
    _imp->_selectionRect->setZValue(1);
    _imp->_selectionRect->hide();

    _imp->_navigator = new Navigator(0);
    scene->addItem(_imp->_navigator);
    _imp->_navigator->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_navigator->hide();


    _imp->_cacheSizeText = new QGraphicsTextItem(0);
    scene->addItem(_imp->_cacheSizeText);
    _imp->_cacheSizeText->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    _imp->_cacheSizeText->setDefaultTextColor( QColor(200,200,200) );

    QObject::connect( &_imp->_refreshCacheTextTimer,SIGNAL( timeout() ),this,SLOT( updateCacheSizeText() ) );
    _imp->_refreshCacheTextTimer.start(NATRON_CACHE_SIZE_TEXT_REFRESH_INTERVAL_MS);

    _imp->_undoStack = new QUndoStack(this);
    _imp->_undoStack->setUndoLimit( appPTR->getCurrentSettings()->getMaximumUndoRedoNodeGraph() );
    _imp->_gui->registerNewUndoStack(_imp->_undoStack);

    _imp->_hintInputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_imp->_nodeRoot);
    _imp->_hintInputEdge->setDefaultColor( QColor(0,255,0,100) );
    _imp->_hintInputEdge->hide();

    _imp->_hintOutputEdge = new Edge(0,0,boost::shared_ptr<NodeGui>(),_imp->_nodeRoot);
    _imp->_hintOutputEdge->setDefaultColor( QColor(0,255,0,100) );
    _imp->_hintOutputEdge->hide();

    _imp->_tL = new QGraphicsTextItem(0);
    _imp->_tL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_tL);

    _imp->_tR = new QGraphicsTextItem(0);
    _imp->_tR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_tR);

    _imp->_bR = new QGraphicsTextItem(0);
    _imp->_bR->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_bR);

    _imp->_bL = new QGraphicsTextItem(0);
    _imp->_bL->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(_imp->_bL);

    scene->setSceneRect(0,0,10000,10000);
    _imp->_tL->setPos( _imp->_tL->mapFromScene( QPointF(0,10000) ) );
    _imp->_tR->setPos( _imp->_tR->mapFromScene( QPointF(10000,10000) ) );
    _imp->_bR->setPos( _imp->_bR->mapFromScene( QPointF(10000,0) ) );
    _imp->_bL->setPos( _imp->_bL->mapFromScene( QPointF(0,0) ) );
    centerOn(5000,5000);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    _imp->_menu = new QMenu(this);
    _imp->_menu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );

    QObject::connect( _imp->_gui->getApp()->getTimeLine().get(),SIGNAL( frameChanged(SequenceTime,int) ),
                      this,SLOT( onTimeChanged(SequenceTime,int) ) );
}

NodeGraph::~NodeGraph()
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin();
         it != _imp->_nodes.end();
         ++it) {
        (*it)->discardGraphPointer();
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin();
         it != _imp->_nodesTrash.end();
         ++it) {
        (*it)->discardGraphPointer();
    }

    if (_imp->_gui) {
        QGraphicsScene* scene = _imp->_hintInputEdge->scene();
        if (scene) {
            scene->removeItem(_imp->_hintInputEdge);
        }
        _imp->_hintInputEdge->setParentItem(NULL);
        delete _imp->_hintInputEdge;

        scene = _imp->_hintOutputEdge->scene();
        if (scene) {
            scene->removeItem(_imp->_hintOutputEdge);
        }
        _imp->_hintOutputEdge->setParentItem(NULL);
        delete _imp->_hintOutputEdge;
    }

    QObject::disconnect( &_imp->_refreshCacheTextTimer,SIGNAL( timeout() ),this,SLOT( updateCacheSizeText() ) );
    _imp->_nodeCreationShortcutEnabled = false;

    //onProjectNodesCleared();
}

void
NodeGraph::setPropertyBinPtr(QScrollArea* propertyBin)
{
    _imp->_propertyBin = propertyBin;
}

const std::list< boost::shared_ptr<NodeGui> > &
NodeGraph::getSelectedNodes() const
{
    return _imp->_selection.nodes;
}

QGraphicsItem*
NodeGraph::getRootItem() const
{
    return _imp->_root;
}

Gui*
NodeGraph::getGui() const
{
    return _imp->_gui;
}

void
NodeGraph::discardGuiPointer()
{
    _imp->_gui = 0;
}

void
NodeGraph::onProjectNodesCleared()
{
    _imp->_selection.nodes.clear();
    std::list<boost::shared_ptr<NodeGui> > nodesCpy;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        nodesCpy = _imp->_nodes;
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodesCpy.begin(); it != nodesCpy.end(); ++it) {
        deleteNodePermanantly( *it );
    }

    while ( !_imp->_nodesTrash.empty() ) {
        deleteNodePermanantly( *( _imp->_nodesTrash.begin() ) );
    }
    _imp->_selection.nodes.clear();
    _imp->_magnifiedNode.reset();
    _imp->_nodes.clear();
    _imp->_nodesTrash.clear();
    _imp->_undoStack->clear();

    for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
        delete *it;
    }
    _imp->_backdrops.clear();
}

void
NodeGraph::resizeEvent(QResizeEvent* e)
{
    _imp->_refreshOverlays = true;
    QGraphicsView::resizeEvent(e);
}

void
NodeGraph::paintEvent(QPaintEvent* e)
{
    if (_imp->_refreshOverlays) {
        ///The visible portion of the scene, in scene coordinates
        QRectF visibleScene = visibleSceneRect();
        QRect visibleWidget = visibleWidgetRect();

        ///Set the cache size overlay to be in the top left corner of the view
        _imp->_cacheSizeText->setPos( visibleScene.topLeft() );

        double navWidth = NATRON_NAVIGATOR_BASE_WIDTH * width();
        double navHeight = NATRON_NAVIGATOR_BASE_HEIGHT * height();
        QPoint btmRightWidget = visibleWidget.bottomRight();
        QPoint navTopLeftWidget = btmRightWidget - QPoint(navWidth,navHeight );
        QPointF navTopLeftScene = mapToScene(navTopLeftWidget);

        _imp->_navigator->refreshPosition(navTopLeftScene,navWidth,navHeight);
        updateNavigator();
        _imp->_refreshOverlays = false;
    }
    QGraphicsView::paintEvent(e);
}

QRectF
NodeGraph::visibleSceneRect()
{
    return mapToScene( visibleWidgetRect() ).boundingRect();
}

QRect
NodeGraph::visibleWidgetRect()
{
    return viewport()->rect();
}

boost::shared_ptr<NodeGui>
NodeGraph::createNodeGUI(QVBoxLayout *dockContainer,
                         const boost::shared_ptr<Natron::Node> & node,
                         bool requestedByLoad,
                         double xPosHint,
                         double yPosHint,
                         bool pushUndoRedoCommand)
{
    boost::shared_ptr<NodeGui> node_ui;
    Dot* isDot = dynamic_cast<Dot*>( node->getLiveInstance() );

    if (!isDot) {
        node_ui.reset( new NodeGui(_imp->_nodeRoot) );
    } else {
        node_ui.reset( new DotGui(_imp->_nodeRoot) );
    }
    node_ui->initialize(this, node_ui, dockContainer, node, requestedByLoad);

    ///only move main instances
    if ( node->getParentMultiInstanceName().empty() ) {
        if ( (xPosHint != INT_MIN) && (yPosHint != INT_MIN) && (_imp->_selection.nodes.size() != 1) ) {
            QPointF pos = node_ui->mapToParent( node_ui->mapFromScene( QPointF(xPosHint,yPosHint) ) );
            node_ui->refreshPosition( pos.x(),pos.y() );
        } else {
            moveNodesForIdealPosition(node_ui);
        }
    }

    {
        QMutexLocker l(&_imp->_nodesMutex);
        _imp->_nodes.push_back(node_ui);
    }
    QUndoStack* nodeStack = node_ui->getUndoStack();
    if (nodeStack) {
        _imp->_gui->registerNewUndoStack(nodeStack);
    }

    if (pushUndoRedoCommand) {
        _imp->_undoStack->setActive();
        _imp->_undoStack->push( new AddMultipleNodesCommand(this,node_ui) );
    }
    _imp->_evtState = DEFAULT;

    return node_ui;
}

void
NodeGraph::moveNodesForIdealPosition(boost::shared_ptr<NodeGui> node)
{
    QRectF viewPos = visibleSceneRect();

    ///3 possible values:
    /// 0 = default , i.e: we pop the node in the middle of the graph's current view
    /// 1 = pop the node above the selected node and move the inputs of the selected node a little
    /// 2 = pop the node below the selected node and move the outputs of the selected node a little
    int behavior = 0;
    boost::shared_ptr<NodeGui> selected;

    if (_imp->_selection.nodes.size() == 1) {
        selected = _imp->_selection.nodes.front();
    }

    if (!selected) {
        behavior = 0;
    } else {
        ///this function is redundant with Project::autoConnect, depending on the node selected
        ///and this node we make some assumptions on to where we could put the node.

        //        1) selected is output
        //          a) created is output --> fail
        //          b) created is input --> connect input
        //          c) created is regular --> connect input
        //        2) selected is input
        //          a) created is output --> connect output
        //          b) created is input --> fail
        //          c) created is regular --> connect output
        //        3) selected is regular
        //          a) created is output--> connect output
        //          b) created is input --> connect input
        //          c) created is regular --> connect output

        ///1)
        if ( selected->getNode()->isOutputNode() ) {
            ///case 1-a) just do default we don't know what else to do
            if ( node->getNode()->isOutputNode() ) {
                behavior = 0;
            } else {
                ///for either cases 1-b) or 1-c) we just connect the created node as input of the selected node.
                behavior = 1;
            }
        }
        ///2) and 3) are similar except for case b)
        else {
            ///case 2 or 3- a): connect the created node as output of the selected node.
            if ( node->getNode()->isOutputNode() ) {
                behavior = 2;
            }
            ///case b)
            else if (node->getNode()->getMaxInputCount() == 0) {
                if (selected->getNode()->getMaxInputCount() == 0) {
                    ///case 2-b) just do default we don't know what else to do
                    behavior = 0;
                } else {
                    ///case 3-b): connect the created node as input of the selected node
                    behavior = 1;
                }
            }
            ///case c) connect created as output of the selected node
            else {
                behavior = 2;
            }
        }
    }

    ///if behaviour is 1 , just check that we can effectively connect the node to avoid moving them for nothing
    ///otherwise fallback on behaviour 0
    if (behavior == 1) {
        const std::vector<boost::shared_ptr<Natron::Node> > & inputs = selected->getNode()->getInputs_mt_safe();
        bool oneInputEmpty = false;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (!inputs[i]) {
                oneInputEmpty = true;
                break;
            }
        }
        if (!oneInputEmpty) {
            behavior = 0;
        }
    }

    ///default
    QPointF position;
    if (behavior == 0) {
        position.setX( ( viewPos.bottomRight().x() + viewPos.topLeft().x() ) / 2. );
        position.setY( ( viewPos.topLeft().y() + viewPos.bottomRight().y() ) / 2. );
    }
    ///pop it above the selected node
    else if (behavior == 1) {
        QSize selectedNodeSize = selected->getSize();
        QSize createdNodeSize = node->getSize();
        QPointF selectedNodeMiddlePos = selected->scenePos() +
                                        QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);


        position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
        position.setY( selectedNodeMiddlePos.y() - selectedNodeSize.height() / 2 - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES
                       - createdNodeSize.height() );

        QRectF createdNodeRect( position.x(),position.y(),createdNodeSize.width(),createdNodeSize.height() );

        ///now that we have the position of the node, move the inputs of the selected node to make some space for this node
        const std::map<int,Edge*> & selectedNodeInputs = selected->getInputsArrows();
        for (std::map<int,Edge*>::const_iterator it = selectedNodeInputs.begin(); it != selectedNodeInputs.end(); ++it) {
            if ( it->second->hasSource() ) {
                it->second->getSource()->moveAbovePositionRecursively(createdNodeRect);
            }
        }
    }
    ///pop it below the selected node
    else {
        QSize selectedNodeSize = selected->getSize();
        QSize createdNodeSize = node->getSize();
        QPointF selectedNodeMiddlePos = selected->scenePos() +
                                        QPointF(selectedNodeSize.width() / 2, selectedNodeSize.height() / 2);

        ///actually move the created node where the selected node is
        position.setX(selectedNodeMiddlePos.x() - createdNodeSize.width() / 2);
        position.setY(selectedNodeMiddlePos.y() + (selectedNodeSize.height() / 2) + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);

        QRectF createdNodeRect( position.x(),position.y(),createdNodeSize.width(),createdNodeSize.height() );

        ///and move the selected node below recusively
        const std::list<boost::shared_ptr<Natron::Node> > & outputs = selected->getNode()->getOutputs();
        for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            assert(*it);
            boost::shared_ptr<NodeGui> output = _imp->_gui->getApp()->getNodeGui(*it);
            assert(output);
            output->moveBelowPositionRecursively(createdNodeRect);
        }
    }
    position = node->mapFromScene(position);
    position = node->mapToParent(position);
    node->setPos( position.x(), position.y() );
} // moveNodesForIdealPosition

void
NodeGraph::mousePressEvent(QMouseEvent* e)
{
    assert(e);

    if ( buttonDownIsMiddle(e) ) {
        _imp->_evtState = MOVING_AREA;

        return;
    }

    bool didSomething = false;

    _imp->_lastScenePosClick = mapToScene( e->pos() );

    boost::shared_ptr<NodeGui> selected;
    Edge* selectedEdge = 0;
    Edge* selectedBendPoint = 0;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::reverse_iterator it = _imp->_nodes.rbegin(); it != _imp->_nodes.rend(); ++it) {
            QPointF evpt = (*it)->mapFromScene(_imp->_lastScenePosClick);
            if ( (*it)->isVisible() && (*it)->isActive() && (*it)->contains(evpt) ) {
                selected = *it;
                break;
            }
        }
        if (!selected) {
            ///try to find a selected edge
            for (std::list<boost::shared_ptr<NodeGui> >::reverse_iterator it = _imp->_nodes.rbegin(); it != _imp->_nodes.rend(); ++it) {
                boost::shared_ptr<NodeGui> & n = *it;
                Edge* bendPointEdge = n->hasBendPointNearbyPoint(_imp->_lastScenePosClick);
                if (bendPointEdge) {
                    selectedBendPoint = bendPointEdge;
                    break;
                }
                Edge* edge = n->hasEdgeNearbyPoint(_imp->_lastScenePosClick);
                if (edge) {
                    selectedEdge = edge;
                }
            }
        }
    }

    if (selected) {
        didSomething = true;
        if ( buttonDownIsLeft(e) ) {
            _imp->_magnifiedNode = selected;
            if ( !selected->getIsSelected() ) {
                selectNode( selected, modCASIsShift(e) );
            } else if ( modCASIsShift(e) ) {
                std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_imp->_selection.nodes.begin(),
                                                                                _imp->_selection.nodes.end(),selected);
                if ( it != _imp->_selection.nodes.end() ) {
                    (*it)->setUserSelected(false);
                    _imp->_selection.nodes.erase(it);
                }
            }
            _imp->_evtState = NODE_DRAGGING;
            _imp->_lastNodeDragStartPoint = selected->pos();
        } else if ( buttonDownIsRight(e) ) {
            if ( !selected->getIsSelected() ) {
                selectNode(selected,true); ///< don't wipe the selection
            }
        }
    } else if (selectedBendPoint) {
        _imp->setNodesBendPointsVisible(false);

        boost::shared_ptr<Natron::Node> dotNode = _imp->_gui->getApp()->createNode( CreateNodeArgs("Dot",
                                                                                                   std::string(),
                                                                                                   -1,
                                                                                                   -1,
                                                                                                   true,
                                                                                                   -1,
                                                                                                   false) );
        assert(dotNode);
        boost::shared_ptr<NodeGui> dotNodeGui = _imp->_gui->getApp()->getNodeGui(dotNode);
        assert(dotNodeGui);

        std::list<boost::shared_ptr<NodeGui> > nodesList;
        nodesList.push_back(dotNodeGui);
        _imp->_undoStack->push( new AddMultipleNodesCommand( this,nodesList,std::list<NodeBackDrop*>() ) );

        ///Now connect the node to the edge input
        boost::shared_ptr<Natron::Node> inputNode = selectedBendPoint->getSource()->getNode();
        assert(inputNode);
        ///disconnect previous connection
        boost::shared_ptr<Natron::Node> outputNode = selectedBendPoint->getDest()->getNode();
        assert(outputNode);

        int inputNb = outputNode->inputIndex( inputNode.get() );
        assert(inputNb != -1);
        bool ok = _imp->_gui->getApp()->getProject()->disconnectNodes(inputNode, outputNode);
        assert(ok);

        ok = _imp->_gui->getApp()->getProject()->connectNodes(0, inputNode, dotNode);
        assert(ok);

        _imp->_gui->getApp()->getProject()->connectNodes(inputNb,dotNode,outputNode);

        QPointF pos = dotNodeGui->mapToParent( dotNodeGui->mapFromScene(_imp->_lastScenePosClick) );
        dotNodeGui->refreshPosition( pos.x(), pos.y() );
        if ( !dotNodeGui->getIsSelected() ) {
            selectNode( dotNodeGui, modCASIsShift(e) );
        }
        _imp->_evtState = NODE_DRAGGING;
        _imp->_lastNodeDragStartPoint = dotNodeGui->pos();
        didSomething = true;
    } else if (selectedEdge) {
        _imp->_arrowSelected = selectedEdge;
        didSomething = true;
        _imp->_evtState = ARROW_DRAGGING;
    }

    if (_imp->_evtState == DEFAULT) {
        ///check if nearby a backdrop
        for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
            if ( buttonDownIsLeft(e) ) {
                bool nearbyHeader = (*it)->isNearbyHeader(_imp->_lastScenePosClick);
                bool nearbyResizeHandle = (*it)->isNearbyResizeHandle(_imp->_lastScenePosClick);


                if (nearbyHeader || nearbyResizeHandle) {
                    if ( buttonDownIsLeft(e) ) {
                        didSomething = true;
                        if ( !(*it)->getIsSelected() ) {
                            selectBackDrop( *it, modCASIsShift(e) );
                        } else if ( modCASIsShift(e) ) {
                            std::list<NodeBackDrop* >::iterator found = std::find(_imp->_selection.bds.begin(),
                                                                                  _imp->_selection.bds.end(),*it);
                            if ( found != _imp->_selection.bds.end() ) {
                                (*it)->setUserSelected(false);
                                _imp->_selection.bds.erase(found);
                            }
                        }

                        if (nearbyHeader) {
                            _imp->_evtState = BACKDROP_DRAGGING;
                        } else if (nearbyResizeHandle) {
                            _imp->_backdropResized = *it;
                            _imp->_evtState = BACKDROP_RESIZING;
                        }
                    } else if ( buttonDownIsRight(e) ) {
                        if ( !(*it)->getIsSelected() ) {
                            selectBackDrop(*it,true); ///< don't wipe the selection
                        }
                    }
                    break;
                }
            }
        }
    }
    ///Don't forget to reset back to null the _backdropResized pointer
    if (_imp->_evtState != BACKDROP_RESIZING) {
        _imp->_backdropResized = NULL;
    }

    if ( buttonDownIsRight(e) ) {
        showMenu( mapToGlobal( e->pos() ) );
        didSomething = true;
    }
    if (!didSomething) {
        if ( buttonDownIsLeft(e) ) {
            if ( !modCASIsShift(e) ) {
                deselect();
            }
            _imp->_evtState = SELECTION_RECT;
            _imp->_lastSelectionStartPoint = _imp->_lastScenePosClick;
            QPointF clickPos = _imp->_selectionRect->mapFromScene(_imp->_lastScenePosClick);
            _imp->_selectionRect->setRect(clickPos.x(), clickPos.y(), 0, 0);
            _imp->_selectionRect->show();
        } else if ( buttonDownIsMiddle(e) ) {
            _imp->_evtState = MOVING_AREA;
            QGraphicsView::mousePressEvent(e);
        }
    }
} // mousePressEvent

void
NodeGraph::deselect()
{
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.nodes.begin(); it != _imp->_selection.nodes.end(); ++it) {
            (*it)->setUserSelected(false);
        }
    }
    for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
        (*it)->setUserSelected(false);
    }
    _imp->_selection.nodes.clear();
    _imp->_selection.bds.clear();

    if (_imp->_magnifiedNode && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_magnifiedNode->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}

void
NodeGraph::mouseReleaseEvent(QMouseEvent* e)
{
    EVENT_STATE state = _imp->_evtState;

    _imp->_firstMove = true;
    _imp->_evtState = DEFAULT;

    if (state == ARROW_DRAGGING) {
        bool foundSrc = false;
        assert(_imp->_arrowSelected);
        boost::shared_ptr<NodeGui> nodeHoldingEdge = _imp->_arrowSelected->isOutputEdge() ?
                                                     _imp->_arrowSelected->getSource() : _imp->_arrowSelected->getDest();
        assert(nodeHoldingEdge);

        std::list<boost::shared_ptr<NodeGui> > nodes = getAllActiveNodes_mt_safe();
        QPointF ep = mapToScene( e->pos() );

        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            boost::shared_ptr<NodeGui> & n = *it;

            if ( n->isActive() && n->isNearby(ep) &&
                 ( n->getNode()->getName() != nodeHoldingEdge->getNode()->getName() ) ) {
                if ( !_imp->_arrowSelected->isOutputEdge() ) {
                    if ( !n->getNode()->canOthersConnectToThisNode() ) {
                        break;
                    }

                    _imp->_arrowSelected->stackBefore( n.get() );
                    _imp->_undoStack->setActive();
                    _imp->_undoStack->push( new ConnectCommand(this,_imp->_arrowSelected,_imp->_arrowSelected->getSource(),n) );
                } else {
                    ///Find the input edge of the node we just released the mouse over,
                    ///and use that edge to connect to the source of the selected edge.
                    int preferredInput = n->getNode()->getPreferredInputForConnection();
                    if (preferredInput != -1) {
                        const std::map<int,Edge*> & inputEdges = n->getInputsArrows();
                        std::map<int,Edge*>::const_iterator foundInput = inputEdges.find(preferredInput);
                        assert( foundInput != inputEdges.end() );
                        _imp->_undoStack->setActive();
                        _imp->_undoStack->push( new ConnectCommand( this,foundInput->second,
                                                                    foundInput->second->getSource(),_imp->_arrowSelected->getSource() ) );
                    }
                }
                foundSrc = true;

                break;
            }
        }

        ///if we disconnected the input edge, use the undo/redo stack.
        ///Output edges can never be really connected, they're just there
        ///So the user understands some nodes can have output
        if ( !foundSrc && !_imp->_arrowSelected->isOutputEdge() && _imp->_arrowSelected->getSource() ) {
            _imp->_undoStack->setActive();
            _imp->_undoStack->push( new ConnectCommand( this,_imp->_arrowSelected,_imp->_arrowSelected->getSource(),
                                                        boost::shared_ptr<NodeGui>() ) );
            scene()->update();
        }
        nodeHoldingEdge->refreshEdges();
        scene()->update();
    } else if (state == NODE_DRAGGING) {
        if ( !_imp->_selection.nodes.empty() ) {
            ///now if there was a hint displayed, use it to actually make connections.
            if (_imp->_highLightedEdge) {
                assert( !_imp->_selection.nodes.empty() );
                boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.nodes.front();
                if ( _imp->_highLightedEdge->isOutputEdge() ) {
                    int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                    if (prefInput != -1) {
                        Edge* inputEdge = selectedNode->getInputArrow(prefInput);
                        assert(inputEdge);
                        _imp->_undoStack->push( new ConnectCommand( this,inputEdge,inputEdge->getSource(),
                                                                    _imp->_highLightedEdge->getSource() ) );
                    }
                } else {
                    boost::shared_ptr<NodeGui> src = _imp->_highLightedEdge->getSource();
                    _imp->_undoStack->push( new ConnectCommand(this,_imp->_highLightedEdge,_imp->_highLightedEdge->getSource(),
                                                               selectedNode) );

                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> > & inpNodes = selectedNode->getNode()->getInputs_mt_safe();
                    if (src) {
                        for (U32 i = 0; i < inpNodes.size(); ++i) {
                            if ( inpNodes[i] == src->getNode() ) {
                                alreadyConnected = true;
                                break;
                            }
                        }
                    }

                    if (src && !alreadyConnected) {
                        ///push a second command... this is a bit dirty but I don't have time to add a whole new command just for this
                        int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                        if (prefInput != -1) {
                            Edge* inputEdge = selectedNode->getInputArrow(prefInput);
                            assert(inputEdge);
                            _imp->_undoStack->push( new ConnectCommand(this,inputEdge,inputEdge->getSource(),src) );
                        }
                    }
                }
                _imp->_highLightedEdge->setUseHighlight(false);
                _imp->_highLightedEdge = 0;
                _imp->_hintInputEdge->hide();
                _imp->_hintOutputEdge->hide();
            }
        }
    } else if (state == SELECTION_RECT) {
        _imp->_selectionRect->hide();
        _imp->editSelectionFromSelectionRectangle( modCASIsShift(e) );
    }
    scene()->update();
    update();
    setCursor( QCursor(Qt::ArrowCursor) );
} // mouseReleaseEvent

void
NodeGraph::mouseMoveEvent(QMouseEvent* e)
{
    QPointF newPos = mapToScene( e->pos() );
    double dx = _imp->_root->mapFromScene(newPos).x() - _imp->_root->mapFromScene(_imp->_lastScenePosClick).x();
    double dy = _imp->_root->mapFromScene(newPos).y() - _imp->_root->mapFromScene(_imp->_lastScenePosClick).y();

    if (_imp->_evtState != SELECTION_RECT) {
        ///set cursor
        boost::shared_ptr<NodeGui> selected;
        Edge* selectedEdge = 0;

        {
            QMutexLocker l(&_imp->_nodesMutex);
            for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
                boost::shared_ptr<NodeGui> & n = *it;
                QPointF evpt = n->mapFromScene(newPos);
                if ( n->isActive() && n->contains(evpt) ) {
                    selected = n;
                    break;
                } else {
                    Edge* edge = n->hasEdgeNearbyPoint(newPos);
                    if (edge) {
                        selectedEdge = edge;
                        break;
                    }
                }
            }
        }
        if (selected) {
            setCursor( QCursor(Qt::OpenHandCursor) );
        } else if (selectedEdge) {
        } else if (!selectedEdge && !selected) {
            setCursor( QCursor(Qt::ArrowCursor) );
        }
    }

    bool mustUpdateNavigator = false;
    ///Apply actions
    switch (_imp->_evtState) {
    case ARROW_DRAGGING: {
        QPointF np = _imp->_arrowSelected->mapFromScene(newPos);
        if ( _imp->_arrowSelected->isOutputEdge() ) {
            _imp->_arrowSelected->dragDest(np);
        } else {
            _imp->_arrowSelected->dragSource(np);
        }
        break;
    }
    case NODE_DRAGGING:
    case BACKDROP_DRAGGING: {
        if ( !_imp->_selection.nodes.empty() || !_imp->_selection.bds.empty() ) {
            _imp->_undoStack->setActive();
            std::list<MoveMultipleNodesCommand::NodeToMove> nodesToMove;
            for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.nodes.begin();
                 it != _imp->_selection.nodes.end(); ++it) {
                MoveMultipleNodesCommand::NodeToMove n;
                n.node = *it;
                n.isWithinBD = false;
                nodesToMove.push_back(n);
            }
            //= _imp->_selection.nodes;

            if ( ( ( _imp->_evtState == BACKDROP_DRAGGING) && !modCASIsControl(e) ) ||
                 ( _imp->_evtState == NODE_DRAGGING) ) {
                ///For all backdrops also move all the nodes contained within it
                for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
                    std::list<boost::shared_ptr<NodeGui> > nodesWithinBD = getNodesWithinBackDrop(*it);
                    for (std::list<boost::shared_ptr<NodeGui> >::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                        ///add it only if it's not already in the list
                        bool found = false;
                        for (std::list<MoveMultipleNodesCommand::NodeToMove>::iterator it3 = nodesToMove.begin();
                             it3 != nodesToMove.end(); ++it3) {
                            if (it3->node == *it2) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            MoveMultipleNodesCommand::NodeToMove n;
                            n.node = *it2;
                            n.isWithinBD = true;
                            nodesToMove.push_back(n);
                        }
                    }
                }
            }
            mustUpdateNavigator = true;
            _imp->_undoStack->push( new MoveMultipleNodesCommand(nodesToMove,
                                                                 _imp->_selection.bds,
                                                                 newPos.x() - _imp->_lastScenePosClick.x(),
                                                                 newPos.y() - _imp->_lastScenePosClick.y(),newPos) );
        }

        if (_imp->_selection.nodes.size() == 1) {
            ///try to find a nearby edge
            boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.nodes.front();
            boost::shared_ptr<Natron::Node> internalNode = selectedNode->getNode();
            bool doHints = appPTR->getCurrentSettings()->isConnectionHintEnabled();

            ///for readers already connected don't show hint
            if ( ( internalNode->getMaxInputCount() == 0) && internalNode->hasOutputConnected() ) {
                doHints = false;
            } else if ( ( internalNode->getMaxInputCount() > 0) && internalNode->hasInputConnected() && internalNode->hasOutputConnected() ) {
                doHints = false;
            }

            if (doHints) {
                QRectF rect = selectedNode->mapToParent( selectedNode->boundingRect() ).boundingRect();
                double tolerance = 20;
                rect.adjust(-tolerance, -tolerance, tolerance, tolerance);

                Edge* edge = 0;
                {
                    QMutexLocker l(&_imp->_nodesMutex);
                    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
                        boost::shared_ptr<NodeGui> & n = *it;
                        if ( n != selectedNode && n->isVisible() ) {
                            edge = n->hasEdgeNearbyRect(rect);

                            ///if the edge input is the selected node don't continue
                            if ( edge && ( edge->getSource() == selectedNode) ) {
                                edge = 0;
                            }

                            if ( edge && edge->isOutputEdge() ) {
                                ///if the edge is an output edge but the node doesn't have any inputs don't continue
                                if ( selectedNode->getInputsArrows().empty() ) {
                                    edge = 0;
                                }
                                ///if the source of that edge is already connected also skip
                                const std::vector<boost::shared_ptr<Natron::Node> > & inpNodes =
                                    selectedNode->getNode()->getInputs_mt_safe();
                                for (U32 i = 0; i < inpNodes.size(); ++i) {
                                    if ( edge && ( inpNodes[i] == edge->getSource()->getNode() ) ) {
                                        edge = 0;
                                        break;
                                    }
                                }
                            }

                            if ( edge && !edge->isOutputEdge() ) {
                                ///if the edge is an input edge but the selected node can't be connected don't continue
                                if ( !selectedNode->getNode()->canOthersConnectToThisNode() ) {
                                    edge = 0;
                                }

                                ///if the selected node doesn't have any input but the edge has an input don't continue
                                if ( edge && selectedNode->getInputsArrows().empty() && edge->getSource() ) {
                                    edge = 0;
                                }
                            }

                            if (edge) {
                                edge->setUseHighlight(true);
                                break;
                            }
                        }
                    }
                }
                if ( _imp->_highLightedEdge && ( _imp->_highLightedEdge != edge) ) {
                    _imp->_highLightedEdge->setUseHighlight(false);
                    _imp->_hintInputEdge->hide();
                    _imp->_hintOutputEdge->hide();
                }

                _imp->_highLightedEdge = edge;

                if ( edge && edge->getSource() && edge->getDest() ) {
                    ///setup the hints edge

                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> > & inpNodes = selectedNode->getNode()->getInputs_mt_safe();
                    for (U32 i = 0; i < inpNodes.size(); ++i) {
                        if ( inpNodes[i] == edge->getSource()->getNode() ) {
                            alreadyConnected = true;
                            break;
                        }
                    }

                    if ( !_imp->_hintInputEdge->isVisible() ) {
                        if (!alreadyConnected) {
                            int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), selectedNode);
                            _imp->_hintInputEdge->setVisible(true);
                        }
                        _imp->_hintOutputEdge->setInputNumber( edge->getInputNumber() );
                        _imp->_hintOutputEdge->setSourceAndDestination( selectedNode, edge->getDest() );
                        _imp->_hintOutputEdge->setVisible(true);
                    } else {
                        if (!alreadyConnected) {
                            _imp->_hintInputEdge->initLine();
                        }
                        _imp->_hintOutputEdge->initLine();
                    }
                } else if (edge) {
                    ///setup only 1 of the hints edge

                    if ( _imp->_highLightedEdge && !_imp->_hintInputEdge->isVisible() ) {
                        if ( edge->isOutputEdge() ) {
                            int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), selectedNode);
                        } else {
                            _imp->_hintInputEdge->setInputNumber( edge->getInputNumber() );
                            _imp->_hintInputEdge->setSourceAndDestination( selectedNode,edge->getDest() );
                        }
                        _imp->_hintInputEdge->setVisible(true);
                    } else if ( _imp->_highLightedEdge && _imp->_hintInputEdge->isVisible() ) {
                        _imp->_hintInputEdge->initLine();
                    }
                }
            }
        }
        setCursor( QCursor(Qt::ClosedHandCursor) );
        break;
    }
    case MOVING_AREA: {
        mustUpdateNavigator = true;
        _imp->_root->moveBy(dx, dy);
        setCursor( QCursor(Qt::SizeAllCursor) );
        break;
    }
    case BACKDROP_RESIZING: {
        mustUpdateNavigator = true;
        assert(_imp->_backdropResized);
        _imp->_undoStack->setActive();
        QPointF p = _imp->_backdropResized->scenePos();
        int w = newPos.x() - p.x();
        int h = newPos.y() - p.y();
        _imp->_undoStack->push( new ResizeBackDropCommand(_imp->_backdropResized,w,h) );
        break;
    }
    case SELECTION_RECT: {
        QPointF startDrag = _imp->_selectionRect->mapFromScene(_imp->_lastSelectionStartPoint);
        QPointF cur = _imp->_selectionRect->mapFromScene(newPos);
        double xmin = std::min( cur.x(),startDrag.x() );
        double xmax = std::max( cur.x(),startDrag.x() );
        double ymin = std::min( cur.y(),startDrag.y() );
        double ymax = std::max( cur.y(),startDrag.y() );
        _imp->_selectionRect->setRect(xmin,ymin,xmax - xmin,ymax - ymin);
        break;
    }
    default:
        break;
    } // switch

    _imp->_lastScenePosClick = newPos;

    if (mustUpdateNavigator) {
        _imp->_refreshOverlays = true;
    }

    update();
} // mouseMoveEvent

void
NodeGraphPrivate::resetSelection()
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _selection.nodes.begin(); it != _selection.nodes.end(); ++it) {
        (*it)->setUserSelected(false);
    }
    for (std::list<NodeBackDrop*>::iterator it = _selection.bds.begin(); it != _selection.bds.end(); ++it) {
        (*it)->setUserSelected(false);
    }

    _selection.nodes.clear();
    _selection.bds.clear();
}

void
NodeGraphPrivate::editSelectionFromSelectionRectangle(bool addToSelection)
{
    if (!addToSelection) {
        resetSelection();
    }

    QRectF selection = _selectionRect->mapToScene( _selectionRect->rect() ).boundingRect();

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( selection.contains(bbox) ) {
            _selection.nodes.push_back(*it);
            (*it)->setUserSelected(true);
        }
    }
    for (std::list<NodeBackDrop*>::iterator it = _backdrops.begin(); it != _backdrops.end(); ++it) {
        QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( selection.contains(bbox) ) {
            _selection.bds.push_back(*it);
            (*it)->setUserSelected(true);
        }
    }
}

void
NodeGraph::mouseDoubleClickEvent(QMouseEvent* /*e*/)
{
    std::list<boost::shared_ptr<NodeGui> > nodes = getAllActiveNodes_mt_safe();

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        QPointF evpt = (*it)->mapFromScene(_imp->_lastScenePosClick);
        if ( (*it)->isVisible() && (*it)->isActive() && (*it)->contains(evpt) && (*it)->getSettingPanel() ) {
            if ( !(*it)->isSettingsPanelVisible() ) {
                (*it)->setVisibleSettingsPanel(true);
            }
            if ( !(*it)->wasBeginEditCalled() ) {
                (*it)->beginEditKnobs();
            }
            _imp->_gui->putSettingsPanelFirst( (*it)->getSettingPanel() );
            getGui()->getApp()->redrawAllViewers();

            return;
        }
    }

    for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
        if ( (*it)->isNearbyHeader(_imp->_lastScenePosClick) ) {
            if ( (*it)->isSettingsPanelClosed() ) {
                (*it)->setSettingsPanelClosed(false);
            }
            _imp->_gui->putSettingsPanelFirst( (*it)->getSettingsPanel() );

            return;
        }
    }
}

bool
NodeGraph::event(QEvent* e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        if (ke && (ke->key() == Qt::Key_Tab) && _imp->_nodeCreationShortcutEnabled) {
            NodeCreationDialog* nodeCreation = new NodeCreationDialog(this);

            ///This allows us to have a non-modal dialog: when the user clicks outside of the dialog,
            ///it closes it.
            QObject::connect( nodeCreation,SIGNAL( accepted() ),this,SLOT( onNodeCreationDialogFinished() ) );
            QObject::connect( nodeCreation,SIGNAL( rejected() ),this,SLOT( onNodeCreationDialogFinished() ) );
            nodeCreation->show();


            ke->accept();

            return true;
        }
    }

    return QGraphicsView::event(e);
}

void
NodeGraph::onNodeCreationDialogFinished()
{
    NodeCreationDialog* dialog = qobject_cast<NodeCreationDialog*>( sender() );

    if (dialog) {
        QDialog::DialogCode ret = (QDialog::DialogCode)dialog->result();
        switch (ret) {
        case QDialog::Accepted: {
            QString res = dialog->getNodeName();
            const std::vector<Natron::Plugin*> & allPlugins = appPTR->getPluginsList();
            for (U32 i = 0; i < allPlugins.size(); ++i) {
                if (allPlugins[i]->getPluginID() == res) {
                    QPointF posHint = mapToScene( mapFromGlobal( QCursor::pos() ) );
                    getGui()->getApp()->createNode( CreateNodeArgs( res,
                                                                    "",
                                                                    -1,
                                                                    -1,
                                                                    true,
                                                                    -1,
                                                                    true,
                                                                    posHint.x(),
                                                                    posHint.y() ) );
                    break;
                }
            }
            break;
        }
        case QDialog::Rejected:
        default:
            break;
        }
        dialog->deleteLater();
    }
}

void
NodeGraph::keyPressEvent(QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, modifiers, key) ) {
        QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
        QCoreApplication::postEvent(parentWidget(),ev);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateReader, modifiers, key) ) {
        _imp->_gui->createReader();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateWriter, modifiers, key) ) {
        _imp->_gui->createWriter();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRemoveNodes, modifiers, key) ) {
        deleteSelection();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphForcePreview, modifiers, key) ) {
        forceRefreshAllPreviews();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCopy, modifiers, key) ) {
        copySelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphPaste, modifiers, key) ) {
        pasteNodeClipBoards();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCut, modifiers, key) ) {
        cutSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDuplicate, modifiers, key) ) {
        duplicateSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphClone, modifiers, key) ) {
        cloneSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDeclone, modifiers, key) ) {
        decloneSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFrameNodes, modifiers, key) ) {
        centerOnAllNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphEnableHints, modifiers, key) ) {
        toggleConnectionHints();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSwitchInputs, modifiers, key) ) {
        ///No need to make an undo command for this, the user just have to do it a second time to reverse the effect
        switchInputs1and2ForSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAll, modifiers, key) ) {
        selectAllNodes(false);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAllVisible, modifiers, key) ) {
        selectAllNodes(true);
    } else if (key == Qt::Key_Control) {
        _imp->setNodesBendPointsVisible(true);
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectUp, modifiers, key) ||
                isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateUpstream, modifiers, key) ) {
        ///We try to find if the last selected node has an input, if so move selection (or add to selection)
        ///the first valid input node
        if ( !_imp->_selection.nodes.empty() ) {
            boost::shared_ptr<NodeGui> lastSelected = ( *_imp->_selection.nodes.rbegin() );
            const std::map<int,Edge*> & inputs = lastSelected->getInputsArrows();
            for (std::map<int,Edge*>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
                if ( it->second->hasSource() ) {
                    boost::shared_ptr<NodeGui> input = it->second->getSource();
                    if ( input->getIsSelected() && modCASIsShift(e) ) {
                        std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.nodes.begin(),
                                                                                           _imp->_selection.nodes.end(),lastSelected);
                        if ( found != _imp->_selection.nodes.end() ) {
                            lastSelected->setUserSelected(false);
                            _imp->_selection.nodes.erase(found);
                        }
                    } else {
                        selectNode( it->second->getSource(), modCASIsShift(e) );
                    }
                    break;
                }
            }
        }
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectDown, modifiers, key) ||
                isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateDownstream, modifiers, key) ) {
        ///We try to find if the last selected node has an output, if so move selection (or add to selection)
        ///the first valid output node
        if ( !_imp->_selection.nodes.empty() ) {
            boost::shared_ptr<NodeGui> lastSelected = ( *_imp->_selection.nodes.rbegin() );
            const std::list<boost::shared_ptr<Natron::Node> > & outputs = lastSelected->getNode()->getOutputs();
            if ( !outputs.empty() ) {
                boost::shared_ptr<NodeGui> output = getGui()->getApp()->getNodeGui( outputs.front() );
                assert(output);
                if ( output->getIsSelected() && modCASIsShift(e) ) {
                    std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.nodes.begin(),
                                                                                       _imp->_selection.nodes.end(),lastSelected);
                    if ( found != _imp->_selection.nodes.end() ) {
                        lastSelected->setUserSelected(false);
                        _imp->_selection.nodes.erase(found);
                    }
                } else {
                    selectNode( output, modCASIsShift(e) );
                }
            }
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getGui()->getLastSelectedViewer() ) {
            getGui()->getLastSelectedViewer()->previousFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( getGui()->getLastSelectedViewer() ) {
            getGui()->getLastSelectedViewer()->nextFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        if ( getGui()->getLastSelectedViewer() ) {
            getGui()->getLastSelectedViewer()->firstFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        if ( getGui()->getLastSelectedViewer() ) {
            getGui()->getLastSelectedViewer()->lastFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        if ( getGui()->getLastSelectedViewer() ) {
            getGui()->getLastSelectedViewer()->previousIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        if ( getGui()->getLastSelectedViewer() ) {
            getGui()->getLastSelectedViewer()->nextIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        getGui()->getApp()->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        getGui()->getApp()->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRearrangeNodes, modifiers, key) ) {
        _imp->rearrangeSelectedNodes();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, modifiers, key) ) {
        _imp->toggleSelectedNodesEnabled();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphShowExpressions, modifiers, key) ) {
        toggleKnobLinksVisible();
    } else {
        /// Search for a node which has a shortcut bound
        const std::vector<Natron::Plugin*> & allPlugins = appPTR->getPluginsList();
        for (U32 i = 0; i < allPlugins.size(); ++i) {
            if ( allPlugins[i]->getHasShortcut() ) {
                QString group(kShortcutGroupNodes);
                QStringList groupingSplit = allPlugins[i]->getGrouping();
                for (int j = 0; j < groupingSplit.size(); ++j) {
                    group.push_back('/');
                    group.push_back(groupingSplit[j]);
                }
                if ( isKeybind(group.toStdString().c_str(), allPlugins[i]->getPluginID().toStdString().c_str(), modifiers, key) ) {
                    QPointF hint = mapToScene( mapFromGlobal( QCursor::pos() ) );
                    getGui()->getApp()->createNode( CreateNodeArgs( allPlugins[i]->getPluginID(),"",-1,-1,true,-1,true,hint.x(),hint.y() ) );
                    break;
                }
            }
        }
    }
} // keyPressEvent

void
NodeGraphPrivate::rearrangeSelectedNodes()
{
    if ( !_selection.nodes.empty() ) {
        _undoStack->push( new RearrangeNodesCommand(_selection.nodes) );
    }
}

void
NodeGraphPrivate::setNodesBendPointsVisible(bool visible)
{
    _bendPointsVisible = visible;

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        const std::map<int,Edge*> & edges = (*it)->getInputsArrows();
        for (std::map<int,Edge*>::const_iterator it2 = edges.begin(); it2 != edges.end(); ++it2) {
            if (visible) {
                if ( !it2->second->isOutputEdge() && it2->second->hasSource() && (it2->second->line().length() > 50) ) {
                    it2->second->setBendPointVisible(visible);
                }
            } else {
                if ( !it2->second->isOutputEdge() ) {
                    it2->second->setBendPointVisible(visible);
                }
            }
        }
    }
}

void
NodeGraph::selectAllNodes(bool onlyInVisiblePortion)
{
    _imp->resetSelection();
    if (onlyInVisiblePortion) {
        QRectF r = visibleSceneRect();
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
            if ( r.intersects(bbox) && (*it)->isActive() && (*it)->isVisible() ) {
                (*it)->setUserSelected(true);
                _imp->_selection.nodes.push_back(*it);
            }
        }
        for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
            QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
            if ( r.intersects(bbox) ) {
                (*it)->setUserSelected(true);
                _imp->_selection.bds.push_back(*it);
            }
        }
    } else {
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( (*it)->isActive() && (*it)->isVisible() ) {
                (*it)->setUserSelected(true);
                _imp->_selection.nodes.push_back(*it);
            }
        }
        for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
            (*it)->setUserSelected(true);
            _imp->_selection.bds.push_back(*it);
        }
    }
}

void
NodeGraph::connectCurrentViewerToSelection(int inputNB)
{
    if ( !_imp->_gui->getLastSelectedViewer() ) {
        _imp->_gui->getApp()->createNode( CreateNodeArgs("Viewer") );
    }

    ///get a pointer to the last user selected viewer
    boost::shared_ptr<InspectorNode> v = boost::dynamic_pointer_cast<InspectorNode>( _imp->_gui->getLastSelectedViewer()->
                                                                                     getInternalNode()->getNode() );

    ///if the node is no longer active (i.e: it was deleted by the user), don't do anything.
    if ( !v->isActivated() ) {
        return;
    }

    ///get a ptr to the NodeGui
    boost::shared_ptr<NodeGui> gui = _imp->_gui->getApp()->getNodeGui(v);
    assert(gui);

    ///if there's no selected node or the viewer is selected, then try refreshing that input nb if it is connected.
    bool viewerAlreadySelected = std::find(_imp->_selection.nodes.begin(),_imp->_selection.nodes.end(),gui) != _imp->_selection.nodes.end();
    if (_imp->_selection.nodes.empty() || (_imp->_selection.nodes.size() > 1) || viewerAlreadySelected) {
        v->setActiveInputAndRefresh(inputNB);
        gui->refreshEdges();

        return;
    }

    boost::shared_ptr<NodeGui> selected = _imp->_selection.nodes.front();


    if ( !selected->getNode()->canOthersConnectToThisNode() ) {
        return;
    }

    ///if the node doesn't have the input 'inputNb' created yet, populate enough input
    ///so it can be created.
    NodeGui::InputEdgesMap::const_iterator it = gui->getInputsArrows().find(inputNB);
    while ( it == gui->getInputsArrows().end() ) {
        v->addEmptyInput();
        it = gui->getInputsArrows().find(inputNB);
    }

    ///set the undostack the active one before connecting
    _imp->_undoStack->setActive();

    ///and push a connect command to the selected node.
    _imp->_undoStack->push( new ConnectCommand(this,it->second,it->second->getSource(),selected) );

    ///Set the viewer as the selected node (also wipe the current selection)
    selectNode(gui,false);
} // connectCurrentViewerToSelection

void
NodeGraph::enterEvent(QEvent* e)
{
    QGraphicsView::enterEvent(e);

    _imp->_nodeCreationShortcutEnabled = true;
    setFocus();
}

void
NodeGraph::leaveEvent(QEvent* e)
{
    QGraphicsView::leaveEvent(e);

    _imp->_nodeCreationShortcutEnabled = false;
    setFocus();
}

void
NodeGraph::wheelEvent(QWheelEvent* e)
{
    if (e->orientation() != Qt::Vertical) {
        return;
    }
    QPointF newPos = mapToScene( e->pos() );
    double scaleFactor = pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    qreal newZoomfactor = transform().scale(scaleFactor, scaleFactor).mapRect( QRectF(0, 0, 1, 1) ).width();
    if ( (newZoomfactor < 0.07) || (newZoomfactor > 10) ) {
        return;
    }

    if (modCASIsControl(e) && _imp->_magnifiedNode) {
        if (!_imp->_magnifOn) {
            _imp->_magnifOn = true;
            _imp->_nodeSelectedScaleBeforeMagnif = _imp->_magnifiedNode->scale();
        }
        _imp->_magnifiedNode->setScale_natron(_imp->_magnifiedNode->scale() * scaleFactor);
    } else {
//        QPointF centerScene = visibleSceneRect().center();
//        QPointF deltaScene;
//        deltaScene.rx() = newPos.x() - centerScene.x();
//        deltaScene.ry() = newPos.y() - centerScene.y();
//        QTransform t = transform();
//        QTransform mapping;
//        mapping.translate(-deltaScene.x(),-deltaScene.y());
//        mapping.scale(scaleFactor,scaleFactor);
//        mapping.translate(deltaScene.x(),deltaScene.y());
//        t.translate(-deltaScene.x(),-deltaScene.y());
//        t.scale(scaleFactor,scaleFactor);
//        t.translate(deltaScene.x(),deltaScene.y());
//        setTransform(t);
//        centerScene = mapping.map(centerScene);
        scale(scaleFactor,scaleFactor);
//        centerOn(centerScene);
        _imp->_refreshOverlays = true;
    }
    _imp->_lastScenePosClick = newPos;
}

void
NodeGraph::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        if (_imp->_magnifOn) {
            _imp->_magnifOn = false;
            _imp->_magnifiedNode->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
        }
        if (_imp->_bendPointsVisible) {
            _imp->setNodesBendPointsVisible(false);
        }
    }
}

void
NodeGraph::removeNode(const boost::shared_ptr<NodeGui> & node)
{
    const std::vector<boost::shared_ptr<KnobI> > & knobs = node->getNode()->getKnobs();

    for (U32 i = 0; i < knobs.size(); ++i) {
        std::list<KnobI*> listeners;
        knobs[i]->getListeners(listeners);
        ///For all listeners make sure they belong to a node
        bool foundEffect = false;
        for (std::list<KnobI*>::iterator it2 = listeners.begin(); it2 != listeners.end(); ++it2) {
            EffectInstance* isEffect = dynamic_cast<EffectInstance*>( (*it2)->getHolder() );
            if ( isEffect && ( isEffect != node->getNode()->getLiveInstance() ) ) {
                foundEffect = true;
                break;
            }
        }
        if (foundEffect) {
            Natron::StandardButton reply = Natron::questionDialog( tr("Delete").toStdString(), tr("This node has one or several "
                                                                                                  "parameters from which other parameters "
                                                                                                  "of the project rely on through expressions "
                                                                                                  "or links. Deleting this node will "
                                                                                                  "remove these expressions permanantly "
                                                                                                  "and undoing the action will not recover "
                                                                                                  "them. Do you wish to continue ?")
                                                                   .toStdString() );
            if (reply == Natron::No) {
                return;
            }
            break;
        }
    }

    node->setUserSelected(false);
    std::list<boost::shared_ptr<NodeGui> > nodesToRemove;
    std::list<NodeBackDrop*> bds;
    nodesToRemove.push_back(node);
    _imp->_undoStack->setActive();
    _imp->_undoStack->push( new RemoveMultipleNodesCommand(this,nodesToRemove,bds) );
}

void
NodeGraph::deleteSelection()
{
    if ( !_imp->_selection.nodes.empty() || !_imp->_selection.bds.empty() ) {
        std::list<boost::shared_ptr<NodeGui> > nodesToRemove = _imp->_selection.nodes;

        ///For all backdrops also move all the nodes contained within it
        for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
            std::list<boost::shared_ptr<NodeGui> > nodesWithinBD = getNodesWithinBackDrop(*it);
            for (std::list<boost::shared_ptr<NodeGui> >::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(nodesToRemove.begin(),nodesToRemove.end(),*it2);
                if ( found == nodesToRemove.end() ) {
                    nodesToRemove.push_back(*it2);
                }
            }
        }


        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it) {
            const std::vector<boost::shared_ptr<KnobI> > & knobs = (*it)->getNode()->getKnobs();
            bool mustBreak = false;
            for (U32 i = 0; i < knobs.size(); ++i) {
                std::list<KnobI*> listeners;
                knobs[i]->getListeners(listeners);

                ///For all listeners make sure they belong to a node
                bool foundEffect = false;
                for (std::list<KnobI*>::iterator it2 = listeners.begin(); it2 != listeners.end(); ++it2) {
                    EffectInstance* isEffect = dynamic_cast<EffectInstance*>( (*it2)->getHolder() );
                    if ( isEffect && ( isEffect != (*it)->getNode()->getLiveInstance() ) ) {
                        foundEffect = true;
                        break;
                    }
                }
                if (foundEffect) {
                    Natron::StandardButton reply = Natron::questionDialog( tr("Delete").toStdString(),
                                                                           tr("This node has one or several "
                                                                              "parameters from which other parameters "
                                                                              "of the project rely on through expressions "
                                                                              "or links. Deleting this node will "
                                                                              "remove these expressions permanantly "
                                                                              "and undoing the action will not recover "
                                                                              "them. Do you wish to continue ?")
                                                                           .toStdString() );
                    if (reply == Natron::No) {
                        return;
                    }
                    mustBreak = true;
                    break;
                }
            }
            if (mustBreak) {
                break;
            }
        }

        for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
            (*it)->setUserSelected(false);
        }

        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodesToRemove.begin(); it != nodesToRemove.end(); ++it) {
            (*it)->setUserSelected(false);
        }


        _imp->_undoStack->setActive();
        _imp->_undoStack->push( new RemoveMultipleNodesCommand(this,nodesToRemove,_imp->_selection.bds) );
        _imp->_selection.nodes.clear();
        _imp->_selection.bds.clear();
    }
} // deleteSelection

void
NodeGraph::selectNode(const boost::shared_ptr<NodeGui> & n,
                      bool addToSelection)
{
    if ( !n->isActive() || !n->isVisible() ) {
        return;
    }
    bool alreadyInSelection = std::find(_imp->_selection.nodes.begin(),_imp->_selection.nodes.end(),n) != _imp->_selection.nodes.end();


    assert(n);
    if (addToSelection && !alreadyInSelection) {
        _imp->_selection.nodes.push_back(n);
    } else if (!addToSelection) {
        {
            QMutexLocker l(&_imp->_nodesMutex);
            for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.nodes.begin(); it != _imp->_selection.nodes.end(); ++it) {
                (*it)->setUserSelected(false);
            }
        }
        for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
            (*it)->setUserSelected(false);
        }
        _imp->_selection.nodes.clear();
        _imp->_selection.bds.clear();
        _imp->_selection.nodes.push_back(n);
    }

    n->setUserSelected(true);

    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( n->getNode()->getLiveInstance() );
    if (isViewer) {
        OpenGLViewerI* viewer = isViewer->getUiContext();
        const std::list<ViewerTab*> & viewerTabs = _imp->_gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewer ) {
                _imp->_gui->setLastSelectedViewer( (*it) );
            }
        }
    }

    bool magnifiedNodeSelected = false;
    if (_imp->_magnifiedNode) {
        magnifiedNodeSelected = std::find(_imp->_selection.nodes.begin(),_imp->_selection.nodes.end(),_imp->_magnifiedNode)
                                != _imp->_selection.nodes.end();
    }
    if (magnifiedNodeSelected && _imp->_magnifOn) {
        _imp->_magnifOn = false;
        _imp->_magnifiedNode->setScale_natron(_imp->_nodeSelectedScaleBeforeMagnif);
    }
}

void
NodeGraph::selectBackDrop(NodeBackDrop* bd,
                          bool addToSelection)
{
    bool alreadyInSelection = std::find(_imp->_selection.bds.begin(),_imp->_selection.bds.end(),bd) != _imp->_selection.bds.end();

    assert(bd);
    if (addToSelection && !alreadyInSelection) {
        _imp->_selection.bds.push_back(bd);
    } else if (!addToSelection) {
        {
            QMutexLocker l(&_imp->_nodesMutex);
            for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.nodes.begin(); it != _imp->_selection.nodes.end(); ++it) {
                (*it)->setUserSelected(false);
            }
        }
        for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
            (*it)->setUserSelected(false);
        }
        _imp->_selection.bds.clear();
        _imp->_selection.nodes.clear();
        _imp->_selection.bds.push_back(bd);
    }
    bd->setUserSelected(true);
}

void
NodeGraph::updateNavigator()
{
    if ( !areAllNodesVisible() ) {
        _imp->_navigator->setPixmap( QPixmap::fromImage( getFullSceneScreenShot() ) );
        _imp->_navigator->show();
    } else {
        _imp->_navigator->hide();
    }
}

bool
NodeGraph::areAllNodesVisible()
{
    QRectF rect = visibleSceneRect();
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ( (*it)->isVisible() ) {
            if ( !rect.contains( (*it)->boundingRectWithEdges() ) ) {
                return false;
            }
        }
    }
    for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
        QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( !rect.contains(bbox) ) {
            return false;
        }
    }

    return true;
}

QImage
NodeGraph::getFullSceneScreenShot()
{
    ///The bbox of all nodes in the nodegraph
    QRectF sceneR = _imp->calcNodesBoundingRect();

    ///The visible portion of the nodegraph
    QRectF viewRect = visibleSceneRect();

    sceneR = sceneR.united(viewRect);
    ///Make sure the visible rect is included in the scene rect
    //viewRect = viewRect.intersect(sceneR);

    double navWidth = width() * NATRON_NAVIGATOR_BASE_WIDTH;
    double navHeight = height() * NATRON_NAVIGATOR_BASE_HEIGHT;

    ///Make sceneR and viewRect keep the same aspect ratio as the navigator
    double xScale = navWidth / sceneR.width();
    double yScale =  navHeight / sceneR.height();
    double scaleFactor = std::min(xScale,yScale);
    QRectF renderRect(0,0,sceneR.width() * scaleFactor, sceneR.height() * scaleFactor);

    ///Render the scene in an image with the same aspect ratio  as the scene rect
    QImage renderImage(renderRect.width(),renderRect.height(),QImage::Format_ARGB32_Premultiplied);
    ///Fill the background
    renderImage.fill( QColor(71,71,71,255) );

    ///Offset the visible rect corner as an offset relative to the scene rect corner
    viewRect.setX( viewRect.x() - sceneR.x() );
    viewRect.setY( viewRect.y() - sceneR.y() );
    viewRect.setWidth( viewRect.width() - sceneR.x() );
    viewRect.setHeight( viewRect.height() - sceneR.y() );

    QRectF scaledViewRect = viewRect;
    scaledViewRect.setLeft(viewRect.left() * scaleFactor);
    scaledViewRect.setBottom(viewRect.bottom() * scaleFactor);
    scaledViewRect.setRight(viewRect.right() * scaleFactor);
    scaledViewRect.setTop(viewRect.top() * scaleFactor);

    ///Paint the visible portion with a highlight
    QPainter painter(&renderImage);

    ///Remove the overlays from the scene before rendering it
    scene()->removeItem(_imp->_cacheSizeText);
    scene()->removeItem(_imp->_navigator);

    ///Render into the QImage with downscaling
    scene()->render(&painter,renderImage.rect(),sceneR,Qt::KeepAspectRatio);

    ///Add the overlays back
    scene()->addItem(_imp->_navigator);
    scene()->addItem(_imp->_cacheSizeText);

    painter.fillRect( scaledViewRect, QColor(200,200,200,100) );
    QPen p;
    p.setWidth(2);
    p.setBrush(Qt::yellow);
    painter.setPen(p);
    ///Make sure the border is visible
    scaledViewRect.adjust(2, 2, -2, -2);
    painter.drawRect(scaledViewRect);

    ///Now make an image of the size of the navigator and center the render image into it
    QImage img(std::ceil(navWidth), std::ceil(navHeight), QImage::Format_ARGB32_Premultiplied);
    img.fill( QColor(71,71,71,255) );

    int xOffset = ( img.width() - renderImage.width() ) / 2;
    int yOffset = ( img.height() - renderImage.height() ) / 2;
    assert( ( xOffset + renderImage.width() ) <= img.width() && ( yOffset + renderImage.height() ) <= img.height() );

    int yDest = yOffset;
    for (int y = 0; y < renderImage.height(); ++y,++yDest) {
        QRgb* dst_pixels = (QRgb*)img.scanLine(yDest);
        const QRgb* src_pixels = (const QRgb*)renderImage.scanLine(y);
        int xDest = xOffset;
        for (int x = 0; x < renderImage.width(); ++x,++xDest) {
            dst_pixels[xDest] = src_pixels[x];
        }
    }

    return img;
} // getFullSceneScreenShot

const std::list<boost::shared_ptr<NodeGui> > &
NodeGraph::getAllActiveNodes() const
{
    return _imp->_nodes;
}

std::list<boost::shared_ptr<NodeGui> >
NodeGraph::getAllActiveNodes_mt_safe() const
{
    QMutexLocker l(&_imp->_nodesMutex);

    return _imp->_nodes;
}

void
NodeGraph::moveToTrash(NodeGui* node)
{
    assert(node);
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ( (*it).get() == node ) {
            _imp->_nodesTrash.push_back(*it);
            _imp->_nodes.erase(it);
            break;
        }
    }
}

void
NodeGraph::restoreFromTrash(NodeGui* node)
{
    assert(node);
    QMutexLocker l(&_imp->_nodesMutex);
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
        if ( (*it).get() == node ) {
            _imp->_nodes.push_back(*it);
            _imp->_nodesTrash.erase(it);
            break;
        }
    }
}

void
NodeGraph::refreshAllEdges()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->refreshEdges();
    }
}

// grabbed from QDirModelPrivate::size() in qtbase/src/widgets/itemviews/qdirmodel.cpp
static
QString
QDirModelPrivate_size(quint64 bytes)
{
    // According to the Si standard KB is 1000 bytes, KiB is 1024
    // but on windows sizes are calulated by dividing by 1024 so we do what they do.
    const quint64 kb = 1024;
    const quint64 mb = 1024 * kb;
    const quint64 gb = 1024 * mb;
    const quint64 tb = 1024 * gb;

    if (bytes >= tb) {
        return QFileSystemModel::tr("%1 TB").arg( QLocale().toString(qreal(bytes) / tb, 'f', 3) );
    }
    if (bytes >= gb) {
        return QFileSystemModel::tr("%1 GB").arg( QLocale().toString(qreal(bytes) / gb, 'f', 2) );
    }
    if (bytes >= mb) {
        return QFileSystemModel::tr("%1 MB").arg( QLocale().toString(qreal(bytes) / mb, 'f', 1) );
    }
    if (bytes >= kb) {
        return QFileSystemModel::tr("%1 KB").arg( QLocale().toString(bytes / kb) );
    }

    return QFileSystemModel::tr("%1 byte(s)").arg( QLocale().toString(bytes) );
}

void
NodeGraph::updateCacheSizeText()
{
    _imp->_cacheSizeText->setPlainText( tr("Memory cache size: %1")
                                        .arg( QDirModelPrivate_size( appPTR->getCachesTotalMemorySize() ) ) );
}

QRectF
NodeGraphPrivate::calcNodesBoundingRect()
{
    QRectF ret;
    QMutexLocker l(&_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        if ( (*it)->isVisible() ) {
            ret = ret.united( (*it)->boundingRectWithEdges() );
        }
    }
    for (std::list<NodeBackDrop*>::iterator it = _backdrops.begin(); it != _backdrops.end(); ++it) {
        ret = ret.united( (*it)->mapToScene( (*it)->boundingRect() ).boundingRect() );
    }

    return ret;
}

void
NodeGraph::toggleCacheInfos()
{
    if ( _imp->_cacheSizeText->isVisible() ) {
        _imp->_cacheSizeText->hide();
    } else {
        _imp->_cacheSizeText->show();
    }
}

void
NodeGraph::populateMenu()
{
    _imp->_menu->clear();


    QMenu* editMenu = new QMenu(tr("Edit"),_imp->_menu);
    editMenu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );
    _imp->_menu->addAction( editMenu->menuAction() );

    QAction* copyAction = new QAction(tr("Copy"),editMenu);
    copyAction->setShortcut(QKeySequence::Copy);
    QObject::connect( copyAction,SIGNAL( triggered() ),this,SLOT( copySelectedNodes() ) );
    editMenu->addAction(copyAction);

    QAction* cutAction = new QAction(tr("Cut"),editMenu);
    cutAction->setShortcut(QKeySequence::Cut);
    QObject::connect( cutAction,SIGNAL( triggered() ),this,SLOT( cutSelectedNodes() ) );
    editMenu->addAction(cutAction);


    QAction* pasteAction = new QAction(tr("Paste"),editMenu);
    pasteAction->setShortcut(QKeySequence::Paste);
    pasteAction->setEnabled( !_imp->_nodeClipBoard.isEmpty() );
    QObject::connect( pasteAction,SIGNAL( triggered() ),this,SLOT( pasteNodeClipBoards() ) );
    editMenu->addAction(pasteAction);

    QAction* deleteAction = new QAction(tr("Delete"),editMenu);
    deleteAction->setShortcut(QKeySequence::Delete);
    QObject::connect( deleteAction,SIGNAL( triggered() ),this,SLOT( deleteSelection() ) );
    editMenu->addAction(deleteAction);

    QAction* duplicateAction = new QAction(tr("Duplicate"),editMenu);
    duplicateAction->setShortcut( QKeySequence(Qt::AltModifier + Qt::Key_C) );
    QObject::connect( duplicateAction,SIGNAL( triggered() ),this,SLOT( duplicateSelectedNodes() ) );
    editMenu->addAction(duplicateAction);

    QAction* cloneAction = new QAction(tr("Clone"),editMenu);
    cloneAction->setShortcut( QKeySequence(Qt::AltModifier + Qt::Key_K) );
    QObject::connect( cloneAction,SIGNAL( triggered() ),this,SLOT( cloneSelectedNodes() ) );
    editMenu->addAction(cloneAction);

    QAction* decloneAction = new QAction(tr("Declone"),editMenu);
    decloneAction->setShortcut( QKeySequence(Qt::AltModifier + Qt::ShiftModifier + Qt::Key_K) );
    QObject::connect( decloneAction,SIGNAL( triggered() ),this,SLOT( decloneSelectedNodes() ) );
    editMenu->addAction(decloneAction);

    QAction* switchInputs = new QAction(tr("Switch inputs 1 & 2"),editMenu);
    QObject::connect( switchInputs, SIGNAL( triggered() ), this, SLOT( switchInputs1and2ForSelectedNodes() ) );
    switchInputs->setShortcut( QKeySequence(Qt::SHIFT + Qt::Key_X) );
    editMenu->addAction(switchInputs);


    QAction* displayCacheInfoAction = new QAction(tr("Display memory consumption"),this);
    displayCacheInfoAction->setCheckable(true);
    displayCacheInfoAction->setChecked( _imp->_cacheSizeText->isVisible() );
    QObject::connect( displayCacheInfoAction,SIGNAL( triggered() ),this,SLOT( toggleCacheInfos() ) );
    _imp->_menu->addAction(displayCacheInfoAction);

    QAction* turnOffPreviewAction = new QAction(tr("Toggle on/off previews"),this);
    turnOffPreviewAction->setCheckable(true);
    turnOffPreviewAction->setChecked(false);
    QObject::connect( turnOffPreviewAction,SIGNAL( triggered() ),this,SLOT( togglePreviewsForSelectedNodes() ) );
    _imp->_menu->addAction(turnOffPreviewAction);

    QAction* connectionHints = new QAction(tr("Use connections hint"),this);
    connectionHints->setCheckable(true);
    connectionHints->setChecked( appPTR->getCurrentSettings()->isConnectionHintEnabled() );
    QObject::connect( connectionHints,SIGNAL( triggered() ),this,SLOT( toggleConnectionHints() ) );
    _imp->_menu->addAction(connectionHints);

    QAction* knobLinks = new QAction(tr("Display parameters links"),this);
    knobLinks->setCheckable(true);
    knobLinks->setChecked( areKnobLinksVisible() );
    knobLinks->setShortcut( QKeySequence(Qt::ShiftModifier + Qt::Key_E) );
    QObject::connect( knobLinks,SIGNAL( triggered() ),this,SLOT( toggleKnobLinksVisible() ) );
    _imp->_menu->addAction(knobLinks);

    QAction* autoPreview = new QAction(tr("Enable auto preview"),this);
    autoPreview->setCheckable(true);
    autoPreview->setChecked( _imp->_gui->getApp()->getProject()->isAutoPreviewEnabled() );
    QObject::connect( autoPreview,SIGNAL( triggered() ),this,SLOT( toggleAutoPreview() ) );
    QObject::connect( _imp->_gui->getApp()->getProject().get(),SIGNAL( autoPreviewChanged(bool) ),autoPreview,SLOT( setChecked(bool) ) );
    _imp->_menu->addAction(autoPreview);

    QAction* forceRefreshPreviews = new QAction(tr("Refresh previews"),this);
    forceRefreshPreviews->setShortcut( QKeySequence(Qt::Key_P) );
    QObject::connect( forceRefreshPreviews,SIGNAL( triggered() ),this,SLOT( forceRefreshAllPreviews() ) );
    _imp->_menu->addAction(forceRefreshPreviews);

    QAction* frameAllNodes = new QAction(tr("Frame nodes"),this);
    frameAllNodes->setShortcut( QKeySequence(Qt::Key_F) );
    QObject::connect( frameAllNodes,SIGNAL( triggered() ),this,SLOT( centerOnAllNodes() ) );
    _imp->_menu->addAction(frameAllNodes);

    _imp->_menu->addSeparator();

    std::list<ToolButton*> orederedToolButtons = _imp->_gui->getToolButtonsOrdered();
    for (std::list<ToolButton*>::iterator it = orederedToolButtons.begin(); it != orederedToolButtons.end(); ++it) {
        (*it)->getMenu()->setIcon( (*it)->getIcon() );
        _imp->_menu->addAction( (*it)->getMenu()->menuAction() );
    }
} // populateMenu

void
NodeGraph::toggleKnobLinksVisible()
{
    _imp->_knobLinksVisible = !_imp->_knobLinksVisible;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            (*it)->setKnobLinksVisible(_imp->_knobLinksVisible);
        }
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
            (*it)->setKnobLinksVisible(_imp->_knobLinksVisible);
        }
    }
}

void
NodeGraph::toggleAutoPreview()
{
    _imp->_gui->getApp()->getProject()->toggleAutoPreview();
}

void
NodeGraph::forceRefreshAllPreviews()
{
    _imp->_gui->forceRefreshAllPreviews();
}

void
NodeGraph::showMenu(const QPoint & pos)
{
    populateMenu();
    _imp->_menu->exec(pos);
}

void
NodeGraph::dropEvent(QDropEvent* e)
{
    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    QStringList filesList;
    QList<QUrl> urls = e->mimeData()->urls();
    for (int i = 0; i < urls.size(); ++i) {
        const QUrl & rl = urls.at(i);
        QString path = rl.path();

#ifdef __NATRON_WIN32__
        if ( !path.isEmpty() && ( path.at(0) == QChar('/') ) || ( path.at(0) == QChar('\\') ) ) {
            path = path.remove(0,1);
        }
#endif
        QDir dir(path);

        //if the path dropped is not a directory append it
        if ( !dir.exists() ) {
            filesList << path;
        } else {
            //otherwise append everything inside the dir recursively
            SequenceFileDialog::appendFilesFromDirRecursively(&dir,&filesList);
        }
    }

    QStringList supportedExtensions;
    std::map<std::string,std::string> writersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    for (std::map<std::string,std::string>::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it) {
        supportedExtensions.push_back( it->first.c_str() );
    }

    std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> > files = SequenceFileDialog::fileSequencesFromFilesList(filesList,supportedExtensions);
    std::locale local;
    for (U32 i = 0; i < files.size(); ++i) {
        ///get all the decoders
        std::map<std::string,std::string> readersForFormat;
        appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);

        boost::shared_ptr<SequenceParsing::SequenceFromFiles> & sequence = files[i];

        ///find a decoder for this file type
        std::string ext = sequence->fileExtension();
        std::string extLower;
        for (size_t j = 0; j < ext.size(); ++j) {
            extLower.append( 1,std::tolower( ext.at(j),local ) );
        }
        std::map<std::string,std::string>::iterator found = readersForFormat.find(extLower);
        if ( found == readersForFormat.end() ) {
            errorDialog("Reader", "No plugin capable of decoding " + extLower + " was found.");
        } else {
            boost::shared_ptr<Natron::Node>  n = getGui()->getApp()->createNode( CreateNodeArgs(found->second.c_str(),"",-1,-1,false) );
            const std::vector<boost::shared_ptr<KnobI> > & knobs = n->getKnobs();
            for (U32 i = 0; i < knobs.size(); ++i) {
                if ( knobs[i]->typeName() == File_Knob::typeNameStatic() ) {
                    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
                    assert(fk);

                    if ( !fk->isAnimationEnabled() && (sequence->count() > 1) ) {
                        errorDialog( tr("Reader").toStdString(), tr("This plug-in doesn't support image sequences, please select only 1 file.").toStdString() );
                        break;
                    } else {
                        fk->setValue(sequence->generateValidSequencePattern(),0);
                        if ( n->isPreviewEnabled() ) {
                            n->computePreviewImage( _imp->_gui->getApp()->getTimeLine()->currentFrame() );
                        }
                        break;
                    }
                }
            }
        }
    }
} // dropEvent

void
NodeGraph::dragEnterEvent(QDragEnterEvent* e)
{
    e->accept();
}

void
NodeGraph::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}

void
NodeGraph::dragMoveEvent(QDragMoveEvent* e)
{
    e->accept();
}

void
NodeGraph::togglePreviewsForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.nodes.begin();
         it != _imp->_selection.nodes.end();
         ++it) {
        (*it)->togglePreview();
    }
}

void
NodeGraph::switchInputs1and2ForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.nodes.begin();
         it != _imp->_selection.nodes.end();
         ++it) {
        (*it)->onSwitchInputActionTriggered();
    }
}

void
NodeGraph::centerOnNode(const boost::shared_ptr<NodeGui> & n)
{
    _imp->_refreshOverlays = true;
    centerOn( n.get() );
}

void
NodeGraph::copySelectedNodes()
{
    if ( _imp->_selection.nodes.empty() && _imp->_selection.bds.empty() ) {
        Natron::warningDialog( tr("Copy").toStdString(), tr("You must select at least a node to copy first.").toStdString() );

        return;
    }

    _imp->copyNodesInternal(_imp->_nodeClipBoard);
}

void
NodeGraphPrivate::resetAllClipboards()
{
    _nodeClipBoard.nodesUI.clear();
    _nodeClipBoard.nodes.clear();
    _nodeClipBoard.bds.clear();
}

void
NodeGraph::cutSelectedNodes()
{
    if ( _imp->_selection.nodes.empty() && _imp->_selection.bds.empty() ) {
        Natron::warningDialog( tr("Cut").toStdString(), tr("You must select at least a node to cut first.").toStdString() );

        return;
    }
    copySelectedNodes();
    deleteSelection();
}

void
NodeGraph::pasteNodeClipBoards()
{
    _imp->pasteNodesInternal(_imp->_nodeClipBoard);
}

void
NodeGraphPrivate::copyNodesInternal(NodeClipBoard & clipboard)
{
    ///Clear clipboard
    clipboard.bds.clear();
    clipboard.nodes.clear();
    clipboard.nodesUI.clear();

    std::list<boost::shared_ptr<NodeGui> > nodesToCopy = _selection.nodes;
    for (std::list<NodeBackDrop*>::iterator it = _selection.bds.begin(); it != _selection.bds.end(); ++it) {
        boost::shared_ptr<NodeBackDropSerialization> bdS( new NodeBackDropSerialization() );
        bdS->initialize(*it);
        clipboard.bds.push_back(bdS);

        ///Also copy all nodes within the backdrop
        std::list<boost::shared_ptr<NodeGui> > nodesWithinBD = _publicInterface->getNodesWithinBackDrop(*it);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
            std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(nodesToCopy.begin(),nodesToCopy.end(),*it2);
            if ( found == nodesToCopy.end() ) {
                nodesToCopy.push_back(*it2);
            }
        }
    }

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        if ( (*it)->getNode()->isMultiInstance() ) {
            QString err = QString("%1 cannot be copied.").arg( (*it)->getNode()->getName().c_str() );
            Natron::errorDialog( "Copy", err.toStdString() );

            return;
        }
    }

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        boost::shared_ptr<NodeSerialization> ns( new NodeSerialization( (*it)->getNode() ) );
        boost::shared_ptr<NodeGuiSerialization> nGuiS(new NodeGuiSerialization);
        (*it)->serialize( nGuiS.get() );
        clipboard.nodes.push_back(ns);
        clipboard.nodesUI.push_back(nGuiS);
    }
}

void
NodeGraphPrivate::pasteNodesInternal(const NodeClipBoard & clipboard)
{
    if ( !clipboard.isEmpty() ) {
        std::list<boost::shared_ptr<NodeGui> > newNodes;
        std::list<NodeBackDrop*> newBds;
        double xmax = INT_MIN;
        double xmin = INT_MAX;
        ///find out what is the right most X coordinate and offset relative to that coordinate
        for (std::list <boost::shared_ptr<NodeBackDropSerialization> >::const_iterator it = clipboard.bds.begin();
             it != clipboard.bds.end(); ++it) {
            double x,y;
            int w,h;
            (*it)->getPos(x, y);
            (*it)->getSize(w, h);
            if ( (x + w) > xmax ) {
                xmax = x + w;
            }
            if (x < xmin) {
                xmin = x;
            }
        }
        for (std::list<boost::shared_ptr<NodeGuiSerialization> >::const_iterator it = clipboard.nodesUI.begin();
             it != clipboard.nodesUI.end(); ++it) {
            double x = (*it)->getX();
            if (x > xmax) {
                xmax = x;
            }
            if (x < xmin) {
                xmin = x;
            }
        }


        ///offset of 100 pixels to be sure we copy on the right side of any node (true size of nodes isn't serializd unlike
        ///the size of the backdrops)
        xmax += 100;

        double offset = xmax - xmin;

        for (std::list <boost::shared_ptr<NodeBackDropSerialization> >::const_iterator it = clipboard.bds.begin();
             it != clipboard.bds.end(); ++it) {
            NodeBackDrop* bd = pasteBackdrop( **it,QPointF(offset,0) );
            newBds.push_back(bd);
        }


        assert( clipboard.nodes.size() == clipboard.nodesUI.size() );
        std::list<boost::shared_ptr<NodeSerialization> >::const_iterator itOther = clipboard.nodes.begin();
        for (std::list<boost::shared_ptr<NodeGuiSerialization> >::const_iterator it = clipboard.nodesUI.begin();
             it != clipboard.nodesUI.end(); ++it,++itOther) {
            boost::shared_ptr<NodeGui> node = pasteNode( **itOther,**it,QPointF(offset,0) );
            newNodes.push_back(node);
        }
        assert( clipboard.nodes.size() == newNodes.size() );

        ///Now that all nodes have been duplicated, try to restore nodes connections
        restoreConnections(clipboard.nodes, newNodes);


        _undoStack->setActive();
        _undoStack->push( new AddMultipleNodesCommand(_publicInterface,newNodes,newBds) );
    }
} // pasteNodesInternal

boost::shared_ptr<NodeGui>
NodeGraphPrivate::pasteNode(const NodeSerialization & internalSerialization,
                            const NodeGuiSerialization & guiSerialization,
                            const QPointF & offset)
{
    boost::shared_ptr<Natron::Node> n = _gui->getApp()->loadNode( LoadNodeArgs(internalSerialization.getPluginID().c_str(),
                                                                               "",
                                                                               internalSerialization.getPluginMajorVersion(),
                                                                               internalSerialization.getPluginMinorVersion(),&internalSerialization,false) );

    assert(n);
    boost::shared_ptr<NodeGui> gui = _gui->getApp()->getNodeGui(n);
    assert(gui);

    std::stringstream ss;
    ss << internalSerialization.getPluginLabel();
    ss << (" - copy");
    std::string bearName = ss.str();
    int no = 0;
    while ( _publicInterface->checkIfNodeNameExists( ss.str(),gui.get() ) ) {
        ++no;
        ss.str( std::string() );
        ss.clear();
        ss << bearName;
        ss << no;
    }

    n->setName( ss.str().c_str() );

    const std::string & masterNodeName = internalSerialization.getMasterNodeName();
    if ( masterNodeName.empty() ) {
        std::vector<boost::shared_ptr<Natron::Node> > allNodes;
        _gui->getApp()->getActiveNodes(&allNodes);
        n->restoreKnobsLinks(internalSerialization,allNodes);
    } else {
        boost::shared_ptr<Natron::Node> masterNode = _gui->getApp()->getProject()->getNodeByName(masterNodeName);

        ///the node could not exist any longer if the user deleted it in the meantime
        if ( masterNode && masterNode->isActivated() ) {
            n->getLiveInstance()->slaveAllKnobs( masterNode->getLiveInstance() );
        }
    }


    gui->copyFrom(guiSerialization);
    QPointF newPos = gui->pos() + offset;
    gui->refreshPosition( newPos.x(), newPos.y() );
    gui->forceComputePreview( _gui->getApp()->getProject()->currentFrame() );

    return gui;
}

void
NodeGraphPrivate::restoreConnections(const std::list<boost::shared_ptr<NodeSerialization> > & serializations,
                                     const std::list<boost::shared_ptr<NodeGui> > & newNodes)
{
    ///For all nodes restore its connections
    std::list<boost::shared_ptr<NodeSerialization> >::const_iterator itSer = serializations.begin();

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = newNodes.begin(); it != newNodes.end(); ++it,++itSer) {
        const std::vector<std::string> & inputNames = (*itSer)->getInputs();

        ///Restore each input
        for (U32 i = 0; i < inputNames.size(); ++i) {
            if ( inputNames[i].empty() ) {
                continue;
            }
            ///find a node with the containing the same name. It should not match exactly because there's already
            /// the "-copy" that was added to its name
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it2 = newNodes.begin(); it2 != newNodes.end(); ++it2) {
                if ( (*it2)->getNode()->getName().find(inputNames[i]) != std::string::npos ) {
                    _publicInterface->getGui()->getApp()->getProject()->connectNodes( i, (*it2)->getNode(), (*it)->getNode() );
                    break;
                }
            }
        }
    }
}

NodeBackDrop*
NodeGraphPrivate::pasteBackdrop(const NodeBackDropSerialization & serialization,
                                const QPointF & offset)
{
    NodeBackDrop* bd = new NodeBackDrop(_publicInterface,_root);
    QString name( serialization.getName().c_str() );

    name.append(" - copy");
    QString bearName = name;
    int no = 0;
    while ( _publicInterface->checkIfBackDropNameExists(name,NULL) ) {
        ++no;
        name = bearName;
        name.append( QString::number(no) );
    }

    bd->initialize( name, true,serialization,_gui->getPropertiesLayout() );
    _publicInterface->insertNewBackDrop(bd);
    double x,y;
    serialization.getPos(x, y);
    bd->setPos_mt_safe(QPointF(x,y) + offset);

    return bd;
}

void
NodeGraph::duplicateSelectedNodes()
{
    if ( _imp->_selection.nodes.empty() && _imp->_selection.bds.empty() ) {
        Natron::warningDialog( tr("Duplicate").toStdString(), tr("You must select at least a node to duplicate first.").toStdString() );

        return;
    }

    ///Don't use the member clipboard as the user might have something copied
    NodeClipBoard tmpClipboard;
    _imp->copyNodesInternal(tmpClipboard);
    _imp->pasteNodesInternal(tmpClipboard);
}

void
NodeGraph::cloneSelectedNodes()
{
    if ( _imp->_selection.nodes.empty() && _imp->_selection.bds.empty() ) {
        Natron::warningDialog( tr("Clone").toStdString(), tr("You must select at least a node to clone first.").toStdString() );

        return;
    }

    double xmax = INT_MIN;
    double xmin = INT_MAX;
    std::list<boost::shared_ptr<NodeGui> > nodesToCopy = _imp->_selection.nodes;
    for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
        if ( (*it)->isSlave() ) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString() );

            return;
        }
        QRectF bbox = (*it)->boundingRect();
        if ( ( bbox.x() + bbox.width() ) > xmax ) {
            xmax = ( bbox.x() + bbox.width() );
        }
        if (bbox.x() < xmin) {
            xmin = bbox.x();
        }

        ///Also copy all nodes within the backdrop
        std::list<boost::shared_ptr<NodeGui> > nodesWithinBD = getNodesWithinBackDrop(*it);
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
            std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(nodesToCopy.begin(),nodesToCopy.end(),*it2);
            if ( found == nodesToCopy.end() ) {
                nodesToCopy.push_back(*it2);
            }
        }
    }

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        QRectF bbox = (*it)->boundingRect();
        if ( ( bbox.x() + bbox.width() ) > xmax ) {
            xmax = bbox.x() + bbox.width();
        }
        if (bbox.x() < xmin) {
            xmin = bbox.x();
        }
        if ( (*it)->getNode()->getLiveInstance()->isSlave() ) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("You cannot clone a node which is already a clone.").toStdString() );

            return;
        }
        if ( (*it)->getNode()->getPluginID() == "Viewer" ) {
            Natron::errorDialog( tr("Clone").toStdString(), tr("Cloning a viewer is not a valid operation.").toStdString() );

            return;
        }
        if ( (*it)->getNode()->isMultiInstance() ) {
            QString err = QString("%1 cannot be cloned.").arg( (*it)->getNode()->getName_mt_safe().c_str() );
            Natron::errorDialog( tr("Clone").toStdString(),
                                 tr( err.toStdString().c_str() ).toStdString() );

            return;
        }
    }

    ///Offset of some pixels to clone on the right of the original nodes
    xmax += 30;

    double offset = xmax - xmin;
    std::list<boost::shared_ptr<NodeGui> > newNodes;
    std::list<NodeBackDrop*> newBackdrops;
    std::list <boost::shared_ptr<NodeSerialization> > serializations;
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        boost::shared_ptr<NodeSerialization>  internalSerialization( new NodeSerialization( (*it)->getNode() ) );
        NodeGuiSerialization guiSerialization;
        (*it)->serialize(&guiSerialization);
        boost::shared_ptr<NodeGui> clone = _imp->pasteNode( *internalSerialization, guiSerialization, QPointF(offset,0) );
        DotGui* isDot = dynamic_cast<DotGui*>( clone.get() );
        ///Dots cannot be cloned, just copy them
        if (!isDot) {
            clone->getNode()->getLiveInstance()->slaveAllKnobs( (*it)->getNode()->getLiveInstance() );
        }

        newNodes.push_back(clone);
        serializations.push_back(internalSerialization);
    }

    for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
        NodeBackDropSerialization s;
        s.initialize(*it);
        NodeBackDrop* bd = _imp->pasteBackdrop( s,QPointF(offset,0) );
        bd->slaveTo(*it);
        newBackdrops.push_back(bd);
    }

    assert( serializations.size() == newNodes.size() );
    ///restore connections
    _imp->restoreConnections(serializations, newNodes);


    _imp->_undoStack->setActive();
    _imp->_undoStack->push( new AddMultipleNodesCommand(this,newNodes,newBackdrops) );
} // cloneSelectedNodes

void
NodeGraph::decloneSelectedNodes()
{
    if ( _imp->_selection.nodes.empty() && _imp->_selection.bds.empty() ) {
        Natron::warningDialog( tr("Declone").toStdString(), tr("You must select at least a node to declone first.").toStdString() );

        return;
    }
    std::list<boost::shared_ptr<NodeGui> > nodesToDeclone;


    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.nodes.begin(); it != _imp->_selection.nodes.end(); ++it) {
        if ( (*it)->getNode()->getLiveInstance()->isSlave() ) {
            nodesToDeclone.push_back(*it);
        }
    }

    for (std::list<NodeBackDrop*>::iterator it = _imp->_selection.bds.begin(); it != _imp->_selection.bds.end(); ++it) {
        if ( (*it)->isSlave() ) {
            ///Also copy all nodes within the backdrop
            std::list<boost::shared_ptr<NodeGui> > nodesWithinBD = getNodesWithinBackDrop(*it);
            for (std::list<boost::shared_ptr<NodeGui> >::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(nodesToDeclone.begin(),nodesToDeclone.end(),*it2);
                if ( found == nodesToDeclone.end() ) {
                    nodesToDeclone.push_back(*it2);
                }
            }
        }
    }
    _imp->_undoStack->setActive();
    _imp->_undoStack->push( new DecloneMultipleNodesCommand(this,nodesToDeclone,_imp->_selection.bds) );
}

boost::shared_ptr<NodeGui>
NodeGraph::getNodeGuiSharedPtr(const NodeGui* n) const
{
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ( (*it).get() == n ) {
            return *it;
        }
    }
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
        if ( (*it).get() == n ) {
            return *it;
        }
    }
    ///it must either be in the trash or in the active nodes
    assert(false);

    return boost::shared_ptr<NodeGui>();
}

void
NodeGraph::setUndoRedoStackLimit(int limit)
{
    _imp->_undoStack->clear();
    _imp->_undoStack->setUndoLimit(limit);
}

void
NodeGraph::deleteNodePermanantly(boost::shared_ptr<NodeGui> n)
{
    boost::shared_ptr<Natron::Node> internalNode = n->getNode();

    assert(internalNode);
    internalNode->deactivate(std::list< boost::shared_ptr<Natron::Node> >(),false,false,true,false);
    std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_imp->_nodesTrash.begin(),_imp->_nodesTrash.end(),n);

    if ( it != _imp->_nodesTrash.end() ) {
        _imp->_nodesTrash.erase(it);
    }

    {
        QMutexLocker l(&_imp->_nodesMutex);
        std::list<boost::shared_ptr<NodeGui> >::iterator it = std::find(_imp->_nodes.begin(),_imp->_nodes.end(),n);
        if ( it != _imp->_nodes.end() ) {
            _imp->_nodes.erase(it);
        }
    }


    n->deleteReferences();
    n->discardGraphPointer();

    if ( getGui() ) {
        getGui()->removeRotoInterface(n.get(),true);

        ///now that we made the command dirty, delete the node everywhere in Natron
        getGui()->getApp()->deleteNode(n);


        getGui()->getCurveEditor()->removeNode( n.get() );
        std::list<boost::shared_ptr<NodeGui> >::iterator found = std::find(_imp->_selection.nodes.begin(),_imp->_selection.nodes.end(),n);
        if ( found != _imp->_selection.nodes.end() ) {
            n->setUserSelected(false);
            _imp->_selection.nodes.erase(found);
        }
    }

    ///remove the node from the clipboard if it is
    for (std::list< boost::shared_ptr<NodeSerialization> >::iterator it = _imp->_nodeClipBoard.nodes.begin();
         it != _imp->_nodeClipBoard.nodes.end(); ++it) {
        if ( (*it)->getNode() == internalNode ) {
            _imp->_nodeClipBoard.nodes.erase(it);
            break;
        }
    }

    for (std::list<boost::shared_ptr<NodeGuiSerialization> >::iterator it = _imp->_nodeClipBoard.nodesUI.begin();
         it != _imp->_nodeClipBoard.nodesUI.end(); ++it) {
        if ( (*it)->getName() == internalNode->getName() ) {
            _imp->_nodeClipBoard.nodesUI.erase(it);
            break;
        }
    }
} // deleteNodePermanantly

void
NodeGraph::invalidateAllNodesParenting()
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( it->get() );
        }
    }
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( it->get() );
        }
    }
    for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem(*it);
        }
    }
}

void
NodeGraph::centerOnAllNodes()
{
    assert( QThread::currentThread() == qApp->thread() );
    double xmin = INT_MAX;
    double xmax = INT_MIN;
    double ymin = INT_MAX;
    double ymax = INT_MIN;
    {
        QMutexLocker l(&_imp->_nodesMutex);


        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( (*it)->isActive() && (*it)->isVisible() ) {
                QSize size = (*it)->getSize();
                QPointF pos = (*it)->scenePos();
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax,pos.x() + size.width() );
                ymin = std::min( ymin,pos.y() );
                ymax = std::max( ymax,pos.y() + size.height() );
            }
        }

        for (std::list<NodeBackDrop*>::iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
            QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
            xmin = std::min( xmin,bbox.x() );
            ymin = std::min( ymin,bbox.y() );
            xmax = std::max( xmax,bbox.x() + bbox.width() );
            ymax = std::max( ymax,bbox.y() + bbox.height() );
        }
    }
    QRect rect( xmin,ymin,(xmax - xmin),(ymax - ymin) );
    fitInView(rect,Qt::KeepAspectRatio);
    _imp->_refreshOverlays = true;
    repaint();
}

void
NodeGraph::toggleConnectionHints()
{
    appPTR->getCurrentSettings()->setConnectionHintsEnabled( !appPTR->getCurrentSettings()->isConnectionHintEnabled() );
}

NodeBackDrop*
NodeGraph::createBackDrop(QVBoxLayout *dockContainer,
                          bool requestedByLoad,
                          const NodeBackDropSerialization & serialization)
{
    QString name(NATRON_BACKDROP_NODE_NAME);
    int no = _imp->_backdrops.size() + 1;

    name += QString::number(no);
    while ( checkIfBackDropNameExists(name,NULL) ) {
        ++no;
        name = QString(NATRON_BACKDROP_NODE_NAME);
        name += QString::number(no);
    }
    NodeBackDrop* bd = new NodeBackDrop(this,_imp->_root);
    bd->initialize(name, requestedByLoad,serialization, dockContainer);
    _imp->_backdrops.push_back(bd);
    _imp->_undoStack->setActive();
    if (!requestedByLoad) {
        _imp->_undoStack->push( new AddMultipleNodesCommand(this,bd) );
        if ( !_imp->_selection.nodes.empty() ) {
            ///make the backdrop large enough to contain the selected nodes and position it correctly
            QRectF bbox;
            for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_selection.nodes.begin(); it != _imp->_selection.nodes.end(); ++it) {
                QRectF nodeBbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
                bbox = bbox.united(nodeBbox);
            }

            int border = 100;
            int headerHeight = bd->getHeaderHeight();
            bd->setPos(bbox.x() - border, bbox.y() - border);
            bd->resize(bbox.width() + 2 * border, bbox.height() + 2 * border - headerHeight);
        } else {
            QRectF viewPos = visibleSceneRect();
            QPointF mapped = bd->mapFromScene( QPointF( ( viewPos.bottomRight().x() + viewPos.topLeft().x() ) / 2.,
                                                        ( viewPos.topLeft().y() + viewPos.bottomRight().y() ) / 2. ) );
            mapped = bd->mapToParent(mapped);
            bd->setPos(mapped);
            bd->resize(NATRON_BACKDROP_DEFAULT_WIDTH,NATRON_BACKDROP_DEFAULT_HEIGHT);
        }
    }

    return bd;
}

bool
NodeGraph::checkIfBackDropNameExists(const QString & n,
                                     const NodeBackDrop* bd) const
{
    for (std::list<NodeBackDrop*>::const_iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
        if ( ( (*it)->getName() == n ) && ( (*it) != bd ) ) {
            return true;
        }
    }

    return false;
}

bool
NodeGraph::checkIfNodeNameExists(const std::string & n,
                                 const NodeGui* node) const
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ( (it->get() != node) && ( (*it)->getNode()->getName() == n ) ) {
            return true;
        }
    }

    return false;
}

std::list<NodeBackDrop*>
NodeGraph::getBackDrops() const
{
    return _imp->_backdrops;
}

std::list<NodeBackDrop*>
NodeGraph::getActiveBackDrops() const
{
    std::list<NodeBackDrop*> ret;

    for (std::list<NodeBackDrop*>::const_iterator it = _imp->_backdrops.begin(); it != _imp->_backdrops.end(); ++it) {
        if ( (*it)->isVisible() ) {
            ret.push_back(*it);
        }
    }

    return ret;
}

std::list<boost::shared_ptr<NodeGui> > NodeGraph::getNodesWithinBackDrop(const NodeBackDrop* bd) const
{
    QRectF bbox = bd->mapToScene( bd->boundingRect() ).boundingRect();
    std::list<boost::shared_ptr<NodeGui> > ret;
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        QRectF nodeBbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( bbox.contains(nodeBbox) ) {
            ret.push_back(*it);
        }
    }

    return ret;
}

void
NodeGraph::insertNewBackDrop(NodeBackDrop* bd)
{
    _imp->_backdrops.push_back(bd);
}

void
NodeGraph::removeBackDrop(NodeBackDrop* bd)
{
    std::list<NodeBackDrop*>::iterator it = std::find(_imp->_backdrops.begin(),_imp->_backdrops.end(),bd);

    if ( it != _imp->_backdrops.end() ) {
        _imp->_backdrops.erase(it);
    }
}

void
NodeGraph::refreshNodesKnobsAtTime(SequenceTime time)
{
    ///Refresh all knobs at the current time
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->refreshKnobsAfterTimeChange(time);
    }
}

void
NodeGraph::onTimeChanged(SequenceTime time,
                         int reason)
{
    std::vector<ViewerInstance* > viewers;

    if (!_imp->_gui) {
        return;
    }
    boost::shared_ptr<Natron::Project> project = _imp->_gui->getApp()->getProject();

    ///Refresh all knobs at the current time
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( (*it)->getNode()->getLiveInstance() );
        if (isViewer) {
            viewers.push_back(isViewer);
        }
        (*it)->refreshKnobsAfterTimeChange(time);
    }
    Natron::OutputEffectInstance* lastTimelineSeekCaller = project->getLastTimelineSeekCaller();
    project->setLastTimelineSeekCaller(NULL);

    ///Syncrhronize viewers
    
    for (U32 i = 0; i < viewers.size(); ++i) {
        if ( (viewers[i] != lastTimelineSeekCaller) || (reason == USER_SEEK) ) {
            boost::shared_ptr<VideoEngine> engine = viewers[i]->getVideoEngine();
            engine->render(1, //< frame count
                           false, //< seek timeline
                           false, //<refresh tree
                           true, //< forward
                           true, // <same frame
                           false); //< force preview
        }
    }
}

void
NodeGraph::onGuiFrozenChanged(bool frozen)
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->onGuiFrozenChanged(frozen);
    }
}


void
NodeGraph::focusInEvent(QFocusEvent* e)
{
    QGraphicsView::focusInEvent(e);

    _imp->_undoStack->setActive();
}

void
NodeGraph::focusOutEvent(QFocusEvent* e)
{
    if (_imp->_bendPointsVisible) {
        _imp->setNodesBendPointsVisible(false);
    }
    QGraphicsView::focusOutEvent(e);
}

void
NodeGraphPrivate::toggleSelectedNodesEnabled()
{
    std::list<boost::shared_ptr<NodeGui> > toProcess;

    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _selection.nodes.begin(); it != _selection.nodes.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            toProcess.push_back(*it);
        }
    }
    ///if some nodes are disabled , enable them before

    if ( toProcess.size() == _selection.nodes.size() ) {
        _undoStack->push( new EnableNodesCommand(_selection.nodes) );
    } else if (toProcess.size() > 0) {
        _undoStack->push( new EnableNodesCommand(toProcess) );
    } else {
        _undoStack->push( new DisableNodesCommand(_selection.nodes) );
    }
}

bool
NodeGraph::areKnobLinksVisible() const
{
    return _imp->_knobLinksVisible;
}

