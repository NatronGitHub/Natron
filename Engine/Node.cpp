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
#include "ofxNatron.h"

#include <limits>

#include <QtCore/QDebug>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>

#include <boost/bind.hpp>

#include <ofxNatron.h>

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


typedef std::list<Node::KnobLink> KnobLinkList ;
    
}

struct Node::Implementation {
    Implementation(AppInstance* app_,LibraryBinary* plugin_)
        : app(app_)
        , knobsInitialized(false)
        , inputsInitialized(false)
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
        , mustQuitPreview(false)
        , mustQuitPreviewMutex()
        , mustQuitPreviewCond()
        , perFrameMutexesLock()
        , renderInstancesFullySafePerFrameMutexes()
        , knobsAge(0)
        , knobsAgeMutex()
        , masterNodeMutex()
        , masterNode()
        , nodeLinks()
        , enableMaskKnob()
        , maskChannelKnob()
        , nodeSettingsPage()
        , nodeLabelKnob()
        , previewEnabledKnob()
        , disableNodeKnob()
        , rotoContext()
        , imagesBeingRenderedMutex()
        , imagesBeingRendered()
        , supportedDepths()
        , isMultiInstance(false)
        , multiInstanceParent(NULL)
        , multiInstanceParentName()
        , duringInputChangedAction(false)
        , keyframesDisplayedOnTimeline(false)
    {
    }
    
    void abortPreview();

    AppInstance* app; // pointer to the app: needed to access the application's default-project's format
    
    bool knobsInitialized;

    bool inputsInitialized;

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
    
    bool mustQuitPreview;
    QMutex mustQuitPreviewMutex;
    QWaitCondition mustQuitPreviewCond;
    
    QMutex renderInstancesSharedMutex; //< see INSTANCE_SAFE in EffectInstance::renderRoI
                                       //only 1 clone can render at any time
    
    QMutex perFrameMutexesLock; //< protects renderInstancesFullySafePerFrameMutexes
    std::map<int,boost::shared_ptr<QMutex> > renderInstancesFullySafePerFrameMutexes; //< see FULLY_SAFE in EffectInstance::renderRoI
                                                                   //only 1 render per frame
    
    U64 knobsAge; //< the age of the knobs in this effect. It gets incremented every times the liveInstance has its evaluate() function called.
    mutable QReadWriteLock knobsAgeMutex; //< protects knobsAge and hash
    Hash64 hash; //< recomputed everytime knobsAge is changed.
    
    mutable QMutex masterNodeMutex; //< protects masterNode and nodeLinks
    boost::shared_ptr<Node> masterNode; //< this points to the master when the node is a clone
    KnobLinkList nodeLinks; //< these point to the parents of the params links
  
    ///For each mask, the input number and the knob
    std::map<int,boost::shared_ptr<Bool_Knob> > enableMaskKnob;
    std::map<int,boost::shared_ptr<Choice_Knob> > maskChannelKnob;
    
    boost::shared_ptr<Page_Knob> nodeSettingsPage;
    boost::shared_ptr<String_Knob> nodeLabelKnob;
    boost::shared_ptr<Bool_Knob> previewEnabledKnob;
    boost::shared_ptr<Bool_Knob> disableNodeKnob;
    
    boost::shared_ptr<RotoContext> rotoContext; //< valid when the node has a rotoscoping context (i.e: paint context)
    
    mutable QMutex imagesBeingRenderedMutex;
    QWaitCondition imageBeingRenderedCond;
    std::list< boost::shared_ptr<Image> > imagesBeingRendered; ///< a list of all the images being rendered simultaneously
    
    std::list <Natron::ImageBitDepth> supportedDepths;
    
    ///True when several effect instances are represented under the same node.
    bool isMultiInstance;
    Natron::Node* multiInstanceParent;
    
    ///the name of the parent at the time this node was created
    std::string multiInstanceParentName;
    
    bool duringInputChangedAction; //< true if we're during onInputChanged(...). MT-safe since only modified by the main thread
    
