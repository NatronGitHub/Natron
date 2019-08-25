/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QtGlobal> // for Q_OS_*
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QAtomicInt>
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(uninitialized)


#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include "Engine/OSGLContext_x11.h"
#elif defined(Q_OS_WIN32)
#include "Engine/OSGLContext_win.h"
#elif defined(Q_OS_MAC)
#include "Engine/OSGLContext_mac.h"
#endif

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/ExistenceCheckThread.h"
#include "Engine/StorageDeleterThread.h"
#include "Engine/Image.h"
#include "Engine/GPUContextPool.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/TreeRenderQueueManager.h"
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

    AppTLS globalTLS;

    AppManager::AppTypeEnum _appType; //< the type of app

    mutable QMutex _appInstancesMutex;

    AppInstanceVec _appInstances; //< the instances mapped against their ID

    int _availableID; //< the ID for the next instance

    int _topLevelInstanceID; //< the top level app ID

    SettingsPtr _settings; //< app settings

    std::vector<Format> _formats; //<a list of the "base" formats available in the application

    PluginsMap _plugins; //< list of the plugins

    IOPluginsMap readerPlugins; // for all reader plug-ins which are best suited for each format

    IOPluginsMap writerPlugins; // for all writer plug-ins which are best suited for each format

    boost::scoped_ptr<OfxHost> ofxHost; //< OpenFX host

    boost::shared_ptr<TLSHolder<AppManager::PythonTLSData> > pythonTLS;

    // Multi-thread handler
    boost::scoped_ptr<MultiThread> multiThreadSuite;

    boost::scoped_ptr<KnobFactory> _knobFactory; //< knob maker

    CacheBasePtr generalPurposeCache, tileCache;

    boost::scoped_ptr<MappedProcessWatcherThread> mappedProcessWatcher;

    boost::scoped_ptr<StorageDeleterThread> storageDeleteThread; // thread used to kill cache entries without blocking a render thread

    boost::scoped_ptr<ProcessInputChannel> _backgroundIPC; //< object used to communicate with the main app

    //if this app is background, see the ProcessInputChannel def
    bool _loaded; //< true when the first instance is completely loaded.

    std::string binaryPath;

    mutable QMutex errorLogMutex;

    std::list<LogEntry> errorLog;

    std::string currentOCIOConfigPath; //< the currentOCIO config path

    int hardwareThreadCount,physicalThreadCount;

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
    qint64 breakpadProcessPID;
    boost::shared_ptr<google_breakpad::ExceptionHandler> breakpadHandler;
    boost::shared_ptr<ExistenceCheckerThread> breakpadAliveThread;
#endif

#ifdef USE_NATRON_GIL
    QMutex natronPythonGIL;
#endif
    int pythonGILRCount;

#ifdef Q_OS_WIN32
    //On Windows only, track the UNC path we came across because the WIN32 API does not provide any function to map
    //from UNC path to path with drive letter.
    std::map<QChar, QString> uncPathMapping;
#endif

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
    int glVersionMajor,glVersionMinor;

#ifdef Q_OS_WIN32
    boost::scoped_ptr<OSGLContext_wgl_data> wglInfo;
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    boost::scoped_ptr<OSGLContext_glx_data> glxInfo;
#endif

    boost::scoped_ptr<GPUContextPool> renderingContextPool;
    std::list<OpenGLRendererInfo> openGLRenderers;
    boost::scoped_ptr<QCoreApplication> _qApp;


    // User add custom menu entries that point to python commands
    std::list<PythonUserCommand> pythonCommands;

    // The application global manager that schedules render and maximizes CPU utilization
    TreeRenderQueueManagerPtr tasksQueueManager;

public:
    AppManagerPrivate();

    ~AppManagerPrivate();

    void initProcessInputChannel(const QString & mainProcessServerName);

    void loadBuiltinFormats();

    static void addOpenGLRequirementsString(QString& str, OpenGLRequirementsTypeEnum type, bool displayRenderers);


    /**
     * @brief Called on startup to initialize the max opened files
     **/
    void setMaxCacheFiles();

    PluginPtr findPluginById(const std::string& oldId, int major, int minor) const;

    void declareSettingsToPython();

#ifdef NATRON_USE_BREAKPAD
    void initBreakpad(const QString& breakpadPipePath, const QString& breakpadComPipePath, int breakpad_client_fd);

    void createBreakpadHandler(const QString& breakpadPipePath, int breakpad_client_fd);
#endif

    void initGl(bool checkRenderingReq);

    void initGLAPISpecific();

    void tearDownGL();

    void handleCommandLineArgs(int argc, char** argv);
    void handleCommandLineArgsW(int argc, wchar_t** argv);

    void copyUtf8ArgsToMembers(const std::vector<std::string>& utf8Args);
};

NATRON_NAMESPACE_EXIT

#endif // Engine_AppManagerPrivate_h
