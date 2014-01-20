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
    QObject::connect(_imp->timeline.get(),SIGNAL(frameChanged(SequenceTime,int)),this,SLOT(onTimeChanged(SequenceTime,int)));
}

Project::~Project(){
    clearNodes();
}

void Project::initializeKnobs(){
    _imp->formatKnob = Natron::createKnob<Choice_Knob>(this, "Output Format");
    const std::vector<Format*>& appFormats = appPTR->getFormats();
    std::vector<std::string> entries;
    for (U32 i = 0; i < appFormats.size(); ++i) {
        Format* f = appFormats[i];
        QString formatStr = Natron::generateStringFromFormat(*f);
        if(f->width() == 1920 && f->height() == 1080){
            _imp->formatKnob->setValue(i);
        }
        entries.push_back(formatStr.toStdString());
        _imp->availableFormats.push_back(*f);
    }

    _imp->formatKnob->populate(entries);
    _imp->formatKnob->turnOffAnimation();
    _imp->addFormatKnob = Natron::createKnob<Button_Knob>(this,"New format...");


    _imp->viewsCount = Natron::createKnob<Int_Knob>(this,"Number of views");
    _imp->viewsCount->turnOffAnimation();
    _imp->viewsCount->setMinimum(1);
    _imp->viewsCount->setValue(1);
    _imp->viewsCount->disableSlider();
}


void Project::evaluate(Knob* /*knob*/,bool isSignificant){
    if(isSignificant){
        getApp()->checkViewersConnection();
    }
}


const Format& Project::getProjectDefaultFormat() const{
    int index = _imp->formatKnob->getActiveEntry();
    return _imp->availableFormats[index];
}

void Project::initNodeCountersAndSetName(Node* n){
    assert(n);
    std::map<std::string,int>::iterator it = _imp->nodeCounters.find(n->pluginID());
    if(it != _imp->nodeCounters.end()){
        it->second++;
        n->setName(QString(QString(n->pluginLabel().c_str())+ "_" + QString::number(it->second)).toStdString());
    }else{
        _imp->nodeCounters.insert(make_pair(n->pluginID(), 1));
        n->setName(QString(QString(n->pluginLabel().c_str())+ "_" + QString::number(1)).toStdString());
    }
    _imp->currentNodes.push_back(n);
}

void Project::clearNodes(){
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        if(_imp->currentNodes[i]->isOutputNode()){
            _imp->currentNodes[i]->quitAnyProcessing();
        }
    }
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        delete _imp->currentNodes[i];
    }
    _imp->currentNodes.clear();
}

void Project::setFrameRange(int first, int last){
    _imp->timeline->setFrameRange(first,last);
}


int Project::currentFrame() const {
    return _imp->timeline->currentFrame();
}

int Project::firstFrame() const {
    return _imp->timeline->firstFrame();
}

int Project::lastFrame() const {
    return _imp->timeline->lastFrame();
}



int Project::tryAddProjectFormat(const Format& f){
    getApp()->lockProject();
    
    if(f.left() >= f.right() || f.bottom() >= f.top()){
        getApp()->unlockProject();
        return -1;
    }
    for (U32 i = 0; i < _imp->availableFormats.size(); ++i) {
        if(f == _imp->availableFormats[i]){
            getApp()->unlockProject();
            return i;
        }
    }
    std::vector<Format> currentFormats = _imp->availableFormats;
    _imp->availableFormats.clear();
    std::vector<std::string> entries;
    for (U32 i = 0; i < currentFormats.size(); ++i) {
        const Format& f = currentFormats[i];
        QString formatStr = generateStringFromFormat(f);
        entries.push_back(formatStr.toStdString());
        _imp->availableFormats.push_back(f);
    }
    QString formatStr = generateStringFromFormat(f);
    entries.push_back(formatStr.toStdString());
    _imp->availableFormats.push_back(f);
    _imp->formatKnob->populate(entries);
    getApp()->unlockProject();
    return _imp->availableFormats.size() - 1;
}

void Project::setProjectDefaultFormat(const Format& f) {
    
    int index = tryAddProjectFormat(f);
    _imp->formatKnob->setValue(index);
    getApp()->notifyViewersProjectFormatChanged(f);
    getApp()->triggerAutoSave();
}



int Project::getProjectViewsCount() const{
    return _imp->viewsCount->getValue<int>();
}

const std::vector<Node*>& Project::getCurrentNodes() const{
    return _imp->currentNodes;
}

const QString& Project::getProjectName() const {return _imp->projectName;}

void Project::setProjectName(const QString& name) {_imp->projectName = name;}

