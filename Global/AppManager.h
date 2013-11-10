//  Natron
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

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>


#ifndef Q_MOC_RUN
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Row.h"
#include "Engine/Image.h"
#include "Engine/FrameEntry.h"
#include "Engine/Format.h"

/*macro to get the unique pointer to the controler*/
#define appPTR AppManager::instance()

class KnobFactory;
class NodeGui;
class ViewerInstance;
class ViewerTab;
class TabWidget;
class Gui;
class VideoEngine;
class QMutex;
class TimeLine;
class Settings;

namespace Natron {
class OutputEffectInstance;
class LibraryBinary;
class EffectInstance;
class OfxHost;
class Node;
class Project;
}



class AppInstance : public QObject,public boost::noncopyable
{
    Q_OBJECT
public:
    AppInstance(bool backgroundMode,int appID,const QString& projectName = QString(),const QStringList& writers = QStringList());
    
    ~AppInstance();
    
    int getAppID() const {return _appID;}
    
    bool isBackground() const {return _isBackground;}
    
    /*Create a new node  in the node graph.
     The name passed in parameter must match a valid node name,
     otherwise an exception is thrown. You should encapsulate the call
     by a try-catch block.*/
    Natron::Node* createNode(const QString& name,bool requestedByLoad = false);
    
    /*Pointer to the GUI*/
    Gui* getGui() WARN_UNUSED_RETURN {return _gui;}

    const std::vector<NodeGui*>& getAllActiveNodes() const WARN_UNUSED_RETURN;

    const QString& getCurrentProjectName() const WARN_UNUSED_RETURN ;

    const QString& getCurrentProjectPath() const WARN_UNUSED_RETURN ;

    int getCurrentProjectViewsCount() const;

    boost::shared_ptr<Natron::Project> getProject() const { return _currentProject;}

    boost::shared_ptr<TimeLine> getTimeLine() const WARN_UNUSED_RETURN ;

    void setCurrentProjectName(const QString& name) ;

    bool shouldAutoSetProjectFormat() const ;

    void setAutoSetProjectFormat(bool b);

    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN ;

    const Format& getProjectFormat() const WARN_UNUSED_RETURN ;

    bool loadProject(const QString& path,const QString& name,bool background);

    void saveProject(const QString& path,const QString& name,bool autoSave);

    void autoSave();

    void tryAddProjectFormat(const Format& frmt);

    void setProjectFormat(const Format& frmt);

    void resetCurrentProject();

    int getKnobsAge() const;

    void incrementKnobsAge();

    void clearNodes();

    bool isSaveUpToDate() const WARN_UNUSED_RETURN;

    void deselectAllNodes() const;

    static QString autoSavesDir() WARN_UNUSED_RETURN;

    ViewerTab* addNewViewerTab(ViewerInstance* node,TabWidget* where) WARN_UNUSED_RETURN;


    bool connect(int inputNumber,const std::string& inputName,Natron::Node* output);

    bool connect(int inputNumber,Natron::Node* input,Natron::Node* output);

    bool disconnect(Natron::Node* input,Natron::Node* output);

    void autoConnect(Natron::Node* target,Natron::Node* created);

    NodeGui* getNodeGui(Natron::Node* n) const WARN_UNUSED_RETURN;

    Natron::Node* getNode(NodeGui* n) const WARN_UNUSED_RETURN;


    void connectViewersToViewerCache();

    void disconnectViewersFromViewerCache();

    QMutex* getAutoSaveMutex() const WARN_UNUSED_RETURN {return _autoSaveMutex;}

    void errorDialog(const std::string& title,const std::string& message) const;

    void warningDialog(const std::string& title,const std::string& message) const;

    void informationDialog(const std::string& title,const std::string& message) const;

    Natron::StandardButton questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons =
            Natron::StandardButtons(Natron::Yes | Natron::No),
                                           Natron::StandardButton defaultButton = Natron::NoButton) const WARN_UNUSED_RETURN;



public slots:

    /* The following methods are forwarded to the model */
    void checkViewersConnection();

    void setupViewersForViews(int viewsCount);

    void notifyViewersProjectFormatChanged(const Format& format);

    void setViewersCurrentView(int view);

    void triggerAutoSave();

    void onRenderingOnDiskStarted(Natron::OutputEffectInstance* writer,const QString& sequenceName,int firstFrame,int lastFrame);


signals:

    void pluginsPopulated();

private:


    void removeAutoSaves() const;

    /*Attemps to find an autosave. If found one,prompts the user
 whether he/she wants to load it. If something was loaded this function
 returns true,otherwise false.*/
    bool findAutoSave() WARN_UNUSED_RETURN;

    /*Used in background mode only*/
    void startWritersRendering(const QStringList& writers);

    Gui* _gui; // the view of the MVC pattern

    boost::shared_ptr<Natron::Project> _currentProject;

    int _appID;

    std::map<Natron::Node*,NodeGui*> _nodeMapping;

    QMutex* _autoSaveMutex;

    bool _isBackground;

};

class PluginToolButton{
    
    QString _name;
    QString _iconPath;
    std::vector<PluginToolButton*> _children;
    PluginToolButton* _parent;
    
public:
    PluginToolButton(const QString& name,
                     const QString& iconPath)
    :
     _name(name)
    , _iconPath(iconPath)
    , _children()
    , _parent(NULL)
    {
        
    }
    
    
    const QString& getName() const {return _name;}
    
    void setName(const QString& name) {_name = name;}
    
    const QString& getIconPath() const {return _iconPath;}
    
    void setIconPath(const QString& iconPath) {_iconPath = iconPath;}
    
    const std::vector<PluginToolButton*>& getChildren() const {return _children;}
    
