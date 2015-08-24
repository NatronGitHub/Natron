//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "AppManager.h"

#if defined(Q_OS_UNIX)
#include <sys/time.h>     // for getrlimit on linux
#include <sys/resource.h> // for getrlimit
#endif

#include <clocale>
#include <cstddef>
#include <stdexcept>

#include <QDebug>
#include <QTextCodec>
#include <QProcess>
#include <QAbstractSocket>
#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>
#include <QTemporaryFile>
#include <QThreadPool>
#include <QtCore/QAtomicInt>

#ifdef NATRON_USE_BREAKPAD
#if defined(Q_OS_MAC)
#include "client/mac/handler/exception_handler.h"
#elif defined(Q_OS_LINUX)
#include "client/linux/handler/exception_handler.h"
#elif defined(Q_OS_WIN32)
#include "client/windows/handler/exception_handler.h"
#endif
#endif

#include "Global/MemoryInfo.h"
#include "Global/QtCompat.h" // for removeRecursively
#include "Global/GlobalDefines.h" // for removeRecursively
#include "Global/Enums.h"
#include "Global/GitVersion.h"


#include "Engine/AppInstance.h"
#include "Engine/BackDrop.h"
#include "Engine/Cache.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Dot.h"
#include "Engine/Format.h"
#include "Engine/FrameEntry.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/Image.h"
#include "Engine/Knob.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Log.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Project.h"
#include "Engine/RectISerialization.h"
#include "Engine/RectDSerialization.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoSmear.h"
#include "Engine/Settings.h"
#include "Engine/StandardPaths.h"
#include "Engine/Transform.h"
#include "Engine/Variant.h"
#include "Engine/ViewerInstance.h"


BOOST_CLASS_EXPORT(Natron::FrameParams)
BOOST_CLASS_EXPORT(Natron::ImageParams)

#define NATRON_CACHE_VERSION 2


using namespace Natron;

AppManager* AppManager::_instance = 0;



struct AppManagerPrivate
{
    AppManager::AppTypeEnum _appType; //< the type of app
    std::map<int,AppInstanceRef> _appInstances; //< the instances mapped against their ID
    int _availableID; //< the ID for the next instance
    int _topLevelInstanceID; //< the top level app ID
    boost::shared_ptr<Settings> _settings; //< app settings
    std::vector<Format*> _formats; //<a list of the "base" formats available in the application
    PluginsMap _plugins; //< list of the plugins
    boost::scoped_ptr<Natron::OfxHost> ofxHost; //< OpenFX host
    boost::scoped_ptr<KnobFactory> _knobFactory; //< knob maker
    boost::shared_ptr<Natron::Cache<Natron::Image> >  _nodeCache; //< Images cache
    boost::shared_ptr<Natron::Cache<Natron::Image> >  _diskCache; //< Images disk cache (used by DiskCache nodes)
    boost::shared_ptr<Natron::Cache<Natron::FrameEntry> > _viewerCache; //< Viewer textures cache
    
    mutable QMutex diskCachesLocationMutex;
    QString diskCachesLocation;
    
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
    // - This only takes into account the current Natron process and disregard completly CPU activity.
    // - We might count a thread that is actually waiting in a mutex as a running thread
    // Another method could be to analyse all cores running, but this is way more expensive and would impair performances.
    QAtomicInt runningThreadsCount;
    
     //To by-pass a bug introduced in RC2 / RC3 with the serialization of bezier curves
    bool lastProjectLoadedCreatedDuringRC2Or3;
    
    ///Python needs wide strings as from Python 3.x onwards everything is unicode based
#ifndef IS_PYTHON_2
    std::vector<wchar_t*> args;
#else
    std::vector<char*> args;
#endif
    
    PyObject* mainModule;
    PyThreadState* mainThreadState;
    
#ifdef NATRON_USE_BREAKPAD
    boost::shared_ptr<google_breakpad::ExceptionHandler> breakpadHandler;
    boost::shared_ptr<QProcess> crashReporter;
    QString crashReporterBreakpadPipe;
    boost::shared_ptr<QLocalServer> crashClientServer;
    QLocalSocket* crashServerConnection;
#endif
    
    QMutex natronPythonGIL;

#ifdef Q_OS_WIN32
	//On Windows only, track the UNC path we came across because the WIN32 API does not provide any function to map
	//from UNC path to path with drive letter.
	std::map<QChar,QString> uncPathMapping;
#endif
    
    AppManagerPrivate();
    
