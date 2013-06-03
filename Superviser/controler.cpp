//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include <QtWidgets/QLabel>
#include "Gui/GLViewer.h"
#include "Superviser/controler.h"
#include "Gui/mainGui.h"
#include "Core/model.h"
#include "Gui/viewerTab.h"


Controler::Controler(){}

void Controler::initControler(Model *coreEngine,QLabel* loadingScreen){
    this->coreEngine=coreEngine;
    graphicalInterface=new Gui();
    graphicalInterface->setControler(this);
    graphicalInterface->createGui();
	coreEngine->setControler(this);
    loadingScreen->hide();

#ifdef __POWITER_OSX__
	graphicalInterface->show();

#else
	graphicalInterface->showMaximized();
#endif

    delete loadingScreen;

    try{
        addNewNode(0,0,"Viewer");
    }catch(...){
        std::cout << "Couldn't create node viewer" << std::endl;
    }
    
}

Controler::~Controler(){
    delete coreEngine;
}

void Controler::exit(){}

QStringList& Controler::getNodeNameList(){
    return coreEngine->getNodeNameList();
}

void Controler::setProgressBarProgression(int value){

}
void Controler::addNewNode(qreal x,qreal y,QString name){
   
	QMutex *mutex=coreEngine->mutex();
   // Node* node=new Node(mutex);
    Node* node=NULL;
    UI_NODE_TYPE type;

    type=coreEngine->createNode(node,name,mutex);

    if(type!=UNDEFINED){
        graphicalInterface->addNode_ui(x,y,type,node);
		
    }else{
        throw "Node creation failed!";
        std::cout << "(Controler::addNewNode) Node creation failed " << std::endl;
    }
    



}
