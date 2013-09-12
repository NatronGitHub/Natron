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
#include <QLabel>
#include <QMessageBox>
#include <QtCore/QDir>

#include "Global/LibraryBinary.h"
#include "Global/MemoryInfo.h"


#include "Gui/ViewerGL.h"
#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGraph.h"
#include "Gui/SequenceFileDialog.h"

#include "Engine/Model.h"
#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerCache.h"
#include "Engine/Knob.h"
#include "Engine/NodeCache.h"
#include "Engine/OfxHost.h"
#include "Engine/Format.h"


#include "Readers/Reader.h"
#include "Readers/Read.h"
#include "Readers/ReadExr.h"
#include "Readers/ReadFfmpeg_deprecated.h"
#include "Readers/ReadQt.h"

#include "Writers/Writer.h"
#include "Writers/Write.h"
#include "Writers/WriteQt.h"
#include "Writers/WriteExr.h"



using namespace Powiter;
using namespace std;

AppInstance::AppInstance(int appID,const QString& projectName):
_model(0),_gui(0),_appID(appID)
{
    _model = new Model(this);
    _gui=new Gui(this);
    
    _gui->createGui();
    
    const vector<AppManager::PluginToolButton*>& _toolButtons = appPTR->getPluginsToolButtons();
    for (U32 i = 0; i < _toolButtons.size(); ++i) {
        assert(_toolButtons[i]);
        QString name = _toolButtons[i]->_pluginName;
        if (_toolButtons[i]->_groups.size() >= 1) {
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
    _gui->show();
        
    /*Check if auto-save dir already exists*/
    QDir dir(QDir::tempPath()+QDir::separator()+"Powiter");
    if(!dir.exists()){
        QDir::temp().mkdir("Powiter");
    }
    
    /*If this is the first instance of the software*/
    if(_appID == 0){
        if(!findAutoSave()){
            if(projectName.isEmpty()){
                QString text("Powiter - ");
                text.append(_currentProject._projectName);
                _gui->setWindowTitle(text);
                createNode("Viewer");
            }else{
                QString name = SequenceFileDialog::removePath(projectName);
                QString path = projectName.left(projectName.indexOf(name));
                loadProject(path,name);
            }
        }
    }else{
        if(projectName.isEmpty()){
            QString text("Powiter - ");
            text.append(_currentProject._projectName);
            _gui->setWindowTitle(text);
            createNode("Viewer");
        }else{
            QString name = SequenceFileDialog::removePath(projectName);
            QString path = projectName.left(projectName.indexOf(name));
            loadProject(path,name);
        }
    }
    
}


AppInstance::~AppInstance(){
    removeAutoSaves();
    delete _model;
    appPTR->removeInstance(_appID);
}

void AppInstance::updateDAG(OutputNode *output,bool isViewer){
    _model->updateDAG(output, isViewer);
}

Node* AppInstance::createNode(const QString& name) {
    Node* node = _model->createNode(name.toStdString());
    NodeGui* nodegui = 0;
    if (node) {
        nodegui = _gui->createNodeGUI(node);
        _nodeMapping.insert(make_pair(node,nodegui));
        node->initializeInputs();
        Node* selected  =  0;
        if(_gui->getSelectedNode()){
            selected = _gui->getSelectedNode()->getNode();
        }
        _model->initNodeCountersAndSetName(node);
        autoConnect(selected, node);
        if(node->className() == "Viewer"){
            _gui->createViewerGui(node);
        }
        node->initializeKnobs();
    } else {
        cout << "(Controler::createNode): Couldn't create Node " << name.toStdString() << endl;
        return NULL;
    }
    return node;
}
void AppInstance::autoConnect(Node* target,Node* created){
    _gui->autoConnect(getNodeGui(target),getNodeGui(created));
}

OutputNode* AppInstance::getCurrentOutput(){
    return _model->getCurrentOutput();
}
ViewerNode* AppInstance::getCurrentViewer(){
    OutputNode* output = _model->getCurrentOutput();
    return dynamic_cast<ViewerNode*>(output);
}

bool AppInstance::isRendering() const{
    return _model->getVideoEngine()->isWorking();
}
void AppInstance::changeDAGAndStartRendering(OutputNode* output){
    _model->getVideoEngine()->changeDAGAndStartEngine(output);
}
void AppInstance::startRendering(int nbFrames){
    _model->startVideoEngine(nbFrames);
}

Writer* AppInstance::getCurrentWriter(){
    OutputNode* output = _model->getCurrentOutput();
    return dynamic_cast<Writer*>(output);
}

const std::vector<Node*> AppInstance::getAllActiveNodes() const{
    assert(_gui->_nodeGraphTab->_nodeGraphArea);
    const std::vector<NodeGui*>&  actives= _gui->_nodeGraphTab->_nodeGraphArea->getAllActiveNodes();
    vector<Node*> ret;
    for (U32 j = 0; j < actives.size(); ++j) {
        ret.push_back(actives[j]->getNode());
    }
    return ret;
}
void AppInstance::loadProject(const QString& path,const QString& name){
    _model->loadProject(path+name);
    QDateTime time = QDateTime::currentDateTime();
    _currentProject._hasProjectBeenSavedByUser = true;
    _currentProject._projectName = name;
    _currentProject._projectPath = path;
    _currentProject._age = time;
    _currentProject._lastAutoSave = time;
    QString text("Powiter - ");
    text.append(_currentProject._projectName);
    _gui->setWindowTitle(text);
}
void AppInstance::saveProject(const QString& path,const QString& name,bool autoSave){
    QDateTime time = QDateTime::currentDateTime();
    if(!autoSave) {
        
        if((_currentProject._age != _currentProject._lastAutoSave) ||
           !QFile::exists(path+name)){
            
            _model->saveProject(path,name);
            _currentProject._hasProjectBeenSavedByUser = true;
            _currentProject._projectName = name;
            _currentProject._projectPath = path;
            _currentProject._age = time;
            _currentProject._lastAutoSave = time;
            QString text("Powiter - ");
            text.append(_currentProject._projectName);
            _gui->setWindowTitle(text);
        }
    }else{
        if(!_gui->isGraphWorthless()){
            
            removeAutoSaves();
            _model->saveProject(path,name+"."+time.toString("dd.MM.yyyy.hh:mm:ss:zzz"),true);
            _currentProject._projectName = name;
            _currentProject._projectPath = path;
            _currentProject._lastAutoSave = time;
        }
    }
}

void AppInstance::autoSave(){
    saveProject(_currentProject._projectPath, _currentProject._projectName, true);
}
void AppInstance::triggerAutoSaveOnNextEngineRun(){
    _model->triggerAutoSaveOnNextEngineRun();
}

void AppInstance::setProjectTitleAsModified(){
    QString text("Powiter - ");
    text.append(_currentProject._projectName);
    text.append(" (*)");
    _gui->setWindowTitle(text);
    
}
void AppInstance::removeAutoSaves() const{
    /*removing all previous autosave files*/
    QDir savesDir(autoSavesDir());
    QStringList entries = savesDir.entryList();
    for(int i = 0; i < entries.size();++i) {
        const QString& entry = entries.at(i);
        int suffixPos = entry.indexOf(".rs.");
        if (suffixPos != -1) {
            QFile::remove(savesDir.path()+QDir::separator()+entry);
        }
    }
}
void AppInstance::clearNodes(){
    _model->clearNodes();
}

bool AppInstance::isSaveUpToDate() const{
    return _currentProject._age == _currentProject._lastAutoSave;
}
void AppInstance::resetCurrentProject(){
    _currentProject._hasProjectBeenSavedByUser = false;
    _currentProject._projectName = "Untitled.rs";
    _currentProject._projectPath = "";
    QString text("Powiter - ");
    text.append(_currentProject._projectName);
    _gui->setWindowTitle(text);
}
bool AppInstance::findAutoSave(){
    QDir savesDir(autoSavesDir());
    QStringList entries = savesDir.entryList();
    for(int i = 0; i < entries.size();++i) {
        const QString& entry = entries.at(i);
        int suffixPos = entry.indexOf(".rs.");
        if (suffixPos != -1) {
            QFile autoSave(savesDir.path()+QDir::separator()+entry);
            autoSave.open(QIODevice::ReadOnly);
            QTextStream inStream(&autoSave);
            QString firstLine = inStream.readLine();
            QString path;
            if(!firstLine.isEmpty()){
                int j = 0;
                while(j < firstLine.size() &&  (firstLine.at(j) != QChar('\n'))){
                    path.append(firstLine.at(j));
                    ++j;
                }
            }
            autoSave.close();
            QString filename = entry.left(suffixPos+3);
            QString filenameWithPath = path + QDir::separator() + filename;
            bool exists = QFile::exists(filenameWithPath);
            QString text;
            
            
            QDateTime time = QDateTime::currentDateTime();
            if(exists){
                _currentProject._hasProjectBeenSavedByUser = true;
                _currentProject._projectName = filename;
                _currentProject._projectPath = path;
            }else{
                _currentProject._hasProjectBeenSavedByUser = false;
                _currentProject._projectName = "Untitled.rs";
                _currentProject._projectPath = "";
            }
            _currentProject._age = time;
            _currentProject._lastAutoSave = time;
            QString title("Powiter - ");
            title.append(_currentProject._projectName);
            title.append(" (*)");
            _gui->setWindowTitle(title);

            if(exists){
                text = "A recent auto-save of " + filename + " was found.\n"
                "Would you like to restore it entirely? Clicking No will remove this auto-save.";
            }else{
                text = "An auto-save was restored successfully. It didn't belong to any project\n"
                "Would you like to restore it ? Clicking No will remove this auto-save forever.";
            }

            
            QMessageBox::StandardButton ret = QMessageBox::question(_gui, "Auto-save", text,
                                                                    QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes);
            if(ret == QMessageBox::No || ret == QMessageBox::Escape){
                removeAutoSaves();
                clearNodes();
                resetCurrentProject();
                return false;
            }else{
                _model->loadProject(savesDir.path()+ QDir::separator() + entry,true);
                removeAutoSaves();
                if(exists){
                    QDateTime now = QDateTime::currentDateTime();
                    _currentProject._projectName = filename;
                    _currentProject._projectPath = path;
                    _currentProject._lastAutoSave = now;
                    _currentProject._hasProjectBeenSavedByUser = true;
                    _currentProject._age = now.addSecs(1);
                }
                return true;
            }
        }
    }
    removeAutoSaves();
    return false;
}
const QString AppInstance::autoSavesDir() {return QString(QDir::tempPath() + QDir::separator() + "Powiter" + QDir::separator());}

void AppInstance::deselectAllNodes() const{
    _gui->_nodeGraphTab->_nodeGraphArea->deselect();
}

void AppInstance::showErrorDialog(const QString& title,const QString& message) const{
    _gui->errorDialog(title, message);

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
    map<int,AppInstance*>::const_iterator it;
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
    return _model->connect(inputNumber, parentName, output);
}

bool AppInstance::connect(int inputNumber,Node* input,Node* output){
    return _model->connect(inputNumber, input, output);
}
bool AppInstance::disconnect(Node* input,Node* output){
    return _model->disconnect(input, output);
}


NodeGui* AppInstance::getNodeGui(Node* n) const {
    map<Node*,NodeGui*>::const_iterator it = _nodeMapping.find(n);
    if(it==_nodeMapping.end()){
        return NULL;
    }else{
        return it->second;
    }
    }
Node* AppInstance::getNode(NodeGui* n) const{
    for (map<Node*,NodeGui*>::const_iterator it = _nodeMapping.begin(); it!=_nodeMapping.end(); ++it) {
        if(it->second == n){
            return it->first;
        }
    }
    return NULL;

}

void AppInstance::connectViewersToViewerCache(){
    _model->connectViewersToViewerCache();
}

void AppInstance::disconnectViewersFromViewerCache(){
    _model->disconnectViewersFromViewerCache();
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
                int index = filename.lastIndexOf("."POWITER_LIBRARY_EXT);
                className = filename.left(index);
                string binaryPath = POWITER_PLUGINS_PATH + className.toStdString() + "." + POWITER_LIBRARY_EXT;
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
    map<int,AppInstance*>::const_iterator it = _appInstances.find(_topLevelInstanceID);
    if(it == _appInstances.end()){
        return NULL;
    }else{
        return it->second;
    }
}

AppManager::AppManager():
_availableID(0),
_topLevelInstanceID(0),
ofxHost(new Powiter::OfxHost())
{
    connect(ofxHost.get(), SIGNAL(toolButtonAdded(QStringList,QString,QString,QString)),
                     this, SLOT(addPluginToolButtons(QStringList,QString,QString,QString)));
    
    /*loading all plugins*/
    loadAllPlugins();
    
    loadBuiltinFormats();
    
    /*node cache initialisation & restoration*/
    NodeCache* nc = NodeCache::getNodeCache();
    U64 nodeCacheMaxSize = (Settings::getPowiterCurrentSettings()->_cacheSettings.maxCacheMemoryPercent-
                            Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent)*
    getSystemTotalRAM();
    nc->setMaximumCacheSize(nodeCacheMaxSize);
    
    
    /*viewer cache initialisation & restoration*/
    ViewerCache* viewerCache = ViewerCache::getViewerCache();
    viewerCache->setMaximumCacheSize((U64)((double)Settings::getPowiterCurrentSettings()->_cacheSettings.maxDiskCache));
    viewerCache->setMaximumInMemorySize(Settings::getPowiterCurrentSettings()->_cacheSettings.maxPlayBackMemoryPercent);
    viewerCache->restore();
    
    KnobFactory::instance();
    
}

AppManager::~AppManager(){
    ViewerCache::getViewerCache()->save();
    for(ReadPluginsIterator it = _readPluginsLoaded.begin(); it!=_readPluginsLoaded.end(); ++it) {
        delete it->second.second;
    }
    for(WritePluginsIterator it = _writePluginsLoaded.begin(); it!=_writePluginsLoaded.end(); ++it) {
        delete it->second.second;
    }
    _nodeNames.clear();
    foreach(Format* f,_formats){
        delete f;
    }
    foreach(PluginToolButton* p,_toolButtons){
        delete p;
    }
}

void AppManager::loadAllPlugins(){
    /*loading node plugins*/
    loadBuiltinNodePlugins();
    
    /*loading read plugins*/
    loadReadPlugins();
    
    /*loading write plugins*/
    loadWritePlugins();
    
    /*loading ofx plugins*/
    QStringList ofxPluginNames = ofxHost->loadOFXPlugins();
    _nodeNames.append(ofxPluginNames);
    
}

void AppManager::loadReadPlugins(){
    vector<string> functions;
    functions.push_back("BuildRead");
    vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(POWITER_READERS_PLUGINS_PATH, functions);
    for (U32 i = 0 ; i < plugins.size(); ++i) {
        pair<bool,ReadBuilder> func = plugins[i]->findFunction<ReadBuilder>("BuildRead");
        if(func.first){
            Read* read = func.second(NULL);
            assert(read);
            vector<string> extensions = read->fileTypesDecoded();
            string decoderName = read->decoderName();
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
            defaultMapping.insert(make_pair("jpg", it->second.second));
            defaultMapping.insert(make_pair("bmp", it->second.second));
            defaultMapping.insert(make_pair("jpeg", it->second.second));
            defaultMapping.insert(make_pair("gif", it->second.second));
            defaultMapping.insert(make_pair("png", it->second.second));
            defaultMapping.insert(make_pair("pbm", it->second.second));
            defaultMapping.insert(make_pair("pgm", it->second.second));
            defaultMapping.insert(make_pair("ppm", it->second.second));
            defaultMapping.insert(make_pair("xbm", it->second.second));
            defaultMapping.insert(make_pair("xpm", it->second.second));
        }
    }
    Settings::getPowiterCurrentSettings()->_readersSettings.fillMap(defaultMapping);
}

void AppManager::loadBuiltinReads(){
    Read* readExr = ReadExr::BuildRead(NULL);
    assert(readExr);
    std::vector<std::string> extensions = readExr->fileTypesDecoded();
    std::string decoderName = readExr->decoderName();
    
    std::map<std::string,void*> EXRfunctions;
    EXRfunctions.insert(make_pair("BuildRead", (void*)&ReadExr::BuildRead));
    LibraryBinary *EXRplugin = new LibraryBinary(EXRfunctions);
    assert(EXRplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,EXRplugin)));
    }
    delete readExr;
    
    Read* readQt = ReadQt::BuildRead(NULL);
    assert(readQt);
    extensions = readQt->fileTypesDecoded();
    decoderName = readQt->decoderName();
    
    std::map<std::string,void*> Qtfunctions;
    Qtfunctions.insert(make_pair("BuildRead", (void*)&ReadQt::BuildRead));
    LibraryBinary *Qtplugin = new LibraryBinary(Qtfunctions);
    assert(Qtplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,Qtplugin)));
    }
    delete readQt;
    
    Read* readFfmpeg = ReadFFMPEG::BuildRead(NULL);
    assert(readFfmpeg);
    extensions = readFfmpeg->fileTypesDecoded();
    decoderName = readFfmpeg->decoderName();
    
    std::map<std::string,void*> FFfunctions;
    FFfunctions.insert(make_pair("ReadBuilder", (void*)&ReadFFMPEG::BuildRead));
    LibraryBinary *FFMPEGplugin = new LibraryBinary(FFfunctions);
    assert(FFMPEGplugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _readPluginsLoaded.insert(make_pair(decoderName,make_pair(extensions,FFMPEGplugin)));
    }
    delete readFfmpeg;
    
}
void AppManager::loadBuiltinNodePlugins(){
    // these  are built-in nodes
    _nodeNames.append("Reader");
    _nodeNames.append("Viewer");
    _nodeNames.append("Writer");
    QStringList grouping;
    grouping.push_back("io");
    addPluginToolButtons(grouping, "Reader", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
    addPluginToolButtons(grouping, "Viewer", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
    addPluginToolButtons(grouping, "Writer", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
}

/*loads extra writer plug-ins*/
void AppManager::loadWritePlugins(){
    
    vector<string> functions;
    functions.push_back("BuildWrite");
    vector<LibraryBinary*> plugins = AppManager::loadPluginsAndFindFunctions(POWITER_WRITERS_PLUGINS_PATH, functions);
    for (U32 i = 0 ; i < plugins.size(); ++i) {
        pair<bool,WriteBuilder> func = plugins[i]->findFunction<WriteBuilder>("BuildWrite");
        if(func.first){
            Write* write = func.second(NULL);
            assert(write);
            vector<string> extensions = write->fileTypesEncoded();
            string encoderName = write->encoderName();
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
            defaultMapping.insert(make_pair("jpg", it->second.second));
            defaultMapping.insert(make_pair("bmp", it->second.second));
            defaultMapping.insert(make_pair("jpeg", it->second.second));
            defaultMapping.insert(make_pair("gif", it->second.second));
            defaultMapping.insert(make_pair("png", it->second.second));
            defaultMapping.insert(make_pair("pbm", it->second.second));
            defaultMapping.insert(make_pair("pgm", it->second.second));
            defaultMapping.insert(make_pair("ppm", it->second.second));
            defaultMapping.insert(make_pair("xbm", it->second.second));
            defaultMapping.insert(make_pair("xpm", it->second.second));
        }
    }
    Settings::getPowiterCurrentSettings()->_writersSettings.fillMap(defaultMapping);
}

/*loads writes that are built-ins*/
void AppManager::loadBuiltinWrites(){
    Write* writeQt = WriteQt::BuildWrite(NULL);
    assert(writeQt);
    std::vector<std::string> extensions = writeQt->fileTypesEncoded();
    string encoderName = writeQt->encoderName();
    
    std::map<std::string,void*> Qtfunctions;
    Qtfunctions.insert(make_pair("BuildWrite",(void*)&WriteQt::BuildWrite));
    LibraryBinary *QtWritePlugin = new LibraryBinary(Qtfunctions);
    assert(QtWritePlugin);
    for (U32 i = 0 ; i < extensions.size(); ++i) {
        _writePluginsLoaded.insert(make_pair(encoderName,make_pair(extensions,QtWritePlugin)));
    }
    delete writeQt;
    
    Write* writeEXR = WriteExr::BuildWrite(NULL);
    std::vector<std::string> extensionsExr = writeEXR->fileTypesEncoded();
    string encoderNameExr = writeEXR->encoderName();
    
    std::map<std::string,void*> EXRfunctions;
    EXRfunctions.insert(make_pair("BuildWrite",(void*)&WriteExr::BuildWrite));
    LibraryBinary *ExrWritePlugin = new LibraryBinary(EXRfunctions);
    assert(ExrWritePlugin);
    for (U32 i = 0 ; i < extensionsExr.size(); ++i) {
        _writePluginsLoaded.insert(make_pair(encoderName,make_pair(extensionsExr,ExrWritePlugin)));
    }
    delete writeEXR;
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
    
    std::vector< std::vector<float> > resolutions;
    std::vector<float> pcvideo; pcvideo.push_back(640); pcvideo.push_back(480); pcvideo.push_back(1);
    std::vector<float> ntsc; ntsc.push_back(720); ntsc.push_back(486); ntsc.push_back(0.91f);
    std::vector<float> pal; pal.push_back(720); pal.push_back(576); pal.push_back(1.09f);
    std::vector<float> hd; hd.push_back(1920); hd.push_back(1080); hd.push_back(1);
    std::vector<float> ntsc169; ntsc169.push_back(720); ntsc169.push_back(486); ntsc169.push_back(1.21f);
    std::vector<float> pal169; pal169.push_back(720); pal169.push_back(576); pal169.push_back(1.46f);
    std::vector<float> super351k; super351k.push_back(1024); super351k.push_back(778); super351k.push_back(1);
    std::vector<float> cine1k; cine1k.push_back(914); cine1k.push_back(778); cine1k.push_back(2);
    std::vector<float> super352k; super352k.push_back(2048); super352k.push_back(1556); super352k.push_back(1);
    std::vector<float> cine2K; cine2K.push_back(1828); cine2K.push_back(1556); cine2K.push_back(2);
    std::vector<float> super4K35; super4K35.push_back(4096); super4K35.push_back(3112); super4K35.push_back(1);
    std::vector<float> cine4K; cine4K.push_back(3656); cine4K.push_back(3112); cine4K.push_back(2);
    std::vector<float> square256; square256.push_back(256); square256.push_back(256); square256.push_back(1);
    std::vector<float> square512; square512.push_back(512); square512.push_back(512); square512.push_back(1);
    std::vector<float> square1K; square1K.push_back(1024); square1K.push_back(1024); square1K.push_back(1);
    std::vector<float> square2K; square2K.push_back(2048); square2K.push_back(2048); square2K.push_back(1);
    
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
        const std::vector<float>& v = resolutions[i];
        assert(v.size() >= 3);
        Format* _frmt = new Format(0,0,v[0],v[1],formatNames[i],v[2]);
        assert(_frmt);
        _formats.push_back(_frmt);
    }

}

Format* AppManager::findExistingFormat(int w, int h, double pixel_aspect){
    
	for(U32 i =0;i< _formats.size();++i) {
		Format* frmt = _formats[i];
        assert(frmt);
		if(frmt->width() == w && frmt->height() == h && frmt->pixel_aspect()==pixel_aspect){
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
    for(map<int,AppInstance*>::iterator it = _appInstances.begin();it!=_appInstances.end();++it){
        if (it->first != _topLevelInstanceID) {
            it->second->disconnectViewersFromViewerCache();
        }else{
            it->second->connectViewersToViewerCache();
        }
    }
}