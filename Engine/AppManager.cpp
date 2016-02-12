/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AppManager.h"
#include "AppManagerPrivate.h"

#include <clocale>
#include <csignal>
#include <cstddef>
#include <cassert>
#include <stdexcept>

#if defined(Q_OS_LINUX)
#include <sys/signal.h>
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <ucontext.h>
#include <execinfo.h>
#endif

#ifdef Q_OS_UNIX
#include <stdio.h>
#include <stdlib.h>
#ifdef Q_OS_MAC
#include <sys/sysctl.h>
#include <libproc.h>
#endif
#endif

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTextCodec>
#include <QtCore/QCoreApplication>
#include <QtCore/QSettings>
#include <QtCore/QThreadPool>
#include <QtCore/QTextStream>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>


#include "Global/ProcInfo.h"


#include "Engine/AppInstance.h"
#include "Engine/Backdrop.h"
#include "Engine/CLArgs.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Dot.h"
#include "Engine/ExistenceCheckThread.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Log.h"
#include "Engine/Node.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/ProcessHandler.h" // ProcessInputChannel
#include "Engine/Project.h"
#include "Engine/PrecompNode.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoSmear.h"
#include "Engine/StandardPaths.h"
#include "Engine/ViewerInstance.h" // RenderStatsMap

#if QT_VERSION < 0x050000
Q_DECLARE_METATYPE(QAbstractSocket::SocketState)
#endif

NATRON_NAMESPACE_ENTER;

AppManager* AppManager::_instance = 0;


#ifdef __NATRON_LINUX__

//namespace  {
static void
handleShutDownSignal( int /*signalId*/ )
{
    if (appPTR) {
        std::cerr << "\nCaught termination signal, exiting!" << std::endl;
        appPTR->quitApplication();
    }
}

static void
setShutDownSignal(int signalId)
{
#if defined(__NATRON_UNIX__)
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handleShutDownSignal;
    if (sigaction(signalId, &sa, NULL) == -1) {
        std::perror("setting up termination signal");
        std::exit(1);
    }
#else
    std::signal(signalId, handleShutDownSignal);
#endif
}
#endif


#if defined(__NATRON_LINUX__)

#define NATRON_UNIX_BACKTRACE_STACK_DEPTH 16

static void
backTraceSigSegvHandler(int sig, siginfo_t *info,
                   void *secret) {
    
    void *trace[NATRON_UNIX_BACKTRACE_STACK_DEPTH];
    char **messages = (char **)NULL;
    int i, trace_size = 0;
    ucontext_t *uc = (ucontext_t *)secret;
    
    /* Do something useful with siginfo_t */
    if (sig == SIGSEGV) {
        QThread* curThread = QThread::currentThread();
        std::string threadName;
        if (curThread) {
            threadName = (qApp && qApp->thread() == curThread) ? "Main" : curThread->objectName().toStdString();
        }
        std::cerr << "Caught segmentation fault (SIGSEGV) from thread "  << threadName << "(" << curThread << "), faulty address is " <<
             #ifndef __x86_64__
                     (void*)uc->uc_mcontext.gregs[REG_EIP]
             #else
                     (void*)uc->uc_mcontext.gregs[REG_RIP]
             #endif
                     << " from " << info->si_addr << std::endl;
    } else {
        printf("Got signal %d#92;n", sig);
    }
    
    trace_size = backtrace(trace, NATRON_UNIX_BACKTRACE_STACK_DEPTH);
    /* overwrite sigaction with caller's address */
#ifndef __x86_64__
       trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
#else
       trace[1] = (void *) uc->uc_mcontext.gregs[REG_RIP];
#endif

    
    messages = backtrace_symbols(trace, trace_size);
    /* skip first stack frame (points here) */
    std::cerr << "Backtrace:" << std::endl;
    for (i = 1; i < trace_size; ++i) {
        std::cerr << "[Frame " << i << "]: " << messages[i] << std::endl;
    }
    exit(1);
}

static void
setSigSegvSignal()
{
    struct sigaction sa;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    /* if SA_SIGINFO is set, sa_sigaction is to be used instead of sa_handler. */
    sa.sa_sigaction = backTraceSigSegvHandler;
   
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        std::perror("setting up sigsegv signal");
        std::exit(1);
    }
}
#endif

//} // anon namespace

void
AppManager::saveCaches() const
{
    _imp->saveCaches();
}

int
AppManager::getHardwareIdealThreadCount()
{
    return _imp->idealThreadCount;
}



AppManager::AppManager()
    : QObject()
    , _imp( new AppManagerPrivate() )
{
    assert(!_instance);
    _instance = this;

    QObject::connect(this, SIGNAL(s_requestOFXDialogOnMainThread(OfxImageEffectInstance*, void*)), this, SLOT(onOFXDialogOnMainThreadReceived(OfxImageEffectInstance*, void*)));
}

void
AppManager::takeNatronGIL()
{
    _imp->natronPythonGIL.lock();
}

void
AppManager::releaseNatronGIL()
{
    _imp->natronPythonGIL.unlock();
}



bool
AppManager::load(int &argc,
                 char *argv[],
                 const CLArgs& cl)
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
    

    // set fontconfig path on all platforms
    if (qgetenv("FONTCONFIG_PATH").isNull()) {
        // set FONTCONFIG_PATH to Natron/Resources/etc/fonts (required by plugins using fontconfig)
        QString path = QCoreApplication::applicationDirPath() + "/../Resources/etc/fonts";
        QString pathcfg = path + "/fonts.conf";
        if (!QFile(pathcfg).exists()) {
            qWarning() << "Fontconfig configuration file" << pathcfg << "does not exist, not setting FONTCONFIG_PATH";
        } else {
            qDebug() << "Setting FONTCONFIG_PATH to" << path;
            qputenv("FONTCONFIG_PATH", path.toUtf8());
        }
    }

    try {
        initPython(argc, argv);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }

    _imp->idealThreadCount = QThread::idealThreadCount();
    QThreadPool::globalInstance()->setExpiryTimeout(-1); //< make threads never exit on their own
    //otherwise it might crash with thread local storage

#if QT_VERSION < 0x050000
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif

    assert(argv);

    ///the QCoreApplication must have been created so far.
    assert(qApp);

    bool ret = loadInternal(cl);
    if (!hadArgs) {
        delete [] argv[0];
        delete [] argv;
    }
    return ret;
}

AppManager::~AppManager()
{
    
#ifdef NATRON_USE_BREAKPAD
    if (_imp->breakpadAliveThread) {
        _imp->breakpadAliveThread->quitThread();
    }
#endif
    
    bool appsEmpty;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        appsEmpty = _imp->_appInstances.empty();
    }
    while (!appsEmpty) {
        AppInstance* front = 0;
        {
            QMutexLocker k(&_imp->_appInstancesMutex);
            front = _imp->_appInstances.begin()->second.app;
        }
        if (front) {
            front->quit();
        }
        {
            QMutexLocker k(&_imp->_appInstancesMutex);
            appsEmpty = _imp->_appInstances.empty();
        }
        
    }
    
    for (PluginsMap::iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        for (PluginMajorsOrdered::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            delete *it2;
        }
    }

    Q_FOREACH (Format * f, _imp->_formats) {
        delete f;
    }

    delete _imp->_backgroundIPC;

    try {
        _imp->saveCaches();
    } catch (std::runtime_error) {
        // ignore errors
    }
    


    ///Caches may have launched some threads to delete images, wait for them to be done
    QThreadPool::globalInstance()->waitForDone();
    
    ///Kill caches now because decreaseNCacheFilesOpened can be called
    _imp->_nodeCache->waitForDeleterThread();
    _imp->_diskCache->waitForDeleterThread();
    _imp->_viewerCache->waitForDeleterThread();
    _imp->_nodeCache.reset();
    _imp->_viewerCache.reset();
    _imp->_diskCache.reset();
    
    tearDownPython();
    
    _instance = 0;

    delete qApp;
}



void
AppManager::quit(AppInstance* instance)
{
    instance->aboutToQuit();
    
    int nbApps;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        std::map<int, AppInstanceRef>::iterator found = _imp->_appInstances.find( instance->getAppID() );
        assert( found != _imp->_appInstances.end() );
        found->second.status = eAppInstanceStatusInactive;
        nbApps = (int)_imp->_appInstances.size();
    }
    ///if we exited the last instance, exit the event loop, this will make
    /// the exec() function return.
    if (nbApps == 1) {
        assert(qApp);
        qApp->quit();
    }
    delete instance;
}

void
AppManager::quitApplication()
{
    bool appsEmpty;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        appsEmpty = _imp->_appInstances.empty();
    }
    while (!appsEmpty) {
        AppInstance* app = 0;
        {
            QMutexLocker k(&_imp->_appInstancesMutex);
            app = _imp->_appInstances.begin()->second.app;
        }
        if (app) {
            quit(app);
        }
        
        {
            QMutexLocker k(&_imp->_appInstancesMutex);
            appsEmpty = _imp->_appInstances.empty();
        }
    }
}

void
AppManager::initializeQApp(int &argc,
                           char **argv)
{
    new QCoreApplication(argc,argv);
}


bool
AppManager::loadInternal(const CLArgs& cl)
{
    assert(!_imp->_loaded);

    _imp->_binaryPath = QCoreApplication::applicationDirPath();

    registerEngineMetaTypes();
    registerGuiMetaTypes();

    qApp->setOrganizationName(NATRON_ORGANIZATION_NAME);
    qApp->setOrganizationDomain(NATRON_ORGANIZATION_DOMAIN);
    qApp->setApplicationName(NATRON_APPLICATION_NAME);

    //Set it once setApplicationName is set since it relies on it
    _imp->diskCachesLocation = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache) ;

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

    // Set the C and C++ locales
    // see http://en.cppreference.com/w/cpp/locale/locale/global
    // Maybe this can also workaround the OSX crash in loadlocale():
    // https://discussions.apple.com/thread/3479591
    // https://github.com/cth103/dcpomatic/blob/master/src/lib/safe_stringstream.h
    // stringstreams don't seem to be thread-safe on OSX because the change the locale.

    // We also set explicitely the LC_NUMERIC locale to "C" to avoid juggling
    // between locales when using stringstreams.
    // See function __convert_from_v(...) in
    // /usr/include/c++/4.2.1/x86_64-apple-darwin10/bits/c++locale.h
    // https://www.opensource.apple.com/source/libstdcxx/libstdcxx-104.1/include/c++/4.2.1/bits/c++locale.h
    // See also https://stackoverflow.com/questions/22753707/is-ostream-operator-in-libstdc-thread-hostile

    // set the C++ locale first
    try {
        std::locale::global(std::locale(std::locale("en_US.UTF-8"), "C", std::locale::numeric));
    } catch (std::runtime_error) {
        try {
            std::locale::global(std::locale(std::locale("UTF8"), "C", std::locale::numeric));
        } catch (std::runtime_error) {
            try {
                std::locale::global(std::locale("C"));
            } catch (std::runtime_error) {
                qDebug() << "Could not set C++ locale!";
            }
        }
    }

    // set the C locale second, because it will not overwrite the changes you made to the C++ locale
    // see https://stackoverflow.com/questions/12373341/does-stdlocaleglobal-make-affect-to-printf-function
    char *category = std::setlocale(LC_ALL,"en_US.UTF-8");
    if (category == NULL) {
        category = std::setlocale(LC_ALL,"UTF-8");
    }
    if (category == NULL) {
        category = std::setlocale(LC_ALL,"C");
    }
    if (category == NULL) {
        qDebug() << "Could not set C locale!";
    }
    std::setlocale(LC_NUMERIC,"C"); // set the locale for LC_NUMERIC only

    Log::instance(); //< enable logging
    
    
    bool mustSetSignalsHandlers = true;
#ifdef NATRON_USE_BREAKPAD
    //Enabled breakpad only if the process was spawned from the crash reporter
    const QString& breakpadProcessExec = cl.getBreakpadProcessExecutableFilePath();
    if (!breakpadProcessExec.isEmpty() && QFile::exists(breakpadProcessExec)) {
        _imp->breakpadProcessExecutableFilePath = breakpadProcessExec;
        _imp->breakpadProcessPID = (Q_PID)cl.getBreakpadProcessPID();
        const QString& breakpadPipePath = cl.getBreakpadPipeFilePath();
        const QString& breakpadComPipePath = cl.getBreakpadComPipeFilePath();
        int breakpad_client_fd = cl.getBreakpadClientFD();
        _imp->initBreakpad(breakpadPipePath, breakpadComPipePath, breakpad_client_fd);
        mustSetSignalsHandlers = false;
    }
