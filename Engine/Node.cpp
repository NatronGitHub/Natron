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

using namespace Natron;
using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;

namespace { // protect local classes in anonymous namespace

/*A key to identify an image rendered for this node.*/
struct ImageBeingRenderedKey{

    ImageBeingRenderedKey():_time(0),_view(0){}

    ImageBeingRenderedKey(int time,int view):_time(time),_view(view){}

    SequenceTime _time;
    int _view;

    bool operator==(const ImageBeingRenderedKey& other) const {
        return _time == other._time &&
                _view == other._view;
    }

    bool operator<(const ImageBeingRenderedKey& other) const {
        return _time < other._time ||
                _view < other._view;
    }
};

typedef std::multimap<ImageBeingRenderedKey,boost::shared_ptr<Image> > ImagesMap;


/*The output node was connected from inputNumber to this...*/
typedef std::map<Node*,int > DeactivatedState;

}

struct Node::Implementation {
    Implementation(AppInstance* app_,LibraryBinary* plugin_)
        : app(app_)
        , outputs()
        , inputsMutex()
        , inputs()
        , inputsQueue()
        , liveInstance(NULL)
        , inputLabels()
        , name()
        , deactivatedState()
        , activatedMutex()
        , activated(true)
        , imageBeingRenderedMutex()
        , imagesBeingRenderedNotEmpty()
        , imagesBeingRendered()
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
        , masterNode(NULL)
    {
    }
    

    AppInstance* app; // pointer to the app: needed to access the application's default-project's format
    
    std::list<Node*> outputs; //< unlike the inputs we do not need a copy so the gui can modify it
                                //in a thread-safe manner because it is never read by the render thread.
    
    mutable QMutex inputsMutex; //< protects inputsMutex
    std::vector<Node*> inputs; //< Written to by the render thread once before rendering a frame.
    std::vector<Node*> inputsQueue; //< This is written to by the GUI only. Then the render thread copies this queue
                                    //to the inputs in a thread-safe manner.

    Natron::EffectInstance*  liveInstance; //< the effect hosted by this node


    mutable QMutex nameMutex;
    std::vector<std::string> inputLabels; // inputs name
    std::string name; //node name set by the user

    DeactivatedState deactivatedState;
    mutable QMutex activatedMutex;
    bool activated;
    
    QMutex imageBeingRenderedMutex;
    QWaitCondition imagesBeingRenderedNotEmpty; //to avoid computing preview in parallel of the real rendering

    ImagesMap imagesBeingRendered; //< a map storing the ongoing render for this node
    
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
    Node* masterNode;
    
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
    
}

void Node::load(const std::string& pluginID,const NodeSerialization& serialization,bool dontLoadName) {
    
    ///Called from the main thread. MT-safe
    assert(QThread::currentThread() == qApp->thread());
    
    ///cannot load twice
    assert(!_imp->liveInstance);
    
    if (!serialization.isNull() && !dontLoadName) {
        setName(serialization.getPluginLabel());
    }
    
    std::pair<bool,EffectBuilder> func = _imp->plugin->findFunction<EffectBuilder>("BuildEffect");
    if (func.first) {
        _imp->liveInstance = func.second(this);
        if (!serialization.isNull() && serialization.getPluginID() == pluginID &&
            majorVersion() == serialization.getPluginMajorVersion() && minorVersion() == serialization.getPluginMinorVersion()) {
            loadKnobs(serialization);
        }
    } else { //ofx plugin
        _imp->liveInstance = appPTR->createOFXEffect(pluginID,this,&serialization);
        _imp->liveInstance->initializeOverlayInteract();
    }
    
    initializeKnobs();
    initializeInputs();
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
            for (U32 i = 0; i < _imp->inputs.size();++i) {
                if (isInspector && isInspector->activeInput() != (int)i) {
                    continue;
                }
                if (_imp->inputs[i]) {
                    _imp->hash.append(_imp->inputs[i]->getHashValue());
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
    for (std::list<Node*>::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
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
    setKnobsAge(serialization.getKnobsAge());
}

void Node::restoreKnobsLinks(const NodeSerialization& serialization,const std::vector<Natron::Node*>& allNodes) {
    
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
    {
        QMutexLocker locker(&_imp->imageBeingRenderedMutex);
        _imp->imagesBeingRenderedNotEmpty.wakeAll();
        if (_imp->computingPreview) {
            QMutexLocker l(&_imp->mustQuitProcessingMutex);
            _imp->mustQuitProcessing = true;
            while (_imp->mustQuitProcessing) {
                _imp->mustQuitProcessingCond.wait(&_imp->mustQuitProcessingMutex);
            }
        }
        
    }
}

Node::~Node()
{
    delete _imp->liveInstance;
}

const std::vector<std::string>& Node::getInputLabels() const
{
    ///MT-safe as it never changes.
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->inputLabels;
}

const std::list<Natron::Node*>& Node::getOutputs() const
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
    int firstOptionalEmptyInput = -1;
    
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
                    if (firstOptionalEmptyInput == -1) {
                        firstOptionalEmptyInput = i;
                    }
                }
            }
        }
    }
    
    if (firstNonOptionalEmptyInput != -1) {
        return firstNonOptionalEmptyInput;
    } else if(firstOptionalEmptyInput != -1) {
        return firstOptionalEmptyInput;
    } else {
        return -1;
    }
}

