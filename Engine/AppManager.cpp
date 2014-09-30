//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "AppManager.h"

#if defined(Q_OS_UNIX)
#include <sys/time.h>     // for getrlimit on linux
#include <sys/resource.h> // for getrlimit
#endif

#include <clocale>
#include <cstddef>
#include <QDebug>
#include <QAbstractSocket>
#include <QCoreApplication>

#include "Global/MemoryInfo.h"
#include "Global/QtCompat.h" // for removeRecursively
#include "Global/GlobalDefines.h" // for removeRecursively
#include "Global/Enums.h"

#include "Engine/AppInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/Settings.h"
#include "Engine/LibraryBinary.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/ViewerInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Image.h"
#include "Engine/FrameEntry.h"
#include "Engine/Format.h"
#include "Engine/Log.h"
#include "Engine/Cache.h"
#include "Engine/ChannelSet.h"
#include "Engine/Variant.h"
#include "Engine/Knob.h"
#include "Engine/Rect.h"
#include "Engine/NoOp.h"

BOOST_CLASS_EXPORT(Natron::FrameParams)
BOOST_CLASS_EXPORT(Natron::ImageParams)

using namespace Natron;

AppManager* AppManager::_instance = 0;
struct AppManagerPrivate
{
    AppManager::AppType _appType; //< the type of app
    std::map<int,AppInstanceRef> _appInstances; //< the instances mapped against their ID
    int _availableID; //< the ID for the next instance
    int _topLevelInstanceID; //< the top level app ID
    boost::shared_ptr<Settings> _settings; //< app settings
    std::vector<Format*> _formats; //<a list of the "base" formats available in the application
    std::vector<Natron::Plugin*> _plugins; //< list of the plugins
    boost::scoped_ptr<Natron::OfxHost> ofxHost; //< OpenFX host
    boost::scoped_ptr<KnobFactory> _knobFactory; //< knob maker
    boost::shared_ptr<Natron::Cache<Natron::Image> >  _nodeCache; //< Images cache
    boost::shared_ptr<Natron::Cache<Natron::FrameEntry> > _viewerCache; //< Viewer textures cache
    ProcessInputChannel* _backgroundIPC; //< object used to communicate with the main app
    //if this app is background, see the ProcessInputChannel def
    bool _loaded; //< true when the first instance is completly loaded.
    QString _binaryPath; //< the path to the application's binary
    mutable QMutex _wasAbortCalledMutex;
    bool _wasAbortAnyProcessingCalled; // < has abortAnyProcessing() called at least once ?
    U64 _nodesGlobalMemoryUse; //< how much memory all the nodes are using (besides the cache)
    mutable QMutex _ofxLogMutex;
    QString _ofxLog;
    size_t maxCacheFiles; //< the maximum number of files the application can open for caching. This is the hard limit * 0.9
    size_t currentCacheFilesCount; //< the number of cache files currently opened in the application
    mutable QMutex currentCacheFilesCountMutex; //< protects currentCacheFilesCount
    

    std::string currentOCIOConfigPath; //< the currentOCIO config path
    
    AppManagerPrivate()
        : _appType(AppManager::APP_BACKGROUND)
          , _appInstances()
          , _availableID(0)
          , _topLevelInstanceID(0)
          , _settings( new Settings(NULL) )
          , _formats()
          , _plugins()
          , ofxHost( new Natron::OfxHost() )
          , _knobFactory( new KnobFactory() )
          , _nodeCache()
          , _viewerCache()
          ,_backgroundIPC(0)
          ,_loaded(false)
          ,_binaryPath()
          ,_wasAbortAnyProcessingCalled(false)
          ,_nodesGlobalMemoryUse(0)
          ,_ofxLogMutex()
          ,_ofxLog()
          ,maxCacheFiles(0)
          ,currentCacheFilesCount(0)
          ,currentCacheFilesCountMutex()

    {
        setMaxCacheFiles();
    }

    void initProcessInputChannel(const QString & mainProcessServerName);

    void loadBuiltinFormats();

    void saveCaches();

    void restoreCaches();

    bool checkForCacheDiskStructure(const QString & cachePath);

    void cleanUpCacheDiskStructure(const QString & cachePath);

    /**
     * @brief Called on startup to initialize the max opened files
     **/
    void setMaxCacheFiles();
};

void
AppManager::printBackGroundWelcomeMessage()
{
    std::cout << "================================================================================" << std::endl;
    std::cout << NATRON_APPLICATION_NAME << "    " << QObject::tr(" version: ").toStdString() << NATRON_VERSION_STRING << std::endl;
    std::cout << QObject::tr(">>>Running in background mode (off-screen rendering only).<<<").toStdString() << std::endl;
    std::cout << QObject::tr("Please note that the background mode is in early stage and accepts only project files "
                             "that would produce a valid output from the graphical version of ").toStdString() << NATRON_APPLICATION_NAME << std::endl;
    std::cout << QObject::tr("If the background mode doesn't output any result, please adjust your project via the application interface "
                             "and then re-try using the background mode.").toStdString() << std::endl;
}

void
AppManager::printUsage(const std::string& programName)
{
    std::cout << NATRON_APPLICATION_NAME << QObject::tr(" usage: ").toStdString() << std::endl;
    std::cout << programName << QObject::tr("    <project file path>").toStdString() << std::endl;
    std::cout << QObject::tr("[--background] or [-b] enables background mode rendering. No graphical interface will be shown. "
                             "When using NatronRenderer this argument is implicit and you don't need to use it.")
    .toStdString() << std::endl;
    std::cout << QObject::tr("[--writer <Writer node name>] When in background mode, the renderer will only try to render with the node"
                             " name following the --writer argument. If no such node exists in the project file, the process will abort."
                             "Note that if you don't pass the --writer argument, it will try to start rendering with all the writers in the project's file.").toStdString() << std::endl;
}

bool
AppManager::parseCmdLineArgs(int argc,
                             char* argv[],
                             bool* isBackground,
                             QString & projectFilename,
                             QStringList & writers,
                             QString & mainProcessServerName)
{
    if (!argv) {
        return false;
    }

    *isBackground = false;
    bool expectWriterNameOnNextArg = false;
    bool expectPipeFileNameOnNextArg = false;
    QStringList args;
    for (int i = 0; i < argc; ++i) {
        args.push_back( QString(argv[i]) );
    }

    for (int i = 0; i < args.size(); ++i) {
        if ( args.at(i).contains("." NATRON_PROJECT_FILE_EXT) ) {
            if (expectWriterNameOnNextArg || expectPipeFileNameOnNextArg) {
                AppManager::printUsage(argv[0]);

                return false;
            }
            projectFilename = args.at(i);
            continue;
        } else if ( (args.at(i) == "--background") || (args.at(i) == "-b") ) {
            if (expectWriterNameOnNextArg  || expectPipeFileNameOnNextArg) {
                AppManager::printUsage(argv[0]);

                return false;
            }
            *isBackground = true;
            continue;
        } else if ( (args.at(i) == "--writer") || (args.at(i) == "-w") ) {
            if (expectWriterNameOnNextArg  || expectPipeFileNameOnNextArg) {
                AppManager::printUsage(argv[0]);

                return false;
            }
            expectWriterNameOnNextArg = true;
            continue;
        } else if (args.at(i) == "--IPCpipe") {
            if (expectWriterNameOnNextArg || expectPipeFileNameOnNextArg) {
                AppManager::printUsage(argv[0]);

                return false;
            }
            expectPipeFileNameOnNextArg = true;
            continue;
        }

        if (expectWriterNameOnNextArg) {
            assert(!expectPipeFileNameOnNextArg);
            writers << args.at(i);
            expectWriterNameOnNextArg = false;
        }
        if (expectPipeFileNameOnNextArg) {
            assert(!expectWriterNameOnNextArg);
            mainProcessServerName = args.at(i);
            expectPipeFileNameOnNextArg = false;
        }
    }

    return true;
} // parseCmdLineArgs

