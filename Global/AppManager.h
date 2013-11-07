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
class Writer;
class ViewerTab;
class TabWidget;
class Gui;
class VideoEngine;
class QMutex;
class OutputNodeInstance;
class TimeLine;
namespace Powiter {
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
    AppInstance(bool backgroundMode,int appID,const QString& projectName = QString());
    
    ~AppInstance();
    
    int getAppID() const {return _appID;}
    
    bool isBackground() const {return _isBackground;}
    
    /*Create a new node  in the node graph.
     The name passed in parameter must match a valid node name,
     otherwise an exception is thrown. You should encapsulate the call
     by a try-catch block.*/
    Powiter::Node* createNode(const QString& name,bool requestedByLoad = false);
    
    /*Pointer to the GUI*/
    Gui* getGui() WARN_UNUSED_RETURN {return _gui;}

    const std::vector<NodeGui*>& getAllActiveNodes() const WARN_UNUSED_RETURN;

    const QString& getCurrentProjectName() const WARN_UNUSED_RETURN ;

    const QString& getCurrentProjectPath() const WARN_UNUSED_RETURN ;

    int getCurrentProjectViewsCount() const;

    boost::shared_ptr<Powiter::Project> getProject() const { return _currentProject;}

    boost::shared_ptr<TimeLine> getTimeLine() const WARN_UNUSED_RETURN ;

    void setCurrentProjectName(const QString& name) ;

    bool shouldAutoSetProjectFormat() const ;

    void setAutoSetProjectFormat(bool b);

    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN ;

    const Format& getProjectFormat() const WARN_UNUSED_RETURN ;

    void loadProject(const QString& path,const QString& name);

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


    bool connect(int inputNumber,const std::string& inputName,Powiter::Node* output);

    bool connect(int inputNumber,Powiter::Node* input,Powiter::Node* output);

    bool disconnect(Powiter::Node* input,Powiter::Node* output);

    void autoConnect(Powiter::Node* target,Powiter::Node* created);

    NodeGui* getNodeGui(Powiter::Node* n) const WARN_UNUSED_RETURN;

    Powiter::Node* getNode(NodeGui* n) const WARN_UNUSED_RETURN;


    void connectViewersToViewerCache();

    void disconnectViewersFromViewerCache();

    QMutex* getAutoSaveMutex() const WARN_UNUSED_RETURN {return _autoSaveMutex;}

    void errorDialog(const std::string& title,const std::string& message) const;

    void warningDialog(const std::string& title,const std::string& message) const;

    void informationDialog(const std::string& title,const std::string& message) const;

    Powiter::StandardButton questionDialog(const std::string& title,const std::string& message,Powiter::StandardButtons buttons =
            Powiter::StandardButtons(Powiter::Yes | Powiter::No),
                                           Powiter::StandardButton defaultButton = Powiter::NoButton) const WARN_UNUSED_RETURN;



public slots:

    /* The following methods are forwarded to the model */
    void checkViewersConnection();

    void setupViewersForViews(int viewsCount);

    void notifyViewersProjectFormatChanged(const Format& format);

    void setViewersCurrentView(int view);

    void triggerAutoSave();

    void onRenderingOnDiskStarted(Writer* writer,const QString& sequenceName,int firstFrame,int lastFrame);


signals:

    void pluginsPopulated();

private:


    void removeAutoSaves() const;

    /*Attemps to find an autosave. If found one,prompts the user
 whether he/she wants to load it. If something was loaded this function
 returns true,otherwise false.*/
    bool findAutoSave() WARN_UNUSED_RETURN;



    Gui* _gui; // the view of the MVC pattern

    boost::shared_ptr<Powiter::Project> _currentProject;

    int _appID;

    std::map<Powiter::Node*,NodeGui*> _nodeMapping;

    QMutex* _autoSaveMutex;

    bool _isBackground;

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
    
    const boost::scoped_ptr<Powiter::OfxHost>& getOfxHost() const WARN_UNUSED_RETURN {return ofxHost;}

    AppInstance* newAppInstance(bool background,const QString& projectName = QString());

    void registerAppInstance(AppInstance* app){ _appInstances.insert(std::make_pair(app->getAppID(),app));}

