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

#include "ofxhPluginCache.h"
#include "ofxhPropertySuite.h"
#include "ofxhImageEffect.h"
#include "ofxhImageEffectAPI.h"

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

class KnobFactory;
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

/// a host combines several things...
///    - a factory to create a new instance of your plugin
///      - it also gets to filter some calls during in the
///        API to check for validity and perform custom
///        operations (eg: add extra properties).
///    - it provides a description of the host application
///      which is passed back to the plugin.
class Model: public QObject,public boost::noncopyable,public OFX::Host::ImageEffect::Host
{
    Q_OBJECT
    

public:
    Model();
    ~Model();
    
    /*Loads all kind of plugins*/
    void loadAllPlugins();

	
  
	/*Create a new node internally*/
    bool createNode(Node *&node,const std::string name);
    
    void removeNode(Node* n);

	/*Return a list of the name of all nodes available currently in Powiter*/
    QStringList& getNodeNameList(){return _nodeNames;}

    
	/*output to the console the name of the loaded plugins*/
    void displayLoadedPlugins();
    
    /*output to the console the readPlugins multimap content*/
    void displayLoadedReads();
    
    /*starts the videoEngine for nbFrames. It will re-init the viewer so the
     *frame fit in the viewer.*/
    void startVideoEngine(int nbFrames=-1){emit vengineNeeded(nbFrames);}

	/*Set the output of the graph used by the videoEngine.*/
    std::pair<int,bool> setVideoEngineRequirements(Node* output,bool isViewer);


    VideoEngine* getVideoEngine(){return _videoEngine;}
    
    
	/*add a new built-in format to the default ones*/
    void addFormat(Format* frmt);

	/*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double pixel_aspect = 1.0);
    
    ViewerCache* getViewerCache(){return _viewerCache;}
    
    
    typedef std::vector< std::pair <std::string, PluginID*> >::iterator ReadPluginsIterator;
    typedef ReadPluginsIterator WritePluginsIterator;
    
    
    /// Create a new instance of an image effect plug-in.
    ///
    /// It is called by ImageEffectPlugin::createInstance which the
    /// client code calls when it wants to make a new instance.
    ///
    ///   \arg clientData - the clientData passed into the ImageEffectPlugin::createInstance
    ///   \arg plugin - the plugin being created
    ///   \arg desc - the descriptor for that plugin
    ///   \arg context - the context to be created in
    virtual OFX::Host::ImageEffect::Instance* newInstance(void* clientData,
                                                          OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                          OFX::Host::ImageEffect::Descriptor& desc,
                                                          const std::string& context);
    
    /// Override this to create a descriptor, this makes the 'root' descriptor
    virtual OFX::Host::ImageEffect::Descriptor *makeDescriptor(OFX::Host::ImageEffect::ImageEffectPlugin* plugin);
    
    /// used to construct a context description, rootContext is the main context
    virtual OFX::Host::ImageEffect::Descriptor *makeDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                                                               OFX::Host::ImageEffect::ImageEffectPlugin *plug);
    
    /// used to construct populate the cache
    virtual OFX::Host::ImageEffect::Descriptor *makeDescriptor(const std::string &bundlePath,
                                                               OFX::Host::ImageEffect::ImageEffectPlugin *plug);
    
    /// vmessage
    virtual OfxStatus vmessage(const char* type,
                               const char* id,
                               const char* format,
                               va_list args);
    
    
signals:
    void vengineNeeded(int nbFrames);
    
    
public slots:
    void clearPlaybackCache();
    
    void clearDiskCache();
    
    void clearNodeCache();
    
    void resetInternalDAG();
    
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
    
    /*Reads OFX plugin cache and scan plugins directories
     to load them all.*/
    void loadOFXPlugins();
    
    /*Writes all plugins loaded and their descriptors to
     the OFX plugin cache.*/
    void writeOFXCache();
    
	/*used internally to set an appropriate name to the Node.
	 *It also read the string returned by Node::description()
	 *to know whether it is an outputNode,InputNode or an operator.*/
    void initCounterAndGetDescription(Node*& node);


    VideoEngine* _videoEngine; // Video Engine
  
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
    
    OFX::Host::ImageEffect::PluginCache _imageEffectPluginCache;
    
    
    /*plugin name -> pair< plugin id , plugin grouping >
     The name of the plugin is followed by the first part of the grouping in brackets
     to help identify two distinct plugins with the same name. e.g :
     1)Invert [OFX]  with plugin id net.sourceforge.openfx.invert and grouping OFX/
     2)Invert [Toto] with plugin id com.toto.invert and grouping Toto/SuperPlugins/OFX/
     */
    typedef std::map<std::string,std::pair<std::string,std::string> > OFXPluginsMap;
    typedef OFXPluginsMap::const_iterator OFXPluginsIterator;
    
    OFXPluginsMap _ofxPlugins;

};

#endif // MODEL_H
