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

#include <QtConcurrentRun>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/ProjectPrivate.h"
#include "Engine/VideoEngine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/StandardPaths.h"

using std::cout; using std::endl;
using std::make_pair;


namespace Natron{
    

Project::Project(AppInstance* appInstance)
    : KnobHolder(appInstance)
    , _imp(new ProjectPrivate(this))
{
    QObject::connect(_imp->timeline.get(),SIGNAL(frameChanged(SequenceTime,int)),this,SLOT(onTimeChanged(SequenceTime,int)));
}

Project::~Project() {
    clearNodes();
    removeAutoSaves();
}

bool Project::loadProject(const QString& path,const QString& name){
    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        assert(!_imp->isLoadingProject);
        _imp->isLoadingProject = true;
    }

    reset();
    
    try {
        loadProjectInternal(path,name);
    } catch (const std::exception& e) {
        {
            QMutexLocker l(&_imp->isLoadingProjectMutex);
            _imp->isLoadingProject = false;
        }
        Natron::errorDialog("Project loader", std::string("Error while loading project") + ": " + e.what());
        if(!appPTR->isBackground()) {
            getApp()->createNode("Viewer");
        }
        return false;
    } catch (...) {
        {
            QMutexLocker l(&_imp->isLoadingProjectMutex);
            _imp->isLoadingProject = false;
        }
        Natron::errorDialog("Project loader", std::string("Unkown error while loading project"));
        if(!appPTR->isBackground()) {
            getApp()->createNode("Viewer");
        }
        return false;
    }
    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        _imp->isLoadingProject = false;
    }
    return true;
}

void Project::loadProjectInternal(const QString& path,const QString& name) {



    QString filePath = path+name;
    if(!QFile::exists(filePath)){
        throw std::invalid_argument(QString(filePath + " : no such file.").toStdString());
    }
    std::ifstream ifile;
    try{
        ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifile.open(filePath.toStdString().c_str(),std::ifstream::in);
    }catch(const std::ifstream::failure& e){
        throw std::runtime_error(std::string(std::string("Exception opening ")+ e.what() + filePath.toStdString()));
    }
    try{
        boost::archive::xml_iarchive iArchive(ifile);
        bool bgProject;
        iArchive >> boost::serialization::make_nvp("Background_project",bgProject);
        ProjectSerialization projectSerializationObj;
        iArchive >> boost::serialization::make_nvp("Project",projectSerializationObj);
        load(projectSerializationObj);
        if(!bgProject){
            getApp()->loadProjectGui(iArchive);
        }
    }catch(const boost::archive::archive_exception& e){
        throw std::runtime_error(std::string("Serialization error: ") + std::string(e.what()));
    }
    ifile.close();

    QDateTime time = QDateTime::currentDateTime();
    _imp->autoSetProjectFormat = false;
    _imp->hasProjectBeenSavedByUser = true;
    _imp->projectName = name;
    _imp->projectPath = path;
    _imp->ageSinceLastSave = time;
    _imp->lastAutoSave = time;
    emit projectNameChanged(name);

    /*Refresh all previews*/
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        if (_imp->currentNodes[i]->isPreviewEnabled()) {
            _imp->currentNodes[i]->computePreviewImage(_imp->timeline->currentFrame());
        }
    }

    /*Refresh all viewers as it was*/
    if(!appPTR->isBackground()){
        emit formatChanged(getProjectDefaultFormat());
        const std::vector<Node*>& nodes = getCurrentNodes();
        for (U32 i = 0; i < nodes.size(); ++i) {
            assert(nodes[i]);
            if (nodes[i]->pluginID() == "Viewer") {
                ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
                assert(n);
                n->getVideoEngine()->render(1, true,true,false,true,false,true);
            }
        }
    }

}

