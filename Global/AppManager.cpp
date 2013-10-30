//  Powiter
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
#include <QMessageBox>
#include <boost/scoped_ptr.hpp>
#include <QLabel>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtConcurrentRun>
#if QT_VERSION < 0x050000
#include <QtGui/QDesktopServices>
#else
#include <QStandardPaths>
#endif


#include <ImfThreading.h>


#include "Global/LibraryBinary.h"
#include "Global/MemoryInfo.h"
#include "Engine/Project.h"

#include "Gui/ViewerGL.h"
#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGraph.h"
#include "Gui/SequenceFileDialog.h"

#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/ViewerNode.h"
#include "Engine/Knob.h"
#include "Engine/OfxHost.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxNode.h"
#include "Engine/TimeLine.h"

#include "Readers/Reader.h"
#include "Readers/Decoder.h"
#include "Readers/ExrDecoder.h"
#include "Readers/QtDecoder.h"

#include "Writers/Writer.h"
#include "Writers/Encoder.h"
#include "Writers/QtEncoder.h"
#include "Writers/ExrEncoder.h"

using namespace Powiter;
using std::cout; using std::endl;
using std::make_pair;

AppInstance::AppInstance(int appID,const QString& projectName):
_gui(new Gui(this))
, _currentProject(new Powiter::Project(this))
, _appID(appID)
, _nodeMapping()
, _autoSaveMutex(new QMutex)
{
    _gui->createGui();
    
    const std::vector<AppManager::PluginToolButton*>& _toolButtons = appPTR->getPluginsToolButtons();
    for (U32 i = 0; i < _toolButtons.size(); ++i) {
        assert(_toolButtons[i]);
        QString name = _toolButtons[i]->_pluginName;
        if (_toolButtons[i]->_groups.size() >= 1 &&
            name != "Reader" &&
            name != "Viewer" &&
            name != "Writer") {
            name.append("  [");
            name.append(_toolButtons[i]->_groups[0]);
            name.append("]");
        }
        _gui->addPluginToolButton(name,
                                  _toolButtons[i]->_groups,
                                  _toolButtons[i]->_pluginName,
                                  _toolButtons[i]->_pluginIconPath,
                                  _toolButtons[i]->_groupIconPath);
    }
    emit pluginsPopulated();
    _gui->show();
    
    /* Create auto-save dir if it does not exists*/
    QDir dir = autoSavesDir();
    dir.mkpath(".");
    
    /*If this is the first instance of the software*/
    if(_appID == 0){
        if(!findAutoSave()){
            if(projectName.isEmpty()){
                QString text(QCoreApplication::applicationName() + " - ");
                text.append(_currentProject->getProjectName());
                _gui->setWindowTitle(text);
                createNode("Viewer");
            }else{
                QString name = SequenceFileDialog::removePath(projectName);
                QString path = projectName.left(projectName.indexOf(name));
                try {
                    loadProject(path,name);
                } catch (const std::exception& e) {
                    Powiter::errorDialog("Project loader",e.what());
                    cout << e.what() << endl;
                    createNode("Viewer");
                }
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
            try {
                loadProject(path,name);
            } catch (const std::exception& e) {
                Powiter::errorDialog("Project loader",e.what());
                cout << e.what() << endl;
                createNode("Viewer");
            }

        }
    }
    
}


AppInstance::~AppInstance(){
    removeAutoSaves();
    appPTR->removeInstance(_appID);
    delete _autoSaveMutex;
    
}


Node* AppInstance::createNode(const QString& name) {
    Node* node = 0;
    if(name == "Reader"){
		node = new Reader(this);
	}else if(name =="Viewer"){
        node = new ViewerNode(this);
	}else if(name == "Writer"){
		node = new Writer(this);
        QObject::connect(node,SIGNAL(renderingOnDiskStarted(Writer*,QString,int,int)),this,
                         SLOT(onRenderingOnDiskStarted(Writer*, QString, int, int)));
    } else {
        node = appPTR->getOfxHost()->createOfxNode(name.toStdString(),this);
    }
    if(!node){
        cout << "(Controler::createNode): Couldn't create Node " << name.toStdString() << endl;
        return NULL;
    }
    QObject::connect(node,SIGNAL(deactivated()),this,SLOT(checkViewersConnection()));
    QObject::connect(node, SIGNAL(deactivated()), this, SLOT(triggerAutoSave()));
    QObject::connect(node,SIGNAL(activated()),this,SLOT(checkViewersConnection()));
    QObject::connect(node, SIGNAL(activated()), this, SLOT(triggerAutoSave()));
    QObject::connect(node, SIGNAL(knobUndoneChange()), this, SLOT(triggerAutoSave()));
    QObject::connect(node, SIGNAL(knobRedoneChange()), this, SLOT(triggerAutoSave()));
    
    NodeGui* nodegui = 0;
    nodegui = _gui->createNodeGUI(node);
    _nodeMapping.insert(make_pair(node,nodegui));
    node->initializeInputs();
    Node* selected  =  0;
    if(_gui->getSelectedNode()){
        selected = _gui->getSelectedNode()->getNode();
    }
    _currentProject->initNodeCountersAndSetName(node);
    if(node->className() == "Viewer"){
        _gui->createViewerGui(node);
    }
    node->initializeKnobs();
    if(node->isOpenFXNode()){
        OfxNode* ofxNode = dynamic_cast<OfxNode*>(node);
        ofxNode->openFilesForAllFileParams();
        bool ok = ofxNode->effectInstance()->getClipPreferences();
        if(!ok){
            node->deleteNode();
            return NULL;
        }
        if(node->canMakePreviewImage())
            node->refreshPreviewImage(0);
    }
    autoConnect(selected, node);
    
    return node;
}




void AppInstance::autoConnect(Node* target,Node* created){
    _gui->autoConnect(getNodeGui(target),getNodeGui(created));
}


const std::vector<NodeGui*>& AppInstance::getAllActiveNodes() const{
    assert(_gui->_nodeGraphTab->_nodeGraphArea);
    return  _gui->_nodeGraphTab->_nodeGraphArea->getAllActiveNodes();
    
}
void AppInstance::loadProject(const QString& path,const QString& name){
    try{
        _currentProject->loadProject(path,name);
    }catch(const std::exception& e){
        Powiter::errorDialog("Project loader",e.what());
        cout << e.what() << endl;
        createNode("Viewer");
        return;
    }
    QString text(QCoreApplication::applicationName() + " - ");
    text.append(name);
    _gui->setWindowTitle(text);
}
void AppInstance::saveProject(const QString& path,const QString& name,bool autoSave){
    QDateTime time = QDateTime::currentDateTime();
    if(!autoSave) {
        
        if((_currentProject->projectAgeSinceLastSave() != _currentProject->projectAgeSinceLastAutosave()) ||
           !QFile::exists(path+name)){
            
            _currentProject->saveProject(path,name);
            _currentProject->setHasProjectBeenSavedByUser(true);
            _currentProject->setProjectName(name);
            _currentProject->setProjectPath(path);
            _currentProject->setProjectAgeSinceLastSave(time);
            _currentProject->setProjectAgeSinceLastAutosaveSave(time);
            QString text(QCoreApplication::applicationName() + " - ");
            text.append(name);
            _gui->setWindowTitle(text);
        }
    }else{
        if(!_gui->isGraphWorthless()){
            
            removeAutoSaves();
            _currentProject->saveProject(path,name+"."+time.toString("dd.MM.yyyy.hh:mm:ss:zzz"),true);
            _currentProject->setProjectName(name);
            _currentProject->setProjectPath(path);
            _currentProject->setProjectAgeSinceLastAutosaveSave(time);
        }
    }
}

void AppInstance::autoSave(){
    saveProject(_currentProject->getProjectPath(), _currentProject->getProjectName(), true);
}
void AppInstance::triggerAutoSave(){
    QtConcurrent::run(this,&AppInstance::autoSave);
    QString text(QCoreApplication::applicationName() + " - ");
    text.append(_currentProject->getProjectName());
    text.append(" (*)");
    _gui->setWindowTitle(text);
}

void AppInstance::removeAutoSaves() const{
    /*removing all previous autosave files*/
    QDir savesDir(autoSavesDir());
    QStringList entries = savesDir.entryList();
    for(int i = 0; i < entries.size();++i) {
        const QString& entry = entries.at(i);
        QString searchStr('.');
        searchStr.append(POWITER_PROJECT_FILE_EXTENION);
        searchStr.append('.');
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos != -1) {
            QFile::remove(savesDir.path()+QDir::separator()+entry);
        }
    }
}


