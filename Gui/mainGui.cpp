#include "Gui/mainGui.h"
#include "Superviser/controler.h"
#include "Gui/GLViewer.h"
#include "Core/model.h"
#include "Core/VideoEngine.h"

using namespace std;

Gui::Gui(QWidget* parent):QMainWindow(parent)
{

}
Gui::~Gui(){
    

}
void Gui::exit(){
  //  delete crossPlatform;
   // crossPlatform->exit();
    crossPlatform->getModel()->getVideoEngine()->abort();
    qApp->exit(0);
    
}
void Gui::closeEvent(QCloseEvent *e){
    // save project ...
    exit();
    e->accept();
}


void Gui::addNode_ui(qreal x,qreal y,UI_NODE_TYPE type, Node* node){
    nodeGraphArea->addNode_ui(layout_settings, x,y,type,node);

}

void Gui::setControler(Controler* crossPlatform){
    this->crossPlatform=crossPlatform;
}
void Gui::createGui(){

   
    setupUi(crossPlatform);
    
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(exit()));
    nodeGraphArea->setControler(crossPlatform);
    
    nodeGraphArea->installEventFilter(this);
    viewer_tab->installEventFilter(this);
    rightDock->installEventFilter(this);
}

void Gui::keyPressEvent(QKeyEvent *e){


    if(e->modifiers().testFlag(Qt::AltModifier) &&
       e->modifiers().testFlag(Qt::ControlModifier) &&
       e->key() == Qt::Key_K){

    }
    QMainWindow::keyPressEvent(e);
}

bool Gui::eventFilter(QObject *target, QEvent *event){
    
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Right) {
            crossPlatform->getModel()->getVideoEngine()->nextFrame();
            focusNextChild();
            return true;
        }else if(keyEvent->key() == Qt::Key_Left){
            crossPlatform->getModel()->getVideoEngine()->previousFrame();
            focusNextChild();
            return true;
        }
    }
    return QMainWindow::eventFilter(target, event);;
}