    bool keyframesDisplayedOnTimeline;
    
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

void
Node::createRotoContextConditionnally()
{
    assert(!_imp->rotoContext);
    assert(_imp->liveInstance);
    ///Initialize the roto context if any
    if (isRotoNode()) {
        _imp->rotoContext.reset(new RotoContext(this));
    }
}

void
Node::load(const std::string& pluginID,
           const std::string& parentMultiInstanceName,
           int childIndex ,
           const boost::shared_ptr<Natron::Node>& thisShared,
           const NodeSerialization& serialization,
           bool dontLoadName)
{
    ///Called from the main thread. MT-safe
    assert(QThread::currentThread() == qApp->thread());
    
    ///cannot load twice
    assert(!_imp->liveInstance);
    
    
    bool nameSet = false;

    bool isMultiInstanceChild = false;
    if (!parentMultiInstanceName.empty()) {
        _imp->multiInstanceParentName = parentMultiInstanceName;
        
        ///Fetch the parent pointer ONLY when not loading (otherwise parent node might still node be created
        ///at that time)
        if (serialization.isNull()) {
            fetchParentMultiInstancePointer();
            setName(QString(parentMultiInstanceName.c_str()) + '_' + QString::number(childIndex));
            nameSet = true;
        }
        isMultiInstanceChild = true;
        _imp->isMultiInstance = false;
       
    }
    
    if (!serialization.isNull() && !dontLoadName && !nameSet) {
        setName(serialization.getPluginLabel().c_str());
        nameSet = true;
    }
    
    
    std::pair<bool,EffectBuilder> func = _imp->plugin->findFunction<EffectBuilder>("BuildEffect");
    if (func.first) {
        _imp->liveInstance = func.second(thisShared);
        assert(_imp->liveInstance);
        createRotoContextConditionnally();
    } else { //ofx plugin
        _imp->liveInstance = appPTR->createOFXEffect(pluginID,thisShared,&serialization);
        assert(_imp->liveInstance);
        _imp->liveInstance->initializeOverlayInteract();
    }
    
    _imp->liveInstance->addSupportedBitDepth(&_imp->supportedDepths);
    
    if (_imp->supportedDepths.empty()) {
        //From the spec:
        //The default for a plugin is to have none set, the plugin \em must define at least one in its describe action.
        throw std::runtime_error("Plug-in does not support 8bits, 16bits or 32bits floating point image processing.");
    }
    
    initializeInputs();
    initializeKnobs(serialization);

    ///Special case for trackers: set as multi instance
    if (isTrackerNode()) {
        _imp->isMultiInstance = true;
        ///declare knob that are instance specific
        boost::shared_ptr<KnobI> subLabelKnob = getKnobByName(kOfxParamStringSublabelName);
        if (subLabelKnob) {
            subLabelKnob->setAsInstanceSpecific();
        }
        
        boost::shared_ptr<KnobI> centerKnob = getKnobByName("center");
        if (centerKnob) {
            centerKnob->setAsInstanceSpecific();
        }
    }
    
    if (!nameSet) {
        getApp()->getProject()->initNodeCountersAndSetName(this);
        if (!isMultiInstanceChild && _imp->isMultiInstance) {
            updateEffectLabelKnob(getName().c_str());
        }
    }
    if (isMultiInstanceChild && serialization.isNull()) {
        assert(nameSet);
        updateEffectLabelKnob(QString(parentMultiInstanceName.c_str()) + '_' + QString::number(childIndex));

    }

    computeHash(); 
    assert(_imp->liveInstance);
}

void
Node::fetchParentMultiInstancePointer()
{
    std::vector<boost::shared_ptr<Node> > nodes = getApp()->getProject()->getCurrentNodes();
    for (U32 i = 0; i < nodes.size() ; ++i) {
        if (nodes[i]->getName() == _imp->multiInstanceParentName) {
            
            ///no need to store the boost pointer because the main instance lives the same time
            ///as the child
            _imp->multiInstanceParent = nodes[i].get();
            break;
        }
    }
}

bool
Node::isMultiInstance() const
{
    return _imp->isMultiInstance;
}

///Accessed by the serialization thread, but mt safe since never changed
std::string
Node::getParentMultiInstanceName() const
{
    if (_imp->multiInstanceParent) {
        return _imp->multiInstanceParent->getName_mt_safe();
    } else {
        return "";
    }
}

U64
Node::getHashValue() const
{
    QReadLocker l(&_imp->knobsAgeMutex);
    return _imp->hash.value();
}

void
Node::computeHash()
{    
    ///Always called in the main thread
    assert(QThread::currentThread() == qApp->thread());
    if (!_imp->inputsInitialized) {
        qDebug() << "Node::computeHash(): inputs not initialized";
    }

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
                        ///Add the index of the input to its hash.
                        ///Explanation: if we didn't add this, just switching inputs would produce a similar
                        ///hash.
                        _imp->hash.append(_imp->inputsQueue[i]->getHashValue() + i);
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

void
Node::loadKnobs(const NodeSerialization& serialization)
{
    ///Only called from the main thread
    assert(QThread::currentThread() == qApp->thread());
    assert(!_imp->knobsInitialized);

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

void
Node::loadKnob(const boost::shared_ptr<KnobI>& knob,
               const NodeSerialization& serialization)
{
    assert(!_imp->knobsInitialized);
    const NodeSerialization::KnobValues& knobsValues = serialization.getKnobsValues();
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it!=knobsValues.end();++it) {
        if((*it)->getName() == knob->getName()){
            // don't load the value if the Knob is not persistant! (it is just the default value in this case)
            if (knob->getIsPersistant()) {
                boost::shared_ptr<KnobI> serializedKnob = (*it)->getKnob();
                knob->clone(serializedKnob);
                knob->setSecret(serializedKnob->getIsSecret());
                if (knob->getDimension() == serializedKnob->getDimension()) {
                    for (int i = 0; i < knob->getDimension();++i) {
                        knob->setEnabled(i, serializedKnob->isEnabled(i));
                    }
                }
            }
            break;
        }
    }
}

void
Node::restoreKnobsLinks(const NodeSerialization& serialization,
                        const std::vector<boost::shared_ptr<Natron::Node> >& allNodes)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    const NodeSerialization::KnobValues& knobsValues = serialization.getKnobsValues();
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it!=knobsValues.end();++it) {
        boost::shared_ptr<KnobI> knob = getKnobByName((*it)->getName());
        if (!knob) {
            qDebug() << "Couldn't find a knob named " << (*it)->getName().c_str();
            continue;
        }
        (*it)->restoreKnobLinks(knob,allNodes);
        (*it)->restoreTracks(knob,allNodes);
    }
}

void
Node::setKnobsAge(U64 newAge)
{
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

void
Node::incrementKnobsAge()
{
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

U64
Node::getKnobsAge() const
{
    QReadLocker l(&_imp->knobsAgeMutex);
    return _imp->knobsAge;
}

bool
Node::isRenderingPreview() const
{
    QMutexLocker l(&_imp->computingPreviewMutex);
    return _imp->computingPreview;
}

void
Node::Implementation::abortPreview()
{
    bool computing;
    {
        QMutexLocker locker(&computingPreviewMutex);
        computing = computingPreview;
    }
    if (computing) {
        QMutexLocker l(&mustQuitPreviewMutex);
        mustQuitPreview = true;
        while (mustQuitPreview) {
            mustQuitPreviewCond.wait(&mustQuitPreviewMutex);
        }
    }
    

}

void
Node::abortAnyProcessing()
{
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>(getLiveInstance());
    if (isOutput) {
        isOutput->getVideoEngine()->abortRendering(true);
    }
    _imp->abortPreview();
}

void
Node::quitAnyProcessing() {
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>(getLiveInstance());
    if (isOutput) {
        isOutput->getVideoEngine()->quitEngineThread();
    }
    _imp->abortPreview();
    
}

Node::~Node()
{
    if (_imp->liveInstance) {
        delete _imp->liveInstance;
    }
}

void
Node::removeReferences()
{
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>(_imp->liveInstance);
    if (isOutput && isOutput->getVideoEngine()->isThreadRunning()) {
        isOutput->getVideoEngine()->quitEngineThread();
    }
    delete _imp->liveInstance;
    _imp->liveInstance = 0;
}

const std::vector<std::string>&
Node::getInputLabels() const
{
    assert(_imp->inputsInitialized);
    ///MT-safe as it never changes.
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->inputLabels;
}

const std::list<boost::shared_ptr<Natron::Node> >&
Node::getOutputs() const
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    return _imp->outputsQueue;
}

std::vector<std::string>
Node::getInputNames() const
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

int
Node::getPreferredInputForConnection() const
{
    assert(QThread::currentThread() == qApp->thread());
    if (getMaxInputCount() == 0) {
        return -1;
    }
    
    ///we return the first non-optional empty input
    int firstNonOptionalEmptyInput = -1;
    std::list<int> optionalEmptyInputs;
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size() ; ++i) {
            if (!_imp->inputsQueue[i]) {
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

void
Node::getOutputsConnectedToThisNode(std::map<boost::shared_ptr<Node>,int>* outputs)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->outputsQueue.begin(); it != _imp->outputsQueue.end(); ++it) {
        assert(*it);
        int indexOfThis = (*it)->inputIndex(this);
        assert(indexOfThis != -1);
        outputs->insert(std::make_pair(*it, indexOfThis));
    }
}

const std::string&
Node::getName() const
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->nameMutex);
    return _imp->name;
}

std::string
Node::getName_mt_safe() const
{
    QMutexLocker l(&_imp->nameMutex);
    return _imp->name;
}

void
Node::setName(const QString& name)
{
    {
        QMutexLocker l(&_imp->nameMutex);
        _imp->name = name.toStdString();
    }
    emit nameChanged(name);
}

