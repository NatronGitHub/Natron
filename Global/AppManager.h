//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GLOBAL_APPMANAGER_H_
#define NATRON_GLOBAL_APPMANAGER_H_

#include <vector>
#include <string>
#include <map>

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

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
#include "Engine/KnobFactory.h"

/*macro to get the unique pointer to the controler*/
#define appPTR AppManager::instance()

class AppInstance;
class KnobGuiFactory;
class KnobHolder;
class NodeGui;
class ViewerInstance;
class ViewerTab;
class TabWidget;
class Gui;
class VideoEngine;
class QMutex;
class TimeLine;
class QFile;
class QTextStream;
class Settings;
class KnobSerialization;
struct KnobsClipBoard;
class ProcessHandler;
class SplashScreen;
class ProcessInputChannel;
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

    /** @brief Create a new node  in the node graph.
      * The name passed in parameter must match a valid node name,
      * otherwise an exception is thrown. You should encapsulate the call
      * by a try-catch block.
      * If the majorVersion is not -1 then this function will attempt to find a plugin with the matching
      * majorVersion, or otherwise it will throw an exception.
      * If the minorVersion is not -1 then this function will attempt to load a plugin with the greatest minorVersion
      * greater or equal to this minorVersion. 
      * By default this function also create the node's graphical user interface and attempts to automatically
      * connect this node to other nodes selected. 
      * If requestedByLoad is true then it will never attempt to do this auto-connection.
      * If openImageFileDialog is true then if the node has a file knob indicating an image file it will automatically
      * prompt the user with a file dialog.
     **/
    Natron::Node* createNode(const QString& name,int majorVersion = -1,int minorVersion = -1,
                             bool requestedByLoad = false,bool openImageFileDialog = true);

    /*Pointer to the GUI*/
    Gui* getGui() const WARN_UNUSED_RETURN {return _gui;}

    const std::vector<NodeGui*>& getVisibleNodes() const WARN_UNUSED_RETURN;

    void getActiveNodes(std::vector<Natron::Node *> *activeNodes) const;

    boost::shared_ptr<Natron::Project> getProject() const { return _currentProject;}

    boost::shared_ptr<TimeLine> getTimeLine() const WARN_UNUSED_RETURN ;

    /*true if the user is NOT scrubbing the timeline*/
    bool shouldRefreshPreview() const;

//////////
//////////////// PROJECT RELATED
/////////

    const QString& getCurrentProjectName() const WARN_UNUSED_RETURN ;

    const QString& getCurrentProjectPath() const WARN_UNUSED_RETURN ;

    int getProjectViewsCount() const;

    bool isAutoPreviewEnabled() const;

    void toggleAutoPreview();

    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN ;

    const Format& getProjectFormat() const WARN_UNUSED_RETURN ;

    bool isLoadingProject() const;

    bool loadProject(const QString& path,const QString& name);

    void saveProject(const QString& path,const QString& name,bool autoSave);

    void autoSave();

    void setOrAddProjectFormat(const Format& frmt,bool skipAdd = false);

    int getKnobsAge() const;

    void incrementKnobsAge();

    void clearNodes();

    bool isSaveUpToDate() const WARN_UNUSED_RETURN;

    static QString autoSavesDir() WARN_UNUSED_RETURN;

    const std::vector<Natron::Node*>& getCurrentNodes() const;

    void beginProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller);

    void stackEvaluateRequest(Natron::ValueChangedReason reason, KnobHolder* caller, Knob *k, bool isSignificant);

    void endProjectWideValueChanges(KnobHolder* caller);

    void lockProject();

    void unlockProject();

private:

    void resetCurrentProject();

    void removeAutoSaves() const;

    void loadProjectInternal(const QString& path,const QString& name);

    void saveProjectInternal(const QString& path,const QString& filename,bool autosave = false);

    /** @brief Attemps to find an autosave. If found one,prompts the user
     * whether he/she wants to load it. If something was loaded this function
     * returns true,otherwise false.
     **/
    bool findAutoSave() WARN_UNUSED_RETURN;

