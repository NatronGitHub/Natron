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


#include <fstream>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include <QMutex>
#include <QString>
#include <QDateTime>

#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/archive_exception.hpp>

#include "Global/AppManager.h"

#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Format.h"
#include "Engine/ViewerInstance.h"
#include "Engine/VideoEngine.h"
#include "Engine/CurveSerialization.h"
#include "Engine/KnobTypes.h"

#include "Gui/NodeGui.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/Gui.h"

using std::cout; using std::endl;
using std::make_pair;

namespace Natron{


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

struct ProjectPrivate{
    QString _projectName;
    QString _projectPath;
    QString _lastAutoSaveFilePath;
    bool _hasProjectBeenSavedByUser;
    QDateTime _ageSinceLastSave;
    QDateTime _lastAutoSave;
    ComboBox_Knob* _formatKnob;
    Button_Knob* _addFormatKnob;
    Int_Knob* _viewsCount;
    boost::shared_ptr<TimeLine> _timeline; // global timeline
    
    std::map<std::string,int> _nodeCounters;
    bool _autoSetProjectFormat;
    mutable QMutex _projectDataLock;
    std::vector<Node*> _currentNodes;
    
    std::vector<Format> _availableFormats;
    
    int _knobsAge; //< the age of the knobs in the app. This is updated on each value changed.
    Natron::OutputEffectInstance* _lastTimelineSeekCaller;
    
    ProjectPrivate(Project* project)
        : _projectName("Untitled." NATRON_PROJECT_FILE_EXT)
        , _hasProjectBeenSavedByUser(false)
        , _ageSinceLastSave(QDateTime::currentDateTime())
        , _formatKnob(NULL)
        , _addFormatKnob(NULL)
        , _viewsCount(NULL)
        , _timeline(new TimeLine(project))
        , _autoSetProjectFormat(true)
        , _projectDataLock()
        , _currentNodes()
        , _availableFormats()
        , _knobsAge(0)
        , _lastTimelineSeekCaller(NULL)
    {
        
    }
};

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
    _imp->_formatKnob = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", this, "Output Format"));
    const std::vector<Format*>& appFormats = appPTR->getFormats();
    std::vector<std::string> entries;
    for (U32 i = 0; i < appFormats.size(); ++i) {
        Format* f = appFormats[i];
        QString formatStr = generateStringFromFormat(*f);
        if(f->width() == 1920 && f->height() == 1080){
            _imp->_formatKnob->setValue(i);
        }
        entries.push_back(formatStr.toStdString());
        _imp->_availableFormats.push_back(*f);
    }
    
    _imp->_formatKnob->populate(entries);
    
    _imp->_addFormatKnob = dynamic_cast<Button_Knob*>(appPTR->getKnobFactory().createKnob("Button",this,"New format..."));
    
    
    _imp->_viewsCount = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int",this,"Number of views"));
    _imp->_viewsCount->turnOffAnimation();
    _imp->_viewsCount->setMinimum(1);
    _imp->_viewsCount->setValue(1);
    _imp->_viewsCount->disableSlider();
}


