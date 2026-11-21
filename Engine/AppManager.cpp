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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#undef Py_LIMITED_API  // Needed for PyRun_SimpleString, PyRun_String
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AppManager.h"
//#include "AppManagerPrivate.h" // include breakpad after Engine, because it includes /usr/include/AssertMacros.h on OS X which defines a check(x) macro, which conflicts with boost

#if defined(__APPLE__) && defined(_LIBCPP_VERSION)
#include <AvailabilityMacros.h>
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1090
// Disable availability macros on macOS
// because we may be using libc++ on an older macOS,
// so that std::locale::numeric may be available
// even on macOS < 10.9.
// see _LIBCPP_AVAILABILITY_LOCALE_CATEGORY
// in /opt/local/libexec/llvm-5.0/include/c++/v1/__config
// and /opt/local/libexec/llvm-5.0/include/c++/v1/__locale
#if defined(_LIBCPP_USE_AVAILABILITY_APPLE)
#error "this must be compiled with _LIBCPP_DISABLE_AVAILABILITY defined"
#else
#ifndef _LIBCPP_DISABLE_AVAILABILITY
#define _LIBCPP_DISABLE_AVAILABILITY
#endif
#endif
#endif
#endif

#include <clocale>
#include <csignal>
#include <cstddef>
#include <cassert>
#include <stdexcept>
#include <cstring> // for std::memcpy
#include <iostream>
#include <sstream> // stringstream
#include <locale>

#include <QtGlobal> // for Q_OS_*
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
#include <errno.h>
#include <fcntl.h> // fcntl
#include <signal.h> // sigaction, raise
#include <unistd.h> // pipe, read, write
#include <sys/file.h> // flock
#include <sys/stat.h> // fstat, stat
#ifdef __linux__
#include <sys/vfs.h> // statfs
#else
#include <sys/param.h>
#include <sys/mount.h> // statfs, MNT_LOCAL
#endif
#ifdef Q_OS_DARWIN
#include <sys/sysctl.h>
#include <libproc.h>
#endif
#endif

#ifdef Q_OS_WIN
#include <shlobj.h>
#endif

#include <cairo/cairo.h>
#include <boost/version.hpp>
#include <ceres/version.h>
#include <openMVG/version.hpp>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QSysInfo>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QRegularExpression>
#include <QSettings>
#include <QSocketNotifier>
#include <QThreadPool>
#include <QTextStream>
#include <QAbstractSocket>
#include <QLocalServer>
#include <QLocalSocket>


#include "Global/ProcInfo.h"
#include "Global/GLIncludes.h"
#include "Global/StrUtils.h"
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Global/PythonUtils.h"

#include "Engine/AppInstance.h"
#include "Engine/Backdrop.h"
#include "Engine/CLArgs.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Dot.h"
#include "Engine/ExistenceCheckThread.h"
#include "Engine/FileSystemModel.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/JoinViewsNode.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Log.h"
#include "Engine/MemoryInfo.h" // getSystemTotalRAM, printAsRAM
#include "Engine/Node.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/OSGLContext.h"
#include "Engine/OneViewNode.h"
#include "Engine/ProcessHandler.h" // ProcessInputChannel
#include "Engine/Project.h"
#include "Engine/PrecompNode.h"
#include "Engine/ReadNode.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoSmear.h"
#include "Engine/StandardPaths.h"
#include "Engine/TrackerNode.h"
#include "Engine/ThreadPool.h"
#include "Engine/Utils.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h" // RenderStatsMap
#include "Engine/WriteNode.h"

#include "sbkversion.h" // shiboken/pyside version

#include "AppManagerPrivate.h" // include breakpad after Engine, because it includes /usr/include/AssertMacros.h on OS X which defines a check(x) macro, which conflicts with boost

NATRON_NAMESPACE_ENTER

AppManager* AppManager::_instance = 0;

#ifdef __NATRON_UNIX__

// Set by the first termination signal caught while an event loop can quit the application.
static volatile std::sig_atomic_t shutDownSignalCaught = 0;
// Self-pipe that wakes up the main thread, see AppManager::setUpShutDownSignalNotifier().
// The write end is -1 when there is no event loop to quit (background mode).
static int shutDownSignalPipe[2] = { -1, -1 };

//namespace  {
// A signal handler runs on any thread, interrupting whatever it was doing (maybe while
// it holds a lock), so it may only call async-signal-safe functions (see signal-safety(7)).
// Quitting the application from here crashed or deadlocked (#1253).
static void
handleShutDownSignal(int signalId)
{
    const int savedErrno = errno; // write() may change it under the interrupted code
    static const char msg[] = "\nCaught termination signal, exiting!\n";
    ssize_t ret = ::write(STDERR_FILENO, msg, sizeof(msg) - 1);

    (void)ret;
    if ( !shutDownSignalCaught && (shutDownSignalPipe[1] != -1) ) {
        // Let the main thread quit the application, see AppManager::onShutDownSignalReceived().
        shutDownSignalCaught = 1;
        const char c = 0;
        ret = ::write(shutDownSignalPipe[1], &c, 1);
        (void)ret;
        errno = savedErrno;

        return;
    }
    // No event loop can quit the application (background mode, where the main thread
    // may be rendering or running the Python interpreter; the event loop has ended), or
    // the signal was sent again: terminate now, with the default action of the signal.
    // Nothing is cleaned up: the disk cache index is not saved (the disk cache is lost),
    // and the project lock file stays (a lock whose process is gone is ignored).
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_DFL;
    sigaction(signalId, &sa, NULL);
    raise(signalId); // delivered when the handler returns
    errno = savedErrno;
}

// Called when no event loop will run again: from then on, handleShutDownSignal() terminates.
static void
disarmShutDownSignalPipe()
{
    shutDownSignalPipe[1] = -1;
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


#if defined(__NATRON_LINUX__) && !defined(__FreeBSD__)

#define NATRON_UNIX_BACKTRACE_STACK_DEPTH 16

// The address of the faulting instruction, or NULL on architectures not handled here.
static void*
faultInstructionAddress(const ucontext_t* uc)
{
#if defined(__x86_64__)
    return (void*)uc->uc_mcontext.gregs[REG_RIP];
#elif defined(__i386__)
    return (void*)uc->uc_mcontext.gregs[REG_EIP];
#elif defined(__aarch64__)
    return (void*)uc->uc_mcontext.pc;
#elif defined(__arm__)
    return (void*)uc->uc_mcontext.arm_pc;
#else
    (void)uc;
    return NULL;
#endif
}

static void
backTraceSigSegvHandler(int sig,
                        siginfo_t *info,
                        void *secret)
{
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
            faultInstructionAddress(uc) << " from " << info->si_addr << std::endl;
    } else {
        printf("Got signal %d#92;n", sig);
    }

    trace_size = backtrace(trace, NATRON_UNIX_BACKTRACE_STACK_DEPTH);
    /* overwrite sigaction with caller's address */
    void* faultAddress = faultInstructionAddress(uc);
    if (faultAddress && trace_size > 1) {
        trace[1] = faultAddress;
    }


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

#endif // if defined(__NATRON_LINUX__) && !defined(__FreeBSD__)

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

int
AppManager::getMaxThreadCount()
{
    return QThreadPool::globalInstance()->maxThreadCount();
}

AppManager::AppManager()
    : QObject()
    , _imp( new AppManagerPrivate() )
{
    assert(!_instance);
    _instance = this;

    QObject::connect( this, SIGNAL(s_requestOFXDialogOnMainThread(OfxImageEffectInstance*,void*)), this, SLOT(onOFXDialogOnMainThreadReceived(OfxImageEffectInstance*,void*)) );

#ifdef __NATRON_WIN32__
    FileSystemModel::initDriveLettersToNetworkShareNamesMapping();
#endif
}

#ifdef USE_NATRON_GIL
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
#endif

void
StrUtils::ensureLastPathSeparator(QString& path)
{
    static const QChar separator( QLatin1Char('/') );

    if ( !path.endsWith(separator) ) {
        path += separator;
    }
}


bool
AppManager::loadFromArgs(const CLArgs& cl)
{

#ifdef DEBUG
    for (std::size_t i = 0; i < _imp->commandLineArgsWide.size(); ++i) {
        std::cout << "argv[" << i << "] = " << StrUtils::utf16_to_utf8( std::wstring(_imp->commandLineArgsWide[i]) ) << std::endl;
    }
#endif

    // Before creating qApp and before initPython(): the GUI registers the test module only in test mode.
    _imp->testMode = !cl.getTestScript().isEmpty() && !cl.isBackgroundMode();
    if ( _imp->testMode && !qEnvironmentVariableIsEmpty("CI") && (qgetenv("NATRON_GUI_TESTS") != "1") ) {
        // In CI, the GUI tests only run where the job enables them (NATRON_GUI_TESTS=1, e.g. under Xvfb with
        // Mesa): skipped here, before anything is initialized, not in the test script, which runs much later.
        std::cout << "SKIP: GUI tests in CI need NATRON_GUI_TESTS=1" << std::endl;
        _imp->requestedExitCode = 77;
        _imp->quitRequested = true;

        return false;
    }
#if defined(__NATRON_UNIX__) && !defined(__NATRON_OSX__)
    if ( _imp->testMode && qgetenv("DISPLAY").isEmpty() && qgetenv("WAYLAND_DISPLAY").isEmpty() &&
         qgetenv("QT_QPA_PLATFORM").isEmpty() ) {
        // Qt would abort: a GUI test without a display is skipped, not failed
        std::cout << "SKIP: no display (DISPLAY and WAYLAND_DISPLAY are not set): run the GUI tests under xvfb-run" << std::endl;
        _imp->requestedExitCode = 77;
        _imp->quitRequested = true;

        return false;
    }
#endif
    if (_imp->testMode) {
        // The settings written with the default QSettings constructor (e.g. the recent files) go to a
        // temporary directory of this run, removed at exit, not to the user's preferences. The Natron
        // settings are not saved with --no-settings, which --test implies, and the other writers of
        // QSettings(organization, application) skip their writes in test mode.
        _imp->testSettingsDir.reset( new QTemporaryDir( QDir::tempPath() + QString::fromUtf8("/NatronGuiTest-settings-XXXXXX") ) );
        if ( _imp->testSettingsDir->isValid() ) {
            QSettings::setDefaultFormat(QSettings::IniFormat);
            QSettings::setPath( QSettings::IniFormat, QSettings::UserScope, _imp->testSettingsDir->path() );
        } else {
            std::cout << "NatronGuiTest: FAIL: cannot create a temporary settings directory in "
                      << QDir::tempPath().toStdString() << std::endl;
            _imp->requestedExitCode = 1;
            _imp->quitRequested = true;

            return false;
        }
        // A GUI test that fails to start (e.g. an error while loading) fails: only the test runner (or a
        // skip) sets another exit status (see main()).
        _imp->requestedExitCode = 1;
    }
    if ( _imp->testMode && !qEnvironmentVariableIsSet("NATRON_TEST_NO_FOCUS") ) {
        // A GUI test never takes the user's keyboard focus or activates its windows (#1464). Set before
        // the QApplication exists. (--no-dialogs does not imply it.)
        qputenv("NATRON_TEST_NO_FOCUS", "1");
    }
    if (_imp->testMode) {
        // Before initPython(): the output of a test reaches the log even when the watchdog ends it
        qputenv("PYTHONUNBUFFERED", "1");
#ifdef DEBUG
        // No trapped floating point exception, in any thread: before qApp, any OpenGL context and any
        // thread (see AppManagerPrivate::testModeNoTrap)
        boost_adaptbx::floating_point::trapping_disabled() = true;
        // and in the child processes, e.g. NatronRenderer
        qputenv("NATRON_NO_FP_TRAPS", "1");
        _imp->testModeNoTrap.reset(new boost_adaptbx::floating_point::no_exception_trapping);
#endif
    }

    // This needs to be done BEFORE creating qApp because
    // on Linux, X11 will create a context that would corrupt
    // the XUniqueContext created by Qt
    // scoped_ptr
    _imp->renderingContextPool.reset( new GPUContextPool() );
    initializeOpenGLFunctionsOnce(true);

    //  QCoreApplication will hold a reference to that appManagerArgc integer until it dies.
    //  Thus ensure that the QCoreApplication is destroyed when returning this function.
    initializeQApp(_imp->nArgs, &_imp->commandLineArgsUtf8.front()); // calls QCoreApplication::QCoreApplication(), which calls setlocale()
    // see C++ standard 23.2.4.2 vector capacity [lib.vector.capacity]
    // resizing to a smaller size doesn't free/move memory, so the data pointer remains valid
    assert(_imp->nArgs <= (int)_imp->commandLineArgsUtf8.size());
    _imp->commandLineArgsUtf8.resize(_imp->nArgs); // Qt may have reduced the numlber of args

#ifdef QT_CUSTOM_THREADPOOL
    // Set the global thread pool (pointed is owned and deleted by QThreadPool at exit)
    QThreadPool::setGlobalInstance(new ThreadPool);
#endif

    // set fontconfig path on all platforms
    if ( qgetenv("FONTCONFIG_PATH").isNull() ) {
        // set FONTCONFIG_PATH to Natron/Resources/etc/fonts (required by plugins using fontconfig)
        QString path = QCoreApplication::applicationDirPath() + QString::fromUtf8("/../Resources/etc/fonts");
        QFileInfo fileInfo(path);
        if ( !fileInfo.exists() ) {
            std::cerr <<  "Fontconfig configuration file " << path.toStdString() << " does not exist, not setting FONTCONFIG_PATH "<< std::endl;
        } else {
            QString fcPath = fileInfo.canonicalFilePath();

            std::string stdFcPath = fcPath.toStdString();

            qDebug() << "Setting FONTCONFIG_PATH to" << stdFcPath.c_str();
#if 0 //def __NATRON_WIN32__ // commented out in https://github.com/NatronGitHub/Natron/commit/3445d671f15fbd97bca164b53ceb41cef47c61c3
            // qputenv on mingw will just call putenv, but we want to keep the utf16 info, so we need to call _wputenv
            _wputenv_s(L"FONTCONFIG_PATH", StrUtils::utf8_to_utf16(stdFcPath).c_str());
#else
             qputenv( "FONTCONFIG_PATH", stdFcPath.c_str() );
#endif
        }
    }

    try {
        initPython(); // calls Py_InitializeEx(), which calls setlocale()
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;

        return false;
    }

    _imp->idealThreadCount = QThread::idealThreadCount();


    QThreadPool::globalInstance()->setExpiryTimeout(-1); //< make threads never exit on their own
    //otherwise it might crash with thread-local storage

    // The pooled threads render too (QtConcurrent tasks, viewer renders): give them the same stack as
    // the render threads (#1434). This only applies to threads that the pool creates from now on.
    QThreadPool::globalInstance()->setStackSize( getRenderThreadStackSize() );


    ///the QCoreApplication must have been created so far.
    assert(qApp);

    bool ret = false;
    try {
        ret = loadInternal(cl);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
    return ret;
} // loadFromArgs

bool
AppManager::load(int argc,
                 char **argv,
                 const CLArgs& cl)
{
    // Ensure application has correct locale before doing anything
    // Warning: Qt resets it in the QCoreApplication constructor
    // see http://doc.qt.io/qt-4.8/qcoreapplication.html#locale-settings
    setApplicationLocale();
    _imp->handleCommandLineArgs(argc, argv);
    return loadFromArgs(cl);
}

bool
AppManager::loadW(int argc,
                 wchar_t **argv,
                 const CLArgs& cl)
{
    // Ensure application has correct locale before doing anything
    // Warning: Qt resets it in the QCoreApplication constructor
    // see http://doc.qt.io/qt-4.8/qcoreapplication.html#locale-settings
    setApplicationLocale();
    _imp->handleCommandLineArgsW(argc, argv);
    return loadFromArgs(cl);
}

AppManager::~AppManager()
{
    // The atexit hook of Python, run by Py_Finalize() in tearDownPython() below, must not clean up: this does it
    _imp->destroying = true;
#ifdef __NATRON_UNIX__
    disarmShutDownSignalPipe();
#endif
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
        AppInstancePtr front;
        {
            QMutexLocker k(&_imp->_appInstancesMutex);
            front = _imp->_appInstances.front();
        }
        if (front) {
            front->quitNow();
        }
        {
            QMutexLocker k(&_imp->_appInstancesMutex);
            appsEmpty = _imp->_appInstances.empty();
        }
    }

    for (PluginsMap::iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        for (PluginVersionsOrdered::reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            delete *itver;
        }
    }

    _imp->_backgroundIPC.reset();

    try {
        _imp->saveCaches();
    } catch (std::runtime_error&) {
        // ignore errors
    }

    ///Caches may have launched some threads to delete images, wait for them to be done
    QThreadPool::globalInstance()->waitForDone();

    ///Kill caches now because decreaseNCacheFilesOpened can be called
    if (_imp->_nodeCache) {
        _imp->_nodeCache->waitForDeleterThread();
    }
    if (_imp->_diskCache) {
        _imp->_diskCache->waitForDeleterThread();
    }
    if (_imp->_viewerCache) {
        _imp->_viewerCache->waitForDeleterThread();
    }
    _imp->_nodeCache.reset();
    _imp->_viewerCache.reset();
    _imp->_diskCache.reset();

    tearDownPython();
    _imp->tearDownGL();

    _instance = 0;

    // After this line, everything is cleaned-up (should be) and the process may resume in the main and could in theory be able to re-create a new AppManager
    _imp->_qApp.reset();
}

class QuitInstanceArgs
    : public GenericWatcherCallerArgs
{
public:

    AppInstanceWPtr instance;

    QuitInstanceArgs()
        : GenericWatcherCallerArgs()
        , instance()
    {
    }

    virtual ~QuitInstanceArgs() {}
};

typedef std::shared_ptr<QuitInstanceArgs> QuitInstanceArgsPtr;

void
AppManager::afterQuitProcessingCallback(const GenericWatcherCallerArgsPtr& args)
{
    // Called by the watcher of Project::quitAnyProcessingForAllNodes() once the nodes stopped
    // processing. In GUI mode, this may be in a nested event loop (e.g. a script that called
    // app.closeProject() and processes events), with code under it that uses the windows that
    // quitting deletes: quit from the event loop (#1554).
    if ( isBackground() ) {
        quitInstanceNow(args);
    } else {
        queueAfterQuitProcessingCallback(args);
    }
}

void
AppManager::quitInstanceNow(const GenericWatcherCallerArgsPtr& args)
{
    QuitInstanceArgs* inArgs = dynamic_cast<QuitInstanceArgs*>( args.get() );

    if (!inArgs) {
        return;
    }

    AppInstancePtr instance = inArgs->instance.lock();
    if ( !instance || (getAppInstance( instance->getAppID() ) != instance) ) {
        // already quit: quit() may be called twice before the queued call below runs
        return;
    }

    instance->aboutToQuit();

    appPTR->removeInstance( instance->getAppID() );

    int nbApps = getNumInstances();
    ///if we exited the last instance, exit the event loop, this will make
    /// the exec() function return.
    if (nbApps == 0) {
        assert(qApp);
        qApp->quit();
    }

    // This should kill the AppInstance
    instance.reset();
}

void
AppManager::quitNow(const AppInstancePtr& instance)
{
    NodesList nodesToWatch;

    instance->getProject()->getNodes_recursive(nodesToWatch, false);
    if ( !nodesToWatch.empty() ) {
        for (NodesList::iterator it = nodesToWatch.begin(); it != nodesToWatch.end(); ++it) {
            (*it)->quitAnyProcessing_blocking(false);
        }
    }
    QuitInstanceArgsPtr args = std::make_shared<QuitInstanceArgs>();
    args->instance = instance;
    quitInstanceNow(args);
}

void
AppManager::quit(const AppInstancePtr& instance)
{
    if ( instance->isQuitPending() ) {
        // already quitting (e.g. app.closeProject(), then natron.quit() or the end of a GUI test): a second
        // quit would run aboutToQuit() again, possibly from the events that the first one processes, and
        // delete the settings panels twice (#1567)
        return;
    }
    QuitInstanceArgsPtr args = std::make_shared<QuitInstanceArgs>();

    args->instance = instance;
    instance->setQuitPending();
    if ( instance->getProject()->quitAnyProcessingForAllNodes(this, args) ) {
        // afterQuitProcessingCallback() is called once the processing is over, and quits from the
        // event loop in GUI mode
        return;
    }
    if ( isBackground() ) {
        quitInstanceNow(args);
    } else {
        // GUI: quit() is called from event handlers of the widgets that aboutToQuit() deletes (the
        // window's close button, File > Close Project and Quit, a script run from the Script Editor,
        // Gui::abortProject() itself), which use them after quit() returns: quit from the event
        // loop, as when nodes are still processing (#1554).
        queueAfterQuitProcessingCallback(args);
    }
}

void
AppManager::queueAfterQuitProcessingCallback(const GenericWatcherCallerArgsPtr& args)
{
    QMetaObject::invokeMethod( this, [this, args]() {
        if (getNestedEventLoopsCount() > 0) {
            // a nested event loop runs (e.g. a GUI test's waitIdle()): the code under it may use
            // the windows of the instance, try again once it is over
            QTimer::singleShot( 50, this, [this, args]() {
                queueAfterQuitProcessingCallback(args);
            } );

            return;
        }
        quitInstanceNow(args);
    }, Qt::QueuedConnection );
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
        AppInstancePtr app;
        {
            QMutexLocker k(&_imp->_appInstancesMutex);
            app = _imp->_appInstances.front();
        }
        if (app) {
            quitNow(app);
        }

        {
            QMutexLocker k(&_imp->_appInstancesMutex);
            appsEmpty = _imp->_appInstances.empty();
        }
    }
}