public:
/////////
///////////// END PROJECT RELATED
/////////

    void deselectAllNodes() const;


    ViewerTab* addNewViewerTab(ViewerInstance* node,TabWidget* where) WARN_UNUSED_RETURN;

    bool connect(int inputNumber,const std::string& inputName,Natron::Node* output);

    bool connect(int inputNumber,Natron::Node* input,Natron::Node* output);

    bool disconnect(Natron::Node* input,Natron::Node* output);

    void autoConnect(Natron::Node* target,Natron::Node* created);

    NodeGui* getNodeGui(Natron::Node* n) const WARN_UNUSED_RETURN;

    NodeGui* getNodeGui(const std::string& nodeName) const WARN_UNUSED_RETURN;

    Natron::Node* getNode(NodeGui* n) const WARN_UNUSED_RETURN;

    void connectViewersToViewerCache();

    void disconnectViewersFromViewerCache();

    void errorDialog(const std::string& title,const std::string& message) const;

    void warningDialog(const std::string& title,const std::string& message) const;

    void informationDialog(const std::string& title,const std::string& message) const;

    Natron::StandardButton questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons =
            Natron::StandardButtons(Natron::Yes | Natron::No),
                                          Natron::StandardButton defaultButton = Natron::NoButton) const WARN_UNUSED_RETURN;
    void notifyRenderFinished(Natron::OutputEffectInstance* writer);


public slots:

    /* The following methods are forwarded to the model */
    void checkViewersConnection();

    void redrawAllViewers();

    void setupViewersForViews(int viewsCount);

    void notifyViewersProjectFormatChanged(const Format& format);

    void setViewersCurrentView(int view);

    void triggerAutoSave();

    /*Used in background mode only*/
    void startWritersRendering(const QStringList& writers);

    void clearOpenFXPluginsCaches();

signals:

    void pluginsPopulated();


private:

    void startRenderingFullSequence(Natron::OutputEffectInstance* writer);

    Gui* _gui; // the view of the MVC pattern

    mutable QMutex _projectLock;
    boost::shared_ptr<Natron::Project> _currentProject;
    bool _isLoadingProject;
    bool _isSavingProject;

    int _appID;

    std::map<Natron::Node*,NodeGui*> _nodeMapping;

    bool _isBackground;

    QMutex _isQuittingMutex;
    bool _isQuitting;


    class ActiveBackgroundRender{


        bool _running;
        QWaitCondition _runningCond;
        QMutex _runningMutex;
        Natron::OutputEffectInstance* _writer;
    public:

        ActiveBackgroundRender(Natron::OutputEffectInstance* writer);

        virtual ~ActiveBackgroundRender(){}

        Natron::OutputEffectInstance* getWriter() const WARN_UNUSED_RETURN {return _writer;}

        void notifyFinished();

        void blockingRender();

    };
    QMutex _activeRenderersMutex;
    std::vector<ActiveBackgroundRender*> _activeRenderers;


};

class PluginToolButton{

    QString _id;
    QString _label;
    QString _iconPath;
    std::vector<PluginToolButton*> _children;
    PluginToolButton* _parent;

public:
    PluginToolButton(const QString& pluginID,
                     const QString& pluginLabel,
                     const QString& iconPath)
        :
          _id(pluginID)
        , _label(pluginLabel)
        , _iconPath(iconPath)
        , _children()
        , _parent(NULL)
    {

    }

    const QString& getID() const {return _id;}

    const QString& getLabel() const {return _label;}

    void setLabel(const QString& label) {_label = label;}

    const QString& getIconPath() const {return _iconPath;}

    void setIconPath(const QString& iconPath) {_iconPath = iconPath;}

    const std::vector<PluginToolButton*>& getChildren() const {return _children;}

    void tryAddChild(PluginToolButton* plugin);

    PluginToolButton* getParent() const WARN_UNUSED_RETURN {return _parent;}

    void setParent(PluginToolButton* parent) {_parent = parent;}

    bool hasParent() const {return _parent != NULL;}

};

namespace Natron{
class Plugin {

    Natron::LibraryBinary* _binary;
    QString _id;
    QString _label;
    QMutex* _lock;
    int _majorVersion;
    int _minorVersion;

public:

    Plugin():
        _binary(NULL)
      , _id()
      , _label()
      , _lock()
      , _majorVersion(0)
      , _minorVersion(0)
    {}

    Plugin(Natron::LibraryBinary* binary,
           const QString& id,
           const QString& label,
           QMutex* lock,
           int majorVersion,
           int minorVersion
           ):
        _binary(binary)
      , _id(id)
      , _label(label)
      , _lock(lock)
      , _majorVersion(majorVersion)
      , _minorVersion(minorVersion)
    {

    }

