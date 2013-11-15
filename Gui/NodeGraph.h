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
#include <QGraphicsView>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QDialog>
#include <QLabel>
#include <QUndoCommand>
#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#endif

#include "Global/Macros.h"

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
        
        virtual QSize sizeHint() const {return QSize(_w,_h);};
        
        virtual ~NodeGraphNavigator(){}
    };

public:

    explicit NodeGraph(Gui* gui,QGraphicsScene* scene=0,QWidget *parent=0);

    virtual ~NodeGraph();
 
    void setPropertyBinPtr(QScrollArea* propertyBin){_propertyBin = propertyBin;}
    
    NodeGui* createNodeGUI(QVBoxLayout *dockContainer,Natron::Node *node);
    
    NodeGui* getSelectedNode() const {return _nodeSelected;}
    
    void removeNode(NodeGui* n);

    virtual void enterEvent(QEvent *event);
    
    virtual void leaveEvent(QEvent *event);

    virtual void keyPressEvent(QKeyEvent *e);
    
    virtual bool event(QEvent* event);
    
    void autoConnect(NodeGui* selected,NodeGui* created);
    
    void setSmartNodeCreationEnabled(bool enabled){smartNodeCreationEnabled=enabled;}
    
    void selectNode(NodeGui* n);
        
    QRectF visibleRect();
    
    void deselect();
    
    QImage getFullSceneScreenShot();
    
    bool areAllNodesVisible();
    
    void updateNavigator();
    
    QGraphicsItem* getRootItem() const {return _root;}
    
    const std::vector<NodeGui*>& getAllActiveNodes() const;
    
    void moveToTrash(NodeGui* node);
    
    void restoreFromTrash(NodeGui* node);
    
    /*Returns true if the graph has no value, i.e:
     this is just output nodes*/
    bool isGraphWorthLess() const;
        
    Gui* getGui() const {return _gui;}
    
    void refreshAllEdges();

    bool areAllPreviewTurnedOff() const { return _previewsTurnedOff; }
    
public slots:
    
    void deleteSelectedNode();
    
    void connectCurrentViewerToSelection(int inputNB);

    void updateCacheSizeText();
    
    void showMenu(const QPoint& pos);
    
    void populateMenu();
    
    void toggleCacheInfos();
    
    void turnOffPreviewForAllNodes();

protected:

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
    
    QRectF calcNodesBoundingRect();
    
    bool smartNodeCreationEnabled;
    
    Gui* _gui;
    
    QPointF _lastScenePosClick;
    
    QPointF _lastNodeDragStartPoint;

    EVENT_STATE _evtState;
    
    NodeGui* _nodeSelected;
    
    Edge* _arrowSelected;
    
    std::vector<NodeGui*> _nodes;
    
    std::vector<NodeGui*> _nodesTrash;
    
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
    
    QAction* _undoAction,*_redoAction;
    
    QMenu* _menu;
    
    QGraphicsItem *_tL,*_tR,*_bR,*_bL;
    
    bool _refreshOverlays;
    
    bool _previewsTurnedOff;
};


class MoveCommand : public QUndoCommand{
public:
    MoveCommand(NodeGui *node, const QPointF &oldPos,
                QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    NodeGui* _node;
    QPointF _oldPos;
    QPointF _newPos;
};


class AddCommand : public QUndoCommand{
public:

    AddCommand(NodeGraph* graph,NodeGui *node,QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    
private:
    std::multimap<int,Natron::Node*> _outputs;
    std::map<int,Natron::Node*> _inputs;
    NodeGui* _node;
    NodeGraph* _graph;
    bool _undoWasCalled;
};

class RemoveCommand : public QUndoCommand{
public:
    
    RemoveCommand(NodeGraph* graph,NodeGui *node,QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    
private:
    std::multimap<int,Natron::Node*> _outputs;
    std::map<int,Natron::Node*> _inputs;
    NodeGui* _node;
    NodeGraph* _graph;
};

class ConnectCommand : public QUndoCommand{
public:
    ConnectCommand(NodeGraph* graph,Edge* edge,NodeGui *oldSrc,NodeGui* newSrc,QUndoCommand *parent = 0);
    
    virtual void undo();
    virtual void redo();
private:
    Edge* _edge;
    NodeGui *_oldSrc,*_newSrc;
    NodeGraph* _graph;
};


class SmartInputDialog:public QDialog
{
Q_OBJECT

public:
    explicit SmartInputDialog(NodeGraph* graph);
    virtual ~SmartInputDialog(){}
    void keyPressEvent(QKeyEvent *e);
    bool eventFilter(QObject * obj, QEvent * e);
private:
    NodeGraph* graph;
    QVBoxLayout* layout;
    QLabel* textLabel;
    QComboBox* textEdit;
    
};
#endif // NATRON_GUI_NODEGRAPH_H_
