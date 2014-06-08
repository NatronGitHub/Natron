//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Node.h"

#include <limits>

#include <QtCore/QDebug>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>

#include <boost/bind.hpp>

#include "Engine/Hash64.h"
#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/Knob.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Engine/Project.h"
#include "Engine/EffectInstance.h"
#include "Engine/Log.h"
#include "Engine/NodeSerialization.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/KnobTypes.h"
#include "Engine/ImageParams.h"
#include "Engine/RotoContext.h"

using namespace Natron;
using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;

namespace { // protect local classes in anonymous namespace

/*The output node was connected from inputNumber to this...*/
typedef std::map<boost::shared_ptr<Node> ,int > DeactivatedState;

}

struct Node::Implementation {
    Implementation(AppInstance* app_,LibraryBinary* plugin_)
        : app(app_)
        , outputsMutex()
        , outputs()
        , outputsQueue()
        , inputsMutex()
        , inputs()
        , inputsQueue()
        , liveInstance(0)
        , inputsComponents()
        , outputComponents()
        , inputLabels()
        , name()
        , deactivatedState()
        , activatedMutex()
        , activated(true)
        , plugin(plugin_)
        , computingPreview(false)
        , computingPreviewMutex()
        , computingPreviewCond()
        , pluginInstanceMemoryUsed(0)
        , memoryUsedMutex()
        , mustQuitProcessing(false)
        , mustQuitProcessingMutex()
        , mustQuitProcessingCond()
        , perFrameMutexesLock()
        , renderInstancesFullySafePerFrameMutexes()
        , knobsAge(0)
        , knobsAgeMutex()
        , masterNodeMutex()
        , masterNode()
        , enableMaskKnob()
        , maskChannelKnob()
        , invertMaskKnob()
        , nodeSettingsPage()
        , previewEnabledKnob()
        , disableNodeKnob()
        , rotoContext()
        , imagesBeingRenderedMutex()
        , imagesBeingRendered()
    {
    }
    

    AppInstance* app; // pointer to the app: needed to access the application's default-project's format
    
    mutable QMutex outputsMutex;
    std::list<boost::shared_ptr<Node> > outputs; //< written to by the render thread once before rendering a frame
    std::list<boost::shared_ptr<Node> > outputsQueue; //< Written to by the GUI only.
    
    mutable QMutex inputsMutex; //< protects inputsMutex
    std::vector<boost::shared_ptr<Node> > inputs; //< Written to by the render thread once before rendering a frame.
    std::vector<boost::shared_ptr<Node> > inputsQueue; //< This is written to by the GUI only. Then the render thread copies this queue
                                    //to the inputs in a thread-safe manner.

    Natron::EffectInstance*  liveInstance; //< the effect hosted by this node

    ///These two are also protected by inputsMutex
    std::vector< std::list<Natron::ImageComponents> > inputsComponents;
    std::list<Natron::ImageComponents> outputComponents;
    
    mutable QMutex nameMutex;
    std::vector<std::string> inputLabels; // inputs name
    std::string name; //node name set by the user

    DeactivatedState deactivatedState;
    mutable QMutex activatedMutex;
    bool activated;
    
    LibraryBinary* plugin; //< the plugin which stores the function to instantiate the effect
    
    bool computingPreview;
    mutable QMutex computingPreviewMutex;
    QWaitCondition computingPreviewCond;
    
    size_t pluginInstanceMemoryUsed; //< global count on all EffectInstance's of the memory they use.
    QMutex memoryUsedMutex; //< protects _pluginInstanceMemoryUsed
    
    bool mustQuitProcessing;
    QMutex mustQuitProcessingMutex;
    QWaitCondition mustQuitProcessingCond;
    
    QMutex renderInstancesSharedMutex; //< see INSTANCE_SAFE in EffectInstance::renderRoI
                                       //only 1 clone can render at any time
    
    QMutex perFrameMutexesLock; //< protects renderInstancesFullySafePerFrameMutexes
    std::map<int,boost::shared_ptr<QMutex> > renderInstancesFullySafePerFrameMutexes; //< see FULLY_SAFE in EffectInstance::renderRoI
                                                                   //only 1 render per frame
    
    U64 knobsAge; //< the age of the knobs in this effect. It gets incremented every times the liveInstance has its evaluate() function called.
    mutable QReadWriteLock knobsAgeMutex; //< protects knobsAge and hash
    Hash64 hash; //< recomputed everytime knobsAge is changed.
    
    mutable QMutex masterNodeMutex;
    boost::shared_ptr<Node> masterNode;
  
    ///For each mask, the input number and the knob
    std::map<int,boost::shared_ptr<Bool_Knob> > enableMaskKnob;
    std::map<int,boost::shared_ptr<Choice_Knob> > maskChannelKnob;
    std::map<int,boost::shared_ptr<Bool_Knob> > invertMaskKnob;
    
    boost::shared_ptr<Page_Knob> nodeSettingsPage;
    boost::shared_ptr<Bool_Knob> previewEnabledKnob;
    boost::shared_ptr<Bool_Knob> disableNodeKnob;
    
    boost::shared_ptr<RotoContext> rotoContext; //< valid when the node has a rotoscoping context (i.e: paint context)
    
    mutable QMutex imagesBeingRenderedMutex;
    QWaitCondition imageBeingRenderedCond;
    std::list< boost::shared_ptr<Image> > imagesBeingRendered; ///< a list of all the images being rendered simultaneously
};

