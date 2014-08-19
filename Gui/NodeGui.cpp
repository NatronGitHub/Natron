//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "NodeGui.h"

#include <cassert>
#include <boost/scoped_array.hpp>

#include <QLayout>
#include <QAction> 
#include <QtConcurrentRun>
#include <QFontMetrics>
#include <QMenu>
#include <QTextDocument> // for Qt::convertFromPlainText
#include <QTextBlockFormat>
#include <QTextCursor>

#include "Gui/Edge.h"
#include "Gui/DockablePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"
#include "Gui/Gui.h"
#include "Gui/KnobGui.h"
#include "Gui/ViewerGL.h"
#include "Gui/CurveEditor.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGuiTypes.h"

#include "Engine/OfxEffectInstance.h"
#include "Engine/ViewerInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/ChannelSet.h"
#include "Engine/Timer.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/Settings.h"
#include "Engine/Knob.h"
#define NATRON_STATE_INDICATOR_OFFSET 5

#define NATRON_EDGE_DROP_TOLERANCE 15

#define NATRON_MAGNETIC_GRID_GRIP_TOLERANCE 20

#define NATRON_MAGNETIC_GRID_RELEASE_DISTANCE 30

#define NATRON_ELLIPSE_WARN_DIAMETER 10

#define NODE_WIDTH 80
#define NODE_HEIGHT 30
#define NODE_WITH_PREVIEW_WIDTH NODE_WIDTH / 2 + NATRON_PREVIEW_WIDTH
#define NODE_WITH_PREVIEW_HEIGHT NODE_HEIGHT + NATRON_PREVIEW_HEIGHT

#define DOT_GUI_DIAMETER 15

using namespace Natron;

using std::make_pair;

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

static QString replaceLineBreaksWithHtmlParagraph(QString txt) {
    txt.replace("\n", "<br>");
    return txt;
}


NodeGui::NodeGui(QGraphicsItem *parent)
: QObject()
, QGraphicsItem(parent)
, _graph(NULL)
, _internalNode()
, _selected(false)
, _settingNameFromGui(false)
, _nameItem(NULL)
, _boundingBox(NULL)
, _channelsPixmap(NULL)
, _previewPixmap(NULL)
, _persistentMessage(NULL)
, _lastPersistentMessageType(0)
, _stateIndicator(NULL)
, _bitDepthWarning(NULL)
, _inputEdges()
, _outputEdge(NULL)
, _settingsPanel(NULL)
, _mainInstancePanel(NULL)
, _selectedGradient(NULL)
, _defaultGradient(NULL)
, _clonedGradient(NULL)
, _lastRenderStartedSlotCallTime()
, _lastInputNRenderStartedSlotCallTime()
, _wasRenderStartedSlotRun(false)
, _wasBeginEditCalled(false)
, positionMutex()
, _slaveMasterLink(NULL)
, _masterNodeGui()
, _knobsLinks()
, _expressionIndicator(NULL)
, _magnecEnabled()
, _magnecDistance()
, _updateDistanceSinceLastMagnec()
, _distanceSinceLastMagnec()
, _magnecStartingPos()
, _nodeLabel()
, _parentMultiInstance()
{
    
}

NodeGui::~NodeGui()
{
    
    deleteReferences();
    
    delete _clonedGradient;
    delete _selectedGradient;
    delete _defaultGradient;
    delete _disabledGradient;
    delete _bitDepthWarning;
    delete _expressionIndicator;
}


void NodeGui::initialize(NodeGraph* dag,
                         const boost::shared_ptr<NodeGui>& thisAsShared,
                         QVBoxLayout *dockContainer,
                         const boost::shared_ptr<Natron::Node>& internalNode,
                         bool requestedByLoad)
{
    _internalNode = internalNode;
    assert(internalNode);
    _graph = dag;
    
    QObject::connect(this, SIGNAL(nameChanged(QString)), _internalNode.get(), SLOT(setName(QString)));
    
    QObject::connect(_internalNode.get(), SIGNAL(nameChanged(QString)), this, SLOT(onInternalNameChanged(QString)));
    QObject::connect(_internalNode.get(), SIGNAL(refreshEdgesGUI()),this,SLOT(refreshEdges()));
    QObject::connect(_internalNode.get(), SIGNAL(knobsInitialized()),this,SLOT(initializeKnobs()));
    QObject::connect(_internalNode.get(), SIGNAL(inputsInitialized()),this,SLOT(initializeInputs()));
    QObject::connect(_internalNode.get(), SIGNAL(previewImageChanged(int)), this, SLOT(updatePreviewImage(int)));
    QObject::connect(_internalNode.get(), SIGNAL(previewRefreshRequested(int)), this, SLOT(forceComputePreview(int)));
    QObject::connect(_internalNode.get(), SIGNAL(deactivated()),this,SLOT(deactivate()));
    QObject::connect(_internalNode.get(), SIGNAL(activated()), this, SLOT(activate()));
    QObject::connect(_internalNode.get(), SIGNAL(inputChanged(int)), this, SLOT(connectEdge(int)));
    QObject::connect(_internalNode.get(), SIGNAL(persistentMessageChanged(int,QString)),this,SLOT(onPersistentMessageChanged(int,QString)));
    QObject::connect(_internalNode.get(), SIGNAL(persistentMessageCleared()), this, SLOT(onPersistentMessageCleared()));
    QObject::connect(_internalNode.get(), SIGNAL(renderingStarted()), this, SLOT(onRenderingStarted()));
    QObject::connect(_internalNode.get(), SIGNAL(renderingEnded()), this, SLOT(onRenderingFinished()));
    QObject::connect(_internalNode.get(), SIGNAL(inputNIsRendering(int)), this, SLOT(onInputNRenderingStarted(int)));
    QObject::connect(_internalNode.get(), SIGNAL(inputNIsFinishedRendering(int)), this, SLOT(onInputNRenderingFinished(int)));
    QObject::connect(_internalNode.get(), SIGNAL(allKnobsSlaved(bool)), this, SLOT(onAllKnobsSlaved(bool)));
    QObject::connect(_internalNode.get(), SIGNAL(knobsLinksChanged()), this, SLOT(onKnobsLinksChanged()));
    QObject::connect(_internalNode.get(), SIGNAL(outputsChanged()),this,SLOT(refreshOutputEdgeVisibility()));
    QObject::connect(_internalNode.get(), SIGNAL(previewKnobToggled()),this,SLOT(onPreviewKnobToggled()));
    QObject::connect(_internalNode.get(), SIGNAL(disabledKnobToggled(bool)),this,SLOT(onDisabledKnobToggled(bool)));
    QObject::connect(_internalNode.get(), SIGNAL(bitDepthWarningToggled(bool,QString)),this,SLOT(toggleBitDepthIndicator(bool,QString)));
    QObject::connect(_internalNode.get(), SIGNAL(nodeExtraLabelChanged(QString)),this,SLOT(onNodeExtraLabelChanged(QString)));
    
    setCacheMode(DeviceCoordinateCache);
    setZValue(4);
    
    gettimeofday(&_lastRenderStartedSlotCallTime, 0);
    gettimeofday(&_lastInputNRenderStartedSlotCallTime, 0);
    

    
    createGui();
    
    /*building settings panel*/
    _settingsPanel = createPanel(dockContainer,requestedByLoad,thisAsShared);
    if (_settingsPanel) {
        QObject::connect(_settingsPanel,SIGNAL(nameChanged(QString)),this,SLOT(setName(QString)));
        QObject::connect(_settingsPanel,SIGNAL(closeChanged(bool)), this, SIGNAL(settingsPanelClosed(bool)));
        QObject::connect(_settingsPanel,SIGNAL(colorChanged(QColor)),this,SLOT(setDefaultGradientColor(QColor)));
    }
    OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
    if (ofxNode) {
        ofxNode->effectInstance()->beginInstanceEditAction();
    }
    
    
    if (_internalNode->makePreviewByDefault()) {
        ///It calls updateShape
        togglePreview_internal(false);
        
    } else {
        initializeShape();
    }
    
    
    QRectF rect = boundingRect();
    
    _selectedGradient = new QLinearGradient(rect.topLeft(), rect.bottomRight());
    
    _defaultGradient = new QLinearGradient(rect.topLeft(), rect.bottomRight());
    QColor defaultColor = getCurrentColor();

    _clonedGradient = new QLinearGradient(rect.topLeft(), rect.bottomRight());
    _clonedGradient->setColorAt(0, QColor(200,70,100));
    _clonedGradient->setColorAt(1, QColor(120,120,120));
    
    _disabledGradient = new QLinearGradient(rect.topLeft(), rect.bottomRight());
    _disabledGradient->setColorAt(0,QColor(0,0,0));
    _disabledGradient->setColorAt(1, QColor(20,20,20));
    
    setDefaultGradientColor(defaultColor);

    if (!_internalNode->isMultiInstance()) {
        _nodeLabel = _internalNode->getNodeExtraLabel().c_str();
        _nodeLabel = replaceLineBreaksWithHtmlParagraph(_nodeLabel);
    }
 
    ///Refresh the name in the line edit
    onInternalNameChanged(_internalNode->getName().c_str());
    
    ///Make the output edge
    if (!_internalNode->isOutputNode()) {
        _outputEdge = new Edge(thisAsShared,parentItem());
    }
    
    ///Refresh the disabled knob
    if (_internalNode->isNodeDisabled()) {
        onDisabledKnobToggled(true);
    }
    
}