const QString& Project::getLastAutoSaveFilePath() const {return _imp->lastAutoSaveFilePath;}

const QString& Project::getProjectPath() const {return _imp->projectPath;}

void Project::setProjectPath(const QString& path) {_imp->projectPath = path;}

bool Project::hasProjectBeenSavedByUser() const {
    return _imp->hasProjectBeenSavedByUser;
}

void Project::setHasProjectBeenSavedByUser(bool s) {
    _imp->hasProjectBeenSavedByUser = s;
}

const QDateTime& Project::projectAgeSinceLastSave() const {
    return _imp->ageSinceLastSave;
}

void Project::setProjectAgeSinceLastSave(const QDateTime& t) {
    _imp->ageSinceLastSave = t;
}

const QDateTime& Project::projectAgeSinceLastAutosave() const {
    return _imp->lastAutoSave;
}

void Project::setProjectAgeSinceLastAutosaveSave(const QDateTime& t) {
    _imp->lastAutoSave = t;
}

bool Project::shouldAutoSetProjectFormat() const {
    return _imp->autoSetProjectFormat;
}

void Project::setAutoSetProjectFormat(bool b){
    _imp->autoSetProjectFormat = b;
}

boost::shared_ptr<TimeLine> Project::getTimeLine() const  {return _imp->timeline;}


void Project::setProjectLastAutoSavePath(const QString& str){
    _imp->lastAutoSaveFilePath = str;
}

const std::vector<Format>& Project::getProjectFormats() const {
    return _imp->availableFormats;
}

void Project::incrementKnobsAge() {
    if(_imp->_knobsAge < 99999)
        ++_imp->_knobsAge;
    else
        _imp->_knobsAge = 0;
}

int Project::getKnobsAge() const {return _imp->_knobsAge;}

void Project::setLastTimelineSeekCaller(Natron::OutputEffectInstance* output){
    _imp->lastTimelineSeekCaller = output;
}

void Project::onTimeChanged(SequenceTime time,int reason){
    std::vector<ViewerInstance*> viewers;
    
    
    beginProjectWideValueChanges(Natron::TIME_CHANGED,this);
    
    refreshAfterTimeChange(time); //refresh project knobs
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        //refresh all knobs
        if(!_imp->currentNodes[i]->isActivated()){
            continue;
        }
        if(_imp->currentNodes[i]->pluginID() == "Viewer"){
            viewers.push_back(dynamic_cast<ViewerInstance*>(_imp->currentNodes[i]->getLiveInstance()));
        }
        _imp->currentNodes[i]->getLiveInstance()->refreshAfterTimeChange(time);

    }
    
    endProjectWideValueChanges(this);
    
    for(U32 i = 0; i < viewers.size();++i){
        if(viewers[i] != _imp->lastTimelineSeekCaller || reason == USER_SEEK){
            viewers[i]->refreshAndContinueRender();
        }
    }

}
void Project::save(ProjectSerialization* serializationObject) const {
    serializationObject->initialize(this);
}

void Project::load(const ProjectSerialization& obj){
    _imp->nodeCounters.clear();
    try{
        _imp->restoreFromSerialization(obj);
        emit formatChanged(getProjectDefaultFormat());
    }catch(const std::exception& e){
        throw e;
    }
}

void Project::beginProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller){
    
    
    ///increase the begin calls count
    ++_imp->beginEndBracketsCount;
    
    ///If begin was already called on this caller, increase the calls count for this caller, otherwise
    ///insert a new entry in the holdersWhoseBeginWasCalled map with a call count of 1 and the reason.
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);

    if(found == _imp->holdersWhoseBeginWasCalled.end()){
        
        caller->beginKnobsValuesChanged(reason);
        
        ///insert the caller in the map
        _imp->holdersWhoseBeginWasCalled.insert(std::make_pair(caller,std::make_pair(1,reason)));
    }else{
        
        ///increase the begin calls count of 1
        ++found->second.first;
    }


}