void
AppManager::setUpShutDownSignalNotifier()
{
#ifdef __NATRON_UNIX__
    // Only an event loop can quit the application on a termination signal: in background
    // mode, the main thread doesn't run one, and handleShutDownSignal() terminates.
    if ( isBackground() || (shutDownSignalPipe[0] != -1) ) {
        return;
    }
    int fds[2];
    if (::pipe(fds) == -1) {
        std::perror("setting up termination signal pipe");

        return;
    }
    for (int i = 0; i < 2; ++i) {
        ::fcntl(fds[i], F_SETFD, FD_CLOEXEC);
        ::fcntl(fds[i], F_SETFL, ::fcntl(fds[i], F_GETFL) | O_NONBLOCK); // the handler must never block
    }
    shutDownSignalPipe[0] = fds[0];
    shutDownSignalPipe[1] = fds[1];
    QSocketNotifier* notifier = new QSocketNotifier(fds[0], QSocketNotifier::Read, this);
    QObject::connect( notifier, SIGNAL(activated(QSocketDescriptor,QSocketNotifier::Type)), this, SLOT(onShutDownSignalReceived()) );
#endif
}

void
AppManager::onShutDownSignalReceived()
{
#ifdef __NATRON_UNIX__
    // Called on the main thread by the event loop, after handleShutDownSignal().
    char c;
    while (::read(shutDownSignalPipe[0], &c, 1) > 0) {
    }
    quitApplication();
#endif
}

void
AppManager::initializeQApp(int &argc,
                           char **argv)
{
    assert(!_imp->_qApp);
    // scoped_ptr
    _imp->_qApp.reset( new QCoreApplication(argc, argv) );
}

// setApplicationLocale is called twice:
// - before parsing the command-line arguments
// - after the QCoreApplication was constructed, because the QCoreApplication
// constructor resets the locale to the system locale
// see http://doc.qt.io/qt-4.8/qcoreapplication.html#locale-settings
void
AppManager::setApplicationLocale()
{
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

    // We also set explicitly the LC_NUMERIC locale to "C" to avoid juggling
    // between locales when using stringstreams.
    // See function __convert_from_v(...) in
    // /usr/include/c++/4.2.1/x86_64-apple-darwin10/bits/c++locale.h
    // https://www.opensource.apple.com/source/libstdcxx/libstdcxx-104.1/include/c++/4.2.1/bits/c++locale.h
    // See also https://stackoverflow.com/questions/22753707/is-ostream-operator-in-libstdc-thread-hostile

    // set the C++ locale first
#if defined(__APPLE__) && defined(_LIBCPP_VERSION) && (defined(_LIBCPP_USE_AVAILABILITY_APPLE) || !defined(_LIBCPP_DISABLE_AVAILABILITY)) && (__MAC_OS_X_VERSION_MIN_REQUIRED < 1090)
    try {
        std::locale::global( std::locale("C") );
    } catch (std::runtime_error&) {
        qDebug() << "Could not set C++ locale!";
    }
#else
    try {
        std::locale::global( std::locale(std::locale("en_US.UTF-8"), "C", std::locale::numeric) );
    } catch (std::runtime_error&) {
        try {
            std::locale::global( std::locale(std::locale("C.UTF-8"), "C", std::locale::numeric) );
        } catch (std::runtime_error&) {
            try {
                std::locale::global( std::locale(std::locale("UTF-8"), "C", std::locale::numeric) );
            } catch (std::runtime_error&) {
                try {
                    std::locale::global( std::locale("C") );
                } catch (std::runtime_error&) {
                    qDebug() << "Could not set C++ locale!";
                }
            }
        }
    }
#endif

    // set the C locale second, because it will not overwrite the changes you made to the C++ locale
    // see https://stackoverflow.com/questions/12373341/does-stdlocaleglobal-make-affect-to-printf-function
    char *category = std::setlocale(LC_ALL, "en_US.UTF-8");
    if (category == NULL) {
        category = std::setlocale(LC_ALL, "C.UTF-8");
    }
    if (category == NULL) {
        category = std::setlocale(LC_ALL, "UTF-8");
    }
    if (category == NULL) {
        category = std::setlocale(LC_ALL, "C");
    }
    if (category == NULL) {
        qDebug() << "Could not set C locale!";
    }
    std::setlocale(LC_NUMERIC, "C"); // set the locale for LC_NUMERIC only
    QLocale::setDefault( QLocale(QLocale::English, QLocale::UnitedStates) );
}

bool
AppManager::loadInternal(const CLArgs& cl)
{
    assert(!_imp->_loaded);

    _imp->_binaryPath = QCoreApplication::applicationDirPath();
    assert(StrUtils::is_utf8(_imp->_binaryPath.toStdString().c_str()));

    registerEngineMetaTypes();
    registerGuiMetaTypes();

    qApp->setOrganizationName( QString::fromUtf8(NATRON_ORGANIZATION_NAME) );
    qApp->setOrganizationDomain( QString::fromUtf8(NATRON_ORGANIZATION_DOMAIN) );
    qApp->setApplicationName( QString::fromUtf8(NATRON_APPLICATION_NAME) );

    //Set it once setApplicationName is set since it relies on it
    refreshDiskCacheLocation();

    // Set the locale AGAIN, because Qt resets it in the QCoreApplication constructor and in Py_InitializeEx
    // see http://doc.qt.io/qt-4.8/qcoreapplication.html#locale-settings
    setApplicationLocale();
    
    _imp->dialogsDisabled = cl.areDialogsDisabled();

    Log::instance(); //< enable logging
    bool mustSetSignalsHandlers = true;
#ifdef NATRON_USE_BREAKPAD
    //Enabled breakpad only if the process was spawned from the crash reporter
    const QString& breakpadProcessExec = cl.getBreakpadProcessExecutableFilePath();
    if ( !breakpadProcessExec.isEmpty() && QFile::exists(breakpadProcessExec) ) {
        _imp->breakpadProcessExecutableFilePath = breakpadProcessExec;
        _imp->breakpadProcessPID = (Q_PID)cl.getBreakpadProcessPID();
        const QString& breakpadPipePath = cl.getBreakpadPipeFilePath();
        const QString& breakpadComPipePath = cl.getBreakpadComPipeFilePath();
        int breakpad_client_fd = cl.getBreakpadClientFD();
        _imp->initBreakpad(breakpadPipePath, breakpadComPipePath, breakpad_client_fd);
        mustSetSignalsHandlers = false;
    }
#endif


# ifdef __NATRON_UNIX__
    if (mustSetSignalsHandlers) {
        setUpShutDownSignalNotifier();
        setShutDownSignal(SIGINT);   // shut down on ctrl-c
        setShutDownSignal(SIGTERM);   // shut down on killall
#     if defined(__NATRON_LINUX__) && !defined(__FreeBSD__)
        //Catch SIGSEGV only when google-breakpad is not active
        setSigSegvSignal();
#     endif
    }
# else
    Q_UNUSED(mustSetSignalsHandlers);
# endif


    _imp->_settings = std::make_shared<Settings>();
    _imp->_settings->initializeKnobsPublic();

    bool hasGLForRendering = hasOpenGLForRequirements(eOpenGLRequirementsTypeRendering, 0);
    if (_imp->hasInitializedOpenGLFunctions && hasGLForRendering) {
        OSGLContext::getGPUInfos(_imp->openGLRenderers);
        for (std::list<OpenGLRendererInfo>::iterator it = _imp->openGLRenderers.begin(); it != _imp->openGLRenderers.end(); ++it) {
            qDebug() << "Found OpenGL Renderer:" << it->rendererName.c_str() << ", Vendor:" << it->vendorName.c_str()
                     << ", OpenGL Version:" << it->glVersionString.c_str() << ", Max. Texture Size" << it->maxTextureSize <<
                ",Max GPU Memory:" << printAsRAM(it->maxMemBytes);;
        }
    }
    _imp->_settings->populateOpenGLRenderers(_imp->openGLRenderers);


    // Settings: we must load these and set the custom settings (using python) ASAP, before creating the OFX Plugin Cache
    // Settings: always call restoreSettings, but call restoreKnobsFromSettings conditionally
    // Call restore after initializing knobs
    _imp->_settings->restoreSettings( cl.isLoadedUsingDefaultSettings() );
    if (cl.isLoadedUsingDefaultSettings()) {
        _imp->_settings->setSaveSettings(false);
    }

    _imp->declareSettingsToPython();

    // executeCommandLineSettingCommands
    {
        const std::list<std::string>& commands = cl.getSettingCommands();

        // do not save settings if there is a --setting option
        if ( !commands.empty() ) {
            _imp->_settings->setSaveSettings(false);
        }
        for (std::list<std::string>::const_iterator it = commands.begin(); it != commands.end(); ++it) {
            std::string err;
            std::string output;
            bool ok  = NATRON_PYTHON_NAMESPACE::interpretPythonScript(*it, &err, &output);
            if (!ok) {
                const QString sp( QString::fromUtf8(" ") );
                QString m = tr("Failed to execute the following Python command:") + sp +
                QString::fromUtf8( it->c_str() ) + sp +
                tr("Error:") + sp +
                QString::fromUtf8( err.c_str() );
                throw std::runtime_error( m.toStdString() );
            } else if ( !output.empty() ) {
                std::cout << output << std::endl;
            }
        }
    }

    ///basically show a splashScreen load fonts etc...
    return initGui(cl);
} // loadInternal

const std::list<OpenGLRendererInfo>&
AppManager::getOpenGLRenderers() const
{
    return _imp->openGLRenderers;
}

bool
AppManager::isSpawnedFromCrashReporter() const
{
#ifdef NATRON_USE_BREAKPAD

    return _imp->breakpadHandler.get() != 0;
#else

    return false;
#endif
}

void
AppManager::setPluginsUseInputImageCopyToRender(bool b)
{
    _imp->pluginsUseInputImageCopyToRender = b;
}

bool
AppManager::isCopyInputImageForPluginRenderEnabled() const
{
    return _imp->pluginsUseInputImageCopyToRender;
}

bool
AppManager::isOnWayland() const
{
    return _imp->onWayland;
}

bool
AppManager::isOpenGLLoaded() const
{
    QMutexLocker k(&_imp->openGLFunctionsMutex);

    return _imp->hasInitializedOpenGLFunctions;
}

bool
AppManager::isTextureFloatSupported() const
{
    return _imp->glHasTextureFloat;
}

bool
AppManager::hasOpenGLForRequirements(OpenGLRequirementsTypeEnum type, QString* missingOpenGLError ) const
{
    std::map<OpenGLRequirementsTypeEnum,AppManagerPrivate::OpenGLRequirementsData>::const_iterator found =  _imp->glRequirements.find(type);
    assert(found != _imp->glRequirements.end());
    if (found == _imp->glRequirements.end()) {
        return false;
    }
    if (missingOpenGLError && !found->second.hasRequirements) {
        *missingOpenGLError = found->second.error;
    }
    return found->second.hasRequirements;
}

bool
AppManager::initializeOpenGLFunctionsOnce(bool createOpenGLContext)
{
#ifdef DEBUG
    // The floating point exceptions that main() traps in debug builds must not be trapped in the OpenGL
    // driver: Mesa's llvmpipe raises them while it creates a context (SIGFPE in driCreateContextAttribs()
    // under Xvfb). This runs before the event loop, which does not trap them (see exec()).
    boost_adaptbx::floating_point::no_exception_trapping noTrap;
#endif
    QMutexLocker k(&_imp->openGLFunctionsMutex);

    if (!_imp->hasInitializedOpenGLFunctions) {
        OSGLContextPtr glContext;
        bool checkRenderingReq = true;
        if (createOpenGLContext) {
            try {
                try {
                    _imp->initGLAPISpecific();
                    glContext = _imp->renderingContextPool->attachGLContextToRender(false /*checkIfGLLoaded*/);
                } catch (const std::exception& e) {
                    // On Linux, retry once with X11/GLX if Wayland/EGL failed.
                    if ( !_imp->fallBackToGLX( e.what() ) ) {
                        throw;
                    }
                    const std::string eglError = e.what();
                    try {
                        glContext = _imp->renderingContextPool->attachGLContextToRender(false /*checkIfGLLoaded*/);
                    } catch (const std::exception& e2) {
                        // Report both failures, not only the last one.
                        throw std::runtime_error("Wayland/EGL: " + eglError + "; X11/GLX: " + e2.what());
                    }
                }
                if (glContext) {
                    // Make the context current and check its version
                    glContext->setContextCurrentNoRender();
                } else {
                    AppManagerPrivate::OpenGLRequirementsData& vdata = _imp->glRequirements[eOpenGLRequirementsTypeViewer];
                    AppManagerPrivate::OpenGLRequirementsData& rdata = _imp->glRequirements[eOpenGLRequirementsTypeRendering];
                    rdata.error = tr("Error creating OpenGL context.");
                    vdata.error = rdata.error;
                    rdata.hasRequirements = false;
                    vdata.hasRequirements = false;
                    AppManagerPrivate::addOpenGLRequirementsString(rdata.error, eOpenGLRequirementsTypeRendering);
                    AppManagerPrivate::addOpenGLRequirementsString(vdata.error, eOpenGLRequirementsTypeViewer);
                }


            } catch (const std::exception& e) {
                std::cerr << "Error while loading OpenGL: " << e.what() << std::endl;
                std::cerr << "OpenGL rendering is disabled. " << std::endl;
                AppManagerPrivate::OpenGLRequirementsData& vdata = _imp->glRequirements[eOpenGLRequirementsTypeViewer];
                AppManagerPrivate::OpenGLRequirementsData& rdata = _imp->glRequirements[eOpenGLRequirementsTypeRendering];
                rdata.hasRequirements = false;
                vdata.hasRequirements = false;
                vdata.error = tr("Error while creating OpenGL context: %1").arg(QString::fromUtf8(e.what()));
                rdata.error = tr("Error while creating OpenGL context: %1").arg(QString::fromUtf8(e.what()));
                AppManagerPrivate::addOpenGLRequirementsString(rdata.error, eOpenGLRequirementsTypeRendering);
                AppManagerPrivate::addOpenGLRequirementsString(vdata.error, eOpenGLRequirementsTypeViewer);
                checkRenderingReq = false;
            }
            if (!glContext) {
                return false;
            }
        }

        // The following requires a valid OpenGL context to be created
        _imp->initGl(checkRenderingReq);
        if (createOpenGLContext) {
            if (hasOpenGLForRequirements(eOpenGLRequirementsTypeRendering)) {
                try {
                    OSGLContext::checkOpenGLVersion();
                } catch (const std::exception& e) {
                    AppManagerPrivate::OpenGLRequirementsData& data = _imp->glRequirements[eOpenGLRequirementsTypeRendering];
                    data.hasRequirements = false;
                    if ( !data.error.isEmpty() ) {
                        data.error = QString::fromUtf8( e.what() );
                    }
                }
            }
            
            _imp->renderingContextPool->releaseGLContextFromRender(glContext);
            glContext->unsetCurrentContextNoRender();

            // Clear created contexts because this context was created with the "default" OpenGL renderer and it might be different from the one
            // selected by the user in the settings (which are not created yet).
            _imp->renderingContextPool->clear();
        } else {
            updateAboutWindowLibrariesVersion();
        }

        return true;
    }

    return false;
}

#ifdef __NATRON_WIN32__
const OSGLContext_wgl_data*
AppManager::getWGLData() const
{
    return _imp->wglInfo.get();
}

#endif
#ifdef __NATRON_LINUX__
const OSGLContext_egl_data*
AppManager::getEGLData() const
{
    return _imp->eglInfo.get();
}

const OSGLContext_glx_data*
AppManager::getGLXData() const
{
    return _imp->glxInfo.get();
}

