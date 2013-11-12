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

#include <QMessageBox>
#include <QLabel>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
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
#include "Engine/ViewerInstance.h"
#include "Engine/Knob.h"
#include "Engine/OfxHost.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Log.h"

#include "Readers/Reader.h"
#include "Readers/Decoder.h"
#include "Readers/ExrDecoder.h"
#include "Readers/QtDecoder.h"

#include "Writers/Writer.h"
#include "Writers/Encoder.h"
#include "Writers/QtEncoder.h"
#include "Writers/ExrEncoder.h"

using namespace Natron;
using std::cout; using std::endl;
using std::make_pair;





AppInstance::AppInstance(bool backgroundMode,int appID,const QString& projectName,const QStringList& writers):
_gui(NULL)
, _currentProject(new Natron::Project(this))
, _appID(appID)
, _nodeMapping()
, _autoSaveMutex(new QMutex)
, _isBackground(backgroundMode)
{
    appPTR->registerAppInstance(this);
    appPTR->setAsTopLevelInstance(appID);

    if(!_isBackground){
        _gui = new Gui(this);
    }
    if(_isBackground && projectName.isEmpty()){
        // cannot start a background process without a file
        throw std::invalid_argument("Project file name empty");
    }
    _currentProject->initializeKnobs();
    
    if(!_isBackground){
        _gui->createGui();
        const std::vector<PluginToolButton*>& _toolButtons = appPTR->getPluginsToolButtons();
        for (U32 i = 0; i < _toolButtons.size(); ++i) {
            assert(_toolButtons[i]);
            _gui->findOrCreateToolButton(_toolButtons[i]);
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
                    loadProject(path,name,false);
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
                loadProject(path,name,false);
            }
        }

    }else{
        QString name = SequenceFileDialog::removePath(projectName);
        QString path = projectName.left(projectName.indexOf(name));
        if(!loadProject(path,name,true)){
            throw std::invalid_argument("Project file loading failed.");
        }
        try{
            startWritersRendering(writers);
        }catch(const std::exception& e){
            throw e;
        }
    }
    
    
}


AppInstance::~AppInstance(){
    removeAutoSaves();
    appPTR->removeInstance(_appID);
    delete _autoSaveMutex;
    
}


Node* AppInstance::createNode(const QString& name,bool requestedByLoad ) {
    Node* node = 0;
    LibraryBinary* pluginBinary = appPTR->getPluginBinary(name);
    if(!pluginBinary){
        QString err("Reason:\n The plugin binary for ");
        err.append(name);
        err.append(" couldn't be located");
        Natron::errorDialog("Failed to create node",err.toStdString());
        return NULL;
    }
    try{
        if(name != "Viewer"){ // for now only the viewer can be an inspector.
            node = new Node(this,pluginBinary,name.toStdString());
        }else{
            node = new InspectorNode(this,pluginBinary,name.toStdString());
        }
    }catch(const std::exception& e){
        std::cout << e.what() << std::endl;
        delete node;
        return NULL;
    }
    

    QObject::connect(node,SIGNAL(deactivated()),this,SLOT(checkViewersConnection()));
    QObject::connect(node, SIGNAL(deactivated()), this, SLOT(triggerAutoSave()));
    QObject::connect(node,SIGNAL(activated()),this,SLOT(checkViewersConnection()));
    QObject::connect(node, SIGNAL(activated()), this, SLOT(triggerAutoSave()));
    
    NodeGui* nodegui = 0;
    if(!_isBackground){
        nodegui = _gui->createNodeGUI(node);
        _nodeMapping.insert(make_pair(node,nodegui));
    }
    node->initializeKnobs();
    node->initializeInputs();
    
    _currentProject->initNodeCountersAndSetName(node);
    if(node->className() == "Viewer" && !_isBackground){
        _gui->createViewerGui(node);
    }
    

    if(!_isBackground){
        if(_gui->getSelectedNode()){
            Node* selected = _gui->getSelectedNode()->getNode();
            autoConnect(selected, node);
        }
        _gui->selectNode(nodegui);
        if(!requestedByLoad)
            node->openFilesForAllFileKnobs();
    }
    
    return node;
}




void AppInstance::autoConnect(Node* target,Node* created){
    _gui->autoConnect(getNodeGui(target),getNodeGui(created));
}