AppInstance*
Node::getApp() const
{
    return _imp->app;
}

bool
Node::isActivated() const
{
    QMutexLocker l(&_imp->activatedMutex);
    return _imp->activated;
}


void
Node::initializeKnobs(const NodeSerialization& serialization)
{
    ////Only called by the main-thread
    _imp->liveInstance->blockEvaluation();
    
    assert(QThread::currentThread() == qApp->thread());
    assert(!_imp->knobsInitialized);
    _imp->liveInstance->initializeKnobsPublic();
    
    ///If the effect has a mask, add additionnal mask controls
    int inputsCount = getMaxInputCount();
    for (int i = 0; i < inputsCount; ++i) {
        if (_imp->liveInstance->isInputMask(i) && !_imp->liveInstance->isInputRotoBrush(i)) {
            std::string maskName = _imp->liveInstance->getInputLabel(i);
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
            
            
            ///and load it
            loadKnob(enableMaskKnob, serialization);
            loadKnob(maskChannelKnob, serialization);
        }
    }
    
    _imp->nodeSettingsPage = Natron::createKnob<Page_Knob>(_imp->liveInstance, NATRON_EXTRA_PARAMETER_PAGE_NAME,1,false);
    
    _imp->nodeLabelKnob = Natron::createKnob<String_Knob>(_imp->liveInstance, "Label",1,false);
    assert(_imp->nodeLabelKnob);
    _imp->nodeLabelKnob->setName("label_natron");
    _imp->nodeLabelKnob->setAnimationEnabled(false);
    _imp->nodeLabelKnob->setEvaluateOnChange(false);
    _imp->nodeLabelKnob->setAsMultiLine();
    _imp->nodeLabelKnob->setUsesRichText(true);
    _imp->nodeLabelKnob->setHintToolTip("This label gets appended to the node name on the node graph.");
    _imp->nodeSettingsPage->addKnob(_imp->nodeLabelKnob);
    loadKnob(_imp->nodeLabelKnob, serialization);
    
    _imp->previewEnabledKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance, "Preview enabled",1,false);
    assert(_imp->previewEnabledKnob);
    _imp->previewEnabledKnob->setDefaultValue(makePreviewByDefault());
    _imp->previewEnabledKnob->setName("preview_enabled_natron");
    _imp->previewEnabledKnob->setAnimationEnabled(false);
    _imp->previewEnabledKnob->turnOffNewLine();
    _imp->previewEnabledKnob->setIsPersistant(false);
    _imp->previewEnabledKnob->setEvaluateOnChange(false);
    _imp->previewEnabledKnob->setHintToolTip("Whether to show a preview on the node box in the node-graph.");
    _imp->nodeSettingsPage->addKnob(_imp->previewEnabledKnob);
    
    _imp->disableNodeKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance, "Disable",1,false);
    assert(_imp->disableNodeKnob);
    _imp->disableNodeKnob->setAnimationEnabled(false);
    _imp->disableNodeKnob->setDefaultValue(false);
    _imp->disableNodeKnob->setName(kDisableNodeKnobName);
    _imp->disableNodeKnob->setHintToolTip("When disabled, this node acts as a pass through.");
    _imp->nodeSettingsPage->addKnob(_imp->disableNodeKnob);
    loadKnob(_imp->disableNodeKnob, serialization);

    _imp->knobsInitialized = true;
    _imp->liveInstance->unblockEvaluation();
    emit knobsInitialized();
}

void
Node::beginEditKnobs()
{
    _imp->liveInstance->beginEditKnobs();
}

void
Node::createKnobDynamically()
{
    emit knobsInitialized();
}


void
Node::setLiveInstance(Natron::EffectInstance* liveInstance)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->liveInstance = liveInstance;
}

Natron::EffectInstance*
Node::getLiveInstance() const
{
    ///Thread safe as it never changes
    return _imp->liveInstance;
}

bool
Node::hasEffect() const
{
    return _imp->liveInstance != NULL;
}

void
Node::hasViewersConnected(std::list<ViewerInstance* >* viewers) const
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

void
Node::hasWritersConnected(std::list<Natron::OutputEffectInstance* >* writers) const
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

int
Node::getMajorVersion() const
{
     ///Thread safe as it never changes
    return _imp->liveInstance->getMajorVersion();
}

int
Node::getMinorVersion() const
{
     ///Thread safe as it never changes
    return _imp->liveInstance->getMinorVersion();
}

void
Node::initializeInputs()
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    int oldCount = (int)_imp->inputs.size();
    int inputCount = getMaxInputCount();
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        _imp->inputs.resize(inputCount);
        _imp->inputsQueue.resize(inputCount);
        _imp->inputLabels.resize(inputCount);
        ///if we added inputs, just set to NULL the new inputs, and add their label to the labels map
        if (inputCount > oldCount) {
            for (int i = oldCount ; i < inputCount; ++i) {
                _imp->inputLabels[i] = _imp->liveInstance->getInputLabel(i);
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
    _imp->inputsInitialized = true;
    emit inputsInitialized();
}

boost::shared_ptr<Node>
Node::input(int index) const
{
    ////Only called by the main-thread
    ////@see input_other_thread for the MT version
    assert(QThread::currentThread() == qApp->thread());
    

    if (_imp->multiInstanceParent) {
        return _imp->multiInstanceParent->input(index);
    }
    if (!_imp->inputsInitialized) {
        qDebug() << "Node::input(): inputs not initialized";
    }
    QMutexLocker l(&_imp->inputsMutex);
    if (index >= (int)_imp->inputsQueue.size() || index < 0) {
        return boost::shared_ptr<Node>();
    }

    return _imp->inputsQueue[index];
}

boost::shared_ptr<Node>
Node::input_other_thread(int index) const
{
    assert(_imp->inputsInitialized);

    if (_imp->multiInstanceParent) {
        return _imp->multiInstanceParent->input_other_thread(index);
    }
    QMutexLocker l(&_imp->inputsMutex);
    if (index >= (int)_imp->inputs.size() || index < 0) {
        return boost::shared_ptr<Node>();
    }
    return _imp->inputs[index];
}

void
Node::updateRenderInputs()
{
    assert(_imp->inputsInitialized);

    {
        QMutexLocker l(&_imp->inputsMutex);
        _imp->inputs = _imp->inputsQueue;
    }
    {
        QMutexLocker l(&_imp->outputsMutex);
        _imp->outputs = _imp->outputsQueue;
    }
}

void
Node::updateRenderInputsRecursive()
{
    assert(_imp->inputsInitialized);

    updateRenderInputs();
    for (std::vector<boost::shared_ptr<Node> >::iterator it = _imp->inputsQueue.begin(); it!=_imp->inputsQueue.end(); ++it) {
        if ((*it)) {
            (*it)->updateRenderInputs();
        }
    }
}

const std::vector<boost::shared_ptr<Natron::Node> >&
Node::getInputs_other_thread() const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsMutex);
    if (_imp->multiInstanceParent) {
        return _imp->multiInstanceParent->getInputs_other_thread();
    }
    return _imp->inputs;
}

