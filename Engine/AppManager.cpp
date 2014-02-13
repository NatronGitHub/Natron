//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "AppManager.h"

#include <cassert>

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/archive/archive_exception.hpp>

#include <QMessageBox>
#include <QLabel>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QtGui/QPixmapCache>
#include <QBitmap>


#if QT_VERSION < 0x050000
#include <QtGui/QDesktopServices>
#else
#include <QStandardPaths>
#endif

#include "Global/MemoryInfo.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Project.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Knob.h"
#include "Engine/OfxHost.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Log.h"
#include "Engine/KnobFile.h"
#include "Engine/TextureRectSerialization.h"

// FIXME: too much Gui stuff in here!
#pragma message WARN("too much Gui stuff in the Engine")
#include "Gui/ViewerGL.h"
#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGraph.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/ProjectGuiSerialization.h"
#include "Gui/ProcessHandler.h"
#include "Gui/SplashScreen.h"

#include "Gui/QtDecoder.h"

#include "Gui/QtEncoder.h"

using namespace Natron;
using std::cout; using std::endl;
using std::make_pair;

struct KnobsClipBoard {
    KnobSerialization k; //< the serialized knob to copy
    bool isEmpty; //< is the clipboard empty
    bool copyAnimation; //< should we copy all the animation or not
};