bool AppInstance::isSaveUpToDate() const{
    return _currentProject->projectAgeSinceLastSave() == _currentProject->projectAgeSinceLastAutosave();
}
void AppInstance::resetCurrentProject(){
    _currentProject->setAutoSetProjectFormat(true);
    _currentProject->setHasProjectBeenSavedByUser(false);
    _currentProject->setProjectName("Untitled." POWITER_PROJECT_FILE_EXTENION);
    _currentProject->setProjectPath("");
    QString text(QCoreApplication::applicationName() + " - ");
    text.append(_currentProject->getProjectName());
    _gui->setWindowTitle(text);
}

bool AppInstance::findAutoSave() {
    QDir savesDir(autoSavesDir());
    QStringList entries = savesDir.entryList();
    for (int i = 0; i < entries.size();++i) {
        const QString& entry = entries.at(i);
        QString searchStr('.');
        searchStr.append(POWITER_PROJECT_FILE_EXTENION);
        searchStr.append('.');
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos != -1) {
            QFile autoSaveFile(savesDir.path()+QDir::separator()+entry);
            autoSaveFile.open(QIODevice::ReadOnly);
            QTextStream inStream(&autoSaveFile);
            QString firstLine = inStream.readLine();
            QString path;
            if (!firstLine.isEmpty()) {
                int j = 0;
                while (j < firstLine.size() &&  (firstLine.at(j) != QChar('\n'))) {
                    path.append(firstLine.at(j));
                    ++j;
                }
            }
            autoSaveFile.close();
            QString filename = entry.left(suffixPos+3);
            QString filenameWithPath = path + QDir::separator() + filename;
            bool exists = QFile::exists(filenameWithPath);
            QString text;
            
            if (exists) {
                text = QString(tr("A recent auto-save of %1 was found.\n"
                                  "Would you like to restore it entirely? "
                                  "Clicking No will remove this auto-save.")).arg(filename);;
            } else {
                text = tr("An auto-save was restored successfully. It didn't belong to any project\n"
                          "Would you like to restore it ? Clicking No will remove this auto-save forever.");
            }
            
            
            QMessageBox::StandardButton ret = QMessageBox::question(_gui, tr("Auto-save"), text,
                                                                    QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes);
            if (ret == QMessageBox::No || ret == QMessageBox::Escape) {
                removeAutoSaves();
                clearNodes();
                resetCurrentProject();
                return false;
            } else {
                try{
                    _currentProject->loadProject(savesDir.path()+QDir::separator(), entry);
                }catch(const std::exception& e){
                    Powiter::errorDialog("Project loader",e.what());
                    cout << e.what() << endl;
                    createNode("Viewer");
                }
                if (exists) {
                    _currentProject->setHasProjectBeenSavedByUser(true);
                    _currentProject->setProjectName(filename);
                    _currentProject->setProjectPath(path);
                } else {
                    _currentProject->setHasProjectBeenSavedByUser(false);
                    _currentProject->setProjectName("Untitled." POWITER_PROJECT_FILE_EXTENION);
                    _currentProject->setProjectPath("");
                }
           
                QString title(QCoreApplication::applicationName() + " - ");
                title.append(_currentProject->getProjectName());
                title.append(" (*)");
                _gui->setWindowTitle(title);
                removeAutoSaves(); // clean previous auto-saves
                autoSave(); // auto-save the recently recovered project, in case the program crashes again
                return true;
            }
        }
    }
    removeAutoSaves();
    return false;
}

