//  Powiter
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
#include <QLayout>
#include <QAction> 
#include <QtConcurrentRun>

#include "Gui/Edge.h"
#include "Gui/DockablePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"
#include "Gui/Gui.h"
#include "Gui/KnobGui.h"

#include "Readers/Reader.h"

#include "Engine/OfxNode.h"
#include "Engine/ViewerNode.h"
#include "Engine/Knob.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/ChannelSet.h"

#include "Global/AppManager.h"

using std::make_pair;

static const double pi=3.14159265358979323846264338327950288419717;

NodeGui::NodeGui(NodeGraph* dag,
                 QVBoxLayout *dockContainer_,
                 Node *node_,
                 qreal x, qreal y,
                 QGraphicsItem *parent)
: QObject()
, QGraphicsItem(parent)
, _dag(dag)
, node(node_)
, _selected(false)
, name(NULL)
, dockContainer(dockContainer_)
, rectangle(NULL)
, channels(NULL)
, prev_pix(NULL)
, inputs()
, settingsPanel_displayed(false)
, settings(0)
{

    assert(node_);
    QObject::connect(this, SIGNAL(nameChanged(QString)), node, SLOT(onGUINameChanged(QString)));
    QObject::connect(this, SIGNAL(nameChanged(QString)), this, SLOT(onLineEditNameChanged(QString)));
    QObject::connect(node, SIGNAL(nameChanged(QString)), this, SLOT(onInternalNameChanged(QString)));
    QObject::connect(node, SIGNAL(deleteWanted()), this, SLOT(deleteNode()));
    QObject::connect(node, SIGNAL(refreshEdgesGUI()),this,SLOT(refreshEdges()));
    QObject::connect(node, SIGNAL(knobsInitialized()),this,SLOT(initializeKnobs()));
    QObject::connect(node, SIGNAL(inputsInitialized()),this,SLOT(initializeInputs()));
    QObject::connect(node, SIGNAL(previewImageChanged(int)), this, SLOT(updatePreviewImage(int)));
    QObject::connect(node, SIGNAL(deactivated()),this,SLOT(deactivate()));
    QObject::connect(node, SIGNAL(activated()), this, SLOT(activate()));
    QObject::connect(node, SIGNAL(inputChanged(int)), this, SLOT(connectEdge(int)));
    
    /*Disabled for now*/
    //QObject::connect(node,SIGNAL(channelsChanged(ChannelSet)),this,SLOT(updateChannelsTooltip(ChannelSet)));
    
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
    
    setPos(x,y);
    QPointF itemPos = mapFromScene(QPointF(x,y));
	
	if(node->canMakePreviewImage()){ 
		rectangle = new QGraphicsRectItem(QRectF(itemPos,QSizeF(NodeGui::NODE_LENGTH+NodeGui::PREVIEW_LENGTH,NodeGui::NODE_HEIGHT+NodeGui::PREVIEW_HEIGHT)),this);
	}else{
		rectangle = new QGraphicsRectItem(QRectF(itemPos,QSizeF(NodeGui::NODE_LENGTH,NodeGui::NODE_HEIGHT)),this);
	}
	
    
    QImage img(POWITER_IMAGES_PATH"RGBAchannels.png");
    
    
    QPixmap pixmap=QPixmap::fromImage(img);
    pixmap=pixmap.scaled(10,10);
    channels= new QGraphicsPixmapItem(pixmap,this);
    channels->setX(itemPos.x()+1);
    channels->setY(itemPos.y()+1);
	
    name = new QGraphicsSimpleTextItem(node->getName().c_str(),this);
    if(node->canMakePreviewImage()){
        name->setX(itemPos.x()+35);
		name->setY(itemPos.y()+1);
        QImage prev(POWITER_PREVIEW_WIDTH, POWITER_PREVIEW_HEIGHT, QImage::Format_ARGB32);
        prev.fill(Qt::black);
        QPixmap prev_pixmap = QPixmap::fromImage(prev);
        prev_pix = new QGraphicsPixmapItem(prev_pixmap,this);
        prev_pix->setX(itemPos.x() + POWITER_PREVIEW_WIDTH/2);
        prev_pix->setY(itemPos.y() + POWITER_PREVIEW_HEIGHT/2);
	}else{
		name->setX(itemPos.x()+10);
		name->setY(itemPos.y()+channels->boundingRect().height()+5);
	}

    
    /*building settings panel*/
	if(node->className() != "Viewer"){
		settingsPanel_displayed=true;
		assert(dockContainer);
		settings=new NodeSettingsPanel(this,dockContainer->parentWidget());
		dockContainer->addWidget(settings);
        if(node->isOpenFXNode()){
            OfxNode* ofxNode = dynamic_cast<OfxNode*>(node);
            ofxNode->effectInstance()->beginInstanceEditAction();
        }
	}
    
}

