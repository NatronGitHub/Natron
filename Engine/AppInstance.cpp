/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AppInstance.h"

#include <fstream>
#include <list>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QEventLoop>
#include <QtCore/QSettings>
#include <QtNetwork/QNetworkReply>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)

#include "Global/QtCompat.h" // removeFileExtension

#include "Engine/BlockingBackgroundRender.h"
#include "Engine/CLArgs.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/FileDownloader.h"
#include "Engine/GroupOutput.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/ProcessHandler.h"
#include "Engine/ReadNode.h"
#include "Engine/Settings.h"
#include "Engine/WriteNode.h"

NATRON_NAMESPACE_ENTER

FlagSetter::FlagSetter(bool initialValue,
                       bool* p)
    : p(p)
    , lock(0)
{
    *p = initialValue;
}

FlagSetter::FlagSetter(bool initialValue,
                       bool* p,
                       QMutex* mutex)
    : p(p)
    , lock(mutex)
{
    lock->lock();
    *p = initialValue;
    lock->unlock();
}

FlagSetter::~FlagSetter()
{
    if (lock) {
        lock->lock();
    }
    *p = !*p;
    if (lock) {
        lock->unlock();
    }
}

FlagIncrementer::FlagIncrementer(int* p)
    : p(p)
    , lock(0)
{
    *p = *p + 1;
}

FlagIncrementer::FlagIncrementer(int* p,
                                 QMutex* mutex)
    : p(p)
    , lock(mutex)
{
    lock->lock();
    *p = *p + 1;
    lock->unlock();
}

FlagIncrementer::~FlagIncrementer()
{
    if (lock) {
        lock->lock();
    }
    *p = *p - 1;
    if (lock) {
        lock->unlock();
    }
}

struct RenderQueueItem
{
    AppInstance::RenderWork work;
    QString sequenceName;
    QString savePath;
    ProcessHandlerPtr process;
};

struct AppInstancePrivate
{
    Q_DECLARE_TR_FUNCTIONS(AppInstance)

public:
    AppInstance* _publicInterface;
    ProjectPtr _currentProject; //< ptr to the project
    int _appID; //< the unique ID of this instance (or window)
    bool _projectCreatedWithLowerCaseIDs;
    mutable QMutex creatingGroupMutex;

    //When a pyplug is created
    int _creatingGroup;
    // True when a pyplug is being created internally without being shown to the user
    bool _creatingInternalNode;

    //When a node is created, it gets appended to this list (since for a PyPlug more than 1 node can be created)
    std::list<NodePtr> _creatingNodeQueue;

    //When a node tree is created
    int _creatingTree;
    mutable QMutex renderQueueMutex;
    std::list<RenderQueueItem> renderQueue, activeRenders;
    mutable QMutex invalidExprKnobsMutex;
    std::list<KnobIWPtr> invalidExprKnobs;

    ProjectBeingLoadedInfo projectBeingLoaded;

    AppInstancePrivate(int appID,
                       AppInstance* app)

        : _publicInterface(app)
        , _currentProject()
        , _appID(appID)
        , _projectCreatedWithLowerCaseIDs(false)
        , creatingGroupMutex()
        , _creatingGroup(0)
        , _creatingInternalNode(false)
        , _creatingNodeQueue()
        , _creatingTree(0)
        , renderQueueMutex()
        , renderQueue()
        , activeRenders()
        , invalidExprKnobsMutex()
        , invalidExprKnobs()
        , projectBeingLoaded()
    {
    }

    void declareCurrentAppVariable_Python();


    void executeCommandLinePythonCommands(const CLArgs& args);

    bool validateRenderOptions(const AppInstance::RenderWork& w,
                               int* firstFrame,
                               int* lastFrame,
                               int* frameStep);

    void getSequenceNameFromWriter(const OutputEffectInstance* writer, QString* sequenceName);

    void startRenderingFullSequence(bool blocking, const RenderQueueItem& writerWork);
};

AppInstance::AppInstance(int appID)
    : QObject()
    , _imp( new AppInstancePrivate(appID, this) )
{
}

AppInstance::~AppInstance()
{
    _imp->_currentProject->clearNodesBlocking();
}

const ProjectBeingLoadedInfo&
AppInstance::getProjectBeingLoadedInfo() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->projectBeingLoaded;
}

void
AppInstance::setProjectBeingLoadedInfo(const ProjectBeingLoadedInfo& info)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->projectBeingLoaded = info;
}

const std::list<NodePtr>&
AppInstance::getNodesBeingCreated() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->_creatingNodeQueue;
}

bool
AppInstance::isCreatingNodeTree() const
{
    QMutexLocker k(&_imp->creatingGroupMutex);

    return _imp->_creatingTree;
}

void
AppInstance::setIsCreatingNodeTree(bool b)
{
    QMutexLocker k(&_imp->creatingGroupMutex);

    if (b) {
        ++_imp->_creatingTree;
    } else {
        if (_imp->_creatingTree >= 1) {
            --_imp->_creatingTree;
        } else {
            _imp->_creatingTree = 0;
        }
    }
}

void
AppInstance::checkForNewVersion() const
{
    FileDownloader* downloader = new FileDownloader( QUrl( QString::fromUtf8(NATRON_LATEST_VERSION_URL) ), false );
    QObject::connect( downloader, SIGNAL(downloaded()), this, SLOT(newVersionCheckDownloaded()) );
    QObject::connect( downloader, SIGNAL(error()), this, SLOT(newVersionCheckError()) );

    ///make the call blocking
    QEventLoop loop;

    connect( downloader->getReply(), SIGNAL(finished()), &loop, SLOT(quit()) );
    loop.exec();
}

//return -1 if a < b, 0 if a == b and 1 if a > b
//Returns -2 if not understood
static
int
compareDevStatus(const QString& a,
                 const QString& b)
{
    if ( ( a == QString::fromUtf8(NATRON_DEVELOPMENT_DEVEL) ) || ( a == QString::fromUtf8(NATRON_DEVELOPMENT_SNAPSHOT) ) ) {
        //Do not try updates when update available is a dev build
        return -1;
    } else if ( ( b == QString::fromUtf8(NATRON_DEVELOPMENT_DEVEL) ) || ( b == QString::fromUtf8(NATRON_DEVELOPMENT_SNAPSHOT) ) ) {
        //This is a dev build, do not try updates
        return -1;
    } else if ( a == QString::fromUtf8(NATRON_DEVELOPMENT_ALPHA) ) {
        if ( b == QString::fromUtf8(NATRON_DEVELOPMENT_ALPHA) ) {
            return 0;
        } else {
            return -1;
        }
    } else if ( a == QString::fromUtf8(NATRON_DEVELOPMENT_BETA) ) {
        if ( b == QString::fromUtf8(NATRON_DEVELOPMENT_ALPHA) ) {
            return 1;
        } else if ( b == QString::fromUtf8(NATRON_DEVELOPMENT_BETA) ) {
            return 0;
        } else {
            return -1;
        }
    } else if ( a == QString::fromUtf8(NATRON_DEVELOPMENT_RELEASE_CANDIDATE) ) {
        if ( b == QString::fromUtf8(NATRON_DEVELOPMENT_ALPHA) ) {
            return 1;
        } else if ( b == QString::fromUtf8(NATRON_DEVELOPMENT_BETA) ) {
            return 1;
        } else if ( b == QString::fromUtf8(NATRON_DEVELOPMENT_RELEASE_CANDIDATE) ) {
            return 0;
        } else {
            return -1;
        }
    } else if ( a == QString::fromUtf8(NATRON_DEVELOPMENT_RELEASE_STABLE) ) {
        if ( b == QString::fromUtf8(NATRON_DEVELOPMENT_RELEASE_STABLE) ) {
            return 0;
        } else {
            return 1;
        }
    }
    assert(false);

    return -2;
}

