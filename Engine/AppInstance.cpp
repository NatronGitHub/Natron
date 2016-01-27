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

#include "AppInstance.h"

#include <fstream>
#include <list>
#include <stdexcept>

#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QEventLoop>
#include <QtCore/QSettings>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Global/QtCompat.h" // removeFileExtension

#include "Engine/BlockingBackgroundRender.h"
#include "Engine/CLArgs.h"
#include "Engine/FileDownloader.h"
#include "Engine/GroupOutput.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxHost.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER;

FlagSetter::FlagSetter(bool initialValue,bool* p)
: p(p)
, lock(0)
{
    *p = initialValue;
}

FlagSetter::FlagSetter(bool initialValue,bool* p, QMutex* mutex)
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



struct AppInstancePrivate
{
    boost::shared_ptr<Project> _currentProject; //< ptr to the project
    int _appID; //< the unique ID of this instance (or window)
    bool _projectCreatedWithLowerCaseIDs;
    
    mutable QMutex creatingGroupMutex;
    
    //When a pyplug is created
    bool _creatingGroup;
    
    //When a node is created
    bool _creatingNode;
    
    //When a node tree is created
    int _creatingTree;
    
    AppInstancePrivate(int appID,
                       AppInstance* app)
    : _currentProject( new Project(app) )
    , _appID(appID)
    , _projectCreatedWithLowerCaseIDs(false)
    , creatingGroupMutex()
    , _creatingGroup(false)
    , _creatingNode(false)
    , _creatingTree(0)
    {
    }
    
    void declareCurrentAppVariable_Python();
    
    
    void executeCommandLinePythonCommands(const CLArgs& args);
    
};

AppInstance::AppInstance(int appID)
    : QObject()
      , _imp( new AppInstancePrivate(appID,this) )
{
    appPTR->registerAppInstance(this);
    appPTR->setAsTopLevelInstance(appID);


    ///initialize the knobs of the project before loading anything else.
    _imp->_currentProject->initializeKnobsPublic();
    
}

AppInstance::~AppInstance()
{

    appPTR->removeInstance(_imp->_appID);
}

void
AppInstance::setCreatingNode(bool b)
{
    QMutexLocker k(&_imp->creatingGroupMutex);
    _imp->_creatingNode = b;
}

bool
AppInstance::isCreatingNode() const
{
    QMutexLocker k(&_imp->creatingGroupMutex);
    return _imp->_creatingNode;
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
    FileDownloader* downloader = new FileDownloader( QUrl(NATRON_LAST_VERSION_URL), false );
    QObject::connect( downloader, SIGNAL( downloaded() ), this, SLOT( newVersionCheckDownloaded() ) );
    QObject::connect( downloader, SIGNAL( error() ), this, SLOT( newVersionCheckError() ) );

    ///make the call blocking
    QEventLoop loop;

    connect( downloader->getReply(), SIGNAL( finished() ), &loop, SLOT( quit() ) );
    loop.exec();
}