AppManager::AppManager()
    : QObject()
      , _imp( new AppManagerPrivate() )
{
    assert(!_instance);
    _instance = this;
}

bool
AppManager::load(int &argc,
                 char *argv[],
                 const QString & projectFilename,
                 const QStringList & writers,
                 const QString & mainProcessServerName)
{
    ///if the user didn't specify launch arguments (e.g unit testing)
    ///find out the binary path
    bool hadArgs = true;

    if (!argv) {
        QString binaryPath = QDir::currentPath();
        argc = 1;
        argv = new char*[1];
        argv[0] = new char[binaryPath.size() + 1];
        for (int i = 0; i < binaryPath.size(); ++i) {
            argv[0][i] = binaryPath.at(i).unicode();
        }
        argv[0][binaryPath.size()] = '\0';
        hadArgs = false;
    }
    initializeQApp(argc, argv);

    assert(argv);
    if (!hadArgs) {
        delete [] argv[0];
        delete [] argv;
    }

    ///the QCoreApplication must have been created so far.
    assert(qApp);

    return loadInternal(projectFilename,writers,mainProcessServerName);
}

AppManager::~AppManager()
{
    assert( _imp->_appInstances.empty() );

    for (U32 i = 0; i < _imp->_plugins.size(); ++i) {
        delete _imp->_plugins[i];
    }
    foreach(Format * f,_imp->_formats) {
        delete f;
    }


    if (_imp->_backgroundIPC) {
        delete _imp->_backgroundIPC;
    }

    _imp->saveCaches();


    _instance = 0;


    if (qApp) {
        delete qApp;
    }
}

void
AppManager::quit(AppInstance* instance)
{
    instance->aboutToQuit();
    std::map<int, AppInstanceRef>::iterator found = _imp->_appInstances.find( instance->getAppID() );
    assert( found != _imp->_appInstances.end() );
    found->second.status = APP_INACTIVE;
    ///if we exited the last instance, exit the event loop, this will make
    /// the exec() function return.
    if (_imp->_appInstances.size() == 1) {
        assert(qApp);
        qApp->quit();
    }
    delete instance;
}

void
AppManager::initializeQApp(int &argc,
                           char **argv)
{
    new QCoreApplication(argc,argv);
}

bool
AppManager::loadInternal(const QString & projectFilename,
                         const QStringList & writers,
                         const QString & mainProcessServerName)
{
    assert(!_imp->_loaded);

    _imp->_binaryPath = QCoreApplication::applicationDirPath();

    registerEngineMetaTypes();
    registerGuiMetaTypes();

    qApp->setOrganizationName(NATRON_ORGANIZATION_NAME);
    qApp->setOrganizationDomain(NATRON_ORGANIZATION_DOMAIN);
    qApp->setApplicationName(NATRON_APPLICATION_NAME);


    // Natron is not yet internationalized, so it is better for now to use the "C" locale,
    // until it is tested for robustness against locale choice.
    // The locale affects numerics printing and scanning, date and time.
    // Note that with other locales (e.g. "de" or "fr"), the floating-point numbers may have
    // a comma (",") as the decimal separator instead of a point (".").
    // There is also an OpenCOlorIO issue with non-C numeric locales:
    // https://github.com/imageworks/OpenColorIO/issues/297
    //
    // this must be done after initializing the QCoreApplication, see
    // https://qt-project.org/doc/qt-5/qcoreapplication.html#locale-settings
    std::setlocale(LC_NUMERIC,"C"); // set the locale for LC_NUMERIC only
    std::setlocale(LC_ALL,"C"); // set the locale for everything
    Natron::Log::instance(); //< enable logging

    _imp->_settings->initializeKnobsPublic();
    ///Call restore after initializing knobs
    _imp->_settings->restoreSettings();

    ///basically show a splashScreen
    initGui();


    size_t maxCacheRAM = _imp->_settings->getRamMaximumPercent() * getSystemTotalRAM();
    U64 maxDiskCache = _imp->_settings->getMaximumDiskCacheSize();
    U64 playbackSize = maxCacheRAM * _imp->_settings->getRamPlaybackMaximumPercent();

    U64 viewerCacheSize = maxDiskCache + playbackSize;
    _imp->_nodeCache.reset( new Cache<Image>("NodeCache",0x1, maxCacheRAM - playbackSize,1) );
    _imp->_viewerCache.reset( new Cache<FrameEntry>("ViewerCache",0x1,viewerCacheSize,(double)playbackSize / (double)viewerCacheSize) );

    setLoadingStatus( tr("Restoring the image cache...") );
    _imp->restoreCaches();

    setLoadingStatus( tr("Restoring user settings...") );


    ///Set host properties after restoring settings since it depends on the host name.
    _imp->ofxHost->setProperties();

    /*loading all plugins*/
    loadAllPlugins();
    _imp->loadBuiltinFormats();


    if ( isBackground() && !mainProcessServerName.isEmpty() ) {
        _imp->initProcessInputChannel(mainProcessServerName);
        printBackGroundWelcomeMessage();
    }


    if ( isBackground() ) {
        if ( !projectFilename.isEmpty() ) {
            if (!mainProcessServerName.isEmpty()) {
                _imp->_appType = APP_BACKGROUND_AUTO_RUN_LAUNCHED_FROM_GUI;
            } else {
                _imp->_appType = APP_BACKGROUND_AUTO_RUN;
            }
        } else {
            _imp->_appType = APP_BACKGROUND;
        }
    } else {
        _imp->_appType = APP_GUI;
    }

    AppInstance* mainInstance = newAppInstance(projectFilename,writers);

    hideSplashScreen();

    if (!mainInstance) {
        return false;
    } else {
        onLoadCompleted();

        ///In background project auto-run the rendering is finished at this point, just exit the instance
        if ( (_imp->_appType == APP_BACKGROUND_AUTO_RUN ||
              _imp->_appType == APP_BACKGROUND_AUTO_RUN_LAUNCHED_FROM_GUI) && mainInstance ) {
            mainInstance->quit();
        }

        return true;
    }
} // loadInternal