/**
 *@brief Actually converting to ARGB... but it is called BGRA by
 the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int
toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;

unsigned int
toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

Node::Node(AppInstance* app,LibraryBinary* plugin)
    : QObject()
    , _imp(new Implementation(app,plugin))
{
    QObject::connect(this, SIGNAL(pluginMemoryUsageChanged(qint64)), appPTR, SLOT(onNodeMemoryRegistered(qint64)));
}

void Node::createRotoContextConditionnally()
{
    assert(!_imp->rotoContext);
    assert(_imp->liveInstance);
    ///Initialize the roto context if any
    if (isRotoNode()) {
        _imp->rotoContext.reset(new RotoContext(this));
    }
}

void Node::load(const std::string& pluginID,const boost::shared_ptr<Natron::Node>& thisShared,
                const NodeSerialization& serialization,bool dontLoadName) {
    
    ///Called from the main thread. MT-safe
    assert(QThread::currentThread() == qApp->thread());
    
    ///cannot load twice
    assert(!_imp->liveInstance);
    
    bool nameSet = false;
    if (!serialization.isNull() && !dontLoadName) {
        setName(serialization.getPluginLabel());
        nameSet = true;
    }
    
    std::pair<bool,EffectBuilder> func = _imp->plugin->findFunction<EffectBuilder>("BuildEffect");
    if (func.first) {
        _imp->liveInstance = func.second(thisShared);
        createRotoContextConditionnally();
    } else { //ofx plugin
        _imp->liveInstance = appPTR->createOFXEffect(pluginID,thisShared,&serialization);
        _imp->liveInstance->initializeOverlayInteract();
    }
    
    initializeInputs();
    initializeKnobs(serialization);
    
    if (!nameSet) {
         getApp()->getProject()->initNodeCountersAndSetName(this);
    }
    
    
    computeHash(); 
    assert(_imp->liveInstance);
}

U64 Node::getHashValue() const {
    QReadLocker l(&_imp->knobsAgeMutex);
    return _imp->hash.value();
}

void Node::computeHash() {
    
    ///Always called in the main thread
    assert(QThread::currentThread() == qApp->thread());
    
    {
        QWriteLocker l(&_imp->knobsAgeMutex);
        
        ///reset the hash value
        _imp->hash.reset();
        
        ///append the effect's own age
        _imp->hash.append(_imp->knobsAge);
        
        ///append all inputs hash
        {
            ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->liveInstance);
            QMutexLocker l(&_imp->inputsMutex);
            if (isViewer) {
                int activeInput[2];
                isViewer->getActiveInputs(activeInput[0], activeInput[1]);
                for (int i = 0; i < 2; ++i) {
                    if (activeInput[i] >= 0 && _imp->inputsQueue[i]) {
                        _imp->hash.append(_imp->inputsQueue[i]->getHashValue());
                    }
                }
            } else {
                for (U32 i = 0; i < _imp->inputsQueue.size();++i) {
                    if (_imp->inputsQueue[i]) {
                        _imp->hash.append(_imp->inputsQueue[i]->getHashValue());
                    }
                }
            }
        }
        
        ///Also append the effect's label to distinguish 2 instances with the same parameters
        ::Hash64_appendQString(&_imp->hash, QString(getName().c_str()));
        
        
        ///Also append the project's creation time in the hash because 2 projects openend concurrently
        ///could reproduce the same (especially simple graphs like Viewer-Reader)
        _imp->hash.append(getApp()->getProject()->getProjectCreationTime());
        
        _imp->hash.computeHash();
    }
    
    ///call it on all the outputs
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputsQueue.begin(); it != _imp->outputsQueue.end(); ++it) {
        assert(*it);
        (*it)->computeHash();
    }
}

void Node::loadKnobs(const NodeSerialization& serialization) {
    
    ///Only called from the main thread
    assert(QThread::currentThread() == qApp->thread());
    
    const std::vector< boost::shared_ptr<KnobI> >& nodeKnobs = getKnobs();
    ///for all knobs of the node
    for (U32 j = 0; j < nodeKnobs.size();++j) {
        loadKnob(nodeKnobs[j], serialization);
    }
    ///now restore the roto context if the node has a roto context
    if (serialization.hasRotoContext() && _imp->rotoContext) {
        _imp->rotoContext->load(serialization.getRotoContext());
    }
    
    setKnobsAge(serialization.getKnobsAge());
}

void Node::loadKnob(const boost::shared_ptr<KnobI>& knob,const NodeSerialization& serialization)
{
    const NodeSerialization::KnobValues& knobsValues = serialization.getKnobsValues();
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it!=knobsValues.end();++it) {
        if((*it)->getName() == knob->getName()){
            // don't load the value if the Knob is not persistant! (it is just the default value in this case)
            if (knob->getIsPersistant()) {
                knob->clone((*it)->getKnob());
            }
            break;
        }
    }
}

void Node::restoreKnobsLinks(const NodeSerialization& serialization,const std::vector<boost::shared_ptr<Natron::Node> >& allNodes) {
    
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    const NodeSerialization::KnobValues& knobsValues = serialization.getKnobsValues();
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it!=knobsValues.end();++it) {
        (*it)->restoreKnobLinks(allNodes);
    }
    
}

void Node::setKnobsAge(U64 newAge)  {
    
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QWriteLocker l(&_imp->knobsAgeMutex);
    if (_imp->knobsAge != newAge) {
        _imp->knobsAge = newAge;
        emit knobsAgeChanged(_imp->knobsAge);
        l.unlock();
        computeHash();
        l.relock();
    }
}

void Node::incrementKnobsAge() {
    
    U32 newAge;
    {
        QWriteLocker l(&_imp->knobsAgeMutex);
        ++_imp->knobsAge;
        
        ///if the age of an effect somehow reaches the maximum age (will never happen)
        ///handle it by clearing the cache and resetting the age to 0.
        if (_imp->knobsAge == std::numeric_limits<U64>::max()) {
            appPTR->clearAllCaches();
            _imp->knobsAge = 0;
        }
        newAge = _imp->knobsAge;
    }
    emit knobsAgeChanged(newAge);
    computeHash();
}

U64 Node::getKnobsAge() const {
    QReadLocker l(&_imp->knobsAgeMutex);
    return _imp->knobsAge;
}

bool Node::isRenderingPreview() const {
    QMutexLocker l(&_imp->computingPreviewMutex);
    return _imp->computingPreview;
}

void Node::quitAnyProcessing() {
    if (isOutputNode()) {
        dynamic_cast<Natron::OutputEffectInstance*>(this->getLiveInstance())->getVideoEngine()->quitEngineThread();
    }
//    {
//        QMutexLocker locker(&_imp->imageBeingRenderedMutex);
//        _imp->imagesBeingRenderedNotEmpty.wakeAll();
//    }
    bool computingPreview;
    {
        QMutexLocker locker(&_imp->computingPreviewMutex);
        computingPreview = _imp->computingPreview;
    }
    if (computingPreview) {
        QMutexLocker l(&_imp->mustQuitProcessingMutex);
        _imp->mustQuitProcessing = true;
        while (_imp->mustQuitProcessing) {
            _imp->mustQuitProcessingCond.wait(&_imp->mustQuitProcessingMutex);
        }
    }
    
    
}

Node::~Node()
{
    if (_imp->liveInstance) {
        delete _imp->liveInstance;
    }
}

void Node::removeReferences()
{
    delete _imp->liveInstance;
    _imp->liveInstance = 0;
}

const std::vector<std::string>& Node::getInputLabels() const
{
    ///MT-safe as it never changes.
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->inputLabels;
}

const std::list<boost::shared_ptr<Natron::Node> >& Node::getOutputs() const
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->outputsQueue;
}

std::vector<std::string> Node::getInputNames() const
{
    std::vector<std::string> ret;
    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputsQueue.size(); ++i) {
        if (_imp->inputsQueue[i]) {
            ret.push_back(_imp->inputsQueue[i]->getName_mt_safe());
        } else {
            ret.push_back("");
        }
    }
    return ret;
}

int Node::getPreferredInputForConnection() const {
    
    if (maximumInputs() == 0) {
        return -1;
    }
    
    ///we return the first non-optional empty input
    int firstNonOptionalEmptyInput = -1;
    std::list<int> optionalEmptyInputs;
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputs.size() ; ++i) {
            if (!_imp->inputs[i]) {
                if (!_imp->liveInstance->isInputOptional(i)) {
                    if (firstNonOptionalEmptyInput == -1) {
                        firstNonOptionalEmptyInput = i;
                        break;
                    }
                } else {
                    optionalEmptyInputs.push_back(i);
                }
            }
        }
    }
    
    if (firstNonOptionalEmptyInput != -1) {
        return firstNonOptionalEmptyInput;
    }  else {
        if (!optionalEmptyInputs.empty()) {
            std::list<int>::iterator  first = optionalEmptyInputs.begin();
            while (first != optionalEmptyInputs.end() && _imp->liveInstance->isInputRotoBrush(*first)) {
                ++first;
            }
            if (first == optionalEmptyInputs.end()) {
                return -1;
            } else {
                return *first;
            }
        } else {
            return -1;
        }
    }
}

void Node::getOutputsConnectedToThisNode(std::map<boost::shared_ptr<Node>,int>* outputs) {
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputsQueue.begin(); it != _imp->outputsQueue.end(); ++it) {
        assert(*it);
        int indexOfThis = (*it)->inputIndex(this);
        assert(indexOfThis != -1);
        outputs->insert(std::make_pair(*it, indexOfThis));
    }
}

const std::string& Node::getName() const
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->nameMutex);
    return _imp->name;
}

std::string Node::getName_mt_safe() const {
    QMutexLocker l(&_imp->nameMutex);
    return _imp->name;
}

void Node::setName(const std::string& name)
{
    {
        QMutexLocker l(&_imp->nameMutex);
        _imp->name = name;
    }

    emit nameChanged(name.c_str());
}

AppInstance* Node::getApp() const
{
    return _imp->app;
}

bool Node::isActivated() const
{
    QMutexLocker l(&_imp->activatedMutex);
    return _imp->activated;
}

void Node::onGUINameChanged(const QString& str) {
    
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    QMutexLocker l(&_imp->nameMutex);
    _imp->name = str.toStdString();
}

void Node::initializeKnobs(const NodeSerialization& serialization) {
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    _imp->liveInstance->initializeKnobsPublic();
    
    ///If the effect has a mask, add additionnal mask controls
    int inputsCount = maximumInputs();
    for (int i = 0; i < inputsCount; ++i) {
        if (_imp->liveInstance->isInputMask(i) && !_imp->liveInstance->isInputRotoBrush(i)) {
            std::string maskName = _imp->liveInstance->inputLabel(i);
            boost::shared_ptr<Bool_Knob> enableMaskKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance, maskName,1,false);
            _imp->enableMaskKnob.insert(std::make_pair(i,enableMaskKnob));
            enableMaskKnob->setDefaultValue(false, 0);
            enableMaskKnob->turnOffNewLine();
            std::string enableMaskName("enable_mask_natron_" + maskName);
            enableMaskKnob->setName(enableMaskName);
            enableMaskKnob->setAnimationEnabled(false);
            enableMaskKnob->setHintToolTip("Enable the mask to come from the channel named by the choice parameter on the right. "
                                                 "Turning this off will act as though the mask was disconnected.");
            
            boost::shared_ptr<Choice_Knob> maskChannelKnob = Natron::createKnob<Choice_Knob>(_imp->liveInstance, "",1,false);
            _imp->maskChannelKnob.insert(std::make_pair(i,maskChannelKnob));
            std::vector<std::string> choices;
            choices.push_back("None");
            choices.push_back("Red");
            choices.push_back("Green");
            choices.push_back("Blue");
            choices.push_back("Alpha");
            maskChannelKnob->populateChoices(choices);
            maskChannelKnob->setDefaultValue(4, 0);
            maskChannelKnob->setAnimationEnabled(false);
            maskChannelKnob->turnOffNewLine();
            maskChannelKnob->setHintToolTip("Use this channel from the original input to mix the output with the original input. "
                                                  "Setting this to None is the same as disabling the mask.");
            std::string channelMaskName("mask_channel_natron_" + maskName);
            maskChannelKnob->setName(channelMaskName);
            
            boost::shared_ptr<Bool_Knob> invertMaskKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance, "Invert",1,false);
            _imp->invertMaskKnob.insert(std::make_pair(i, invertMaskKnob));
            invertMaskKnob->setDefaultValue(false, 0);
            invertMaskKnob->setAnimationEnabled(false);
            std::string inverMaskName("invert_mask_natron_" + maskName);
            invertMaskKnob->setName(inverMaskName);
            invertMaskKnob->setHintToolTip("Invert the use of the mask");
            
            ///and load it
            loadKnob(enableMaskKnob, serialization);
            loadKnob(maskChannelKnob, serialization);
            loadKnob(invertMaskKnob, serialization);
        }
    }
    
    _imp->nodeSettingsPage = Natron::createKnob<Page_Knob>(_imp->liveInstance, NATRON_EXTRA_PARAMETER_PAGE_NAME,1,false);
    
    _imp->previewEnabledKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance, "Preview enabled",1,false);
    _imp->previewEnabledKnob->setDefaultValue(makePreviewByDefault());
    _imp->previewEnabledKnob->setAnimationEnabled(false);
    _imp->previewEnabledKnob->setIsPersistant(false);
    _imp->previewEnabledKnob->setEvaluateOnChange(false);
    _imp->previewEnabledKnob->setHintToolTip("Whether to show a preview on the node box in the node-graph.");
    _imp->nodeSettingsPage->addKnob(_imp->previewEnabledKnob);
    
    _imp->disableNodeKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance, "Disable",1,false);
    _imp->disableNodeKnob->setAnimationEnabled(false);
    _imp->disableNodeKnob->setDefaultValue(false);
    _imp->disableNodeKnob->setHintToolTip("When disabled, this node acts as a pass through.");
    _imp->nodeSettingsPage->addKnob(_imp->disableNodeKnob);
    
    emit knobsInitialized();
}

void Node::beginEditKnobs() {
    _imp->liveInstance->beginEditKnobs();
}

void Node::createKnobDynamically()
{
    emit knobsInitialized();
}


void Node::setLiveInstance(Natron::EffectInstance* liveInstance)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->liveInstance = liveInstance;
}

Natron::EffectInstance* Node::getLiveInstance() const
{
    ///Thread safe as it never changes
    return _imp->liveInstance;
}

bool Node::hasEffect() const
{
    return _imp->liveInstance != NULL;
}

void Node::hasViewersConnected(std::list< ViewerInstance* >* viewers) const
{
    
    ViewerInstance* thisViewer = dynamic_cast<ViewerInstance*>(_imp->liveInstance);
    if(thisViewer) {
        std::list<ViewerInstance* >::const_iterator alreadyExists = std::find(viewers->begin(), viewers->end(), thisViewer);
        if(alreadyExists == viewers->end()){
            viewers->push_back(thisViewer);
        }
    } else {
        if (QThread::currentThread() == qApp->thread()) {
            for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputsQueue.begin(); it != _imp->outputsQueue.end(); ++it) {
                assert(*it);
                (*it)->hasViewersConnected(viewers);
            }
        } else {
            QMutexLocker l(&_imp->outputsMutex);
            for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
                assert(*it);
                (*it)->hasViewersConnected(viewers);
            }
        }
    }
}

void Node::hasWritersConnected(std::list<Natron::OutputEffectInstance* >* writers) const
{
    Natron::OutputEffectInstance* thisWriter = dynamic_cast<Natron::OutputEffectInstance*>(_imp->liveInstance);
    if(thisWriter) {
        std::list<Natron::OutputEffectInstance* >::const_iterator alreadyExists = std::find(writers->begin(), writers->end(), thisWriter);
        if(alreadyExists == writers->end()){
            writers->push_back(thisWriter);
        }
    } else {
        if (QThread::currentThread() == qApp->thread()) {
            for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputsQueue.begin(); it != _imp->outputsQueue.end(); ++it) {
                assert(*it);
                (*it)->hasWritersConnected(writers);
            }
        } else {
            QMutexLocker l(&_imp->outputsMutex);
            for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
                assert(*it);
                (*it)->hasWritersConnected(writers);
            }
        }
    }
}

int Node::majorVersion() const {
     ///Thread safe as it never changes
    return _imp->liveInstance->majorVersion();
}

int Node::minorVersion() const {
     ///Thread safe as it never changes
    return _imp->liveInstance->minorVersion();
}

void Node::initializeInputs()
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int oldCount = (int)_imp->inputs.size();
    int inputCount = maximumInputs();
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        _imp->inputs.resize(inputCount);
        _imp->inputsQueue.resize(inputCount);
        _imp->inputLabels.resize(inputCount);
        ///if we added inputs, just set to NULL the new inputs, and add their label to the labels map
        if (inputCount > oldCount) {
            for (int i = oldCount ; i < inputCount; ++i) {
                _imp->inputLabels[i] = _imp->liveInstance->inputLabel(i);
                _imp->inputs[i].reset();
                _imp->inputsQueue[i].reset();
            }
            
        }
        
        ///Set the components the plug-in accepts
        _imp->inputsComponents.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputsComponents[i].clear();
            _imp->liveInstance->addAcceptedComponents(i, &_imp->inputsComponents[i]);
        }
        _imp->outputComponents.clear();
        _imp->liveInstance->addAcceptedComponents(-1, &_imp->outputComponents);
        
    }
    emit inputsInitialized();
}

boost::shared_ptr<Node> Node::input(int index) const
{
    
    ////Only called by the main-thread
    ////@see input_other_thread for the MT version
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->inputsMutex);
    if (index >= (int)_imp->inputsQueue.size() || index < 0) {
        return boost::shared_ptr<Node>();
    }
    return _imp->inputsQueue[index];
    
}

boost::shared_ptr<Node> Node::input_other_thread(int index) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if (index >= (int)_imp->inputs.size() || index < 0) {
        return boost::shared_ptr<Node>();
    }
    return _imp->inputs[index];
}

void Node::updateRenderInputs()
{
    {
        QMutexLocker l(&_imp->inputsMutex);
        _imp->inputs = _imp->inputsQueue;
    }
    {
        QMutexLocker l(&_imp->outputsMutex);
        _imp->outputs = _imp->outputsQueue;
    }
}

const std::vector<boost::shared_ptr<Natron::Node> >& Node::getInputs_other_thread() const
{
    QMutexLocker l(&_imp->inputsMutex);
    return _imp->inputs;
}

const std::vector<boost::shared_ptr<Natron::Node> >& Node::getInputs_mt_safe() const
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->inputsQueue;
}

std::string Node::getInputLabel(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if (inputNb < 0 || inputNb >= (int)_imp->inputLabels.size()) {
        throw std::invalid_argument("Index out of range");
    }
    return _imp->inputLabels[inputNb];
}


bool Node::isInputConnected(int inputNb) const
{
    return input(inputNb) != NULL;
}

bool Node::hasInputConnected() const
{
    QMutexLocker l(&_imp->inputsMutex);
    if (QThread::currentThread() == qApp->thread()) {
        for (U32 i = 0; i < _imp->inputsQueue.size(); ++i) {
            if (_imp->inputsQueue[i]) {
                return true;
            }
        }
    } else {
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            if (_imp->inputs[i]) {
                return true;
            }
        }
    }
    return false;
}

bool Node::hasOutputConnected() const
{
    ////Only called by the main-thread
    if (QThread::currentThread() == qApp->thread()) {
        return _imp->outputsQueue.size() > 0;
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        return _imp->outputs.size() > 0;
    }
}

bool Node::checkIfConnectingInputIsOk(Natron::Node* input) const
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    if (input == this) {
        return false;
    }
    bool found;
    input->isNodeUpstream(this, &found);
    return !found;
}

void Node::isNodeUpstream(const Natron::Node* input,bool* ok) const
{
    
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    if (!input) {
        *ok = false;
        return;
    }
    
    QMutexLocker l(&_imp->inputsMutex);
    
    for (U32 i = 0; i  < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].get() == input) {
            *ok = true;
            return;
        }
    }
    *ok = false;
    for (U32 i = 0; i  < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i]) {
            _imp->inputs[i]->isNodeUpstream(input, ok);
            if (*ok) {
                return;
            }
        }
    }
    
    
}

bool Node::connectInput(boost::shared_ptr<Node> input,int inputNumber)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(input);
    
    if (!checkIfConnectingInputIsOk(input.get())) {
        return false;
    }
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        if (inputNumber < 0 || inputNumber > (int)_imp->inputsQueue.size() || _imp->inputsQueue[inputNumber] != NULL) {
            return false;
        }
        _imp->inputsQueue[inputNumber] = input;
    }
    QObject::connect(input.get(), SIGNAL(nameChanged(QString)), this, SLOT(onInputNameChanged(QString)));
    emit inputChanged(inputNumber);
    onInputChanged(inputNumber);
    computeHash();
    return true;
}

void Node::onInputNameChanged(const QString& name)
{
    assert(QThread::currentThread() == qApp->thread());
    EffectInstance* inp = dynamic_cast<EffectInstance*>(sender());
    assert(inp);
    int inputNb = -1;
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size(); ++i) {
            if (_imp->inputsQueue[i]->getLiveInstance() == inp) {
                inputNb = i;
                break;
            }
        }
    }
    if (inputNb != - 1) {
        emit inputNameChanged(inputNb, name);
    }
}

void Node::connectOutput(boost::shared_ptr<Node> output)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(output);
    {
        QMutexLocker l(&_imp->outputsMutex);
        _imp->outputsQueue.push_back(output);
    }
    emit outputsChanged();
}

int Node::disconnectInput(int inputNumber)
{
    
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        if (inputNumber < 0 || inputNumber > (int)_imp->inputsQueue.size() || _imp->inputsQueue[inputNumber] == NULL) {
            return -1;
        }
        QObject::disconnect(_imp->inputsQueue[inputNumber].get(), SIGNAL(nameChanged(QString)), this, SLOT(onInputNameChanged(QString)));
        _imp->inputsQueue[inputNumber].reset();
    }
    emit inputChanged(inputNumber);
    onInputChanged(inputNumber);
    computeHash();
    return inputNumber;
}

int Node::disconnectInput(boost::shared_ptr<Node> input)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size(); ++i) {
            if (_imp->inputsQueue[i] == input) {
                _imp->inputsQueue[i].reset();
                l.unlock();
                emit inputChanged(i);
                onInputChanged(i);
                computeHash();
                l.relock();
                return i;
            }
        }
    }
    return -1;
    
}

int Node::disconnectOutput(boost::shared_ptr<Node> output)
{

    assert(output);
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    int ret = -1;
    {
        QMutexLocker l(&_imp->outputsMutex);
        std::list<boost::shared_ptr<Node> >::iterator it = std::find(_imp->outputsQueue.begin(),_imp->outputsQueue.end(),output);
        
        if (it != _imp->outputsQueue.end()) {
            ret = std::distance(_imp->outputsQueue.begin(), it);
            _imp->outputsQueue.erase(it);
        }
    }
    emit outputsChanged();
    return ret;
}

int Node::inputIndex(Node* n) const {
    
    if (!n) {
        return -1;
    }
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size(); ++i) {
            if (_imp->inputsQueue[i].get() == n) {
                return i;
            }
        }
    }
    return -1;
}

/*After this call this node still knows the link to the old inputs/outputs
 but no other node knows this node.*/
