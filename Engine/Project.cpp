//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Project.h"

#include <fstream>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>


#include "Global/AppManager.h"

#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/OfxNode.h"
#include "Engine/Knob.h"
#include "Engine/ViewerNode.h"

#include "Gui/NodeGui.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/Gui.h"

using namespace Powiter;
using std::cout; using std::endl;
using std::make_pair;


static QString generateStringFromFormat(const Format& f){
    QString formatStr;
    formatStr.append(f.getName().c_str());
    formatStr.append("  ");
    formatStr.append(QString::number(f.width()));
    formatStr.append(" x ");
    formatStr.append(QString::number(f.height()));
    formatStr.append("  ");
    formatStr.append(QString::number(f.getPixelAspect()));
    return formatStr;
}

Project::Project(AppInstance* appInstance):
_projectName("Untitled." POWITER_PROJECT_FILE_EXTENION),
_hasProjectBeenSavedByUser(false),
_ageSinceLastSave(QDateTime::currentDateTime()),
_timeline(new TimeLine())
,_autoSetProjectFormat(true)
,_projectDataLock()
,_currentNodes()
,_availableFormats()
,_appInstance(appInstance)
{
    QObject::connect(this,SIGNAL(projectViewsCountChanged(int)),_appInstance,SLOT(setupViewersForViews(int)));
    
    _formatKnob = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", NULL, "Output Format"));
    const std::vector<Format*>& appFormats = appPTR->getFormats();
    std::vector<std::string> entries;
    for (U32 i = 0; i < appFormats.size(); ++i) {
        Format* f = appFormats[i];
        QString formatStr = generateStringFromFormat(*f);
                if(f->width() == 1920 && f->height() == 1080){
            _formatKnob->setValue(i);
        }
        entries.push_back(formatStr.toStdString());
        _availableFormats.push_back(*f);
    }
    
    _formatKnob->populate(entries);
    QObject::connect(_formatKnob,SIGNAL(valueChangedByUser()),this,SLOT(onProjectFormatChanged()));
    _projectKnobs.push_back(_formatKnob);
    
    QObject::connect(_formatKnob, SIGNAL(knobUndoneChange()), _appInstance, SLOT(triggerAutoSave()));
    QObject::connect(_formatKnob, SIGNAL(knobRedoneChange()), _appInstance, SLOT(triggerAutoSave()));
    
    
    _addFormatKnob = dynamic_cast<Button_Knob*>(appPTR->getKnobFactory().createKnob("Button",NULL,"New format..."));
    _projectKnobs.push_back(_addFormatKnob);
    QObject::connect(_addFormatKnob, SIGNAL(valueChangedByUser()), this, SLOT(createNewFormat()));
    
    
    _viewsCount = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int",NULL,"Number of views"));
    _viewsCount->setMinimum(1);
    _viewsCount->setValue(1);
    QObject::connect(_viewsCount, SIGNAL(valueChangedByUser()),this,SLOT(onNumberOfViewsChanged()));
    QObject::connect(_viewsCount, SIGNAL(knobUndoneChange()), _appInstance, SLOT(triggerAutoSave()));
    QObject::connect(_viewsCount, SIGNAL(knobRedoneChange()), _appInstance, SLOT(triggerAutoSave()));
    _projectKnobs.push_back(_viewsCount);
}
Project::~Project(){
    for (U32 i = 0; i < _currentNodes.size(); ++i) {
        _currentNodes[i]->deleteNode();
    }
    _currentNodes.clear();
}

void Project::onNumberOfViewsChanged(){
    int viewsCount = _viewsCount->value<int>();
    emit projectViewsCountChanged(viewsCount);
}

void Project::onProjectFormatChanged(){
    const Format& f = _availableFormats[_formatKnob->getActiveEntry()];
    for(U32 i = 0 ; i < _currentNodes.size() ; ++i){
        if (_currentNodes[i]->className() == "Viewer") {
            ViewerNode* n = dynamic_cast<ViewerNode*>(_currentNodes[i]);
            n->getUiContext()->viewer->onProjectFormatChanged(f);
            n->refreshAndContinueRender();
        }
    }
}

