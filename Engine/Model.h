//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_ENGINE_MODEL_H_
#define POWITER_ENGINE_MODEL_H_

#include <vector>
#include <string>
#include <utility>
#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include <QtCore/QString>
#include <QtCore/QStringList>

/*This is the core class of Powiter. It is where the plugins get loaded.
 *This class is the front-end of the core (processing part) of the software.
 **/

namespace Powiter {
    class OfxHost; // defined in Engine/OfxHost.h
}

class PluginID;

class CounterID{
public:
    CounterID(int first, const std::string& second){
        this->first=first;
        this->second=second;
        
    }
    ~CounterID(){}
    
    int first;
    std::string second;
};

namespace XMLProjectLoader {
    

class XMLParsedElement{
public:
    QString _element;
    XMLParsedElement(const QString& element):
    _element(element)
    {
    }
};

class InputXMLParsedElement : public XMLParsedElement{
public:
    int _number;
    QString _name;
    InputXMLParsedElement(const QString& name, int number):XMLParsedElement("Input"),
    _number(number),_name(name)
    {
    }
};

class KnobXMLParsedElement :  public XMLParsedElement{
public:
    QString _description;
    QString _param;
    KnobXMLParsedElement(const QString& description, const QString& param):XMLParsedElement("Knob"),
    _description(description),_param(param)
    {
    }
};

class NodeGuiXMLParsedElement :  public XMLParsedElement{
public:
    double _x,_y;
    NodeGuiXMLParsedElement(double x, double y):XMLParsedElement("Gui"),
    _x(x),_y(y)
    {
    }
};

}


class KnobFactory;
class Format;
class InputNode;
class NodeCache;
class ViewerCache;
class AppInstance;
class OutputNode;
class ViewerNodeNode;
//class Hash64;
class VideoEngine;
class QMutex;
class Node;
class OutputNode;

/// a host combines several things...
///    - a factory to create a new instance of your plugin
///      - it also gets to filter some calls during in the
///        API to check for validity and perform custom
///        operations (eg: add extra properties).
///    - it provides a description of the host application
///      which is passed back to the plugin.
class Model: public QObject,public boost::noncopyable
{
    Q_OBJECT
    

public:
    Model(AppInstance* appInstance);
    ~Model();
    
    /*Loads all kind of plugins*/
    void loadAllPlugins();

	
  
	/*Create a new node internally*/
    Node* createNode(const std::string& name);
    
    void removeNode(Node* n);

	/*Return a list of the name of all nodes available currently in Powiter*/
    const QStringList& getNodeNameList(){return _nodeNames;}

    /*output to the console the readPlugins multimap content*/
    void displayLoadedReads();
    
    /*starts the videoEngine for nbFrames. It will re-init the viewer so the
     *frame fit in the viewer.*/
    void startVideoEngine(int nbFrames=-1);
    
    VideoEngine* getVideoEngine() const;

    OutputNode* getCurrentOutput() const {return _currentOutput;}

	/*Set the output of the graph used by the videoEngine.*/
    std::pair<int,bool> setCurrentGraph(OutputNode* output,bool isViewer);


    
    
	/*add a new built-in format to the default ones*/
    void addFormat(Format* frmt);

	/*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double pixel_aspect = 1.0);
    
    ViewerCache* getViewerCache(){return _viewerCache;}
    
    
    typedef std::vector< std::pair <std::string, PluginID*> >::iterator ReadPluginsIterator;
    typedef ReadPluginsIterator WritePluginsIterator;
    
    void loadProject(const QString& filename,bool autoSave = false);
    
    void saveProject(const QString& path,const QString& filename,bool autoSave = false);
    
    void clearNodes();
    
    AppInstance* getApp() const {return _appInstance;}
    
    ViewerCache* getViewerCache() const {return _viewerCache;}
    
public slots:
    void clearPlaybackCache();
    
    void clearDiskCache();
    
    void clearNodeCache();
    
    
private:
    
    /*loads plugins(nodes)*/
    // void loadPluginsAndInitNameList();
    
    /* Viewer,Reader,Writer...etc*/
    void loadBuiltinPlugins();
    
    /*loads extra reader plug-ins */
    void loadReadPlugins();
    
    /*loads reads that are incorporated to Powiter*/
    void loadBuiltinReads();
    
    /*loads extra writer plug-ins*/
    void loadWritePlugins();
    
    /*loads writes that are built-ins*/
    void loadBuiltinWrites();
    
	/*used internally to set an appropriate name to the Node.
	 *It also read the string returned by Node::description()
	 *to know whether it is an outputNode,InputNode or an operator.*/
    void initCounterAndGetDescription(Node*& node);

    
    /*Serializes the active nodes in the editor*/
    QString serializeNodeGraph() const;
    
    /*restores the node graph from string*/
    void restoreGraphFromString(const QString& str);


        
    /*Analyses and takes action for 1 node ,given 1 attribute from the serialized version
     of the node graph.*/
    void analyseSerializedNodeString(Node* n,XMLProjectLoader::XMLParsedElement* v);
    
    
    AppInstance* _appInstance;

    OutputNode* _currentOutput; /*The output of the currently active graph.*/
    
    /*All nodes currently active in the node graph*/
    std::vector<Node*> _currentNodes;
    
    std::vector<Format*> _formats;
    std::vector<CounterID*> _nodeCounters;
    // std::vector<PluginID*> _pluginsLoaded;
    
    
    std::vector< std::pair< std::string,PluginID*> > _readPluginsLoaded;
    std::vector< std::pair< std::string,PluginID*> > _writePluginsLoaded;
    
    QStringList _nodeNames;

    KnobFactory* _knobFactory;
    
    NodeCache* _nodeCache;
    
    ViewerCache* _viewerCache;

    boost::scoped_ptr<Powiter::OfxHost> ofxHost;
};

#endif // POWITER_ENGINE_MODEL_H_