void Node::deactivate()
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    //first tell the gui to clear any persistent message linked to this node
    clearPersistentMessage();

    ///if the node has 1 non-optional input, attempt to connect the outputs to the input of the current node
    ///this node is the node the outputs should attempt to connect to
    boost::shared_ptr<Node> inputToConnectTo;
    boost::shared_ptr<Node> firstOptionalInput;
    int firstNonOptionalInput = -1;
    bool hasOnlyOneInputConnected = false;
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size() ; ++i) {
            if (_imp->inputsQueue[i]) {
                if (!_imp->liveInstance->isInputOptional(i)) {
                    if (firstNonOptionalInput == -1) {
                        firstNonOptionalInput = i;
                        hasOnlyOneInputConnected = true;
                    } else {
                        hasOnlyOneInputConnected = false;
                    }
                } else if(!firstOptionalInput) {
                    firstOptionalInput = _imp->inputsQueue[i];
                    if (hasOnlyOneInputConnected) {
                        hasOnlyOneInputConnected = false;
                    } else {
                        hasOnlyOneInputConnected = true;
                    }
                }
            }
        }
    }
    if (hasOnlyOneInputConnected) {
        if (firstNonOptionalInput != -1) {
            inputToConnectTo = input(firstNonOptionalInput);
        } else if(firstOptionalInput) {
            inputToConnectTo = firstOptionalInput;
        }
    }
    
    /*Removing this node from the output of all inputs*/
    _imp->deactivatedState.clear();
    
    boost::shared_ptr<Natron::Node> thisShared = getApp()->getProject()->getNodePointer(this);
    assert(thisShared);
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size() ; ++i) {
            if(_imp->inputsQueue[i]) {
                _imp->inputsQueue[i]->disconnectOutput(thisShared);
            }
        }
    }
    
    ///For each output node we remember that the output node  had its input number inputNb connected
    ///to this node
    {
        QMutexLocker l(&_imp->outputsMutex);
        for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputsQueue.begin(); it!=_imp->outputsQueue.end(); ++it) {
            assert(*it);
            int inputNb = (*it)->disconnectInput(thisShared);
            _imp->deactivatedState.insert(make_pair(*it, inputNb));
        }
    }
    
    if (inputToConnectTo) {
        for (std::map<boost::shared_ptr<Node>,int >::iterator it = _imp->deactivatedState.begin();
             it!=_imp->deactivatedState.end(); ++it) {
            getApp()->getProject()->connectNodes(it->second, inputToConnectTo, it->first);
        }
    }
    
    ///kill any thread it could have started (e.g: VideoEngine or preview)
    quitAnyProcessing();
    
    emit deactivated();
    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = false;
    }
    
}