void NodeGui::deleteNode(){
    delete this;
}

NodeGui::~NodeGui(){
    _dag->removeNode(this);
    for(InputEdgesMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
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
    if(!node->isOpenFXNode())
        delete node;
}
void NodeGui::refreshPosition(double x,double y){
    setPos(x, y);
    refreshEdges();
    for (Node::OutputMap::const_iterator it = node->getOutputs().begin(); it!=node->getOutputs().end(); ++it) {
        if(it->second){
            it->second->doRefreshEdgesGUI();
        }
    }
}
void NodeGui::refreshEdges(){
    for (NodeGui::InputEdgesMap::const_iterator i = inputs.begin(); i!= inputs.end(); ++i){
        const Node::InputMap& nodeInputs = node->getInputs();
        Node::InputMap::const_iterator it = nodeInputs.find(i->first);
        assert(it!=nodeInputs.end());
        NodeGui *nodeInputGui = _dag->getGui()->getApp()->getNodeGui(it->second);
        i->second->setSource(nodeInputGui);
        i->second->initLine();
    }
}


void NodeGui::markInputNull(Edge* e){
    for (U32 i = 0; i < inputs.size(); ++i) {
        if (inputs[i] == e) {
            inputs[i] = 0;
        }
    }
}



void NodeGui::updateChannelsTooltip(const Powiter::ChannelSet& chan){
    QString tooltip;
    tooltip.append("Channels in input: ");
    foreachChannels( z,chan){
        tooltip.append("\n");
        tooltip.append(Powiter::getChannelName(z).c_str());
        
    }
    channels->setToolTip(tooltip);
}

void NodeGui::updatePreviewImage(int time){
    if(node->canMakePreviewImage()){
        QtConcurrent::run(this,&NodeGui::computePreviewImage,time);
    }
}

void NodeGui::computePreviewImage(int time){
    int w = POWITER_PREVIEW_WIDTH;
    int h = POWITER_PREVIEW_HEIGHT;
    size_t dataSize = 4*w*h;
    U32* buf = (U32*)malloc(dataSize);
    for(int i = 0; i < w*h ; ++i ){ buf[i] = qRgba(0, 0, 0, 255); }
    node->makePreviewImage(time, w, h, buf);
    {
        QImage img((const uchar*)buf,w,h,QImage::Format_ARGB32_Premultiplied);
        QPixmap prev_pixmap = QPixmap::fromImage(img);
        prev_pix->setPixmap(prev_pixmap);
    }
    free(buf);
}
void NodeGui::initializeInputs(){
    int inputnb = node->maximumInputs();
    while ((int)inputs.size() > inputnb) {
        InputEdgesMap::iterator it = inputs.end();
        --it;
        delete it->second;
        inputs.erase(it);
    }
    for(int i = 0; i < inputnb;++i){
        if(inputs.find(i) == inputs.end()){
            Edge* edge = new Edge(i,0.,this,parentItem());
            inputs.insert(make_pair(i,edge));
        }
    }
    int emptyInputsCount = 0;
    for (InputEdgesMap::iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        if(!it->second->hasSource()){
            ++emptyInputsCount;
        }
    }
    /*if only 1 empty input, display it aside*/
    if(emptyInputsCount == 1 && node->maximumInputs() > 1){
        for (InputEdgesMap::iterator it = inputs.begin(); it!=inputs.end(); ++it) {
            if(!it->second->hasSource()){
                it->second->setAngle(pi);
                it->second->initLine();
                return;
            }
        }

    }
    
    double piDividedbyX = (double)(pi/(double)(emptyInputsCount+1));
    double angle = pi-piDividedbyX;
    for (InputEdgesMap::iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        if(!it->second->hasSource()){
            it->second->setAngle(angle);
            angle -= piDividedbyX;
            it->second->initLine();
        }
    }
    
}
bool NodeGui::contains(const QPointF &point) const{
    return rectangle->contains(point);
}

QPainterPath NodeGui::shape() const
{
    return rectangle->shape();
    
}

QRectF NodeGui::boundingRect() const{
    return rectangle->boundingRect();
}
QRectF NodeGui::boundingRectWithEdges() const{
    QRectF ret;
    ret = ret.united(mapToScene(rectangle->boundingRect()).boundingRect());
    for (InputEdgesMap::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        ret = ret.united(it->second->mapToScene(it->second->boundingRect()).boundingRect());
    }
    ret.setTopLeft(ret.topLeft() - QPointF(50,50));
    ret.setBottomRight(ret.bottomRight() + QPointF(50,50));
    return ret;
}

void NodeGui::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *parent){
    
    (void)parent;
    (void)options;
    // Shadow
    QRectF rect=boundingRect();
    QRectF sceneRect =boundingRect();//this->sceneRect();
    QRectF rightShadow(sceneRect.right(), sceneRect.top() + 5, 5, sceneRect.height());
    QRectF bottomShadow(sceneRect.left() + 5, sceneRect.bottom(), sceneRect.width(), 5);
    if (rightShadow.intersects(rect) || rightShadow.contains(rect))
        painter->fillRect(rightShadow, Qt::darkGray);
    if (bottomShadow.intersects(rect) || bottomShadow.contains(rect))
        painter->fillRect(bottomShadow, Qt::darkGray);
    
    // Fill
    if(_selected){
        QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
        gradient.setColorAt(0, QColor(249,187,81));
        gradient.setColorAt(1, QColor(150,187,81));
        painter->fillRect(rect.intersected(sceneRect), gradient);
    }else{
        QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
        gradient.setColorAt(0, QColor(224,224,224));
        gradient.setColorAt(1, QColor(142,142,142));
        painter->fillRect(rect.intersected(sceneRect), gradient);
        
    }
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sceneRect);
    // Text
    QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4,
                    sceneRect.width() - 4, sceneRect.height() - 4);
    
    
    QFont font = painter->font();
    font.setBold(true);
    font.setPointSize(14);
    painter->setFont(font);
}
bool NodeGui::isNearby(QPointF &point){
    QRectF r(rectangle->rect().x()-10,rectangle->rect().y()-10,rectangle->rect().width()+10,rectangle->rect().height()+10);
    return r.contains(point);
}

