/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_AppManagerPrivate_h
#define Engine_AppManagerPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <atomic>

#include "Global/Macros.h"

#include <list>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <condition_variable>
#include <mutex>
#include <thread>

#include <QtGlobal> // for Q_OS_*
#include <QMutex>
#include <QString>
#include <QAtomicInt>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>


#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include "Engine/OSGLContext_wayland.h"
#include "Engine/OSGLContext_x11.h"
#elif defined(Q_OS_WIN32)
#include "Engine/OSGLContext_win.h"
#elif defined(Q_OS_DARWIN)
#include "Engine/OSGLContext_mac.h"
#endif

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/FrameEntry.h"
#include "Engine/Image.h"
#include "Engine/GPUContextPool.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/TLSHolder.h"

// include breakpad after Engine, because it includes /usr/include/AssertMacros.h on OS X which defines a check(x) macro, which conflicts with boost
#ifdef NATRON_USE_BREAKPAD
#if defined(Q_OS_DARWIN)
GCC_DIAG_OFF(deprecated)
#include "client/mac/handler/exception_handler.h"
GCC_DIAG_ON(deprecated)
#elif defined(Q_OS_LINUX)
#include <fcntl.h>
#include "client/linux/handler/exception_handler.h"
#include "client/linux/crash_generation/crash_generation_server.h"
#elif defined(Q_OS_WIN32)
#include "client/windows/handler/exception_handler.h"
#endif
#endif

#include "Engine/EngineFwd.h"
#include "Global/FloatingPointExceptions.h"

NATRON_NAMESPACE_ENTER

struct AppManagerPrivate
{
    Q_DECLARE_TR_FUNCTIONS(AppManagerPrivate)

public:
    typedef Cache<Image> ImageCache;
    typedef std::shared_ptr<ImageCache> ImageCachePtr;
    typedef Cache<FrameEntry> FrameEntryCache;
    typedef std::shared_ptr<FrameEntryCache> FrameEntryCachePtr;

#ifdef DEBUG
    // Test mode, debug builds: no trapped floating point exception, in the main thread (which main()
    // enables) or in the threads that trap them (trapping_disabled()). Mesa llvmpipe's threads inherit
    // the floating point environment of the thread that starts them (a context creation, the first draw
    // or shader compilation), and their JIT-compiled shaders raise exceptions (SIGFPE). The first
    // member, so that it is destroyed last.
    std::unique_ptr<boost_adaptbx::floating_point::no_exception_trapping> testModeNoTrap;
#endif
    AppTLS globalTLS;
    AppManager::AppTypeEnum _appType; //< the type of app
    mutable QMutex _appInstancesMutex;
    std::vector<AppInstancePtr> _appInstances; //< the instances mapped against their ID
    int _availableID; //< the ID for the next instance
    int _topLevelInstanceID; //< the top level app ID
    std::set<int> _internalInstanceIDs; //< internal instances (a Precomp node's, see newBackgroundInstance()), never top-level
    SettingsPtr _settings; //< app settings
    std::vector<Format> _formats; //<a list of the "base" formats available in the application
    PluginsMap _plugins; //< list of the plugins
    IOPluginsMap readerPlugins; // for all reader plug-ins which are best suited for each format
    IOPluginsMap writerPlugins; // for all writer plug-ins which are best suited for each format
    std::unique_ptr<OfxHost> ofxHost; //< OpenFX host
    std::unique_ptr<KnobFactory> _knobFactory; //< knob maker
    ImageCachePtr _nodeCache; //< Images cache
    ImageCachePtr _diskCache; //< Images disk cache (used by DiskCache nodes)
    FrameEntryCachePtr _viewerCache; //< Viewer textures cache
    mutable QMutex diskCachesLocationMutex;
    QString diskCachesLocation;
    // The file that tells the other Natron processes that this one uses the disk caches in diskCachesLocation
    // (see AppManager::isDiskCacheUsedByAnotherProcess()), protected by diskCachesLocationMutex
    std::unique_ptr<QFile> diskCachesInstanceFile;
    // A thread touches diskCachesInstanceFile every diskCachesInstanceFileTouchMs: on a network file system, the other
    // hosts consider it stale if it is not touched. It is a thread because NatronRenderer and the background renders
    // never run an event loop (the main thread blocks while rendering), so a QTimer would never fire there.
    std::thread diskCachesInstanceFileToucher;
    std::mutex diskCachesInstanceFileToucherMutex;
    std::condition_variable diskCachesInstanceFileToucherCond;
    bool diskCachesInstanceFileToucherMustQuit; // protected by diskCachesInstanceFileToucherMutex
    int diskCachesInstanceFileTouchMs; // protected by diskCachesInstanceFileToucherMutex

