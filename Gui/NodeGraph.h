//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_NODEGRAPH_H_
#define NATRON_GUI_NODEGRAPH_H_

#include <vector>
#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGraphicsView>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QDialog>
#include <QLabel>
#include <QUndoCommand>
#include <QMutex>
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#endif

class QVBoxLayout;
class QScrollArea;
class QGraphicsProxyWidget;
class QUndoStack;
class QGraphicsTextItem;
class QComboBox;
class QEvent;
class QKeyEvent;
class Gui;
class NodeSettingsPanel;
class NodeGui;
class AppInstance;
class Edge;
class QMenu;
class SmartInputDialog;
class QDropEvent;
class QDragEnterEvent;
class NodeSerialization;
class NodeGuiSerialization;
namespace Natron{
    class Node;
}


class NodeGraph: public QGraphicsView , public boost::noncopyable{
    
    enum EVENT_STATE{DEFAULT,MOVING_AREA,ARROW_DRAGGING,NODE_DRAGGING};
    
    Q_OBJECT
    
    class NodeGraphNavigator : public QLabel{
        int _w,_h;
    public:
        
        explicit NodeGraphNavigator(QWidget* parent = 0);
        
        void setImage(const QImage& img);
        
        virtual QSize sizeHint() const OVERRIDE FINAL {return QSize(_w,_h);};
        
        virtual ~NodeGraphNavigator(){}
    };

public:

    explicit NodeGraph(Gui* gui,QGraphicsScene* scene=0,QWidget *parent=0);

    virtual ~NodeGraph() OVERRIDE;
 
    void setPropertyBinPtr(QScrollArea* propertyBin){_propertyBin = propertyBin;}
    
    boost::shared_ptr<NodeGui> createNodeGUI(QVBoxLayout *dockContainer,const boost::shared_ptr<Natron::Node>& node,bool requestedByLoad);
    
    boost::shared_ptr<NodeGui> getSelectedNode() const {return _nodeSelected;}
    
    void setSmartNodeCreationEnabled(bool enabled){smartNodeCreationEnabled=enabled;}
    
    void selectNode(const boost::shared_ptr<NodeGui>& n);
        
    QRectF visibleRect();
    
    void deselect();
    
    QImage getFullSceneScreenShot();
    
    bool areAllNodesVisible();
    
    void updateNavigator();
    
    QGraphicsItem* getRootItem() const {return _root;}
    
    const std::list<boost::shared_ptr<NodeGui> >& getAllActiveNodes() const;
    
    std::list<boost::shared_ptr<NodeGui> > getAllActiveNodes_mt_safe() const;
    
    void moveToTrash(NodeGui* node);
    
    void restoreFromTrash(NodeGui* node);
        
    Gui* getGui() const {return _gui;}
    
    void refreshAllEdges();

    bool areAllPreviewTurnedOff() const {
        QMutexLocker l(&_previewsTurnedOffMutex);
        return _previewsTurnedOff;
    }
    
    void deleteNode(const boost::shared_ptr<NodeGui>& n);
    
    void centerOnNode(const boost::shared_ptr<NodeGui>& n);
    
    void copyNode(const boost::shared_ptr<NodeGui>& n);
    
    void cutNode(const boost::shared_ptr<NodeGui>& n);
    
    void duplicateNode(const boost::shared_ptr<NodeGui>& n);
    
    void cloneNode(const boost::shared_ptr<NodeGui>& n);
    
    void decloneNode(const boost::shared_ptr<NodeGui>& n);

    boost::shared_ptr<NodeGui> getNodeGuiSharedPtr(const NodeGui* n) const;
    
    void setUndoRedoStackLimit(int limit);
    
    void deleteNodePermanantly(const boost::shared_ptr<NodeGui>& n);
    
public slots:
    
    void deleteSelectedNode();
    
    void connectCurrentViewerToSelection(int inputNB);

    void updateCacheSizeText();
    
    void showMenu(const QPoint& pos);
    
    void populateMenu();
    
    void toggleCacheInfos();
    
    void turnOffPreviewForAllNodes();
    
    void toggleAutoPreview();
    
    void forceRefreshAllPreviews();

    void onProjectNodesCleared();
    
