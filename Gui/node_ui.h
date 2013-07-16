//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef NODE_UI_H
#define NODE_UI_H


//#include <inputarrow.h>
#include "Superviser/powiterFn.h"
#include <QtCore/QRectF>
#include <QtWidgets/QGraphicsItem>

static const int NODE_LENGTH=80;
static const int NODE_HEIGHT=30;
static const int PREVIEW_LENGTH=40;
static const int PREVIEW_HEIGHT=40;
class Edge;
class QPainterPath;
class QScrollArea;
class SettingsPanel;
class QVBoxLayout;
class Node;
class Controler;
class NodeGraph;
class NodeGui : public QObject,public QGraphicsItem
{
    Q_OBJECT

public:

    NodeGui(NodeGraph* dag,QVBoxLayout *dockContainer,Node *node,qreal x,qreal y , QGraphicsItem *parent=0,QGraphicsScene *sc=0,QObject* parentObj=0);

    ~NodeGui();
    
    /*returns a ptr to the internal node*/
    Node* getNode(){return node;}
    
    /*Returns a pointer to the dag gui*/
    NodeGraph* getDagGui(){return _dag;}
    
    /*Must be implemented by class deriving NodeGUI to inform 
     *the dag if they will have an output*/
    virtual bool hasOutput()=0;
    
    /*Returns tru if the NodeGUI contains the point*/
    virtual bool contains(const QPointF &point) const;
    
    /*returns a QPainterPath indicating the global shape of the node.
     This must be provided so the QGraphicsView framework recognises the
     item correctly.*/
    virtual QPainterPath shape() const;
    
    /*Returns the bouding box of the nodeGUI, must be derived if you
     plan on changing the shape of the node.*/
    virtual QRectF boundingRect() const;
    
    /*this function does the painting, using QPainter, you can overload it to change the aspect of
     the node.*/
    virtual void paint(QPainter* painter,const QStyleOptionGraphicsItem* options,QWidget* parent);
    
    /*initialises the input arrows*/
    void initInputArrows();
    
    /*Returns a ref to the vector of all the input arrows. This can be used
     to query the src and dst of a specific arrow.*/
    const std::vector<Edge*>& getInputsArrows() const {return inputs;}
    
    /*Returns true if the point is included in the rectangle +10px on all edges.*/
    bool isNearby(QPointF &point);
    
    QVBoxLayout* getSettingsLayout(){return dockContainer;}
    
    /*Returns true if the settings panel for this node is displayed*/
    bool isThisPanelEnabled(){return settingsPanel_displayed;}
    
    /*Set the boolean to true if the settings panel for this node will be displayed.*/
    void setSettingsPanelEnabled(bool enabled){settingsPanel_displayed=enabled;}
    
    /*Adds a children to the node*/
    void addChild(NodeGui* c){children.push_back(c);}
    
    /*Adds a parent to the node*/
    void addParent(NodeGui* p){parents.push_back(p);}
    
    /*Removes the child c from the children of this node*/
    void substractChild(NodeGui* c);
    
    /*Removes the parent p from the parents of this node*/
	void substractParent(NodeGui* p);
    
    /*Returns a ref to the vector of the parents of this nodes.*/
    const std::vector<NodeGui*>& getParents(){return parents;}
    
    /*Returns a ref to the vector of children of this node.*/
    const std::vector<NodeGui*>& getChildren(){return children;}
    
    /*Returns a pointer to the settings panel of this node.*/
    SettingsPanel* getSettingPanel(){return settings;}
    
    /*Returns a pointer to the layout containing settings panels.*/
    QVBoxLayout* getDockContainer(){return dockContainer;}
    
    /*Updates the preview image if this is a reader node. Can
     only be called by reader nodes, otherwise does nothing*/
	void updatePreviewImageForReader();
    
    /*Updates the channels tooltip. This is called by Node::validate(),
     i.e, when the channel requested for the node change.*/
    void updateChannelsTooltip();
    
    /*Returns a pointer to the Viewer nodeGUI ptr is this node has a viewer connected,
     otherwise, returns NULL.*/
    static NodeGui* hasViewerConnected(NodeGui* node);
    
    /*toggles selected on/off*/
    void setSelected(bool b);
    
    bool isSelected(){return _selected;}
    
    /*Returns a pointer to the first available input. Otherwise returns NULL*/
    Edge* firstAvailableEdge();
    
    /*find the edge connecting this as dst and the parent as src.
     Return a valid ptr to the edge if it found it, otherwise returns NULL.*/
    Edge* findConnectedEdge(NodeGui* parent);
    
    /*Moves the settings panel on top of the list of panels.*/
    void putSettingsPanelFirst();
    
    void remove();
    
private:
    /*used internally by hasViewerConnected.*/
    static void _hasViewerConnected(NodeGui* node,bool* ok,NodeGui*& out);
    
    
public slots:
    void setName(QString);
protected:

    /*children of the node*/
    std::vector<NodeGui*> children;
    
    /*parents of the node*/
    std::vector<NodeGui*> parents;
    
    /*pointer to the dag*/
    NodeGraph* _dag;
    
    /*pointer to the internal node*/
    Node* node;
    
    /*true if the node is selected by the user*/
    bool _selected;

    /*A pointer to the graphical text displaying the name.*/
    QGraphicsSimpleTextItem *name;
    
    /*A pointer to the layout containing setting panels*/
    QVBoxLayout* dockContainer;
    
    /*A pointer to the global scene representing the DAG*/
    QGraphicsScene* sc;
    
    /*A pointer to the rectangle of the node.*/
    QGraphicsRectItem* rectangle;
    
    /*A pointer to the channels pixmap displayed*/
    QGraphicsPixmapItem* channels;
    
    /*A pointer to the preview pixmap displayed for readers/*/
	QGraphicsPixmapItem* prev_pix;
    
    /*the graphical input arrows*/
    std::vector<Edge*> inputs;
    
    /*settings panel related*/
    bool settingsPanel_displayed;
    SettingsPanel* settings;

};

#endif // NODE_UI_H