#endif
    
#if defined(__NATRON_LINUX__)
    if (mustSetSignalsHandlers) {
        setShutDownSignal(SIGINT);   // shut down on ctrl-c
        setShutDownSignal(SIGTERM);   // shut down on killall
        //Catch SIGSEGV only when google-breakpad is not active
        setSigSegvSignal();
    }
#endif
    (void)mustSetSignalsHandlers;
    
    _imp->_settings.reset(new Settings());
    _imp->_settings->initializeKnobsPublic();
    ///Call restore after initializing knobs
    _imp->_settings->restoreSettings();

    ///basically show a splashScreen load fonts etc...
    return initGui(cl);


} // loadInternal


bool
AppManager::isSpawnedFromCrashReporter() const
{
#ifdef NATRON_USE_BREAKPAD
    return _imp->breakpadHandler.get() != 0;
#else
    return false;
#endif
}

bool
AppManager::initGui(const CLArgs& cl)
{
    ///In background mode, directly call the rest of the loading code
    return loadInternalAfterInitGui(cl);
}

bool
AppManager::loadInternalAfterInitGui(const CLArgs& cl)
{
    try {
        size_t maxCacheRAM = _imp->_settings->getRamMaximumPercent() * getSystemTotalRAM();
        U64 maxViewerDiskCache = _imp->_settings->getMaximumViewerDiskCacheSize();
        U64 playbackSize = maxCacheRAM * _imp->_settings->getRamPlaybackMaximumPercent();
        U64 viewerCacheSize = maxViewerDiskCache + playbackSize;
        
        U64 maxDiskCacheNode = _imp->_settings->getMaximumDiskCacheNodeSize();
        
        _imp->_nodeCache.reset( new Cache<Image>("NodeCache",NATRON_CACHE_VERSION, maxCacheRAM - playbackSize,1.) );
        _imp->_diskCache.reset( new Cache<Image>("DiskCache",NATRON_CACHE_VERSION, maxDiskCacheNode,0.) );
        _imp->_viewerCache.reset( new Cache<FrameEntry>("ViewerCache",NATRON_CACHE_VERSION,viewerCacheSize,(double)playbackSize / (double)viewerCacheSize) );
    } catch (std::logic_error) {
        // ignore
    }
    
    int oldCacheVersion = 0;
    {
        QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
        
        if (settings.contains(kNatronCacheVersionSettingsKey)) {
            oldCacheVersion = settings.value(kNatronCacheVersionSettingsKey).toInt();
        }
        settings.setValue(kNatronCacheVersionSettingsKey, NATRON_CACHE_VERSION);
    }
    
    setLoadingStatus( tr("Restoring the image cache...") );
    
    if (oldCacheVersion != NATRON_CACHE_VERSION) {
        wipeAndCreateDiskCacheStructure();
    } else {
        _imp->restoreCaches();
    }
    
    setLoadingStatus( tr("Restoring user settings...") );
    
    
    ///Set host properties after restoring settings since it depends on the host name.
    try {
        _imp->ofxHost->setProperties();
    } catch (std::logic_error) {
        // ignore
    }
    
    /*loading all plugins*/
    try {
        loadAllPlugins();
        _imp->loadBuiltinFormats();
    } catch (std::logic_error) {
        // ignore
    }
    
    if ( isBackground() && !cl.getIPCPipeName().isEmpty() ) {
        _imp->initProcessInputChannel(cl.getIPCPipeName());
    }
    
    
    if (cl.isInterpreterMode()) {
        _imp->_appType = eAppTypeInterpreter;
    } else if ( isBackground() ) {
        if ( !cl.getScriptFilename().isEmpty() ) {
            if (!cl.getIPCPipeName().isEmpty()) {
                _imp->_appType = eAppTypeBackgroundAutoRunLaunchedFromGui;
            } else {
                _imp->_appType = eAppTypeBackgroundAutoRun;
            }
        } else {
            _imp->_appType = eAppTypeBackground;
        }
    } else {
        _imp->_appType = eAppTypeGui;
    }
    
    //Now that the locale is set, re-parse the command line arguments because the filenames might have non UTF-8 encodings
    CLArgs args;
    if (!cl.getScriptFilename().isEmpty()) {
        args = CLArgs(qApp->arguments(), cl.isBackgroundMode());
    } else{
        args = cl;
    }
    
    AppInstance* mainInstance = newAppInstance(args, false);
    
    hideSplashScreen();
    
    if (!mainInstance) {
        return false;
    } else {
        onLoadCompleted();
        
        ///In background project auto-run the rendering is finished at this point, just exit the instance
        if ( (_imp->_appType == eAppTypeBackgroundAutoRun ||
              _imp->_appType == eAppTypeBackgroundAutoRunLaunchedFromGui ||
              _imp->_appType == eAppTypeInterpreter) && mainInstance ) {
            try {
                mainInstance->getProject()->closeProject(true);
            } catch (std::logic_error) {
                // ignore
            }
            try {
                mainInstance->quit();
            } catch (std::logic_error) {
                // ignore
            }
        }
        
        return true;
    }
}

AppInstance*
AppManager::newAppInstanceInternal(const CLArgs& cl, bool alwaysBackground, bool makeEmptyInstance)
{
    AppInstance* instance;
    if (!alwaysBackground) {
        instance = makeNewInstance(_imp->_availableID);
    } else {
        instance = new AppInstance(_imp->_availableID);
    }
    ++_imp->_availableID;
    
    try {
        instance->load(cl, makeEmptyInstance);
    } catch (const std::exception & e) {
        Dialogs::errorDialog( NATRON_APPLICATION_NAME,e.what(), false );
        removeInstance(_imp->_availableID);
        delete instance;
        --_imp->_availableID;
        return NULL;
    } catch (...) {
        Dialogs::errorDialog( NATRON_APPLICATION_NAME, tr("Cannot load project").toStdString(), false );
        removeInstance(_imp->_availableID);
        delete instance;
        --_imp->_availableID;
        return NULL;
    }
    
    
    ///flag that we finished loading the Appmanager even if it was already true
    _imp->_loaded = true;
    
    return instance;
}

AppInstance*
AppManager::newBackgroundInstance(const CLArgs& cl, bool makeEmptyInstance)
{
    return newAppInstanceInternal(cl, true, makeEmptyInstance);
}

AppInstance*
AppManager::newAppInstance(const CLArgs& cl, bool makeEmptyInstance)
{
    return newAppInstanceInternal(cl, false, makeEmptyInstance);
}

AppInstance*
AppManager::getAppInstance(int appID) const
{
    QMutexLocker k(&_imp->_appInstancesMutex);
    
    std::map<int,AppInstanceRef>::const_iterator it;

    it = _imp->_appInstances.find(appID);
    if ( it != _imp->_appInstances.end() ) {
        return it->second.app;
    } else {
        return NULL;
    }
}

int
AppManager::getNumInstances() const
{
    QMutexLocker k(&_imp->_appInstancesMutex);
    return (int)_imp->_appInstances.size();
}

const std::map<int,AppInstanceRef> &
AppManager::getAppInstances() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->_appInstances;
}

void
AppManager::removeInstance(int appID)
{
    int newApp = -1;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        _imp->_appInstances.erase(appID);
        if ( !_imp->_appInstances.empty() ) {
            newApp = _imp->_appInstances.begin()->first;
        }
    }
    if (newApp != -1) {
        setAsTopLevelInstance(newApp);
    }
}

AppManager::AppTypeEnum
AppManager::getAppType() const
{
    return _imp->_appType;
}

void
AppManager::clearPlaybackCache()
{
    _imp->_viewerCache->clearInMemoryPortion();
    clearLastRenderedTextures();

}

void
AppManager::clearDiskCache()
{
    clearLastRenderedTextures();
    _imp->_viewerCache->clear();
    _imp->_diskCache->clear();
}

void
AppManager::clearNodeCache()
{
    std::map<int,AppInstanceRef> copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }
    for (std::map<int,AppInstanceRef>::iterator it = copy.begin(); it != copy.end(); ++it) {
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
    std::map<int,AppInstanceRef> copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }
    
    for (std::map<int,AppInstanceRef>::iterator it = copy.begin(); it != copy.end(); ++it) {
        it->second.app->abortAllViewers();
    }
    
    clearDiskCache();
    clearNodeCache();

 
    ///for each app instance clear all its nodes cache
    for (std::map<int,AppInstanceRef>::iterator it = copy.begin(); it != copy.end(); ++it) {
        it->second.app->clearOpenFXPluginsCaches();
    }
    
    for (std::map<int,AppInstanceRef>::iterator it = copy.begin(); it != copy.end(); ++it) {
        it->second.app->renderAllViewers(true);
    }
    
    Project::clearAutoSavesDir();
}

void
AppManager::wipeAndCreateDiskCacheStructure()
{
    //Should be called on the main-thread because potentially can interact with rendering
    assert(QThread::currentThread() == qApp->thread());
    
    abortAnyProcessing();
    
    clearAllCaches();
    
    assert(_imp->_diskCache);
    _imp->cleanUpCacheDiskStructure(_imp->_diskCache->getCachePath());
    assert(_imp->_viewerCache);
    _imp->cleanUpCacheDiskStructure(_imp->_viewerCache->getCachePath());
}


AppInstance*
AppManager::getTopLevelInstance () const
{
    QMutexLocker k(&_imp->_appInstancesMutex);
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
AppManager::abortAnyProcessing()
{
    std::map<int,AppInstanceRef> copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }
    for (std::map<int,AppInstanceRef>::iterator it = copy.begin(); it != copy.end(); ++it) {
        
        it->second.app->getProject()->quitAnyProcessingForAllNodes();
    }
}