void Project::saveProject(const QString& path,const QString& name,bool autoSave){
    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        if(_imp->isLoadingProject){
            return;
        }
    }

    try {
        if (!autoSave) {
            if  (!isSaveUpToDate() || !QFile::exists(path+name)) {

                saveProjectInternal(path,name);
            }
        } else {
            if (!isGraphWorthLess()) {

                removeAutoSaves();
                saveProjectInternal(path,name,true);
            }
        }
    } catch (const std::exception& e) {
        if(!autoSave) {
            Natron::errorDialog("Save", e.what());
        } else {
            qDebug() << "Save failure: " << e.what();
        }
    }
}

void Project::saveProjectInternal(const QString& path,const QString& name,bool autoSave) {

    QDateTime time = QDateTime::currentDateTime();
    QString timeStr = time.toString();
    Hash64 timeHash;
    for(int i = 0 ; i < timeStr.size();++i) {
        timeHash.append<unsigned short>(timeStr.at(i).unicode());
    }
    timeHash.computeHash();
    QString timeHashStr = QString::number(timeHash.value());

    QString actualFileName = name;
    if(autoSave){
        QString pathCpy = path;

#ifdef __NATRON_WIN32__
        ///on windows, we must also modifiy the root name otherwise it would fail to save with a filename containing for example C:/
        QFileInfoList roots = QDir::drives();
        QString root;
        for (int i = 0; i < roots.size(); ++i) {
            QString rootPath = roots[i].absolutePath();
            rootPath = rootPath.remove(QChar('\\'));
            rootPath = rootPath.remove(QChar('/'));
            if (pathCpy.startsWith(rootPath)) {
                root = rootPath;
                QString rootToPrepend("_ROOT_");
                rootToPrepend.append(root.at(0)); //< append the root character, e.g the 'C' of C:
                rootToPrepend.append("_N_ROOT_");
                pathCpy.replace(rootPath, rootToPrepend);
                break;
            }
        }

#endif
        pathCpy = pathCpy.replace("/", "_SEP_");
        pathCpy = pathCpy.replace("\\", "_SEP_");
        actualFileName.prepend(pathCpy);
        actualFileName.append("."+timeHashStr);
    }
    QString filePath;
    if (autoSave) {
        filePath = Project::autoSavesDir() + QDir::separator() + actualFileName;
        _imp->lastAutoSaveFilePath = filePath;
    } else {
        filePath = path+actualFileName;
    }
    std::ofstream ofile(filePath.toStdString().c_str(),std::ofstream::out);
    if (!ofile.good()) {
        qDebug() << "Failed to open file " << filePath.toStdString().c_str();
        throw std::runtime_error("Failed to open file " + filePath.toStdString());
    }
    boost::archive::xml_oarchive oArchive(ofile);
    bool bgProject = appPTR->isBackground();
    oArchive << boost::serialization::make_nvp("Background_project",bgProject);
    ProjectSerialization projectSerializationObj;
    save(&projectSerializationObj);
    oArchive << boost::serialization::make_nvp("Project",projectSerializationObj);
    if(!bgProject){
        getApp()->saveProjectGui(oArchive);
    }

    _imp->projectName = name;
    if (!autoSave) {
        emit projectNameChanged(name); //< notify the gui so it can update the title
    } else {
        emit projectNameChanged(name + " (*)");
    }
    _imp->projectPath = path;
    if(!autoSave){
        _imp->hasProjectBeenSavedByUser = true;
        _imp->ageSinceLastSave = time;
    }
    _imp->lastAutoSave = time;

}

void Project::autoSave(){

    ///don't autosave in background mode...
    if (appPTR->isBackground()) {
        return;
    }

    saveProject(_imp->projectPath, _imp->projectName, true);
}

void Project::triggerAutoSave() {

    if (appPTR->isBackground()) {
        return;
    }
    QtConcurrent::run(this,&Project::autoSave);
}