void
AppInstance::newVersionCheckDownloaded()
{
    FileDownloader* downloader = qobject_cast<FileDownloader*>( sender() );

    assert(downloader);

    QString extractedFileVersionStr, extractedSoftwareVersionStr, extractedDevStatusStr, extractedBuildNumberStr;
    QString fileVersionTag( QString::fromUtf8("File version: ") );
    QString softwareVersionTag( QString::fromUtf8("Software version: ") );
    QString devStatusTag( QString::fromUtf8("Development status: ") );
    QString buildNumberTag( QString::fromUtf8("Build number: ") );
    QString data( QString::fromUtf8( downloader->downloadedData() ) );
    QTextStream ts(&data);

    while ( !ts.atEnd() ) {
        QString line = ts.readLine();
        if ( line.startsWith( QChar::fromLatin1('#') ) || line.startsWith( QChar::fromLatin1('\n') ) ) {
            continue;
        }

        if ( line.startsWith(fileVersionTag) ) {
            int i = fileVersionTag.size();
            while ( i < line.size() && !line.at(i).isSpace() ) {
                extractedFileVersionStr.push_back( line.at(i) );
                ++i;
            }
        } else if ( line.startsWith(softwareVersionTag) ) {
            int i = softwareVersionTag.size();
            while ( i < line.size() && !line.at(i).isSpace() ) {
                extractedSoftwareVersionStr.push_back( line.at(i) );
                ++i;
            }
        } else if ( line.startsWith(devStatusTag) ) {
            int i = devStatusTag.size();
            while ( i < line.size() && !line.at(i).isSpace() ) {
                extractedDevStatusStr.push_back( line.at(i) );
                ++i;
            }
        } else if ( line.startsWith(buildNumberTag) ) {
            int i = buildNumberTag.size();
            while ( i < line.size() && !line.at(i).isSpace() ) {
                extractedBuildNumberStr.push_back( line.at(i) );
                ++i;
            }
        }
    }

    downloader->deleteLater();


    if ( extractedFileVersionStr.isEmpty() || (extractedFileVersionStr.toInt() < NATRON_LAST_VERSION_FILE_VERSION) ) {
        //The file cannot be decoded here
        return;
    }


    QStringList versionDigits = extractedSoftwareVersionStr.split( QChar::fromLatin1('.') );

    ///we only understand 3 digits formed version numbers
    if (versionDigits.size() != 3) {
        return;
    }


    int buildNumber = extractedBuildNumberStr.toInt();
    int major = versionDigits[0].toInt();
    int minor = versionDigits[1].toInt();
    int revision = versionDigits[2].toInt();
    const QString currentDevStatus = QString::fromUtf8(NATRON_DEVELOPMENT_STATUS);
    int devStatCompare = compareDevStatus(extractedDevStatusStr, currentDevStatus);
    int versionEncoded = NATRON_VERSION_ENCODE(major, minor, revision);
    bool hasUpdate = false;

    if ( (versionEncoded > NATRON_VERSION_ENCODED) ||
         ( ( versionEncoded == NATRON_VERSION_ENCODED) &&
           ( ( devStatCompare > 0) || ( ( devStatCompare == 0) && ( buildNumber > NATRON_BUILD_NUMBER) ) ) ) ) {
        if (devStatCompare == 0) {
            if ( ( buildNumber > NATRON_BUILD_NUMBER) && ( versionEncoded == NATRON_VERSION_ENCODED) ) {
                hasUpdate = true;
            } else if (versionEncoded > NATRON_VERSION_ENCODED) {
                hasUpdate = true;
            }
        } else {
            hasUpdate = true;
        }
    }
    if (hasUpdate) {
        const QString popen = QString::fromUtf8("<p>");
        const QString pclose = QString::fromUtf8("</p>");
        QString text =  popen
               + tr("Updates for %1 are now available for download.")
               .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) )
               + pclose
               + popen
               + tr("You are currently using %1 version %2 - %3.")
               .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) )
               .arg( QString::fromUtf8(NATRON_VERSION_STRING) )
               .arg( QString::fromUtf8(NATRON_DEVELOPMENT_STATUS) )
               + pclose
               + popen
               + tr("The latest version of %1 is version %4 - %5.")
               .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) )
               .arg(extractedSoftwareVersionStr)
               .arg(extractedDevStatusStr)
               + pclose
               + popen
               + tr("You can download it from %1.")
               .arg( QString::fromUtf8("<a href=\"https://natrongithub.github.io/#download\">"
                                       "natrongithub.github.io</a>") )
               + pclose;
        Dialogs::informationDialog( tr("New version").toStdString(), text.toStdString(), true );
    }
} // AppInstance::newVersionCheckDownloaded

void
AppInstance::newVersionCheckError()
{
    ///Nothing to do,
    FileDownloader* downloader = qobject_cast<FileDownloader*>( sender() );

    assert(downloader);
    downloader->deleteLater();
}

void
AppInstance::getWritersWorkForCL(const CLArgs& cl,
                                 std::list<AppInstance::RenderWork>& requests)
{
    const std::list<CLArgs::WriterArg>& writers = cl.getWriterArgs();

    for (std::list<CLArgs::WriterArg>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
        NodePtr writerNode;
        if (!it->mustCreate) {
            std::string writerName = it->name.toStdString();
            writerNode = getNodeByFullySpecifiedName(writerName);

            if (!writerNode) {
                QString s = tr("%1 does not belong to the project file. Please enter a valid Write node script-name.").arg(it->name);
                throw std::invalid_argument( s.toStdString() );
            } else {
                if ( !writerNode->isOutputNode() ) {
                    QString s = tr("%1 is not an output node! It cannot render anything.").arg(it->name);
                    throw std::invalid_argument( s.toStdString() );
                }
            }

            if ( !it->filename.isEmpty() ) {
                KnobIPtr fileKnob = writerNode->getKnobByName(kOfxImageEffectFileParamName);
                if (fileKnob) {
                    KnobOutputFile* outFile = dynamic_cast<KnobOutputFile*>( fileKnob.get() );
                    if (outFile) {
                        outFile->setValue( it->filename.toStdString() );
                    }
                }
            }
        } else {
            CreateNodeArgs args(PLUGINID_NATRON_WRITE, getProject());
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
            args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            writerNode = createWriter( it->filename.toStdString(), args );
            if (!writerNode) {
                throw std::runtime_error( tr("Failed to create writer for %1.").arg(it->filename).toStdString() );
            }

            //Connect the writer to the corresponding Output node input
            NodePtr output = getProject()->getNodeByFullySpecifiedName( it->name.toStdString() );
            if (!output) {
                throw std::invalid_argument( tr("%1 is not the name of a valid Output node of the script").arg(it->name).toStdString() );
            }
            GroupOutput* isGrpOutput = dynamic_cast<GroupOutput*>( output->getEffectInstance().get() );
            if (!isGrpOutput) {
                throw std::invalid_argument( tr("%1 is not the name of a valid Output node of the script").arg(it->name).toStdString() );
            }
            NodePtr outputInput = output->getRealInput(0);
            if (outputInput) {
                writerNode->connectInput(outputInput, 0);
            }
        }

        assert(writerNode);
        OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>( writerNode->getEffectInstance().get() );

        if ( cl.hasFrameRange() ) {
            const std::list<std::pair<int, std::pair<int, int> > >& frameRanges = cl.getFrameRanges();
            for (std::list<std::pair<int, std::pair<int, int> > >::const_iterator it2 = frameRanges.begin(); it2 != frameRanges.end(); ++it2) {
                AppInstance::RenderWork r( effect, it2->second.first, it2->second.second, it2->first, cl.areRenderStatsEnabled() );
                requests.push_back(r);
            }
        } else {
            AppInstance::RenderWork r( effect, INT_MIN, INT_MAX, INT_MIN, cl.areRenderStatsEnabled() );
            requests.push_back(r);
        }
    }
} // AppInstance::getWritersWorkForCL

