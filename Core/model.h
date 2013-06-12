//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef MODEL_H
#define MODEL_H
#include <iostream>
#include <fstream>
#include <QtCore/QStringList>
#include "Superviser/powiterFn.h"
#ifdef __POWITER_UNIX__
#include <dlfcn.h>
#endif
#include <map>
#include <vector>
#include <ImfStandardAttributes.h>
#include <boost/noncopyable.hpp>
/*This is the core class of Powiter. It is where the plugins get loaded.
 *This class is the front-end of the core (processing part) of the software.
 **/

#ifdef __POWITER_WIN32__
class PluginID{
public:
    PluginID(HINSTANCE first,std::string second){
        this->first=first;
        this->second=second;
    }
    ~PluginID(){
        
        FreeLibrary(first);
    
    }
    HINSTANCE first;
    std::string second;
};
#elif defined(__POWITER_UNIX__)
class PluginID{
public:
    PluginID(void* first,std::string second){
        this->first=first;
        this->second=second;
    }
    ~PluginID(){
        
        dlclose(first);
    }
    void* first;
    std::string second;
};
#endif
class CounterID{
public:
    CounterID(int first,std::string second){
        this->first=first;
        this->second=second;
        
    }
    ~CounterID(){}
    
    int first;
    std::string second;
};

class Format;
class InputNode;
class NodeCache;
class ViewerCache;
class Controler;
class OutputNode;
class Viewer;
class Hash;
class VideoEngine;
class QMutex;
class Node;
class Model: public QObject,public boost::noncopyable
{
    Q_OBJECT
    

public:
    Model();
    ~Model();
    
    /*loads plugins(nodes)*/
    void loadPluginsAndInitNameList();
    
    /*name says it all*/
    void loadBuiltinPlugins();
    
    /*loads extra reader plug-ins */
    void loadReadPlugins();
    
    /*loads reads that are incorporated to Powiter*/
    void loadBuiltinReads();
    

    /*utility functions used to parse*/
    std::string removePrefixSpaces(std::string str);
    std::string getNextWord(std::string str);
	
  
	/*Create a new node internally*/
    Powiter_Enums::UI_NODE_TYPE createNode(Node *&node,QString &name,QMutex* m);

	/*Return a list of the name of all nodes available currently in Powiter*/
    QStringList& getNodeNameList(){return _nodeNames;}

	/*this is the general mutex used by all the nodes in the graph*/
	QMutex* mutex(){return _mutex;}
    
	/*output to the console the name of the loaded plugins*/
    void displayLoadedPlugins();
    
    /*output to the console the readPlugins multimap content*/
    void displayLoadedReads();
    
    /*starts the videoEngine for nbFrames. It will re-init the viewer so the
     *frame fit in the viewer.*/
    void startVideoEngine(int nbFrames=-1){emit vengineNeeded(nbFrames);}

	/*Set the output of the graph used by the videoEngine.*/
    void setVideoEngineRequirements(OutputNode* output);


    VideoEngine* getVideoEngine(){return _videoEngine;}
    
    
	/*add a new built-in format to the default ones*/
    void addFormat(Format* frmt);

	/*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double pixel_aspect = 1.0);
    
    ViewerCache* getViewerCache(){return _viewerCache;}
    
    
    typedef std::vector< std::pair <std::string, PluginID*> >::iterator ReadPluginsIterator;
    
signals:
    void vengineNeeded(int nbFrames);
    
    
public slots:
    void clearPlaybackCache();
    
    void clearDiskCache();
    
    void clearNodeCache();
    
private:
    
    
	/*used internally to set an appropriate name to the Node.
	 *It also read the string returned by Node::description()
	 *to know whether it is an outputNode,InputNode or an operator.*/
    Powiter_Enums::UI_NODE_TYPE initCounterAndGetDescription(Node*& node);


    VideoEngine* _videoEngine; // Video Engine
    QMutex* _mutex; // mutex for workerthreads
  
    /*All nodes currently active in the node graph*/
    std::vector<Node*> _currentNodes;
    
    std::vector<Format*> _formats;
    std::vector<CounterID*> _nodeCounters;
    std::vector<PluginID*> _pluginsLoaded;
    std::vector< std::pair< std::string,PluginID*> > _readPluginsLoaded;
    QStringList _nodeNames;


    NodeCache* _nodeCache;
    
    ViewerCache* _viewerCache;

};

#endif // MODEL_H