AppInstance*
AppManager::newAppInstance(const QString & projectName,
                           const QStringList & writers)
{
    AppInstance* instance = makeNewInstance(_imp->_availableID);

    try {
        instance->load(projectName,writers);
    } catch (const std::exception & e) {
        Natron::errorDialog( NATRON_APPLICATION_NAME,e.what() );
        removeInstance(_imp->_availableID);
        delete instance;

        return NULL;
    } catch (...) {
        Natron::errorDialog( NATRON_APPLICATION_NAME, tr("Cannot load project").toStdString() );
        removeInstance(_imp->_availableID);
        delete instance;

        return NULL;
    }

    ++_imp->_availableID;

    ///flag that we finished loading the Appmanager even if it was already true
    _imp->_loaded = true;

    return instance;
}

AppInstance*
AppManager::getAppInstance(int appID) const
{
    std::map<int,AppInstanceRef>::const_iterator it;

    it = _imp->_appInstances.find(appID);
    if ( it != _imp->_appInstances.end() ) {
        return it->second.app;
    } else {
        return NULL;
    }
}

const std::map<int,AppInstanceRef> &
AppManager::getAppInstances() const
{
    return _imp->_appInstances;
}

void
AppManager::removeInstance(int appID)
{
    _imp->_appInstances.erase(appID);
    if ( !_imp->_appInstances.empty() ) {
        setAsTopLevelInstance(_imp->_appInstances.begin()->first);
    }
}

AppManager::AppType
AppManager::getAppType() const
{
    return _imp->_appType;
}

void
AppManager::clearPlaybackCache()
{
    _imp->_viewerCache->clearInMemoryPortion();
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        it->second.app->clearViewersLastRenderedTexture();
    }
}

void
AppManager::clearDiskCache()
{
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        it->second.app->clearViewersLastRenderedTexture();
    }
    _imp->_viewerCache->clear();
}

void
AppManager::clearNodeCache()
{
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        it->second.app->clearAllLastRenderedImages();
    }
    _imp->_nodeCache->clear();
}

void
AppManager::clearPluginsLoadedCache()
{
    _imp->ofxHost->clearPluginsLoadedCache();
}

void
AppManager::clearAllCaches()
{
    clearDiskCache();
    clearNodeCache();

    ///for each app instance clear all its nodes cache
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        it->second.app->clearOpenFXPluginsCaches();
    }
}

std::vector<LibraryBinary*>
AppManager::loadPlugins(const QString &where)
{
    std::vector<LibraryBinary*> ret;
    QDir d(where);

    if ( d.isReadable() ) {
        QStringList filters;
        filters << QString( QString("*.") + QString(NATRON_LIBRARY_EXT) );
        d.setNameFilters(filters);
        QStringList fileList = d.entryList();
        for (int i = 0; i < fileList.size(); ++i) {
            QString filename = fileList.at(i);
            if ( filename.endsWith(".dll") || filename.endsWith(".dylib") || filename.endsWith(".so") ) {
                QString className;
                int index = filename.lastIndexOf("." NATRON_LIBRARY_EXT);
                className = filename.left(index);
                std::string binaryPath = NATRON_PLUGINS_PATH + className.toStdString() + "." + NATRON_LIBRARY_EXT;
                LibraryBinary* plugin = new LibraryBinary(binaryPath);
                if ( !plugin->isValid() ) {
                    delete plugin;
                } else {
                    ret.push_back(plugin);
                }
            } else {
                continue;
            }
        }
    }

    return ret;
}

std::vector<Natron::LibraryBinary*>
AppManager::loadPluginsAndFindFunctions(const QString & where,
                                        const std::vector<std::string> & functions)
{
    std::vector<LibraryBinary*> ret;
    std::vector<LibraryBinary*> loadedLibraries = loadPlugins(where);

    for (U32 i = 0; i < loadedLibraries.size(); ++i) {
        if ( loadedLibraries[i]->loadFunctions(functions) ) {
            ret.push_back(loadedLibraries[i]);
        }
    }

    return ret;
}

AppInstance*
AppManager::getTopLevelInstance () const
{
    std::map<int,AppInstanceRef>::const_iterator it = _imp->_appInstances.find(_imp->_topLevelInstanceID);

    if ( it == _imp->_appInstances.end() ) {
        return NULL;
    } else {
        return it->second.app;
    }
}

bool
AppManager::isLoaded() const
{
    return _imp->_loaded;
}

void
AppManagerPrivate::initProcessInputChannel(const QString & mainProcessServerName)
{
    _backgroundIPC = new ProcessInputChannel(mainProcessServerName);
}

bool
AppManager::hasAbortAnyProcessingBeenCalled() const
{
    QMutexLocker l(&_imp->_wasAbortCalledMutex);

    return _imp->_wasAbortAnyProcessingCalled;
}

void
AppManager::abortAnyProcessing()
{
    {
        QMutexLocker l(&_imp->_wasAbortCalledMutex);
        _imp->_wasAbortAnyProcessingCalled = true;
    }
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        std::vector<boost::shared_ptr<Natron::Node> > nodes;
        it->second.app->getActiveNodes(&nodes);
        for (U32 i = 0; i < nodes.size(); ++i) {
            nodes[i]->quitAnyProcessing();
        }
    }
}

bool
AppManager::writeToOutputPipe(const QString & longMessage,
                              const QString & shortMessage)
{
    if (!_imp->_backgroundIPC) {
        qDebug() << longMessage;

        return false;
    }
    _imp->_backgroundIPC->writeToOutputChannel(shortMessage);

    return true;
}

void
AppManager::registerAppInstance(AppInstance* app)
{
    AppInstanceRef ref;

    ref.app = app;
    ref.status = Natron::APP_ACTIVE;
    _imp->_appInstances.insert( std::make_pair(app->getAppID(),ref) );
}

void
AppManager::setApplicationsCachesMaximumMemoryPercent(double p)
{
    size_t maxCacheRAM = p * getSystemTotalRAM_conditionnally();
    U64 playbackSize = maxCacheRAM * _imp->_settings->getRamPlaybackMaximumPercent();

    _imp->_nodeCache->setMaximumCacheSize(maxCacheRAM - playbackSize);
    _imp->_nodeCache->setMaximumInMemorySize(1);
    U64 maxDiskCacheSize = _imp->_settings->getMaximumDiskCacheSize();
    _imp->_viewerCache->setMaximumInMemorySize( (double)playbackSize / (double)maxDiskCacheSize );
}

void
AppManager::setApplicationsCachesMaximumDiskSpace(unsigned long long size)
{
    size_t maxCacheRAM = _imp->_settings->getRamMaximumPercent() * getSystemTotalRAM_conditionnally();
    U64 playbackSize = maxCacheRAM * _imp->_settings->getRamPlaybackMaximumPercent();

    _imp->_viewerCache->setMaximumCacheSize(size);
    _imp->_viewerCache->setMaximumInMemorySize( (double)playbackSize / (double)size );
}

