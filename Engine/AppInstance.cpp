/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <QDir>
#include <QtConcurrentMap>
#include <QThreadPool>
#include <QUrl>
#include <QFileInfo>
#include <QEventLoop>
#include <QSettings>

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

using namespace Natron;

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
    boost::shared_ptr<Natron::Project> _currentProject; //< ptr to the project
    int _appID; //< the unique ID of this instance (or window)
    bool _projectCreatedWithLowerCaseIDs;
    
    mutable QMutex creatingGroupMutex;
    bool _creatingGroup;
    bool _creatingNode;
    
    AppInstancePrivate(int appID,
                       AppInstance* app)
    : _currentProject( new Natron::Project(app) )
    , _appID(appID)
    , _projectCreatedWithLowerCaseIDs(false)
    , creatingGroupMutex()
    , _creatingGroup(false)
    , _creatingNode(false)
    {
    }
    
    void declareCurrentAppVariable_Python();
    
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

    ///Clear nodes now, not in the destructor of the project as
    ///deleting nodes might reference the project.
    _imp->_currentProject->clearNodes(false);
    _imp->_currentProject->discardAppPointer();
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
    if (a == NATRON_DEVELOPMENT_ALPHA) {
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
                QObject::tr("<p>You can download it from ") + QString("<a href='http://sourceforge.net/projects/natron/'>"
                                                                    "<font color=\"orange\">Sourceforge</a>. </p>");

            }
        
            Natron::informationDialog( "New version",text.toStdString(), true );
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
AppInstance::getWritersWorkForCL(const CLArgs& cl,std::list<AppInstance::RenderRequest>& requests)
{
    const std::list<CLArgs::WriterArg>& writers = cl.getWriterArgs();
    for (std::list<CLArgs::WriterArg>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
        AppInstance::RenderRequest r;
        
        int firstFrame = INT_MIN,lastFrame = INT_MAX;
        if (cl.hasFrameRange()) {
            const std::pair<int,int>& range = cl.getFrameRange();
            firstFrame = range.first;
            lastFrame = range.second;
        }
        
        if (it->mustCreate) {
            
            NodePtr writer = createWriter(it->filename.toStdString(), getProject(), firstFrame, lastFrame);
            
            //Connect the writer to the corresponding Output node input
            NodePtr output = getProject()->getNodeByFullySpecifiedName(it->name.toStdString());
            if (!output) {
                throw std::invalid_argument(it->name.toStdString() + tr(" is not the name of a valid Output node of the script").toStdString());
            }
            GroupOutput* isGrpOutput = dynamic_cast<GroupOutput*>(output->getLiveInstance());
            if (!isGrpOutput) {
                throw std::invalid_argument(it->name.toStdString() + tr(" is not the name of a valid Output node of the script").toStdString());
            }
            NodePtr outputInput = output->getRealInput(0);
            if (outputInput) {
                writer->connectInput(outputInput, 0);
            }
            
            r.writerName = writer->getScriptName().c_str();
            r.firstFrame = firstFrame;
            r.lastFrame = lastFrame;
        } else {
            r.writerName = it->name;
            r.firstFrame = firstFrame;
            r.lastFrame = lastFrame;
        }
        requests.push_back(r);
    }
}

NodePtr
AppInstance::createWriter(const std::string& filename,
                          const boost::shared_ptr<NodeCollection>& collection,
                          bool userEdited,
                          int firstFrame, int lastFrame)
{
    std::map<std::string,std::string> writersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    
    QString fileCpy(filename.c_str());
    std::string ext = Natron::removeFileExtension(fileCpy).toStdString();
    std::map<std::string,std::string>::iterator found = writersForFormat.find(ext);
    if ( found == writersForFormat.end() ) {
        Natron::errorDialog( tr("Writer").toStdString(),
                            tr("No plugin capable of encoding ").toStdString() + ext + tr(" was found.").toStdString(),false );
        return NodePtr();
    }
    
    
    CreateNodeArgs::DefaultValuesList defaultValues;
    defaultValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, filename));
    if (firstFrame != INT_MIN && lastFrame != INT_MAX) {
        defaultValues.push_back(createDefaultValueForParam<int>("frameRange", 2));
        defaultValues.push_back(createDefaultValueForParam<int>("firstFrame", firstFrame));
        defaultValues.push_back(createDefaultValueForParam<int>("lastFrame", lastFrame));
    }
    CreateNodeArgs args(found->second.c_str(),
                        "",
                        -1,
                        -1,
                        true,
                        INT_MIN,INT_MIN,
                        true,
                        true,
                        userEdited,
                        QString(),
                        defaultValues,
                        collection);
    return createNode(args);
    
}

