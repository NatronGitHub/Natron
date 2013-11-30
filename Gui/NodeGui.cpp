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

#include "Gui/Edge.h"
#include "Gui/DockablePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"
#include "Gui/Gui.h"
#include "Gui/KnobGui.h"
#include "Gui/ViewerGL.h"
#include "Gui/CurveEditor.h"

#include "Readers/Reader.h"

#include "Engine/OfxEffectInstance.h"
#include "Engine/ViewerInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/ChannelSet.h"

#include "Global/AppManager.h"

#define NATRON_STATE_INDICATOR_OFFSET 5

using std::make_pair;

static const double pi=3.14159265358979323846264338327950288419717;

NodeGui::NodeGui(NodeGraph* dag,
                 QVBoxLayout *dockContainer_,
                 Natron::Node *node_,
                 qreal x, qreal y,
                 QGraphicsItem *parent)
: QObject()
, QGraphicsItem(parent)
, _graph(dag)
, _internalNode(node_)
, _selected(false)
, _nameItem(NULL)
, _boundingBox(NULL)
, _channelsPixmap(NULL)
, _previewPixmap(NULL)
, _persistentMessage(NULL)
, _lastPersistentMessageType(0)
, _stateIndicator(NULL)
, _inputEdges()
, _panelDisplayed(false)
, _settingsPanel(0)
, _selectedGradient(NULL)
, _defaultGradient(NULL)
, _menu(new QMenu(dag))
{
    
    assert(node_);
    QObject::connect(this, SIGNAL(nameChanged(QString)), _internalNode, SLOT(onGUINameChanged(QString)));
    
    QObject::connect(_internalNode, SIGNAL(nameChanged(QString)), this, SLOT(onInternalNameChanged(QString)));
    QObject::connect(_internalNode, SIGNAL(deleteWanted()), this, SLOT(deleteNode()));
    QObject::connect(_internalNode, SIGNAL(refreshEdgesGUI()),this,SLOT(refreshEdges()));
    QObject::connect(_internalNode, SIGNAL(knobsInitialized()),this,SLOT(initializeKnobs()));
    QObject::connect(_internalNode, SIGNAL(inputsInitialized()),this,SLOT(initializeInputs()));
    QObject::connect(_internalNode, SIGNAL(previewImageChanged(int)), this, SLOT(updatePreviewImage(int)));
    QObject::connect(_internalNode, SIGNAL(deactivated()),this,SLOT(deactivate()));
    QObject::connect(_internalNode, SIGNAL(activated()), this, SLOT(activate()));
    QObject::connect(_internalNode, SIGNAL(inputChanged(int)), this, SLOT(connectEdge(int)));
    QObject::connect(_internalNode, SIGNAL(persistentMessageChanged(int,QString)), this, SLOT(onPersistentMessageChanged(int,QString)));
    QObject::connect(_internalNode, SIGNAL(persistentMessageCleared()), this, SLOT(onPersistentMessageCleared()));
    /*Disabled for now*/
    
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
    setPos(x,y);	

    _boundingBox = new QGraphicsRectItem(this);
    _boundingBox->setZValue(-1);
	
    QImage img(NATRON_IMAGES_PATH"RGBAchannels.png");
    QPixmap pixmap = QPixmap::fromImage(img);
    pixmap = pixmap.scaled(10,10);
    _channelsPixmap= new QGraphicsPixmapItem(pixmap,this);
	
    _nameItem = new QGraphicsTextItem(_internalNode->getName().c_str(),this);
    _nameItem->setDefaultTextColor(QColor(0,0,0,255));

    _persistentMessage = new QGraphicsTextItem("",this);
    _persistentMessage->setZValue(1);
    QFont f = _persistentMessage->font();
    f.setPixelSize(25);
    _persistentMessage->setFont(f);
    _persistentMessage->hide();
    
    _stateIndicator = new QGraphicsRectItem(this);
    _stateIndicator->setZValue(-5);
    _stateIndicator->hide();

    /*building settings panel*/
    if(_internalNode->pluginID() != "Viewer"){
        _panelDisplayed=true;
        assert(dockContainer_);
        _settingsPanel = new NodeSettingsPanel(this,dockContainer_,dockContainer_->parentWidget());
        QObject::connect(_settingsPanel,SIGNAL(nameChanged(QString)),this,SLOT(setName(QString)));
        dockContainer_->addWidget(_settingsPanel);
        if(_internalNode->isOpenFXNode()){
            OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
            ofxNode->effectInstance()->beginInstanceEditAction();
        }
	}

    if(_internalNode->makePreviewByDefault() && !_graph->areAllPreviewTurnedOff()){
        togglePreview();

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
    
    _boundingBox->setBrush(*_defaultGradient);
    
    populateMenu();
}

void NodeGui::togglePreview(){
    _internalNode->togglePreview();
    if(_internalNode->isPreviewEnabled()){
        if(!_previewPixmap){
            QImage prev(NATRON_PREVIEW_WIDTH, NATRON_PREVIEW_HEIGHT, QImage::Format_ARGB32);
            prev.fill(Qt::black);
            QPixmap prev_pixmap = QPixmap::fromImage(prev);
            _previewPixmap = new QGraphicsPixmapItem(prev_pixmap,this);
        }
        updateShape(NODE_LENGTH+PREVIEW_LENGTH,NODE_HEIGHT+PREVIEW_HEIGHT);
        _previewPixmap->show();
    }else{
        _previewPixmap->hide();
        updateShape(NODE_LENGTH,NODE_HEIGHT);
    }
}

void NodeGui::deleteNode(){
    delete this;
}

NodeGui::~NodeGui(){
    _graph->removeNode(this);
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
    delete _selectedGradient;
    delete _defaultGradient;
}

void NodeGui::updateShape(int width,int height){
    QPointF topLeft = mapFromParent(pos());
    _boundingBox->setRect(topLeft.x(),topLeft.y(),width,height);
    _channelsPixmap->setPos(topLeft.x()+1,topLeft.y()+1);
    
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
        _previewPixmap->setPos(topLeft.x() + NATRON_PREVIEW_WIDTH/2,topLeft.y() + NATRON_PREVIEW_HEIGHT/2);

    refreshEdges();
    refreshPosition(pos().x(), pos().y());
}

void NodeGui::refreshPosition(double x,double y){
    setPos(x, y);
    refreshEdges();
    for (Natron::Node::OutputMap::const_iterator it = _internalNode->getOutputs().begin(); it!=_internalNode->getOutputs().end(); ++it) {
        if(it->second){
            it->second->doRefreshEdgesGUI();
        }
    }
}
void NodeGui::refreshEdges(){
    for (NodeGui::InputEdgesMap::const_iterator i = _inputEdges.begin(); i!= _inputEdges.end(); ++i){
        const Natron::Node::InputMap& nodeInputs = _internalNode->getInputs();
        Natron::Node::InputMap::const_iterator it = nodeInputs.find(i->first);
        assert(it!=nodeInputs.end());
        NodeGui *nodeInputGui = _graph->getGui()->getApp()->getNodeGui(it->second);
        i->second->setSource(nodeInputGui);
        i->second->initLine();
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
    tooltip.append("Channels in input: ");
    foreachChannels( z,chan){
        tooltip.append("\n");
        tooltip.append(Natron::getChannelName(z).c_str());
        
    }
    _channelsPixmap->setToolTip(tooltip);
}

void NodeGui::updatePreviewImage(int time){
    QtConcurrent::run(this,&NodeGui::computePreviewImage,time);
}

void NodeGui::computePreviewImage(int time){
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
void NodeGui::initializeInputs(){
    int inputnb = _internalNode->maximumInputs();
    while ((int)_inputEdges.size() > inputnb) {
        InputEdgesMap::iterator it = _inputEdges.end();
        --it;
        delete it->second;
        _inputEdges.erase(it);
    }
    for(int i = 0; i < inputnb;++i){
        if(_inputEdges.find(i) == _inputEdges.end()){
            Edge* edge = new Edge(i,0.,this,parentItem());
            _inputEdges.insert(make_pair(i,edge));
        }
    }
    int emptyInputsCount = 0;
    for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        if(!it->second->hasSource()){
            ++emptyInputsCount;
        }
    }
    /*if only 1 empty input, display it aside*/
    if(emptyInputsCount == 1 && _internalNode->maximumInputs() > 1){
        for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
            if(!it->second->hasSource()){
                it->second->setAngle(pi);
                it->second->initLine();
                return;
            }
        }

    }
    
    double piDividedbyX = (double)(pi/(double)(emptyInputsCount+1));
    double angle = pi-piDividedbyX;
    for (InputEdgesMap::iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        if(!it->second->hasSource()){
            it->second->setAngle(angle);
            angle -= piDividedbyX;
            it->second->initLine();
        }
    }
    
}
bool NodeGui::contains(const QPointF &point) const{
    return _boundingBox->contains(point);
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
    QRectF r(_boundingBox->rect().x()-10,_boundingBox->rect().y()-10,_boundingBox->rect().width()+10,_boundingBox->rect().height()+10);
    return r.contains(point);
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
    if(b){
        _boundingBox->setBrush(*_selectedGradient);
    }else{
        _boundingBox->setBrush(*_defaultGradient);
    }
    update();
    if(_settingsPanel){
        _settingsPanel->setSelected(b);
        _settingsPanel->update();
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
        
        if (e && e->getSource() == parent) {
            return e;
        }
    }
    return NULL;
}

bool NodeGui::connectEdge(int edgeNumber){
    Natron::Node::InputMap::const_iterator it = _internalNode->getInputs().find(edgeNumber);
    if(it == _internalNode->getInputs().end()){
        return false;
    }
    NodeGui* src = _graph->getGui()->getApp()->getNodeGui(it->second);
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
    return NULL;
}

void NodeGui::activate(){
    show();
    setActive(true);
    _graph->restoreFromTrash(this);
    _graph->getGui()->_curveEditor->addNode(this);
    for (NodeGui::InputEdgesMap::const_iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        _graph->scene()->addItem(it->second);
        it->second->setParentItem(this);
        it->second->setActive(true);
    }
    refreshEdges();
    for (Natron::Node::OutputMap::const_iterator it = _internalNode->getOutputs().begin(); it!=_internalNode->getOutputs().end(); ++it) {
        if(it->second){
            it->second->doRefreshEdgesGUI();
        }
    }
    if(_internalNode->pluginID() != "Viewer"){
        if(isSettingsPanelVisible()){
            setVisibleSettingsPanel(false);
        }
    }else{
        ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(_internalNode->getLiveInstance());
        _graph->getGui()->addViewerTab(viewer->getUiContext(), _graph->getGui()->_viewersPane);
        viewer->getUiContext()->show();
    }
    if(_internalNode->isOpenFXNode()){
        OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
        ofxNode->effectInstance()->beginInstanceEditAction();
    }
    
}

void NodeGui::deactivate(){
    hide();
    setActive(false);
    _graph->moveToTrash(this);
    _graph->getGui()->_curveEditor->removeNode(this);
    for (NodeGui::InputEdgesMap::const_iterator it = _inputEdges.begin(); it!=_inputEdges.end(); ++it) {
        _graph->scene()->removeItem(it->second);
        it->second->setActive(false);
        it->second->setSource(NULL);
    }
   
    if(_internalNode->pluginID() != "Viewer"){
        if(isSettingsPanelVisible()){
            setVisibleSettingsPanel(false);
        }
        
    }else{
        ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(_internalNode->getLiveInstance());
        _graph->getGui()->removeViewerTab(viewer->getUiContext(), false,false);
        viewer->getUiContext()->hide();
    }
    if(_internalNode->isOpenFXNode()){
        OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>(_internalNode->getLiveInstance());
        ofxNode->effectInstance()->endInstanceEditAction();
    }
    
}

void NodeGui::initializeKnobs(){
    if(_settingsPanel){
        _settingsPanel->initializeKnobs();
    }
}



void NodeGui::setVisibleSettingsPanel(bool b){
    if(_settingsPanel){
        _settingsPanel->setVisible(b);
    }
}

bool NodeGui::isSettingsPanelVisible() const{
    if(_settingsPanel){
        return _settingsPanel->isVisible();
    }else{
        return false;
    }
}

NodeGui::SerializedState::SerializedState(const NodeGui* n):_node(n){
    
    const std::vector<boost::shared_ptr<Knob> >& knobs = _node->getNode()->getKnobs();
    
    for (U32 i  = 0; i < knobs.size(); ++i) {
        if(knobs[i]->isPersistent()){
            _knobsValues.insert(std::make_pair(knobs[i]->getDescription(),dynamic_cast<AnimatingParam&>(*knobs[i].get())));
        }
    }
    
    _name = _node->getNode()->getName();
     
    _className = _node->getNode()->pluginID();
   
    const Natron::Node::InputMap& inputs = _node->getNode()->getInputs();
    for(Natron::Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second){
            _inputs.insert(std::make_pair(it->first, it->second->getName()));
        }else{
             _inputs.insert(std::make_pair(it->first, ""));
        }
    }
    
    QPointF pos = _node->pos();
    _posX = pos.x();
    _posY = pos.y();
}

