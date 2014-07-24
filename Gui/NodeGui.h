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

#ifndef NATRON_GUI_NODEGUI_H_
#define NATRON_GUI_NODEGUI_H_

#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QRectF>
#include <QtCore/QMutex>
#include <QGraphicsItem>
#include <QGradient>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

class Edge;
class QPainterPath;
class QScrollArea;
class NodeSettingsPanel;
class QVBoxLayout;
class QLinearGradient;
class QGradient;
class AppInstance;
class NodeGraph;
class QAction;
class KnobI;
class NodeGuiSerialization;
class KnobGui;
class QUndoStack;
class MultiInstancePanel;
class QMenu;
namespace Natron {
class ChannelSet;
class Node;
}

struct NodeGuiIndicatorPrivate;
class NodeGuiIndicator
{
public:
    
    NodeGuiIndicator(const QString& text,
                     const QPointF& topLeft,
                     int width,int height,
                     const QGradientStops& gradient,
                     const QColor& textColor,
                     QGraphicsItem* parent);
    
    ~NodeGuiIndicator();
    
    void setToolTip(const QString& tooltip);
    
    void setActive(bool active);
    
    void refreshPosition(const QPointF& topLeft);
    
private:
    
    boost::scoped_ptr<NodeGuiIndicatorPrivate> _imp;
};

///A richer text item which allows text alignmenent
class TextItem : public QGraphicsTextItem
{
    Q_OBJECT
    
public:
    
    enum { Type = UserType + 1 };
    
    TextItem(QGraphicsItem* parent = 0);
    TextItem(const QString& text, QGraphicsItem* parent = 0);
    
    void setAlignment(Qt::Alignment alignment);
    virtual int type() const;
    
public slots:
    
    void updateGeometry(int, int, int);
    void updateGeometry();
    
private:
    void init();
    
    Qt::Alignment _alignement;
};

class NodeGui : public QObject,public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    
    
    
public:
    

    typedef std::map<int,Edge*> InputEdgesMap;
    
    NodeGui(QGraphicsItem *parent=0);

    void initialize(NodeGraph* dag,
                    const boost::shared_ptr<NodeGui>& thisAsShared,
                    QVBoxLayout *dockContainer,
                    const boost::shared_ptr<Natron::Node>& internalNode,
                    bool requestedByLoad);
    
    /**
     * @brief Called by NodeGraph::clearExceedingUndoRedoEvents when we want to delete a node.
     **/
    void deleteReferences();
    
    ~NodeGui() OVERRIDE;
    
    /**
     * @brief Fills the serializationObject with the current state of the NodeGui.
     **/
    void serialize(NodeGuiSerialization* serializationObject) const;
    
    
    void copyFrom(const NodeGuiSerialization& obj);

    
    boost::shared_ptr<Natron::Node> getNode() const {return _internalNode;}
    
    /*Returns a pointer to the dag gui*/
    NodeGraph* getDagGui(){return _graph;}
    
    
  
    /*Returns true if the NodeGUI contains the point (in items coordinates)*/
    virtual bool contains(const QPointF &point) const OVERRIDE;
    
    /*Returns true if the bounding box of the node intersects the given rectangle in scene coordinates.*/
    bool intersects(const QRectF& rect) const;
    
    /*returns a QPainterPath indicating the global shape of the node.
     This must be provided so the QGraphicsView framework recognises the
     item correctly.*/
    virtual QPainterPath shape() const OVERRIDE;
    
    /*Returns the bouding box of the nodeGUI, must be derived if you
     plan on changing the shape of the node.*/
    virtual QRectF boundingRect() const OVERRIDE;
    
    QRectF boundingRectWithEdges() const;
    
    /*this function does the painting, using QPainter, you can overload it to change the aspect of
     the node.*/
    virtual void paint(QPainter* painter,const QStyleOptionGraphicsItem* options,QWidget* parent) OVERRIDE;
    
   
    
    /*Returns a ref to the vector of all the input arrows. This can be used
     to query the src and dst of a specific arrow.*/
    const std::map<int,Edge*>& getInputsArrows() const {return _inputEdges;}
    
    Edge* getOutputArrow() const;
    
    Edge* getInputArrow(int inputNb) const;
    
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
    
    const std::map<boost::shared_ptr<KnobI>,KnobGui*>& getKnobs() const;
    

    static const int DEFAULT_OFFSET_BETWEEN_NODES = 20;

        
    QSize getSize() const;
    
        
    /*Returns an edge if the node has an edge close to the
     point pt. pt is in scene coord.*/
    Edge* hasEdgeNearbyPoint(const QPointF& pt);
    
    Edge* hasEdgeNearbyRect(const QRectF& rect);
    
    void refreshPosition(double x,double y,bool skipMagnet = false);
    
    void changePosition(double dx,double dy);
    
    bool isSettingsPanelVisible() const;
    
    void setSelectedGradient(const QLinearGradient& gradient);
    
    void setDefaultGradient(const QLinearGradient& gradient);
    
    void removeSettingsPanel();
    
    QUndoStack* getUndoStack() const;
    
    void removeUndoStack();
    
    /**
     * @brief Given the rectangle r, move the node down so it doesn't belong
     * to this rectangle and call the same function with the new bounding box of this node
     * recursively on its outputs.
     **/
    void moveBelowPositionRecursively(const QRectF& r);
    
    /**
     * @brief Given the rectangle r, move the node up so it doesn't belong
     * to this rectangle and call the same function with the new bounding box of this node
     * recursively on its inputs.
     **/
    void moveAbovePositionRecursively(const QRectF& r);
    
    QPointF getPos_mt_safe() const;
    
    void setPos_mt_safe(const QPointF& pos);
    
    ///same as setScale() but also scales the arrows
    void setScale_natron(double scale);
    
    void removeHighlightOnAllEdges();
    
    QColor getCurrentColor() const;
    
    void setCurrentColor(const QColor& c);
    
    void refreshKnobsAfterTimeChange(SequenceTime time);
    
    boost::shared_ptr<MultiInstancePanel> getMultiInstancePanel() const;
    
    bool shouldDrawOverlay() const;
    
    void setParentMultiInstance(const boost::shared_ptr<NodeGui>& parent);
    