void
AppInstancePrivate::executeCommandLinePythonCommands(const CLArgs& args)
{
    const std::list<std::string>& commands = args.getPythonCommands();

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

void
AppInstance::executeCommandLinePythonCommands(const CLArgs& args)
{
    _imp->executeCommandLinePythonCommands(args);
}

void
AppInstance::load(const CLArgs& cl,
                  bool makeEmptyInstance)
{
    // Initialize the knobs of the project before loading anything else.
    assert(!_imp->_currentProject); // < This function may only be called once per AppInstance
    _imp->_currentProject = Project::create( shared_from_this() );
    _imp->_currentProject->initializeKnobsPublic();

    loadInternal(cl, makeEmptyInstance);
}

void
AppInstance::loadInternal(const CLArgs& cl,
                          bool makeEmptyInstance)
{
    try {
        declareCurrentAppVariable_Python();
    } catch (const std::exception& e) {
        throw std::runtime_error( e.what() );
    }

    if (makeEmptyInstance) {
        return;
    }

    executeCommandLinePythonCommands(cl);

    QString exportDocPath = cl.getExportDocsPath();
    if ( !exportDocPath.isEmpty() ) {
        exportDocs(exportDocPath);

        return;
    }

    ///if the app is a background project autorun and the project name is empty just throw an exception.
    if ( ( (appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRun) ||
           ( appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRunLaunchedFromGui) ) ) {
        const QString& scriptFilename =  cl.getScriptFilename();

        if ( scriptFilename.isEmpty() ) {
            // cannot start a background process without a file
            throw std::invalid_argument( tr("Project file name is empty.").toStdString() );
        }


        QFileInfo info(scriptFilename);
        if ( !info.exists() ) {
            throw std::invalid_argument( tr("%1: No such file.").arg(scriptFilename).toStdString() );
        }

        std::list<AppInstance::RenderWork> writersWork;


        if ( info.suffix() == QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ) {
            ///Load the project
            if ( !_imp->_currentProject->loadProject( info.path(), info.fileName() ) ) {
                throw std::invalid_argument( tr("Project file loading failed.").toStdString() );
            }
        } else if ( info.suffix() == QString::fromUtf8("py") ) {
            ///Load the python script
            loadPythonScript(info);
        } else {
            throw std::invalid_argument( tr("%1 only accepts python scripts or .ntp project files.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        }

        // exec the python script specified via --onload
        const QString& extraOnProjectCreatedScript = cl.getDefaultOnProjectLoadedScript();
        if ( !extraOnProjectCreatedScript.isEmpty() ) {
            QFileInfo cbInfo(extraOnProjectCreatedScript);
            if ( cbInfo.exists() ) {
                loadPythonScript(cbInfo);
            }
        }


        getWritersWorkForCL(cl, writersWork);


        ///Set reader parameters if specified from the command-line
        const std::list<CLArgs::ReaderArg>& readerArgs = cl.getReaderArgs();
        for (std::list<CLArgs::ReaderArg>::const_iterator it = readerArgs.begin(); it != readerArgs.end(); ++it) {
            std::string readerName = it->name.toStdString();
            NodePtr readNode = getNodeByFullySpecifiedName(readerName);

            if (!readNode) {
                std::string exc( tr("%1 does not belong to the project file. Please enter a valid Read node script-name.").arg( QString::fromUtf8( readerName.c_str() ) ).toStdString() );
                throw std::invalid_argument(exc);
            } else {
                if ( !readNode->getEffectInstance()->isReader() ) {
                    std::string exc( tr("%1 is not a Read node! It cannot render anything.").arg( QString::fromUtf8( readerName.c_str() ) ).toStdString() );
                    throw std::invalid_argument(exc);
                }
            }

            if ( it->filename.isEmpty() ) {
                std::string exc( tr("%1: Filename specified is empty but [-i] or [--reader] was passed to the command-line.").arg( QString::fromUtf8( readerName.c_str() ) ).toStdString() );
                throw std::invalid_argument(exc);
            }
            KnobIPtr fileKnob = readNode->getKnobByName(kOfxImageEffectFileParamName);
            if (fileKnob) {
                KnobFile* outFile = dynamic_cast<KnobFile*>( fileKnob.get() );
                if (outFile) {
                    outFile->setValue( it->filename.toStdString() );
                }
            }
        }

        ///launch renders
        if ( !writersWork.empty() ) {
            startWritersRendering(false, writersWork);
        } else {
            std::list<std::string> writers;
            startWritersRenderingFromNames( cl.areRenderStatsEnabled(), false, writers, cl.getFrameRanges() );
        }
    } else if (appPTR->getAppType() == AppManager::eAppTypeInterpreter) {
        QFileInfo info( cl.getScriptFilename() );
        if ( info.exists() ) {
            if ( info.suffix() == QString::fromUtf8("py") ) {
                loadPythonScript(info);
            } else if ( info.suffix() == QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ) {
                if ( !_imp->_currentProject->loadProject( info.path(), info.fileName() ) ) {
                    throw std::invalid_argument( tr("Project file loading failed.").toStdString() );
                }
            }
        }

        // exec the python script specified via --onload
        const QString& extraOnProjectCreatedScript = cl.getDefaultOnProjectLoadedScript();
        if ( !extraOnProjectCreatedScript.isEmpty() ) {
            QFileInfo cbInfo(extraOnProjectCreatedScript);
            if ( cbInfo.exists() ) {
                loadPythonScript(cbInfo);
            }
        }


        appPTR->launchPythonInterpreter();
    } else {
        execOnProjectCreatedCallback();

        // exec the python script specified via --onload
        const QString& extraOnProjectCreatedScript = cl.getDefaultOnProjectLoadedScript();
        if ( !extraOnProjectCreatedScript.isEmpty() ) {
            QFileInfo cbInfo(extraOnProjectCreatedScript);
            if ( cbInfo.exists() ) {
                loadPythonScript(cbInfo);
            }
        }
    }
} // AppInstance::load

bool
AppInstance::loadPythonScript(const QFileInfo& file)
{
    std::string addToPythonPath("sys.path.append(\"");

    addToPythonPath += file.path().toStdString();
    addToPythonPath += "\")\n";

    std::string err;
    bool ok  = NATRON_PYTHON_NAMESPACE::interpretPythonScript(addToPythonPath, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::loadPythonScript(" + file.path().toStdString() + "): interpretPythonScript(" + addToPythonPath + " failed!");
    }

    std::string s = "app = app1\n";
    ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(s, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::loadPythonScript(" + file.path().toStdString() + "): interpretPythonScript(" + s + " failed!");
    }

    QFile f( file.absoluteFilePath() );
    if ( !f.open(QIODevice::ReadOnly) ) {
        return false;
    }
    QTextStream ts(&f);
    QString content = ts.readAll();
    bool hasCreateInstance = content.contains( QString::fromUtf8("def createInstance") );
    /*
       The old way of doing it was

        QString hasCreateInstanceScript = QString("import sys\n"
        "import %1\n"
        "ret = True\n"
        "if not hasattr(%1,\"createInstance\") or not hasattr(%1.createInstance,\"__call__\"):\n"
        "    ret = False\n").arg(filename);


        ok = interpretPythonScript(hasCreateInstanceScript.toStdString(), &err, 0);


       which is wrong because it will try to import the script first.
       But we in the case of regular scripts, we allow the user to access externally declared variables such as "app", "app1" etc...
       and this would not be possible if the script was imported. Importing the module would then fail because it could not
       find the variables and the script could not be executed.
     */

    if (hasCreateInstance) {
        QString moduleName = file.fileName();
        int lastDotPos = moduleName.lastIndexOf( QChar::fromLatin1('.') );
        if (lastDotPos != -1) {
            moduleName = moduleName.left(lastDotPos);
        }

        std::stringstream ss;
        ss << "import " << moduleName.toStdString() << '\n';
        ss << moduleName.toStdString() << ".createInstance(app,app)";

        std::string output;
        FlagIncrementer flag(&_imp->_creatingGroup, &_imp->creatingGroupMutex);
        CreatingNodeTreeFlag_RAII createNodeTree( shared_from_this() );
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(ss.str(), &err, &output) ) {
            if ( !err.empty() ) {
                Dialogs::errorDialog(tr("Python").toStdString(), err);
            }

            return false;
        } else {
            if ( !output.empty() ) {
                if ( appPTR->isBackground() ) {
                    std::cout << output << std::endl;
                } else {
                    appendToScriptEditor(output);
                }
            }
        }

        getProject()->forceComputeInputDependentDataOnAllTrees();
    } else {
        PythonGILLocker pgl;

        PyRun_SimpleString( content.toStdString().c_str() );

        PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
        std::string error;
        ///Gui session, do stdout, stderr redirection
        PyObject *errCatcher = 0;

        if ( PyObject_HasAttrString(mainModule, "catchErr") ) {
            errCatcher = PyObject_GetAttrString(mainModule, "catchErr"); //get our catchOutErr created above, new ref
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

        if ( !error.empty() ) {
            QString message( QString::fromUtf8("Failed to load ") );
            message.append( file.absoluteFilePath() );
            message.append( QString::fromUtf8(": ") );
            message.append( QString::fromUtf8( error.c_str() ) );
            appendToScriptEditor( message.toStdString() );
        }
    }

    return true;
} // AppInstance::loadPythonScript

class AddCreateNode_RAII
{
    AppInstancePrivate* _imp;
    NodePtr _node;

public:


    AddCreateNode_RAII(AppInstancePrivate* imp,
                       const NodePtr& node)
        : _imp(imp)
        , _node(node)
    {
        if (node) {
            _imp->_creatingNodeQueue.push_back(node);
        }
    }

    virtual ~AddCreateNode_RAII()
    {
        std::list<NodePtr>::iterator found = std::find(_imp->_creatingNodeQueue.begin(), _imp->_creatingNodeQueue.end(), _node);

        if ( found != _imp->_creatingNodeQueue.end() ) {
            _imp->_creatingNodeQueue.erase(found);
        }
    }
};

NodePtr
AppInstance::createNodeFromPythonModule(Plugin* plugin,
                                        const CreateNodeArgs& args)

{
    /*If the plug-in is a toolset, execute the toolset script and don't actually create a node*/
    bool istoolsetScript = plugin->getToolsetScript();
    NodePtr node;

    NodeSerializationPtr serialization = args.getProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    NodeCollectionPtr group = args.getProperty<NodeCollectionPtr>(kCreateNodeArgsPropGroupContainer);
    {
        FlagIncrementer fs(&_imp->_creatingGroup, &_imp->creatingGroupMutex);
        if (_imp->_creatingGroup == 1) {
            bool createGui = !args.getProperty<bool>(kCreateNodeArgsPropNoNodeGUI) && !args.getProperty<bool>(kCreateNodeArgsPropOutOfProject);
            _imp->_creatingInternalNode = !createGui;
        }
        CreatingNodeTreeFlag_RAII createNodeTree( shared_from_this() );
        NodePtr containerNode;
        if (!istoolsetScript) {
            CreateNodeArgs groupArgs = args;
            groupArgs.setProperty<std::string>(kCreateNodeArgsPropPluginID,PLUGINID_NATRON_GROUP);
            groupArgs.setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, true);
            containerNode = createNode(groupArgs);
            if (!containerNode) {
                return containerNode;
            }

            if (!serialization && args.getProperty<std::string>(kCreateNodeArgsPropNodeInitialName).empty()) {
                std::string containerName;
                try {
                    if (group) {
                        group->initNodeName(plugin->getLabelWithoutSuffix().toStdString(), &containerName);
                    }
                    containerNode->setScriptName(containerName);
                    containerNode->setLabel(containerName);
                } catch (...) {
                }
            }
        }

        AddCreateNode_RAII creatingNode_raii(_imp.get(), containerNode);
        std::string containerFullySpecifiedName;
        if (containerNode) {
            containerFullySpecifiedName = containerNode->getFullyQualifiedName();
        }


        QString moduleName;
        QString modulePath;
        plugin->getPythonModuleNameAndPath(&moduleName, &modulePath);

        if ( containerNode && !moduleName.isEmpty() ) {
            setGroupLabelIDAndVersion(containerNode, modulePath, moduleName);
        }

        int appID = getAppID() + 1;
        std::stringstream ss;
        ss << moduleName.toStdString();
        ss << ".createInstance(app" << appID;
        if (istoolsetScript) {
            ss << ",\"\"";
        } else {
            ss << ", app" << appID << "." << containerFullySpecifiedName;
        }
        ss << ")\n";
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(ss.str(), &err, &output) ) {
            Dialogs::errorDialog(tr("Group plugin creation error").toStdString(), err);
            if (containerNode) {
                containerNode->destroyNode(false, false);
            }

            return node;
        } else {
            if ( !output.empty() ) {
                appendToScriptEditor(output);
            }
            node = containerNode;
        }
        if (istoolsetScript) {
            return NodePtr();
        }

        // If there's a serialization, restore the serialization of the group node because the Python script probably overridden any state
        if (serialization) {
            containerNode->loadKnobs(*serialization);
        }
    } //FlagSetter fs(true,&_imp->_creatingGroup,&_imp->creatingGroupMutex);

    ///Now that the group is created and all nodes loaded, autoconnect the group like other nodes.
    bool autoConnect = args.getProperty<bool>(kCreateNodeArgsPropAutoConnect);
    onGroupCreationFinished(node, serialization, autoConnect);

    return node;
} // AppInstance::createNodeFromPythonModule

void
AppInstance::setGroupLabelIDAndVersion(const NodePtr& node,
                                       const QString& pythonModulePath,
                                       const QString &pythonModule)
{
    assert(node);
    if (!node) {
        throw std::logic_error(__func__);
    }
    std::string pluginID, pluginLabel, iconFilePath, pluginGrouping, description;
    unsigned int version;
    bool istoolset;

    if ( NATRON_PYTHON_NAMESPACE::getGroupInfos(pythonModulePath.toStdString(), pythonModule.toStdString(), &pluginID, &pluginLabel, &iconFilePath, &pluginGrouping, &description, &istoolset, &version) ) {
        QString groupingStr = QString::fromUtf8( pluginGrouping.c_str() );
        QStringList groupingSplits = groupingStr.split( QLatin1Char('/') );
        std::list<std::string> stdGrouping;
        for (QStringList::iterator it = groupingSplits.begin(); it != groupingSplits.end(); ++it) {
            stdGrouping.push_back( it->toStdString() );
        }

        node->setPluginIDAndVersionForGui(stdGrouping, pluginLabel, pluginID, description, iconFilePath, version);
        node->setPluginPythonModule( QString( pythonModulePath + pythonModule + QString::fromUtf8(".py") ).toStdString() );
    }
}

NodePtr
AppInstance::createReader(const std::string& filename,
                          CreateNodeArgs& args)
{
    std::string pluginID;

#ifndef NATRON_ENABLE_IO_META_NODES

    std::map<std::string, std::string> readersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    QString fileCpy = QString::fromUtf8( filename.c_str() );
    QString extq = QtCompat::removeFileExtension(fileCpy).toLower();
    std::string ext = extq.toStdString();
    std::map<std::string, std::string>::iterator found = readersForFormat.find(ext);
    if ( found == readersForFormat.end() ) {
        Dialogs::errorDialog( tr("Reader").toStdString(),
                              tr("No plugin capable of decoding %1 was found").arg(extq).toStdString(), false );

        return NodePtr();
    }
    pluginID = found->second;
    CreateNodeArgs args(QString::fromUtf8( found->second.c_str() ), reason, group);
#endif

    args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
    std::string canonicalFilename = filename;
    getProject()->canonicalizePath(canonicalFilename);

    int firstFrame, lastFrame;
    Node::getOriginalFrameRangeForReader(pluginID, canonicalFilename, &firstFrame, &lastFrame);
    std::vector<int> originalRange(2);
    originalRange[0] = firstFrame;
    originalRange[1] = lastFrame;
    args.addParamDefaultValueN<int>(kReaderParamNameOriginalFrameRange, originalRange);

    return createNode(args);
}

NodePtr
AppInstance::createWriter(const std::string& filename,
                          CreateNodeArgs& args,
                          int firstFrame,
                          int lastFrame)
{
#ifndef NATRON_ENABLE_IO_META_NODES
    std::map<std::string, std::string> writersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);

    QString fileCpy = QString::fromUtf8( filename.c_str() );
    QString extq = QtCompat::removeFileExtension(fileCpy).toLower();
    std::string ext = extq.toStdString();
    std::map<std::string, std::string>::iterator found = writersForFormat.find(ext);
    if ( found == writersForFormat.end() ) {
        Dialogs::errorDialog( tr("Writer").toStdString(),
                              tr("No plugin capable of encoding %1 was found.").arg(extq).toStdString(), false );

        return NodePtr();
    }


    CreateNodeArgs args(QString::fromUtf8( found->second.c_str() ), reason, collection);
#endif
    args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
    if ( (firstFrame != INT_MIN) && (lastFrame != INT_MAX) ) {
        args.addParamDefaultValue<int>("frameRange", 2);
        args.addParamDefaultValue<int>("firstFrame", firstFrame);
        args.addParamDefaultValue<int>("lastFrame", lastFrame);
    }

    return createNode(args);
}

static std::string removeTrailingDigits(const std::string& str)
{
    if (str.empty()) {
        return std::string();
    }
    std::size_t i = str.size() - 1;
    while (i > 0 && std::isdigit(str[i])) {
        --i;
    }

    if (i == 0) {
        // Name only consists of digits
        return std::string();
    }

    return str.substr(0, i + 1);
}

/**
 * @brief An inspector node is like a viewer node with hidden inputs that unfolds one after another.
 * This functions returns the number of inputs to use for inspectors or 0 for a regular node.
 **/
static bool
isEntitledForInspector(Plugin* plugin,
                       OFX::Host::ImageEffect::Descriptor* ofxDesc)
{
    if ( ( plugin->getPluginID() == QString::fromUtf8(PLUGINID_NATRON_VIEWER) ) ||
         ( plugin->getPluginID() == QString::fromUtf8(PLUGINID_NATRON_ROTOPAINT) ) ||
         ( plugin->getPluginID() == QString::fromUtf8(PLUGINID_NATRON_ROTO) ) ) {
        return true;
    }

    if (!ofxDesc) {
        return false;
    }

    // Find the number of inputs that share the same basename

    int nInputsWithSameBasename = 0;

    std::string baseInputName;

    // We need a boolean here because the baseInputName may be empty in the case input names only contain numbers
    bool baseInputNameSet = false;

    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*>& clips = ofxDesc->getClipsByOrder();
    int nInputs = 0;
    for (std::vector<OFX::Host::ImageEffect::ClipDescriptor*>::const_iterator it = clips.begin(); it != clips.end(); ++it) {
        if ( !(*it)->isOutput() ) {
            ++nInputs;
            if (!baseInputNameSet) {
                baseInputName = removeTrailingDigits((*it)->getName());
                baseInputNameSet = true;
                nInputsWithSameBasename = 1;
            } else {
                std::string thisBaseName = removeTrailingDigits((*it)->getName());
                if (thisBaseName == baseInputName) {
                    ++nInputsWithSameBasename;
                }
            }
        }
    }
    return nInputs > 4 && nInputsWithSameBasename >= 4;

}

NodePtr
AppInstance::createNodeInternal(CreateNodeArgs& args)
{
    NodePtr node;
    Plugin* plugin = 0;
    QString findId;

    NodeSerializationPtr serialization = args.getProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    bool trustPluginID = args.getProperty<bool>(kCreateNodeArgsPropTrustPluginID);
    QString argsPluginID = QString::fromUtf8(args.getProperty<std::string>(kCreateNodeArgsPropPluginID).c_str());
    int versionMajor = args.getProperty<int>(kCreateNodeArgsPropPluginVersion, 0);
    int versionMinor = args.getProperty<int>(kCreateNodeArgsPropPluginVersion, 1);

    //Roto has moved to a built-in plugin
    if ( (!trustPluginID || serialization) &&
         ( ( !_imp->_projectCreatedWithLowerCaseIDs && ( argsPluginID == QString::fromUtf8(PLUGINID_OFX_ROTO) ) ) || ( _imp->_projectCreatedWithLowerCaseIDs && ( argsPluginID == QString::fromUtf8(PLUGINID_OFX_ROTO).toLower() ) ) ) ) {
        findId = QString::fromUtf8(PLUGINID_NATRON_ROTO);
    } else {
        findId = argsPluginID;
    }

    bool isSilentCreation = args.getProperty<bool>(kCreateNodeArgsPropSilent);


#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr argsIOContainer = args.getProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer);
    //If it is a reader or writer, create a ReadNode or WriteNode
    if (!argsIOContainer) {
        if ( ReadNode::isBundledReader( argsPluginID.toStdString(), wasProjectCreatedWithLowerCaseIDs() ) ) {
            args.addParamDefaultValue(kNatronReadNodeParamDecodingPluginID, argsPluginID.toStdString());
            findId = QString::fromUtf8(PLUGINID_NATRON_READ);
        } else if ( WriteNode::isBundledWriter( argsPluginID.toStdString(), wasProjectCreatedWithLowerCaseIDs() ) ) {
            args.addParamDefaultValue(kNatronWriteNodeParamEncodingPluginID, argsPluginID.toStdString());
            findId = QString::fromUtf8(PLUGINID_NATRON_WRITE);
        }
    }
#endif

    try {
        plugin = appPTR->getPluginBinary(findId, versionMajor, versionMinor, _imp->_projectCreatedWithLowerCaseIDs && serialization);
    } catch (const std::exception & e1) {
        ///Ok try with the old Ids we had in Natron prior to 1.0
        try {
            plugin = appPTR->getPluginBinaryFromOldID(argsPluginID, versionMajor, versionMinor);
        } catch (const std::exception& e2) {
            if (!isSilentCreation) {
                Dialogs::errorDialog(tr("Plugin error").toStdString(),
                                 tr("Cannot load plug-in executable.").toStdString() + ": " + e2.what(), false );
            } else {
                std::cerr << tr("Cannot load plug-in executable.").toStdString() + ": " + e2.what() << std::endl;
            }
            return node;
        }
    }

    if (!plugin) {
        return node;
    }

    bool allowUserCreatablePlugins = args.getProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins);
    if ( !plugin->getIsUserCreatable() && !allowUserCreatablePlugins ) {
        //The plug-in should not be instantiable by the user
        qDebug() << "Attempt to create" << argsPluginID << "which is not user creatable";

        return node;
    }


    /*
       If the plug-in is a PyPlug create it with createNodeFromPythonModule() except in the following situations:
       - The user speicifed in the Preferences that he/she prefers loading PyPlugs from their serialization in the .ntp
       file rather than load the Python script
       - The user is copy/pasting in which case we don't want to run the Python code which could override modifications
       made by the user on the original node
     */
    const QString& pythonModule = plugin->getPythonModule();
    if ( !pythonModule.isEmpty() ) {
        bool disableLoadFromScript = args.getProperty<bool>(kCreateNodeArgsPropDoNotLoadPyPlugFromScript);
        if (!disableLoadFromScript) {
            try {
                return createNodeFromPythonModule(plugin, args);
            } catch (const std::exception& e) {
                if (!isSilentCreation) {
                    Dialogs::errorDialog(tr("Plugin error").toStdString(),
                                     tr("Cannot create PyPlug:").toStdString() + e.what(), false );
                } else {
                    std::cerr << tr("Cannot create PyPlug").toStdString() + ": " + e.what() << std::endl;
                }
                return node;
            }
        } else {
            plugin = appPTR->getPluginBinary(QString::fromUtf8(PLUGINID_NATRON_GROUP), -1, -1, false);
            assert(plugin);
        }
    }

    std::string foundPluginID = plugin->getPluginID().toStdString();
    ContextEnum ctx;
    OFX::Host::ImageEffect::Descriptor* ofxDesc = plugin->getOfxDesc(&ctx);

    if (!ofxDesc) {
        OFX::Host::ImageEffect::ImageEffectPlugin* ofxPlugin = plugin->getOfxPlugin();
        if (ofxPlugin) {
            try {
                //  Should this method be in AppManager?
                // ofxDesc = appPTR->getPluginContextAndDescribe(ofxPlugin, &ctx);
                ofxDesc = appPTR->getPluginContextAndDescribe(ofxPlugin, &ctx);
            } catch (const std::exception& e) {
                if (!isSilentCreation) {
                    errorDialog(tr("Error while creating node").toStdString(), tr("Failed to create an instance of %1:").arg(argsPluginID).toStdString()
                            + '\n' + e.what(), false);
                } else {
                    std::cerr << tr("Failed to create an instance of %1:").arg(argsPluginID).toStdString() + '\n' + e.what() << std::endl;
                }
                return NodePtr();
            }

            assert(ofxDesc);
            plugin->setOfxDesc(ofxDesc, ctx);
        }
    }

    NodeCollectionPtr argsGroup = args.getProperty<NodeCollectionPtr>(kCreateNodeArgsPropGroupContainer);
    if (!argsGroup) {
        argsGroup = getProject();
    }

    bool useInspector = isEntitledForInspector(plugin, ofxDesc);

    if (!useInspector) {
        node = boost::make_shared<Node>(shared_from_this(), argsGroup, plugin);
    } else {
        node = boost::make_shared<InspectorNode>(shared_from_this(), argsGroup, plugin);
    }




    AddCreateNode_RAII creatingNode_raii(_imp.get(), node);

    {
        ///Furnace plug-ins don't handle using the thread pool
        SettingsPtr settings = appPTR->getCurrentSettings();
        if ( !isSilentCreation && boost::starts_with(foundPluginID, "uk.co.thefoundry.furnace") &&
             ( settings->useGlobalThreadPool() || ( settings->getNumberOfParallelRenders() != 1) ) ) {
            StandardButtonEnum reply = Dialogs::questionDialog(tr("Warning").toStdString(),
                                                               tr("The settings of the application are currently set to use "
                                                                  "the global thread-pool for rendering effects.\n"
                                                                  "The Foundry Furnace "
                                                                  "is known not to work well when this setting is checked.\n"
                                                                  "Would you like to turn it off?").toStdString(), false);
            if (reply == eStandardButtonYes) {
                settings->setUseGlobalThreadPool(false);
                settings->setNumberOfParallelRenders(1);
            }
        }

        // If this is a stereo plug-in, check that the project has been set for multi-view
        if (!isSilentCreation) {
            const QStringList& grouping = plugin->getGrouping();
            if ( !grouping.isEmpty() && ( grouping[0] == QString::fromUtf8(PLUGIN_GROUP_MULTIVIEW) ) ) {
                int nbViews = getProject()->getProjectViewsCount();
                if (nbViews < 2) {
                    StandardButtonEnum reply = Dialogs::questionDialog(tr("Multi-View").toStdString(),
                                                                       tr("Using a multi-view node requires the project settings to be setup "
                                                                          "for multi-view.\n"
                                                                          "Would you like to setup the project for stereo?").toStdString(), false);
                    if (reply == eStandardButtonYes) {
                        getProject()->setupProjectForStereo();
                    }
                }
            }
        }
    }


    //if (args.addToProject) {
    //Add the node to the project before loading it so it is present when the python script that registers a variable of the name
    //of the node works
    if (argsGroup) {
        argsGroup->addNode(node);
    }
    //}
    assert(node);
    try {
        node->load(args);
    } catch (const std::exception & e) {
        if (argsGroup) {
            argsGroup->removeNode(node.get());
        }
        std::string error( e.what() );
        if ( !error.empty() ) {
            std::string title("Error while creating node");
            std::string message = title + " " + foundPluginID + ": " + e.what();
            std::cerr << message.c_str() << std::endl;
            if (!isSilentCreation) {
                errorDialog(title, message, false);
            }
        }

        return NodePtr();
    }

    NodePtr multiInstanceParent = node->getParentMultiInstance();


    // _imp->_creatingInternalNode will be set to false if we created a pyPlug with the flag args.createGui = false
    bool createGui = !_imp->_creatingInternalNode;
    bool argsNoNodeGui = args.getProperty<bool>(kCreateNodeArgsPropNoNodeGUI);
    if (argsNoNodeGui) {
        createGui = false;
    }
    if (createGui) {
        try {
            createNodeGui(node, multiInstanceParent, args);
        } catch (const std::exception& e) {
            node->destroyNode(true, false);
            if (!isSilentCreation) {
                std::string title("Error while creating node");
                std::string message = title + " " + foundPluginID + ": " + e.what();
                qDebug() << message.c_str();
                errorDialog(title, message, false);
            }
            return NodePtr();
        }
    }

    NodeGroupPtr isGrp = boost::dynamic_pointer_cast<NodeGroup>( node->getEffectInstance()->shared_from_this() );

    if (isGrp) {
        bool autoConnect = args.getProperty<bool>(kCreateNodeArgsPropAutoConnect);
        bool createInitialNodes = !args.getProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes);
        if (serialization) {
            if ( serialization && !serialization->getPythonModule().empty() ) {
                QString pythonModulePath = QString::fromUtf8( ( serialization->getPythonModule().c_str() ) );
                QString moduleName;
                QString modulePath;
                int foundLastSlash = pythonModulePath.lastIndexOf( QChar::fromLatin1('/') );
                if (foundLastSlash != -1) {
                    modulePath = pythonModulePath.mid(0, foundLastSlash + 1);
                    moduleName = pythonModulePath.remove(0, foundLastSlash + 1);
                }
                QtCompat::removeFileExtension(moduleName);
                setGroupLabelIDAndVersion(node, modulePath, moduleName);
            }
            onGroupCreationFinished(node, serialization, autoConnect);
        } else if ( createInitialNodes && !serialization  && createGui && !_imp->_creatingGroup && (isGrp->getPluginID() == PLUGINID_NATRON_GROUP) ) {
            //if the node is a group and we're not loading the project, create one input and one output
            NodePtr input, output;

            {
                CreateNodeArgs args(PLUGINID_NATRON_OUTPUT, isGrp);
                args.setProperty(kCreateNodeArgsPropAutoConnect, false);
                args.setProperty(kCreateNodeArgsPropAddUndoRedoCommand, false);
                args.setProperty(kCreateNodeArgsPropSettingsOpened, false);
                output = createNode(args);
                try {
                    output->setLabel("Output");
                } catch (...) {
                }

                assert(output);
            }
            {
                CreateNodeArgs args(PLUGINID_NATRON_INPUT, isGrp);
                args.setProperty(kCreateNodeArgsPropAutoConnect, false);
                args.setProperty(kCreateNodeArgsPropAddUndoRedoCommand, false);
                args.setProperty(kCreateNodeArgsPropSettingsOpened, false);
                input = createNode(args);
                assert(input);
            }
            if ( input && output && !output->getInput(0) ) {
                output->connectInput(input, 0);

                double x, y;
                output->getPosition(&x, &y);
                y -= 100;
                input->setPosition(x, y);
            }
            onGroupCreationFinished(node, serialization, autoConnect);

            ///Now that the group is created and all nodes loaded, autoconnect the group like other nodes.
        }
    }

    return node;
} // createNodeInternal

NodePtr
AppInstance::createNode(CreateNodeArgs & args)
{
    return createNodeInternal(args);
}

int
AppInstance::getAppID() const
{
    return _imp->_appID;
}

void
AppInstance::exportDocs(const QString path)
{
    if ( !path.isEmpty() ) {
        QStringList groups;
        // Group ordering is set at every place in the code where GROUP_ORDER appears in the comments
        groups << QString::fromUtf8(PLUGIN_GROUP_IMAGE);
        groups << QString::fromUtf8(PLUGIN_GROUP_PAINT);
        groups << QString::fromUtf8(PLUGIN_GROUP_TIME);
        groups << QString::fromUtf8(PLUGIN_GROUP_CHANNEL);
        groups << QString::fromUtf8(PLUGIN_GROUP_COLOR);
        groups << QString::fromUtf8(PLUGIN_GROUP_FILTER);
        groups << QString::fromUtf8(PLUGIN_GROUP_KEYER);
        groups << QString::fromUtf8(PLUGIN_GROUP_MERGE);
        groups << QString::fromUtf8(PLUGIN_GROUP_TRANSFORM);
        groups << QString::fromUtf8(PLUGIN_GROUP_MULTIVIEW);
        groups << QString::fromUtf8(PLUGIN_GROUP_OTHER);
        groups << QString::fromUtf8("GMIC"); // openfx-gmic
        groups << QString::fromUtf8("Extra"); // openfx-arena
        QVector<QStringList> plugins;

        // Generate a MD for each plugin
        // IMPORTANT: this code is *very* similar to DocumentationManager::handler(...) is section "_group.html"
        std::list<std::string> pluginIDs = appPTR->getPluginIDs();
        for (std::list<std::string>::iterator it = pluginIDs.begin(); it != pluginIDs.end(); ++it) {
            QString pluginID = QString::fromUtf8( it->c_str() );
            if ( !pluginID.isEmpty() ) {
                Plugin* plugin = 0;
                QString pluginID = QString::fromUtf8( it->c_str() );
                plugin = appPTR->getPluginBinary(pluginID, -1, -1, false);
                if ( plugin && !plugin->getIsDeprecated() && ( !plugin->getIsForInternalUseOnly() || plugin->isReader() || plugin->isWriter() ) ) {
                    QStringList groups = plugin->getGrouping();
                    groups << groups.at(0);
                    QStringList plugList;
                    plugList << plugin->getGrouping().at(0) << pluginID << Plugin::makeLabelWithoutSuffix( plugin->getPluginLabel() );
                    plugins << plugList;
                    CreateNodeArgs args( pluginID.toStdString(), NodeCollectionPtr() );
                    args.setProperty(kCreateNodeArgsPropNoNodeGUI, true);
                    args.setProperty(kCreateNodeArgsPropOutOfProject, true);
                    args.setProperty(kCreateNodeArgsPropSilent, true);
                    qDebug() << pluginID;
                    // IMPORTANT: this code is *very* similar to DocumentationManager::handler(...) is section "_plugin.html"
                    NodePtr node = createNode(args);
                    if ( node &&
                         pluginID != QString::fromUtf8(PLUGINID_NATRON_READ) &&
                         pluginID != QString::fromUtf8(PLUGINID_NATRON_WRITE) ) {
                        EffectInstancePtr effectInstance = node->getEffectInstance();
                        if ( effectInstance && effectInstance->isReader() ) {
                            ReadNode* isReadNode = dynamic_cast<ReadNode*>( effectInstance.get() );

                            if (isReadNode) {
                                NodePtr subnode = isReadNode->getEmbeddedReader();
                                if (subnode) {
                                    node = subnode;
                                }
                            }
                        } else if ( effectInstance && effectInstance->isWriter() ) {
                            WriteNode* isWriteNode = dynamic_cast<WriteNode*>( effectInstance.get() );

                            if (isWriteNode) {
                                NodePtr subnode = isWriteNode->getEmbeddedWriter();
                                if (subnode) {
                                    node = subnode;
                                }
                            }
                        }
                    }
                    if (node) {
                        QDir mdDir(path);
                        if ( !mdDir.exists() ) {
                            mdDir.mkpath(path);
                        }

                        mdDir.mkdir(QLatin1String("plugins"));
                        mdDir.cd(QLatin1String("plugins"));

                        QFile imgFile( plugin->getIconFilePath() );
                        if ( imgFile.exists() ) {
                            QString dstPath = mdDir.absolutePath() + QString::fromUtf8("/") + pluginID + QString::fromUtf8(".png");
                            if (QFile::exists(dstPath)) {
                                QFile::remove(dstPath);
                            }
                            if ( !imgFile.copy(dstPath) ) {
                                std::cout << "ERROR: failed to copy image: " << imgFile.fileName().toStdString() << std::endl;
                            }
                        }

                        QString md = node->makeDocumentation(false);
                        QFile mdFile( mdDir.absolutePath() + QString::fromUtf8("/") + pluginID + QString::fromUtf8(".md") );
                        if ( mdFile.open(QIODevice::Text | QIODevice::WriteOnly) ) {
                            QTextStream out(&mdFile);
                            out << md;
                            mdFile.close();
                        } else {
                            std::cout << "ERROR: failed to write to file: " << mdFile.fileName().toStdString() << std::endl;
                        }
                    }
                }
            }
        }

        // Generate RST for plugin groups
        // IMPORTANT: this code is *very* similar to DocumentationManager::handler in the "_group.html" section
        groups.removeDuplicates();
        QString groupMD;
        groupMD.append( QString::fromUtf8(".. ") + tr("Do not edit this file! It is generated automatically by %1 itself.").arg ( QString::fromUtf8( NATRON_APPLICATION_NAME) ) + QString::fromUtf8("\n\n") );
        groupMD.append( QString::fromUtf8(".. _reference-guide:\n\n") );
        groupMD.append( tr("Reference Guide") );
        groupMD.append( QString::fromUtf8("\n============================================================\n\n") );
        groupMD.append( tr("The first section in this manual describes the various options available from the %1 preference settings. The next section gives the documentation for the various environment variables that may be used to control %1's behavior. It is followed by one section for each node group in %1.")
                       .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) + QLatin1Char(' ') + tr("Node groups are available by clicking on buttons in the left toolbar, or by right-clicking the mouse in the Node Graph area.") /*+ QLatin1Char(' ') + tr("Please note that documentation is also generated automatically for third-party OpenFX plugins.")*/ );
        groupMD.append( QString::fromUtf8("\n\n") );
        //groupMD.append( QString::fromUtf8("Contents:\n\n") );
        groupMD.append( QString::fromUtf8(".. toctree::\n") );
        groupMD.append( QString::fromUtf8("    :maxdepth: 1\n\n") );
        groupMD.append( QString::fromUtf8("    _prefs.rst\n") );
        groupMD.append( QString::fromUtf8("    _environment.rst\n") );

        Q_FOREACH(const QString &group, groups) {
            QString plugMD;

            plugMD.append( QString::fromUtf8(".. ") + tr("Do not edit this file! It is generated automatically by %1 itself.").arg ( QString::fromUtf8( NATRON_APPLICATION_NAME) ) + QString::fromUtf8("\n\n") );
            plugMD.append( tr("%1 nodes").arg( tr( group.toUtf8().constData() ) ) );
            plugMD.append( QString::fromUtf8("\n============================================================\n\n") );
            plugMD.append( tr("The following sections contain documentation about every node in the %1 group.").arg( tr( group.toUtf8().constData() ) ) + QLatin1Char(' ') + tr("Node groups are available by clicking on buttons in the left toolbar, or by right-clicking the mouse in the Node Graph area.") /*+ QLatin1Char(' ') + tr("Please note that documentation is also generated automatically for third-party OpenFX plugins.")*/ );
            plugMD.append( QString::fromUtf8("\n\n") );
            //plugMD.append( QString::fromUtf8("Contents:\n\n") );
            plugMD.append( QString::fromUtf8(".. toctree::\n") );
            plugMD.append( QString::fromUtf8("    :maxdepth: 1\n\n") );

            QMap<QString, QString> pluginsOrderedByLabel; // use a map so that it gets sorted by label
            Q_FOREACH(const QStringList &currPlugin, plugins) {
                if (currPlugin.size() == 3) {
                    if ( group == currPlugin.at(0) ) {
                        pluginsOrderedByLabel[currPlugin.at(2)] = currPlugin.at(1);
                    }
                }
            }
            for (QMap<QString, QString>::const_iterator i = pluginsOrderedByLabel.constBegin();
                 i != pluginsOrderedByLabel.constEnd();
                 ++i) {
                const QString& plugID = i.value();
                //const QString& plugName = i.key();
                plugMD.append( QString::fromUtf8("    plugins/") + plugID + QString::fromUtf8(".rst\n") );
            }

            groupMD.append( QString::fromUtf8("    _group") + group + QString::fromUtf8(".rst\n") );

            QFile plugFile( path + QString::fromUtf8("/_group") + group + QString::fromUtf8(".rst") );
            plugMD.append( QString::fromUtf8("\n") );
            if ( plugFile.open(QIODevice::Text | QIODevice::WriteOnly | QIODevice::Truncate) ) {
                QTextStream out(&plugFile);
                out << plugMD;
                plugFile.close();
            }
        }

        // Generate RST for plugins ToC
        QFile groupFile( path + QString::fromUtf8("/_group.rst") );
        groupMD.append( QString::fromUtf8("\n") );
        if ( groupFile.open(QIODevice::Text | QIODevice::WriteOnly | QIODevice::Truncate) ) {
            QTextStream out(&groupFile);
            out << groupMD;
            groupFile.close();
        }

        // Generate MD for settings
        SettingsPtr settings = appPTR->getCurrentSettings();
        QString prefsMD = settings->makeHTMLDocumentation(false);
        QFile prefsFile( path + QString::fromUtf8("/_prefs.md") );
        if ( prefsFile.open(QIODevice::Text | QIODevice::WriteOnly | QIODevice::Truncate) ) {
            QTextStream out(&prefsFile);
            out << prefsMD;
            prefsFile.close();
        }
    }
} // AppInstance::exportDocs