    ~AppManagerPrivate()
    {
        for (U32 i = 0; i < args.size() ; ++i) {
            free(args[i]);
        }
        args.clear();
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
    
    Natron::Plugin* findPluginById(const QString& oldId,int major, int minor) const;
    
    void declareSettingsToPython();
    
#ifdef NATRON_USE_BREAKPAD
    void initBreakpad();
#endif
};

#ifdef DEBUG
void crash_application()
{
#ifdef __NATRON_UNIX__
    sleep(2);
#endif
    volatile int* a = (int*)(NULL);
    // coverity[var_deref_op]
    *a = 1;
}
#endif


AppManagerPrivate::AppManagerPrivate()
: _appType(AppManager::eAppTypeBackground)
, _appInstances()
, _availableID(0)
, _topLevelInstanceID(0)
, _settings( new Settings(NULL) )
, _formats()
, _plugins()
, ofxHost( new Natron::OfxHost() )
, _knobFactory( new KnobFactory() )
, _nodeCache()
, _diskCache()
, _viewerCache()
, diskCachesLocationMutex()
, diskCachesLocation()
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
,idealThreadCount(0)
,nThreadsToRender(0)
,nThreadsPerEffect(0)
,useThreadPool(true)
,nThreadsMutex()
,runningThreadsCount()
,lastProjectLoadedCreatedDuringRC2Or3(false)
,args()
,mainModule(0)
,mainThreadState(0)
#ifdef NATRON_USE_BREAKPAD
,breakpadHandler()
,crashReporter()
,crashReporterBreakpadPipe()
,crashClientServer()
,crashServerConnection(0)
#endif
,natronPythonGIL(QMutex::Recursive)
{
    setMaxCacheFiles();
    
    runningThreadsCount = 0;
}



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



static bool tryParseFrameRange(const QString& arg,std::pair<int,int>& range)
{
    bool frameRangeFound = true;
    
    QStringList strRange = arg.split('-');
    if (strRange.size() != 2) {
        frameRangeFound = false;
    }
    for (int i = 0; i < strRange.size(); ++i) {
        strRange[i] = strRange[i].trimmed();
    }
    if (frameRangeFound) {
        bool ok;
        range.first = strRange[0].toInt(&ok);
        if (!ok) {
            frameRangeFound = false;
        }
        
        if (frameRangeFound) {
            range.second = strRange[1].toInt(&ok);
            if (!ok) {
                frameRangeFound = false;
            }
        }
    }
    
    return frameRangeFound;
}


AppManager::AppManager()
    : QObject()
    , _imp( new AppManagerPrivate() )
{
    assert(!_instance);
    _instance = this;
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

struct CLArgsPrivate
{
    QStringList args;
    
    QString filename;
    
    bool isPythonScript;
    
    QString defaultOnProjectLoadedScript;
    
    std::list<CLArgs::WriterArg> writers;
    
    bool isBackground;
    
    QString ipcPipe;
    
    int error;
    
    bool isInterpreterMode;
    
    std::pair<int,int> range;
    bool rangeSet;
    
    bool isEmpty;
    
    CLArgsPrivate()
    : args()
    , filename()
    , isPythonScript(false)
    , defaultOnProjectLoadedScript()
    , writers()
    , isBackground(false)
    , ipcPipe()
    , error(0)
    , isInterpreterMode(false)
    , range()
    , rangeSet(false)
    , isEmpty(true)
    {
        
    }
 
    
    void parse();
    
    QStringList::iterator hasToken(const QString& longName,const QString& shortName);
    
    QStringList::iterator hasOutputToken(QString& indexStr);
    
    QStringList::iterator findFileNameWithExtension(const QString& extension);
    
};

CLArgs::CLArgs()
: _imp(new CLArgsPrivate())
{
    
}

CLArgs::CLArgs(int& argc,char* argv[],bool forceBackground)
: _imp(new CLArgsPrivate())
{    
    _imp->isEmpty = false;
    if (forceBackground) {
        _imp->isBackground = true;
    }
    for (int i = 0; i < argc; ++i) {
        QString str = argv[i];
        if (str.size() >= 2 && str[0] == '"' && str[str.size() - 1] == '"') {
            str.remove(0, 1);
            str.remove(str.size() - 1, 1);
        }
#ifdef DEBUG
        std::cout << "argv[" << i << "] = " << str.toStdString() << std::endl;
#endif
        _imp->args.push_back(str);
    }
    
    _imp->parse();
}

CLArgs::CLArgs(const QStringList &arguments, bool forceBackground)
    : _imp(new CLArgsPrivate())
{
    _imp->isEmpty = false;
    if (forceBackground) {
        _imp->isBackground = true;
    }
    for (int i = 0; i < arguments.size(); ++i) {
        QString str = arguments[i];
        if (str.size() >= 2 && str[0] == '"' && str[str.size() - 1] == '"') {
            str.remove(0, 1);
            str.remove(str.size() - 1, 1);
        }
        _imp->args.push_back(str);
    }
    _imp->parse();
}

// GCC 4.2 requires the copy constructor
CLArgs::CLArgs(const CLArgs& other)
: _imp(new CLArgsPrivate())
{
    *this = other;
}

CLArgs::~CLArgs()
{
    
}

void
CLArgs::operator=(const CLArgs& other)
{
    _imp->args = other._imp->args;
    _imp->filename = other._imp->filename;
    _imp->isPythonScript = other._imp->isPythonScript;
    _imp->defaultOnProjectLoadedScript = other._imp->defaultOnProjectLoadedScript;
    _imp->writers = other._imp->writers;
    _imp->isBackground = other._imp->isBackground;
    _imp->ipcPipe = other._imp->ipcPipe;
    _imp->error = other._imp->error;
    _imp->isInterpreterMode = other._imp->isInterpreterMode;
    _imp->range = other._imp->range;
    _imp->rangeSet = other._imp->rangeSet;
    _imp->isEmpty = other._imp->isEmpty;
}

bool
CLArgs::isEmpty() const
{
    return _imp->isEmpty;
}

void
CLArgs::printBackGroundWelcomeMessage()
{
    QString msg = QObject::tr("%1 Version %2\n"
                             "Copyright (C) 2015 the %1 developers\n"
                              ">>>Use the --help or -h option to print usage.<<<").arg(NATRON_APPLICATION_NAME).arg(NATRON_VERSION_STRING);
    std::cout << msg.toStdString() << std::endl;
}

void
CLArgs::printUsage(const std::string& programName)
{
    QString msg = QObject::tr(/* Text must hold in 80 columns ************************************************/
                              "%3 usage:\n"
                              "Three distinct execution modes exist in background mode:\n"
                              "- The execution of %1 projects (.%2)\n"
                              "- The execution of Python scripts that contain commands for %1\n"
                              "- An interpreter mode where commands can be given directly to the Python\n"
                              "  interpreter\n"
                              "\n"
                              "General options:\n"
                              "  -h [ --help ] :\n"
                              "    Produce help message.\n"
                              "  -v [ --version ]  :\n"
                              "    Print informations about %1 version.\n"
                              "  -b [ --background ] :\n"
                              "    Enable background rendering mode. No graphical interface is shown.\n"
                              "    When using %1Renderer or the -t option, this argument is implicit\n"
                              "    and does not have to be given.\n"
                              "    If using %1 and this option is not specified, the project is loaded\n"
                              "    as if opened from the file menu.\n"
                              "  -t [ --interpreter ] <python script file path> :\n"
                              "    Enable Python interpreter mode.\n"
                              "    Python commands can be given to the interpreter and executed on the fly.\n"
                              "    An optional Python script filename can be specified to source a script\n"
                              "    before the interpreter is made accessible.\n"
                              "    Note that %1 will not start rendering any Write node of the sourced\n"
                              "    script: it must be started explicitely.\n"
                              "    %1Renderer and %1 do the same thing in this mode, only the\n"
                              "    init.py script is loaded.\n"
                              "\n"
                              /* Text must hold in 80 columns ************************************************/
                              "Options for the execution of %1 projects:\n"
                              "  %3 <project file path> [options]\n"
                              "  -w [ --writer ] <Writer node script name> [<filename>] [<frameRange>] :\n"
                              "    Specify a Write node to render.\n"
                              "    When in background mode, the renderer only renders the node script name\n"
                              "    following this argument. If no such node exists in the project file, the\n"
                              "    process aborts.\n"
                              "    Note that if there is no --writer option, it renders all the writers in\n"
                              "    the project.\n"
                              "    After the writer node script name you can pass an optional output\n"
                              "    filename and pass an optional frame range in the format:\n"
                              "      <firstFrame>-<lastFrame> (e.g: 10-40).\n"
                              "    Note that several -w options can be set to specify multiple Write nodes\n"
                              "    to render.\n"
                              "    Note that if specified, the frame range is the same for all Write nodes\n"
                              "    to render.\n"
                              "  -l [ --onload ] <python script file path> :\n"
                              "    Specify a Python script to be executed after a project is created or\n"
                              "    loaded.\n"
                              "    Note that this is executed in GUI mode or with NatronRenderer, after\n"
                              "    executing the callbacks onProjectLoaded and onProjectCreated.\n"
                              "    The rules on the execution of Python scripts (see below) also apply to\n"
                              "    this script.\n"
                              "Sample uses:\n"
                              "  %1 /Users/Me/MyNatronProjects/MyProject.ntp\n"
                              "  %1 -b -w MyWriter /Users/Me/MyNatronProjects/MyProject.ntp\n"
                              "  %1Renderer -w MyWriter /Users/Me/MyNatronProjects/MyProject.ntp\n"
                              "  %1Renderer -w MyWriter /FastDisk/Pictures/sequence'###'.exr 1-100 /Users/Me/MyNatronProjects/MyProject.ntp\n"
                              "  %1Renderer -w MyWriter -w MySecondWriter 1-10 /Users/Me/MyNatronProjects/MyProject.ntp\n"
                              "  %1Renderer -w MyWriter 1-10 -l /Users/Me/Scripts/onProjectLoaded.py /Users/Me/MyNatronProjects/MyProject.ntp\n"
                              "\n"
                              /* Text must hold in 80 columns ************************************************/
                              "Options for the execution of Python scripts:\n"
                              "  %3 <Python script path> [options]\n"
                              "  [Note that the following does not apply if the -t option was given.]\n"
                              "  The script argument can either be the script of a Group that was exported\n"
                              "  from the graphical user interface, or an exported project, or a script\n"
                              "  written by hand.\n"
                              "  When executing a script, %1 first looks for a function with the\n"
                              "  following signature:\n"
                              "    def createInstance(app,group):\n"
                              "  If this function is found, it is executed, otherwise the whole content of\n"
                              "  the script is interpreted as though it were given to Python natively.\n"
                              "  In either case, the \"app\" variable is always defined and points to the\n"
                              "  correct application instance.\n"
                              "  Note that the GUI version of the program (%1) sources the script before\n"
                              "  creating the graphical user interface and does not start rendering.\n"
                              "  If in background mode, the nodes to render have to be given using the -w\n"
                              "  option (as described above) or with the following option:\n"
                              "  -o [ --output ] <filename> <frameRange> :\n"
                              "    Specify an Output node in the script that should be replaced with a\n"
                              "    Write node.\n"
                              "    The option looks for a node named Output1 in the script and replaces it\n"
                              "    by a Write node (like when creating a Write node in interactive GUI mode).\n"
                              "    <filename> is a pattern for the output file names.\n"
                              "    <frameRange> must be specified if it was not specified earlier on the\n"
                              "    command line.\n"
                              "    This option can also be used to render out multiple Output nodes, in\n"
                              "    which case it has to be used like this:\n"
                              "    -o1 [ --output1 ] : look for a node named Output1.\n"
                              "    -o2 [ --output2 ] : look for a node named Output2 \n"
                              "    etc...\n"
                              "Sample uses:\n"
                              "  %1 /Users/Me/MyNatronScripts/MyScript.py\n"
                              "  %1 -b -w MyWriter /Users/Me/MyNatronScripts/MyScript.py\n"
                              "  %1Renderer -w MyWriter /Users/Me/MyNatronScripts/MyScript.py\n"
                              "  %1Renderer -o /FastDisk/Pictures/sequence'###'.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py\n"
                              "  %1Renderer -o1 /FastDisk/Pictures/sequence'###'.exr -o2 /FastDisk/Pictures/test'###'.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py\n"
                              "  %1Renderer -w MyWriter -o /FastDisk/Pictures/sequence'###'.exr 1-100 /Users/Me/MyNatronScripts/MyScript.py\n"
                              "\n"
                              /* Text must hold in 80 columns ************************************************/
                              "Options for the execution of the interpreter mode:\n"
                              "  %3 -t [<Python script path>]\n"
                              "  %1 sources the optional script given as argument, if any, and then reads\n"
                              "  Python commands from the standard input, which are interpreted by Python.\n"
                              "Sample uses:\n"
                              "  %1 -t\n"
                              "  %1Renderer -t\n"
                              "  %1Renderer -t /Users/Me/MyNatronScripts/MyScript.py\n")
    .arg(/*%1=*/NATRON_APPLICATION_NAME).arg(/*%2=*/NATRON_PROJECT_FILE_EXT).arg(/*%3=*/programName.c_str());
    std::cout << msg.toStdString() << std::endl;
}


int
CLArgs::getError() const
{
    return _imp->error;
}

const std::list<CLArgs::WriterArg>&
CLArgs::getWriterArgs() const
{
    return _imp->writers;
}

bool
CLArgs::hasFrameRange() const
{
    return _imp->rangeSet;
}

const std::pair<int,int>&
CLArgs::getFrameRange() const
{
    return _imp->range;
}

bool
CLArgs::isBackgroundMode() const
{
    return _imp->isBackground;
}

bool
CLArgs::isInterpreterMode() const
{
    return _imp->isInterpreterMode;
}

const QString&
CLArgs::getFilename() const
{
    return _imp->filename;
}

const QString&
CLArgs::getDefaultOnProjectLoadedScript() const
{
    return _imp->defaultOnProjectLoadedScript;
}

const QString&
CLArgs::getIPCPipeName() const
{
    return _imp->ipcPipe;
}

bool
CLArgs::isPythonScript() const
{
    return _imp->isPythonScript;
}

QStringList::iterator
CLArgsPrivate::findFileNameWithExtension(const QString& extension)
{
    bool isPython = extension == "py";
    for (QStringList::iterator it = args.begin(); it != args.end() ; ++it) {
        if (isPython) {
            //Check that we do not take the python script specified for the --onload argument as the file to execute
            if (it == args.begin()) {
                continue;
            }
            QStringList::iterator prev = it;
            --prev;
            if (*prev == "--onload" || *prev == "-l") {
                continue;
            }
        }
        if (it->endsWith("." + extension)) {
            return it;
        }
    }
    return args.end();
}

QStringList::iterator
CLArgsPrivate::hasToken(const QString& longName,const QString& shortName)
{
    QString longToken = "--" + longName;
    QString shortToken =  !shortName.isEmpty() ? "-" + shortName : QString();
    for (QStringList::iterator it = args.begin(); it != args.end() ; ++it) {
        if (*it == longToken || (!shortToken.isEmpty() && *it == shortToken)) {
            return it;
        }
    }
    return args.end();
}


QStringList::iterator
CLArgsPrivate::hasOutputToken(QString& indexStr)
{
    QString outputLong("--output");
    QString outputShort("-o");
    
    for (QStringList::iterator it = args.begin(); it != args.end(); ++it) {
        int indexOf = it->indexOf(outputLong);
        if (indexOf != -1) {
            indexOf += outputLong.size();
            if (indexOf < it->size()) {
                indexStr = it->mid(indexOf);
                bool ok;
                indexStr.toInt(&ok);
                if (!ok) {
                    error = 1;
                    std::cout << QObject::tr("Wrong formating for the -o option").toStdString() << std::endl;
                    return args.end();
                }
            } else {
                indexStr = "1";
            }
            return it;
        } else {
            indexOf = it->indexOf(outputShort);
            if (indexOf != -1) {
                if (it->size() > 2 && !it->at(2).isDigit()) {
                    //This is probably the --onload option
                    return args.end();
                }
                indexOf += outputShort.size();
                if (indexOf < it->size()) {
                    indexStr = it->mid(indexOf);
                    bool ok;
                    indexStr.toInt(&ok);
                    if (!ok) {
                        error = 1;
                        std::cout << QObject::tr("Wrong formating for the -o option").toStdString() << std::endl;
                        return args.end();
                    }
                } else {
                    indexStr = "1";
                }
                return it;
            }
        }
    }
    return args.end();
}

void
CLArgsPrivate::parse()
{
    {
        QStringList::iterator it = hasToken("version", "v");
        if (it != args.end()) {
            QString msg = QObject::tr("%1 version %2 at commit %3 on branch %4 built on %4").arg(NATRON_APPLICATION_NAME).arg(NATRON_VERSION_STRING).arg(GIT_COMMIT).arg(GIT_BRANCH).arg(__DATE__);
            std::cout << msg.toStdString() << std::endl;
            error = 1;
            return;
        }
    }
    
    {
        QStringList::iterator it = hasToken("help", "h");
        if (it != args.end()) {
            CLArgs::printUsage(args[0].toStdString());
            error = 1;
            return;
        }
    }
    
    {
        QStringList::iterator it = hasToken("background", "b");
        if (it != args.end()) {
            isBackground = true;
            args.erase(it);
        }
    }
    
    {
        QStringList::iterator it = hasToken("interpreter", "t");
        if (it != args.end()) {
            isInterpreterMode = true;
            isBackground = true;
            std::cout << QObject::tr("Note: -t argument given, loading in command-line interpreter mode, only Python commands / scripts are accepted").toStdString()
            << std::endl;
            args.erase(it);
        }
    }
    
    
    {
        QStringList::iterator it = hasToken("IPCpipe", "");
        if (it != args.end()) {
            ++it;
            if (it != args.end()) {
                ipcPipe = *it;
                args.erase(it);
            } else {
                std::cout << QObject::tr("You must specify the IPC pipe filename").toStdString() << std::endl;
                error = 1;
                return;
            }
        }
    }
    
    {
        QStringList::iterator it = hasToken("onload", "l");
        if (it != args.end()) {
            ++it;
            if (it != args.end()) {
                defaultOnProjectLoadedScript = *it;
#ifdef __NATRON_UNIX__
                defaultOnProjectLoadedScript = AppManager::qt_tildeExpansion(defaultOnProjectLoadedScript);
#endif
                args.erase(it);
                if (!defaultOnProjectLoadedScript.endsWith(".py")) {
                    std::cout << QObject::tr("The optional on project load script must be a Python script (.py).").toStdString() << std::endl;
                    error = 1;
                    return;
                }
                if (!QFile::exists(defaultOnProjectLoadedScript)) {
                    std::cout << QObject::tr("WARNING: --onload %1 ignored because the file does not exist.").arg(defaultOnProjectLoadedScript).toStdString() << std::endl;
                }
            } else {
                std::cout << QObject::tr("--onload or -l specified, you must enter a script filename afterwards.").toStdString() << std::endl;
                error = 1;
                return;
            }
        }
    }
    
    {
        QStringList::iterator it = findFileNameWithExtension(NATRON_PROJECT_FILE_EXT);
        if (it == args.end()) {
            it = findFileNameWithExtension("py");
            if (it == args.end() && !isInterpreterMode && isBackground) {
                std::cout << QObject::tr("You must specify the filename of a script or %1 project. (.%2)").arg(NATRON_APPLICATION_NAME).arg(NATRON_PROJECT_FILE_EXT).toStdString() << std::endl;
                error = 1;
                return;
            }
            isPythonScript = true;
            
        }
        if (it != args.end()) {
            filename = *it;
#if defined(Q_OS_UNIX)
            filename = AppManager::qt_tildeExpansion(filename);
#endif
            args.erase(it);
        }
    }
    
    //Parse frame range
    for (int i = 0; i < args.size(); ++i) {
        if (tryParseFrameRange(args[i], range)) {
            if (rangeSet) {
                std::cout << QObject::tr("Only a single frame range can be specified").toStdString() << std::endl;
                error = 1;
                return;
            }
            rangeSet = true;
        }
    }
    
    //Parse writers
    for (;;) {
        QStringList::iterator it = hasToken("writer", "w");
        if (it == args.end()) {
            break;
        }
        
        if (!isBackground || isInterpreterMode) {
            std::cout << QObject::tr("You cannot use the -w option in interactive or interpreter mode").toStdString() << std::endl;
            error = 1;
            return;
        }
        
        QStringList::iterator next = it;
        if (next != args.end()) {
            ++next;
        }
        if (next == args.end()) {
            std::cout << QObject::tr("You must specify the name of a Write node when using the -w option").toStdString() << std::endl;
            error = 1;
            return;
        }
        
        
        //Check that the name is conform to a Python acceptable script name
        std::string pythonConform = Natron::makeNameScriptFriendly(next->toStdString());
        if (next->toStdString() != pythonConform) {
            std::cout << QObject::tr("The name of the Write node specified is not valid: it cannot contain non alpha-numerical "
                                     "characters and must not start with a digit.").toStdString() << std::endl;
            error = 1;
            return;
        }
        
        CLArgs::WriterArg w;
        w.name = *next;
        
        QStringList::iterator nextNext = next;
        if (nextNext != args.end()) {
            ++nextNext;
        }
        if (nextNext != args.end()) {
            //Check for an optional filename
            if (!nextNext->startsWith("-") && !nextNext->startsWith("--")) {
                w.filename = *nextNext;
#if defined(Q_OS_UNIX)
                w.filename = AppManager::qt_tildeExpansion(w.filename);
#endif
            }
        }
        
        writers.push_back(w);
        if (nextNext != args.end()) {
            ++nextNext;
        }
        args.erase(it,nextNext);
        
    } // for (;;)
    
    bool atLeastOneOutput = false;
    ///Parse outputs
    for (;;) {
        QString indexStr;
        QStringList::iterator it  = hasOutputToken(indexStr);
        if (error > 0) {
            return;
        }
        if (it == args.end()) {
            break;
        }
        
        if (!isBackground) {
            std::cout << QObject::tr("You cannot use the -o option in interactive or interpreter mode").toStdString() << std::endl;
            error = 1;
            return;
        }

        CLArgs::WriterArg w;
        w.name = QString("Output%1").arg(indexStr);
        w.mustCreate = true;
        atLeastOneOutput = true;
        
        //Check for a mandatory file name
        QStringList::iterator next = it;
        if (next != args.end()) {
            ++next;
        }
        if (next == args.end()) {
            std::cout << QObject::tr("Filename is not optional with the -o option").toStdString() << std::endl;
            error = 1;
            return;
        }
        
        //Check for an optional filename
        if (!next->startsWith("-") && !next->startsWith("--")) {
            w.filename = *next;
        }
        
        writers.push_back(w);
        
        QStringList::iterator endToErase = next;
        ++endToErase;
        args.erase(it, endToErase);
    }
    
    if (atLeastOneOutput && !rangeSet) {
        std::cout << QObject::tr("A frame range must be set when using the -o option").toStdString() << std::endl;
        error = 1;
        return;
    }
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

#if defined(__NATRON_OSX__)
    if (qgetenv("FONTCONFIG_PATH").isNull()) {
        // set FONTCONFIG_PATH to Natron.app/Contents/Resources/etc/fonts (required by plugins using fontconfig)
        QString path = QCoreApplication::applicationDirPath() + "/../Resources/etc/fonts";
        QString pathcfg = path + "/fonts.conf";
        // Note:
        // a minimalist fonts.conf file for OS X is:
        // <fontconfig><dir>/System/Library/Fonts</dir><dir>/Library/Fonts</dir><dir>~/Library/Fonts</dir></fontconfig>
        if (!QFile(pathcfg).exists()) {
            qWarning() << "Fontconfig configuration file" << pathcfg << "does not exist, not setting FONTCONFIG_PATH";
        } else {
            qDebug() << "Setting FONTCONFIG_PATH to" << path;
            qputenv("FONTCONFIG_PATH", path.toUtf8());
        }
    }
#endif


    initPython(argc, argv);

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
    while (!_imp->_appInstances.empty()) {
        _imp->_appInstances.begin()->second.app->quit();
    }
    
    for (PluginsMap::iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        for (PluginMajorsOrdered::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            delete *it2;
        }
    }

    Q_FOREACH (Format * f, _imp->_formats) {
        delete f;
    }


    if (_imp->_backgroundIPC) {
        delete _imp->_backgroundIPC;
    }

    try {
        _imp->saveCaches();
    } catch (std::runtime_error) {
        // ignore errors
    }

    _instance = 0;

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
    
#ifdef NATRON_USE_BREAKPAD
    if (_imp->crashReporter) {
        _imp->crashReporter->terminate();
    }
#endif
    
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
    found->second.status = eAppInstanceStatusInactive;
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

#ifdef NATRON_USE_BREAKPAD
void
AppManagerPrivate::initBreakpad()
{
    if (appPTR->isBackground()) {
        return;
    }
    
    assert(!breakpadHandler);
    std::srand(2000);
    QString filename;
    int handle;
    {
        QTemporaryFile tmpf(NATRON_APPLICATION_NAME "_CRASH_PIPE_");
        tmpf.open();
        handle = tmpf.handle();
        filename = tmpf.fileName();
        tmpf.remove();
    }
    
    QString comPipeFilename = filename + "_COM_PIPE_";
    crashClientServer.reset(new QLocalServer());
    QObject::connect(crashClientServer.get(),SIGNAL( newConnection() ),appPTR,SLOT( onNewCrashReporterConnectionPending() ) );
    crashClientServer->listen(comPipeFilename);
    
    crashReporterBreakpadPipe = filename;
    QStringList args;
    args << filename;
    args << QString::number(handle);
    args << comPipeFilename;
    crashReporter.reset(new QProcess);
    QString crashReporterBinaryPath = qApp->applicationDirPath() + "/NatronCrashReporter";
    crashReporter->start(crashReporterBinaryPath, args);
    
    
}
#endif

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
    _imp->diskCachesLocation = Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationCache) ;

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

    Natron::Log::instance(); //< enable logging
    
#ifdef NATRON_USE_BREAKPAD
    _imp->initBreakpad();
#endif
    
    _imp->_settings->initializeKnobsPublic();
    ///Call restore after initializing knobs
    _imp->_settings->restoreSettings();

    ///basically show a splashScreen
    initGui();


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

    setLoadingStatus( tr("Restoring the image cache...") );
    _imp->restoreCaches();

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
        if ( !cl.getFilename().isEmpty() ) {
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
    if (!cl.getFilename().isEmpty()) {
        args = CLArgs(qApp->arguments(), cl.isBackgroundMode());
    } else{
        args = cl;
    }

    AppInstance* mainInstance = newAppInstance(args);
    
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
} // loadInternal

AppInstance*
AppManager::newAppInstance(const CLArgs& cl)
{
    AppInstance* instance = makeNewInstance(_imp->_availableID);
    ++_imp->_availableID;

    try {
        instance->load(cl);
    } catch (const std::exception & e) {
        Natron::errorDialog( NATRON_APPLICATION_NAME,e.what(), false );
        removeInstance(_imp->_availableID);
        delete instance;
        --_imp->_availableID;
        return NULL;
    } catch (...) {
        Natron::errorDialog( NATRON_APPLICATION_NAME, tr("Cannot load project").toStdString(), false );
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

int
AppManager::getNumInstances() const
{
    return (int)_imp->_appInstances.size();
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
    
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        it->second.app->renderAllViewers();
    }
    
    Project::clearAutoSavesDir();
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
    ref.status = Natron::eAppInstanceStatusActive;
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
    
    std::vector<Natron::Plugin*> ignoredPlugins;
    _imp->_settings->populatePluginsTab(ignoredPlugins);
    
    ///Remove from the plug-ins the ignore plug-ins
    for (std::vector<Natron::Plugin*>::iterator it = ignoredPlugins.begin(); it != ignoredPlugins.end(); ++it) {
        
        PluginsMap::iterator foundId = _imp->_plugins.find((*it)->getPluginID().toStdString());
        if (foundId != _imp->_plugins.end()) {
            PluginMajorsOrdered::iterator found = foundId->second.end();
            
            for (PluginMajorsOrdered::iterator it2 = foundId->second.begin() ; it2 != foundId->second.end() ; ++it2) {
                if (*it2 == *it) {
                    found = it2;
                    break;
                }
            }
            if (found != foundId->second.end()) {
                
                ignorePlugin(*it);
                ///Remove it from the readersMap and writersMap
                
                std::string pluginId = (*it)->getPluginID().toStdString();
                if ((*it)->isReader()) {
                    for (std::map<std::string,std::vector< std::pair<std::string,double> > >::iterator it2 = readersMap.begin(); it2 != readersMap.end(); ++it2) {
                        for (std::vector< std::pair<std::string,double> >::iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                            if (it3->first == pluginId) {
                                it2->second.erase(it3);
                                break;
                            }
                        }
                    }
                }
                if ((*it)->isWriter()) {
                    for (std::map<std::string,std::vector< std::pair<std::string,double> > >::iterator it2 = writersMap.begin(); it2 != writersMap.end(); ++it2) {
                        for (std::vector< std::pair<std::string,double> >::iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                            if (it3->first == pluginId) {
                                it2->second.erase(it3);
                                break;
                            }
                        }
                    }
                    
                }
                delete *found;
                foundId->second.erase(found);
                if (foundId->second.empty()) {
                    _imp->_plugins.erase(foundId);
                }
            }
        }
    }
    
    _imp->_settings->populateReaderPluginsAndFormats(readersMap);
    _imp->_settings->populateWriterPluginsAndFormats(writersMap);

    _imp->declareSettingsToPython();
    
    //Load python groups and init.py & initGui.py scripts
    //Should be done after settings are declared
    loadPythonGroups();

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
        
        QString labelWithoutSuffix = Plugin::makeLabelWithoutSuffix((*first)->getPluginLabel());
        
        //Find a duplicate
        for (PluginsMap::const_iterator it2 = plugins.begin(); it2!=plugins.end(); ++it2) {
            if (it->first == it2->first) {
                continue;
            }
            PluginMajorsOrdered::iterator other = it2->second.begin();
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
            (*it2)->setLabelWithoutSuffix(labelWithoutSuffix);
        }
        
        onPluginLoaded(*first);
        
    }
}

void
AppManager::loadBuiltinNodePlugins(std::map<std::string,std::vector< std::pair<std::string,double> > >* /*readersMap*/,
                                   std::map<std::string,std::vector< std::pair<std::string,double> > >* /*writersMap*/)
{
    {
        boost::shared_ptr<EffectInstance> node( BackDrop::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&BackDrop::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);
        
        std::list<std::string> grouping;
        node->getPluginGrouping(&grouping);
        QStringList qgrouping;
        
        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }
        registerPlugin(qgrouping, node->getPluginID().c_str(), node->getPluginLabel().c_str(), NATRON_IMAGES_PATH "backdrop_icon.png", "", false, false, binary, false, node->getMajorVersion(), node->getMinorVersion(), true);
    }
    {
        boost::shared_ptr<EffectInstance> node( GroupOutput::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&GroupOutput::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);
        
        std::list<std::string> grouping;
        node->getPluginGrouping(&grouping);
        QStringList qgrouping;
        
        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }
        registerPlugin(qgrouping, node->getPluginID().c_str(), node->getPluginLabel().c_str(), NATRON_IMAGES_PATH "output_icon.png", "", false, false, binary, false, node->getMajorVersion(), node->getMinorVersion(), true);
    }
    {
        boost::shared_ptr<EffectInstance> node( GroupInput::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&GroupInput::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);
        
        std::list<std::string> grouping;
        node->getPluginGrouping(&grouping);
        QStringList qgrouping;
        
        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }
        registerPlugin(qgrouping, node->getPluginID().c_str(), node->getPluginLabel().c_str(), NATRON_IMAGES_PATH "input_icon.png", "", false, false, binary, false, node->getMajorVersion(), node->getMinorVersion(), true);
    }
    {
        boost::shared_ptr<EffectInstance> groupNode( NodeGroup::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&NodeGroup::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);
        
        std::list<std::string> grouping;
        groupNode->getPluginGrouping(&grouping);
        QStringList qgrouping;
        
        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }
        registerPlugin(qgrouping, groupNode->getPluginID().c_str(), groupNode->getPluginLabel().c_str(), NATRON_IMAGES_PATH "group_icon.png", "", false, false, binary, false, groupNode->getMajorVersion(), groupNode->getMinorVersion(), true);
    }
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
        registerPlugin(qgrouping, dotNode->getPluginID().c_str(), dotNode->getPluginLabel().c_str(), NATRON_IMAGES_PATH "dot_icon.png", "", false, false, binary, false, dotNode->getMajorVersion(), dotNode->getMinorVersion(), true);
    }
    {
        boost::shared_ptr<EffectInstance> node( DiskCacheNode::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&DiskCacheNode::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);
        
        std::list<std::string> grouping;
        node->getPluginGrouping(&grouping);
        QStringList qgrouping;
        
        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }
        registerPlugin(qgrouping, node->getPluginID().c_str(), node->getPluginLabel().c_str(), NATRON_IMAGES_PATH "diskcache_icon.png", "", false, false, binary, false, node->getMajorVersion(), node->getMinorVersion(), true);
    }
    {
        boost::shared_ptr<EffectInstance> node( RotoPaint::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&RotoPaint::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);
        
        std::list<std::string> grouping;
        node->getPluginGrouping(&grouping);
        QStringList qgrouping;
        
        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }
        registerPlugin(qgrouping, node->getPluginID().c_str(), node->getPluginLabel().c_str(),
                       NATRON_IMAGES_PATH "GroupingIcons/Set2/paint_grouping_2.png", "", false, false, binary, false, node->getMajorVersion(), node->getMinorVersion(), true);
    }
    {
        boost::shared_ptr<EffectInstance> node( RotoNode::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&RotoNode::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);
        
        std::list<std::string> grouping;
        node->getPluginGrouping(&grouping);
        QStringList qgrouping;
        
        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }
        registerPlugin(qgrouping, node->getPluginID().c_str(), node->getPluginLabel().c_str(),
                       NATRON_IMAGES_PATH "rotoNodeIcon.png", "", false, false, binary, false, node->getMajorVersion(), node->getMinorVersion(), true);
    }
    {
        boost::shared_ptr<EffectInstance> node( RotoSmear::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        std::map<std::string,void*> functions;
        functions.insert( std::make_pair("BuildEffect", (void*)&RotoSmear::BuildEffect) );
        LibraryBinary *binary = new LibraryBinary(functions);
        assert(binary);
        
        std::list<std::string> grouping;
        node->getPluginGrouping(&grouping);
        QStringList qgrouping;
        
        for (std::list<std::string>::iterator it = grouping.begin(); it != grouping.end(); ++it) {
            qgrouping.push_back( it->c_str() );
        }
        registerPlugin(qgrouping, node->getPluginID().c_str(), node->getPluginLabel().c_str(),
                        "", "", false, false, binary, false, node->getMajorVersion(), node->getMinorVersion(), false);
    }

}

