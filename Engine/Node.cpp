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

/*A key to identify an image rendered for this node.*/
struct ImageBeingRenderedKey{

    ImageBeingRenderedKey():_time(0),_view(0) , _mipMapLevel(0) {}

    ImageBeingRenderedKey(int time,int view,unsigned mipmapLevel):_time(time),_view(view) , _mipMapLevel(mipmapLevel){}

    SequenceTime _time;
    int _view;
    unsigned int _mipMapLevel;

    bool operator==(const ImageBeingRenderedKey& other) const {
        return _time == other._time &&
                _view == other._view &&
        _mipMapLevel == other._mipMapLevel;
    }

    bool operator<(const ImageBeingRenderedKey& other) const {
        return _time < other._time ||
                _view < other._view;
    }
};

typedef std::multimap<ImageBeingRenderedKey,boost::shared_ptr<Image> > ImagesMap;


/*The output node was connected from inputNumber to this...*/
typedef std::map<boost::shared_ptr<Node> ,int > DeactivatedState;

}

struct Node::Implementation {
    Implementation(AppInstance* app_,LibraryBinary* plugin_)
        : app(app_)
        , outputs()
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
//        , imageBeingRenderedMutex()
//        , imagesBeingRenderedNotEmpty()
//        , imagesBeingRendered()
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
        , rotoContext()
    {
    }
    

    AppInstance* app; // pointer to the app: needed to access the application's default-project's format
    
    std::list<boost::shared_ptr<Node> > outputs; //< unlike the inputs we do not need a copy so the gui can modify it
                                //in a thread-safe manner because it is never read by the render thread.
    
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
    
//    QMutex imageBeingRenderedMutex;
//    QWaitCondition imagesBeingRenderedNotEmpty; //to avoid computing preview in parallel of the real rendering
//
//    ImagesMap imagesBeingRendered; //< a map storing the ongoing render for this node
    
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
  
    boost::shared_ptr<Bool_Knob> enableMaskKnob;
    boost::shared_ptr<Choice_Knob> maskChannelKnob;
    boost::shared_ptr<Bool_Knob> invertMaskKnob;
    
    boost::shared_ptr<RotoContext> rotoContext; //< valid when the node has a rotoscoping context (i.e: paint context)
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
    initializeKnobs();
    
    
    
    ///non OpenFX-plugin
    if (func.first && !serialization.isNull() && serialization.getPluginID() == pluginID &&
        majorVersion() == serialization.getPluginMajorVersion() && minorVersion() == serialization.getPluginMinorVersion()) {
            loadKnobs(serialization);
    }
    
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
            InspectorNode* isInspector = dynamic_cast<InspectorNode*>(this);
            QMutexLocker l(&_imp->inputsMutex);
            for (U32 i = 0; i < _imp->inputsQueue.size();++i) {
                if (isInspector && isInspector->activeInput() != (int)i) {
                    continue;
                }
                if (_imp->inputsQueue[i]) {
                    _imp->hash.append(_imp->inputsQueue[i]->getHashValue());
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
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
        assert(*it);
        (*it)->computeHash();
    }
}

void Node::loadKnobs(const NodeSerialization& serialization) {
    
    ///Only called from the main thread
    assert(QThread::currentThread() == qApp->thread());
    
    const std::vector< boost::shared_ptr<KnobI> >& nodeKnobs = getKnobs();
    const NodeSerialization::KnobValues& knobsValues = serialization.getKnobsValues();
    ///for all knobs of the node
    for (U32 j = 0; j < nodeKnobs.size();++j) {
        
        ///try to find a serialized value for this knob
        for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it!=knobsValues.end();++it) {
            if((*it)->getName() == nodeKnobs[j]->getName()){
                // don't load the value if the Knob is not persistant! (it is just the default value in this case)
                if (nodeKnobs[j]->getIsPersistant()) {
                    nodeKnobs[j]->clone((*it)->getKnob());
                }
                break;
            }
        }
    }
    
    ///now restore the roto context if the node has a roto context
    if (serialization.hasRotoContext() && _imp->rotoContext) {
        _imp->rotoContext->load(serialization.getRotoContext());
    }
    
    setKnobsAge(serialization.getKnobsAge());
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
    return _imp->outputs;
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
    
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
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
    
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
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

void Node::initializeKnobs() {
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    _imp->liveInstance->initializeKnobsPublic();
    
    ///If the effect has a mask, add additionnal mask controls
    int inputsCount = maximumInputs();
    for (int i = 0; i < inputsCount; ++i) {
        if (_imp->liveInstance->isInputMask(i) && !_imp->liveInstance->isInputRotoBrush(i)) {
            std::string maskName = _imp->liveInstance->inputLabel(i);
            _imp->enableMaskKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance, maskName);
            _imp->enableMaskKnob->setDefaultValue(false, 0);
            _imp->enableMaskKnob->turnOffNewLine();
            _imp->enableMaskKnob->setName("enable_mask_natron_" + maskName);
            _imp->enableMaskKnob->setAnimationEnabled(false);
            _imp->enableMaskKnob->setHintToolTip("Enable the mask to come from the channel named by the choice parameter on the right. "
                                                 "Turning this off will fill with 1's the mask.");
            
            _imp->maskChannelKnob = Natron::createKnob<Choice_Knob>(_imp->liveInstance, "");
            std::vector<std::string> choices;
            choices.push_back("None");
            choices.push_back("Red");
            choices.push_back("Green");
            choices.push_back("Blue");
            choices.push_back("Alpha");
            _imp->maskChannelKnob->populateChoices(choices);
            _imp->maskChannelKnob->setDefaultValue(4, 0);
            _imp->maskChannelKnob->setAnimationEnabled(false);
            _imp->maskChannelKnob->turnOffNewLine();
            _imp->maskChannelKnob->setHintToolTip("Use this channel from the original input to mix the output with the original input. "
                                                  "Setting this to None is the same as disabling the mask.");
            _imp->maskChannelKnob->setName("mask_channel_natron_" + maskName);
            
            _imp->invertMaskKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance, "Invert");
            _imp->invertMaskKnob->setDefaultValue(false, 0);
            _imp->invertMaskKnob->setAnimationEnabled(false);
            _imp->invertMaskKnob->setName("invert_mask_natron_" + maskName);
            _imp->invertMaskKnob->setHintToolTip("Invert the use of the mask");
        }
    }
    
    
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

