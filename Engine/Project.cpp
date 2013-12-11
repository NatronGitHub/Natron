//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Project.h"


#include "Global/AppManager.h"

#include "Engine/ProjectPrivate.h"
#include "Engine/TimeLine.h"
#include "Engine/VideoEngine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ProjectSerialization.h"

using std::cout; using std::endl;
using std::make_pair;

namespace Natron{

Project::Project(AppInstance* appInstance)
    : KnobHolder(appInstance)
    , _imp(new ProjectPrivate(this))
{
    QObject::connect(_imp->_timeline.get(),SIGNAL(frameChanged(SequenceTime,int)),this,SLOT(onTimeChanged(SequenceTime,int)));
}

Project::~Project(){
    QMutexLocker locker(&_imp->_projectDataLock);
    for (U32 i = 0; i < _imp->_currentNodes.size(); ++i) {
        if(_imp->_currentNodes[i]->isOutputNode()){
            dynamic_cast<OutputEffectInstance*>(_imp->_currentNodes[i]->getLiveInstance())->getVideoEngine()->quitEngineThread();
        }
    }
    for (U32 i = 0; i < _imp->_currentNodes.size(); ++i) {
        delete _imp->_currentNodes[i];
    }
    _imp->_currentNodes.clear();
}

void Project::initializeKnobs(){
    _imp->_formatKnob = appPTR->getKnobFactory().createKnob<Choice_Knob>(this, "Output Format");
    const std::vector<Format*>& appFormats = appPTR->getFormats();
    std::vector<std::string> entries;
    for (U32 i = 0; i < appFormats.size(); ++i) {
        Format* f = appFormats[i];
        QString formatStr = Natron::generateStringFromFormat(*f);
        if(f->width() == 1920 && f->height() == 1080){
            _imp->_formatKnob->setValue(i);
        }
        entries.push_back(formatStr.toStdString());
        _imp->_availableFormats.push_back(*f);
    }
    
    _imp->_formatKnob->populate(entries);
    _imp->_formatKnob->turnOffAnimation();
    _imp->_addFormatKnob = appPTR->getKnobFactory().createKnob<Button_Knob>(this,"New format...");
    

    _imp->_viewsCount = appPTR->getKnobFactory().createKnob<Int_Knob>(this,"Number of views");
    _imp->_viewsCount->turnOffAnimation();
    _imp->_viewsCount->setMinimum(1);
    _imp->_viewsCount->setValue(1);
    _imp->_viewsCount->disableSlider();
}


void Project::evaluate(Knob* /*knob*/,bool isSignificant){
    if(isSignificant){
        getApp()->checkViewersConnection();
    }
}


const Format& Project::getProjectDefaultFormat() const{
    int index = _imp->_formatKnob->getActiveEntry();
    return _imp->_availableFormats[index];
}

void Project::initNodeCountersAndSetName(Node* n){
    assert(n);
    std::map<std::string,int>::iterator it = _imp->_nodeCounters.find(n->pluginID());
    if(it != _imp->_nodeCounters.end()){
        it->second++;
        n->setName(QString(QString(n->pluginLabel().c_str())+ "_" + QString::number(it->second)).toStdString());
    }else{
        _imp->_nodeCounters.insert(make_pair(n->pluginID(), 1));
        n->setName(QString(QString(n->pluginLabel().c_str())+ "_" + QString::number(1)).toStdString());
    }
    _imp->_currentNodes.push_back(n);
}

void Project::clearNodes(){
    foreach(Node* n,_imp->_currentNodes){
        delete n;
    }
    _imp->_currentNodes.clear();
}

void Project::setFrameRange(int first, int last){
    _imp->_timeline->setFrameRange(first,last);
}


int Project::currentFrame() const {
    return _imp->_timeline->currentFrame();
}

int Project::firstFrame() const {
    return _imp->_timeline->firstFrame();
}

int Project::lastFrame() const {
    return _imp->_timeline->lastFrame();
}



int Project::tryAddProjectFormat(const Format& f){
    for (U32 i = 0; i < _imp->_availableFormats.size(); ++i) {
        if(f == _imp->_availableFormats[i]){
            return i;
        }
    }
    std::vector<Format> currentFormats = _imp->_availableFormats;
    _imp->_availableFormats.clear();
    std::vector<std::string> entries;
    for (U32 i = 0; i < currentFormats.size(); ++i) {
        const Format& f = currentFormats[i];
        QString formatStr = generateStringFromFormat(f);
        entries.push_back(formatStr.toStdString());
        _imp->_availableFormats.push_back(f);
    }
    QString formatStr = generateStringFromFormat(f);
    entries.push_back(formatStr.toStdString());
    _imp->_availableFormats.push_back(f);
    _imp->_formatKnob->populate(entries);
    return _imp->_availableFormats.size() - 1;
}

void Project::setProjectDefaultFormat(const Format& f) {
    
    int index = tryAddProjectFormat(f);
    _imp->_formatKnob->setValue(index);
    getApp()->notifyViewersProjectFormatChanged(f);
    getApp()->triggerAutoSave();
}



int Project::getProjectViewsCount() const{
    return _imp->_viewsCount->getValue<int>();
}

const std::vector<Node*>& Project::getCurrentNodes() const{return _imp->_currentNodes;}

const QString& Project::getProjectName() const {return _imp->_projectName;}

void Project::setProjectName(const QString& name) {_imp->_projectName = name;}

const QString& Project::getLastAutoSaveFilePath() const {return _imp->_lastAutoSaveFilePath;}

const QString& Project::getProjectPath() const {return _imp->_projectPath;}

void Project::setProjectPath(const QString& path) {_imp->_projectPath = path;}

bool Project::hasProjectBeenSavedByUser() const {return _imp->_hasProjectBeenSavedByUser;}

void Project::setHasProjectBeenSavedByUser(bool s) {_imp->_hasProjectBeenSavedByUser = s;}

const QDateTime& Project::projectAgeSinceLastSave() const {return _imp->_ageSinceLastSave;}

void Project::setProjectAgeSinceLastSave(const QDateTime& t) {_imp->_ageSinceLastSave = t;}

const QDateTime& Project::projectAgeSinceLastAutosave() const {return _imp->_lastAutoSave;}

void Project::setProjectAgeSinceLastAutosaveSave(const QDateTime& t) {_imp->_lastAutoSave = t;}

bool Project::shouldAutoSetProjectFormat() const {return _imp->_autoSetProjectFormat;}

void Project::setAutoSetProjectFormat(bool b){_imp->_autoSetProjectFormat = b;}

boost::shared_ptr<TimeLine> Project::getTimeLine() const  {return _imp->_timeline;}

void  Project::lock() const {_imp->_projectDataLock.lock();}

void  Project::unlock() const { assert(!_imp->_projectDataLock.tryLock());_imp->_projectDataLock.unlock();}

void Project::setProjectLastAutoSavePath(const QString& str){ _imp->_lastAutoSaveFilePath = str; }

const std::vector<Format>& Project::getProjectFormats() const {return _imp->_availableFormats;}

void Project::incrementKnobsAge() {
    if(_imp->_knobsAge < 99999)
        ++_imp->_knobsAge;
    else
        _imp->_knobsAge = 0;
}

int Project::getKnobsAge() const {return _imp->_knobsAge;}

void Project::setLastTimelineSeekCaller(Natron::OutputEffectInstance* output){
    _imp->_lastTimelineSeekCaller = output;
}

void Project::onTimeChanged(SequenceTime time,int /*reason*/){
    std::vector<ViewerInstance*> viewers;
    beginProjectWideValueChanges(Natron::TIME_CHANGED,this);
    refreshAfterTimeChange(time); //refresh project knobs
    for (U32 i = 0; i < _imp->_currentNodes.size(); ++i) {     
        //refresh all knobs
        if(_imp->_currentNodes[i]->pluginID() == "Viewer"){
            viewers.push_back(dynamic_cast<ViewerInstance*>(_imp->_currentNodes[i]->getLiveInstance()));
        }
        _imp->_currentNodes[i]->getLiveInstance()->refreshAfterTimeChange(time);
    }
    endProjectWideValueChanges(Natron::TIME_CHANGED,this);

    for(U32 i = 0; i < viewers.size();++i){
        if(viewers[i] != _imp->_lastTimelineSeekCaller){
            viewers[i]->refreshAndContinueRender();
        }
    }

}
void Project::save(ProjectSerialization* serializationObject) const {
    serializationObject->initialize(this);
}

void Project::load(const ProjectSerialization& obj){
    try{
        _imp->restoreFromSerialization(obj);
        emit formatChanged(getProjectDefaultFormat());
    }catch(const std::exception& e){
        throw e;
    }
}

void Project::beginProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller){
    //std::cout <<"Begin: " << _imp->_beginEndBracketsCount << std::endl;
    ++_imp->_beginEndBracketsCount;
    
    std::map<KnobHolder*,int>::iterator found = _imp->_holdersWhoseBeginWasCalled.find(caller);
    if(found == _imp->_holdersWhoseBeginWasCalled.end()){
        caller->beginKnobsValuesChanged(reason);
        _imp->_holdersWhoseBeginWasCalled.insert(std::make_pair(caller, 1));
    }else{
        ++found->second;
    }
}

