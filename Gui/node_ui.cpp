//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "Gui/node_ui.h"
#include "Gui/arrowGUI.h"
#include "Gui/dockableSettings.h"
#include "Reader/Reader.h"
#include "Core/node.h"
#include "Superviser/controler.h"
#include <QtWidgets/QtWidgets>
int Node_ui::nodeNumber=0;
const qreal pi=3.14159265358979323846264338327950288419717;
Node_ui::Node_ui(Controler* ctrl,std::vector<Node_ui*> nodes,QVBoxLayout *dockContainer,Node *node,qreal x, qreal y, QGraphicsItem *parent,QGraphicsScene* scene,QObject* parentObj) : QGraphicsItem(parent),QObject(parentObj)
{
    
	this->ctrl = ctrl;
    this->graphNodes=nodes;
    this->node=node;
	this->node->setNodeUi(this);
    this->sc=scene;
    //setFlag(QGraphicsItem::ItemIsMovable);
    //setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
    number=nodeNumber;
    nodeNumber++;
	
	if(strcmp(node->className(),"Reader")==0){ // if the node is not a reader
		rectangle=scene->addRect(QRectF(mapFromScene(QPointF(x,y)),QSizeF(NODE_LENGTH+PREVIEW_LENGTH,NODE_HEIGHT+PREVIEW_HEIGHT)));	
	}else{
		rectangle=scene->addRect(QRectF(mapFromScene(QPointF(x,y)),QSizeF(NODE_LENGTH,NODE_HEIGHT)));
	}
	
    rectangle->setParentItem(this);

    QImage img(IMAGES_PATH"RGBAchannels.png");

    
    QPixmap pixmap=QPixmap::fromImage(img);
    pixmap=pixmap.scaled(10,10);
    channels=scene->addPixmap(pixmap);
    channels->setX(x+1);
    channels->setY(y+1);
    channels->setParentItem(this);



    QString tooltip;
    ChannelSet chans= node->getInfo()->channels();
    tooltip.append("Channels: ");
    foreachChannels( z,chans){
        tooltip.append("\n");
        tooltip.append(getChannelName(z));

    }

	
    channels->setToolTip(tooltip);
    name=scene->addSimpleText((node->getName()));
	
	if(strcmp(node->className(),"Reader")==0){
		name->setX(x+35);
		name->setY(y+1);
	}else{
		name->setX(x+10);
		name->setY(y+channels->boundingRect().height()+5);
	}
    if(strcmp(node->className(),"Reader")==0){
		if(static_cast<Reader*>(node)->hasPreview()){
			QImage *prev=static_cast<Reader*>(node)->getPreview();
			QPixmap prev_pixmap=QPixmap::fromImage(*prev);
			prev_pixmap=prev_pixmap.scaled(60,40);
			prev_pix=scene->addPixmap(prev_pixmap);
			prev_pix->setX(x+30);
			prev_pix->setY(y+20);
			prev_pix->setParentItem(this);
		}else{
			QImage prev(60,40,QImage::Format_ARGB32);
			prev.fill(Qt::black);
			QPixmap prev_pixmap=QPixmap::fromImage(prev);
			prev_pix=scene->addPixmap(prev_pixmap);
			prev_pix->setX(x+30);
			prev_pix->setY(y+20);
			prev_pix->setParentItem(this);
		}
		
	}
    
    name->setParentItem(this);
    initInputArrows();

    /*building settings panel*/
	if(strcmp(node->className(),"Viewer")){
		settingsPanel_displayed=true;
		this->dockContainer=dockContainer;
		settings=new SettingsPanel(this);

		dockContainer->addWidget(settings);
	}
  


    // needed for the layout to work correctly
    QWidget* pr=dockContainer->parentWidget();
    pr->setMinimumSize(dockContainer->sizeHint());

    scene->addItem(this);




}
void Node_ui::updatePreviewImageForReader(){
	QImage *prev=static_cast<Reader*>(node)->getPreview();
	QPixmap prev_pixmap=QPixmap::fromImage(*prev);
	prev_pixmap=prev_pixmap.scaled(60,40);
    // clear the previous pixmap first :
	//prev_pix=sc->addPixmap(prev_pixmap);
    prev_pix->setPixmap(prev_pixmap);
	prev_pix->setX(30);
	prev_pix->setY(20);
	prev_pix->setParentItem(this);
}
void Node_ui::initInputArrows(){
    int i=0;
    int inputnb=node->getInputsNb();
    double piDividedbyX=(qreal)(pi/(qreal)(inputnb+1));
    double angle=pi-piDividedbyX;
    while(i<inputnb){
        Arrow* edge=new Arrow(i,angle,this,0,sc);
        edge->setParentItem(this);
        inputs.push_back(edge);
        angle-=piDividedbyX;
        i++;
    }
}
bool Node_ui::contains(const QPointF &point) const{
    return rectangle->contains(point);
}

QPainterPath Node_ui::shape() const
 {
     return rectangle->shape();

 }

QRectF Node_ui::boundingRect() const{
    return rectangle->boundingRect();
}

void Node_ui::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *parent){

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
        QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
        gradient.setColorAt(0, QColor(224,224,224));
        gradient.setColorAt(1, QColor(142,142,142));
        painter->fillRect(rect.intersected(sceneRect), gradient);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(sceneRect);

    #if !defined(Q_OS_SYMBIAN) && !defined(Q_WS_MAEMO_5)
        // Text
        QRectF textRect(sceneRect.left() + 4, sceneRect.top() + 4,
                        sceneRect.width() - 4, sceneRect.height() - 4);


        QFont font = painter->font();
        font.setBold(true);
        font.setPointSize(14);
        painter->setFont(font);



    #endif

}
bool Node_ui::isNearby(QPointF &point){
    QRectF r(rectangle->rect().x()-10,rectangle->rect().y()-10,rectangle->rect().width()+10,rectangle->rect().height()+10);
    return r.contains(point);
}


void  Node_ui::substractChild(Node_ui* c){
	if(!children.empty()){
		for(int i=0;i<children.size();i++){
			Node_ui* node=children[i];
			if(node->getNode()->getName()==c->getNode()->getName()){
				Node_ui* tmp=node;
				children[i]=children[children.size()-1];
				children[children.size()-1]=tmp;
				children.pop_back();

				break;
			}
		}
	}
}
void  Node_ui::substractParent(Node_ui* p){
	if(!parents.empty()){
		for(int i=0;i<parents.size();i++){
			Node_ui* node=parents[i];
			if(node->getNode()->getName()==p->getNode()->getName()){
				Node_ui* tmp=node;
				parents[i]=parents[parents.size()-1];
				parents[parents.size()-1]=tmp;
				parents.pop_back();

				break;
			}
		}
	}
}


Node_ui* Node_ui::hasViewerConnected(Node_ui* node){
    Node_ui* out;
    bool ok=false;
    _hasViewerConnected(node,&ok,out);
    if (ok) {
        return out;
    }else{
        return NULL;
    }
    
}
void Node_ui::_hasViewerConnected(Node_ui* node,bool* ok,Node_ui*& out){
    if (*ok == true) {
        return;
    }
    if(!strcmp(node->getNode()->className(), "Viewer")){
        out = node;
        *ok = true;
    }else{
        foreach(Node_ui* c,node->getChildren()){
            _hasViewerConnected(c,ok,out);
        }
    }
}

void Node_ui::setName(QString s){
    name->setText(s);
    node->setName(s);
    sc->update();
}