QString AppInstance::autoSavesDir() {
#if QT_VERSION < 0x050000
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QDir::separator() + "Autosaves";
#else
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator() + "Autosaves";
#endif
    //return QString(QDir::tempPath() + QDir::separator() + QCoreApplication::applicationName() + QDir::separator());
}

void AppInstance::deselectAllNodes() const{
    _gui->_nodeGraphTab->_nodeGraphArea->deselect();
}

void AppInstance::errorDialog(const std::string& title,const std::string& message) const{
    _gui->errorDialog(title.c_str(), message.c_str());
    
}

void AppInstance::warningDialog(const std::string& title,const std::string& message) const{
    _gui->warningDialog(title.c_str(), message.c_str());
    
}

void AppInstance::informationDialog(const std::string& title,const std::string& message) const{
    _gui->informationDialog(title.c_str(), message.c_str());
    
}

Powiter::StandardButton AppInstance::questionDialog(const std::string& title,const std::string& message,Powiter::StandardButtons buttons,
                                                    Powiter::StandardButton defaultButton) const{
    return _gui->questionDialog(title.c_str(), message.c_str(),buttons,defaultButton);
}


ViewerTab* AppInstance::addNewViewerTab(ViewerNode* node,TabWidget* where){
    return  _gui->addNewViewerTab(node, where);
}

