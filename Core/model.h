//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef MODEL_H
#define MODEL_H
#include "Superviser/PwStrings.h"
#include <iostream>
#include <fstream>
#include <QtCore/QStringList>
#ifdef __POWITER_UNIX__
#include "dlfcn.h"
#endif
#include <map>
#include <ImfStandardAttributes.h>
#include "Core/inputnode.h"
#include "Core/settings.h"

/*This is the core class of Powiter. It is where the plugins get loaded.
 *This class is the front-end of the core (processing part) of the software.
 **/
using namespace Powiter_Enums;

#ifdef __POWITER_WIN32__
class PluginID{
public:
    PluginID(HINSTANCE first,const char* second){
        this->first=first;
        this->second=second;
    }
    ~PluginID(){
        
        FreeLibrary(first);
        delete[] second;
    }
    HINSTANCE first;
    const char* second;
};
#elif defined(__POWITER_UNIX__)
class PluginID{
public:
    PluginID(void* first,const char* second){
        this->first=first;
        this->second=second;
    }
    ~PluginID(){
        
        dlclose(first);
        delete[] second;
    }
    void* first;
    const char* second;
};
#endif
class CounterID{
public:
    CounterID(int first,const char* second){
        this->first=first;
        this->second=second;
        
    }
    ~CounterID(){
        delete[] second;
    }
    
    int first;
    const char* second;
};



class Controler;
class OutputNode;
class Viewer;
class Hash;
class VideoEngine;
class QMutex;
//class Reader;

class Model: public QObject
{
    Q_OBJECT
    

public:
    Model(QObject* parent=0);
    virtual ~Model();
    
    void loadPluginsAndInitNameList();

    /*utility functions used to parse*/
    std::string removePrefixSpaces(std::string str);
    std::string getNextWord(std::string str);
	
	/*fills the inputs vector with the InputNodes found
	 *upstream of output.*/
	void getGraphInput(std::vector<InputNode*> &inputs,Node* output);

	/*set pointer to the controler*/
    void setControler(Controler* ctrl);

	/*Create a new node internally*/
    UI_NODE_TYPE createNode(Node *&node,QString &name,QMutex* m);

	/*Return a list of the name of all nodes available currently in Powiter*/
    QStringList& getNodeNameList(){return nodeNameList;}

	/*this is the general mutex used by all the nodes in the graph*/
	QMutex* mutex(){return _mutex;}
    
	/*output to the console the name of the loaded plugins*/
    void displayLoadedPlugins();
    
    /*starts the videoEngine for nbFrames. It will re-init the viewer so the
     *frame fit in the viewer.*/
    void startVideoEngine(int nbFrames=-1){emit vengineNeeded(nbFrames);}

	/*Set the inputs and output of the graph used by the videoEngine.*/
    void setVideoEngineRequirements(std::vector<InputNode*> inputs,OutputNode* output);


    VideoEngine* getVideoEngine(){return vengine;}
    
    Controler* getControler(){return ctrl;}
    
	/*add a new built-in format to the default ones*/
    void addFormat(Format* frmt);

	/*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double pixel_aspect = 1.0);
    
    /*Get the current settings of Powiter*/
    Settings* getCurrentPowiterSettings(){return _powiterSettings;}
signals:
    void vengineNeeded(int nbFrames);
    
    
private:
	/*used internally to set an appropriate name to the Node.
	 *It also read the string returned by Node::description()
	 *to know whether it is an outputNode,InputNode or an operator.*/
    UI_NODE_TYPE initCounterAndGetDescription(Node*& node);

    
    std::vector<const char*> formatNames;
    std::vector<Imath::V3f*> resolutions;
    std::vector<Format*> formats_list;
    std::vector<CounterID*> counters;
    std::vector<PluginID*> plugins;
    QStringList nodeNameList;

	/*All nodes currently active in the node graph*/
    std::vector<Node*> allNodes;
    
    Settings* _powiterSettings;
    Controler* ctrl;
    VideoEngine* vengine; // Video Engine
    QMutex* _mutex; // mutex for workerthreads



};

#endif // MODEL_H
