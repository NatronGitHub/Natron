//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GLOBAL_APPMANAGER_H_
#define POWITER_GLOBAL_APPMANAGER_H_

#include <vector>
#include <string>
#include <map>
#include <boost/scoped_ptr.hpp>

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

#ifndef Q_MOC_RUN
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"

#include "Engine/Singleton.h"


/*macro to get the unique pointer to the controler*/
#define appPTR AppManager::instance()

class KnobFactory;
class NodeCache;
class ViewerCache;
class NodeGui;
class Node;
class Model;
class ViewerNode;
class Writer;
class ViewerTab;
class TabWidget;
class Gui;
class Format;
class VideoEngine;
class OutputNode;

namespace Powiter {
    class LibraryBinary;
    class OfxHost;
}

class Project{
public:
    Project();
    
    QString _projectName;
    QString _projectPath;
    bool _hasProjectBeenSavedByUser;
    QDateTime _age;
    QDateTime _lastAutoSave;
    Format* _format;
};

/*Controler (see Model-view-controler pattern on wikipedia). This class
 implements the singleton pattern to ensure there's only 1 single
 instance of the object living. Also you can access the controler
 by the handy macro appPTR*/
class AppInstance : public QObject
{
    Q_OBJECT
public:
    AppInstance(int appID,const QString& projectName = QString());
    
    ~AppInstance();
    
    int getAppID() const {return _appID;}

    /*Create a new node  in the node graph.
     The name passed in parameter must match a valid node name,
     otherwise an exception is thrown. You should encapsulate the call
     by a try-catch block.*/
    Node* createNode(const QString& name);
    
    /*Pointer to the GUI*/
    Gui* getGui(){return _gui;}
    
    /* The following methods are forwarded to the model */
    VideoEngine* getVideoEngine();
    void checkViewersConnection();

    void updateDAG(OutputNode *output,bool initViewer);
    
    bool isRendering() const;
        
    void startRendering(int nbFrames = -1);
    
    OutputNode* getCurrentOutput();
    
    ViewerNode* getCurrentViewer();
    
    Writer* getCurrentWriter();
    
    const std::vector<Node*> getAllActiveNodes() const;
    
    const QString& getCurrentProjectName() const {return _currentProject._projectName;}
    
    const QString& getCurrentProjectPath() const {return _currentProject._projectPath;}
    
    void setCurrentProjectName(const QString& name) {_currentProject._projectName = name;}
    
    void loadProject(const QString& path,const QString& name);
    
    void saveProject(const QString& path,const QString& name,bool autoSave);
    
    void autoSave();
    
    void triggerAutoSave();
            
    bool hasProjectBeenSavedByUser() const {return _currentProject._hasProjectBeenSavedByUser;}
    
    const Format& getProjectFormat() const {return *(_currentProject._format);}
    
    void setProjectFormat(Format* frmt){_currentProject._format = frmt;}
    
    void resetCurrentProject();
    
    void clearNodes();
        
    bool isSaveUpToDate() const;
    
    void deselectAllNodes() const;
    
    void showErrorDialog(const QString& title,const QString& message) const;
    
    static QString autoSavesDir();
    
    ViewerTab* addNewViewerTab(ViewerNode* node,TabWidget* where);
    
    bool connect(int inputNumber,const std::string& inputName,Node* output);
    
    bool connect(int inputNumber,Node* input,Node* output);
    
    bool disconnect(Node* input,Node* output);
    
    void autoConnect(Node* target,Node* created);
    
    NodeGui* getNodeGui(Node* n) const;
    
    Node* getNode(NodeGui* n) const;
    
    void connectViewersToViewerCache();
    
    void disconnectViewersFromViewerCache();
    
    void onRenderingOnDiskStarted(Writer* writer,const QString& sequenceName,int firstFrame,int lastFrame);

public slots:
    void clearPlaybackCache();

    void clearDiskCache();

    void clearNodeCache();

private:
    
    
	void removeAutoSaves() const;
    
    /*Attemps to find an autosave. If found one,prompts the user
     whether he/she wants to load it. If something was loaded this function
     returns true,otherwise false.*/
    bool findAutoSave();
        
    boost::scoped_ptr<Model> _model; // the model of the MVC pattern
    Gui* _gui; // the view of the MVC pattern
    