//return -1 if a < b, 0 if a == b and 1 if a > b
//Returns -2 if not understood
static
int compareDevStatus(const QString& a,const QString& b)
{
    if (a == NATRON_DEVELOPMENT_DEVEL || a == NATRON_DEVELOPMENT_SNAPSHOT) {
        //Do not try updates when update available is a dev build
        return -1;
    } else if (b == NATRON_DEVELOPMENT_DEVEL || b == NATRON_DEVELOPMENT_SNAPSHOT) {
        //This is a dev build, do not try updates
        return -1;
    } else if (a == NATRON_DEVELOPMENT_ALPHA) {
        if (b == NATRON_DEVELOPMENT_ALPHA) {
            return 0;
        } else {
            return -1;
        }
    } else if (a == NATRON_DEVELOPMENT_BETA) {
        if (b == NATRON_DEVELOPMENT_ALPHA) {
            return 1;
        } else if (b == NATRON_DEVELOPMENT_BETA) {
            return 0;
        } else {
            return -1;
        }
    } else if (a == NATRON_DEVELOPMENT_RELEASE_CANDIDATE) {
        if (b == NATRON_DEVELOPMENT_ALPHA) {
            return 1;
        } else if (b == NATRON_DEVELOPMENT_BETA) {
            return 1;
        } else if (b == NATRON_DEVELOPMENT_RELEASE_CANDIDATE) {
            return 0;
        } else {
            return -1;
        }
    } else if (a == NATRON_DEVELOPMENT_RELEASE_STABLE) {
        if (b == NATRON_DEVELOPMENT_RELEASE_STABLE) {
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
    
    QString extractedFileVersionStr,extractedSoftwareVersionStr,extractedDevStatusStr,extractedBuildNumberStr;
    
    QString fileVersionTag("File version: ");
    QString softwareVersionTag("Software version: ");
    QString devStatusTag("Development status: ");
    QString buildNumberTag("Build number: ");
    
    QString data( downloader->downloadedData() );
    QTextStream ts(&data);
    
    while (!ts.atEnd()) {
        QString line = ts.readLine();
        if (line.startsWith(QChar('#')) || line.startsWith(QChar('\n'))) {
            continue;
        }
        
        if (line.startsWith(fileVersionTag)) {
            int i = fileVersionTag.size();
            while ( i < line.size() && !line.at(i).isSpace()) {
                extractedFileVersionStr.push_back( line.at(i) );
                ++i;
            }
        } else if (line.startsWith(softwareVersionTag)) {
            int i = softwareVersionTag.size();
            while ( i < line.size() && !line.at(i).isSpace()) {
                extractedSoftwareVersionStr.push_back( line.at(i) );
                ++i;
            }
        } else if (line.startsWith(devStatusTag)) {
            int i = devStatusTag.size();
            while ( i < line.size() && !line.at(i).isSpace()) {
            extractedDevStatusStr.push_back( line.at(i) );
                ++i;
            }
        } else if (line.startsWith(buildNumberTag)) {
            int i = buildNumberTag.size();
            while ( i < line.size() && !line.at(i).isSpace()) {
                extractedBuildNumberStr.push_back( line.at(i) );
                ++i;
            }

        }
    }
    
    if (extractedFileVersionStr.isEmpty() || extractedFileVersionStr.toInt() < NATRON_LAST_VERSION_FILE_VERSION) {
        //The file cannot be decoded here
        downloader->deleteLater();
        return;
    }
    
    
    
    QStringList versionDigits = extractedSoftwareVersionStr.split( QChar('.') );

    ///we only understand 3 digits formed version numbers
    if (versionDigits.size() != 3) {
        downloader->deleteLater();
        return;
    }
    
    int buildNumber = extractedBuildNumberStr.toInt();

    int major = versionDigits[0].toInt();
    int minor = versionDigits[1].toInt();
    int revision = versionDigits[2].toInt();
    
    int devStatCompare = compareDevStatus(extractedDevStatusStr, QString(NATRON_DEVELOPMENT_STATUS));
    
    int versionEncoded = NATRON_VERSION_ENCODE(major, minor, revision);
    if (versionEncoded > NATRON_VERSION_ENCODED ||
        (versionEncoded == NATRON_VERSION_ENCODED &&
         (devStatCompare > 0 || (devStatCompare == 0 && buildNumber > NATRON_BUILD_NUMBER)))) {
            
            QString text;
            if (devStatCompare == 0 && buildNumber > NATRON_BUILD_NUMBER && versionEncoded == NATRON_VERSION_ENCODED) {
                ///show build number in version
                text =  QObject::tr("<p>Updates for %1 are now available for download. "
                                    "You are currently using %1 version %2 - %3 - build %4. "
                                    "The latest version of %1 is version %5 - %6 - build %7.</p> ")
                .arg(NATRON_APPLICATION_NAME)
                .arg(NATRON_VERSION_STRING)
                .arg(NATRON_DEVELOPMENT_STATUS)
                .arg(NATRON_BUILD_NUMBER)
                .arg(extractedSoftwareVersionStr)
                .arg(extractedDevStatusStr)
                .arg(extractedBuildNumberStr) +
                QObject::tr("<p>You can download it from ") + QString("<a href='http://sourceforge.net/projects/natron/'>"
                                                                     "<font color=\"orange\">Sourceforge</a>. </p>");
            } else {
                text =  QObject::tr("<p>Updates for %1 are now available for download. "
                                    "You are currently using %1 version %2 - %3. "
                                    "The latest version of %1 is version %4 - %5.</p> ")
                .arg(NATRON_APPLICATION_NAME)
                .arg(NATRON_VERSION_STRING)
                .arg(NATRON_DEVELOPMENT_STATUS)
                .arg(extractedSoftwareVersionStr)
                .arg(extractedDevStatusStr) +
                QObject::tr("<p>You can download it from ") + QString("<a href='www.natron.fr/download'>"
                                                                    "<font color=\"orange\">www.natron.fr</a>. </p>");

            }
        
            Dialogs::informationDialog( "New version", text.toStdString(), true );
    }
    downloader->deleteLater();
}

void
AppInstance::newVersionCheckError()
{
    ///Nothing to do,
    FileDownloader* downloader = qobject_cast<FileDownloader*>( sender() );

    assert(downloader);
    downloader->deleteLater();
}

void
AppInstance::getWritersWorkForCL(const CLArgs& cl,std::list<AppInstance::RenderWork>& requests)
{
    const std::list<CLArgs::WriterArg>& writers = cl.getWriterArgs();
    for (std::list<CLArgs::WriterArg>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
        
        NodePtr writerNode;
        if (!it->mustCreate) {
            std::string writerName = it->name.toStdString();
            writerNode = getNodeByFullySpecifiedName(writerName);
            
            if (!writerNode) {
                std::string exc(writerName);
                exc.append(tr(" does not belong to the project file. Please enter a valid Write node script-name.").toStdString());
                throw std::invalid_argument(exc);
            } else {
                if (!writerNode->isOutputNode()) {
                    std::string exc(writerName);
                    exc.append(tr(" is not an output node! It cannot render anything.").toStdString());
                    throw std::invalid_argument(exc);
                }
            }
            
            if (!it->filename.isEmpty()) {
                KnobPtr fileKnob = writerNode->getKnobByName(kOfxImageEffectFileParamName);
                if (fileKnob) {
                    KnobOutputFile* outFile = dynamic_cast<KnobOutputFile*>(fileKnob.get());
                    if (outFile) {
                        outFile->setValue(it->filename.toStdString(), 0);
                    }
                }
            }
        } else {
            writerNode = createWriter(it->filename.toStdString(), eCreateNodeReasonInternal, getProject());
            
            //Connect the writer to the corresponding Output node input
            NodePtr output = getProject()->getNodeByFullySpecifiedName(it->name.toStdString());
            if (!output) {
                throw std::invalid_argument(it->name.toStdString() + tr(" is not the name of a valid Output node of the script").toStdString());
            }
            GroupOutput* isGrpOutput = dynamic_cast<GroupOutput*>(output->getEffectInstance().get());
            if (!isGrpOutput) {
                throw std::invalid_argument(it->name.toStdString() + tr(" is not the name of a valid Output node of the script").toStdString());
            }
            NodePtr outputInput = output->getRealInput(0);
            if (outputInput) {
                writerNode->connectInput(outputInput, 0);
            }
            
            writerNode->getScriptName().c_str();
        }
        
        assert(writerNode);
        OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>(writerNode->getEffectInstance().get());

        if (cl.hasFrameRange()) {
            const std::list<std::pair<int,std::pair<int,int> > >& frameRanges = cl.getFrameRanges();
            for (std::list<std::pair<int,std::pair<int,int> > >::const_iterator it2 = frameRanges.begin(); it2 != frameRanges.end(); ++it2) {
                AppInstance::RenderWork r;
                r.firstFrame = it2->second.first;
                r.lastFrame = it2->second.second;
                r.frameStep = it2->first;
                r.writer = effect;
                requests.push_back(r);
            }
        } else {
            AppInstance::RenderWork r;
            r.firstFrame = INT_MIN;
            r.lastFrame = INT_MAX;
            r.frameStep = INT_MIN;
            r.writer = effect;
            requests.push_back(r);
        }
    
    }
}

NodePtr
AppInstance::createWriter(const std::string& filename,
                          CreateNodeReason reason,
                          const boost::shared_ptr<NodeCollection>& collection,
                          int firstFrame, int lastFrame)
{
    std::map<std::string,std::string> writersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    
    QString fileCpy(filename.c_str());
    std::string ext = QtCompat::removeFileExtension(fileCpy).toLower().toStdString();
    std::map<std::string,std::string>::iterator found = writersForFormat.find(ext);
    if ( found == writersForFormat.end() ) {
        Dialogs::errorDialog( tr("Writer").toStdString(),
                            tr("No plugin capable of encoding ").toStdString() + ext + tr(" was found.").toStdString(),false );
        return NodePtr();
    }
    
    
    CreateNodeArgs args(found->second.c_str(), reason, collection);
    args.paramValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, filename));
    if (firstFrame != INT_MIN && lastFrame != INT_MAX) {
        args.paramValues.push_back(createDefaultValueForParam<int>("frameRange", 2));
        args.paramValues.push_back(createDefaultValueForParam<int>("firstFrame", firstFrame));
        args.paramValues.push_back(createDefaultValueForParam<int>("lastFrame", lastFrame));
    }
  
    return createNode(args);
    
}