void Project::stackEvaluateRequest(Natron::ValueChangedReason reason,KnobHolder* caller,Knob* k,bool isSignificant){
    bool wasBeginCalled = true;

    std::map<KnobHolder*,int>::iterator found = _imp->_holdersWhoseBeginWasCalled.find(caller);
    if(found == _imp->_holdersWhoseBeginWasCalled.end() || _imp->_beginEndBracketsCount == 0){
        beginProjectWideValueChanges(reason,caller);
        wasBeginCalled = false;
    }

    if(!_imp->_isSignificantChange && isSignificant){
        _imp->_isSignificantChange = true;
    }
    ++_imp->_evaluationsCount;
    caller->onKnobValueChanged(k,reason);
    if(!wasBeginCalled){
        endProjectWideValueChanges(reason,caller);
    }
}

void Project::endProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller){
    --_imp->_beginEndBracketsCount;
    //   std::cout <<"End: " << _imp->_beginEndBracketsCount << std::endl;
    std::map<KnobHolder*,int>::iterator found = _imp->_holdersWhoseBeginWasCalled.find(caller);
    assert(found != _imp->_holdersWhoseBeginWasCalled.end());
    if(found->second == 1){
        caller->endKnobsValuesChanged(reason);
        _imp->_holdersWhoseBeginWasCalled.erase(found);
    }else{
        --found->second;
    }
    if(_imp->_beginEndBracketsCount != 0){
        return;
    }
    if(_imp->_evaluationsCount != 0){
        _imp->_evaluationsCount = 0;
        if(reason == Natron::USER_EDITED){
            getApp()->triggerAutoSave();
        }
        if(reason != Natron::OTHER_REASON && reason != Natron::TIME_CHANGED){
            caller->evaluate(NULL,_imp->_isSignificantChange);
        }
    }
}

void Project::beginKnobsValuesChanged(Natron::ValueChangedReason /*reason*/){
    //beginProjectWideValueChanges(reason, this);
}

void Project::endKnobsValuesChanged(Natron::ValueChangedReason /*reason*/) {
    //endProjectWideValueChanges(reason,this);
}

void Project::onKnobValueChanged(Knob* knob,Natron::ValueChangedReason /*reason*/){
    if(knob == _imp->_viewsCount){
        int viewsCount = _imp->_viewsCount->getValue<int>();
        getApp()->setupViewersForViews(viewsCount);
    }else if(knob == _imp->_formatKnob){
        const Format& f = _imp->_availableFormats[_imp->_formatKnob->getActiveEntry()];
        for(U32 i = 0 ; i < _imp->_currentNodes.size() ; ++i){
            if (_imp->_currentNodes[i]->pluginID() == "Viewer") {
                emit formatChanged(f);
            }
        }
    }else if(knob == _imp->_addFormatKnob){
        emit mustCreateFormat();
    }

}
} //namespace Natron