bool Project::findAndTryLoadAutoSave() {
    QDir savesDir(autoSavesDir());
    QStringList entries = savesDir.entryList();
    for (int i = 0; i < entries.size();++i) {
        const QString& entry = entries.at(i);
        QString searchStr('.');
        searchStr.append(NATRON_PROJECT_FILE_EXT);
        searchStr.append('.');
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos != -1) {

            QString filename = entry.left(suffixPos+searchStr.size()-1);
            bool exists = false;

            if(!filename.contains(NATRON_PROJECT_UNTITLED)){
#ifdef __NATRON_WIN32__
                ///on windows we must extract the root of the filename (@see saveProjectInternal)
                int rootPos = filename.indexOf("_ROOT_");
                int endRootPos =  filename.indexOf("_N_ROOT_");
                QString rootName;
                if (rootPos != -1) {
                    assert(endRootPos != -1);//< if we found _ROOT_ then _N_ROOT must exist too
                    int startRootNamePos = rootPos + 6;
                    rootName = filename.mid(startRootNamePos,endRootPos - startRootNamePos);
                }
                filename.replace("_ROOT" + rootName + "_N_ROOT_",rootName + ':');
#endif
                filename = filename.replace("_SEP_",QDir::separator());
                exists = QFile::exists(filename);
            }

            QString text;

            if (exists) {
                text = QString(tr("A recent auto-save of %1 was found.\n"
                                  "Would you like to restore it entirely? "
                                  "Clicking No will remove this auto-save.")).arg(filename);;
            } else {
                text = tr("An auto-save was restored successfully. It didn't belong to any project\n"
                          "Would you like to restore it ? Clicking No will remove this auto-save forever.");
            }

            appPTR->hideSplashScreen();

            Natron::StandardButton ret = Natron::questionDialog(tr("Auto-save").toStdString(),
                                                                text.toStdString(),Natron::StandardButtons(Natron::Yes | Natron::No),
                                                                Natron::Yes);
            if (ret == Natron::No || ret == Natron::Escape) {
                removeAutoSaves();
                reset();
                return false;
            } else {
                {
                    QMutexLocker l(&_imp->isLoadingProjectMutex);
                    assert(!_imp->isLoadingProject);
                    _imp->isLoadingProject = true;
                }
                try {
                    loadProjectInternal(savesDir.path()+QDir::separator(), entry);
                } catch (const std::exception& e) {
                    Natron::errorDialog("Project loader", std::string("Error while loading auto-saved project") + ": " + e.what());
                    getApp()->createNode("Viewer");
                } catch (...) {
                    Natron::errorDialog("Project loader", std::string("Error while loading auto-saved project"));
                    getApp()->createNode("Viewer");
                }
                {
                    QMutexLocker l(&_imp->isLoadingProjectMutex);
                    _imp->isLoadingProject = false;
                }

                _imp->autoSetProjectFormat = false;

                if (exists) {
                    _imp->hasProjectBeenSavedByUser = true;
                    QString path = filename.left(filename.lastIndexOf(QDir::separator())+1);
                    filename = filename.remove(path);
                    _imp->projectName = filename;
                    _imp->projectPath = path;

                } else {
                    _imp->hasProjectBeenSavedByUser = false;
                    _imp->projectName = NATRON_PROJECT_UNTITLED;
                    _imp->projectPath.clear();
                }
                _imp->lastAutoSave = QDateTime::currentDateTime();
                _imp->ageSinceLastSave = QDateTime();

                emit projectNameChanged(_imp->projectName + " (*)");

                return true;
            }
        }
    }
    removeAutoSaves();
    reset();
    return false;
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
    
    _imp->previewMode = Natron::createKnob<Bool_Knob>(this, "Auto previews");
    _imp->previewMode->setHintToolTip("When true, preview images on the node graph will be"
                                      "refreshed automatically. You can uncheck this option to improve performances."
                                      "Press P in the node graph to refresh the previews yourself.");
    _imp->previewMode->turnOffAnimation();
    bool autoPreviewEnabled = appPTR->getCurrentSettings()->isAutoPreviewOnForNewProjects();
    _imp->previewMode->setValue<bool>(autoPreviewEnabled);
    
    emit knobsInitialized();
    
}


void Project::evaluate(Knob* /*knob*/,bool isSignificant){
    if(isSignificant){
        getApp()->checkViewersConnection();
    }
}


const Format& Project::getProjectDefaultFormat() const{
    QMutexLocker l(&_imp->formatMutex);
    int index = _imp->formatKnob->getActiveEntry();
    return _imp->availableFormats[index];
}

