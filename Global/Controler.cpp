//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */








#include <QLabel>
#include <QMessageBox>
#include <QtCore/QDir>
#include "Gui/ViewerGL.h"
#include "Global/Controler.h"
#include "Gui/Gui.h"
#include "Engine/Model.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Engine/VideoEngine.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGraph.h"
#include "Engine/Settings.h"
#include "Gui/SequenceFileDialog.h"


using namespace Powiter;
using namespace std;
Controler::Controler():_model(0),_gui(0){
    
}

void Controler::initControler(Model *model,QLabel* loadingScreen,QString projectName){
    this->_model=model;
    _gui=new Gui();
    
    _gui->createGui();
    
    addBuiltinPluginToolButtons();
    for (U32 i = 0; i < _toolButtons.size(); i++) {
        string name = _toolButtons[i]->_pluginName;
        name.append("  [");
        name.append(_toolButtons[i]->_groups[0]);
        name.append("]");
        _gui->addPluginToolButton(name,
                                  _toolButtons[i]->_groups,
                                  _toolButtons[i]->_pluginName,
                                  _toolButtons[i]->_pluginIconPath,
                                  _toolButtons[i]->_groupIconPath);
        delete _toolButtons[i];
    }
    _toolButtons.clear();
    
    loadingScreen->hide();
    
    
    
#ifdef __POWITER_OSX__
	_gui->show();
    
#else
	_gui->showMaximized();
#endif
    
    delete loadingScreen;
    
    if(!findAutoSave()){
        if(projectName.isEmpty()){
            createNode("Viewer");
        }else{
            loadProject(projectName);
        }
    }
    
}

Controler::~Controler(){
    removeAutoSaves();
    delete _model;
}

void Controler::addBuiltinPluginToolButtons(){
    vector<string> grouping;
    grouping.push_back("io");
    _gui->addPluginToolButton("Reader", grouping, "Reader", "", IMAGES_PATH"ioGroupingIcon.png");
    _gui->addPluginToolButton("Viewer", grouping, "Viewer", "", IMAGES_PATH"ioGroupingIcon.png");
    _gui->addPluginToolButton("Writer", grouping, "Writer", "", IMAGES_PATH"ioGroupingIcon.png");
}

const QStringList& Controler::getNodeNameList(){
    return _model->getNodeNameList();
}


Node* Controler::createNode(QString name){
    
    Node* node = 0;
    if(_model->createNode(node,name.toStdString())){
        _gui->createNodeGUI(node);
    }else{
        cout << "(Controler::createNode): Couldn't create Node " << name.toStdString() << endl;
        return NULL;
    }
    return node;
}

ViewerNode* Controler::getCurrentViewer(){
    Controler* ctrl = Controler::instance();
    return ctrl->getModel()->getVideoEngine()->getCurrentDAG().outputAsViewer();
}