#endif


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
        U64 viewerCacheSize = _imp->_settings->getMaximumViewerDiskCacheSize();
        U64 maxDiskCacheNode = _imp->_settings->getMaximumDiskCacheNodeSize();

        _imp->_nodeCache = std::make_shared<Cache<Image> >("NodeCache", NATRON_CACHE_VERSION, maxCacheRAM, 1.);
        _imp->_diskCache = std::make_shared<Cache<Image> >("DiskCache", NATRON_CACHE_VERSION, maxDiskCacheNode, 0.);
        _imp->_viewerCache = std::make_shared<Cache<FrameEntry> >("ViewerCache", NATRON_CACHE_VERSION, viewerCacheSize, 0.);
        _imp->setViewerCacheTileSize();
    } catch (std::logic_error&) {
        // ignore
    }

    int oldCacheVersion = 0;
    if ( isTestMode() ) {
        // a test does not record its cache version in the user's preferences (the next launch of an
        // installed Natron would clear the user's caches)
        oldCacheVersion = NATRON_CACHE_VERSION;
    } else {
        QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

        if ( settings.contains( QString::fromUtf8(kNatronCacheVersionSettingsKey) ) ) {
            oldCacheVersion = settings.value( QString::fromUtf8(kNatronCacheVersionSettingsKey) ).toInt();
        }
        settings.setValue(QString::fromUtf8(kNatronCacheVersionSettingsKey), NATRON_CACHE_VERSION);
    }

    if (oldCacheVersion != NATRON_CACHE_VERSION || cl.isCacheClearRequestedOnLaunch()) {
        setLoadingStatus( tr("Clearing the image cache...") );
        // If another process uses the caches, they are not wiped (it would delete its files): start with empty caches
        if ( !wipeAndCreateDiskCacheStructure() ) {
            std::cerr << "Warning: the disk caches in " << getDiskCacheLocation().toStdString()
                      << " were not cleared, because another " NATRON_APPLICATION_NAME " process uses them. "
                      << "Starting with empty caches." << std::endl;
        }
    } else {
        setLoadingStatus( tr("Restoring the image cache...") );
        _imp->restoreCaches();
    }

    if (cl.isOpenFXCacheClearRequestedOnLaunch()) {
        setLoadingStatus( tr("Clearing the OpenFX Plugins cache...") );
        clearPluginsLoadedCache();
    } else {
        setLoadingStatus( tr("Loading plugin cache...") );
    }


    ///Set host properties after restoring settings since it depends on the host name.
    try {
        _imp->ofxHost->setProperties();
    } catch (std::logic_error&) {
        // ignore
    }

    /*loading all plugins*/
    try {
        loadAllPlugins();
        _imp->loadBuiltinFormats();
    } catch (std::logic_error&) {
        // ignore
    }

    if ( isBackground() && !cl.getIPCPipeName().isEmpty() ) {
        _imp->initProcessInputChannel( cl.getIPCPipeName() );
    }


    if ( cl.isInterpreterMode() ) {
        _imp->_appType = eAppTypeInterpreter;
    } else if ( isBackground() ) {
        if ( !cl.getScriptFilename().isEmpty() ) {
            if ( !cl.getIPCPipeName().isEmpty() ) {
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
    if ( !cl.getScriptFilename().isEmpty() ) {
        const QStringList& appArgs = qApp->arguments();
        args = CLArgs( appArgs, cl.isBackgroundMode() );
    } else {
        args = cl;
    }

    AppInstancePtr mainInstance = newAppInstance(args, false);

    hideSplashScreen();

    if (!mainInstance) {
        qApp->quit();

        return false;
    } else {
        onLoadCompleted();

        ///In background project auto-run the rendering is finished at this point, just exit the instance
        if ( ( (_imp->_appType == eAppTypeBackgroundAutoRun) ||
               ( _imp->_appType == eAppTypeBackgroundAutoRunLaunchedFromGui) ||
               ( _imp->_appType == eAppTypeInterpreter) ) && mainInstance ) {
            bool wasKilled = true;
            const AppInstanceVec& instances = appPTR->getAppInstances();
            for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
                if ( (*it == mainInstance) ) {
                    wasKilled = false;
                }
            }
            if (!wasKilled) {
                // quitNow() resets the project (AppInstance::aboutToQuit()): resetting it here
                // too ran the beforeProjectClose callback twice
                try {
                    mainInstance->quitNow();
                } catch (std::logic_error&) {
                    // ignore
                }
            }
        }

        return true;
    }
} // AppManager::loadInternalAfterInitGui

void
AppManager::onViewerTileCacheSizeChanged()
{
    if (_imp->_viewerCache) {
        _imp->_viewerCache->clear();
        _imp->setViewerCacheTileSize();
    }
}

void
AppManagerPrivate::setViewerCacheTileSize()
{
    if (!_viewerCache) {
        return;
    }
    std::size_t tileSize =  (std::size_t)ipow( 2, _settings->getViewerTilesPowerOf2() );

    // Viewer tiles are always RGBA
    tileSize = tileSize * tileSize * 4;


    ImageBitDepthEnum viewerDepth = _settings->getViewersBitDepth();
    switch (viewerDepth) {
        case eImageBitDepthFloat:
        case eImageBitDepthHalf:
            tileSize *= sizeof(float);
            break;
        default:
            break;
    }
    _viewerCache->setTiled(true, tileSize);
}

AppInstancePtr
AppManager::newAppInstanceInternal(const CLArgs& cl,
                                   bool alwaysBackground,
                                   bool makeEmptyInstance)
{
    AppInstancePtr instance;

    if (!alwaysBackground) {
        instance = makeNewInstance(_imp->_availableID);
    } else {
        instance = std::make_shared<AppInstance>(_imp->_availableID);
    }

    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        _imp->_appInstances.push_back(instance);
        if (alwaysBackground) {
            _imp->_internalInstanceIDs.insert( instance->getAppID() );
        }
    }

    // A background instance (the one a Precomp node loads its project in, see newBackgroundInstance())
    // is internal: it never replaces the instance the user works in as the top-level one (#1501).
    if (!alwaysBackground) {
        setAsTopLevelInstance( instance->getAppID() );
    }

    ++_imp->_availableID;

    try {
        instance->load(cl, makeEmptyInstance);
    } catch (const std::exception & e) {
        Dialogs::errorDialog( NATRON_APPLICATION_NAME, e.what(), false );
        // _availableID was already incremented: remove the instance that failed to load by its own ID
        removeInstance( instance->getAppID() );
        instance.reset();
        --_imp->_availableID;

        return instance;
    } catch (...) {
        Dialogs::errorDialog( NATRON_APPLICATION_NAME, tr("Cannot load project").toStdString(), false );
        // _availableID was already incremented: remove the instance that failed to load by its own ID
        removeInstance( instance->getAppID() );
        instance.reset();
        --_imp->_availableID;

        return instance;
    }

    ///flag that we finished loading the Appmanager even if it was already true
    _imp->_loaded = true;

    return instance;
}

AppInstancePtr
AppManager::newBackgroundInstance(const CLArgs& cl,
                                  bool makeEmptyInstance)
{
    return newAppInstanceInternal(cl, true, makeEmptyInstance);
}

AppInstancePtr
AppManager::newAppInstance(const CLArgs& cl,
                           bool makeEmptyInstance)
{
    return newAppInstanceInternal(cl, false, makeEmptyInstance);
}

AppInstancePtr
AppManager::getAppInstance(int appID) const
{
    QMutexLocker k(&_imp->_appInstancesMutex);

    for (AppInstanceVec::const_iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        if ( (*it)->getAppID() == appID ) {
            return *it;
        }
    }

    return AppInstancePtr();
}

int
AppManager::getNumInstances() const
{
    QMutexLocker k(&_imp->_appInstancesMutex);

    return (int)_imp->_appInstances.size();
}

const AppInstanceVec &
AppManager::getAppInstances() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->_appInstances;
}

void
AppManager::removeInstance(int appID)
{
    int newApp = -1;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        for (AppInstanceVec::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
            if ( (*it)->getAppID() == appID ) {
                _imp->_appInstances.erase(it);
                break;
            }
        }
        _imp->_internalInstanceIDs.erase(appID);

        // Only replace the top-level instance if it was removed, and never by an internal instance
        // (a Precomp node's, #1501).
        if (appID == _imp->_topLevelInstanceID) {
            for (AppInstanceVec::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
                if ( _imp->_internalInstanceIDs.find( (*it)->getAppID() ) == _imp->_internalInstanceIDs.end() ) {
                    newApp = (*it)->getAppID();
                    break;
                }
            }
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
    if (!_imp->_viewerCache) {
        return;
    }
    clearLastRenderedTextures();
    // The viewer cache is tiled: its entries are always in its disk portion (the tiles are memory-mapped),
    // so clearing its memory portion would do nothing (#1119).
    _imp->_viewerCache->clear();
}

void
AppManager::clearViewerCache()
{
    if (!_imp->_viewerCache) {
        return;
    }

    _imp->_viewerCache->clear();
}

void
AppManager::clearDiskCache()
{
    if (!_imp->_viewerCache) {
        return;
    }

    clearLastRenderedTextures();
    _imp->_viewerCache->clear();
    _imp->_diskCache->clear();
}

void
AppManager::clearNodeCache()
{
    AppInstanceVec copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }

    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->clearAllLastRenderedImages();
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
    AppInstanceVec copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }

    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->abortAllViewers();
    }

    clearDiskCache();
    clearNodeCache();


    ///for each app instance clear all its nodes cache
    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->clearOpenFXPluginsCaches();
    }

    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->renderAllViewers(true);
    }

    // Do not remove the auto-saves here (#1258): they are not a cache, but the crash-recovery files of the
    // unsaved projects of every running Natron. This is also called when the disk cache is wiped at
    // startup (--clear-cache or a new cache version), before the user is offered to restore them.
}

bool
AppManager::wipeAndCreateDiskCacheStructure()
{
    //Should be called on the main-thread because potentially can interact with rendering
    assert( QThread::currentThread() == qApp->thread() );

    abortAnyProcessing();

    clearAllCaches();

    // The files of the cache folders may be used by another Natron process: only delete them if there is none.
    // (clearAllCaches() only removed the files of the entries of this process.)
    if ( isDiskCacheUsedByAnotherProcess() ) {
        qDebug() << "The disk caches in" << getDiskCacheLocation() << "are used by another Natron process: they are not wiped.";

        return false;
    }
    assert(_imp->_diskCache);
    _imp->cleanUpCacheDiskStructure( _imp->_diskCache->getCachePath(), false );
    assert(_imp->_viewerCache);
    _imp->cleanUpCacheDiskStructure( _imp->_viewerCache->getCachePath() , true);

    return true;
}

AppInstancePtr
AppManager::getTopLevelInstance () const
{
    QMutexLocker k(&_imp->_appInstancesMutex);

    for (AppInstanceVec::const_iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        if ( (*it)->getAppID() == _imp->_topLevelInstanceID ) {
            return *it;
        }
    }

    return AppInstancePtr();
}

bool
AppManager::isLoaded() const
{
    return _imp->_loaded;
}

void
AppManager::abortAnyProcessing()
{
    AppInstanceVec copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }

    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->getProject()->quitAnyProcessingForAllNodes_non_blocking();
    }
}

bool
AppManager::writeToOutputPipe(const QString & longMessage,
                              const QString & shortMessage,
                              bool printIfNoChannel)
{
    if (!_imp->_backgroundIPC) {
        if (printIfNoChannel) {
            QMutexLocker k(&_imp->errorLogMutex);
            ///Don't use qdebug here which is disabled if QT_NO_DEBUG_OUTPUT is defined.
            std::cout << longMessage.toStdString() << std::endl;
        }

        return false;
    }
    _imp->_backgroundIPC->writeToOutputChannel(shortMessage);

    return true;
}

void
AppManager::setApplicationsCachesMaximumMemoryPercent(double p)
{
    // The --setting commands are run by loadInternal() before loadInternalAfterInitGui() creates the
    // caches (#1571), which then read their sizes from the settings: there is nothing to do yet.
    if (!_imp->_nodeCache) {
        return;
    }
    size_t maxCacheRAM = p * getSystemTotalRAM_conditionnally();

    _imp->_nodeCache->setMaximumCacheSize(maxCacheRAM);
    _imp->_nodeCache->setMaximumInMemorySize(1);
}

void
AppManager::setApplicationsCachesMaximumViewerDiskSpace(unsigned long long size)
{
    // Not created yet when called by a --setting command: see setApplicationsCachesMaximumMemoryPercent()
    if (!_imp->_viewerCache) {
        return;
    }
    _imp->_viewerCache->setMaximumCacheSize(size);
}

void
AppManager::setApplicationsCachesMaximumDiskSpace(unsigned long long size)
{
    // Not created yet when called by a --setting command: see setApplicationsCachesMaximumMemoryPercent()
    if (!_imp->_diskCache) {
        return;
    }
    _imp->_diskCache->setMaximumCacheSize(size);
}

void
AppManager::loadAllPlugins()
{
    assert( _imp->_plugins.empty() );
    assert( _imp->_formats.empty() );

    // Load plug-ins bundled into Natron
    loadBuiltinNodePlugins(&_imp->readerPlugins, &_imp->writerPlugins);

    // Load OpenFX plug-ins
    _imp->ofxHost->loadOFXPlugins( &_imp->readerPlugins, &_imp->writerPlugins);

    // Load PyPlugs and init.py & initGui.py scripts
    // Should be done after settings are declared
    loadPythonGroups();

    _imp->_settings->restorePluginSettings();


    onAllPluginsLoaded();
}

void
AppManager::onAllPluginsLoaded()
{
    //We try to make nicer plug-in labels, only do this if the user use Natron with some sort of interaction (either command line
    //or GUI, otherwise don't bother doing this)

    AppManager::AppTypeEnum appType = appPTR->getAppType();

    if ( (appType != AppManager::eAppTypeBackground) &&
         ( appType != AppManager::eAppTypeGui) &&
         ( appType != AppManager::eAppTypeInterpreter) ) {
        return;
    }

    //Make sure there is no duplicates with the same label
    const PluginsMap& plugins = getPluginsList();
    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        assert( !it->second.empty() );
        if (it->second.empty()) {
            continue;
        }
        PluginVersionsOrdered::reverse_iterator first = it->second.rbegin();
        bool isUserCreatable = false;
        for (PluginVersionsOrdered::reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            if ( (*itver)->getIsUserCreatable() ) {
                isUserCreatable = true;
            } else {
                (*itver)->setLabelWithoutSuffix( (*itver)->getPluginLabel() );
            }
        }
        if (!isUserCreatable) {
            continue;
        }

        QString labelWithoutSuffix = Plugin::makeLabelWithoutSuffix( (*first)->getPluginLabel() );

        //Find a duplicate
        for (PluginsMap::const_iterator it2 = plugins.begin(); it2 != plugins.end(); ++it2) {
            if (it->first == it2->first) {
                continue;
            }


            PluginVersionsOrdered::reverse_iterator other = it2->second.rbegin();
            bool isOtherUserCreatable = false;
            for (PluginVersionsOrdered::reverse_iterator it2ver = it2->second.rbegin(); it2ver != it2->second.rend(); ++it2ver) {
                if ( (*it2ver)->getIsUserCreatable() ) {
                    isOtherUserCreatable = true;
                    break;
                }
            }

            if (!isOtherUserCreatable) {
                continue;
            }

            QString otherLabelWithoutSuffix = Plugin::makeLabelWithoutSuffix( (*other)->getPluginLabel() );
            if (otherLabelWithoutSuffix == labelWithoutSuffix) {
                QString otherGrouping = (*other)->getGrouping().join( QChar::fromLatin1('/') );
                const QStringList& thisGroupingSplit = (*first)->getGrouping();
                QString thisGrouping = thisGroupingSplit.join( QChar::fromLatin1('/') );
                if (otherGrouping == thisGrouping) {
                    labelWithoutSuffix = (*first)->getPluginLabel();
                }
                break;
            }
        }


        for (PluginVersionsOrdered::reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            if ( (*itver)->getIsUserCreatable() ) {
                (*itver)->setLabelWithoutSuffix(labelWithoutSuffix);
                onPluginLoaded(*itver);
            }
        }
    }
} // AppManager::onAllPluginsLoaded

template <typename PLUGIN>
void
AppManager::registerBuiltInPlugin(const QString& iconPath,
                                  bool isDeprecated,
                                  bool internalUseOnly)
{
    EffectInstancePtr node( PLUGIN::BuildEffect( NodePtr() ) );
    std::map<std::string, void (*)()> functions;

    functions.insert( std::make_pair("BuildEffect", ( void (*)() ) & PLUGIN::BuildEffect) );
    LibraryBinary *binary = new LibraryBinary(functions);
    assert(binary);

    std::list<std::string> grouping;
    node->getPluginGrouping(&grouping);
    QStringList qgrouping;

    for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
        qgrouping.push_back( QString::fromUtf8( it->c_str() ) );
    }

    // Empty since statically bundled
    QString resourcesPath = QString();
    Plugin* p = registerPlugin(resourcesPath, qgrouping, QString::fromUtf8( node->getPluginID().c_str() ), QString::fromUtf8( node->getPluginLabel().c_str() ),
                               iconPath, QStringList(), node->isReader(), node->isWriter(), binary, node->renderThreadSafety() == eRenderSafetyUnsafe, node->getMajorVersion(), node->getMinorVersion(), isDeprecated);
    std::list<PluginActionShortcut> shortcuts;
    node->getPluginShortcuts(&shortcuts);
    p->setShorcuts(shortcuts);

    PluginOpenGLRenderSupport glSupport = node->supportsOpenGLRender();
    p->setOpenGLRenderSupport(glSupport);

    if (internalUseOnly) {
        p->setForInternalUseOnly(true);
    }
}

void
AppManager::loadBuiltinNodePlugins(IOPluginsMap* /*readersMap*/,
                                   IOPluginsMap* /*writersMap*/)
{
    registerBuiltInPlugin<Backdrop>(QString::fromUtf8(NATRON_IMAGES_PATH "backdrop_icon.png"), false, false);
    registerBuiltInPlugin<GroupOutput>(QString::fromUtf8(NATRON_IMAGES_PATH "output_icon.png"), false, false);
    registerBuiltInPlugin<GroupInput>(QString::fromUtf8(NATRON_IMAGES_PATH "input_icon.png"), false, false);
    registerBuiltInPlugin<NodeGroup>(QString::fromUtf8(NATRON_IMAGES_PATH "group_icon.png"), false, false);
    registerBuiltInPlugin<Dot>(QString::fromUtf8(NATRON_IMAGES_PATH "dot_icon.png"), false, false);
    registerBuiltInPlugin<DiskCacheNode>(QString::fromUtf8(NATRON_IMAGES_PATH "diskcache_icon.png"), false, false);
    registerBuiltInPlugin<RotoPaint>(QString::fromUtf8(NATRON_IMAGES_PATH "GroupingIcons/Set2/paint_grouping_2.png"), false, false);
    registerBuiltInPlugin<RotoNode>(QString::fromUtf8(NATRON_IMAGES_PATH "rotoNodeIcon.png"), false, false);
    registerBuiltInPlugin<RotoSmear>(QString::fromUtf8(""), false, true);
    registerBuiltInPlugin<PrecompNode>(QString::fromUtf8(NATRON_IMAGES_PATH "precompNodeIcon.png"), false, false);
    registerBuiltInPlugin<TrackerNode>(QString::fromUtf8(NATRON_IMAGES_PATH "trackerNodeIcon.png"), false, false);
    registerBuiltInPlugin<JoinViewsNode>(QString::fromUtf8(NATRON_IMAGES_PATH "joinViewsNode.png"), false, false);
    registerBuiltInPlugin<OneViewNode>(QString::fromUtf8(NATRON_IMAGES_PATH "oneViewNode.png"), false, false);
#ifdef NATRON_ENABLE_IO_META_NODES
    registerBuiltInPlugin<ReadNode>(QString::fromUtf8(NATRON_IMAGES_PATH "readImage.png"), false, false);
    registerBuiltInPlugin<WriteNode>(QString::fromUtf8(NATRON_IMAGES_PATH "writeImage.png"), false, false);
#endif

    if ( !isBackground() ) {
        registerBuiltInPlugin<ViewerInstance>(QString::fromUtf8(NATRON_IMAGES_PATH "viewer_icon.png"), false, false);
    }
}

bool
AppManager::findAndRunScriptFile(const QString& path,
                     const QStringList& files,
                     const QString& script)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return false;