///only called on the main thread
void Project::initNodeCountersAndSetName(Node* n) {
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
    QMutexLocker l(&_imp->projectLock);

    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        _imp->currentNodes[i]->quitAnyProcessing();
    }
    
    
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        delete _imp->currentNodes[i];
    }
    _imp->currentNodes.clear();
    
    emit nodesCleared();
}

void Project::setFrameRange(int first, int last){
    QMutexLocker l(&_imp->timelineMutex);
    _imp->timeline->setFrameRange(first,last);
}


int Project::currentFrame() const {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->currentFrame();
}

int Project::firstFrame() const {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->firstFrame();
}

int Project::lastFrame() const {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->lastFrame();
}

int Project::leftBound() const  {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->leftBound();
}

int Project::rightBound() const  {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->rightBound();
}

int Project::tryAddProjectFormat(const Format& f){
    assert(!_imp->formatMutex.tryLock());
    if(f.left() >= f.right() || f.bottom() >= f.top()){
        return -1;
    }
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
    assert(!_imp->formatMutex.tryLock());
    int index = tryAddProjectFormat(f);
    {
        _imp->formatKnob->setValue(index);
    }
    emit formatChanged(f);
    getApp()->triggerAutoSave();
}



int Project::getProjectViewsCount() const {
    QMutexLocker l(&_imp->viewsCountMutex);
    return _imp->viewsCount->getValue<int>();
}

const std::vector<Node*> Project::getCurrentNodes() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->currentNodes;
}

const QString& Project::getProjectName() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->projectName;
}


const QString& Project::getLastAutoSaveFilePath() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->lastAutoSaveFilePath;
}

const QString& Project::getProjectPath() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->projectPath;
}


bool Project::hasProjectBeenSavedByUser() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->hasProjectBeenSavedByUser;
}

const QDateTime& Project::projectAgeSinceLastSave() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->ageSinceLastSave;
}


const QDateTime& Project::projectAgeSinceLastAutosave() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->lastAutoSave;
}

bool Project::isAutoPreviewEnabled() const {
    QMutexLocker l(&_imp->previewModeMutex);
    return _imp->previewMode->getValue<bool>();
}

void Project::toggleAutoPreview() {
    QMutexLocker l(&_imp->previewModeMutex);
    _imp->previewMode->setValue<bool>(!_imp->previewMode->getValue<bool>());
}

boost::shared_ptr<TimeLine> Project::getTimeLine() const  {return _imp->timeline;}


const std::vector<Format>& Project::getProjectFormats() const {
    QMutexLocker l(&_imp->formatMutex);
    return _imp->availableFormats;
}

void Project::incrementKnobsAge() {
    ///this is called by a setValue() on a knob which
    QMutexLocker l(&_imp->knobsAgeMutex);
    if(_imp->_knobsAge < 99999)
        ++_imp->_knobsAge;
    else
        _imp->_knobsAge = 0;
}

int Project::getKnobsAge() const {
    QMutexLocker l(&_imp->knobsAgeMutex);
    return _imp->_knobsAge;
}

void Project::setLastTimelineSeekCaller(Natron::OutputEffectInstance* output) {
    QMutexLocker l(&_imp->projectLock);
    _imp->lastTimelineSeekCaller = output;
}

bool Project::isSaveUpToDate() const{
    QMutexLocker l(&_imp->projectLock);
    return _imp->ageSinceLastSave == _imp->lastAutoSave;
}

///this function is only called in the main thread
void Project::onTimeChanged(SequenceTime time,int reason) {
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
            viewers[i]->getVideoEngine()->render(1, false,false,false,true,false);
        }
    }
    _imp->lastTimelineSeekCaller = 0;

}
void Project::save(ProjectSerialization* serializationObject) const {
    serializationObject->initialize(this);
}
    
void Project::load(const ProjectSerialization& obj){
    _imp->nodeCounters.clear();
    _imp->restoreFromSerialization(obj);
    emit formatChanged(getProjectDefaultFormat());
}
    
