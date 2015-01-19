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


#include <boost/bind.hpp>

#include "Global/QtCompat.h"

#include "Engine/Project.h"
#include "Engine/Plugin.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/NodeSerialization.h"
#include "Engine/FileDownloader.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"
#include "Engine/NoOp.h"

using namespace Natron;

class FlagSetter {
    
    bool* p;
public:
    
    FlagSetter(bool initialValue,bool* p)
    : p(p)
    {
        *p = initialValue;
    }
    
    ~FlagSetter()
    {
        *p = !*p;
    }
};


struct AppInstancePrivate
{
    boost::shared_ptr<Natron::Project> _currentProject; //< ptr to the project
    int _appID; //< the unique ID of this instance (or window)
    bool _projectCreatedWithLowerCaseIDs;
    
    bool _creatingGroup;
    
    AppInstancePrivate(int appID,
                       AppInstance* app)
    : _currentProject( new Natron::Project(app) )
    , _appID(appID)
    , _projectCreatedWithLowerCaseIDs(false)
    , _creatingGroup(false)
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
AppInstance::checkForNewVersion() const
{
    FileDownloader* downloader = new FileDownloader( QUrl(NATRON_LAST_VERSION_URL) );
    QObject::connect( downloader, SIGNAL( downloaded() ), this, SLOT( newVersionCheckDownloaded() ) );
    QObject::connect( downloader, SIGNAL( error() ), this, SLOT( newVersionCheckError() ) );

    ///make the call blocking
    QEventLoop loop;

    connect( downloader->getReply(), SIGNAL( finished() ), &loop, SLOT( quit() ) );
    loop.exec();
}

//return -1 if a < b, 0 if a == b and 1 if a > b
//Returns -2 if not understood
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
                        QString(),
                        defaultValues,
                        collection);
    return createNode(args);
    
}