#endif
    for (QStringList::const_iterator it = files.begin(); it != files.end(); ++it) {
        if (*it == script) {
            QString absolutePath = path + *it;
            QFile file(absolutePath);
            if ( file.open(QIODevice::ReadOnly) ) {
                QTextStream ts(&file);
                QString content = ts.readAll();
                PythonGILLocker pgl;

                PyRun_SimpleString( content.toStdString().c_str() );


                PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
                std::string error, output;

                ///Gui session, do stdout, stderr redirection
                PyObject *errCatcher = 0;
                PyObject *outCatcher = 0;

                if ( PyObject_HasAttrString(mainModule, "catchErr") ) {
                    errCatcher = PyObject_GetAttrString(mainModule, "catchErr"); //get our catchOutErr created above, new ref
                }

                if ( PyObject_HasAttrString(mainModule, "catchOut") ) {
                    outCatcher = PyObject_GetAttrString(mainModule, "catchOut"); //get our catchOutErr created above, new ref
                }

                PyErr_Print(); //make python print any errors

                PyObject *errorObj = 0;
                if (errCatcher) {
                    errorObj = PyObject_GetAttrString(errCatcher, "value"); //get the  stderr from our catchErr object, new ref
                    assert(errorObj);

                    error = NATRON_PYTHON_NAMESPACE::PyStringToStdString(errorObj);
                    PyObject* unicode = PyUnicode_FromString("");
                    PyObject_SetAttrString(errCatcher, "value", unicode);
                    Py_DECREF(errorObj);
                    Py_DECREF(errCatcher);
                }
                PyObject *outObj = 0;
                if (outCatcher) {
                    outObj = PyObject_GetAttrString(outCatcher, "value"); //get the stdout from our catchOut object, new ref
                    assert(outObj);
                    output = NATRON_PYTHON_NAMESPACE::PyStringToStdString(outObj);
                    PyObject* unicode = PyUnicode_FromString("");
                    PyObject_SetAttrString(outCatcher, "value", unicode);
                    Py_DECREF(outObj);
                    Py_DECREF(outCatcher);
                }


                if ( !error.empty() ) {
                    QString message(tr("Failed to load %1: %2").arg(absolutePath).arg(QString::fromUtf8( error.c_str() )) );
                    appPTR->writeToErrorLog_mt_safe(tr("Python Script"), QDateTime::currentDateTime(), message, false);
                    std::cerr << message.toStdString() << std::endl;

                    return false;
                }
                if ( !output.empty() ) {
                    QString message;
                    message.append(absolutePath);
                    message.append( QString::fromUtf8(": ") );
                    message.append( QString::fromUtf8( output.c_str() ) );
                    if ( appPTR->getTopLevelInstance() ) {
                        appPTR->getTopLevelInstance()->appendToScriptEditor( message.toStdString() );
                    }
                    std::cout << message.toStdString() << std::endl;
                }

                return true;
            }
            break;
        }
    }

    return false;
} // findAndRunScriptFile

QStringList
AppManager::getAllNonOFXPluginsPaths() const
{
    QStringList templatesSearchPath;

    //add ~/.Natron
    QString dataLocation = QDir::homePath();
    QString mainPath = dataLocation + QString::fromUtf8("/.") + QString::fromUtf8(NATRON_APPLICATION_NAME);
    QDir mainPathDir(mainPath);

    if ( !mainPathDir.exists() ) {
        QDir dataDir(dataLocation);
        if ( dataDir.exists() ) {
            dataDir.mkdir( QChar::fromLatin1('.') + QString( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
        }
    }

    QString envvar( QString::fromUtf8( qgetenv(NATRON_PLUGIN_PATH_ENV_VAR) ) );
# ifdef __NATRON_WIN32__
    const QChar pathSep = QChar::fromLatin1(';');
# else
    const QChar pathSep = QChar::fromLatin1(':');
# endif
    QStringList splitDirs = envvar.split(pathSep);
    std::list<std::string> userSearchPaths;
    _imp->_settings->getPythonGroupsSearchPaths(&userSearchPaths);


    //This is the bundled location for PyPlugs
    QDir cwd( QCoreApplication::applicationDirPath() );
    cwd.cdUp();
    QString natronBundledPluginsPath = QString( cwd.absolutePath() +  QString::fromUtf8("/Plugins/PyPlugs") );
    bool preferBundleOverSystemWide = _imp->_settings->preferBundledPlugins();
    bool useBundledPlugins = _imp->_settings->loadBundledPlugins();
    if (preferBundleOverSystemWide && useBundledPlugins) {
        ///look-in the bundled plug-ins
        templatesSearchPath.push_back(natronBundledPluginsPath);
    }

    ///look-in the main system wide plugin path
    templatesSearchPath.push_back(mainPath);

    ///look-in the global system wide plugin path
    templatesSearchPath.push_back( getPyPlugsGlobalPath() );

    ///look-in the locations indicated by NATRON_PLUGIN_PATH
    Q_FOREACH(const QString &splitDir, splitDirs) {
        if ( !splitDir.isEmpty() ) {
            templatesSearchPath.push_back(splitDir);
        }
    }

    ///look-in extra search path set in the preferences
    for (std::list<std::string>::iterator it = userSearchPaths.begin(); it != userSearchPaths.end(); ++it) {
        if ( !it->empty() ) {
            templatesSearchPath.push_back( QString::fromUtf8( it->c_str() ) );
        }
    }

    if (!preferBundleOverSystemWide && useBundledPlugins) {
        ///look-in the bundled plug-ins
        templatesSearchPath.push_back(natronBundledPluginsPath);
    }
    qDebug() << "templatesSearchPath: " << templatesSearchPath;
    
    return templatesSearchPath;
} // AppManager::getAllNonOFXPluginsPaths

QString
AppManager::getPyPlugsGlobalPath() const
{
    QString path;

#ifdef __NATRON_UNIX__
#ifdef __NATRON_OSX__
    path = QString::fromUtf8("/Library/Application Support/%1/Plugins").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) );
#else
    path = QString::fromUtf8("/usr/share/%1/Plugins").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) );
#endif
#elif defined(__NATRON_WIN32__)
    wchar_t buffer[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, SHGFP_TYPE_CURRENT, buffer);
    std::wstring str;
    str.append(L"\\");
    str.append( QString::fromUtf8("%1\\Plugins").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdWString() );
    wcscat_s(buffer, MAX_PATH, str.c_str());
    path = QString::fromStdWString( std::wstring(buffer) );
#endif

    return path;
}

typedef void (*NatronPathFunctor)(const QDir&);
static void
operateOnPathRecursive(NatronPathFunctor functor,
                       const QDir& directory)
{
    if ( !directory.exists() ) {
        return;
    }

    functor(directory);

    QStringList subDirs = directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    Q_FOREACH(const QString &subDir, subDirs) {
        QDir d(directory.absolutePath() + QChar::fromLatin1('/') + subDir);

        operateOnPathRecursive(functor, d);
    }
}

static void
addToPythonPathFunctor(const QDir& directory)
{
    std::string addToPythonPath("sys.path.append(str('");

    addToPythonPath += directory.absolutePath().toStdString();
    addToPythonPath += "'))\n";

    std::string err;
    bool ok  = NATRON_PYTHON_NAMESPACE::interpretPythonScript(addToPythonPath, &err, 0);
    if (!ok) {
        std::string message = QCoreApplication::translate("AppManager", "Could not add %1 to python path:").arg( directory.absolutePath() ).toStdString() + ' ' + err;
        std::cerr << message << std::endl;
        AppInstancePtr topLevel = appPTR->getTopLevelInstance();
        if (topLevel) {
            topLevel->appendToScriptEditor( message.c_str() );
        }
    }
}

void
AppManager::findAllScriptsRecursive(const QDir& directory,
                        QStringList& allPlugins,
                        QStringList *foundInit,
                        QStringList *foundInitGui)
{
    if ( !directory.exists() ) {
        return;
    }

    QStringList filters;
    filters << QString::fromUtf8("*.py");
    QStringList files = directory.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);
    bool ok = findAndRunScriptFile( directory.absolutePath() + QChar::fromLatin1('/'), files, QString::fromUtf8("init.py") );
    if (ok) {
        foundInit->append( directory.absolutePath() + QString::fromUtf8("/init.py") );
    }
    if ( !appPTR->isBackground() ) {
        ok = findAndRunScriptFile( directory.absolutePath() + QChar::fromLatin1('/'), files, QString::fromUtf8("initGui.py") );
        if (ok) {
            foundInitGui->append( directory.absolutePath() + QString::fromUtf8("/initGui.py") );
        }
    }

    for (QStringList::iterator it = files.begin(); it != files.end(); ++it) {
        if ( it->endsWith( QString::fromUtf8(".py") ) && ( *it != QString::fromUtf8("init.py") ) && ( *it != QString::fromUtf8("initGui.py") ) ) {
            allPlugins.push_back(directory.absolutePath() + QChar::fromLatin1('/') + *it);
        }
    }

    QStringList subDirs = directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    Q_FOREACH(const QString &subDir, subDirs) {
        QDir d(directory.absolutePath() + QChar::fromLatin1('/') + subDir);

        findAllScriptsRecursive(d, allPlugins, foundInit, foundInitGui);
    }
}