const std::vector<boost::shared_ptr<Natron::Node> >&
Node::getInputs_mt_safe() const
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->inputsInitialized);

    if (_imp->multiInstanceParent) {
        return _imp->multiInstanceParent->getInputs_mt_safe();
    }
    return _imp->inputsQueue;
}

std::string
Node::getInputLabel(int inputNb) const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsMutex);
    if (inputNb < 0 || inputNb >= (int)_imp->inputLabels.size()) {
        throw std::invalid_argument("Index out of range");
    }
    return _imp->inputLabels[inputNb];
}


bool
Node::isInputConnected(int inputNb) const
{
    assert(_imp->inputsInitialized);

    return input(inputNb) != NULL;
}

bool
Node::hasInputConnected() const
{
    assert(_imp->inputsInitialized);

    if (_imp->multiInstanceParent) {
        return _imp->multiInstanceParent->hasInputConnected();
    }
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

bool
Node::hasOutputConnected() const
{
    ////Only called by the main-thread
    if (_imp->multiInstanceParent) {
        return _imp->multiInstanceParent->hasInputConnected();
    }
    if (QThread::currentThread() == qApp->thread()) {
        return _imp->outputsQueue.size() > 0;
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        return _imp->outputs.size() > 0;
    }
}

bool
Node::checkIfConnectingInputIsOk(Natron::Node* input) const
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

void
Node::isNodeUpstream(const Natron::Node* input,bool* ok) const
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    if (!input) {
        *ok = false;
        return;
    }
    
    QMutexLocker l(&_imp->inputsMutex);
    
    for (U32 i = 0; i  < _imp->inputsQueue.size(); ++i) {
        if (_imp->inputsQueue[i].get() == input) {
            *ok = true;
            return;
        }
    }
    *ok = false;
    for (U32 i = 0; i  < _imp->inputsQueue.size(); ++i) {
        if (_imp->inputsQueue[i]) {
            _imp->inputsQueue[i]->isNodeUpstream(input, ok);
            if (*ok) {
                return;
            }
        }
    }
}

bool
Node::connectInput(boost::shared_ptr<Node> input,int inputNumber)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->inputsInitialized);
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

void
Node::switchInput0And1()
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->inputsInitialized);
    int maxInputs = getMaxInputCount();
    if (maxInputs < 2) {
        return;
    }
    ///get the first input number to switch
    int inputAIndex = -1;
    for (int i = 0; i < maxInputs; ++i) {
        if (!_imp->liveInstance->isInputMask(i)) {
            inputAIndex = i;
            break;
        }
    }
    
    ///There's only a mask ??
    if (inputAIndex == -1) {
        return;
    }
    
    ///get the second input number to switch
    int inputBIndex = -1;
    int firstMaskInput = -1;
    for (int j = 0 ; j < maxInputs; ++j) {
        if (j == inputAIndex) {
            continue;
        }
        if (!_imp->liveInstance->isInputMask(j)) {
            inputBIndex = j;
            break;
        } else {
            firstMaskInput = j;
        }
    }
    if (inputBIndex == -1) {
        ///if there's a mask use it as input B for the switch
        if (firstMaskInput != -1) {
            inputBIndex = firstMaskInput;
            
        } else {
            ///there's only 1 input
            return;
        }
    }
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        assert(inputAIndex < (int)_imp->inputsQueue.size() && inputBIndex < (int)_imp->inputsQueue.size());
        boost::shared_ptr<Natron::Node> input0 = _imp->inputsQueue[inputAIndex];
        _imp->inputsQueue[inputAIndex] = _imp->inputsQueue[inputBIndex];
        _imp->inputsQueue[inputBIndex] = input0;
    }
    emit inputChanged(inputAIndex);
    emit inputChanged(inputBIndex);
    onInputChanged(inputAIndex);
    onInputChanged(inputBIndex);
    computeHash();
}

void
Node::onInputNameChanged(const QString& name)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->inputsInitialized);
    Natron::Node* inp = dynamic_cast<Natron::Node*>(sender());
    assert(inp);
    if (!inp) {
        return;
    }
    int inputNb = -1;
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputsQueue.size(); ++i) {
            if (_imp->inputsQueue[i].get() == inp) {
                inputNb = i;
                break;
            }
        }
    }
    if (inputNb != - 1) {
        emit inputNameChanged(inputNb, name);
    }
}

void
Node::connectOutput(boost::shared_ptr<Node> output)
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

int
Node::disconnectInput(int inputNumber)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->inputsInitialized);

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

int
Node::disconnectInput(boost::shared_ptr<Node> input)
{
    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->inputsInitialized);
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

int
Node::disconnectOutput(boost::shared_ptr<Node> output)
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

int
Node::inputIndex(Node* n) const
{
    if (!n) {
        return -1;
    }
    
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->inputsInitialized);
    if (_imp->multiInstanceParent) {
        return _imp->multiInstanceParent->inputIndex(n);
    }
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

void
Node::clearLastRenderedImage()
{
    _imp->liveInstance->clearLastRenderedImage();
}

/*After this call this node still knows the link to the old inputs/outputs
 but no other node knows this node.*/