void NodeGui::setName(const QString& name_){
    onInternalNameChanged(name_);
    emit nameChanged(name_);
}
void NodeGui::onLineEditNameChanged(const QString& s){
    name->setText(s);
    scene()->update();
}
void NodeGui::onInternalNameChanged(const QString& s){
    name->setText(s);
    if(settings)
        settings->onNameChanged(s);
    scene()->update();
}

Edge* NodeGui::firstAvailableEdge(){
    for (U32 i = 0 ; i < inputs.size(); ++i) {
        Edge* a = inputs[i];
        if (!a->hasSource()) {
            if(node->isOpenFXNode()){
                OfxNode* ofxNode = dynamic_cast<OfxNode*>(node);
                if(ofxNode->isInputOptional(i))
                    continue;
            }
            return a;
        }
    }
    return NULL;
}



void NodeGui::setSelected(bool b){
    _selected = b;
    update();
    if(settings){
        settings->setSelected(b);
        settings->update();
    }
}

Edge* NodeGui::findConnectedEdge(NodeGui* parent){
    for (U32 i =0 ; i < inputs.size(); ++i) {
        Edge* e = inputs[i];
        
        if (e && e->getSource() == parent) {
            return e;
        }
    }
    return NULL;
}

bool NodeGui::connectEdge(int edgeNumber){
    Node::InputMap::const_iterator it = node->getInputs().find(edgeNumber);
    if(it == node->getInputs().end()){
        return false;
    }
    NodeGui* src = _dag->getGui()->getApp()->getNodeGui(it->second);
    InputEdgesMap::const_iterator it2 = inputs.find(edgeNumber);
    if(it2 == inputs.end()){
        return false;
    }else{
        it2->second->setSource(src);
        it2->second->initLine();
        return true;
    }
}