bool
AppManager::writeToOutputPipe(const QString & longMessage,
                              const QString & shortMessage)
{
    if (!_imp->_backgroundIPC) {
        
        QMutexLocker k(&_imp->_ofxLogMutex);
        ///Don't use qdebug here which is disabled if QT_NO_DEBUG_OUTPUT is defined.
        std::cout << longMessage.toStdString() << std::endl;
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
    ref.status = eAppInstanceStatusActive;
    
    QMutexLocker k(&_imp->_appInstancesMutex);
    _imp->_appInstances.insert( std::make_pair(app->getAppID(),ref) );
}

void
AppManager::setApplicationsCachesMaximumMemoryPercent(double p)
{
    size_t maxCacheRAM = p * getSystemTotalRAM_conditionnally();
    U64 playbackSize = maxCacheRAM * _imp->_settings->getRamPlaybackMaximumPercent();

    _imp->_nodeCache->setMaximumCacheSize(maxCacheRAM - playbackSize);
    _imp->_nodeCache->setMaximumInMemorySize(1);
    U64 maxDiskCacheSize = _imp->_settings->getMaximumViewerDiskCacheSize();
    _imp->_viewerCache->setMaximumInMemorySize( (double)playbackSize / (double)maxDiskCacheSize );
}

void
AppManager::setApplicationsCachesMaximumViewerDiskSpace(unsigned long long size)
{
    size_t maxCacheRAM = _imp->_settings->getRamMaximumPercent() * getSystemTotalRAM_conditionnally();
    U64 playbackSize = maxCacheRAM * _imp->_settings->getRamPlaybackMaximumPercent();

    _imp->_viewerCache->setMaximumCacheSize(size);
    _imp->_viewerCache->setMaximumInMemorySize( (double)playbackSize / (double)size );
}

void
AppManager::setApplicationsCachesMaximumDiskSpace(unsigned long long size)
{
    _imp->_diskCache->setMaximumCacheSize(size);
}

void
AppManager::setPlaybackCacheMaximumSize(double p)
{
    size_t maxCacheRAM = _imp->_settings->getRamMaximumPercent() * getSystemTotalRAM_conditionnally();
    U64 playbackSize = maxCacheRAM * p;

    _imp->_nodeCache->setMaximumCacheSize(maxCacheRAM - playbackSize);
    _imp->_nodeCache->setMaximumInMemorySize(1);
    U64 maxDiskCacheSize = _imp->_settings->getMaximumViewerDiskCacheSize();
    _imp->_viewerCache->setMaximumInMemorySize( (double)playbackSize / (double)maxDiskCacheSize );
}

void
AppManager::loadAllPlugins()
{
    assert( _imp->_plugins.empty() );
    assert( _imp->_formats.empty() );


    std::map<std::string,std::vector< std::pair<std::string,double> > > readersMap;
    std::map<std::string,std::vector< std::pair<std::string,double> > > writersMap;

    /*loading node plugins*/

    loadBuiltinNodePlugins(&readersMap, &writersMap);

    /*loading ofx plugins*/
    _imp->ofxHost->loadOFXPlugins( &readersMap, &writersMap);
    
    _imp->_settings->populateReaderPluginsAndFormats(readersMap);
    _imp->_settings->populateWriterPluginsAndFormats(writersMap);

    _imp->declareSettingsToPython();
    
    //Load python groups and init.py & initGui.py scripts
    //Should be done after settings are declared
    loadPythonGroups();

    _imp->_settings->populatePluginsTab();

    
    onAllPluginsLoaded();
}

void
AppManager::onAllPluginsLoaded()
{
    //We try to make nicer plug-in labels, only do this if the user use Natron with some sort of interaction (either command line
    //or GUI, otherwise don't bother doing this)
    
    AppManager::AppTypeEnum appType = appPTR->getAppType();
    if (appType != AppManager::eAppTypeBackground &&
         appType != AppManager::eAppTypeGui &&
        appType != AppManager::eAppTypeInterpreter) {
        return;
    }
    
    //Make sure there is no duplicates with the same label
    const PluginsMap& plugins = getPluginsList();
    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
    
        assert(!it->second.empty());
        PluginMajorsOrdered::iterator first = it->second.begin();
        bool isUserCreatable = false;
        for (PluginMajorsOrdered::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ((*it2)->getIsUserCreatable()) {
                isUserCreatable = true;
                break;
            }
        }
        if (!isUserCreatable) {
            continue;
        }
        
        QString labelWithoutSuffix = Plugin::makeLabelWithoutSuffix((*first)->getPluginLabel());
        
        //Find a duplicate
        for (PluginsMap::const_iterator it2 = plugins.begin(); it2!=plugins.end(); ++it2) {
            if (it->first == it2->first) {
                continue;
            }
            
            
            PluginMajorsOrdered::iterator other = it2->second.begin();
            bool isOtherUserCreatable = false;
            for (PluginMajorsOrdered::iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                if ((*it3)->getIsUserCreatable()) {
                    isOtherUserCreatable = true;
                    break;
                }
            }

            if (!isOtherUserCreatable) {
                continue;
            }
        
            QString otherLabelWithoutSuffix = Plugin::makeLabelWithoutSuffix((*other)->getPluginLabel());
            if (otherLabelWithoutSuffix == labelWithoutSuffix) {
                QString otherGrouping = (*other)->getGrouping().join("/");
                
                const QStringList& thisGroupingSplit = (*first)->getGrouping();
                QString thisGrouping = thisGroupingSplit.join("/");
                if (otherGrouping == thisGrouping) {
                    labelWithoutSuffix = (*first)->getPluginLabel();
                }
                break;
            }
        }
        
        
        for (PluginMajorsOrdered::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ((*it2)->getIsUserCreatable()) {
                (*it2)->setLabelWithoutSuffix(labelWithoutSuffix);
                onPluginLoaded(*it2);
            }
        }
        
        
        
    }
}

template <typename PLUGIN>
void
AppManager::registerBuiltInPlugin(const QString& iconPath, bool isDeprecated, bool internalUseOnly)
{
    EffectInstPtr node( PLUGIN::BuildEffect( NodePtr() ) );
    std::map<std::string,void(*)()> functions;
    functions.insert( std::make_pair("BuildEffect", (void(*)())&PLUGIN::BuildEffect) );
    LibraryBinary *binary = new LibraryBinary(functions);
    assert(binary);
    
    std::list<std::string> grouping;
    node->getPluginGrouping(&grouping);
    QStringList qgrouping;
    
    for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
        qgrouping.push_back( it->c_str() );
    }
    Plugin* p = registerPlugin(qgrouping, node->getPluginID().c_str(), node->getPluginLabel().c_str(),
                                       iconPath, QStringList(), node->isReader(), node->isWriter(), binary, node->renderThreadSafety() == eRenderSafetyUnsafe, node->getMajorVersion(), node->getMinorVersion(), isDeprecated);
    if (internalUseOnly) {
        p->setForInternalUseOnly(true);
    }
}

void
AppManager::loadBuiltinNodePlugins(std::map<std::string,std::vector< std::pair<std::string,double> > >* /*readersMap*/,
                                   std::map<std::string,std::vector< std::pair<std::string,double> > >* /*writersMap*/)
{
    registerBuiltInPlugin<Backdrop>(NATRON_IMAGES_PATH "backdrop_icon.png", false, false);
    registerBuiltInPlugin<GroupOutput>(NATRON_IMAGES_PATH "output_icon.png", false, false);
    registerBuiltInPlugin<GroupInput>(NATRON_IMAGES_PATH "input_icon.png", false, false);
    registerBuiltInPlugin<NodeGroup>(NATRON_IMAGES_PATH "group_icon.png", false, false);
    registerBuiltInPlugin<Dot>(NATRON_IMAGES_PATH "dot_icon.png", false, false);
    registerBuiltInPlugin<DiskCacheNode>(NATRON_IMAGES_PATH "diskcache_icon.png", false, false);
    registerBuiltInPlugin<RotoPaint>(NATRON_IMAGES_PATH "GroupingIcons/Set2/paint_grouping_2.png", false, false);
    registerBuiltInPlugin<RotoNode>(NATRON_IMAGES_PATH "rotoNodeIcon.png", false, false);
    registerBuiltInPlugin<RotoSmear>("", false, true);
    registerBuiltInPlugin<PrecompNode>(NATRON_IMAGES_PATH "precompNodeIcon.png", false, false);
    if (!isBackground()) {
        registerBuiltInPlugin<ViewerInstance>(NATRON_IMAGES_PATH "viewer_icon.png", false, false);
    }
}

static bool findAndRunScriptFile(const QString& path,const QStringList& files,const QString& script)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return false;
#endif
    for (QStringList::const_iterator it = files.begin(); it != files.end(); ++it) {
        if (*it == script) {
            QFile file(path + *it);
            if (file.open(QIODevice::ReadOnly)) {
                QTextStream ts(&file);
                QString content = ts.readAll();
                PyRun_SimpleString(content.toStdString().c_str());

                PyObject* mainModule = Python::getMainModule();
                std::string error;
                ///Gui session, do stdout, stderr redirection
                PyObject *errCatcher = 0;
                
                if (PyObject_HasAttrString(mainModule, "catchErr")) {
                    errCatcher = PyObject_GetAttrString(mainModule,"catchErr"); //get our catchOutErr created above, new ref
                }
                
                PyErr_Print(); //make python print any errors
                
                PyObject *errorObj = 0;
                if (errCatcher) {
                    errorObj = PyObject_GetAttrString(errCatcher,"value"); //get the  stderr from our catchErr object, new ref
                    assert(errorObj);
                    error = Python::PY3String_asString(errorObj);
                    PyObject* unicode = PyUnicode_FromString("");
                    PyObject_SetAttrString(errCatcher, "value", unicode);
                    Py_DECREF(errorObj);
                    Py_DECREF(errCatcher);
                }

                if (!error.empty()) {
                    QString message("Failed to load ");
                    message.append(script);
                    message.append(": ");
                    message.append(error.c_str());
                    appPTR->writeToOfxLog_mt_safe(message);
                    std::cerr << message.toStdString() << std::endl;
                    return false;
                }
                return true;
            }
            break;
        }
    }
    return false;
}


QStringList
AppManager::getAllNonOFXPluginsPaths() const
{
    QStringList templatesSearchPath;
    
    //add ~/.Natron
    QString dataLocation = QDir::homePath();
    QString mainPath = dataLocation + "/." + QString(NATRON_APPLICATION_NAME);

    QDir mainPathDir(mainPath);
    if (!mainPathDir.exists()) {
        QDir dataDir(dataLocation);
        if (dataDir.exists()) {
            dataDir.mkdir("." + QString(NATRON_APPLICATION_NAME));
        }
    }
    
    QString envvar(qgetenv(NATRON_PATH_ENV_VAR));
    QStringList splitDirs = envvar.split(QChar(';'));
    std::list<std::string> userSearchPaths;
    _imp->_settings->getPythonGroupsSearchPaths(&userSearchPaths);
    

    //This is the bundled location for PyPlugs
    QDir cwd( QCoreApplication::applicationDirPath() );
    cwd.cdUp();
    QString natronBundledPluginsPath = QString(cwd.absolutePath() +  "/Plugins/PyPlugs");
    
    //Also take a look at PyPlugs embedded in the qrc
    QString natronBuiltinPluginsPath = QString(":/Resources/PyPlugs");

    bool preferBundleOverSystemWide = _imp->_settings->preferBundledPlugins();
    bool useBundledPlugins = _imp->_settings->loadBundledPlugins();
    if (preferBundleOverSystemWide && useBundledPlugins) {
        ///look-in the bundled plug-ins
        templatesSearchPath.push_back(natronBundledPluginsPath);
        templatesSearchPath.push_back(natronBuiltinPluginsPath);
    }
    
    ///look-in the main system wide plugin path
    templatesSearchPath.push_back(mainPath);
    
    ///look-in the locations indicated by NATRON_PLUGIN_PATH
    for (int i = 0; i < splitDirs.size(); ++i) {
        if (!splitDirs[i].isEmpty()) {
            templatesSearchPath.push_back(splitDirs[i]);
        }
    }
    
    ///look-in extra search path set in the preferences
    for (std::list<std::string>::iterator it = userSearchPaths.begin(); it != userSearchPaths.end(); ++it) {
        if (!it->empty()) {
            templatesSearchPath.push_back(QString(it->c_str()));
        }
    }
    
    if (!preferBundleOverSystemWide && useBundledPlugins) {
        ///look-in the bundled plug-ins
        templatesSearchPath.push_back(natronBundledPluginsPath);
        templatesSearchPath.push_back(natronBuiltinPluginsPath);
    }
    return templatesSearchPath;
}

typedef void (*NatronPathFunctor)(const QDir&);

static void operateOnPathRecursive(NatronPathFunctor functor, const QDir& directory)
{
    if (!directory.exists()) {
        return;
    }
    
    functor(directory);
    
    QStringList subDirs = directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < subDirs.size(); ++i) {
        QDir d(directory.absolutePath() + "/" + subDirs[i]);
        operateOnPathRecursive(functor,d);
    }
}

static void addToPythonPathFunctor(const QDir& directory)
{
    std::string addToPythonPath("sys.path.append(\"");
    addToPythonPath += directory.absolutePath().toStdString();
    addToPythonPath += "\")\n";
    
    std::string err;
    bool ok  = Python::interpretPythonScript(addToPythonPath, &err, 0);
    if (!ok) {
        std::string message = QObject::tr("Could not add").toStdString() + ' ' + directory.absolutePath().toStdString() + ' ' +
         QObject::tr("to python path").toStdString() + ": " + err;
        std::cerr << message << std::endl;
        AppInstance* topLevel = appPTR->getTopLevelInstance();
        if (topLevel) {
            topLevel->appendToScriptEditor(message.c_str());
        }
    }

}

static void findAllScriptsRecursive(const QDir& directory,
                                    QStringList& allPlugins,
                                    QStringList *foundInit,
                                    QStringList *foundInitGui)
{
    if (!directory.exists()) {
        return;
    }
    
    QStringList filters;
    filters << "*.py";
    QStringList files = directory.entryList(filters,QDir::Files | QDir::NoDotAndDotDot);
    bool ok = findAndRunScriptFile(directory.absolutePath() + '/', files,"init.py");
    if (ok) {
        foundInit->append(directory.absolutePath() + "/init.py");
    }
    if (!appPTR->isBackground()) {
        ok = findAndRunScriptFile(directory.absolutePath() + '/',files,"initGui.py");
        if (ok) {
            foundInitGui->append(directory.absolutePath() + "/initGui.py");
        }
    }
    
    for (QStringList::iterator it = files.begin(); it != files.end(); ++it) {
        if (it->endsWith(".py") && *it != QString("init.py") && *it != QString("initGui.py")) {
            allPlugins.push_back(directory.absolutePath() + "/" + *it);
        }
    }
    
    QStringList subDirs = directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (int i = 0; i < subDirs.size(); ++i) {
        QDir d(directory.absolutePath() + "/" + subDirs[i]);
        findAllScriptsRecursive(d,allPlugins, foundInit, foundInitGui);
    }

}

