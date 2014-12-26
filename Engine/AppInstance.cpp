//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "AppInstance.h"

#include <list>
#include <stdexcept>

#include <QDir>
#include <QtConcurrentMap>
#include <QThreadPool>
#include <QUrl>
#include <QEventLoop>
#include <QSettings>

#include <boost/bind.hpp>

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

using namespace Natron;

struct AppInstancePrivate
{
    boost::shared_ptr<Natron::Project> _currentProject; //< ptr to the project
    int _appID; //< the unique ID of this instance (or window)


    AppInstancePrivate(int appID,
                       AppInstance* app)
        : _currentProject( new Natron::Project(app) )
          , _appID(appID)
    {
    }
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
    QThreadPool::globalInstance()->waitForDone();

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
AppInstance::load(const QString & projectName,
                  const std::list<RenderRequest>& writersWork)
{
    if ( (getAppID() == 0) && appPTR->getCurrentSettings()->isCheckForUpdatesEnabled() ) {
        QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
        settings.beginGroup("General");
        bool checkUpdates = true;
        if ( !settings.contains("checkForUpdates") ) {
            Natron::StandardButtonEnum reply = Natron::questionDialog("Updates", "Do you want " NATRON_APPLICATION_NAME " to check for updates "
                                                                  "on launch of the application ?", false);
            if (reply == Natron::eStandardButtonNo) {
                checkUpdates = false;
            }
            appPTR->getCurrentSettings()->setCheckUpdatesEnabled(checkUpdates);
        }
        if (checkUpdates) {
            checkForNewVersion();
        }
    }

    ///if the app is a background project autorun and the project name is empty just throw an exception.
    if ( (appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRun ||
          appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRunLaunchedFromGui) && projectName.isEmpty() ) {
        // cannot start a background process without a file
        throw std::invalid_argument("Project file name empty");
    }
    if (appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRun ||
        appPTR->getAppType() == AppManager::eAppTypeBackgroundAutoRunLaunchedFromGui) {
        QString realProjectName = projectName;
        int lastSep = realProjectName.lastIndexOf( QDir::separator() );
        if (lastSep == -1) {
            throw std::invalid_argument("Filename has no path. It must be absolute.");
        }

        QString path = realProjectName.left(lastSep);
        if ( !path.isEmpty() && ( path.at(path.size() - 1) != QDir::separator() ) ) {
            path += QDir::separator();
        }

        QString name = realProjectName.remove(path);

        if ( !_imp->_currentProject->loadProject(path,name) ) {
            throw std::invalid_argument("Project file loading failed.");
        }
        startWritersRendering(writersWork);
    }
}


boost::shared_ptr<Natron::Node>
AppInstance::createNodeInternal(const QString & pluginID,
                                const std::string & multiInstanceParentName,
                                int majorVersion,
                                int minorVersion,
                                bool requestedByLoad,
                                const NodeSerialization & serialization,
                                bool dontLoadName,
                                int childIndex,
                                bool autoConnect,
                                double xPosHint,
                                double yPosHint,
                                bool pushUndoRedoCommand,
                                bool addToProject,
                                const QString& fixedName,
                                const CreateNodeArgs::DefaultValuesList& paramValues,
                                const boost::shared_ptr<NodeCollection>& group)
{
    boost::shared_ptr<Node> node;
    Natron::Plugin* plugin = 0;

    try {
        plugin = appPTR->getPluginBinary(pluginID,majorVersion,minorVersion);
    } catch (const std::exception & e1) {
        
        ///Ok try with the old Ids we had in Natron prior to 1.0
        try {
            plugin = appPTR->getPluginBinaryFromOldID(pluginID, majorVersion, minorVersion);
        } catch (const std::exception& e2) {
            Natron::errorDialog( "Plugin error", std::string("Cannot load plugin executable") + ": " + e2.what(), false );
            return node;
        }
        
    }
    if (!plugin) {
        return node;
    }

    std::string foundPluginID = plugin->getPluginID().toStdString();
    if (foundPluginID != NATRON_VIEWER_ID) { // for now only the viewer can be an inspector.
        node.reset( new Node(this, group, plugin) );
    } else {
        node.reset( new InspectorNode(this, group, plugin) );
    }
    
    {
        ///Furnace plug-ins don't handle using the thread pool
        if (foundPluginID.find("Furnace") != std::string::npos && appPTR->getUseThreadPool()) {
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
        _imp->_currentProject->addNode(node);
    }
    assert(node);
    try {
        node->load(foundPluginID,multiInstanceParentName,childIndex, serialization,dontLoadName,fixedName,paramValues);
    } catch (const std::exception & e) {
        _imp->_currentProject->removeNode(node);
        std::string title = std::string("Error while creating node");
        std::string message = title + " " + foundPluginID + ": " + e.what();
        qDebug() << message.c_str();
        errorDialog(title, message, false);

        return boost::shared_ptr<Natron::Node>();
    } catch (...) {
        _imp->_currentProject->removeNode(node);
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


    return node;
} // createNodeInternal

boost::shared_ptr<Natron::Node>
AppInstance::createNode(const CreateNodeArgs & args)
{
    ///use the same entry point to create backdrops.
    ///Since they are purely GUI we don't actually return a node.
    if ( args.pluginID == QString(NATRON_BACKDROP_NODE_NAME) ) {
        createBackDrop();

        return boost::shared_ptr<Natron::Node>();
    }

    return createNodeInternal(args.pluginID,
                              args.multiInstanceParentName,
                              args.majorV, args.minorV,
                              false,
                              NodeSerialization( boost::shared_ptr<Natron::Node>() ),
                              !args.fixedName.isEmpty(),
                              args.childIndex,
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
                              -1,
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
AppInstance::getNodeByName(const std::string & name) const
{
    return _imp->_currentProject->getNodeByName(name);
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
    NodeList projectNodes;
    _imp->_currentProject->getActiveNodes(&projectNodes);
    
   
    std::list<RenderWork> renderers;

    if ( !writers.empty() ) {
        for (std::list<RenderRequest>::const_iterator it = writers.begin(); it != writers.end(); ++it) {
            
            boost::shared_ptr<Node> node;
            std::string writerName =  it->writerName.toStdString();
            
            for (NodeList::const_iterator it2 = projectNodes.begin(); it2 != projectNodes.end(); ++it2) {
                if ( (*it2)->getName() == writerName) {
                    node = *it2;
                    break;
                }
            }
            if (!node) {
                std::string exc(writerName);
                exc.append(" does not belong to the project file. Please enter a valid writer name.");
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
        for (NodeList::const_iterator it2 = projectNodes.begin(); it2 != projectNodes.end(); ++it2) {
            if ( (*it2)->getLiveInstance()->isWriter() ) {
                
                RenderWork w;
                w.writer = dynamic_cast<OutputEffectInstance*>( (*it2)->getLiveInstance() );
                assert(w.writer);
                if (w.writer) {
                    w.writer->getFrameRange_public(w.writer->getHash(), &w.firstFrame, &w.lastFrame);
                }
                renderers.push_back(w);
            }
        }
    }
    
    startWritersRendering(renderers);
}

void
AppInstance::startWritersRendering(const std::list<RenderWork>& writers)
{
    
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
            first = getTimeLine()->leftBound();
            last = getTimeLine()->rightBound();
        }
    } else {
        first = writerWork.firstFrame;
        last = writerWork.lastFrame;
    }
    
    backgroundRender.blockingRender(first,last); //< doesn't return before rendering is finished
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

std::size_t
AppInstance::declareCurrentAppVariable_Python(std::string& script)
{
    size_t firstLine = ensureScriptHasModuleImport(NATRON_ENGINE_PYTHON_MODULE_NAME,script);
    
    ///Now define the app variable
    std::stringstream ss;
    ss << "app = getInstance(" << getAppID() << ") \n";
    std::string toInsert = ss.str();
    script.insert(firstLine, toInsert);
    
    return firstLine + toInsert.size();

}

double
AppInstance::getProjectFrameRate() const
{
    return _imp->_currentProject->getProjectFrameRate();
}