void Project::beginProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller){
    QMutexLocker l(&_imp->beginEndMutex);
    
    ///increase the begin calls count
    ++_imp->beginEndBracketsCount;
    
    ///If begin was already called on this caller, increase the calls count for this caller, otherwise
    ///insert a new entry in the holdersWhoseBeginWasCalled map with a call count of 1 and the reason.
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);

    if(found == _imp->holdersWhoseBeginWasCalled.end()){
        
        //getApp()->unlockProject();
        caller->beginKnobsValuesChanged(reason);
        //getApp()->lockProject();
        
        ///insert the caller in the map
        _imp->holdersWhoseBeginWasCalled.insert(std::make_pair(caller,std::make_pair(1,reason)));
    }else{
        
        ///increase the begin calls count of 1
        ++found->second.first;
    }


}

void Project::stackEvaluateRequest(Natron::ValueChangedReason reason,KnobHolder* caller,Knob* k,bool isSignificant){
    QMutexLocker l(&_imp->beginEndMutex);

    ///This function may be called outside of a begin/end bracket call, in which case we call them ourselves.
    
    bool wasBeginCalled = true;

    ///if begin was not called for this caller, call it ourselves
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
    if(found == _imp->holdersWhoseBeginWasCalled.end() || _imp->beginEndBracketsCount == 0){
        
        beginProjectWideValueChanges(reason,caller);
        
        ///flag that we called begin
        wasBeginCalled = false;
    } else {
        ///THIS IS WRONG AND COMMENTED OUT : THIS LEADS TO INFINITE RECURSION.
        ///if we found a call made to begin already for this caller, adjust the reason to the reason of the
        /// outermost begin call
        //   reason = found->second.second;
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
    QMutexLocker l(&_imp->beginEndMutex);

    
    ///decrease the beginEndBracket count
    --_imp->beginEndBracketsCount;
    
    ///Get the count of begin calls made to this caller.
    ///It must absolutely be present in holdersWhoseBeginWasCalled because we either added it by
    ///calling stackEvaluateRequest or beginProjectWideValueChanges
    ///This means that calling endProjectWideValueChanges twice in a row will crash in the assertion below.
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
    assert(found != _imp->holdersWhoseBeginWasCalled.end());
    
    Natron::ValueChangedReason outerMostReason = found->second.second;
    
    ///If we're closing the last bracket, call the caller portion of endKnobsValuesChanged
    if(found->second.first == 1){
        
        ///remove the caller from the holdersWhoseBeginWasCalled map
        _imp->holdersWhoseBeginWasCalled.erase(found);
        
        caller->endKnobsValuesChanged(outerMostReason);

        
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
        if(outerMostReason == Natron::USER_EDITED && _imp->lastKnobChanged->typeName() != Button_Knob::typeNameStatic()){
            getApp()->triggerAutoSave();
        }
        
        ///if the outermost bracket reason was not OTHER_REASON or TIME_CHANGED, then call evaluate
        ///on the last caller with
        ///the significant param recorded in the stackEvaluateRequest function.
        if(outerMostReason != Natron::OTHER_REASON && outerMostReason != Natron::TIME_CHANGED){
            
            caller->evaluate(_imp->lastKnobChanged,_imp->isSignificantChange);
            
        }
    }
    
    ////reset back the isSignificantChange flag to false, otherwise next insignificant evaluations
    ////would be evaluated.
    _imp->isSignificantChange = false;
    
    ///the stack must be empty, i.e: crash if the user didn't correctly end the brackets
    assert(_imp->holdersWhoseBeginWasCalled.empty());
    
}

void Project::beginKnobsValuesChanged(Natron::ValueChangedReason /*reason*/){}

void Project::endKnobsValuesChanged(Natron::ValueChangedReason /*reason*/) {}


    
///this function is only called on the main thread
void Project::onKnobValueChanged(Knob* knob,Natron::ValueChangedReason /*reason*/) {
    if (knob == _imp->viewsCount.get()) {
        int viewsCount = _imp->viewsCount->getValue<int>();
        getApp()->setupViewersForViews(viewsCount);
    } else if(knob == _imp->formatKnob.get()) {
        int index = _imp->formatKnob->getActiveEntry();
        if(index < (int)_imp->availableFormats.size()){
            const Format& f = _imp->availableFormats[index];
            for(U32 i = 0 ; i < _imp->currentNodes.size() ; ++i){
                if (_imp->currentNodes[i]->pluginID() == "Viewer") {
                    emit formatChanged(f);
                }

            }
        }
    } else if(knob == _imp->addFormatKnob.get()) {
        emit mustCreateFormat();
    } else if(knob == _imp->previewMode.get()) {
        emit autoPreviewChanged(_imp->previewMode->getValue<bool>());
    } 
    
}


bool Project::isLoadingProject() const {
    QMutexLocker l(&_imp->isLoadingProjectMutex);
    return _imp->isLoadingProject;
}

bool Project::isGraphWorthLess() const{
    bool worthLess = true;
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        if (!_imp->currentNodes[i]->isOutputNode()) {
            worthLess = false;
            break;
        }
    }
    return worthLess;
}