void Node::hasViewersConnected(std::list< ViewerInstance* >* viewers) const
{
    
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    if(pluginID() == "Viewer") {
        ViewerInstance* thisViewer = dynamic_cast<ViewerInstance*>(_imp->liveInstance);
        assert(thisViewer);
        std::list<ViewerInstance* >::const_iterator alreadyExists = std::find(viewers->begin(), viewers->end(), thisViewer);
        if(alreadyExists == viewers->end()){
            viewers->push_back(thisViewer);
        }
    } else {
        for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
            assert(*it);
            (*it)->hasViewersConnected(viewers);
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
    QMutexLocker l(&_imp->inputsMutex);
    _imp->inputs = _imp->inputsQueue;
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

bool Node::hasOutputConnected() const
{
    return _imp->outputs.size() > 0;
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
    emit inputChanged(inputNumber);
    computeHash();
    return true;
}

void Node::connectOutput(boost::shared_ptr<Node> output)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(output);
    _imp->outputs.push_back(output);
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
        _imp->inputsQueue[inputNumber].reset();
    }
    emit inputChanged(inputNumber);
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
    
    std::list<boost::shared_ptr<Node> >::iterator it = std::find(_imp->outputs.begin(),_imp->outputs.end(),output);
    int ret = -1;
    if (it != _imp->outputs.end()) {
        ret = std::distance(_imp->outputs.begin(), it);
        _imp->outputs.erase(it);
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
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
        assert(*it);
        int inputNb = (*it)->disconnectInput(thisShared);
        _imp->deactivatedState.insert(make_pair(*it, inputNb));
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


//boost::shared_ptr<Image> Node::getImageBeingRendered(SequenceTime time,int view,unsigned int mipmaplevel) const
//{
//    QMutexLocker l(&_imp->imageBeingRenderedMutex);
//    ImagesMap::const_iterator it = _imp->imagesBeingRendered.find(ImageBeingRenderedKey(time,view,mipmaplevel));
//    if (it!=_imp->imagesBeingRendered.end()) {
//        return it->second;
//    }
//    return boost::shared_ptr<Image>();
//}
//
//void Node::addImageBeingRendered(boost::shared_ptr<Image> image,SequenceTime time,int view,unsigned int mipmaplevel )
//{
//    /*before rendering we add to the _imp->imagesBeingRendered member the image*/
//    ImageBeingRenderedKey renderedImageKey(time,view,mipmaplevel);
//    QMutexLocker locker(&_imp->imageBeingRenderedMutex);
//    _imp->imagesBeingRendered.insert(std::make_pair(renderedImageKey, image));
//}
//
//void Node::removeImageBeingRendered(SequenceTime time,int view,unsigned int mipmaplevel )
//{
//    /*now that we rendered the image, remove it from the images being rendered*/
//    
//    QMutexLocker locker(&_imp->imageBeingRenderedMutex);
//    ImageBeingRenderedKey renderedImageKey(time,view,mipmaplevel);
//    std::pair<ImagesMap::iterator,ImagesMap::iterator> it = _imp->imagesBeingRendered.equal_range(renderedImageKey);
//    assert(it.first != it.second);
//    _imp->imagesBeingRendered.erase(it.first);
//
//    _imp->imagesBeingRenderedNotEmpty.wakeOne(); // wake up any preview thread waiting for render to finish
//}

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

//    {
//        QMutexLocker locker(&_imp->imageBeingRenderedMutex);
//        while(!_imp->imagesBeingRendered.empty()){
//            _imp->imagesBeingRenderedNotEmpty.wait(&_imp->imageBeingRenderedMutex);
//        }
//    }
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
    ///MT-safe from EffectInstance
    _imp->liveInstance->togglePreview();
}

bool Node::isPreviewEnabled() const
{
     ///MT-safe from EffectInstance
    return _imp->liveInstance->isPreviewEnabled();
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
        refreshPreviewImage(getApp()->getTimeLine()->currentFrame());
    }
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
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

int Node::getMaskChannel() const
{
    if (!_imp->maskChannelKnob) {
        return -1;
    } else {
        return _imp->maskChannelKnob->getValue() - 1;
    }
}


bool Node::isMaskEnabled() const
{
    if (!_imp->enableMaskKnob) {
        return false;
    } else {
        return _imp->enableMaskKnob->getValue();
    }
}


bool Node::isMaskInverted() const
{
    if (!_imp->invertMaskKnob) {
        return false;
    } else {
        return _imp->invertMaskKnob->getValue();
    }
}

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