void
Node::deactivate(const std::list< boost::shared_ptr<Natron::Node> >& outputsToDisconnect,
                 bool disconnectAll,
                 bool reconnect,
                 bool hideGui,
                 bool triggerRender)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    if (!_imp->liveInstance) {
        return;
    }

    //first tell the gui to clear any persistent message linked to this node
    clearPersistentMessage();

    ///For all knobs that have listeners, kill expressions
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        std::list<KnobI*> listeners;
        knobs[i]->getListeners(listeners);
        for (std::list<KnobI*>::iterator it = listeners.begin(); it!=listeners.end(); ++it) {
            for (int dim = 0; dim < (*it)->getDimension(); ++dim) {
                std::pair<int, boost::shared_ptr<KnobI> > master = (*it)->getMaster(dim);
                if (master.second == knobs[i]) {
                    (*it)->unSlave(dim, true);
                }
            }
        }
    }
    
    ///if the node has 1 non-optional input, attempt to connect the outputs to the input of the current node
    ///this node is the node the outputs should attempt to connect to
    boost::shared_ptr<Node> inputToConnectTo;
    boost::shared_ptr<Node> firstOptionalInput;
    int firstNonOptionalInput = -1;
    if (reconnect) {
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
    }
    /*Removing this node from the output of all inputs*/
    _imp->deactivatedState.clear();
    
    boost::shared_ptr<Natron::Node> thisShared = getApp()->getProject()->getNodePointer(this);
    assert(thisShared);
    std::vector<boost::shared_ptr<Node> > inputsQueueCopy;
    {
        QMutexLocker l(&_imp->inputsMutex);
        inputsQueueCopy = _imp->inputsQueue;
    }
    for (U32 i = 0; i < inputsQueueCopy.size() ; ++i) {
        if(inputsQueueCopy[i]) {
            inputsQueueCopy[i]->disconnectOutput(thisShared);
        }
    }
    
    
    ///For each output node we remember that the output node  had its input number inputNb connected
    ///to this node
    std::list<boost::shared_ptr<Node> > outputsQueueCopy;
    {
        QMutexLocker l(&_imp->outputsMutex);
        outputsQueueCopy = _imp->outputsQueue;
    }
    
    
    for (std::list<boost::shared_ptr<Node> >::iterator it = outputsQueueCopy.begin(); it!=outputsQueueCopy.end(); ++it) {
        assert(*it);
        bool dc;
        if (disconnectAll) {
            dc = true;
        } else {
            std::list<boost::shared_ptr<Node> >::const_iterator found =
            std::find(outputsToDisconnect.begin(), outputsToDisconnect.end(), *it);
            dc = found != outputsToDisconnect.end();
        }
        if (dc) {
            int inputNb = (*it)->disconnectInput(thisShared);
            _imp->deactivatedState.insert(make_pair(*it, inputNb));
            
            ///reconnect if inputToConnectTo is not null
            if (inputToConnectTo) {
                getApp()->getProject()->connectNodes(inputNb, inputToConnectTo, *it);
            }
        }
    }
    
    
    ///kill any thread it could have started (e.g: VideoEngine or preview)
    ///Commented-out: If we were to undo the deactivate we don't want all threads to be
    ///exited, just exit them when the effect is really deleted instead
    //quitAnyProcessing();
    abortAnyProcessing();
    
    ///Free all memory used by the plug-in.
    _imp->liveInstance->clearPluginMemoryChunks();
    clearLastRenderedImage();
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->liveInstance);
    if (isViewer) {
        isViewer->clearLastRenderedTexture();
    }
    
    if (hideGui) {
        emit deactivated(triggerRender);
    }
    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = false;
    }
    
}

void
Node::activate(const std::list< boost::shared_ptr<Natron::Node> >& outputsToRestore,
               bool restoreAll,bool triggerRender)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    if (!_imp->liveInstance) {
        return;
    }
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
        
        bool restore;
        if (restoreAll) {
            restore = true;
        } else {
            std::list<boost::shared_ptr<Node> >::const_iterator found =
            std::find(outputsToRestore.begin(), outputsToRestore.end(), it->first);
            restore = found != outputsToRestore.end();
        }
        
        if (restore) {
            ///before connecting the outputs to this node, disconnect any link that has been made
            ///between the outputs by the user. This should normally never happen as the undo/redo
            ///stack follow always the same order.
            boost::shared_ptr<Node> outputHasInput = it->first->input(it->second);
            if (outputHasInput) {
                bool ok = getApp()->getProject()->disconnectNodes(outputHasInput, it->first);
                assert(ok);
                (void)ok;
            }
            
            ///and connect the output to this node
            it->first->connectInput(thisShared, it->second);
        }
    }
    
    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = true; //< flag it true before notifying the GUI because the gui rely on this flag (espcially the Viewer)
    }
    emit activated(triggerRender);
}


boost::shared_ptr<KnobI>
Node::getKnobByName(const std::string& name) const
{
    ///MT-safe, never changes
    assert(_imp->knobsInitialized);
    return _imp->liveInstance->getKnobByName(name);
}


namespace {

///output is always RGBA with alpha = 255
template<typename PIX,int maxValue>
void renderPreview(const Natron::Image& srcImg,
                       int elemCount,
                       int *dstWidth,
                       int *dstHeight,
                       bool convertToSrgb,
                       unsigned int* dstPixels)
{
    ///recompute it after the rescaling
    const RectI& srcBounds = srcImg.getBounds();
    double yZoomFactor = *dstHeight/(double)srcBounds.height();
    double xZoomFactor = *dstWidth/(double)srcBounds.width();
    double zoomFactor;
    if (xZoomFactor < yZoomFactor) {
        zoomFactor = xZoomFactor;
        *dstHeight = srcBounds.height() * zoomFactor;
    } else {
        zoomFactor = yZoomFactor;
		*dstWidth = srcBounds.width() * zoomFactor;
    }
    assert(elemCount >= 3);

    for (int i = 0; i < *dstHeight; ++i) {
        double y = (i - *dstHeight/2.)/zoomFactor + (srcBounds.y1 + srcBounds.y2)/2.;
        int yi = std::floor(y+0.5);

        U32 *dst_pixels = dstPixels + *dstWidth * (*dstHeight-1-i);
        
        const PIX* src_pixels = (const PIX*)srcImg.pixelAt(srcBounds.x1, yi);
        if (!src_pixels) {
            // out of bounds
            for (int j = 0; j < *dstWidth; ++j) {
#ifndef __NATRON_WIN32__
				dst_pixels[j] = toBGRA(0, 0, 0, 0);
#else
				dst_pixels[j] = toBGRA(0, 0, 0, 255);
#endif
            }
        } else {
            for (int j = 0; j < *dstWidth; ++j) {
                // bilinear interpolation is pointless when downscaling a lot, and this is a preview anyway.
                // just use nearest neighbor
                double x = (j- *dstWidth/2.)/zoomFactor + (srcBounds.x1 + srcBounds.x2)/2.;

                int xi = std::floor(x+0.5); // round to nearest
                if (xi < 0 || xi >=(srcBounds.x2-srcBounds.x1)) {
#ifndef __NATRON_WIN32__
					dst_pixels[j] = toBGRA(0, 0, 0, 0);
#else
					 dst_pixels[j] = toBGRA(0, 0, 0, 255);
#endif
                   
                } else {
                    float rFilt = src_pixels[xi * elemCount + 0]/(float)maxValue;
                    float gFilt = src_pixels[xi * elemCount + 1]/(float)maxValue;
                    float bFilt = src_pixels[xi * elemCount + 2]/(float)maxValue;

                    int r = Color::floatToInt<256>(convertToSrgb ? Natron::Color::to_func_srgb(rFilt) : rFilt);
                    int g = Color::floatToInt<256>(convertToSrgb ? Natron::Color::to_func_srgb(gFilt) : gFilt);
                    int b = Color::floatToInt<256>(convertToSrgb ? Natron::Color::to_func_srgb(bFilt) : bFilt);
                    dst_pixels[j] = toBGRA(r, g, b, 255);
                }
            }
        }
    }

}

}

