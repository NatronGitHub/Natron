//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GLOBAL_APPMANAGER_H_
#define NATRON_GLOBAL_APPMANAGER_H_

#include <list>
#include <string>
#include "Global/GlobalDefines.h"
CLANG_DIAG_OFF(deprecated)
// /usr/include/qt5/QtCore/qgenericatomic.h:177:13: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QStringList>

#ifndef Q_MOC_RUN
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#endif

#include "Engine/KnobFactory.h"
#include "Engine/ImageLocker.h"

/*macro to get the unique pointer to the controler*/
#define appPTR AppManager::instance()

class QMutex;

class AppInstance;
class Format;
class Settings;
class KnobHolder;
class NodeSerialization;
class KnobSerialization;

namespace Natron {
class Node;
class EffectInstance;
class LibraryBinary;
class ImageKey;
class FrameKey;
class Image;
class ImageParams;
class FrameParams;
class FrameEntry;
class Plugin;
class CacheSignalEmitter;

enum AppInstanceStatus
{
    APP_INACTIVE = 0,     //< the app has not been loaded yet or has been closed already
    APP_ACTIVE     //< the app is active and can be used
};
}

struct AppInstanceRef
{
    AppInstance* app;
    Natron::AppInstanceStatus status;
};

struct AppManagerPrivate;
class AppManager
    : public QObject, public boost::noncopyable
{
    Q_OBJECT

public:

    enum AppType
    {
        APP_BACKGROUND = 0, //< a background AppInstance which will not do anything but instantiate the class making it ready for use.
                            //this is used by the unit tests

        APP_BACKGROUND_AUTO_RUN, //< a background AppInstance that will launch a project and render it. If projectName is empty or
                                 //writers is empty, it doesn't make sense to call AppInstance with this parameter.
        
        APP_BACKGROUND_AUTO_RUN_LAUNCHED_FROM_GUI, //same as APP_BACKGROUND_AUTO_RUN but a bg process launched by GUI of a main process

        APP_GUI //< a GUI AppInstance, the end-user can interact with it.
    };


    AppManager();

    /**
     * @brief This function initializes the QCoreApplication (or QApplication) object and the AppManager object (load plugins etc...)
     * It must be called right away after the constructor. It cannot be put in the constructor as this function relies on RTTI infos.
     * @param argc The number of arguments passed to the main function.
     * @param argv An array of strings passed to the main function.
     * @param projectFilename The project absolute filename that the application's main instance should try to load.
     * @param writers A list of the writers the project should use to render. This is only meaningful for background applications.
     * If empty all writers in the project will be rendered.
     * @param mainProcessServerName The name of the main process named pipe so the background application can communicate with the
     * main process.
     **/
    bool load( int &argc, char **argv, const QString & projectFilename,
               const QStringList & writers,
               const std::list<std::pair<int,int> >& frameRanges,
               const QString & mainProcessServerName  );

    virtual ~AppManager();

    static AppManager* instance()
    {
        return _instance;
    }

    virtual bool isBackground() const
    {
        return true;
    }

    AppManager::AppType getAppType() const;
    static void printBackGroundWelcomeMessage();
    static void printUsage(const std::string& programName);

    bool isLoaded() const;

    AppInstance* newAppInstance( const QString & projectName,
                                const QStringList & writers,
                                const std::list<std::pair<int,int> >& frameRanges);
    virtual void hideSplashScreen()
    {
    }

    Natron::EffectInstance* createOFXEffect(const std::string & pluginID,boost::shared_ptr<Natron::Node> node,
                                            const NodeSerialization* serialization,
                                            const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                                            bool allowFileDialogs) const;

    void registerAppInstance(AppInstance* app);

    AppInstance* getAppInstance(int appID) const WARN_UNUSED_RETURN;

    void removeInstance(int appID);

    void setAsTopLevelInstance(int appID);

    const std::map<int,AppInstanceRef> & getAppInstances() const WARN_UNUSED_RETURN;
    AppInstance* getTopLevelInstance () const WARN_UNUSED_RETURN;
    const std::vector<Natron::Plugin*> & getPluginsList() const WARN_UNUSED_RETURN;
    QMutex* getMutexForPlugin(const QString & pluginId) const WARN_UNUSED_RETURN;
    Natron::LibraryBinary* getPluginBinary(const QString & pluginId,int majorVersion,int minorVersion) const WARN_UNUSED_RETURN;

    /*Find a builtin format with the same resolution and aspect ratio*/
    Format* findExistingFormat(int w, int h, double par = 1.0) const WARN_UNUSED_RETURN;
    const std::vector<Format*> & getFormats() const WARN_UNUSED_RETURN;

    /*Tries to load all plugins in directory "where"*/
    static std::vector<Natron::LibraryBinary*> loadPlugins (const QString & where) WARN_UNUSED_RETURN;

    /*Tries to load all plugins in directory "where" that contains all the functions described by
       their name in "functions".*/
    static std::vector<Natron::LibraryBinary*> loadPluginsAndFindFunctions(const QString & where,
                                                                           const std::vector<std::string> & functions) WARN_UNUSED_RETURN;


    /**
     * @brief Attempts to load an image from cache, returns true if it could find a matching image, false otherwise.
     **/
    bool getImage(const Natron::ImageKey & key,std::list<boost::shared_ptr<Natron::Image> >* returnValue) const;

    /**
     * @brief Same as getImage, but if it couldn't find a matching image in the cache, it will create one with the given parameters.
     **/
    bool getImageOrCreate(const Natron::ImageKey & key,const boost::shared_ptr<Natron::ImageParams>& params,
                          ImageLocker* imageLocker,
                          std::list<boost::shared_ptr<Natron::Image> >* returnValue) const;
    
    void createImageInCache(const Natron::ImageKey & key,const boost::shared_ptr<Natron::ImageParams>& params,
                          ImageLocker* imageLocker,
                          boost::shared_ptr<Natron::Image>* returnValue) const;

    bool getTexture(const Natron::FrameKey & key,
                    boost::shared_ptr<Natron::FrameEntry>* returnValue) const;

    bool getTextureOrCreate(const Natron::FrameKey & key,const boost::shared_ptr<Natron::FrameParams>& params,
                            FrameEntryLocker* entryLocker,
                            boost::shared_ptr<Natron::FrameEntry>* returnValue) const;

    U64 getCachesTotalMemorySize() const;

    Natron::CacheSignalEmitter* getOrActivateViewerCacheSignalEmitter() const;

    void setApplicationsCachesMaximumMemoryPercent(double p);

    void setApplicationsCachesMaximumDiskSpace(unsigned long long size);

    void setPlaybackCacheMaximumSize(double p);

    void removeFromNodeCache(const boost::shared_ptr<Natron::Image> & image);
    void removeFromViewerCache(const boost::shared_ptr<Natron::FrameEntry> & texture);
    
    void removeFromNodeCache(U64 hash);
    void removeFromViewerCache(U64 hash);
    /**
     * @brief Given the following tree version, removes all images from the node cache with a matching
     * tree version. This is useful to wipe the cache for one particular node.
     **/
    void  removeAllImagesFromCacheWithMatchingKey(U64 treeVersion);
    void  removeAllTexturesFromCacheWithMatchingKey(U64 treeVersion);

    boost::shared_ptr<Settings> getCurrentSettings() const WARN_UNUSED_RETURN;
    const KnobFactory & getKnobFactory() const WARN_UNUSED_RETURN;

    /**
     * @brief If the current process is a background process, then it will right the output pipe the
     * short message. Otherwise the longMessage is printed to stdout
     **/
    bool writeToOutputPipe(const QString & longMessage,const QString & shortMessage);

    void abortAnyProcessing();

    bool hasAbortAnyProcessingBeenCalled() const;

    virtual void setLoadingStatus(const QString & str);

  

    const QString & getApplicationBinaryPath() const;
    static bool parseCmdLineArgs(int argc,char* argv[],
                                 bool* isBackground,
                                 QString & projectFilename,
                                 QStringList & writers,
                                 std::list<std::pair<int,int> >& frameRanges,
                                 QString & mainProcessServerName);

    /**
     * @brief Called when the instance is exited
     **/
    void quit(AppInstance* instance);

    /**
     * @brief Starts the event loop and doesn't return until
     * quit() is called on all AppInstance's
     **/
    int exec();

    virtual void setUndoRedoStackLimit(int /*limit*/)
    {
    }

    QString getOfxLog_mt_safe() const;

    void writeToOfxLog_mt_safe(const QString & str);
    
    void clearOfxLog_mt_safe();
    
    virtual void showOfxLog() {}

    virtual void debugImage(const Natron::Image* /*image*/,
                            const QString & /*filename = QString()*/) const
    {
    }

    void registerPlugin(const QStringList & groups,
                        const QString & pluginID,
                        const QString & pluginLabel,
                        const QString & pluginIconPath,
                        const QString & groupIconPath,
                        Natron::LibraryBinary* binary,
                        bool mustCreateMutex,
                        int major,
                        int minor);

    bool isNCacheFilesOpenedCapped() const;
    size_t getNCacheFilesOpened() const;
    void increaseNCacheFilesOpened();
    void decreaseNCacheFilesOpened();

    /**
     * @brief Called by the caches to check that there's enough free memory on the computer to perform the allocation.
     * WARNING: This functin may remove some entries from the caches.
     **/
    void checkCacheFreeMemoryIsGoodEnough();
    
    void onCheckerboardSettingsChanged() { emit checkerboardSettingsChanged(); }
    
    void onOCIOConfigPathChanged(const std::string& path);
    ///Non MT-safe!
    const std::string& getOCIOConfigPath() const;


    int getHardwareIdealThreadCount();
    
    
    /**
     * @brief Toggle on/off multi-threading globally in Natron
     **/
    void setNumberOfThreads(int threadsNb);
    
    /**
     * @brief The value held by the Number of render threads settings.
     * It is stored it for faster access (1 mutex instead of 3 read/write locks)
     *
     * WARNING: This has nothing to do with the setNumberOfThreads function!
     * This function is just called by the Settings so that the getNThreadsSettings
     * function is cheap (no need to pass by the Knob API), whereas the 
     * setNumberOfThreads function actually set the value of the Knob in the settings!
     **/
    void setNThreadsToRender(int nThreads);
    void setNThreadsPerEffect(int nThreadsPerEffect);
    void setUseThreadPool(bool useThreadPool);
    
    void getNThreadsSettings(int* nThreadsToRender,int* nThreadsPerEffect) const;
    bool getUseThreadPool() const;
    
    /**
     * @brief Updates the global runningThreadsCount maintained across the whole application
     **/
    void fetchAndAddNRunningThreads(int nThreads);
    
    /**
     * @brief Returns the number of threads that were launched by Natron for rendering.
     * This sums up threads launched by the multi-thread suite and threads launched for 
     * parallel rendering.
     **/
    int getNRunningThreads() const;
    
    void setThreadAsActionCaller(bool actionCaller);

    /**
     * @brief Returns a list of IDs of all the plug-ins currently loaded.
     * Each ID can be passed to the AppInstance::createNode function to instantiate a node
     * with a plug-in.
     **/
    std::list<std::string> getPluginIDs() const;
    
public slots:
    

    ///Closes the application not saving any projects.
    virtual void exitApp();

    void clearPlaybackCache();

    void clearDiskCache();

    void clearNodeCache();

    void clearExceedingEntriesFromNodeCache();

    void clearPluginsLoadedCache();

    void clearAllCaches();

    void onNodeMemoryRegistered(qint64 mem);

    qint64 getTotalNodesMemoryRegistered() const;

    void onMaxPanelsOpenedChanged(int maxPanels);


#ifdef Q_OS_UNIX
    static QString qt_tildeExpansion(const QString &path, bool *expanded = 0);
#endif

signals:


    void checkerboardSettingsChanged();
protected:

    virtual void initGui()
    {
    }

    virtual void loadBuiltinNodePlugins(std::vector<Natron::Plugin*>* plugins,
                                        std::map<std::string,std::vector< std::pair<std::string,double> > >* readersMap,
                                        std::map<std::string,std::vector< std::pair<std::string,double> > >* writersMap);
    virtual AppInstance* makeNewInstance(int appID) const;
    virtual void registerGuiMetaTypes() const
    {
    }

    virtual void initializeQApp(int &argc,char** argv);
    virtual void onLoadCompleted()
    {
    }

    virtual void onPluginLoaded(Natron::Plugin* /*plugin*/)
    {
    }

    virtual void onAllPluginsLoaded()
    {
    }
    
    virtual void clearLastRenderedTextures() {}

private:


    bool loadInternal(const QString & projectFilename,
                      const QStringList & writers,
                      const std::list<std::pair<int,int> >& frameRanges,
                      const QString & mainProcessServerName);

    void registerEngineMetaTypes() const;

    void loadAllPlugins();

    static AppManager *_instance;
    boost::scoped_ptr<AppManagerPrivate> _imp;
};

