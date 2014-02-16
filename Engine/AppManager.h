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


/*macro to get the unique pointer to the controler*/
#define appPTR AppManager::instance()

class QMutex;

class AppInstance;
class Format;
class Settings;
class KnobHolder;

namespace Natron {
    class Node;
    class EffectInstance;
    class LibraryBinary;
    class ImageKey;
    class FrameKey;
    class Image;
    class FrameEntry;
    class Plugin;
    class CacheSignalEmitter;
}

struct AppManagerPrivate;
class AppManager : public QObject , public boost::noncopyable
{

    Q_OBJECT

public:

    enum AppType {
        APP_BACKGROUND = 0, //< a background AppInstance which will not do anything but instantiate the class making it ready for use.
                            //this is used by the unit tests
        
        APP_BACKGROUND_AUTO_RUN, //< a background AppInstance that will launch a project and render it. If projectName is empty or
                                 //writers is empty, it doesn't make sense to call AppInstance with this parameter.
        
        APP_GUI //< a GUI AppInstance, the end-user can interact with it.
    };


    AppManager();

    virtual ~AppManager();
    
    static AppManager* instance() { return _instance; }
    
    virtual bool isBackground() const { return true; }
    
    AppManager::AppType getAppType() const;
    
    static void printBackGroundWelcomeMessage();
    
    ///called right away after the constructor by main.cpp
    /// @param binaryPath is used to locate auxiliary files, such as the OpenColorIO config files
    bool load(const QString& binaryPath,const QString& mainProcServerName,
              const QString& projectName = QString(),const QStringList& writers = QStringList());
    
    bool isLoaded() const;
    
    AppInstance* newAppInstance(const QString& projectName = QString(),const QStringList& writers = QStringList());
    
    virtual void hideSplashScreen() {}
    
    Natron::EffectInstance* createOFXEffect(const std::string& pluginID,Natron::Node* node) const;

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

    const std::vector<Format*>& getFormats() const WARN_UNUSED_RETURN;

    /*Tries to load all plugins in directory "where"*/
    static std::vector<Natron::LibraryBinary*> loadPlugins (const QString& where) WARN_UNUSED_RETURN;

    /*Tries to load all plugins in directory "where" that contains all the functions described by
 their name in "functions".*/
    static std::vector<Natron::LibraryBinary*> loadPluginsAndFindFunctions(const QString& where,
                                                                           const std::vector<std::string>& functions) WARN_UNUSED_RETURN;


    bool getImage(const Natron::ImageKey& key,boost::shared_ptr<Natron::Image>* returnValue) const;

    bool getTexture(const Natron::FrameKey& key,boost::shared_ptr<Natron::FrameEntry>* returnValue) const;

    U64 getCachesTotalMemorySize() const;

    Natron::CacheSignalEmitter* getOrActivateViewerCacheSignalEmitter() const;

    void setApplicationsCachesMaximumMemoryPercent(double p);

    void setApplicationsCachesMaximumDiskSpace(unsigned long long size);

    void setPlaybackCacheMaximumSize(double p);

    void removeFromNodeCache(boost::shared_ptr<Natron::Image> image);

    void removeFromViewerCache(boost::shared_ptr<Natron::FrameEntry> texture);

    boost::shared_ptr<Settings> getCurrentSettings() const WARN_UNUSED_RETURN;

    bool isInitialized() const WARN_UNUSED_RETURN;

    const KnobFactory& getKnobFactory() const WARN_UNUSED_RETURN;

    /**
     * @brief If the current process is a background process, then it will right the output pipe the
     * short message. Otherwise the longMessage is printed to stdout
     **/
    bool writeToOutputPipe(const QString& longMessage,const QString& shortMessage);

    void abortAnyProcessing();

    virtual void setLoadingStatus(const QString& str);

    /**
    * @brief Toggle on/off multi-threading globally in Natron
    **/
    void setMultiThreadEnabled(bool enabled);

    const QString& getApplicationBinaryPath() const;


public slots:

    void clearPlaybackCache();

    void clearDiskCache();

    void clearNodeCache();

    void clearExceedingEntriesFromNodeCache();

    void clearPluginsLoadedCache();

    void clearAllCaches();

    virtual void addPluginToolButtons(const QStringList& /*groups*/,
                              const QString& /*pluginID*/,
                              const QString& /*pluginLabel*/,
                              const QString& /*pluginIconPath*/,
                                      const QString& /*groupIconPath*/) {}
    

signals:

    void imageRemovedFromNodeCache(SequenceTime time);
    void imageRemovedFromViewerCache(SequenceTime time);

protected:

    virtual void loadExtra(){}

    virtual void loadBuiltinNodePlugins(std::vector<Natron::Plugin*>* plugins,
                                    std::map<std::string,std::vector<std::string> >* readersMap,
                                    std::map<std::string,std::vector<std::string> >* writersMap);

    virtual AppInstance* makeNewInstance(int appID) const;
private:
    

    void loadAllPlugins();

    ///called by loadAllPlugins
    void loadNodePlugins(std::map<std::string,std::vector<std::string> >* readersMap,
                         std::map<std::string,std::vector<std::string> >* writersMap);


    static AppManager *_instance;

    boost::scoped_ptr<AppManagerPrivate> _imp;
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