void
Node::makePreviewImage(SequenceTime time,
                       int *width,
                       int *height,
                       unsigned int* buf)
{
    assert(_imp->knobsInitialized);

    {
        QMutexLocker locker(&_imp->mustQuitPreviewMutex);
        if (_imp->mustQuitPreview) {
            _imp->mustQuitPreview = false;
            _imp->mustQuitPreviewCond.wakeOne();
            return;
        }
    }
  
    QMutexLocker locker(&_imp->computingPreviewMutex); /// prevent 2 previews to occur at the same time since there's only 1 preview instance
    _imp->computingPreview = true;
    
    RectD rod;
    bool isProjectFormat;
    RenderScale scale;
    scale.x = scale.y = 1.;
    Natron::Status stat = _imp->liveInstance->getRegionOfDefinition_public(time, scale, 0, &rod, &isProjectFormat);
    if (stat == StatFailed || rod.isNull()) {
        _imp->computingPreview = false;
        _imp->computingPreviewCond.wakeOne();
        return;
    }
    assert(!rod.isNull());
    double yZoomFactor = (double)*height/(double)rod.height();
    double xZoomFactor = (double)*width/(double)rod.width();
    
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
    
    RectI renderWindow;
    rod.toPixelEnclosing(mipMapLevel, &renderWindow);
    // Exceptions are caught because the program can run without a preview,
    // but any exception in renderROI is probably fatal.
    try {
        img = _imp->liveInstance->renderRoI(EffectInstance::RenderRoIArgs(time,
                                                                          scale,
                                                                          mipMapLevel,
                                                                          0, //< preview only renders view 0 (left)
                                                                          renderWindow,
                                                                          false,
                                                                          true,
                                                                          false,
                                                                          rod,
                                                                          Natron::ImageComponentRGB,
                                                                          getBitDepth())); //< preview is always rgb...
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

    ImageComponents components = img->getComponents();
    int elemCount = getElementsCountForComponents(components);

    ///we convert only when input is Linear.
    //Rec709 and srGB is acceptable for preview
    bool convertToSrgb = getApp()->getDefaultColorSpaceForBitDepth(img->getBitDepth()) == Natron::Linear;
    
    switch (img->getBitDepth()) {
        case Natron::IMAGE_BYTE: {
            renderPreview<unsigned char, 255>(*img, elemCount, width, height,convertToSrgb, buf);
        } break;
        case Natron::IMAGE_SHORT: {
            renderPreview<unsigned short, 65535>(*img, elemCount, width, height,convertToSrgb, buf);
        } break;
        case Natron::IMAGE_FLOAT: {
            renderPreview<float, 1>(*img, elemCount, width, height,convertToSrgb, buf);
        } break;
        case Natron::IMAGE_NONE:
            break;
    }
    
    _imp->computingPreview = false;
    _imp->computingPreviewCond.wakeOne();
#ifdef NATRON_LOG
    Log::endFunction(getName(),"makePreviewImage");
#endif
}

bool
Node::isInputNode() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->isGenerator();
}


bool
Node::isOutputNode() const
{   ///MT-safe, never changes
    return _imp->liveInstance->isOutput();
}


bool
Node::isOpenFXNode() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->isOpenFX();
}

bool
Node::isRotoNode() const
{
    ///Runs only in the main thread (checked by getName())
    ///Crude way to distinguish between Rotoscoping and Rotopainting nodes.
    QString name = getPluginID().c_str();
    return name.contains("roto",Qt::CaseInsensitive);
}

/**
 * @brief Returns true if the node is a rotopaint node
 **/
bool
Node::isRotoPaintingNode() const
{
    ///Runs only in the main thread (checked by getName())
    QString name = getPluginID().c_str();
    return name.contains("rotopaint",Qt::CaseInsensitive);
}

boost::shared_ptr<RotoContext>
Node::getRotoContext() const
{
    return _imp->rotoContext;
}

const std::vector<boost::shared_ptr<KnobI> >&
Node::getKnobs() const
{
    ///MT-safe from EffectInstance::getKnobs()
    return _imp->liveInstance->getKnobs();
}

std::string
Node::getPluginID() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->getPluginID();
}

std::string
Node::getPluginLabel() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->getPluginLabel();
}

std::string
Node::getDescription() const
{
    ///MT-safe, never changes
    return _imp->liveInstance->getDescription();
}

int
Node::getMaxInputCount() const
{
    ///MT-safe, never changes
    assert(_imp->liveInstance);
    return _imp->liveInstance->getMaxInputCount();
}

bool
Node::makePreviewByDefault() const
{
    ///MT-safe, never changes
    assert(_imp->liveInstance);
    return _imp->liveInstance->makePreviewByDefault();
}

void
Node::togglePreview()
{
    ///MT-safe from Knob
    assert(_imp->knobsInitialized);
    assert(_imp->previewEnabledKnob);
    _imp->previewEnabledKnob->setValue(!_imp->previewEnabledKnob->getValue(),0);
}

bool
Node::isPreviewEnabled() const
{
    ///MT-safe from EffectInstance
    if (!_imp->knobsInitialized) {
        qDebug() << "Node::isPreviewEnabled(): knobs not initialized (including previewEnabledKnob)";
    }
    if (_imp->previewEnabledKnob) {
        return _imp->previewEnabledKnob->getValue();
    }
    return false;
}

bool
Node::aborted() const
{
    ///MT-safe from EffectInstance
    assert(_imp->liveInstance);
    return _imp->liveInstance->aborted();
}

void
Node::setAborted(bool b)
{
    ///MT-safe from EffectInstance
    assert(_imp->liveInstance);
    _imp->liveInstance->setAborted(b);
}

bool
Node::message(MessageType type,
              const std::string& content) const
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

void
Node::setPersistentMessage(MessageType type,
                           const std::string& content)
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

void
Node::clearPersistentMessage()
{
    if(!appPTR->isBackground()){
        emit persistentMessageCleared();
    }
}



void
Node::purgeAllInstancesCaches()
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    assert(_imp->liveInstance);
    _imp->liveInstance->purgeCaches();
}

void
Node::notifyInputNIsRendering(int inputNb)
{
    emit inputNIsRendering(inputNb);
}

void
Node::notifyInputNIsFinishedRendering(int inputNb)
{
    emit inputNIsFinishedRendering(inputNb);
}

void
Node::notifyRenderingStarted()
{
    emit renderingStarted();
}

void
Node::notifyRenderingEnded()
{
    emit renderingEnded();
}

void
Node::setOutputFilesForWriter(const std::string& pattern)
{
    assert(_imp->liveInstance);
    _imp->liveInstance->setOutputFilesForWriter(pattern);
}

void
Node::registerPluginMemory(size_t nBytes)
{
    {
        QMutexLocker l(&_imp->memoryUsedMutex);
        _imp->pluginInstanceMemoryUsed += nBytes;
    }
    emit pluginMemoryUsageChanged(nBytes);
}

void
Node::unregisterPluginMemory(size_t nBytes)
{
    {
        QMutexLocker l(&_imp->memoryUsedMutex);
        _imp->pluginInstanceMemoryUsed -= nBytes;
    }
    emit pluginMemoryUsageChanged(-nBytes);
}