public slots:
    
    void setDefaultGradientColor(const QColor& color);
  
    void togglePreview();
    
    void onPreviewKnobToggled();
    
    void onDisabledKnobToggled(bool disabled);

    /**
     * @brief Updates the position of the items contained by the node to fit into
     * the new width and height.
     **/
    void updateShape(int width,int height);
    
    /*Updates the preview image, only if the project is in auto-preview mode*/
	void updatePreviewImage(int time);
    
    /*Updates the preview image no matter what*/
    void forceComputePreview(int time);
    
    /*Updates the channels tooltip. This is called by Node::validate(),
     i.e, when the channel requested for the node change.*/
    void updateChannelsTooltip(const Natron::ChannelSet& _channelsPixmap);
    
    void setName(const QString& _nameItem);
    
    void onInternalNameChanged(const QString&);
    
    void onPersistentMessageChanged(int type,const QString& message);
    
    void onPersistentMessageCleared();
            
    void refreshEdges();
    
    /*initialises the input edges*/
    void initializeInputs();
    
    void initializeKnobs();
    
    void activate();
    
    void deactivate();
    
    void hideGui();
    
    void showGui();
    
    /*Use NULL for src to disconnect.*/
    bool connectEdge(int edgeNumber);
    
    void setVisibleSettingsPanel(bool b);
    
    void onRenderingStarted();
    
    void onRenderingFinished();
    
    void onInputNRenderingStarted(int input);
    
    void onInputNRenderingFinished(int input);
    
    void beginEditKnobs();
    
    bool wasBeginEditCalled() { return _wasBeginEditCalled; }
    
    void centerGraphOnIt();
    
    void onSlaveStateChanged(bool b);
    
    void refreshSlaveMasterLinkPosition();
    
    void refreshOutputEdgeVisibility();
    
    void toggleBitDepthIndicator(bool on,const QString& tooltip);
    
    void onNodeExtraLabelChanged(const QString& label);
    
    void onSwitchInputActionTriggered();
    
    void onSettingsPanelClosedChanged(bool closed);

signals:
    
    void nameChanged(QString);
    
    void positionChanged();
    
    void settingsPanelClosed(bool b);
    
private:
    
    void setNameItemHtml(const QString& name,const QString& label);
    
    void togglePreview_internal(bool refreshPreview = true);
    
    void setAboveItem(QGraphicsItem* item);
    
    void computePreviewImage(int time);
    
    void populateMenu();
    
    void refreshCurrentBrush();
    
    /*pointer to the dag*/
    NodeGraph* _graph;
    
    /*pointer to the internal node*/
    boost::shared_ptr<Natron::Node> _internalNode;
    
    /*true if the node is selected by the user*/
    bool _selected;

    /*A pointer to the graphical text displaying the name.*/
    bool _settingNameFromGui;
    QGraphicsTextItem *_nameItem;
        
    /*A pointer to the rectangle of the node.*/
    QGraphicsRectItem* _boundingBox;
    
    /*A pointer to the channels pixmap displayed*/
    QGraphicsPixmapItem* _channelsPixmap;
    
    /*A pointer to the preview pixmap displayed for readers/*/
    QGraphicsPixmapItem* _previewPixmap;
    
    QGraphicsTextItem* _persistentMessage;
    QString _lastPersistentMessage;
    int _lastPersistentMessageType;
    QGraphicsRectItem* _stateIndicator;
    
    NodeGuiIndicator* _bitDepthWarning;
    
    /*the graphical input arrows*/
    std::map<int,Edge*> _inputEdges;
    
    Edge* _outputEdge;
    
    /*settings panel related*/
    bool _panelDisplayed;
    NodeSettingsPanel* _settingsPanel;
    
    QGradient* _selectedGradient;
    QGradient* _defaultGradient;
    QGradient* _clonedGradient;
    QGradient* _disabledGradient;
        
    timeval _lastRenderStartedSlotCallTime;
    timeval _lastInputNRenderStartedSlotCallTime;
    bool _wasRenderStartedSlotRun; //< true if we changed the color of the widget after a call to onRenderingStarted
  
    bool _wasBeginEditCalled;
    
    mutable QMutex positionMutex;
    
    QGraphicsLineItem* _slaveMasterLink;
    boost::shared_ptr<NodeGui> _masterNodeGui;
    
    bool _magnecEnabled;
    QPoint _magnecDistance;
    bool _updateDistanceSinceLastMagnec;
    QPoint _distanceSinceLastMagnec;
    QPoint _magnecStartingPos;
    
    QString _nodeLabel;
    
    boost::shared_ptr<NodeGui> _parentMultiInstance;
};

#endif // NATRON_GUI_NODEGUI_H_