NodePtr
AppInstance::getNodeByFullySpecifiedName(const std::string & name) const
{
    return _imp->_currentProject->getNodeByFullySpecifiedName(name);
}

ProjectPtr
AppInstance::getProject() const
{
    return _imp->_currentProject;
}

TimeLinePtr
AppInstance::getTimeLine() const
{
    return _imp->_currentProject->getTimeLine();
}

void
AppInstance::errorDialog(const std::string & title,
                         const std::string & message,
                         bool /*useHtml*/) const
{
    std::cout << "ERROR: " << title + ": " << message << std::endl;
}

void
AppInstance::errorDialog(const std::string & title,
                         const std::string & message,
                         bool* stopAsking,
                         bool /*useHtml*/) const
{
    std::cout << "ERROR: " << title + ": " << message << std::endl;

    *stopAsking = false;
}

void
AppInstance::warningDialog(const std::string & title,
                           const std::string & message,
                           bool /*useHtml*/) const
{
    std::cout << "WARNING: " << title + ": " << message << std::endl;
}

void
AppInstance::warningDialog(const std::string & title,
                           const std::string & message,
                           bool* stopAsking,
                           bool /*useHtml*/) const
{
    std::cout << "WARNING: " << title + ": " << message << std::endl;

    *stopAsking = false;
}