const Format& Project::getProjectDefaultFormat() const{
    int index = _formatKnob->getActiveEntry();
    return _availableFormats[index];
}

void Project::initNodeCountersAndSetName(Node* n){
    assert(n);
    std::map<std::string,int>::iterator it = _nodeCounters.find(n->className());
    if(it != _nodeCounters.end()){
        it->second++;
        n->setName(QString(QString(n->className().c_str())+ "_" + QString::number(it->second)).toStdString());
    }else{
        _nodeCounters.insert(make_pair(n->className(), 1));
        n->setName(QString(QString(n->className().c_str())+ "_" + QString::number(1)).toStdString());
    }
    _currentNodes.push_back(n);
}

void Project::clearNodes(){
    foreach(Node* n,_currentNodes){
        n->deleteNode();
    }
    _currentNodes.clear();
}

void Project::setFrameRange(int first, int last){
    _timeline->setFrameRange(first,last);
}

void Project::seekFrame(int frame){
    _timeline->seekFrame(frame);
}

void Project::incrementCurrentFrame() {
    _timeline->incrementCurrentFrame();
}
void Project::decrementCurrentFrame(){
    _timeline->decrementCurrentFrame();
}

int Project::currentFrame() const {
    return _timeline->currentFrame();
}

int Project::firstFrame() const {
    return _timeline->firstFrame();
}

int Project::lastFrame() const {
    return _timeline->lastFrame();
}


