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
#include <QUndoStack>
#include <QUndoCommand>

#include "Gui/Edge.h"
#include "Gui/SettingsPanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"
#include "Gui/Gui.h"

#include "Readers/Reader.h"

#include "Engine/OfxNode.h"
#include "Engine/ViewerNode.h"

#include "Global/AppManager.h"
#include "Global/NodeInstance.h"


using namespace std;

static const qreal pi=3.14159265358979323846264338327950288419717;

NodeGui::NodeGui(NodeGraph* dag,
                 QVBoxLayout *dockContainer,
                 NodeInstance *node,
                 qreal x, qreal y,
                 QGraphicsItem *parent,
                 QGraphicsScene* scene,
                 QObject* parentObj):
QObject(parentObj),
QGraphicsItem(parent),
_dag(dag),
node(node),
_selected(false),
sc(scene),
settings(0)
{
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
    
    setPos(x,y);
    QPointF itemPos = mapFromScene(QPointF(x,y));
	
	if(hasPreviewImage()){ 
		rectangle=scene->addRect(QRectF(itemPos,QSizeF(NodeGui::NODE_LENGTH+NodeGui::PREVIEW_LENGTH,NodeGui::NODE_HEIGHT+NodeGui::PREVIEW_HEIGHT)));
	}else{
		rectangle=scene->addRect(QRectF(itemPos,QSizeF(NodeGui::NODE_LENGTH,NodeGui::NODE_HEIGHT)));
	}
	
    rectangle->setParentItem(this);
    
    QImage img(POWITER_IMAGES_PATH"RGBAchannels.png");
    
    
    QPixmap pixmap=QPixmap::fromImage(img);
    pixmap=pixmap.scaled(10,10);
    channels=scene->addPixmap(pixmap);
    channels->setX(itemPos.x()+1);
    channels->setY(itemPos.y()+1);
    channels->setParentItem(this);
	
    name=scene->addSimpleText(node->getName().c_str());
	
	if(hasPreviewImage()){
		name->setX(itemPos.x()+35);
		name->setY(itemPos.y()+1);
	}else{
		name->setX(itemPos.x()+10);
		name->setY(itemPos.y()+channels->boundingRect().height()+5);
	}
    if(int ret = hasPreviewImage()){
        if(ret == 1){
            Reader* n = dynamic_cast<Reader*>(node);
            if(n->hasPreview()){
                QImage *prev=n->getPreview();
                QPixmap prev_pixmap=QPixmap::fromImage(*prev);
                prev_pixmap=prev_pixmap.scaled(60,40);
                prev_pix=scene->addPixmap(prev_pixmap);
                prev_pix->setX(itemPos.x()+30);
                prev_pix->setY(itemPos.y()+20);
                prev_pix->setParentItem(this);
            }else{
                QImage prev(60,40,QImage::Format_ARGB32);
                prev.fill(Qt::black);
                QPixmap prev_pixmap=QPixmap::fromImage(prev);
                prev_pix=scene->addPixmap(prev_pixmap);
                prev_pix->setX(itemPos.x()+30);
                prev_pix->setY(itemPos.y()+20);
                prev_pix->setParentItem(this);
            }
            
        }else if(ret == 2){
            OfxNode* n = dynamic_cast<OfxNode*>(node);
            if(n->hasPreviewImage()){
                QImage *prev=n->getPreview();
                QPixmap prev_pixmap=QPixmap::fromImage(*prev);
                prev_pixmap=prev_pixmap.scaled(60,40);
                prev_pix=scene->addPixmap(prev_pixmap);
                prev_pix->setX(itemPos.x()+30);
                prev_pix->setY(itemPos.y()+20);
                prev_pix->setParentItem(this);
            }else{
                QImage prev(60,40,QImage::Format_ARGB32);
                prev.fill(Qt::black);
                QPixmap prev_pixmap=QPixmap::fromImage(prev);
                prev_pix=scene->addPixmap(prev_pixmap);
                prev_pix->setX(itemPos.x()+30);
                prev_pix->setY(itemPos.y()+20);
                prev_pix->setParentItem(this);
            }

        }
        
	}
    
    name->setParentItem(this);
    
    /*building settings panel*/
	if(node->className() != "Viewer"){
		settingsPanel_displayed=true;
		this->dockContainer=dockContainer;
		settings=new SettingsPanel(this,dag);
        
		dockContainer->addWidget(settings);
	}
    
    // needed for the layout to work correctly
    //  QWidget* pr=dockContainer->parentWidget();
    //  pr->setMinimumSize(dockContainer->sizeHint());
    _undoStack = new QUndoStack;
    
}

NodeGui::~NodeGui(){
    _dag->removeNode(this);
    for(InputEdgesMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        Edge* e = it->second;
        if(e){
            QGraphicsScene* scene = e->getScene();
            if(scene){
                scene->removeItem(e);
            }
            e->setParentItem(NULL);
            delete e;
        }
    }
    delete _undoStack;

}
int NodeGui::hasPreviewImage(){
    if (node->className() == "Reader") {
        return 1;
    }
    if(node->isOpenFXNode()){
        OfxNode* n = dynamic_cast<OfxNode*>(node);
        if(n->canHavePreviewImage())
            return 2;
        else
            return 0;
    }else{
        return 0;
    }
}