AppInstance* AppManager::newAppInstance(const QString& projectName){
    AppInstance* instance = new AppInstance(_availableID,projectName);
    _appInstances.insert(make_pair(_availableID, instance));
    setAsTopLevelInstance(_availableID);
    ++_availableID;
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
void AppManager::removeInstance(int appID){
    _appInstances.erase(appID);
}

bool AppInstance::connect(int inputNumber,const std::string& parentName,Node* output){
    const std::vector<Node*>& nodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->getName() == parentName) {
            return connect(inputNumber,nodes[i], output);
        }
    }
    return false;
}

bool AppInstance::connect(int inputNumber,Node* input,Node* output){
    if(!output->connectInput(input, inputNumber)){
        return false;
    }
    if(!input){
        return true;
    }
    input->connectOutput(output);
    return true;
}
bool AppInstance::disconnect(Node* input,Node* output){
    if(input->disconnectOutput(output) < 0){
        return false;
    }
    if(output->disconnectInput(input) < 0){
        return false;
    }
    return true;
}


NodeGui* AppInstance::getNodeGui(Node* n) const {
    std::map<Node*,NodeGui*>::const_iterator it = _nodeMapping.find(n);
    if(it==_nodeMapping.end()){
        return NULL;
    }else{
        return it->second;
    }
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
        if(n->className() == "Viewer"){
            dynamic_cast<ViewerNode*>(n)->connectSlotsToViewerCache();
        }
    }
}

void AppInstance::disconnectViewersFromViewerCache(){
    foreach(Node* n,_currentProject->getCurrentNodes()){
        assert(n);
        if(n->className() == "Viewer"){
            dynamic_cast<ViewerNode*>(n)->disconnectSlotsToViewerCache();
        }
    }
}


void AppInstance::clearNodes(){
    _currentProject->clearNodes();
}



void AppManager::clearPlaybackCache(){
    _viewerCache->clearInMemoryPortion();
}


void AppManager::clearDiskCache(){
    _viewerCache->clear();
}


void  AppManager::clearNodeCache(){
    _nodeCache->clear();
}