void Node::activate()
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    boost::shared_ptr<Natron::Node> thisShared = getApp()->getProject()->getNodePointer(this);
    assert(thisShared);
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        ///for all inputs, reconnect their output to this node
        for (U32 i = 0; i < _imp->inputsQueue.size(); ++i){
            if (_imp->inputsQueue[i]) {
                _imp->inputsQueue[i]->connectOutput(thisShared);
            }
        }
    }
    
    ///Restore all outputs that was connected to this node
    for (std::map<boost::shared_ptr<Node>,int >::iterator it = _imp->deactivatedState.begin();
         it!= _imp->deactivatedState.end(); ++it) {
        
        ///before connecting the outputs to this node, disconnect any link that has been made
        ///between the outputs by the user. This should normally never happen as the undo/redo
        ///stack follow always the same order.
        boost::shared_ptr<Node> outputHasInput = it->first->input(it->second);
        if (outputHasInput) {
            bool ok = getApp()->getProject()->disconnectNodes(outputHasInput, it->first);
            assert(ok);
        }

        ///and connect the output to this node
        it->first->connectInput(thisShared, it->second);
    }
    
    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = true; //< flag it true before notifying the GUI because the gui rely on this flag (espcially the Viewer)
    }
    emit activated();
}


boost::shared_ptr<KnobI> Node::getKnobByName(const std::string& name) const
{
    ///MT-safe, never changes
    return _imp->liveInstance->getKnobByName(name);
}


