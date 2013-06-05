//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef CONTROLER_H
#define CONTROLER_H
#include <QtCore/QObject>
#include "Gui/node_ui.h"
#include "Superviser/powiterFn.h"
#include "Core/singleton.h"
using namespace Powiter_Enums;

/*macro to get a pointer to the controler*/
#define ctrlPTR Controler::instance()

class Model;
class Gui;
class QLabel;

/*Controler (see Model-view-controler pattern on wikipedia). This class
 implements the singleton pattern to ensure there's only 1 single
 instance of the object living. Also you can access the controler
 by the handy macro ctrlPTR*/
class Controler : public QObject,public Singleton<Controler>
{

public:
    Controler();
    ~Controler();

    /*Create a new node at position (x,y) in the node graph.
     By default x,y has garbage value and the nodegraph will try
     to guess a good position for the node, you can bypass it by specifying
     explicitly x,y in scene coordinates.
     The name passed in parameter must match a valid node name,
     otherwise an exception is thrown. You should encapsulate the call
     by a try-catch block.*/
    void createNode(QString name,double x = INT_MAX,double y = INT_MAX);
    
    /*Get a reference to the list of all the node names 
     available. E.g : Viewer,Reader, Blur, etc...*/
    QStringList& getNodeNameList();
    
    /*Pointer to the GUI*/
    Gui* getGui(){return _gui;}
    
    /*Pointer to the model*/
	Model* getModel(){return _model;}
    
    /*initialize the pointers to the model and the view. It also call 
     gui->createGUI() and build a viewer node.*/
    void initControler(Model* model,QLabel* loadingScreen);
private:
	 
    
    Model* _model; // the model of the MVC pattern
    Gui* _gui; // the view of the MVC pattern

};


#endif // CONTROLER_H