void
AppInstance::informationDialog(const std::string & title,
                               const std::string & message,
                               bool /*useHtml*/) const
{
    std::cout << "INFO: " << title + ": " << message << std::endl;
}

void
AppInstance::informationDialog(const std::string & title,
                               const std::string & message,
                               bool* stopAsking,
                               bool /*useHtml*/) const
{
    std::cout << "INFO: " << title + ": " << message << std::endl;

    *stopAsking = false;
}

StandardButtonEnum
AppInstance::questionDialog(const std::string & title,
                            const std::string & message,
                            bool /*useHtml*/,
                            StandardButtons /*buttons*/,
                            StandardButtonEnum /*defaultButton*/) const
{
    std::cout << "QUESTION: " << title + ": " << message << std::endl;

    return eStandardButtonYes;
}

void
AppInstance::triggerAutoSave()
{
    _imp->_currentProject->triggerAutoSave();
}

void
AppInstance::startWritersRenderingFromNames(bool enableRenderStats,
                                            bool doBlockingRender,
                                            const std::list<std::string>& writers,
                                            const std::list<std::pair<int, std::pair<int, int> > >& frameRanges)
{
    std::list<RenderWork> renderers;

    if ( !writers.empty() ) {
        for (std::list<std::string>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
            const std::string& writerName = *it;
            NodePtr node = getNodeByFullySpecifiedName(writerName);

            if (!node) {
                std::string exc(writerName);
                exc.append( tr(" does not belong to the project file. Please enter a valid Write node script-name.").toStdString() );
                throw std::invalid_argument(exc);
            } else {
                if ( !node->isOutputNode() ) {
                    std::string exc(writerName);
                    exc.append( tr(" is not an output node! It cannot render anything.").toStdString() );
                    throw std::invalid_argument(exc);
                }
                ViewerInstance* isViewer = node->isEffectViewer();
                if (isViewer) {
                    throw std::invalid_argument("Internal issue with the project loader...viewers should have been evicted from the project.");
                }

                if (node->isNodeDisabled() || !node->isActivated()) {
                    continue;
                }
                OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
                assert(effect);

                for (std::list<std::pair<int, std::pair<int, int> > >::const_iterator it2 = frameRanges.begin(); it2 != frameRanges.end(); ++it2) {
                    RenderWork w(effect, it2->second.first, it2->second.second, it2->first, enableRenderStats);
                    renderers.push_back(w);
                }

                if ( frameRanges.empty() ) {
                    RenderWork r(effect, INT_MIN, INT_MAX, INT_MIN, enableRenderStats);
                    renderers.push_back(r);
                }
            }
        }
    } else {
        //start rendering for all writers found in the project
        std::list<OutputEffectInstance*> writers;
        getProject()->getWriters(&writers);

        for (std::list<OutputEffectInstance*>::const_iterator it2 = writers.begin(); it2 != writers.end(); ++it2) {
            assert(*it2);
            if (*it2) {

                if ((*it2)->getNode()->isNodeDisabled() || !(*it2)->getNode()->isActivated()) {
                    continue;
                }

                for (std::list<std::pair<int, std::pair<int, int> > >::const_iterator it3 = frameRanges.begin(); it3 != frameRanges.end(); ++it3) {
                    RenderWork w(*it2, it3->second.first, it3->second.second, it3->first, enableRenderStats);
                    renderers.push_back(w);
                }

                if ( frameRanges.empty() ) {
                    RenderWork r(*it2, INT_MIN, INT_MAX, INT_MIN, enableRenderStats);
                    renderers.push_back(r);
                }
            }
        }
    }


    if ( renderers.empty() ) {
        throw std::invalid_argument("Project file is missing a writer node. This project cannot render anything.");
    }

    startWritersRendering(doBlockingRender, renderers);
} // AppInstance::startWritersRenderingFromNames