void
AppManager::setPlaybackCacheMaximumSize(double p)
{
    size_t maxCacheRAM = _imp->_settings->getRamMaximumPercent() * getSystemTotalRAM_conditionnally();
    U64 playbackSize = maxCacheRAM * p;

    _imp->_nodeCache->setMaximumCacheSize(maxCacheRAM - playbackSize);
    _imp->_nodeCache->setMaximumInMemorySize(1);
    U64 maxDiskCacheSize = _imp->_settings->getMaximumDiskCacheSize();
    _imp->_viewerCache->setMaximumInMemorySize( (double)playbackSize / (double)maxDiskCacheSize );
}

void
AppManager::loadAllPlugins()
{
    assert( _imp->_plugins.empty() );
    assert( _imp->_formats.empty() );


    std::map<std::string,std::vector<std::string> > readersMap;
    std::map<std::string,std::vector<std::string> > writersMap;

    /*loading node plugins*/

    loadBuiltinNodePlugins(&_imp->_plugins, &readersMap, &writersMap);

    /*loading ofx plugins*/
    _imp->ofxHost->loadOFXPlugins( &readersMap, &writersMap);

    _imp->_settings->populateReaderPluginsAndFormats(readersMap);
    _imp->_settings->populateWriterPluginsAndFormats(writersMap);

    onAllPluginsLoaded();
}

void
AppManager::loadBuiltinNodePlugins(std::vector<Natron::Plugin*>* plugins,
                                   std::map<std::string,std::vector<std::string> >* /*readersMap*/,
                                   std::map<std::string,std::vector<std::string> >* /*writersMap*/)
{
    {
        boost::shared_ptr<EffectInstance> dotNode( Dot::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&Dot::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);

        std::list<std::string> grouping;
        dotNode->getPluginGrouping(&grouping);
        QStringList qgrouping;

        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }

        Natron::Plugin* plugin = new Natron::Plugin( binary,dotNode->getPluginID().c_str(),dotNode->getPluginLabel().c_str(),
                                                     "","",qgrouping,NULL,dotNode->getMajorVersion(),dotNode->getMinorVersion() );
        plugins->push_back(plugin);


        onPluginLoaded(plugin);
    }
}

void
AppManager::registerPlugin(const QStringList & groups,
                           const QString & pluginID,
                           const QString & pluginLabel,
                           const QString & pluginIconPath,
                           const QString & groupIconPath,
                           Natron::LibraryBinary* binary,
                           bool mustCreateMutex,
                           int major,
                           int minor)
{
    QMutex* pluginMutex = 0;

    if (mustCreateMutex) {
        pluginMutex = new QMutex(QMutex::Recursive);
    }
    Natron::Plugin* plugin = new Natron::Plugin(binary,pluginID,pluginLabel,pluginIconPath,groupIconPath,groups,pluginMutex,major,minor);
    _imp->_plugins.push_back(plugin);
    onPluginLoaded(plugin);
}

void
AppManagerPrivate::loadBuiltinFormats()
{
    /*initializing list of all Formats available*/
    std::vector<std::string> formatNames;

    formatNames.push_back("PC_Video");
    formatNames.push_back("NTSC");
    formatNames.push_back("PAL");
    formatNames.push_back("HD");
    formatNames.push_back("NTSC_16:9");
    formatNames.push_back("PAL_16:9");
    formatNames.push_back("1K_Super_35(full-ap)");
    formatNames.push_back("1K_Cinemascope");
    formatNames.push_back("2K_Super_35(full-ap)");
    formatNames.push_back("2K_Cinemascope");
    formatNames.push_back("4K_Super_35(full-ap)");
    formatNames.push_back("4K_Cinemascope");
    formatNames.push_back("square_256");
    formatNames.push_back("square_512");
    formatNames.push_back("square_1K");
    formatNames.push_back("square_2K");

    std::vector< std::vector<double> > resolutions;
    std::vector<double> pcvideo; pcvideo.push_back(640); pcvideo.push_back(480); pcvideo.push_back(1);
    std::vector<double> ntsc; ntsc.push_back(720); ntsc.push_back(486); ntsc.push_back(0.91f);
    std::vector<double> pal; pal.push_back(720); pal.push_back(576); pal.push_back(1.09f);
    std::vector<double> hd; hd.push_back(1920); hd.push_back(1080); hd.push_back(1);
    std::vector<double> ntsc169; ntsc169.push_back(720); ntsc169.push_back(486); ntsc169.push_back(1.21f);
    std::vector<double> pal169; pal169.push_back(720); pal169.push_back(576); pal169.push_back(1.46f);
    std::vector<double> super351k; super351k.push_back(1024); super351k.push_back(778); super351k.push_back(1);
    std::vector<double> cine1k; cine1k.push_back(914); cine1k.push_back(778); cine1k.push_back(2);
    std::vector<double> super352k; super352k.push_back(2048); super352k.push_back(1556); super352k.push_back(1);
    std::vector<double> cine2K; cine2K.push_back(1828); cine2K.push_back(1556); cine2K.push_back(2);
    std::vector<double> super4K35; super4K35.push_back(4096); super4K35.push_back(3112); super4K35.push_back(1);
    std::vector<double> cine4K; cine4K.push_back(3656); cine4K.push_back(3112); cine4K.push_back(2);
    std::vector<double> square256; square256.push_back(256); square256.push_back(256); square256.push_back(1);
    std::vector<double> square512; square512.push_back(512); square512.push_back(512); square512.push_back(1);
    std::vector<double> square1K; square1K.push_back(1024); square1K.push_back(1024); square1K.push_back(1);
    std::vector<double> square2K; square2K.push_back(2048); square2K.push_back(2048); square2K.push_back(1);

    resolutions.push_back(pcvideo);
    resolutions.push_back(ntsc);
    resolutions.push_back(pal);
    resolutions.push_back(hd);
    resolutions.push_back(ntsc169);
    resolutions.push_back(pal169);
    resolutions.push_back(super351k);
    resolutions.push_back(cine1k);
    resolutions.push_back(super352k);
    resolutions.push_back(cine2K);
    resolutions.push_back(super4K35);
    resolutions.push_back(cine4K);
    resolutions.push_back(square256);
    resolutions.push_back(square512);
    resolutions.push_back(square1K);
    resolutions.push_back(square2K);

    assert( formatNames.size() == resolutions.size() );
    for (U32 i = 0; i < formatNames.size(); ++i) {
        const std::vector<double> & v = resolutions[i];
        assert(v.size() >= 3);
        Format* _frmt = new Format(0, 0, (int)v[0], (int)v[1], formatNames[i], v[2]);
        assert(_frmt);
        _formats.push_back(_frmt);
    }
} // loadBuiltinFormats