void
AppManager::loadPythonGroups()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    PythonGILLocker pgl; // useless?
    QStringList templatesSearchPath = getAllNonOFXPluginsPaths();
    std::string err;
    QStringList allPlugins;

    ///For all search paths, first add the path to the python path, then run in order the init.py and initGui.py
    Q_FOREACH(const QString &templatesSearchDir, templatesSearchPath) {
        //Adding Qt resources to Python path is useless as Python does not know how to use it
        if ( templatesSearchDir.startsWith( QString::fromUtf8(":/Resources") ) ) {
            continue;
        }
        QDir d(templatesSearchDir);
        operateOnPathRecursive(&addToPythonPathFunctor, d);
    }

    ///Also import qtpy.QtCore and qtpy.QtGui (the later only in non background mode)
    {
        std::string s;
        s = "import qtpy\nfrom qtpy import QtCore";
        bool ok  = NATRON_PYTHON_NAMESPACE::interpretPythonScript(s, &err, 0);
        if (!ok) {
            QString message = tr("Failed to import qtpy.QtCore, make sure it is bundled with your Natron installation "
                                     "or reachable through the Python path. "
                                     "Note that Natron disables usage "
                                 "of site-packages).");
            std::cerr << message.toStdString() << std::endl;
            appPTR->writeToErrorLog_mt_safe(QLatin1String("qtpy.QtCore"), QDateTime::currentDateTime(), message);
        }
    }

    if ( !isBackground() ) {
        std::string s = "from qtpy import QtGui";
        bool ok  = NATRON_PYTHON_NAMESPACE::interpretPythonScript(s, &err, 0);
        if (!ok) {
            QString message = tr("Failed to import qtpy.QtGui");
            std::cerr << message.toStdString() << std::endl;
            appPTR->writeToErrorLog_mt_safe(QLatin1String("qtpy.QtGui"), QDateTime::currentDateTime(), message);
        }
    }


    QStringList foundInit;
    QStringList foundInitGui;
    Q_FOREACH(const QString &templatesSearchDir, templatesSearchPath) {
        QDir d(templatesSearchDir);

        findAllScriptsRecursive(d, allPlugins, &foundInit, &foundInitGui);
    }
    if ( foundInit.isEmpty() ) {
        QString message = tr("Info: init.py script not loaded (this is not an error)");
        appPTR->setLoadingStatus(message);
        if ( !appPTR->isBackground() ) {
            std::cout << message.toStdString() << std::endl;
        }
    } else {
        Q_FOREACH(const QString &found, foundInit) {
            QString message = tr("Info: init.py script found and loaded at %1").arg(found);

            appPTR->setLoadingStatus(message);
            if ( !appPTR->isBackground() ) {
                std::cout << message.toStdString() << std::endl;
            }
        }
    }

    if ( !appPTR->isBackground() ) {
        if ( foundInitGui.isEmpty() ) {
            QString message = tr("Info: initGui.py script not loaded (this is not an error)");
            appPTR->setLoadingStatus(message);
            if ( !appPTR->isBackground() ) {
                std::cout << message.toStdString() << std::endl;
            }
        } else {
            Q_FOREACH(const QString &found, foundInitGui) {
                QString message = tr("Info: initGui.py script found and loaded at %1").arg(found);

                appPTR->setLoadingStatus(message);
                if ( !appPTR->isBackground() ) {
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
        Q_FOREACH(const QString &newTemplatesSearchDir, newTemplatesSearchPath) {
            if ( !templatesSearchPath.contains(newTemplatesSearchDir) ) {
                diffSearch.push_back(newTemplatesSearchDir);
            }
        }

        //Add only paths that did not exist so far
        Q_FOREACH(const QString &diffDir, diffSearch) {
            QDir d(diffDir);

            operateOnPathRecursive(&addToPythonPathFunctor, d);
        }
    }

    appPTR->setLoadingStatus( tr("Loading PyPlugs...") );

    Q_FOREACH(const QString &plugin, allPlugins) {
        QString moduleName = plugin;
        QString modulePath;
        int lastDot = moduleName.lastIndexOf( QChar::fromLatin1('.') );

        if (lastDot != -1) {
            moduleName = moduleName.left(lastDot);
        }
        int lastSlash = moduleName.lastIndexOf( QChar::fromLatin1('/') );
        if (lastSlash != -1) {
            modulePath = moduleName.mid(0, lastSlash + 1);
            moduleName = moduleName.remove(0, lastSlash + 1);
        }


        {
            // Open the file and check for a line that imports NatronGui, if so do not attempt to load the script.
            QFile file(plugin);
            if (!file.open(QIODevice::ReadOnly)) {
                continue;
            }
            QTextStream ts(&file);
            bool gotNatronGuiImport = false;
            bool isPyPlug = false;
            while (!ts.atEnd()) {
                QString line = ts.readLine();
                if (line.startsWith(QString::fromUtf8("import %1").arg(QLatin1String(NATRON_GUI_PYTHON_MODULE_NAME))) ||
                    line.startsWith(QString::fromUtf8("from %1 import").arg(QLatin1String(NATRON_GUI_PYTHON_MODULE_NAME)))) {
                    gotNatronGuiImport = true;
                }
                // We have to find a way to tell PyPlugs from other python files.
                // We could check if the file was created by Natron...
                if (line.startsWith(QString::fromUtf8(NATRON_PYPLUG_GENERATED))) {
                    isPyPlug = true;
                }
                // Or we could check if createInstance(app,group) is defined
                if ( line.startsWith( QString::fromUtf8("def createInstance(") ) ) {
                    isPyPlug = true;
                }
                // Or we could check if it implements getIsToolset(), the name getGroupInfos() calls (#1262)
                if ( line.startsWith( QString::fromUtf8("def getIsToolset(") ) ) {
                    isPyPlug = true;
                }
                // Or we could check for the magic line that is in the doc.
                // See https://natron.readthedocs.io/en/master/devel/groups.html#creating-a-group-by-hand
                // and https://natron.readthedocs.io/en/master/devel/groups.html#toolsets
                if ( line.startsWith( QString::fromUtf8(NATRON_PYPLUG_MAGIC) ) ) {
                    isPyPlug = true;
                }
            }
            if (appPTR->isBackground() && gotNatronGuiImport) {
                continue;
            }
            if (!isPyPlug) {
                continue;
            }
        }

        std::string pluginLabel, pluginID, pluginGrouping, iconFilePath, pluginDescription;
        unsigned int version;
        bool isToolset;
        bool gotInfos = NATRON_PYTHON_NAMESPACE::getGroupInfos(modulePath.toStdString(), moduleName.toStdString(), &pluginID, &pluginLabel, &iconFilePath, &pluginGrouping, &pluginDescription, &isToolset, &version);


        if (gotInfos) {
            qDebug() << "Loading" << moduleName;
            QStringList grouping = QString::fromUtf8( pluginGrouping.c_str() ).split( QChar::fromLatin1('/') );
            Plugin* p = registerPlugin(modulePath, grouping, QString::fromUtf8( pluginID.c_str() ), QString::fromUtf8( pluginLabel.c_str() ), QString::fromUtf8( iconFilePath.c_str() ), QStringList(), false, false, 0, false, version, 0, false);

            p->setPythonModule(modulePath + moduleName);
            p->setToolsetScript(isToolset);
        }
    }
} // AppManager::loadPythonGroups

Plugin*
AppManager::registerPlugin(const QString& resourcesPath,
                           const QStringList & groups,
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
    QRecursiveMutex* pluginMutex = nullptr;

    if (mustCreateMutex) {
        pluginMutex = new QRecursiveMutex();
    }
    Plugin* plugin = new Plugin(binary, resourcesPath, pluginID, pluginLabel, pluginIconPath, groupIconPath, groups, pluginMutex, major, minor,
                                isReader, isWriter, isDeprecated);
    std::string stdID = pluginID.toStdString();

#ifdef NATRON_ENABLE_IO_META_NODES
    if ( ReadNode::isBundledReader( stdID, false ) ||
         WriteNode::isBundledWriter( stdID, false ) ) {
        plugin->setForInternalUseOnly(true);
    }
#endif

    PluginsMap::iterator found = _imp->_plugins.find(stdID);
    if ( found != _imp->_plugins.end() ) {
        found->second.insert(plugin);
    } else {
        PluginVersionsOrdered set;
        set.insert(plugin);
        _imp->_plugins.insert( std::make_pair(stdID, set) );
    }

    return plugin;
}

Format
AppManager::findExistingFormat(int w,
                               int h,
                               double par) const
{
    for (U32 i = 0; i < _imp->_formats.size(); ++i) {
        const Format& frmt = _imp->_formats[i];
        if ( (frmt.width() == w) && (frmt.height() == h) && (frmt.getPixelAspectRatio() == par) ) {
            return frmt;
        }
    }

    return Format();
}

void
AppManager::setAsTopLevelInstance(int appID)
{
    QMutexLocker k(&_imp->_appInstancesMutex);

    if (_imp->_topLevelInstanceID == appID) {
        return;
    }
    _imp->_topLevelInstanceID = appID;
    for (AppInstanceVec::iterator it = _imp->_appInstances.begin();
         it != _imp->_appInstances.end();
         ++it) {
        if ( (*it)->getAppID() != _imp->_topLevelInstanceID ) {
            if ( !isBackground() ) {
                (*it)->disconnectViewersFromViewerCache();
            }
        } else {
            if ( !isBackground() ) {
                (*it)->connectViewersToViewerCache();
                setOFXHostHandle( (*it)->getOfxHostOSHandle() );
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


const std::vector<Format> &
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
AppManager::getPluginBinaryFromOldID(const QString & pluginId,
                                     int majorVersion,
                                     int minorVersion) const
{
    std::map<int, Plugin*> matches;

    if ( pluginId == QString::fromUtf8("Viewer") ) {
        return _imp->findPluginById(QString::fromUtf8(PLUGINID_NATRON_VIEWER), majorVersion, minorVersion);
    } else if ( pluginId == QString::fromUtf8("Dot") ) {
        return _imp->findPluginById(QString::fromUtf8(PLUGINID_NATRON_DOT), majorVersion, minorVersion );
    } else if ( pluginId == QString::fromUtf8("DiskCache") ) {
        return _imp->findPluginById(QString::fromUtf8(PLUGINID_NATRON_DISKCACHE), majorVersion, minorVersion);
    } else if ( pluginId == QString::fromUtf8("Backdrop") ) { // DO NOT change the capitalization, even if it's wrong
        return _imp->findPluginById(QString::fromUtf8(PLUGINID_NATRON_BACKDROP), majorVersion, minorVersion);
    } else if ( pluginId == QString::fromUtf8("RotoOFX  [Draw]") ) {
        return _imp->findPluginById(QString::fromUtf8(PLUGINID_NATRON_ROTO), majorVersion, minorVersion);
    }

    ///Try remapping these ids to old ids we had in Natron < 1.0 for backward-compat
    for (PluginsMap::const_iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        assert( !it->second.empty() );
        PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin();
        QString friendlyLabel = (*itver)->getPluginLabel();
        const QStringList& s = (*itver)->getGrouping();
        QString grouping;
        if (s.size() > 0) {
            grouping = s[0];
        }
        friendlyLabel += QString::fromUtf8("  [") + grouping + QChar::fromLatin1(']');

        if (friendlyLabel == pluginId) {
            if (majorVersion == -1) {
                // -1 means we want to load the highest version existing
                return *( it->second.rbegin() );
            }

            PluginsMap::const_iterator foundID = it;
            
            // see also OFX::Host::PluginCache::getPluginById()
            // see also AppManager::getPluginBinary()

            // Let's be a bit smarter than HostSupport.
            // The best compatible plugin is, by order of preference
            // - the plugin with the same major version and the highest minor
            // - the plugin with the closest major above and the highest minor
            // - the plugin with the highest major and the highest minor
            //
            // For example, if versions (1,0) (1,2) (3,1) (3,4) (4,2) are available,
            // - asking for (1,0) returns (1,2)
            // - asking for (2,7) returns (3,4)
            // - asking for (3,1) returns (3,4)
            // - asking for (6,0) returns (4,2)

            // Try to find the exact major version, with the highest minor
            // (thus the reverse iterator)
            Plugin* nextPlugin = *(foundID->second.rbegin());
            int nextVersion = nextPlugin->getMajorVersion();
            for (PluginVersionsOrdered::const_reverse_iterator itver = foundID->second.rbegin(); itver != foundID->second.rend(); ++itver) {
                int thisMajorVersion = (*itver)->getMajorVersion();
                if (thisMajorVersion == majorVersion) {
                    return *itver;
                } else if (thisMajorVersion > majorVersion &&
                           thisMajorVersion < nextVersion) {
                    nextPlugin = *itver;
                    nextVersion = thisMajorVersion;
                } else if (thisMajorVersion < majorVersion) {
                    // no need to continue
                    break;
                }
            }
            assert(nextPlugin != NULL); // there should be at least one version

            // Could not find the exact version... get the major version above,
            // else the highest version.
            return nextPlugin;
        }
    }

    return 0;
}

Plugin*
AppManager::getPluginBinary(const QString & pluginId,
                            int majorVersion,
                            int minorVersion,
                            bool convertToLowerCase) const
{
    Q_UNUSED(minorVersion);
    
    PluginsMap::const_iterator foundID = _imp->_plugins.find( pluginId.toStdString() );
    if ( convertToLowerCase && foundID == _imp->_plugins.end() ) {
        foundID = _imp->_plugins.end();
        for (PluginsMap::const_iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
            QString pID = QString::fromUtf8( it->first.c_str() );
            if ( !pluginId.startsWith( QString::fromUtf8(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB ".built-in.") ) ) {
                QString lowerCase = pID.toLower();
                if (lowerCase == pluginId) {
                    foundID = it;
                    break;
                }
            }
        }
    }

    if ( foundID != _imp->_plugins.end() ) {
        assert( !foundID->second.empty() );

        // see also OFX::Host::PluginCache::getPluginById()
        // see also AppManager::getPluginBinaryFromOldID()

        if (majorVersion == -1) {
            // -1 means we want to load the highest version existing
            return *foundID->second.rbegin();
        }

        // Let's be a bit smarter than HostSupport.
        // The best compatible plugin is, by order of preference
        // - the plugin with the same major version and the highest minor
        // - the plugin with the closest major above and the highest minor
        // - the plugin with the highest major and the highest minor
        //
        // For example, if versions (1,0) (1,2) (3,1) (3,4) (4,2) are available,
        // - asking for (1,0) returns (1,2)
        // - asking for (2,7) returns (3,4)
        // - asking for (3,1) returns (3,4)
        // - asking for (6,0) returns (4,2)

        // Try to find the exact major version, with the highest minor
        // (thus the reverse iterator)
        Plugin* nextPlugin = *(foundID->second.rbegin());
        int nextVersion = nextPlugin->getMajorVersion();
        for (PluginVersionsOrdered::const_reverse_iterator itver = foundID->second.rbegin(); itver != foundID->second.rend(); ++itver) {
            int thisMajorVersion = (*itver)->getMajorVersion();
            if (thisMajorVersion == majorVersion) {
                return *itver;
            } else if (thisMajorVersion > majorVersion &&
                       thisMajorVersion < nextVersion) {
                nextPlugin = *itver;
                nextVersion = thisMajorVersion;
            } else if (thisMajorVersion < majorVersion) {
                // no need to continue
                break;
            }
        }
        assert(nextPlugin != NULL); // there should be at least one version

        // Could not find the exact version... get the major version above,
        // else the highest version.
        return nextPlugin;
    }
    QString exc = QString::fromUtf8("Couldn't find a plugin attached to the ID %1, with a major version of %2")
                  .arg(pluginId)
                  .arg(majorVersion);

    throw std::invalid_argument( exc.toStdString() );

    return 0;
}

EffectInstancePtr
AppManager::createOFXEffect(NodePtr node,
                            const CreateNodeArgs& args
#ifndef NATRON_ENABLE_IO_META_NODES
                            ,
                            bool allowFileDialogs,
                            bool *hasUsedFileDialog
#endif
                            ) const
{
    return _imp->ofxHost->createOfxEffect(node, args
#ifndef NATRON_ENABLE_IO_META_NODES
                                          , allowFileDialogs,
                                          hasUsedFileDialog
#endif
                                          );
}

void
AppManager::removeFromNodeCache(const ImagePtr & image)
{
    _imp->_nodeCache->removeEntry(image);
}

void
AppManager::removeFromViewerCache(const FrameEntryPtr & texture)
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
AppManager::removeAllImagesFromCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder,
                                                                  U64 treeVersion)
{
    _imp->_nodeCache->removeAllEntriesWithDifferentNodeHashForHolderPublic(holder, treeVersion);
}

void
AppManager::removeAllImagesFromDiskCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder,
                                                                      U64 treeVersion)
{
    _imp->_diskCache->removeAllEntriesWithDifferentNodeHashForHolderPublic(holder, treeVersion);
}

void
AppManager::removeAllTexturesFromCacheWithMatchingIDAndDifferentKey(const CacheEntryHolder* holder,
                                                                    U64 treeVersion)
{
    _imp->_viewerCache->removeAllEntriesWithDifferentNodeHashForHolderPublic(holder, treeVersion);
}

void
AppManager::removeAllTexturesFromCacheForHolderAtTime(const CacheEntryHolder* holder,
                                                      double time)
{
    _imp->_viewerCache->removeAllEntriesForHolderAtTime(holder, time);
}

void
AppManager::removeAllCacheEntriesForHolder(const CacheEntryHolder* holder,
                                           bool blocking)
{
    _imp->_nodeCache->removeAllEntriesForHolderPublic(holder, blocking);
    _imp->_diskCache->removeAllEntriesForHolderPublic(holder, blocking);
    _imp->_viewerCache->removeAllEntriesForHolderPublic(holder, blocking);
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
                     std::list<ImagePtr>* returnValue) const
{
    return _imp->_nodeCache->get(key, returnValue);
}

bool
AppManager::getImageOrCreate(const ImageKey & key,
                             const ImageParamsPtr& params,
                             ImagePtr* returnValue) const
{
    return _imp->_nodeCache->getOrCreate(key, params, 0, returnValue);
}

bool
AppManager::getImage_diskCache(const ImageKey & key,
                               std::list<ImagePtr>* returnValue) const
{
    return _imp->_diskCache->get(key, returnValue);
}

bool
AppManager::getImageOrCreate_diskCache(const ImageKey & key,
                                       const ImageParamsPtr& params,
                                       ImagePtr* returnValue) const
{
    return _imp->_diskCache->getOrCreate(key, params, 0, returnValue);
}

bool
AppManager::getTexture(const FrameKey & key,
                       std::list<FrameEntryPtr>* returnValue) const
{
    std::list<FrameEntryPtr> retList;
    bool ret =  _imp->_viewerCache->get(key, &retList);

    *returnValue = retList;

    return ret;
}

bool
AppManager::getTextureOrCreate(const FrameKey & key,
                               const FrameParamsPtr& params,
                               FrameEntryLocker* locker,
                               FrameEntryPtr* returnValue) const
{
    return _imp->_viewerCache->getOrCreate(key, params, locker, returnValue);
}

bool
AppManager::isAggressiveCachingEnabled() const
{
    return _imp->_settings->isAggressiveCachingEnabled();
}

U64
AppManager::getCachesTotalMemorySize() const
{
    return  _imp->_nodeCache->getMemoryCacheSize();
}

U64
AppManager::getCachesTotalDiskSize() const
{
    return  _imp->_diskCache->getDiskCacheSize() + _imp->_viewerCache->getDiskCacheSize();
}

CacheSignalEmitterPtr
AppManager::getOrActivateViewerCacheSignalEmitter() const
{
    return _imp->_viewerCache->activateSignalEmitter();
}

SettingsPtr AppManager::getCurrentSettings() const
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

AppInstancePtr
AppManager::makeNewInstance(int appID) const
{
    return std::make_shared<AppInstance>(appID);
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
    qRegisterMetaType<ViewSpec>("ViewSpec");
    qRegisterMetaType<NodePtr>("NodePtr");
    qRegisterMetaType<std::list<double> >("std::list<double>");
}

// The disk caches of all the Natron processes (Natron, NatronRenderer, the background renders) are in the same
// folder. Each process keeps a file of its own open in the "Instances" sub-folder, named <host>@<pid>-<uuid>.lock,
// locked with flock() on Unix (the lock is released when the process exits, even if it crashes), and open without
// sharing deletion on Windows (a file that is open, also by another host on an SMB share, can't be removed).
// A process only restores or wipes the caches (and deletes the cache files it doesn't know) when no other process
// uses them.
// - On a local file system, the lock tells everything: the host name is ignored (on macOS it changes with the network).
// - On a network file system, the lock of another host can't be trusted (e.g. NFS mounted with "nolock" makes flock()
//   local to each host): the file of another host is in use if it was touched by its owner in the last
//   kDiskCacheInstanceStaleHours (the owner touches it every 10 minutes, from a thread: see
//   AppManagerPrivate::startDiskCachesInstanceFileToucher()).
// - The files whose lock can't be tested (another user's file, a file system without flock(), which gives an error
//   other than EWOULDBLOCK) are considered in use, so the cleanup is disabled on such file systems.
// - Unlocked files with an older name format (no UUID) are stale and removed.
NATRON_NAMESPACE_ANONYMOUS_ENTER

const int kDiskCacheInstanceStaleHours = 2;

QString
diskCacheInstancesFolder(const QString& diskCachesLocation)
{
    return diskCachesLocation + QString::fromUtf8("/Instances");
}

// The file names start with "<host>@": hostnames can't contain '@'
QString
diskCacheInstanceFilePrefix()
{
    return QSysInfo::machineHostName() + QLatin1Char('@');
}

#ifdef Q_OS_UNIX
// Returns false if the folder is on a network file system
bool
isLocalFileSystem(const QString& folder)
{
    struct statfs fsStat;
    if (::statfs(folder.toUtf8().constData(), &fsStat) != 0) {
        return false; // can't tell: the safe side
    }
#if defined(__linux__)
    // Only the file systems that are known to be local: the others are considered as network file systems (the safe side)
    switch ( (unsigned long)(unsigned int)fsStat.f_type ) { // the magic numbers are 32-bit
    case 0xEF53UL:     // ext2, ext3, ext4
    case 0x58465342UL: // XFS
    case 0x9123683EUL: // Btrfs
    case 0x01021994UL: // tmpfs
    case 0x2FC12FC1UL: // ZFS
    case 0xF2F52010UL: // F2FS
    case 0x794C7630UL: // overlayfs
    case 0x858458F6UL: // ramfs
    case 0x73717368UL: // squashfs
    case 0x3153464AUL: // JFS
    case 0x52654973UL: // ReiserFS
    case 0xCA451A4EUL: // bcachefs
    case 0x4D44UL:     // FAT
    case 0x5346544EUL: // NTFS (ntfs3)
        return true;
    default:
        return false;
    }
#elif defined(MNT_LOCAL)
    return (fsStat.f_flags & MNT_LOCAL) != 0;
#else
    return false;
#endif
}

// True if the name has the format <host>@<pid>-<uuid>.lock
bool
isCurrentDiskCacheInstanceFileName(const QString& name)
{
    static const QRegularExpression re( QString::fromUtf8("^.+@[0-9]+-[0-9a-fA-F-]{36}\\.lock$") );

    return re.match(name).hasMatch();
}
#endif

// Opens and locks the file of this process. Returns NULL on failure.
QFile*
createDiskCacheInstanceFile(const QString& diskCachesLocation)
{
    const QString folder = diskCacheInstancesFolder(diskCachesLocation);
    QDir().mkpath(folder);
    // The PID is not enough: containers may share the hostname and the PID
    const QString path = folder + QLatin1Char('/') + diskCacheInstanceFilePrefix() +
                         QString::fromUtf8("%1-%2.lock").arg( QCoreApplication::applicationPid() ).arg( QUuid::createUuid().toString(QUuid::WithoutBraces) );
    for (int attempt = 0; attempt < 100; ++attempt) {
        std::unique_ptr<QFile> file( new QFile(path) );
        if ( !file->open(QIODevice::ReadWrite) ) {
            return NULL;
        }
#ifdef Q_OS_UNIX
        // Another process checking the files may hold a lock on it for a short time (see isDiskCacheInstanceFileInUse())
        int rc;
        while ( (rc = ::flock(file->handle(), LOCK_EX | LOCK_NB)) != 0 && (errno == EINTR) ) {
        }
        if (rc != 0) {
            if (errno == EWOULDBLOCK) {
                QThread::msleep(10);
                continue;
            }
            // No flock() on this file system: keep the file anyway, the other processes then consider it in use
            qDebug() << "Warning: could not lock" << path << ":" << std::strerror(errno);

            return file.release();
        }
        // A process checking the files may have locked it before this one, and removed it as stale: check that the
        // locked file is the one at this path
        struct stat fdStat, pathStat;
        if ( (::fstat(file->handle(), &fdStat) == 0) && (::stat(path.toUtf8().constData(), &pathStat) == 0) &&
             (fdStat.st_dev == pathStat.st_dev) && (fdStat.st_ino == pathStat.st_ino) ) {
            return file.release();
        }
        // Removed meanwhile: create it again
#else
        return file.release();
#endif
    }

    return NULL;
}

// Returns true if the file of another process is in use, and removes it otherwise (the process is gone).
bool
isDiskCacheInstanceFileInUse(const QString& path,
                             bool isLocal)
{
#ifdef Q_OS_UNIX
    QFile file(path);
    if ( !file.open(QIODevice::ReadWrite) ) {
        // e.g. another user's file: can't tell, and don't remove it
        return file.exists();
    }
    int rc;
    while ( (rc = ::flock(file.handle(), LOCK_EX | LOCK_NB)) != 0 && (errno == EINTR) ) {
    }
    if (rc != 0) {
        // EWOULDBLOCK: locked by a running process. Other errors (no flock()): can't tell
        return true;
    }
    const QString name = QFileInfo(path).fileName();
    if ( !isLocal && isCurrentDiskCacheInstanceFileName(name) && !name.startsWith( diskCacheInstanceFilePrefix() ) ) {
        // Another host, on a network file system: the lock can't be trusted, use the time it was last touched
        const QDateTime lastTouched = QFileInfo(path).lastModified();
        if ( lastTouched.secsTo( QDateTime::currentDateTime() ) < kDiskCacheInstanceStaleHours * 3600LL ) {
            return true;
        }
    }
    // Remove it while it is locked: a process that creates it again checks that its file is still there
    QFile::remove(path);

    return false;
#else
    Q_UNUSED(isLocal);
    // A file that is open by a running process can't be removed
    return !QFile::remove(path) && QFile::exists(path);
#endif
}

NATRON_NAMESPACE_ANONYMOUS_EXIT

void
AppManager::refreshDiskCacheLocation()
{
    // The environment variable takes precedence over the "Disk cache path" preference, as documented (#1265).
    // The settings do not exist yet when this is first called by loadInternal().
    QStringList paths;
    paths << QString::fromUtf8( qgetenv(NATRON_DISK_CACHE_PATH_ENV_VAR) );
    if (_imp->_settings) {
        paths << _imp->_settings->getDiskCachePath();
    }
    QMutexLocker k(&_imp->diskCachesLocationMutex);
    QString location;
    Q_FOREACH(const QString &path, paths) {
        if ( !path.isEmpty() ) {
            QDir d(path);
            // create the full path if the directory does not exist
            if ( d.exists() || d.mkpath(path) ) {
                location = path;
                break;
            }
        }
    }
    if ( location.isEmpty() ) {
        location = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache);
    }
    if ( (location != _imp->diskCachesLocation) || !_imp->diskCachesInstanceFile ) {
        // Tell the other processes that this one uses the caches in the new location
        if (_imp->diskCachesInstanceFile) {
            const QString oldPath = _imp->diskCachesInstanceFile->fileName();
            _imp->diskCachesInstanceFile.reset();
            QFile::remove(oldPath);
        }
        _imp->diskCachesInstanceFile.reset( createDiskCacheInstanceFile(location) );
        if (!_imp->diskCachesInstanceFile) {
            qDebug() << "Warning: could not create the instance file of the disk cache in" << diskCacheInstancesFolder(location);
        }
        // Touch it periodically: on a network file system, the other hosts consider it stale if it is not touched
        _imp->startDiskCachesInstanceFileToucher();
    }
    _imp->diskCachesLocation = location;
}

void
AppManager::setDiskCacheInstanceFileTouchInterval(int milliseconds)
{
    {
        std::lock_guard<std::mutex> l(_imp->diskCachesInstanceFileToucherMutex);
        _imp->diskCachesInstanceFileTouchMs = milliseconds;
    }
    _imp->diskCachesInstanceFileToucherCond.notify_all();
}

bool
AppManager::isDiskCacheUsedByAnotherProcess() const
{
    QString folder;
    QString ownFile;
    {
        QMutexLocker k(&_imp->diskCachesLocationMutex);
        folder = diskCacheInstancesFolder(_imp->diskCachesLocation);
        if (_imp->diskCachesInstanceFile) {
            ownFile = QFileInfo( _imp->diskCachesInstanceFile->fileName() ).fileName();
        }
    }
    const QStringList files = QDir(folder).entryList(QStringList() << QString::fromUtf8("*.lock"), QDir::Files);
#ifdef Q_OS_UNIX
    const bool isLocal = files.isEmpty() || isLocalFileSystem(folder);
#else
    const bool isLocal = true; // unused
#endif
    Q_FOREACH(const QString &file, files) {
        if ( (file != ownFile) && isDiskCacheInstanceFileInUse(folder + QLatin1Char('/') + file, isLocal) ) {
            return true;
        }
    }

    return false;
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
    AppInstanceVec copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }

    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->onMaxPanelsOpenedChanged(maxPanels);
    }
}

void
AppManager::onQueueRendersChanged(bool queuingEnabled)
{
    AppInstanceVec copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }

    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->onRenderQueuingChanged(queuingEnabled);
    }
}

bool
AppManager::isDialogsDisabled() const
{
    return _imp->dialogsDisabled;
}

void
AppManager::setDialogsDisabled(bool disabled)
{
    _imp->dialogsDisabled = disabled;
}

void
AppManager::requestQuit(int exitCode)
{
    // Background mode: there is no event loop to leave. Record the exit code (returned by
    // main()), and stop what is running. AppInstance::load() skips the renders and the
    // interpreter that were not started yet, then the instance quits as usual.
    _imp->requestedExitCode = exitCode;
    _imp->quitRequested = true;
    if ( isBackground() || ( QThread::currentThread() == qApp->thread() ) ) {
        // abortAnyProcessing() does not block. In background mode, call it even from a render
        // thread (e.g. natron.quit() in afterFrameRender): the main thread waits for the render
        // and runs no event loop, so a queued call would only run once all frames are rendered.
        abortAnyProcessing();
    } else {
        // GUI, called from a render thread: stop the renders from the main thread
        QMetaObject::invokeMethod( this, [this]() {
            abortAnyProcessing();
        }, Qt::QueuedConnection );
    }
}

bool
AppManager::isQuitRequested() const
{
    return _imp->quitRequested;
}

int
AppManager::getRequestedExitCode() const
{
    if (!_imp->quitRequested && _imp->backgroundRenderFailed) {
        // A farm manager reads the exit status: a render that failed must not report a success.
        return 1;
    }

    return _imp->requestedExitCode;
}