void
AppInstance::startWritersRendering(bool doBlockingRender,
                                   const std::list<RenderWork>& writers)
{
    if ( writers.empty() ) {
        return;
    }


    bool renderInSeparateProcess = appPTR->getCurrentSettings()->isRenderInSeparatedProcessEnabled();
    QString savePath;
    if (renderInSeparateProcess) {
        getProject()->saveProject_imp(QString(), QString::fromUtf8("RENDER_SAVE.ntp"), true, false, &savePath);
    }

    std::list<RenderQueueItem> itemsToQueue;
    for (std::list<RenderWork>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
        if (it->writer->getNode()->isNodeDisabled() || !it->writer->getNode()->isActivated()) {
            continue;
        }
        RenderQueueItem item;
        item.work = *it;
        if ( !_imp->validateRenderOptions(item.work, &item.work.firstFrame, &item.work.lastFrame, &item.work.frameStep) ) {
            continue;;
        }
        _imp->getSequenceNameFromWriter(it->writer, &item.sequenceName);
        item.savePath = savePath;

        if (renderInSeparateProcess) {
            item.process = boost::make_shared<ProcessHandler>(savePath, item.work.writer);
            QObject::connect( item.process.get(), SIGNAL(processFinished(int)), this, SLOT(onBackgroundRenderProcessFinished()) );
        } else {
            QObject::connect(item.work.writer->getRenderEngine().get(), SIGNAL(renderFinished(int)), this, SLOT(onQueuedRenderFinished(int)), Qt::UniqueConnection);
        }

        bool canPause = !item.work.writer->isVideoWriter();

        if (!it->isRestart) {
            notifyRenderStarted(item.sequenceName, item.work.firstFrame, item.work.lastFrame, item.work.frameStep, canPause, item.work.writer, item.process);
        } else {
            notifyRenderRestarted(item.work.writer, item.process);
        }
        itemsToQueue.push_back(item);
    }
    if ( itemsToQueue.empty() ) {
        return;
    }

    if (appPTR->isBackground() || doBlockingRender) {
        //blocking call, we don't want this function to return pre-maturely, in which case it would kill the app
        QtConcurrent::blockingMap( itemsToQueue, boost::bind(&AppInstancePrivate::startRenderingFullSequence, _imp.get(), true, _1) );
    } else {
        bool isQueuingEnabled = appPTR->getCurrentSettings()->isRenderQueuingEnabled();
        if (isQueuingEnabled) {
            QMutexLocker k(&_imp->renderQueueMutex);
            if ( !_imp->activeRenders.empty() ) {
                _imp->renderQueue.insert( _imp->renderQueue.end(), itemsToQueue.begin(), itemsToQueue.end() );

                return;
            } else {
                std::list<RenderQueueItem>::const_iterator it = itemsToQueue.begin();
                const RenderQueueItem& firstWork = *it;
                ++it;
                for (; it != itemsToQueue.end(); ++it) {
                    _imp->renderQueue.push_back(*it);
                }
                k.unlock();
                _imp->startRenderingFullSequence(false, firstWork);
            }
        } else {
            for (std::list<RenderQueueItem>::const_iterator it = itemsToQueue.begin(); it != itemsToQueue.end(); ++it) {
                _imp->startRenderingFullSequence(false, *it);
            }
        }
    }
} // AppInstance::startWritersRendering