Format*
AppManager::findExistingFormat(int w,
                               int h,
                               double pixel_aspect) const
{
    for (U32 i = 0; i < _imp->_formats.size(); ++i) {
        Format* frmt = _imp->_formats[i];
        assert(frmt);
        if ( (frmt->width() == w) && (frmt->height() == h) && (frmt->getPixelAspect() == pixel_aspect) ) {
            return frmt;
        }
    }

    return NULL;
}

void
AppManager::setAsTopLevelInstance(int appID)
{
    if (_imp->_topLevelInstanceID == appID) {
        return;
    }
    _imp->_topLevelInstanceID = appID;
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin();
         it != _imp->_appInstances.end();
         ++it) {
        if (it->first != _imp->_topLevelInstanceID) {
            if ( !isBackground() ) {
                it->second.app->disconnectViewersFromViewerCache();
            }
        } else {
            if ( !isBackground() ) {
                it->second.app->connectViewersToViewerCache();
            }
        }
    }
}

void
AppManager::clearExceedingEntriesFromNodeCache()
{
    _imp->_nodeCache->clearExceedingEntries();
}

const std::vector<Natron::Plugin*> &
AppManager::getPluginsList() const
{
    return _imp->_plugins;
}

QMutex*
AppManager::getMutexForPlugin(const QString & pluginId) const
{
    for (U32 i = 0; i < _imp->_plugins.size(); ++i) {
        if (_imp->_plugins[i]->getPluginID() == pluginId) {
            return _imp->_plugins[i]->getPluginLock();
        }
    }
    std::string exc("Couldn't find a plugin named ");
    exc.append( pluginId.toStdString() );
    throw std::invalid_argument(exc);
}

const std::vector<Format*> &
AppManager::getFormats() const
{
    return _imp->_formats;
}

const KnobFactory &
AppManager::getKnobFactory() const
{
    return *(_imp->_knobFactory);
}

Natron::LibraryBinary*
AppManager::getPluginBinary(const QString & pluginId,
                            int majorVersion,
                            int minorVersion) const
{
    std::map<int,Natron::Plugin*> matches;

    for (U32 i = 0; i < _imp->_plugins.size(); ++i) {
        if (_imp->_plugins[i]->getPluginID() != pluginId) {
            continue;
        }
        if ( (majorVersion != -1) && (_imp->_plugins[i]->getMajorVersion() != majorVersion) ) {
            continue;
        }
        matches.insert( std::make_pair(_imp->_plugins[i]->getMinorVersion(),_imp->_plugins[i]) );
    }

    if ( matches.empty() ) {
        QString exc = QString("Couldn't find a plugin named %1, with a major version of %2 and a minor version greater or equal to %3.")
                      .arg(pluginId)
                      .arg(majorVersion)
                      .arg(minorVersion);
        throw std::invalid_argument( exc.toStdString() );
    } else {
        std::map<int,Natron::Plugin*>::iterator greatest = matches.end();
        --greatest;

        return greatest->second->getLibraryBinary();
    }
}

Natron::EffectInstance*
AppManager::createOFXEffect(const std::string & pluginID,
                            boost::shared_ptr<Natron::Node> node,
                            const NodeSerialization* serialization,
                            const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                            bool allowFileDialogs) const
{
    return _imp->ofxHost->createOfxEffect(pluginID, node,serialization,paramValues,allowFileDialogs);
}

void
AppManager::removeFromNodeCache(const boost::shared_ptr<Natron::Image> & image)
{
    _imp->_nodeCache->removeEntry(image);
}

void
AppManager::removeFromViewerCache(const boost::shared_ptr<Natron::FrameEntry> & texture)
{
    _imp->_viewerCache->removeEntry(texture);
    if (texture) {
        emit imageRemovedFromViewerCache( texture->getKey().getTime() );
    }
}

void
AppManager::removeAllImagesFromCacheWithMatchingKey(U64 treeVersion)
{
    std::list< boost::shared_ptr<Natron::Image> > cacheContent;

    _imp->_nodeCache->getCopy(&cacheContent);
    for (std::list< boost::shared_ptr<Natron::Image> >::iterator it = cacheContent.begin(); it != cacheContent.end(); ++it) {
        if ( (*it)->getKey().getTreeVersion() == treeVersion ) {
            _imp->_nodeCache->removeEntry(*it);
        }
    }
}

void
AppManager::removeAllTexturesFromCacheWithMatchingKey(U64 treeVersion)
{
    std::list< boost::shared_ptr<Natron::FrameEntry> > cacheContent;

    _imp->_viewerCache->getCopy(&cacheContent);
    for (std::list< boost::shared_ptr<Natron::FrameEntry> >::iterator it = cacheContent.begin(); it != cacheContent.end(); ++it) {
        if ( (*it)->getKey().getTreeVersion() == treeVersion ) {
            _imp->_viewerCache->removeEntry(*it);
        }
    }
}

const QString &
AppManager::getApplicationBinaryPath() const
{
    return _imp->_binaryPath;
}

void
AppManager::setNumberOfThreads(int threadsNb)
{
    _imp->_settings->setNumberOfThreads(threadsNb);
}

bool
AppManager::getImage(const Natron::ImageKey & key,
                     boost::shared_ptr<Natron::ImageParams>* params,
                     boost::shared_ptr<Natron::Image>* returnValue) const
{
    boost::shared_ptr<NonKeyParams> paramsBase;

#ifdef NATRON_LOG
    Log::beginFunction("AppManager","getImage");
    bool ret = _imp->_nodeCache->get(key,&paramsBase,returnValue);
    if (ret) {
        *params = boost::dynamic_pointer_cast<Natron::ImageParams>(paramsBase);
        Log::print("Image found in cache!");
    } else {
        Log::print("Image not found in cache!");
    }
    Log::endFunction("AppManager","getImage");

    return ret;
#else
    bool ret = _imp->_nodeCache->get(key,&paramsBase, returnValue);
    if (ret) {
        *params = boost::dynamic_pointer_cast<Natron::ImageParams>(paramsBase);
    }

    return ret;
#endif
}

bool
AppManager::getImageOrCreate(const Natron::ImageKey & key,
                             boost::shared_ptr<Natron::ImageParams> params,
                             ImageLocker* imageLocker,
                             boost::shared_ptr<Natron::Image>* returnValue) const
{
#ifdef NATRON_LOG
    Log::beginFunction("AppManager","getImage");
    bool ret = _imp->_nodeCache->getOrCreate(key,params,imageLocker,returnValue);
    if (ret) {
        Log::print("Image found in cache!");
    } else {
        Log::print("Image not found in cache!");
    }
    Log::endFunction("AppManager","getImage");

    return ret;
#else

    return _imp->_nodeCache->getOrCreate(key,params,imageLocker,returnValue);
#endif
}