static void findAndRunScriptFile(const QString& path,const QStringList& files,const QString& script)
{
    for (QStringList::const_iterator it = files.begin(); it != files.end(); ++it) {
        if (*it == script) {
            QFile file(path + *it);
            if (file.open(QIODevice::ReadOnly)) {
                QTextStream ts(&file);
                QString content = ts.readAll();
                PyRun_SimpleString(content.toStdString().c_str());

                PyObject* mainModule = getMainModule();
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
                    error = PY3String_asString(errorObj);
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
                }

            }
            break;
        }
    }
}

QString
AppManager::getSystemNonOFXPluginsPath() const
{
    return  Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationData) +
      QDir::separator() + "Plugins";
}

QStringList
AppManager::getAllNonOFXPluginsPaths() const
{
    QStringList templatesSearchPath;
    
    QString dataLocation = Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationData);
    QString mainPath = dataLocation + QDir::separator() + "Plugins";
    
    QDir mainPathDir(mainPath);
    if (!mainPathDir.exists()) {
        QDir dataDir(dataLocation);
        if (dataDir.exists()) {
            dataDir.mkdir("Plugins");
        }
    }
    
    QString envvar(qgetenv(NATRON_PATH_ENV_VAR));
    QStringList splitDirs = envvar.split(QChar(';'));
    std::list<std::string> userSearchPaths;
    _imp->_settings->getPythonGroupsSearchPaths(&userSearchPaths);
    
    QDir cwd( QCoreApplication::applicationDirPath() );
    cwd.cdUp();
    QString natronBundledPluginsPath = QString(cwd.absolutePath() +  "/Plugins");

    bool preferBundleOverSystemWide = _imp->_settings->preferBundledPlugins();
    bool useBundledPlugins = _imp->_settings->loadBundledPlugins();
    if (preferBundleOverSystemWide && useBundledPlugins) {
        ///look-in the bundled plug-ins
        templatesSearchPath.push_back(natronBundledPluginsPath);
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
    }
    return templatesSearchPath;
}