void NodeGui::initializeShape()
{
    updateShape(NODE_WIDTH,NODE_HEIGHT);
}

NodeSettingsPanel* NodeGui::createPanel(QVBoxLayout* container,bool requestedByLoad,const boost::shared_ptr<NodeGui>& thisAsShared)
{
    NodeSettingsPanel* panel = 0;
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_internalNode->getLiveInstance());
    if (!isViewer) {
        assert(container);
        boost::shared_ptr<MultiInstancePanel> multiPanel;
        if (_internalNode->isTrackerNode() && _internalNode->isMultiInstance() && _internalNode->getParentMultiInstanceName().empty()) {
            multiPanel.reset(new TrackerPanel(thisAsShared));
            
            ///This is valid only if the node is a multi-instance and this is the main instance.
            ///The "real" panel showed on the gui will be the _settingsPanel, but we still need to create
            ///another panel for the main-instance (hidden) knobs to function properly (and also be showed in the CurveEditor)
            
            _mainInstancePanel = new NodeSettingsPanel(boost::shared_ptr<MultiInstancePanel>(),_graph->getGui(),
                                                       thisAsShared,container,container->parentWidget());
            _mainInstancePanel->blockSignals(true);
            _mainInstancePanel->setClosed(true);
            _mainInstancePanel->initializeKnobs();
        }
        panel = new NodeSettingsPanel(multiPanel,_graph->getGui(),thisAsShared,container,container->parentWidget());
        
        if (!requestedByLoad) {
            if (_internalNode->getParentMultiInstanceName().empty()) {
                _graph->getGui()->putSettingsPanelFirst(panel);
                _graph->getGui()->addVisibleDockablePanel(panel);
            }
        } else {
            if (panel) {
                panel->setClosed(true);
            }
        }
	}
    return panel;
}

void NodeGui::createGui()
{
    _boundingBox = new QGraphicsRectItem(this);
    _boundingBox->setZValue(0);
	
    _nameItem = new QGraphicsTextItem(_internalNode->getName().c_str(),this);
    _nameItem->setDefaultTextColor(QColor(0,0,0,255));
    _nameItem->setFont(QFont(NATRON_FONT, NATRON_FONT_SIZE_12));
    _nameItem->setZValue(1);
    
    _persistentMessage = new QGraphicsTextItem("",this);
    _persistentMessage->setZValue(3);
    QFont f = _persistentMessage->font();
    f.setPixelSize(25);
    _persistentMessage->setFont(f);
    _persistentMessage->hide();
    
    _stateIndicator = new QGraphicsRectItem(this);
    _stateIndicator->setZValue(-1);
    _stateIndicator->hide();
    
    QRectF bbox = boundingRect();
    QGradientStops bitDepthGrad;
    bitDepthGrad.push_back(qMakePair(0., QColor(Qt::white)));
    bitDepthGrad.push_back(qMakePair(0.3, QColor(Qt::yellow)));
    bitDepthGrad.push_back(qMakePair(1., QColor(243,137,0)));
    _bitDepthWarning = new NodeGuiIndicator("C",bbox.topLeft(),NATRON_ELLIPSE_WARN_DIAMETER,NATRON_ELLIPSE_WARN_DIAMETER,
                                            bitDepthGrad,QColor(0,0,0,255),this);
    _bitDepthWarning->setActive(false);
    
    
    QGradientStops exprGrad;
    exprGrad.push_back(qMakePair(0., QColor(Qt::white)));
    exprGrad.push_back(qMakePair(0.3, QColor(Qt::green)));
    exprGrad.push_back(qMakePair(1., QColor(69,96,63)));
    _expressionIndicator = new NodeGuiIndicator("E",bbox.topRight(),NATRON_ELLIPSE_WARN_DIAMETER,NATRON_ELLIPSE_WARN_DIAMETER,
                                                exprGrad,QColor(255,255,255),this);
    _expressionIndicator->setToolTip(tr("This node has one or several expression(s) involving values of parameters of other "
                                        "nodes in the project. Hover the mouse on the green connections to see what are the effective links."));
    _expressionIndicator->setActive(false);

}

void NodeGui::setDefaultGradientColor(const QColor& color)
{
    assert(_clonedGradient && _defaultGradient && _disabledGradient && _selectedGradient);
    _defaultGradient->setColorAt(1,color);
    QColor colorBrightened;
    colorBrightened.setRedF(Natron::clamp(color.redF() * 1.5));
    colorBrightened.setGreenF(Natron::clamp(color.greenF() * 1.5));
    colorBrightened.setBlueF(Natron::clamp(color.blueF() * 1.5));
    _defaultGradient->setColorAt(0, colorBrightened);
    refreshCurrentBrush();
}

void NodeGui::beginEditKnobs() {
    _wasBeginEditCalled = true;
    _internalNode->beginEditKnobs();
}

void NodeGui::togglePreview_internal(bool refreshPreview)
{
    if (!canMakePreview()) {
        return;
    }
    if (_internalNode->isPreviewEnabled()) {
        if(!_previewPixmap){
            QImage prev(NATRON_PREVIEW_WIDTH, NATRON_PREVIEW_HEIGHT, QImage::Format_ARGB32);
            prev.fill(Qt::black);
            QPixmap prev_pixmap = QPixmap::fromImage(prev);
            _previewPixmap = new QGraphicsPixmapItem(prev_pixmap,this);
            _previewPixmap->setZValue(1);
           
        }
        updateShape(NODE_WITH_PREVIEW_WIDTH,NODE_WITH_PREVIEW_HEIGHT);
        _previewPixmap->stackBefore(_nameItem);
        _previewPixmap->show();
        if (refreshPreview) {
            _internalNode->computePreviewImage(_graph->getGui()->getApp()->getTimeLine()->currentFrame());
        }
    } else {
        if (_previewPixmap) {
            _previewPixmap->hide();
        }
        updateShape(NODE_WIDTH,NODE_HEIGHT);
    }
}

void NodeGui::onPreviewKnobToggled()
{
    togglePreview_internal();
}

void NodeGui::togglePreview(){
    _internalNode->togglePreview();
    togglePreview_internal();
}


void NodeGui::removeUndoStack(){
    if(_graph->getGui() && getUndoStack()){
        _graph->getGui()->removeUndoStack(getUndoStack());
    }
}

void NodeGui::removeSettingsPanel(){
    //called by DockablePanel when it is deleted by Qt's parenting scheme
    _settingsPanel = NULL;
}

void NodeGui::updateShape(int width,int height)
{
    QPointF topLeft = mapFromParent(pos());
    
    QRectF labelBbox = _nameItem->boundingRect();
    double realHeight =  std::max((double)height,labelBbox.height());
    
    _boundingBox->setRect(topLeft.x(),topLeft.y(),width,realHeight);
    
    QFont f(NATRON_FONT_ALT, NATRON_FONT_SIZE_12);
    QFontMetrics metrics(f);
    int nameWidth = labelBbox.width();
    _nameItem->setX(topLeft.x() + (width / 2) - (nameWidth / 2));
    _nameItem->setY(topLeft.y()+10 - metrics.height() / 2);
    
    QString persistentMessage = _persistentMessage->toPlainText();
    f.setPixelSize(25);
    metrics = QFontMetrics(f);
    int pMWidth = metrics.width(persistentMessage);
    
    QPointF bitDepthPos(topLeft.x() + width / 2,0);
    _bitDepthWarning->refreshPosition(bitDepthPos);
    
    _expressionIndicator->refreshPosition(topLeft + QPointF(width,0));
    
    _persistentMessage->setPos(topLeft.x() + (width/2) - (pMWidth/2), topLeft.y() + height/2 - metrics.height()/2);
    _stateIndicator->setRect(topLeft.x()-NATRON_STATE_INDICATOR_OFFSET,topLeft.y()-NATRON_STATE_INDICATOR_OFFSET,
                             width+NATRON_STATE_INDICATOR_OFFSET*2,height+NATRON_STATE_INDICATOR_OFFSET*2);
    if(_previewPixmap)
        _previewPixmap->setPos(topLeft.x() + width / 2 - NATRON_PREVIEW_WIDTH / 2,
                               topLeft.y() + height / 2 - NATRON_PREVIEW_HEIGHT / 2 + 10);

    refreshPosition(pos().x(), pos().y());
}