void
AppInstancePrivate::getSequenceNameFromWriter(const OutputEffectInstance* writer,
                                              QString* sequenceName)
{
    ///get the output file knob to get the name of the sequence
    const DiskCacheNode* isDiskCache = dynamic_cast<const DiskCacheNode*>(writer);

    assert(writer);
    if (!writer) {
        throw std::logic_error(__func__);
    }
    if (isDiskCache) {
        *sequenceName = tr("Caching");
    } else {
        *sequenceName = QString();
        KnobIPtr fileKnob = writer->getKnobByName(kOfxImageEffectFileParamName);
        if (fileKnob) {
            KnobStringBase* isString = dynamic_cast<KnobStringBase*>( fileKnob.get() );
            assert(isString);
            if (isString) {
                *sequenceName = QString::fromUtf8( isString->getValue().c_str() );
            }
        }
    }
}

bool
AppInstancePrivate::validateRenderOptions(const AppInstance::RenderWork& w,
                                          int* firstFrame,
                                          int* lastFrame,
                                          int* frameStep)
{
    ///validate the frame range to render
    if ( (w.firstFrame == INT_MIN) || (w.lastFrame == INT_MAX) ) {
        double firstFrameD, lastFrameD;
        w.writer->getFrameRange_public(w.writer->getHash(), &firstFrameD, &lastFrameD, true);
        if ( (firstFrameD == INT_MIN) || (lastFrameD == INT_MAX) ) {
            _publicInterface->getFrameRange(&firstFrameD, &lastFrameD);
        }

        if (firstFrameD > lastFrameD) {
            Dialogs::errorDialog(w.writer->getNode()->getLabel_mt_safe(),
                                 tr("First frame index in the sequence is greater than the last frame index.").toStdString(), false );

            return false;
        }
        *firstFrame = (int)firstFrameD;
        *lastFrame = (int)lastFrameD;
    } else {
        *firstFrame = w.firstFrame;
        *lastFrame = w.lastFrame;
    }

    if ( (w.frameStep == INT_MAX) || (w.frameStep == INT_MIN) ) {
        ///Get the frame step from the frame step parameter of the Writer
        *frameStep = w.writer->getNode()->getFrameStepKnobValue();
    } else {
        *frameStep = std::max(1, w.frameStep);
    }

    return true;
}

void
AppInstancePrivate::startRenderingFullSequence(bool blocking,
                                               const RenderQueueItem& w)
{
    if (blocking) {
        BlockingBackgroundRender backgroundRender(w.work.writer);
        backgroundRender.blockingRender(w.work.useRenderStats, w.work.firstFrame, w.work.lastFrame, w.work.frameStep); //< doesn't return before rendering is finished
        return;
    }


    {
        QMutexLocker k(&renderQueueMutex);
        activeRenders.push_back(w);
    }


    if (w.process) {
        w.process->startProcess();
    } else {
        w.work.writer->renderFullSequence(false, w.work.useRenderStats, NULL, w.work.firstFrame, w.work.lastFrame, w.work.frameStep);
    }
}

void
AppInstance::onQueuedRenderFinished(int /*retCode*/)
{
    RenderEngine* engine = qobject_cast<RenderEngine*>( sender() );

    if (!engine) {
        return;
    }
    OutputEffectInstancePtr effect = engine->getOutput();
    if (!effect) {
        return;
    }
    startNextQueuedRender( effect.get() );
}

void
AppInstance::removeRenderFromQueue(OutputEffectInstance* writer)
{
    QMutexLocker k(&_imp->renderQueueMutex);

    for (std::list<RenderQueueItem>::iterator it = _imp->renderQueue.begin(); it != _imp->renderQueue.end(); ++it) {
        if (it->work.writer == writer) {
            _imp->renderQueue.erase(it);
            break;
        }
    }
}

