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
int NodeGui::nodeNumber=0;
const qreal pi=3.14159265358979323846264338327950288419717;
NodeGui::NodeGui(Controler* ctrl,std::vector<NodeGui*> nodes,QVBoxLayout *dockContainer,Node *node,qreal x, qreal y, QGraphicsItem *parent,QGraphicsScene* scene,QObject* parentObj) : QGraphicsItem(parent),QObject(parentObj)
{
    
	this->ctrl = ctrl;
    this->graphNodes=nodes;
    this->node=node;
	this->node->setNodeUi(this);
    this->sc=scene;
    
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
    number=nodeNumber;
    nodeNumber++;
	
	if(node->className() == string("Reader")){ // if the node is not a reader
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
        tooltip.append(getChannelName(z).c_str());

    }

	
    channels->setToolTip(tooltip);
    name=scene->addSimpleText((node->getName()));
	
	if(node->className() == string("Reader")){
		name->setX(x+35);
		name->setY(y+1);
	}else{
		name->setX(x+10);
		name->setY(y+channels->boundingRect().height()+5);
	}
    if(node->className() == string("Reader")){
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
	if(node->className() != string("Viewer")){
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
void NodeGui::updatePreviewImageForReader(){
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
void NodeGui::initInputArrows(){
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
bool NodeGui::isNearby(QPointF &point){
    QRectF r(rectangle->rect().x()-10,rectangle->rect().y()-10,rectangle->rect().width()+10,rectangle->rect().height()+10);
    return r.contains(point);
}


void  NodeGui::substractChild(NodeGui* c){
	if(!children.empty()){
		for(U32 i=0;i<children.size();i++){
			NodeGui* node=children[i];
			if(node->getNode()->getName()==c->getNode()->getName()){
				NodeGui* tmp=node;
				children[i]=children[children.size()-1];
				children[children.size()-1]=tmp;
				children.pop_back();

				break;
			}
		}
	}
}
void  NodeGui::substractParent(NodeGui* p){
	if(!parents.empty()){
		for(U32 i=0;i<parents.size();i++){
			NodeGui* node=parents[i];
			if(node->getNode()->getName()==p->getNode()->getName()){
				NodeGui* tmp=node;
				parents[i]=parents[parents.size()-1];
				parents[parents.size()-1]=tmp;
				parents.pop_back();

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
    node->setName(s);
    sc->update();
}