void
AppInstancePrivate::executeCommandLinePythonCommands(const CLArgs& args)
{
    const std::list<std::string>& commands = args.getPythonCommands();
    
    for (std::list<std::string>::const_iterator it = commands.begin(); it!=commands.end(); ++it) {
        std::string err;
        std::string output;
        bool ok  = Python::interpretPythonScript(*it, &err, &output);
        if (!ok) {
            QString m = QObject::tr("Failed to execute given command-line Python command: ");
            m.append(it->c_str());
            m.append(" Error: ");
            m.append(err.c_str());
            throw std::runtime_error(m.toStdString());
        } else if (!output.empty()) {
            std::cout << output << std::endl;
        }

    }
}

void
AppInstance::load(const CLArgs& cl,bool makeEmptyInstance)
{
    
    declareCurrentAppVariable_Python();

    if (makeEmptyInstance) {
        return;
    }
    
    const QString& extraOnProjectCreatedScript = cl.getDefaultOnProjectLoadedScript();
    
    _imp->executeCommandLinePythonCommands(cl);

    
    ///if the app is a background project autorun and the project name is empty just throw an exception.
    if ( (appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRun ||
          appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRunLaunchedFromGui)) {
        
        if (cl.getScriptFilename().isEmpty()) {
            // cannot start a background process without a file
            throw std::invalid_argument(tr("Project file name empty").toStdString());
        }
        

        QFileInfo info(cl.getScriptFilename());
        if (!info.exists()) {
            throw std::invalid_argument(tr("Specified project file does not exist").toStdString());
        }
        
        std::list<AppInstance::RenderWork> writersWork;
        

        if (info.suffix() == NATRON_PROJECT_FILE_EXT) {
            
            ///Load the project
            if ( !_imp->_currentProject->loadProject(info.path(),info.fileName()) ) {
                throw std::invalid_argument(tr("Project file loading failed.").toStdString());
            }
            
            getWritersWorkForCL(cl, writersWork);

        } else if (info.suffix() == "py") {
            
            ///Load the python script
            loadPythonScript(info);
            getWritersWorkForCL(cl, writersWork);

        } else {
            throw std::invalid_argument(tr(NATRON_APPLICATION_NAME " only accepts python scripts or .ntp project files").toStdString());
        }
        
        
        ///exec the python script specified via --onload
        if (!extraOnProjectCreatedScript.isEmpty()) {
            QFileInfo cbInfo(extraOnProjectCreatedScript);
            if (cbInfo.exists()) {
                loadPythonScript(cbInfo);
            }
        }
        
        
        ///Set reader parameters if specified from the command-line
        const std::list<CLArgs::ReaderArg>& readerArgs = cl.getReaderArgs();
        for (std::list<CLArgs::ReaderArg>::const_iterator it = readerArgs.begin(); it!=readerArgs.end(); ++it) {
            std::string readerName = it->name.toStdString();
            NodePtr readNode = getNodeByFullySpecifiedName(readerName);
            
            if (!readNode) {
                std::string exc(readerName);
                exc.append(tr(" does not belong to the project file. Please enter a valid Read node script-name.").toStdString());
                throw std::invalid_argument(exc);
            } else {
                if (!readNode->getEffectInstance()->isReader()) {
                    std::string exc(readerName);
                    exc.append(tr(" is not a Read node! It cannot render anything.").toStdString());
                    throw std::invalid_argument(exc);
                }
            }
            
            if (it->filename.isEmpty()) {
                std::string exc(readerName);
                exc.append(tr(": Filename specified is empty but [-i] or [--reader] was passed to the command-line").toStdString());
                throw std::invalid_argument(exc);
            }
            KnobPtr fileKnob = readNode->getKnobByName(kOfxImageEffectFileParamName);
            if (fileKnob) {
                KnobFile* outFile = dynamic_cast<KnobFile*>(fileKnob.get());
                if (outFile) {
                    outFile->setValue(it->filename.toStdString(), 0);
                }
            }

        }
       
        ///launch renders
        if (!writersWork.empty()) {
            startWritersRendering(cl.areRenderStatsEnabled(), false, writersWork);
        } else {
            std::list<std::string> writers;
            startWritersRendering(cl.areRenderStatsEnabled(), false, writers, cl.getFrameRanges());
        }
       
        
        
        
    } else if (appPTR->getAppType() == AppManager::eAppTypeInterpreter) {
        QFileInfo info(cl.getScriptFilename());
        if (info.exists()) {
            
            if (info.suffix() == "py") {
                loadPythonScript(info);
            } else if (info.suffix() == NATRON_PROJECT_FILE_EXT) {
                if ( !_imp->_currentProject->loadProject(info.path(),info.fileName()) ) {
                    throw std::invalid_argument(tr("Project file loading failed.").toStdString());
                }
            }
        }
        
        if (!extraOnProjectCreatedScript.isEmpty()) {
            QFileInfo cbInfo(extraOnProjectCreatedScript);
            if (cbInfo.exists()) {
                loadPythonScript(cbInfo);
            }
        }

        
        appPTR->launchPythonInterpreter();
    } else {
        execOnProjectCreatedCallback();
        
        if (!extraOnProjectCreatedScript.isEmpty()) {
            QFileInfo cbInfo(extraOnProjectCreatedScript);
            if (cbInfo.exists()) {
                loadPythonScript(cbInfo);
            }
        }

    }
}

