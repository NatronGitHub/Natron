#ifndef OUTPUTNODE_UI_H
#define OUTPUTNODE_UI_H
#include "Gui/node_ui.h"
class OutputNode_ui : public Node_ui
{
public:
    OutputNode_ui(Controler* ctrl,std::vector<Node_ui*> nodes,QVBoxLayout* dockContainer,
                  Node* node,qreal x, qreal y,QGraphicsItem* parent=0,QGraphicsScene* sc=0);
    bool hasOutput(){return false;}
    void initInputArrows(){Node_ui::initInputArrows();}
    QRectF boundingRect() const{return Node_ui::boundingRect();}
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *parent){
        Node_ui::paint(painter,options,parent);
    }
};

#endif // OUTPUTNODE_UI_H