void NodeGui::refreshPositionEnd(double x, double y)
{
    setPos(x, y);
    QRectF bbox = mapRectToScene(boundingRect());
    const std::list<boost::shared_ptr<NodeGui> >& allNodes = _graph->getAllActiveNodes();
    
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = allNodes.begin(); it!=allNodes.end(); ++it) {
        if (it->get() != this && (*it)->intersects(bbox)) {
            setAboveItem(it->get());
        }
    }
    refreshEdges();
    const std::list<boost::shared_ptr<Natron::Node> >& outputs = _internalNode->getOutputs();

    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
        assert(*it);
        (*it)->doRefreshEdgesGUI();
    }
    emit positionChanged();
}

void NodeGui::refreshPosition(double x,double y,bool skipMagnet,const QPointF& mouseScenePos)
{
   
    if (appPTR->getCurrentSettings()->isSnapToNodeEnabled() && !skipMagnet) {
        QSize size = getSize();
        ///handle magnetic grid
        QPointF middlePos(x + size.width() / 2,y + size.height() / 2);

        
        if (_magnecEnabled.x() || _magnecEnabled.y()) {
            if (_magnecEnabled.x()) {
                _magnecDistance.rx() += (x - _magnecStartingPos.x());
                if (std::abs(_magnecDistance.x()) >= NATRON_MAGNETIC_GRID_RELEASE_DISTANCE) {
                    _magnecEnabled.rx() = 0;
                    _updateDistanceSinceLastMagnec.rx() = 1;
                    _distanceSinceLastMagnec.rx() = 0;
                }
            }
            if (_magnecEnabled.y()) {
                _magnecDistance.ry() += (y - _magnecStartingPos.y());
                if (std::abs(_magnecDistance.y()) >= NATRON_MAGNETIC_GRID_RELEASE_DISTANCE) {
                    _magnecEnabled.ry() = 0;
                    _updateDistanceSinceLastMagnec.ry() = 1;
                    _distanceSinceLastMagnec.ry() = 0;
                }
            }
            
            
            if (!_magnecEnabled.x() && !_magnecEnabled.y()) {
                ///When releasing the grip, make sure to follow the mouse
                QPointF newPos = (mapToParent(mapFromScene(mouseScenePos)));
                newPos.rx() -= size.width() / 2;
                newPos.ry() -= size.height() / 2;
                refreshPositionEnd(newPos.x(),newPos.y());
                return;

            } else if (_magnecEnabled.x() && !_magnecEnabled.y()) {
                x = pos().x();
            } else if (!_magnecEnabled.x() && _magnecEnabled.y()) {
                y = pos().y();
            } else {
                return;
            }
        }
        
        bool continueMagnet = true;
        if (_updateDistanceSinceLastMagnec.rx() == 1) {
            _distanceSinceLastMagnec.rx() = x - _magnecStartingPos.x();
            if (std::abs(_distanceSinceLastMagnec.x()) > (NATRON_MAGNETIC_GRID_GRIP_TOLERANCE)) {
                _updateDistanceSinceLastMagnec.rx() = 0;
            } else {
                continueMagnet = false;
            }
        }
        if (_updateDistanceSinceLastMagnec.ry() == 1) {
            _distanceSinceLastMagnec.ry() = y - _magnecStartingPos.y();
            if (std::abs(_distanceSinceLastMagnec.y()) > (NATRON_MAGNETIC_GRID_GRIP_TOLERANCE)) {
                _updateDistanceSinceLastMagnec.ry() = 0;
            } else {
                continueMagnet = false;
            }
        }
        
        
        
        if ((!_magnecEnabled.x() || !_magnecEnabled.y()) && continueMagnet) {
            for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
                ///For each input try to find if the magnet should be enabled
                boost::shared_ptr<NodeGui> inputSource = it->second->getSource();
                if (inputSource) {
                    QSize inputSize = inputSource->getSize();
                    QPointF inputScenePos = inputSource->scenePos();
                    QPointF inputPos = inputScenePos + QPointF(inputSize.width() / 2,inputSize.height() / 2);
                    QPointF mapped = mapToParent(mapFromScene(inputPos));
                    if (!contains(mapped)) {
                        if (!_magnecEnabled.x() && (mapped.x() >= (middlePos.x() - NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) &&
                             mapped.x() <= (middlePos.x() + NATRON_MAGNETIC_GRID_GRIP_TOLERANCE))) {
                            _magnecEnabled.rx() = 1;
                            _magnecDistance.rx() = 0;
                            x = mapped.x() - size.width() / 2;
                            _magnecStartingPos.setX(x);
                        } else if (!_magnecEnabled.y() && (mapped.y() >= (middlePos.y() - NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) &&
                                    mapped.y() <= (middlePos.y() + NATRON_MAGNETIC_GRID_GRIP_TOLERANCE))) {
                            _magnecEnabled.ry() = 1;
                            _magnecDistance.ry() = 0;
                            y = mapped.y() - size.height() / 2;
                            _magnecStartingPos.setY(y);
                        }
                    }
                }
            }
            
            if ((!_magnecEnabled.x() || !_magnecEnabled.y())) {
                ///check now the outputs
                const std::list<boost::shared_ptr<Natron::Node> >& outputs = _internalNode->getOutputs();
                for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
                    boost::shared_ptr<NodeGui> node = _graph->getGui()->getApp()->getNodeGui(*it);
                    assert(node);
                    QSize outputSize = node->getSize();
                    QPointF nodeScenePos = node->scenePos();
                    QPointF outputPos = nodeScenePos  + QPointF(outputSize.width() / 2,outputSize.height() / 2);
                    QPointF mapped = mapToParent(mapFromScene(outputPos));
                    if (!contains(mapped)) {
                        if (!_magnecEnabled.x() && (mapped.x() >= (middlePos.x() - NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) &&
                                                    mapped.x() <= (middlePos.x() + NATRON_MAGNETIC_GRID_GRIP_TOLERANCE))) {
                            _magnecEnabled.rx() = 1;
                            _magnecDistance.rx() = 0;
                            x = mapped.x() - size.width() / 2;
                            _magnecStartingPos.setX(x);
                        } else if (!_magnecEnabled.y() && (mapped.y() >= (middlePos.y() - NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) &&
                                                           mapped.y() <= (middlePos.y() + NATRON_MAGNETIC_GRID_GRIP_TOLERANCE))) {
                            _magnecEnabled.ry() = 1;
                             _magnecDistance.ry() = 0;
                            y = mapped.y() - size.height() / 2;
                            _magnecStartingPos.setY(y);
                        }
                    }
                    
                }
            }
            
        }
    }
    
    refreshPositionEnd(x, y);
}

void NodeGui::setAboveItem(QGraphicsItem* item)
{
    item->stackBefore(this);
    for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        boost::shared_ptr<NodeGui> inputSource = it->second->getSource();
        if (inputSource.get() != item) {
            item->stackBefore(it->second);
        }
    }
    if (_outputEdge) {
        item->stackBefore(_outputEdge);
    }
}

void NodeGui::changePosition(double dx,double dy) {
    QPointF p = pos();
    refreshPosition(p.x() + dx, p.y() + dy);
}

void NodeGui::refreshEdges() {
    const std::vector<boost::shared_ptr<Natron::Node> >& nodeInputs = _internalNode->getInputs_mt_safe();
    for (NodeGui::InputEdgesMap::const_iterator i = _inputEdges.begin(); i!= _inputEdges.end(); ++i){
        assert(i->first < (int)nodeInputs.size() && i->first >= 0);
        boost::shared_ptr<NodeGui> nodeInputGui = _graph->getGui()->getApp()->getNodeGui(nodeInputs[i->first]);
        i->second->setSource(nodeInputGui);
        i->second->initLine();
    }
    if (_outputEdge) {
        _outputEdge->initLine();
    }
}

void NodeGui::refreshKnobLinks()
{
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it!=_knobsLinks.end(); ++it) {
        it->arrow->refreshPosition();
    }
    if (_slaveMasterLink) {
        _slaveMasterLink->refreshPosition();
    }
}

void NodeGui::markInputNull(Edge* e){
    for (U32 i = 0; i < _inputEdges.size(); ++i) {
        if (_inputEdges[i] == e) {
            _inputEdges[i] = 0;
        }
    }
}



void NodeGui::updateChannelsTooltip(const Natron::ChannelSet& chan){
    QString tooltip;
    tooltip += "Channels in input: ";
    foreachChannels( z,chan){
        tooltip += "\n";
        tooltip += Natron::getChannelName(z).c_str();
    }
    //_channelsPixmap->setToolTip(Qt::convertFromPlainText(tooltip, Qt::WhiteSpaceNormal));
}

void NodeGui::updatePreviewImage(int time) {
    
    if(_internalNode->isPreviewEnabled()  && _internalNode->getApp()->getProject()->isAutoPreviewEnabled()) {
        QtConcurrent::run(this,&NodeGui::computePreviewImage,time);
    }
}

void NodeGui::forceComputePreview(int time) {
    if(_internalNode->isPreviewEnabled()) {
        QtConcurrent::run(this,&NodeGui::computePreviewImage,time);
    }
}