NodeGui::SerializedState NodeGui::serialize() const{
    return NodeGui::SerializedState(this);
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
    std::list<ViewerInstance*> viewers;
    _internalNode->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->getUiContext()->viewer->setPersistentMessage(type,message);
    }
    QRectF rect = _boundingBox->rect();
    updateShape(rect.width(), rect.height());
}

void NodeGui::onPersistentMessageCleared(){
    _persistentMessage->hide();
    _stateIndicator->hide();
    
    std::list<ViewerInstance*> viewers;
    _internalNode->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->getUiContext()->viewer->clearPersistentMessage();
    }
}

QVBoxLayout* NodeGui::getDockContainer() const {
    return _settingsPanel->getContainer();
}

void NodeGui::paint(QPainter* /*painter*/,const QStyleOptionGraphicsItem* /*options*/,QWidget* /*parent*/){
    //nothing special
}
void NodeGui::showMenu(const QPoint& pos){
    _menu->exec(pos);
}

void NodeGui::populateMenu(){
    _menu->clear();
    QAction* togglePreviewAction = new QAction("Toggle preview image",this);
    togglePreviewAction->setCheckable(true);
    togglePreviewAction->setChecked(_internalNode->makePreviewByDefault());
    QObject::connect(togglePreviewAction,SIGNAL(triggered()),this,SLOT(togglePreview()));
    _menu->addAction(togglePreviewAction);
    
    QAction* deleteAction = new QAction("Delete",this);
    QObject::connect(deleteAction,SIGNAL(triggered()),_graph,SLOT(deleteSelectedNode()));
    _menu->addAction(deleteAction);

}
const std::map<Knob*,KnobGui*>& NodeGui::getKnobs() const{
    return _settingsPanel->getKnobs();
}