    void copySelectedNode();
    
    void cutSelectedNode();
    
    void pasteNodeClipBoard();
    
    void duplicateSelectedNode();
    
    void cloneSelectedNode();
    
    void decloneSelectedNode();
    
    void centerOnAllNodes();

private:
    
    /**
     * @brief Given the node, it tries to move it to the ideal position
     * according to the position of the selected node and its inputs/outputs.
     * This is used when creating a node to position it correctly.
     * It will move the inputs / outputs slightly to fit this node into the nodegraph
     * so they do not overlap.
     **/
    void moveNodesForIdealPosition(boost::shared_ptr<NodeGui> n);
    
    void pasteNode(const NodeSerialization& internalSerialization,const NodeGuiSerialization& guiSerialization);
  

    virtual void enterEvent(QEvent *event) OVERRIDE FINAL;

    virtual void leaveEvent(QEvent *event) OVERRIDE FINAL;

    virtual void keyPressEvent(QKeyEvent *e) OVERRIDE FINAL;

    virtual bool event(QEvent* event) OVERRIDE FINAL;

    void mousePressEvent(QMouseEvent *event);
    
    void mouseReleaseEvent(QMouseEvent *event);
    
    void mouseMoveEvent(QMouseEvent *event);
    
    void mouseDoubleClickEvent(QMouseEvent *event);
    
    void resizeEvent(QResizeEvent* event);
    
    void paintEvent(QPaintEvent* event);

    void wheelEvent(QWheelEvent *event);

    void dropEvent(QDropEvent* event);
    
    void dragEnterEvent(QDragEnterEvent *ev);
    
    void dragMoveEvent(QDragMoveEvent* e);
    
    void dragLeaveEvent(QDragLeaveEvent* e);

private:
    
    
    
    // FIXME: PIMPL
    QRectF calcNodesBoundingRect();
    
    bool smartNodeCreationEnabled;
    
    Gui* _gui;
    
    QPointF _lastScenePosClick;
    
    QPointF _lastNodeDragStartPoint;

    EVENT_STATE _evtState;
    
    boost::shared_ptr<NodeGui> _nodeSelected;
    
    Edge* _arrowSelected;
    
    mutable QMutex _nodesMutex;
    
    std::list<boost::shared_ptr<NodeGui> > _nodes;
    std::list<boost::shared_ptr<NodeGui> > _nodesTrash;
    
    bool _nodeCreationShortcutEnabled;
        
    QGraphicsItem* _root;
    
    QScrollArea* _propertyBin;

    QGraphicsTextItem* _cacheSizeText;
    
    QTimer _refreshCacheTextTimer;
    
    NodeGraphNavigator* _navigator;
    
    QGraphicsLineItem* _navLeftEdge;
    QGraphicsLineItem* _navBottomEdge;
    QGraphicsLineItem* _navRightEdge;
    QGraphicsLineItem* _navTopEdge;
    
    QGraphicsProxyWidget* _navigatorProxy;
    
    QUndoStack* _undoStack;

        
    QMenu* _menu;
    
    QGraphicsItem *_tL,*_tR,*_bR,*_bL;
    
    bool _refreshOverlays;
    
    mutable QMutex _previewsTurnedOffMutex;
    bool _previewsTurnedOff;
    
    struct NodeClipBoard {
        boost::shared_ptr<NodeSerialization> _internal;
        boost::shared_ptr<NodeGuiSerialization> _gui;
        
        NodeClipBoard()
        : _internal()
        , _gui()
        {
        }
        
        bool isEmpty() const { return !_internal || !_gui; }
    };
    
    NodeClipBoard _nodeClipBoard;
    
    
};



class SmartInputDialog:public QDialog
{
Q_OBJECT

public:
    explicit SmartInputDialog(NodeGraph* graph);
    virtual ~SmartInputDialog() OVERRIDE {}
    void keyPressEvent(QKeyEvent *e);
    bool eventFilter(QObject * obj, QEvent * e);
private:
    NodeGraph* graph;
    QVBoxLayout* layout;
    QLabel* textLabel;
    QComboBox* textEdit;
    
};
#endif // NATRON_GUI_NODEGRAPH_H_