void NodeGui::computePreviewImage(int time){
    
    if (_internalNode->isRenderingPreview()) {
        return;
    }
    
    int w = NATRON_PREVIEW_WIDTH;
    int h = NATRON_PREVIEW_HEIGHT;
    size_t dataSize = 4*w*h;
    {
#ifndef __NATRON_WIN32__
        unsigned int* buf = (unsigned int*)calloc(dataSize,1);
#else
		unsigned int* buf = (unsigned int*)malloc(dataSize);
		for (int i = 0; i < w*h ;++i) {
				buf[i] = qRgba(0,0,0,255);
		}
#endif
        _internalNode->makePreviewImage(time, &w, &h, buf);
        {
            QImage img(reinterpret_cast<const uchar*>(buf), w, h, QImage::Format_ARGB32_Premultiplied);
            QPixmap prev_pixmap = QPixmap::fromImage(img);
            _previewPixmap->setPixmap(prev_pixmap);
            QPointF topLeft = mapFromParent(pos());
            QRectF bbox = boundingRect();
            _previewPixmap->setPos(topLeft.x() + bbox.width() / 2 - w / 2,
                                   topLeft.y() + bbox.height() / 2 - h / 2 + 10);
        }
        free(buf);
    }
}
void NodeGui::initializeInputs()
{
    
    ///Also refresh the output position
    if (_outputEdge) {
        _outputEdge->initLine();
    }
    
    ///The actual numbers of inputs of the internal node
    int inputnb = _internalNode->getMaxInputCount();
    
    ///Delete all un-necessary inputs that may exist (This is true for inspector nodes)
    while ((int)_inputEdges.size() > inputnb) {
        InputEdgesMap::iterator it = _inputEdges.end();
        --it;
        delete it->second;
        _inputEdges.erase(it);
    }
    
    ///Make new edge for all non existing inputs
    boost::shared_ptr<NodeGui> thisShared = _graph->getNodeGuiSharedPtr(this);
    for(int i = 0; i < inputnb;++i){
        if(_inputEdges.find(i) == _inputEdges.end()){
            Edge* edge = new Edge(i,0.,thisShared,parentItem());
            if (_internalNode->getLiveInstance()->isInputRotoBrush(i)) {
                edge->setActive(false);
                edge->hide();
            }
            _inputEdges.insert(make_pair(i,edge));
        }
    }
    
    int emptyInputsCount = 0;
    for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        if(!it->second->hasSource() && it->second->isVisible()){
            ++emptyInputsCount;
        }
    }
    
    InspectorNode* isInspector = dynamic_cast<InspectorNode*>(_internalNode.get());
    if (isInspector) {
        ///if the node is an inspector and it has only 1 empty input, display it aside
        if(emptyInputsCount == 1 && _internalNode->getMaxInputCount() > 1){
            for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
                if(!it->second->hasSource()){
                    it->second->setAngle(M_PI);
                    it->second->initLine();
                    return;
                }
            }
            
        }
    }
    
    
    double piDividedbyX = M_PI/(emptyInputsCount+1);
    double angle = M_PI - piDividedbyX;
    for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        if(!it->second->hasSource() && it->second->isVisible()){
            it->second->setAngle(angle);
            angle -= piDividedbyX;
            it->second->initLine();
        }
    }
    
}
bool NodeGui::contains(const QPointF &point) const {
    QRectF bbox = boundingRect();
    bbox.adjust(-5, -5, 5, 5);
    return bbox.contains(point);
}

bool NodeGui::intersects(const QRectF& rect) const
{
    QRectF mapped = mapRectFromScene(rect);
    return boundingRect().intersects(mapped);
}

QPainterPath NodeGui::shape() const
{
    return _boundingBox->shape();
    
}

QRectF NodeGui::boundingRect() const{
    QTransform t;
    QRectF bbox = _boundingBox->boundingRect();
    QPointF center = bbox.center();
    t.translate(center.x(), center.y());
    t.scale(scale(), scale());
    t.translate(-center.x(), -center.y());
    return t.mapRect(bbox);
}
QRectF NodeGui::boundingRectWithEdges() const{
    QRectF ret;
    QRectF bbox = boundingRect();
    ret = mapToScene(bbox).boundingRect();
    for (InputEdgesMap::const_iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        ret = ret.united(it->second->mapToScene(it->second->boundingRect()).boundingRect());
    }
    return ret;
}

bool NodeGui::isNearby(QPointF &point){
    QPointF p = mapFromScene(point);
    QRectF bbox = boundingRect();
    QRectF r(bbox.x()-NATRON_EDGE_DROP_TOLERANCE,bbox.y()-NATRON_EDGE_DROP_TOLERANCE,
             bbox.width()+NATRON_EDGE_DROP_TOLERANCE,bbox.height()+NATRON_EDGE_DROP_TOLERANCE);
    return r.contains(p);
}

void NodeGui::setName(const QString& name_){
    onInternalNameChanged(name_);
    _settingNameFromGui = true;
    emit nameChanged(name_);
    _settingNameFromGui = false;
}

void NodeGui::onInternalNameChanged(const QString& s){
    if (_settingNameFromGui) {
        return;
    }
    
    setNameItemHtml(s,_nodeLabel);
    
    if(_settingsPanel)
        _settingsPanel->onNameChanged(s);
    scene()->update();
}


Edge* NodeGui::firstAvailableEdge(){
    for (U32 i = 0 ; i < _inputEdges.size(); ++i) {
        Edge* a = _inputEdges[i];
        if (!a->hasSource()) {
            if(_internalNode->getLiveInstance()->isInputOptional(i))
                continue;
        }
        return a;
    }
    return NULL;
}

void NodeGui::applyBrush(const QBrush& brush)
{
     _boundingBox->setBrush(brush);
}

void NodeGui::refreshCurrentBrush()
{
    assert(_clonedGradient && _defaultGradient && _disabledGradient && _selectedGradient);
    bool disabled = _internalNode->isNodeDisabled();
    bool isMultiInstance = !_internalNode->getParentMultiInstanceName().empty() || _internalNode->isMultiInstance();
    if (_internalNode && ((!disabled && !isMultiInstance) || isMultiInstance)) {
        if (_selected) {
            float selectedR,selectedG,selectedB;
            appPTR->getCurrentSettings()->getDefaultSelectedNodeColor(&selectedR, &selectedG, &selectedB);
            QColor selColor;
            selColor.setRgbF(selectedR, selectedG, selectedB);
            QColor brightenedSelColor ;
            brightenedSelColor.setRgbF(Natron::clamp(selColor.redF() * 1.2)
                                       ,Natron::clamp(selColor.greenF() * 1.2)
                                       ,Natron::clamp(selColor.blueF() * 1.2));
            _selectedGradient->setColorAt(1, selColor);
            _selectedGradient->setColorAt(0, brightenedSelColor);
            applyBrush(*_selectedGradient);
        } else {
           if (_slaveMasterLink) {
                applyBrush(*_clonedGradient);
            } else {
                applyBrush(*_defaultGradient);
            }
        }

    } else {
        applyBrush(*_disabledGradient);
    }
}

void NodeGui::setSelected(bool b){
    {
        QMutexLocker l(&_selectedMutex);
        _selected = b;
    }
    refreshCurrentBrush();
    update();
    if(_settingsPanel){
        _settingsPanel->setSelected(b);
        _settingsPanel->update();
        if (b && isSettingsPanelVisible() && _internalNode->isRotoNode()) {
            _graph->getGui()->setRotoInterface(this);
        }
    }
}

bool NodeGui::isSelected() { QMutexLocker l(&_selectedMutex); return _selected; }

void NodeGui::setSelectedGradient(const QLinearGradient& gradient){
    *_selectedGradient = gradient;
    if(_selected){
        applyBrush(*_selectedGradient);
    }
}

void NodeGui::setDefaultGradient(const QLinearGradient& gradient){
    *_defaultGradient = gradient;
    if(!_selected){
        applyBrush(*_defaultGradient);
    }
}

Edge* NodeGui::findConnectedEdge(NodeGui* parent){
    for (U32 i =0 ; i < _inputEdges.size(); ++i) {
        Edge* e = _inputEdges[i];
        
        if (e && e->getSource().get() == parent) {
            return e;
        }
    }
    return NULL;
}

bool NodeGui::connectEdge(int edgeNumber) {
    
    const std::vector<boost::shared_ptr<Natron::Node> >& inputs = _internalNode->getInputs_mt_safe();
    if (edgeNumber < 0 || edgeNumber >= (int)inputs.size()) {
        return false;
    }
   
    boost::shared_ptr<NodeGui> src = _graph->getGui()->getApp()->getNodeGui(inputs[edgeNumber]);
    InputEdgesMap::const_iterator it2 = _inputEdges.find(edgeNumber);
    if(it2 == _inputEdges.end()){
        return false;
    }else{
        it2->second->setSource(src);
        it2->second->initLine();
        return true;
    }
}