void
AppInstance::startNextQueuedRender(OutputEffectInstance* finishedWriter)
{
    RenderQueueItem nextWork;

    // Do not make the process die under the mutex otherwise we may deadlock
    ProcessHandlerPtr processDying;
    {
        QMutexLocker k(&_imp->renderQueueMutex);
        for (std::list<RenderQueueItem>::iterator it = _imp->activeRenders.begin(); it != _imp->activeRenders.end(); ++it) {
            if (it->work.writer == finishedWriter) {
                processDying = it->process;
                _imp->activeRenders.erase(it);
                break;
            }
        }
        if ( !_imp->renderQueue.empty() ) {
            nextWork = _imp->renderQueue.front();
            _imp->renderQueue.pop_front();
        } else {
            return;
        }
    }
    processDying.reset();

    _imp->startRenderingFullSequence(false, nextWork);
}

void
AppInstance::onBackgroundRenderProcessFinished()
{
    ProcessHandler* proc = qobject_cast<ProcessHandler*>( sender() );
    OutputEffectInstance* effect = 0;

    if (proc) {
        effect = proc->getWriter();
    }
    if (effect) {
        startNextQueuedRender(effect);
    }
}

void
AppInstance::getFrameRange(double* first,
                           double* last) const
{
    return _imp->_currentProject->getFrameRange(first, last);
}

void
AppInstance::clearOpenFXPluginsCaches()
{
    NodesList activeNodes;

    _imp->_currentProject->getActiveNodes(&activeNodes);

    for (NodesList::iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        (*it)->purgeAllInstancesCaches();
    }
}

void
AppInstance::clearAllLastRenderedImages()
{
    NodesList activeNodes;

    _imp->_currentProject->getNodes_recursive(activeNodes, false);

    for (NodesList::iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        (*it)->clearLastRenderedImage();
    }
}

void
AppInstance::aboutToQuit()
{
    ///Clear nodes now, not in the destructor of the project as
    ///deleting nodes might reference the project.
    _imp->_currentProject->reset(true /*aboutToQuit*/, /*blocking*/true);
}

void
AppInstance::quit()
{
    appPTR->quit( shared_from_this() );
}

void
AppInstance::quitNow()
{
    appPTR->quitNow( shared_from_this() );
}

ViewerColorSpaceEnum
AppInstance::getDefaultColorSpaceForBitDepth(ImageBitDepthEnum bitdepth) const
{
    return _imp->_currentProject->getDefaultColorSpaceForBitDepth(bitdepth);
}

void
AppInstance::onOCIOConfigPathChanged(const std::string& path)
{
    _imp->_currentProject->onOCIOConfigPathChanged(path, false);
}

void
AppInstance::declareCurrentAppVariable_Python()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    /// define the app variable
    std::stringstream ss;

    ss << "app" << _imp->_appID + 1 << " = " << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.getInstance(" << _imp->_appID << ") \n";
    const KnobsVec& knobs = _imp->_currentProject->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss << "app" << _imp->_appID + 1 << "." << (*it)->getName() << " = app" << _imp->_appID + 1 << ".getProjectParam('" <<
        (*it)->getName() << "')\n";
    }
    std::string script = ss.str();
    std::string err;
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::declareCurrentAppVariable_Python() failed!");
    }

    if ( appPTR->isBackground() ) {
        std::string err;
        ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript("app = app1\n", &err, 0);
        assert(ok);
    }
}

double
AppInstance::getProjectFrameRate() const
{
    return _imp->_currentProject->getProjectFrameRate();
}

void
AppInstance::setProjectWasCreatedWithLowerCaseIDs(bool b)
{
    _imp->_projectCreatedWithLowerCaseIDs = b;
}

bool
AppInstance::wasProjectCreatedWithLowerCaseIDs() const
{
    return _imp->_projectCreatedWithLowerCaseIDs;
}

bool
AppInstance::isCreatingPythonGroup() const
{
    QMutexLocker k(&_imp->creatingGroupMutex);

    return _imp->_creatingGroup;
}

void
AppInstance::appendToScriptEditor(const std::string& str)
{
    std::cout << str <<  std::endl;
}

void
AppInstance::printAutoDeclaredVariable(const std::string& /*str*/)
{
}

void
AppInstance::execOnProjectCreatedCallback()
{
    std::string cb = appPTR->getCurrentSettings()->getOnProjectCreatedCB();

    if ( cb.empty() ) {
        return;
    }


    std::vector<std::string> args;
    std::string error;
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        appendToScriptEditor( std::string("Failed to get signature of onProjectCreated callback: ")
                              + e.what() );

        return;
    }

    if ( !error.empty() ) {
        appendToScriptEditor("Failed to get signature of onProjectCreated callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The on project created callback supports the following signature(s):\n");
    signatureError.append("- callback(app)");
    if (args.size() != 1) {
        appendToScriptEditor("Wrong signature of onProjectCreated callback: " + signatureError);

        return;
    }
    if (args[0] != "app") {
        appendToScriptEditor("Wrong signature of onProjectCreated callback: " + signatureError);

        return;
    }
    std::string appID = getAppIDString();
    std::string script;
    if (appID != "app") {
        script = script + "app = " + appID;
    }
    script = script + "\n" + cb + "(" + appID + ")\n";
    std::string err;
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
        appendToScriptEditor("Failed to run onProjectCreated callback: " + err);
    } else {
        if ( !output.empty() ) {
            appendToScriptEditor(output);
        }
    }
} // AppInstance::execOnProjectCreatedCallback

std::string
AppInstance::getAppIDString() const
{
    if ( appPTR->isBackground() ) {
        return "app";
    } else {
        QString appID =  QString( QString::fromUtf8("app%1") ).arg(getAppID() + 1);

        return appID.toStdString();
    }
}

void
AppInstance::onGroupCreationFinished(const NodePtr& node,
                                     const NodeSerializationPtr& serialization, bool /*autoConnect*/)
{
    assert(node);
    if (!node) {
        throw std::logic_error(__func__);
    }

    // If the node is a PyPlug we might have set in the Python scripts the Mask and Optional knobs of the GroupInput
    // nodes, but nothing updated the inputs since we were loading the PyPlug, ensure they are up to date.
    node->initializeInputs();
    if ( !_imp->_currentProject->isLoadingProject() && !serialization ) {
        NodeGroup* isGrp = node->isEffectGroup();
        assert(isGrp);
        if (!isGrp) {
            return;
        }

        isGrp->forceComputeInputDependentDataOnAllTrees();
    }
}

bool
AppInstance::saveTemp(const std::string& filename)
{
    std::string outFile = filename;
    std::string path = SequenceParsing::removePath(outFile);
    ProjectPtr project = getProject();

    return project->saveProject_imp(QString::fromUtf8( path.c_str() ), QString::fromUtf8( outFile.c_str() ), false, false, 0);
}

bool
AppInstance::save(const std::string& filename)
{
    ProjectPtr project = getProject();

    if ( project->hasProjectBeenSavedByUser() ) {
        QString projectFilename = project->getProjectFilename();
        QString projectPath = project->getProjectPath();

        return project->saveProject(projectPath, projectFilename, 0);
    } else {
        return saveAs(filename);
    }
}

bool
AppInstance::saveAs(const std::string& filename)
{
    std::string outFile = filename;
    std::string path = SequenceParsing::removePath(outFile);

    return getProject()->saveProject(QString::fromUtf8( path.c_str() ), QString::fromUtf8( outFile.c_str() ), 0);
}

AppInstancePtr
AppInstance::loadProject(const std::string& filename)
{
    QFileInfo file( QString::fromUtf8( filename.c_str() ) );

    if ( !file.exists() ) {
        return AppInstancePtr();
    }
    QString fileUnPathed = file.fileName();
    QString path = file.path() + QChar::fromLatin1('/');

    //We are in background mode, there can only be 1 instance active, wipe the current project
    ProjectPtr project = getProject();
    project->resetProject();

    bool ok  = project->loadProject( path, fileUnPathed);
    if (ok) {
        return shared_from_this();
    }

    project->resetProject();

    return AppInstancePtr();
}

///Close the current project but keep the window
bool
AppInstance::resetProject()
{
    getProject()->reset(false /*aboutToQuit*/, true /*blocking*/);

    return true;
}

///Reset + close window, quit if last window
bool
AppInstance::closeProject()
{
    getProject()->reset(true/*aboutToQuit*/, true /*blocking*/);
    quit();

    return true;
}

///Opens a new project
AppInstancePtr
AppInstance::newProject()
{
    CLArgs cl;
    AppInstancePtr app = appPTR->newAppInstance(cl, false);

    return app;
}

void
AppInstance::addInvalidExpressionKnob(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->invalidExprKnobsMutex);

    for (std::list<KnobIWPtr>::iterator it = _imp->invalidExprKnobs.begin(); it != _imp->invalidExprKnobs.end(); ++it) {
        if ( it->lock().get() ) {
            return;
        }
    }
    _imp->invalidExprKnobs.push_back(knob);
}

void
AppInstance::removeInvalidExpressionKnob(const KnobI* knob)
{
    QMutexLocker k(&_imp->invalidExprKnobsMutex);

    for (std::list<KnobIWPtr>::iterator it = _imp->invalidExprKnobs.begin(); it != _imp->invalidExprKnobs.end(); ++it) {
        if (it->lock().get() == knob) {
            _imp->invalidExprKnobs.erase(it);
            break;
        }
    }
}

void
AppInstance::recheckInvalidExpressions()
{
    if (getProject()->isProjectClosing()) {
        return;
    }
    std::list<KnobIPtr> knobs;
    {
        QMutexLocker k(&_imp->invalidExprKnobsMutex);
        for (std::list<KnobIWPtr>::iterator it = _imp->invalidExprKnobs.begin(); it != _imp->invalidExprKnobs.end(); ++it) {
            KnobIPtr k = it->lock();
            if (k) {
                knobs.push_back(k);
            }
        }
    }
    std::list<KnobIWPtr> newInvalidKnobs;

    for (std::list<KnobIPtr>::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( !(*it)->checkInvalidExpressions() ) {
            newInvalidKnobs.push_back(*it);
        }
    }
    {
        QMutexLocker k(&_imp->invalidExprKnobsMutex);
        _imp->invalidExprKnobs = newInvalidKnobs;
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AppInstance.cpp"