const std::vector<NodeGui*>& AppInstance::getAllActiveNodes() const{
    assert(_gui->_nodeGraphTab->_nodeGraphArea);
    return  _gui->_nodeGraphTab->_nodeGraphArea->getAllActiveNodes();
    
}
bool AppInstance::loadProject(const QString& path,const QString& name,bool background){
    try{
        _currentProject->loadProject(path,name,background);
    }catch(const std::exception& e){
        Natron::errorDialog("Project loader",e.what());
        if(!background)
            createNode("Viewer");
        return false;
    }
    if(!background){
        QString text(QCoreApplication::applicationName() + " - ");
        text.append(name);
        _gui->setWindowTitle(text);
    }
    return true;
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
        searchStr.append(NATRON_PROJECT_FILE_EXTENION);
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
    _currentProject->setProjectName("Untitled." NATRON_PROJECT_FILE_EXTENION);
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
        searchStr.append(NATRON_PROJECT_FILE_EXTENION);
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
                    _currentProject->loadProject(savesDir.path()+QDir::separator(), entry,false);
                }catch(const std::exception& e){
                    Natron::errorDialog("Project loader",e.what());
                    cout << e.what() << endl;
                    createNode("Viewer");
                }
                if (exists) {
                    _currentProject->setHasProjectBeenSavedByUser(true);
                    _currentProject->setProjectName(filename);
                    _currentProject->setProjectPath(path);
                } else {
                    _currentProject->setHasProjectBeenSavedByUser(false);
                    _currentProject->setProjectName("Untitled." NATRON_PROJECT_FILE_EXTENION);
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


ViewerTab* AppInstance::addNewViewerTab(ViewerInstance* node,TabWidget* where){
    return  _gui->addNewViewerTab(node, where);
}

AppInstance* AppManager::newAppInstance(bool background,const QString& projectName,const QStringList& writers){
    AppInstance* instance = 0;
    try{
        instance = new AppInstance(background,_availableID,projectName,writers);
    }catch(const std::exception& e){
        Natron::errorDialog(NATRON_APPLICATION_NAME, e.what());
        printUsage();
        return NULL;
    }
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
            dynamic_cast<ViewerInstance*>(n->getLiveInstance())->connectSlotsToViewerCache();
        }
    }
}

void AppInstance::disconnectViewersFromViewerCache(){
    foreach(Node* n,_currentProject->getCurrentNodes()){
        assert(n);
        if(n->className() == "Viewer"){
            dynamic_cast<ViewerInstance*>(n->getLiveInstance())->disconnectSlotsToViewerCache();
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
                std::string binaryPath = NATRON_PLUGINS_PATH + className.toStdString() + "." + POWITER_LIBRARY_EXT;
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
, _settings(new Settings())
, _appInstances()
, _availableID(0)
, _topLevelInstanceID(0)
, _readPluginsLoaded()
, _writePluginsLoaded()
, _formats()
, _plugins()
, ofxHost(new Natron::OfxHost())
, _toolButtons()
, _knobFactory(new KnobFactory())
,_nodeCache(new Cache<Image>("NodeCache",0x1, (_settings->_cacheSettings.maxCacheMemoryPercent -
                                               _settings->_cacheSettings.maxPlayBackMemoryPercent)*getSystemTotalRAM(),1))
,_viewerCache(new Cache<FrameEntry>("ViewerCache",0x1,_settings->_cacheSettings.maxDiskCache
,_settings->_cacheSettings.maxPlayBackMemoryPercent))
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
    for(PluginsMap::iterator it = _plugins.begin();it!=_plugins.end();++it){
        if(it->second.second)
            delete it->second.second;
        if(it->second.first)
            delete it->second.first;
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
    loadNodePlugins();
    
    assert(_readPluginsLoaded.empty());
    /*loading read plugins*/
    loadReadPlugins();
    
    assert(_writePluginsLoaded.empty());
    /*loading write plugins*/
    loadWritePlugins();
    
    /*loading ofx plugins*/
    std::map<QString,QMutex*> ofxPluginNames = ofxHost->loadOFXPlugins();
    
    for(std::map<QString,QMutex*>::iterator it = ofxPluginNames.begin();it!=ofxPluginNames.end();++it){
        std::map<std::string,void*> functions;
        LibraryBinary *plugin = new Natron::LibraryBinary(Natron::LibraryBinary::BUILTIN);
        _plugins.insert(std::make_pair(it->first, std::make_pair(plugin, it->second)));
    }
}

void AppManager::loadReadPlugins(){
    std::vector<std::string> functions;
    functions.push_back("BuildRead");
    std::vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(NATRON_READERS_PLUGINS_PATH, functions);
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
    _settings->_readersSettings.fillMap(defaultMapping);
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

}
void AppManager::loadNodePlugins(){
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
                pluginMutex = new QMutex;
            }
            _plugins.insert(make_pair(effect->className().c_str(),make_pair(plugins[i],pluginMutex)));
            delete effect;
        }
    }
    
    loadBuiltinNodePlugins();
}