void
AppManager::loadPythonGroups()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return;
#endif
    PythonGILLocker pgl;
    
    QStringList templatesSearchPath = getAllNonOFXPluginsPaths();
    
    std::string err;
    
    QStringList allPlugins;
    
    ///For all search paths, first add the path to the python path, then run in order the init.py and initGui.py
    for (int i = 0; i < templatesSearchPath.size(); ++i) {
        //Adding Qt resources to Python path is useless as Python does not know how to use it
        if (templatesSearchPath[i].startsWith(":/Resources")) {
            continue;
        }
        QDir d(templatesSearchPath[i]);
        operateOnPathRecursive(&addToPythonPathFunctor,d);
    }
    
    ///Also import Pyside.QtCore and Pyside.QtGui (the later only in non background mode)
    {
        std::string s = "import PySide\nimport PySide.QtCore as QtCore";
        bool ok  = Python::interpretPythonScript(s, &err, 0);
        if (!ok) {
            std::string message = QObject::tr("Failed to import PySide.QtCore, make sure it is bundled with your Natron installation "
                                              "or reachable through the Python path. (Note that Natron disables usage "
                                              "of site-packages ").toStdString();
            std::cerr << message << std::endl;
            appPTR->writeToOfxLog_mt_safe(message.c_str());
        }
    }
    
    if (!isBackground()) {
        std::string s = "import PySide.QtGui as QtGui";
        bool ok  = Python::interpretPythonScript(s, &err, 0);
        if (!ok) {
            std::string message = QObject::tr("Failed to import PySide.QtGui").toStdString();
            std::cerr << message << std::endl;
            appPTR->writeToOfxLog_mt_safe(message.c_str());
        }
    }

    
    QStringList foundInit;
    QStringList foundInitGui;
    for (int i = 0; i < templatesSearchPath.size(); ++i) {
        QDir d(templatesSearchPath[i]);
        findAllScriptsRecursive(d,allPlugins,&foundInit, &foundInitGui);
    }
    if (foundInit.isEmpty()) {
        QString message = QObject::tr("init.py script not loaded");
        appPTR->setLoadingStatus(message);
        if (!appPTR->isBackground()) {
            std::cout << message.toStdString() << std::endl;
        }
    } else {
        for (int i = 0; i < foundInit.size(); ++i) {
            QString message = QObject::tr("init.py script found and loaded at: ");
            message.append(foundInit[i]);
            appPTR->setLoadingStatus(message);
            if (!appPTR->isBackground()) {
                std::cout << message.toStdString() << std::endl;
            }
        }
        
    }
    
    if (!appPTR->isBackground()) {
        
        if (foundInitGui.isEmpty()) {
            QString message = QObject::tr("initGui.py script not loaded");
            appPTR->setLoadingStatus(message);
            if (!appPTR->isBackground()) {
                std::cout << message.toStdString() << std::endl;
            }

        } else {
            for (int i = 0; i < foundInitGui.size(); ++i) {
                QString message = QObject::tr("initGui.py script found and loaded at: ");
                message.append(foundInitGui[i]);
                appPTR->setLoadingStatus(message);
                if (!appPTR->isBackground()) {
                    std::cout << message.toStdString() << std::endl;
                }
            }
        }
        
    }
    
    // Now that init.py and InitGui.py have run, we need to set the search path again for the PyPlug
    // as the user might have called appendToNatronPath
    
    QStringList newTemplatesSearchPath = getAllNonOFXPluginsPaths();
    {
        QStringList diffSearch;
        for (int i = 0; i < newTemplatesSearchPath.size(); ++i) {
            if (!templatesSearchPath.contains(newTemplatesSearchPath[i])) {
                diffSearch.push_back(newTemplatesSearchPath[i]);
            }
        }
        
        //Add only paths that did not exist so far
        for (int i = 0; i < diffSearch.size(); ++i) {
            QDir d(diffSearch[i]);
            operateOnPathRecursive(&addToPythonPathFunctor,d);
        }
    }
    
    appPTR->setLoadingStatus(QString(QObject::tr("Loading PyPlugs...")));

    for (int i = 0; i < allPlugins.size(); ++i) {
        
        QString moduleName = allPlugins[i];
        QString modulePath;
        int lastDot = moduleName.lastIndexOf('.');
        if (lastDot != -1) {
            moduleName = moduleName.left(lastDot);
        }
        int lastSlash = moduleName.lastIndexOf('/');
        if (lastSlash != -1) {
            modulePath = moduleName.mid(0,lastSlash + 1);
            moduleName = moduleName.remove(0,lastSlash + 1);
        }
        
        std::string pluginLabel,pluginID,pluginGrouping,iconFilePath,pluginDescription;
        unsigned int version;
        
        bool gotInfos = Python::getGroupInfos(modulePath.toStdString(),moduleName.toStdString(), &pluginID, &pluginLabel, &iconFilePath, &pluginGrouping, &pluginDescription, &version);

        
        if (gotInfos) {
            qDebug() << "Loading " << moduleName;
            QStringList grouping = QString(pluginGrouping.c_str()).split(QChar('/'));
            Plugin* p = registerPlugin(grouping, pluginID.c_str(), pluginLabel.c_str(), iconFilePath.c_str(), QStringList(), false, false, 0, false, version, 0, false);
            
            p->setPythonModule(modulePath + moduleName);
            
        }
        
    }
}

Plugin*
AppManager::registerPlugin(const QStringList & groups,
                           const QString & pluginID,
                           const QString & pluginLabel,
                           const QString & pluginIconPath,
                           const QStringList & groupIconPath,
                           bool isReader,
                           bool isWriter,
                           LibraryBinary* binary,
                           bool mustCreateMutex,
                           int major,
                           int minor,
                           bool isDeprecated)
{
    QMutex* pluginMutex = 0;

    if (mustCreateMutex) {
        pluginMutex = new QMutex(QMutex::Recursive);
    }
    Plugin* plugin = new Plugin(binary,pluginID,pluginLabel,pluginIconPath,groupIconPath,groups,pluginMutex,major,minor,
                                                isReader,isWriter, isDeprecated);
    std::string stdID = pluginID.toStdString();
    PluginsMap::iterator found = _imp->_plugins.find(stdID);
    if (found != _imp->_plugins.end()) {
        found->second.insert(plugin);
    } else {
        PluginMajorsOrdered set;
        set.insert(plugin);
        _imp->_plugins.insert(std::make_pair(stdID, set));
    }
    
    return plugin;
}


Format*
AppManager::findExistingFormat(int w,
                               int h,
                               double par) const
{
    for (U32 i = 0; i < _imp->_formats.size(); ++i) {
        Format* frmt = _imp->_formats[i];
        assert(frmt);
        if ( (frmt->width() == w) && (frmt->height() == h) && (frmt->getPixelAspectRatio() == par) ) {
            return frmt;
        }
    }

    return NULL;
}

void
AppManager::setAsTopLevelInstance(int appID)
{
    QMutexLocker k(&_imp->_appInstancesMutex);
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
                if (it->second.app) {
                    it->second.app->connectViewersToViewerCache();
                    setOFXHostHandle(it->second.app->getOfxHostOSHandle());
                }

            }
        }
    }
    
}

void
AppManager::setOFXHostHandle(void* handle)
{
    _imp->ofxHost->setOfxHostOSHandle(handle);
}

void
AppManager::clearExceedingEntriesFromNodeCache()
{
    _imp->_nodeCache->clearExceedingEntries();
}

const PluginsMap&
AppManager::getPluginsList() const
{
    return _imp->_plugins;
}