bool
AppInstance::loadPythonScript(const QFileInfo& file)
{
    
    std::string addToPythonPath("sys.path.append(\"");
    addToPythonPath += file.path().toStdString();
    addToPythonPath += "\")\n";
    
    std::string err;
    bool ok  = Python::interpretPythonScript(addToPythonPath, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::loadPythonScript(" + file.path().toStdString() + "): interpretPythonScript("+addToPythonPath+" failed!");
    }

    std::string s = "app = app1\n";
    ok = Python::interpretPythonScript(s, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::loadPythonScript(" + file.path().toStdString() + "): interpretPythonScript("+s+" failed!");
    }
    
    QFile f(file.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    QTextStream ts(&f);
    QString content = ts.readAll();
    bool hasCreateInstance = content.contains("def createInstance");
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
        int lastDotPos = moduleName.lastIndexOf(QChar('.'));
        if (lastDotPos != -1) {
            moduleName = moduleName.left(lastDotPos);
        }
    
        std::stringstream ss;
        ss << "import " << moduleName.toStdString() << '\n';
        ss << moduleName.toStdString() << ".createInstance(app,app)";
        
        std::string output;
        FlagSetter flag(true, &_imp->_creatingGroup, &_imp->creatingGroupMutex);
        CreatingNodeTreeFlag_RAII createNodeTree(this);
        if (!Python::interpretPythonScript(ss.str(), &err, &output)) {
            if (!err.empty()) {
                Dialogs::errorDialog(tr("Python").toStdString(), err);
            }
            return false;
        } else {
            if (!output.empty()) {
                if (appPTR->isBackground()) {
                    std::cout << output << std::endl;
                } else {
                    appendToScriptEditor(output);
                }
            }
        }
        
        getProject()->forceComputeInputDependentDataOnAllTrees();
    } else {
        QFile f(file.absoluteFilePath());
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
            message.append(file.absoluteFilePath());
            message.append(": ");
            message.append(error.c_str());
            appendToScriptEditor(message.toStdString());
        }
        
    }
    
    return true;
}

NodePtr
AppInstance::createNodeFromPythonModule(Plugin* plugin,
                                        const boost::shared_ptr<NodeCollection>& group,
                                        CreateNodeReason reason,
                                        const boost::shared_ptr<NodeSerialization>& serialization)

{
   
    
    NodePtr node;
    
    {
        FlagSetter fs(true,&_imp->_creatingGroup,&_imp->creatingGroupMutex);
        CreatingNodeTreeFlag_RAII createNodeTree(this);
        
        NodePtr containerNode;
        CreateNodeArgs groupArgs(PLUGINID_NATRON_GROUP, reason, group);
        groupArgs.serialization = serialization;
        containerNode = createNode(groupArgs);
        if (!containerNode) {
            return containerNode;
        }
        
        if (reason == eCreateNodeReasonUserCreate || reason == eCreateNodeReasonInternal) {
            std::string containerName;
            try {
                group->initNodeName(plugin->getLabelWithoutSuffix().toStdString(),&containerName);
                containerNode->setScriptName(containerName);
                containerNode->setLabel(containerName);
            } catch (...) {
                
            }
        }
        
        
        
        std::string containerFullySpecifiedName = containerNode->getFullyQualifiedName();
        
        QString moduleName;
        QString modulePath;
        plugin->getPythonModuleNameAndPath(&moduleName, &modulePath);
        
        int appID = getAppID() + 1;
        
        std::stringstream ss;
        ss << moduleName.toStdString();
        ss << ".createInstance(app" << appID;
        ss << ", app" << appID << "." << containerFullySpecifiedName;
        ss << ")\n";
        std::string err;
        std::string output;
        if (!Python::interpretPythonScript(ss.str(), &err, &output)) {
            Dialogs::errorDialog(tr("Group plugin creation error").toStdString(), err);
            containerNode->destroyNode(false);
            return node;
        } else {
            if (!output.empty()) {
                appendToScriptEditor(output);
            }
            node = containerNode;
        }
        
        if (!moduleName.isEmpty()) {
            setGroupLabelIDAndVersion(node,modulePath, moduleName);
        }
     
        
    } //FlagSetter fs(true,&_imp->_creatingGroup,&_imp->creatingGroupMutex);
    
    ///Now that the group is created and all nodes loaded, autoconnect the group like other nodes.
    onGroupCreationFinished(node, reason);
    
    return node;
}


void
AppInstance::setGroupLabelIDAndVersion(const NodePtr& node,
                                       const QString& pythonModulePath,
                               const QString &pythonModule)
{
    std::string pluginID,pluginLabel,iconFilePath,pluginGrouping,description;
    unsigned int version;
    if (Python::getGroupInfos(pythonModulePath.toStdString(),pythonModule.toStdString(), &pluginID, &pluginLabel, &iconFilePath, &pluginGrouping, &description, &version)) {
        node->setPluginIconFilePath(iconFilePath);
        node->setPluginDescription(description);
        node->setPluginIDAndVersionForGui(pluginLabel, pluginID, version);
        node->setPluginPythonModule(QString(pythonModulePath + pythonModule + ".py").toStdString(), version);
    }
    
}



/**
 * @brief An inspector node is like a viewer node with hidden inputs that unfolds one after another.
 * This functions returns the number of inputs to use for inspectors or 0 for a regular node.
 **/
static int isEntitledForInspector(Plugin* plugin,OFX::Host::ImageEffect::Descriptor* ofxDesc)  {
    
    if (plugin->getPluginID() == PLUGINID_NATRON_VIEWER) {
        return 10;
    } else if (plugin->getPluginID() == PLUGINID_NATRON_ROTOPAINT ||
               plugin->getPluginID() == PLUGINID_NATRON_ROTO) {
        return 11;
    }
    
    if (!ofxDesc) {
        return 0;
    }
    
    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*>& clips = ofxDesc->getClipsByOrder();
    int nInputs = 0;
    bool firstInput = true;
    for (std::vector<OFX::Host::ImageEffect::ClipDescriptor*>::const_iterator it = clips.begin(); it != clips.end(); ++it) {
        if (!(*it)->isOutput()) {
            if (!(*it)->isOptional()) {
                if (!firstInput) {                    // allow one non-optional input
                    return 0;
                }
            } else {
                ++nInputs;
            }
            firstInput = false;
        }
    }
    if (nInputs > 4) {
        return nInputs;
    }
    return 0;
}

NodePtr
AppInstance::createNodeInternal(const CreateNodeArgs& args)
{
    
    NodePtr node;
    Plugin* plugin = 0;
    QString findId;
    
    //Roto has moved to a built-in plugin
    if ((args.reason == eCreateNodeReasonUserCreate || args.reason == eCreateNodeReasonProjectLoad) &&
        ((!_imp->_projectCreatedWithLowerCaseIDs && args.pluginID == PLUGINID_OFX_ROTO) || (_imp->_projectCreatedWithLowerCaseIDs && args.pluginID == QString(PLUGINID_OFX_ROTO).toLower()))) {
        findId = PLUGINID_NATRON_ROTO;
    } else {
        findId = args.pluginID;
    }
    
    try {
        plugin = appPTR->getPluginBinary(findId,args.majorV,args.minorV,_imp->_projectCreatedWithLowerCaseIDs && args.reason == eCreateNodeReasonProjectLoad);
    } catch (const std::exception & e1) {
        
        ///Ok try with the old Ids we had in Natron prior to 1.0
        try {
            plugin = appPTR->getPluginBinaryFromOldID(args.pluginID, args.majorV, args.minorV);
        } catch (const std::exception& e2) {
            Dialogs::errorDialog(tr("Plugin error").toStdString(),
                                tr("Cannot load plugin executable").toStdString() + ": " + e2.what(), false );
            return node;
        }
        
    }
    
    
    if (!plugin) {
        return node;
    }
    
    if (!plugin->getIsUserCreatable() && args.reason == eCreateNodeReasonUserCreate) {
        //The plug-in should not be instantiable by the user
        qDebug() << "Attempt to create" << args.pluginID << "which is not user creatable";
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
    if (!pythonModule.isEmpty()) {
        if (args.reason != eCreateNodeReasonCopyPaste) {
            try {
                return createNodeFromPythonModule(plugin, args.group, args.reason, args.serialization);
            } catch (const std::exception& e) {
                Dialogs::errorDialog(tr("Plugin error").toStdString(),
                                     tr("Cannot create PyPlug:").toStdString()+ e.what(), false );
                return node;
            }
        } else {
            plugin = appPTR->getPluginBinary(PLUGINID_NATRON_GROUP,-1,-1, false);
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
                ofxDesc = appPTR->getPluginContextAndDescribe(ofxPlugin,&ctx);
            } catch (const std::exception& e) {
                errorDialog(tr("Error while creating node").toStdString(), tr("Failed to create an instance of ").toStdString()
                            + args.pluginID.toStdString() + ": " + e.what(), false);
                return NodePtr();
            }
            assert(ofxDesc);
            plugin->setOfxDesc(ofxDesc, ctx);
        }
    }
    
    int nInputsForInspector = isEntitledForInspector(plugin,ofxDesc);
    
    if (!nInputsForInspector) {
        node.reset( new Node(this, args.addToProject ? args.group : boost::shared_ptr<NodeCollection>(), plugin) );
    } else {
        node.reset( new InspectorNode(this, args.addToProject ? args.group : boost::shared_ptr<NodeCollection>(), plugin,nInputsForInspector) );
    }
    
    {
        ///Furnace plug-ins don't handle using the thread pool
        boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
        if (foundPluginID.find("uk.co.thefoundry.furnace") != std::string::npos &&
            (settings->useGlobalThreadPool() || settings->getNumberOfParallelRenders() != 1)) {
            StandardButtonEnum reply = Dialogs::questionDialog(tr("Warning").toStdString(),
                                                                  tr("The settings of the application are currently set to use "
                                                                     "the global thread-pool for rendering effects. The Foundry Furnace "
                                                                     "is known not to work well when this setting is checked. "
                                                                     "Would you like to turn it off ? ").toStdString(), false);
            if (reply == eStandardButtonYes) {
                settings->setUseGlobalThreadPool(false);
                settings->setNumberOfParallelRenders(1);
            }
        }
        
        ///If this is a stereo plug-in, check that the project has been set for multi-view
        if (args.reason == eCreateNodeReasonUserCreate) {
            const QStringList& grouping = plugin->getGrouping();
            if (!grouping.isEmpty() && grouping[0] == PLUGIN_GROUP_MULTIVIEW) {
                int nbViews = getProject()->getProjectViewsCount();
                if (nbViews < 2) {
                    StandardButtonEnum reply = Dialogs::questionDialog(tr("Multi-View").toStdString(),
                                                                              tr("Using a multi-view node requires the project settings to be setup "
                                                                                 "for multi-view. Would you like to setup the project for stereo?").toStdString(), false);
                    if (reply == eStandardButtonYes) {
                        getProject()->setupProjectForStereo();
                    }
                }
            }
        }
    }
    
    
    if (args.addToProject) {
        //Add the node to the project before loading it so it is present when the python script that registers a variable of the name
        //of the node works
        assert(args.group);
        if (args.group) {
            args.group->addNode(node);
        }
    }
    assert(node);
    try {
        node->load(args);
    } catch (const std::exception & e) {
        if (args.group) {
            args.group->removeNode(node);
        }
        std::string title("Error while creating node");
        std::string message = title + " " + foundPluginID + ": " + e.what();
        qDebug() << message.c_str();
        errorDialog(title, message, false);

        return NodePtr();
    } catch (...) {
        if (args.group) {
            args.group->removeNode(node);
        }
        std::string title("Error while creating node");
        std::string message = title + " " + foundPluginID;
        qDebug() << message.c_str();
        errorDialog(title, message, false);

        return NodePtr();
    }

    NodePtr multiInstanceParent = node->getParentMultiInstance();
    
    if (args.createGui) {
        // createNodeGui also sets the filename parameter for reader or writers
        try {
            createNodeGui(node, multiInstanceParent, args);
        } catch (const std::exception& e) {
            node->destroyNode(false);
            std::string title("Error while creating node");
            std::string message = title + " " + foundPluginID + ": " + e.what();
            qDebug() << message.c_str();
            errorDialog(title, message, false);
            
            return boost::shared_ptr<Natron::Node>();
        }
    }
    
    boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>(node->getEffectInstance()->shared_from_this());

    if (isGrp) {
        
        if (args.reason == eCreateNodeReasonProjectLoad || args.reason == eCreateNodeReasonCopyPaste) {
            if (args.serialization && !args.serialization->getPythonModule().empty()) {
                
                QString pythonModulePath(args.serialization->getPythonModule().c_str());
                QString moduleName;
                QString modulePath;
                int foundLastSlash = pythonModulePath.lastIndexOf('/');
                if (foundLastSlash != -1) {
                    modulePath = pythonModulePath.mid(0,foundLastSlash + 1);
                    moduleName = pythonModulePath.remove(0,foundLastSlash + 1);
                }
                QtCompat::removeFileExtension(moduleName);
                setGroupLabelIDAndVersion(node, modulePath, moduleName);
            }
            onGroupCreationFinished(node, args.reason);

        } else if (args.reason == eCreateNodeReasonUserCreate && !_imp->_creatingGroup) {
            //if the node is a group and we're not loading the project, create one input and one output
            NodePtr input,output;
            
            {
                CreateNodeArgs args(PLUGINID_NATRON_OUTPUT, eCreateNodeReasonInternal, isGrp);
                output = createNode(args);
                try {
                    output->setScriptName("Output");
                } catch (...) {
                    
                }
                assert(output);
            }
            {
                CreateNodeArgs args(PLUGINID_NATRON_INPUT, eCreateNodeReasonInternal, isGrp);
                input = createNode(args);
                assert(input);
            }
            if (input && output && !output->getInput(0)) {
                output->connectInput(input, 0);
                
                double x,y;
                output->getPosition(&x, &y);
                y -= 100;
                input->setPosition(x, y);
            }
            onGroupCreationFinished(node, args.reason);

            ///Now that the group is created and all nodes loaded, autoconnect the group like other nodes.
        }

    }
    
    return node;
} // createNodeInternal

NodePtr
AppInstance::createNode(const CreateNodeArgs & args)
{
    return createNodeInternal(args);
}


int
AppInstance::getAppID() const
{
    return _imp->_appID;
}

NodePtr
AppInstance::getNodeByFullySpecifiedName(const std::string & name) const
{
    return _imp->_currentProject->getNodeByFullySpecifiedName(name);
}

boost::shared_ptr<Project>
AppInstance::getProject() const
{
    return _imp->_currentProject;
}

boost::shared_ptr<TimeLine>
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
AppInstance::errorDialog(const std::string & title,const std::string & message,bool* stopAsking,bool /*useHtml*/) const
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
AppInstance::warningDialog(const std::string & title,const std::string & message,bool* stopAsking,
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
AppInstance::informationDialog(const std::string & title,const std::string & message,bool* stopAsking,
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
AppInstance::startWritersRendering(bool enableRenderStats,bool doBlockingRender,
                                   const std::list<std::string>& writers,
                                   const std::list<std::pair<int,std::pair<int,int> > >& frameRanges)
{
    
    std::list<RenderWork> renderers;

    if (!writers.empty()) {
        for (std::list<std::string>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
            
            const std::string& writerName = *it;
            
            NodePtr node = getNodeByFullySpecifiedName(writerName);
           
            if (!node) {
                std::string exc(writerName);
                exc.append(tr(" does not belong to the project file. Please enter a valid Write node script-name.").toStdString());
                throw std::invalid_argument(exc);
            } else {
                if ( !node->isOutputNode() ) {
                    std::string exc(writerName);
                    exc.append(tr(" is not an output node! It cannot render anything.").toStdString());
                    throw std::invalid_argument(exc);
                }
                ViewerInstance* isViewer = node->isEffectViewer();
                if (isViewer) {
                    throw std::invalid_argument("Internal issue with the project loader...viewers should have been evicted from the project.");
                }
                
                OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
                assert(effect);
                
                for (std::list<std::pair<int,std::pair<int,int> > >::const_iterator it2 = frameRanges.begin(); it2!=frameRanges.end();++it2) {
                    RenderWork w;
                    w.writer = effect;
                    w.firstFrame = it2->second.first;
                    w.lastFrame = it2->second.second;
                    w.frameStep = it2->first;
                    renderers.push_back(w);
                }
                
                if (frameRanges.empty()) {
                    RenderWork r;
                    r.firstFrame = INT_MIN;
                    r.lastFrame = INT_MAX;
                    r.frameStep = INT_MIN;
                    r.writer = effect;
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
                
                for (std::list<std::pair<int,std::pair<int,int> > >::const_iterator it3 = frameRanges.begin(); it3!=frameRanges.end();++it3) {
                    RenderWork w;
                    w.writer = *it2;
                    w.firstFrame = it3->second.first;
                    w.lastFrame = it3->second.second;
                    w.frameStep = it3->first;
                    renderers.push_back(w);
                }
                
                if (frameRanges.empty()) {
                    RenderWork r;
                    r.firstFrame = INT_MIN;
                    r.lastFrame = INT_MAX;
                    r.frameStep = INT_MIN;
                    r.writer = *it2;
                    renderers.push_back(r);
                }
            }
        }
    }
    
    
    if (renderers.empty()) {
        throw std::invalid_argument("Project file is missing a writer node. This project cannot render anything.");
    }
    
    startWritersRendering(enableRenderStats, doBlockingRender, renderers);
}

void
AppInstance::startWritersRendering(bool enableRenderStats,bool doBlockingRender, const std::list<RenderWork>& writers)
{
    
    
    if (writers.empty()) {
        return;
    }
    
    getProject()->resetTotalTimeSpentRenderingForAllNodes();

    
    if (appPTR->isBackground() || doBlockingRender) {
        
        //blocking call, we don't want this function to return pre-maturely, in which case it would kill the app
        QtConcurrent::blockingMap( writers,boost::bind(&AppInstance::startRenderingBlockingFullSequence,this,enableRenderStats, _1,false,QString()) );
    } else {
        
        //Take a snapshot of the graph at this time, this will be the version loaded by the process
        bool renderInSeparateProcess = appPTR->getCurrentSettings()->isRenderInSeparatedProcessEnabled();
        QString savePath;
        getProject()->saveProject_imp("","RENDER_SAVE.ntp",true, false,&savePath);

        for (std::list<RenderWork>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
            ///Use the frame range defined by the writer GUI because we're in an interactive session
            startRenderingFullSequence(enableRenderStats,*it,renderInSeparateProcess,savePath);
        }
    }
}

void
AppInstance::startRenderingBlockingFullSequence(bool enableRenderStats,const RenderWork& writerWork,bool /*renderInSeparateProcess*/,const QString& /*savePath*/)
{
    BlockingBackgroundRender backgroundRender(writerWork.writer);
    double first,last;
    if (writerWork.firstFrame == INT_MIN || writerWork.lastFrame == INT_MAX) {
        writerWork.writer->getFrameRange_public(writerWork.writer->getHash(), &first, &last);
        if (first == INT_MIN || last == INT_MAX) {
            getFrameRange(&first, &last);
        }
    } else {
        first = writerWork.firstFrame;
        last = writerWork.lastFrame;
    }
    
    int frameStep;
    if (writerWork.frameStep == INT_MAX || writerWork.frameStep == INT_MIN) {
        ///Get the frame step from the frame step parameter of the Writer
        frameStep = writerWork.writer->getNode()->getFrameStepKnobValue();
    } else {
        frameStep = std::max(1, writerWork.frameStep);
    }
    
    backgroundRender.blockingRender(enableRenderStats,first,last,frameStep); //< doesn't return before rendering is finished
}

void
AppInstance::startRenderingFullSequence(bool enableRenderStats,const RenderWork& writerWork,bool renderInSeparateProcess,const QString& savePath)
{
    startRenderingBlockingFullSequence(enableRenderStats, writerWork, renderInSeparateProcess, savePath);
}

void
AppInstance::getFrameRange(double* first,double* last) const
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
    _imp->_currentProject->getActiveNodes(&activeNodes);
    
    for (NodesList::iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        (*it)->clearLastRenderedImage();
    }
}

void
AppInstance::aboutToQuit()
{
    ///Clear nodes now, not in the destructor of the project as
    ///deleting nodes might reference the project.
    _imp->_currentProject->closeProject(true);
    _imp->_currentProject->discardAppPointer();
    
}

void
AppInstance::quit()
{
    appPTR->quit(this);
}

ViewerColorSpaceEnum
AppInstance::getDefaultColorSpaceForBitDepth(ImageBitDepthEnum bitdepth) const
{
    return _imp->_currentProject->getDefaultColorSpaceForBitDepth(bitdepth);
}

void
AppInstance::onOCIOConfigPathChanged(const std::string& path)
{
    _imp->_currentProject->onOCIOConfigPathChanged(path,false);
}

void
AppInstance::declareCurrentAppVariable_Python()
{
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
    
    bool ok = Python::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::declareCurrentAppVariable_Python() failed!");
    }

    if (appPTR->isBackground()) {
        std::string err;
        ok = Python::interpretPythonScript("app = app1\n", &err, 0);
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
    if (cb.empty()) {
        return;
    }
    
    
    std::vector<std::string> args;
    std::string error;
    try {
        Python::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        appendToScriptEditor(std::string("Failed to run onProjectCreated callback: ")
                                                         + e.what());
        return;
    }
    if (!error.empty()) {
        appendToScriptEditor("Failed to run onProjectCreated callback: " + error);
        return;
    }
    
    std::string signatureError;
    signatureError.append("The on project created callback supports the following signature(s):\n");
    signatureError.append("- callback(app)");
    if (args.size() != 1) {
        appendToScriptEditor("Failed to run onProjectCreated callback: " + signatureError);
        return;
    }
    if (args[0] != "app") {
        appendToScriptEditor("Failed to run onProjectCreated callback: " + signatureError);
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
    if (!Python::interpretPythonScript(script, &err, &output)) {
        appendToScriptEditor("Failed to run onProjectCreated callback: " + err);
    } else {
        if (!output.empty()) {
            appendToScriptEditor(output);
        }
    }
}

std::string
AppInstance::getAppIDString() const
{
    if (appPTR->isBackground()) {
        return "app";
    } else {
        QString appID =  QString("app%1").arg(getAppID() + 1);
        return appID.toStdString();
    }
}

void
AppInstance::onGroupCreationFinished(const NodePtr& node,
                                     CreateNodeReason reason)
{
    assert(node);
    if (!_imp->_currentProject->isLoadingProject() && reason != eCreateNodeReasonProjectLoad && reason != eCreateNodeReasonCopyPaste) {
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
    boost::shared_ptr<Project> project= getProject();
    return project->saveProject_imp(path.c_str(), outFile.c_str(), false, false,0);
}

bool
AppInstance::save(const std::string& filename)
{
    boost::shared_ptr<Project> project= getProject();
    if (project->hasProjectBeenSavedByUser()) {
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
    return getProject()->saveProject(path.c_str(), outFile.c_str(), 0);
}

AppInstance*
AppInstance::loadProject(const std::string& filename)
{
    
    QFileInfo file(filename.c_str());
    if (!file.exists()) {
        return 0;
    }
    QString fileUnPathed = file.fileName();
    QString path = file.path() + "/";
    
    //We are in background mode, there can only be 1 instance active, wipe the current project
    boost::shared_ptr<Project> project = getProject();
    project->resetProject();
 
    bool ok  = project->loadProject( path, fileUnPathed);
    if (ok) {
        return this;
    }
    
    project->resetProject();
    
    return 0;
}

///Close the current project but keep the window
bool
AppInstance::resetProject()
{
    getProject()->closeProject(false);
    return true;
}

///Reset + close window, quit if last window
bool
AppInstance::closeProject()
{
    getProject()->closeProject(true);
    quit();
    return true;
}

///Opens a new project
AppInstance*
AppInstance::newProject()
{
    CLArgs cl;
    AppInstance* app = appPTR->newAppInstance(cl, false);
    return app;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_AppInstance.cpp"