    void startDiskCachesInstanceFileToucher();
    void touchDiskCachesInstanceFile();
    std::unique_ptr<ProcessInputChannel> _backgroundIPC; //< object used to communicate with the main app
    //if this app is background, see the ProcessInputChannel def
    bool _loaded; //< true when the first instance is completely loaded.
    bool dialogsDisabled; //< --no-dialogs was given
    std::atomic<bool> quitRequested; //< natron.quit() was called
    std::atomic<int> requestedExitCode; //< the exit code given to natron.quit()
    std::atomic<bool> backgroundRenderFailed; //< a render failed in background mode
    std::atomic<int> nestedEventLoops; //< see AppManager::NestedEventLoopRAII
    bool pythonInterpreterRunning; //< Py_Main() of the -t option is running
    QString _binaryPath; //< the path to the application's binary
    U64 _nodesGlobalMemoryUse; //< how much memory all the nodes are using (besides the cache)
    mutable QMutex errorLogMutex;
    std::list<LogEntry> errorLog;
    size_t maxCacheFiles; //< the maximum number of files the application can open for caching. This is the hard limit * 0.9
    size_t currentCacheFilesCount; //< the number of cache files currently opened in the application
    mutable QMutex currentCacheFilesCountMutex; //< protects currentCacheFilesCount
    std::string currentOCIOConfigPath; //< the currentOCIO config path
    int idealThreadCount; // return value of QThread::idealThreadCount() cached here
    int nThreadsToRender; // the value held by the corresponding Knob in the Settings, stored here for faster access (3 RW lock vs 1 mutex here)
    int nThreadsPerEffect;  // the value held by the corresponding Knob in the Settings, stored here for faster access (3 RW lock vs 1 mutex here)
    bool useThreadPool; // whether the multi-thread suite should use the global thread pool (of QtConcurrent) or not
    mutable QMutex nThreadsMutex; // protects nThreadsToRender & nThreadsPerEffect & useThreadPool

    //The idea here is to keep track of the number of threads launched by Natron (except the ones of the global thread pool of QtConcurrent)
    //So that we can properly have an estimation of how much the cores of the CPU are used.
    //This method has advantages and drawbacks:
    // Advantages:
    // - This is quick and fast
    // - This very well describes the render activity of Natron
    //
    // Disadvantages:
    // - This only takes into account the current Natron process and disregard completely CPU activity.
    // - We might count a thread that is actually waiting in a mutex as a running thread
    // Another method could be to analyse all cores running, but this is way more expensive and would impair performances.
    QAtomicInt runningThreadsCount;

    //To by-pass a bug introduced in RC2 / RC3 with the serialization of bezier curves
    bool lastProjectLoadedCreatedDuringRC2Or3;

    // ~AppManager runs: it does what AppManager::cleanUpAtPythonExit() would do itself, in its own order (#1557)
    bool destroying;

    // GUI test mode (--test)
    bool testMode;
    std::unique_ptr<QTemporaryDir> testSettingsDir; // test mode: the settings of this run (removed at exit)