void AppManager::loadBuiltinNodePlugins(){
    // these  are built-in nodes
    QStringList grouping;
    grouping.push_back("IO");
    {
        EffectInstance* reader = Reader::BuildEffect(NULL);
        assert(reader);
        std::map<std::string,void*> readerFunctions;
        readerFunctions.insert(make_pair("BuildEffect", (void*)&Reader::BuildEffect));
        LibraryBinary *readerPlugin = new LibraryBinary(readerFunctions);
        assert(readerPlugin);
        _plugins.insert(std::make_pair(reader->className().c_str(),std::make_pair(readerPlugin,(QMutex*)NULL)));
        addPluginToolButtons(grouping,reader->className().c_str(), "", NATRON_IMAGES_PATH "ioGroupingIcon.png");
        delete reader;
    }
    {
        EffectInstance* viewer = ViewerInstance::BuildEffect(NULL);
        assert(viewer);
        std::map<std::string,void*> viewerFunctions;
        viewerFunctions.insert(make_pair("BuildEffect", (void*)&ViewerInstance::BuildEffect));
        LibraryBinary *viewerPlugin = new LibraryBinary(viewerFunctions);
        assert(viewerPlugin);
        _plugins.insert(std::make_pair(viewer->className().c_str(),std::make_pair(viewerPlugin,(QMutex*)NULL)));
        addPluginToolButtons(grouping,viewer->className().c_str(), "", NATRON_IMAGES_PATH "ioGroupingIcon.png");
        delete viewer;
    }
    {
        EffectInstance* writer = Writer::BuildEffect(NULL);
        assert(writer);
        std::map<std::string,void*> writerFunctions;
        writerFunctions.insert(make_pair("BuildEffect", (void*)&Writer::BuildEffect));
        LibraryBinary *writerPlugin = new LibraryBinary(writerFunctions);
        assert(writerPlugin);
        _plugins.insert(std::make_pair(writer->className().c_str(),std::make_pair(writerPlugin,(QMutex*)NULL)));
        addPluginToolButtons(grouping,writer->className().c_str(), "", NATRON_IMAGES_PATH "ioGroupingIcon.png");
        delete writer;
    }
}

/*loads extra writer plug-ins*/
void AppManager::loadWritePlugins(){
    
    std::vector<std::string> functions;
    functions.push_back("BuildWrite");
    std::vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(NATRON_WRITERS_PLUGINS_PATH, functions);
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
    _settings->_writersSettings.fillMap(defaultMapping);
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

void PluginToolButton::tryAddChild(PluginToolButton* plugin){
    for(unsigned int i = 0; i < _children.size() ; ++i){
        if(_children[i] == plugin)
            return;
    }
    _children.push_back(plugin);
}

void AppManager::addPluginToolButtons(const QStringList& groups,
                                      const QString& pluginName,
                                      const QString& pluginIconPath,
                                      const QString& groupIconPath){
    PluginToolButton* parent = NULL;
    for(int i = 0; i < groups.size();++i){
        
        PluginToolButton* child = findPluginToolButtonOrCreate(groups.at(i),groupIconPath);
        if(parent){
            parent->tryAddChild(child);
            child->setParent(parent);
        }
        parent = child;
        
    }
    PluginToolButton* lastChild = findPluginToolButtonOrCreate(pluginName,pluginIconPath);
    if(parent){
        parent->tryAddChild(lastChild);
        lastChild->setParent(parent);
    }

    //_toolButtons.push_back(new PluginToolButton(groups,pluginName,pluginIconPath,groupIconPath));
}
PluginToolButton* AppManager::findPluginToolButtonOrCreate(const QString& name,const QString& iconPath){
    for(U32 i = 0 ; i < _toolButtons.size();++i){
        if(_toolButtons[i]->getName() == name)
            return _toolButtons[i];
    }
    PluginToolButton* ret = new PluginToolButton(name,iconPath);
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
            ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
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
            ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
            assert(n);
            n->getUiContext()->updateViewsMenu(viewsCount);
        }
    }
}