void Project::loadProject(const QString& path,const QString& name){
    QString filePath = path+name;
    std::ifstream ifile(filePath.toStdString().c_str(),std::ifstream::in);
    boost::archive::xml_iarchive iArchive(ifile);
    std::list<NodeGui::SerializedState> nodeStates;
    iArchive >> boost::serialization::make_nvp("Nodes",nodeStates);
    iArchive >> boost::serialization::make_nvp("Project_formats",_availableFormats);
    
    /*we must restore the entries in the combobox before restoring the value*/
    std::vector<std::string> entries;
    for (U32 i = 0; i < _availableFormats.size(); ++i) {
        QString formatStr = generateStringFromFormat(_availableFormats[i]);
        entries.push_back(formatStr.toStdString());
    }
    _formatKnob->populate(entries);
    std::string currentFormat;
    iArchive >> boost::serialization::make_nvp("Project_output_format",currentFormat);
    setAutoSetProjectFormat(false);
    _formatKnob->restoreFromString(currentFormat);
    std::string viewsCount;
    iArchive >> boost::serialization::make_nvp("Project_views_count",viewsCount);
    _viewsCount->restoreFromString(viewsCount);
    ifile.close();
    
    /*first create all nodes and restore the knobs values*/
    for (std::list<NodeGui::SerializedState>::const_iterator it = nodeStates.begin() ; it!=nodeStates.end(); ++it) {
        const NodeGui::SerializedState& state = *it;
        Node* n = _appInstance->createNode(state.getClassName().c_str());
        if(!n){
            clearNodes();
            QString text("Failed to restore the graph! \n The node ");
            text.append(state.getClassName().c_str());
            text.append(" was found in the auto-save script but doesn't seem \n"
                        "to exist in the currently loaded plug-ins.");
            _appInstance->clearNodes();
            throw std::invalid_argument(text.toStdString());
        }
        n->setName(state.getName());
        const std::map<std::string,std::string>& knobsValues = state.getKnobsValues();
        for (std::map<std::string,std::string>::const_iterator v = knobsValues.begin(); v!=knobsValues.end(); ++v) {
            if(v->second.empty())
                continue;
            Knob* knob = n->getKnobByDescription(v->first);
            if(!knob){
                cout << "Couldn't restore knob value ( " << v->first << " )." << endl;
            }else{
                knob->restoreFromString(v->second);
            }
        }
        NodeGui* nGui = _appInstance->getNodeGui(n);
        nGui->setPos(state.getX(),state.getY());
        _appInstance->deselectAllNodes();
    }
    
    /*now that we have all nodes, just connect them*/
    for(std::list<NodeGui::SerializedState>::const_iterator it = nodeStates.begin() ; it!=nodeStates.end(); ++it){
        const std::map<int, std::string>& inputs = (*it).getInputs();
        Node* thisNode = NULL;
        for (U32 i = 0; i < _currentNodes.size(); ++i) {
            if (_currentNodes[i]->getName() == (*it).getName()) {
                thisNode = _currentNodes[i];
                break;
            }
        }
        for (std::map<int, std::string>::const_iterator input = inputs.begin(); input!=inputs.end(); ++input) {
            if(input->second.empty())
                continue;
            if(!_appInstance->connect(input->first, input->second,thisNode)){
                cout << "Failed to connect " << (*it).getName() << " to " << input->second << endl;
            }
        }
        
    }
    
    QDateTime time = QDateTime::currentDateTime();
    setAutoSetProjectFormat(false);
    setHasProjectBeenSavedByUser(true);
    setProjectName(name);
    setProjectPath(path);
    setProjectAgeSinceLastSave(time);
    setProjectAgeSinceLastAutosaveSave(time);
    
    /*Refresh all viewers as it was*/
    emit projectFormatChanged(_availableFormats[_formatKnob->getActiveEntry()]);
    _appInstance->checkViewersConnection();
}
void Project::saveProject(const QString& path,const QString& filename,bool autoSave){
    QString filePath;
    if(autoSave){
        filePath = AppInstance::autoSavesDir()+QDir::separator()+filename;
    }else{
        filePath = path+filename;
    }
    std::ofstream ofile(filePath.toStdString().c_str(),std::ofstream::out);
    boost::archive::xml_oarchive oArchive(ofile);
    if(!ofile.is_open()){
        cout << "Failed to open file " << filePath.toStdString() << endl;
        return;
    }
    std::list<NodeGui::SerializedState> nodeStates;
    const std::vector<NodeGui*>& activeNodes = _appInstance->getAllActiveNodes();
    for (U32 i = 0; i < activeNodes.size(); ++i) {
        NodeGui::SerializedState state = activeNodes[i]->serialize();
        nodeStates.push_back(state);
    }
    oArchive << boost::serialization::make_nvp("Nodes",nodeStates);
    oArchive << boost::serialization::make_nvp("Project_formats",_availableFormats);
    std::string currentFormat = _formatKnob->serialize();
    oArchive << boost::serialization::make_nvp("Project_output_format",currentFormat);
    std::string viewsCount = _viewsCount->serialize();
    oArchive << boost::serialization::make_nvp("Project_views_count",viewsCount);
    ofile.close();
}

 
const std::vector<Knob*>& Project::getProjectKnobs() const{
    
    return _projectKnobs;
}

int Project::tryAddProjectFormat(const Format& f){
    for (U32 i = 0; i < _availableFormats.size(); ++i) {
        if(f == _availableFormats[i]){
            return i;
        }
    }
    std::vector<Format> currentFormats = _availableFormats;
    _availableFormats.clear();
    std::vector<std::string> entries;
    for (U32 i = 0; i < currentFormats.size(); ++i) {
        const Format& f = currentFormats[i];
        QString formatStr = generateStringFromFormat(f);
        entries.push_back(formatStr.toStdString());
        _availableFormats.push_back(f);
    }
    QString formatStr = generateStringFromFormat(f);
    entries.push_back(formatStr.toStdString());
    _availableFormats.push_back(f);
    _formatKnob->populate(entries);
    return _availableFormats.size() - 1;
}

void Project::setProjectDefaultFormat(const Format& f) {
    
    int index = tryAddProjectFormat(f);
    _formatKnob->setValue(index);
    emit projectFormatChanged(f);
    _appInstance->triggerAutoSave();
}

 
void Project::createNewFormat(){
    AddFormatDialog dialog(_appInstance->getGui());
    if(dialog.exec()){
        tryAddProjectFormat(dialog.getFormat());
    }
}

int Project::getProjectViewsCount() const{
    return _viewsCount->value<int>();
}