QMutex*
AppManager::getMutexForPlugin(const QString & pluginId,int major,int /*minor*/) const
{
    for (PluginsMap::iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        for (PluginMajorsOrdered::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ((*it2)->getPluginID() == pluginId && (*it2)->getMajorVersion() == major) {
                return (*it2)->getPluginLock();
            }
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


Plugin*
AppManager::getPluginBinaryFromOldID(const QString & pluginId,int majorVersion,int minorVersion) const
{
    std::map<int,Plugin*> matches;
    
    if (pluginId == "Viewer") {
        return _imp->findPluginById(PLUGINID_NATRON_VIEWER, majorVersion, minorVersion);
    } else if (pluginId == "Dot") {
        return _imp->findPluginById(PLUGINID_NATRON_DOT,majorVersion, minorVersion );
    } else if (pluginId == "DiskCache") {
        return _imp->findPluginById(PLUGINID_NATRON_DISKCACHE, majorVersion, minorVersion);
    } else if (pluginId == "Backdrop") { // DO NOT change the capitalization, even if it's wrong
        return _imp->findPluginById(PLUGINID_NATRON_BACKDROP, majorVersion, minorVersion);
    } else if (pluginId == "RotoOFX  [Draw]") {
        return _imp->findPluginById(PLUGINID_NATRON_ROTO, majorVersion, minorVersion);
    }
    
    ///Try remapping these ids to old ids we had in Natron < 1.0 for backward-compat
    for (PluginsMap::const_iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        
        
        assert(!it->second.empty());
        PluginMajorsOrdered::const_iterator it2 = it->second.begin();
        
        QString friendlyLabel = (*it2)->getPluginLabel();
        const QStringList& s = (*it2)->getGrouping();
        QString grouping;
        if (s.size() > 0) {
            grouping = s[0];
        }
        friendlyLabel += "  [" + grouping + "]";

        if (friendlyLabel == pluginId) {
            if (majorVersion == -1) {
                // -1 means we want to load the highest version existing
                return *(it->second.rbegin());
            }
            
            //Look for the exact version
            for (; it2 != it->second.end(); ++it2) {
                if ((*it2)->getMajorVersion() == majorVersion) {
                    return *it2;
                }
            }
            ///Could not find the exact version... let's just use the highest version found
            return *(it->second.rbegin());

        }
        
        
    }
    return 0;
}

Plugin*
AppManager::getPluginBinary(const QString & pluginId,
                            int majorVersion,
                            int /*minorVersion*/,
                            bool convertToLowerCase) const
{
    
    PluginsMap::const_iterator foundID = _imp->_plugins.end();
    for (PluginsMap::const_iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        if (convertToLowerCase &&
            !pluginId.startsWith(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.")) {
            
            QString lowerCase = QString(it->first.c_str()).toLower();
            if (lowerCase == pluginId) {
                foundID = it;
                break;
            }
            
        } else {
            if (QString(it->first.c_str()) == pluginId) {
                foundID = it;
                break;
            }
        }
    }
    
    
    
    if (foundID != _imp->_plugins.end()) {
        
        assert(!foundID->second.empty());
        
        if (majorVersion == -1) {
            // -1 means we want to load the highest version existing
            return *foundID->second.rbegin();
        }
        
        ///Try to find the exact version
        for (PluginMajorsOrdered::const_iterator it = foundID->second.begin(); it != foundID->second.end(); ++it) {
            if (((*it)->getMajorVersion() == majorVersion)) {
                return *it;
            }
        }
        
        ///Could not find the exact version... let's just use the highest version found
        return *foundID->second.rbegin();
    }
    QString exc = QString("Couldn't find a plugin attached to the ID %1, with a major version of %2")
    .arg(pluginId)
    .arg(majorVersion);
    throw std::invalid_argument( exc.toStdString() );
    return 0;
    
}

EffectInstPtr
AppManager::createOFXEffect(NodePtr node,
                            const NodeSerialization* serialization,
                            const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                            bool allowFileDialogs,
                            bool disableRenderScaleSupport,
                            bool *hasUsedFileDialog) const
{
    return _imp->ofxHost->createOfxEffect(node,serialization,paramValues,allowFileDialogs,disableRenderScaleSupport,hasUsedFileDialog);
}

void
AppManager::removeFromNodeCache(const boost::shared_ptr<Image> & image)
{
    _imp->_nodeCache->removeEntry(image);
}

void
AppManager::removeFromViewerCache(const boost::shared_ptr<FrameEntry> & texture)
{
    _imp->_viewerCache->removeEntry(texture);

}

void
AppManager::removeFromNodeCache(U64 hash)
{
    _imp->_nodeCache->removeEntry(hash);
}

void
AppManager::removeFromViewerCache(U64 hash)
{
    _imp->_viewerCache->removeEntry(hash);
}

void
AppManager::getMemoryStatsForCacheEntryHolder(const CacheEntryHolder* holder,
                                       std::size_t* ramOccupied,
                                       std::size_t* diskOccupied) const
{
    assert(holder);
    
    *ramOccupied = 0;
    *diskOccupied = 0;
    
    std::size_t viewerCacheMem = 0;
    std::size_t viewerCacheDisk = 0;
    std::size_t diskCacheMem = 0;
    std::size_t diskCacheDisk = 0;
    std::size_t nodeCacheMem = 0;
    std::size_t nodeCacheDisk = 0;
    
    const Node* isNode = dynamic_cast<const Node*>(holder);
    if (isNode) {
        ViewerInstance* isViewer = isNode->isEffectViewer();
        if (isViewer) {
            _imp->_viewerCache->getMemoryStatsForCacheEntryHolder(holder, &viewerCacheMem, &viewerCacheDisk);
        }
    }
    _imp->_diskCache->getMemoryStatsForCacheEntryHolder(holder, &diskCacheMem, &diskCacheDisk);
    _imp->_nodeCache->getMemoryStatsForCacheEntryHolder(holder, &nodeCacheMem, &nodeCacheDisk);
    
    *ramOccupied = diskCacheMem + viewerCacheMem + nodeCacheMem;
    *diskOccupied = diskCacheDisk + viewerCacheDisk + nodeCacheDisk;
}

void
AppManager::removeAllImagesFromCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder, U64 treeVersion)
{
    _imp->_nodeCache->removeAllEntriesWithDifferentNodeHashForHolderPublic(holder, treeVersion);
}

void
AppManager::removeAllImagesFromDiskCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder, U64 treeVersion)
{
    _imp->_diskCache->removeAllEntriesWithDifferentNodeHashForHolderPublic(holder, treeVersion);
}

void
AppManager::removeAllTexturesFromCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder, U64 treeVersion)
{
    _imp->_viewerCache->removeAllEntriesWithDifferentNodeHashForHolderPublic(holder, treeVersion);
}

void
AppManager::removeAllCacheEntriesForHolder(const CacheEntryHolder* holder,bool blocking)
{
    _imp->_nodeCache->removeAllEntriesForHolderPublic(holder,blocking);
    _imp->_diskCache->removeAllEntriesForHolderPublic(holder,blocking);
    _imp->_viewerCache->removeAllEntriesForHolderPublic(holder,blocking);
}

const QString &
AppManager::getApplicationBinaryPath() const
{
    return _imp->_binaryPath;
}

void
AppManager::setNumberOfThreads(int threadsNb)
{
    if (_imp->_settings) {
        _imp->_settings->setNumberOfThreads(threadsNb);
    }
}

bool
AppManager::getImage(const ImageKey & key,
                     std::list<boost::shared_ptr<Image> >* returnValue) const
{
    return _imp->_nodeCache->get(key,returnValue);
}

bool
AppManager::getImageOrCreate(const ImageKey & key,
                             const boost::shared_ptr<ImageParams>& params,
                             boost::shared_ptr<Image>* returnValue) const
{
    return _imp->_nodeCache->getOrCreate(key,params,returnValue);
}

bool
AppManager::getImage_diskCache(const ImageKey & key,std::list<boost::shared_ptr<Image> >* returnValue) const
{
    return _imp->_diskCache->get(key, returnValue);
}

bool
AppManager::getImageOrCreate_diskCache(const ImageKey & key,const boost::shared_ptr<ImageParams>& params,
                                boost::shared_ptr<Image>* returnValue) const
{
    return _imp->_diskCache->getOrCreate(key, params, returnValue);
}


bool
AppManager::getTexture(const FrameKey & key,
                       boost::shared_ptr<FrameEntry>* returnValue) const
{
    std::list<boost::shared_ptr<FrameEntry> > retList;
    
    bool ret =  _imp->_viewerCache->get(key, &retList);
    
    if (!retList.empty()) {
        if (retList.size() > 1) {
            qDebug() << "WARNING: Several FrameEntry's were found in the cache for with the same key, this is a bug since they are unique.";
        }
        
        *returnValue = retList.front();
    }
    
    return ret;

}

bool
AppManager::getTextureOrCreate(const FrameKey & key,
                               const boost::shared_ptr<FrameParams>& params,
                               boost::shared_ptr<FrameEntry>* returnValue) const
{
    
    return _imp->_viewerCache->getOrCreate(key, params,returnValue);
}

bool
AppManager::isAggressiveCachingEnabled() const
{
    return _imp->_settings->isAggressiveCachingEnabled();
}

U64
AppManager::getCachesTotalMemorySize() const
{
    return _imp->_viewerCache->getMemoryCacheSize() + _imp->_nodeCache->getMemoryCacheSize();
}

CacheSignalEmitter*
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


void
AppManager::registerEngineMetaTypes() const
{
    qRegisterMetaType<Variant>("Variant");
    qRegisterMetaType<Format>("Format");
    qRegisterMetaType<SequenceTime>("SequenceTime");
    qRegisterMetaType<StandardButtons>("StandardButtons");
    qRegisterMetaType<RectI>("RectI");
    qRegisterMetaType<RectD>("RectD");
    qRegisterMetaType<RenderStatsPtr>("RenderStatsPtr");
    qRegisterMetaType<RenderStatsMap>("RenderStatsMap");
    qRegisterMetaType<ViewIdx>("ViewIdx");
#if QT_VERSION < 0x050000
    qRegisterMetaType<QAbstractSocket::SocketState>("SocketState");
#endif
}

void
AppManager::setDiskCacheLocation(const QString& path)
{
    
    QDir d(path);
    QMutexLocker k(&_imp->diskCachesLocationMutex);
    if (d.exists() && !path.isEmpty()) {
        _imp->diskCachesLocation = path;
    } else {
        _imp->diskCachesLocation = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache);
    }
    
}

const QString&
AppManager::getDiskCacheLocation() const
{
    QMutexLocker k(&_imp->diskCachesLocationMutex);
    return _imp->diskCachesLocation;
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
        qDebug() << "Cache has more files opened than the limit allowed:" << _imp->currentCacheFilesCount << '/' << _imp->maxCacheFiles;
    }
#endif
#ifdef NATRON_DEBUG_CACHE
    qDebug() << "N Cache Files Opened:" << _imp->currentCacheFilesCount;
#endif
}

void
AppManager::decreaseNCacheFilesOpened()
{
    QMutexLocker l(&_imp->currentCacheFilesCountMutex);

    --_imp->currentCacheFilesCount;
#ifdef NATRON_DEBUG_CACHE
    qDebug() << "NFiles Opened:" << _imp->currentCacheFilesCount;
#endif
}

void
AppManager::onMaxPanelsOpenedChanged(int maxPanels)
{
    std::map<int,AppInstanceRef> copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }
    for (std::map<int,AppInstanceRef>::iterator it = copy.begin(); it != copy.end(); ++it) {
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

    _imp->_ofxLog.append(str + '\n' + '\n');
}

void
AppManager::clearOfxLog_mt_safe()
{
    QMutexLocker l(&_imp->_ofxLogMutex);
    _imp->_ofxLog.clear();
}

void
AppManager::exitApp(bool /*warnUserForSave*/)
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

bool
AppManager::isNodeCacheAlmostFull() const
{
    std::size_t nodeCacheSize = _imp->_nodeCache->getMemoryCacheSize();
    std::size_t nodeMaxCacheSize = _imp->_nodeCache->getMaximumMemorySize();
    
    if (nodeMaxCacheSize == 0) {
        return true;
    }
    
    if ((double)nodeCacheSize / nodeMaxCacheSize >= NATRON_CACHE_LIMIT_PERCENT) {
        return true;
    } else {
        return false;
    }
}