void AppInstance::notifyViewersProjectFormatChanged(const Format& format){
    const std::vector<Node*>& nodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->className() == "Viewer") {
            ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
            assert(n);
            n->getUiContext()->viewer->onProjectFormatChanged(format);
        }
    }
}
void AppInstance::setViewersCurrentView(int view){
    const std::vector<Node*>& nodes = _currentProject->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->className() == "Viewer") {
            ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
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
    for(PluginsMap::const_iterator it = _plugins.begin();it!=_plugins.end();++it){
        ret.append(it->first);
    }
    return ret;
}

QMutex* AppManager::getMutexForPlugin(const QString& pluginName) const {
    PluginsMap::const_iterator it = _plugins.find(pluginName);
    if(it != _plugins.end()){
        return it->second.second;
    }
    return NULL;
}
void AppManager::printPluginsLoaded(){
    for(PluginsMap::const_iterator it = _plugins.begin();it!=_plugins.end();++it){
        std::cout << it->first.toStdString() << std::endl;
    }
}

Natron::LibraryBinary* AppManager::getPluginBinary(const QString& pluginName) const{
    PluginsMap::const_iterator it = _plugins.find(pluginName);
    if(it != _plugins.end()){
        return it->second.first;
    }
    return NULL;

}
int AppInstance::getKnobsAge() const{
    return _currentProject->getKnobsAge();
}
void AppInstance::incrementKnobsAge(){
    _currentProject->incrementKnobsAge();
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
                if(node->className() == "Viewer"){
                    throw std::invalid_argument("Internal issue with the project loader...viewers should have been evicted from the project.");
                }
                renderers.push_back(dynamic_cast<OutputEffectInstance*>(node->getLiveInstance()));
            }
        }
    }else{
        //start rendering for all writers found in the project
        for (U32 j = 0; j < projectNodes.size(); ++j) {
            if(projectNodes[j]->isOutputNode() && projectNodes[j]->className() != "Viewer"){
                renderers.push_back(dynamic_cast<OutputEffectInstance*>(projectNodes[j]->getLiveInstance()));
            }
        }

    }
    
    if(_isBackground){
    //blocking call, we don't want this function to return pre-maturely, in which case it would kill the app
        QtConcurrent::blockingMap(renderers,boost::bind(&AppInstance::startRenderingFullSequence,this,_1));
    }else{
        for (U32 i = 0; i < renderers.size(); ++i) {
            startRenderingFullSequence(renderers[i]);
        }
    }
}

void AppManager::printUsage(){
    std::cout << NATRON_APPLICATION_NAME << " usage: " << std::endl;
    std::cout << "./" NATRON_APPLICATION_NAME "    <project file path>" << std::endl;
    std::cout << "[--background] enables background mode rendering. No graphical interface will be shown." << std::endl;
    std::cout << "[--writer <Writer node name>] When in background mode, the renderer will only try to render with the node"
    " name following the --writer argument. If no such node exists in the project file, the process will abort."
    "Note that if you don't pass the --writer argument, it will try to start rendering with all the writers in the project's file."<< std::endl;

}

void AppInstance::startRenderingFullSequence(Natron::OutputEffectInstance* writer){
    if(!_isBackground){
        /*Start the renderer in a background process.*/
        autoSave(); //< takes a snapshot of the graph at this time, this will be the version loaded by the process
        QStringList appArgs = QCoreApplication::arguments();
        QStringList processArgs;
        processArgs << _currentProject->getLastAutoSaveFilePath() << "--background" << "--writer" << writer->getName().c_str();
        ProcessHandler* newProcess = 0;
        
        try{
            newProcess = new ProcessHandler(this,appArgs.at(0),processArgs,writer); //< the process will delete itself
        }catch(const std::exception& e){
            Natron::errorDialog(writer->getName(),e.what());
            delete newProcess;
        }
    }else{
        _activeRenderersMutex.lock();
        ActiveBackgroundRender* backgroundRender = new ActiveBackgroundRender(writer);
        _activeRenderers.push_back(backgroundRender);
        _activeRenderersMutex.unlock();
        backgroundRender->blockingRender(); //< doesn't return before rendering is finished
        
        //remove the renderer from the list
        _activeRenderersMutex.lock();
        for (U32 i = 0; i < _activeRenderers.size(); ++i) {
            if (_activeRenderers[i] == backgroundRender) {
                _activeRenderers.erase(_activeRenderers.begin()+i);
                break;
            }
        }
        _activeRenderersMutex.unlock();
    }
}

