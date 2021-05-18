/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "Global/Macros.h"

#include <list>
#include <string>
#include <vector>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include <QtCore/QtGlobal> // for Q_OS_*
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QAtomicInt>
#include <QtCore/QCoreApplication>


#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include "Engine/OSGLContext_x11.h"
#elif defined(Q_OS_WIN32)
#include "Engine/OSGLContext_win.h"
#elif defined(Q_OS_MAC)
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
#if defined(Q_OS_MAC)
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

NATRON_NAMESPACE_ENTER

struct AppManagerPrivate
{
    Q_DECLARE_TR_FUNCTIONS(AppManagerPrivate)

public:
    typedef Cache<Image> ImageCache;
    typedef boost::shared_ptr<ImageCache> ImageCachePtr;
    typedef Cache<FrameEntry> FrameEntryCache;
    typedef boost::shared_ptr<FrameEntryCache> FrameEntryCachePtr;

    AppTLS globalTLS;
    AppManager::AppTypeEnum _appType; //< the type of app
    mutable QMutex _appInstancesMutex;
    std::vector<AppInstancePtr> _appInstances; //< the instances mapped against their ID
    int _availableID; //< the ID for the next instance
    int _topLevelInstanceID; //< the top level app ID
    SettingsPtr _settings; //< app settings
    std::vector<Format> _formats; //<a list of the "base" formats available in the application
    PluginsMap _plugins; //< list of the plugins
    IOPluginsMap readerPlugins; // for all reader plug-ins which are best suited for each format
    IOPluginsMap writerPlugins; // for all writer plug-ins which are best suited for each format
    boost::scoped_ptr<OfxHost> ofxHost; //< OpenFX host
    boost::scoped_ptr<KnobFactory> _knobFactory; //< knob maker
    ImageCachePtr _nodeCache; //< Images cache
    ImageCachePtr _diskCache; //< Images disk cache (used by DiskCache nodes)
    FrameEntryCachePtr _viewerCache; //< Viewer textures cache
    mutable QMutex diskCachesLocationMutex;
    QString diskCachesLocation;
    boost::scoped_ptr<ProcessInputChannel> _backgroundIPC; //< object used to communicate with the main app
    //if this app is background, see the ProcessInputChannel def
    bool _loaded; //< true when the first instance is completely loaded.
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

    ///Python needs wide strings as from Python 3.x onwards everything is unicode based
#if PY_MAJOR_VERSION >= 3
    // Python 3
    std::vector<wchar_t*> commandLineArgsWideOriginal; // must be free'd on exit
    std::vector<wchar_t*> commandLineArgsWide; // a copy of the above, may be modified
#else
    std::vector<char*> commandLineArgsUtf8Original; // must be free'd on exit
    std::vector<char*> commandLineArgsUtf8; // a copy of the above, may be modified
#endif

    //  QCoreApplication will hold a reference to that int until QCoreApplication destructor is invoked.
    //  Thus ensure that the QCoreApplication is destroyed before AppManager itself
    int nArgs;

    PyObject* mainModule;
    PyThreadState* mainThreadState;

#ifdef NATRON_USE_BREAKPAD
    QString breakpadProcessExecutableFilePath;
    Q_PID breakpadProcessPID;
    boost::shared_ptr<google_breakpad::ExceptionHandler> breakpadHandler;
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

#ifdef Q_OS_WIN32
    boost::scoped_ptr<OSGLContext_wgl_data> wglInfo;
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    boost::scoped_ptr<OSGLContext_glx_data> glxInfo;
#endif

    boost::scoped_ptr<GPUContextPool> renderingContextPool;
    std::list<OpenGLRendererInfo> openGLRenderers;
    boost::scoped_ptr<QCoreApplication> _qApp;

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

    void tearDownGL();

    void setViewerCacheTileSize();

    void handleCommandLineArgs(int argc, char** argv);
    void handleCommandLineArgsW(int argc, wchar_t** argv);

    void copyUtf8ArgsToMembers(const std::vector<std::string>& utf8Args);
};

NATRON_NAMESPACE_EXIT

#endif // Engine_AppManagerPrivate_h