Edge* NodeGui::hasEdgeNearbyPoint(const QPointF& pt){
    for (NodeGui::InputEdgesMap::const_iterator i = _inputEdges.begin(); i!= _inputEdges.end(); ++i){
        if(i->second->contains(i->second->mapFromScene(pt))){
            return i->second;
        }
    }
    if (_outputEdge && _outputEdge->contains(_outputEdge->mapFromScene(pt))) {
        return _outputEdge;
    }
    return NULL;
}

Edge* NodeGui::hasBendPointNearbyPoint(const QPointF& pt)
{
    for (NodeGui::InputEdgesMap::const_iterator i = _inputEdges.begin(); i!= _inputEdges.end(); ++i) {
        if (i->second->hasSource() && i->second->isBendPointVisible()) {
            if (i->second->isNearbyBendPoint(pt)) {
                return i->second;
            }
        }
    }
    return NULL;
}

Edge* NodeGui::hasEdgeNearbyRect(const QRectF& rect)
{
    ///try with all 4 corners

    QLineF rectEdges[4] =
    {
        QLineF(rect.topLeft(),rect.topRight()),
        QLineF(rect.topRight(),rect.bottomRight()),
        QLineF(rect.bottomRight(),rect.bottomLeft()),
        QLineF(rect.bottomLeft(),rect.topLeft())
    };
    
    QPointF intersection;
    QPointF middleRect = rect.center();
    Edge* closest = 0;
    double closestSquareDist = 0;
    for (NodeGui::InputEdgesMap::const_iterator i = _inputEdges.begin(); i!= _inputEdges.end(); ++i){
        QLineF edgeLine = i->second->line();
        for (int j = 0; j < 4; ++j) {
            if (edgeLine.intersect(rectEdges[j], &intersection) == QLineF::BoundedIntersection) {
                if (!closest) {
                    closest = i->second;
                    closestSquareDist = (intersection.x() - middleRect.x()) * (intersection.x() - middleRect.x())
                    + (intersection.y() - middleRect.y()) * (intersection.y() - middleRect.y());
                } else {
                    double dist = (intersection.x() - middleRect.x()) * (intersection.x() - middleRect.x())
                    + (intersection.y() - middleRect.y()) * (intersection.y() - middleRect.y());
                    if (dist < closestSquareDist) {
                        closestSquareDist = dist;
                        closest = i->second;
                    }
                }
                break;
            }
        }
    }
    if (closest) {
        return closest;
    }
    
    if (_outputEdge) {
        QLineF edgeLine = _outputEdge->line();
        for (int j = 0; j < 4; ++j) {
            if (edgeLine.intersect(rectEdges[j], &intersection) == QLineF::BoundedIntersection) {
                return _outputEdge;
            }
        }
    }
    return NULL;
}

void NodeGui::showGui()
{
    show();
    setActive(true);
    for (NodeGui::InputEdgesMap::const_iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        _graph->scene()->addItem(it->second);
        it->second->setParentItem(parentItem());
        if (!_internalNode->getLiveInstance()->isInputRotoBrush(it->first)) {
            it->second->setActive(true);
        }
    }
    if (_outputEdge) {
        _graph->scene()->addItem(_outputEdge);
        _outputEdge->setParentItem(parentItem());
        _outputEdge->setActive(true);
    }
    refreshEdges();
    const std::list<boost::shared_ptr<Natron::Node> >& outputs = _internalNode->getOutputs();
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
        assert(*it);
        (*it)->doRefreshEdgesGUI();
    }
    ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(_internalNode->getLiveInstance());
    if (viewer) {
        _graph->getGui()->activateViewerTab(viewer);
    } else {
        if (isSettingsPanelVisible()) {
            setVisibleSettingsPanel(true);
        }
        if (_internalNode->isRotoNode()) {
            _graph->getGui()->setRotoInterface(this);
        }
        OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
        if (ofxNode) {
            ofxNode->effectInstance()->beginInstanceEditAction();
        }

    }
    
    if (_slaveMasterLink) {
        if (!_internalNode->getMasterNode()) {
            onAllKnobsSlaved(false);
        } else {
            _slaveMasterLink->show();
        }
    }
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it!=_knobsLinks.end(); ++it) {
        it->arrow->show();
    }
    
}

void NodeGui::activate() {
    
    ///first activate all child instance if any
    if (_internalNode->isMultiInstance() && _internalNode->getParentMultiInstanceName().empty()) {
        boost::shared_ptr<MultiInstancePanel> panel = getMultiInstancePanel();
        const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& childrenInstances = panel->getInstances();
        for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = childrenInstances.begin();
             it!=childrenInstances.end(); ++it) {
            if (it->first == _internalNode) {
                continue;
            }
            it->first->activate(std::list< boost::shared_ptr<Natron::Node> >(),false);
        }
    }
    
    bool isMultiInstanceChild = !_internalNode->getParentMultiInstanceName().empty();
    
    if (!isMultiInstanceChild) {
        showGui();
    } else {
        ///don't show gui if it is a multi instance child, but still emit the begin edit action
        OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
        if (ofxNode) {
            ofxNode->effectInstance()->beginInstanceEditAction();
        }
    }
    _graph->restoreFromTrash(this);
    _graph->getGui()->getCurveEditor()->addNode(_graph->getNodeGuiSharedPtr(this));

}

void NodeGui::hideGui()
{
    if (!_graph->getGui()) {
        return;
    }
    hide();
    setActive(false);
    for (NodeGui::InputEdgesMap::const_iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        if (it->second->scene()) {
            it->second->scene()->removeItem(it->second);
        }
        it->second->setActive(false);
        it->second->setSource(boost::shared_ptr<NodeGui>());
    }
    if (_outputEdge) {
        if (_outputEdge->scene()) {
            _outputEdge->scene()->removeItem(_outputEdge);
        }
        _outputEdge->setActive(false);
    }
    
    if (_slaveMasterLink) {
        _slaveMasterLink->hide();
    }
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it!=_knobsLinks.end(); ++it) {
        it->arrow->hide();
    }
    
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_internalNode->getLiveInstance());
    if (isViewer) {
        ViewerGL* viewerGui = dynamic_cast<ViewerGL*>(isViewer->getUiContext());
        assert(viewerGui);
        
        const std::list<ViewerTab*>& viewerTabs = _graph->getGui()->getViewersList();
        ViewerTab* currentlySelectedViewer = _graph->getGui()->getLastSelectedViewer();
        
        if (currentlySelectedViewer == viewerGui->getViewerTab()) {
            bool foundOne = false;
            for(std::list<ViewerTab*>::const_iterator it = viewerTabs.begin();it!=viewerTabs.end();++it) {
                if((*it)->getViewer() != viewerGui && (*it)->getInternalNode()->getNode()->isActivated()) {
                    foundOne = true;
                    _graph->getGui()->setLastSelectedViewer((*it));
                    break;
                }
            }
            if (!foundOne) {
                _graph->getGui()->setLastSelectedViewer(NULL);
            }
        }
        _graph->getGui()->deactivateViewerTab(isViewer);

    } else {
        if(isSettingsPanelVisible()){
            setVisibleSettingsPanel(false);
        }
        
        if (_internalNode->isRotoNode()) {
            _graph->getGui()->removeRotoInterface(this, false);
        }
        if (_internalNode->isTrackerNode() && _internalNode->getParentMultiInstanceName().empty()) {
            _graph->getGui()->removeTrackerInterface(this, false);
        }
      
    }
}

void NodeGui::deactivate() {
    ///first deactivate all child instance if any
    if (_internalNode->isMultiInstance() && _internalNode->getParentMultiInstanceName().empty()) {
        boost::shared_ptr<MultiInstancePanel> panel = getMultiInstancePanel();
        assert(panel);
        
        ///Remove keyframes since the settings panel is already closed anyway
        const std::list< std::pair<boost::shared_ptr<Natron::Node>,bool> >& childrenInstances = panel->getInstances();
        
        for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = childrenInstances.begin();
             it!=childrenInstances.end(); ++it) {
            
            if (it->first == _internalNode) {
                continue;
            }
            it->first->deactivate(std::list< boost::shared_ptr<Natron::Node> >(),false,false);
        }
    }
    
    bool isMultiInstanceChild = !_internalNode->getParentMultiInstanceName().empty();
    if (!isMultiInstanceChild) {
        hideGui();
    }
    OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
    if (ofxNode) {
        ofxNode->effectInstance()->endInstanceEditAction();
    }
    _graph->moveToTrash(this);
    if (_graph->getGui()) {
        _graph->getGui()->getCurveEditor()->removeNode(this);
    }
    
    
    if (!isMultiInstanceChild) {
        std::list<ViewerInstance* > viewers;
        getNode()->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it) {
            (*it)->updateTreeAndRender();
        }
    }
}

void NodeGui::initializeKnobs(){
    if(_settingsPanel){
        _settingsPanel->initializeKnobs();
    }
}



void NodeGui::setVisibleSettingsPanel(bool b){
    if (_settingsPanel) {
        _settingsPanel->setClosed(!b);
    }
}

bool NodeGui::isSettingsPanelVisible() const{
    if (_settingsPanel) {
        return !_settingsPanel->isClosed();
    } else {
        return false;
    }
}