namespace {
static float bilinearFiltering(const float* srcPixelsFloor,const float* srcPixelsCeil,int fx,int cx,
                               int elementsCount,int comp,double dx,double dy,const RectI& srcRoD)
{
    const double Icc = (!srcPixelsFloor || fx < srcRoD.x1) ? 0. : srcPixelsFloor[fx * elementsCount + comp];
    const double Inc = (!srcPixelsFloor || cx >= srcRoD.x2) ? 0. : srcPixelsFloor[cx * elementsCount + comp];
    const double Icn = (!srcPixelsCeil || fx < srcRoD.x1) ? 0. : srcPixelsCeil[fx * elementsCount + comp];
    const double Inn = (!srcPixelsCeil || cx >= srcRoD.x2) ? 0. : srcPixelsCeil[cx * elementsCount + comp];
    return Icc + dx*(Inc-Icc + dy*(Icc+Inn-Icn-Inc)) + dy*(Icn-Icc);
}
}

void Node::makePreviewImage(SequenceTime time,int width,int height,unsigned int* buf)
{

    {
        QMutexLocker locker(&_imp->mustQuitProcessingMutex);
        if (_imp->mustQuitProcessing) {
            _imp->mustQuitProcessing = false;
            _imp->mustQuitProcessingCond.wakeOne();
            return;
        }
    }
  
    QMutexLocker locker(&_imp->computingPreviewMutex); /// prevent 2 previews to occur at the same time since there's only 1 preview instance
    _imp->computingPreview = true;
    
    RectI rod;
    bool isProjectFormat;
    RenderScale scale;
    scale.x = scale.y = 1.;
    Natron::Status stat = _imp->liveInstance->getRegionOfDefinition_public(time,scale,0, &rod,&isProjectFormat);
    if (stat == StatFailed) {
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    }
    double yZoomFactor = (double)height/(double)rod.height();
    double xZoomFactor = (double)width/(double)rod.width();
    
    double closestPowerOf2X = xZoomFactor >= 1 ? 1 : std::pow(2,-std::ceil(std::log(xZoomFactor) / std::log(2.)));
    double closestPowerOf2Y = yZoomFactor >= 1 ? 1 : std::pow(2,-std::ceil(std::log(yZoomFactor) / std::log(2.)));
    
    int closestPowerOf2 = std::max(closestPowerOf2X,closestPowerOf2Y);
    unsigned int mipMapLevel = std::min(std::log((double)closestPowerOf2) / std::log(2.),5.);
    
    scale.x = Natron::Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;
#ifdef NATRON_LOG
    Log::beginFunction(getName(),"makePreviewImage");
    Log::print(QString("Time "+QString::number(time)).toStdString());
#endif

    boost::shared_ptr<Image> img;
    
    RectI scaledRod = rod.roundPowerOfTwoLargestEnclosed(mipMapLevel);
    // Exceptions are caught because the program can run without a preview,
    // but any exception in renderROI is probably fatal.
    try {
        img = _imp->liveInstance->renderRoI(EffectInstance::RenderRoIArgs(time, scale,mipMapLevel, 0,scaledRod,false,true,false,NULL));
    } catch (const std::exception& e) {
        qDebug() << "Error: Cannot create preview" << ": " << e.what();
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    } catch (...) {
        qDebug() << "Error: Cannot create preview";
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    }
    
    if (!img) {
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    }
  
    ///update the Rod to the scaled image rod
    rod = img->getPixelRoD();

    ImageComponents components = img->getComponents();
    int elemCount = getElementsCountForComponents(components);
    
    ///recompute it after the rescaling
    yZoomFactor = (double)height/(double)rod.height();
    xZoomFactor = (double)width/(double)rod.width();
    
    for (int i = 0; i < height; ++i) {
        double y = (double)i/yZoomFactor + rod.y1;
        int yFloor = std::floor(y);
        int yCeil = std::ceil(y);
        double dy = std::max(0., std::min(y - yFloor, 1.));
        
        U32 *dst_pixels = buf + width * (height-1-i);
        
        const float* src_pixels_floor = img->pixelAt(rod.x1, yFloor);
        const float* src_pixels_ceil = img->pixelAt(rod.x1, yCeil);
        for (int j = 0; j < width; ++j) {
            
            double x = (double)j/xZoomFactor + rod.x1;
            int xFloor = std::floor(x);
            int xCeil = std::ceil(x);
            double dx = std::max(0., std::min(x - xFloor, 1.));
            
#pragma message WARN("Preview only support RGB or RGBA images")
            float rFilt = bilinearFiltering(src_pixels_floor, src_pixels_ceil, xFloor, xCeil, elemCount, 0, dx, dy, rod);
            float gFilt = bilinearFiltering(src_pixels_floor, src_pixels_ceil, xFloor, xCeil, elemCount, 1, dx, dy, rod);
            float bFilt = bilinearFiltering(src_pixels_floor, src_pixels_ceil, xFloor, xCeil, elemCount, 2, dx, dy, rod);
            
            int r = Color::floatToInt<256>(Natron::Color::to_func_srgb(rFilt));
            int g = Color::floatToInt<256>(Natron::Color::to_func_srgb(gFilt));
            int b = Color::floatToInt<256>(Natron::Color::to_func_srgb(bFilt));
            dst_pixels[j] = toBGRA(r, g, b, 255);

        }
    }
    _imp->computingPreview = false;
    _imp->computingPreviewCond.wakeOne();
#ifdef NATRON_LOG
    Log::endFunction(getName(),"makePreviewImage");
#endif
}