void
AppInstance::load(const CLArgs& cl)
{
    
    declareCurrentAppVariable_Python();

    const QString& extraOnProjectCreatedScript = cl.getDefaultOnProjectLoadedScript();
    
    ///if the app is a background project autorun and the project name is empty just throw an exception.
    if ( (appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRun ||
          appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRunLaunchedFromGui)) {
        
        if (cl.getFilename().isEmpty()) {
            // cannot start a background process without a file
            throw std::invalid_argument(tr("Project file name empty").toStdString());
        }
        

        QFileInfo info(cl.getFilename());
        if (!info.exists()) {
            throw std::invalid_argument(tr("Specified project file does not exist").toStdString());
        }
        
        std::list<AppInstance::RenderRequest> writersWork;

        if (info.suffix() == NATRON_PROJECT_FILE_EXT) {
            
            if ( !_imp->_currentProject->loadProject(info.path(),info.fileName()) ) {
                throw std::invalid_argument(tr("Project file loading failed.").toStdString());
            }
            
            getWritersWorkForCL(cl, writersWork);

        } else if (info.suffix() == "py") {
            
            loadPythonScript(info);
            getWritersWorkForCL(cl, writersWork);

        } else {
            throw std::invalid_argument(tr(NATRON_APPLICATION_NAME " only accepts python scripts or .ntp project files").toStdString());
        }
        
        if (!extraOnProjectCreatedScript.isEmpty()) {
            QFileInfo cbInfo(extraOnProjectCreatedScript);
            if (cbInfo.exists()) {
                loadPythonScript(cbInfo);
            }
        }
        
        try {
            startWritersRendering(cl.areRenderStatsEnabled(),writersWork);
        } catch (const std::exception& e) {
            getProject()->removeLockFile();
            throw e;
        }
        
        
        
    } else if (appPTR->getAppType() == AppManager::eAppTypeInterpreter) {
        QFileInfo info(cl.getFilename());
        if (info.exists() && info.suffix() == "py") {
            loadPythonScript(info);
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
    bool ok  = interpretPythonScript(addToPythonPath, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::loadPythonScript(" + file.path().toStdString() + "): interpretPythonScript("+addToPythonPath+" failed!");
    }

    std::string s = "app = app1\n";
    ok = Natron::interpretPythonScript(s, &err, 0);
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
     
     
        ok = Natron::interpretPythonScript(hasCreateInstanceScript.toStdString(), &err, 0);

     
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
    
        
        std::string output;
        FlagSetter flag(true, &_imp->_creatingGroup, &_imp->creatingGroupMutex);
        if (!Natron::interpretPythonScript(moduleName.toStdString() + ".createInstance(app,app)", &err, &output)) {
            if (!err.empty()) {
                Natron::errorDialog(tr("Python").toStdString(), err);
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
    } else {
        QFile f(file.absoluteFilePath());
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
            message.append(file.absoluteFilePath());
            message.append(": ");
            message.append(error.c_str());
            appendToScriptEditor(message.toStdString());
        }
        
    }
    
    return true;
}

boost::shared_ptr<Natron::Node>
AppInstance::createNodeFromPythonModule(Natron::Plugin* plugin,
                                        const boost::shared_ptr<NodeCollection>& group,
                                        bool requestedByLoad,
                                        const NodeSerialization & serialization)

{
    QString pythonModulePath = plugin->getPythonModule();
    QString moduleName;
    QString modulePath;
    int foundLastSlash = pythonModulePath.lastIndexOf('/');
    if (foundLastSlash != -1) {
        modulePath = pythonModulePath.mid(0,foundLastSlash + 1);
        moduleName = pythonModulePath.remove(0,foundLastSlash + 1);
    }
    
    boost::shared_ptr<Natron::Node> node;
    
    {
        FlagSetter fs(true,&_imp->_creatingGroup,&_imp->creatingGroupMutex);
        
        NodePtr containerNode;
        if (!requestedByLoad) {
            CreateNodeArgs groupArgs(PLUGINID_NATRON_GROUP,
                                     "",
                                     -1,-1,
                                     true, //< autoconnect
                                     INT_MIN,INT_MIN,
                                     true, //< push undo/redo command
                                     true, // add to project
                                     true,
                                     QString(),
                                     CreateNodeArgs::DefaultValuesList(),
                                     group);
            containerNode = createNode(groupArgs);
            std::string containerName;
            group->initNodeName(plugin->getLabelWithoutSuffix().toStdString(),&containerName);
            containerNode->setScriptName(containerName);
            
            
        } else {
            
            LoadNodeArgs groupArgs(PLUGINID_NATRON_GROUP,
                                   "",
                                   -1,-1,
                                   &serialization,
                                   false,
                                   group);
            containerNode = loadNode(groupArgs);
        }
        if (!containerNode) {
            return containerNode;
        }
        
        
        std::string containerFullySpecifiedName = containerNode->getFullyQualifiedName();
        
        int appID = getAppID() + 1;
        
        std::stringstream ss;
        ss << moduleName.toStdString();
        ss << ".createInstance(app" << appID;
        ss << ", app" << appID << "." << containerFullySpecifiedName;
        ss << ")\n";
        std::string err;
        std::string output;
        if (!Natron::interpretPythonScript(ss.str(), &err, &output)) {
            Natron::errorDialog(tr("Group plugin creation error").toStdString(), err);
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
    onGroupCreationFinished(node, requestedByLoad);
    
    return node;
}


void
AppInstance::setGroupLabelIDAndVersion(const boost::shared_ptr<Natron::Node>& node,
                                       const QString& pythonModulePath,
                               const QString &pythonModule)
{
    std::string pluginID,pluginLabel,iconFilePath,pluginGrouping,description;
    unsigned int version;
    if (Natron::getGroupInfos(pythonModulePath.toStdString(),pythonModule.toStdString(), &pluginID, &pluginLabel, &iconFilePath, &pluginGrouping, &description, &version)) {
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
static int isEntitledForInspector(Natron::Plugin* plugin,OFX::Host::ImageEffect::Descriptor* ofxDesc)  {
    
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

boost::shared_ptr<Natron::Node>
AppInstance::createNodeInternal(const QString & pluginID,
                                const std::string & multiInstanceParentName,
                                int majorVersion,
                                int minorVersion,
                                bool requestedByLoad,
                                const NodeSerialization & serialization,
                                bool dontLoadName,
                                bool autoConnect,
                                double xPosHint,
                                double yPosHint,
                                bool pushUndoRedoCommand,
                                bool addToProject,
                                bool userEdited,
                                bool createGui,
                                const QString& fixedName,
                                const CreateNodeArgs::DefaultValuesList& paramValues,
                                const boost::shared_ptr<NodeCollection>& group)
{
    
    boost::shared_ptr<Node> node;
    Natron::Plugin* plugin = 0;

    try {
        plugin = appPTR->getPluginBinary(pluginID,majorVersion,minorVersion,_imp->_projectCreatedWithLowerCaseIDs);
    } catch (const std::exception & e1) {
        
        ///Ok try with the old Ids we had in Natron prior to 1.0
        try {
            plugin = appPTR->getPluginBinaryFromOldID(pluginID, majorVersion, minorVersion);
        } catch (const std::exception& e2) {
            Natron::errorDialog(tr("Plugin error").toStdString(),
                                tr("Cannot load plugin executable").toStdString() + ": " + e2.what(), false );
            return node;
        }
        
    }
    
    
    if (!plugin) {
        return node;
    }
    
    if (!plugin->getIsUserCreatable() && userEdited) {
        //The plug-in should not be instantiable by the user
        qDebug() << "Attempt to create" << pluginID << "which is not user creatable";
        return node;
    }

    const QString& pythonModule = plugin->getPythonModule();
    if (!pythonModule.isEmpty()) {
        return createNodeFromPythonModule(plugin, group, requestedByLoad, serialization);
    }

    std::string foundPluginID = plugin->getPluginID().toStdString();
    
    ContextEnum ctx;
    OFX::Host::ImageEffect::Descriptor* ofxDesc = plugin->getOfxDesc(&ctx);
    
    if (!ofxDesc) {
        OFX::Host::ImageEffect::ImageEffectPlugin* ofxPlugin = plugin->getOfxPlugin();
        if (ofxPlugin) {
            
            try {
                ofxDesc = Natron::OfxHost::getPluginContextAndDescribe(ofxPlugin,&ctx);
            } catch (const std::exception& e) {
                errorDialog(tr("Error while creating node").toStdString(), tr("Failed to create an instance of ").toStdString()
                            + pluginID.toStdString() + ": " + e.what(), false);
                return NodePtr();
            }
            assert(ofxDesc);
            plugin->setOfxDesc(ofxDesc, ctx);
        }
    }
    
    int nInputsForInspector = isEntitledForInspector(plugin,ofxDesc);
    
    if (!nInputsForInspector) {
        node.reset( new Node(this, addToProject ? group : boost::shared_ptr<NodeCollection>(), plugin) );
    } else {
        node.reset( new InspectorNode(this, addToProject ? group : boost::shared_ptr<NodeCollection>(), plugin,nInputsForInspector) );
    }
    
    {
        ///Furnace plug-ins don't handle using the thread pool
        boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
        if (foundPluginID.find("uk.co.thefoundry.furnace") != std::string::npos &&
            (settings->useGlobalThreadPool() || settings->getNumberOfParallelRenders() != 1)) {
            Natron::StandardButtonEnum reply = Natron::questionDialog(tr("Warning").toStdString(),
                                                                  tr("The settings of the application are currently set to use "
                                                                     "the global thread-pool for rendering effects. The Foundry Furnace "
                                                                     "is known not to work well when this setting is checked. "
                                                                     "Would you like to turn it off ? ").toStdString(), false);
            if (reply == Natron::eStandardButtonYes) {
                settings->setUseGlobalThreadPool(false);
                settings->setNumberOfParallelRenders(1);
            }
        }
    }
    
    
    if (addToProject) {
        //Add the node to the project before loading it so it is present when the python script that registers a variable of the name
        //of the node works
        group->addNode(node);
    }
    assert(node);
    try {
        node->load(multiInstanceParentName, serialization,dontLoadName, userEdited, addToProject, fixedName,paramValues);
    } catch (const std::exception & e) {
        group->removeNode(node);
        std::string title("Error while creating node");
        std::string message = title + " " + foundPluginID + ": " + e.what();
        qDebug() << message.c_str();
        errorDialog(title, message, false);

        return boost::shared_ptr<Natron::Node>();
    } catch (...) {
        group->removeNode(node);
        std::string title("Error while creating node");
        std::string message = title + " " + foundPluginID;
        qDebug() << message.c_str();
        errorDialog(title, message, false);

        return boost::shared_ptr<Natron::Node>();
    }

    boost::shared_ptr<Natron::Node> multiInstanceParent = node->getParentMultiInstance();
    
    if (createGui) {
        // createNodeGui also sets the filename parameter for reader or writers
        createNodeGui(node,
                      multiInstanceParent,
                      requestedByLoad,
                      autoConnect,
                      xPosHint,
                      yPosHint,
                      pushUndoRedoCommand);
    }
    
    boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>(node->getLiveInstance()->shared_from_this());

    if (isGrp) {
        
        if (requestedByLoad) {
            if (!serialization.isNull() && !serialization.getPythonModule().empty()) {
                
                QString pythonModulePath(serialization.getPythonModule().c_str());
                QString moduleName;
                QString modulePath;
                int foundLastSlash = pythonModulePath.lastIndexOf('/');
                if (foundLastSlash != -1) {
                    modulePath = pythonModulePath.mid(0,foundLastSlash + 1);
                    moduleName = pythonModulePath.remove(0,foundLastSlash + 1);
                }
                setGroupLabelIDAndVersion(node, modulePath, moduleName);
            }
        } else if (!requestedByLoad && !_imp->_creatingGroup) {
            //if the node is a group and we're not loading the project, create one input and one output
            NodePtr input,output;
            
            {
                CreateNodeArgs args(PLUGINID_NATRON_OUTPUT,
                                    std::string(),
                                    -1,
                                    -1,
                                    false, //< don't autoconnect
                                    INT_MIN,
                                    INT_MIN,
                                    false, //<< don't push an undo command
                                    true,
                                    false,
                                    QString(),
                                    CreateNodeArgs::DefaultValuesList(),
                                    isGrp);
                output = createNode(args);
                output->setScriptName("Output");
                assert(output);
            }
            {
                CreateNodeArgs args(PLUGINID_NATRON_INPUT,
                                    std::string(),
                                    -1,
                                    -1,
                                    true, // autoconnect
                                    INT_MIN,
                                    INT_MIN,
                                    false, //<< don't push an undo command
                                    true,
                                    false,
                                    QString(),
                                    CreateNodeArgs::DefaultValuesList(),
                                    isGrp);
                input = createNode(args);
                assert(input);
            }
            
            ///Now that the group is created and all nodes loaded, autoconnect the group like other nodes.
            onGroupCreationFinished(node, false);
        }
    }
    
    return node;
} // createNodeInternal

boost::shared_ptr<Natron::Node>
AppInstance::createNode(const CreateNodeArgs & args)
{
    return createNodeInternal(args.pluginID,
                              args.multiInstanceParentName,
                              args.majorV, args.minorV,
                              false,
                              NodeSerialization( boost::shared_ptr<Natron::Node>() ),
                              !args.fixedName.isEmpty(),
                              args.autoConnect,
                              args.xPosHint,args.yPosHint,
                              args.pushUndoRedoCommand,
                              args.addToProject,
                              args.userEdited,
                              args.createGui,
                              args.fixedName,
                              args.paramValues,
                              args.group);
}

boost::shared_ptr<Natron::Node>
AppInstance::loadNode(const LoadNodeArgs & args)
{
    return createNodeInternal(args.pluginID,
                              args.multiInstanceParentName,
                              args.majorV, args.minorV,
                              true,
                              *args.serialization,
                              args.dontLoadName,
                              false,
                              INT_MIN,INT_MIN,
                              false,
                              true,
                              false,
                              true,
                              QString(),
                              CreateNodeArgs::DefaultValuesList(),
                              args.group);
}

int
AppInstance::getAppID() const
{
    return _imp->_appID;
}

boost::shared_ptr<Natron::Node>
AppInstance::getNodeByFullySpecifiedName(const std::string & name) const
{
    return _imp->_currentProject->getNodeByFullySpecifiedName(name);
}

boost::shared_ptr<Natron::Project>
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


Natron::StandardButtonEnum
AppInstance::questionDialog(const std::string & title,
                            const std::string & message,
                            bool /*useHtml*/,
                            Natron::StandardButtons /*buttons*/,
                            Natron::StandardButtonEnum /*defaultButton*/) const
{
    std::cout << "QUESTION: " << title + ": " << message << std::endl;
    return Natron::eStandardButtonYes;
}



void
AppInstance::triggerAutoSave()
{
    _imp->_currentProject->triggerAutoSave();
}


void
AppInstance::startWritersRendering(bool enableRenderStats,const std::list<RenderRequest>& writers)
{
    
    std::list<RenderWork> renderers;

    if ( !writers.empty() ) {
        for (std::list<RenderRequest>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
            
            std::string writerName =  it->writerName.toStdString();
            
            NodePtr node = getNodeByFullySpecifiedName(writerName);
           
            if (!node) {
                std::string exc(writerName);
                exc.append(tr(" does not belong to the project file. Please enter a valid writer name.").toStdString());
                throw std::invalid_argument(exc);
            } else {
                if ( !node->isOutputNode() ) {
                    std::string exc(writerName);
                    exc.append(" is not an output node! It cannot render anything.");
                    throw std::invalid_argument(exc);
                }
                ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(node->getLiveInstance());
                if (isViewer) {
                    throw std::invalid_argument("Internal issue with the project loader...viewers should have been evicted from the project.");
                }
                
                RenderWork w;
                w.writer = dynamic_cast<OutputEffectInstance*>( node->getLiveInstance() );
                assert(w.writer);
                w.firstFrame = it->firstFrame;
                w.lastFrame = it->lastFrame;
                renderers.push_back(w);
            }
        }
    } else {
        //start rendering for all writers found in the project
        std::list<Natron::OutputEffectInstance*> writers;
        getProject()->getWriters(&writers);
        
        for (std::list<Natron::OutputEffectInstance*>::const_iterator it2 = writers.begin(); it2 != writers.end(); ++it2) {
            RenderWork w;
            w.writer = *it2;
            assert(w.writer);
            if (w.writer) {
                double f,l;
                w.writer->getFrameRange_public(w.writer->getHash(), &f, &l);
                w.firstFrame = std::floor(f);
                w.lastFrame = std::ceil(l);
            }
            renderers.push_back(w);
        }
    }
    
    startWritersRendering(enableRenderStats, renderers);
}

void
AppInstance::startWritersRendering(bool enableRenderStats,const std::list<RenderWork>& writers)
{
    
    
    if (writers.empty()) {
        return;
    }
    
    getProject()->resetTotalTimeSpentRenderingForAllNodes();

    
    if ( appPTR->isBackground() ) {
        
        //blocking call, we don't want this function to return pre-maturely, in which case it would kill the app
        QtConcurrent::blockingMap( writers,boost::bind(&AppInstance::startRenderingFullSequence,this,enableRenderStats, _1,false,QString()) );
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
AppInstance::startRenderingFullSequence(bool enableRenderStats,const RenderWork& writerWork,bool /*renderInSeparateProcess*/,const QString& /*savePath*/)
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
    
    backgroundRender.blockingRender(enableRenderStats,first,last); //< doesn't return before rendering is finished
}

void
AppInstance::getFrameRange(double* first,double* last) const
{
    return _imp->_currentProject->getFrameRange(first, last);
}

void
AppInstance::clearOpenFXPluginsCaches()
{
    NodeList activeNodes;
    _imp->_currentProject->getActiveNodes(&activeNodes);

    for (NodeList::iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        (*it)->purgeAllInstancesCaches();
    }
}

void
AppInstance::clearAllLastRenderedImages()
{
    NodeList activeNodes;
    _imp->_currentProject->getActiveNodes(&activeNodes);
    
    for (NodeList::iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        (*it)->clearLastRenderedImage();
    }
}



void
AppInstance::quit()
{
    appPTR->quit(this);
}

Natron::ViewerColorSpaceEnum
AppInstance::getDefaultColorSpaceForBitDepth(Natron::ImageBitDepthEnum bitdepth) const
{
    return _imp->_currentProject->getDefaultColorSpaceForBitDepth(bitdepth);
}

int
AppInstance::getMainView() const
{
    return _imp->_currentProject->getProjectMainView();
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
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->_currentProject->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss << "app" << _imp->_appID + 1 << "." << (*it)->getName() << " = app" << _imp->_appID + 1 << ".getProjectParam('" <<
        (*it)->getName() << "')\n";
    }
    std::string script = ss.str();
    std::string err;
    
    bool ok = Natron::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("AppInstance::declareCurrentAppVariable_Python() failed!");
    }

    if (appPTR->isBackground()) {
        std::string err;
        ok = Natron::interpretPythonScript("app = app1\n", &err, 0);
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
        Natron::getFunctionArguments(cb, &error, &args);
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
    if (!Natron::interpretPythonScript(script, &err, &output)) {
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
AppInstance::onGroupCreationFinished(const boost::shared_ptr<Natron::Node>& node,bool requestedByLoad)
{
    assert(node);
    if (!_imp->_currentProject->isLoadingProject() && !requestedByLoad) {
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>(node->getLiveInstance());
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
    boost::shared_ptr<Natron::Project> project= getProject();
    return project->saveProject_imp(path.c_str(), outFile.c_str(), false, false,0);
}

bool
AppInstance::save(const std::string& filename)
{
    boost::shared_ptr<Natron::Project> project= getProject();
    if (project->hasProjectBeenSavedByUser()) {
        QString projectName = project->getProjectName();
        QString projectPath = project->getProjectPath();
        return project->saveProject(projectPath, projectName, 0);
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
    
    CLArgs cl;
    AppInstance* app = appPTR->newAppInstance(cl);
    
    bool ok  = app->getProject()->loadProject( path, fileUnPathed);
    if (ok) {
        return app;
    }
    
    app->getProject()->closeProject(true);
    app->quit();
    
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
    AppInstance* app = appPTR->newAppInstance(cl);
    return app;
}