void
AppManager::setBackgroundRenderFailed()
{
    if (!_imp->quitRequested) {
        _imp->backgroundRenderFailed = true;
    }
}

bool
AppManager::isPythonInterpreterRunning() const
{
    return _imp->pythonInterpreterRunning;
}

AppManager::NestedEventLoopRAII::NestedEventLoopRAII()
{
    ++appPTR->_imp->nestedEventLoops;
}

AppManager::NestedEventLoopRAII::~NestedEventLoopRAII()
{
    --appPTR->_imp->nestedEventLoops;
}

int
AppManager::getNestedEventLoopsCount() const
{
    return _imp->nestedEventLoops;
}

int
AppManager::exec()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
    int ret = qApp->exec();
#ifdef __NATRON_UNIX__
    // The event loop is over: it can't quit the application on a signal anymore.
    disarmShutDownSignalPipe();
#endif

    return ret;
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

void
AppManager::getErrorLog_mt_safe(std::list<LogEntry>* entries) const
{
    QMutexLocker l(&_imp->errorLogMutex);
    *entries = _imp->errorLog;
}

void
AppManager::writeToErrorLog_mt_safe(const QString& context,
                                    const QDateTime& date,
                                    const QString & str,
                                    bool isHtml,
                                    const LogEntry::LogEntryColor& color)
{
    QMutexLocker l(&_imp->errorLogMutex);
    LogEntry e;
    e.context = context;
    e.date = date;
    e.message = str;
    e.isHtml = isHtml;
    e.color = color;
    _imp->errorLog.push_back(e);
}

void
AppManager::showErrorLog()
{
    std::list<LogEntry> log;
    getErrorLog_mt_safe(&log);
    for (std::list<LogEntry>::iterator it = log.begin(); it != log.end(); ++it) {
        // only print time - QTime.toString() uses the system locale, that's not what we want
        std::cout << QString::fromUtf8("[%2] %1: %3")
                     .arg(it->context)
                     .arg( QLocale().toString( it->date.time(), QString::fromUtf8("HH:mm:ss.zzz")) )
                     .arg(it->message).toStdString() << std::endl;
    }
}

void
AppManager::clearErrorLog_mt_safe()
{
    QMutexLocker l(&_imp->errorLogMutex);

    _imp->errorLog.clear();
}

void
AppManager::exitApp(bool /*warnUserForSave*/)
{
    const AppInstanceVec & instances = getAppInstances();

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        (*it)->quitNow();
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

    if ( (double)nodeCacheSize / nodeMaxCacheSize >= NATRON_CACHE_LIMIT_PERCENT ) {
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

    while (totalFreeRAM <= systemRAMToKeepFree) {
#ifdef NATRON_DEBUG_CACHE
        qDebug() << "Total system free RAM is below the threshold:" << printAsRAM(totalFreeRAM)
        << ", clearing least recently used NodeCache image...";
#endif
        if ( !_imp->_nodeCache->evictLRUInMemoryEntry() ) {
            break;
        }


        totalFreeRAM = getAmountFreePhysicalRAM();
    }
}

void
AppManager::onOCIOConfigPathChanged(const std::string& path)
{
    _imp->currentOCIOConfigPath = path;

    AppInstanceVec copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }

    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->onOCIOConfigPathChanged(path);
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
AppManager::getNThreadsSettings(int* nThreadsToRender,
                                int* nThreadsPerEffect) const
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
AppManager::setThreadAsActionCaller(OfxImageEffectInstance* instance,
                                    bool actionCaller)
{
    _imp->ofxHost->setThreadAsActionCaller(instance, actionCaller);
}

void
AppManager::requestOFXDIalogOnMainThread(OfxImageEffectInstance* instance,
                                         void* instanceData)
{
    if ( QThread::currentThread() == qApp->thread() ) {
        onOFXDialogOnMainThreadReceived(instance, instanceData);
    } else {
        Q_EMIT s_requestOFXDialogOnMainThread(instance, instanceData);
    }
}

void
AppManager::onOFXDialogOnMainThreadReceived(OfxImageEffectInstance* instance,
                                            void* instanceData)
{
    assert( QThread::currentThread() == qApp->thread() );
    // never NULL: for a request with no instance handle, OfxHost::requestDialog() finds the effect on the
    // requesting thread, or fails (#1497)
    assert(instance);
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

    for (PluginsMap::const_iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        assert( !it->second.empty() );
        ret.push_back(it->first);
    }

    return ret;
}

std::list<std::string>
AppManager::getPluginIDs(const std::string& filter)
{
    QString qFilter = QString::fromUtf8( filter.c_str() );
    std::list<std::string> ret;

    for (PluginsMap::const_iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        assert( !it->second.empty() );

        QString pluginID = QString::fromUtf8( it->first.c_str() );
        if ( pluginID.contains(qFilter, Qt::CaseInsensitive) ) {
            ret.push_back(it->first);
        }
    }

    return ret;
}


void
AppManager::initPython()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    //Disable user sites as they could conflict with Natron bundled packages.
    //If this is set, Python won’t add the user site-packages directory to sys.path.
    //See https://www.python.org/dev/peps/pep-0370/
    // (initializePython3() also disables it in the Python configuration)
    qputenv("PYTHONNOUSERSITE", "1");

    //
    // set up paths, clear those that don't exist or are not valid
    //
    QString binPath = QCoreApplication::applicationDirPath();
    binPath = QDir::toNativeSeparators(binPath);
    std::wstring pythonHome = NATRON_PYTHON_NAMESPACE::setupPythonEnv(binPath.toStdString());

    ///Must be called prior to Py_Initialize (calls PyImport_AppendInittab())
    initBuiltinPythonModules();
    
    _imp->mainModule = NATRON_PYTHON_NAMESPACE::initializePython3(_imp->commandLineArgsWide, pythonHome);

    std::string err;
    std::string modulename = NATRON_ENGINE_PYTHON_MODULE_NAME;
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\nfrom math import *\nimport " + modulename, &err, 0);
    if (!ok) {
        throw std::runtime_error( tr("Error while loading python module %1: %2").arg( QString::fromUtf8( modulename.c_str() ) ).arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
    }

    ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(modulename + ".natron = " + modulename + ".PyCoreApplication()\n", &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error( tr("Error while loading python module %1: %2").arg( QString::fromUtf8( modulename.c_str() ) ).arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
    }

    if ( !isBackground() ) {
        modulename = NATRON_GUI_PYTHON_MODULE_NAME;
        ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\nimport " + modulename, &err, 0);
        assert(ok);
        if (!ok) {
            throw std::runtime_error( tr("Error while loading python module %1: %2").arg( QString::fromUtf8( modulename.c_str() ) ).arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
        }

        ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(modulename + ".natron = " +
                                                            modulename + ".PyGuiApplication()\n", &err, 0);
        assert(ok);
        if (!ok) {
            throw std::runtime_error( tr("Error while loading python module %1: %2").arg( QString::fromUtf8( modulename.c_str() ) ).arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
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
        ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
        assert(ok);
        if (!ok) {
            throw std::runtime_error( tr("Error while loading StreamCatcher: %1").arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
        }
    }
    // Set QT_API for QtPy, to the PySide that matches the Qt version Natron is built with (#1262)
    // https://github.com/spyder-ide/qtpy
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qputenv("QT_API", "pyside6");
#else
    qputenv("QT_API", "pyside2");
#endif
} // AppManager::initPython

void
AppManager::tearDownPython()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if ( !Py_IsInitialized() ) {
        return;
    }

    ///See https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
#if !defined(NDEBUG)
    QThread* curThread = QThread::currentThread();
    QString threadname = (qApp && qApp->thread() == curThread) ? QString::fromUtf8("Main") : curThread->objectName();
    qDebug() << QString::fromUtf8("Thread '%1' is asking the Python GIL").arg(threadname);
#endif
    PyGILState_Ensure();
#if !defined(NDEBUG)
    qDebug() << QString::fromUtf8("Thread '%1' got the Python GIL").arg(threadname);
#endif

    Py_DECREF(_imp->mainModule);
#if !defined(NDEBUG)
    qDebug() << "Finalizing Python";
#endif
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
PyObject* PyInit_NatronEngine();
}

void
AppManager::initBuiltinPythonModules()
{
    int ret = PyImport_AppendInittab(NATRON_ENGINE_PYTHON_MODULE_NAME, &PyInit_NatronEngine);
    if (ret == -1) {
        throw std::runtime_error("Failed to initialize built-in Python module.");
    }
}

bool
AppManager::isTestMode() const
{
    return _imp->testMode;
}

bool
AppManager::keepsOtherSessionsAutoSaves() const
{
    return isDialogsDisabled() || isTestMode();
}

bool
AppManager::hasRenderInProgress() const
{
    AppInstanceVec instances;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        instances = _imp->_appInstances;
    }
    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        if ( *it && (*it)->hasRenderInProgress() ) {
            return true;
        }
    }

    return false;
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
    AppInstanceVec copy;
    {
        QMutexLocker k(&_imp->_appInstancesMutex);
        copy = _imp->_appInstances;
    }

    for (AppInstanceVec::iterator it = copy.begin(); it != copy.end(); ++it) {
        (*it)->toggleAutoHideGraphInputs();
    }
}

// Registered with atexit by registerCloseProjectsAtPythonExit() in the background modes that run
// scripts: Py_Main() (-t), or sys.exit() in a script, finalizes Python, so close the projects while Python
// still runs, for the onProjectClose callbacks and the removal of the node variables (#1253), then unload the
// OFX plug-ins (cleanUpAtPythonExit(), #1557): sys.exit() then calls exit(), and ~AppManager never runs.
// After Py_Main(), loadInternalAfterInitGui() then quits the instances.
static PyObject*
closeProjectsAtPythonExit(PyObject* /*self*/,
                          PyObject* /*args*/)
{
    // SystemExit raised on another thread (e.g. sys.exit() in an expression evaluated by a
    // render thread) also finalizes Python: the projects can only be reset on the main thread.
    if ( !qApp || (QThread::currentThread() != qApp->thread()) ) {
        Py_RETURN_NONE;
    }
    if (appPTR) {
        const AppInstanceVec instances = appPTR->getAppInstances();
        // Release the GIL while the nodes are deleted: they may wait for threads that need it.
        // The callbacks take it again with PythonGILLocker.
        Py_BEGIN_ALLOW_THREADS
        bool projectsClosed = true;
        for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            try {
                (*it)->getProject()->reset(true /*aboutToQuit*/, true /*blocking*/);
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                projectsClosed = false;
            }
        }
        // sys.exit() leaves through exit(): ~AppManager won't unload the OFX plug-ins (#1557).
        // Not if a project could not be closed: its OFX instances are still alive, and would be destroyed
        // (kOfxActionDestroyInstance) after their plug-ins were unloaded.
        if (projectsClosed) {
            appPTR->cleanUpAtPythonExit();
        }
        Py_END_ALLOW_THREADS
    }
    Py_RETURN_NONE;
}

void
AppManager::cleanUpAtPythonExit()
{
    // Python is finalized by sys.exit() or natron.quit() in a script or at the prompt of the -t option, which then
    // call exit() (Py_Exit()), or at the end of the prompt (Py_Main()), after which ~AppManager runs. In the first
    // case, the OFX plug-ins would never get kOfxActionUnload, and the threads that some start (e.g. the thread
    // pool of OpenImageIO) would only be stopped by their static destructors, in exit(), which may deadlock on
    // Windows (#1557). os._exit() runs nothing, not even this. Neither does a SystemExit raised on another thread
    // (see closeProjectsAtPythonExit()), nor sys.exit() in a script of the GUI or of natron-python, which don't
    // register the hook. The atexit callbacks registered before this one (e.g. by init.py) run after it, with the
    // plug-ins unloaded and the projects closed.
    // The callers run on the main thread, with the projects closed: their renders were aborted and waited for, and
    // their nodes deleted. Only the tasks of the global thread pool may still run (the threads of the
    // multi-thread suite of the OFX plug-ins, the deletion of images): wait for them, as ~AppManager does.
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->destroying) {
        return;
    }
    // sys.exit() in an expression or a Python callback that Natron evaluates during an OFX action on the main thread:
    // the plug-in is still in that action, its binary can't be unloaded under it
    if ( _imp->ofxHost && _imp->ofxHost->isActionRunningInThisThread() ) {
        std::cerr << "Warning: Python exits during an OpenFX action: the OpenFX plug-ins are not unloaded." << std::endl;

        return;
    }
    QThreadPool::globalInstance()->waitForDone();
    // Keep the images of the DiskCache nodes for the next process, as ~AppManager does
    try {
        _imp->saveCaches();
    } catch (std::runtime_error&) {
        // ignore errors
    }
    // Wait for the images that the caches gave to their cleaner and deleter threads, as ~AppManager does. The caches
    // stay until ~AppManager (after the -t prompt) or exit().
    if (_imp->_nodeCache) {
        _imp->_nodeCache->waitForDeleterThread();
    }
    if (_imp->_diskCache) {
        _imp->_diskCache->waitForDeleterThread();
    }
    if (_imp->_viewerCache) {
        _imp->_viewerCache->waitForDeleterThread();
    }
    // Each loaded plug-in gets kOfxActionUnload, and its binary is unloaded, on the main thread, before exit().
    // The plug-ins are unloaded once: ~OfxHost won't unload them again.
    if (_imp->ofxHost) {
        _imp->ofxHost->unloadPlugins();
    }
}

bool
AppManager::areOFXPluginsUnloaded() const
{
    return _imp->ofxHost && _imp->ofxHost->arePluginsUnloaded();
}

static PyMethodDef closeProjectsAtPythonExitDef = {
    "_natronCloseProjectsAtExit", closeProjectsAtPythonExit, METH_NOARGS, NULL
};

void
AppManager::registerCloseProjectsAtPythonExit()
{
    // Before the scripts of the background modes run: sys.exit() in a script finalizes Python too (#1253).
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;
    PythonGILLocker pgl;
    PyObject* atexitModule = PyImport_ImportModule("atexit");
    PyObject* func = PyCFunction_New(&closeProjectsAtPythonExitDef, NULL);
    PyObject* ret = (atexitModule && func) ? PyObject_CallMethod(atexitModule, "register", "O", func) : NULL;
    if (!ret) {
        PyErr_Print();
    }
    Py_XDECREF(ret);
    Py_XDECREF(func);
    Py_XDECREF(atexitModule);
}

void
AppManager::launchPythonInterpreter()
{
    std::string err;
    std::string s = "app = app1\n";
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(s, &err, 0);

    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::launchPythonInterpreter(): interpretPythonScript(" + s + " failed!");
    }

    PyGILState_Ensure(); // Py_Main does PyGILState_Release() for us.

    assert(PyThreadState_Get());
#if PY_VERSION_HEX >= 0x030400F0
    assert(PyGILState_Check()); // Not available prior to Python 3.4
#endif

#ifdef __NATRON_WIN32__
    // On Windows, isatty() is true for every character device, the NUL device included: with stdin
    // redirected from NUL (a render farm job, "< /dev/null" in a script), Python (3.13 and later)
    // takes stdin for a terminal and starts its new prompt (pyrepl), which needs a console. Each
    // read then fails with an OSError that the prompt prints before it retries, forever: the
    // process never exits and its output grows without bound. Without a console on stdin, ask for
    // the basic prompt, which reads the commands from stdin and exits at the end of the file.
    {
        HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
        DWORD consoleMode = 0;
        if ( !stdinHandle || (stdinHandle == INVALID_HANDLE_VALUE) || !GetConsoleMode(stdinHandle, &consoleMode) ) {
            qputenv("PYTHON_BASIC_REPL", "1");
        }
    }
#endif

    _imp->pythonInterpreterRunning = true;
    const int status = Py_Main(1, &_imp->commandLineArgsWide[0]);
    _imp->pythonInterpreterRunning = false;
    // The exit status of the prompt, as for python: 1 if the commands read from a file or a pipe raised an
    // exception, and, from Python 3.13 on, the status of sys.exit() at the prompt: Py_Main() returns it instead of
    // calling exit() (_pymain_run_repl()), so Natron exited with status 0 (#1557). natron.quit(exitCode) gave its
    // own status.
    if ( (status != 0) && !_imp->quitRequested ) {
        _imp->requestedExitCode = status;
    }
}

int
AppManager::isProjectAlreadyOpened(const std::string& projectFilePath) const
{
    QMutexLocker k(&_imp->_appInstancesMutex);

    for (AppInstanceVec::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        ProjectPtr proj = (*it)->getProject();
        if (proj) {
            QString path = proj->getProjectPath();
            QString name = proj->getProjectFilename();
            std::string existingProject = path.toStdString() + name.toStdString();
            if (existingProject == projectFilePath) {
                return (*it)->getAppID();
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
    QString error = tr("%1 has detected that the crash reporter process is no longer responding. "
                       "This most likely indicates that it was killed or that the "
                       "communication between the 2 processes is failing.")
                    .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) );
    std::cerr << error.toStdString() << std::endl;
    writeToErrorLog_mt_safe(tr("Crash-Reporter"), QDateTime::currentDateTime(), error );
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
        ret.push_back( it->toStdString() );
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
AppManager::registerUNCPath(const QString& path,
                            const QChar& driveLetter)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->uncPathMapping[driveLetter] = path;
}

QString
AppManager::mapUNCPathToPathWithDriveLetter(const QString& uncPath) const
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( uncPath.isEmpty() ) {
        return uncPath;
    }
    for (std::map<QChar, QString>::const_iterator it = _imp->uncPathMapping.begin(); it != _imp->uncPathMapping.end(); ++it) {
        int index = uncPath.indexOf(it->second);
        if (index == 0) {
            //We found the UNC mapping at the start of the path, replace it with a drive letter
            QString ret = uncPath;
            ret.remove( 0, it->second.size() );
            QString drive;
            drive.append(it->first);
            drive.append( QLatin1Char(':') );
            if ( !ret.isEmpty() && !ret.startsWith( QLatin1Char('/') ) ) {
                drive.append( QLatin1Char('/') );
            }
            ret.prepend(drive);

            return ret;
        }
    }

    return uncPath;
}

#endif

const IOPluginsMap&
AppManager::getFileFormatsForReadingAndReader() const
{
    return _imp->readerPlugins;
}

const IOPluginsMap&
AppManager::getFileFormatsForWritingAndWriter() const
{
    return _imp->writerPlugins;
}

void
AppManager::getSupportedReaderFileFormats(std::vector<std::string>* formats) const
{
    const IOPluginsMap& readersForFormat = getFileFormatsForReadingAndReader();

    formats->resize( readersForFormat.size() );
    int i = 0;
    for (IOPluginsMap::const_iterator it = readersForFormat.begin(); it != readersForFormat.end(); ++it, ++i) {
        (*formats)[i] = it->first;
    }
}

void
AppManager::getSupportedWriterFileFormats(std::vector<std::string>* formats) const
{
    const IOPluginsMap& writersForFormat = getFileFormatsForWritingAndWriter();

    formats->resize( writersForFormat.size() );
    int i = 0;
    for (IOPluginsMap::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it, ++i) {
        (*formats)[i] = it->first;
    }
}

void
AppManager::getReadersForFormat(const std::string& format,
                                IOPluginSetForFormat* decoders) const
{
    // This will perform a case insensitive find
    IOPluginsMap::const_iterator found = _imp->readerPlugins.find(format);

    if ( found == _imp->readerPlugins.end() ) {
        return;
    }
    *decoders = found->second;
}

void
AppManager::getWritersForFormat(const std::string& format,
                                IOPluginSetForFormat* encoders) const
{
    // This will perform a case insensitive find
    IOPluginsMap::const_iterator found = _imp->writerPlugins.find(format);

    if ( found == _imp->writerPlugins.end() ) {
        return;
    }
    *encoders = found->second;
}

std::string
AppManager::getReaderPluginIDForFileType(const std::string & extension) const
{
    // This will perform a case insensitive find
    IOPluginsMap::const_iterator found = _imp->readerPlugins.find(extension);

    if ( found == _imp->readerPlugins.end() ) {
        return std::string();
    }
    // Return the "best" plug-in (i.e: higher score)

    return found->second.empty() ? std::string() : found->second.rbegin()->pluginID;
}

std::string
AppManager::getWriterPluginIDForFileType(const std::string & extension) const
{
    // This will perform a case insensitive find
    IOPluginsMap::const_iterator found = _imp->writerPlugins.find(extension);

    if ( found == _imp->writerPlugins.end() ) {
        return std::string();
    }
    // Return the "best" plug-in (i.e: higher score)

    return found->second.empty() ? std::string() : found->second.rbegin()->pluginID;
}


AppTLS*
AppManager::getAppTLS() const
{
    return &_imp->globalTLS;
}


QString
AppManager::getBoostVersion() const
{
    return QString::fromUtf8(BOOST_LIB_VERSION);
}

QString
AppManager::getQtVersion() const
{
    return QString::fromUtf8(QT_VERSION_STR) + QString::fromUtf8(" / ") + QString::fromUtf8( qVersion() );
}

QString
AppManager::getCairoVersion() const
{
    return QString::fromUtf8(CAIRO_VERSION_STRING) + QString::fromUtf8(" / ") + QString::fromUtf8( cairo_version_string() );
}


QString
AppManager::getMd4cVersion() const
{
    // md4c has no version macro or function: NATRON_MD4C_VERSION is defined
    // by the build system (libs/md4c/CMakeLists.txt, libs.pri)
    return QString::fromUtf8(NATRON_MD4C_VERSION);
}


QString
AppManager::getCeresVersion() const
{
    return QString::fromUtf8(CERES_VERSION_STRING);
}


QString
AppManager::getOpenMVGVersion() const
{
    return QString::fromUtf8(OPENMVG_VERSION_STRING);
}


QString
AppManager::getPySideVersion() const
{
    return QString::fromUtf8(SHIBOKEN_VERSION);
}

const NATRON_NAMESPACE::OfxHost*
AppManager::getOFXHost() const
{
    return _imp->ofxHost.get();
}

GPUContextPool*
AppManager::getGPUContextPool() const
{
    return _imp->renderingContextPool.get();
}

void
AppManager::refreshOpenGLRenderingFlagOnAllInstances()
{
    for (std::size_t i = 0; i < _imp->_appInstances.size(); ++i) {
        _imp->_appInstances[i]->getProject()->refreshOpenGLRenderingFlagOnNodes();
    }
}

void
Dialogs::errorDialog(const std::string & title,
                     const std::string & message,
                     bool useHtml)
{
    if ( appPTR->isDialogsDisabled() ) {
        printDismissedDialog(title, message);

        return;
    }

    appPTR->hideSplashScreen();
    AppInstancePtr topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->errorDialog(title, message, useHtml);
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
    if ( appPTR->isDialogsDisabled() ) {
        printDismissedDialog(title, message);
        *stopAsking = false;

        return;
    }

    appPTR->hideSplashScreen();
    AppInstancePtr topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->errorDialog(title, message, stopAsking, useHtml);
    } else {
        std::cerr << "ERROR: " << title << ": " <<  message << std::endl;
    }
}