QMutex&
Node::getRenderInstancesSharedMutex()
{
    return _imp->renderInstancesSharedMutex;
}

QMutex&
Node::getFrameMutex(int time)
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

void
Node::refreshPreviewsRecursively()
{
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


void
Node::onAllKnobsSlaved(bool isSlave,KnobHolder* master)
{
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
    
    emit allKnobsSlaved(isSlave);

}

void
Node::onKnobSlaved(const boost::shared_ptr<KnobI>& knob,
                   int dimension,
                   bool isSlave,
                   KnobHolder* master)
{
    ///ignore the call if the node is a clone
    {
        QMutexLocker l(&_imp->masterNodeMutex);
        if (_imp->masterNode) {
            return;
        }
    }
    
    ///If the holder isn't an effect, ignore it too
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(master);
    if (!isEffect) {
        return;
    }
    boost::shared_ptr<Natron::Node> parentNode  = isEffect->getNode();
    bool changed = false;
    {   QMutexLocker l(&_imp->masterNodeMutex);
        KnobLinkList::iterator found = _imp->nodeLinks.end();
        for (KnobLinkList::iterator it = _imp->nodeLinks.begin(); it!= _imp->nodeLinks.end(); ++it) {
            if (it->masterNode == parentNode) {
                found = it;
                break;
            }
        }
        
        if (found == _imp->nodeLinks.end()) {
            if (!isSlave) {
                ///We want to unslave from the given node but the link didn't existed, just return
                return;
            } else {
                ///Add a new link
                KnobLink link;
                link.masterNode = parentNode;
                link.knob = knob;
                link.dimension = dimension;
                _imp->nodeLinks.push_back(link);
                changed = true;
                
            }
        } else if (found != _imp->nodeLinks.end()) {
            if (isSlave) {
                ///We want to slave to the given node but it already has a link on another parameter, just return
                return;
            } else {
                ///Remove the given link
                _imp->nodeLinks.erase(found);
                changed = true;
                
            }
        }
    }
    if (changed) {
        emit knobsLinksChanged();
    }
}

void
Node::getKnobsLinks(std::list<Node::KnobLink>& links) const
{
    QMutexLocker l(&_imp->masterNodeMutex);
    links = _imp->nodeLinks;
}

void
Node::onMasterNodeDeactivated()
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    _imp->liveInstance->unslaveAllKnobs();
}

boost::shared_ptr<Natron::Node>
Node::getMasterNode() const
{
    QMutexLocker l(&_imp->masterNodeMutex);
    return _imp->masterNode;
}

bool
Node::isSupportedComponent(int inputNb,
                           Natron::ImageComponents comp) const
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

Natron::ImageComponents
Node::findClosestSupportedComponents(int inputNb,
                                     Natron::ImageComponents comp) const
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

int
Node::getMaskChannel(int inputNb) const
{
    std::map<int, boost::shared_ptr<Choice_Knob> >::const_iterator it = _imp->maskChannelKnob.find(inputNb);
    if (it != _imp->maskChannelKnob.end()) {
        return it->second->getValue() - 1;
    } else {
        return 3;
    }
 
}


bool
Node::isMaskEnabled(int inputNb) const
{
    std::map<int, boost::shared_ptr<Bool_Knob> >::const_iterator it = _imp->enableMaskKnob.find(inputNb);
    if (it != _imp->enableMaskKnob.end()) {
        return it->second->getValue();
    } else {
        return true;
    }
}



void
Node::addImageBeingRendered(const boost::shared_ptr<Natron::Image>& image)
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

void
Node::removeImageBeingRendered(const boost::shared_ptr<Natron::Image>& image)
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

boost::shared_ptr<Natron::Image>
Node::getImageBeingRendered(int time,
                            unsigned int mipMapLevel,
                            int view)
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

void
Node::onInputChanged(int inputNb)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->duringInputChangedAction = true;
    std::map<int, boost::shared_ptr<Bool_Knob> >::iterator it = _imp->enableMaskKnob.find(inputNb);
    if (it != _imp->enableMaskKnob.end()) {
        boost::shared_ptr<Node> inp = input(inputNb);
        it->second->setEvaluateOnChange(false);
        it->second->setValue(inp ? true : false, 0);
        it->second->setEvaluateOnChange(true);
    }
    _imp->liveInstance->onInputChanged(inputNb);
    _imp->duringInputChangedAction = false;
}

void
Node::onMultipleInputChanged()
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->duringInputChangedAction = true;
    for (std::map<int, boost::shared_ptr<Bool_Knob> >::iterator it = _imp->enableMaskKnob.begin(); it!=_imp->enableMaskKnob.end(); ++it) {
        boost::shared_ptr<Node> inp = input(it->first);
        it->second->setValue(inp ? true : false, 0);
    }
    _imp->liveInstance->onMultipleInputsChanged();
    _imp->duringInputChangedAction = false;
}

bool
Node::duringInputChangedAction() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->duringInputChangedAction;
}

void
Node::onEffectKnobValueChanged(KnobI* what,
                               Natron::ValueChangedReason reason)
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
        if (reason == Natron::USER_EDITED || reason == Natron::SLAVE_REFRESH) {
            emit previewKnobToggled();
        }
    } else if (what == _imp->disableNodeKnob.get() && !_imp->isMultiInstance && !_imp->multiInstanceParent) {
        emit disabledKnobToggled(_imp->disableNodeKnob->getValue());
        getApp()->redrawAllViewers();
    } else if (what == _imp->nodeLabelKnob.get()) {
        emit nodeExtraLabelChanged(_imp->nodeLabelKnob->getValue().c_str());
    } else if (what->getName() == kOfxParamStringSublabelName) {
        //special hack for the merge node and others so we can retrieve the sublabel and display it in the node's label
        String_Knob* strKnob = dynamic_cast<String_Knob*>(what);
        if (what) {
            QString operation = strKnob->getValue().c_str();
            replaceCustomDataInlabel('(' + operation + ')');
        }
    }
}

void
Node::replaceCustomDataInlabel(const QString& data)
{
    
    assert(QThread::currentThread() == qApp->thread());
    
    QString label = _imp->nodeLabelKnob->getValue().c_str();
    ///Since the label is html encoded, find the text's start
    
    int foundFontTag = label.indexOf("<font");
    bool htmlPresent =  (foundFontTag != -1);
    ///we're sure this end tag is the one of the font tag
    QString endFont("\">");
    int endFontTag = label.indexOf(endFont,foundFontTag);
    
    QString customTagStart(NATRON_CUSTOM_HTML_TAG_START);
    QString customTagEnd(NATRON_CUSTOM_HTML_TAG_END);
    int foundNatronCustomDataTag = label.indexOf(customTagStart,endFontTag == -1 ? 0 : endFontTag);
    if (foundNatronCustomDataTag != -1) {
        ///remove the current custom data
        int foundNatronEndTag = label.indexOf(customTagEnd,foundNatronCustomDataTag);
        assert(foundNatronEndTag != -1);
        
        foundNatronEndTag += customTagEnd.size();
        label.remove(foundNatronCustomDataTag, foundNatronEndTag - foundNatronCustomDataTag);
    }
    
    int i = htmlPresent ? endFontTag + endFont.size() : 0;
    label.insert(i, customTagStart);
    label.insert(i + customTagStart.size(), data);
    label.insert(i + customTagStart.size() + data.size(), customTagEnd);
    _imp->nodeLabelKnob->setValue(label.toStdString(), 0);
}