void NodeGui::pushUndoCommand(QUndoCommand* command){
    _undoStack->push(command);
    if(settings){
        settings->setEnabledUndoButton(_undoStack->canUndo());
        settings->setEnabledRedoButton(_undoStack->canRedo());
    }
}
void NodeGui::undoCommand(){
    _undoStack->undo();
    if(settings){
        settings->setEnabledUndoButton(_undoStack->canUndo());
        settings->setEnabledRedoButton(_undoStack->canRedo());
    }
}
void NodeGui::redoCommand(){
    _undoStack->redo();
    if(settings){
        settings->setEnabledUndoButton(_undoStack->canUndo());
        settings->setEnabledRedoButton(_undoStack->canRedo());
    }
}

void NodeGui::markInputNull(Edge* e){
    for (U32 i = 0; i < inputs.size(); ++i) {
        if (inputs[i] == e) {
            inputs[i] = 0;
        }
    }
}

void NodeGui::remove(){
    //dockContainer->removeWidget(settings);
    delete settings;
    delete this;
}

void NodeGui::updateChannelsTooltip(const ChannelSet& chan){
    QString tooltip;
    tooltip.append("Channels in input: ");
    foreachChannels( z,chan){
        tooltip.append("\n");
        tooltip.append(getChannelName(z).c_str());
        
    }
    channels->setToolTip(tooltip);
}

void NodeGui::updatePreviewImageForReader(){
    if(int ret = hasPreviewImage()){
        if(ret == 1){
            Reader* reader = dynamic_cast<Reader*>(node);
            QImage *prev = reader->getPreview();
            QPixmap prev_pixmap=QPixmap::fromImage(*prev);
            prev_pixmap=prev_pixmap.scaled(60,40);
            prev_pix->setPixmap(prev_pixmap);
        }else if(ret == 2){
            OfxNode* n = dynamic_cast<OfxNode*>(node);
            if(n->hasPreviewImage()){
                QImage *prev = n->getPreview();
                QPixmap prev_pixmap=QPixmap::fromImage(*prev);
                prev_pixmap=prev_pixmap.scaled(60,40);
                prev_pix->setPixmap(prev_pixmap);
            }
            
        }
    }
}
void NodeGui::initializeInputs(){
    int inputnb = node->getNode()->maximumInputs();
    double piDividedbyX = (double)(pi/(double)(inputnb+1));
    double angle = pi-piDividedbyX;
    for(int i = 0; i < inputnb;++i){
        Edge* edge = new Edge(i,angle,this,parentItem(),sc);
        inputs.insert(make_pair(i,edge));
        angle -= piDividedbyX;
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
    
    
    
    
    updateChannelsTooltip(node->getNode()->info().channels());
    
    
}
bool NodeGui::isNearby(QPointF &point){
    QRectF r(rectangle->rect().x()-10,rectangle->rect().y()-10,rectangle->rect().width()+10,rectangle->rect().height()+10);
    return r.contains(point);
}

void NodeGui::setName(QString s){
    name->setText(s);
    if(settings)
        settings->setNodeName(s);
    sc->update();
}

Edge* NodeGui::firstAvailableEdge(){
    for (U32 i = 0 ; i < inputs.size(); ++i) {
        Edge* a = inputs[i];
        if (!a->hasSource()) {
            
            if(getNodeInstance()->isOpenFXNode()){
                OfxNode* ofxNode = dynamic_cast<OfxNode*>(getNodeInstance()->getNode());
                if(ofxNode->isInputOptional(i))
                    continue;
            }
            return a;
        }
    }
    return NULL;
}

void NodeGui::putSettingsPanelFirst(){
    dockContainer->removeWidget(settings);
    dockContainer->insertWidget(0, settings);
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

bool NodeGui::connectEdge(int edgeNumber,NodeGui* src){
    assert(src);
    InputEdgesMap::const_iterator it = inputs.find(edgeNumber);
    if(it == inputs.end()){
        return false;
    }else{
        it->second->setSource(src);
        it->second->initLine();
    }
}


void NodeGui::refreshEdges(){
    for (NodeGui::InputEdgesMap::const_iterator i = inputs.begin(); i!= inputs.end(); ++i){
        i->second->initLine();
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
    sc->addItem(this);
    setActive(true);
    _dag->restoreFromTrash(this);
    for (NodeGui::InputEdgesMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        _dag->scene()->addItem(it->second);
        it->second->setActive(true);
    }
    if(node->className() != "Viewer"){
        if(isThisPanelEnabled()){
            setSettingsPanelEnabled(false);
            getSettingPanel()->setVisible(true);
        }
    }else{
        ViewerNode* viewer = dynamic_cast<ViewerNode*>(node->getNode());
        _dag->getGui()->addViewerTab(viewer->getUiContext(), _dag->getGui()->_viewersPane);
        viewer->getUiContext()->show();
    }
}

void NodeGui::deactivate(){
    sc->removeItem(this);
    setActive(false);
    _dag->moveToTrash(this);
    for (NodeGui::InputEdgesMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        _dag->scene()->removeItem(it->second);
        it->second->setActive(false);
    }
    if(node->className() != "Viewer"){
        if(isThisPanelEnabled()){
            setSettingsPanelEnabled(false);
            getSettingPanel()->close();
        }
        
    }else{
        ViewerNode* viewer = dynamic_cast<ViewerNode*>(node->getNode());
        _dag->getGui()->removeViewerTab(viewer->getUiContext(), false,false);
        viewer->getUiContext()->hide();
    }
}

void NodeGui::initializeKnobs(){
    if(settings){
        settings->initialize_knobs();
    }
}