void Project::removeAutoSaves() const {
    /*removing all autosave files*/
    QDir savesDir(autoSavesDir());
    QStringList entries = savesDir.entryList();
    for(int i = 0; i < entries.size();++i) {
        const QString& entry = entries.at(i);
        QString searchStr('.');
        searchStr.append(NATRON_PROJECT_FILE_EXT);
        searchStr.append('.');
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos != -1) {
            QFile::remove(savesDir.path()+QDir::separator()+entry);
        }
    }
}

QString Project::autoSavesDir() {
    return Natron::StandardPaths::writableLocation(Natron::StandardPaths::DataLocation) + QDir::separator() + "Autosaves";
}

void Project::reset(){
    _imp->autoSetProjectFormat = true;
    _imp->hasProjectBeenSavedByUser = false;
    _imp->projectName = NATRON_PROJECT_UNTITLED;
    _imp->projectPath.clear();
    emit projectNameChanged(_imp->projectName);
    clearNodes();
}

void Project::setOrAddProjectFormat(const Format& frmt,bool skipAdd) {

    QMutexLocker l(&_imp->formatMutex);
    if(_imp->autoSetProjectFormat){
        Format dispW;
        _imp->autoSetProjectFormat = false;
        dispW = frmt;

        Format* df = appPTR->findExistingFormat(dispW.width(), dispW.height(),dispW.getPixelAspect());
        if (df) {
            dispW.setName(df->getName());
            setProjectDefaultFormat(dispW);
        } else {
            setProjectDefaultFormat(dispW);
        }

    } else if(!skipAdd) {
        Format dispW;
        dispW = frmt;
        tryAddProjectFormat(dispW);

    }
}

///do not need to lock this function as all calls are thread-safe already
bool Project::connectNodes(int inputNumber,const std::string& parentName,Node* output){
    const std::vector<Node*> nodes = getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->getName() == parentName) {
            return connectNodes(inputNumber,nodes[i], output);
        }
    }
    return false;
}

bool Project::connectNodes(int inputNumber,Node* input,Node* output,bool force) {

    Node* existingInput = output->input(inputNumber);
    if (force && existingInput) {
        bool ok = disconnectNodes(existingInput, output);
        assert(ok);
        if (!input->isInputNode()) {
            ok = connectNodes(input->getPreferredInputForConnection(), existingInput, input);
            assert(ok);
        }
    }

    QMutexLocker l(&_imp->projectLock);

    if(!output->connectInput(input, inputNumber)){
        return false;
    }
    if(!input){
        return true;
    }
    input->connectOutput(output);
    return true;
}
bool Project::disconnectNodes(Node* input,Node* output,bool autoReconnect) {
    QMutexLocker l(&_imp->projectLock);

    Node* inputToReconnectTo = 0;
    int indexOfInput = output->inputIndex(input);
    if (indexOfInput == -1) {
        return false;
    }

    int inputsCount = input->maximumInputs();
    if (inputsCount == 1) {
        inputToReconnectTo = input->input(0);
    }

    if(input->disconnectOutput(output) < 0){
        return false;
    }
    if(output->disconnectInput(input) < 0){
        return false;
    }

    if (autoReconnect && inputToReconnectTo) {
        bool ok = connectNodes(indexOfInput, inputToReconnectTo, output);
        assert(ok);
    }

    return true;
}

