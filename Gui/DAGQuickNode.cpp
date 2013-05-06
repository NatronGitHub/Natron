#include "Gui/DAGQuickNode.h"
#include "Gui/DAG.h"


SmartInputDialog::SmartInputDialog(Controler* ctrl,NodeGraph* graph, QWidget *parent):QFrame(parent)
{
   //this->setParent(dynamic_cast<QWidget*>(ctrlPTR->getGui()));
    this->graph=graph;
    this->ctrl=ctrl;
    setWindowTitle(QString("Node creation tool"));
    setWindowFlags(Qt::Popup);
    setObjectName(QString("SmartDialog"));
    setStyleSheet(QString("SmartInputDialog#SmartDialog{border-style:outset;border-width: 2px; border-color: black; background-color:silver;}"));
    layout=new QVBoxLayout(this);
    textLabel=new QLabel(QString("Input a node name:"),this);
    textEdit=new QComboBox(this);
    textEdit->setEditable(true);

    textEdit->addItems(ctrl->getNodeNameList());
    layout->addWidget(textLabel);
    layout->addWidget(textEdit);
    textEdit->lineEdit()->selectAll();
    textEdit->setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(textEdit->lineEdit());
    textEdit->lineEdit()->setFocus(Qt::ActiveWindowFocusReason);
    textEdit->grabKeyboard();
    installEventFilter(this);
}
void SmartInputDialog::keyPressEvent(QKeyEvent *e){
    if(e->key() == Qt::Key_Return){
      QString res=textEdit->lineEdit()->text();
      if(ctrl->getNodeNameList().contains(res)){
          try{

          ctrl->addNewNode(0,0,res);
          }catch(...){
              std::cout << "(SmartInputDialog:KeyPressEvent) Couldn't create node " << qPrintable(res) << std::endl;
          }

          graph->setSmartNodeCreationEnabled(true);
          graph->setMouseTracking(true);
          textEdit->releaseKeyboard();

          graph->setFocus(Qt::ActiveWindowFocusReason);
          delete this;


      }else{

      }
    }else if(e->key()== Qt::Key_Escape){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        textEdit->releaseKeyboard();

        graph->setFocus(Qt::ActiveWindowFocusReason);


        delete this;


    }
}
bool SmartInputDialog::eventFilter(QObject *obj, QEvent *e){


    if(e->type()==QEvent::Close){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        textEdit->releaseKeyboard();

        graph->setFocus(Qt::ActiveWindowFocusReason);


    }
    return false;
}