bool Node::isInputNode() const{
    ///MT-safe, never changes
    return _imp->liveInstance->isGenerator();
}


bool Node::isOutputNode() const
{   ///MT-safe, never changes
    return _imp->liveInstance->isOutput();
}


bool Node::isInputAndProcessingNode() const
{   ///MT-safe, never changes
    return _imp->liveInstance->isGeneratorAndFilter();
}


bool Node::isOpenFXNode() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->isOpenFX();
}

bool Node::isRotoNode() const
{
    ///Runs only in the main thread (checked by getName())
    ///Crude way to distinguish between Rotoscoping and Rotopainting nodes.
    QString name = pluginID().c_str();
    return name.contains("roto",Qt::CaseInsensitive);
}

/**
 * @brief Returns true if the node is a rotopaint node
 **/
bool Node::isRotoPaintingNode() const
{
    ///Runs only in the main thread (checked by getName())
    QString name = pluginID().c_str();
    return name.contains("rotopaint",Qt::CaseInsensitive);
}

boost::shared_ptr<RotoContext> Node::getRotoContext() const
{
    return _imp->rotoContext;
}

const std::vector< boost::shared_ptr<KnobI> >& Node::getKnobs() const
{
    ///MT-safe from EffectInstance::getKnobs()
    return _imp->liveInstance->getKnobs();
}

std::string Node::pluginID() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->pluginID();
}

std::string Node::pluginLabel() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->pluginLabel();
}

std::string Node::description() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->description();
}

int Node::maximumInputs() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->maximumInputs();
}

bool Node::makePreviewByDefault() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->makePreviewByDefault();
}

void Node::togglePreview()
{
    ///MT-safe from Knob
    _imp->previewEnabledKnob->setValue(!_imp->previewEnabledKnob->getValue(),0);
}

bool Node::isPreviewEnabled() const
{
     ///MT-safe from EffectInstance
    return _imp->previewEnabledKnob->getValue();
}

bool Node::aborted() const
{
     ///MT-safe from EffectInstance
    return _imp->liveInstance->aborted();
}

void Node::setAborted(bool b)
{
     ///MT-safe from EffectInstance
    _imp->liveInstance->setAborted(b);
}

bool Node::message(MessageType type,const std::string& content) const
{
    switch (type) {
        case INFO_MESSAGE:
            informationDialog(getName_mt_safe(), content);
            return true;
        case WARNING_MESSAGE:
            warningDialog(getName_mt_safe(), content);
            return true;
        case ERROR_MESSAGE:
            errorDialog(getName_mt_safe(), content);
            return true;
        case QUESTION_MESSAGE:
            return questionDialog(getName_mt_safe(), content) == Yes;
        default:
            return false;
    }
}

void Node::setPersistentMessage(MessageType type,const std::string& content)
{
    if (!appPTR->isBackground()) {
        //if the message is just an information, display a popup instead.
        if (type == INFO_MESSAGE) {
            message(type,content);
            return;
        }
        QString message;
        message.append(getName_mt_safe().c_str());
        if (type == ERROR_MESSAGE) {
            message.append(" error: ");
        } else if(type == WARNING_MESSAGE) {
            message.append(" warning: ");
        }
        message.append(content.c_str());
        emit persistentMessageChanged((int)type,message);
    }else{
        std::cout << "Persistent message" << std::endl;
        std::cout << content << std::endl;
    }
}

void Node::clearPersistentMessage()
{
    if(!appPTR->isBackground()){
        emit persistentMessageCleared();
    }
}



void Node::purgeAllInstancesCaches() {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->liveInstance->purgeCaches();
}

void Node::notifyInputNIsRendering(int inputNb) {
    emit inputNIsRendering(inputNb);
}

void Node::notifyInputNIsFinishedRendering(int inputNb) {
    emit inputNIsFinishedRendering(inputNb);
}

void Node::notifyRenderingStarted() {
    emit renderingStarted();
}

void Node::notifyRenderingEnded() {
    emit renderingEnded();
}


void Node::setInputFilesForReader(const std::vector<std::string>& files) {
    _imp->liveInstance->setInputFilesForReader(files);
}

void Node::setOutputFilesForWriter(const std::string& pattern) {
    _imp->liveInstance->setOutputFilesForWriter(pattern);
}

void Node::registerPluginMemory(size_t nBytes) {
    {
        QMutexLocker l(&_imp->memoryUsedMutex);
        _imp->pluginInstanceMemoryUsed += nBytes;
    }
    emit pluginMemoryUsageChanged(nBytes);
}