void Project::evaluate(Knob* knob,bool /*isSignificant*/){
    if(knob == _imp->_viewsCount){
        int viewsCount = _imp->_viewsCount->getValue<int>();
        getApp()->setupViewersForViews(viewsCount);
    }else if(knob == _imp->_formatKnob){
        const Format& f = _imp->_availableFormats[_imp->_formatKnob->getActiveEntry()];
        for(U32 i = 0 ; i < _imp->_currentNodes.size() ; ++i){
            if (_imp->_currentNodes[i]->pluginID() == "Viewer") {
                ViewerInstance* n = dynamic_cast<ViewerInstance*>(_imp->_currentNodes[i]->getLiveInstance());
                assert(n);
                n->getUiContext()->viewer->onProjectFormatChanged(f);
                n->refreshAndContinueRender();
            }
        }
    }else if(knob == _imp->_addFormatKnob){
        createNewFormat();
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


void Project::loadProject(const QString& path,const QString& name,bool background){
    QString filePath = path+name;
    if(!QFile::exists(filePath)){
        throw std::invalid_argument(QString(filePath + " : no such file.").toStdString());
    }
    std::list<NodeGui::SerializedState> nodeStates;
    std::ifstream ifile;
    try{
        ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifile.open(filePath.toStdString().c_str(),std::ifstream::in);
    }catch(const std::ifstream::failure& e){
        throw std::runtime_error(std::string(std::string("Exception opening ")+ e.what() + filePath.toStdString()));
    }
    try{
        boost::archive::xml_iarchive iArchive(ifile);
        iArchive >> boost::serialization::make_nvp("Nodes",nodeStates);
        iArchive >> boost::serialization::make_nvp("Project_formats",_imp->_availableFormats);
        /*we must restore the entries in the combobox before restoring the value*/
        std::vector<std::string> entries;
        for (U32 i = 0; i < _imp->_availableFormats.size(); ++i) {
            QString formatStr = generateStringFromFormat(_imp->_availableFormats[i]);
            entries.push_back(formatStr.toStdString());
        }
        _imp->_formatKnob->populate(entries);
        
        AnimatingParam formatValue(_imp->_formatKnob->getDimension());
        AnimatingParam viewsValue(_imp->_viewsCount->getDimension());
        iArchive >> boost::serialization::make_nvp("Project_output_format",formatValue);
        _imp->_formatKnob->onStartupRestoration(formatValue);
        setAutoSetProjectFormat(false);
        iArchive >> boost::serialization::make_nvp("Project_views_count",viewsValue);
        _imp->_viewsCount->onStartupRestoration(viewsValue);
        
        SequenceTime leftBound,rightBound,current;
        iArchive >> boost::serialization::make_nvp("Timeline_current_time",current);
        iArchive >> boost::serialization::make_nvp("Timeline_left_bound",leftBound);
        iArchive >> boost::serialization::make_nvp("Timeline_right_bound",rightBound);
        _imp->_timeline->setBoundaries(leftBound, rightBound);
        _imp->_timeline->seekFrame(current,NULL);
    }catch(const boost::archive::archive_exception& e){
        throw std::runtime_error(std::string("Serialization error: ") + std::string(e.what()));
    }
    ifile.close();
    
    bool hasProjectAWriter = false;
    /*first create all nodes and restore the knobs values*/
    for (std::list<NodeGui::SerializedState>::const_iterator it = nodeStates.begin() ; it!=nodeStates.end(); ++it) {
        const NodeGui::SerializedState& state = *it;
        
        if (background && state.getPluginID() == "Viewer") { //if the node is a viewer, don't try to load it in background mode
            continue;
        }
        
        Node* n = getApp()->createNode(state.getPluginID().c_str(),true);
        if(!n){
            Natron::errorDialog("Loading failed", "Cannot load node " + state.getPluginID());
            continue;
        }
        n->getLiveInstance()->beginValuesChanged(AnimatingParam::PLUGIN_EDITED,true);
        if(n->pluginID() == "Writer" || (n->isOpenFXNode() && n->isOutputNode())){
            hasProjectAWriter = true;
        }
        
        if(!n){
            clearNodes();
            QString text("Failed to restore the graph! \n The node ");
            text.append(state.getPluginID().c_str());
            text.append(" was found in the auto-save script but doesn't seem \n"
                        "to exist in the currently loaded plug-ins.");
            throw std::invalid_argument(text.toStdString());
        }
        n->setName(state.getPluginLabel());
        const NodeGui::SerializedState::KnobValues& knobsValues = state.getKnobsValues();
        //begin changes to params
        for (NodeGui::SerializedState::KnobValues::const_iterator i = knobsValues.begin();
             i != knobsValues.end();++i) {
            boost::shared_ptr<Knob> knob = n->getKnobByDescription(i->first);
            if (!knob) {
                std::string message = std::string("Couldn't find knob ") + i->first;
                qDebug() << message.c_str();
                throw std::runtime_error(message);
            }
            knob->onStartupRestoration(i->second);
        }
        if(!background){
            NodeGui* nGui = getApp()->getNodeGui(n);
            assert(nGui);
            nGui->setPos(state.getX(),state.getY());
            getApp()->deselectAllNodes();
            if(state.isPreviewEnabled() && !n->isPreviewEnabled()){
                nGui->togglePreview();
            }
        }

        n->getLiveInstance()->endValuesChanged(AnimatingParam::PLUGIN_EDITED);
    }
    
    if(!hasProjectAWriter && background){
        clearNodes();
        throw std::invalid_argument("Project file is missing a writer node. This project cannot render anything.");
    }
    
    /*now that we have all nodes, just connect them*/
    for(std::list<NodeGui::SerializedState>::const_iterator it = nodeStates.begin() ; it!=nodeStates.end(); ++it){
        
        if(background && (*it).getPluginID() == "Viewer")//ignore viewers on background mode
            continue;
        
        const std::map<int, std::string>& inputs = (*it).getInputs();
        Node* thisNode = NULL;
        for (U32 i = 0; i < _imp->_currentNodes.size(); ++i) {
            if (_imp->_currentNodes[i]->getName() == (*it).getPluginLabel()) {
                thisNode = _imp->_currentNodes[i];
                break;
            }
        }
        for (std::map<int, std::string>::const_iterator input = inputs.begin(); input!=inputs.end(); ++input) {
            if(input->second.empty())
                continue;
            if(!getApp()->connect(input->first, input->second,thisNode)) {
                std::string message = std::string("Failed to connect node ") + (*it).getPluginLabel() + " to " + input->second;
                qDebug() << message.c_str();
                throw std::runtime_error(message);
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
    if(!background){
        getApp()->notifyViewersProjectFormatChanged(_imp->_availableFormats[_imp->_formatKnob->getActiveEntry()]);
        getApp()->checkViewersConnection();
    }
}
void Project::saveProject(const QString& path,const QString& filename,bool autoSave)
{
    QString filePath;
    if (autoSave) {
        filePath = AppInstance::autoSavesDir()+QDir::separator()+filename;

        _imp->_lastAutoSaveFilePath = filePath;
    } else {
        filePath = path+filename;
    }
    std::ofstream ofile(filePath.toStdString().c_str(),std::ofstream::out);
    if (!ofile.good()) {
        qDebug() << "Failed to open file " << filePath.toStdString().c_str();
        throw std::runtime_error("Failed to open file " + filePath.toStdString());
    }
    try {
        boost::archive::xml_oarchive oArchive(ofile);
        std::list<NodeGui::SerializedState> nodeStates;
        const std::vector<NodeGui*>& activeNodes = getApp()->getAllActiveNodes();
        for (U32 i = 0; i < activeNodes.size(); ++i) {
            NodeGui::SerializedState state = activeNodes[i]->serialize();
            nodeStates.push_back(state);
        }
        oArchive << boost::serialization::make_nvp("Nodes",nodeStates);
        oArchive << boost::serialization::make_nvp("Project_formats",_imp->_availableFormats);
        oArchive << boost::serialization::make_nvp("Project_output_format",dynamic_cast<const AnimatingParam&>(*_imp->_formatKnob));
        oArchive << boost::serialization::make_nvp("Project_views_count",dynamic_cast<const AnimatingParam&>(*_imp->_viewsCount));
        SequenceTime leftBound,rightBound,current;
        leftBound = _imp->_timeline->leftBound();
        rightBound = _imp->_timeline->rightBound();
        current = _imp->_timeline->currentFrame();
        oArchive << boost::serialization::make_nvp("Timeline_current_time",current);
        oArchive << boost::serialization::make_nvp("Timeline_left_bound",leftBound);
        oArchive << boost::serialization::make_nvp("Timeline_right_bound",rightBound);
    }catch (const std::exception& e) {
        qDebug() << "Error while saving project" << ": " << e.what();
        throw;
    } catch (...) {
        qDebug() << "Error while saving project";
        throw;
    }
    
}
    // ofstream destructor closes the file




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


void Project::createNewFormat(){
    AddFormatDialog dialog(this,getApp()->getGui());
    if(dialog.exec()){
        tryAddProjectFormat(dialog.getFormat());
    }
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
    
    void Project::onTimeChanged(SequenceTime time,int reason){
        std::list<ViewerInstance*> viewers;
        
        refreshAfterTimeChange(time);
        
        for (U32 i = 0; i < _imp->_currentNodes.size(); ++i) {
        if(_imp->_currentNodes[i]->pluginID() == "Viewer"){
            viewers.push_back(dynamic_cast<ViewerInstance*>(_imp->_currentNodes[i]->getLiveInstance()));
        }
        _imp->_currentNodes[i]->getLiveInstance()->refreshAfterTimeChange(time);
    }
        //Notify all knobs that the current time changed.
        //It lets a chance for an animated knob to change its value according to the current time
        for (std::list<ViewerInstance*>::const_iterator it = viewers.begin(); it != viewers.end() ;++it)  {
            if((Natron::TIMELINE_CHANGE_REASON)reason == Natron::PLAYBACK_SEEK && (*it) == _imp->_lastTimelineSeekCaller){
                continue;
            }
            (*it)->refreshAndContinueRender();
        }
}

} //namespace Natron