    ///Python needs wide strings as from Python 3.x onwards everything is unicode based
#if PY_MAJOR_VERSION >= 3
    // Python 3
    std::vector<wchar_t*> commandLineArgsWideOriginal; // must be free'd on exit
    std::vector<wchar_t*> commandLineArgsWide; // a copy of the above, may be modified
#endif
    std::vector<char*> commandLineArgsUtf8Original; // must be free'd on exit
    std::vector<char*> commandLineArgsUtf8; // a copy of the above, may be modified

    //  QCoreApplication will hold a reference to that int until QCoreApplication destructor is invoked.
    //  Thus ensure that the QCoreApplication is destroyed before AppManager itself
    int nArgs;

    PyObject* mainModule;
    PyThreadState* mainThreadState;

#ifdef NATRON_USE_BREAKPAD
    QString breakpadProcessExecutableFilePath;
    Q_PID breakpadProcessPID;
    std::shared_ptr<google_breakpad::ExceptionHandler> breakpadHandler;
    ExistenceCheckerThreadPtr breakpadAliveThread;
#endif

#ifdef USE_NATRON_GIL
    QMutex natronPythonGIL;
#endif

#ifdef Q_OS_WIN32
    //On Windows only, track the UNC path we came across because the WIN32 API does not provide any function to map
    //from UNC path to path with drive letter.
    std::map<QChar, QString> uncPathMapping;
#endif

    // Copy of the setting knob for faster access from OfxImage constructor
    bool pluginsUseInputImageCopyToRender;

    // True if we can use OpenGL
    struct OpenGLRequirementsData
    {
        QString error;
        bool hasRequirements;

        OpenGLRequirementsData()
        : error()
        , hasRequirements(false)
        {

        }
    };
    std::map<OpenGLRequirementsTypeEnum, OpenGLRequirementsData> glRequirements;
    bool glHasTextureFloat;
    bool hasInitializedOpenGLFunctions;
    mutable QMutex openGLFunctionsMutex;

    bool onWayland;

#ifdef Q_OS_WIN32
    std::unique_ptr<OSGLContext_wgl_data> wglInfo;
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    std::unique_ptr<OSGLContext_egl_data> eglInfo;

    std::unique_ptr<OSGLContext_glx_data> glxInfo;
#endif

    std::unique_ptr<GPUContextPool> renderingContextPool;
    std::list<OpenGLRendererInfo> openGLRenderers;
    std::unique_ptr<QCoreApplication> _qApp;

public:
    AppManagerPrivate();

    ~AppManagerPrivate();

    void initProcessInputChannel(const QString & mainProcessServerName);

    void loadBuiltinFormats();

    void saveCaches();

    void restoreCaches();

    static void addOpenGLRequirementsString(QString& str, OpenGLRequirementsTypeEnum type);

    bool checkForCacheDiskStructure(const QString & cachePath, bool isTiled);

    void cleanUpCacheDiskStructure(const QString & cachePath, bool isTiled);

    /**
     * @brief Called on startup to initialize the max opened files
     **/
    void setMaxCacheFiles();

    Plugin* findPluginById(const QString& oldId, int major, int minor) const;

    void declareSettingsToPython();

#ifdef NATRON_USE_BREAKPAD
    void initBreakpad(const QString& breakpadPipePath, const QString& breakpadComPipePath, int breakpad_client_fd);

    void createBreakpadHandler(const QString& breakpadPipePath, int breakpad_client_fd);
#endif

    void initGl(bool checkRenderingReq);

    void initGLAPISpecific();

    // If the Wayland/EGL off-screen context failed, switch to X11/GLX. Returns false if not possible.
    bool fallBackToGLX(const char* reason);

    void tearDownGL();

    void setViewerCacheTileSize();

    void handleCommandLineArgs(int argc, char** argv);
    void handleCommandLineArgsW(int argc, wchar_t** argv);

    void copyUtf8ArgsToMembers(const std::vector<std::string>& utf8Args);
};

NATRON_NAMESPACE_EXIT

#endif // Engine_AppManagerPrivate_h