    ~Plugin();

    void setPluginID(const QString& id) { _id = id; }

    const QString& getPluginID() const { return _id; }

    void setPluginLabel(const QString& label){ _label = label; }

    const QString& getPluginLabel() const { return _label; }

    QMutex* getPluginLock() const WARN_UNUSED_RETURN { return _lock; }

    Natron::LibraryBinary* getLibraryBinary() const WARN_UNUSED_RETURN { return _binary; }
    
    int getMajorVersion() const { return _majorVersion; }
    
    int getMinorVersion() const { return _minorVersion; }

};
}

class AppManager : public QObject, public Singleton<AppManager>
{

    Q_OBJECT

public:


    typedef std::map< std::string,std::pair< std::vector<std::string> ,Natron::LibraryBinary*> >::iterator ReadPluginsIterator;
    typedef ReadPluginsIterator WritePluginsIterator;

    AppManager();

    virtual ~AppManager();
    
    ///called right away after the constructor by main.cpp
    void load(SplashScreen* splashScreen);
    
    bool isLoaded() const;
    
    void hideSplashScreen();

    const boost::scoped_ptr<Natron::OfxHost>& getOfxHost() const WARN_UNUSED_RETURN {return ofxHost;}

    AppInstance* newAppInstance(bool background,const QString& projectName = QString(),const QStringList& writers = QStringList());

    void registerAppInstance(AppInstance* app);

    AppInstance* getAppInstance(int appID) const WARN_UNUSED_RETURN;

    void removeInstance(int appID);

    void setAsTopLevelInstance(int appID);

    const std::map<int,AppInstance*>& getAppInstances() const WARN_UNUSED_RETURN;

    AppInstance* getTopLevelInstance () const WARN_UNUSED_RETURN;

    /*Return a list of the name of all nodes available currently in the software*/
    QStringList getNodeNameList() const WARN_UNUSED_RETURN;

    QMutex* getMutexForPlugin(const QString& pluginId) const WARN_UNUSED_RETURN;

    Natron::LibraryBinary* getPluginBinary(const QString& pluginId,int majorVersion,int minorVersion) const WARN_UNUSED_RETURN;

    /*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double pixel_aspect = 1.0) const WARN_UNUSED_RETURN;

    const std::vector<Format*>& getFormats() const WARN_UNUSED_RETURN {return _formats;}

    const std::vector<PluginToolButton*>& getPluginsToolButtons() const WARN_UNUSED_RETURN {return _toolButtons;}

    /*Tries to load all plugins in directory "where"*/
    static std::vector<Natron::LibraryBinary*> loadPlugins (const QString& where) WARN_UNUSED_RETURN;

    /*Tries to load all plugins in directory "where" that contains all the functions described by
 their name in "functions".*/
    static std::vector<Natron::LibraryBinary*> loadPluginsAndFindFunctions(const QString& where,
                                                                           const std::vector<std::string>& functions) WARN_UNUSED_RETURN;

    const Natron::Cache<Natron::FrameEntry>& getViewerCache() const WARN_UNUSED_RETURN {return *_viewerCache;}

    const Natron::Cache<Natron::Image>& getNodeCache() const WARN_UNUSED_RETURN {return *_nodeCache;}

    bool getImage(const Natron::ImageKey& key,boost::shared_ptr<Natron::Image>* returnValue) const;

    bool getTexture(const Natron::FrameKey& key,boost::shared_ptr<Natron::FrameEntry>* returnValue) const;

    void setApplicationsCachesMaximumMemoryPercent(double p);

    void setApplicationsCachesMaximumDiskSpace(unsigned long long size);

    void setPlaybackCacheMaximumSize(double p);

    void removeFromNodeCache(boost::shared_ptr<Natron::Image> image);

    void removeFromViewerCache(boost::shared_ptr<Natron::FrameEntry> texture);

    const KnobFactory& getKnobFactory() const WARN_UNUSED_RETURN {return *_knobFactory;}

    const KnobGuiFactory& getKnobGuiFactory() const WARN_UNUSED_RETURN {return *_knobGuiFactory;}

    PluginToolButton* findPluginToolButtonOrCreate(const QString& pluginID,const QString& name,const QString& iconPath);

    void getIcon(Natron::PixmapEnum e,QPixmap* pix) const;