void
AppManager::checkCacheFreeMemoryIsGoodEnough()
{
    ///Before allocating the memory check that there's enough space to fit in memory
    size_t systemRAMToKeepFree = getSystemTotalRAM() * appPTR->getCurrentSettings()->getUnreachableRamPercent();
    size_t totalFreeRAM = getAmountFreePhysicalRAM();
    

    double playbackRAMPercent = appPTR->getCurrentSettings()->getRamPlaybackMaximumPercent();
    while (totalFreeRAM <= systemRAMToKeepFree) {
        
        size_t nodeCacheSize =  _imp->_nodeCache->getMemoryCacheSize();
        size_t viewerRamCacheSize =  _imp->_viewerCache->getMemoryCacheSize();
        
        ///If the viewer cache represents more memory than the node cache, clear some of the viewer cache
        if (nodeCacheSize == 0 || (viewerRamCacheSize / (double)nodeCacheSize) > playbackRAMPercent) {
#ifdef NATRON_DEBUG_CACHE
            qDebug() << "Total system free RAM is below the threshold:" << printAsRAM(totalFreeRAM)
                     << ", clearing least recently used ViewerCache texture...";
#endif
            
            if (!_imp->_viewerCache->evictLRUInMemoryEntry()) {
                break;
            }
            
            
        } else {
#ifdef NATRON_DEBUG_CACHE
            qDebug() << "Total system free RAM is below the threshold:" << printAsRAM(totalFreeRAM)
                     << ", clearing least recently used NodeCache image...";
#endif
            if (!_imp->_nodeCache->evictLRUInMemoryEntry()) {
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
    
    std::map<int,AppInstanceRef> copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }
    
    for (std::map<int,AppInstanceRef>::iterator it = copy.begin() ; it != copy.end(); ++it) {
        it->second.app->onOCIOConfigPathChanged(path);
    }
}

const std::string&
AppManager::getOCIOConfigPath() const
{
    return _imp->currentOCIOConfigPath;
}



void
AppManager::setNThreadsToRender(int nThreads)
{
    QMutexLocker l(&_imp->nThreadsMutex);
    _imp->nThreadsToRender = nThreads;
}

void
AppManager::getNThreadsSettings(int* nThreadsToRender,int* nThreadsPerEffect) const
{
    QMutexLocker l(&_imp->nThreadsMutex);
    *nThreadsToRender = _imp->nThreadsToRender;
    *nThreadsPerEffect = _imp->nThreadsPerEffect;
}


void
AppManager::setNThreadsPerEffect(int nThreadsPerEffect)
{
    QMutexLocker l(&_imp->nThreadsMutex);
    _imp->nThreadsPerEffect = nThreadsPerEffect;
}

void
AppManager::setUseThreadPool(bool useThreadPool)
{
    QMutexLocker l(&_imp->nThreadsMutex);
    _imp->useThreadPool = useThreadPool;
}

bool
AppManager::getUseThreadPool() const
{
    QMutexLocker l(&_imp->nThreadsMutex);
    return _imp->useThreadPool;
}

void
AppManager::fetchAndAddNRunningThreads(int nThreads)
{
    _imp->runningThreadsCount.fetchAndAddRelaxed(nThreads);
}

int
AppManager::getNRunningThreads() const
{
    return (int)_imp->runningThreadsCount;
}

void
AppManager::setThreadAsActionCaller(OfxImageEffectInstance* instance, bool actionCaller)
{
    _imp->ofxHost->setThreadAsActionCaller(instance,actionCaller);
}

void
AppManager::requestOFXDIalogOnMainThread(OfxImageEffectInstance* instance, void* instanceData)
{
    if (QThread::currentThread() == qApp->thread()) {
        onOFXDialogOnMainThreadReceived(instance, instanceData);
    } else {
        Q_EMIT s_requestOFXDialogOnMainThread(instance, instanceData);
    }
}

void
AppManager::onOFXDialogOnMainThreadReceived(OfxImageEffectInstance* instance, void* instanceData)
{
    assert(QThread::currentThread() == qApp->thread());
    if (instance == NULL) {
        // instance may be NULL if using OfxDialogSuiteV1
        OfxHost::OfxHostDataTLSPtr tls = _imp->ofxHost->getTLSData();
        instance = tls->lastEffectCallingMainEntry;
    } else {
#ifdef DEBUG
        OfxHost::OfxHostDataTLSPtr tls = _imp->ofxHost->getTLSData();
        assert(instance == tls->lastEffectCallingMainEntry);
#endif
    }
#ifdef OFX_SUPPORTS_DIALOG
    if (instance) {
        instance->dialog(instanceData);
    }
#else
    Q_UNUSED(instanceData);
#endif
}

std::list<std::string>
AppManager::getPluginIDs() const
{
    std::list<std::string> ret;
    for (PluginsMap::const_iterator it = _imp->_plugins.begin() ; it != _imp->_plugins.end(); ++it) {
        assert(!it->second.empty());
        ret.push_back(it->first);
    }
    return ret;
}

std::list<std::string>
AppManager::getPluginIDs(const std::string& filter)
{
    QString qFilter(filter.c_str());
    std::list<std::string> ret;
    for (PluginsMap::const_iterator it = _imp->_plugins.begin() ; it != _imp->_plugins.end(); ++it) {
        assert(!it->second.empty());
        
        QString pluginID(it->first.c_str());
        if (pluginID.contains(qFilter,Qt::CaseInsensitive)) {
            ret.push_back(it->first);
        }
    }
    return ret;
}

#ifndef IS_PYTHON_2
//Borrowed from https://github.com/python/cpython/blob/634cb7aa2936a09e84c5787d311291f0e042dba3/Python/fileutils.c
//Somehow Python 3 dev forced every C application embedding python to have their own code to convert char** to wchar_t**
static wchar_t*
char2wchar(char* arg)
{
    wchar_t *res = NULL;
#ifdef HAVE_BROKEN_MBSTOWCS
    /* Some platforms have a broken implementation of
     * mbstowcs which does not count the characters that
     * would result from conversion.  Use an upper bound.
     */
    size_t argsize = strlen(arg);
#else
    size_t argsize = mbstowcs(NULL, arg, 0);
#endif
    size_t count;
    unsigned char *in;
    wchar_t *out;
#ifdef HAVE_MBRTOWC
    mbstate_t mbs;
#endif
    if (argsize != (size_t)-1) {
        res = (wchar_t *)malloc((argsize+1)*sizeof(wchar_t));
        if (!res)
            goto oom;
        count = mbstowcs(res, arg, argsize+1);
        if (count != (size_t)-1) {
            wchar_t *tmp;
            /* Only use the result if it contains no
             surrogate characters. */
            for (tmp = res; *tmp != 0 &&
                 (*tmp < 0xd800 || *tmp > 0xdfff); tmp++)
                ;
            if (*tmp == 0)
                return res;
        }
        free(res);
    }
    /* Conversion failed. Fall back to escaping with surrogateescape. */
#ifdef HAVE_MBRTOWC
    /* Try conversion with mbrtwoc (C99), and escape non-decodable bytes. */
    /* Overallocate; as multi-byte characters are in the argument, the
     actual output could use less memory. */
    argsize = strlen(arg) + 1;
    res = (wchar_t*)malloc(argsize*sizeof(wchar_t));
    if (!res) goto oom;
    in = (unsigned char*)arg;
    out = res;
    memset(&mbs, 0, sizeof mbs);
    while (argsize) {
        size_t converted = mbrtowc(out, (char*)in, argsize, &mbs);
        if (converted == 0)
        /* Reached end of string; null char stored. */
            break;
        if (converted == (size_t)-2) {
            /* Incomplete character. This should never happen,
             since we provide everything that we have -
             unless there is a bug in the C library, or I
             misunderstood how mbrtowc works. */
            fprintf(stderr, "unexpected mbrtowc result -2\n");
            free(res);
            return NULL;
        }
        if (converted == (size_t)-1) {
            /* Conversion error. Escape as UTF-8b, and start over
             in the initial shift state. */
            *out++ = 0xdc00 + *in++;
            argsize--;
            memset(&mbs, 0, sizeof mbs);
            continue;
        }
        if (*out >= 0xd800 && *out <= 0xdfff) {
            /* Surrogate character.  Escape the original
             byte sequence with surrogateescape. */
            argsize -= converted;
            while (converted--)
                *out++ = 0xdc00 + *in++;
            continue;
        }
        /* successfully converted some bytes */
        in += converted;
        argsize -= converted;
        out++;
    }
#else
    /* Cannot use C locale for escaping; manually escape as if charset
     is ASCII (i.e. escape all bytes > 128. This will still roundtrip
     correctly in the locale's charset, which must be an ASCII superset. */
    res = (wchar_t*)malloc((strlen(arg)+1)*sizeof(wchar_t));
    if (!res) goto oom;
    in = (unsigned char*)arg;
    out = res;
    while(*in)
        if(*in < 128)
            *out++ = *in++;
        else
            *out++ = 0xdc00 + *in++;
    *out = 0;
#endif
    return res;
oom:
    fprintf(stderr, "out of memory\n");
    free(res);
    return NULL;
}
#endif


std::string
Python::PY3String_asString(PyObject* obj)
{
    std::string ret;
    if (PyUnicode_Check(obj)) {
        PyObject * temp_bytes = PyUnicode_AsEncodedString(obj, "ASCII", "strict"); // Owned reference
        if (temp_bytes != NULL) {
            char* cstr = PyBytes_AS_STRING(temp_bytes); // Borrowed pointer
            ret.append(cstr);
            Py_DECREF(temp_bytes);
        }
    } else if (PyBytes_Check(obj)) {
        char* cstr = PyBytes_AS_STRING(obj); // Borrowed pointer
        ret.append(cstr);
    }
    return ret;
}

void
AppManager::initPython(int argc,char* argv[])
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return;
#endif
    //Disable user sites as they could conflict with Natron bundled packages.
    //If this is set, Python won’t add the user site-packages directory to sys.path.
    //See https://www.python.org/dev/peps/pep-0370/
    qputenv("PYTHONNOUSERSITE","1");
    ++Py_NoUserSiteDirectory;
    
    QString pythonPath(qgetenv("PYTHONPATH"));
    //Add the Python distribution of Natron to the Python path
    QString binPath = QCoreApplication::applicationDirPath();
    binPath = QDir::toNativeSeparators(binPath);
    bool pathEmpty = pythonPath.isEmpty();
    QString toPrepend;
#ifdef __NATRON_WIN32__
    toPrepend.append(binPath + "\\..\\Plugins");
    if (!pathEmpty) {
        toPrepend.push_back(';');
    }
#elif defined(__NATRON_OSX__)
    toPrepend.append(binPath + "/../Frameworks/Python.framework/Versions/" NATRON_PY_VERSION_STRING "/lib/python" NATRON_PY_VERSION_STRING);
    toPrepend.append(':');
    toPrepend.append(binPath + "/../Plugins");
    if (!pathEmpty) {
        toPrepend.push_back(':');
    }
#elif defined(__NATRON_LINUX__)
    toPrepend.append(binPath + "/../lib/python" NATRON_PY_VERSION_STRING);
    toPrepend.append(':');
    toPrepend.append(binPath + "/../Plugins");
    if (!pathEmpty) {
        toPrepend.push_back(':');
    }
#endif


    pythonPath.prepend(toPrepend);
    qputenv("PYTHONPATH",pythonPath.toLatin1());

    _imp->args.resize(argc);
    for (int i = 0; i < argc; ++i) {
#ifndef IS_PYTHON_2
        _imp->args[i] = char2wchar(argv[i]);
#else
        _imp->args[i] = strdup(argv[i]); // free'd in ~AppManagerPrivate()
#endif
    }
    
 
    Py_SetProgramName(_imp->args[0]);

    
    ///Must be called prior to Py_Initialize
    initBuiltinPythonModules();
//#ifndef DEBUG
    
    //See https://developer.blender.org/T31507
    //Python will not load anything in site-packages if this is set
    //Py_NoSiteFlag = 1; // NEVER comment this
//#endif
    Py_Initialize();
    // pythonHome must be const, so that the c_str() pointer is never invalidated
    
#ifndef IS_PYTHON_2
#ifdef __NATRON_WIN32__
    static const std::wstring pythonHome(Global::s2ws("."));
#elif defined(__NATRON_LINUX__)
    static const std::wstring pythonHome(Global::s2ws("../lib"));
#elif defined(__NATRON_OSX__)
    static const std::wstring pythonHome(Global::s2ws("../Frameworks/Python.framework/Versions/" NATRON_PY_VERSION_STRING "/lib"));
#endif
    Py_SetPythonHome(const_cast<wchar_t*>(pythonHome.c_str()));
    PySys_SetArgv(argc, &_imp->args.front()); /// relative module import
#else
#ifdef __NATRON_WIN32__
    static const std::string pythonHome(".");
#elif defined(__NATRON_LINUX__)
    static const std::string pythonHome("../lib");
#elif defined(__NATRON_OSX__)
    static const std::string pythonHome("../Frameworks/Python.framework/Versions/" NATRON_PY_VERSION_STRING "/lib");
#endif
    Py_SetPythonHome(const_cast<char*>(pythonHome.c_str()));
    PySys_SetArgv(argc, &_imp->args.front()); /// relative module import
#endif
    
    _imp->mainModule = PyImport_ImportModule("__main__"); //create main module , new ref
    
    //See http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    //Python releases the GIL every 100 virtual Python instructions, we do not want that to happen in the middle of an expression.
    //_PyEval_SetSwitchInterval(LONG_MAX);
    
    //See answer for http://stackoverflow.com/questions/15470367/pyeval-initthreads-in-python-3-how-when-to-call-it-the-saga-continues-ad-naus
    PyEval_InitThreads();
    
    ///Do as per http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    ///All calls to the Python API should call PythonGILLocker beforehand.
    //_imp->mainThreadState = PyGILState_GetThisThreadState();
    //PyEval_ReleaseThread(_imp->mainThreadState);
    
    std::string err;
#if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
    /// print info about python lib
    {
        printf("Py_GetProgramName is %s\n", Py_GetProgramName());
        printf("Py_GetPrefix is %s\n", Py_GetPrefix());
        printf("Py_GetExecPrefix is %s\n", Py_GetPrefix());
        printf("Py_GetProgramFullPath is %s\n", Py_GetProgramFullPath());
        printf("Py_GetPath is %s\n", Py_GetPath());
        printf("Py_GetPythonHome is %s\n", Py_GetPythonHome());
        bool ok = Python::interpretPythonScript("from distutils.sysconfig import get_python_lib; print('Python library is in ' + get_python_lib())", &err, 0);
        assert(ok);
    }
#endif
    bool ok = Python::interpretPythonScript("import sys\nfrom math import *\nimport " + std::string(NATRON_ENGINE_PYTHON_MODULE_NAME), &err, 0);
    if (!ok) {
        throw std::runtime_error("Error while loading python module "NATRON_ENGINE_PYTHON_MODULE_NAME": " + err);
    }

    ok = Python::interpretPythonScript(std::string(NATRON_ENGINE_PYTHON_MODULE_NAME) + ".natron = " + std::string(NATRON_ENGINE_PYTHON_MODULE_NAME) + ".PyCoreApplication()\n" , &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("Error while loading python module "NATRON_ENGINE_PYTHON_MODULE_NAME": " + err);
    }

    if (!isBackground()) {
        
        ok = Python::interpretPythonScript("import sys\nimport " + std::string(NATRON_GUI_PYTHON_MODULE_NAME), &err, 0);
        assert(ok);
        if (!ok) {
            throw std::runtime_error("Error while loading python module "NATRON_GUI_PYTHON_MODULE_NAME": " + err);
        }

        ok = Python::interpretPythonScript(std::string(NATRON_GUI_PYTHON_MODULE_NAME) + ".natron = " +
                                   std::string(NATRON_GUI_PYTHON_MODULE_NAME) + ".PyGuiApplication()\n" , &err, 0);
        assert(ok);
        if (!ok) {
            throw std::runtime_error("Error while loading python module "NATRON_GUI_PYTHON_MODULE_NAME": " + err);
        }

        //redirect stdout/stderr
        std::string script(
        "class StreamCatcher:\n"
        "   def __init__(self):\n"
        "       self.value = ''\n"
        "   def write(self,txt):\n"
        "       self.value += txt\n"
        "   def clear(self):\n"
        "       self.value = ''\n"
        "catchOut = StreamCatcher()\n"
        "catchErr = StreamCatcher()\n"
        "sys.stdout = catchOut\n"
        "sys.stderr = catchErr\n");
        ok = Python::interpretPythonScript(script,&err,0);
        assert(ok);
        if (!ok) {
            throw std::runtime_error("Error while loading StreamCatcher: " + err);
        }
    }
}

void
AppManager::tearDownPython()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
        return;
#endif
    ///See http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    PyGILState_Ensure();
    
    Py_DECREF(_imp->mainModule);
    Py_Finalize();
}