void NodeGui::onPersistentMessageChanged(int type,const QString& message)
{
    //keep type in synch with this enum:
    //enum MessageType{INFO_MESSAGE = 0,ERROR_MESSAGE = 1,WARNING_MESSAGE = 2,QUESTION_MESSAGE = 3};
    
    
    ///don't do anything if the last persistent message is the same
    if (message == _lastPersistentMessage || !_persistentMessage || !_stateIndicator) {
        return;
    }
    _persistentMessage->show();
    _stateIndicator->show();
   
    _lastPersistentMessage = message;
    if (type == 1) {
        _persistentMessage->setPlainText("ERROR");
        QColor errColor(128,0,0,255);
        _persistentMessage->setDefaultTextColor(errColor);
        _stateIndicator->setBrush(errColor);
        _lastPersistentMessageType = 1;
    } else if(type == 2) {
        _persistentMessage->setPlainText("WARNING");
        QColor warColor(180,180,0,255);
        _persistentMessage->setDefaultTextColor(warColor);
        _stateIndicator->setBrush(warColor);
        _lastPersistentMessageType = 2;
    } else {
        return;
    }
    setToolTip(message);
    std::list<ViewerInstance* > viewers;
    _internalNode->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
        ViewerTab* tab = _graph->getGui()->getViewerTabForInstance(*it);
        ///the tab might not exist if the node is being deactivated following a tab close request by the user.
  
        if (tab) {
            ///don't update the viewer message if it is the same
            if (tab->getViewer()->getCurrentPersistentMessage() != message) {
                tab->getViewer()->setPersistentMessage(type,message);
            }
        }
    }
    QRectF rect = _boundingBox->rect();
    updateShape(rect.width(), rect.height());
}

void NodeGui::onPersistentMessageCleared() {
    
    if (!_persistentMessage || !_persistentMessage->isVisible()) {
        return;
    }
    _lastPersistentMessage.clear();
    _persistentMessage->hide();
    _stateIndicator->hide();
    setToolTip("");
    std::list<ViewerInstance* > viewers;
    _internalNode->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
        ViewerTab* tab = _graph->getGui()->getViewerTabForInstance(*it);

        ///the tab might not exist if the node is being deactivated following a tab close request by the user.
        if (tab) {
            tab->getViewer()->clearPersistentMessage();
        }
    }
}

QVBoxLayout* NodeGui::getDockContainer() const {
    return _settingsPanel->getContainer();
}

void NodeGui::paint(QPainter* /*painter*/,const QStyleOptionGraphicsItem* /*options*/,QWidget* /*parent*/){
    //nothing special
}


const std::map<boost::shared_ptr<KnobI> ,KnobGui*>& NodeGui::getKnobs() const{
    assert(_settingsPanel);
    if (_mainInstancePanel) {
        return _mainInstancePanel->getKnobs();
    }
    return _settingsPanel->getKnobs();
}

void NodeGui::serialize(NodeGuiSerialization* serializationObject) const{
    serializationObject->initialize(_graph->getNodeGuiSharedPtr(this));
}

void NodeGui::copyFrom(const NodeGuiSerialization& obj) {
    setPos_mt_safe(QPointF(obj.getX(),obj.getY()));
    if (_internalNode->isPreviewEnabled() != obj.isPreviewEnabled()) {
        togglePreview();
    }
}

QUndoStack* NodeGui::getUndoStack() const{
    if (_settingsPanel) {
        return _settingsPanel->getUndoStack();
    } else {
        return NULL;
    }
}

void NodeGui::onRenderingStarted() {
    timeval now;
    gettimeofday(&now, 0);
    double t =  now.tv_sec  - _lastRenderStartedSlotCallTime.tv_sec +
    (now.tv_usec - _lastRenderStartedSlotCallTime.tv_usec) * 1e-6f;
    
    ///if some time elapsed since we last changed the color of the node, allow it to
    ///change again, otherwise it would flicker the screen
    if(t > 0.5) {
        _stateIndicator->setBrush(Qt::yellow);
        _stateIndicator->show();
        _lastRenderStartedSlotCallTime = now;
        _wasRenderStartedSlotRun = true;
    }
}

void NodeGui::onRenderingFinished() {
    if (_wasRenderStartedSlotRun) {
        _stateIndicator->hide();
        _wasRenderStartedSlotRun = false;
    }
}

void NodeGui::onInputNRenderingStarted(int input) {
    timeval now;
    gettimeofday(&now, 0);
    double t =  now.tv_sec  - _lastInputNRenderStartedSlotCallTime.tv_sec +
    (now.tv_usec - _lastInputNRenderStartedSlotCallTime.tv_usec) * 1e-6f;
    
    ///if some time  elapsed since we last changed the color of the edge, allow it to
    ///change again, otherwise it would flicker the screen
    if (t >= 0.5) {
        std::map<int,Edge*>::iterator it = _inputEdges.find(input);
        assert(it != _inputEdges.end());
        it->second->turnOnRenderingColor();
        _lastInputNRenderStartedSlotCallTime = now;
    }
    
    
}

void NodeGui::onInputNRenderingFinished(int input) {
    std::map<int,Edge*>::iterator it = _inputEdges.find(input);
    assert(it != _inputEdges.end());
    it->second->turnOffRenderingColor();
}

void NodeGui::moveBelowPositionRecursively(const QRectF& r) {
    QRectF sceneRect = mapToScene(boundingRect()).boundingRect();

    if (r.intersects(sceneRect)) {
        changePosition(0, r.height() + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
        const std::list<boost::shared_ptr<Natron::Node> >& outputs = getNode()->getOutputs();
        for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!= outputs.end(); ++it) {
            assert(*it);
            boost::shared_ptr<NodeGui> output = _graph->getGui()->getApp()->getNodeGui(*it);
            assert(output);
            sceneRect = mapToScene(boundingRect()).boundingRect();
            output->moveBelowPositionRecursively(sceneRect);
            
        }
    }
}

void NodeGui::moveAbovePositionRecursively(const QRectF& r) {


    QRectF sceneRect = mapToScene(boundingRect()).boundingRect();
    if (r.intersects(sceneRect)) {
        changePosition(0,- r.height() - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
        for (std::map<int,Edge*>::const_iterator it = _inputEdges.begin(); it!=_inputEdges.end();++it) {
            if (it->second->hasSource()) {
                sceneRect = mapToScene(boundingRect()).boundingRect();
                it->second->getSource()->moveAbovePositionRecursively(sceneRect);
            }
        }
    }
}

QPointF NodeGui::getPos_mt_safe() const {
    QMutexLocker l(&positionMutex);
    return pos();
}

void NodeGui::setPos_mt_safe(const QPointF& pos) {
    QMutexLocker l(&positionMutex);
    setPos(pos);
}

void NodeGui::centerGraphOnIt()
{
    _graph->centerOnNode(_graph->getNodeGuiSharedPtr(this));
}

void NodeGui::onAllKnobsSlaved(bool b) {
    if (b) {
        boost::shared_ptr<Natron::Node> masterNode = _internalNode->getMasterNode();
        assert(masterNode);
        boost::shared_ptr<NodeGui> masterNodeGui = _graph->getGui()->getApp()->getNodeGui(masterNode);
        assert(masterNodeGui);
        _masterNodeGui = masterNodeGui;
        assert(!_slaveMasterLink);

        _slaveMasterLink = new LinkArrow(_masterNodeGui.get(),this,parentItem());
        _slaveMasterLink->setColor(QColor(200,100,100));
        _slaveMasterLink->setArrowHeadColor(QColor(243,137,20));
        _slaveMasterLink->setWidth(3);
        if (!_internalNode->isNodeDisabled()) {
            if (!isSelected()) {
                applyBrush(*_clonedGradient);
            }
        }
        
    } else {

        assert(_slaveMasterLink);
        delete _slaveMasterLink;
        _slaveMasterLink = 0;
        _masterNodeGui.reset();
        if (!_internalNode->isNodeDisabled()) {
            if (!isSelected()) {
                applyBrush(*_defaultGradient);
            }
        }
    }
    update();
}

void NodeGui::onKnobsLinksChanged()
{
    typedef std::list<Natron::Node::KnobLink> InternalLinks;
    InternalLinks links;
    _internalNode->getKnobsLinks(links);
    
    ///1st pass: remove the no longer needed links
    KnobGuiLinks newLinks;
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it!=_knobsLinks.end(); ++it) {
        bool found = false;
        for (InternalLinks::iterator it2 = links.begin(); it2!=links.end(); ++it2) {
            if (it2->knob.get() == it->knob && it2->dimension == it->dimension) {
                found = true;
                break;
            }
        }
        if (!found) {
            delete it->arrow;
        } else {
            newLinks.push_back(*it);
        }
    }
    _knobsLinks = newLinks;
    
    ///2nd pass: create the new links
    
    for (InternalLinks::iterator it = links.begin(); it!=links.end(); ++it) {
        bool found = false;
        for (KnobGuiLinks::iterator it2 = _knobsLinks.begin(); it2!=_knobsLinks.end(); ++it2) {
            if (it2->knob == it->knob.get()) {
                found = true;
                break;
            }
        }
        if (!found) {
            boost::shared_ptr<NodeGui> master = getDagGui()->getGui()->getApp()->getNodeGui(it->masterNode);
            LinkArrow* arrow = new LinkArrow(master.get(),this,parentItem());
            arrow->setWidth(2);
            arrow->setColor(QColor(143,201,103));
            arrow->setArrowHeadColor(QColor(200,255,200));
            
            int masterDim,slaveDim;
            slaveDim = it->dimension;
            std::pair<int,boost::shared_ptr<KnobI> > masterKnob = it->knob->getMaster(slaveDim);
            assert(masterKnob.second);
            masterDim = masterKnob.first;
            QString tt;
            tt.append(master->getNode()->getName().c_str());
            tt.append(".");
            tt.append(masterKnob.second->getDescription().c_str());
            if (masterKnob.second->getDimension() > 1) {
                tt.append(".");
                tt.append(masterKnob.second->getDimensionName(masterDim).c_str());
            }
            tt.append(" (master) ");
            
            tt.append("------->");
            
            tt.append(getNode()->getName().c_str());
            tt.append(".");
            tt.append(QString(it->knob->getDescription().c_str()));
            if (it->knob->getDimension() > 1) {
                tt.append(".");
                tt.append(it->knob->getDimensionName(slaveDim).c_str());
            }
            
            tt.append(" (slave) ");
            
            arrow->setToolTip(tt);
            if (!getDagGui()->areKnobLinksVisible()) {
                arrow->setVisible(false);
            }
            LinkedDim guilink;
            guilink.knob = it->knob.get();
            guilink.dimension = slaveDim;
            guilink.arrow = arrow;
            _knobsLinks.push_back(guilink);

        }
    }
    
    if (_knobsLinks.size() > 0) {
        if (!_expressionIndicator->isActive()) {
            _expressionIndicator->setActive(true);
        }
    } else {
        if (_expressionIndicator->isActive()) {
            _expressionIndicator->setActive(false);
        }
    }
    
}