void
Dialogs::warningDialog(const std::string & title,
                       const std::string & message,
                       bool useHtml)
{
    if ( appPTR->isDialogsDisabled() ) {
        printDismissedDialog(title, message);

        return;
    }

    appPTR->hideSplashScreen();
    AppInstancePtr topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->warningDialog(title, message, useHtml);
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
    if ( appPTR->isDialogsDisabled() ) {
        printDismissedDialog(title, message);
        *stopAsking = false;

        return;
    }

    appPTR->hideSplashScreen();
    AppInstancePtr topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->warningDialog(title, message, stopAsking, useHtml);
    } else {
        std::cerr << "WARNING: " << title << ":" << message << std::endl;
    }
}

void
Dialogs::informationDialog(const std::string & title,
                           const std::string & message,
                           bool useHtml)
{
    if ( appPTR->isDialogsDisabled() ) {
        printDismissedDialog(title, message);

        return;
    }

    appPTR->hideSplashScreen();
    AppInstancePtr topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->informationDialog(title, message, useHtml);
    } else {
        std::cout << "INFO: " << title << ":" << message << std::endl;
    }
}

void
Dialogs::informationDialog(const std::string & title,
                           const std::string & message,
                           bool* stopAsking,
                           bool useHtml)
{
    if ( appPTR->isDialogsDisabled() ) {
        printDismissedDialog(title, message);
        *stopAsking = false;

        return;
    }

    appPTR->hideSplashScreen();
    AppInstancePtr topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        topLvlInstance->informationDialog(title, message, stopAsking, useHtml);
    } else {
        std::cout << "INFO: " << title << ":" << message << std::endl;
    }
}

StandardButtonEnum
Dialogs::questionDialog(const std::string & title,
                        const std::string & message,
                        bool useHtml,
                        StandardButtons buttons,
                        StandardButtonEnum defaultButton)
{
    if ( appPTR->isDialogsDisabled() ) {
        printDismissedDialog(title, message);

        return getDismissedDialogAnswer(buttons, defaultButton);
    }

    appPTR->hideSplashScreen();
    AppInstancePtr topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        return topLvlInstance->questionDialog(title, message, useHtml, buttons, defaultButton);
    } else {
        std::cout << "QUESTION ASKED: " << title << ":" << message << std::endl;
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
    if ( appPTR->isDialogsDisabled() ) {
        printDismissedDialog(title, message);
        *stopAsking = false;

        return getDismissedDialogAnswer(buttons, defaultButton);
    }

    appPTR->hideSplashScreen();
    AppInstancePtr topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        return topLvlInstance->questionDialog(title, message, useHtml, buttons, defaultButton, stopAsking);
    } else {
        std::cout << "QUESTION ASKED: " << title << ":" << message << std::endl;
        std::cout << NATRON_APPLICATION_NAME " answered yes." << std::endl;

        return eStandardButtonYes;
    }
}

StandardButtonEnum
Dialogs::getDismissedDialogAnswer(StandardButtons buttons,
                                  StandardButtonEnum defaultButton)
{
    // Enter picks the default button
    if ( (defaultButton != eStandardButtonNoButton) && (defaultButton != eStandardButtonEscape) ) {
        return defaultButton;
    }
    // otherwise, pick the button Escape would pick (see QMessageBox::detectEscapeButton())
    const StandardButtonEnum escapeButtons[] = {
        eStandardButtonCancel, eStandardButtonClose, eStandardButtonAbort, eStandardButtonNo, eStandardButtonNoToAll
    };
    for (std::size_t i = 0; i < sizeof(escapeButtons) / sizeof(escapeButtons[0]); ++i) {
        if ( buttons.testFlag(escapeButtons[i]) ) {
            return escapeButtons[i];
        }
    }

    return eStandardButtonEscape;
}

void
Dialogs::printDismissedDialog(const std::string & title,
                              const std::string & message)
{
    std::cerr << "[dialog dismissed] " << title << ": " << message << std::endl;
}

#if 0 // dead code
std::size_t
NATRON_PYTHON_NAMESPACE::findNewLineStartAfterImports(std::string& script)
{
    ///Find position of the last import
    size_t foundImport = script.find("import ");

    if (foundImport != std::string::npos) {
        for (;; ) {
            size_t found = script.find("import ", foundImport + 1);
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
    size_t endLine = script.find('\n', foundImport + 1);


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
NATRON_PYTHON_NAMESPACE::getMainModule()
{
    return appPTR->getMainModule();
}

#if 0 // dead code
std::size_t
NATRON_PYTHON_NAMESPACE::ensureScriptHasModuleImport(const std::string& moduleName,
                                                     std::string& script)
{
    /// import module
    script = "from " + moduleName + " import * \n" + script;

    return NATRON_PYTHON_NAMESPACE::findNewLineStartAfterImports(script);
}

#endif

namespace {
// The number of interpretPythonScript() calls running, in all threads. It is only read and written with
// the GIL held, which protects it.
// The captured sys.stdout and sys.stderr are shared by all threads, so this counter is process-wide, not
// per thread: a call made while another script is running (nested in it, or in another thread while the
// running script released the GIL, e.g. a render callback during app.render()) must not take the text
// that the running script wrote so far. Interleaving between threads is still only approximate: text
// written by one thread while another thread's script runs may be attributed to that script, but no
// text is lost, and it never makes a script fail.
int pythonScriptDepth = 0;

// The number of interpretPythonScript() calls running in this thread, for raiseSystemExitAtPrompt().
// It covers the whole call, not only PyRun_String(): Natron runs Python code after the script too
// (traceback.format_exception() in takePythonException()), where a pending call may fire, and a
// SystemExit raised there would be cleared with the exception of the script.
thread_local int pythonScriptDepthInThread = 0;

class PythonScriptDepth_RAII
{
    const bool _nested;

public:
    // The GIL must be held.
    PythonScriptDepth_RAII()
        : _nested(pythonScriptDepth > 0)
    {
        ++pythonScriptDepth;
        ++pythonScriptDepthInThread;
    }

    // The GIL must still be held.
    ~PythonScriptDepth_RAII()
    {
        --pythonScriptDepthInThread;
        --pythonScriptDepth;
    }

    // Whether another interpretPythonScript() call was running when this one started
    bool isNested() const
    {
        return _nested;
    }
};

// Converts a Python str to UTF-8. Characters that cannot be encoded (lone surrogates, e.g. from a
// non-UTF-8 file name decoded with surrogateescape) are written as backslash escapes, rather than
// making the whole conversion fail (PyStringToStdString() returns an empty string then).
// The GIL must be held.
std::string
pyUnicodeToUtf8(PyObject* str)
{
    std::string ret;
    PyObject* bytes = PyUnicode_AsEncodedString(str, "utf-8", "backslashreplace"); // new ref

    if (bytes) {
        ret.assign( PyBytes_AS_STRING(bytes), (std::size_t)PyBytes_GET_SIZE(bytes) );
        Py_DECREF(bytes);
    } else {
        PyErr_Clear();
    }

    return ret;
}

// In GUI sessions, sys.stdout and sys.stderr are the catchOut and catchErr StreamCatcher objects of the
// __main__ module (see AppManager::initPython()). Returns the text in the catcher named catcherName and
// empties it, or an empty string if there is no such catcher (background sessions).
// The GIL must be held.
std::string
takeCaughtStream(PyObject* mainModule,
                 const char* catcherName)
{
    std::string ret;

    if ( !PyObject_HasAttrString(mainModule, catcherName) ) {
        return ret;
    }
    PyObject* catcher = PyObject_GetAttrString(mainModule, catcherName); // new ref
    if (!catcher) {
        PyErr_Clear();

        return ret;
    }
    PyObject* value = PyObject_GetAttrString(catcher, "value"); // new ref
    if ( value && PyUnicode_Check(value) && (PyUnicode_GetLength(value) > 0) ) {
        ret = pyUnicodeToUtf8(value);
        {
            PyObject* empty = PyUnicode_FromString("");
            PyObject_SetAttrString(catcher, "value", empty);
            Py_DECREF(empty);
        }
    }
    Py_XDECREF(value);
    Py_DECREF(catcher);
    PyErr_Clear();

    return ret;
}

// Sets the text in the catcher named catcherName (see takeCaughtStream()). The GIL must be held.
void
setCaughtStream(PyObject* mainModule,
                const char* catcherName,
                const std::string& text)
{
    if ( !PyObject_HasAttrString(mainModule, catcherName) ) {
        return;
    }
    PyObject* catcher = PyObject_GetAttrString(mainModule, catcherName); // new ref
    if (!catcher) {
        PyErr_Clear();

        return;
    }
    PyObject* value = PyUnicode_FromStringAndSize( text.c_str(), (Py_ssize_t)text.size() ); // new ref
    if (value) {
        PyObject_SetAttrString(catcher, "value", value);
        Py_DECREF(value);
    }
    Py_DECREF(catcher);
    PyErr_Clear();
}

// Shows text that Python wrote to the captured sys.stdout or sys.stderr in the Script Editor of the
// top-level project window, or on the standard error if there is none yet. header, if not empty, is
// shown on the line before the text.
void
forwardToScriptEditor(std::string text,
                      const char* header = "")
{
    if ( text.empty() ) {
        return;
    }
    // appendToScriptEditor() adds a newline
    if (text[text.size() - 1] == '\n') {
        text.erase(text.size() - 1);
    }
    if (*header) {
        text = std::string(header) + '\n' + text;
    }
    AppInstancePtr app = appPTR ? appPTR->getTopLevelInstance() : AppInstancePtr();
    if (app) {
        app->appendPlainTextToScriptEditor(text);
    } else {
        std::cerr << text << std::endl;
    }
}

// Returns the message and traceback of the pending Python exception, and clears it. The GIL must be held.
std::string
takePythonException()
{
    std::string ret;
    PyObject *pyExcType;
    PyObject *pyExcValue;
    PyObject *pyExcTraceback;

    PyErr_Fetch(&pyExcType, &pyExcValue, &pyExcTraceback); // also clears the error indicator
    PyErr_NormalizeException(&pyExcType, &pyExcValue, &pyExcTraceback);

    PyObject* pyStr = pyExcValue ? PyObject_Str(pyExcValue) : 0;
    if (pyStr) {
        std::string str = PyUnicode_Check(pyStr) ? pyUnicodeToUtf8(pyStr) : std::string();
        if ( !str.empty() ) {
            ret += std::string("Python exception: ") + str + '\n';
        }
        Py_DECREF(pyStr);
    } else {
        // str() of the exception raised: still get the traceback
        PyErr_Clear();
    }

    // See if we can get a full traceback
    PyObject* module_name = PyUnicode_FromString("traceback");
    PyObject* pyth_module = PyImport_Import(module_name);
    Py_DECREF(module_name);

    if (pyth_module != NULL) {
        PyObject* pyth_func;
        if (!pyExcTraceback) {
            pyth_func = PyObject_GetAttrString(pyth_module, "format_exception_only");
        } else {
            pyth_func = PyObject_GetAttrString(pyth_module, "format_exception");
        }
        Py_DECREF(pyth_module);
        if ( pyth_func && PyCallable_Check(pyth_func) ) {
            PyObject *pyth_val = PyObject_CallFunctionObjArgs(pyth_func, pyExcType, pyExcValue, pyExcTraceback, NULL);
            if (pyth_val) {
                PyObject *emptyString = PyUnicode_FromString("");
                PyObject *strList = PyObject_CallMethod(emptyString, (char*)"join", (char*)"(O)", pyth_val);
                Py_DECREF(emptyString);
                Py_DECREF(pyth_val);
                if ( strList && PyUnicode_Check(strList) ) {
                    std::string str = pyUnicodeToUtf8(strList);
                    if ( !str.empty() ) {
                        ret += str + '\n';
                    }
                }
                Py_XDECREF(strList);
            }
        }
        Py_XDECREF(pyth_func);
    }
    PyErr_Clear();
    Py_XDECREF(pyExcType);
    Py_XDECREF(pyExcValue);
    Py_XDECREF(pyExcTraceback);
    PyErr_Clear(); // in case formatting the exception failed

    return ret;
}
} // anon namespace

// Raises SystemExit at the interactive prompt of the -t option, once no
// interpretPythonScript() is running in this thread, the main thread, where CPython runs the pending
// calls (see interpretPythonScript())
static int
raiseSystemExitAtPrompt(void*)
{
    if (pythonScriptDepthInThread > 0) {
        // still in a script run by Natron (e.g. a callback), or in the code that handles its result:
        // try again when it returns. This re-polls at every check of the interpreter until then.
        Py_AddPendingCall(raiseSystemExitAtPrompt, 0);

        return 0;
    }
    PyObject* code = PyLong_FromLong( appPTR->getRequestedExitCode() );
    PyErr_SetObject(PyExc_SystemExit, code);
    Py_XDECREF(code);

    return -1;
}

bool
NATRON_PYTHON_NAMESPACE::interpretPythonScript(const std::string& script,
                                               std::string* error,
                                               std::string* output,
                                               PyObject* globals,
                                               PyObject* locals)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return true;
#endif
    if ( !Py_IsInitialized() ) {
        // e.g. after Py_Main() returned in interpreter mode (#1253)
        if (error) {
            *error = "Python is not initialized or was already finalized";
        }

        return false;
    }
    PythonGILLocker pgl;
    // Re-entrant: the script may call the Natron API, which may run other scripts through this function
    // (e.g. app.createNode() declares the node variable).
    // It also delays a SystemExit raised for the -t prompt until this call has returned (see
    // raiseSystemExitAtPrompt()). It is destroyed before the GIL is released.
    PythonScriptDepth_RAII depth;
    PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();

    PyErr_Clear();

    // In GUI sessions, sys.stdout and sys.stderr are StreamCatcher objects (see AppManager::initPython()).
    // Text that is already there was not written by this script (#1436): if another call is running
    // (nested, or in another thread, see pythonScriptDepth), it is the output of that script so far,
    // which is put back when this script is done; otherwise, it
    // was written outside of this function (e.g. a traceback from a PySide slot) and it is forwarded
    // to the Script Editor, rather than lost or taken for an error of this script.
    const bool captured = PyObject_HasAttrString(mainModule, "catchErr");
    const std::string previousErr = takeCaughtStream(mainModule, "catchErr");
    const std::string previousOut = takeCaughtStream(mainModule, "catchOut");
    if ( !depth.isNested() ) {
        forwardToScriptEditor(previousOut + previousErr, "# Earlier output:");
    }

    int status = -1;
    PyObject* dict = globals ? globals : PyModule_GetDict(mainModule);

    ///This is faster than PyRun_SimpleString since is doesn't call PyImport_AddModule("__main__")
    PyObject* v = PyRun_String(script.c_str(), Py_file_input, dict, locals);
    if (v) {
        Py_DECREF(v);
        status = 0;
    }

    // The success of the script is given by the result of PyRun_String(), not by what it wrote to
    // sys.stderr (warnings, tracebacks printed and handled by the script...).
    std::string exceptionText;
    PyObject* ex = PyErr_Occurred();
    if ( ex && PyErr_GivenExceptionMatches(ex, PyExc_SystemExit) &&
         appPTR->isQuitRequested() && appPTR->isPythonInterpreterRunning() ) {
        // natron.quit() was called from a script run by Natron (e.g. a callback) while the
        // interactive prompt of the -t option runs: the SystemExit it raised stops here, so
        // raise it again at the prompt to end the session.
        Py_AddPendingCall(raiseSystemExitAtPrompt, 0);
    }
    if (ex) {
        assert(status < 0);
        exceptionText = takePythonException();
    }

    // What this script wrote to sys.stdout and sys.stderr
    std::string scriptErr = takeCaughtStream(mainModule, "catchErr");
    std::string scriptOut = takeCaughtStream(mainModule, "catchOut");

    if (output) {
        output->clear();
        if (status == 0) {
            // e.g. the Script Editor shows it with the result of the script, after the script
            *output = scriptOut + scriptErr;
            scriptOut.clear();
            scriptErr.clear();
        }
        // else most callers only show the error: the output of the failed script is forwarded
        // below, so that it is shown before the error
    }
    if (error) {
        error->clear();
        if (status != 0) {
            *error = exceptionText + scriptErr;
            scriptErr.clear();
            if (captured) {
                *error = "While executing script:\n" + script + "Python error:\n" + *error;
            }
        }
    } else if (status != 0) {
        // the caller does not want the error: don't lose the traceback
        scriptErr.insert(0, exceptionText);
    }

    // Text that the caller did not take (stderr of a successful script, output of a failed script or not
    // requested by the caller) goes where the script would have written it: to the calling script in a nested call, or
    // to the Script Editor.
    if ( captured && depth.isNested() ) {
        setCaughtStream(mainModule, "catchErr", previousErr + scriptErr);
        setCaughtStream(mainModule, "catchOut", previousOut + scriptOut);
    } else if (captured) {
        forwardToScriptEditor(scriptOut);
        forwardToScriptEditor(scriptErr);
    } else if ( !scriptErr.empty() ) {
        // background session: sys.stderr is not captured, only the exception was not printed yet
        std::cerr << scriptErr << std::flush;
    }

    return status == 0;
} // NATRON_PYTHON_NAMESPACE::interpretPythonScript

#if 0 // dead code
void
NATRON_PYTHON_NAMESPACE::compilePyScript(const std::string& script,
                                         PyObject** code)
{
    ///Must be locked
    assert( PyThreadState_Get() );

    *code = (PyObject*)Py_CompileString(script.c_str(), "<string>", Py_file_input);
    if (PyErr_Occurred() || !*code) {
#ifdef DEBUG
        PyErr_Print();
#endif
        throw std::runtime_error("failed to compile the script");
    }
}

#endif

static std::string
makeNameScriptFriendlyInternal(const std::string& str,
                               bool allowDots)
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
        if ( cpy.empty() && std::isdigit(str[i], loc) ) {
            cpy.push_back('p');
            cpy.push_back(str[i]);
            continue;
        }

        ///Spaces becomes underscores
        if ( std::isspace(str[i], loc) ) {
            cpy.push_back('_');
        }
        ///Non alpha-numeric characters are not allowed in python
        else if ( (str[i] == '_') || std::isalnum(str[i], loc) || ( allowDots && (str[i] == '.') ) ) {
            cpy.push_back(str[i]);
        }
    }

    return cpy;
}