bool Project::tryLock() const {
    return _imp->projectLock.tryLock();
}

void Project::unlock() const {
    assert(!_imp->projectLock.tryLock());
    _imp->projectLock.unlock();
}

bool Project::autoConnectNodes(Node* selected,Node* created) {
    ///We follow this rule:
    //        1) selected is output
    //          a) created is output --> fail
    //          b) created is input --> connect input
    //          c) created is regular --> connect input
    //        2) selected is input
    //          a) created is output --> connect output
    //          b) created is input --> fail
    //          c) created is regular --> connect output
    //        3) selected is regular
    //          a) created is output--> connect output
    //          b) created is input --> connect input
    //          c) created is regular --> connect output

    ///if true if will connect 'created' as input of 'selected',
    ///otherwise as output.
    bool connectAsInput = false;

    ///cannot connect 2 input nodes together: case 2-b)
    if (selected->isInputNode() && created->isInputNode()) {
        return false;
    }
    ///cannot connect 2 output nodes together: case 1-a)
    if (selected->isOutputNode() && created->isOutputNode()) {
        return false;
    }

    ///1)
    if (selected->isOutputNode()) {

        ///assert we're not in 1-a)
        assert(!created->isOutputNode());

        ///for either cases 1-b) or 1-c) we just connect the created node as input of the selected node.
        connectAsInput = true;
    }
    ///2) and 3) are similar exceptfor case b)
    else {

        ///case 2 or 3- a): connect the created node as output of the selected node.
        if (created->isOutputNode()) {
            connectAsInput = false;
        }
        ///case b)
        else if (created->isInputNode()) {
            if (selected->isInputNode()) {
                ///assert we're not in 2-b)
                assert(!created->isInputNode());
            } else {
                ///case 3-b): connect the created node as input of the selected node
                connectAsInput = true;
            }
        }
        ///case c) connect created as output of the selected node
        else {
            connectAsInput = false;
        }
    }

    bool ret = false;
    if (connectAsInput) {
        ///if the selected node is and inspector, we want to connect the created node on the active input
        InspectorNode* inspector = dynamic_cast<InspectorNode*>(selected);
        if (inspector) {
            int activeInputIndex = inspector->activeInput();
            bool ok = connectNodes(activeInputIndex, created, selected,true);
            assert(ok);
            ret = true;
        } else {
            ///connect it to the first input
            int selectedInput = selected->getPreferredInputForConnection();
            if (selectedInput != -1) {
                bool ok = connectNodes(selectedInput, created, selected,true);
                assert(ok);
                ret = true;
            } else {
                ret = false;
            }
        }
    } else {

        if (!created->isOutputNode()) {
            ///we find all the nodes that were previously connected to the selected node,
            ///and connect them to the created node instead.
            std::map<Node*,int> outputsConnectedToSelectedNode;
            selected->getOutputsConnectedToThisNode(&outputsConnectedToSelectedNode);
            for (std::map<Node*,int>::iterator it = outputsConnectedToSelectedNode.begin(); it!=outputsConnectedToSelectedNode.end(); ++it) {
                bool ok = disconnectNodes(selected, it->first);
                assert(ok);

                ok = connectNodes(it->second, created, it->first);
                assert(ok);
            }

        }
        ///finally we connect the created node to the selected node
        int createdInput = created->getPreferredInputForConnection();
        if (createdInput != -1) {
            bool ok = connectNodes(createdInput, selected, created);
            assert(ok);
            ret = true;
        } else {
            ret = false;
        }
    }

    ///update the render trees
    std::list<ViewerInstance*> viewers;
    created->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->updateTreeAndRender();
    }
    return ret;

}

} //namespace Natron