    AppInstance* getAppInstance(int appID) const;

    void removeInstance(int appID);

    void setAsTopLevelInstance(int appID);

    AppInstance* getTopLevelInstance () const WARN_UNUSED_RETURN;

    /*Return a list of the name of all nodes available currently in the software*/
    QStringList getNodeNameList() const WARN_UNUSED_RETURN;

    QMutex* getMutexForPlugin(const QString& pluginName) const;

    Powiter::LibraryBinary* getPluginBinary(const QString& pluginName) const;

    /*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double pixel_aspect = 1.0) const WARN_UNUSED_RETURN;

    const std::vector<Format*>& getFormats() const {return _formats;}

    const std::vector<PluginToolButton*>& getPluginsToolButtons() const WARN_UNUSED_RETURN {return _toolButtons;}

    /*Tries to load all plugins in directory "where"*/
    static std::vector<Powiter::LibraryBinary*> loadPlugins (const QString& where) WARN_UNUSED_RETURN;

    /*Tries to load all plugins in directory "where" that contains all the functions described by
 their name in "functions".*/
    static std::vector<Powiter::LibraryBinary*> loadPluginsAndFindFunctions(const QString& where,
                                                                            const std::vector<std::string>& functions) WARN_UNUSED_RETURN;

    const Powiter::Cache<Powiter::FrameEntry>& getViewerCache() const WARN_UNUSED_RETURN {return *_viewerCache;}

    const Powiter::Cache<Powiter::Image>& getNodeCache() const WARN_UNUSED_RETURN {return *_nodeCache;}

    const KnobFactory& getKnobFactory() const WARN_UNUSED_RETURN {return *_knobFactory;}


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


    void printPluginsLoaded();

    std::map<int,AppInstance*> _appInstances;

    int _availableID;

    int _topLevelInstanceID;

    /*map< decoder name, pair< vector<file type decoded>, decoder library> >*/
    std::map< std::string,std::pair< std::vector<std::string> ,Powiter::LibraryBinary*> > _readPluginsLoaded;

    /*map< encoder name, pair< vector<file type encoded>, encoder library> >*/
    std::map< std::string,std::pair< std::vector<std::string> ,Powiter::LibraryBinary*> > _writePluginsLoaded;

    std::vector<Format*> _formats;

    typedef std::map<QString,std::pair<Powiter::LibraryBinary*, QMutex*> > PluginsMap;
    PluginsMap _plugins; /*!< map of all plug-ins loaded + a global mutex for each plug-in*/

    boost::scoped_ptr<Powiter::OfxHost> ofxHost;

    std::vector<PluginToolButton*> _toolButtons;

    boost::scoped_ptr<KnobFactory> _knobFactory;

    boost::scoped_ptr<Powiter::Cache<Powiter::Image> >  _nodeCache;

    boost::scoped_ptr<Powiter::Cache<Powiter::FrameEntry> > _viewerCache;

};

namespace Powiter{

inline void errorDialog(const std::string& title,const std::string& message){
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->errorDialog(title,message);
    }else{
        std::cout << title << std::endl;
        std::cout << message << std::endl;
    }
    
}

inline void warningDialog(const std::string& title,const std::string& message){
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->warningDialog(title,message);
    }else{
        std::cout << title << std::endl;
        std::cout << message << std::endl;
    }
}

inline void informationDialog(const std::string& title,const std::string& message){
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->informationDialog(title,message);
    }else{
        std::cout << title << std::endl;
        std::cout << message << std::endl;
    }
}

inline Powiter::StandardButton questionDialog(const std::string& title,const std::string& message,Powiter::StandardButtons buttons =
        Powiter::StandardButtons(Powiter::Yes | Powiter::No),
                                              Powiter::StandardButton defaultButton = Powiter::NoButton){
    
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        return topLvlInstance->questionDialog(title,message,buttons,defaultButton);
    }else{
        std::cout << title << std::endl;
        std::cout << message << std::endl;
        return Powiter::Yes;
    }
}
} // namespace Powiter


#endif // POWITER_GLOBAL_CONTROLER_H_