bool
AppManager::getTexture(const Natron::FrameKey & key,
                       boost::shared_ptr<Natron::FrameParams>* params,
                       boost::shared_ptr<Natron::FrameEntry>* returnValue) const
{
    boost::shared_ptr<NonKeyParams> paramsBase;

#ifdef NATRON_LOG
    Log::beginFunction("AppManager","getTexture");
    bool ret = _imp->_viewerCache->get(key,paramsBase, returnValue);
    if (ret) {
        *params = boost::dynamic_pointer_cast<Natron::FrameParams>(paramsBase);
        Log::print("Texture found in cache!");
    } else {
        Log::print("Texture not found in cache!");
    }
    Log::endFunction("AppManager","getTexture");

    return ret;
#else
    bool ret =  _imp->_viewerCache->get(key, &paramsBase,returnValue);
    if (ret && params) {
        *params = boost::dynamic_pointer_cast<Natron::FrameParams>(paramsBase);
    }

    return ret;

#endif
}

bool
AppManager::getTextureOrCreate(const Natron::FrameKey & key,
                               boost::shared_ptr<Natron::FrameParams> params,
                               FrameEntryLocker* entryLocker,
                               boost::shared_ptr<Natron::FrameEntry>* returnValue) const
{
#ifdef NATRON_LOG
    Log::beginFunction("AppManager","getTexture");
    bool ret = _imp->_viewerCache->getOrCreate(key,params,entryLocker returnValue);
    if (ret) {
        Log::print("Texture found in cache!");
    } else {
        Log::print("Texture not found in cache!");
    }
    Log::endFunction("AppManager","getTexture");

    return ret;
#else

    return _imp->_viewerCache->getOrCreate(key, params,entryLocker, returnValue);
#endif
}

U64
AppManager::getCachesTotalMemorySize() const
{
    return _imp->_viewerCache->getMemoryCacheSize() + _imp->_nodeCache->getMemoryCacheSize();
}

Natron::CacheSignalEmitter*
AppManager::getOrActivateViewerCacheSignalEmitter() const
{
    return _imp->_viewerCache->activateSignalEmitter();
}

boost::shared_ptr<Settings> AppManager::getCurrentSettings() const
{
    return _imp->_settings;
}

void
AppManager::setLoadingStatus(const QString & str)
{
    if ( isLoaded() ) {
        return;
    }
    std::cout << str.toStdString() << std::endl;
}

AppInstance*
AppManager::makeNewInstance(int appID) const
{
    return new AppInstance(appID);
}

#if QT_VERSION < 0x050000
Q_DECLARE_METATYPE(QAbstractSocket::SocketState)
#endif

void
AppManager::registerEngineMetaTypes() const
{
    qRegisterMetaType<Variant>();
    qRegisterMetaType<Natron::ChannelSet>();
    qRegisterMetaType<Format>();
    qRegisterMetaType<SequenceTime>("SequenceTime");
    qRegisterMetaType<Natron::StandardButtons>();
    qRegisterMetaType<RectI>();
    qRegisterMetaType<RectD>();
#if QT_VERSION < 0x050000
    qRegisterMetaType<QAbstractSocket::SocketState>("SocketState");
#endif
}

void
AppManagerPrivate::saveCaches()
{
    {
        std::ofstream ofile;
        ofile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        std::string cacheRestoreFilePath = _viewerCache->getRestoreFilePath();
        try {
            ofile.open(cacheRestoreFilePath.c_str(),std::ofstream::out);
        } catch (const std::ofstream::failure & e) {
            qDebug() << "Exception occured when opening file " <<  cacheRestoreFilePath.c_str() << ": " << e.what();

            return;
        }

        if ( !ofile.good() ) {
            qDebug() << "Failed to save cache to " << cacheRestoreFilePath.c_str();

            return;
        }

        Natron::Cache<FrameEntry>::CacheTOC toc;
        _viewerCache->save(&toc);

        try {
            boost::archive::binary_oarchive oArchive(ofile);
            oArchive << toc;
        } catch (const std::exception & e) {
            qDebug() << "Failed to serialize the cache table of contents: " << e.what();
        }

        ofile.close();
    }


    {
        std::ofstream ofile;
        ofile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        std::string cacheRestoreFilePath = _nodeCache->getRestoreFilePath();
        try {
            ofile.open(cacheRestoreFilePath.c_str(),std::ofstream::out);
        } catch (const std::ofstream::failure & e) {
            qDebug() << "Exception occured when opening file " << cacheRestoreFilePath.c_str() << ": " << e.what();

            return;
        }

        if ( !ofile.good() ) {
            qDebug() << "Failed to save cache to " << cacheRestoreFilePath.c_str();

            return;
        }

        Natron::Cache<Image>::CacheTOC toc;
        _nodeCache->save(&toc);

        try {
            boost::archive::binary_oarchive oArchive(ofile);
            oArchive << toc;
            ofile.close();
        } catch (const std::exception & e) {
            qDebug() << "Failed to serialize the cache table of contents: " << e.what();
        }
    }
} // saveCaches

void
AppManagerPrivate::restoreCaches()
{
    {
        if ( checkForCacheDiskStructure( _nodeCache->getCachePath() ) ) {
            std::ifstream ifile;
            std::string settingsFilePath = _nodeCache->getRestoreFilePath();
            try {
                ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                ifile.open(settingsFilePath.c_str(),std::ifstream::in);
            } catch (const std::ifstream::failure & e) {
                qDebug() << "Failed to open the cache restoration file: " << e.what();

                return;
            }

            if ( !ifile.good() ) {
                qDebug() << "Failed to cache file for restoration: " <<  settingsFilePath.c_str();
                ifile.close();

                return;
            }

            Natron::Cache<Image>::CacheTOC tableOfContents;
            try {
                boost::archive::binary_iarchive iArchive(ifile);
                iArchive >> tableOfContents;
            } catch (const std::exception & e) {
                qDebug() << e.what();
                ifile.close();

                return;
            }

            ifile.close();

            QFile restoreFile( settingsFilePath.c_str() );
            restoreFile.remove();

            _nodeCache->restore(tableOfContents);
        }
    }
    {
        if ( checkForCacheDiskStructure( _viewerCache->getCachePath() ) ) {
            std::ifstream ifile;
            std::string settingsFilePath = _viewerCache->getRestoreFilePath();
            try {
                ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                ifile.open(settingsFilePath.c_str(),std::ifstream::in);
            } catch (const std::ifstream::failure & e) {
                qDebug() << "Failed to open the cache restoration file: " << e.what();

                return;
            }

            if ( !ifile.good() ) {
                qDebug() << "Failed to cache file for restoration: " <<  settingsFilePath.c_str();
                ifile.close();

                return;
            }

            Natron::Cache<FrameEntry>::CacheTOC tableOfContents;
            try {
                boost::archive::binary_iarchive iArchive(ifile);
                iArchive >> tableOfContents;
            } catch (const std::exception & e) {
                qDebug() << e.what();
                ifile.close();

                return;
            }

            ifile.close();

            QFile restoreFile( settingsFilePath.c_str() );
            restoreFile.remove();

            _viewerCache->restore(tableOfContents);
        }
    }
} // restoreCaches