    void tryAddChild(PluginToolButton* plugin);
    
    PluginToolButton* getParent() const {return _parent;}
    
    void setParent(PluginToolButton* parent) {_parent = parent;}
    
    bool hasParent() const {return _parent != NULL;}
    
};

class AppManager : public QObject, public Singleton<AppManager>
{
    
    Q_OBJECT
    
public:
    
    
    typedef std::map< std::string,std::pair< std::vector<std::string> ,Natron::LibraryBinary*> >::iterator ReadPluginsIterator;
    typedef ReadPluginsIterator WritePluginsIterator;
    
    AppManager();
    
    virtual ~AppManager();
    
    const boost::scoped_ptr<Natron::OfxHost>& getOfxHost() const WARN_UNUSED_RETURN {return ofxHost;}

    AppInstance* newAppInstance(bool background,const QString& projectName = QString(),const QStringList& writers = QStringList());

    void registerAppInstance(AppInstance* app){ _appInstances.insert(std::make_pair(app->getAppID(),app));}

    AppInstance* getAppInstance(int appID) const;

    void removeInstance(int appID);

    void setAsTopLevelInstance(int appID);

    AppInstance* getTopLevelInstance () const WARN_UNUSED_RETURN;

    /*Return a list of the name of all nodes available currently in the software*/
    QStringList getNodeNameList() const WARN_UNUSED_RETURN;

    QMutex* getMutexForPlugin(const QString& pluginName) const;

    Natron::LibraryBinary* getPluginBinary(const QString& pluginName) const;

    /*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double pixel_aspect = 1.0) const WARN_UNUSED_RETURN;

    const std::vector<Format*>& getFormats() const {return _formats;}

    const std::vector<PluginToolButton*>& getPluginsToolButtons() const WARN_UNUSED_RETURN {return _toolButtons;}

    /*Tries to load all plugins in directory "where"*/
    static std::vector<Natron::LibraryBinary*> loadPlugins (const QString& where) WARN_UNUSED_RETURN;

    /*Tries to load all plugins in directory "where" that contains all the functions described by
 their name in "functions".*/
    static std::vector<Natron::LibraryBinary*> loadPluginsAndFindFunctions(const QString& where,
                                                                            const std::vector<std::string>& functions) WARN_UNUSED_RETURN;

    const Natron::Cache<Natron::FrameEntry>& getViewerCache() const WARN_UNUSED_RETURN {return *_viewerCache;}

    const Natron::Cache<Natron::Image>& getNodeCache() const WARN_UNUSED_RETURN {return *_nodeCache;}

    const KnobFactory& getKnobFactory() const WARN_UNUSED_RETURN {return *_knobFactory;}

    const Settings& getCurrentSettings() const {return *_settings;}

    PluginToolButton* findPluginToolButtonOrCreate(const QString& name,const QString& iconPath);

    static void printUsage();

public slots:

    void clearPlaybackCache();

    void clearDiskCache();

    void clearNodeCache();

    void clearExceedingEntriesFromNodeCache();

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
    void loadNodePlugins();

    /* Viewer,Reader,Writer...etc.*/
    void loadBuiltinNodePlugins();
    //////////////////////////////

    //////////////////////////////
    //// READ PLUGINS
    /*loads extra reader plug-ins */
    void loadReadPlugins();
    /*loads reads that are built-in*/
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

    void printPluginsLoaded();

    boost::scoped_ptr<Settings> _settings;

    std::map<int,AppInstance*> _appInstances;

    int _availableID;

    int _topLevelInstanceID;

    /*map< decoder name, pair< vector<file type decoded>, decoder library> >*/
    std::map< std::string,std::pair< std::vector<std::string> ,Natron::LibraryBinary*> > _readPluginsLoaded;

    /*map< encoder name, pair< vector<file type encoded>, encoder library> >*/
    std::map< std::string,std::pair< std::vector<std::string> ,Natron::LibraryBinary*> > _writePluginsLoaded;

    std::vector<Format*> _formats;

    typedef std::map<QString,std::pair<Natron::LibraryBinary*, QMutex*> > PluginsMap;
    PluginsMap _plugins; /*!< map of all plug-ins loaded + a global mutex for each plug-in*/

    boost::scoped_ptr<Natron::OfxHost> ofxHost;

    std::vector<PluginToolButton*> _toolButtons;

    boost::scoped_ptr<KnobFactory> _knobFactory;

    boost::scoped_ptr<Natron::Cache<Natron::Image> >  _nodeCache;

    boost::scoped_ptr<Natron::Cache<Natron::FrameEntry> > _viewerCache;

};

namespace Natron{

inline void errorDialog(const std::string& title,const std::string& message){
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->errorDialog(title,message);
    }else{
        std::cout << "ERROR: " << message << std::endl;
    }
    
}

inline void warningDialog(const std::string& title,const std::string& message){
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->warningDialog(title,message);
    }else{
        std::cout << "WARNING: "<< message << std::endl;
    }
}

inline void informationDialog(const std::string& title,const std::string& message){
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->informationDialog(title,message);
    }else{
        std::cout << "INFO: "<< message << std::endl;
    }
}

inline Natron::StandardButton questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons =
        Natron::StandardButtons(Natron::Yes | Natron::No),
                                              Natron::StandardButton defaultButton = Natron::NoButton){
    
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        return topLvlInstance->questionDialog(title,message,buttons,defaultButton);
    }else{
        std::cout << "QUESTION ASKED: " << message << std::endl;
        std::cout << NATRON_APPLICATION_NAME " answered yes." << std::endl;
        return Natron::Yes;
    }
}
} // namespace Natron


#endif // POWITER_GLOBAL_CONTROLER_H_