bool
Node::isNodeDisabled() const
{
    return _imp->disableNodeKnob->getValue();
}

void
Node::setNodeDisabled(bool disabled)
{
    _imp->disableNodeKnob->setValue(disabled, 0);
}

void
Node::showKeyframesOnTimeline(bool emitSignal)
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->keyframesDisplayedOnTimeline || appPTR->isBackground()) {
        return;
    }
    _imp->keyframesDisplayedOnTimeline = true;
    std::list<SequenceTime> keys;
    getAllKnobsKeyframes(&keys);
    getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(keys, emitSignal);
    
}

void
Node::hideKeyframesFromTimeline(bool emitSignal)
{
    assert(QThread::currentThread() == qApp->thread());
    if (!_imp->keyframesDisplayedOnTimeline || appPTR->isBackground()) {
        return;
    }
    _imp->keyframesDisplayedOnTimeline = false;
    std::list<SequenceTime> keys;
    getAllKnobsKeyframes(&keys);
    getApp()->getTimeLine()->removeMultipleKeyframeIndicator(keys, emitSignal);

}

bool
Node::areKeyframesVisibleOnTimeline() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->keyframesDisplayedOnTimeline;
}

void
Node::getAllKnobsKeyframes(std::list<SequenceTime>* keyframes)
{
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getIsSecret()) {
            continue;
        }
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
}


Natron::ImageBitDepth
Node::getBitDepth() const
{
    bool foundShort = false;
    bool foundByte = false;
    for (std::list<ImageBitDepth>::const_iterator it = _imp->supportedDepths.begin(); it!= _imp->supportedDepths.end(); ++it) {
        switch (*it) {
            case Natron::IMAGE_FLOAT:
                return Natron::IMAGE_FLOAT;
                break;
            case Natron::IMAGE_BYTE:
                foundByte = true;
                break;
            case Natron::IMAGE_SHORT:
                foundShort = true;
                break;
            case Natron::IMAGE_NONE:
                break;
        }
    }
    
    if (foundShort) {
        return Natron::IMAGE_SHORT;
    } else if (foundByte) {
        return Natron::IMAGE_BYTE;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);
        return Natron::IMAGE_NONE;
    }
}

bool
Node::isSupportedBitDepth(Natron::ImageBitDepth depth) const
{
    return std::find(_imp->supportedDepths.begin(), _imp->supportedDepths.end(), depth) != _imp->supportedDepths.end();
}

std::string
Node::getNodeExtraLabel() const
{
    return _imp->nodeLabelKnob->getValue();
}

bool
Node::hasSequentialOnlyNodeUpstream(std::string& nodeName) const
{
    
    if (_imp->liveInstance->getSequentialPreference() == Natron::EFFECT_ONLY_SEQUENTIAL && _imp->liveInstance->isWriter()) {
        nodeName = getName_mt_safe();
        return true;
    } else {
        QMutexLocker l(&_imp->inputsMutex);
        
        if (QThread::currentThread() == qApp->thread()) {
            for (std::vector<boost::shared_ptr<Node> >::iterator it = _imp->inputsQueue.begin(); it!=_imp->inputsQueue.end(); ++it) {
                if ((*it) && (*it)->hasSequentialOnlyNodeUpstream(nodeName) && (*it)->getLiveInstance()->isWriter()) {
                    nodeName = (*it)->getName();
                    return true;
                }
            }
        } else {
            for (std::vector<boost::shared_ptr<Node> >::iterator it = _imp->inputs.begin(); it!=_imp->inputs.end(); ++it) {
                if ((*it) && (*it)->hasSequentialOnlyNodeUpstream(nodeName) && (*it)->getLiveInstance()->isWriter()) {
                    nodeName = (*it)->getName_mt_safe();
                    return true;
                }
            }
        }
        return false;
    }
}


bool
Node::isTrackerNode() const
{
    return getPluginID().find("Tracker") != std::string::npos;
}

void
Node::updateEffectLabelKnob(const QString& name)
{
    if (!_imp->liveInstance) {
        return;
    }
    boost::shared_ptr<KnobI> knob = getKnobByName(kOfxParamStringSublabelName);
    String_Knob* strKnob = dynamic_cast<String_Knob*>(knob.get());
    if (strKnob) {
        strKnob->setValue(name.toStdString(), 0);
    }
}

bool
Node::canOthersConnectToThisNode() const
{
    ///In debug mode only allow connections to Writer nodes
# ifdef DEBUG
    return dynamic_cast<const ViewerInstance*>(_imp->liveInstance) == NULL;
# else // !DEBUG
    return dynamic_cast<const ViewerInstance*>(_imp->liveInstance) == NULL && !_imp->liveInstance->isWriter();
# endif // !DEBUG
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

bool
InspectorNode::connectInput(boost::shared_ptr<Node> input,int inputNumber)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    ///cannot connect more than 10 inputs.
    assert(inputNumber <= 10);
    
    assert(input);
    
    if (!checkIfConnectingInputIsOk(input.get())) {
        return false;
    }

    ///If the node 'input' is already to an input of the inspector, find it.
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
    
    /*Adding all empty edges so it creates at least the inputNB'th one.*/
    while (_inputsCount <= inputNumber) {
        
        ///this function might not succeed if we already have 10 inputs OR the last input is already empty
        addEmptyInput();
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



bool
InspectorNode::tryAddEmptyInput()
{
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

void
InspectorNode::addEmptyInput()
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker activeInputLocker(&_activeInputMutex);
        _activeInput = _inputsCount-1;
    }
    ++_inputsCount;
    initializeInputs();
}

void
InspectorNode::removeEmptyInputs()
{
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

int
InspectorNode::disconnectInput(int inputNumber)
{
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

int
InspectorNode::disconnectInput(boost::shared_ptr<Node> input)
{
    ///Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    return disconnectInput(inputIndex(input.get()));
}

void
InspectorNode::setActiveInputAndRefresh(int inputNb)
{
    if (inputNb > (_inputsCount - 1) || inputNb < 0 || input(inputNb) == NULL) {
        return;
    }
    {
        QMutexLocker activeInputLocker(&_activeInputMutex);
        _activeInput = inputNb;
    }
    computeHash();
    onInputChanged(inputNb);
    if (isOutputNode()) {
        dynamic_cast<Natron::OutputEffectInstance*>(getLiveInstance())->updateTreeAndRender();
    }
}