void Node::getOutputsConnectedToThisNode(std::map<Node*,int>* outputs) {
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    for (std::list<Node*>::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
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

void Node::hasViewersConnected(std::list<ViewerInstance*>* viewers) const
{
    
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    if(pluginID() == "Viewer") {
        ViewerInstance* thisViewer = dynamic_cast<ViewerInstance*>(_imp->liveInstance);
        assert(thisViewer);
        std::list<ViewerInstance*>::const_iterator alreadyExists = std::find(viewers->begin(), viewers->end(), thisViewer);
        if(alreadyExists == viewers->end()){
            viewers->push_back(thisViewer);
        }
    } else {
        for (std::list<Node*>::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
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
                _imp->inputs[i] = NULL;
                _imp->inputsQueue[i] = NULL;
            }
            
        }
    }
    emit inputsInitialized();
}

Node* Node::input(int index) const
{
    
    ////Only called by the main-thread
    ////@see input_other_thread for the MT version
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->inputsMutex);
    if (index >= (int)_imp->inputsQueue.size() || index < 0) {
        return NULL;
    }
    return _imp->inputsQueue[index];
    
}

Node* Node::input_other_thread(int index) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if (index >= (int)_imp->inputs.size() || index < 0) {
        return NULL;
    }
    return _imp->inputs[index];
}

void Node::updateRenderInputs()
{
    QMutexLocker l(&_imp->inputsMutex);
    _imp->inputs = _imp->inputsQueue;
}

const std::vector<Natron::Node*>& Node::getInputs_other_thread() const
{
    QMutexLocker l(&_imp->inputsMutex);
    return _imp->inputs;
}

const std::vector<Natron::Node*>& Node::getInputs_mt_safe() const
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
        if (_imp->inputs[i] == input) {
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

bool Node::connectInput(Node* input,int inputNumber)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(input);
    
    if (!checkIfConnectingInputIsOk(input)) {
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

void Node::connectOutput(Node* output)
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
        _imp->inputsQueue[inputNumber] = NULL;
    }
    emit inputChanged(inputNumber);
    computeHash();
    return inputNumber;
}

