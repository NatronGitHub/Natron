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


Controler::Controler():_model(0),_gui(0){}

void Controler::initControler(Model *model,QLabel* loadingScreen){
    this->_model=model;
    _gui=new Gui();

    _gui->createGui();
	model->postInitialisation();
    loadingScreen->hide();

#ifdef __POWITER_OSX__
	_gui->show();

#else
	_gui->showMaximized();
#endif

    delete loadingScreen;

    try{
        createNode(0,0,"Viewer");
    }catch(...){
        std::cout << "Couldn't create node viewer" << std::endl;
    }
    
}

Controler::~Controler(){
    delete _model;
}

void Controler::exit(){}

QStringList& Controler::getNodeNameList(){
    return _model->getNodeNameList();
}

void Controler::setProgressBarProgression(int value){

}
void Controler::createNode(qreal x,qreal y,QString name){
   
	QMutex *mutex=_model->mutex();
   // Node* node=new Node(mutex);
    Node* node=NULL;
    UI_NODE_TYPE type;

    type=_model->createNode(node,name,mutex);

    if(type!=UNDEFINED){
        _gui->createNodeGUI(x,y,type,node);
		
    }else{
        throw "Node creation failed!";
        std::cout << "(Controler::addNewNode) Node creation failed " << std::endl;
    }
    



}