std::string
NATRON_PYTHON_NAMESPACE::makeNameScriptFriendlyWithDots(const std::string& str)
{
    return makeNameScriptFriendlyInternal(str, true);
}

std::string
NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(const std::string& str)
{
    return makeNameScriptFriendlyInternal(str, false);
}

bool
NATRON_PYTHON_NAMESPACE::isKeyword(const std::string& str)
{
    const char* const kw[] = { "False", "None", "True", "and", "as", "assert",
        "async", "await", "break", "class", "continue", "def", "del", "elif",
        "else", "except", "finally", "for", "from", "global", "if", "import",
        "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
        "try", "while", "with", "yield", NULL };
    for (const char* const *k = kw; *k != NULL; ++k) {
        if (str == *k) {
            return true;
        }
    }
    return false;
}

#if !defined(NDEBUG) && !defined(DEBUG_PYTHON_GIL)
//#pragma message WARN("define DEBUG_PYTHON_GIL in AppManager.h to debug Python GIL issues")
#endif

#ifdef DEBUG_PYTHON_GIL
QMap<QString,int> PythonGILLocker::pythonCount;
#ifdef USE_NATRON_GIL
QMap<QString,int> PythonGILLocker::natronCount;
#endif
#endif

// Follow https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
PythonGILLocker::PythonGILLocker()
    : state(PyGILState_UNLOCKED)
    , locked(false)
{
#ifdef DEBUG_PYTHON_GIL
    if (!Py_IsInitialized()) {
        throw std::runtime_error("Trying to execute python code, but Py_IsInitialized() returns false");
    }
#endif
    if ( !Py_IsInitialized() ) {
        // Taking the GIL of a finalized interpreter crashes (#1253): lock nothing.
        return;
    }
    locked = true;

    // Also take the Python GIL, since not doing so seems to crash Natron during recursive Natron->Python->Natron->Python calls, see https://github.com/NatronGitHub/Natron/issues/379
    // Follow https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    // Take the GIL for this thread
#ifdef DEBUG_PYTHON_GIL
    QThread* curThread = QThread::currentThread();
    QString threadname = (qApp && qApp->thread() == curThread) ? QString::fromUtf8("Main") : curThread->objectName();
    qDebug() << QString::fromUtf8("Thread '%1' is asking the Python GIL").arg(threadname);
#endif
    state = PyGILState_Ensure();
#ifdef DEBUG_PYTHON_GIL
    ++pythonCount[threadname];
    qDebug() << QString::fromUtf8("Thread '%1' got the Python GIL (%2)").arg(threadname).arg(pythonCount[threadname]);
#endif

#ifdef USE_NATRON_GIL
    // Take the Natron GIL https://github.com/NatronGitHub/Natron/commit/46d9d616dfebfbb931a79776734e2fa17202f7cb
    // We do this after we got the Python GIL, to avoid deadlocks:
    // If we do it the other way, this thread may be waiting for the Python GIL while keeping the Natron GIL
    // locked, thus preventing other threads from getting the Python GIL.
    if (appPTR) {
#ifdef DEBUG_PYTHON_GIL
        qDebug() << QString::fromUtf8("Thread '%1' is asking the Natron GIL").arg(threadname);
#endif
        appPTR->takeNatronGIL();
#ifdef DEBUG_PYTHON_GIL
        ++natronCount[threadname];
        qDebug() << QString::fromUtf8("Thread '%1' got the Natron GIL (%2)").arg(threadname).arg(natronCount[threadname]);
#endif
    }
#endif

    assert(PyThreadState_Get());
#if PY_VERSION_HEX >= 0x030400F0
    assert(PyGILState_Check()); // Not available prior to Python 3.4
#endif
}

PythonGILLocker::~PythonGILLocker()
{
    if (!locked) {
        return;
    }
    // Release the Natron GIL https://github.com/NatronGitHub/Natron/commit/46d9d616dfebfbb931a79776734e2fa17202f7cb
#ifdef DEBUG_PYTHON_GIL
    QThread* curThread = QThread::currentThread();
    QString threadname = (qApp && qApp->thread() == curThread) ? QString::fromUtf8("Main") : curThread->objectName();
#endif
#ifdef USE_NATRON_GIL
    if (appPTR) {
#ifdef DEBUG_PYTHON_GIL
        qDebug() << QString::fromUtf8("Thread '%1' is releasing the Natron GIL (%2)").arg(threadname).arg(natronCount[threadname]);
        --natronCount[threadname];
#endif
        appPTR->releaseNatronGIL();
    }
#endif

    // We took the Python GIL too, so release it here.
    // Follow https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
#if PY_VERSION_HEX >= 0x030400F0
    assert(PyGILState_Check());  // Not available prior to Python 3.4
#endif
    // Release the GIL, no thread will own it afterwards.
#ifdef DEBUG_PYTHON_GIL
    qDebug() << QString::fromUtf8("Thread '%1' is releasing the Python GIL (%2)").arg(threadname).arg(pythonCount[threadname]);
    --pythonCount[threadname];
#endif
    PyGILState_Release(state);
}

PythonGILUnlocker::PythonGILUnlocker()
{
#ifdef DEBUG_PYTHON_GIL
    if (!Py_IsInitialized()) {
        throw std::runtime_error("Trying to execute python code, but Py_IsInitialized() returns false");
    }
#endif
    assert(Py_IsInitialized());

    // Also take the Python GIL, since not doing so seems to crash Natron during recursive Natron->Python->Natron->Python calls, see https://github.com/NatronGitHub/Natron/issues/379
    // Follow https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    // Take the GIL for this thread
#ifdef DEBUG_PYTHON_GIL
    QThread* curThread = QThread::currentThread();
    QString threadname = (qApp && qApp->thread() == curThread) ? QString::fromUtf8("Main") : curThread->objectName();
    qDebug() << QString::fromUtf8("Thread '%1' is releasing the Python GIL").arg(threadname);
#endif
    assert(PyThreadState_Get());
#if PY_VERSION_HEX >= 0x030400F0
    assert(PyGILState_Check()); // Not available prior to Python 3.4
#endif
    PyGILState_Release(PyGILState_UNLOCKED);
#ifdef DEBUG_PYTHON_GIL
    qDebug() << QString::fromUtf8("Thread '%1' released the Python GIL").arg(threadname);
#endif

#ifdef USE_NATRON_GIL
    // Take the Natron GIL https://github.com/NatronGitHub/Natron/commit/46d9d616dfebfbb931a79776734e2fa17202f7cb
    // We do this after we got the Python GIL, to avoid deadlocks:
    // If we do it the other way, this thread may be waiting for the Python GIL while keeping the Natron GIL
    // locked, thus preventing other threads from getting the Python GIL.
    if (appPTR) {
#ifdef DEBUG_PYTHON_GIL
        qDebug() << QString::fromUtf8("Thread '%1' is releasing the Natron GIL").arg(threadname);
#endif
        appPTR->releaseNatronGIL();
#ifdef DEBUG_PYTHON_GIL
        qDebug() << QString::fromUtf8("Thread '%1' released the Natron GIL").arg(threadname);
#endif
    }
#endif
}

PythonGILUnlocker::~PythonGILUnlocker()
{
    // Take the Natron GIL https://github.com/NatronGitHub/Natron/commit/46d9d616dfebfbb931a79776734e2fa17202f7cb
#ifdef DEBUG_PYTHON_GIL
    QThread* curThread = QThread::currentThread();
    QString threadname = (qApp && qApp->thread() == curThread) ? QString::fromUtf8("Main") : curThread->objectName();
#endif
#ifdef USE_NATRON_GIL
    if (appPTR) {
#ifdef DEBUG_PYTHON_GIL
        qDebug() << QString::fromUtf8("Thread '%1' is asking the Natron GIL").arg(threadname);
#endif
        appPTR->takeNatronGIL();
    }
#endif

    // We released the Python GIL too, so take it here.
    // Follow https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
#if PY_VERSION_HEX >= 0x030400F0
    assert(!PyGILState_Check());  // Not available prior to Python 3.4
#endif
    // Release the GIL, no thread will own it afterwards.
#ifdef DEBUG_PYTHON_GIL
    qDebug() << QString::fromUtf8("Thread '%1' is asking the Python GIL").arg(threadname);
#endif
    /* PyGILState_STATE state = */
    PyGILState_Ensure();
#ifdef DEBUG_PYTHON_GIL
    qDebug() << QString::fromUtf8("Thread '%1' got the Python GIL").arg(threadname);
#endif

    assert(PyThreadState_Get());
#if PY_VERSION_HEX >= 0x030400F0
    assert(PyGILState_Check()); // Not available prior to Python 3.4
#endif
}

static bool
getGroupInfosInternal(const std::string& modulePath,
                      const std::string& pythonModule,
                      std::string* pluginID,
                      std::string* pluginLabel,
                      std::string* iconFilePath,
                      std::string* grouping,
                      std::string* description,
                      bool* isToolset,
                      unsigned int* version)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return false;
#endif
    PythonGILLocker pgl;
    static const QString script = QString::fromUtf8("import sys\n"
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
                                                    "isToolset = False\n"
                                                    "if hasattr(%1,\"getVersion\") and hasattr(%1.getVersion,\"__call__\"):\n"
                                                    "    version = %1.getVersion()\n"
                                                    "if hasattr(%1,\"getIsToolset\") and hasattr(%1.getIsToolset,\"__call__\"):\n"
                                                    "    isToolset = %1.getIsToolset()\n"
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
    std::string toRun = script.arg( QString::fromUtf8( pythonModule.c_str() ) ).toStdString();
    std::string err;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(toRun, &err, 0) ) {
        QString logStr = QCoreApplication::translate("AppManager", "Was not recognized as a PyPlug: %1").arg( QString::fromUtf8( err.c_str() ) );
        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(pythonModule.c_str()), QDateTime::currentDateTime(), logStr);

        return false;
    }

    PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
    PyObject* retObj = PyObject_GetAttrString(mainModule, "ret"); //new ref
    assert(retObj);
    if (PyObject_IsTrue(retObj) == 0) {
        Py_XDECREF(retObj);

        return false;
    }
    Py_XDECREF(retObj);

    std::string deleteScript("del ret\n"
                             "del templateLabel\n");
    PyObject* labelObj = 0;
    labelObj = PyObject_GetAttrString(mainModule, "templateLabel"); //new ref

    PyObject* idObj = 0;
    idObj = PyObject_GetAttrString(mainModule, "pluginID"); //new ref

    PyObject* iconObj = 0;
    if ( PyObject_HasAttrString(mainModule, "templateIcon") ) {
        iconObj = PyObject_GetAttrString(mainModule, "templateIcon"); //new ref
    }
    PyObject* iconGrouping = 0;
    if ( PyObject_HasAttrString(mainModule, "templateGrouping") ) {
        iconGrouping = PyObject_GetAttrString(mainModule, "templateGrouping"); //new ref
    }

    PyObject* versionObj = 0;
    if ( PyObject_HasAttrString(mainModule, "version") ) {
        versionObj = PyObject_GetAttrString(mainModule, "version"); //new ref
    }

    PyObject* isToolsetObj = 0;
    if ( PyObject_HasAttrString(mainModule, "isToolset") ) {
        isToolsetObj = PyObject_GetAttrString(mainModule, "isToolset"); //new ref
    }

    PyObject* pluginDescriptionObj = 0;
    if ( PyObject_HasAttrString(mainModule, "description") ) {
        pluginDescriptionObj = PyObject_GetAttrString(mainModule, "description"); //new ref
    }

    assert(labelObj);


    *pluginLabel = NATRON_PYTHON_NAMESPACE::PyStringToStdString(labelObj);
    Py_XDECREF(labelObj);

    if (idObj) {
        *pluginID = NATRON_PYTHON_NAMESPACE::PyStringToStdString(idObj);
        deleteScript.append("del pluginID\n");
        Py_XDECREF(idObj);
    }

    if (iconObj) {
        *iconFilePath = NATRON_PYTHON_NAMESPACE::PyStringToStdString(iconObj);
        QFileInfo iconInfo( QString::fromUtf8( modulePath.c_str() ) + QString::fromUtf8( iconFilePath->c_str() ) );
        *iconFilePath =  iconInfo.canonicalFilePath().toStdString();

        deleteScript.append("del templateIcon\n");
        Py_XDECREF(iconObj);
    }
    if (iconGrouping) {
        *grouping = NATRON_PYTHON_NAMESPACE::PyStringToStdString(iconGrouping);
        deleteScript.append("del templateGrouping\n");
        Py_XDECREF(iconGrouping);
    }

    if (versionObj) {
        *version = (unsigned int)PyLong_AsLong(versionObj);
        deleteScript.append("del version\n");
        Py_XDECREF(versionObj);
    }

    if ( isToolsetObj && PyBool_Check(isToolsetObj) ) {
        *isToolset = (isToolsetObj == Py_True) ? true : false;
        deleteScript.append("del isToolset\n");
        Py_XDECREF(isToolsetObj);
    }


    if (pluginDescriptionObj) {
        *description = NATRON_PYTHON_NAMESPACE::PyStringToStdString(pluginDescriptionObj);
        deleteScript.append("del description\n");
        Py_XDECREF(pluginDescriptionObj);
    }

    if ( grouping->empty() ) {
        *grouping = PLUGIN_GROUP_OTHER;
    }


    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(deleteScript, &err, NULL);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("getGroupInfos(): interpretPythonScript(" + deleteScript + " failed!");
    }

    return true;
} // getGroupInfosInternal

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
                                bool* isToolset,
                                unsigned int* version)
{
    ///Must be locked
    assert( PyThreadState_Get() );

    QString qModulePath = QString::fromUtf8( resourceFileName.c_str() );

    assert( qModulePath.startsWith( QString::fromUtf8(":/Resources") ) );

    QFile moduleContent(qModulePath);
    if ( !moduleContent.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        return false;
    }

    QByteArray utf8bytes = QString::fromUtf8( pythonModule.c_str() ).toUtf8();
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
    return getGroupInfosInternal(modulePath, pythonModule, pluginID, pluginLabel, iconFilePath, grouping, description, isToolset, version);
    //PyDict_SetItemString(priv->globals, moduleName, module);
}

bool
NATRON_PYTHON_NAMESPACE::getGroupInfos(const std::string& modulePath,
                                       const std::string& pythonModule,
                                       std::string* pluginID,
                                       std::string* pluginLabel,
                                       std::string* iconFilePath,
                                       std::string* grouping,
                                       std::string* description,
                                       bool* isToolset,
                                       unsigned int* version)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return false;
#endif

    {
        std::string tofind(":/Resources");
        if (modulePath.substr( 0, tofind.size() ) == tofind) {
            std::string resourceFileName = modulePath + pythonModule + ".py";

            return getGroupInfosFromQtResourceFile(resourceFileName, modulePath, pythonModule, pluginID, pluginLabel, iconFilePath, grouping, description, isToolset, version);
        }
    }

    return getGroupInfosInternal(modulePath, pythonModule, pluginID, pluginLabel, iconFilePath, grouping, description, isToolset, version);
}

void
NATRON_PYTHON_NAMESPACE::getFunctionArguments(const std::string& pyFunc,
                                              std::string* error,
                                              std::vector<std::string>* args)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if ( !Py_IsInitialized() ) {
        // e.g. after Py_Main() returned in interpreter mode (#1253)
        if (error) {
            *error = "Python is not initialized or was already finalized";
        }

        return;
    }
    std::stringstream ss;
    ss << "import inspect\n";
    ss << "args_spec = inspect.getfullargspec(" << pyFunc << ")\n";
    std::string script = ss.str();
    // no output: what the script writes is forwarded rather than dropped
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, error, 0);
    if (!ok) {
        throw std::runtime_error("NATRON_PYTHON_NAMESPACE::getFunctionArguments(): interpretPythonScript(" + script + " failed!");
    }
    PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
    PyObject* args_specObj = 0;
    PythonGILLocker pgl;

    if ( PyObject_HasAttrString(mainModule, "args_spec") ) {
        args_specObj = PyObject_GetAttrString(mainModule, "args_spec");
    }
    assert(args_specObj);
    PyObject* argListObj = 0;

    if (args_specObj) {
        argListObj = PyTuple_GetItem(args_specObj, 0);
        assert(argListObj);
        if (argListObj) {
            // size = PyObject_Size(argListObj)
            assert( PyList_Check(argListObj) );
            Py_ssize_t size = PyList_Size(argListObj);
            for (Py_ssize_t i = 0; i < size; ++i) {
                PyObject* itemObj = PyList_GetItem(argListObj, i);
                assert(itemObj);
                if (itemObj) {
                    std::string itemName = PyStringToStdString(itemObj);
                    assert( !itemName.empty() );
                    if ( !itemName.empty() ) {
                        args->push_back(itemName);
                    }
                }
            }
            if ( (PyTuple_GetItem(args_specObj, 1) != Py_None) || (PyTuple_GetItem(args_specObj, 2) != Py_None) ) {
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
NATRON_PYTHON_NAMESPACE::getAttrRecursive(const std::string& fullyQualifiedName,
                                          PyObject* parentObj,
                                          bool* isDefined)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return 0;
#endif
    std::size_t foundDot = fullyQualifiedName.find(".");
    std::string attrName = foundDot == std::string::npos ? fullyQualifiedName : fullyQualifiedName.substr(0, foundDot);
    PyObject* obj = 0;
    {
        PythonGILLocker pgl;
        if ( PyObject_HasAttrString( parentObj, attrName.c_str() ) ) {
            obj = PyObject_GetAttrString( parentObj, attrName.c_str() );
        }
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
        if ( !recurseName.empty() ) {
            return NATRON_PYTHON_NAMESPACE::getAttrRecursive(recurseName, obj, isDefined);
        } else {
            *isDefined = true;

            return obj;
        }
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AppManager.cpp"