std::vector<LibraryBinary*> AppManager::loadPlugins(const QString &where){
    std::vector<LibraryBinary*> ret;
    QDir d(where);
    if (d.isReadable())
    {
        QStringList filters;
        filters << QString(QString("*.")+QString(POWITER_LIBRARY_EXT));
        d.setNameFilters(filters);
		QStringList fileList = d.entryList();
        for(int i = 0 ; i < fileList.size() ; ++i) {
            QString filename = fileList.at(i);
            if(filename.endsWith(".dll") || filename.endsWith(".dylib") || filename.endsWith(".so")){
                QString className;
                int index = filename.lastIndexOf("." POWITER_LIBRARY_EXT);
                className = filename.left(index);
                std::string binaryPath = POWITER_PLUGINS_PATH + className.toStdString() + "." + POWITER_LIBRARY_EXT;
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

std::vector<Powiter::LibraryBinary*> AppManager::loadPluginsAndFindFunctions(const QString& where,
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
, _readPluginsLoaded()
, _writePluginsLoaded()
, _formats()
, _plugins()
, ofxHost(new Powiter::OfxHost())
, _toolButtons()
, _knobFactory(new KnobFactory())
,_nodeCache(new Cache<Image>("NodeCache",0x1, (Settings::getPowiterCurrentSettings()->_cacheSettings.maxCacheMemoryPercent -
                                             Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent)*getSystemTotalRAM(),1))
,_viewerCache(new Cache<FrameEntry>("ViewerCache",0x1,Settings::getPowiterCurrentSettings()->_cacheSettings.maxDiskCache
                                    ,Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent))
{
    connect(ofxHost.get(), SIGNAL(toolButtonAdded(QStringList,QString,QString,QString)),
            this, SLOT(addPluginToolButtons(QStringList,QString,QString,QString)));
    
    
    /*loading all plugins*/
    loadAllPlugins();
    loadBuiltinFormats();
    
    /*Adjusting multi-threading for OpenEXR library.*/
    Imf::setGlobalThreadCount(QThread::idealThreadCount());
    
    
    
}

AppManager::~AppManager(){
    for(ReadPluginsIterator it = _readPluginsLoaded.begin(); it!=_readPluginsLoaded.end(); ++it) {
        delete it->second.second;
    }
    for(WritePluginsIterator it = _writePluginsLoaded.begin(); it!=_writePluginsLoaded.end(); ++it) {
        delete it->second.second;
    }
    for(std::map<QString,QMutex*>::iterator it = _plugins.begin();it!=_plugins.end();++it){
        if(it->second)
            delete it->second;
    }
    foreach(Format* f,_formats){
        delete f;
    }
    foreach(PluginToolButton* p,_toolButtons){
        delete p;
    }
}
void AppManager::quit(){
    delete appPTR;
}

void AppManager::loadAllPlugins() {
    assert(_plugins.empty());
    assert(_formats.empty());
    assert(_toolButtons.empty());
    
    /*loading node plugins*/
    loadBuiltinNodePlugins();
    
    assert(_readPluginsLoaded.empty());
    /*loading read plugins*/
    loadReadPlugins();
    
    assert(_writePluginsLoaded.empty());
    /*loading write plugins*/
    loadWritePlugins();
    
    /*loading ofx plugins*/
    std::map<QString,QMutex*> ofxPluginNames = ofxHost->loadOFXPlugins();
    for(std::map<QString,QMutex*>::iterator it = ofxPluginNames.begin();it!=ofxPluginNames.end();++it){
        _plugins.insert(*it);
    }
    
}

void AppManager::loadReadPlugins(){
    std::vector<std::string> functions;
    functions.push_back("BuildRead");
    std::vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(POWITER_READERS_PLUGINS_PATH, functions);
    for (U32 i = 0 ; i < plugins.size(); ++i) {
        std::pair<bool,ReadBuilder> func = plugins[i]->findFunction<ReadBuilder>("BuildRead");
        if(func.first){
            Decoder* read = func.second(NULL);
            assert(read);
            std::vector<std::string> extensions = read->fileTypesDecoded();
            std::string decoderName = read->decoderName();
            _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,plugins[i])));
            delete read;
        }
    }
    
    loadBuiltinReads();
    
    std::map<std::string, LibraryBinary*> defaultMapping;
    for (ReadPluginsIterator it = _readPluginsLoaded.begin(); it!=_readPluginsLoaded.end(); ++it) {
        if(it->first == "OpenEXR"){
            defaultMapping.insert(make_pair("exr", it->second.second));
        }else if(it->first == "QImage (Qt)"){
            const std::vector<std::string>& decodedFormats = it->second.first;
            for (U32 i = 0; i < decodedFormats.size(); ++i) {
                defaultMapping.insert(make_pair(decodedFormats[i], it->second.second));
            }
        }
    }
    Settings::getPowiterCurrentSettings()->_readersSettings.fillMap(defaultMapping);
}

void AppManager::loadBuiltinReads(){
    {
        Decoder* readExr = ExrDecoder::BuildRead(NULL);
        assert(readExr);
        std::vector<std::string> extensions = readExr->fileTypesDecoded();
        std::string decoderName = readExr->decoderName();
        
        std::map<std::string,void*> EXRfunctions;
        EXRfunctions.insert(make_pair("BuildRead", (void*)&ExrDecoder::BuildRead));
        LibraryBinary *EXRplugin = new LibraryBinary(EXRfunctions);
        assert(EXRplugin);
        for (U32 i = 0 ; i < extensions.size(); ++i) {
            _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,EXRplugin)));
        }
        delete readExr;
    }
    {
        Decoder* readQt = QtDecoder::BuildRead(NULL);
        assert(readQt);
        std::vector<std::string> extensions = readQt->fileTypesDecoded();
        std::string decoderName = readQt->decoderName();
        
        std::map<std::string,void*> Qtfunctions;
        Qtfunctions.insert(make_pair("BuildRead", (void*)&QtDecoder::BuildRead));
        LibraryBinary *Qtplugin = new LibraryBinary(Qtfunctions);
        assert(Qtplugin);
        for (U32 i = 0 ; i < extensions.size(); ++i) {
            _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,Qtplugin)));
        }
        delete readQt;
    }