    Project _currentProject;
    
    int _appID;
    
    std::map<Node*,NodeGui*> _nodeMapping;
};

class AppManager : public QObject, public Singleton<AppManager>
{

    Q_OBJECT
    
public:
    
    class PluginToolButton{
    public:
        PluginToolButton(const QStringList& groups,
                         const QString& pluginName,
                         const QString& pluginIconPath,
                         const QString& groupIconPath):
        _groups(groups),
        _pluginName(pluginName),
        _pluginIconPath(pluginIconPath),
        _groupIconPath(groupIconPath)
        {
            
        }
        QStringList _groups;
        QString _pluginName;
        QString _pluginIconPath;
        QString _groupIconPath;
        
    };

    typedef std::map< std::string,std::pair< std::vector<std::string> ,Powiter::LibraryBinary*> >::iterator ReadPluginsIterator;
    typedef ReadPluginsIterator WritePluginsIterator;
    
    AppManager();
    
    virtual ~AppManager();
    
    const boost::scoped_ptr<Powiter::OfxHost>& getOfxHost() const {return ofxHost;}
    
    AppInstance* newAppInstance(const QString& projectName = QString());
    
    AppInstance* getAppInstance(int appID) const;
    
    void removeInstance(int appID);
    
    void setAsTopLevelInstance(int appID);
    
    AppInstance* getTopLevelInstance () const;
    
    /*Return a list of the name of all nodes available currently in the software*/
    const QStringList& getNodeNameList(){return _nodeNames;}
    
    /*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double pixel_aspect = 1.0);
    
    const std::vector<PluginToolButton*>& getPluginsToolButtons() const {return _toolButtons;}
    
    /*Tries to load all plugins in directory "where"*/
    static std::vector<Powiter::LibraryBinary*> loadPlugins(const QString& where);
    
    /*Tries to load all plugins in directory "where" that contains all the functions described by
     their name in "functions".*/
    static std::vector<Powiter::LibraryBinary*> loadPluginsAndFindFunctions(const QString& where,
                                                                            const std::vector<std::string>& functions);
    
    ViewerCache* getViewerCache() const {return _viewerCache;}
    
    NodeCache* getNodeCache() const {return _nodeCache;}
    
    KnobFactory* getKnobFactory() const {return _knobFactory;}
    
 
public slots:
    
    void addPluginToolButtons(const QStringList& groups,
                                const QString& pluginName,
                                const QString& pluginIconPath,
                                const QString& groupIconPath);
    /*Quit the app*/
    static void quit();
private:
    
    /*Loads all kind of plugins*/
    void loadAllPlugins();
    
    //////////////////////////////
    //// NODE PLUGINS
    /* Viewer,Reader,Writer...etc.
     No function to load external plugins
     yet since the SDK isn't released.*/
    void loadBuiltinNodePlugins();
    //////////////////////////////
    
    //////////////////////////////
    //// READ PLUGINS
    /*loads extra reader plug-ins */
    void loadReadPlugins();
    /*loads reads that are incorporated to Powiter*/
    void loadBuiltinReads();
    //////////////////////////////

    //////////////////////////////
    //// WRITE PLUGINS
    /*loads extra writer plug-ins*/
    void loadWritePlugins();
    /*loads writes that are built-ins*/
    void loadBuiltinWrites();
    //////////////////////////////
    
    void loadBuiltinFormats();
    
    
    std::map<int,AppInstance*> _appInstances;
    
    int _availableID;
    
    int _topLevelInstanceID;
    
    std::map< std::string,std::pair< std::vector<std::string> ,Powiter::LibraryBinary*> > _readPluginsLoaded;
    std::map< std::string,std::pair< std::vector<std::string> ,Powiter::LibraryBinary*> > _writePluginsLoaded;
    
    std::vector<Format*> _formats;
    
    QStringList _nodeNames;
    
    boost::scoped_ptr<Powiter::OfxHost> ofxHost;
    
    std::vector<PluginToolButton*> _toolButtons;
    
    KnobFactory* _knobFactory;
    
    NodeCache* _nodeCache;
    
    ViewerCache* _viewerCache;
    
};


#endif // POWITER_GLOBAL_CONTROLER_H_