    const QCursor& getColorPickerCursor() const WARN_UNUSED_RETURN { return *_colorPickerCursor; }

    boost::shared_ptr<Settings> getCurrentSettings() const WARN_UNUSED_RETURN {return _settings;}

    bool isInitialized() const WARN_UNUSED_RETURN { return _initialized; }

    void setKnobClipBoard(const KnobSerialization& s,bool copyAnimation);

    bool isClipBoardEmpty() const;

    void getKnobClipBoard(KnobSerialization* k,bool* copyAnimation) const;

    void initProcessInputChannel(const QString& mainProcessServerName);

    /**
     * @brief If the current process is a background process, then it will right the output pipe the
     * short message. Otherwise the longMessage is printed to stdout
     **/
    bool writeToOutputPipe(const QString& longMessage,const QString& shortMessage);

    void abortAnyProcessing();

    void setLoadingStatus(const QString& str);

    void updateAllRecentFileMenus();

public slots:

    void clearPlaybackCache();

    void clearDiskCache();

    void clearNodeCache();

    void clearExceedingEntriesFromNodeCache();

    void clearPluginsLoadedCache();

    void clearAllCaches();

    void addPluginToolButtons(const QStringList& groups,
                              const QString& pluginID,
                              const QString& pluginLabel,
                              const QString& pluginIconPath,
                              const QString& groupIconPath);
    /*Quit the app*/
    static void quit();

signals:

    void imageRemovedFromNodeCache(SequenceTime time);
    void imageRemovedFromViewerCache(SequenceTime time);

private:


    /*Loads all kind of plugins*/
    void loadAllPlugins();

    //////////////////////////////
    //// NODE PLUGINS
    void loadNodePlugins(std::map<std::string,std::vector<std::string> >* readersMap,
                     std::map<std::string,std::vector<std::string> >* writersMap);

    /* Viewer,Reader,Writer...etc.*/
    void loadBuiltinNodePlugins(std::map<std::string,std::vector<std::string> >* readersMap,
                            std::map<std::string,std::vector<std::string> >* writersMap);
    //////////////////////////////


    void loadBuiltinFormats();

    void printPluginsLoaded();

    void populateIcons();

    void createColorPickerCursor();

private:
    // FIXME: PIMPL
    std::map<int,AppInstance*> _appInstances;

    int _availableID;

    int _topLevelInstanceID;

    boost::shared_ptr<Settings> _settings;

    std::vector<Format*> _formats;

    std::vector<Natron::Plugin*> _plugins;

    boost::scoped_ptr<Natron::OfxHost> ofxHost;

    std::vector<PluginToolButton*> _toolButtons;

    boost::scoped_ptr<KnobFactory> _knobFactory;

    boost::scoped_ptr<KnobGuiFactory> _knobGuiFactory;

    boost::shared_ptr<Natron::Cache<Natron::Image> >  _nodeCache;

    boost::shared_ptr<Natron::Cache<Natron::FrameEntry> > _viewerCache;

    QCursor* _colorPickerCursor;

    bool _initialized;

    boost::scoped_ptr<KnobsClipBoard> _knobsClipBoard;

    ProcessInputChannel* _backgroundIPC; //< object used to communicate with the main app
                                         //if this app is background, see the ProcessInputChannel def
    SplashScreen* _splashScreen;
    bool _loaded;

};

namespace Natron{

void errorDialog(const std::string& title,const std::string& message);

void warningDialog(const std::string& title,const std::string& message);

void informationDialog(const std::string& title,const std::string& message);

Natron::StandardButton questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons =
        Natron::StandardButtons(Natron::Yes | Natron::No),
                                      Natron::StandardButton defaultButton = Natron::NoButton);

template <class K>
boost::shared_ptr<K> createKnob(KnobHolder  *holder, const std::string &description, int dimension = 1){
    return appPTR->getKnobFactory().createKnob<K>(holder,description,dimension);
}
    
inline bool getImageFromCache(const Natron::ImageKey& key,boost::shared_ptr<Natron::Image> *returnValue) {
    return appPTR->getImage(key, returnValue);
}
    
inline bool getTextureFromCache(const Natron::FrameKey& key,boost::shared_ptr<Natron::FrameEntry>* returnValue) {
    return appPTR->getTexture(key,returnValue);
}


    
} // namespace Natron


#endif // NATRON_GLOBAL_CONTROLER_H_