void
AppInstance::load(const CLArgs& cl)
{

    ///if the app is a background project autorun and the project name is empty just throw an exception.
    if ( (appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRun ||
          appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRunLaunchedFromGui)) {
        
        if (cl.getFilename().isEmpty()) {
            // cannot start a background process without a file
            throw std::invalid_argument(tr("Project file name empty").toStdString());
        }
        
        
        QFileInfo info(cl.getFilename());
        if (!info.exists()) {
            throw std::invalid_argument(tr("Specified file does not exist").toStdString());
        }
        
        std::list<AppInstance::RenderRequest> writersWork;

        if (info.suffix() == NATRON_PROJECT_FILE_EXT) {
            
            if ( !_imp->_currentProject->loadProject(info.path(),info.fileName()) ) {
                throw std::invalid_argument(tr("Project file loading failed.").toStdString());
            }
            
            getWritersWorkForCL(cl, writersWork);
            startWritersRendering(writersWork);

        } else if (info.suffix() == "py") {
            
            loadPythonScript(info);
            getWritersWorkForCL(cl, writersWork);

        } else {
            throw std::invalid_argument(tr(NATRON_APPLICATION_NAME " only accepts python scripts or .ntp project files").toStdString());
        }
        
        startWritersRendering(writersWork);
        
    } else if (appPTR->getAppType() == AppManager::eAppTypeInterpreter) {
        QFileInfo info(cl.getFilename());
        if (info.exists() && info.suffix() == "py") {
            loadPythonScript(info);
        }
        
        
        appPTR->launchPythonInterpreter();
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

    ok = Natron::interpretPythonScript("app = app1\n", &err, 0);
    assert(ok);
    
    QString filename = file.fileName();
    int lastDotPos = filename.lastIndexOf(QChar('.'));
    if (lastDotPos != -1) {
        filename = filename.left(lastDotPos);
    }
    
    QString hasCreateInstanceScript = QString("import sys\n"
                                              "import %1\n"
                                              "ret = True\n"
                                              "if not hasattr(%1,\"createInstance\") or not hasattr(%1.createInstance,\"__call__\"):\n"
                                              "    ret = False\n").arg(filename);
    
    
    ok = Natron::interpretPythonScript(hasCreateInstanceScript.toStdString(), &err, 0);
    
    if (!ok) {
        Natron::errorDialog(tr("Python").toStdString(), err);
        return false;
    }
    
    
    PyObject* mainModule = getMainModule();
    PyObject* retObj = PyObject_GetAttrString(mainModule,"ret"); //new ref
    assert(retObj);
    bool hasCreateInstance = PyObject_IsTrue(retObj) == 1;
    Py_XDECREF(retObj);
    
    ok = interpretPythonScript("del ret\n", &err, 0);
    assert(ok);
    
    if (hasCreateInstance) {
        std::string output;
        FlagSetter flag(true, &_imp->_creatingGroup);
        if (!Natron::interpretPythonScript(filename.toStdString() + ".createInstance(app,app)", &err, &output)) {
            Natron::errorDialog(tr("Python").toStdString(), err);
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
    }
    
    return true;
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
                                const QString& fixedName,
                                const CreateNodeArgs::DefaultValuesList& paramValues,
                                const boost::shared_ptr<NodeCollection>& group)
{
    assert(group);
    
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
    

    const QString& pythonModule = plugin->getPythonModule();
    if (!pythonModule.isEmpty()) {
        
        FlagSetter fs(true,&_imp->_creatingGroup);
        
        CreateNodeArgs groupArgs(PLUGINID_NATRON_GROUP,
                                 "",
                                 -1,-1,
                                 true, //< autoconnect
                                 INT_MIN,INT_MIN,
                                 true, //< push undo/redo command
                                 true, // add to project
                                 QString(),
                                 CreateNodeArgs::DefaultValuesList(),
                                 group);
        NodePtr containerNode = createNode(groupArgs);
        if (!containerNode) {
            return containerNode;
        }
        std::string containerName;
        group->initNodeName(plugin->getPluginLabel().toStdString(),&containerName);
        containerNode->setScriptName(containerName);
        
        if (!requestedByLoad) {
            std::string containerFullySpecifiedName = containerNode->getFullyQualifiedName();
            
            int appID = getAppID() + 1;
            
            std::stringstream ss;
            ss << pythonModule.toStdString();
            ss << ".createInstance(app" << appID;
            ss << ", app" << appID << "." << containerFullySpecifiedName;
            ss << ")\n";
            std::string err;
            if (!Natron::interpretPythonScript(ss.str(), &err, NULL)) {
                Natron::errorDialog(tr("Group plugin creation error").toStdString(), err);
                containerNode->destroyNode(false);
                return node;
            } else {
                return containerNode;
            }
        } else {
            containerNode->loadKnobs(serialization);
            if (!serialization.isNull() && !serialization.getUserPages().empty()) {
                containerNode->getLiveInstance()->refreshKnobs();
            }
            return containerNode;
        }
    }

    std::string foundPluginID = plugin->getPluginID().toStdString();

    if (foundPluginID != PLUGINID_NATRON_VIEWER) { // for now only the viewer can be an inspector.
        node.reset( new Node(this, addToProject ? group : boost::shared_ptr<NodeCollection>(), plugin) );
    } else {
        node.reset( new InspectorNode(this, addToProject ? group : boost::shared_ptr<NodeCollection>(), plugin) );
    }
    
    {
        ///Furnace plug-ins don't handle using the thread pool
        if (foundPluginID.find("uk.co.thefoundry.furnace") != std::string::npos && appPTR->getUseThreadPool()) {
            Natron::StandardButtonEnum reply = Natron::questionDialog(tr("Warning").toStdString(),
                                                                  tr("The settings of the application are currently set to use "
                                                                     "the global thread-pool for rendering effects. The Foundry Furnace "
                                                                     "is known not to work well when this setting is checked. "
                                                                     "Would you like to turn it off ? ").toStdString(), false);
            if (reply == Natron::eStandardButtonYes) {
                appPTR->getCurrentSettings()->setUseGlobalThreadPool(false);
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
        node->load(multiInstanceParentName, serialization,dontLoadName,fixedName,paramValues);
    } catch (const std::exception & e) {
        group->removeNode(node);
        std::string title = std::string("Error while creating node");
        std::string message = title + " " + foundPluginID + ": " + e.what();
        qDebug() << message.c_str();
        errorDialog(title, message, false);

        return boost::shared_ptr<Natron::Node>();
    } catch (...) {
        group->removeNode(node);
        std::string title = std::string("Error while creating node");
        std::string message = title + " " + foundPluginID;
        qDebug() << message.c_str();
        errorDialog(title, message, false);

        return boost::shared_ptr<Natron::Node>();
    }

    boost::shared_ptr<Natron::Node> multiInstanceParent = node->getParentMultiInstance();

    // createNodeGui also sets the filename parameter for reader or writers
    createNodeGui(node,
                  multiInstanceParent,
                  requestedByLoad,
                  autoConnect,
                  xPosHint,
                  yPosHint,
                  pushUndoRedoCommand);
    
    boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>(node->getLiveInstance()->shared_from_this());

    if (isGrp && !requestedByLoad && !_imp->_creatingGroup) {
        
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
                                QString(),
                                CreateNodeArgs::DefaultValuesList(),
                                isGrp);
            input = createNode(args);
            assert(input);
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
AppInstance::startWritersRendering(const std::list<RenderRequest>& writers)
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
                w.writer->getFrameRange_public(w.writer->getHash(), &w.firstFrame, &w.lastFrame);
            }
            renderers.push_back(w);
        }
    }
    
    startWritersRendering(renderers);
}

void
AppInstance::startWritersRendering(const std::list<RenderWork>& writers)
{
    
    if (writers.empty()) {
        return;
    }
    
    if ( appPTR->isBackground() ) {
        
        //blocking call, we don't want this function to return pre-maturely, in which case it would kill the app
        QtConcurrent::blockingMap( writers,boost::bind(&AppInstance::startRenderingFullSequence,this,_1,false,QString()) );
    } else {
        
        //Take a snapshot of the graph at this time, this will be the version loaded by the process
        bool renderInSeparateProcess = appPTR->getCurrentSettings()->isRenderInSeparatedProcessEnabled();
        QString savePath = getProject()->saveProject("","RENDER_SAVE.ntp",true);

        for (std::list<RenderWork>::const_iterator it = writers.begin();it!=writers.end();++it) {
            ///Use the frame range defined by the writer GUI because we're in an interactive session
            startRenderingFullSequence(*it,renderInSeparateProcess,savePath);
        }
    }
}

void
AppInstance::startRenderingFullSequence(const RenderWork& writerWork,bool /*renderInSeparateProcess*/,const QString& /*savePath*/)
{
    BlockingBackgroundRender backgroundRender(writerWork.writer);
    int first,last;
    if (writerWork.firstFrame == INT_MIN || writerWork.lastFrame == INT_MAX) {
        writerWork.writer->getFrameRange_public(writerWork.writer->getHash(), &first, &last);
        if (first == INT_MIN || last == INT_MAX) {
            getFrameRange(&first, &last);
        }
    } else {
        first = writerWork.firstFrame;
        last = writerWork.lastFrame;
    }
    
    backgroundRender.blockingRender(first,last); //< doesn't return before rendering is finished
}

void
AppInstance::getFrameRange(int* first,int* last) const
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
    ss << "app" << _imp->_appID + 1 << " = getInstance(" << _imp->_appID << ") \n";
    std::string script = ss.str();
    std::string err;
    bool ok = Natron::interpretPythonScript(script, &err, 0);
    assert(ok);
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
    return _imp->_creatingGroup;
}

void
AppInstance::appendToScriptEditor(const std::string& str)
{
    std::cout << str <<  std::endl;
}