Edge* NodeGui::hasEdgeNearbyPoint(const QPointF& pt){
    for (NodeGui::InputEdgesMap::const_iterator i = inputs.begin(); i!= inputs.end(); ++i){
        if(i->second->contains(i->second->mapFromScene(pt))){
            return i->second;
        }
    }
    return NULL;
}

void NodeGui::activate(){
    show();
    setActive(true);
    _dag->restoreFromTrash(this);
    for (NodeGui::InputEdgesMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        _dag->scene()->addItem(it->second);
        it->second->setParentItem(this);
        it->second->setActive(true);
    }
    refreshEdges();
    for (Node::OutputMap::const_iterator it = node->getOutputs().begin(); it!=node->getOutputs().end(); ++it) {
        if(it->second){
            it->second->doRefreshEdgesGUI();
        }
    }
    if(node->className() != "Viewer"){
        if(isSettingsPanelVisible()){
            setVisibleSettingsPanel(false);
        }
    }else{
        ViewerNode* viewer = dynamic_cast<ViewerNode*>(node);
        _dag->getGui()->addViewerTab(viewer->getUiContext(), _dag->getGui()->_viewersPane);
        viewer->getUiContext()->show();
    }
    if(node->isOpenFXNode()){
        OfxNode* ofxNode = dynamic_cast<OfxNode*>(node);
        ofxNode->effectInstance()->beginInstanceEditAction();
    }
    
}

void NodeGui::deactivate(){
    hide();
    setActive(false);
    _dag->moveToTrash(this);
    for (NodeGui::InputEdgesMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        _dag->scene()->removeItem(it->second);
        it->second->setActive(false);
        it->second->setSource(NULL);
    }
   
    if(node->className() != "Viewer"){
        if(isSettingsPanelVisible()){
            setVisibleSettingsPanel(false);
        }
        
    }else{
        ViewerNode* viewer = dynamic_cast<ViewerNode*>(node);
        _dag->getGui()->removeViewerTab(viewer->getUiContext(), false,false);
        viewer->getUiContext()->hide();
    }
    if(node->isOpenFXNode()){
        OfxNode* ofxNode = dynamic_cast<OfxNode*>(node);
        ofxNode->effectInstance()->beginInstanceEditAction();
    }
    
}

void NodeGui::initializeKnobs(){
    if(settings){
        settings->initializeKnobs();
    }
}



void NodeGui::setVisibleSettingsPanel(bool b){
    if(settings){
        settings->setVisible(b);
    }
}

bool NodeGui::isSettingsPanelVisible() const{
    if(settings){
        return settings->isVisible();
    }else{
        return false;
    }
}

NodeGui::SerializedState::SerializedState(const NodeGui* n):_node(n){
    
    const std::vector<Knob*>& knobs = _node->getNode()->getKnobs();
    for (U32 i  = 0; i < knobs.size(); ++i) {
        std::string serializedValue = knobs[i]->serialize();
        if(!serializedValue.empty()){
            _knobsValues.insert(std::make_pair(knobs[i]->getDescription(), serializedValue));
        }
    }
    
    _name = _node->getNode()->getName();
    
    if(!n->getNode()->isOpenFXNode()){
        _className = _node->getNode()->className();
    }else{
        OfxNode* ofxNode = dynamic_cast<OfxNode*>(n->getNode());
        QString name = ofxNode->className().c_str();
        QStringList groups = ofxNode->getPluginGrouping();
        if (groups.size() >= 1) {
            name.append("  [");
            name.append(groups[0]);
            name.append("]");
        }
        _className = name.toStdString();
    }
    
    const Node::InputMap& inputs = _node->getNode()->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
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