void NodeGui::refreshOutputEdgeVisibility() {
    if (_outputEdge) {
        if (_internalNode->getOutputs().empty()) {
            if (!_outputEdge->isVisible()) {
                _outputEdge->setActive(true);
                _outputEdge->show();
            }
            
        } else {
            if (_outputEdge->isVisible()) {
                _outputEdge->setActive(false);
                _outputEdge->hide();
            }
        }
    }
}

void NodeGui::deleteReferences()
{
    removeUndoStack();
    for(InputEdgesMap::const_iterator it = _inputEdges.begin();it!=_inputEdges.end();++it){
        Edge* e = it->second;
        if(e){
            QGraphicsScene* scene = e->scene();
            if(scene){
                scene->removeItem(e);
            }
            e->setParentItem(NULL);
            delete e;
        }
    }
    _inputEdges.clear();
    
    if (_outputEdge) {
        QGraphicsScene* scene = _outputEdge->scene();
        if(scene){
            scene->removeItem(_outputEdge);
        }
        _outputEdge->setParentItem(NULL);
        delete _outputEdge;
        _outputEdge = NULL;
    }
    
    if (_settingsPanel) {
        _settingsPanel->setParent(NULL);
        delete _settingsPanel;
        _settingsPanel = NULL;
    }
}

QSize NodeGui::getSize() const
{
    QRectF bbox = boundingRect();
    return QSize(bbox.width(),bbox.height());
}


void NodeGui::onDisabledKnobToggled(bool disabled) {
    if (!_nameItem) {
        return;
    }
    
    ///When received whilst the node is under a multi instance, let the MultiInstancePanel call this slot instead.
    if (sender() == _internalNode.get() && _internalNode->isMultiInstance()) {
        return;
    }
    
    if (disabled) {
        _nameItem->setDefaultTextColor(QColor(120,120,120));
    } else {
        _nameItem->setDefaultTextColor(QColor(0,0,0));
    }
    refreshCurrentBrush();
}

void NodeGui::toggleBitDepthIndicator(bool on,const QString& tooltip)
{
    if (on) {
        setToolTip(Qt::convertFromPlainText(tooltip,Qt::WhiteSpaceNormal));
        _bitDepthWarning->setToolTip(tooltip);
    } else {
        setToolTip("");
        _bitDepthWarning->setToolTip("");
    }
    _bitDepthWarning->setActive(on);
    
}

////////////////////////////////////////// NodeGuiIndicator ////////////////////////////////////////////////////////

struct NodeGuiIndicatorPrivate
{
    
    QGraphicsEllipseItem* ellipse;
    QGraphicsTextItem* textItem;
    QGradientStops gradStops;
    
    NodeGuiIndicatorPrivate(const QString& text,
                            const QPointF& topLeft,
                            int width,int height,
                            const QGradientStops& gradient,
                            const QColor& textColor,
                            QGraphicsItem* parent)
    : ellipse(NULL)
    , textItem(NULL)
    , gradStops(gradient)
    {
        ellipse = new QGraphicsEllipseItem(parent);
        int ellipseRad = width / 2;
        QPoint ellipsePos(topLeft.x()+ (width / 2) -ellipseRad, -ellipseRad);
        QRectF ellipseRect(ellipsePos.x(),ellipsePos.y(),width,height);
        ellipse->setRect(ellipseRect);
        ellipse->setZValue(2);
        
        QPointF ellipseCenter = ellipseRect.center();

        QRadialGradient radialGrad(ellipseCenter,ellipseRad);
        radialGrad.setStops(gradStops);
        ellipse->setBrush(radialGrad);

        
        textItem = new QGraphicsTextItem(text,parent);
        QFont font(NATRON_FONT_ALT, NATRON_FONT_SIZE_10);
        QFontMetrics fm(font);
        textItem->setPos(topLeft.x()  - 2 * width / 3, topLeft.y() - 2 * fm.height() / 3);
        textItem->setFont(font);
        textItem->setDefaultTextColor(textColor);
        textItem->setZValue(2);
#if QT_VERSION < 0x050000
        textItem->scale(0.8, 0.8);
#else
        textItem->setScale(0.8);
#endif
    }
};

NodeGuiIndicator::NodeGuiIndicator(const QString& text,
                                   const QPointF& topLeft,
                                   int width,int height,
                                   const QGradientStops& gradient,
                                   const QColor& textColor,
                                   QGraphicsItem* parent)
: _imp(new NodeGuiIndicatorPrivate(text,topLeft,width,height,gradient,textColor,parent))
{
    
}

NodeGuiIndicator::~NodeGuiIndicator()
{
    
}

void NodeGuiIndicator::setToolTip(const QString& tooltip)
{
    _imp->ellipse->setToolTip(Qt::convertFromPlainText(tooltip,Qt::WhiteSpaceNormal));
}

void NodeGuiIndicator::setActive(bool active)
{
    _imp->ellipse->setActive(active);
    _imp->textItem->setActive(active);
    _imp->ellipse->setVisible(active);
    _imp->textItem->setVisible(active);
}

bool NodeGuiIndicator::isActive() const
{
    return _imp->ellipse->isVisible();
}

void NodeGuiIndicator::refreshPosition(const QPointF& topLeft)
{
    QRectF r = _imp->ellipse->rect();
    int ellipseRad = r.width() / 2;
    QPoint ellipsePos(topLeft.x() - ellipseRad, topLeft.y() - ellipseRad);

    QRectF ellipseRect(ellipsePos.x(), ellipsePos.y(), r.width(), r.height());
    _imp->ellipse->setRect(ellipseRect);
    
    QRadialGradient radialGrad(ellipseRect.center(),ellipseRad);
    radialGrad.setStops(_imp->gradStops);
    _imp->ellipse->setBrush(radialGrad);
 
    QFont font = _imp->textItem->font();
    QFontMetrics fm(font);
    _imp->textItem->setPos(topLeft.x()  - 2 * r.width() / 3, topLeft.y() - 2 * fm.height() / 3);
}

///////////////////

void NodeGui::setScale_natron(double scale)
{
    setScale(scale);
    for (std::map<int,Edge*>::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        it->second->setScale(scale);
    }
    
    if (_outputEdge) {
        _outputEdge->setScale(scale);
    }
    refreshEdges();
    const std::list<boost::shared_ptr<Natron::Node> >& outputs = _internalNode->getOutputs();
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
        assert(*it);
        (*it)->doRefreshEdgesGUI();
    }
    update();
}

void NodeGui::removeHighlightOnAllEdges()
{
    for (std::map<int,Edge*>::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        it->second->setUseHighlight(false);
    }
    if (_outputEdge) {
        _outputEdge->setUseHighlight(false);
    }
}

Edge* NodeGui::getInputArrow(int inputNb) const
{
    if (inputNb == -1) {
        return _outputEdge;
    }
    std::map<int, Edge*>::const_iterator it = _inputEdges.find(inputNb);
    if (it!= _inputEdges.end()) {
        return it->second;
    }
    return NULL;
}

