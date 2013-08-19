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


/*macro to get the unique pointer to the controler*/
#define ctrlPTR Controler::instance()

#define currentViewer Controler::getCurrentViewer()

#define currentWriter Controler::getCurrentWriter()

class NodeGui;
class Model;
class ViewerNode;
class Writer;
class Gui;
class QLabel;

/*Controler (see Model-view-controler pattern on wikipedia). This class
 implements the singleton pattern to ensure there's only 1 single
 instance of the object living. Also you can access the controler
 by the handy macro ctrlPTR*/
class Controler : public QObject,public Singleton<Controler>
{
    class PluginToolButton{
    public:
        PluginToolButton( std::vector<std::string> groups,
                          std::string pluginName,
                          std::string pluginIconPath,
                          std::string groupIconPath):
        _groups(groups),
        _pluginName(pluginName),
        _pluginIconPath(pluginIconPath),
        _groupIconPath(groupIconPath)
        {
            
        }
        std::vector<std::string> _groups;
        std::string _pluginName;
        std::string _pluginIconPath;
        std::string _groupIconPath;
        
    };
    
    std::vector<PluginToolButton*> _toolButtons;
    
public:
    Controler();
    ~Controler();

    /*Create a new node  in the node graph.
     The name passed in parameter must match a valid node name,
     otherwise an exception is thrown. You should encapsulate the call
     by a try-catch block.*/
    void createNode(QString name);
    
    /*Get a reference to the list of all the node names 
     available. E.g : Viewer,Reader, Blur, etc...*/
    const QStringList& getNodeNameList();
    
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
    static ViewerNode* getCurrentViewer();
    
    /*Returns a pointer to the Writer currently used
     by the VideoEngine. If the output is not a writer,
     it will return NULL.*/
    static Writer* getCurrentWriter();
    
    void stackPluginToolButtons(const std::vector<std::string>& groups,
                             const std::string& pluginName,
                             const std::string& pluginIconPath,
                             const std::string& groupIconPath);
    
private:
	 
    void addBuiltinPluginToolButtons();
    
    Model* _model; // the model of the MVC pattern
    Gui* _gui; // the view of the MVC pattern

};


#endif // CONTROLER_H

