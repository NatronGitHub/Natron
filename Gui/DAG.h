#ifndef NODEGRAPH_H
#define NODEGRAPH_H
#include <cmath>
#include <QtGui/QGraphicsView>
#include <iostream>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QInputDialog>
#include "Gui/outputnode_ui.h"
#include "Gui/inputnode_ui.h"
#include "Gui/operatornode_ui.h"
#include "Superviser/powiterFn.h"
using namespace Powiter_Enums;
class Controler;
class Arrow;
class SmartInputDialog;
class SettingsPanel;

class NodeGraph: public QGraphicsView{
    enum EVENT_STATE{DEFAULT,MOVING_AREA,ARROW_DRAGGING,NODE_DRAGGING};
    Q_OBJECT
public slots:

    void zoomIn(){
         scaleView(qreal(1.2));
    }

    void zoomOut(){
         scaleView(1 / qreal(1.2));
    }

public:

    NodeGraph(QGraphicsScene* scene=0,QWidget *parent=0);

    ~NodeGraph(){ nodeCreation_shortcut_enabled=false; nodes.clear();}
    void setControler(Controler* c){ctrl= c;}
    void addNode_ui(QVBoxLayout *dockContainer,qreal x,qreal y,UI_NODE_TYPE type,Node *node);

    void enterEvent(QEvent *event)
    {   
        QGraphicsView::enterEvent(event);
        if(smartNodeCreationEnabled){

            nodeCreation_shortcut_enabled=true;
            setFocus();
            grabKeyboard();
        }
    }
    void leaveEvent(QEvent *event)
    {
        QGraphicsView::leaveEvent(event);
        if(smartNodeCreationEnabled){
            nodeCreation_shortcut_enabled=false;
            setFocus();
            releaseKeyboard();
         }
    }

    void keyPressEvent(QKeyEvent *e);


    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void setSmartNodeCreationEnabled(bool enabled){smartNodeCreationEnabled=enabled;}

	
protected:



    void wheelEvent(QWheelEvent *event){


        scaleView(pow((double)2, event->delta() / 240.0));
    }




    void scaleView(qreal scaleFactor){
        qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
        if (factor < 0.07 || factor > 100)
            return;

        scale(scaleFactor, scaleFactor);
    }

private:
    bool smartNodeCreationEnabled;
    QPointF old_pos;
    QPointF oldp;
    Controler* ctrl;
    EVENT_STATE state;
    Node_ui* node_dragged;
    Arrow* arrow_dragged;
    std::vector<Node_ui*> nodes;
    int timerId;
    bool nodeCreation_shortcut_enabled;
    bool _fullscreen;
};


#endif // NODEGRAPH_H
