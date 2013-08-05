//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include "QtCore/QStringList"

#include "Gui/DAGQuickNode.h"
#include "Gui/DAG.h"
#include "Superviser/controler.h"
SmartInputDialog::SmartInputDialog(NodeGraph* graph):QDialog()
{
    this->graph=graph;
    setWindowTitle(QString("Node creation tool"));
    setWindowFlags(Qt::Popup);
    setObjectName(QString("SmartDialog"));
    setStyleSheet(QString("SmartInputDialog#SmartDialog{border-style:outset;border-width: 2px; border-color: black; background-color:silver;}"));
    layout=new QVBoxLayout(this);
    textLabel=new QLabel(QString("Input a node name:"),this);
    textEdit=new QComboBox(this);
    textEdit->setEditable(true);
    
    textEdit->addItems(ctrlPTR->getNodeNameList());
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
        if(ctrlPTR->getNodeNameList().contains(res)){
            ctrlPTR->createNode(res,INT_MAX,INT_MAX);
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
    Q_UNUSED(obj);
    
    if(e->type()==QEvent::Close){
        graph->setSmartNodeCreationEnabled(true);
        graph->setMouseTracking(true);
        textEdit->releaseKeyboard();
        
        graph->setFocus(Qt::ActiveWindowFocusReason);
        
        
    }
    return false;
}
