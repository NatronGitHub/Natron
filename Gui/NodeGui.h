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

#ifndef POWITER_GUI_NODEGUI_H_
#define POWITER_GUI_NODEGUI_H_

#include <map>

#include <QtCore/QRectF>
#include <QGraphicsItem>

#include "Global/Macros.h"

class Edge;
class QPainterPath;
class QScrollArea;
class NodeSettingsPanel;
class QVBoxLayout;
class AppInstance;
class NodeGraph;
class QAction;
class KnobGui;
class Knob;
class ChannelSet;
class Node;

class NodeGui : public QObject,public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
public:

    typedef std::map<int,Edge*> InputEdgesMap;
    
    NodeGui(NodeGraph* dag,
            QVBoxLayout *dockContainer,
            Node *node,
            qreal x,qreal y ,
            QGraphicsItem *parent=0);

    ~NodeGui();
    
    Node* getNode() const {return node;}
    
    /*Returns a pointer to the dag gui*/
    NodeGraph* getDagGui(){return _dag;}
    
    
  
    /*Returns tru if the NodeGUI contains the point*/
    virtual bool contains(const QPointF &point) const;
    
    /*returns a QPainterPath indicating the global shape of the node.
     This must be provided so the QGraphicsView framework recognises the
     item correctly.*/
    virtual QPainterPath shape() const;
    
    /*Returns the bouding box of the nodeGUI, must be derived if you
     plan on changing the shape of the node.*/
    virtual QRectF boundingRect() const;
    
    QRectF boundingRectWithEdges() const;
    
    /*this function does the painting, using QPainter, you can overload it to change the aspect of
     the node.*/
    virtual void paint(QPainter* painter,const QStyleOptionGraphicsItem* options,QWidget* parent);
    
   
    
    /*Returns a ref to the vector of all the input arrows. This can be used
     to query the src and dst of a specific arrow.*/
    const std::map<int,Edge*>& getInputsArrows() const {return inputs;}
    
   
    
    /*Returns true if the point is included in the rectangle +10px on all edges.*/
    bool isNearby(QPointF &point);
    
    QVBoxLayout* getSettingsLayout(){return dockContainer;}
    
    /*Returns a pointer to the settings panel of this node.*/
    NodeSettingsPanel* getSettingPanel(){return settings;}
    
    /*Returns a pointer to the layout containing settings panels.*/
    QVBoxLayout* getDockContainer(){return dockContainer;}
    
        
       
    /*toggles selected on/off*/
    void setSelected(bool b);
    
    bool isSelected(){return _selected;}
    
    /*Returns a pointer to the first available input. Otherwise returns NULL*/
    Edge* firstAvailableEdge();
    
    /*find the edge connecting this as dst and the parent as src.
     Return a valid ptr to the edge if it found it, otherwise returns NULL.*/
    Edge* findConnectedEdge(NodeGui* parent);
    
        
    void markInputNull(Edge* e);
    
    
    
    static const int NODE_LENGTH = 80;
    static const int NODE_HEIGHT = 30;
    static const int PREVIEW_LENGTH = 40;
    static const int PREVIEW_HEIGHT = 40;
    
        
    /*Returns an edge if the node has an edge close to the
     point pt. pt is in scene coord.*/
    Edge* hasEdgeNearbyPoint(const QPointF& pt);
    
    void setName(const QString& name);
    
    void refreshPosition(double x,double y);
    
    bool isSettingsPanelVisible() const;
public slots:
  
    
    /*Updates the preview image.*/
	void updatePreviewImage(int time);
    
    /*Updates the channels tooltip. This is called by Node::validate(),
     i.e, when the channel requested for the node change.*/
    void updateChannelsTooltip(const ChannelSet& channels);
    
    void onLineEditNameChanged(const QString&);
    
    void onInternalNameChanged(const QString&);
    
    void deleteNode();
        
    void refreshEdges();
    
    /*initialises the input edges*/
    void initializeInputs();
    
    void initializeKnobs();
    
    void activate();
    
    void deactivate();
    
    /*Use NULL for src to disconnect.*/
    bool connectEdge(int edgeNumber);
    
    void setVisibleSettingsPanel(bool b);
    
signals:
    void nameChanged(QString);
    
private:
    
    void computePreviewImage(int time);
    
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
        
    /*A pointer to the rectangle of the node.*/
    QGraphicsRectItem* rectangle;
    
    /*A pointer to the channels pixmap displayed*/
    QGraphicsPixmapItem* channels;
    
    /*A pointer to the preview pixmap displayed for readers/*/
	QGraphicsPixmapItem* prev_pix;
    
    /*the graphical input arrows*/
    std::map<int,Edge*> inputs;
       /*settings panel related*/
    bool settingsPanel_displayed;
    NodeSettingsPanel* settings;
    
  
};

#endif // POWITER_GUI_NODEGUI_H_