int Node::disconnectInput(Node* input)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size(); ++i) {
            if (_imp->inputsQueue[i] == input) {
                _imp->inputsQueue[i] = NULL;
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

int Node::disconnectOutput(Node* output)
{

    assert(output);
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    std::list<Node*>::iterator it = std::find(_imp->outputs.begin(),_imp->outputs.end(),output);
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
            if (_imp->inputsQueue[i] == n) {
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
    Node* inputToConnectTo = 0;
    Node* firstOptionalInput = 0;
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
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size() ; ++i) {
            if(_imp->inputsQueue[i]) {
                _imp->inputsQueue[i]->disconnectOutput(this);
            }
        }
    }
    
    ///For each output node we remember that the output node  had its input number inputNb connected
    ///to this node
    for (std::list<Node*>::iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
        assert(*it);
        int inputNb = (*it)->disconnectInput(this);
        _imp->deactivatedState.insert(make_pair(*it, inputNb));
    }
    
    if (inputToConnectTo) {
        for (std::map<Node*,int >::iterator it = _imp->deactivatedState.begin();
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
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        ///for all inputs, reconnect their output to this node
        for (U32 i = 0; i < _imp->inputsQueue.size(); ++i){
            if (_imp->inputsQueue[i]) {
                _imp->inputsQueue[i]->connectOutput(this);
            }
        }
    }
    
    ///Restore all outputs that was connected to this node
    for (std::map<Node*,int >::iterator it = _imp->deactivatedState.begin();
         it!= _imp->deactivatedState.end(); ++it) {
        
        ///before connecting the outputs to this node, disconnect any link that has been made
        ///between the outputs by the user. This should normally never happen as the undo/redo
        ///stack follow always the same order.
        Node* outputHasInput = it->first->input(it->second);
        if (outputHasInput) {
            bool ok = getApp()->getProject()->disconnectNodes(outputHasInput, it->first);
            assert(ok);
        }

        ///and connect the output to this node
        it->first->connectInput(this, it->second);
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


boost::shared_ptr<Image> Node::getImageBeingRendered(SequenceTime time,int view) const
{
    QMutexLocker l(&_imp->imageBeingRenderedMutex);
    ImagesMap::const_iterator it = _imp->imagesBeingRendered.find(ImageBeingRenderedKey(time,view));
    if (it!=_imp->imagesBeingRendered.end()) {
        return it->second;
    }
    return boost::shared_ptr<Image>();
}

void Node::addImageBeingRendered(boost::shared_ptr<Image> image,SequenceTime time,int view )
{
    /*before rendering we add to the _imp->imagesBeingRendered member the image*/
    ImageBeingRenderedKey renderedImageKey(time,view);
    QMutexLocker locker(&_imp->imageBeingRenderedMutex);
    _imp->imagesBeingRendered.insert(std::make_pair(renderedImageKey, image));
}

void Node::removeImageBeingRendered(SequenceTime time,int view )
{
    /*now that we rendered the image, remove it from the images being rendered*/
    
    QMutexLocker locker(&_imp->imageBeingRenderedMutex);
    ImageBeingRenderedKey renderedImageKey(time,view);
    std::pair<ImagesMap::iterator,ImagesMap::iterator> it = _imp->imagesBeingRendered.equal_range(renderedImageKey);
    assert(it.first != it.second);
    _imp->imagesBeingRendered.erase(it.first);

    _imp->imagesBeingRenderedNotEmpty.wakeOne(); // wake up any preview thread waiting for render to finish
}

void Node::makePreviewImage(SequenceTime time,int width,int height,unsigned int* buf)
{

    {
        QMutexLocker locker(&_imp->imageBeingRenderedMutex);
        while(!_imp->imagesBeingRendered.empty()){
            _imp->imagesBeingRenderedNotEmpty.wait(&_imp->imageBeingRenderedMutex);
        }
    }
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
    Natron::Status stat = _imp->liveInstance->getRegionOfDefinition(time, &rod,&isProjectFormat);
    if (stat == StatFailed) {
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    }
    int h,w;
    h = rod.height() < height ? rod.height() : height;
    w = rod.width() < width ? rod.width() : width;
    double yZoomFactor = (double)h/(double)rod.height();
    double xZoomFactor = (double)w/(double)rod.width();

#ifdef NATRON_LOG
    Log::beginFunction(getName(),"makePreviewImage");
    Log::print(QString("Time "+QString::number(time)).toStdString());
#endif

    RenderScale scale;
    scale.x = scale.y = 1.;
    boost::shared_ptr<Image> img;

    // Exceptions are caught because the program can run without a preview,
    // but any exception in renderROI is probably fatal.
    try {
        img = _imp->liveInstance->renderRoI(EffectInstance::RenderRoIArgs(time, scale, 0,rod,false,true,false,NULL));
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
    
    for (int i=0; i < h; ++i) {
        double y = (double)i/yZoomFactor;
        int nearestY = std::floor(y+0.5);

        U32 *dst_pixels = buf + width*(h-1-i);
        const float* src_pixels = img->pixelAt(0, nearestY);
        
        for(int j = 0;j < w;++j) {
            
            double x = (double)j/xZoomFactor;
            int nearestX = std::floor(x+0.5);
            int r = Color::floatToInt<256>(Natron::Color::to_func_srgb(src_pixels[nearestX*4]));
            int g = Color::floatToInt<256>(Natron::Color::to_func_srgb(src_pixels[nearestX*4+1]));
            int b = Color::floatToInt<256>(Natron::Color::to_func_srgb(src_pixels[nearestX*4+2]));
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

void Node::drawOverlay()
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    ///MT-safe
    _imp->liveInstance->drawOverlay();
}

bool Node::onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->liveInstance->onOverlayPenDown(viewportPos, pos);
}

bool Node::onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->liveInstance->onOverlayPenMotion(viewportPos, pos);
}

bool Node::onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->liveInstance->onOverlayPenUp(viewportPos, pos);
}

bool Node::onOverlayKeyDown(Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->liveInstance->onOverlayKeyDown(key,modifiers);
}

bool Node::onOverlayKeyUp(Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->liveInstance->onOverlayKeyUp(key,modifiers);
}

bool Node::onOverlayKeyRepeat(Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->liveInstance->onOverlayKeyRepeat(key,modifiers);
}

bool Node::onOverlayFocusGained()
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->liveInstance->onOverlayFocusGained();
}

bool Node::onOverlayFocusLost()
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->liveInstance->onOverlayFocusLost();
}


bool Node::message(MessageType type,const std::string& content) const
{
    switch (type) {
        case INFO_MESSAGE:
            informationDialog(getName(), content);
            return true;
        case WARNING_MESSAGE:
            warningDialog(getName(), content);
            return true;
        case ERROR_MESSAGE:
            errorDialog(getName(), content);
            return true;
        case QUESTION_MESSAGE:
            return questionDialog(getName(), content) == Yes;
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


void Node::setInputFilesForReader(const QStringList& files) {
    _imp->liveInstance->setInputFilesForReader(files);
}

void Node::setOutputFilesForWriter(const QString& pattern) {
    _imp->liveInstance->setOutputFilesForWriter(pattern);
}

void Node::registerPluginMemory(size_t nBytes) {
    QMutexLocker l(&_imp->memoryUsedMutex);
    _imp->pluginInstanceMemoryUsed += nBytes;
    emit pluginMemoryUsageChanged((unsigned long long)_imp->pluginInstanceMemoryUsed);
}

void Node::unregisterPluginMemory(size_t nBytes) {
    QMutexLocker l(&_imp->memoryUsedMutex);
    _imp->pluginInstanceMemoryUsed -= nBytes;
    emit pluginMemoryUsageChanged((unsigned long long)_imp->pluginInstanceMemoryUsed);
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
    for (std::list<Node*>::iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
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
        Natron::Node* masterNode = effect->getNode();
        {
            QMutexLocker l(&_imp->masterNodeMutex);
            _imp->masterNode = masterNode;
        }
        QObject::connect(masterNode, SIGNAL(deactivated()), this, SLOT(onMasterNodeDeactivated()));
        QObject::connect(masterNode, SIGNAL(knobsAgeChanged(U64)), this, SLOT(setKnobsAge(U64)));
        QObject::connect(masterNode, SIGNAL(previewImageChanged(int)), this, SLOT(refreshPreviewImage(int)));
    } else {
        QObject::disconnect(_imp->masterNode, SIGNAL(deactivated()), this, SLOT(onMasterNodeDeactivated()));
        QObject::disconnect(_imp->masterNode, SIGNAL(knobsAgeChanged(U64)), this, SLOT(setKnobsAge(U64)));
        QObject::disconnect(_imp->masterNode, SIGNAL(previewImageChanged(int)), this, SLOT(refreshPreviewImage(int)));
        {
            QMutexLocker l(&_imp->masterNodeMutex);
            _imp->masterNode = NULL;
        }
    }
    
    emit slavedStateChanged(isSlave);

}

void Node::onMasterNodeDeactivated() {
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->liveInstance->unslaveAllKnobs();
}

Natron::Node* Node::getMasterNode() const {
    QMutexLocker l(&_imp->masterNodeMutex);
    return _imp->masterNode;
}

InspectorNode::InspectorNode(AppInstance* app,LibraryBinary* plugin)
    : Node(app,plugin)
    , _inputsCount(1)
    , _activeInput(0)
    , _activeInputMutex()
{}


InspectorNode::~InspectorNode(){
    
}

bool InspectorNode::connectInput(Node* input,int inputNumber) {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    ///cannot connect more than 10 inputs.
    assert(inputNumber <= 10);
    
    assert(input);
    
    if (!checkIfConnectingInputIsOk(input)) {
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
    int inputAlreadyConnected = inputIndex(input);
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
        QMutexLocker activeInputLocker(&_activeInputMutex);
        _activeInput = oldActiveInput;
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

int InspectorNode::disconnectInput(Node* input) {
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    return disconnectInput(inputIndex(input));
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

