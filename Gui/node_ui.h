//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef NODE_UI_H
#define NODE_UI_H


//#include <inputarrow.h>
#include "Superviser/powiterFn.h"
#include <QtCore/QRectF>
#include <QtWidgets/QGraphicsItem>

static const int NODE_LENGTH=80;
static const int NODE_HEIGHT=30;
static const int PREVIEW_LENGTH=40;
static const int PREVIEW_HEIGHT=40;
class Arrow;
class QPainterPath;
class QScrollArea;
class SettingsPanel;
class QVBoxLayout;
class Node;
class Controler;
class NodeGui : public QObject,public QGraphicsItem
{
    Q_OBJECT

public:
    static int nodeNumber;

    NodeGui(std::vector<NodeGui*> nodes,QVBoxLayout *dockContainer,Node *node,qreal x,qreal y , QGraphicsItem *parent=0,QGraphicsScene *sc=0,QObject* parentObj=0);


    int getNumber(){return number;}
    Node* getNode(){return node;}
    virtual bool hasOutput()=0;
    virtual void initInputArrows();
    virtual bool contains(const QPointF &point) const;
    virtual QPainterPath shape() const;
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter,const QStyleOptionGraphicsItem* options,QWidget* parent);
    std::vector<Arrow*> getInputsArrows(){return inputs;}
    bool isNearby(QPointF &point);
    QVBoxLayout* getSettingsLayout(){return dockContainer;}
    bool isThisPanelEnabled(){return settingsPanel_displayed;}
    void setSettingsPanelEnabled(bool enabled){settingsPanel_displayed=enabled;}
    void addChild(NodeGui* c){children.push_back(c);}
    void addParent(NodeGui* p){parents.push_back(p);}
    void substractChild(NodeGui* c);
	void substractParent(NodeGui* p);
    std::vector<NodeGui*> getParents(){return parents;}
    std::vector<NodeGui*> getChildren(){return children;}
    SettingsPanel* getSettingPanel(){return settings;}
    QVBoxLayout* getDockContainer(){return dockContainer;}
	void updatePreviewImageForReader();
    void updateChannelsTooltip();
    
    static NodeGui* hasViewerConnected(NodeGui* node);
    static void _hasViewerConnected(NodeGui* node,bool* ok,NodeGui*& out);
    
    
public slots:
    void setName(QString);
protected:


    std::vector<NodeGui*> children;
    std::vector<NodeGui*> parents;
    std::vector<NodeGui*> graphNodes;

    QVBoxLayout* dockContainer;
    int number;
    QGraphicsScene* sc;
    QGraphicsRectItem* rectangle;
    QGraphicsPixmapItem* channels;
	QGraphicsPixmapItem* prev_pix;
    Node* node;
    std::vector<Arrow*> inputs;
    QGraphicsSimpleTextItem *name;
    bool settingsPanel_displayed;
    SettingsPanel* settings;

};

#endif // NODE_UI_H