bool
AppManagerPrivate::checkForCacheDiskStructure(const QString & cachePath)
{
    QString settingsFilePath(cachePath + QDir::separator() + "restoreFile." NATRON_CACHE_FILE_EXT);

    if ( !QFile::exists(settingsFilePath) ) {
        qDebug() << "Disk cache empty.";
        cleanUpCacheDiskStructure(cachePath);

        return false;
    }
    QDir directory(cachePath);
    QStringList files = directory.entryList(QDir::AllDirs);


    /*Now counting actual data files in the cache*/
    /*check if there's 256 subfolders, otherwise reset cache.*/
    int count = 0; // -1 because of the restoreFile
    int subFolderCount = 0;
    for (int i = 0; i < files.size(); ++i) {
        QString subFolder(cachePath);
        subFolder.append( QDir::separator() );
        subFolder.append(files[i]);
        if ( ( subFolder.right(1) == QString(".") ) || ( subFolder.right(2) == QString("..") ) ) {
            continue;
        }
        QDir d(subFolder);
        if ( d.exists() ) {
            ++subFolderCount;
            QStringList items = d.entryList();
            for (int j = 0; j < items.size(); ++j) {
                if ( ( items[j] != QString(".") ) && ( items[j] != QString("..") ) ) {
                    ++count;
                }
            }
        }
    }
    if (subFolderCount < 256) {
        qDebug() << cachePath << " doesn't contain sub-folders indexed from 00 to FF. Reseting.";
        cleanUpCacheDiskStructure(cachePath);

        return false;
    }

    return true;
}

void
AppManagerPrivate::cleanUpCacheDiskStructure(const QString & cachePath)
{
    /*re-create cache*/

    QDir cacheFolder(cachePath);

#   if QT_VERSION < 0x050000
    removeRecursively(cachePath);
#   else
    if ( cacheFolder.exists() ) {
        cacheFolder.removeRecursively();
    }
#endif
    cacheFolder.mkpath(".");

    QStringList etr = cacheFolder.entryList(QDir::NoDotAndDotDot);
    // if not 256 subdirs, we re-create the cache
    if (etr.size() < 256) {
        foreach(QString e, etr) {
            cacheFolder.rmdir(e);
        }
    }
    for (U32 i = 0x00; i <= 0xF; ++i) {
        for (U32 j = 0x00; j <= 0xF; ++j) {
            std::ostringstream oss;
            oss << std::hex <<  i;
            oss << std::hex << j;
            std::string str = oss.str();
            cacheFolder.mkdir( str.c_str() );
        }
    }
}

void
AppManagerPrivate::setMaxCacheFiles()
{
    /*Default to something reasonnable if the code below would happen to not work for some reason*/
    size_t hardMax = NATRON_MAX_CACHE_FILES_OPENED;

#if defined(Q_OS_UNIX) && defined(RLIMIT_NOFILE)
    /*
       Avoid 'Too many open files' on Unix.

       Increase the number of file descriptors that the process can open to the maximum allowed.
       - By default, Mac OS X only allows 256 file descriptors, which can easily be reached.
       - On Linux, the default limit is usually 1024.
     */
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        if (rl.rlim_max > rl.rlim_cur) {
            rl.rlim_cur = rl.rlim_max;
            if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
#             if defined(__APPLE__) && defined(OPEN_MAX)
                // On Mac OS X, setrlimit(RLIMIT_NOFILE, &rl) fails to set
                // rlim_cur above OPEN_MAX even if rlim_max > OPEN_MAX.
                if (rl.rlim_cur > OPEN_MAX) {
                    rl.rlim_cur = OPEN_MAX;
                    hardMax = rl.rlim_cur;
                    setrlimit(RLIMIT_NOFILE, &rl);
                }
#             endif
            } else {
                hardMax = rl.rlim_cur;
            }
        }
    }
//#elif defined(Q_OS_WIN)
    // The following code sets the limit for stdio-based calls only.
    // Note that low-level calls (CreateFile(), WriteFile(), ReadFile(), CloseHandle()...) are not affected by this limit.
    // References:
    // - http://msdn.microsoft.com/en-us/library/6e3b887c.aspx
    // - https://stackoverflow.com/questions/870173/is-there-a-limit-on-number-of-open-files-in-windows/4276338
    // - http://bugs.mysql.com/bug.php?id=24509
    //_setmaxstdio(2048); // sets the limit for stdio-based calls
    // On Windows there seems to be no limit at all. The following test program can prove it:
    /*
       #include <windows.h>
       int
       main(int argc,
         char *argv[])
       {
        const int maxFiles = 10000;

        std::list<HANDLE> files;

        for (int i = 0; i < maxFiles; ++i) {
            std::stringstream ss;
            ss << "C:/Users/Lex/Documents/GitHub/Natron/App/win32/debug/testMaxFiles/file" << i ;
            std::string filename = ss.str();
            HANDLE file_handle = ::CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE,
                                              0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);


            if (file_handle != INVALID_HANDLE_VALUE) {
                files.push_back(file_handle);
                std::cout << "Good files so far: " << files.size() << std::endl;

            } else {
                char* message ;
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,GetLastError(),0,(LPSTR)&message,0,NULL);
                std::cout << "Failed to open " << filename << ": " << message << std::endl;
                LocalFree(message);
            }
        }
        std::cout << "Total opened files: " << files.size() << std::endl;
        for (std::list<HANDLE>::iterator it = files.begin(); it!= files.end();++it) {
            CloseHandle(*it);
        }
       }
     */
#endif

    maxCacheFiles = hardMax * 0.9;
}

bool
AppManager::isNCacheFilesOpenedCapped() const
{
    QMutexLocker l(&_imp->currentCacheFilesCountMutex);

    return _imp->currentCacheFilesCount >= _imp->maxCacheFiles;
}

size_t
AppManager::getNCacheFilesOpened() const
{
    QMutexLocker l(&_imp->currentCacheFilesCountMutex);

    return _imp->currentCacheFilesCount;
}

void
AppManager::increaseNCacheFilesOpened()
{
    QMutexLocker l(&_imp->currentCacheFilesCountMutex);

    ++_imp->currentCacheFilesCount;
#ifdef DEBUG
    if (_imp->currentCacheFilesCount > _imp->maxCacheFiles) {
        qDebug() << "Cache has more files opened than the limit allowed: " << _imp->currentCacheFilesCount << " / " << _imp->maxCacheFiles;
    }
#endif
#ifdef NATRON_DEBUG_CACHE
    qDebug() << "N Cache Files Opened: " << _imp->currentCacheFilesCount;
#endif
}

void
AppManager::decreaseNCacheFilesOpened()
{
    QMutexLocker l(&_imp->currentCacheFilesCountMutex);

    --_imp->currentCacheFilesCount;
#ifdef NATRON_DEBUG_CACHE
    qDebug() << "NFiles Opened: " << _imp->currentCacheFilesCount;
#endif
}

void
AppManager::onMaxPanelsOpenedChanged(int maxPanels)
{
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        it->second.app->onMaxPanelsOpenedChanged(maxPanels);
    }
}

