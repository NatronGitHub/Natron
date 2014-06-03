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

#include "Gui/Edge.h"
#include "Gui/DockablePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"
#include "Gui/Gui.h"
#include "Gui/KnobGui.h"
#include "Gui/ViewerGL.h"
#include "Gui/CurveEditor.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"

#include "Engine/OfxEffectInstance.h"
#include "Engine/ViewerInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/ChannelSet.h"
#include "Engine/Timer.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"

#define NATRON_STATE_INDICATOR_OFFSET 5

#define NATRON_EDGE_DROP_TOLERANCE 15

#define NATRON_MAGNETIC_GRID_GRIP_TOLERANCE 10

#define NATRON_MAGNETIC_GRID_RELEASE_DISTANCE 25

using namespace Natron;

using std::make_pair;

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NodeGui::NodeGui(QGraphicsItem *parent)
: QObject()
, QGraphicsItem(parent)
, _graph(NULL)
, _internalNode()
, _selected(false)
, _nameItem(NULL)
, _boundingBox(NULL)
, _channelsPixmap(NULL)
, _previewPixmap(NULL)
, _persistentMessage(NULL)
, _lastPersistentMessageType(0)
, _stateIndicator(NULL)
, _inputEdges()
, _outputEdge(NULL)
, _panelDisplayed(false)
, _settingsPanel(0)
, _selectedGradient(NULL)
, _defaultGradient(NULL)
, _clonedGradient(NULL)
, _menu()
, _lastRenderStartedSlotCallTime()
, _lastInputNRenderStartedSlotCallTime()
, _wasRenderStartedSlotRun(false)
, _wasBeginEditCalled(false)
, positionMutex()
, _slaveMasterLink(NULL)
, _masterNodeGui()
, _magnecEnabled(false)
, _magnecStartingPos()
{
    
}

NodeGui::~NodeGui(){
    
    deleteReferences();
    
    delete _clonedGradient;
    delete _selectedGradient;
    delete _defaultGradient;
    delete _disabledGradient;
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
    _menu = new QMenu(dag);
    
    QObject::connect(this, SIGNAL(nameChanged(QString)), _internalNode.get(), SLOT(onGUINameChanged(QString)));
    
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
    QObject::connect(_internalNode.get(), SIGNAL(slavedStateChanged(bool)), this, SLOT(onSlaveStateChanged(bool)));
    QObject::connect(_internalNode.get(), SIGNAL(outputsChanged()),this,SLOT(refreshOutputEdgeVisibility()));
    QObject::connect(_internalNode.get(), SIGNAL(previewKnobToggled()),this,SLOT(onPreviewKnobToggled()));
    QObject::connect(_internalNode.get(), SIGNAL(disabledKnobToggled(bool)),this,SLOT(onDisabledKnobToggled(bool)));
    
    setCacheMode(DeviceCoordinateCache);
    setZValue(1);
    
    _boundingBox = new QGraphicsRectItem(this);
    _boundingBox->setZValue(0.5);
	
    QPixmap pixmap;
    appPTR->getIcon(NATRON_PIXMAP_RGBA_CHANNELS,&pixmap);
    //_channelsPixmap= new QGraphicsPixmapItem(pixmap,this);
	
    _nameItem = new QGraphicsTextItem(_internalNode->getName().c_str(),this);
    _nameItem->setDefaultTextColor(QColor(0,0,0,255));
    _nameItem->setZValue(0.6);
    
    _persistentMessage = new QGraphicsTextItem("",this);
    _persistentMessage->setZValue(0.7);
    QFont f = _persistentMessage->font();
    f.setPixelSize(25);
    _persistentMessage->setFont(f);
    _persistentMessage->hide();
    
    _stateIndicator = new QGraphicsRectItem(this);
    _stateIndicator->setZValue(-1);
    _stateIndicator->hide();
    
    /*building settings panel*/
    if(_internalNode->pluginID() != "Viewer"){
        _panelDisplayed=true;
        assert(dockContainer);
        _settingsPanel = new NodeSettingsPanel(_graph->getGui(),thisAsShared,dockContainer,dockContainer->parentWidget());
        QObject::connect(_settingsPanel,SIGNAL(nameChanged(QString)),this,SLOT(setName(QString)));
        QObject::connect(_settingsPanel, SIGNAL(closeChanged(bool)), this, SIGNAL(settingsPanelClosed(bool)));
        dockContainer->addWidget(_settingsPanel);
        
        if (!requestedByLoad) {
            _graph->getGui()->putSettingsPanelFirst(_settingsPanel);
        } else {
            setVisibleSettingsPanel(false);
        }
        
        if(_internalNode->isOpenFXNode()){
            OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
            ofxNode->effectInstance()->beginInstanceEditAction();
        }
	}
    
    if(_internalNode->makePreviewByDefault() && !_graph->areAllPreviewTurnedOff()){
        togglePreview_internal();
        
    }else{
        updateShape(NODE_LENGTH,NODE_HEIGHT);
    }
    
    
    QRectF rect = _boundingBox->rect();
    
    _selectedGradient = new QLinearGradient(rect.topLeft(), rect.bottomRight());
    _selectedGradient->setColorAt(0, QColor(249,187,81));
    _selectedGradient->setColorAt(1, QColor(150,187,81));
    
    _defaultGradient = new QLinearGradient(rect.topLeft(), rect.bottomRight());
    _defaultGradient->setColorAt(0, QColor(224,224,224));
    _defaultGradient->setColorAt(1, QColor(142,142,142));
    
    _clonedGradient = new QLinearGradient(rect.topLeft(), rect.bottomRight());
    _clonedGradient->setColorAt(0, QColor(200,70,100));
    _clonedGradient->setColorAt(1, QColor(120,120,120));
    
    _disabledGradient = new QLinearGradient(rect.topLeft(), rect.bottomRight());
    _disabledGradient->setColorAt(0,QColor(0,0,0));
    _disabledGradient->setColorAt(1, QColor(20,20,20));
    
    _boundingBox->setBrush(*_defaultGradient);

    
    gettimeofday(&_lastRenderStartedSlotCallTime, 0);
    gettimeofday(&_lastInputNRenderStartedSlotCallTime, 0);
    
    onInternalNameChanged(_internalNode->getName().c_str());
    
    if (!_internalNode->isOutputNode()) {
        _outputEdge = new Edge(thisAsShared,parentItem());
    }

}