void Project::stackEvaluateRequest(Natron::ValueChangedReason reason,KnobHolder* caller,Knob* k,bool isSignificant){
    
    ///This function may be called outside of a begin/end bracket call, in which case we call them ourselves.
    
    bool wasBeginCalled = true;

    ///if begin was not called for this caller, call it ourselves
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
    if(found == _imp->holdersWhoseBeginWasCalled.end() || _imp->beginEndBracketsCount == 0){
        
        beginProjectWideValueChanges(reason,caller);
        
        ///flag that we called begin
        wasBeginCalled = false;
    } else {
        ///if we found a call made to begin already for this caller, adjust the reason to the reason of the
        /// outermost begin call
        reason = found->second.second;
    }

    ///if the evaluation is significant , set the flag isSignificantChange to true
    if(!_imp->isSignificantChange && isSignificant){
        _imp->isSignificantChange = true;
    }
    
    ///increase the count of evaluation requests
    ++_imp->evaluationsCount;
    
    ///remember the last caller, this is the one on which we will call evaluate
    _imp->lastKnobChanged = k;
   
    ///if the reason of the outermost begin call is OTHER_REASON then we don't call
    ///the onKnobValueChanged. This way the plugin can avoid infinite recursions by doing so:
    /// beginValueChange(OTHER_REASON)
    /// ...
    /// endValueChange()
    if(reason != Natron::OTHER_REASON) {
        caller->onKnobValueChanged(k,reason);
    }
    
    ////if begin was not call prior to calling this function, call the end bracket oruselves
    if(!wasBeginCalled){
        endProjectWideValueChanges(caller);
    }

}

void Project::endProjectWideValueChanges(KnobHolder* caller){
    
    
    ///decrease the beginEndBracket count
    --_imp->beginEndBracketsCount;
    
    ///Get the count of begin calls made to this caller.
    ///It must absolutely be present in holdersWhoseBeginWasCalled because we either added it by
    ///calling stackEvaluateRequest or beginProjectWideValueChanges
    ///This means that calling endProjectWideValueChanges twice in a row will crash in the assertion below.
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
    assert(found != _imp->holdersWhoseBeginWasCalled.end());
    
    ///If we're closing the last bracket, call the caller portion of endKnobsValuesChanged
    if(found->second.first == 1){
        caller->endKnobsValuesChanged(found->second.second);
        
        ///remove the caller from the holdersWhoseBeginWasCalled map

        _imp->holdersWhoseBeginWasCalled.erase(found);
        
    }else{
        ///we're not on the last begin/end bracket for this caller, just decrease the begin calls count
        --found->second.first;
    }
    
    ///if we're not in the last begin/end bracket (globally to all callers) then return
    if(_imp->beginEndBracketsCount != 0){
        return;
    }
    
    ///If we reach here that means that we're closing the last global bracket.
    ///If the bracket was empty of evaluation we don't have to do anything.
    if(_imp->evaluationsCount != 0){
        
        ///reset the evaluation count to 0
        _imp->evaluationsCount = 0;
        
        ///if the outermost bracket reason was USER_EDITED, trigger an auto-save.
        if(found->second.second == Natron::USER_EDITED && _imp->lastKnobChanged->typeName() != Button_Knob::typeNameStatic()){
            getApp()->triggerAutoSave();
        }
        
        ///if the outermost bracket reason was not OTHER_REASON or TIME_CHANGED, then call evaluate
        ///on the last caller with
        ///the significant param recorded in the stackEvaluateRequest function.
        if(found->second.second != Natron::OTHER_REASON && found->second.second != Natron::TIME_CHANGED){
            caller->evaluate(_imp->lastKnobChanged,_imp->isSignificantChange);
        }
    }
    
    ////reset back the isSignificantChange flag to false, otherwise next insignificant evaluations
    ////would be evaluated.
    _imp->isSignificantChange = false;
    
    ///the stack must be empty, i.e: crash if the user didn't correctly end the brackets
    assert(_imp->holdersWhoseBeginWasCalled.empty());
    
}

void Project::beginKnobsValuesChanged(Natron::ValueChangedReason /*reason*/){
    //beginProjectWideValueChanges(reason, this);
}

void Project::endKnobsValuesChanged(Natron::ValueChangedReason /*reason*/) {
    //endProjectWideValueChanges(reason,this);
}

void Project::onKnobValueChanged(Knob* knob,Natron::ValueChangedReason /*reason*/){
    if(knob == _imp->viewsCount.get()){
        int viewsCount = _imp->viewsCount->getValue<int>();
        getApp()->setupViewersForViews(viewsCount);
    }else if(knob == _imp->formatKnob.get()){
        int index = _imp->formatKnob->getActiveEntry();
        if(index < (int)_imp->availableFormats.size()){
            const Format& f = _imp->availableFormats[index];
            for(U32 i = 0 ; i < _imp->currentNodes.size() ; ++i){
                if (_imp->currentNodes[i]->pluginID() == "Viewer") {
                    emit formatChanged(f);
                }

            }
        }
    }else if(knob == _imp->addFormatKnob.get()){
        emit mustCreateFormat();
    }

}
    

} //namespace Natron
