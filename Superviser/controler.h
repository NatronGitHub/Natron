//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 




#ifndef CONTROLER_H
#define CONTROLER_H
#include <QtCore/QObject>
#include "Superviser/powiterFn.h"
#include "Core/singleton.h"
using namespace Powiter_Enums;

/*macro to get the unique pointer to the controler*/
#define ctrlPTR Controler::instance()

#define currentViewer Controler::getCurrentViewer()

#define currentWriter Controler::getCurrentWriter()

class NodeGui;
class Model;
class Viewer;
class Writer;
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
    
    /*Returns a pointer to the Viewer currently used
     by the VideoEngine. If the output is not a viewer,
     it will return NULL.*/
    static Viewer* getCurrentViewer();
    
    /*Returns a pointer to the Writer currently used
     by the VideoEngine. If the output is not a writer,
     it will return NULL.*/
    static Writer* getCurrentWriter();
    
private:
	 
    
    Model* _model; // the model of the MVC pattern
    Gui* _gui; // the view of the MVC pattern

};


#endif // CONTROLER_H