void NodeGui::beginEditKnobs() {
    _wasBeginEditCalled = true;
    _internalNode->beginEditKnobs();
}

void NodeGui::togglePreview_internal()
{
    if(_internalNode->isPreviewEnabled()){
        if(!_previewPixmap){
            QImage prev(NATRON_PREVIEW_WIDTH, NATRON_PREVIEW_HEIGHT, QImage::Format_ARGB32);
            prev.fill(Qt::black);
            QPixmap prev_pixmap = QPixmap::fromImage(prev);
            _previewPixmap = new QGraphicsPixmapItem(prev_pixmap,this);
            _previewPixmap->setZValue(0.6);
        }
        updateShape(NODE_WITH_PREVIEW_LENGTH,NODE_WITH_PREVIEW_HEIGHT);
        _previewPixmap->show();
    }else{
        if (_previewPixmap) {
            _previewPixmap->hide();
        }
        updateShape(NODE_LENGTH,NODE_HEIGHT);
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

QSize NodeGui::nodeSize(bool withPreview) {
    QSize ret;
    if (withPreview) {
        ret.setWidth(NodeGui::NODE_WITH_PREVIEW_LENGTH);
        ret.setHeight(NodeGui::NODE_WITH_PREVIEW_HEIGHT);
    } else {
        ret.setWidth(NodeGui::NODE_LENGTH);
        ret.setHeight(NodeGui::NODE_HEIGHT);
    }
    return ret;
}


void NodeGui::removeUndoStack(){
    if(getUndoStack()){
        _graph->getGui()->removeUndoStack(getUndoStack());
    }
}

void NodeGui::removeSettingsPanel(){
    //called by DockablePanel when it is deleted by Qt's parenting scheme
    _settingsPanel = NULL;
}

void NodeGui::updateShape(int width,int height){
    QPointF topLeft = mapFromParent(pos());
    _boundingBox->setRect(topLeft.x(),topLeft.y(),width,height);
    //_channelsPixmap->setPos(topLeft.x()+1,topLeft.y()+1);
    
    QFont f(NATRON_FONT_ALT, NATRON_FONT_SIZE_12);
    QFontMetrics metrics(f);
    int nameWidth = metrics.width(_nameItem->toPlainText());
    _nameItem->setX(topLeft.x()+(_boundingBox->rect().width()/2)-(nameWidth/2));
    _nameItem->setY(topLeft.y()+10 - metrics.height()/2);
    
    QString persistentMessage = _persistentMessage->toPlainText();
    f.setPixelSize(25);
    metrics = QFontMetrics(f);
    int pMWidth = metrics.width(persistentMessage);
    
    _persistentMessage->setPos(topLeft.x() + (width/2) - (pMWidth/2), topLeft.y() + height/2 - metrics.height()/2);
    _stateIndicator->setRect(topLeft.x()-NATRON_STATE_INDICATOR_OFFSET,topLeft.y()-NATRON_STATE_INDICATOR_OFFSET,
                             width+NATRON_STATE_INDICATOR_OFFSET*2,height+NATRON_STATE_INDICATOR_OFFSET*2);
    if(_previewPixmap)
        _previewPixmap->setPos(topLeft.x() + width / 2 - NATRON_PREVIEW_WIDTH / 2,
                               topLeft.y() + height / 2 - NATRON_PREVIEW_HEIGHT / 2 + 10);

    refreshEdges();
    const std::list<boost::shared_ptr<Natron::Node> >& outputs = _internalNode->getOutputs();
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
        assert(*it);
        (*it)->doRefreshEdgesGUI();
    }
    refreshPosition(pos().x(), pos().y());
}



void NodeGui::refreshPosition(double x,double y){
    
    
    QRectF bbox = mapRectToScene(_boundingBox->rect());
    const std::list<boost::shared_ptr<Natron::Node> >& outputs = _internalNode->getOutputs();

    if (appPTR->getCurrentSettings()->isSnapToNodeEnabled()) {
        
        
        if (_magnecEnabled) {
            double dist = sqrt((x - _magnecStartingPos.x()) * (x - _magnecStartingPos.x()) +
                               (y - _magnecStartingPos.y()) * (y - _magnecStartingPos.y()));
            if (dist >= NATRON_MAGNETIC_GRID_RELEASE_DISTANCE) {
                _magnecEnabled = false;
            } else {
                return;
            }
        }
        
        
        QSize size = getSize();
        
        ///handle magnetic grid
        QPointF middlePos(x + size.width() / 2,y + size.height() / 2);
        for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
            boost::shared_ptr<NodeGui> inputSource = it->second->getSource();
            if (inputSource) {
                QSize inputSize = inputSource->getSize();
                QPointF inputScenePos = inputSource->pos();
                QPointF inputPos = inputScenePos + QPointF(inputSize.width() / 2,inputSize.height() / 2);
                QPointF mapped = mapFromParent(inputPos);
                if (!contains(mapped)) {
                    if ((inputPos.x() >= (middlePos.x() - NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) &&
                         inputPos.x() <= (middlePos.x() + NATRON_MAGNETIC_GRID_GRIP_TOLERANCE))) {
                        _magnecEnabled = true;
                        x = inputPos.x() - size.width() / 2;
                        _magnecStartingPos.setX(x);
                        _magnecStartingPos.setY(y);
                    } else if ((inputPos.y() >= (middlePos.y() - NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) &&
                                inputPos.y() <= (middlePos.y() + NATRON_MAGNETIC_GRID_GRIP_TOLERANCE))) {
                        _magnecEnabled = true;
                        _magnecStartingPos.setX(x);
                        y = inputPos.y() - size.height() / 2;
                        _magnecStartingPos.setY(y);
                    }
                }
            }
        }
        
        if (!_magnecEnabled) {
            ///check now the outputs
            for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
                boost::shared_ptr<NodeGui> node = _graph->getGui()->getApp()->getNodeGui(*it);
                assert(node);
                QSize outputSize = node->getSize();
                QPointF nodeScenePos = node->pos();
                QPointF outputPos = nodeScenePos  + QPointF(outputSize.width() / 2,outputSize.height() / 2);
                QPointF mapped = mapFromParent(outputPos);
                if (!contains(mapped)) {
                    if ((outputPos.x() >= (middlePos.x() - NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) &&
                         outputPos.x() <= (middlePos.x() + NATRON_MAGNETIC_GRID_GRIP_TOLERANCE))) {
                        _magnecEnabled = true;
                        x = outputPos.x() - size.width() / 2;
                        _magnecStartingPos.setX(x);
                        _magnecStartingPos.setY(y);
                    } else if ((outputPos.y() >= (middlePos.y() - NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) &&
                                outputPos.y() <= (middlePos.y() + NATRON_MAGNETIC_GRID_GRIP_TOLERANCE))) {
                        _magnecEnabled = true;
                        _magnecStartingPos.setX(x);
                        y = outputPos.y() - size.height() / 2;
                        _magnecStartingPos.setY(y);
                    }
                }
                
            }
            
        }
    }
    
    
    setPos(x, y);
    
    const std::list<boost::shared_ptr<NodeGui> >& allNodes = _graph->getAllActiveNodes();
    
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = allNodes.begin(); it!=allNodes.end(); ++it) {
        if (it->get() != this && (*it)->intersects(bbox)) {
            setAboveItem(it->get());
        }
    }
    
    refreshEdges();
    
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = outputs.begin(); it!=outputs.end(); ++it) {
        assert(*it);
        (*it)->doRefreshEdgesGUI();
    }
    emit positionChanged();
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
        boost::scoped_array<U32> buf(new U32[dataSize]);
        for (int i = 0; i < w*h ; ++i) {
            buf[i] = qRgba(0, 0, 0, 255);
        }
        _internalNode->makePreviewImage(time, w, h, buf.get());
        {
            QImage img(reinterpret_cast<const uchar*>(buf.get()), w, h, QImage::Format_ARGB32_Premultiplied);
            QPixmap prev_pixmap = QPixmap::fromImage(img);
            _previewPixmap->setPixmap(prev_pixmap);
        }
    }
}
void NodeGui::initializeInputs()
{
    
    ///Also refresh the output position
    if (_outputEdge) {
        _outputEdge->initLine();
    }
    
    ///The actual numbers of inputs of the internal node
    int inputnb = _internalNode->maximumInputs();
    
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
        if(emptyInputsCount == 1 && _internalNode->maximumInputs() > 1){
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
bool NodeGui::contains(const QPointF &point) const{
    return _boundingBox->contains(point);
}

bool NodeGui::intersects(const QRectF& rect) const
{
    QRectF mapped = mapRectFromScene(rect);
    return _boundingBox->rect().intersects(mapped);
}

QPainterPath NodeGui::shape() const
{
    return _boundingBox->shape();
    
}

QRectF NodeGui::boundingRect() const{
    return _boundingBox->boundingRect();
}
QRectF NodeGui::boundingRectWithEdges() const{
    QRectF ret;
    ret = ret.united(mapToScene(_boundingBox->boundingRect()).boundingRect());
    for (InputEdgesMap::const_iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        ret = ret.united(it->second->mapToScene(it->second->boundingRect()).boundingRect());
    }
    ret.setTopLeft(ret.topLeft() - QPointF(50,50));
    ret.setBottomRight(ret.bottomRight() + QPointF(50,50));
    return ret;
}

bool NodeGui::isNearby(QPointF &point){
    QPointF p = mapFromScene(point);
    QRectF r(_boundingBox->rect().x()-NATRON_EDGE_DROP_TOLERANCE,_boundingBox->rect().y()-NATRON_EDGE_DROP_TOLERANCE,
             _boundingBox->rect().width()+NATRON_EDGE_DROP_TOLERANCE,_boundingBox->rect().height()+NATRON_EDGE_DROP_TOLERANCE);
    return r.contains(p);
}

void NodeGui::setName(const QString& name_){
    onInternalNameChanged(name_);
    emit nameChanged(name_);
}

void NodeGui::onInternalNameChanged(const QString& s){
    _nameItem->setPlainText(s);
    QRectF rect = _boundingBox->boundingRect();
    updateShape(rect.width(), rect.height());
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



void NodeGui::setSelected(bool b){
    _selected = b;
    if (!_internalNode->isNodeDisabled()) {
        if (b) {
            _boundingBox->setBrush(*_selectedGradient);
        } else {
            if (_slaveMasterLink) {
                _boundingBox->setBrush(*_clonedGradient);
            } else {
                _boundingBox->setBrush(*_defaultGradient);
            }
        }
    }
    update();
    if(_settingsPanel){
        _settingsPanel->setSelected(b);
        _settingsPanel->update();
        if (b && isSettingsPanelVisible() && _internalNode->isRotoNode()) {
            _graph->getGui()->setRotoInterface(this);
        }
    }
}

void NodeGui::setSelectedGradient(const QLinearGradient& gradient){
    *_selectedGradient = gradient;
    if(_selected){
        _boundingBox->setBrush(*_selectedGradient);
    }
}

void NodeGui::setDefaultGradient(const QLinearGradient& gradient){
    *_defaultGradient = gradient;
    if(!_selected){
        _boundingBox->setBrush(*_defaultGradient);
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

void NodeGui::activate() {
    show();
    setActive(true);
    _graph->restoreFromTrash(this);
    _graph->getGui()->getCurveEditor()->addNode(_graph->getNodeGuiSharedPtr(this));
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
    if(_internalNode->pluginID() != "Viewer"){
        if(isSettingsPanelVisible()){
            setVisibleSettingsPanel(false);
        }
    }else{
        ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(_internalNode->getLiveInstance());
        _graph->getGui()->activateViewerTab(viewer);
    }
    
    if (_internalNode->isRotoNode()) {
        _graph->getGui()->setRotoInterface(this);
    }
    
    
    if(_internalNode->isOpenFXNode()){
        OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
        ofxNode->effectInstance()->beginInstanceEditAction();
    }
    
    if (_slaveMasterLink) {
        if (!_internalNode->getMasterNode()) {
            onSlaveStateChanged(false);
        } else {
            _slaveMasterLink->show();
        }
    }

    getNode()->getApp()->triggerAutoSave();
    getNode()->getApp()->checkViewersConnection();

}

void NodeGui::deactivate() {
    hide();
    setActive(false);
    _graph->moveToTrash(this);
    _graph->getGui()->getCurveEditor()->removeNode(_graph->getNodeGuiSharedPtr(this));
    for (NodeGui::InputEdgesMap::const_iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        _graph->scene()->removeItem(it->second);
        it->second->setActive(false);
        it->second->setSource(boost::shared_ptr<NodeGui>());
    }
    if (_outputEdge) {
        _graph->scene()->removeItem(_outputEdge);
        _outputEdge->setActive(false);
    }

    if (_slaveMasterLink) {
        _slaveMasterLink->hide();
    }
    
    if(_internalNode->pluginID() != "Viewer"){
        if(isSettingsPanelVisible()){
            setVisibleSettingsPanel(false);
        }
        
    }else{
        ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(_internalNode->getLiveInstance());
        _graph->getGui()->deactivateViewerTab(viewer);
    }
    
    if (_internalNode->isRotoNode()) {
        _graph->getGui()->removeRotoInterface(this, false);
    }
    
    
    if(_internalNode->isOpenFXNode()){
        OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
        ofxNode->effectInstance()->endInstanceEditAction();
    }
    
    getNode()->getApp()->triggerAutoSave();
    std::list<ViewerInstance* > viewers;
    getNode()->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it) {
            (*it)->updateTreeAndRender();
    }
}

void NodeGui::initializeKnobs(){
    if(_settingsPanel){
        _settingsPanel->initializeKnobs();
    }
}



void NodeGui::setVisibleSettingsPanel(bool b){
    if(_settingsPanel){
        _settingsPanel->setClosed(!b);
    }
}

bool NodeGui::isSettingsPanelVisible() const{
    if(_settingsPanel){
        return !_settingsPanel->isClosed();
    }else{
        return false;
    }
}


void NodeGui::onPersistentMessageChanged(int type,const QString& message){
    //keep type in synch with this enum:
    //enum MessageType{INFO_MESSAGE = 0,ERROR_MESSAGE = 1,WARNING_MESSAGE = 2,QUESTION_MESSAGE = 3};
    _persistentMessage->show();
    _stateIndicator->show();
    if(type == 1){
        _persistentMessage->setPlainText("ERROR");
        QColor errColor(128,0,0,255);
        _persistentMessage->setDefaultTextColor(errColor);
        _stateIndicator->setBrush(errColor);
        _lastPersistentMessageType = 1;
    }else if(type == 2){
        _persistentMessage->setPlainText("WARNING");
        QColor warColor(180,180,0,255);
        _persistentMessage->setDefaultTextColor(warColor);
        _stateIndicator->setBrush(warColor);
        _lastPersistentMessageType = 2;
    }else{
        return;
    }
    std::list<ViewerInstance* > viewers;
    _internalNode->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
        ViewerTab* tab = _graph->getGui()->getViewerTabForInstance(*it);
        ///the tab might not exist if the node is being deactivated following a tab close request by the user.

        if (tab) {
            tab->getViewer()->setPersistentMessage(type,message);
        }
    }
    QRectF rect = _boundingBox->rect();
    updateShape(rect.width(), rect.height());
}

void NodeGui::onPersistentMessageCleared(){
    _persistentMessage->hide();
    _stateIndicator->hide();
    
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
void NodeGui::showMenu(const QPoint& pos){
    populateMenu();
    _menu->exec(pos);
}

void NodeGui::populateMenu(){
    _menu->clear();
    
    QAction* copyAction = new QAction(tr("Copy"),_menu);
    copyAction->setShortcut(QKeySequence::Copy);
    QObject::connect(copyAction,SIGNAL(triggered()),this,SLOT(copyNode()));
    _menu->addAction(copyAction);
    
    QAction* cutAction = new QAction(tr("Cut"),_menu);
    cutAction->setShortcut(QKeySequence::Cut);
    QObject::connect(cutAction,SIGNAL(triggered()),this,SLOT(cutNode()));
    _menu->addAction(cutAction);
    
    QAction* duplicateAction = new QAction(tr("Duplicate"),_menu);
    duplicateAction->setShortcut(QKeySequence(Qt::AltModifier + Qt::Key_C));
    QObject::connect(duplicateAction,SIGNAL(triggered()),this,SLOT(duplicateNode()));
    _menu->addAction(duplicateAction);
    
    QAction* cloneAction = new QAction(tr("Clone"),_menu);
    cloneAction->setShortcut(QKeySequence(Qt::AltModifier + Qt::Key_K));
    QObject::connect(cloneAction,SIGNAL(triggered()),this,SLOT(cloneNode()));
    _menu->addAction(cloneAction);
    
    QAction* decloneAction = new QAction(tr("Declone"),_menu);
    decloneAction->setShortcut(QKeySequence(Qt::AltModifier + Qt::ShiftModifier + Qt::Key_K));
    QObject::connect(decloneAction,SIGNAL(triggered()),this,SLOT(decloneNode()));
    decloneAction->setEnabled(_internalNode->getMasterNode() != NULL);
    _menu->addAction(decloneAction);
    
    QAction* togglePreviewAction = new QAction("Toggle preview image",_menu);
    togglePreviewAction->setCheckable(true);
    togglePreviewAction->setChecked(_internalNode->isPreviewEnabled());
    QObject::connect(togglePreviewAction,SIGNAL(triggered()),this,SLOT(togglePreview()));
    _menu->addAction(togglePreviewAction);
    
    QAction* deleteAction = new QAction("Delete",_menu);
    QObject::connect(deleteAction,SIGNAL(triggered()),_graph,SLOT(deleteSelectedNode()));
    _menu->addAction(deleteAction);

}
const std::map<boost::shared_ptr<KnobI> ,KnobGui*>& NodeGui::getKnobs() const{
    assert(_settingsPanel);
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

void NodeGui::onSlaveStateChanged(bool b) {
    if (b) {
        boost::shared_ptr<Natron::Node> masterNode = _internalNode->getMasterNode();
        assert(masterNode);
        boost::shared_ptr<NodeGui> masterNodeGui = _graph->getGui()->getApp()->getNodeGui(masterNode);
        assert(masterNodeGui);
        _masterNodeGui = masterNodeGui;
        assert(!_slaveMasterLink);

        QObject::connect(_masterNodeGui.get(), SIGNAL(positionChanged()), this, SLOT(refreshSlaveMasterLinkPosition()));
        QObject::connect(this, SIGNAL(positionChanged()), this, SLOT(refreshSlaveMasterLinkPosition()));
        _slaveMasterLink = new QGraphicsLineItem(parentItem());
        _slaveMasterLink->setZValue(-1);
        QPen pen;
        pen.setWidth(3);
        pen.setBrush(QColor(200,100,100));
        _slaveMasterLink->setPen(pen);
        if (!_internalNode->isNodeDisabled()) {
            if (!isSelected()) {
                _boundingBox->setBrush(*_clonedGradient);
            }
        }
        
    } else {
        QObject::disconnect(_masterNodeGui.get(), SIGNAL(positionChanged()), this, SLOT(refreshSlaveMasterLinkPosition()));
        QObject::disconnect(this, SIGNAL(positionChanged()), this, SLOT(refreshSlaveMasterLinkPosition()));

        assert(_slaveMasterLink);
        delete _slaveMasterLink;
        _slaveMasterLink = 0;
        _masterNodeGui.reset();
        if (!_internalNode->isNodeDisabled()) {
            if (!isSelected()) {
                _boundingBox->setBrush(*_defaultGradient);
            }
        }
    }
    update();
}

void NodeGui::refreshSlaveMasterLinkPosition() {
    if (!_masterNodeGui || !_slaveMasterLink) {
        return;
    }
    
    QRectF bboxThisNode = boundingRect();
    QRectF bboxMasterNode = _masterNodeGui->boundingRect();

    QPointF dst = _slaveMasterLink->mapFromItem(_masterNodeGui.get(),QPointF(bboxMasterNode.x(),bboxMasterNode.y())
                              + QPointF(bboxMasterNode.width() / 2., bboxMasterNode.height() / 2.));
    QPointF src = _slaveMasterLink->mapFromItem(this,QPointF(bboxThisNode.x(),bboxThisNode.y())
                              + QPointF(bboxThisNode.width() / 2., bboxThisNode.height() / 2.));
    _slaveMasterLink->setLine(QLineF(src,dst));
}

void NodeGui::copyNode() {
    _graph->copyNode(_graph->getNodeGuiSharedPtr(this));
}

void NodeGui::cutNode() {
    _graph->cutNode(_graph->getNodeGuiSharedPtr(this));
}

void NodeGui::cloneNode() {
    _graph->cloneNode(_graph->getNodeGuiSharedPtr(this));
}

void NodeGui::decloneNode() {
    _graph->decloneNode(_graph->getNodeGuiSharedPtr(this));
}

void NodeGui::duplicateNode() {
    _graph->duplicateNode(_graph->getNodeGuiSharedPtr(this));
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
    if (_previewPixmap && _previewPixmap->isVisible()) {
        return QSize(NODE_WITH_PREVIEW_LENGTH,NODE_WITH_PREVIEW_HEIGHT);
    } else {
        return QSize(NODE_LENGTH,NODE_HEIGHT);
    }
}


void NodeGui::onDisabledKnobToggled(bool disabled) {
    if (disabled) {
        _boundingBox->setBrush(*_disabledGradient);
        _nameItem->setDefaultTextColor(QColor(120,120,120));
    } else {
        if (_slaveMasterLink) {
            _boundingBox->setBrush(*_clonedGradient);
        } else if (isSelected()) {
            _boundingBox->setBrush(*_selectedGradient);
        } else {
            _boundingBox->setBrush(*_defaultGradient);
        }
        _boundingBox->setBrush(*_defaultGradient);
        _nameItem->setDefaultTextColor(QColor(0,0,0));
    }
}