Writer* Controler::getCurrentWriter(){
    Controler* ctrl = Controler::instance();
    return ctrl->getModel()->getVideoEngine()->getCurrentDAG().outputAsWriter();
}
void Controler::stackPluginToolButtons(const std::vector<std::string>& groups,
                                       const std::string& pluginName,
                                       const std::string& pluginIconPath,
                                       const std::string& groupIconPath){
    _toolButtons.push_back(new PluginToolButton(groups,pluginName,pluginIconPath,groupIconPath));
}
const std::vector<NodeGui*>& Controler::getAllActiveNodes() const{
    assert(_gui->_nodeGraphTab->_nodeGraphArea);
    return _gui->_nodeGraphTab->_nodeGraphArea->getAllActiveNodes();
}
void Controler::loadProject(const QString& name){
    _model->loadProject(name);
    QDateTime time = QDateTime::currentDateTime();
    _currentProject._hasProjectBeenSavedByUser = true;
    _currentProject._projectName = SequenceFileDialog::removePath(name);
    _currentProject._age = time;
    _currentProject._lastAutoSave = time;
}
void Controler::saveProject(const QString& name,bool autoSave){
    QDateTime time = QDateTime::currentDateTime();
    if(!autoSave) {
        
        if((_currentProject._age != _currentProject._lastAutoSave) ||
           !QFile::exists(QString(Settings::getPowiterCurrentSettings()->_generalSettings._projectsDirectory.c_str()+
                                 name))){
            
            _model->saveProject(QString(Settings::getPowiterCurrentSettings()->_generalSettings._projectsDirectory.c_str()+
                                        name));
            _currentProject._hasProjectBeenSavedByUser = true;
            _currentProject._projectName = name;
            _currentProject._age = time;
            _currentProject._lastAutoSave = time;
        }
    }else{
        if(!_gui->isGraphWorthless()){
            
            removeAutoSaves();
            _model->saveProject(QString(Settings::getPowiterCurrentSettings()->_generalSettings._projectsDirectory.c_str()+
                                        name)+"."+time.toString("dd.MM.yyyy.hh:mm:ss:zzz"));
            _currentProject._projectName = name;
            _currentProject._lastAutoSave = time;
        }
    }
}
void Controler::removeAutoSaves() const{
    /*removing all previous autosave files*/
    QDir savesDir(Settings::getPowiterCurrentSettings()->_generalSettings._projectsDirectory.c_str());
    QStringList entries = savesDir.entryList();
    for(int i = 0; i < entries.size();i++){
        const QString& entry = entries.at(i);
        int suffixPos = entry.indexOf(".rs.");
        if (suffixPos != -1) {
            QFile::remove(savesDir.path()+QDir::separator()+entry);
        }
    }
}

void Controler::clearInternalNodes(){
    _model->clearNodes();
}
void Controler::clearNodeGuis(){
    _gui->_nodeGraphTab->_nodeGraphArea->clear();
}
bool Controler::isSaveUpToDate() const{
    return _currentProject._age == _currentProject._lastAutoSave;
}

bool Controler::findAutoSave(){
    QDir savesDir(Settings::getPowiterCurrentSettings()->_generalSettings._projectsDirectory.c_str());
    QStringList entries = savesDir.entryList();
    for(int i = 0; i < entries.size();i++){
        const QString& entry = entries.at(i);
        int suffixPos = entry.indexOf(".rs.");
        if (suffixPos != -1) {
            QString filename = entry.left(suffixPos+3);
            QString filenameWithPath = savesDir.path() + QDir::separator() + filename;
            bool exists = QFile::exists(filenameWithPath);
            QString text;
            _model->loadProject(savesDir.path()+ QDir::separator() + entry);
            if(exists){
                text = "A recent auto-save of " + filename + " was found. You can preview it now.\n"
                "Would you like to restore it entirely? Clicking No will remove this auto-save.";
            }else{
                text = "An auto-save was restored successfully. It didn't belong to any project\n"
                "You can preview it now.\n"
                "Would you like to restore it ? Clicking No will remove this auto-save forever.";
            }
            QMessageBox::StandardButton ret = QMessageBox::question(_gui, "Auto-save", text,
                                                                    QMessageBox::Yes | QMessageBox::No);
            if(ret == QMessageBox::No || ret == QMessageBox::Escape){
                removeAutoSaves();
                return false;
            }else{
                removeAutoSaves();
                if(exists){
                    QDateTime now = QDateTime::currentDateTime();
                    _currentProject._projectName = filename;
                    _currentProject._lastAutoSave = now;
                    _currentProject._hasProjectBeenSavedByUser = true;
                    _currentProject._age = now;
                }
                return true;
            }
        }
    }
    removeAutoSaves();
    return false;
}
void Controler::deselectAllNodes() const{
    _gui->_nodeGraphTab->_nodeGraphArea->deselect();
}

void Controler::showErrorDialog(const QString& title,const QString& message) const{
    _gui->errorDialog(title, message);
}