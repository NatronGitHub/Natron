#ifndef NODE_UI_H
#define NODE_UI_H


//#include <inputarrow.h>
#include "Superviser/powiterFn.h"
#include <QtCore/QRectF>
#include <QtGui/QGraphicsItem>

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
class Node_ui : public QObject,public QGraphicsItem
{
    Q_OBJECT

public:
    static int nodeNumber;

    Node_ui(Controler* ctrl,std::vector<Node_ui*> nodes,QVBoxLayout *dockContainer,Node *node,qreal x,qreal y , QGraphicsItem *parent=0,QGraphicsScene *sc=0,QObject* parentObj=0);


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
    void addChild(Node_ui* c){children.push_back(c);}
    void addParent(Node_ui* p){parents.push_back(p);}
    void substractChild(Node_ui* c);
	void substractParent(Node_ui* p);
    std::vector<Node_ui*> getParents(){return parents;}
    std::vector<Node_ui*> getChildren(){return children;}
    SettingsPanel* getSettingPanel(){return settings;}
    QVBoxLayout* getDockContainer(){return dockContainer;}
	void updatePreviewImageForReader();
    
    bool hasInputNodeConnected(Node_ui*& node);
    bool hasOutputNodeConnected(Node_ui*& node);
    
    Controler* getControler(){return ctrl;}
    
public slots:
    void setName(QString);
protected:


    std::vector<Node_ui*> children;
    std::vector<Node_ui*> parents;
    std::vector<Node_ui*> graphNodes;

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
    Controler* ctrl;
};

#endif // NODE_UI_H