void
AppManager::loadPythonGroups()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
	return;
#endif
    Natron::PythonGILLocker pgl;
    
    QStringList templatesSearchPath = getAllNonOFXPluginsPaths();
    
    
    
    QStringList filters;
    filters << "*.py";
    
    std::string err;
    
    QStringList allPlugins;
    
    ///For all search paths, first add the path to the python path, then run in order the init.py and initGui.py
    for (int i = 0; i < templatesSearchPath.size(); ++i) {
        
        std::string addToPythonPath("sys.path.append(\"");
        addToPythonPath += templatesSearchPath[i].toStdString();
        addToPythonPath += "\")\n";
        
        bool ok  = interpretPythonScript(addToPythonPath, &err, 0);
        assert(ok);
        Q_UNUSED(ok);
    }
    
    ///Also import Pyside.QtCore and Pyside.QtGui (the later only in non background mode
    {
        bool ok  = interpretPythonScript("import PySide.QtCore as QtCore", &err, 0);
        assert(ok);
        Q_UNUSED(ok);
    }
    
    if (!isBackground()) {
        bool ok  = interpretPythonScript("import PySide.QtGui as QtGui", &err, 0);
        assert(ok);
        Q_UNUSED(ok);
    }

    
    for (int i = 0; i < templatesSearchPath.size(); ++i) {
        QDir d(templatesSearchPath[i]);
        if (d.exists()) {
            
            QStringList files = d.entryList(filters,QDir::Files | QDir::NoDotAndDotDot);
            findAndRunScriptFile(d.absolutePath() + '/', files,"init.py");
            if (!isBackground()) {
                findAndRunScriptFile(d.absolutePath() + '/',files,"initGui.py");
            }
            
            QStringList scripts = d.entryList(QStringList(QString("*.py")),QDir::Files | QDir::NoDotAndDotDot);

            for (QStringList::iterator it = scripts.begin(); it != scripts.end(); ++it) {
                if (it->endsWith(".py") && *it != QString("init.py") && *it != QString("initGui.py")) {
                    allPlugins.push_back(d.absolutePath() + "/" + *it);
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
            
            std::string addToPythonPath("sys.path.append(\"");
            addToPythonPath += diffSearch[i].toStdString();
            addToPythonPath += "\")\n";
            
            bool ok  = interpretPythonScript(addToPythonPath, &err, 0);
            assert(ok);
            Q_UNUSED(ok);
        }
    }
    
    appPTR->setLoadingStatus(QString(QObject::tr("Loading PyPlugs...")));

    for (int i = 0; i < allPlugins.size(); ++i) {
        
        QString moduleName = allPlugins[i];
        qDebug() << "Loading " << moduleName;
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
        
        if (getGroupInfos(modulePath.toStdString(),moduleName.toStdString(), &pluginID, &pluginLabel, &iconFilePath, &pluginGrouping, &pluginDescription, &version)) {

            QStringList grouping = QString(pluginGrouping.c_str()).split(QChar('/'));
            Natron::Plugin* p = registerPlugin(grouping, pluginID.c_str(), pluginLabel.c_str(), iconFilePath.c_str(), QString(), false, false, 0, false, version, 0, true);
            
            p->setPythonModule(modulePath + moduleName);
            
        }
        
    }
}

Natron::Plugin*
AppManager::registerPlugin(const QStringList & groups,
                           const QString & pluginID,
                           const QString & pluginLabel,
                           const QString & pluginIconPath,
                           const QString & groupIconPath,
                           bool isReader,
                           bool isWriter,
                           Natron::LibraryBinary* binary,
                           bool mustCreateMutex,
                           int major,
                           int minor,
                           bool canBeUserCreated)
{
    QMutex* pluginMutex = 0;

    if (mustCreateMutex) {
        pluginMutex = new QMutex(QMutex::Recursive);
    }
    Natron::Plugin* plugin = new Natron::Plugin(binary,pluginID,pluginLabel,pluginIconPath,groupIconPath,groups,pluginMutex,major,minor,
                                                isReader,isWriter, canBeUserCreated);
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

Natron::Plugin*
AppManagerPrivate::findPluginById(const QString& newId,int major, int minor) const
{
    for (PluginsMap::const_iterator it = _plugins.begin(); it != _plugins.end(); ++it) {
        for (PluginMajorsOrdered::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ((*it2)->getPluginID() == newId && (*it2)->getMajorVersion() == major && (*it2)->getMinorVersion() == minor) {
                return (*it2);
            }
        }
        
    }
    return 0;
}

Natron::Plugin*
AppManager::getPluginBinaryFromOldID(const QString & pluginId,int majorVersion,int minorVersion) const
{
    std::map<int,Natron::Plugin*> matches;
    
    if (pluginId == "Viewer") {
        return _imp->findPluginById(PLUGINID_NATRON_VIEWER, majorVersion, minorVersion);
    } else if (pluginId == "Dot") {
        return _imp->findPluginById(PLUGINID_NATRON_DOT,majorVersion, minorVersion );
    } else if (pluginId == "DiskCache") {
        return _imp->findPluginById(PLUGINID_NATRON_DISKCACHE, majorVersion, minorVersion);
    } else if (pluginId == "BackDrop") {
        return _imp->findPluginById(PLUGINID_NATRON_BACKDROP, majorVersion, minorVersion);
    }
    
    ///Try remapping these ids to old ids we had in Natron < 1.0 for backward-compat
    for (PluginsMap::const_iterator it = _imp->_plugins.begin(); it != _imp->_plugins.end(); ++it) {
        for (PluginMajorsOrdered::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ((*it2)->generateUserFriendlyPluginID() == pluginId &&
                ((*it2)->getMajorVersion() == majorVersion || majorVersion == -1) &&
                ((*it2)->getMinorVersion() == minorVersion || minorVersion == -1)) {
                return *it2;
            }
        }
        
    }
    return 0;
}

Natron::Plugin*
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

boost::shared_ptr<Natron::EffectInstance>
AppManager::createOFXEffect(boost::shared_ptr<Natron::Node> node,
                            const NodeSerialization* serialization,
                            const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                            bool allowFileDialogs,
                            bool disableRenderScaleSupport) const
{
    return _imp->ofxHost->createOfxEffect(node,serialization,paramValues,allowFileDialogs,disableRenderScaleSupport);
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
AppManager::removeAllImagesFromCacheWithMatchingKey(bool useTreeVersion, U64 treeVersion)
{
    _imp->_nodeCache->removeAllImagesFromCacheWithMatchingKey(useTreeVersion, treeVersion);
}

void
AppManager::removeAllImagesFromDiskCacheWithMatchingKey(bool useTreeVersion, U64 treeVersion)
{
    _imp->_diskCache->removeAllImagesFromCacheWithMatchingKey(useTreeVersion, treeVersion);
}

void
AppManager::removeAllTexturesFromCacheWithMatchingKey(bool useTreeVersion, U64 treeVersion)
{
    _imp->_viewerCache->removeAllImagesFromCacheWithMatchingKey(useTreeVersion, treeVersion);
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
                     std::list<boost::shared_ptr<Natron::Image> >* returnValue) const
{
    return _imp->_nodeCache->get(key,returnValue);
}

bool
AppManager::getImageOrCreate(const Natron::ImageKey & key,
                             const boost::shared_ptr<Natron::ImageParams>& params,
                             boost::shared_ptr<Natron::Image>* returnValue) const
{
    return _imp->_nodeCache->getOrCreate(key,params,returnValue);
}

bool
AppManager::getImage_diskCache(const Natron::ImageKey & key,std::list<boost::shared_ptr<Natron::Image> >* returnValue) const
{
    return _imp->_diskCache->get(key, returnValue);
}

bool
AppManager::getImageOrCreate_diskCache(const Natron::ImageKey & key,const boost::shared_ptr<Natron::ImageParams>& params,
                                boost::shared_ptr<Natron::Image>* returnValue) const
{
    return _imp->_diskCache->getOrCreate(key, params, returnValue);
}


bool
AppManager::getTexture(const Natron::FrameKey & key,
                       boost::shared_ptr<Natron::FrameEntry>* returnValue) const
{
    std::list<boost::shared_ptr<Natron::FrameEntry> > retList;
    
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
AppManager::getTextureOrCreate(const Natron::FrameKey & key,
                               const boost::shared_ptr<Natron::FrameParams>& params,
                               boost::shared_ptr<Natron::FrameEntry>* returnValue) const
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
    qRegisterMetaType<Format>();
    qRegisterMetaType<SequenceTime>("SequenceTime");
    qRegisterMetaType<Natron::StandardButtons>();
    qRegisterMetaType<RectI>();
    qRegisterMetaType<RectD>();
#if QT_VERSION < 0x050000
    qRegisterMetaType<QAbstractSocket::SocketState>("SocketState");
#endif
}

template <typename T>
void saveCache(Natron::Cache<T>* cache)
{
    std::ofstream ofile;
    ofile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    std::string cacheRestoreFilePath = cache->getRestoreFilePath();
    try {
        ofile.open(cacheRestoreFilePath.c_str(),std::ofstream::out);
    } catch (const std::ios_base::failure & e) {
        qDebug() << "Exception occured when opening file" <<  cacheRestoreFilePath.c_str() << ':' << e.what();
        // The following is C++11 only:
        //<< "Error code: " << e.code().value()
        //<< " (" << e.code().message().c_str() << ")\n"
        //<< "Error category: " << e.code().category().name();
        
        return;
    }
    
    if ( !ofile.good() ) {
        qDebug() << "Failed to save cache to" << cacheRestoreFilePath.c_str();
        
        return;
    }
    
    typename Natron::Cache<T>::CacheTOC toc;
    cache->save(&toc);
    unsigned int version = cache->cacheVersion();
    try {
        boost::archive::binary_oarchive oArchive(ofile);
        oArchive << version;
        oArchive << toc;
    } catch (const std::exception & e) {
        qDebug() << "Failed to serialize the cache table of contents:" << e.what();
    }
    
    ofile.close();

}

void
AppManager::setDiskCacheLocation(const QString& path)
{
    
    QDir d(path);
    QMutexLocker k(&_imp->diskCachesLocationMutex);
    if (d.exists() && !path.isEmpty()) {
        _imp->diskCachesLocation = path;
    } else {
        _imp->diskCachesLocation = Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationCache);
    }
    
}

const QString&
AppManager::getDiskCacheLocation() const
{
    QMutexLocker k(&_imp->diskCachesLocationMutex);
    return _imp->diskCachesLocation;
}

void
AppManagerPrivate::saveCaches()
{
    saveCache<FrameEntry>(_viewerCache.get());
    saveCache<Image>(_diskCache.get());
} // saveCaches

template <typename T>
void restoreCache(AppManagerPrivate* p,Natron::Cache<T>* cache)
{
    if ( p->checkForCacheDiskStructure( cache->getCachePath() ) ) {
        std::ifstream ifile;
        std::string settingsFilePath = cache->getRestoreFilePath();
        try {
            ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ifile.open(settingsFilePath.c_str(),std::ifstream::in);
        } catch (const std::ifstream::failure & e) {
            qDebug() << "Failed to open the cache restoration file:" << e.what();
            
            return;
        }
        
        if ( !ifile.good() ) {
            qDebug() << "Failed to cache file for restoration:" <<  settingsFilePath.c_str();
            ifile.close();
            
            return;
        }
        
        typename Natron::Cache<T>::CacheTOC tableOfContents;
        unsigned int cacheVersion = 0x1; //< default to 1 before NATRON_CACHE_VERSION was introduced
        try {
            boost::archive::binary_iarchive iArchive(ifile);
            if (cache->cacheVersion() >= NATRON_CACHE_VERSION) {
                iArchive >> cacheVersion;
            }
            //Only load caches with same version, otherwise wipe it!
            if (cacheVersion == cache->cacheVersion()) {
                iArchive >> tableOfContents;
            } else {
                p->cleanUpCacheDiskStructure(cache->getCachePath());
            }
        } catch (const std::exception & e) {
            qDebug() << "Exception when reading disk cache TOC:" << e.what();
            ifile.close();
            
            return;
        }
        
        ifile.close();
        
        QFile restoreFile( settingsFilePath.c_str() );
        restoreFile.remove();
        
        cache->restore(tableOfContents);
    }
}

void
AppManagerPrivate::restoreCaches()
{
    //    {
    //        if ( checkForCacheDiskStructure( _nodeCache->getCachePath() ) ) {
    //            std::ifstream ifile;
    //            std::string settingsFilePath = _nodeCache->getRestoreFilePath();
    //            try {
    //                ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    //                ifile.open(settingsFilePath.c_str(),std::ifstream::in);
    //            } catch (const std::ifstream::failure & e) {
    //                qDebug() << "Failed to open the cache restoration file:" << e.what();
    //
    //                return;
    //            }
    //
    //            if ( !ifile.good() ) {
    //                qDebug() << "Failed to cache file for restoration:" <<  settingsFilePath.c_str();
    //                ifile.close();
    //
    //                return;
    //            }
    //
    //            Natron::Cache<Image>::CacheTOC tableOfContents;
    //            try {
    //                boost::archive::binary_iarchive iArchive(ifile);
    //                iArchive >> tableOfContents;
    //            } catch (const std::exception & e) {
    //                qDebug() << e.what();
    //                ifile.close();
    //
    //                return;
    //            }
    //
    //            ifile.close();
    //
    //            QFile restoreFile( settingsFilePath.c_str() );
    //            restoreFile.remove();
    //
    //            _nodeCache->restore(tableOfContents);
    //        }
    //    }
    if (!appPTR->isBackground()) {
        restoreCache<FrameEntry>(this, _viewerCache.get());
        restoreCache<Image>(this, _diskCache.get());
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
        qDebug() << cachePath << "doesn't contain sub-folders indexed from 00 to FF. Reseting.";
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
        Q_FOREACH (QString e, etr) {
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
        for (std::list<HANDLE>::iterator it = files.begin(); it != files.end(); ++it) {
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

    _imp->_ofxLog.append(str + '\n' + '\n');
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
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin() ; it != _imp->_appInstances.end(); ++it) {
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
AppManager::setThreadAsActionCaller(bool actionCaller)
{
    _imp->ofxHost->setThreadAsActionCaller(actionCaller);
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
Natron::PY3String_asString(PyObject* obj)
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
    toPrepend.append(binPath + "/../Frameworks/Python.framework/Versions/3.4/lib/python3.4");
    toPrepend.append(':');
    toPrepend.append(binPath + "/../Plugins");
    if (!pathEmpty) {
        toPrepend.push_back(':');
    }
#elif defined(__NATRON_LINUX__)
    toPrepend.append(binPath + "/../lib/python3.4");
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
    //Py_NoSiteFlag = 1; 
    Py_Initialize();
    
    // pythonHome must be const, so that the c_str() pointer is never invalidated
    
#ifndef IS_PYTHON_2
#ifdef __NATRON_WIN32__
    static const std::wstring pythonHome(Natron::s2ws("."));
#elif defined(__NATRON_LINUX__)
    static const std::wstring pythonHome(Natron::s2ws("../lib"));
#elif defined(__NATRON_OSX__)
    static const std::wstring pythonHome(Natron::s2ws("../Frameworks/Python.framework/Versions/3.4/lib"));
#endif
    Py_SetPythonHome(const_cast<wchar_t*>(pythonHome.c_str()));
    PySys_SetArgv(argc,_imp->args.data()); /// relative module import
#else
#ifdef __NATRON_WIN32__
    static const std::string pythonHome(".");
#elif defined(__NATRON_LINUX__)
    static const std::string pythonHome("../lib");
#elif defined(__NATRON_OSX__)
    static const std::string pythonHome("../Frameworks/Python.framework/Versions/3.4/lib");
#endif
    Py_SetPythonHome(const_cast<char*>(pythonHome.c_str()));
    PySys_SetArgv(argc,_imp->args.data()); /// relative module import
#endif
    
    _imp->mainModule = PyImport_ImportModule("__main__"); //create main module , new ref
    
    //See http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    //Python releases the GIL every 100 virtual Python instructions, we do not want that to happen in the middle of an expression.
    //_PyEval_SetSwitchInterval(LONG_MAX);
    
    //See answer for http://stackoverflow.com/questions/15470367/pyeval-initthreads-in-python-3-how-when-to-call-it-the-saga-continues-ad-naus
    PyEval_InitThreads();
    
    ///Do as per http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    ///All calls to the Python API should call Natron::PythonGILLocker beforehand.
    //_imp->mainThreadState = PyGILState_GetThisThreadState();
    //PyEval_ReleaseThread(_imp->mainThreadState);
    
    std::string err;
    bool ok = interpretPythonScript("import sys\nfrom math import *\nimport " + std::string(NATRON_ENGINE_PYTHON_MODULE_NAME), &err, 0);
    assert(ok);
    
    ok = interpretPythonScript(std::string(NATRON_ENGINE_PYTHON_MODULE_NAME) + ".natron = " + std::string(NATRON_ENGINE_PYTHON_MODULE_NAME) + ".PyCoreApplication()\n" , &err, 0);
    assert(ok);
    
    if (!isBackground()) {
        
        ok = interpretPythonScript("import sys\nimport " + std::string(NATRON_GUI_PYTHON_MODULE_NAME), &err, 0);
        assert(ok);
        
        ok = interpretPythonScript(std::string(NATRON_GUI_PYTHON_MODULE_NAME) + ".natron = " +
                                   std::string(NATRON_GUI_PYTHON_MODULE_NAME) + ".PyGuiApplication()\n" , &err, 0);
        assert(ok);
        
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
        ok = interpretPythonScript(script,&err,0);
        assert(ok);
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
    for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        it->second.app->toggleAutoHideGraphInputs();
    }
}


void
AppManager::launchPythonInterpreter()
{
    std::string err;
    bool ok = Natron::interpretPythonScript("app = app1\n", &err, 0);
    assert(ok);
    Q_UNUSED(ok);

   // Natron::PythonGILLocker pgl;
    
    Py_Main(1, &_imp->args[0]);
}

void
AppManagerPrivate::declareSettingsToPython()
{
    std::stringstream ss;
    ss <<  NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings = " << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.getSettings()\n";
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _settings->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss <<  NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings." <<
        (*it)->getName() << " = " << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings.getParam('" << (*it)->getName() << "')\n";
    }
}

int
AppManager::isProjectAlreadyOpened(const std::string& projectFilePath) const
{
	for (std::map<int,AppInstanceRef>::iterator it = _imp->_appInstances.begin(); it != _imp->_appInstances.end(); ++it) {
        boost::shared_ptr<Natron::Project> proj = it->second.app->getProject();
		if (proj) {
			QString path = proj->getProjectPath();
			QString name = proj->getProjectName();
			std::string existingProject = path.toStdString() + name.toStdString();
			if (existingProject == projectFilePath) {
				return it->first;
			}
		}
    }
	return -1;
}

#ifdef NATRON_USE_BREAKPAD
void
AppManager::onCrashReporterOutputWritten()
{
    ///always running in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    
    QString str = _imp->crashServerConnection->readLine();
    while ( str.endsWith('\n') ) {
        str.chop(1);
    }

    if (str.startsWith("-i")) {
        //At this point, the CrashReporter just notified us it is ready, so we can create our exception handler safely
        //because we know the pipe is opened on the other side.
        
#if defined(Q_OS_MAC)
        _imp->breakpadHandler.reset(new google_breakpad::ExceptionHandler( std::string(), 0, 0/*dmpcb*/,  0, true,
                                                                          _imp->crashReporterBreakpadPipe.toStdString().c_str()));
#elif defined(Q_OS_LINUX)
        _imp->breakpadHandler.reset(new google_breakpad::ExceptionHandler( google_breakpad::MinidumpDescriptor(std::string()), 0, 0/*dmpCb*/,
                                                                          0, true, handle));
#elif defined(Q_OS_WIN32)
        _imp->breakpadHandler.reset(new google_breakpad::ExceptionHandler( std::wstring(), 0, 0/*dmpcb*/,
                                                                          google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                                          MiniDumpNormal,
                                                                          filename.toStdWString(),
                                                                          0));
#endif
        
        //crash_application();

    } else {
        qDebug() << "Error: Unable to interpret message.";
        throw std::runtime_error("AppManager::onCrashReporterOutputWritten() received erroneous message");
    }


}
#endif

#ifdef NATRON_USE_BREAKPAD
void
AppManager::onNewCrashReporterConnectionPending()
{
    ///accept only 1 connection!
    if (_imp->crashServerConnection) {
        return;
    }
    
    _imp->crashServerConnection = _imp->crashClientServer->nextPendingConnection();
    
    QObject::connect( _imp->crashServerConnection, SIGNAL( readyRead() ), this, SLOT( onCrashReporterOutputWritten() ) );
}
#endif

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

namespace Natron {
void
errorDialog(const std::string & title,
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
errorDialog(const std::string & title,
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
warningDialog(const std::string & title,
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
warningDialog(const std::string & title,
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
informationDialog(const std::string & title,
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
informationDialog(const std::string & title,
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


Natron::StandardButtonEnum
questionDialog(const std::string & title,
               const std::string & message,
               bool useHtml,
               Natron::StandardButtons buttons,
               Natron::StandardButtonEnum defaultButton)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        return topLvlInstance->questionDialog(title,message,useHtml,buttons,defaultButton);
    } else {
        std::cout << "QUESTION ASKED: " << title << " :" << message << std::endl;
        std::cout << NATRON_APPLICATION_NAME " answered yes." << std::endl;

        return Natron::eStandardButtonYes;
    }
}

Natron::StandardButtonEnum
questionDialog(const std::string & title,
               const std::string & message,
               bool useHtml,
               Natron::StandardButtons buttons,
               Natron::StandardButtonEnum defaultButton,
               bool* stopAsking)
{
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    if ( topLvlInstance && !appPTR->isBackground() ) {
        return topLvlInstance->questionDialog(title,message,useHtml,buttons,defaultButton,stopAsking);
    } else {
        std::cout << "QUESTION ASKED: " << title << " :" << message << std::endl;
        std::cout << NATRON_APPLICATION_NAME " answered yes." << std::endl;

        return Natron::eStandardButtonYes;
    }
}
    
std::size_t findNewLineStartAfterImports(std::string& script)
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
    
PyObject* getMainModule()
{
    return appPTR->getMainModule();
}

std::size_t ensureScriptHasModuleImport(const std::string& moduleName,std::string& script)
{
    /// import module
    script = "from " + moduleName + " import * \n" + script;
    return findNewLineStartAfterImports(script);
}
    
bool interpretPythonScript(const std::string& script,std::string* error,std::string* output)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
	return true;
#endif
    Natron::PythonGILLocker pgl;
    
    PyObject* mainModule = getMainModule();
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

    
void compilePyScript(const std::string& script,PyObject** code)
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
    

    
std::string
makeNameScriptFriendly(const std::string& str)
{
    if (str == "from") {
        return "pFrom";
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
    
bool
getGroupInfos(const std::string& modulePath,
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
    Natron::PythonGILLocker pgl;
    
    QString script("import sys\n"
                   "import %1\n"
                   "ret = True\n"
                   "if not hasattr(%1,\"createInstance\") or not hasattr(%1.createInstance,\"__call__\"):\n"
                   "    ret = False\n"
                   "if not hasattr(%1,\"getLabel\") or not hasattr(%1.getLabel,\"__call__\"):\n"
                   "    ret = False\n"
                   "if ret == True:\n"
                   "    global templateLabel\n"
                   "    templateLabel = %1.getLabel()\n"
                   "pluginID = templateLabel\n"
                   "version = 1\n"
                   "if hasattr(%1,\"getVersion\") and hasattr(%1.getVersion,\"__call__\"):\n"
                   "    version = %1.getVersion()\n"
                   "if hasattr(%1,\"getDescription\") and hasattr(%1.getDescription,\"__call__\"):\n"
                   "    global description\n"
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
    if (!interpretPythonScript(toRun, &err, 0)) {
        QString logStr("Failure when loading ");
        logStr.append(pythonModule.c_str());
        logStr.append(": ");
        logStr.append(err.c_str());
        appPTR->writeToOfxLog_mt_safe(logStr);
        qDebug() << logStr;
        return false;
    }
    
    PyObject* mainModule = getMainModule();
    PyObject* retObj = PyObject_GetAttrString(mainModule,"ret"); //new ref
    assert(retObj);
    if (PyObject_IsTrue(retObj) == 0) {
        Py_XDECREF(retObj);
        return false;
    } else {
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
        
        *pluginLabel = PY3String_asString(labelObj);
        Py_XDECREF(labelObj);
        
        if (idObj) {
            *pluginID = PY3String_asString(idObj);
            deleteScript.append("del pluginID\n");
            Py_XDECREF(idObj);
        }
        
        if (iconObj) {
            *iconFilePath = PY3String_asString(iconObj);
            QFileInfo iconInfo(QString(modulePath.c_str()) + QString(iconFilePath->c_str()));
            *iconFilePath =  iconInfo.canonicalFilePath().toStdString();

            deleteScript.append("del templateIcon\n");
            Py_XDECREF(iconObj);
        }
        if (iconGrouping) {
            *grouping = PY3String_asString(iconGrouping);
            deleteScript.append("del templateGrouping\n");
            Py_XDECREF(iconGrouping);
        }
        
        if (versionObj) {
            *version = (unsigned int)PyLong_AsLong(versionObj);
            deleteScript.append("del version\n");
            Py_XDECREF(versionObj);
        }
        
        if (pluginDescriptionObj) {
            *description = PY3String_asString(pluginDescriptionObj);
            deleteScript.append("del description\n");
            Py_XDECREF(pluginDescriptionObj);
        }
        
        if (grouping->empty()) {
            *grouping = PLUGIN_GROUP_OTHER;
        }
        
        
        bool ok = interpretPythonScript(deleteScript, &err, NULL);
        assert(ok);
        Q_UNUSED(ok);
        return true;
    }
}
    
#if 0
void saveRestoreVariable(const std::string& variableBaseName,std::string* scriptToExec,std::string* restoreScript)
{
    PyObject* mainModule = getMainModule();
    std::string attr = variableBaseName;
    if (!scriptToExec->empty() && scriptToExec->at(scriptToExec->size() - 1) != '\n') {
        scriptToExec->append("\n");
    }
    if (!restoreScript->empty() && restoreScript->at(restoreScript->size() - 1) != '\n') {
        restoreScript->append("\n");
    }
    std::string toSave;
    std::string toRestore;
    std::string attrBak;
    while (PyObject_HasAttrString(mainModule,attr.c_str())) {
        attrBak = attr + "_bak";
        toSave.insert(0,attrBak + " = " + attr + "\n");
        toRestore.insert(0,attr + " = " + attrBak + "\n");
        attr.append("_bak");
    }
    if (!attrBak.empty()) {
        toRestore.append("del " + attrBak + "\n");
    }
    scriptToExec->append(toSave);
    restoreScript->append(toRestore);
}
#endif
    
void getFunctionArguments(const std::string& pyFunc,std::string* error,std::vector<std::string>* args)
{
    std::stringstream ss;
    ss << "import inspect\n";
    ss << "args_spec = inspect.getargspec(" << pyFunc << ")\n";
    std::string script = ss.str();
    std::string output;
    bool ok = interpretPythonScript(script, error, &output);
    if (!ok) {
        return;
    }
    PyObject* mainModule = getMainModule();
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
    
} //Namespace Natron