namespace Natron {
void errorDialog(const std::string & title,const std::string & message);

void warningDialog(const std::string & title,const std::string & message);

void informationDialog(const std::string & title,const std::string & message);

Natron::StandardButton questionDialog(const std::string & title,const std::string & message,Natron::StandardButtons buttons =
                                          Natron::StandardButtons(Natron::Yes | Natron::No),
                                      Natron::StandardButton defaultButton = Natron::NoButton);

template <class K>
boost::shared_ptr<K> createKnob(KnobHolder*  holder,
                                const std::string &description,
                                int dimension = 1,
                                bool declaredByPlugin = true)
{
    return appPTR->getKnobFactory().createKnob<K>(holder,description,dimension,declaredByPlugin);
}

inline bool
getImageFromCache(const Natron::ImageKey & key,
                  std::list<boost::shared_ptr<Natron::Image> >* returnValue)
{
    return appPTR->getImage(key, returnValue);
}

inline bool
getImageFromCacheOrCreate(const Natron::ImageKey & key,
                          const boost::shared_ptr<Natron::ImageParams>& params,
                          ImageLocker* imageLocker,
                          std::list<boost::shared_ptr<Natron::Image> >* returnValue)
{
    return appPTR->getImageOrCreate(key,params, imageLocker, returnValue);
}
    
inline void createImageInCache(const Natron::ImageKey & key,const boost::shared_ptr<Natron::ImageParams>& params,
                                  ImageLocker* imageLocker,
                                  boost::shared_ptr<Natron::Image>* returnValue) 
{
    appPTR->createImageInCache(key, params, imageLocker, returnValue);
}

inline bool
getTextureFromCache(const Natron::FrameKey & key,
                    boost::shared_ptr<Natron::FrameEntry>* returnValue)
{
    return appPTR->getTexture(key,returnValue);
}

inline bool
getTextureFromCacheOrCreate(const Natron::FrameKey & key,
                            const boost::shared_ptr<Natron::FrameParams> &params,
                            FrameEntryLocker* entryLocker,
                            boost::shared_ptr<Natron::FrameEntry>* returnValue)
{
    return appPTR->getTextureOrCreate(key,params,entryLocker, returnValue);
}
    
/**
* @brief Returns a list of IDs of all the plug-ins currently loaded.
* Each ID can be passed to the AppInstance::createNode function to instantiate a node
* with a plug-in.
**/
inline std::list<std::string> getPluginIDs()
{
    return appPTR->getPluginIDs();
}
    
} // namespace Natron


#endif // NATRON_GLOBAL_CONTROLER_H_