int
AppManager::exec()
{
    return qApp->exec();
}

void
AppManager::onNodeMemoryRegistered(qint64 mem)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    if ( ( (qint64)_imp->_nodesGlobalMemoryUse + mem ) < 0 ) {
        qDebug() << "Memory underflow...a node is trying to release more memory than it registered.";
        _imp->_nodesGlobalMemoryUse = 0;

        return;
    }

    _imp->_nodesGlobalMemoryUse += mem;
}

qint64
AppManager::getTotalNodesMemoryRegistered() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->_nodesGlobalMemoryUse;
}

QString
AppManager::getOfxLog_mt_safe() const
{
    QMutexLocker l(&_imp->_ofxLogMutex);

    return _imp->_ofxLog;
}

void
AppManager::writeToOfxLog_mt_safe(const QString & str)
{
    QMutexLocker l(&_imp->_ofxLogMutex);

    _imp->_ofxLog.append(str + '\n');
}

void
AppManager::clearOfxLog_mt_safe()
{
    QMutexLocker l(&_imp->_ofxLogMutex);
    _imp->_ofxLog.clear();
}

void
AppManager::exitApp()
{
    const std::map<int,AppInstanceRef> & instances = getAppInstances();

    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        it->second.app->quit();
    }
}

#ifdef Q_OS_UNIX
QString
AppManager::qt_tildeExpansion(const QString &path,
                              bool *expanded)
{
    if (expanded != 0) {
        *expanded = false;
    }
    if ( !path.startsWith( QLatin1Char('~') ) ) {
        return path;
    }
    QString ret = path;
    QStringList tokens = ret.split( QDir::separator() );
    if ( tokens.first() == QLatin1String("~") ) {
        ret.replace( 0, 1, QDir::homePath() );
    } /*else {
         QString userName = tokens.first();
         userName.remove(0, 1);

         const QString homePath = QString::fro#if defined(Q_OS_VXWORKS)
         const QString homePath = QDir::homePath();
         #elif defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD)
         passwd pw;
         passwd *tmpPw;
         char buf[200];
         const int bufSize = sizeof(buf);
         int err = 0;
         #if defined(Q_OS_SOLARIS) && (_POSIX_C_SOURCE - 0 < 199506L)
         tmpPw = getpwnam_r(userName.toLocal8Bit().constData(), &pw, buf, bufSize);
         #else
         err = getpwnam_r(userName.toLocal8Bit().constData(), &pw, buf, bufSize, &tmpPw);
         #endif
         if (err || !tmpPw)
         return ret;mLocal8Bit(pw.pw_dir);
         #else
         passwd *pw = getpwnam(userName.toLocal8Bit().constData());
         if (!pw)
         return ret;
         const QString homePath = QString::fromLocal8Bit(pw->pw_dir);
         #endif
         ret.replace(0, tokens.first().length(), homePath);
         }*/
    if (expanded != 0) {
        *expanded = true;
    }

    return ret;
}

#endif


void
AppManager::checkCacheFreeMemoryIsGoodEnough()
{
    ///Before allocating the memory check that there's enough space to fit in memory
    size_t systemRAMToKeepFree = getSystemTotalRAM() * appPTR->getCurrentSettings()->getUnreachableRamPercent();
    size_t totalFreeRAM = getAmountFreePhysicalRAM();
    
    double playbackRAMPercent = appPTR->getCurrentSettings()->getRamPlaybackMaximumPercent();
    while (totalFreeRAM <= systemRAMToKeepFree) {
        
        size_t nodeCacheSize = _imp->_nodeCache->getMemoryCacheSize();
        size_t viewerRamCacheSize = _imp->_viewerCache->getMemoryCacheSize();
        
        ///If the viewer cache represents more memory than the node cache, clear some of the viewer cache
        if (nodeCacheSize == 0 || (viewerRamCacheSize / (double)nodeCacheSize) > playbackRAMPercent) {
#ifdef NATRON_DEBUG_CACHE
            qDebug() << "Total system free RAM is below the threshold: " << printAsRAM(totalFreeRAM)
            << ", clearing last recently used ViewerCache texture...";
#endif
            
            
            if (! _imp->_viewerCache->evictLRUInMemoryEntry()) {
                break;
            }
            
            
        } else {
#ifdef NATRON_DEBUG_CACHE
            qDebug() << "Total system free RAM is below the threshold: " << printAsRAM(totalFreeRAM)
            << ", clearing last recently used NodeCache image...";
#endif
            if (! _imp->_nodeCache->evictLRUInMemoryEntry()) {
                break;
            }
        }
        
        totalFreeRAM = getAmountFreePhysicalRAM();
    }

}

void
AppManager::onOCIOConfigPathChanged(const std::string& path)
{
    _imp->currentOCIOConfigPath = path;
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin() ; it != _imp->_appInstances.end(); ++it) {
        it->second.app->onOCIOConfigPathChanged(path);
    }
}

const std::string&
AppManager::getOCIOConfigPath() const
{
    return _imp->currentOCIOConfigPath;
}

namespace Natron {
void
errorDialog(const std::string & title,
            const std::string & message)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if ( !appPTR->isBackground() ) {
        topLvlInstance->errorDialog(title,message);
    } else {
        std::cout << "ERROR: " << title << " :" <<  message << std::endl;
    }

#ifdef NATRON_LOG
    Log::beginFunction(title,"ERROR");
    Log::print(message);
    Log::endFunction(title,"ERROR");
#endif
}

void
warningDialog(const std::string & title,
              const std::string & message)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if ( !appPTR->isBackground() ) {
        topLvlInstance->warningDialog(title,message);
    } else {
        std::cout << "WARNING: " << title << " :" << message << std::endl;
    }
#ifdef NATRON_LOG
    Log::beginFunction(title,"WARNING");
    Log::print(message);
    Log::endFunction(title,"WARNING");
#endif
}

void
informationDialog(const std::string & title,
                  const std::string & message)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if ( !appPTR->isBackground() ) {
        topLvlInstance->informationDialog(title,message);
    } else {
        std::cout << "INFO: " << title << " :" << message << std::endl;
    }
#ifdef NATRON_LOG
    Log::beginFunction(title,"INFO");
    Log::print(message);
    Log::endFunction(title,"INFO");
#endif
}

Natron::StandardButton
questionDialog(const std::string & title,
               const std::string & message,
               Natron::StandardButtons buttons,
               Natron::StandardButton defaultButton)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if ( !appPTR->isBackground() ) {
        return topLvlInstance->questionDialog(title,message,buttons,defaultButton);
    } else {
        std::cout << "QUESTION ASKED: " << title << " :" << message << std::endl;
        std::cout << NATRON_APPLICATION_NAME " answered yes." << std::endl;

        return Natron::Yes;
    }
#ifdef NATRON_LOG
    Log::beginFunction(title,"QUESTION");
    Log::print(message);
    Log::endFunction(title,"QUESTION");
#endif
}
} //Namespace Natron