void Node::unregisterPluginMemory(size_t nBytes) {
    {
        QMutexLocker l(&_imp->memoryUsedMutex);
        _imp->pluginInstanceMemoryUsed -= nBytes;
    }
    emit pluginMemoryUsageChanged(-nBytes);
}

QMutex& Node::getRenderInstancesSharedMutex()
{
    return _imp->renderInstancesSharedMutex;
}

QMutex& Node::getFrameMutex(int time)
{
    QMutexLocker l(&_imp->perFrameMutexesLock);
    std::map<int,boost::shared_ptr<QMutex> >::const_iterator it = _imp->renderInstancesFullySafePerFrameMutexes.find(time);
    if (it != _imp->renderInstancesFullySafePerFrameMutexes.end()) {
        // found the mutex, return it
        return *(it->second);
    }
    // create new map entry containing the mutex for this frame
    shared_ptr<QMutex> m(new QMutex);
    _imp->renderInstancesFullySafePerFrameMutexes.insert(std::make_pair(time,m));
    return *m;
}

void Node::refreshPreviewsRecursively() {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    if (isPreviewEnabled()) {
        refreshPreviewImage(_imp->liveInstance->getCurrentFrameRecursive());
    }
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputsQueue.begin(); it!=_imp->outputsQueue.end(); ++it) {
        assert(*it);
        (*it)->refreshPreviewsRecursively();
    }
}


void Node::onSlaveStateChanged(bool isSlave,KnobHolder* master) {
   
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    if (isSlave) {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(master);
        assert(effect);
        boost::shared_ptr<Natron::Node> masterNode = effect->getNode();
        {
            QMutexLocker l(&_imp->masterNodeMutex);
            _imp->masterNode = masterNode;
        }
        QObject::connect(masterNode.get(), SIGNAL(deactivated()), this, SLOT(onMasterNodeDeactivated()));
        QObject::connect(masterNode.get(), SIGNAL(knobsAgeChanged(U64)), this, SLOT(setKnobsAge(U64)));
        QObject::connect(masterNode.get(), SIGNAL(previewImageChanged(int)), this, SLOT(refreshPreviewImage(int)));
    } else {
        QObject::disconnect(_imp->masterNode.get(), SIGNAL(deactivated()), this, SLOT(onMasterNodeDeactivated()));
        QObject::disconnect(_imp->masterNode.get(), SIGNAL(knobsAgeChanged(U64)), this, SLOT(setKnobsAge(U64)));
        QObject::disconnect(_imp->masterNode.get(), SIGNAL(previewImageChanged(int)), this, SLOT(refreshPreviewImage(int)));
        {
            QMutexLocker l(&_imp->masterNodeMutex);
            _imp->masterNode.reset();
        }
    }
    
    emit slavedStateChanged(isSlave);

}

void Node::onMasterNodeDeactivated() {
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->liveInstance->unslaveAllKnobs();
}

boost::shared_ptr<Natron::Node> Node::getMasterNode() const {
    QMutexLocker l(&_imp->masterNodeMutex);
    return _imp->masterNode;
}

bool Node::isSupportedComponent(int inputNb,Natron::ImageComponents comp) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if (inputNb >= 0) {
        assert(inputNb < (int)_imp->inputsComponents.size());
        std::list<Natron::ImageComponents>::const_iterator found =
        std::find(_imp->inputsComponents[inputNb].begin(),_imp->inputsComponents[inputNb].end(),comp);
        return found != _imp->inputsComponents[inputNb].end();
    } else {
        assert(inputNb == -1);
        std::list<Natron::ImageComponents>::const_iterator found =
        std::find(_imp->outputComponents.begin(),_imp->outputComponents.end(),comp);
        return found != _imp->outputComponents.end();

    }
}

Natron::ImageComponents Node::findClosestSupportedComponents(int inputNb,Natron::ImageComponents comp) const
{
    int compCount = getElementsCountForComponents(comp);

    QMutexLocker l(&_imp->inputsMutex);
    if (inputNb >= 0) {
        assert(inputNb < (int)_imp->inputsComponents.size());
        
        
        const std::list<Natron::ImageComponents>& comps = _imp->inputsComponents[inputNb];
        if (comps.empty()) {
            return Natron::ImageComponentNone;
        }
        std::list<Natron::ImageComponents>::const_iterator closestComp = comps.end();
        for (std::list<Natron::ImageComponents>::const_iterator it = comps.begin(); it != comps.end(); ++it) {
            if (closestComp == comps.end()) {
                closestComp = it;
            } else {
                if (std::abs(getElementsCountForComponents(*it) - compCount) <
                    std::abs(getElementsCountForComponents(*closestComp) - compCount)) {
                    closestComp = it;
                }
            }
        }
        assert(closestComp != comps.end());
        return *closestComp;
    } else {
        assert(inputNb == -1);
        const std::list<Natron::ImageComponents>& comps = _imp->outputComponents;
        if (comps.empty()) {
            return Natron::ImageComponentNone;
        }
        std::list<Natron::ImageComponents>::const_iterator closestComp = comps.end();
        for (std::list<Natron::ImageComponents>::const_iterator it = comps.begin(); it != comps.end(); ++it) {
            if (closestComp == comps.end()) {
                closestComp = it;
            } else {
                if (std::abs(getElementsCountForComponents(*it) - compCount) <
                    std::abs(getElementsCountForComponents(*closestComp) - compCount)) {
                    closestComp = it;
                }
            }
        }
        assert(closestComp != comps.end());
        return *closestComp;
    }
}

int Node::getMaskChannel(int inputNb) const
{
    std::map<int, boost::shared_ptr<Choice_Knob> >::const_iterator it = _imp->maskChannelKnob.find(inputNb);
    if (it != _imp->maskChannelKnob.end()) {
        return it->second->getValue() - 1;
    } else {
        return 3;
    }
 
}


bool Node::isMaskEnabled(int inputNb) const
{
    std::map<int, boost::shared_ptr<Bool_Knob> >::const_iterator it = _imp->enableMaskKnob.find(inputNb);
    if (it != _imp->enableMaskKnob.end()) {
        return it->second->getValue();
    } else {
        return true;
    }
}


bool Node::isMaskInverted(int inputNb) const
{
    std::map<int, boost::shared_ptr<Bool_Knob> >::const_iterator it = _imp->invertMaskKnob.find(inputNb);
    if (it != _imp->invertMaskKnob.end()) {
        return it->second->getValue();
    } else {
        return false;
    }
}


void Node::addImageBeingRendered(const boost::shared_ptr<Natron::Image>& image)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<boost::shared_ptr<Natron::Image> >::iterator it =
    std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    while (it != _imp->imagesBeingRendered.end()) {
        _imp->imageBeingRenderedCond.wait(&_imp->imagesBeingRenderedMutex);
        it = std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    }
    ///Okay the image is not used by any other thread, claim that we want to use it
    assert(it == _imp->imagesBeingRendered.end());
    _imp->imagesBeingRendered.push_back(image);
}

void Node::removeImageBeingRendered(const boost::shared_ptr<Natron::Image>& image)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    
    std::list<boost::shared_ptr<Natron::Image> >::iterator it =
    std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    
    ///The image must exist, otherwise this is a bug
    assert(it != _imp->imagesBeingRendered.end());
    
    _imp->imagesBeingRendered.erase(it);
    
    ///Notify all waiting threads that we're finished
    _imp->imageBeingRenderedCond.wakeAll();
}