PyObject*
AppManager::getMainModule()
{
    return _imp->mainModule;
}

///The symbol has been generated by Shiboken in  Engine/NatronEngine/natronengine_module_wrapper.cpp
extern "C"
{
#ifndef IS_PYTHON_2
    PyObject* PyInit_NatronEngine();
#else
    void initNatronEngine();
#endif
}

void
AppManager::initBuiltinPythonModules()
{
#ifndef IS_PYTHON_2
    int ret = PyImport_AppendInittab(NATRON_ENGINE_PYTHON_MODULE_NAME,&PyInit_NatronEngine);
#else
    int ret = PyImport_AppendInittab(NATRON_ENGINE_PYTHON_MODULE_NAME,&initNatronEngine);
#endif
    if (ret == -1) {
        throw std::runtime_error("Failed to initialize built-in Python module.");
    }
}

void
AppManager::setProjectCreatedDuringRC2Or3(bool b)
{
    _imp->lastProjectLoadedCreatedDuringRC2Or3 = b;
}

//To by-pass a bug introduced in RC3 with the serialization of bezier curves
bool
AppManager::wasProjectCreatedDuringRC2Or3() const
{
    return _imp->lastProjectLoadedCreatedDuringRC2Or3;
}

void
AppManager::toggleAutoHideGraphInputs()
{
    std::map<int,AppInstanceRef> copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }
    for (std::map<int,AppInstanceRef>::iterator it = copy.begin(); it != copy.end(); ++it) {
        it->second.app->toggleAutoHideGraphInputs();
    }
}


void
AppManager::launchPythonInterpreter()
{
    std::string err;
    std::string s = "app = app1\n";
    bool ok = Python::interpretPythonScript(s, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::launchPythonInterpreter(): interpretPythonScript("+s+" failed!");
    }

   // PythonGILLocker pgl;
    
    Py_Main(1, &_imp->args[0]);
}

int
AppManager::isProjectAlreadyOpened(const std::string& projectFilePath) const
{
    QMutexLocker k(&_imp->_appInstancesMutex);
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        boost::shared_ptr<Project> proj = it->second.app->getProject();
        if (proj) {
            QString path = proj->getProjectPath();
            QString name = proj->getProjectFilename();
            std::string existingProject = path.toStdString() + name.toStdString();
            if (existingProject == projectFilePath) {
                return it->first;
            }
        }
    }
    return -1;
}


void
AppManager::onCrashReporterNoLongerResponding()
{
#ifdef NATRON_USE_BREAKPAD
    //Crash reporter seems to no longer exist, quit
    exitApp(false);
#endif
}


void
AppManager::setOnProjectLoadedCallback(const std::string& pythonFunc)
{
    _imp->_settings->setOnProjectLoadedCB(pythonFunc);
}

void
AppManager::setOnProjectCreatedCallback(const std::string& pythonFunc)
{
    _imp->_settings->setOnProjectCreatedCB(pythonFunc);
}


OFX::Host::ImageEffect::Descriptor*
AppManager::getPluginContextAndDescribe(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                                ContextEnum* ctx)
{
    return _imp->ofxHost->getPluginContextAndDescribe(plugin, ctx);
}

std::list<std::string>
AppManager::getNatronPath()
{
    std::list<std::string> ret;
    QStringList p = appPTR->getAllNonOFXPluginsPaths();
    for (QStringList::iterator it = p.begin(); it != p.end(); ++it) {
        ret.push_back(it->toStdString());
    }
    return ret;
}

void
AppManager::appendToNatronPath(const std::string& path)
{
    appPTR->getCurrentSettings()->appendPythonGroupsPath(path);
}


#ifdef __NATRON_WIN32__
void
AppManager::registerUNCPath(const QString& path, const QChar& driveLetter)
{
        assert(QThread::currentThread() == qApp->thread());
        _imp->uncPathMapping[driveLetter] = path;
}

QString
AppManager::mapUNCPathToPathWithDriveLetter(const QString& uncPath) const
{
    assert(QThread::currentThread() == qApp->thread());
    if (uncPath.isEmpty()) {
        return uncPath;
    }
    for (std::map<QChar,QString>::const_iterator it = _imp->uncPathMapping.begin(); it!= _imp->uncPathMapping.end(); ++it) {
        int index = uncPath.indexOf(it->second);
        if (index == 0) {
            //We found the UNC mapping at the start of the path, replace it with a drive letter
            QString ret = uncPath;
            ret.remove(0,it->second.size());
            QString drive;
            drive.append(it->first);
            drive.append(':');
            if (!ret.isEmpty() && !ret.startsWith('/')) {
                drive.append('/');
            }
            ret.prepend(drive);
            return ret;
        }
    }
    return uncPath;
}
#endif

std::string
AppManager::isImageFileSupportedByNatron(const std::string& ext)
{
    std::string readId;
    try {
        readId = appPTR->getCurrentSettings()->getReaderPluginIDForFileType(ext);
    } catch (...) {
        return std::string();
    }
    return readId;
}

AppTLS*
AppManager::getAppTLS() const
{
    return &_imp->globalTLS;
}

bool
AppManager::hasThreadsRendering() const
{
    std::map<int,AppInstanceRef> copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }
    for (std::map<int,AppInstanceRef>::const_iterator it = copy.begin(); it!=copy.end(); ++it) {
        if (it->second.app->getProject()->hasNodeRendering()) {
            return true;
        }
    }
    return false;
}

const NATRON_NAMESPACE::OfxHost*
AppManager::getOFXHost() const
{
    return _imp->ofxHost.get();
}

void
Dialogs::errorDialog(const std::string & title,
            const std::string & message,
            bool useHtml)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->errorDialog(title,message,useHtml);
    } else {
        std::cerr << "ERROR: " << title << ": " <<  message << std::endl;
    }
}

void
Dialogs::errorDialog(const std::string & title,
            const std::string & message,
            bool* stopAsking,
            bool useHtml)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->errorDialog(title,message,stopAsking,useHtml);
    } else {
        std::cerr << "ERROR: " << title << ": " <<  message << std::endl;
    }
}

void
Dialogs::warningDialog(const std::string & title,
              const std::string & message,
              bool useHtml)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->warningDialog(title,message,useHtml);
    } else {
        std::cerr << "WARNING: " << title << ": " << message << std::endl;
    }
}

void
Dialogs::warningDialog(const std::string & title,
              const std::string & message,
              bool* stopAsking,
              bool useHtml)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->warningDialog(title,message, stopAsking,useHtml);
    } else {
        std::cerr << "WARNING: " << title << " :" << message << std::endl;
    }
}

void
Dialogs::informationDialog(const std::string & title,
                  const std::string & message,
                  bool useHtml)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->informationDialog(title,message,useHtml);
    } else {
        std::cout << "INFO: " << title << " :" << message << std::endl;
    }
}

void
Dialogs::informationDialog(const std::string & title,
                  const std::string & message,
                  bool* stopAsking,
                  bool useHtml)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->informationDialog(title,message,stopAsking,useHtml);
    } else {
        std::cout << "INFO: " << title << " :" << message << std::endl;
    }
}

StandardButtonEnum
Dialogs::questionDialog(const std::string & title,
               const std::string & message,
               bool useHtml,
               StandardButtons buttons,
               StandardButtonEnum defaultButton)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        return topLvlInstance->questionDialog(title,message,useHtml,buttons,defaultButton);
    } else {
        std::cout << "QUESTION ASKED: " << title << " :" << message << std::endl;
        std::cout << NATRON_APPLICATION_NAME " answered yes." << std::endl;

        return eStandardButtonYes;
    }
}

StandardButtonEnum
Dialogs::questionDialog(const std::string & title,
               const std::string & message,
               bool useHtml,
               StandardButtons buttons,
               StandardButtonEnum defaultButton,
               bool* stopAsking)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        return topLvlInstance->questionDialog(title,message,useHtml,buttons,defaultButton,stopAsking);
    } else {
        std::cout << "QUESTION ASKED: " << title << " :" << message << std::endl;
        std::cout << NATRON_APPLICATION_NAME " answered yes." << std::endl;

        return eStandardButtonYes;
    }
}

#if 0 // dead code
std::size_t
Python::findNewLineStartAfterImports(std::string& script)
{
    ///Find position of the last import
    size_t foundImport = script.find("import ");
    if (foundImport != std::string::npos) {
        for (;;) {
            size_t found = script.find("import ",foundImport + 1);
            if (found == std::string::npos) {
                break;
            } else {
                foundImport = found;
            }
        }
    }
    
    if (foundImport == std::string::npos) {
        return 0;
    }
    
    ///find the next end line aftr the import
    size_t endLine = script.find('\n',foundImport + 1);
    
    
    if (endLine == std::string::npos) {
        //no end-line, add one
        script.append("\n");
        return script.size();
    } else {
        return endLine + 1;
    }
}
#endif

PyObject*
Python::getMainModule()
{
    return appPTR->getMainModule();
}

#if 0 // dead code
std::size_t
Python::ensureScriptHasModuleImport(const std::string& moduleName,std::string& script)
{
    /// import module
    script = "from " + moduleName + " import * \n" + script;
    return Python::findNewLineStartAfterImports(script);
}
#endif

bool
Python::interpretPythonScript(const std::string& script,std::string* error,std::string* output)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return true;
#endif
    PythonGILLocker pgl;
    
    PyObject* mainModule = Python::getMainModule();
    PyObject* dict = PyModule_GetDict(mainModule);
    
    ///This is faster than PyRun_SimpleString since is doesn't call PyImport_AddModule("__main__")
    PyObject* v = PyRun_String(script.c_str(), Py_file_input, dict, 0);
    if (v) {
        Py_DECREF(v);
    }
    if (!appPTR->isBackground()) {
        
        ///Gui session, do stdout, stderr redirection
        PyObject *errCatcher = 0;
        PyObject *outCatcher = 0;

        if (error && PyObject_HasAttrString(mainModule, "catchErr")) {
            errCatcher = PyObject_GetAttrString(mainModule,"catchErr"); //get our catchOutErr created above, new ref
        }
        
        if (output && PyObject_HasAttrString(mainModule, "catchOut")) {
            outCatcher = PyObject_GetAttrString(mainModule,"catchOut"); //get our catchOutErr created above, new ref
        }
        
        PyErr_Print(); //make python print any errors
        
        PyObject *errorObj = 0;
        if (errCatcher && error) {
            errorObj = PyObject_GetAttrString(errCatcher,"value"); //get the  stderr from our catchErr object, new ref
            assert(errorObj);
            *error = PY3String_asString(errorObj);
            PyObject* unicode = PyUnicode_FromString("");
            PyObject_SetAttrString(errCatcher, "value", unicode);
            Py_DECREF(errorObj);
            Py_DECREF(errCatcher);
        }
        PyObject *outObj = 0;
        if (outCatcher && output) {
            outObj = PyObject_GetAttrString(outCatcher,"value"); //get the stdout from our catchOut object, new ref
            assert(outObj);
            *output = PY3String_asString(outObj);
            PyObject* unicode = PyUnicode_FromString("");
            PyObject_SetAttrString(outCatcher, "value", unicode);
            Py_DECREF(outObj);
            Py_DECREF(outCatcher);
        }

        if (error && !error->empty()) {
            *error = "While executing script:\n" + script + "Python error:\n" + *error;
            return false;
        }
        return true;
    } else {
        if (PyErr_Occurred()) {
            PyErr_Print();
            return false;
        } else {
            return true;
        }
    }

}

