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


#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/version.hpp>


#include "Engine/Knob.h"
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
namespace Powiter {
class ChannelSet;
class Node;
}

class NodeGui : public QObject,public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    
    
    
public:
    
    
    class SerializedState {
        
        const NodeGui* _node;
        std::map<std::string,std::string> _knobsValues;
        std::string _name;
        std::string _className;
        
        std::map<int,std::string> _inputs;
        double _posX,_posY;
        
        
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            (void)version;
            ar & boost::serialization::make_nvp("Knobs_values_map", _knobsValues);
            ar & boost::serialization::make_nvp("Node_instance_unique_name",_name);
            ar & boost::serialization::make_nvp("Node_class_name",_className);
            ar & boost::serialization::make_nvp("Input_nodes_map",_inputs);
            ar & boost::serialization::make_nvp("X_position",_posX);
            ar & boost::serialization::make_nvp("Y_position",_posY);
        }
        
    public:
        
        SerializedState():_node(NULL){};
        
        SerializedState(const NodeGui* n);
        
        const std::map<std::string,std::string>& getKnobsValues() const {return _knobsValues;}
        
        const std::string getName() const {return _name;}
        
        const std::string getClassName() const {return _className;}
        
        const std::map<int,std::string>& getInputs() const {return _inputs;}
                
        double getX() const {return _posX;}
        
        double getY() const {return _posY;}
        
    };
    
    

    typedef std::map<int,Edge*> InputEdgesMap;
    
    NodeGui(NodeGraph* dag,
            QVBoxLayout *dockContainer,
            Powiter::Node *_internalNode,
            qreal x,qreal y ,
            QGraphicsItem *parent=0);

    ~NodeGui();
    
    NodeGui::SerializedState serialize() const;
    
    Powiter::Node* getNode() const {return _internalNode;}
    
    /*Returns a pointer to the dag gui*/
    NodeGraph* getDagGui(){return _graph;}
    
    
  
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
    const std::map<int,Edge*>& getInputsArrows() const {return _inputEdges;}
    
   
    
    /*Returns true if the point is included in the rectangle +10px on all edges.*/
    bool isNearby(QPointF &point);
    
    /*Returns a pointer to the settings panel of this node.*/
    NodeSettingsPanel* getSettingPanel() const {return _settingsPanel;}
    
    /*Returns a pointer to the layout containing settings panels.*/
    QVBoxLayout* getDockContainer() const ;
    
        
       
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
    
    
    void refreshPosition(double x,double y);
    
    bool isSettingsPanelVisible() const;
    
    void setSelectedGradient(const QLinearGradient& gradient);
    
    void setDefaultGradient(const QLinearGradient& gradient);
public slots:
  
    void togglePreview();

    /**
     * @brief Updates the position of the items contained by the node to fit into
     * the new width and height.
     **/
    void updateShape(int width,int height);
    
    /*Updates the preview image.*/
	void updatePreviewImage(int time);
    
    /*Updates the channels tooltip. This is called by Node::validate(),
     i.e, when the channel requested for the node change.*/
    void updateChannelsTooltip(const Powiter::ChannelSet& _channelsPixmap);
    
    void setName(const QString& _nameItem);
    
    void onInternalNameChanged(const QString&);
    
    void onPersistentMessageChanged(int type,const QString& message);
    
    void onPersistentMessageCleared();
    
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
    NodeGraph* _graph;
    
    /*pointer to the internal node*/
    Powiter::Node* _internalNode;
    
    /*true if the node is selected by the user*/
    bool _selected;

    /*A pointer to the graphical text displaying the name.*/
    QGraphicsTextItem *_nameItem;
        
    /*A pointer to the rectangle of the node.*/
    QGraphicsRectItem* _boundingBox;
    
    /*A pointer to the channels pixmap displayed*/
    QGraphicsPixmapItem* _channelsPixmap;
    
    /*A pointer to the preview pixmap displayed for readers/*/
    QGraphicsPixmapItem* _previewPixmap;
    
    QGraphicsTextItem* _persistentMessage;
    int _lastPersistentMessageType;
    QGraphicsRectItem* _stateIndicator;
    
    /*the graphical input arrows*/
    std::map<int,Edge*> _inputEdges;
       /*settings panel related*/
    bool _panelDisplayed;
    NodeSettingsPanel* _settingsPanel;
    
    QGradient* _selectedGradient;
    QGradient* _defaultGradient;
    
  
};
BOOST_CLASS_VERSION(NodeGui::SerializedState, 1)

#endif // POWITER_GUI_NODEGUI_H_