boost::shared_ptr<Natron::Image> Node::getImageBeingRendered(int time,unsigned int mipMapLevel,int view)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    for (std::list<boost::shared_ptr<Natron::Image> >::iterator it = _imp->imagesBeingRendered.begin();
         it!= _imp->imagesBeingRendered.end(); ++it) {
        const Natron::ImageKey &key = (*it)->getKey();
        if (key._view == view && key._mipMapLevel == mipMapLevel && key._time == time) {
            return *it;
        }
    }
    return boost::shared_ptr<Natron::Image>();
}

void Node::onInputChanged(int inputNb)
{
    
    std::map<int, boost::shared_ptr<Bool_Knob> >::iterator it = _imp->enableMaskKnob.find(inputNb);
    if (it != _imp->enableMaskKnob.end()) {
        boost::shared_ptr<Node> inp = input(inputNb);
        it->second->setValue(inp ? true : false, 0);
    }
    _imp->liveInstance->onInputChanged(inputNb);
    
}

void Node::onMultipleInputChanged()
{
    for (std::map<int, boost::shared_ptr<Bool_Knob> >::iterator it = _imp->enableMaskKnob.begin(); it!=_imp->enableMaskKnob.end(); ++it) {
        boost::shared_ptr<Node> inp = input(it->first);
        it->second->setValue(inp ? true : false, 0);
    }
    _imp->liveInstance->onMultipleInputsChanged();
}

void Node::onEffectKnobValueChanged(KnobI* what,Natron::ValueChangedReason reason)
{
    for (std::map<int, boost::shared_ptr<Choice_Knob> >::iterator it = _imp->maskChannelKnob.begin(); it!=_imp->maskChannelKnob.end(); ++it) {
        if (it->second.get() == what) {
            int index = it->second->getValue();
            std::map<int, boost::shared_ptr<Bool_Knob> >::iterator found = _imp->enableMaskKnob.find(it->first);
            if (index == 0 && found->second->isEnabled(0)) {
                found->second->setValue(false, 0);
                found->second->setEnabled(0, false);
            } else if (!found->second->isEnabled(0)) {
                found->second->setEnabled(0, true);
                if (input(it->first)) {
                    found->second->setValue(true, 0);
                }
            }
            break;
        }
    }
    
    if (what == _imp->previewEnabledKnob.get()) {
        if (reason == Natron::USER_EDITED) {
            emit previewKnobToggled();
        }
    } else if (what == _imp->disableNodeKnob.get()) {
        emit disabledKnobToggled(_imp->disableNodeKnob->getValue());
    }
}

bool Node::isNodeDisabled() const
{
    return _imp->disableNodeKnob->getValue();
}

void Node::getAllKnobsKeyframes(std::list<SequenceTime>* keyframes)
{
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        int dim = knobs[i]->getDimension();
        File_Knob* isFile = dynamic_cast<File_Knob*>(knobs[i].get());
        if (isFile) {
            ///skip file knobs
            continue;
        }
        for (int j = 0; j < dim ;++j) {
            KeyFrameSet kfs = knobs[i]->getCurve()->getKeyFrames_mt_safe();
            for (KeyFrameSet::iterator it = kfs.begin(); it!=kfs.end(); ++it) {
                keyframes->push_back(it->getTime());
            }
        }
    }
    if (_imp->rotoContext) {
        _imp->rotoContext->getBeziersKeyframeTimes(keyframes);
    }
}

//////////////////////////////////

InspectorNode::InspectorNode(AppInstance* app,LibraryBinary* plugin)
    : Node(app,plugin)
    , _inputsCount(1)
    , _activeInput(0)
    , _activeInputMutex()
{}


InspectorNode::~InspectorNode(){
    
}

bool InspectorNode::connectInput(boost::shared_ptr<Node> input,int inputNumber) {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    ///cannot connect more than 10 inputs.
    assert(inputNumber <= 10);
    
    assert(input);
    
    if (!checkIfConnectingInputIsOk(input.get())) {
        return false;
    }
    
    
    /*Adding all empty edges so it creates at least the inputNB'th one.*/
    while (_inputsCount <= inputNumber) {
        
        ///this function might not succeed if we already have 10 inputs OR the last input is already empty
        addEmptyInput();
    }
    
    ///If the node 'input' is already an input of the inspector, find it.
    ///If it has the same input number as what we want just return, otherwise
    ///disconnect it and continue as usual.
    int inputAlreadyConnected = inputIndex(input.get());
    if (inputAlreadyConnected != -1) {
        if (inputAlreadyConnected == inputNumber) {
            return false;
        } else {
            disconnectInput(inputAlreadyConnected);
        }
    }

    int oldActiveInput;
    {
        QMutexLocker activeInputLocker(&_activeInputMutex);
        oldActiveInput = _activeInput;
        _activeInput = inputNumber;
    }
    if (!Node::connectInput(input, inputNumber)) {
        {
            QMutexLocker activeInputLocker(&_activeInputMutex);
            _activeInput = oldActiveInput;
        }
        computeHash();
    }
    tryAddEmptyInput();
    return true;
    
}



bool InspectorNode::tryAddEmptyInput() {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());

    
    ///if we already reached 10 inputs, just don't do anything
    if (_inputsCount <= 10) {
        
        
        if (_inputsCount > 0) {
            ///if there are already living inputs, look at the last one
            ///and if it is not connected, just don't add an input.
            ///Otherwise, add an empty input.
            if(input(_inputsCount - 1) != NULL){
                addEmptyInput();
                return true;
            }
            
        } else {
            ///there'is no inputs yet, just add one.
            addEmptyInput();
            return true;
        }
        
    }
    return false;
}
void InspectorNode::addEmptyInput() {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker activeInputLocker(&_activeInputMutex);
        _activeInput = _inputsCount-1;
    }
    ++_inputsCount;
    initializeInputs();
}

void InspectorNode::removeEmptyInputs() {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    /*While there're NULL inputs at the tail of the map,remove them.
     Stops at the first non-NULL input.*/
    while (_inputsCount > 1) {
        if(input(_inputsCount - 1) == NULL && input(_inputsCount - 2) == NULL){
            --_inputsCount;
            initializeInputs();
        } else {
            return;
        }
    }
}
int InspectorNode::disconnectInput(int inputNumber) {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int ret = Node::disconnectInput(inputNumber);
 
    if (ret != -1) {
        removeEmptyInputs();
        {
            QMutexLocker activeInputLocker(&_activeInputMutex);
            _activeInput = _inputsCount -1;
        }
    }
    return ret;
}

int InspectorNode::disconnectInput(boost::shared_ptr<Node> input) {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    return disconnectInput(inputIndex(input.get()));
}

void InspectorNode::setActiveInputAndRefresh(int inputNb){
    if (inputNb > (_inputsCount - 1) || inputNb < 0 || input(inputNb) == NULL) {
        return;
    }
    {
        QMutexLocker activeInputLocker(&_activeInputMutex);
        _activeInput = inputNb;
    }
    computeHash();
    if (isOutputNode()) {
        dynamic_cast<Natron::OutputEffectInstance*>(getLiveInstance())->updateTreeAndRender();
    }
}

