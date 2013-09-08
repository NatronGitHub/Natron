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

#include "Global/NodeInstance.h"

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
#include "Engine/LibraryBinary.h"

#include "Writers/Writer.h"



using namespace Powiter;
using namespace std;

AppInstance::AppInstance(int appID,const QString& projectName):
_model(0),_gui(0),_appID(appID)
{
    _model = new Model(this);
    _gui=new Gui(this);
    
    _gui->createGui();
    
    addBuiltinPluginToolButtons();
    for (U32 i = 0; i < _toolButtons.size(); ++i) {
        assert(_toolButtons[i]);
        string name = _toolButtons[i]->_pluginName;
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
        delete _toolButtons[i];
    }
    _toolButtons.clear();
        
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
    for (U32 i = 0; i < _currentNodes.size(); ++i) {
        delete _currentNodes[i];
    }
    removeAutoSaves();
    delete _model;
    appPTR->removeInstance(_appID);
}

std::pair<int,bool> AppInstance::setCurrentGraph(OutputNode *output,bool isViewer){
    return _model->setCurrentGraph(output, isViewer);
}

void AppInstance::addBuiltinPluginToolButtons(){
    vector<string> grouping;
    grouping.push_back("io");
    _gui->addPluginToolButton("Reader", grouping, "Reader", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
    _gui->addPluginToolButton("Viewer", grouping, "Viewer", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
    _gui->addPluginToolButton("Writer", grouping, "Writer", "", POWITER_IMAGES_PATH"ioGroupingIcon.png");
}

const QStringList& AppInstance::getNodeNameList(){
    return _model->getNodeNameList();
}


NodeInstance* AppInstance::createNode(const QString& name) {
    Node* node = _model->createNode(name.toStdString());
    NodeGui* nodegui = 0;
    NodeInstance* instance = 0;
    if (node) {
        instance = new NodeInstance(node,this);
        nodegui = _gui->createNodeGUI(instance);
        instance->setNodeGuiPTR(nodegui);
        instance->initializeInputs();
        instance->initializeKnobs();
    } else {
        cout << "(Controler::createNode): Couldn't create Node " << name.toStdString() << endl;
        return NULL;
    }
    initNodeCountersAndSetName(instance);
    return instance;
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

void AppInstance::stackPluginToolButtons(const std::vector<std::string>& groups,
                                         const std::string& pluginName,
                                         const std::string& pluginIconPath,
                                         const std::string& groupIconPath){
    _toolButtons.push_back(new PluginToolButton(groups,pluginName,pluginIconPath,groupIconPath));
}
const std::vector<NodeInstance*> AppInstance::getAllActiveNodes() const{
    assert(_gui->_nodeGraphTab->_nodeGraphArea);
    const std::vector<NodeGui*>&  actives= _gui->_nodeGraphTab->_nodeGraphArea->getAllActiveNodes();
    vector<NodeInstance*> ret;
    for (U32 j = 0; j < actives.size(); ++j) {
        ret.push_back(actives[j]->getNodeInstance());
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
    if(_model->getVideoEngine()){
        _model->getVideoEngine()->triggerAutoSaveOnNextRun();
        QString text("Powiter - ");
        text.append(_currentProject._projectName);
        text.append(" (*)");
        _gui->setWindowTitle(text);
    }
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
    foreach(NodeInstance* n,_currentNodes){
        delete n;
    }
    _currentNodes.clear();
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
                if(getCurrentViewer())
                    getCurrentViewer()->getUiContext()->viewer->disconnectViewer();
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

bool AppInstance::connect(int inputNumber,const std::string& parentName,NodeInstance* output){
    for (U32 i = 0; i < _currentNodes.size(); ++i) {
        if (_currentNodes[i]->getName() == parentName) {
            connect(inputNumber,_currentNodes[i], output);
            return true;
        }
    }
    return false;
}

bool AppInstance::connect(int inputNumber,NodeInstance* input,NodeInstance* output){

    if(!output->connectInput(input, inputNumber)){
        return false;
    }
    input->connectOutput(output);
    input->refreshEdgesGui();
    output->refreshEdgesGui();
    return true;
}
bool AppInstance::disconnect(NodeInstance* input,NodeInstance* output){
    if(!output->disconnectInput(input)){
        return false;
    }
    if(!input->disconnectOutput(output)){
        return false;
    }
    input->refreshEdgesGui();
    output->refreshEdgesGui();
    return true;
}

void AppInstance::initNodeCountersAndSetName(NodeInstance* n){
    assert(n);
    map<string,int>::iterator it = _nodeCounters.find(n->className());
    if(it == _nodeCounters.end()){
        it->second++;
        n->setName(QString(QString(n->className().c_str())+ "_" + QString::number(it->second)));
    }else{
        _nodeCounters.insert(make_pair(n->className(), 1));
        n->setName(QString(QString(n->className().c_str())+ "_" + QString::number(1)));
    }
}

std::vector<LibraryBinary*> AppManager::loadPlugins(const QString &where){
    std::vector<LibraryBinary*> ret;
    QDir d(where);
    if (d.isReadable())
    {
        QStringList filters;
        filters << QString(QString("*.")+QString(LIBRARY_EXT));
        d.setNameFilters(filters);
		QStringList fileList = d.entryList();
        for(int i = 0 ; i < fileList.size() ; ++i) {
            QString filename = fileList.at(i);
            if(filename.endsWith(".dll") || filename.endsWith(".dylib") || filename.endsWith(".so")){
                QString className;
                int index = filename.lastIndexOf("."LIBRARY_EXT);
                className = filename.left(index);
                string dll = POWITER_PLUGINS_PATH + className.toStdString() + "." + LIBRARY_EXT;
                
                vector<string> functions;
                functions.push_back("BuildKnob");
                LibraryBinary* plugin = new LibraryBinary(dll,functions);
                if(!plugin->isValid()){
                    delete plugin;
                }
                pair<bool,KnobBuilder> builder = plugin->findFunction<KnobBuilder>("BuildKnob");
                if(!builder.first){
                    continue;
                }else{
                    Knob* knob = builder.second(NULL,str,1,0);
                    _loadedKnobs.insert(make_pair(knob->name(), plugin));
                    delete knob;
                }
            }else{
                continue;
            }
        }
    }

}