Edge* NodeGui::getOutputArrow() const
{
    return _outputEdge;
}

void NodeGui::setNameItemHtml(const QString& name,const QString& label)
{
    if (!_nameItem) {
        return;
    }
    QString textLabel;
    textLabel.append("<div align=\"center\">");
    bool hasFontData = true;
    if (!label.isEmpty()) {
        QString labelCopy = label;
        
        ///remove any custom data tag natron might have added
        QString startCustomTag(NATRON_CUSTOM_HTML_TAG_START);
        int startCustomData = labelCopy.indexOf(startCustomTag);
        if (startCustomData != -1) {
            labelCopy.remove(startCustomData, startCustomTag.size());
            
            QString endCustomTag(NATRON_CUSTOM_HTML_TAG_END);
            int endCustomData = labelCopy.indexOf(endCustomTag,startCustomData);
            assert(endCustomData != -1);
            labelCopy.remove(endCustomData, endCustomTag.size());
        }
        
        ///add the node name into the html encoded label
        int startFontTag = labelCopy.indexOf("<font size=");
        hasFontData = startFontTag != -1;
        
        QString toFind("\">");
        int endFontTag = labelCopy.indexOf(toFind,startFontTag);
        
        int i = endFontTag += toFind.size();
        labelCopy.insert(i == -1 ? 0 : i, name + "<br>");
        
        textLabel.append(labelCopy);
    } else {
        ///Default to something not too bad
        QString fontTag = QString("<font size=\"%1\" color=\"%2\" face=\"%3\">")
        .arg(6)
        .arg(QColor(Qt::black).name())
        .arg("Verdana");
        textLabel.append(fontTag);
        textLabel.append(name);
        textLabel.append("</font>");
    }
    textLabel.append("</div>");
    _nameItem->setHtml(textLabel);
    _nameItem->adjustSize();

    
    
    QFont f;
    if (hasFontData) {
        String_KnobGui::parseFont(textLabel, f);
    }
    _nameItem->setFont(f);
    
    
    bool hasPreview =  _internalNode->isPreviewEnabled();
    double nodeHeight = hasPreview ? NODE_WITH_PREVIEW_HEIGHT : NODE_HEIGHT;
    double nodeWidth = hasPreview ? NODE_WITH_PREVIEW_WIDTH : NODE_WIDTH;
    QRectF labelBbox = _nameItem->boundingRect();
    updateShape(nodeWidth, std::max(nodeHeight,labelBbox.height()));
    

}

void NodeGui::onNodeExtraLabelChanged(const QString& label)
{
    _nodeLabel = label;
    if (_internalNode->isMultiInstance()) {
        ///The multi-instances store in the kOfxParamStringSublabelName knob the name of the instance
        ///Since the "main-instance" is the one displayed on the node-graph we don't want it to display its name
        ///hence we remove it
        _nodeLabel = String_KnobGui::removeNatronHtmlTag(_nodeLabel);
    }
    _nodeLabel = replaceLineBreaksWithHtmlParagraph(_nodeLabel); ///< maybe we should do this in the knob itself when the user writes ?
    setNameItemHtml(_internalNode->getName().c_str(),_nodeLabel);
}

QColor NodeGui::getCurrentColor() const
{
    return _settingsPanel ? _settingsPanel->getCurrentColor() : QColor(142,142,142);
}

void NodeGui::setCurrentColor(const QColor& c)
{
    if (_settingsPanel) {
        _settingsPanel->setCurrentColor(c);
    }
}

void NodeGui::onSwitchInputActionTriggered()
{
    if (_internalNode->getMaxInputCount() >= 2) {
        _internalNode->switchInput0And1();
        std::list<ViewerInstance* > viewers;
        _internalNode->hasViewersConnected(&viewers);
        for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
            (*it)->updateTreeAndRender();
        }
        _internalNode->getApp()->triggerAutoSave();
    }
}

///////////////////

TextItem::TextItem(QGraphicsItem* parent )
: QGraphicsTextItem(parent)
, _alignement(Qt::AlignCenter)
{
    init();
}

TextItem::TextItem(const QString& text, QGraphicsItem* parent)
: QGraphicsTextItem(text,parent)
, _alignement(Qt::AlignCenter)
{
    init();
}

void TextItem::setAlignment(Qt::Alignment alignment)
{
    _alignement = alignment;
    QTextBlockFormat format;
    format.setAlignment(alignment);
    QTextCursor cursor = textCursor();      // save cursor position
    int position = textCursor().position();
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    cursor.clearSelection();
    cursor.setPosition(position);           // restore cursor position
    setTextCursor(cursor);
}

int TextItem::type() const
{
    return Type;
}

void TextItem::updateGeometry(int, int, int)
{
    updateGeometry();
}

void TextItem::updateGeometry()
{
    QPointF topRightPrev = boundingRect().topRight();
    setTextWidth(-1);
    setTextWidth(boundingRect().width());
    setAlignment(_alignement);
    QPointF topRight = boundingRect().topRight();
    
    if (_alignement & Qt::AlignRight)
    {
        setPos(pos() + (topRightPrev - topRight));
    }
}

void TextItem::init()
{
    updateGeometry();
    connect(document(), SIGNAL(contentsChange(int, int, int)),
            this, SLOT(updateGeometry(int, int, int)));
}

void NodeGui::refreshKnobsAfterTimeChange(SequenceTime time)
{
    if ((_settingsPanel && !_settingsPanel->isClosed())) {
        _internalNode->getLiveInstance()->refreshAfterTimeChange(time);
    } else if (!_internalNode->getParentMultiInstanceName().empty()) {
        _internalNode->getLiveInstance()->refreshInstanceSpecificKnobsOnly(time);
    }
}

void NodeGui::onSettingsPanelClosedChanged(bool closed)
{
    if (!_settingsPanel) {
        return;
    }
    
    DockablePanel* panel = dynamic_cast<DockablePanel*>(sender());
    assert(panel);
    if (panel == _settingsPanel) {
        ///if it is a multiinstance, notify the multi instance panel
        if (_mainInstancePanel) {
            _settingsPanel->getMultiInstancePanel()->onSettingsPanelClosed(closed);
        } else {
            if (!closed) {
                SequenceTime time = _internalNode->getApp()->getTimeLine()->currentFrame();
                _internalNode->getLiveInstance()->refreshAfterTimeChange(time);
            }
        }
    }
}

boost::shared_ptr<MultiInstancePanel> NodeGui::getMultiInstancePanel() const
{
    if (_settingsPanel) {
        return _settingsPanel->getMultiInstancePanel();
    } else {
        return boost::shared_ptr<MultiInstancePanel>();
    }
}

bool NodeGui::shouldDrawOverlay() const
{
    if (_internalNode->isNodeDisabled()) {
        return false;
    }
    
    if (_internalNode->isTrackerNode()) {
        ///Tracker overlays are handled in the TrackerGui by Natron so we can have more control over them
        ///This also allows us to have control with the selection in the settings panel
        return false;
    }
    
    if (_parentMultiInstance) {
        return _parentMultiInstance->isSettingsPanelVisible();
    } else {
        return _internalNode->isActivated() && isSettingsPanelVisible();
    }
}

void NodeGui::setParentMultiInstance(const boost::shared_ptr<NodeGui>& node)
{
    _parentMultiInstance = node;
}

void NodeGui::setKnobLinksVisible(bool visible)
{
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it!=_knobsLinks.end(); ++it) {
        it->arrow->setVisible(visible);
    }
}

//////////Dot node gui
DotGui::DotGui(QGraphicsItem* parent)
: NodeGui(parent)
, diskShape(NULL)
{
    
}

void DotGui::createGui()
{
    diskShape = new QGraphicsEllipseItem(this);
    QPointF topLeft = mapFromParent(pos());
    diskShape->setRect(QRectF(topLeft.x(),topLeft.y(),DOT_GUI_DIAMETER,DOT_GUI_DIAMETER)) ;
}

void DotGui::applyBrush(const QBrush& brush)
{
    diskShape->setBrush(brush);
}

NodeSettingsPanel* DotGui::createPanel(QVBoxLayout* container,bool /*requestedByLoad*/,
                                       const boost::shared_ptr<NodeGui>& thisAsShared)
{
    NodeSettingsPanel* panel = new NodeSettingsPanel(boost::shared_ptr<MultiInstancePanel>(),
                                                     getDagGui()->getGui(),
                                                     thisAsShared,
                                                     container,container->parentWidget());
    ///Always close the panel by default for Dots
    panel->setClosed(true);
    return panel;
}

QRectF DotGui::boundingRect() const
{
    QTransform t;
    QRectF bbox = diskShape->boundingRect();
    QPointF center = bbox.center();
    t.translate(center.x(), center.y());
    t.scale(scale(), scale());
    t.translate(-center.x(), -center.y());
    return t.mapRect(bbox);
}

QPainterPath DotGui::shape() const
{
    return diskShape->shape();
    
}