void AppManager::getIcon(Natron::PixmapEnum e,QPixmap* pix) const {
    if(!QPixmapCache::find(QString::number(e),pix)){
        QImage img;
        switch(e){
        case NATRON_PIXMAP_PLAYER_PREVIOUS:
            img.load(NATRON_IMAGES_PATH"back1.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_FIRST_FRAME:
            img.load(NATRON_IMAGES_PATH"firstFrame.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_NEXT:
            img.load(NATRON_IMAGES_PATH"forward1.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_LAST_FRAME:
            img.load(NATRON_IMAGES_PATH"lastFrame.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_NEXT_INCR:
            img.load(NATRON_IMAGES_PATH"nextIncr.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_NEXT_KEY:
            img.load(NATRON_IMAGES_PATH"nextKF.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_PREVIOUS_INCR:
            img.load(NATRON_IMAGES_PATH"previousIncr.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_PREVIOUS_KEY:
            img.load(NATRON_IMAGES_PATH"prevKF.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_REWIND:
            img.load(NATRON_IMAGES_PATH"rewind.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_PLAY:
            img.load(NATRON_IMAGES_PATH"play.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_STOP:
            img.load(NATRON_IMAGES_PATH"stop.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_PLAYER_LOOP_MODE:
            img.load(NATRON_IMAGES_PATH"loopmode.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_MAXIMIZE_WIDGET:
            img.load(NATRON_IMAGES_PATH"maximize.png");
            *pix = QPixmap::fromImage(img).scaled(25,25);
            break;
        case NATRON_PIXMAP_MINIMIZE_WIDGET:
            img.load(NATRON_IMAGES_PATH"minimize.png");
            *pix = QPixmap::fromImage(img).scaled(15,15);
            break;
        case NATRON_PIXMAP_CLOSE_WIDGET:
            img.load(NATRON_IMAGES_PATH"close.png");
            *pix = QPixmap::fromImage(img).scaled(15,15);
            break;
        case NATRON_PIXMAP_HELP_WIDGET:
            img.load(NATRON_IMAGES_PATH"help.png");
            *pix = QPixmap::fromImage(img).scaled(15,15);
            break;
        case NATRON_PIXMAP_GROUPBOX_FOLDED:
            img.load(NATRON_IMAGES_PATH"groupbox_folded.png");
            *pix = QPixmap::fromImage(img).scaled(20, 20);
            break;
        case NATRON_PIXMAP_GROUPBOX_UNFOLDED:
            img.load(NATRON_IMAGES_PATH"groupbox_unfolded.png");
            *pix = QPixmap::fromImage(img).scaled(20, 20);
            break;
        case NATRON_PIXMAP_UNDO:
            img.load(NATRON_IMAGES_PATH"undo.png");
            *pix = QPixmap::fromImage(img).scaled(15, 15);
            break;
        case NATRON_PIXMAP_REDO:
            img.load(NATRON_IMAGES_PATH"redo.png");
            *pix = QPixmap::fromImage(img).scaled(15, 15);
            break;
        case NATRON_PIXMAP_UNDO_GRAYSCALE:
            img.load(NATRON_IMAGES_PATH"undo_grayscale.png");
            *pix = QPixmap::fromImage(img).scaled(15, 15);
            break;
        case NATRON_PIXMAP_REDO_GRAYSCALE:
            img.load(NATRON_IMAGES_PATH"redo_grayscale.png");
            *pix = QPixmap::fromImage(img).scaled(15, 15);
            break;
        case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON:
            img.load(NATRON_IMAGES_PATH"layout.png");
            *pix = QPixmap::fromImage(img).scaled(12,12);
            break;
        case NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY:
            img.load(NATRON_IMAGES_PATH"splitHorizontally.png");
            *pix = QPixmap::fromImage(img).scaled(12,12);
            break;
        case NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY:
            img.load(NATRON_IMAGES_PATH"splitVertically.png");
            *pix = QPixmap::fromImage(img).scaled(12,12);
            break;
        case NATRON_PIXMAP_VIEWER_CENTER:
            img.load(NATRON_IMAGES_PATH"centerViewer.png");
            *pix = QPixmap::fromImage(img).scaled(50, 50);
            break;
        case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT:
            img.load(NATRON_IMAGES_PATH"cliptoproject.png");
            *pix = QPixmap::fromImage(img).scaled(50, 50);
            break;
        case NATRON_PIXMAP_VIEWER_REFRESH:
            img.load(NATRON_IMAGES_PATH"refresh.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_VIEWER_ROI:
            img.load(NATRON_IMAGES_PATH"viewer_roi.png");
            *pix = QPixmap::fromImage(img).scaled(20,20);
            break;
        case NATRON_PIXMAP_OPEN_FILE:
            img.load(NATRON_IMAGES_PATH"open-file.png");
            *pix = QPixmap::fromImage(img).scaled(15,15);
            break;
        case NATRON_PIXMAP_RGBA_CHANNELS:
            img.load(NATRON_IMAGES_PATH"RGBAchannels.png");
            *pix = QPixmap::fromImage(img).scaled(10,10);
            break;
        case NATRON_PIXMAP_COLORWHEEL:
            img.load(NATRON_IMAGES_PATH"colorwheel.png");
            *pix = QPixmap::fromImage(img).scaled(25, 20);
            break;
        case NATRON_PIXMAP_COLOR_PICKER:
            img.load(NATRON_IMAGES_PATH"color_picker.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_IO_GROUPING:
            img.load(NATRON_IMAGES_PATH"io_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_COLOR_GROUPING:
            img.load(NATRON_IMAGES_PATH"color_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_TRANSFORM_GROUPING:
            img.load(NATRON_IMAGES_PATH"transform_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_DEEP_GROUPING:
            img.load(NATRON_IMAGES_PATH"deep_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_FILTER_GROUPING:
            img.load(NATRON_IMAGES_PATH"filter_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_MULTIVIEW_GROUPING:
            img.load(NATRON_IMAGES_PATH"multiview_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_MISC_GROUPING:
            img.load(NATRON_IMAGES_PATH"misc_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_OPEN_EFFECTS_GROUPING:
            img.load(NATRON_IMAGES_PATH"openeffects.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_TIME_GROUPING:
            img.load(NATRON_IMAGES_PATH"time_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_PAINT_GROUPING:
            img.load(NATRON_IMAGES_PATH"paint_low.png");
            *pix = QPixmap::fromImage(img);
            break;
        case NATRON_PIXMAP_COMBOBOX:
            img.load(NATRON_IMAGES_PATH"combobox.png");
            *pix = QPixmap::fromImage(img).scaled(10, 10);
            break;
        case NATRON_PIXMAP_COMBOBOX_PRESSED:
            img.load(NATRON_IMAGES_PATH"pressed_combobox.png");
            *pix = QPixmap::fromImage(img).scaled(10, 10);
            break;
        case NATRON_PIXMAP_APP_ICON:
            img.load(NATRON_APPLICATION_ICON_PATH);
            *pix = QPixmap::fromImage(img);
            break;
        default:
            assert(!"Missing image.");
        }
        QPixmapCache::insert(QString::number(e),*pix);
    }
}


AppInstance::AppInstance(AppInstance::AppType appType,int appID)
  : _gui(NULL)
  , _currentProject(new Natron::Project(this))
  , _appID(appID)
  , _nodeMapping()
  , _appType(appType)
  , _isQuitting(false)
{
    appPTR->registerAppInstance(this);
    appPTR->setAsTopLevelInstance(appID);
}

void
AppInstance::load(const QString& projectName,const QStringList& writers)
{
    QObject::connect(_currentProject.get(), SIGNAL(nodesCleared()), this, SLOT(onProjectNodesCleared()));
    
    if(!isBackground()){
        appPTR->setLoadingStatus("Creating user interface...");
        _gui = new Gui(this);
        _gui->createGui();
    }
    if(_appType == APP_BACKGROUND_AUTO_RUN && projectName.isEmpty()){
        // cannot start a background process without a file
        throw std::invalid_argument("Project file name empty");
    }
    
    _currentProject->initializeKnobsPublic();
    
    if(!isBackground()){
        _gui->initProjectGuiKnobs();
        const std::vector<PluginToolButton*>& _toolButtons = appPTR->getPluginsToolButtons();
        for (U32 i = 0; i < _toolButtons.size(); ++i) {
            assert(_toolButtons[i]);
            _gui->findOrCreateToolButton(_toolButtons[i]);
        }
        emit pluginsPopulated();
        _gui->show();
        
        /* Create auto-save dir if it does not exists*/
        QDir dir = Project::autoSavesDir();
        dir.mkpath(".");
        
        /*If this is the first instance of the software*/
        if(_appID == 0){
            if(!_currentProject->findAndTryLoadAutoSave()){
                if(projectName.isEmpty()){
                    QString text(QCoreApplication::applicationName() + " - ");
                    text.append(_currentProject->getProjectName());
                    _gui->setWindowTitle(text);
                    createNode("Viewer");
                }else{
                    QString name = SequenceFileDialog::removePath(projectName);
                    QString path = projectName.left(projectName.indexOf(name));
                    appPTR->setLoadingStatus("Loading project: " + path + name);
                    _currentProject->loadProject(path,name);
                }
            }
        }else{
            if(projectName.isEmpty()){
                QString text(QCoreApplication::applicationName() + " - ");
                text.append(_currentProject->getProjectName());
                _gui->setWindowTitle(text);
                createNode("Viewer");
            }else{
                QString name = SequenceFileDialog::removePath(projectName);
                QString path = projectName.left(projectName.indexOf(name));
                _currentProject->loadProject(path,name);
            }
        }

    }else{
        
        if (_appType == APP_BACKGROUND_AUTO_RUN) {
            QString name = SequenceFileDialog::removePath(projectName);
            QString path = projectName.left(projectName.indexOf(name));
            if(!_currentProject->loadProject(path,name)){
                throw std::invalid_argument("Project file loading failed.");
                
            }
            startWritersRendering(writers);
        }
    }
    
    
}


AppInstance::~AppInstance(){
    {
        QMutexLocker l(&_isQuittingMutex);
        _isQuitting = true;

    }
    appPTR->removeInstance(_appID);

    _currentProject.reset();
}

void AppInstance::triggerAutoSave() {
    _currentProject->triggerAutoSave();
}

Node* AppInstance::createNode(const QString& name,int majorVersion,int minorVersion,bool requestedByLoad,bool openImageFileDialog) {
    Node* node = 0;
    LibraryBinary* pluginBinary = 0;
    try {
        pluginBinary = appPTR->getPluginBinary(name,majorVersion,minorVersion);
    } catch (const std::exception& e) {
        Natron::errorDialog("Plugin error", std::string("Cannot load plugin executable") + ": " + e.what());
        return node;
    } catch (...) {
        Natron::errorDialog("Plugin error", std::string("Cannot load plugin executable"));
        return node;
    }
    
    try{
        if(name != "Viewer"){ // for now only the viewer can be an inspector.
            node = new Node(this,pluginBinary,name.toStdString());
        }else{
            node = new InspectorNode(this,pluginBinary,name.toStdString());
        }
    } catch (const std::exception& e) {
        std::string title = std::string("Exception while creating node");
        std::string message = title + " " + name.toStdString() + ": " + e.what();
        qDebug() << message.c_str();
        errorDialog(title, message);
        delete node;
        return NULL;
    } catch (...) {
        std::string title = std::string("Exception while creating node");
        std::string message = title + " " + name.toStdString();
        qDebug() << message.c_str();
        errorDialog(title, message);
        delete node;
        return NULL;
    }
    

    QObject::connect(node,SIGNAL(deactivated()),this,SLOT(checkViewersConnection()));
    QObject::connect(node, SIGNAL(deactivated()), this, SLOT(triggerAutoSave()));
    QObject::connect(node,SIGNAL(activated()),this,SLOT(checkViewersConnection()));
    QObject::connect(node, SIGNAL(activated()), this, SLOT(triggerAutoSave()));

    NodeGui* nodegui = 0;
    if(!isBackground()){
        nodegui = _gui->createNodeGUI(node);
        assert(nodegui);
        _nodeMapping.insert(make_pair(node,nodegui));
    }
    node->initializeKnobs();
    node->initializeInputs();

    _currentProject->initNodeCountersAndSetName(node);

    if(!isBackground()){
        assert(nodegui);
        _gui->addNodeGuiToCurveEditor(nodegui);
    }

    if(node->pluginID() == "Viewer" && !isBackground()){
        _gui->createViewerGui(node);
    }
    

    if(!isBackground() && !requestedByLoad){
        if(_gui->getSelectedNode()){
            Node* selected = _gui->getSelectedNode()->getNode();
            _currentProject->autoConnectNodes(selected, node);
        }
        _gui->selectNode(nodegui);
                
        if (openImageFileDialog && !requestedByLoad) {
            node->getLiveInstance()->openImageFileKnob();
        }
    }
    
 
    return node;
}


int AppInstance::getAppID() const {return _appID;}

bool AppInstance::isBackground() const {return _appType != APP_GUI;}

const std::vector<NodeGui*>& AppInstance::getVisibleNodes() const{
    assert(_gui->_nodeGraphArea);
    return  _gui->_nodeGraphArea->getAllActiveNodes();
    
}

bool AppInstance::shouldRefreshPreview() const {
    return !isBackground() && !_gui->isUserScrubbingTimeline();
}

void AppInstance::getActiveNodes(std::vector<Natron::Node*>* activeNodes) const{
    
    const std::vector<Natron::Node*> nodes = _currentProject->getCurrentNodes();
    for(U32 i = 0; i < nodes.size(); ++i){
        if(nodes[i]->isActivated()){
            activeNodes->push_back(nodes[i]);
        }
    }
}




void AppInstance::deselectAllNodes() const{
    _gui->_nodeGraphArea->deselect();
}

void AppInstance::errorDialog(const std::string& title,const std::string& message) const{
    _gui->errorDialog(title, message);
}

void AppInstance::warningDialog(const std::string& title,const std::string& message) const{
    _gui->warningDialog(title, message);
    
}

void AppInstance::informationDialog(const std::string& title,const std::string& message) const{
    _gui->informationDialog(title, message);
    
}

Natron::StandardButton AppInstance::questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons,
                                                   Natron::StandardButton defaultButton) const{
    return _gui->questionDialog(title, message,buttons,defaultButton);
}

AppInstance* AppManager::newAppInstance(AppInstance::AppType appType,const QString& projectName,const QStringList& writers){
    AppInstance* instance = new AppInstance(appType,_availableID);
    try {
        instance->load(projectName,writers);
    } catch (const std::exception& e) {
        Natron::errorDialog(NATRON_APPLICATION_NAME, std::string("Cannot create project") + ": " + e.what());
        removeInstance(_availableID);
        delete instance;
        return NULL;
    } catch (...) {
        Natron::errorDialog(NATRON_APPLICATION_NAME, std::string("Cannot create project"));
        removeInstance(_availableID);
        delete instance;
        return NULL;
    }
    ++_availableID;
    
    ///flag that we finished loading the Appmanager even if it was already true
    _loaded = true;
    return instance;
}

AppInstance* AppManager::getAppInstance(int appID) const{
    std::map<int,AppInstance*>::const_iterator it;
    it = _appInstances.find(appID);
    if(it != _appInstances.end()){
        return it->second;
    }else{
        return NULL;
    }
}

const std::map<int,AppInstance*>&  AppManager::getAppInstances() const{
    return _appInstances;
}

void AppManager::removeInstance(int appID){
    _appInstances.erase(appID);
}




NodeGui* AppInstance::getNodeGui(Node* n) const {
    std::map<Node*,NodeGui*>::const_iterator it = _nodeMapping.find(n);
    if(it==_nodeMapping.end()){
        return NULL;
    }else{
        assert(it->second);
        return it->second;
    }
}

NodeGui* AppInstance::getNodeGui(const std::string& nodeName) const{
    for(std::map<Node*,NodeGui*>::const_iterator it = _nodeMapping.begin();
        it != _nodeMapping.end();++it){
        assert(it->first && it->second);
        if(it->first->getName() == nodeName){
            return it->second;
        }
    }
    return (NodeGui*)NULL;
}

Node* AppInstance::getNode(NodeGui* n) const{
    for (std::map<Node*,NodeGui*>::const_iterator it = _nodeMapping.begin(); it!=_nodeMapping.end(); ++it) {
        if(it->second == n){
            return it->first;
        }
    }
    return NULL;
    
}

void AppInstance::connectViewersToViewerCache(){
    foreach(Node* n,_currentProject->getCurrentNodes()){
        assert(n);
        if(n->pluginID() == "Viewer"){
            dynamic_cast<ViewerInstance*>(n->getLiveInstance())->connectSlotsToViewerCache();
        }
    }
}

void AppInstance::disconnectViewersFromViewerCache(){
    foreach(Node* n,_currentProject->getCurrentNodes()){
        assert(n);
        if(n->pluginID() == "Viewer"){
            dynamic_cast<ViewerInstance*>(n->getLiveInstance())->disconnectSlotsToViewerCache();
        }
    }
}


void AppInstance::onProjectNodesCleared() {
    _nodeMapping.clear();
}


void AppManager::clearPlaybackCache(){
    _viewerCache->clearInMemoryPortion();

}


void AppManager::clearDiskCache(){
    _viewerCache->clear();
    
}


void AppManager::clearNodeCache(){
    _nodeCache->clear();
    
}

void AppManager::clearPluginsLoadedCache() {
    ofxHost->clearPluginsLoadedCache();
}

void AppManager::clearAllCaches() {
    clearDiskCache();
    clearNodeCache();
    
    ///for each app instance clear all its nodes cache
    for (std::map<int,AppInstance*>::iterator it = _appInstances.begin(); it!=_appInstances.end(); ++it) {
        it->second->clearOpenFXPluginsCaches();
    }
}

std::vector<LibraryBinary*> AppManager::loadPlugins(const QString &where){
    std::vector<LibraryBinary*> ret;
    QDir d(where);
    if (d.isReadable())
    {
        QStringList filters;
        filters << QString(QString("*.")+QString(NATRON_LIBRARY_EXT));
        d.setNameFilters(filters);
        QStringList fileList = d.entryList();
        for(int i = 0 ; i < fileList.size() ; ++i) {
            QString filename = fileList.at(i);
            if(filename.endsWith(".dll") || filename.endsWith(".dylib") || filename.endsWith(".so")){
                QString className;
                int index = filename.lastIndexOf("." NATRON_LIBRARY_EXT);
                className = filename.left(index);
                std::string binaryPath = NATRON_PLUGINS_PATH + className.toStdString() + "." + NATRON_LIBRARY_EXT;
                LibraryBinary* plugin = new LibraryBinary(binaryPath);
                if(!plugin->isValid()){
                    delete plugin;
                }else{
                    ret.push_back(plugin);
                }
            }else{
                continue;
            }
        }
    }
    return ret;
}

std::vector<Natron::LibraryBinary*> AppManager::loadPluginsAndFindFunctions(const QString& where,
                                                                            const std::vector<std::string>& functions){
    std::vector<LibraryBinary*> ret;
    std::vector<LibraryBinary*> loadedLibraries = loadPlugins(where);
    for (U32 i = 0; i < loadedLibraries.size(); ++i) {
        if (loadedLibraries[i]->loadFunctions(functions)) {
            ret.push_back(loadedLibraries[i]);
        }
    }
    return ret;
}

AppInstance* AppManager::getTopLevelInstance () const{
    std::map<int,AppInstance*>::const_iterator it = _appInstances.find(_topLevelInstanceID);
    if(it == _appInstances.end()){
        return NULL;
    }else{
        return it->second;
    }
}

AppManager::AppManager()
    : QObject()
    , Singleton<AppManager>()
    , _appInstances()
    , _availableID(0)
    , _topLevelInstanceID(0)
    , _settings(new Settings(NULL))
    , _formats()
    , _plugins()
    , ofxHost(new Natron::OfxHost())
    , _toolButtons()
    , _knobFactory(new KnobFactory())
    , _knobGuiFactory(new KnobGuiFactory())
    , _nodeCache()
    , _viewerCache()
    ,_colorPickerCursor(NULL)
    ,_initialized(false)
    ,_knobsClipBoard(new KnobsClipBoard)
    ,_backgroundIPC(0)
    ,_splashScreen(0)
    ,_loaded(false)
    ,_binaryPath()
{

}

bool AppManager::isLoaded() const {
    return _loaded;
}

void AppManager::hideSplashScreen() {
    if(_splashScreen) {
        _splashScreen->hide();
        delete _splashScreen;
        _splashScreen = 0;
    }
}

void AppManager::load(SplashScreen* splashScreen,const QString& binaryPath) {
    
    _binaryPath = binaryPath;
    _splashScreen = splashScreen;
    
    if (_initialized) {
        return;
    }
    
    _settings->initializeKnobsPublic();
    
    
    connect(ofxHost.get(), SIGNAL(toolButtonAdded(QStringList,QString,QString,QString,QString)),
            this, SLOT(addPluginToolButtons(QStringList,QString,QString,QString,QString)));
    size_t maxCacheRAM = _settings->getRamMaximumPercent() * getSystemTotalRAM();
    U64 maxDiskCache = _settings->getMaximumDiskCacheSize();
    U64 playbackSize = maxCacheRAM * _settings->getRamPlaybackMaximumPercent();
    
    setLoadingStatus("Restoring the image cache...");
    _nodeCache.reset(new Cache<Image>("NodeCache",0x1, maxCacheRAM - playbackSize,1));
    _viewerCache.reset(new Cache<FrameEntry>("ViewerCache",0x1,maxDiskCache,(double)playbackSize / (double)maxDiskCache));
    
    qDebug() << "NodeCache RAM size: " << printAsRAM(_nodeCache->getMaximumMemorySize());
    qDebug() << "ViewerCache RAM size (playback-cache): " << printAsRAM(_viewerCache->getMaximumMemorySize());
    qDebug() << "ViewerCache disk size: " << printAsRAM(maxDiskCache);
    
    
    /*loading all plugins*/
    loadAllPlugins();
    loadBuiltinFormats();
    
    //we're in bg mode
    if(_splashScreen) {
        createColorPickerCursor();
    }
    
    setLoadingStatus("Restoring user settings...");
    
    _knobsClipBoard->isEmpty = true;
    
    ///flag initialized before restoring settings otherwise the value changes wouldn't be taken into account
    _initialized = true;

    _settings->restoreSettings();
    
    ///and save these restored settings in case some couldn't be found
    _settings->saveSettings();
    
    
}

void AppManager::initProcessInputChannel(const QString& mainProcessServerName) {
    _backgroundIPC = new ProcessInputChannel(mainProcessServerName);
}


void AppManager::abortAnyProcessing() {
    for (std::map<int,AppInstance*>::iterator it =_appInstances.begin(); it!= _appInstances.end(); ++it) {
        std::vector<Natron::Node*> nodes;
        it->second->getActiveNodes(&nodes);
        for (U32 i = 0; i < nodes.size(); ++i) {
            nodes[i]->quitAnyProcessing();
        }
    }
}

bool AppManager::writeToOutputPipe(const QString& longMessage,const QString& shortMessage) {
    if(!_backgroundIPC) {
        qDebug() << longMessage;
        return false;
    }
    _backgroundIPC->writeToOutputChannel(shortMessage);
    return true;
}

void AppManager::registerAppInstance(AppInstance* app){
    _appInstances.insert(std::make_pair(app->getAppID(),app));
}

void AppManager::setApplicationsCachesMaximumMemoryPercent(double p){
    size_t maxCacheRAM = p * getSystemTotalRAM();
    U64 playbackSize = maxCacheRAM * _settings->getRamPlaybackMaximumPercent();
    _nodeCache->setMaximumCacheSize(maxCacheRAM - playbackSize);
    _nodeCache->setMaximumInMemorySize(1);
    U64 maxDiskCacheSize = _settings->getMaximumDiskCacheSize();
    _viewerCache->setMaximumInMemorySize((double)playbackSize / (double)maxDiskCacheSize);
    
    qDebug() << "NodeCache RAM size: " << printAsRAM(_nodeCache->getMaximumMemorySize());
    qDebug() << "ViewerCache RAM size (playback-cache): " << printAsRAM(_viewerCache->getMaximumMemorySize());
    qDebug() << "ViewerCache disk size: " << printAsRAM(maxDiskCacheSize);
}

void AppManager::setApplicationsCachesMaximumDiskSpace(unsigned long long size){
    size_t maxCacheRAM = _settings->getRamMaximumPercent() * getSystemTotalRAM();
    U64 playbackSize = maxCacheRAM * _settings->getRamPlaybackMaximumPercent();
    _viewerCache->setMaximumCacheSize(size);
    _viewerCache->setMaximumInMemorySize((double)playbackSize / (double)size);
    
    qDebug() << "NodeCache RAM size: " << printAsRAM(_nodeCache->getMaximumMemorySize());
    qDebug() << "ViewerCache RAM size (playback-cache): " << printAsRAM(_viewerCache->getMaximumMemorySize());
    qDebug() << "ViewerCache disk size: " << printAsRAM(size);
}

void AppManager::setPlaybackCacheMaximumSize(double p){
    size_t maxCacheRAM = _settings->getRamMaximumPercent() * getSystemTotalRAM();
    U64 playbackSize = maxCacheRAM * p;
    _nodeCache->setMaximumCacheSize(maxCacheRAM - playbackSize);
    _nodeCache->setMaximumInMemorySize(1);
    U64 maxDiskCacheSize = _settings->getMaximumDiskCacheSize();
    _viewerCache->setMaximumInMemorySize((double)playbackSize / (double)maxDiskCacheSize);
    
    qDebug() << "NodeCache RAM size: " << printAsRAM(_nodeCache->getMaximumMemorySize());
    qDebug() << "ViewerCache RAM size (playback-cache): " << printAsRAM(_viewerCache->getMaximumMemorySize());
    qDebug() << "ViewerCache disk size: " << printAsRAM(maxDiskCacheSize);
}

AppManager::~AppManager(){
    
    assert(_appInstances.empty());
    
    _settings->saveSettings();
    
    for(U32 i = 0; i < _plugins.size();++i){
        delete _plugins[i];
    }
    foreach(Format* f,_formats){
        delete f;
    }
    foreach(PluginToolButton* p,_toolButtons){
        delete p;
    }

    if (_backgroundIPC) {
        delete _backgroundIPC;
    }

}
void AppManager::quit(){
    delete appPTR;
}

void AppManager::loadAllPlugins() {
    assert(_plugins.empty());
    assert(_formats.empty());
    assert(_toolButtons.empty());
    

    
    std::map<std::string,std::vector<std::string> > readersMap,writersMap;
    
    /*loading node plugins*/
    
    loadNodePlugins(&readersMap,&writersMap);
    
    /*loading ofx plugins*/
    ofxHost->loadOFXPlugins(&_plugins,&readersMap,&writersMap);
    
    _settings->populateReaderPluginsAndFormats(readersMap);
    _settings->populateWriterPluginsAndFormats(writersMap);
    
}

void AppManager::loadNodePlugins(std::map<std::string,std::vector<std::string> >* readersMap,
                                 std::map<std::string,std::vector<std::string> >* writersMap){
    std::vector<std::string> functions;
    functions.push_back("BuildEffect");
    std::vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(NATRON_NODES_PLUGINS_PATH, functions);
    for (U32 i = 0 ; i < plugins.size(); ++i) {
        std::pair<bool,EffectBuilder> func = plugins[i]->findFunction<EffectBuilder>("BuildEffect");
        if(func.first){
            EffectInstance* effect = func.second(NULL);
            assert(effect);
            QMutex* pluginMutex = NULL;
            if(effect->renderThreadSafety() == Natron::EffectInstance::UNSAFE){
                pluginMutex = new QMutex(QMutex::Recursive);
            }
            Natron::Plugin* plugin = new Natron::Plugin(plugins[i],effect->pluginID().c_str(),effect->pluginLabel().c_str(),
                                                        pluginMutex,effect->majorVersion(),effect->minorVersion());
            _plugins.push_back(plugin);
            delete effect;
        }
    }
    
    loadBuiltinNodePlugins(readersMap,writersMap);
}

void AppManager::loadBuiltinNodePlugins(std::map<std::string,std::vector<std::string> >* readersMap,
                                        std::map<std::string,std::vector<std::string> >* writersMap){
    // these  are built-in nodes
    QStringList grouping;
    grouping.push_back("Image"); // Readers, Writers, and Generators are in the "Image" group in Nuke
    {
        QtReader* reader = dynamic_cast<QtReader*>(QtReader::BuildEffect(NULL));
        assert(reader);
        std::map<std::string,void*> readerFunctions;
        readerFunctions.insert(make_pair("BuildEffect", (void*)&QtReader::BuildEffect));
        LibraryBinary *readerPlugin = new LibraryBinary(readerFunctions);
        assert(readerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(readerPlugin,reader->pluginID().c_str(),reader->pluginLabel().c_str(),
                                                    (QMutex*)NULL,reader->majorVersion(),reader->minorVersion());
        _plugins.push_back(plugin);
        addPluginToolButtons(grouping,reader->pluginID().c_str(),reader->pluginLabel().c_str(), "","");
        
        std::vector<std::string> extensions;
        reader->supportedFileFormats(&extensions);
        for(U32 k = 0; k < extensions.size();++k){
            std::map<std::string,std::vector<std::string> >::iterator it;
            it = readersMap->find(extensions[k]);
            
            if(it != readersMap->end()){
                it->second.push_back(reader->pluginID());
            }else{
                std::vector<std::string> newVec(1);
                newVec[0] = reader->pluginID();
                readersMap->insert(std::make_pair(extensions[k], newVec));
            }
        }

        
        delete reader;
    }
    {
        EffectInstance* viewer = ViewerInstance::BuildEffect(NULL);
        assert(viewer);
        std::map<std::string,void*> viewerFunctions;
        viewerFunctions.insert(make_pair("BuildEffect", (void*)&ViewerInstance::BuildEffect));
        LibraryBinary *viewerPlugin = new LibraryBinary(viewerFunctions);
        assert(viewerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(viewerPlugin,viewer->pluginID().c_str(),viewer->pluginLabel().c_str(),
                                                    (QMutex*)NULL,viewer->majorVersion(),viewer->minorVersion());
        _plugins.push_back(plugin);
        addPluginToolButtons(grouping,viewer->pluginID().c_str(),viewer->pluginLabel().c_str(), "","");
        delete viewer;
    }
    {
        QtWriter* writer = dynamic_cast<QtWriter*>(QtWriter::BuildEffect(NULL));
        assert(writer);
        std::map<std::string,void*> writerFunctions;
        writerFunctions.insert(make_pair("BuildEffect", (void*)&QtWriter::BuildEffect));
        LibraryBinary *writerPlugin = new LibraryBinary(writerFunctions);
        assert(writerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(writerPlugin,writer->pluginID().c_str(),writer->pluginLabel().c_str(),
                                                    (QMutex*)NULL,writer->majorVersion(),writer->minorVersion());
        _plugins.push_back(plugin);
        addPluginToolButtons(grouping,writer->pluginID().c_str(),writer->pluginLabel().c_str(), "","");
        
        std::vector<std::string> extensions;
        writer->supportedFileFormats(&extensions);
        for(U32 k = 0; k < extensions.size();++k){
            std::map<std::string,std::vector<std::string> >::iterator it;
            it = writersMap->find(extensions[k]);
            
            if(it != writersMap->end()){
                it->second.push_back(writer->pluginID());
            }else{
                std::vector<std::string> newVec(1);
                newVec[0] = writer->pluginID();
                writersMap->insert(std::make_pair(extensions[k], newVec));
            }
        }
        
        delete writer;
    }
}



void AppManager::loadBuiltinFormats(){
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
    
    assert(formatNames.size() == resolutions.size());
    for(U32 i =0;i<formatNames.size();++i) {
        const std::vector<double>& v = resolutions[i];
        assert(v.size() >= 3);
        Format* _frmt = new Format(0, 0, (int)v[0], (int)v[1], formatNames[i], v[2]);
        assert(_frmt);
        _formats.push_back(_frmt);
    }
    
}



Format* AppManager::findExistingFormat(int w, int h, double pixel_aspect) const {
    
    for(U32 i =0;i< _formats.size();++i) {
        Format* frmt = _formats[i];
        assert(frmt);
        if(frmt->width() == w && frmt->height() == h && frmt->getPixelAspect() == pixel_aspect){
            return frmt;
        }
    }
    return NULL;
}

void AppManager::createColorPickerCursor(){
    QPixmap pickerPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_COLOR_PICKER, &pickerPix);
    pickerPix = pickerPix.scaled(16, 16);
    pickerPix.setMask(pickerPix.createHeuristicMask());
    _colorPickerCursor = new QCursor(pickerPix,0,pickerPix.height());
}


void PluginToolButton::tryAddChild(PluginToolButton* plugin){
    for(unsigned int i = 0; i < _children.size() ; ++i){
        if(_children[i] == plugin)
            return;
    }
    _children.push_back(plugin);
}

void AppManager::addPluginToolButtons(const QStringList& groups,
                                      const QString& pluginID,
                                      const QString& pluginLabel,
                                      const QString& pluginIconPath,
                                      const QString& groupIconPath){
    PluginToolButton* parent = NULL;
    for(int i = 0; i < groups.size();++i){
        PluginToolButton* child = findPluginToolButtonOrCreate(groups.at(i),groups.at(i),groupIconPath);
        if(parent){
            parent->tryAddChild(child);
            child->setParent(parent);
        }
        parent = child;
        
    }
    PluginToolButton* lastChild = findPluginToolButtonOrCreate(pluginID,pluginLabel,pluginIconPath);
    if(parent){
        parent->tryAddChild(lastChild);
        lastChild->setParent(parent);
    }

    //_toolButtons.push_back(new PluginToolButton(groups,pluginName,pluginIconPath,groupIconPath));
}
PluginToolButton* AppManager::findPluginToolButtonOrCreate(const QString& pluginID,const QString& label,const QString& iconPath){
    for(U32 i = 0 ; i < _toolButtons.size();++i){
        if(_toolButtons[i]->getID() == pluginID)
            return _toolButtons[i];
    }
    PluginToolButton* ret = new PluginToolButton(pluginID,label,iconPath);
    _toolButtons.push_back(ret);
    return ret;
}

void AppManager::setAsTopLevelInstance(int appID){
    if(_topLevelInstanceID == appID){
        return;
    }
    _topLevelInstanceID = appID;
    for(std::map<int,AppInstance*>::iterator it = _appInstances.begin();it!=_appInstances.end();++it){
        if (it->first != _topLevelInstanceID) {
            if(!it->second->isBackground())
                it->second->disconnectViewersFromViewerCache();
        }else{
            if(!it->second->isBackground())
                it->second->connectViewersToViewerCache();
        }
    }
}

void AppInstance::checkViewersConnection(){
    const std::vector<Node*> nodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->pluginID() == "Viewer") {
            ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
            assert(n);
            n->updateTreeAndRender();
        }
    }
}

void AppInstance::redrawAllViewers() {
    const std::vector<Node*>& nodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->pluginID() == "Viewer") {
            ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
            assert(n);
            n->redrawViewer();
        }
    }
}

void AppInstance::setupViewersForViews(int viewsCount) {
    if (_gui) {
        _gui->updateViewersViewsMenu(viewsCount);
    }
}


void AppInstance::setViewersCurrentView(int view) {
    if (_gui) {
        _gui->setViewersCurrentView(view);
    }
}


boost::shared_ptr<TimeLine> AppInstance::getTimeLine() const  {
    return _currentProject->getTimeLine();
}

void AppInstance::loadProjectGui(boost::archive::xml_iarchive& archive) const {
    if (!isBackground()) {
        _gui->loadProjectGui(archive);
    }
}

void AppInstance::saveProjectGui(boost::archive::xml_oarchive& archive) {
    if (!isBackground()) {
        _gui->saveProjectGui(archive);
    }
}

void AppManager::clearExceedingEntriesFromNodeCache(){
    _nodeCache->clearExceedingEntries();
}

QStringList AppManager::getNodeNameList() const{
    QStringList ret;
    for (U32 i = 0; i < _plugins.size(); ++i) {
        ret.append(_plugins[i]->getPluginID());
    }
    return ret;
}

QMutex* AppManager::getMutexForPlugin(const QString& pluginId) const {
    for (U32 i = 0; i < _plugins.size(); ++i) {
        if(_plugins[i]->getPluginID() == pluginId){
            return _plugins[i]->getPluginLock();
        }
    }
    std::string exc("Couldn't find a plugin named ");
    exc.append(pluginId.toStdString());
    throw std::invalid_argument(exc);
}
void AppManager::printPluginsLoaded(){
    for(U32 i = 0; i < _plugins.size(); ++i){
        std::cout << _plugins[i]->getPluginID().toStdString() << std::endl;
    }
}

Natron::LibraryBinary* AppManager::getPluginBinary(const QString& pluginId,int majorVersion,int minorVersion) const{
    std::map<int,Natron::Plugin*> matches;
    for (U32 i = 0; i < _plugins.size(); ++i) {
        if(_plugins[i]->getPluginID() != pluginId){
            continue;
        }
        if(majorVersion != -1 && _plugins[i]->getMajorVersion() != majorVersion){
            continue;
        }
        matches.insert(std::make_pair(_plugins[i]->getMinorVersion(),_plugins[i]));
    }
    
    if(matches.empty()){
        QString exc = QString("Couldn't find a plugin named %1, with a major version of %2 and a minor version greater or equal to %3.")
        .arg(pluginId)
        .arg(majorVersion)
        .arg(minorVersion);
        throw std::invalid_argument(exc.toStdString());
    }else{
        std::map<int,Natron::Plugin*>::iterator greatest = matches.end();
        --greatest;
        return greatest->second->getLibraryBinary();
    }
}



void AppManager::setKnobClipBoard(const KnobSerialization& s,bool copyAnimation) {
    _knobsClipBoard->k = s;
    _knobsClipBoard->copyAnimation = copyAnimation;
    _knobsClipBoard->isEmpty = false;
}

bool AppManager::isClipBoardEmpty() const{
    return  _knobsClipBoard->isEmpty;
}

void AppManager::getKnobClipBoard(KnobSerialization* k,bool* copyAnimation) const{
    *k = _knobsClipBoard->k;
    *copyAnimation = copyAnimation;
}
void AppInstance::startWritersRendering(const QStringList& writers){
    
    const std::vector<Node*>& projectNodes = _currentProject->getCurrentNodes();
    
    std::vector<Natron::OutputEffectInstance*> renderers;

    if(!writers.isEmpty()){
        for (int i = 0; i < writers.size(); ++i) {
            Node* node = 0;
            for (U32 j = 0; j < projectNodes.size(); ++j) {
                if(projectNodes[j]->getName() == writers.at(i).toStdString()){
                    node = projectNodes[j];
                    break;
                }
            }
            if(!node){
                std::string exc(writers.at(i).toStdString());
                exc.append(" does not belong to the project file. Please enter a valid writer name.");
                throw std::invalid_argument(exc);
            }else{
                if(!node->isOutputNode()){
                    std::string exc(writers.at(i).toStdString());
                    exc.append(" is not an output node! It cannot render anything.");
                    throw std::invalid_argument(exc);
                }
                if(node->pluginID() == "Viewer"){
                    throw std::invalid_argument("Internal issue with the project loader...viewers should have been evicted from the project.");
                }
                renderers.push_back(dynamic_cast<OutputEffectInstance*>(node->getLiveInstance()));
            }
        }
    }else{
        //start rendering for all writers found in the project
        for (U32 j = 0; j < projectNodes.size(); ++j) {
            if(projectNodes[j]->isOutputNode() && projectNodes[j]->pluginID() != "Viewer"){
                renderers.push_back(dynamic_cast<OutputEffectInstance*>(projectNodes[j]->getLiveInstance()));
            }
        }

    }
    
    if(isBackground()){
        //blocking call, we don't want this function to return pre-maturely, in which case it would kill the app
        QtConcurrent::blockingMap(renderers,boost::bind(&AppInstance::startRenderingFullSequence,this,_1));
    }else{
        for (U32 i = 0; i < renderers.size(); ++i) {
            startRenderingFullSequence(renderers[i]);
        }
    }
}

void AppInstance::startRenderingFullSequence(Natron::OutputEffectInstance* writer){
    if(!isBackground()){

        /*Start the renderer in a background process.*/
        _currentProject->autoSave(); //< takes a snapshot of the graph at this time, this will be the version loaded by the process
    
        ProcessHandler* newProcess = 0;
        try {
            newProcess = new ProcessHandler(this,_currentProject->getLastAutoSaveFilePath() ,writer); //< the process will delete itself
        } catch (const std::exception& e) {
            Natron::errorDialog(writer->getName(), std::string("Error while starting rendering") + ": " + e.what());
            delete newProcess;
        } catch (...) {
            Natron::errorDialog(writer->getName(), std::string("Error while starting rendering"));
            delete newProcess;
        }
    } else {
        ActiveBackgroundRender* backgroundRender;
        {
            QMutexLocker l(&_activeRenderersMutex);
            backgroundRender = new ActiveBackgroundRender(writer);
            _activeRenderers.push_back(backgroundRender);
        }
        backgroundRender->blockingRender(); //< doesn't return before rendering is finished
        
        //remove the renderer from the list
        {
            QMutexLocker l(&_activeRenderersMutex);
            _activeRenderers.remove(backgroundRender);
        }
    }
}


AppInstance::ActiveBackgroundRender::ActiveBackgroundRender(Natron::OutputEffectInstance* writer)
    : _running(false)
    ,_writer(writer)
{
    
}

void AppInstance::ActiveBackgroundRender::blockingRender(){
    _writer->renderFullSequence();
    if (!appPTR->getCurrentSettings()->isMultiThreadingDisabled()) {
        QMutexLocker locker(&_runningMutex);
        _running = true;
        while (_running) {
            _runningCond.wait(&_runningMutex);
        }
    }
}

void AppInstance::ActiveBackgroundRender::notifyFinished(){
    appPTR->writeToOutputPipe(kRenderingFinishedStringLong,kRenderingFinishedStringShort);
    QMutexLocker locker(&_runningMutex);
    _running = false;
    _runningCond.wakeOne();
}

void AppInstance::notifyRenderFinished(Natron::OutputEffectInstance* writer){
    for (std::list<ActiveBackgroundRender*>::iterator it = _activeRenderers.begin(); it != _activeRenderers.end(); ++it) {
        if ((*it)->getWriter() == writer) {
            (*it)->notifyFinished();
        }
    }
}

void AppManager::removeFromNodeCache(boost::shared_ptr<Natron::Image> image){
    _nodeCache->removeEntry(image);
    if(image) {
        emit imageRemovedFromNodeCache(image->getKey()._time);
    }
}

void AppManager::removeFromViewerCache(boost::shared_ptr<Natron::FrameEntry> texture){
    _viewerCache->removeEntry(texture);
    if(texture) {
        emit imageRemovedFromNodeCache(texture->getKey()._time);
    }
}

const QString& AppManager::getApplicationBinaryPath() const {
    return _binaryPath;
}

void AppManager::setMultiThreadEnabled(bool enabled) {
    _settings->setMultiThreadingDisabled(!enabled);
}

void AppInstance::clearOpenFXPluginsCaches(){
    const std::vector<Node*>& activeNodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < activeNodes.size(); ++i) {
        activeNodes[i]->purgeAllInstancesCaches();
    }
}

bool AppManager::getImage(const Natron::ImageKey& key,boost::shared_ptr<Natron::Image>* returnValue) const {
#ifdef NATRON_LOG
    Log::beginFunction("AppManager","getImage");
    bool ret = _nodeCache->get(key, returnValue);
    if(ret) {
        Log::print("Image found in cache!");
    } else {
        Log::print("Image not found in cache!");
    }
    Log::endFunction("AppManager","getImage");
#else
    return _nodeCache->get(key, returnValue);
#endif

}

bool AppManager::getTexture(const Natron::FrameKey& key,boost::shared_ptr<Natron::FrameEntry>* returnValue) const {
#ifdef NATRON_LOG
    Log::beginFunction("AppManager","getTexture");
    bool ret = _viewerCache->get(key, returnValue);
    if(ret) {
        Log::print("Texture found in cache!");
    } else {
        Log::print("Texture not found in cache!");
    }
    Log::endFunction("AppManager","getTexture");
#else
    return _viewerCache->get(key, returnValue);
#endif
}

void AppManager::setLoadingStatus(const QString& str) {
    if (isLoaded()) {
        return;
    }
    if (!_splashScreen) {
        std::cout << str.toStdString() << std::endl;
    } else {
        _splashScreen->updateText(str);
    }
}

void AppManager::updateAllRecentFileMenus() {
    for (std::map<int,AppInstance*>::iterator it = _appInstances.begin(); it!= _appInstances.end(); ++it) {
        assert(!it->second->isBackground());
        it->second->getGui()->updateRecentFileActions();
    }
}

namespace Natron{

void errorDialog(const std::string& title,const std::string& message){
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->errorDialog(title,message);
    }else{
        std::cout << "ERROR: " << message << std::endl;
    }

#ifdef NATRON_LOG
    Log::beginFunction(title,"ERROR");
    Log::print(message);
    Log::endFunction(title,"ERROR");
#endif
}

void warningDialog(const std::string& title,const std::string& message){
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->warningDialog(title,message);
    }else{
        std::cout << "WARNING: "<< message << std::endl;
    }
#ifdef NATRON_LOG
    Log::beginFunction(title,"WARNING");
    Log::print(message);
    Log::endFunction(title,"WARNING");
#endif
}

void informationDialog(const std::string& title,const std::string& message){
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        topLvlInstance->informationDialog(title,message);
    }else{
        std::cout << "INFO: "<< message << std::endl;
    }
#ifdef NATRON_LOG
    Log::beginFunction(title,"INFO");
    Log::print(message);
    Log::endFunction(title,"INFO");
#endif
}

Natron::StandardButton questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons,
                                      Natron::StandardButton defaultButton){
    appPTR->hideSplashScreen();
    AppInstance* topLvlInstance = appPTR->getTopLevelInstance();
    assert(topLvlInstance);
    if(!topLvlInstance->isBackground()){
        return topLvlInstance->questionDialog(title,message,buttons,defaultButton);
    }else{
        std::cout << "QUESTION ASKED: " << message << std::endl;
        std::cout << NATRON_APPLICATION_NAME " answered yes." << std::endl;
        return Natron::Yes;
    }
#ifdef NATRON_LOG
    Log::beginFunction(title,"QUESTION");
    Log::print(message);
    Log::endFunction(title,"QUESTION");
#endif
}
    

Plugin::~Plugin(){
    if(_lock){
        delete _lock;
    }
    if(_binary){
        delete _binary;
    }
}

} //Namespace Natron