ProcessHandler::ProcessHandler(AppInstance* app,const QString& programPath,const QStringList& programArgs,Natron::OutputEffectInstance* writer)
: _app(app)
 ,_process(new QProcess)
 ,_hasProcessBeenDeleted(false)
 ,_writer(writer)
 ,_dialog(NULL)
{
    
    
    QObject::connect(_process,SIGNAL(readyReadStandardOutput()),this,SLOT(onStandardOutputBytesWritten()));
    QObject::connect(_process,SIGNAL(error(QProcess::ProcessError)),this,SLOT(onProcessError(QProcess::ProcessError)));
    QObject::connect(_process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(onProcessEnd(int,QProcess::ExitStatus)));
    int firstFrame,lastFrame;
    writer->getFrameRange(&firstFrame, &lastFrame);
    if(firstFrame > lastFrame)
        throw std::invalid_argument("First frame in the sequence is greater than the last frame");
    
    _process->start(programPath,programArgs);

    std::string outputFileSequence;
    if(writer->isOpenFX()){
        outputFileSequence = dynamic_cast<OfxEffectInstance*>(writer)->getOutputFileName();
    }else{
        outputFileSequence = dynamic_cast<Writer*>(writer)->getOutputFileName();
    }
    assert(app->getGui());
    
    _dialog = new RenderingProgressDialog(outputFileSequence.c_str(),firstFrame,lastFrame,app->getGui());
    QObject::connect(_dialog,SIGNAL(canceled()),this,SLOT(onProcessCanceled()));
    _dialog->show();
    
}

ProcessHandler::~ProcessHandler(){
    if(!_hasProcessBeenDeleted)
        delete _process;
}


void ProcessHandler::onStandardOutputBytesWritten(){
    char buf[10000];
    _process->readLine(buf, 10000);
    QString str(buf);
    if(str.contains(kFrameRenderedString)){
        str = str.remove(kFrameRenderedString);
        _dialog->onFrameRendered(str.toInt());
    }else if(str.contains(kRenderingFinishedString)){
        onProcessCanceled();
    }else if(str.contains(kProgressChangedString)){
        str = str.remove(kProgressChangedString);
        _dialog->onCurrentFrameProgress(str.toInt());
    }
}

void ProcessHandler::onProcessCanceled(){
    _dialog->hide();
    QByteArray ba(kAbortRenderingString);
    if(_process->write(ba) == -1){
        std::cout << "Error writing to the process standard input" << std::endl;
    }
    _process->waitForFinished();

}

void ProcessHandler::onProcessError(QProcess::ProcessError err){
    if(err == QProcess::FailedToStart){
        Natron::errorDialog(_writer->getName(),"The render process failed to start");
    }else if(err == QProcess::Crashed){
        Natron::errorDialog(_writer->getName(),"The render process crashed");
        //@TODO: find out a way to get the backtrace
    }
}

void ProcessHandler::onProcessEnd(int exitCode,QProcess::ExitStatus stat){
    if(stat == QProcess::CrashExit){
        Natron::errorDialog(_writer->getName(),"The render process exited after a crash");
        _hasProcessBeenDeleted = true;

    }else if(exitCode == 1){
        Natron::errorDialog(_writer->getName(), "The process ended with a return code of 1, this indicates an undetermined problem occured.");
    }else{
        Natron::informationDialog(_writer->getName(),"Render finished!");
    }
    _dialog->hide();
    delete this;
}

AppInstance::ActiveBackgroundRender::ActiveBackgroundRender(Natron::OutputEffectInstance* writer)
: _running(false)
 ,_writer(writer)
{
    
}

void AppInstance::ActiveBackgroundRender::blockingRender(){
    _writer->renderFullSequence();
    {
        QMutexLocker locker(&_runningMutex);
        _running = true;
        while (_running) {
            _runningCond.wait(&_runningMutex);
        }
    }

}

void AppInstance::ActiveBackgroundRender::notifyFinished(){
    QMutexLocker locker(&_runningMutex);
    _running = false;
    _runningCond.wakeOne();
}

void AppInstance::notifyRenderFinished(Natron::OutputEffectInstance* writer){
    for (U32 i = 0; i < _activeRenderers.size(); ++i) {
        if(_activeRenderers[i]->getWriter() == writer){
            _activeRenderers[i]->notifyFinished();
        }
    }
}

namespace Natron{

void errorDialog(const std::string& title,const std::string& message){
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
    
} //Namespace Natron
