//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Controler.h"

#include <cassert>
#include <QLabel>
#include <QMessageBox>
#include <QtCore/QDir>

#include "Gui/ViewerGL.h"
#include "Gui/Gui.h"
#include "Engine/Model.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Engine/VideoEngine.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGraph.h"
#include "Engine/Settings.h"
#include "Writers/Writer.h"
#include "Gui/SequenceFileDialog.h"
#include "Engine/ViewerNode.h"


using namespace Powiter;
using namespace std;

Controler::Controler():_model(0),_gui(0){
}

void Controler::initControler(Model *model,QLabel* loadingScreen,QString projectName){
    this->_model=model;
    _gui=new Gui();
    
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
    
    loadingScreen->hide();
    
    
    
#ifdef __POWITER_OSX__
	_gui->show();
    
#else
	_gui->showMaximized();
#endif
    
    delete loadingScreen;
    
    /*Check if auto-save dir already exists*/
    QDir dir(QDir::tempPath()+QDir::separator()+"Powiter");
    if(!dir.exists()){
        QDir::temp().mkdir("Powiter");
    }
    
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

OutputNode* Controler::getCurrentOutput(){
    return instance()->_model->getCurrentOutput();
}
ViewerNode* Controler::getCurrentViewer(){
    OutputNode* output = instance()->_model->getCurrentOutput();
    return dynamic_cast<ViewerNode*>(output);
}

Writer* Controler::getCurrentWriter(){
    OutputNode* output = instance()->_model->getCurrentOutput();
    return dynamic_cast<Writer*>(output);
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
void Controler::loadProject(const QString& path,const QString& name){
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
void Controler::saveProject(const QString& path,const QString& name,bool autoSave){
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

void Controler::autoSave(){
    saveProject(_currentProject._projectPath, _currentProject._projectName, true);
}
void Controler::triggerAutoSaveOnNextEngineRun(){
    if(_model->getVideoEngine()){
        _model->getVideoEngine()->triggerAutoSaveOnNextRun();
        QString text("Powiter - ");
        text.append(_currentProject._projectName);
        text.append(" (*)");
        _gui->setWindowTitle(text);
    }
}
void Controler::removeAutoSaves() const{
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

void Controler::clearInternalNodes(){
    _model->clearNodes();
}
void Controler::clearNodeGuis(){
    _gui->_nodeGraphTab->_nodeGraphArea->clear();
}
bool Controler::isSaveUpToDate() const{
    return _currentProject._age == _currentProject._lastAutoSave;
}
void Controler::resetCurrentProject(){
    _currentProject._hasProjectBeenSavedByUser = false;
    _currentProject._projectName = "Untitled.rs";
    _currentProject._projectPath = "";
    QString text("Powiter - ");
    text.append(_currentProject._projectName);
    _gui->setWindowTitle(text);
}
bool Controler::findAutoSave(){
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
                if(currentViewer)
                    currentViewer->getUiContext()->viewer->disconnectViewer();
                clearNodeGuis();
                clearInternalNodes();
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
const QString Controler::autoSavesDir() {return QString(QDir::tempPath() + QDir::separator() + "Powiter" + QDir::separator());}

void Controler::deselectAllNodes() const{
    _gui->_nodeGraphTab->_nodeGraphArea->deselect();
}

void Controler::showErrorDialog(const QString& title,const QString& message) const{
    _gui->errorDialog(title, message);

}