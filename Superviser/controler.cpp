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
#include "Gui/node_ui.h"
#include "Core/VideoEngine.h"


Controler::Controler():_model(0),_gui(0){}

void Controler::initControler(Model *model,QLabel* loadingScreen){
    this->_model=model;
    _gui=new Gui();

    _gui->createGui();
    loadingScreen->hide();

#ifdef __POWITER_OSX__
	_gui->show();

#else
	_gui->showMaximized();
#endif

    delete loadingScreen;

    try{
        createNode("Viewer",0,0);
    }catch(...){
        std::cout << "Couldn't create node viewer" << std::endl;
    }
    
}

Controler::~Controler(){
    delete _model;
}


QStringList& Controler::getNodeNameList(){
    return _model->getNodeNameList();
}


void Controler::createNode(QString name,double x,double y){
   
	QMutex *mutex=_model->mutex();
   // Node* node=new Node(mutex);
    Node* node=NULL;
    UI_NODE_TYPE type;

    type=_model->createNode(node,name,mutex);

    if(type!=UNDEFINED){
        _gui->createNodeGUI(type,node,x,y);
		
    }else{
        throw "Node creation failed!";
        std::cout << "(Controler::addNewNode) Node creation failed " << std::endl;
    }
    



}

Viewer* Controler::getCurrentViewer(){
    Controler* ctrl = Controler::instance();
    return ctrl->getModel()->getVideoEngine()->getCurrentDAG().outputAsViewer();
}

Writer* Controler::getCurrentWriter(){
    Controler* ctrl = Controler::instance();
    return ctrl->getModel()->getVideoEngine()->getCurrentDAG().outputAsWriter();
}