#if 0 // deprecated
    {
        Read* readFfmpeg = ReadFFMPEG::BuildRead(NULL);
        assert(readFfmpeg);
        std::vector<std::string> extensions = readFfmpeg->fileTypesDecoded();
        std::string decoderName = readFfmpeg->decoderName();
        
        std::map<std::string,void*> FFfunctions;
        FFfunctions.insert(make_pair("ReadBuilder", (void*)&ReadFFMPEG::BuildRead));
        LibraryBinary *FFMPEGplugin = new LibraryBinary(FFfunctions);
        assert(FFMPEGplugin);
        for (U32 i = 0 ; i < extensions.size(); ++i) {
            _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,FFMPEGplugin)));
        }
        delete readFfmpeg;
    }
#endif // deprecated
}
void AppManager::loadBuiltinNodePlugins(){
    // these  are built-in nodes
    _plugins.insert(std::make_pair("Reader",(QMutex*)NULL));
    _plugins.insert(std::make_pair("Viewer",(QMutex*)NULL));
    _plugins.insert(std::make_pair("Writer",(QMutex*)NULL));
    QStringList grouping;
    grouping.push_back("IO");
    addPluginToolButtons(grouping, "Reader", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
    addPluginToolButtons(grouping, "Viewer", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
    addPluginToolButtons(grouping, "Writer", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
}

/*loads extra writer plug-ins*/
void AppManager::loadWritePlugins(){
    
    std::vector<std::string> functions;
    functions.push_back("BuildWrite");
    std::vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(POWITER_WRITERS_PLUGINS_PATH, functions);
    for (U32 i = 0 ; i < plugins.size(); ++i) {
        std::pair<bool,WriteBuilder> func = plugins[i]->findFunction<WriteBuilder>("BuildWrite");
        if(func.first){
            Encoder* write = func.second(NULL);
            assert(write);
            std::vector<std::string> extensions = write->fileTypesEncoded();
            std::string encoderName = write->encoderName();
            _writePluginsLoaded.insert(make_pair(encoderName,make_pair(extensions,plugins[i])));
            delete write;
        }
    }
    loadBuiltinWrites();
    std::map<std::string, LibraryBinary*> defaultMapping;
    for (WritePluginsIterator it = _writePluginsLoaded.begin(); it!=_writePluginsLoaded.end(); ++it) {
        if(it->first == "OpenEXR"){
            defaultMapping.insert(make_pair("exr", it->second.second));
        }else if(it->first == "QImage (Qt)"){
            const std::vector<std::string>& encodedFormats = it->second.first;
            for (U32 i = 0; i < encodedFormats.size(); ++i) {
                defaultMapping.insert(make_pair(encodedFormats[i], it->second.second));
            }
        }
    }
    Settings::getPowiterCurrentSettings()->_writersSettings.fillMap(defaultMapping);
}

/*loads writes that are built-ins*/
void AppManager::loadBuiltinWrites(){
    {
        boost::scoped_ptr<Encoder> writeQt(new QtEncoder(NULL));
        assert(writeQt);
        std::vector<std::string> extensions = writeQt->fileTypesEncoded();
        std::string encoderName = writeQt->encoderName();
        
        std::map<std::string,void*> Qtfunctions;
        Qtfunctions.insert(make_pair("BuildWrite",(void*)&QtEncoder::BuildWrite));
        LibraryBinary *QtWritePlugin = new LibraryBinary(Qtfunctions);
        assert(QtWritePlugin);
        for (U32 i = 0 ; i < extensions.size(); ++i) {
            _writePluginsLoaded.insert(make_pair(encoderName,make_pair(extensions,QtWritePlugin)));
        }
    }
    
    {
        boost::scoped_ptr<Encoder> writeEXR(new ExrEncoder(NULL));
        std::vector<std::string> extensionsExr = writeEXR->fileTypesEncoded();
        std::string encoderNameExr = writeEXR->encoderName();
        
        std::map<std::string,void*> EXRfunctions;
        EXRfunctions.insert(make_pair("BuildWrite",(void*)&ExrEncoder::BuildWrite));
        LibraryBinary *ExrWritePlugin = new LibraryBinary(EXRfunctions);
        assert(ExrWritePlugin);
        for (U32 i = 0 ; i < extensionsExr.size(); ++i) {
            _writePluginsLoaded.insert(make_pair(encoderNameExr,make_pair(extensionsExr,ExrWritePlugin)));
        }
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

void AppManager::addPluginToolButtons(const QStringList& groups,
                                      const QString& pluginName,
                                      const QString& pluginIconPath,
                                      const QString& groupIconPath){
    _toolButtons.push_back(new PluginToolButton(groups,pluginName,pluginIconPath,groupIconPath));
}

void AppManager::setAsTopLevelInstance(int appID){
    if(_topLevelInstanceID == appID){
        return;
    }
    _topLevelInstanceID = appID;
    for(std::map<int,AppInstance*>::iterator it = _appInstances.begin();it!=_appInstances.end();++it){
        if (it->first != _topLevelInstanceID) {
            it->second->disconnectViewersFromViewerCache();
        }else{
            it->second->connectViewersToViewerCache();
        }
    }
}
void AppInstance::onRenderingOnDiskStarted(Writer* writer,const QString& sequenceName,int firstFrame,int lastFrame){
    if(_gui){
        _gui->showProgressDialog(writer, sequenceName,firstFrame,lastFrame);
    }
}

void AppInstance::tryAddProjectFormat(const Format& frmt){
    _currentProject->tryAddProjectFormat(frmt);
}
void AppInstance::setProjectFormat(const Format& frmt){
    Format* df = appPTR->findExistingFormat(frmt.width(), frmt.height(),frmt.getPixelAspect());
    if(df){
        Format currentDW = frmt;
        currentDW.setName(df->getName());
        _currentProject->setProjectDefaultFormat(currentDW);
        
        
    }else{
        _currentProject->setProjectDefaultFormat(frmt);
        
    }
}

void AppInstance::checkViewersConnection(){
    const std::vector<Node*>& nodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->className() == "Viewer") {
            ViewerNode* n = dynamic_cast<ViewerNode*>(nodes[i]);
            assert(n);
            n->updateTreeAndRender();
        }
    }
}
void AppInstance::setupViewersForViews(int viewsCount){
    const std::vector<Node*>& nodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->className() == "Viewer") {
            ViewerNode* n = dynamic_cast<ViewerNode*>(nodes[i]);
            assert(n);
            n->getUiContext()->updateViewsMenu(viewsCount);
        }
    }
}
void AppInstance::setViewersCurrentView(int view){
    const std::vector<Node*>& nodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->className() == "Viewer") {
            ViewerNode* n = dynamic_cast<ViewerNode*>(nodes[i]);
            assert(n);
            n->getUiContext()->setCurrentView(view);
        }
    }
}
const QString& AppInstance::getCurrentProjectName() const  {return _currentProject->getProjectName();}