#if 0 // dead code
void
Python::compilePyScript(const std::string& script,PyObject** code)
{
    ///Must be locked
    assert(PyThreadState_Get());
    
    *code = (PyObject*)Py_CompileString(script.c_str(), "<string>", Py_file_input);
    if (PyErr_Occurred() || !*code) {
#ifdef DEBUG
        PyErr_Print();
#endif
        throw std::runtime_error("failed to compile the script");
    }
}
#endif

    
std::string
Python::makeNameScriptFriendly(const std::string& str)
{
    if (str == "from") {
        return "pFrom";
    } else if (str == "lambda") {
        return "pLambda";
    }
    ///Remove any non alpha-numeric characters from the baseName
    std::locale loc;
    std::string cpy;
    for (std::size_t i = 0; i < str.size(); ++i) {
        
        ///Ignore starting digits
        if (cpy.empty() && std::isdigit(str[i],loc)) {
            cpy.push_back('p');
            cpy.push_back(str[i]);
            continue;
        }
        
        ///Spaces becomes underscores
        if (std::isspace(str[i],loc)){
            cpy.push_back('_');
        }
        
        ///Non alpha-numeric characters are not allowed in python
        else if (str[i] == '_' || std::isalnum(str[i], loc)) {
            cpy.push_back(str[i]);
        }
    }
    return cpy;
}

PythonGILLocker::PythonGILLocker()
//    : state(PyGILState_UNLOCKED)
{
    appPTR->takeNatronGIL();
//    ///Take the GIL for this thread
//    state = PyGILState_Ensure();
//    assert(PyThreadState_Get());
//#if !defined(NDEBUG) && PY_VERSION_HEX >= 0x030400F0
//    assert(PyGILState_Check()); // Not available prior to Python 3.4
//#endif
}
    
PythonGILLocker::~PythonGILLocker()
{
    appPTR->releaseNatronGIL();
    
//#if !defined(NDEBUG) && PY_VERSION_HEX >= 0x030400F0
//    assert(PyGILState_Check());  // Not available prior to Python 3.4
//#endif
//    
//    ///Release the GIL, no thread will own it afterwards.
//    PyGILState_Release(state);
}
    
static bool getGroupInfosInternal(const std::string& modulePath,
                                  const std::string& pythonModule,
                                  std::string* pluginID,
                                  std::string* pluginLabel,
                                  std::string* iconFilePath,
                                  std::string* grouping,
                                  std::string* description,
                                  unsigned int* version)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return false;
#endif
    PythonGILLocker pgl;
    
    QString script("import sys\n"
                   "import %1\n"
                   "ret = True\n"
                   "if not hasattr(%1,\"createInstance\") or not hasattr(%1.createInstance,\"__call__\"):\n"
                   "    ret = False\n"
                   "if not hasattr(%1,\"getLabel\") or not hasattr(%1.getLabel,\"__call__\"):\n"
                   "    ret = False\n"
                   "templateLabel=\"\"\n"
                   "if ret == True:\n"
                   "    templateLabel = %1.getLabel()\n"
                   "pluginID = templateLabel\n"
                   "version = 1\n"
                   "if hasattr(%1,\"getVersion\") and hasattr(%1.getVersion,\"__call__\"):\n"
                   "    version = %1.getVersion()\n"
                   "description=\"\"\n"
                   "if hasattr(%1,\"getPluginDescription\") and hasattr(%1.getPluginDescription,\"__call__\"):\n"
                   "    description = %1.getPluginDescription()\n"
                   "elif hasattr(%1,\"getDescription\") and hasattr(%1.getDescription,\"__call__\"):\n" // Check old function name for compat
                   "    description = %1.getDescription()\n"
                   "if hasattr(%1,\"getPluginID\") and hasattr(%1.getPluginID,\"__call__\"):\n"
                   "    pluginID = %1.getPluginID()\n"
                   "if ret == True and hasattr(%1,\"getIconPath\") and hasattr(%1.getIconPath,\"__call__\"):\n"
                   "    global templateIcon\n"
                   "    templateIcon = %1.getIconPath()\n"
                   "if ret == True and hasattr(%1,\"getGrouping\") and hasattr(%1.getGrouping,\"__call__\"):\n"
                   "    global templateGrouping\n"
                   "    templateGrouping =  %1.getGrouping()\n");
    
    std::string toRun = script.arg(pythonModule.c_str()).toStdString();
    
    std::string err;
    if (!Python::interpretPythonScript(toRun, &err, 0)) {
        QString logStr("Failure when loading ");
        logStr.append(pythonModule.c_str());
        logStr.append(": ");
        logStr.append(err.c_str());
        appPTR->writeToOfxLog_mt_safe(logStr);
        qDebug() << logStr;
        return false;
    }
    
    PyObject* mainModule = Python::getMainModule();
    PyObject* retObj = PyObject_GetAttrString(mainModule,"ret"); //new ref
    assert(retObj);
    if (PyObject_IsTrue(retObj) == 0) {
        Py_XDECREF(retObj);
        return false;
    }
    Py_XDECREF(retObj);
    
    std::string deleteScript("del ret\n"
                             "del templateLabel\n");
    
    
    PyObject* labelObj = 0;
    labelObj = PyObject_GetAttrString(mainModule,"templateLabel"); //new ref
    
    PyObject* idObj = 0;
    idObj = PyObject_GetAttrString(mainModule,"pluginID"); //new ref
    
    PyObject* iconObj = 0;
    if (PyObject_HasAttrString(mainModule, "templateIcon")) {
        iconObj = PyObject_GetAttrString(mainModule,"templateIcon"); //new ref
    }
    PyObject* iconGrouping = 0;
    if (PyObject_HasAttrString(mainModule, "templateGrouping")) {
        iconGrouping = PyObject_GetAttrString(mainModule,"templateGrouping"); //new ref
    }
    
    PyObject* versionObj = 0;
    if (PyObject_HasAttrString(mainModule, "version")) {
        versionObj = PyObject_GetAttrString(mainModule,"version"); //new ref
    }
    
    PyObject* pluginDescriptionObj = 0;
    if (PyObject_HasAttrString(mainModule, "description")) {
        pluginDescriptionObj = PyObject_GetAttrString(mainModule,"description"); //new ref
    }
    
    assert(labelObj);
    
    *pluginLabel = Python::PY3String_asString(labelObj);
    Py_XDECREF(labelObj);
    
    if (idObj) {
        *pluginID = Python::PY3String_asString(idObj);
        deleteScript.append("del pluginID\n");
        Py_XDECREF(idObj);
    }
    
    if (iconObj) {
        *iconFilePath = Python::PY3String_asString(iconObj);
        QFileInfo iconInfo(QString(modulePath.c_str()) + QString(iconFilePath->c_str()));
        *iconFilePath =  iconInfo.canonicalFilePath().toStdString();
        
        deleteScript.append("del templateIcon\n");
        Py_XDECREF(iconObj);
    }
    if (iconGrouping) {
        *grouping = Python::PY3String_asString(iconGrouping);
        deleteScript.append("del templateGrouping\n");
        Py_XDECREF(iconGrouping);
    }
    
    if (versionObj) {
        *version = (unsigned int)PyLong_AsLong(versionObj);
        deleteScript.append("del version\n");
        Py_XDECREF(versionObj);
    }
    
    if (pluginDescriptionObj) {
        *description = Python::PY3String_asString(pluginDescriptionObj);
        deleteScript.append("del description\n");
        Py_XDECREF(pluginDescriptionObj);
    }
    
    if (grouping->empty()) {
        *grouping = PLUGIN_GROUP_OTHER;
    }
    
    
    bool ok = Python::interpretPythonScript(deleteScript, &err, NULL);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("getGroupInfos(): interpretPythonScript("+deleteScript+" failed!");
    }
    return true;
    
}
  
    
static
bool
getGroupInfosFromQtResourceFile(const std::string& resourceFileName,
                                const std::string& modulePath,
                                const std::string& pythonModule,
                                std::string* pluginID,
                                std::string* pluginLabel,
                                std::string* iconFilePath,
                                std::string* grouping,
                                std::string* description,
                                unsigned int* version)
{
    QString qModulePath(resourceFileName.c_str());
    assert(qModulePath.startsWith(":/Resources"));
    
    QFile moduleContent(qModulePath);
    if (!moduleContent.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QByteArray utf8bytes = QString(pythonModule.c_str()).toUtf8();
    char *moduleName = utf8bytes.data();
    
    PyObject* moduleCode = Py_CompileString(moduleContent.readAll().constData(), moduleName, Py_file_input);
    if (!moduleCode) {
        return false;
    }
    PyObject* module = PyImport_ExecCodeModule(moduleName, moduleCode);
    if (!module) {
        return false;
    }
    
    //Now that the module is loaded, use the regular version
    return getGroupInfosInternal(modulePath,pythonModule, pluginID, pluginLabel, iconFilePath, grouping, description, version);
    //PyDict_SetItemString(priv->globals, moduleName, module);
    
}
    
bool
Python::getGroupInfos(const std::string& modulePath,
              const std::string& pythonModule,
              std::string* pluginID,
              std::string* pluginLabel,
              std::string* iconFilePath,
              std::string* grouping,
              std::string* description,
              unsigned int* version)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return false;
#endif
    
    {
        std::string tofind(":/Resources");
        if (modulePath.substr(0,tofind.size()) == tofind) {
            std::string resourceFileName = modulePath + pythonModule + ".py";
            return getGroupInfosFromQtResourceFile(resourceFileName, modulePath, pythonModule, pluginID, pluginLabel, iconFilePath, grouping, description, version);
        }
    }
    
    return getGroupInfosInternal(modulePath, pythonModule, pluginID, pluginLabel, iconFilePath, grouping, description, version);
    
}


    
void
Python::getFunctionArguments(const std::string& pyFunc,std::string* error,std::vector<std::string>* args)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return;
#endif
    std::stringstream ss;
    ss << "import inspect\n";
    ss << "args_spec = inspect.getargspec(" << pyFunc << ")\n";
    std::string script = ss.str();
    std::string output;
    bool ok = Python::interpretPythonScript(script, error, &output);
    if (!ok) {
        throw std::runtime_error("Python::getFunctionArguments(): interpretPythonScript("+script+" failed!");
    }
    PyObject* mainModule = Python::getMainModule();
    PyObject* args_specObj = 0;
    if (PyObject_HasAttrString(mainModule, "args_spec")) {
        args_specObj = PyObject_GetAttrString(mainModule,"args_spec");
    }
    assert(args_specObj);
    PyObject* argListObj = 0;

    if (args_specObj) {
        argListObj = PyTuple_GetItem(args_specObj, 0);
        assert(argListObj);
        if (argListObj) {
           // size = PyObject_Size(argListObj)
            assert(PyList_Check(argListObj));
            Py_ssize_t size = PyList_Size(argListObj);
            for (Py_ssize_t i = 0; i < size; ++i) {
                PyObject* itemObj = PyList_GetItem(argListObj, i);
                assert(itemObj);
                if (itemObj) {
                    std::string itemName = PY3String_asString(itemObj);
                    assert(!itemName.empty());
                    if (!itemName.empty()) {
                        args->push_back(itemName);
                    }
                }
            }
            if (PyTuple_GetItem(args_specObj, 1) != Py_None || PyTuple_GetItem(args_specObj, 2) != Py_None) {
                error->append("Function contains variadic arguments which is unsupported.");
                return;
            }
            
        }
    }
}
    
/**
 * @brief Given a fullyQualifiedName, e.g: app1.Group1.Blur1
 * this function returns the PyObject attribute of Blur1 if it is defined, or Group1 otherwise
 * If app1 or Group1 does not exist at this point, this is a failure.
 **/
PyObject*
Python::getAttrRecursive(const std::string& fullyQualifiedName,PyObject* parentObj,bool* isDefined)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return 0;
#endif
    std::size_t foundDot = fullyQualifiedName.find(".");
    std::string attrName = foundDot == std::string::npos ? fullyQualifiedName : fullyQualifiedName.substr(0, foundDot);
    PyObject* obj = 0;
    if (PyObject_HasAttrString(parentObj, attrName.c_str())) {
        obj = PyObject_GetAttrString(parentObj, attrName.c_str());
    }
    
    ///We either found the parent object or we are on the last object in which case we return the parent
    if (!obj) {
        //assert(fullyQualifiedName.find(".") == std::string::npos);
        *isDefined = false;
        return parentObj;
    } else {
        std::string recurseName;
        if (foundDot != std::string::npos) {
            recurseName = fullyQualifiedName;
            recurseName.erase(0, foundDot + 1);
        }
        if (!recurseName.empty()) {
            return Python::getAttrRecursive(recurseName, obj, isDefined);
        } else {
            *isDefined = true;
            return obj;
        }
    }
    
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_AppManager.cpp"
