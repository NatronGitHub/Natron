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
        QMutexLocker locker(&_imp->projectDataLock);
		for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
			if(_imp->currentNodes[i]->isOutputNode()){
				dynamic_cast<OutputEffectInstance*>(_imp->currentNodes[i]->getLiveInstance())->getVideoEngine()->quitEngineThread();
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
		for (U32 i = 0; i < _imp->availableFormats.size(); ++i) {
			if(f == _imp->availableFormats[i]){
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

	const std::vector<Node*>& Project::getCurrentNodes() const{return _imp->currentNodes;}

	const QString& Project::getProjectName() const {return _imp->projectName;}

	void Project::setProjectName(const QString& name) {_imp->projectName = name;}

	const QString& Project::getLastAutoSaveFilePath() const {return _imp->lastAutoSaveFilePath;}

	const QString& Project::getProjectPath() const {return _imp->projectPath;}

	void Project::setProjectPath(const QString& path) {_imp->projectPath = path;}

	bool Project::hasProjectBeenSavedByUser() const {return _imp->hasProjectBeenSavedByUser;}

	void Project::setHasProjectBeenSavedByUser(bool s) {_imp->hasProjectBeenSavedByUser = s;}

	const QDateTime& Project::projectAgeSinceLastSave() const {return _imp->ageSinceLastSave;}

	void Project::setProjectAgeSinceLastSave(const QDateTime& t) {_imp->ageSinceLastSave = t;}

	const QDateTime& Project::projectAgeSinceLastAutosave() const {return _imp->lastAutoSave;}

	void Project::setProjectAgeSinceLastAutosaveSave(const QDateTime& t) {_imp->lastAutoSave = t;}

	bool Project::shouldAutoSetProjectFormat() const {return _imp->autoSetProjectFormat;}

	void Project::setAutoSetProjectFormat(bool b){_imp->autoSetProjectFormat = b;}

	boost::shared_ptr<TimeLine> Project::getTimeLine() const  {return _imp->timeline;}

	void  Project::lock() const {_imp->projectDataLock.lock();}

	void  Project::unlock() const { assert(!_imp->projectDataLock.tryLock());_imp->projectDataLock.unlock();}

	void Project::setProjectLastAutoSavePath(const QString& str){ _imp->lastAutoSaveFilePath = str; }

	const std::vector<Format>& Project::getProjectFormats() const {return _imp->availableFormats;}

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

	void Project::onTimeChanged(SequenceTime time,int /*reason*/){
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
		endProjectWideValueChanges(Natron::TIME_CHANGED,this);

		for(U32 i = 0; i < viewers.size();++i){
			if(viewers[i] != _imp->lastTimelineSeekCaller){
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
		//std::cout <<"Begin: " << _imp->_beginEndBracketsCount << std::endl;
		++_imp->beginEndBracketsCount;

		std::map<KnobHolder*,int>::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
		if(found == _imp->holdersWhoseBeginWasCalled.end()){
			caller->beginKnobsValuesChanged(reason);
			_imp->holdersWhoseBeginWasCalled.insert(std::make_pair(caller, 1));
		}else{
			++found->second;
		}
	}

	void Project::stackEvaluateRequest(Natron::ValueChangedReason reason,KnobHolder* caller,Knob* k,bool isSignificant){
		bool wasBeginCalled = true;

		std::map<KnobHolder*,int>::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
		if(found == _imp->holdersWhoseBeginWasCalled.end() || _imp->beginEndBracketsCount == 0){
			beginProjectWideValueChanges(reason,caller);
			wasBeginCalled = false;
		}

		if(!_imp->isSignificantChange && isSignificant){
			_imp->isSignificantChange = true;
		}
		++_imp->evaluationsCount;
		_imp->lastKnobChanged = k;
		caller->onKnobValueChanged(k,reason);
		if(!wasBeginCalled){
			endProjectWideValueChanges(reason,caller);
		}
	}

	void Project::endProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller){
		--_imp->beginEndBracketsCount;
		//   std::cout <<"End: " << _imp->_beginEndBracketsCount << std::endl;
		std::map<KnobHolder*,int>::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
		assert(found != _imp->holdersWhoseBeginWasCalled.end());
		if(found->second == 1){
			caller->endKnobsValuesChanged(reason);
			_imp->holdersWhoseBeginWasCalled.erase(found);
		}else{
			--found->second;
		}
		if(_imp->beginEndBracketsCount != 0){
			return;
		}
		if(_imp->evaluationsCount != 0){
			_imp->evaluationsCount = 0;
			if(reason == Natron::USER_EDITED){
				getApp()->triggerAutoSave();
			}
			if(reason != Natron::OTHER_REASON && reason != Natron::TIME_CHANGED){
				caller->evaluate(_imp->lastKnobChanged,_imp->isSignificantChange);
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