const QString& AppInstance::getCurrentProjectPath() const  {return _currentProject->getProjectPath();}

boost::shared_ptr<TimeLine> AppInstance::getTimeLine() const  {return _currentProject->getTimeLine();}

void AppInstance::setCurrentProjectName(const QString& name) {_currentProject->setProjectName(name);}

int AppInstance::getCurrentProjectViewsCount() const{ return _currentProject->getProjectViewsCount();}

bool AppInstance::shouldAutoSetProjectFormat() const {return _currentProject->shouldAutoSetProjectFormat();}

void AppInstance::setAutoSetProjectFormat(bool b){_currentProject->setAutoSetProjectFormat(b);}

bool AppInstance::hasProjectBeenSavedByUser() const  {return _currentProject->hasProjectBeenSavedByUser();}

const Format& AppInstance::getProjectFormat() const  {return _currentProject->getProjectDefaultFormat();}

void AppManager::clearExceedingEntriesFromNodeCache(){
    _nodeCache->clearExceedingEntries();
}

QStringList AppManager::getNodeNameList() const{
    QStringList ret;
    for(std::map<QString,QMutex*>::const_iterator it = _plugins.begin();it!=_plugins.end();++it){
        ret.append(it->first);
    }
    return ret;
}

QMutex* AppManager::getMutexForPlugin(const QString& pluginName) const {
    std::map<QString,QMutex*>::const_iterator it = _plugins.find(pluginName);
    if(it != _plugins.end()){
        return it->second;
    }
    return NULL;
}
