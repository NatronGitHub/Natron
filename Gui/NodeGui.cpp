//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */








#include "Gui/NodeGui.h"

#include <cassert>
#include <QLayout>
#include "Gui/Edge.h"
#include "Gui/SettingsPanel.h"
#include "Readers/Reader.h"
#include "Engine/Node.h"
#include "Gui/NodeGraph.h"
#include "Engine/OfxNode.h"
#include "Global/Controler.h"

const qreal pi=3.14159265358979323846264338327950288419717;
using namespace std;
NodeGui::NodeGui(NodeGraph* dag,QVBoxLayout *dockContainer,Node *node,qreal x, qreal y, QGraphicsItem *parent,QGraphicsScene* scene,QObject* parentObj) :QObject(parentObj), QGraphicsItem(parent),settings(0)
{
    
    _selected = false;
    this->_dag = dag;
    this->node=node;
	this->node->setNodeUi(this);
    this->sc=scene;
    
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
    
    setPos(x,y);
    QPointF itemPos = mapFromScene(QPointF(x,y));
	
	if(node->className() == string("Reader")){ // if the node is not a reader
		rectangle=scene->addRect(QRectF(itemPos,QSizeF(NODE_LENGTH+PREVIEW_LENGTH,NODE_HEIGHT+PREVIEW_HEIGHT)));
	}else{
		rectangle=scene->addRect(QRectF(itemPos,QSizeF(NODE_LENGTH,NODE_HEIGHT)));
	}
	
    rectangle->setParentItem(this);
    
    QImage img(IMAGES_PATH"RGBAchannels.png");
    
    
    QPixmap pixmap=QPixmap::fromImage(img);
    pixmap=pixmap.scaled(10,10);
    channels=scene->addPixmap(pixmap);
    channels->setX(itemPos.x()+1);
    channels->setY(itemPos.y()+1);
    channels->setParentItem(this);
    
    updateChannelsTooltip();
    
	
    name=scene->addSimpleText(node->getName().c_str());
	
	if(node->className() == string("Reader")){
		name->setX(itemPos.x()+35);
		name->setY(itemPos.y()+1);
	}else{
		name->setX(itemPos.x()+10);
		name->setY(itemPos.y()+channels->boundingRect().height()+5);
	}
    if(node->className() == string("Reader")){
		if(static_cast<Reader*>(node)->hasPreview()){
			QImage *prev=static_cast<Reader*>(node)->getPreview();
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
    
    name->setParentItem(this);
    initInputArrows();
    
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
    
}

NodeGui::~NodeGui(){
    
    foreach(Edge* e,inputs){
        if(e){
            QGraphicsScene* scene = e->getScene();
            if(scene){
                scene->removeItem(e);
            }
            e->setParentItem(NULL);
            delete e;
        }
    }
    
    foreach(NodeGui* p,parents){
        p->removeChild(this);
    }
    std::vector<NodeGui*> tmpChildrenCopy;
    foreach(NodeGui* c,children){
        tmpChildrenCopy.push_back(c);
        Edge* e = c->findConnectedEdge(this);
        if(e){
            e->removeSource();
        }
        c->removeParent(this);
    }
    node->removeThisFromChildren();
    node->removeThisFromParents();
    //    foreach(NodeGui*c,tmpChildrenCopy){
    //        _dag->checkIfViewerConnectedAndRefresh(c);
    //    }
    if(!node->isOpenFXNode())
        delete node;
    //    if(settings){
    //        delete settings;
    //        settings = 0;
    //    }
    
}

void NodeGui::markInputNull(Edge* e){
    for (U32 i = 0; i < inputs.size(); i++) {
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

void NodeGui::updateChannelsTooltip(){
    QString tooltip;
    ChannelSet chans= node->getInfo()->channels();
    tooltip.append("Channels in input: ");
    foreachChannels( z,chans){
        tooltip.append("\n");
        tooltip.append(getChannelName(z).c_str());
        
    }
    channels->setToolTip(tooltip);
}

void NodeGui::updatePreviewImageForReader(){
    Reader* reader = static_cast<Reader*>(node);
    /*the node must be a reader to call this function.*/
    if(!reader) return;
    
	QImage *prev = reader->getPreview();
	QPixmap prev_pixmap=QPixmap::fromImage(*prev);
	prev_pixmap=prev_pixmap.scaled(60,40);
    prev_pix->setPixmap(prev_pixmap);
    
}
void NodeGui::initInputArrows(){
    int i=0;
    int inputnb=node->maximumInputs();
    double piDividedbyX=(qreal)(pi/(qreal)(inputnb+1));
    double angle=pi-piDividedbyX;
    while(i<inputnb){
        Edge* edge=new Edge(i,angle,this,parentItem(),sc);
        inputs.push_back(edge);
        angle-=piDividedbyX;
        ++i;
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
    
    
    
    
    updateChannelsTooltip();
    
    
}
bool NodeGui::isNearby(QPointF &point){
    QRectF r(rectangle->rect().x()-10,rectangle->rect().y()-10,rectangle->rect().width()+10,rectangle->rect().height()+10);
    return r.contains(point);
}


void  NodeGui::removeChild(NodeGui* c){
	if(!children.empty()){
		for(U32 i=0;i<children.size();++i) {
            if(children[i] == c){
                children.erase(children.begin()+i);
                break;
            }
        }
	}
}
void  NodeGui::removeParent(NodeGui* p){
	if(!parents.empty()){
		for(U32 i=0;i<parents.size();++i) {
            if(parents[i] == p){
                parents.erase(parents.begin()+i);
                break;
            }
        }
	}
}


NodeGui* NodeGui::hasViewerConnected(NodeGui* node){
    NodeGui* out;
    bool ok=false;
    _hasViewerConnected(node,&ok,out);
    if (ok) {
        return out;
    }else{
        return NULL;
    }
    
}
void NodeGui::_hasViewerConnected(NodeGui* node,bool* ok,NodeGui*& out){
    if (*ok == true) {
        return;
    }
    if(node->getNode()->className() == string("Viewer")){
        out = node;
        *ok = true;
    }else{
        foreach(NodeGui* c,node->getChildren()){
            _hasViewerConnected(c,ok,out);
        }
    }
}

void NodeGui::setName(QString s){
    name->setText(s);
    node->setName(s.toStdString());
    if(settings)
        settings->setNodeName(s);
    sc->update();
}

Edge* NodeGui::firstAvailableEdge(){
    for (U32 i = 0 ; i < inputs.size(); ++i) {
        Edge* a = inputs[i];
        if (!a->hasSource()) {
            
            if(getNode()->isOpenFXNode()){
                OfxNode* ofxNode = dynamic_cast<OfxNode*>(getNode());
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
