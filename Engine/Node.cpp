//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Node.h"

#include <limits>
#include <locale>

#include <QtCore/QDebug>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtCore/QWaitCondition>
#include <boost/bind.hpp>

#include <ofxNatron.h>

#include "Engine/Hash64.h"
#include "Engine/Format.h"
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
#include "Engine/Plugin.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/KnobTypes.h"
#include "Engine/ImageParams.h"
#include "Engine/ThreadStorage.h"
#include "Engine/RotoContext.h"
#include "Engine/Timer.h"
#include "Engine/Settings.h"
#include "Engine/NodeGuiI.h"
#include "Engine/NodeGroup.h"
#include "Engine/BackDrop.h"
///The flickering of edges/nodes in the nodegraph will be refreshed
///at most every...
#define NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS 0.5

using namespace Natron;
using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;

namespace { // protect local classes in anonymous namespace
    /*The output node was connected from inputNumber to this...*/
    typedef std::map<Node*,int > DeactivatedState;
    typedef std::list<Node::KnobLink> KnobLinkList;
    typedef std::vector<boost::shared_ptr<Node> > InputsV;
    
    enum InputActionEnum
    {
        eInputActionConnect,
        eInputActionDisconnect,
        eInputActionReplace
    };
    
    struct ConnectInputAction
    {
        boost::shared_ptr<Natron::Node> node;
        InputActionEnum type;
        int inputNb;
        
        ConnectInputAction(const boost::shared_ptr<Natron::Node>& input,InputActionEnum type,int inputNb)
        : node(input)
        , type(type)
        , inputNb(inputNb)
        {
        }
    };
    
    class ChannelSelector {
        
    public:
        
        boost::shared_ptr<Choice_Knob> layer;
        boost::shared_ptr<String_Knob> layerName;
        boost::shared_ptr<Bool_Knob> enabledChan[4];
        bool useRGBASelectors; //< if false, only the layer knob is created
        bool hasAllChoice; // if true, the layer has a "all" entry
        
        mutable QMutex compsMutex;
        
        //Stores the components available at build time of the choice menu
        EffectInstance::ComponentsAvailableMap compsAvailable;
        
        ChannelSelector()
        : layer()
        , layerName()
        , enabledChan()
        , useRGBASelectors(false)
        , hasAllChoice(false)
        , compsMutex()
        , compsAvailable()
        {
            
        }
        
        ChannelSelector(const ChannelSelector& other) {
            *this = other;
        }
        
        void operator=(const ChannelSelector& other) {
            layer = other.layer;
            for (int i = 0; i< 4; ++i) {
                enabledChan[i] = other.enabledChan[i];
            }
            useRGBASelectors = other.useRGBASelectors;
            hasAllChoice = other.hasAllChoice;
            layerName = other.layerName;
            QMutexLocker k(&compsMutex);
            compsAvailable = other.compsAvailable;
        }
    };
    
    class MaskSelector {
        
    public:
        
        boost::shared_ptr<Bool_Knob> enabled;
        boost::shared_ptr<Choice_Knob> channel;
        boost::shared_ptr<String_Knob> channelName;
        
        mutable QMutex compsMutex;
        //Stores the components available at build time of the choice menu
        std::vector<ImageComponents> compsAvailable;
        
        MaskSelector()
        : enabled()
        , channel()
        , channelName()
        , compsMutex()
        , compsAvailable()
        {
            
        }
        
        MaskSelector(const MaskSelector& other) {
            *this = other;
        }
        
        void operator=(const MaskSelector& other) {
            enabled = other.enabled;
            channel = other.channel;
            channelName = other.channelName;
            QMutexLocker k(&compsMutex);
            compsAvailable = other.compsAvailable;
        }

    };
    
}



struct Node::Implementation
{
    Implementation(Node* publicInterface,
                   AppInstance* app_,
                   const boost::shared_ptr<NodeCollection>& collection,
                   Natron::Plugin* plugin_)
    : _publicInterface(publicInterface)
    , group(collection)
    , app(app_)
    , knobsInitialized(false)
    , inputsInitialized(false)
    , outputsMutex()
    , outputs()
    , inputsMutex()
    , inputs()
    , liveInstance()
    , inputsComponents()
    , outputComponents()
    , inputLabels()
    , scriptName()
    , label()
    , deactivatedState()
    , activatedMutex()
    , activated(true)
    , plugin(plugin_)
    , computingPreview(false)
    , computingPreviewMutex()
    , pluginInstanceMemoryUsed(0)
    , memoryUsedMutex()
    , mustQuitPreview(false)
    , mustQuitPreviewMutex()
    , mustQuitPreviewCond()
    , knobsAge(0)
    , knobsAgeMutex()
    , masterNodeMutex()
    , masterNode()
    , nodeLinks()
    , nodeSettingsPage()
    , nodeLabelKnob()
    , previewEnabledKnob()
    , disableNodeKnob()
    , infoPage()
    , infoDisclaimer()
    , inputFormats()
    , outputFormat()
    , refreshInfoButton()
    , useFullScaleImagesWhenRenderScaleUnsupported()
    , forceCaching()
    , beforeFrameRender()
    , beforeRender()
    , afterFrameRender()
    , afterRender()
    , channelsSelectors()
    , maskSelectors()
    , rotoContext()
    , imagesBeingRenderedMutex()
    , imageBeingRenderedCond()
    , imagesBeingRendered()
    , supportedDepths()
    , isMultiInstance(false)
    , multiInstanceParent()
    , childrenMutex()
    , children()
    , multiInstanceParentName()
    , duringInputChangedAction(false)
    , keyframesDisplayedOnTimeline(false)
    , timersMutex()
    , lastRenderStartedSlotCallTime()
    , lastInputNRenderStartedSlotCallTime()
    , connectionQueue()
    , connectionQueueMutex()
    , nodeIsDequeuing(false)
    , nodeIsDequeuingMutex()
    , nodeIsDequeuingCond()
    , nodeIsRendering(0)
    , nodeIsRenderingMutex()
    , mustQuitProcessing(false)
    , mustQuitProcessingMutex()
    , persistentMessage()
    , persistentMessageType(0)
    , persistentMessageMutex()
    , guiPointer()
    , nativePositionOverlays()
    , pluginPythonModuleMutex()
    , pluginPythonModule()
    , nodeCreated(false)
    , createdComponentsMutex()
    , createdComponents()
    {        
        ///Initialize timers
        gettimeofday(&lastRenderStartedSlotCallTime, 0);
        gettimeofday(&lastInputNRenderStartedSlotCallTime, 0);
    }
    
    void abortPreview();
    
    bool checkForExitPreview();
    
    void setComputingPreview(bool v) {
        QMutexLocker l(&computingPreviewMutex);
        computingPreview = v;
    }
    
    void restoreUserKnobsRecursive(const std::list<boost::shared_ptr<KnobSerializationBase> >& knobs,
                                   const boost::shared_ptr<Group_Knob>& group,
                                   const boost::shared_ptr<Page_Knob>& page);
    
    void restoreKnobLinksRecursive(const GroupKnobSerialization* group,
                                   const std::list<boost::shared_ptr<Natron::Node> > & allNodes);
    
    void ifGroupForceHashChangeOfInputs();
    
    void runOnNodeCreatedCB(bool userEdited);
    
    void runOnNodeDeleteCB();
    
    void appendChild(const boost::shared_ptr<Natron::Node>& child);
    
    void runInputChangedCallback(int index,const std::string& script);
    
    void createChannelSelector(int inputNb,const std::string & inputName, bool isOutput,const boost::shared_ptr<Page_Knob>& page);
    
    void onLayerChanged(int inputNb,const ChannelSelector& selector);
    
    void onMaskSelectorChanged(int inputNb,const MaskSelector& selector);
    
    bool getSelectedLayer(int inputNb,const ChannelSelector& selector, ImageComponents* comp) const;
    
    Node* _publicInterface;
    
    boost::weak_ptr<NodeCollection> group;
    
    AppInstance* app; // pointer to the app: needed to access the application's default-project's format
    
    bool knobsInitialized;
    bool inputsInitialized;
    mutable QMutex outputsMutex;
    std::list<Node*> outputs;
    
    mutable QMutex inputsMutex; //< protects guiInputs so the serialization thread can access them
    
    ///The  inputs are the one used by the GUI whenever the user make changes.
    ///Finally we use only one set of inputs protected by a mutex. This has the drawback that the
    ///inputs connections might change throughout the render of a frame and the plug-in might then have
    ///different results when calling stuff on clips (e.g: getRegionOfDefinition or getComponents).
    ///I couldn't figure out a clean way to have the inputs stable throughout a render, thread-storage is not enough:
    ///image effect suite functions that access to clips can be called from other threads than just the render-thread:
    ///they can be accessed from the multi-thread suite. The problem is we have no way to set thread-local data to multi
    ///thread suite threads because we don't know at all which node is using it.
    InputsV inputs;
    
    //to the inputs in a thread-safe manner.
    boost::shared_ptr<Natron::EffectInstance>  liveInstance; //< the effect hosted by this node
    
    ///These two are also protected by inputsMutex
    std::vector< std::list<Natron::ImageComponents> > inputsComponents;
    std::list<Natron::ImageComponents> outputComponents;
    
    mutable QMutex nameMutex;
    std::vector<std::string> inputLabels; // inputs name
    std::string scriptName; //node name internally and as visible to python
    std::string label; // node label as visible in the GUI
    
    DeactivatedState deactivatedState;
    mutable QMutex activatedMutex;
    bool activated;
    
    Natron::Plugin* plugin; //< the plugin which stores the function to instantiate the effect
    
    bool computingPreview;
    mutable QMutex computingPreviewMutex;
    
    size_t pluginInstanceMemoryUsed; //< global count on all EffectInstance's of the memory they use.
    QMutex memoryUsedMutex; //< protects _pluginInstanceMemoryUsed
    
    bool mustQuitPreview;
    QMutex mustQuitPreviewMutex;
    QWaitCondition mustQuitPreviewCond;
    
    
    QMutex renderInstancesSharedMutex; //< see eRenderSafetyInstanceSafe in EffectInstance::renderRoI
    //only 1 clone can render at any time
    
    U64 knobsAge; //< the age of the knobs in this effect. It gets incremented every times the liveInstance has its evaluate() function called.
    mutable QReadWriteLock knobsAgeMutex; //< protects knobsAge and hash
    Hash64 hash; //< recomputed everytime knobsAge is changed.
    
    mutable QMutex masterNodeMutex; //< protects masterNode and nodeLinks
    boost::weak_ptr<Node> masterNode; //< this points to the master when the node is a clone
    KnobLinkList nodeLinks; //< these point to the parents of the params links
    
    boost::shared_ptr<Page_Knob> nodeSettingsPage;
    boost::shared_ptr<String_Knob> nodeLabelKnob;
    boost::shared_ptr<Bool_Knob> previewEnabledKnob;
    boost::shared_ptr<Bool_Knob> disableNodeKnob;
    boost::shared_ptr<String_Knob> knobChangedCallback;
    boost::shared_ptr<String_Knob> inputChangedCallback;
    
    boost::shared_ptr<Page_Knob> infoPage;
    boost::shared_ptr<String_Knob> infoDisclaimer;
    std::vector< boost::shared_ptr<String_Knob> > inputFormats;
    boost::shared_ptr<String_Knob> outputFormat;
    boost::shared_ptr<Button_Knob> refreshInfoButton;
    
    boost::shared_ptr<Bool_Knob> useFullScaleImagesWhenRenderScaleUnsupported;
    boost::shared_ptr<Bool_Knob> forceCaching;
    
    boost::shared_ptr<String_Knob> beforeFrameRender;
    boost::shared_ptr<String_Knob> beforeRender;
    boost::shared_ptr<String_Knob> afterFrameRender;
    boost::shared_ptr<String_Knob> afterRender;
    
    std::map<int,ChannelSelector> channelsSelectors;
    std::map<int,MaskSelector> maskSelectors;
    
    boost::shared_ptr<RotoContext> rotoContext; //< valid when the node has a rotoscoping context (i.e: paint context)
    
    mutable QMutex imagesBeingRenderedMutex;
    QWaitCondition imageBeingRenderedCond;
    std::list< boost::shared_ptr<Image> > imagesBeingRendered; ///< a list of all the images being rendered simultaneously
    
    std::list <Natron::ImageBitDepthEnum> supportedDepths;
    
    ///True when several effect instances are represented under the same node.
    bool isMultiInstance;
    boost::weak_ptr<Natron::Node> multiInstanceParent;
    mutable QMutex childrenMutex;
    std::list<boost::weak_ptr<Natron::Node> > children;
    
    ///the name of the parent at the time this node was created
    std::string multiInstanceParentName;
    bool duringInputChangedAction; //< true if we're during onInputChanged(...). MT-safe since only modified by the main thread
    bool keyframesDisplayedOnTimeline;
    
    ///This is to avoid the slots connected to the main-thread to be called too much
    QMutex timersMutex; //< protects lastRenderStartedSlotCallTime & lastInputNRenderStartedSlotCallTime
    timeval lastRenderStartedSlotCallTime;
    timeval lastInputNRenderStartedSlotCallTime;
    
    ///Used when the node is rendering and dequeued when it is done rendering
    std::list<ConnectInputAction> connectionQueue;
    QMutex connectionQueueMutex;
    
    ///True when the node is dequeuing the connectionQueue and no render should be started 'til it is empty
    bool nodeIsDequeuing;
    QMutex nodeIsDequeuingMutex;
    QWaitCondition nodeIsDequeuingCond;
    
    ///Counter counting how many parallel renders are active on the node
    int nodeIsRendering;
    mutable QMutex nodeIsRenderingMutex;
    
    bool mustQuitProcessing;
    mutable QMutex mustQuitProcessingMutex;
    
    QString persistentMessage;
    int persistentMessageType;
    mutable QMutex persistentMessageMutex;
    
    boost::weak_ptr<NodeGuiI> guiPointer;
    
    std::list<boost::shared_ptr<Double_Knob> > nativePositionOverlays;
    
    mutable QMutex pluginPythonModuleMutex;
    std::string pluginPythonModule;
    
    bool nodeCreated;
    
    mutable QMutex createdComponentsMutex;
    std::list<Natron::ImageComponents> createdComponents; // comps created by the user
};

/**
 *@brief Actually converting to ARGB... but it is called BGRA by
 the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

Node::Node(AppInstance* app,
           const boost::shared_ptr<NodeCollection>& group,
           Natron::Plugin* plugin)
: QObject()
, _imp( new Implementation(this,app,group,plugin) )
{
    QObject::connect( this, SIGNAL( pluginMemoryUsageChanged(qint64) ), appPTR, SLOT( onNodeMemoryRegistered(qint64) ) );
    QObject::connect(this, SIGNAL(mustDequeueActions()), this, SLOT(dequeueActions()));
}

void
Node::createRotoContextConditionnally()
{
    assert(!_imp->rotoContext);
    assert(_imp->liveInstance);
    ///Initialize the roto context if any
    if ( isRotoNode() ) {
        _imp->rotoContext.reset( new RotoContext(shared_from_this()) );
        _imp->rotoContext->createBaseLayer();
    }
}

const Natron::Plugin*
Node::getPlugin() const
{
    return _imp->plugin;
}

void
Node::switchInternalPlugin(Natron::Plugin* plugin)
{
    _imp->plugin = plugin;
}

void
Node::load(const std::string & parentMultiInstanceName,
           const NodeSerialization & serialization,
           bool dontLoadName,
           bool userEdited,
           const QString& fixedName,
           const CreateNodeArgs::DefaultValuesList& paramValues)
{
    ///Called from the main thread. MT-safe
    assert( QThread::currentThread() == qApp->thread() );
    
    ///cannot load twice
    assert(!_imp->liveInstance);
    
    bool nameSet = false;
    bool isMultiInstanceChild = false;
    if ( !parentMultiInstanceName.empty() ) {
        _imp->multiInstanceParentName = parentMultiInstanceName;
        isMultiInstanceChild = true;
        _imp->isMultiInstance = false;
    }
    
    if (!parentMultiInstanceName.empty()) {
        fetchParentMultiInstancePointer();
    }
    
    if (!serialization.isNull() && !dontLoadName && !nameSet && fixedName.isEmpty()) {
        setScriptName_no_error_check(serialization.getNodeScriptName());
        setLabel(serialization.getNodeLabel());
        nameSet = true;
    }
    
   
    boost::shared_ptr<Node> thisShared = shared_from_this();

    int renderScaleSupportPreference = appPTR->getCurrentSettings()->getRenderScaleSupportPreference(getPluginID());

    LibraryBinary* binary = _imp->plugin->getLibraryBinary();
    std::pair<bool,EffectBuilder> func;
    if (binary) {
        func = binary->findFunction<EffectBuilder>("BuildEffect");
    }
    bool isFileDialogPreviewReader = fixedName.contains("Natron_File_Dialog_Preview_Provider_Reader");
    if (func.first) {
        _imp->liveInstance.reset(func.second(thisShared));
        assert(_imp->liveInstance);
        _imp->liveInstance->initializeData();
        
        createRotoContextConditionnally();
        initializeInputs();
        initializeKnobs(renderScaleSupportPreference);
        
        if (!serialization.isNull()) {
            loadKnobs(serialization);
        }
        if (!paramValues.empty()) {
            setValuesFromSerialization(paramValues);
        }
        
        
        std::string images;
        if (_imp->liveInstance->isReader() && serialization.isNull() && paramValues.empty() && !isFileDialogPreviewReader) {
            images = getApp()->openImageFileDialog();
        } else if (_imp->liveInstance->isWriter() && serialization.isNull() && paramValues.empty() && !isFileDialogPreviewReader) {
            images = getApp()->saveImageFileDialog();
        }
        if (!images.empty()) {
            boost::shared_ptr<KnobSerialization> defaultFile = createDefaultValueForParam(kOfxImageEffectFileParamName, images);
            CreateNodeArgs::DefaultValuesList list;
            list.push_back(defaultFile);
            setValuesFromSerialization(list);
        }
    } else { //ofx plugin
                
        _imp->liveInstance = appPTR->createOFXEffect(thisShared,&serialization,paramValues,!isFileDialogPreviewReader && userEdited,renderScaleSupportPreference == 1);
        assert(_imp->liveInstance);
        _imp->liveInstance->initializeOverlayInteract();
    }
    
    _imp->liveInstance->addSupportedBitDepth(&_imp->supportedDepths);
    
    if ( _imp->supportedDepths.empty() ) {
        //From the spec:
        //The default for a plugin is to have none set, the plugin \em must define at least one in its describe action.
        throw std::runtime_error("Plug-in does not support 8bits, 16bits or 32bits floating point image processing.");
    }
    
    if (isTrackerNode()) {
        _imp->isMultiInstance = true;
    }
    
    if (!nameSet) {
        if (fixedName.isEmpty()) {
            std::string name;
            QString pluginLabel;
            AppManager::AppTypeEnum appType = appPTR->getAppType();
            if (_imp->plugin &&
                (appType == AppManager::eAppTypeBackground ||
                 appType == AppManager::eAppTypeGui ||
                 appType == AppManager::eAppTypeInterpreter)) {
                pluginLabel = _imp->plugin->getLabelWithoutSuffix();
            } else {
                pluginLabel = _imp->plugin->getPluginLabel();
            }
            getGroup()->initNodeName(isMultiInstanceChild ? parentMultiInstanceName + '_' : pluginLabel.toStdString(),&name);
            setNameInternal(name.c_str());
            nameSet = true;
        } else {
            setScriptName(fixedName.toStdString());
        }
        if (!isMultiInstanceChild && _imp->isMultiInstance) {
            updateEffectLabelKnob( getScriptName().c_str() );
        }
    }
    if ( isMultiInstanceChild && serialization.isNull() ) {
        assert(nameSet);
        updateEffectLabelKnob(getScriptName().c_str());
    }
    
    declarePythonFields();
    if  (getRotoContext()) {
        declareRotoPythonField();
    }
    
    boost::shared_ptr<NodeCollection> group = getGroup();
    if (group) {
        group->notifyNodeActivated(thisShared);
    }
    
    computeHash();
    assert(_imp->liveInstance);
    _imp->nodeCreated = true;
    
    refreshChannelSelectors(serialization.isNull());

    _imp->runOnNodeCreatedCB(serialization.isNull());
    
} // load

bool
Node::isNodeCreated() const
{
    return _imp->nodeCreated;
}

void
Node::declareRotoPythonField()
{
    assert(_imp->rotoContext);
    std::string appID = getApp()->getAppIDString();
    std::string fullyQualifiedName = appID + "." + getFullyQualifiedName();
    std::string err;
    std::string script = fullyQualifiedName + ".roto = " + fullyQualifiedName + ".getRotoContext()\n";
    if (!appPTR->isBackground()) {
        getApp()->printAutoDeclaredVariable(script);
    }
    bool ok = Natron::interpretPythonScript(script, &err, 0);
    assert(ok);
    (void)ok;
    _imp->rotoContext->declarePythonFields();
}

boost::shared_ptr<NodeCollection>
Node::getGroup() const
{
    return _imp->group.lock();
}

void
Node::Implementation::appendChild(const boost::shared_ptr<Natron::Node>& child)
{
    QMutexLocker k(&childrenMutex);
    for (std::list<boost::weak_ptr<Natron::Node> >::iterator it = children.begin(); it!=children.end(); ++it) {
        if (it->lock() == child) {
            return;
        }
    }
    children.push_back(child);
}

void
Node::fetchParentMultiInstancePointer()
{
    NodeList nodes = _imp->group.lock()->getNodes();
    
    NodePtr thisShared = shared_from_this();
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->getScriptName() == _imp->multiInstanceParentName) {
            ///no need to store the boost pointer because the main instance lives the same time
            ///as the child
            _imp->multiInstanceParent = *it;
            (*it)->_imp->appendChild(thisShared);
            QObject::connect(it->get(), SIGNAL(inputChanged(int)), this, SLOT(onParentMultiInstanceInputChanged(int)));
            break;
        }
    }
    
}

boost::shared_ptr<Natron::Node>
Node::getParentMultiInstance() const
{
    return _imp->multiInstanceParent.lock();
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
    return _imp->multiInstanceParentName;
}

void
Node::getChildrenMultiInstance(std::list<boost::shared_ptr<Natron::Node> >* children) const
{
    QMutexLocker k(&_imp->childrenMutex);
    for (std::list<boost::weak_ptr<Natron::Node> >::const_iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        children->push_back(it->lock());
    }
}

U64
Node::getHashValue() const
{
    QReadLocker l(&_imp->knobsAgeMutex);
    
    return _imp->hash.value();
}

void
Node::computeHashInternal(std::list<Natron::Node*>& marked)
{
    if (std::find(marked.begin(), marked.end(), this) != marked.end()) {
        return;
    }
    
    ///Always called in the main thread
    assert( QThread::currentThread() == qApp->thread() );
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
            ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->liveInstance.get());
            
            if (isViewer) {
                int activeInput[2];
                isViewer->getActiveInputs(activeInput[0], activeInput[1]);
                
                for (int i = 0; i < 2; ++i) {
                    NodePtr input = getInput(activeInput[i]);
                    if (input) {
                        _imp->hash.append(input->getHashValue() );
                    }
                }
            } else {
                for (U32 i = 0; i < _imp->inputs.size(); ++i) {
                    NodePtr input = getInput(i);
                    if (input) {
                        ///Add the index of the input to its hash.
                        ///Explanation: if we didn't add this, just switching inputs would produce a similar
                        ///hash.
                        _imp->hash.append(input->getHashValue() + i);
                    }
                }
            }
        }
        
        ///Also append the effect's label to distinguish 2 instances with the same parameters
        ::Hash64_appendQString( &_imp->hash, QString( getScriptName().c_str() ) );
        
        
        ///Also append the project's creation time in the hash because 2 projects openend concurrently
        ///could reproduce the same (especially simple graphs like Viewer-Reader)
        qint64 creationTime =  getApp()->getProject()->getProjectCreationTime();
        _imp->hash.append(creationTime);
        
        _imp->hash.computeHash();
    }
    
    marked.push_back(this);
    
    ///call it on all the outputs
    std::list<Node*> outputs;
    getOutputsWithGroupRedirection(outputs);
    for (std::list<Node*>::iterator it = outputs.begin(); it != outputs.end(); ++it) {
        assert(*it);
        (*it)->computeHashInternal(marked);
    }
    
    _imp->liveInstance->onNodeHashChanged(getHashValue());
    
    ///If the node is a group, call it on all nodes in the group
    ///Also force a change to their hash
    NodeGroup* group = dynamic_cast<NodeGroup*>(getLiveInstance());
    if (group) {
        NodeList nodes = group->getNodes();
        for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            assert(*it);
            (*it)->incrementKnobsAge();
            (*it)->computeHashInternal(marked);
        }
    }

}

void
Node::computeHash()
{
    std::list<Natron::Node*> marked;
    computeHashInternal(marked);
    
} // computeHash

void
Node::setValuesFromSerialization(const std::list<boost::shared_ptr<KnobSerialization> >& paramValues)
{
    
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->knobsInitialized);
    
    const std::vector< boost::shared_ptr<KnobI> > & nodeKnobs = getKnobs();
    ///for all knobs of the node
    for (U32 j = 0; j < nodeKnobs.size(); ++j) {
        ///try to find a serialized value for this knob
        for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = paramValues.begin(); it != paramValues.end(); ++it) {
            if ( (*it)->getName() == nodeKnobs[j]->getName() ) {
                boost::shared_ptr<KnobI> serializedKnob = (*it)->getKnob();
                nodeKnobs[j]->clone(serializedKnob);
                break;
            }
        }
        
    }
}

void
Node::loadKnobs(const NodeSerialization & serialization,bool updateKnobGui)
{
    ///Only called from the main thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->knobsInitialized);
    if (serialization.isNull()) {
        return;
    }
    
    {
        QMutexLocker k(&_imp->createdComponentsMutex);
        _imp->createdComponents = serialization.getUserComponents();
    }
    
    const std::vector< boost::shared_ptr<KnobI> > & nodeKnobs = getKnobs();
    ///for all knobs of the node
    for (U32 j = 0; j < nodeKnobs.size(); ++j) {
        loadKnob(nodeKnobs[j], serialization.getKnobsValues(),updateKnobGui);
    }
    ///now restore the roto context if the node has a roto context
    if (serialization.hasRotoContext() && _imp->rotoContext) {
        _imp->rotoContext->load( serialization.getRotoContext() );
    }
    
    restoreUserKnobs(serialization);
    
    setKnobsAge( serialization.getKnobsAge() );
}

void
Node::loadKnob(const boost::shared_ptr<KnobI> & knob,
               const std::list< boost::shared_ptr<KnobSerialization> > & knobsValues,bool updateKnobGui)
{
    
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        if ( (*it)->getName() == knob->getName() ) {
            
            // don't load the value if the Knob is not persistant! (it is just the default value in this case)
            ///EDIT: Allow non persistent params to be loaded if we found a valid serialization for them
            //if ( knob->getIsPersistant() ) {
            boost::shared_ptr<KnobI> serializedKnob = (*it)->getKnob();
            
            Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob.get());
            if (isChoice) {
                const TypeExtraData* extraData = (*it)->getExtraData();
                const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>(extraData);
                assert(choiceData);
                
                Choice_Knob* choiceSerialized = dynamic_cast<Choice_Knob*>(serializedKnob.get());
                assert(choiceSerialized);
                isChoice->choiceRestoration(choiceSerialized, choiceData);
            } else {
                if (updateKnobGui) {
                    knob->cloneAndUpdateGui(serializedKnob.get());
                } else {
                    knob->clone(serializedKnob);
                }
                knob->setSecret( serializedKnob->getIsSecret() );
                if ( knob->getDimension() == serializedKnob->getDimension() ) {
                    for (int i = 0; i < knob->getDimension(); ++i) {
                        knob->setEnabled( i, serializedKnob->isEnabled(i) );
                    }
                }
            }
            
            if (knob->getName() == kOfxImageEffectFileParamName) {
                computeFrameRangeForReader(knob.get());
            }
            
            //}
            break;
        }
    }
}

void
Node::Implementation::restoreKnobLinksRecursive(const GroupKnobSerialization* group,
                                                const std::list<boost::shared_ptr<Natron::Node> > & allNodes)
{
    const std::list <boost::shared_ptr<KnobSerializationBase> >&  children = group->getChildren();
    for (std::list <boost::shared_ptr<KnobSerializationBase> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>(it->get());
        KnobSerialization* isRegular = dynamic_cast<KnobSerialization*>(it->get());
        assert(isGrp || isRegular);
        if (isGrp) {
            restoreKnobLinksRecursive(isGrp,allNodes);
        } else if (isRegular) {
            boost::shared_ptr<KnobI> knob =  _publicInterface->getKnobByName( isRegular->getName() );
            if (!knob) {
                appPTR->writeToOfxLog_mt_safe("Couldn't find a parameter named " + QString((*it)->getName().c_str()));
                continue;
            }
            isRegular->restoreKnobLinks(knob,allNodes);
            isRegular->restoreExpressions(knob);
            isRegular->restoreTracks(knob,allNodes);

        }
    }
}

void
Node::restoreKnobsLinks(const NodeSerialization & serialization,
                        const std::list<boost::shared_ptr<Natron::Node> > & allNodes)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    const NodeSerialization::KnobValues & knobsValues = serialization.getKnobsValues();
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        boost::shared_ptr<KnobI> knob = getKnobByName( (*it)->getName() );
        if (!knob) {
            appPTR->writeToOfxLog_mt_safe("Couldn't find a parameter named " + QString((*it)->getName().c_str()));
            continue;
        }
        (*it)->restoreKnobLinks(knob,allNodes);
        (*it)->restoreExpressions(knob);
        (*it)->restoreTracks(knob,allNodes);
      
    }
    
    const std::list<boost::shared_ptr<GroupKnobSerialization> >& userKnobs = serialization.getUserPages();
    for (std::list<boost::shared_ptr<GroupKnobSerialization> >::const_iterator it = userKnobs.begin(); it != userKnobs.end(); ++it) {
        _imp->restoreKnobLinksRecursive(it->get(), allNodes);
    }
    
}

void
Node::restoreUserKnobs(const NodeSerialization& serialization)
{
    const std::list<boost::shared_ptr<GroupKnobSerialization> >& userPages = serialization.getUserPages();
    for (std::list<boost::shared_ptr<GroupKnobSerialization> >::const_iterator it = userPages.begin() ; it != userPages.end(); ++it) {
        boost::shared_ptr<Page_Knob> page = Natron::createKnob<Page_Knob>(_imp->liveInstance.get(), (*it)->getLabel() , 1, false);
        page->setAsUserKnob();
        page->setName((*it)->getName());
        _imp->restoreUserKnobsRecursive((*it)->getChildren(), boost::shared_ptr<Group_Knob>(), page);
    }
}

void
Node::Implementation::restoreUserKnobsRecursive(const std::list<boost::shared_ptr<KnobSerializationBase> >& knobs,
                                                const boost::shared_ptr<Group_Knob>& group,
                                                const boost::shared_ptr<Page_Knob>& page)
{
    for (std::list<boost::shared_ptr<KnobSerializationBase> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>(it->get());
        KnobSerialization* isRegular = dynamic_cast<KnobSerialization*>(it->get());
        assert(isGrp || isRegular);
        
        if (isGrp) {
            boost::shared_ptr<Group_Knob> grp = Natron::createKnob<Group_Knob>(liveInstance.get(), isGrp->getLabel() , 1, false);
            grp->setAsUserKnob();
            grp->setName((*it)->getName());
            if (isGrp && isGrp->isSetAsTab()) {
                grp->setAsTab();
            }
            page->addKnob(grp);
            if (group) {
                group->addKnob(grp);
            }
            grp->setValue(isGrp->isOpened(), 0);
            restoreUserKnobsRecursive(isGrp->getChildren(), grp, page);
        } else {
            assert(isRegular->isUserKnob());
            boost::shared_ptr<KnobI> sKnob = isRegular->getKnob();
            boost::shared_ptr<KnobI> knob;
            Int_Knob* isInt = dynamic_cast<Int_Knob*>(sKnob.get());
            Double_Knob* isDbl = dynamic_cast<Double_Knob*>(sKnob.get());
            Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(sKnob.get());
            Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(sKnob.get());
            Color_Knob* isColor = dynamic_cast<Color_Knob*>(sKnob.get());
            String_Knob* isStr = dynamic_cast<String_Knob*>(sKnob.get());
            File_Knob* isFile = dynamic_cast<File_Knob*>(sKnob.get());
            OutputFile_Knob* isOutFile = dynamic_cast<OutputFile_Knob*>(sKnob.get());
            Path_Knob* isPath = dynamic_cast<Path_Knob*>(sKnob.get());
            Button_Knob* isBtn = dynamic_cast<Button_Knob*>(sKnob.get());
            
            assert(isInt || isDbl || isBool || isChoice || isColor || isStr || isFile || isOutFile || isPath || isBtn);
            
            if (isInt) {
                boost::shared_ptr<Int_Knob> k = Natron::createKnob<Int_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                             sKnob->getDimension(), false);
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(isRegular->getExtraData());
                assert(data);
                std::vector<int> minimums,maximums,dminimums,dmaximums;
                for (int i = 0; i < k->getDimension(); ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                k->setMinimumsAndMaximums(minimums, maximums);
                k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                knob = k;
            } else if (isDbl) {
                boost::shared_ptr<Double_Knob> k = Natron::createKnob<Double_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                             sKnob->getDimension(), false);
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(isRegular->getExtraData());
                assert(data);
                std::vector<double> minimums,maximums,dminimums,dmaximums;
                for (int i = 0; i < k->getDimension(); ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                k->setMinimumsAndMaximums(minimums, maximums);
                k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                knob = k;
                
                if (isRegular->getUseHostOverlayHandle()) {
                    Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob.get());
                    if (isDbl) {
                        isDbl->setHasNativeOverlayHandle(true);
                    }
                }
                
            } else if (isBool) {
                boost::shared_ptr<Bool_Knob> k = Natron::createKnob<Bool_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                             sKnob->getDimension(), false);
                knob = k;
            } else if (isChoice) {
                boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                               sKnob->getDimension(), false);
                const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>(isRegular->getExtraData());
                assert(data);
                k->populateChoices(data->_entries,data->_helpStrings);
                knob = k;
            } else if (isColor) {
                boost::shared_ptr<Color_Knob> k = Natron::createKnob<Color_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                               sKnob->getDimension(), false);
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(isRegular->getExtraData());
                assert(data);
                std::vector<double> minimums,maximums,dminimums,dmaximums;
                for (int i = 0; i < k->getDimension(); ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                k->setMinimumsAndMaximums(minimums, maximums);
                k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                knob = k;

            } else if (isStr) {
                boost::shared_ptr<String_Knob> k = Natron::createKnob<String_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                                 sKnob->getDimension(), false);
                const TextExtraData* data = dynamic_cast<const TextExtraData*>(isRegular->getExtraData());
                assert(data);
                if (data->label) {
                    k->setAsLabel();
                } else {
                    if (data->multiLine) {
                        k->setAsMultiLine();
                        if (data->richText) {
                            k->setUsesRichText(true);
                        }
                    }
                }
                knob = k;

            } else if (isFile) {
                boost::shared_ptr<File_Knob> k = Natron::createKnob<File_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                                   sKnob->getDimension(), false);
                const FileExtraData* data = dynamic_cast<const FileExtraData*>(isRegular->getExtraData());
                assert(data);
                if (data->useSequences) {
                    k->setAsInputImage();
                }
                knob = k;

            } else if (isOutFile) {
                boost::shared_ptr<OutputFile_Knob> k = Natron::createKnob<OutputFile_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                               sKnob->getDimension(), false);
                const FileExtraData* data = dynamic_cast<const FileExtraData*>(isRegular->getExtraData());
                assert(data);
                if (data->useSequences) {
                    k->setAsOutputImageFile();
                }
                knob = k;
            } else if (isPath) {
                boost::shared_ptr<Path_Knob> k = Natron::createKnob<Path_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                                           sKnob->getDimension(), false);
                const PathExtraData* data = dynamic_cast<const PathExtraData*>(isRegular->getExtraData());
                assert(data);
                if (data->multiPath) {
                    k->setMultiPath(true);
                }
                knob = k;
            } else if (isBtn) {
                boost::shared_ptr<Button_Knob> k = Natron::createKnob<Button_Knob>(liveInstance.get(), isRegular->getLabel() ,
                                                                               sKnob->getDimension(), false);
                knob = k;
            }
            
            assert(knob);
            knob->cloneDefaultValues(sKnob.get());
            knob->clone(sKnob.get());
            knob->setAsUserKnob();
            if (group) {
                group->addKnob(knob);
            } else if (page) {
                page->addKnob(knob);
            }
            knob->setIsPersistant(isRegular->isPersistent());
            knob->setAnimationEnabled(isRegular->isAnimationEnabled());
            knob->setEvaluateOnChange(isRegular->getEvaluatesOnChange());
            knob->setName(isRegular->getName());
            knob->setHintToolTip(isRegular->getHintToolTip());
            if (!isRegular->triggerNewLine()) {
                knob->setAddNewLine(false);
            }
            
        }
    }
}

void
Node::setKnobsAge(U64 newAge)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    QWriteLocker l(&_imp->knobsAgeMutex);
    if (_imp->knobsAge != newAge) {
        _imp->knobsAge = newAge;
        Q_EMIT knobsAgeChanged(_imp->knobsAge);
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
        if ( _imp->knobsAge == std::numeric_limits<U64>::max() ) {
            appPTR->clearAllCaches();
            _imp->knobsAge = 0;
        }
        newAge = _imp->knobsAge;
    }
    Q_EMIT knobsAgeChanged(newAge);
    
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

bool
Node::hasOverlay() const
{
    if (!_imp->liveInstance) {
        return false;
    }

    return _imp->liveInstance->hasOverlay();
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

bool
Node::Implementation::checkForExitPreview()
{
    {
        QMutexLocker locker(&mustQuitPreviewMutex);
        if (mustQuitPreview) {
            mustQuitPreview = false;
            mustQuitPreviewCond.wakeOne();
            
            return true;
        }
        return false;
    }
}

void
Node::abortAnyProcessing()
{
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( getLiveInstance() );
    
    if (isOutput) {
        isOutput->getRenderEngine()->abortRendering(true);
    }
    _imp->abortPreview();
}

void
Node::quitAnyProcessing()
{
    {
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        _imp->nodeIsDequeuing = false;
        _imp->nodeIsDequeuingCond.wakeAll();
    }
    
    {
        QMutexLocker k(&_imp->mustQuitProcessingMutex);
        _imp->mustQuitProcessing = true;
    }
    
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( getLiveInstance() );
    
    if (isOutput) {
        isOutput->getRenderEngine()->quitEngine();
    }
    _imp->abortPreview();
    
}

Node::~Node()
{
    _imp->liveInstance.reset();
}

void
Node::removeReferences(bool ensureThreadsFinished)
{
    if (!_imp->liveInstance) {
        return;
    }
    if (ensureThreadsFinished) {
        getApp()->getProject()->ensureAllProcessingThreadsFinished();
    }
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>(_imp->liveInstance.get());
    if (isOutput) {
        isOutput->getRenderEngine()->quitEngine();
    }
    appPTR->removeAllImagesFromCacheWithMatchingKey( getHashValue() );
    deleteNodeVariableToPython(getFullyQualifiedName());
    _imp->liveInstance.reset();
    if (getGroup()) {
        getGroup()->removeNode(shared_from_this());
    }
}

const std::vector<std::string> &
Node::getInputLabels() const
{
    assert(_imp->inputsInitialized);
    ///MT-safe as it never changes.
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->inputLabels;
}

const std::list<Node* > &
Node::getOutputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->outputs;
}

void
Node::getOutputs_mt_safe(std::list<Node* >& outputs) const
{
    QMutexLocker l(&_imp->outputsMutex);
    outputs =  _imp->outputs;
}

void
Node::getInputNames(std::map<std::string,std::string> & inputNames) const
{
    ///This is called by the serialization thread.
    ///We use the guiInputs because we want to serialize exactly how the tree was to the user
    
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        parent->getInputNames(inputNames);
        return;
    }
    
    QMutexLocker l(&_imp->inputsMutex);
    assert(_imp->inputs.size() == _imp->inputLabels.size());
    for (U32 i = 0;i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i]) {
            inputNames.insert(std::make_pair(_imp->inputLabels[i], _imp->inputs[i]->getScriptName_mt_safe()) );
        } else {
            inputNames.insert(std::make_pair(_imp->inputLabels[i],""));
        }
    }
}

int
Node::getPreferredInputInternal(bool connected) const
{
    if (getMaxInputCount() == 0) {
        return -1;
    }
    
    {
        ///Find an input named Source
        std::string inputNameToFind(kOfxImageEffectSimpleSourceClipName);
        int maxinputs = getMaxInputCount();
        for (int i = 0; i < maxinputs ; ++i) {
            if (getInputLabel(i) == inputNameToFind) {
                NodePtr inp = getInput(i);
                if ((connected && inp) || (!connected && !inp)) {
                    return i;
                }
            }
        }
    }
    
    
    bool useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();
    
    ///Find an input named A
    std::string inputNameToFind,otherName;
    if (useInputA) {
        inputNameToFind = "A";
        otherName = "B";
    } else {
        inputNameToFind = "B";
        otherName = "A";
    }
    int foundOther = -1;
    int maxinputs = getMaxInputCount();
    for (int i = 0; i < maxinputs ; ++i) {
        std::string inputLabel = getInputLabel(i);
        if (inputLabel == inputNameToFind ) {
            NodePtr inp = getInput(i);
            if ((connected && inp) || (!connected && !inp)) {
                return i;
            }
        } else if (inputLabel == otherName) {
            foundOther = i;
        }
    }
    if (foundOther != -1) {
        NodePtr inp = getInput(foundOther);
        if ((connected && inp) || (!connected && !inp)) {
            return foundOther;
        }
    }
    
    
    
    
    ///we return the first non-optional empty input
    int firstNonOptionalEmptyInput = -1;
    std::list<int> optionalEmptyInputs;
    std::list<int> optionalEmptyMasks;
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            if (_imp->liveInstance->isInputRotoBrush(i)) {
                continue;
            }
            
            if ((connected && _imp->inputs[i]) || (!connected && !_imp->inputs[i])) {
                if ( !_imp->liveInstance->isInputOptional(i) ) {
                    if (firstNonOptionalEmptyInput == -1) {
                        firstNonOptionalEmptyInput = i;
                        break;
                    }
                } else {
                    if (_imp->liveInstance->isInputMask(i)) {
                        optionalEmptyMasks.push_back(i);
                    } else {
                        optionalEmptyInputs.push_back(i);
                    }
                }
            }
        }
    }
    
    
    ///Default to the first non optional empty input
    if (firstNonOptionalEmptyInput != -1) {
        return firstNonOptionalEmptyInput;
    }  else {
        if ( !optionalEmptyInputs.empty() ) {
            
            //We return the last optional empty input
            std::list<int>::reverse_iterator first = optionalEmptyInputs.rbegin();
            while ( first != optionalEmptyInputs.rend() && _imp->liveInstance->isInputRotoBrush(*first) ) {
                ++first;
            }
            if ( first == optionalEmptyInputs.rend() ) {
                return -1;
            } else {
                return *first;
            }
            
        } else if (!optionalEmptyMasks.empty()) {
            return optionalEmptyMasks.front();
        } else {
            return -1;
        }
    }

}

int
Node::getPreferredInput() const
{
    return getPreferredInputInternal(true);
}

int
Node::getPreferredInputForConnection() const
{
    return getPreferredInputInternal(false);
}

void
Node::getOutputsConnectedToThisNode(std::map<Node*,int>* outputs)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    for (std::list<Node*>::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
        assert(*it);
        int indexOfThis = (*it)->inputIndex(this);
        assert(indexOfThis != -1);
        if (indexOfThis >= 0) {
            outputs->insert( std::make_pair(*it, indexOfThis) );
        }
    }
}

const std::string &
Node::getScriptName() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->nameMutex);
    
    return _imp->scriptName;
}

std::string
Node::getScriptName_mt_safe() const
{
    QMutexLocker l(&_imp->nameMutex);
    
    return _imp->scriptName;
}

static void prependGroupNameRecursive(const boost::shared_ptr<Natron::Node>& group,std::string& name)
{
    name.insert(0,".");
    name.insert(0, group->getScriptName_mt_safe());
    boost::shared_ptr<NodeCollection> hasParentGroup = group->getGroup();
    boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>(hasParentGroup);
    if (isGrp) {
        prependGroupNameRecursive(isGrp->getNode(), name);
    }
}

std::string
Node::getFullyQualifiedName() const
{
    std::string ret = getScriptName_mt_safe();
    NodePtr parent = getParentMultiInstance();
    if (parent) {
        prependGroupNameRecursive(parent, ret);
    } else {
        boost::shared_ptr<NodeCollection> hasParentGroup = getGroup();
        boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>(hasParentGroup);
        if (isGrp) {
            prependGroupNameRecursive(isGrp->getNode(), ret);
        }
    }
    return ret;
}

void
Node::setLabel(const std::string& label)
{
    assert(QThread::currentThread() == qApp->thread());
    if (dynamic_cast<GroupOutput*>(_imp->liveInstance.get())) {
        return ;
    }
    
    {
        QMutexLocker k(&_imp->nameMutex);
        _imp->label = label;
    }
    boost::shared_ptr<NodeCollection> collection = getGroup();
    if (collection) {
        collection->notifyNodeNameChanged(shared_from_this());
    }
    Q_EMIT labelChanged(QString(label.c_str()));

}

const std::string&
Node::getLabel() const
{
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker k(&_imp->nameMutex);
    return _imp->label;
}

std::string
Node::getLabel_mt_safe() const
{
    QMutexLocker k(&_imp->nameMutex);
    return _imp->label;
}

void
Node::setScriptName_no_error_check(const std::string & name)
{
    setNameInternal(name);
}

void
Node::setNameInternal(const std::string& name)
{
    std::string oldName = getScriptName_mt_safe();
    std::string fullOldName = getFullyQualifiedName();
    std::string newName = name;
    
    boost::shared_ptr<NodeCollection> collection = getGroup();
    if (collection) {
        collection->setNodeName(name,false, false, &newName);
    }
    
    {
        QMutexLocker l(&_imp->nameMutex);
        _imp->scriptName = newName;
        ///Set the label at the same time
        _imp->label = newName;
    }
    
    if (collection) {
        std::string fullySpecifiedName = getFullyQualifiedName();
        if (!oldName.empty()) {
            
            if (fullOldName != fullySpecifiedName) {
                try {
                    setNodeVariableToPython(fullOldName,fullySpecifiedName);
                } catch (const std::exception& e) {
                    qDebug() << e.what();
                }
                
                const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
                
                for (U32 i = 0; i < knobs.size(); ++i) {
                    std::list<KnobI*> listeners;
                    knobs[i]->getListeners(listeners);
                    ///For all listeners make sure they belong to a node
                    bool foundEffect = false;
                    for (std::list<KnobI*>::iterator it2 = listeners.begin(); it2 != listeners.end(); ++it2) {
                        EffectInstance* isEffect = dynamic_cast<EffectInstance*>( (*it2)->getHolder() );
                        if ( isEffect && ( isEffect != _imp->liveInstance.get() ) ) {
                            foundEffect = true;
                            break;
                        }
                    }
                    if (foundEffect) {
                        Natron::warningDialog( tr("Rename").toStdString(), tr("This node has one or several "
                                                                              "parameters from which other parameters "
                                                                              "of the project rely on through expressions "
                                                                              "or links. Changing the name of this node will probably "
                                                                              "break these expressions. You should carefully update them. ")
                                              .toStdString() );
                        break;
                    }
                }
            }
        } else { //if (!oldName.empty()) {
            declareNodeVariableToPython(fullySpecifiedName);
        }
    }
    
    QString qnewName(newName.c_str());
    Q_EMIT scriptNameChanged(qnewName);
    Q_EMIT labelChanged(qnewName);
}

bool
Node::setScriptName(const std::string& name)
{
    std::string newName;
    if (getGroup()) {
        if (!getGroup()->setNodeName(name,false, true, &newName)) {
            return false;
        }
    } else {
        newName = name;
    }
    
    if (dynamic_cast<GroupOutput*>(_imp->liveInstance.get())) {
        return false;
    }
    
    
    setNameInternal(newName);
    return true;
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

std::string
Node::makeInfoForInput(int inputNumber) const
{
    const Natron::Node* inputNode = 0;
    std::string inputName ;
    if (inputNumber != -1) {
        inputNode = getInput(inputNumber).get();
        inputName = _imp->liveInstance->getInputLabel(inputNumber);
    } else {
        inputNode = this;
        inputName = "Output";
    }

    if (!inputNode) {
        return inputName + ": disconnected";
    }
    
    std::list<Natron::ImageComponents> comps;
    Natron::ImageBitDepthEnum depth;
    Natron::ImagePremultiplicationEnum premult;
    
    double par = inputNode->getLiveInstance()->getPreferredAspectRatio();
    premult = inputNode->getLiveInstance()->getOutputPremultiplication();
    
    std::string premultStr;
    switch (premult) {
        case Natron::eImagePremultiplicationOpaque:
            premultStr = "opaque";
            break;
        case Natron::eImagePremultiplicationPremultiplied:
            premultStr= "premultiplied";
            break;
        case Natron::eImagePremultiplicationUnPremultiplied:
            premultStr= "unpremultiplied";
            break;
    }
    
    _imp->liveInstance->getPreferredDepthAndComponents(inputNumber, &comps, &depth);
    assert(!comps.empty());
    
    EffectInstance::ComponentsAvailableMap availableComps;
    _imp->liveInstance->getComponentsAvailable(getApp()->getTimeLine()->currentFrame(), &availableComps);
    
    RenderScale scale;
    scale.x = scale.y = 1.;
    RectD rod;
    bool isProjectFormat;
    StatusEnum stat = inputNode->getLiveInstance()->getRegionOfDefinition_public(getHashValue(),
                                                                                 inputNode->getLiveInstance()->getCurrentTime(),
                                                                                 scale, 0, &rod, &isProjectFormat);
    
    
    std::stringstream ss;
    ss << "<b><font color=\"orange\">"<< inputName << ":\n" << "</font></b>"
    << "<b>Image Format:</b>\n";
    
    EffectInstance::ComponentsAvailableMap::iterator next = availableComps.begin();
    if (!comps.empty()) {
        ++next;
    }
    for (EffectInstance::ComponentsAvailableMap::iterator it = availableComps.begin(); it!=availableComps.end(); ++it) {
        NodePtr origin = it->second.lock();
        if (origin.get() != this || inputNumber == -1) {
            ss << Natron::Image::getFormatString(it->first, depth);
            if (origin) {
                ss << ": (" << origin->getLabel_mt_safe() << ")";
            }
        }
        if (next != availableComps.end()) {
            if (origin.get() != this || inputNumber == -1) {
                ss << "\n";
            }
            ++next;
        }
    }
    
    ss << "\n<b>Alpha premultiplication:</b> " << premultStr
    << "\n<b>Pixel aspect ratio:</b> " << par;
    if (stat != Natron::eStatusFailed) {
        ss << "\n<b>Region of Definition:</b> ";
        ss << "left = " << rod.x1 << " bottom = " << rod.y1 << " right = " << rod.x2 << " top = " << rod.y2 << '\n';
    }
    return ss.str();
}

void
Node::initializeKnobs(int renderScaleSupportPref)
{
    ////Only called by the main-thread
    _imp->liveInstance->beginChanges();
    
    assert( QThread::currentThread() == qApp->thread() );
    assert(!_imp->knobsInitialized);
    
    BackDrop* isBd = dynamic_cast<BackDrop*>(_imp->liveInstance.get());
    Dot* isDot = dynamic_cast<Dot*>(_imp->liveInstance.get());
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->liveInstance.get());
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->liveInstance.get());
    
    ///For groups, declare the plugin knobs after the node knobs because we want to use the Node page
    if (!isGroup) {
        _imp->liveInstance->initializeKnobsPublic();
    }

    ///If the effect has a mask, add additionnal mask controls
    int inputsCount = getMaxInputCount();
    
    if (!isDot && !isViewer) {
        _imp->nodeSettingsPage = Natron::createKnob<Page_Knob>(_imp->liveInstance.get(), NATRON_PARAMETER_PAGE_NAME_EXTRA,1,false);
        
        if (!isBd) {
            
            int foundA = -1;
            int foundB = -1;
            for (int i = 0; i < inputsCount; ++i) {
                
                std::string maskName = _imp->liveInstance->getInputLabel(i);
                if (maskName == "A") {
                    foundA = i;
                } else if (maskName == "B") {
                    foundB = i;
                }

                if ( _imp->liveInstance->isInputMask(i) && !_imp->liveInstance->isInputRotoBrush(i) ) {
                    
                    MaskSelector sel;
                    sel.enabled = Natron::createKnob<Bool_Knob>(_imp->liveInstance.get(), maskName,1,false);

                    sel.enabled->setDefaultValue(false, 0);
                    sel.enabled->setAddNewLine(false);
                    std::string enableMaskName(std::string(kEnableMaskKnobName) + std::string("_") + maskName);
                    sel.enabled->setName(enableMaskName);
                    sel.enabled->setAnimationEnabled(false);
                    sel.enabled->setHintToolTip(tr("Enable the mask to come from the channel named by the choice parameter on the right. "
                                                      "Turning this off will act as though the mask was disconnected.").toStdString());
                    
                    
                    sel.channel = Natron::createKnob<Choice_Knob>(_imp->liveInstance.get(), "",1,false);
                    
                    std::vector<std::string> choices;
                    choices.push_back("None");
                    sel.channel->populateChoices(choices);
                    sel.channel->setDefaultValue(0, 0);
                    sel.channel->setAnimationEnabled(false);
                    sel.channel->setHintToolTip(tr("Use this channel from the original input to mix the output with the original input. "
                                                       "Setting this to None is the same as disabling the mask.").toStdString());
                    std::string channelMaskName(kMaskChannelKnobName + std::string("_") + maskName);
                    sel.channel->setName(channelMaskName);
                    
                    sel.channelName = Natron::createKnob<String_Knob>(_imp->liveInstance.get(), "",1,false);
                    sel.channelName->setSecret(true);
                    sel.channelName->setEvaluateOnChange(false);
                    
                    _imp->maskSelectors[i] = sel;

                }
            }
            
            bool useChannels = !_imp->liveInstance->isMultiPlanar() &&
            !_imp->liveInstance->isReader() &&
            !_imp->liveInstance->isWriter() &&
            !_imp->liveInstance->isTrackerNode() &&
            !dynamic_cast<NodeGroup*>(_imp->liveInstance.get());
            
            if (useChannels) {
                const std::vector< boost::shared_ptr<KnobI> > & knobs = _imp->liveInstance->getKnobs();
                ///find in all knobs a page param to set this param into
                boost::shared_ptr<Page_Knob> mainPage;
                for (U32 i = 0; i < knobs.size(); ++i) {
                    boost::shared_ptr<Page_Knob> p = boost::dynamic_pointer_cast<Page_Knob>(knobs[i]);
                    if ( p && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_INFO) &&
                        (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
                        mainPage = p;
                        break;
                    }
                }
                if (!mainPage) {
                    mainPage = Natron::createKnob<Page_Knob>(_imp->liveInstance.get(), "Settings");
                }
                assert(mainPage);
                
                //There are a A and B inputs and the plug-in is not multi-planar, propose 2 layer selectors for the inputs.
                if (foundA != -1 && foundB != -1) {
                    _imp->createChannelSelector(foundA,"A", false, mainPage);
                    _imp->createChannelSelector(foundB,"B", false, mainPage);
                    
                }
                _imp->createChannelSelector(-1, "Output", true, mainPage);
            }
        }
        _imp->nodeLabelKnob = Natron::createKnob<String_Knob>(_imp->liveInstance.get(),
                                                              isBd ? tr("Name label").toStdString() : tr("Label").toStdString(),1,false);
        assert(_imp->nodeLabelKnob);
        _imp->nodeLabelKnob->setName(kUserLabelKnobName);
        _imp->nodeLabelKnob->setAnimationEnabled(false);
        _imp->nodeLabelKnob->setEvaluateOnChange(false);
        _imp->nodeLabelKnob->setAsMultiLine();
        _imp->nodeLabelKnob->setUsesRichText(true);
        _imp->nodeLabelKnob->setHintToolTip(tr("This label gets appended to the node name on the node graph.").toStdString());
        _imp->nodeSettingsPage->addKnob(_imp->nodeLabelKnob);
        
        if (!isBd) {
            _imp->forceCaching = Natron::createKnob<Bool_Knob>(_imp->liveInstance.get(), "Force caching", 1, false);
            _imp->forceCaching->setName("forceCaching");
            _imp->forceCaching->setDefaultValue(false);
            _imp->forceCaching->setAnimationEnabled(false);
            _imp->forceCaching->setAddNewLine(false);
            _imp->forceCaching->setIsPersistant(true);
            _imp->forceCaching->setEvaluateOnChange(false);
            _imp->forceCaching->setHintToolTip(tr("When checked, the output of this node will always be kept in the RAM cache for fast access of already computed "
                                                  "images.").toStdString());
            _imp->nodeSettingsPage->addKnob(_imp->forceCaching);
            
            _imp->previewEnabledKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance.get(), tr("Preview enabled").toStdString(),1,false);
            assert(_imp->previewEnabledKnob);
            _imp->previewEnabledKnob->setDefaultValue( makePreviewByDefault() );
            _imp->previewEnabledKnob->setName(kEnablePreviewKnobName);
            _imp->previewEnabledKnob->setAnimationEnabled(false);
            _imp->previewEnabledKnob->setAddNewLine(false);
            _imp->previewEnabledKnob->setIsPersistant(false);
            _imp->previewEnabledKnob->setEvaluateOnChange(false);
            _imp->previewEnabledKnob->setHintToolTip(tr("Whether to show a preview on the node box in the node-graph.").toStdString());
            _imp->nodeSettingsPage->addKnob(_imp->previewEnabledKnob);
            
            _imp->disableNodeKnob = Natron::createKnob<Bool_Knob>(_imp->liveInstance.get(), "Disable",1,false);
            assert(_imp->disableNodeKnob);
            _imp->disableNodeKnob->setAnimationEnabled(false);
            _imp->disableNodeKnob->setDefaultValue(false);
            _imp->disableNodeKnob->setName(kDisableNodeKnobName);
            _imp->disableNodeKnob->setAddNewLine(false);
            _imp->disableNodeKnob->setHintToolTip("When disabled, this node acts as a pass through.");
            _imp->nodeSettingsPage->addKnob(_imp->disableNodeKnob);
            
            _imp->useFullScaleImagesWhenRenderScaleUnsupported = Natron::createKnob<Bool_Knob>(_imp->liveInstance.get(), tr("Render high def. upstream").toStdString(),1,false);
            _imp->useFullScaleImagesWhenRenderScaleUnsupported->setAnimationEnabled(false);
            _imp->useFullScaleImagesWhenRenderScaleUnsupported->setDefaultValue(false);
            _imp->useFullScaleImagesWhenRenderScaleUnsupported->setName("highDefUpstream");
            _imp->useFullScaleImagesWhenRenderScaleUnsupported->setHintToolTip(tr("This node doesn't support rendering images at a scale lower than 1, it "
                                                                                  "can only render high definition images. When checked this parameter controls "
                                                                                  "whether the rest of the graph upstream should be rendered with a high quality too or at "
                                                                                  "the most optimal resolution for the current viewer's viewport. Typically checking this "
                                                                                  "means that an image will be slow to be rendered, but once rendered it will stick in the cache "
                                                                                  "whichever zoom level you're using on the Viewer, whereas when unchecked it will be much "
                                                                                  "faster to render but will have to be recomputed when zooming in/out in the Viewer.").toStdString());
            if (renderScaleSupportPref == 0 && getLiveInstance()->supportsRenderScaleMaybe() == EffectInstance::eSupportsYes) {
                _imp->useFullScaleImagesWhenRenderScaleUnsupported->setSecret(true);
            }
            _imp->nodeSettingsPage->addKnob(_imp->useFullScaleImagesWhenRenderScaleUnsupported);
            
            _imp->knobChangedCallback = Natron::createKnob<String_Knob>(_imp->liveInstance.get(), tr("After param changed callback").toStdString());
            _imp->knobChangedCallback->setHintToolTip(tr("Set here the name of a function defined in Python which will be called for each  "
                                                         "parameter change. Either define this function in the Script Editor "
                                                         "or in the init.py script or even in the script of a Python group plug-in.\n"
                                                         "The signature of the callback is: callback(thisParam, thisNode, thisGroup, app, userEdited) where:\n"
                                                         "- thisParam: The parameter which just had its value changed\n"
                                                         "- userEdited: A boolean informing whether the change was due to user interaction or "
                                                         "because something internally triggered the change.\n"
                                                         "- thisNode: The node holding the parameter\n"
                                                         "- app: points to the current application instance\n"
                                                         "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)").toStdString());
            _imp->knobChangedCallback->setAnimationEnabled(false);
            _imp->knobChangedCallback->setName("onParamChanged");
            _imp->nodeSettingsPage->addKnob(_imp->knobChangedCallback);
            
            _imp->inputChangedCallback = Natron::createKnob<String_Knob>(_imp->liveInstance.get(), tr("After input changed callback").toStdString());
            _imp->inputChangedCallback->setHintToolTip(tr("Set here the name of a function defined in Python which will be called after "
                                                          "each connection is changed for the inputs of the node. "
                                                          "Either define this function in the Script Editor "
                                                          "or in the init.py script or even in the script of a Python group plug-in.\n"
                                                          "The signature of the callback is: callback(inputIndex, thisNode, thisGroup, app):\n"
                                                          "- inputIndex: the index of the input which changed, you can query the node "
                                                          "connected to the input by calling the getInput(...) function.\n"
                                                          "- thisNode: The node holding the parameter\n"
                                                          "- app: points to the current application instance\n"
                                                          "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)").toStdString());
            
            _imp->inputChangedCallback->setAnimationEnabled(false);
            _imp->inputChangedCallback->setName("onInputChanged");
            _imp->nodeSettingsPage->addKnob(_imp->inputChangedCallback);
            
            
            _imp->infoPage = Natron::createKnob<Page_Knob>(_imp->liveInstance.get(), tr("Info").toStdString(), 1, false);
            _imp->infoPage->setName(NATRON_PARAMETER_PAGE_NAME_INFO);
            
            _imp->infoDisclaimer = Natron::createKnob<String_Knob>(_imp->liveInstance.get(), tr("Input and output informations").toStdString(), 1, false);
            _imp->infoDisclaimer->setName("infoDisclaimer");
            _imp->infoDisclaimer->setAnimationEnabled(false);
            _imp->infoDisclaimer->setIsPersistant(false);
            _imp->infoDisclaimer->setAsLabel();
            _imp->infoDisclaimer->hideDescription();
            _imp->infoDisclaimer->setEvaluateOnChange(false);
            _imp->infoDisclaimer->setDefaultValue(tr("Input and output informations, press Refresh to update them with current values").toStdString());
            _imp->infoPage->addKnob(_imp->infoDisclaimer);
            
            for (int i = 0; i < inputsCount; ++i) {
                std::string inputLabel = getInputLabel(i);
                boost::shared_ptr<String_Knob> inputInfo = Natron::createKnob<String_Knob>(_imp->liveInstance.get(), inputLabel + ' ' + tr("Info").toStdString(), 1, false);
                inputInfo->setName(inputLabel + "Info");
                inputInfo->setAnimationEnabled(false);
                inputInfo->setIsPersistant(false);
                inputInfo->setEvaluateOnChange(false);
                inputInfo->hideDescription();
                inputInfo->setAsLabel();
                _imp->inputFormats.push_back(inputInfo);
                _imp->infoPage->addKnob(inputInfo);
            }
            
            std::string outputLabel("Output");
            _imp->outputFormat = Natron::createKnob<String_Knob>(_imp->liveInstance.get(), std::string(outputLabel + " Info"), 1, false);
            _imp->outputFormat->setName(outputLabel + "Info");
            _imp->outputFormat->setAnimationEnabled(false);
            _imp->outputFormat->setIsPersistant(false);
            _imp->outputFormat->setEvaluateOnChange(false);
            _imp->outputFormat->hideDescription();
            _imp->outputFormat->setAsLabel();
            _imp->infoPage->addKnob(_imp->outputFormat);
            
            _imp->refreshInfoButton = Natron::createKnob<Button_Knob>(_imp->liveInstance.get(), tr("Refresh Info").toStdString(),1,false);
            _imp->refreshInfoButton->setName("refreshButton");
            _imp->refreshInfoButton->setEvaluateOnChange(false);
            _imp->infoPage->addKnob(_imp->refreshInfoButton);
            
            if (_imp->liveInstance->isWriter()) {
                boost::shared_ptr<Page_Knob> pythonPage = Natron::createKnob<Page_Knob>(_imp->liveInstance.get(), tr("Python").toStdString(),1,false);
                
                _imp->beforeFrameRender =  Natron::createKnob<String_Knob>(_imp->liveInstance.get(), tr("Before frame render").toStdString(), 1 ,false);
                _imp->beforeFrameRender->setName("beforeFrameRender");
                _imp->beforeFrameRender->setAnimationEnabled(false);
                _imp->beforeFrameRender->setHintToolTip(tr("Add here the name of a Python defined function that will be called before rendering "
                                                           "any frame.\n "
                                                           "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                                           "- frame: the frame to be rendered\n"
                                                           "- thisNode: points to the writer node\n"
                                                           "- app: points to the current application instance").toStdString());
                pythonPage->addKnob(_imp->beforeFrameRender);
                
                _imp->beforeRender =  Natron::createKnob<String_Knob>(_imp->liveInstance.get(), tr("Before render").toStdString(),1,false);
                _imp->beforeRender->setName("beforeRender");
                _imp->beforeRender->setAnimationEnabled(false);
                _imp->beforeRender->setHintToolTip(tr("Add here the name of a Python defined function that will be called once when "
                                                      "starting rendering.\n "
                                                      "The signature of the callback is: callback(thisNode, app) where:\n"
                                                      "- thisNode: points to the writer node\n"
                                                      "- app: points to the current application instance").toStdString());
                pythonPage->addKnob(_imp->beforeRender);
                
                _imp->afterFrameRender =  Natron::createKnob<String_Knob>(_imp->liveInstance.get(), tr("After frame render").toStdString(),1,false);
                _imp->afterFrameRender->setName("afterFrameRender");
                _imp->afterFrameRender->setAnimationEnabled(false);
                _imp->afterFrameRender->setHintToolTip(tr("Add here the name of a Python defined function that will be called after rendering "
                                                          "any frame.\n "
                                                          "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                                          "- frame: the frame that has been rendered\n"
                                                          "- thisNode: points to the writer node\n"
                                                          "- app: points to the current application instance").toStdString());
                pythonPage->addKnob(_imp->afterFrameRender);
                
                _imp->afterRender =  Natron::createKnob<String_Knob>(_imp->liveInstance.get(), tr("After render").toStdString(),1,false);
                _imp->afterRender->setName("afterRender");
                _imp->afterRender->setAnimationEnabled(false);
                _imp->afterRender->setHintToolTip(tr("Add here the name of a Python defined function that will be called once when the rendering "
                                                     "is finished.\n "
                                                     "The signature of the callback is: callback(aborted, thisNode, app) where:\n"
                                                     "- aborted: True if the render ended because it was aborted, False upon completion\n"
                                                     "- thisNode: points to the writer node\n"
                                                     "- app: points to the current application instance").toStdString());
                pythonPage->addKnob(_imp->afterRender);
            }
        }
    }
  

    if (isGroup) {
        _imp->liveInstance->initializeKnobsPublic();
    }
    
    _imp->knobsInitialized = true;

    _imp->liveInstance->endChanges();
    Q_EMIT knobsInitialized();
} // initializeKnobs

void
Node::Implementation::createChannelSelector(int inputNb,const std::string & inputName,bool isOutput,
                                            const boost::shared_ptr<Page_Knob>& page)
{
    
    ChannelSelector sel;
    sel.useRGBASelectors = isOutput;
    sel.hasAllChoice = isOutput;
    sel.layer = Natron::createKnob<Choice_Knob>(liveInstance.get(), isOutput ? "Channels" : inputName + " Channels", 1, false);
    sel.layer->setAddNewChoice(isOutput);
    sel.layer->setName(inputName + "_channels");
    if (isOutput) {
        sel.layer->setHintToolTip("Select here the channels onto which the processing should occur.");
    } else {
        sel.layer->setHintToolTip("Select here the channels that will be used by the input " + inputName);
    }
    sel.layer->setAnimationEnabled(false);
    if (sel.useRGBASelectors) {
        sel.layer->setAddNewLine(false);
    }
    page->addKnob(sel.layer);
    std::vector<std::string> baseLayers;
    if (isOutput) {
        baseLayers.push_back("All");
    } else {
        baseLayers.push_back("None");
    }
    baseLayers.push_back(ImageComponents::getRGBAComponents().getComponentsGlobalName());
    baseLayers.push_back(ImageComponents::getDisparityLeftComponents().getLayerName());
    baseLayers.push_back(ImageComponents::getDisparityRightComponents().getLayerName());
    baseLayers.push_back(ImageComponents::getForwardMotionComponents().getLayerName());
    baseLayers.push_back(ImageComponents::getBackwardMotionComponents().getLayerName());
    sel.layer->populateChoices(baseLayers);
    if (isOutput && liveInstance->isPassThroughForNonRenderedPlanes() == EffectInstance::ePassThroughRenderAllRequestedPlanes) {
        sel.layer->setDefaultValue(0);
    } else {
        sel.layer->setDefaultValue(1);
    }
    
    sel.layerName = Natron::createKnob<String_Knob>(liveInstance.get(), inputName + "_layer_name", 1, false);
    sel.layerName->setSecret(true);
    sel.layerName->setAnimationEnabled(false);
    sel.layerName->setEvaluateOnChange(false);
    sel.layerName->setAddNewLine(!sel.useRGBASelectors);
    page->addKnob(sel.layerName);
    
    if (sel.useRGBASelectors) {
        
        std::string channelNames[4] = {"R", "G", "B", "A"};
        for (int i = 0; i < 4; ++i) {
            sel.enabledChan[i] = Natron::createKnob<Bool_Knob>(liveInstance.get(), channelNames[i], 1, false);
            sel.enabledChan[i]->setName(inputName + "_enable_" + channelNames[i]);
            sel.enabledChan[i]->setAnimationEnabled(false);
            sel.enabledChan[i]->setAddNewLine(i == 3);
            sel.enabledChan[i]->setDefaultValue(true);
            sel.enabledChan[i]->setHintToolTip("When checked the corresponding channel of the layer will be used, "
                                               "otherwise it will be considered to be 0 everywhere.");
            page->addKnob(sel.enabledChan[i]);
        }
    }
    channelsSelectors[inputNb] = sel;
    
}

bool
Node::isForceCachingEnabled() const
{
    return _imp->forceCaching->getValue();
}

void
Node::onSetSupportRenderScaleMaybeSet(int support)
{
    if ((EffectInstance::SupportsEnum)support == EffectInstance::eSupportsYes) {
        if (_imp->useFullScaleImagesWhenRenderScaleUnsupported) {
            _imp->useFullScaleImagesWhenRenderScaleUnsupported->setSecret(true);
        }
    }
}

bool
Node::useScaleOneImagesWhenRenderScaleSupportIsDisabled() const
{
    return _imp->useFullScaleImagesWhenRenderScaleUnsupported->getValue();
}

void
Node::beginEditKnobs()
{
    _imp->liveInstance->beginEditKnobs();
}


void
Node::setLiveInstance(const boost::shared_ptr<Natron::EffectInstance>& liveInstance)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    _imp->liveInstance = liveInstance;
    _imp->liveInstance->initializeData();
}

Natron::EffectInstance*
Node::getLiveInstance() const
{
#pragma message WARN("We should fix it and return a shared_ptr instead, but this is a tedious work since it is used everywhere")
    ///Thread safe as it never changes
    return _imp->liveInstance.get();
}

bool
Node::hasEffect() const
{
    return _imp->liveInstance != NULL;
}

void
Node::hasViewersConnected(std::list<ViewerInstance* >* viewers) const
{
    ViewerInstance* thisViewer = dynamic_cast<ViewerInstance*>(_imp->liveInstance.get());
    
    if (thisViewer) {
        std::list<ViewerInstance* >::const_iterator alreadyExists = std::find(viewers->begin(), viewers->end(), thisViewer);
        if ( alreadyExists == viewers->end() ) {
            viewers->push_back(thisViewer);
        }
    } else {
        std::list<Node*> outputs;
        getOutputsWithGroupRedirection(outputs);
     
        for (std::list<Node*>::iterator it = outputs.begin(); it != outputs.end(); ++it) {
            assert(*it);
            (*it)->hasViewersConnected(viewers);
        }
        
    }
}

void
Node::getOutputsWithGroupRedirection(std::list<Node*>& outputs) const
{
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(_imp->liveInstance.get());
    GroupOutput* isOutput = dynamic_cast<GroupOutput*>(_imp->liveInstance.get());
    if (isGrp) {
        isGrp->getInputsOutputs(&outputs);
    } else if (isOutput) {
        boost::shared_ptr<NodeCollection> collection = isOutput->getNode()->getGroup();
        assert(collection);
        isGrp = dynamic_cast<NodeGroup*>(collection.get());
        if (isGrp) {
            
            std::list<Node*> groupOutputs;
            isGrp->getNode()->getOutputs_mt_safe(groupOutputs);
            for (std::list<Node*>::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                outputs.push_back(*it2);
            }
        }
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        outputs.insert(outputs.begin(), _imp->outputs.begin(), _imp->outputs.end());
    }
    
}

void
Node::hasWritersConnected(std::list<Natron::OutputEffectInstance* >* writers) const
{
    Natron::OutputEffectInstance* thisWriter = dynamic_cast<Natron::OutputEffectInstance*>(_imp->liveInstance.get());
    
    if (thisWriter) {
        std::list<Natron::OutputEffectInstance* >::const_iterator alreadyExists = std::find(writers->begin(), writers->end(), thisWriter);
        if ( alreadyExists == writers->end() ) {
            writers->push_back(thisWriter);
        }
    } else {
        if ( QThread::currentThread() == qApp->thread() ) {
            for (std::list<Node*>::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
                assert(*it);
                (*it)->hasWritersConnected(writers);
            }
        } else {
            QMutexLocker l(&_imp->outputsMutex);
            for (std::list<Node*>::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
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
    assert( QThread::currentThread() == qApp->thread() );
    int inputCount = getMaxInputCount();
    
    InputsV oldInputs;
    {
        QMutexLocker l(&_imp->inputsMutex);
        oldInputs = _imp->inputs;
        _imp->inputs.resize(inputCount);
        _imp->inputLabels.resize(inputCount);
        ///if we added inputs, just set to NULL the new inputs, and add their label to the labels map
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputLabels[i] = _imp->liveInstance->getInputLabel(i);
            if (i < (int)oldInputs.size()) {
                _imp->inputs[i] = oldInputs[i];
            } else {
                _imp->inputs[i].reset();
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
    Q_EMIT inputsInitialized();
}

boost::shared_ptr<Node>
Node::getInput(int index) const
{
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getInput(index);
    }
    if (!_imp->inputsInitialized) {
        qDebug() << "Node::getInput(): inputs not initialized";
    }
    QMutexLocker l(&_imp->inputsMutex);
    if ( ( index >= (int)_imp->inputs.size() ) || (index < 0) ) {
        return boost::shared_ptr<Node>();
    }
    
    boost::shared_ptr<Node> ret =  _imp->inputs[index];
    if (ret) {
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>(ret->getLiveInstance());
        if (isGrp) {
            ret =  isGrp->getOutputNodeInput();
        }
        
        if (ret) {
            GroupInput* isInput = dynamic_cast<GroupInput*>(ret->getLiveInstance());
            if (isInput) {
                boost::shared_ptr<NodeCollection> collection = ret->getGroup();
                assert(collection);
                isGrp = dynamic_cast<NodeGroup*>(collection.get());
                if (isGrp) {
                    ret = isGrp->getRealInputForInput(ret);
                }
            }
        }
        
    }
    return ret;
}

boost::shared_ptr<Node>
Node::getRealInput(int index) const
{
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getInput(index);
    }
    if (!_imp->inputsInitialized) {
        qDebug() << "Node::getInput(): inputs not initialized";
    }
    QMutexLocker l(&_imp->inputsMutex);
    if ( ( index >= (int)_imp->inputs.size() ) || (index < 0) ) {
        return boost::shared_ptr<Node>();
    }
    
    return _imp->inputs[index];

}

int
Node::getInputIndex(const Natron::Node* node) const
{
    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].get() == node) {
            return i;
        }
    }
    return -1;
}

const std::vector<boost::shared_ptr<Natron::Node> > &
Node::getInputs_mt_safe() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getInputs_mt_safe();
    }
    
    return _imp->inputs;
}

std::vector<boost::shared_ptr<Natron::Node> >
Node::getInputs_copy() const
{
    assert(_imp->inputsInitialized);
    
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getInputs_mt_safe();
    }
    
    QMutexLocker l(&_imp->inputsMutex);
    return _imp->inputs;
}

std::string
Node::getInputLabel(int inputNb) const
{
    assert(_imp->inputsInitialized);
    
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputLabels.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }
    
    return _imp->inputLabels[inputNb];
}

int
Node::getInputNumberFromLabel(const std::string& inputLabel) const
{
    assert(_imp->inputsInitialized);
    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputLabels.size(); ++i) {
        if (_imp->inputLabels[i] == inputLabel) {
            return i;
        }
    }
    return -1;
}

bool
Node::isInputConnected(int inputNb) const
{
    assert(_imp->inputsInitialized);
    
    return getInput(inputNb) != NULL;
}

bool
Node::hasInputConnected() const
{
    assert(_imp->inputsInitialized);
    
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->hasInputConnected();
    }
    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i]) {
            return true;
        }
    }
    
    
    return false;
}

bool
Node::hasMandatoryInputDisconnected() const
{
    QMutexLocker l(&_imp->inputsMutex);
    
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (!_imp->inputs[i] && !_imp->liveInstance->isInputOptional(i)) {
            return true;
        }
    }
    return false;
}

bool
Node::hasOutputConnected() const
{
    ////Only called by the main-thread
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->hasOutputConnected();
    }
    if ( QThread::currentThread() == qApp->thread() ) {
        if (_imp->outputs.size() == 1) {

            return !(_imp->outputs.front()->isTrackerNode() && _imp->outputs.front()->isMultiInstance());

        } else if (_imp->outputs.size() > 1) {

            return true;
        }

    } else {
        QMutexLocker l(&_imp->outputsMutex);
        if (_imp->outputs.size() == 1) {

            return !(_imp->outputs.front()->isTrackerNode() && _imp->outputs.front()->isMultiInstance());

        } else if (_imp->outputs.size() > 1) {

            return true;
        }
    }

    return false;
}

bool
Node::checkIfConnectingInputIsOk(Natron::Node* input) const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if (input == this) {
        return false;
    }
    bool found;
    input->isNodeUpstream(this, &found);
    
    return !found;
}

void
Node::isNodeUpstream(const Natron::Node* input,
                     bool* ok) const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    if (!input) {
        *ok = false;
        
        return;
    }
    
    ///No need to lock guiInputs is only written to by the main-thread
    
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

Node::CanConnectInputReturnValue
Node::canConnectInput(const boost::shared_ptr<Node>& input,int inputNumber) const
{
   
    
    
    ///No-one is allowed to connect to the other node
    if (!input->canOthersConnectToThisNode()) {
        return eCanConnectInput_givenNodeNotConnectable;
    }
    
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(input->getLiveInstance());
    if (isGrp && !isGrp->getOutputNode()) {
        return eCanConnectInput_groupHasNoOutput;
    }
    
    if (getParentMultiInstance() || input->getParentMultiInstance()) {
        return eCanConnectInput_inputAlreadyConnected;
    }
    
    ///Applying this connection would create cycles in the graph
    if (!checkIfConnectingInputIsOk(input.get())) {
        return eCanConnectInput_graphCycles;
    }
    
    if (_imp->liveInstance->isInputRotoBrush(inputNumber)) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";
        return eCanConnectInput_indexOutOfRange;
    }
    
    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber >= (int)_imp->inputs.size() )) {
            return eCanConnectInput_indexOutOfRange;
        }
        if (_imp->inputs[inputNumber]) {
            return eCanConnectInput_inputAlreadyConnected;
        }
        
        ///Check for invalid pixel aspect ratio if the node doesn't support multiple clip PARs
        if (!_imp->liveInstance->supportsMultipleClipsPAR()) {
            
            double inputPAR = input->getLiveInstance()->getPreferredAspectRatio();
            
            double inputFPS = input->getLiveInstance()->getPreferredFrameRate();
            
            for (InputsV::const_iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
                if (*it) {
                    if ((*it)->getLiveInstance()->getPreferredAspectRatio() != inputPAR) {
                        return eCanConnectInput_differentPars;
                    }
                    
                    if (std::abs((*it)->getLiveInstance()->getPreferredFrameRate() - inputFPS) > 0.01) {
                        return eCanConnectInput_differentFPS;
                    }
                    
                }
            }
        }
    }
    
    return eCanConnectInput_ok;
}

bool
Node::connectInput(const boost::shared_ptr<Node> & input,
                   int inputNumber)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    assert(input);
    
    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }
    if (_imp->liveInstance->isInputRotoBrush(inputNumber)) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";
        return false;
    }
    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber >= (int)_imp->inputs.size() ) || (_imp->inputs[inputNumber]) ) {
            return false;
        }
        
        ///If the node is currently rendering, queue the action instead of executing it
        {
            QMutexLocker k(&_imp->nodeIsRenderingMutex);
            if (_imp->nodeIsRendering > 0 && !appPTR->isBackground()) {
                ConnectInputAction action(input,eInputActionConnect,inputNumber);
                QMutexLocker cql(&_imp->connectionQueueMutex);
                _imp->connectionQueue.push_back(action);
                return true;
            }
        }
        
        ///Set the input
        _imp->inputs[inputNumber] = input;
        input->connectOutput(this);
    }
    
    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL( labelChanged(QString) ), this, SLOT( onInputLabelChanged(QString) ) );
    
    ///Notify the GUI
    Q_EMIT inputChanged(inputNumber);
    
    ///Call the instance changed action with a reason clip changed
    onInputChanged(inputNumber);
    
    ///Recompute the hash
    computeHash();
    
    _imp->ifGroupForceHashChangeOfInputs();
    
    std::string inputChangedCB = getInputChangedCallback();
    if (!inputChangedCB.empty()) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    
    return true;
}

void
Node::Implementation::ifGroupForceHashChangeOfInputs()
{
    ///If the node is a group, force a change of the outputs of the GroupInput nodes so the hash of the tree changes downstream
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(liveInstance.get());
    if (isGrp) {
        std::list<Natron::Node* > inputsOutputs;
        isGrp->getInputsOutputs(&inputsOutputs);
        for (std::list<Natron::Node* >::iterator it = inputsOutputs.begin(); it != inputsOutputs.end(); ++it) {
            (*it)->incrementKnobsAge();
            (*it)->computeHash();
        }
    }
}

bool
Node::replaceInput(const boost::shared_ptr<Node>& input,int inputNumber)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    assert(input);
    
    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }
    if (_imp->liveInstance->isInputRotoBrush(inputNumber)) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";
        return false;
    }
    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber > (int)_imp->inputs.size() ) ) {
            return false;
        }
        
        ///If the node is currently rendering, queue the action instead of executing it
        {
            QMutexLocker k(&_imp->nodeIsRenderingMutex);
            if (_imp->nodeIsRendering > 0 && !appPTR->isBackground()) {
                ConnectInputAction action(input,eInputActionReplace,inputNumber);
                QMutexLocker cql(&_imp->connectionQueueMutex);
                _imp->connectionQueue.push_back(action);
                return true;
            }
        }
        
        ///Set the input
        if (_imp->inputs[inputNumber]) {
            QObject::connect( _imp->inputs[inputNumber].get(), SIGNAL( labelChanged(QString) ), this, SLOT( onInputLabelChanged(QString) ) );
            _imp->inputs[inputNumber]->disconnectOutput(this);
        }
        _imp->inputs[inputNumber] = input;
        input->connectOutput(this);
    }
    
    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL( labelChanged(QString) ), this, SLOT( onInputLabelChanged(QString) ) );
    
    ///Notify the GUI
    Q_EMIT inputChanged(inputNumber);
    
    ///Call the instance changed action with a reason clip changed
    onInputChanged(inputNumber);
    
    ///Recompute the hash
    computeHash();
    
    _imp->ifGroupForceHashChangeOfInputs();
    
    std::string inputChangedCB = getInputChangedCallback();
    if (!inputChangedCB.empty()) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    
    return true;
}

void
Node::switchInput0And1()
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    int maxInputs = getMaxInputCount();
    if (maxInputs < 2) {
        return;
    }
    ///get the first input number to switch
    int inputAIndex = -1;
    for (int i = 0; i < maxInputs; ++i) {
        if ( !_imp->liveInstance->isInputMask(i) ) {
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
    for (int j = 0; j < maxInputs; ++j) {
        if (j == inputAIndex) {
            continue;
        }
        if ( !_imp->liveInstance->isInputMask(j) ) {
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
        } 
    }
    
    ///If the node is currently rendering, queue the action instead of executing it
    {
        QMutexLocker k(&_imp->nodeIsRenderingMutex);
        if (_imp->nodeIsRendering > 0 && !appPTR->isBackground()) {
            QMutexLocker cql(&_imp->connectionQueueMutex);
            ///Replace input A
            {
                ConnectInputAction action(_imp->inputs[inputBIndex],eInputActionReplace,inputAIndex);
                _imp->connectionQueue.push_back(action);
            }
            
            ///Replace input B
            {
                ConnectInputAction action(_imp->inputs[inputAIndex],eInputActionReplace,inputBIndex);
                _imp->connectionQueue.push_back(action);
            }
            
            return;
        }
    }
    {
        QMutexLocker l(&_imp->inputsMutex);
        assert( inputAIndex < (int)_imp->inputs.size() && inputBIndex < (int)_imp->inputs.size() );
        boost::shared_ptr<Natron::Node> input0 = _imp->inputs[inputAIndex];
        _imp->inputs[inputAIndex] = _imp->inputs[inputBIndex];
        _imp->inputs[inputBIndex] = input0;
    }
    Q_EMIT inputChanged(inputAIndex);
    Q_EMIT inputChanged(inputBIndex);
    onInputChanged(inputAIndex);
    onInputChanged(inputBIndex);
    computeHash();
    
    std::string inputChangedCB = getInputChangedCallback();
    if (!inputChangedCB.empty()) {
        _imp->runInputChangedCallback(inputAIndex, inputChangedCB);
        _imp->runInputChangedCallback(inputBIndex, inputChangedCB);
    }

    
    _imp->ifGroupForceHashChangeOfInputs();

} // switchInput0And1

void
Node::onInputLabelChanged(const QString & name)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    Natron::Node* inp = dynamic_cast<Natron::Node*>( sender() );
    assert(inp);
    if (!inp) {
        // coverity[dead_error_line]
        return;
    }
    int inputNb = -1;
    ///No need to lock, guiInputs is only written to by the mainthread
    
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].get() == inp) {
            inputNb = i;
            break;
        }
    }
    
    if (inputNb != -1) {
        Q_EMIT inputLabelChanged(inputNb, name);
    }
}

void
Node::connectOutput(Node* output)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(output);
    
    {
        QMutexLocker l(&_imp->outputsMutex);
        _imp->outputs.push_back(output);
    }
    Q_EMIT outputsChanged();
}

int
Node::disconnectInput(int inputNumber)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber > (int)_imp->inputs.size() ) || (_imp->inputs[inputNumber] == NULL) ) {
            return -1;
        }
        
        ///If the node is currently rendering, queue the action instead of executing it
        {
            QMutexLocker k(&_imp->nodeIsRenderingMutex);
            if (_imp->nodeIsRendering > 0 && !appPTR->isBackground()) {
                ConnectInputAction action(_imp->inputs[inputNumber],eInputActionDisconnect,inputNumber);
                QMutexLocker cql(&_imp->connectionQueueMutex);
                _imp->connectionQueue.push_back(action);
                return inputNumber;
            }
        }
        
        QObject::disconnect( _imp->inputs[inputNumber].get(), SIGNAL( labelChanged(QString) ), this, SLOT( onInputLabelChanged(QString) ) );
        _imp->inputs[inputNumber]->disconnectOutput(this);
        _imp->inputs[inputNumber].reset();
        
    }
    Q_EMIT inputChanged(inputNumber);
    onInputChanged(inputNumber);
    computeHash();
    
    _imp->ifGroupForceHashChangeOfInputs();
    
    std::string inputChangedCB = getInputChangedCallback();
    if (!inputChangedCB.empty()) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }
    return inputNumber;
}

int
Node::disconnectInput(Node* input)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            if (_imp->inputs[i].get() == input) {
                
                ///If the node is currently rendering, queue the action instead of executing it
                {
                    QMutexLocker k(&_imp->nodeIsRenderingMutex);
                    if (_imp->nodeIsRendering > 0 && !appPTR->isBackground()) {
                        ConnectInputAction action(_imp->inputs[i],eInputActionDisconnect,i);
                        QMutexLocker cql(&_imp->connectionQueueMutex);
                        _imp->connectionQueue.push_back(action);
                        return i;
                    }
                }
                
                _imp->inputs[i].reset();
                l.unlock();
                input->disconnectOutput(this);
                Q_EMIT inputChanged(i);
                onInputChanged(i);
                computeHash();
                
                _imp->ifGroupForceHashChangeOfInputs();
                
                std::string inputChangedCB = getInputChangedCallback();
                if (!inputChangedCB.empty()) {
                    _imp->runInputChangedCallback(i, inputChangedCB);
                }
                
                l.relock();
                
                return i;
            }
        }
    }
    
    return -1;
}

int
Node::disconnectOutput(Node* output)
{
    assert(output);
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    int ret = -1;
    {
        QMutexLocker l(&_imp->outputsMutex);
        std::list<Node*>::iterator it = std::find(_imp->outputs.begin(),_imp->outputs.end(),output);
        
        if ( it != _imp->outputs.end() ) {
            ret = std::distance(_imp->outputs.begin(), it);
            _imp->outputs.erase(it);
        }
    }
    Q_EMIT outputsChanged();
    
    return ret;
}


int
Node::inputIndex(Node* n) const
{
    if (!n) {
        return -1;
    }
    
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->inputIndex(n);
    }
    
    ///No need to lock this is only called by the main-thread
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].get() == n) {
            return i;
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
Node::deactivate(const std::list< Node* > & outputsToDisconnect,
                 bool disconnectAll,
                 bool reconnect,
                 bool hideGui,
                 bool triggerRender)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    if (!_imp->liveInstance || !isActivated()) {
        return;
    }
    //first tell the gui to clear any persistent message linked to this node
    clearPersistentMessage(false);
    
    boost::shared_ptr<NodeCollection> parentCol = getGroup();
    assert(parentCol);
    NodeGroup* isParentGroup = dynamic_cast<NodeGroup*>(parentCol.get());
    
    ///For all knobs that have listeners, kill expressions
    const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        std::list<KnobI*> listeners;
        knobs[i]->getListeners(listeners);
        for (std::list<KnobI*>::iterator it = listeners.begin(); it != listeners.end(); ++it) {
            KnobHolder* holder = (*it)->getHolder();
            if (!holder) {
                continue;
            }
            if (holder == _imp->liveInstance.get() || holder == isParentGroup) {
                continue;
            }
            
            Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(holder);
            if (!isEffect) {
                continue;
            }
            
            boost::shared_ptr<NodeCollection> effectParent = isEffect->getNode()->getGroup();
            assert(effectParent);
            NodeGroup* isEffectParentGroup = dynamic_cast<NodeGroup*>(effectParent.get());
            if (isEffectParentGroup && isEffectParentGroup == getLiveInstance()) {
                continue;
            }
            
            for (int dim = 0; dim < (*it)->getDimension(); ++dim) {
                std::pair<int, boost::shared_ptr<KnobI> > master = (*it)->getMaster(dim);
                if (master.second == knobs[i]) {
                    (*it)->unSlave(dim, true);
                }
                
                std::string hasExpr = (*it)->getExpression(dim);
                if (!hasExpr.empty()) {
                    (*it)->clearExpression(dim,true);
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
        
        ///No need to lock guiInputs is only written to by the mainthread
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            
            if (_imp->inputs[i]) {
                if ( !_imp->liveInstance->isInputOptional(i) ) {
                    if (firstNonOptionalInput == -1) {
                        firstNonOptionalInput = i;
                        hasOnlyOneInputConnected = true;
                    } else {
                        hasOnlyOneInputConnected = false;
                    }
                } else if (!firstOptionalInput) {
                    firstOptionalInput = _imp->inputs[i];
                    if (hasOnlyOneInputConnected) {
                        hasOnlyOneInputConnected = false;
                    } else {
                        hasOnlyOneInputConnected = true;
                    }
                }
            }
        }
        
        if (hasOnlyOneInputConnected) {
            if (firstNonOptionalInput != -1) {
                inputToConnectTo = getRealInput(firstNonOptionalInput);
            } else if (firstOptionalInput) {
                inputToConnectTo = firstOptionalInput;
            }
        }
    }
    /*Removing this node from the output of all inputs*/
    _imp->deactivatedState.clear();
    
    
    std::vector<boost::shared_ptr<Node> > inputsQueueCopy;

    ///For multi-instances, if we deactivate the main instance without hiding the GUI (the default state of the tracker node)
    ///then don't remove it from outputs of the inputs
    if (hideGui || !_imp->isMultiInstance) {
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            if (_imp->inputs[i]) {
                _imp->inputs[i]->disconnectOutput(this);
            }
        }
    }
    
    
    ///For each output node we remember that the output node  had its input number inputNb connected
    ///to this node
    std::list<Node*> outputsQueueCopy;
    {
        QMutexLocker l(&_imp->outputsMutex);
        outputsQueueCopy = _imp->outputs;
    }
    
    
    for (std::list<Node*>::iterator it = outputsQueueCopy.begin(); it != outputsQueueCopy.end(); ++it) {
        assert(*it);
        bool dc = false;
        if (disconnectAll) {
            dc = true;
        } else {
            
            for (std::list<Node* >::const_iterator found = outputsToDisconnect.begin(); found != outputsToDisconnect.end(); ++found) {
                if (*found == *it) {
                    dc = true;
                    break;
                }
            }
        }
        if (dc) {
            int inputNb = (*it)->disconnectInput(this);
            _imp->deactivatedState.insert( make_pair(*it, inputNb) );
            
            ///reconnect if inputToConnectTo is not null
            if (inputToConnectTo) {
                getApp()->getProject()->connectNodes(inputNb, inputToConnectTo, *it);
            }
        }
    }
    
    
    ///kill any thread it could have started
    ///Commented-out: If we were to undo the deactivate we don't want all threads to be
    ///exited, just exit them when the effect is really deleted instead
    //quitAnyProcessing();
    abortAnyProcessing();
    
    ///Free all memory used by the plug-in.
    
    ///COMMENTED-OUT: Don't do this, the node may still be rendering here.
    ///_imp->liveInstance->clearPluginMemoryChunks();
    clearLastRenderedImage();
    
    if (parentCol) {
        parentCol->notifyNodeDeactivated(shared_from_this());
    }
    
    if (hideGui) {
        Q_EMIT deactivated(triggerRender);
    }
    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = false;
    }
    
    
    ///If the node is a group, deactivate all nodes within the group
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(getLiveInstance());
    if (isGrp) {
        isGrp->setIsDeactivatingGroup(true);
        NodeList nodes = isGrp->getNodes();
        for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->deactivate(std::list< Node* >(),false,false,true,false);
        }
        isGrp->setIsDeactivatingGroup(false);
    }
    
    ///If the node has children (i.e it is a multi-instance), deactivate its children
    for (std::list<boost::weak_ptr<Node> >::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        it->lock()->deactivate(std::list< Node* >(),false,false,true,false);
    }
    

    
    if (!getApp()->getProject()->isProjectClosing()) {
        _imp->runOnNodeDeleteCB();
    }
    
} // deactivate

void
Node::activate(const std::list< Node* > & outputsToRestore,
               bool restoreAll,
               bool triggerRender)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->liveInstance || isActivated()) {
        return;
    }

    
    ///No need to lock, guiInputs is only written to by the main-thread
    
    ///for all inputs, reconnect their output to this node
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i]) {
            _imp->inputs[i]->connectOutput(this);
        }
    }
    
    boost::shared_ptr<Node> thisShared = shared_from_this();
    
    
    ///Restore all outputs that was connected to this node
    for (std::map<Node*,int >::iterator it = _imp->deactivatedState.begin();
         it != _imp->deactivatedState.end(); ++it) {
        bool restore = false;
        if (restoreAll) {
            restore = true;
        } else {
            for (std::list<Node* >::const_iterator found = outputsToRestore.begin(); found != outputsToRestore.end(); ++found) {
                if (*found == it->first) {
                    restore = true;
                    break;
                }
            }
        }
        
        if (restore) {
            ///before connecting the outputs to this node, disconnect any link that has been made
            ///between the outputs by the user. This should normally never happen as the undo/redo
            ///stack follow always the same order.
            boost::shared_ptr<Node> outputHasInput = it->first->getInput(it->second);
            if (outputHasInput) {
                bool ok = getApp()->getProject()->disconnectNodes(outputHasInput.get(), it->first);
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
    
    boost::shared_ptr<NodeCollection> group = getGroup();
    if (group) {
        group->notifyNodeActivated(shared_from_this());
    }
    Q_EMIT activated(triggerRender);
    
    ///If the node is a group, activate all nodes within the group first
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(getLiveInstance());
    if (isGrp) {
        isGrp->setIsActivatingGroup(true);
        NodeList nodes = isGrp->getNodes();
        for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->activate(std::list< Node* >(),false,false);
        }
        isGrp->setIsActivatingGroup(false);
    }
    
    ///If the node has children (i.e it is a multi-instance), activate its children
    for (std::list<boost::weak_ptr<Node> >::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        it->lock()->activate(std::list< Node* >(),false,false);
    }
    
    _imp->runOnNodeCreatedCB(true);
} // activate

void
Node::destroyNode(bool autoReconnect)
{
    deactivate(std::list< Node* >(),
               true,
               autoReconnect,
               true,
               true);
    
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(getLiveInstance());
    if (isGrp) {
        isGrp->clearNodes(true);
    }
    
    removeReferences(true);
}

boost::shared_ptr<KnobI>
Node::getKnobByName(const std::string & name) const
{
    ///MT-safe, never changes
    assert(_imp->knobsInitialized);
    
    return _imp->liveInstance->getKnobByName(name);
}

namespace {
    ///output is always RGBA with alpha = 255
    template<typename PIX,int maxValue>
    void
    renderPreview(const Natron::Image & srcImg,
                  int elemCount,
                  int *dstWidth,
                  int *dstHeight,
                  bool convertToSrgb,
                  unsigned int* dstPixels)
    {
        ///recompute it after the rescaling
        const RectI & srcBounds = srcImg.getBounds();
        double yZoomFactor = *dstHeight / (double)srcBounds.height();
        double xZoomFactor = *dstWidth / (double)srcBounds.width();
        double zoomFactor;
        
        if (xZoomFactor < yZoomFactor) {
            zoomFactor = xZoomFactor;
            *dstHeight = srcBounds.height() * zoomFactor;
        } else {
            zoomFactor = yZoomFactor;
            *dstWidth = srcBounds.width() * zoomFactor;
        }
        
        Natron::Image::ReadAccess acc = srcImg.getReadRights();
        
        assert(elemCount >= 3);
        
        for (int i = 0; i < *dstHeight; ++i) {
            double y = (i - *dstHeight / 2.) / zoomFactor + (srcBounds.y1 + srcBounds.y2) / 2.;
            int yi = std::floor(y + 0.5);
            U32 *dst_pixels = dstPixels + *dstWidth * (*dstHeight - 1 - i);
            const PIX* src_pixels = (const PIX*)acc.pixelAt(srcBounds.x1, yi);
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
                    double x = (j - *dstWidth / 2.) / zoomFactor + (srcBounds.x1 + srcBounds.x2) / 2.;
                    int xi = std::floor(x + 0.5) - srcBounds.x1; // round to nearest
                    if ( (xi < 0) || ( xi >= (srcBounds.x2 - srcBounds.x1) ) ) {
#ifndef __NATRON_WIN32__
                        dst_pixels[j] = toBGRA(0, 0, 0, 0);
#else
                        dst_pixels[j] = toBGRA(0, 0, 0, 255);
#endif
                    } else {
                        float rFilt = src_pixels[xi * elemCount + 0] / (float)maxValue;
                        float gFilt = src_pixels[xi * elemCount + 1] / (float)maxValue;
                        float bFilt = src_pixels[xi * elemCount + 2] / (float)maxValue;
                        int r = Color::floatToInt<256>(convertToSrgb ? Natron::Color::to_func_srgb(rFilt) : rFilt);
                        int g = Color::floatToInt<256>(convertToSrgb ? Natron::Color::to_func_srgb(gFilt) : gFilt);
                        int b = Color::floatToInt<256>(convertToSrgb ? Natron::Color::to_func_srgb(bFilt) : bFilt);
                        dst_pixels[j] = toBGRA(r, g, b, 255);
                    }
                }
            }
        }
    } // renderPreview
}

class ComputingPreviewSetter_RAII
{
    Node::Implementation* _imp;
    
public:
    ComputingPreviewSetter_RAII(Node::Implementation* imp)
    : _imp(imp)
    {
        _imp->setComputingPreview(true);
    }
    
    ~ComputingPreviewSetter_RAII()
    {
        _imp->setComputingPreview(false);
        
        if (_imp->checkForExitPreview()) {
            return;
        }
    }
};

bool
Node::makePreviewImage(SequenceTime time,
                       int *width,
                       int *height,
                       unsigned int* buf)
{
    assert(_imp->knobsInitialized);
    if (!_imp->liveInstance) {
        return false;
    }
    
    if (_imp->checkForExitPreview()) {
        return false;
    }
    
    /// prevent 2 previews to occur at the same time since there's only 1 preview instance
    ComputingPreviewSetter_RAII computingPreviewRAII(_imp.get());
    
    RectD rod;
    bool isProjectFormat;
    RenderScale scale;
    scale.x = scale.y = 1.;
    U64 nodeHash = getHashValue();
    Natron::StatusEnum stat = _imp->liveInstance->getRegionOfDefinition_public(nodeHash,time, scale, 0, &rod, &isProjectFormat);
    if ( (stat == eStatusFailed) || rod.isNull() ) {
        return false;
    }
    assert( !rod.isNull() );
    double yZoomFactor = (double)*height / (double)rod.height();
    double xZoomFactor = (double)*width / (double)rod.width();
    double closestPowerOf2X = xZoomFactor >= 1 ? 1 : std::pow( 2,-std::ceil( std::log(xZoomFactor) / std::log(2.) ) );
    double closestPowerOf2Y = yZoomFactor >= 1 ? 1 : std::pow( 2,-std::ceil( std::log(yZoomFactor) / std::log(2.) ) );
    int closestPowerOf2 = std::max(closestPowerOf2X,closestPowerOf2Y);
    unsigned int mipMapLevel = std::min(std::log( (double)closestPowerOf2 ) / std::log(2.),5.);
    
    scale.x = Natron::Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;
    
    const double par = _imp->liveInstance->getPreferredAspectRatio();
    
    RectI renderWindow;
    rod.toPixelEnclosing(mipMapLevel, par, &renderWindow);
    
    ParallelRenderArgsSetter frameRenderArgs(getApp()->getProject().get(),
                                             time,
                                             0, //< preview only renders view 0 (left)
                                             true, //<isRenderUserInteraction
                                             false, //isSequential
                                             false, //can abort
                                             0, //render Age
                                             0, // viewer requester
                                             0, //texture index
                                             getApp()->getTimeLine().get());
    
    std::list<ImageComponents> requestedComps;
    requestedComps.push_back(ImageComponents::getRGBComponents());
    RenderingFlagSetter flagIsRendering(this);
    
    // Exceptions are caught because the program can run without a preview,
    // but any exception in renderROI is probably fatal.
    ImageList planes;
    try {
        Natron::EffectInstance::RenderRoIRetCode retCode =
        _imp->liveInstance->renderRoI( EffectInstance::RenderRoIArgs( time,
                                                                     scale,
                                                                     mipMapLevel,
                                                                     0, //< preview only renders view 0 (left)
                                                                     false,
                                                                     renderWindow,
                                                                     rod,
                                                                     requestedComps, //< preview is always rgb...
                                                                     getBitDepth() ) ,&planes);
        if (retCode != Natron::EffectInstance::eRenderRoIRetCodeOk) {
            return false;
        }
    } catch (...) {
        return false;
    }
    
    if (planes.empty()) {
        return false;
    }
    
    const ImagePtr& img = planes.front();
    
    const ImageComponents& components = img->getComponents();
    int elemCount = components.getNumComponents();
    
    ///we convert only when input is Linear.
    //Rec709 and srGB is acceptable for preview
    bool convertToSrgb = getApp()->getDefaultColorSpaceForBitDepth( img->getBitDepth() ) == Natron::eViewerColorSpaceLinear;
    
    switch ( img->getBitDepth() ) {
        case Natron::eImageBitDepthByte: {
            renderPreview<unsigned char, 255>(*img, elemCount, width, height,convertToSrgb, buf);
            break;
        }
        case Natron::eImageBitDepthShort: {
            renderPreview<unsigned short, 65535>(*img, elemCount, width, height,convertToSrgb, buf);
            break;
        }
        case Natron::eImageBitDepthFloat: {
            renderPreview<float, 1>(*img, elemCount, width, height,convertToSrgb, buf);
            break;
        }
        case Natron::eImageBitDepthNone:
            break;
    }
    return true;
    
} // makePreviewImage

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
    return getPluginID() == PLUGINID_OFX_ROTO;
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

const std::vector<boost::shared_ptr<KnobI> > &
Node::getKnobs() const
{
    ///MT-safe from EffectInstance::getKnobs()
    return _imp->liveInstance->getKnobs();
}

void
Node::setKnobsFrozen(bool frozen)
{
    ///MT-safe from EffectInstance::setKnobsFrozen
    _imp->liveInstance->setKnobsFrozen(frozen);
    
    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i]) {
            _imp->inputs[i]->setKnobsFrozen(frozen);
        }
    }
}

std::string
Node::getPluginIconFilePath() const
{
    return _imp->plugin ? _imp->plugin->getIconFilePath().toStdString() : std::string();
}

std::string
Node::getPluginID() const
{
    ///MT-safe, never changes
    if (!_imp->plugin) {
        return std::string();
    }
    return _imp->plugin->getPluginID().toStdString();
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
    if (!_imp->previewEnabledKnob) {
        return;
    }
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
Node::notifyRenderBeingAborted()
{
//
//    if (QThread::currentThread() == qApp->thread()) {
        ///The render thread is waiting for the main-thread to dequeue actions
        ///but the main-thread is waiting for the render thread to abort
        ///cancel the dequeuing
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        _imp->nodeIsDequeuing = false;
        _imp->nodeIsDequeuingCond.wakeAll();
//    }
    
}

bool
Node::message(MessageTypeEnum type,
              const std::string & content) const
{
    ///If the node was aborted, don't transmit any message because we could cause a deadlock
    if ( _imp->liveInstance->aborted() ) {
        return false;
    }
    
    switch (type) {
        case eMessageTypeInfo:
            informationDialog(getLabel_mt_safe(), content);
            
            return true;
        case eMessageTypeWarning:
            warningDialog(getLabel_mt_safe(), content);
            
            return true;
        case eMessageTypeError:
            errorDialog(getLabel_mt_safe(), content);
            
            return true;
        case eMessageTypeQuestion:
            
            return questionDialog(getLabel_mt_safe(), content, false) == eStandardButtonYes;
        default:
            
            return false;
    }
}

void
Node::setPersistentMessage(MessageTypeEnum type,
                           const std::string & content)
{
    if ( !appPTR->isBackground() ) {
        //if the message is just an information, display a popup instead.
        if (type == eMessageTypeInfo) {
            message(type,content);
            
            return;
        }
        
        {
            QMutexLocker k(&_imp->persistentMessageMutex);
            
            QString message;
            message.append( getLabel_mt_safe().c_str() );
            if (type == eMessageTypeError) {
                message.append(" error: ");
                _imp->persistentMessageType = 1;
            } else if (type == eMessageTypeWarning) {
                message.append(" warning: ");
                _imp->persistentMessageType = 2;
            }
            message.append( content.c_str() );
            if (message == _imp->persistentMessage) {
                return;
            }
            _imp->persistentMessage = message;
        }
        Q_EMIT persistentMessageChanged();
    } else {
        std::cout << "Persistent message: " << content << std::endl;
    }
}

bool
Node::hasPersistentMessage() const
{
    QMutexLocker k(&_imp->persistentMessageMutex);
    return !_imp->persistentMessage.isEmpty();
}

void
Node::getPersistentMessage(QString* message,int* type) const
{
    QMutexLocker k(&_imp->persistentMessageMutex);
    *type = _imp->persistentMessageType;
    *message = _imp->persistentMessage;
}

void
Node::clearPersistentMessage(bool recurse)
{
    if ( !appPTR->isBackground() ) {
        {
            QMutexLocker k(&_imp->persistentMessageMutex);
            if (!_imp->persistentMessage.isEmpty()) {
                _imp->persistentMessage.clear();
                k.unlock();
                Q_EMIT persistentMessageChanged();
            }
        }
    }
    
    if (recurse) {
        QMutexLocker l(&_imp->inputsMutex);
        ///No need to lock, guiInputs is only written to by the main-thread
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            if (_imp->inputs[i]) {
                _imp->inputs[i]->clearPersistentMessage(true);
            }
        }
    }
    
}

void
Node::purgeAllInstancesCaches()
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->liveInstance);
    _imp->liveInstance->purgeCaches();
}

bool
Node::notifyInputNIsRendering(int inputNb)
{
    if (getApp()->isGuiFrozen()) {
        return false;
    }
    
    timeval now;
    
    
    gettimeofday(&now, 0);
    
    QMutexLocker l(&_imp->timersMutex);
    
    
    double t =  now.tv_sec  - _imp->lastInputNRenderStartedSlotCallTime.tv_sec +
    (now.tv_usec - _imp->lastInputNRenderStartedSlotCallTime.tv_usec) * 1e-6f;
    
    
    if (t > NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS) {
        
        _imp->lastInputNRenderStartedSlotCallTime = now;
        
        l.unlock();
        
        Q_EMIT inputNIsRendering(inputNb);
        return true;
    }
    return false;
}

void
Node::notifyInputNIsFinishedRendering(int inputNb)
{
    Q_EMIT inputNIsFinishedRendering(inputNb);
}

bool
Node::notifyRenderingStarted()
{
    if (getApp()->isGuiFrozen()) {
        return false;
    }
    
    timeval now;
    
    gettimeofday(&now, 0);
    
    QMutexLocker l(&_imp->timersMutex);
    
    double t =  now.tv_sec  - _imp->lastRenderStartedSlotCallTime.tv_sec +
    (now.tv_usec - _imp->lastRenderStartedSlotCallTime.tv_usec) * 1e-6f;
    
    if (t > NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS) {
        
        _imp->lastRenderStartedSlotCallTime = now;
        
        l.unlock();
        
        Q_EMIT renderingStarted();
        return true;
    }
    return false;
}

void
Node::notifyRenderingEnded()
{
    Q_EMIT renderingEnded();
}

void
Node::setOutputFilesForWriter(const std::string & pattern)
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
    Q_EMIT pluginMemoryUsageChanged(nBytes);
}

void
Node::unregisterPluginMemory(size_t nBytes)
{
    {
        QMutexLocker l(&_imp->memoryUsedMutex);
        _imp->pluginInstanceMemoryUsed -= nBytes;
    }
    Q_EMIT pluginMemoryUsageChanged(-nBytes);
}

QMutex &
Node::getRenderInstancesSharedMutex()
{
    return _imp->renderInstancesSharedMutex;
}

static void refreshPreviewsRecursivelyUpstreamInternal(int time,Node* node,std::list<Node*>& marked)
{
    if (std::find(marked.begin(), marked.end(), node) != marked.end()) {
        return;
    }

    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }
    
    marked.push_back(node);
    
    std::vector<boost::shared_ptr<Node> > inputs = node->getInputs_copy();
    
    for (U32 i = 0; i < inputs.size(); ++i) {
        if (inputs[i]) {
            inputs[i]->refreshPreviewsRecursivelyUpstream(time);
        }
    }

}

void
Node::refreshPreviewsRecursivelyUpstream(int time)
{
    std::list<Node*> marked;
    refreshPreviewsRecursivelyUpstreamInternal(time,this,marked);
}

static void refreshPreviewsRecursivelyDownstreamInternal(int time,Node* node,std::list<Node*>& marked)
{
    if (std::find(marked.begin(), marked.end(), node) != marked.end()) {
        return;
    }
    
    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }
    
    marked.push_back(node);
    
    std::list<Node*> outputs;
    node->getOutputs_mt_safe(outputs);
    for (std::list<Node*>::iterator it = outputs.begin(); it != outputs.end(); ++it) {
        assert(*it);
        (*it)->refreshPreviewsRecursivelyDownstream(time);
    }

}

void
Node::refreshPreviewsRecursivelyDownstream(int time)
{
    std::list<Node*> marked;
    refreshPreviewsRecursivelyDownstreamInternal(time,this,marked);
}

void
Node::onAllKnobsSlaved(bool isSlave,
                       KnobHolder* master)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    if (isSlave) {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(master);
        assert(effect);
        boost::shared_ptr<Natron::Node> masterNode = effect->getNode();
        {
            QMutexLocker l(&_imp->masterNodeMutex);
            _imp->masterNode = masterNode;
        }
        QObject::connect( masterNode.get(), SIGNAL( deactivated(bool) ), this, SLOT( onMasterNodeDeactivated() ) );
        QObject::connect( masterNode.get(), SIGNAL( knobsAgeChanged(U64) ), this, SLOT( setKnobsAge(U64) ) );
        QObject::connect( masterNode.get(), SIGNAL( previewImageChanged(int) ), this, SLOT( refreshPreviewImage(int) ) );
    } else {
        NodePtr master = getMasterNode();
        QObject::disconnect( master.get(), SIGNAL( deactivated(bool) ), this, SLOT( onMasterNodeDeactivated() ) );
        QObject::disconnect( master.get(), SIGNAL( knobsAgeChanged(U64) ), this, SLOT( setKnobsAge(U64) ) );
        QObject::disconnect( master.get(), SIGNAL( previewImageChanged(int) ), this, SLOT( refreshPreviewImage(int) ) );
        {
            QMutexLocker l(&_imp->masterNodeMutex);
            _imp->masterNode.reset();
        }
    }
    
    Q_EMIT allKnobsSlaved(isSlave);
}

void
Node::onKnobSlaved(KnobI* slave,KnobI* master,
                   int dimension,
                   bool isSlave)
{
    ///ignore the call if the node is a clone
    {
        QMutexLocker l(&_imp->masterNodeMutex);
        if (_imp->masterNode.lock()) {
            return;
        }
    }
    
    assert(master->getHolder());
    
    
    ///If the holder isn't an effect, ignore it too
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(master->getHolder());
    
    if (!isEffect) {
        return;
    }
    boost::shared_ptr<Natron::Node> parentNode  = isEffect->getNode();
    bool changed = false;
    {
        QMutexLocker l(&_imp->masterNodeMutex);
        KnobLinkList::iterator found = _imp->nodeLinks.end();
        for (KnobLinkList::iterator it = _imp->nodeLinks.begin(); it != _imp->nodeLinks.end(); ++it) {
            if (it->masterNode == parentNode) {
                found = it;
                break;
            }
        }
        
        if ( found == _imp->nodeLinks.end() ) {
            if (!isSlave) {
                ///We want to unslave from the given node but the link didn't existed, just return
                return;
            } else {
                ///Add a new link
                KnobLink link;
                link.masterNode = parentNode;
                link.slave = slave;
                link.master = master;
                link.dimension = dimension;
                _imp->nodeLinks.push_back(link);
                changed = true;
            }
        } else if ( found != _imp->nodeLinks.end() ) {
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
        Q_EMIT knobsLinksChanged();
    }
} // onKnobSlaved

void
Node::getKnobsLinks(std::list<Node::KnobLink> & links) const
{
    QMutexLocker l(&_imp->masterNodeMutex);
    
    links = _imp->nodeLinks;
}

void
Node::onMasterNodeDeactivated()
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->liveInstance) {
        return;
    }
    _imp->liveInstance->unslaveAllKnobs();
}

boost::shared_ptr<Natron::Node>
Node::getMasterNode() const
{
    QMutexLocker l(&_imp->masterNodeMutex);
    
    return _imp->masterNode.lock();
}

bool
Node::isSupportedComponent(int inputNb,
                           const Natron::ImageComponents& comp) const
{
    QMutexLocker l(&_imp->inputsMutex);
    
    if (inputNb >= 0) {
        assert( inputNb < (int)_imp->inputsComponents.size() );
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
Node::findClosestInList(const Natron::ImageComponents& comp,
                        const std::list<Natron::ImageComponents> &components,
                        bool multiPlanar)
{
    if ( components.empty() ) {
        return ImageComponents::getNoneComponents();
    }
    std::list<Natron::ImageComponents>::const_iterator closestComp = components.end();
    for (std::list<Natron::ImageComponents>::const_iterator it = components.begin(); it != components.end(); ++it) {
        if ( closestComp == components.end() ) {
            if (multiPlanar && it->getNumComponents() == comp.getNumComponents()) {
                return comp;
            }
            closestComp = it;
        } else {
            if (it->getNumComponents() == comp.getNumComponents()) {
                if (multiPlanar) {
                    return comp;
                }
                closestComp = it;
                break;
            } else {
                int diff = it->getNumComponents() - comp.getNumComponents();
                int diffSoFar = closestComp->getNumComponents() - comp.getNumComponents();
                if (diff > diffSoFar) {
                    closestComp = it;
                }
            }
            
        }
    }
    if (closestComp == components.end()) {
        return ImageComponents::getNoneComponents();
    }
    return *closestComp;

}

Natron::ImageComponents
Node::findClosestSupportedComponents(int inputNb,
                                     const Natron::ImageComponents& comp) const
{
    std::list<Natron::ImageComponents> comps;
    {
        QMutexLocker l(&_imp->inputsMutex);
        
        if (inputNb >= 0) {
            assert( inputNb < (int)_imp->inputsComponents.size() );
            comps = _imp->inputsComponents[inputNb];
        } else {
            assert(inputNb == -1);
            comps = _imp->outputComponents;
        }
    }
    return findClosestInList(comp, comps, _imp->liveInstance->isMultiPlanar());
}



int
Node::getMaskChannel(int inputNb,Natron::ImageComponents* comps) const
{
    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);
    if ( it != _imp->maskSelectors.end() ) {
        int index =  it->second.channel->getValue();
        if (index == 0) {
            *comps = ImageComponents::getNoneComponents();
            return -1;
        } else {
            index -= 1; // None choice
            QMutexLocker locker(&it->second.compsMutex);
            int k = 0;
            for (std::size_t i = 0; i < it->second.compsAvailable.size(); ++i) {
                if (index >= k && index < (it->second.compsAvailable[i].getNumComponents() + k)) {
                    int compIndex = index - k;
                    assert(compIndex >= 0 && compIndex <= 3);
                    *comps = it->second.compsAvailable[i];
                    return compIndex;
                }
                k += it->second.compsAvailable[i].getNumComponents();
            }
            
        }
    }
    return -1;
    
}

bool
Node::isMaskEnabled(int inputNb) const
{
    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);
    if ( it != _imp->maskSelectors.end() ) {
        return it->second.enabled->getValue();
    } else {
        return true;
    }
}

void
Node::lock(const boost::shared_ptr<Natron::Image> & image)
{
    
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<boost::shared_ptr<Natron::Image> >::iterator it =
    std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    
    while ( it != _imp->imagesBeingRendered.end() ) {
        _imp->imageBeingRenderedCond.wait(&_imp->imagesBeingRenderedMutex);
        it = std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    }
    ///Okay the image is not used by any other thread, claim that we want to use it
    assert( it == _imp->imagesBeingRendered.end() );
    _imp->imagesBeingRendered.push_back(image);
}

bool
Node::tryLock(const boost::shared_ptr<Natron::Image> & image)
{
    
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<boost::shared_ptr<Natron::Image> >::iterator it =
    std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    
    if ( it != _imp->imagesBeingRendered.end() ) {
        return false;
    }
    ///Okay the image is not used by any other thread, claim that we want to use it
    assert( it == _imp->imagesBeingRendered.end() );
    _imp->imagesBeingRendered.push_back(image);
    return true;
}

void
Node::unlock(const boost::shared_ptr<Natron::Image> & image)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<boost::shared_ptr<Natron::Image> >::iterator it =
    std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    ///The image must exist, otherwise this is a bug
    assert( it != _imp->imagesBeingRendered.end() );
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
         it != _imp->imagesBeingRendered.end(); ++it) {
        const Natron::ImageKey &key = (*it)->getKey();
        if ( (key._view == view) && ((*it)->getMipMapLevel() == mipMapLevel) && (key._time == time) ) {
            return *it;
        }
    }
    return boost::shared_ptr<Natron::Image>();
}

void
Node::onInputChanged(int inputNb)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->duringInputChangedAction = true;
    std::map<int,MaskSelector>::iterator found = _imp->maskSelectors.find(inputNb);
    if ( found != _imp->maskSelectors.end() ) {
        boost::shared_ptr<Node> inp = getInput(inputNb);
        found->second.enabled->blockValueChanges();
        found->second.enabled->setValue(inp ? true : false, 0);
        found->second.enabled->unblockValueChanges();
    }
    _imp->liveInstance->onInputChanged(inputNb);
    _imp->duringInputChangedAction = false;
}

void
Node::onParentMultiInstanceInputChanged(int input)
{
    _imp->duringInputChangedAction = true;
    _imp->liveInstance->onInputChanged(input);
    _imp->duringInputChangedAction = false;
}


bool
Node::duringInputChangedAction() const
{
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->duringInputChangedAction;
}

void
Node::computeFrameRangeForReader(const KnobI* fileKnob)
{
    int leftBound = INT_MIN;
    int rightBound = INT_MAX;
    ///Set the originalFrameRange parameter of the reader if it has one.
    boost::shared_ptr<KnobI> knob = getKnobByName("originalFrameRange");
    if (knob) {
        Int_Knob* originalFrameRange = dynamic_cast<Int_Knob*>(knob.get());
        if (originalFrameRange && originalFrameRange->getDimension() == 2) {
            
            const File_Knob* isFile = dynamic_cast<const File_Knob*>(fileKnob);
            assert(isFile);
            
            std::string pattern = isFile->getValue();
            getApp()->getProject()->canonicalizePath(pattern);
            SequenceParsing::SequenceFromPattern seq;
            SequenceParsing::filesListFromPattern(pattern, &seq);
            if (seq.empty() || seq.size() == 1) {
                leftBound = 1;
                rightBound = 1;
            } else if (seq.size() > 1) {
                leftBound = seq.begin()->first;
                rightBound = seq.rbegin()->first;
            }
            originalFrameRange->beginChanges();
            originalFrameRange->setValue(leftBound, 0);
            originalFrameRange->setValue(rightBound, 1);
            originalFrameRange->endChanges();
            
        }
    }

}

bool
Node::getOverlayColor(double* r,double* g,double* b) const
{
    boost::shared_ptr<NodeGuiI> gui_i = getNodeGui();
    if (!gui_i) {
        return false;
    }
    return gui_i->getOverlayColor(r, g, b);
}

bool
Node::shouldDrawOverlay() const
{
    boost::shared_ptr<NodeGuiI> gui_i = getNodeGui();
    if (!gui_i) {
        return false;
    }
    return gui_i->shouldDrawOverlay();
}

void
Node::drawDefaultOverlay(double scaleX,double scaleY)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        nodeGui->drawDefaultOverlay(scaleX, scaleY);
    }
}

bool
Node::onOverlayPenDownDefault(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayPenDownDefault(scaleX, scaleY, viewportPos, pos);
    }
    return false;
}

bool
Node::onOverlayPenMotionDefault(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayPenMotionDefault(scaleX, scaleY, viewportPos, pos);
    }
    return false;
}

bool
Node::onOverlayPenUpDefault(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayPenUpDefault(scaleX, scaleY, viewportPos, pos);
    }
    return false;
}

bool
Node::onOverlayKeyDownDefault(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayKeyDownDefault(scaleX, scaleY, key, modifiers);
    }
    return false;
}

bool
Node::onOverlayKeyUpDefault(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayKeyUpDefault(scaleX, scaleY, key, modifiers);
    }
    return false;
}

bool
Node::onOverlayKeyRepeatDefault(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayKeyRepeatDefault(scaleX, scaleY, key, modifiers);
    }
    return false;
}

bool
Node::onOverlayFocusGainedDefault(double scaleX,double scaleY)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayFocusGainedDefault(scaleX, scaleY);
    }
    return false;
}

bool
Node::onOverlayFocusLostDefault(double scaleX,double scaleY)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayFocusLostDefault(scaleX, scaleY);
    }
    return false;
}

void
Node::removeDefaultOverlay(KnobI* knob)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        nodeGui->removeDefaultOverlay(knob);
    }
}

void
Node::addDefaultPositionOverlay(const boost::shared_ptr<Double_Knob>& position)
{
    assert(QThread::currentThread() == qApp->thread());
    if (appPTR->isBackground()) {
        return;
    }
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativePositionOverlays.push_back(position);
    } else if (position->getDimension() == 2) {
        nodeGui->addDefaultPositionInteract(position);
    }
}

void
Node::initializeDefaultOverlays()
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (!nodeGui) {
        return;
    }
    for (std::list<boost::shared_ptr<Double_Knob> > ::iterator it = _imp->nativePositionOverlays.begin(); it!=_imp->nativePositionOverlays.end(); ++it)
    {
        nodeGui->addDefaultPositionInteract(*it);
    }
    _imp->nativePositionOverlays.clear();
}

void
Node::setPluginIconFilePath(const std::string& iconFilePath)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (!nodeGui) {
        return;
    }
    nodeGui->setPluginIconFilePath(iconFilePath);
}

void
Node::setPluginDescription(const std::string& description)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (!nodeGui) {
        return;
    }
    nodeGui->setPluginDescription(description);
}


void
Node::setPluginIDAndVersionForGui(const std::string& pluginLabel,const std::string& pluginID,unsigned int version)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (!nodeGui) {
        return;
    }
    nodeGui->setPluginIDAndVersion(pluginLabel,pluginID, version);

}

void
Node::setPluginPythonModule(const std::string& pythonModule)
{
    QMutexLocker k(&_imp->pluginPythonModuleMutex);
    _imp->pluginPythonModule = pythonModule;
}

std::string
Node::getPluginPythonModule() const
{
    QMutexLocker k(&_imp->pluginPythonModuleMutex);
    return _imp->pluginPythonModule;
}

bool
Node::hasDefaultOverlayForParam(const KnobI* knob) const
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui && nodeGui->hasDefaultOverlayForParam(knob)) {
        return true;
    }
    return false;

}

bool
Node::hasDefaultOverlay() const
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui && nodeGui->hasDefaultOverlay()) {
        return true;
    }
    return false;
}

void
Node::setCurrentViewportForDefaultOverlays(OverlaySupport* viewPort)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui && nodeGui->hasDefaultOverlay()) {
        nodeGui->setCurrentViewportForDefaultOverlays(viewPort);
    }
}

void
Node::onEffectKnobValueChanged(KnobI* what,
                               Natron::ValueChangedReasonEnum reason)
{
    if (!what) {
        return;
    }
    for (std::map<int, MaskSelector >::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        if (it->second.channel.get() == what) {
            _imp->onMaskSelectorChanged(it->first, it->second);
            break;
        }
    }
    
    if ( what == _imp->previewEnabledKnob.get() ) {
        if ( (reason == Natron::eValueChangedReasonUserEdited) || (reason == Natron::eValueChangedReasonSlaveRefresh) ) {
            Q_EMIT previewKnobToggled();
        }
    } else if ( ( what == _imp->disableNodeKnob.get() ) && !_imp->isMultiInstance && !_imp->multiInstanceParent.lock() ) {
        Q_EMIT disabledKnobToggled( _imp->disableNodeKnob->getValue() );
        getApp()->redrawAllViewers();
    } else if ( what == _imp->nodeLabelKnob.get() ) {
        Q_EMIT nodeExtraLabelChanged( _imp->nodeLabelKnob->getValue().c_str() );
    } else if (what->getName() == kNatronOfxParamStringSublabelName) {
        //special hack for the merge node and others so we can retrieve the sublabel and display it in the node's label
        String_Knob* strKnob = dynamic_cast<String_Knob*>(what);
        if (strKnob) {
            QString operation = strKnob->getValue().c_str();
            replaceCustomDataInlabel('(' + operation + ')');
        }
    } else if ( (what->getName() == kOfxImageEffectFileParamName) && _imp->liveInstance->isReader() ) {
        ///Refresh the preview automatically if the filename changed
        incrementKnobsAge(); //< since evaluate() is called after knobChanged we have to do this  by hand
        //computePreviewImage( getApp()->getTimeLine()->currentFrame() );
        
        ///union the project frame range if not locked with the reader frame range
        bool isLocked = getApp()->getProject()->isFrameRangeLocked();
        if (!isLocked) {
            int leftBound = INT_MIN,rightBound = INT_MAX;
            _imp->liveInstance->getFrameRange_public(getHashValue(), &leftBound, &rightBound);
    
            if (leftBound != INT_MIN && rightBound != INT_MAX) {
                getApp()->getProject()->unionFrameRangeWith(leftBound, rightBound);
            }
        }
        
    } else if ( what == _imp->refreshInfoButton.get() ) {
        int maxinputs = getMaxInputCount();
        for (int i = 0; i < maxinputs; ++i) {
            std::string inputInfo = makeInfoForInput(i);
            if (i < (int)_imp->inputFormats.size() && _imp->inputFormats[i]) {
                _imp->inputFormats[i]->setValue(inputInfo, 0);
            }
        }
        std::string outputInfo = makeInfoForInput(-1);
        _imp->outputFormat->setValue(outputInfo, 0);
 
    }
    
    for (std::map<int,ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
        if (it->second.layer.get() == what) {
            _imp->onLayerChanged(it->first, it->second);
        }
    }
    
    GroupInput* isInput = dynamic_cast<GroupInput*>(_imp->liveInstance.get());
    if (isInput) {
        if (what->getName() == kNatronGroupInputIsOptionalParamName
            || what->getName() == kNatronGroupInputIsMaskParamName) {
            boost::shared_ptr<NodeCollection> col = isInput->getNode()->getGroup();
            assert(col);
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(col.get());
            assert(isGrp);
            
            ///Refresh input arrows of the node to reflect the state
            isGrp->getNode()->initializeInputs();
        }
    }
}

bool
Node::Implementation::getSelectedLayer(int inputNb,const ChannelSelector& selector, ImageComponents* comp) const
{
    Node* node = 0;
    if (inputNb == -1) {
        node = _publicInterface;
    } else {
        node = _publicInterface->getInput(inputNb).get();
    }
    

    int index = selector.layer->getValue();
    std::vector<std::string> entries = selector.layer->getEntries_mt_safe();
    if (entries.empty()) {
        return false;
    }
    if (index < 0 || index >= (int)entries.size()) {
        return false;
    }
    const std::string& layer = entries[index];
    if (layer == "All") {
        return false;
    } else {
        
        EffectInstance::ComponentsAvailableMap compsAvailable;
        {
            QMutexLocker k(&selector.compsMutex);
            compsAvailable = selector.compsAvailable;
        }
        if (node) {
            for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2!= compsAvailable.end(); ++it2) {
                if (it2->first.isColorPlane()) {
                    if (it2->first.getComponentsGlobalName() == layer) {
                        *comp = it2->first;
                        break;
                        
                    }
                } else {
                    if (it2->first.getLayerName() == layer) {
                        *comp = it2->first;
                        break;
                        
                    }
                }
            }
        }
        if (comp->getNumComponents() == 0) {
            if (layer == kNatronRGBAComponentsName) {
                *comp = ImageComponents::getRGBAComponents();
            } else if (layer == kNatronDisparityLeftPlaneName) {
                *comp = ImageComponents::getDisparityLeftComponents();
            } else if (layer == kNatronDisparityRightPlaneName) {
                *comp = ImageComponents::getDisparityRightComponents();
            } else if (layer == kNatronBackwardMotionVectorsPlaneName) {
                *comp = ImageComponents::getBackwardMotionComponents();
            } else if (layer == kNatronForwardMotionVectorsPlaneName) {
                *comp = ImageComponents::getForwardMotionComponents();
            }
        }
        return true;
    }
    
}

void
Node::Implementation::onLayerChanged(int inputNb,const ChannelSelector& selector)
{
    
    std::vector<std::string> entries = selector.layer->getEntries_mt_safe();
    int curLayer_i = selector.layer->getValue();
    assert(curLayer_i >= 0 && curLayer_i < (int)entries.size());
    selector.layerName->setValue(entries[curLayer_i], 0);
    {
        ///Clip preferences have changed 
        RenderScale s;
        s.x = s.y = 1;
        liveInstance->checkOFXClipPreferences_public(_publicInterface->getApp()->getTimeLine()->currentFrame(),
                                                     s,
                                                     OfxEffectInstance::natronValueChangedReasonToOfxValueChangedReason(Natron::eValueChangedReasonUserEdited),
                                                     true, true);
    }
    if (!selector.useRGBASelectors) {
        return;
    }
    
    Natron::ImageComponents comp ;
    if (!getSelectedLayer(inputNb, selector, &comp)) {
        for (int i = 0; i < 4; ++i) {
            selector.enabledChan[i]->setSecret(true);
        }

    } else {
        const std::vector<std::string>& channels = comp.getComponentsNames();
        for (int i = 0; i < 4; ++i) {
            if (i >= (int)(channels.size())) {
                selector.enabledChan[i]->setSecret(true);
            } else {
                selector.enabledChan[i]->setSecret(false);
                selector.enabledChan[i]->setDescription(channels[i]);
            }
            selector.enabledChan[i]->setValue(true, 0);
        }
    }
}

void
Node::Implementation::onMaskSelectorChanged(int inputNb,const MaskSelector& selector)
{
    
    int index = selector.channel->getValue();
    if ( (index == 0) && selector.enabled->isEnabled(0) ) {
        selector.enabled->setValue(false, 0);
        selector.enabled->setEnabled(0, false);
    } else if ( !selector.enabled->isEnabled(0) ) {
        selector.enabled->setEnabled(0, true);
        if ( _publicInterface->getInput(inputNb) ) {
            selector.enabled->setValue(true, 0);
        }
    }
    
    std::vector<std::string> entries = selector.channel->getEntries_mt_safe();
    int curChan_i = selector.channel->getValue();
    assert(curChan_i >= 0 && curChan_i < (int)entries.size());
    selector.channelName->setValue(entries[curChan_i], 0);
    {
        ///Clip preferences have changed
        RenderScale s;
        s.x = s.y = 1;
        liveInstance->checkOFXClipPreferences_public(_publicInterface->getApp()->getTimeLine()->currentFrame(),
                                                     s,
                                                     OfxEffectInstance::natronValueChangedReasonToOfxValueChangedReason(Natron::eValueChangedReasonUserEdited),
                                                     true, true);
    }
}

bool
Node::getUserComponents(int inputNb,bool* processChannels, bool* isAll,Natron::ImageComponents* layer) const
{
    //If the effect is multi-planar, it is expected to handle itself all the planes
    assert(!_imp->liveInstance->isMultiPlanar());
    
    std::map<int,ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(inputNb);
    if (foundSelector == _imp->channelsSelectors.end()) {
        //Fetch in input what the user has set for the output
        foundSelector = _imp->channelsSelectors.find(-1);
    }
    if (foundSelector == _imp->channelsSelectors.end()) {
        processChannels[0] = processChannels[1] = processChannels[2] = processChannels[3] = true;
        return false;
    }
    
    *isAll = !_imp->getSelectedLayer(inputNb, foundSelector->second, layer);
    if (foundSelector->second.useRGBASelectors) {
        processChannels[0] = foundSelector->second.enabledChan[0]->getValue();
        processChannels[1] = foundSelector->second.enabledChan[1]->getValue();
        processChannels[2] = foundSelector->second.enabledChan[2]->getValue();
        processChannels[3] = foundSelector->second.enabledChan[3]->getValue();
    } else {
        int numChans = layer->getNumComponents();
        processChannels[0] = true;
        if (numChans > 1) {
            processChannels[1] = true;
            if (numChans > 2) {
                processChannels[2] = true;
                if (numChans > 3) {
                    processChannels[3] = true;
                }
            }
        }
    }
 
    return true;

}

bool
Node::hasAtLeastOneChannelToProcess() const
{
    std::map<int,ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(-1);
    if (foundSelector == _imp->channelsSelectors.end()) {
        return true;
    }
    if (foundSelector->second.useRGBASelectors) {
        bool processChannels[4];
        processChannels[0] = foundSelector->second.enabledChan[0]->getValue();
        processChannels[1] = foundSelector->second.enabledChan[1]->getValue();
        processChannels[2] = foundSelector->second.enabledChan[2]->getValue();
        processChannels[3] = foundSelector->second.enabledChan[3]->getValue();
        if (!processChannels[0] && !processChannels[1] && !processChannels[2] && !processChannels[3]) {
            return false;
        }
    }
    return true;
}

void
Node::replaceCustomDataInlabel(const QString & data)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->nodeLabelKnob) {
        return;
    }
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
    bool thisDisabled = _imp->disableNodeKnob ? _imp->disableNodeKnob->getValue() : false;
    NodeGroup* isContainerGrp = dynamic_cast<NodeGroup*>(getGroup().get());
    if (isContainerGrp) {
        return thisDisabled || isContainerGrp->getNode()->isNodeDisabled();
    }
    return thisDisabled;
}

void
Node::setNodeDisabled(bool disabled)
{
    if (_imp->disableNodeKnob) {
        _imp->disableNodeKnob->setValue(disabled, 0);
    }
}

void
Node::showKeyframesOnTimeline(bool emitSignal)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( _imp->keyframesDisplayedOnTimeline || appPTR->isBackground() ) {
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
    assert( QThread::currentThread() == qApp->thread() );
    if ( !_imp->keyframesDisplayedOnTimeline || appPTR->isBackground() ) {
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
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->keyframesDisplayedOnTimeline;
}

void
Node::getAllKnobsKeyframes(std::list<SequenceTime>* keyframes)
{
    assert(keyframes);
    const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
    
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->getIsSecret() || !knobs[i]->getIsPersistant()) {
            continue;
        }
        if (!knobs[i]->canAnimate()) {
            continue;
        }
        int dim = knobs[i]->getDimension();
        File_Knob* isFile = dynamic_cast<File_Knob*>( knobs[i].get() );
        if (isFile) {
            ///skip file knobs
            continue;
        }
        for (int j = 0; j < dim; ++j) {
            if (knobs[i]->canAnimate() && knobs[i]->isAnimated(j)) {
                KeyFrameSet kfs = knobs[i]->getCurve(j)->getKeyFrames_mt_safe();
                for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                    keyframes->push_back( it->getTime() );
                }
            }
        }
    }
}

Natron::ImageBitDepthEnum
Node::getBitDepth() const
{
    bool foundShort = false;
    bool foundByte = false;
    
    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->supportedDepths.begin(); it != _imp->supportedDepths.end(); ++it) {
        switch (*it) {
            case Natron::eImageBitDepthFloat:
                
                return Natron::eImageBitDepthFloat;
                break;
            case Natron::eImageBitDepthByte:
                foundByte = true;
                break;
            case Natron::eImageBitDepthShort:
                foundShort = true;
                break;
            case Natron::eImageBitDepthNone:
                break;
        }
    }
    
    if (foundShort) {
        return Natron::eImageBitDepthShort;
    } else if (foundByte) {
        return Natron::eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);
        
        return Natron::eImageBitDepthNone;
    }
}

bool
Node::isSupportedBitDepth(Natron::ImageBitDepthEnum depth) const
{
    return std::find(_imp->supportedDepths.begin(), _imp->supportedDepths.end(), depth) != _imp->supportedDepths.end();
}

std::string
Node::getNodeExtraLabel() const
{
    if (_imp->nodeLabelKnob) {
        return _imp->nodeLabelKnob->getValue();
    } else {
        return std::string();
    }
}

bool
Node::hasSequentialOnlyNodeUpstream(std::string & nodeName) const
{
    ///Just take into account sequentiallity for writers
    if ( (_imp->liveInstance->getSequentialPreference() == Natron::eSequentialPreferenceOnlySequential) && _imp->liveInstance->isWriter() ) {
        nodeName = getScriptName_mt_safe();
        
        return true;
    } else {
        
        
        QMutexLocker l(&_imp->inputsMutex);
        
        for (InputsV::iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
            if ( (*it) && (*it)->hasSequentialOnlyNodeUpstream(nodeName) && (*it)->getLiveInstance()->isWriter() ) {
                nodeName = (*it)->getScriptName_mt_safe();
                
                return true;
            }
        }
        
        return false;
    }
}

bool
Node::isTrackerNode() const
{
    return _imp->liveInstance->isTrackerNode();
}

bool
Node::isPointTrackerNode() const
{
    return getPluginID() == PLUGINID_OFX_TRACKERPM;
}

bool
Node::isBackDropNode() const
{
    return getPluginID() == PLUGINID_NATRON_BACKDROP;
}

void
Node::updateEffectLabelKnob(const QString & name)
{
    if (!_imp->liveInstance) {
        return;
    }
    boost::shared_ptr<KnobI> knob = getKnobByName(kNatronOfxParamStringSublabelName);
    String_Knob* strKnob = dynamic_cast<String_Knob*>( knob.get() );
    if (strKnob) {
        strKnob->setValue(name.toStdString(), 0);
    }
}

bool
Node::canOthersConnectToThisNode() const
{
    if (dynamic_cast<BackDrop*>(_imp->liveInstance.get())) {
        return false;
    } else if (dynamic_cast<GroupOutput*>(_imp->liveInstance.get())) {
        return false;
    }
    ///In debug mode only allow connections to Writer nodes
# ifdef DEBUG
    
    return dynamic_cast<const ViewerInstance*>(_imp->liveInstance.get()) == NULL;
# else // !DEBUG
    return dynamic_cast<const ViewerInstance*>(_imp->liveInstance.get()) == NULL/* && !_imp->liveInstance->isWriter()*/;
# endif // !DEBUG
}

void
Node::setNodeIsRenderingInternal(std::list<Natron::Node*>& markedNodes)
{
    ///If marked, we alredy set render args
    std::list<Natron::Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), this);
    if (found != markedNodes.end()) {
        return;
    }
    
    ///Wait for the main-thread to be done dequeuing the connect actions queue
    if (QThread::currentThread() != qApp->thread()) {
        bool mustQuitProcessing;
        {
            QMutexLocker k(&_imp->mustQuitProcessingMutex);
            mustQuitProcessing = _imp->mustQuitProcessing;
        }
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        while (_imp->nodeIsDequeuing && !aborted() && !mustQuitProcessing) {
            _imp->nodeIsDequeuingCond.wait(&_imp->nodeIsDequeuingMutex);
            {
                QMutexLocker k(&_imp->mustQuitProcessingMutex);
                mustQuitProcessing = _imp->mustQuitProcessing;
            }
        }
    }
    
    ///Increment the node is rendering counter
    QMutexLocker nrLocker(&_imp->nodeIsRenderingMutex);
    ++_imp->nodeIsRendering;
    
    
    
    ///mark this
    markedNodes.push_back(this);
    
    ///Call recursively
    
    int maxInpu = getMaxInputCount();
    for (int i = 0; i < maxInpu; ++i) {
        boost::shared_ptr<Node> input = getInput(i);
        if (input) {
            input->setNodeIsRenderingInternal(markedNodes);
            
        }
    }
}

void
Node::setNodeIsNoLongerRenderingInternal(std::list<Natron::Node*>& markedNodes)
{
    
    ///If marked, we alredy set render args
    std::list<Natron::Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), this);
    if (found != markedNodes.end()) {
        return;
    }
    
    bool mustDequeue ;
    {
        int nodeIsRendering;
        ///Decrement the node is rendering counter
        QMutexLocker k(&_imp->nodeIsRenderingMutex);
        if (_imp->nodeIsRendering > 1) {
            --_imp->nodeIsRendering;
        } else {
            _imp->nodeIsRendering = 0;
        }
        nodeIsRendering = _imp->nodeIsRendering;
        
        
        mustDequeue = nodeIsRendering == 0 && !appPTR->isBackground();
    }
    
    if (mustDequeue) {

        
        ///Flag that the node is dequeuing.
        ///We don't wait here but in the setParallelRenderArgsInternal instead
        {
            QMutexLocker k(&_imp->nodeIsDequeuingMutex);
            _imp->nodeIsDequeuing = true;
        }
        Q_EMIT mustDequeueActions();
    }
    
    
    ///mark this
    markedNodes.push_back(this);
    
    ///Call recursively
    
    int maxInpu = getMaxInputCount();
    for (int i = 0; i < maxInpu; ++i) {
        boost::shared_ptr<Node> input = getInput(i);
        if (input) {
            input->setNodeIsNoLongerRenderingInternal(markedNodes);
            
        }
    }


}

void
Node::setNodeIsRendering()
{
    std::list<Natron::Node*> marked;
    setNodeIsRenderingInternal(marked);
}

void
Node::unsetNodeIsRendering()
{
    std::list<Natron::Node*> marked;
    setNodeIsNoLongerRenderingInternal(marked);
}

bool
Node::isNodeRendering() const
{
    QMutexLocker k(&_imp->nodeIsRenderingMutex);
    return _imp->nodeIsRendering > 0;
}

void
Node::dequeueActions()
{
    assert(QThread::currentThread() == qApp->thread());
    
    if (_imp->liveInstance) {
        _imp->liveInstance->dequeueValuesSet();
    }
    
    std::list<ConnectInputAction> queue;
    {
        QMutexLocker k(&_imp->connectionQueueMutex);
        queue = _imp->connectionQueue;
        _imp->connectionQueue.clear();
    }
    
    
    for (std::list<ConnectInputAction>::iterator it = queue.begin(); it!= queue.end(); ++it) {
        
        switch (it->type) {
            case eInputActionConnect:
                connectInput(it->node, it->inputNb);
                break;
            case eInputActionDisconnect:
                disconnectInput(it->inputNb);
                break;
            case eInputActionReplace:
                replaceInput(it->node, it->inputNb);
                break;
        }

    }
    
    QMutexLocker k(&_imp->nodeIsDequeuingMutex);
    _imp->nodeIsDequeuing = false;
    _imp->nodeIsDequeuingCond.wakeAll();
}

bool
Node::shouldCacheOutput() const
{
    {


        //If true then we're in analysis, so we cache the input of the analysis effect

        
        QMutexLocker k(&_imp->outputsMutex);
        std::size_t sz = _imp->outputs.size();
        if (sz > 1) {
            ///The node is referenced multiple times below, cache it
            return true;
        } else {
            if (sz == 1) {
                //The output has its settings panel opened, meaning the user is actively editing the output, we want this node to be cached then.
                //If force caching or aggressive caching are enabled, we by-pass and cache it anyway.
                Node* output = _imp->outputs.front();
                
                ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(output->getLiveInstance());
                if (isViewer) {
                    int activeInputs[2];
                    isViewer->getActiveInputs(activeInputs[0], activeInputs[1]);
                    if (output->getInput(activeInputs[0]).get() == this ||
                        output->getInput(activeInputs[1]).get() == this) {
                        return true;
                    }
                }
            
                
                return output->isSettingsPanelOpened() ||
                _imp->liveInstance->doesTemporalClipAccess() ||
                _imp->liveInstance->getRecursionLevel() > 0 ||
                isForceCachingEnabled() ||
                appPTR->isAggressiveCachingEnabled() ||
                (isPreviewEnabled() && !appPTR->isBackground());
            } else {
                // outputs == 0, never cache, unless explicitly set
                return isForceCachingEnabled() || appPTR->isAggressiveCachingEnabled();
            }
        }
    }
    
}

void
Node::setPosition(double x,double y)
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (gui) {
        gui->setPosition(x, y);
    }
}

void
Node::getPosition(double *x,double *y) const
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (gui) {
        gui->getPosition(x, y);
    } else {
        *x = 0.;
        *y = 0.;
    }
}

void
Node::setSize(double w,double h)
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (gui) {
        gui->setSize(w, h);
    }
}

void
Node::getSize(double* w,double* h) const
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (gui) {
        gui->getSize(w, h);
    } else {
        *w = 0.;
        *h = 0.;
    }
}

void
Node::getColor(double* r,double *g, double* b) const
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (gui) {
        gui->getColor(r, g, b);
    } else {
        *r = 0.;
        *g = 0.;
        *b = 0.;
    }
}

void
Node::setColor(double r, double g, double b)
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (gui) {
        gui->setColor(r, g, b);
    }
}

void
Node::setNodeGuiPointer(const boost::shared_ptr<NodeGuiI>& gui)
{
    assert(!_imp->guiPointer.lock());
    assert(QThread::currentThread() == qApp->thread());
    _imp->guiPointer = gui;
}

boost::shared_ptr<NodeGuiI>
Node::getNodeGui() const
{
    return _imp->guiPointer.lock();
}

bool
Node::isSettingsPanelOpened() const
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (!gui) {
        return false;
    }
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->isSettingsPanelOpened();
    }
    {
        NodePtr master = getMasterNode();
        if (master) {
            return master->isSettingsPanelOpened();
        }
        for (KnobLinkList::iterator it = _imp->nodeLinks.begin(); it != _imp->nodeLinks.end(); ++it) {
            if (it->masterNode.get() != this && it->masterNode->isSettingsPanelOpened()) {
                return true;
            }
        }
    }
    return gui->isSettingsPanelOpened();
    
}

void
Node::restoreClipPreferencesRecursive(std::list<Natron::Node*>& markedNodes)
{
    std::list<Natron::Node*>::const_iterator found = std::find(markedNodes.begin(), markedNodes.end(), this);
    if (found != markedNodes.end()) {
        return;
    }

    /*
     * Always call getClipPreferences on the inputs first since the preference of this node may 
     * depend on the inputs.
     */
    
    for (int i = 0; i < getMaxInputCount(); ++i) {
        NodePtr input = getInput(i);
        if (input) {
            input->restoreClipPreferencesRecursive(markedNodes);
        }
    }
    
    /*
     * And now call getClipPreferences on ourselves
     */
    
    _imp->liveInstance->restoreClipPreferences();
    refreshChannelSelectors(false);
    
    markedNodes.push_back(this);
    
}




/**
 * @brief Given a fullyQualifiedName, e.g: app1.Group1.Blur1
 * this function returns the PyObject attribute of Blur1 if it is defined, or Group1 otherwise
 * If app1 or Group1 does not exist at this point, this is a failure.
 **/
static PyObject* getAttrRecursive(const std::string& fullyQualifiedName,PyObject* parentObj,bool* isDefined)
{
    std::size_t foundDot = fullyQualifiedName.find(".");
    std::string attrName = foundDot == std::string::npos ? fullyQualifiedName : fullyQualifiedName.substr(0, foundDot);
    PyObject* obj = 0;
    if (PyObject_HasAttrString(parentObj, attrName.c_str())) {
        obj = PyObject_GetAttrString(parentObj, attrName.c_str());
    }
    
    ///We either found the parent object or we are on the last object in which case we return the parent
    if (!obj) {
        assert(fullyQualifiedName.find(".") == std::string::npos);
        *isDefined = false;
        return parentObj;
    } else {
        assert(obj);
        std::string recurseName;
        if (foundDot != std::string::npos) {
            recurseName = fullyQualifiedName;
            recurseName.erase(0, foundDot + 1);
        }
        if (!recurseName.empty()) {
            return getAttrRecursive(recurseName, obj, isDefined);
        } else {
            *isDefined = true;
            return obj;
        }
    }
    
}


void
Node::declareNodeVariableToPython(const std::string& nodeName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
	return;
#endif
    Natron::PythonGILLocker pgl;
    
    PyObject* mainModule = appPTR->getMainModule();
    assert(mainModule);
    
    std::string appID = getApp()->getAppIDString();
    
    std::string varName = appID + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = getAttrRecursive(varName, mainModule, &alreadyDefined);
    assert(nodeObj);
    (void)nodeObj;

    if (!alreadyDefined) {
        std::string script = varName + " = " + appID + ".getNode(\"";
        script.append(nodeName);
        script.append("\")\n");
        std::string err;
        if (!appPTR->isBackground()) {
            getApp()->printAutoDeclaredVariable(script);
        }
        if (!interpretPythonScript(script, &err, 0)) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::setNodeVariableToPython(const std::string& oldName,const std::string& newName)
{

    QString appID(getApp()->getAppIDString().c_str());
    QString str = QString(appID + ".%1 = " + appID + ".%2\ndel " + appID + ".%2\n").arg(newName.c_str()).arg(oldName.c_str());
    std::string script = str.toStdString();
    std::string err;
    if (!appPTR->isBackground()) {
        getApp()->printAutoDeclaredVariable(script);
    }
    if (!interpretPythonScript(script, &err, 0)) {
        qDebug() << err.c_str();
    }
    
}

void
Node::deleteNodeVariableToPython(const std::string& nodeName)
{
    if (getParentMultiInstance()) {
        return;
    }
    QString appID(getApp()->getAppIDString().c_str());
    QString str = QString("del " + appID + ".%1").arg(nodeName.c_str());
    std::string script = str.toStdString();
    std::string err;
    if (!appPTR->isBackground()) {
        getApp()->printAutoDeclaredVariable(script);
    }
    if (!interpretPythonScript(script, &err, 0)) {
        qDebug() << err.c_str();
    }
}




void
Node::declarePythonFields()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
	return;
#endif
    Natron::PythonGILLocker pgl;
    
    if (!getGroup()) {
        return ;
    }
    
    std::locale locale;
    std::string fullName = getFullyQualifiedName();
    
    std::string appID = getApp()->getAppIDString();
    bool alreadyDefined = false;
    
    std::string nodeFullName = appID + "." + fullName;
    PyObject* nodeObj = getAttrRecursive(nodeFullName, getMainModule(), &alreadyDefined);
    assert(nodeObj);
    (void)nodeObj;
    if (!alreadyDefined) {
        qDebug() << QString("declarePythonFields(): attribute ") + nodeFullName.c_str() + " is not defined";
        throw std::logic_error(std::string("declarePythonFields(): attribute ") + nodeFullName + " is not defined");
    }
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        const std::string& knobName = knobs[i]->getName();
        if (!knobName.empty() && knobName.find(" ") == std::string::npos && !std::isdigit(knobName[0],locale)) {
            declareParameterAsNodeField(nodeFullName, nodeObj, knobName);
        }
    }
}


void
Node::declareParameterAsNodeField(const std::string& nodeName,PyObject* nodeObj,const std::string& parameterName)
{
    if (PyObject_HasAttrString(nodeObj, parameterName.c_str())) {
        return;
    }
    
    std::string script = nodeName +  "." + parameterName + " = " +
    nodeName + ".getParam(\"" + parameterName + "\")\n";
    std::string err;
    if (!appPTR->isBackground()) {
        getApp()->printAutoDeclaredVariable(script);
    }
    if (!interpretPythonScript(script, &err, 0)) {
        qDebug() << err.c_str();
    }

}

std::string
Node::getKnobChangedCallback() const
{
    return _imp->knobChangedCallback ? _imp->knobChangedCallback->getValue() : std::string();
}

std::string
Node::getInputChangedCallback() const
{
    return _imp->inputChangedCallback ? _imp->inputChangedCallback->getValue() : std::string();
}

void
Node::Implementation::runOnNodeCreatedCB(bool userEdited)
{
    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeCreatedCB();
    if (cb.empty()) {
        return;
    }
    
    
    std::vector<std::string> args;
    std::string error;
    Natron::getFunctionArguments(cb, &error, &args);
    if (!error.empty()) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + error);
        return;
    }
    
    std::string signatureError;
    signatureError.append("The on node created callback supports the following signature(s):\n");
    signatureError.append("- callback(thisNode,app,userEdited)");
    if (args.size() != 3) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + signatureError);
        return;
    }
    
    if (args[0] != "thisNode" || args[1] != "app" || args[2] != "userEdited") {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + signatureError);
        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    std::stringstream ss;
    ss << cb << "(" << appID << "." << _publicInterface->getFullyQualifiedName() << "," << appID << ",";
    if (userEdited) {
        ss << "True";
    } else {
        ss << "False";
    }
    ss << ")\n";
    std::string output;
    std::string script = ss.str();
    if (!Natron::interpretPythonScript(script, &error, &output)) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + error);
    } else if (!output.empty()) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }
}

void
Node::Implementation::runOnNodeDeleteCB()
{
    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeDeleteCB();
    if (cb.empty()) {
        return;
    }
    std::vector<std::string> args;
    std::string error;
    Natron::getFunctionArguments(cb, &error, &args);
    if (!error.empty()) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + error);
        return;
    }
    
    std::string signatureError;
    signatureError.append("The on node deletion callback supports the following signature(s):\n");
    signatureError.append("- callback(thisNode,app)");
    if (args.size() != 2) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + signatureError);
        return;
    }
    
    if (args[0] != "thisNode" || args[1] != "app") {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + signatureError);
        return;
    }
    
    std::string appID = _publicInterface->getApp()->getAppIDString();
    std::stringstream ss;
    ss << cb << "(" << appID << "." << _publicInterface->getFullyQualifiedName() << "," << appID << ")\n";
    
    std::string err;
    std::string output;
    if (!Natron::interpretPythonScript(cb, &err, &output)) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + err);
    } else if (!output.empty()) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }
}

std::string
Node::getBeforeRenderCallback() const
{
    return _imp->beforeRender ? _imp->beforeRender->getValue() : std::string();
}

std::string
Node::getBeforeFrameRenderCallback() const
{
    return _imp->beforeFrameRender ? _imp->beforeFrameRender->getValue() : std::string();
}

std::string
Node::getAfterRenderCallback() const
{
    return _imp->afterRender ? _imp->afterRender->getValue() : std::string();
}

std::string
Node::getAfterFrameRenderCallback() const
{
    return _imp->afterFrameRender ? _imp->afterFrameRender->getValue() : std::string();
}

void
Node::runInputChangedCallback(int index)
{
    std::string cb = getInputChangedCallback();
    if (!cb.empty()) {
        _imp->runInputChangedCallback(index, cb);
    }
}

void
Node::Implementation::runInputChangedCallback(int index,const std::string& cb)
{
    std::vector<std::string> args;
    std::string error;
    Natron::getFunctionArguments(cb, &error, &args);
    if (!error.empty()) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + error);
        return;
    }
    
    std::string signatureError;
    signatureError.append("The on input changed callback supports the following signature(s):\n");
    signatureError.append("- callback(inputIndex,thisNode,thisGroup,app)");
    if (args.size() != 4) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + signatureError);
        return;
    }
    
    if (args[0] != "inputIndex" || args[1] != "thisNode" || args[2] != "thisGroup" || args[3] != "app") {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + signatureError);
        return;
    }
    
    std::string appID = _publicInterface->getApp()->getAppIDString();

    boost::shared_ptr<NodeCollection> collection = _publicInterface->getGroup();
    assert(collection);
    if (!collection) {
        return;
    }
    
    std::string thisGroupVar;
    NodeGroup* isParentGrp = dynamic_cast<NodeGroup*>(collection.get());
    if (isParentGrp) {
        thisGroupVar = appID + "." + isParentGrp->getNode()->getFullyQualifiedName();
    } else {
        thisGroupVar = appID;
    }
    
    std::stringstream ss;
    ss << cb << "(" << index << "," << appID << "." << _publicInterface->getFullyQualifiedName() << "," << thisGroupVar << "," << appID << ")\n";
    
    std::string script = ss.str();
    std::string output;
    if (!Natron::interpretPythonScript(script, &error,&output)) {
        _publicInterface->getApp()->appendToScriptEditor(QObject::tr("Failed to execute callback: ").toStdString() + error);
    } else {
        if (!output.empty()) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }

}

void
Node::refreshChannelSelectors(bool setValues)
{
    if (!isNodeCreated()) {
        return;
    }
    _imp->liveInstance->setComponentsAvailableDirty(true);
    
    for (std::map<int,ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it!= _imp->channelsSelectors.end(); ++it) {
        
        NodePtr node;
        if (it->first == -1) {
            node = shared_from_this();
        } else {
            node = getInput(it->first);
        }
        
        std::vector<std::string> currentLayerEntries = it->second.layer->getEntries_mt_safe();
        
        std::string curLayer = it->second.layerName->getValue();

        
        std::vector<std::string> choices;
        if (it->second.hasAllChoice) {
            choices.push_back("All");
        } else {
            choices.push_back("None");
        }
        bool gotColor = false;
        bool gotDisparityLeft = false;
        bool gotDisparityRight = false;
        bool gotMotionBw = false;
        bool gotMotionFw = false;
        
        int colorIndex = -1;
        Natron::ImageComponents colorComp;
        
        if (node) {
            EffectInstance::ComponentsAvailableMap compsAvailable;
            node->getLiveInstance()->getComponentsAvailable(getApp()->getTimeLine()->currentFrame(), &compsAvailable);
            {
                QMutexLocker k(&it->second.compsMutex);
                it->second.compsAvailable = compsAvailable;
            }
            for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2!= compsAvailable.end(); ++it2) {
                if (it2->first.isColorPlane()) {
                    int numComp = it2->first.getNumComponents();
                    colorIndex = choices.size();
                    colorComp = it2->first;
                    if (numComp == 1) {
                        choices.push_back(kNatronAlphaComponentsName);
                    } else if (numComp == 3) {
                        choices.push_back(kNatronRGBComponentsName);
                    } else if (numComp == 4) {
                        choices.push_back(kNatronRGBAComponentsName);
                    } else {
                        assert(false);
                    }
                    gotColor = true;
                } else {
                    choices.push_back(it2->first.getLayerName());
                    if (it2->first.getLayerName() == kNatronBackwardMotionVectorsPlaneName) {
                        gotMotionBw = true;
                    } else if (it2->first.getLayerName() == kNatronForwardMotionVectorsPlaneName) {
                        gotMotionFw = true;
                    } else if (it2->first.getLayerName() == kNatronDisparityLeftPlaneName) {
                        gotDisparityLeft = true;
                    } else if (it2->first.getLayerName() == kNatronForwardMotionVectorsPlaneName) {
                        gotDisparityRight = true;
                    }
                }
            }
        }
        
        if (!gotColor) {
            std::vector<std::string>::iterator pos = choices.begin();
            if (choices.size() > 0) {
                ++pos;
                colorIndex = 1;
            } else {
                colorIndex = 0;
            }
            colorComp = ImageComponents::getRGBAComponents();
            choices.insert(pos,kNatronRGBAComponentsName);
            
        }
        if (!gotDisparityLeft) {
            choices.push_back(kNatronDisparityLeftPlaneName);
        }
        if (!gotDisparityRight) {
            choices.push_back(kNatronDisparityRightPlaneName);
        }
        if (!gotMotionFw) {
            choices.push_back(kNatronForwardMotionVectorsPlaneName);
        }
        if (!gotMotionBw) {
            choices.push_back(kNatronBackwardMotionVectorsPlaneName);
        }

        
        it->second.layer->populateChoices(choices);
 
        if (setValues) {
            assert(colorIndex != -1 && colorIndex >= 0 && colorIndex < (int)choices.size());
            if (it->second.hasAllChoice && _imp->liveInstance->isPassThroughForNonRenderedPlanes() == EffectInstance::ePassThroughRenderAllRequestedPlanes) {
                it->second.layer->setValue(0, 0);
                it->second.layerName->setValue(choices[0], 0);
            } else {
                it->second.layer->setValue(colorIndex,0);
                it->second.layerName->setValue(choices[colorIndex], 0);
            }
        } else {
            if (!curLayer.empty()) {
                bool isColor = curLayer == kNatronRGBAComponentsName ||
                curLayer == kNatronRGBComponentsName ||
                curLayer == kNatronAlphaComponentsName;
                for (std::size_t i = 0; i < choices.size(); ++i) {
                    if (choices[i] == curLayer || (isColor && (choices[i] == kNatronRGBAComponentsName || choices[i] ==
                                                               kNatronRGBComponentsName || choices[i] == kNatronAlphaComponentsName))) {
                        it->second.layer->setValue(i, 0);
                        if (isColor) {
                            assert(colorIndex != -1);
                            //Since color plane may have changed (RGB, or RGBA or Alpha), adjust the secretness of the checkboxes
                            const std::vector<std::string>& channels = colorComp.getComponentsNames();
                            for (int j = 0; j < 4; ++j) {
                                if (j >= (int)(channels.size())) {
                                    it->second.enabledChan[j]->setSecret(true);
                                } else {
                                    it->second.enabledChan[j]->setSecret(false);
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    
    for (std::map<int,MaskSelector>::iterator it = _imp->maskSelectors.begin(); it!=_imp->maskSelectors.end(); ++it) {
        NodePtr node;
        if (it->first == -1) {
            node = shared_from_this();
        } else {
            node = getInput(it->first);
        }
        
        std::vector<std::string> currentLayerEntries = it->second.channel->getEntries_mt_safe();
        
        std::string curLayer = it->second.channelName->getValue();
        
        
        std::vector<std::string> choices;
        choices.push_back("None");
        bool gotColor = false;
        int alphaIndex = -1;
        if (node) {
            EffectInstance::ComponentsAvailableMap compsAvailable;
            node->getLiveInstance()->getComponentsAvailable(getApp()->getTimeLine()->currentFrame(), &compsAvailable);
            
            std::vector<ImageComponents> compsOrdered;
            for (EffectInstance::ComponentsAvailableMap::iterator it = compsAvailable.begin(); it!=compsAvailable.end(); ++it) {
                if (it->first.isColorPlane()) {
                    compsOrdered.insert(compsOrdered.begin(), it->first);
                } else {
                    compsOrdered.push_back(it->first);
                }
            }
            {
                
                QMutexLocker k(&it->second.compsMutex);
                it->second.compsAvailable = compsOrdered;
            }
            for (std::vector<ImageComponents>::iterator it2 = compsOrdered.begin(); it2!= compsOrdered.end(); ++it2) {
                
                const std::vector<std::string>& channels = it2->getComponentsNames();
                const std::string& layerName = it2->isColorPlane() ? it2->getComponentsGlobalName() : it2->getLayerName();
                for (std::size_t i = 0; i < channels.size(); ++i) {
                    choices.push_back(layerName + "." + channels[i]);
                }
                if (it2->isColorPlane()) {
                    if (channels.size() == 1 || channels.size() == 4) {
                        alphaIndex = choices.size() - 1;
                    }
                    gotColor = true;
                }
            }
        }
        
        if (!gotColor) {
            std::vector<std::string>::iterator pos = choices.begin();
            assert(choices.size() > 0);
            ++pos;
            const ImageComponents& rgba = ImageComponents::getRGBAComponents();
            const std::vector<std::string>& channels = rgba.getComponentsNames();
            const std::string& layerName = rgba.getComponentsGlobalName();
            for (std::size_t i = 0; i < channels.size(); ++i) {
                choices.push_back(layerName + "." + channels[i]);
            }
            alphaIndex = choices.size() - 1;
        }
        it->second.channel->populateChoices(choices);
        
        if (setValues) {
            assert(alphaIndex != -1 && alphaIndex >= 0 && alphaIndex < (int)choices.size());
            it->second.channel->setValue(alphaIndex,0);
            it->second.channelName->setValue(choices[alphaIndex], 0);
        } else {
            if (!curLayer.empty()) {
                for (std::size_t i = 0; i < choices.size(); ++i) {
                    if (choices[i] == curLayer) {
                        it->second.channel->setValue(i, 0);
                        break;
                    }
                }
            }
        }
    }
}

void
Node::addUserComponents(const Natron::ImageComponents& comps)
{
    {
        QMutexLocker k(&_imp->createdComponentsMutex);
        for (std::list<ImageComponents>::iterator it = _imp->createdComponents.begin(); it!=_imp->createdComponents.end(); ++it) {
            if (it->getLayerName() == comps.getLayerName()) {
                Natron::errorDialog(tr("Layer").toStdString(), tr("A Layer with the same name already exists").toStdString());
                return;
            }
        }
        
        _imp->createdComponents.push_back(comps);
    }
    {
        ///Clip preferences have changed
        RenderScale s;
        s.x = s.y = 1;
        getLiveInstance()->checkOFXClipPreferences_public(getApp()->getTimeLine()->currentFrame(),
                                                          s,
                                                          OfxEffectInstance::natronValueChangedReasonToOfxValueChangedReason(Natron::eValueChangedReasonUserEdited),
                                                          true, true);
    }
    
}

void
Node::getUserComponents(std::list<Natron::ImageComponents>* comps)
{
    QMutexLocker k(&_imp->createdComponentsMutex);
    *comps = _imp->createdComponents;
}

//////////////////////////////////

InspectorNode::InspectorNode(AppInstance* app,
                             const boost::shared_ptr<NodeCollection>& group,
                             Natron::Plugin* plugin,
                             int maxInputs)
: Node(app,group,plugin)
, _maxInputs(maxInputs)
{
}

InspectorNode::~InspectorNode()
{
}

bool
InspectorNode::connectInput(const boost::shared_ptr<Node>& input,
                            int inputNumber)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    ///cannot connect more than _maxInputs inputs.
    assert(inputNumber <= _maxInputs);
    
    assert(input);
    
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }
    
    ///If the node 'input' is already to an input of the inspector, find it.
    ///If it has the same input number as what we want just return, otherwise
    ///disconnect it and continue as usual.
    int inputAlreadyConnected = inputIndex( input.get() );
    if (inputAlreadyConnected != -1) {
        if (inputAlreadyConnected == inputNumber) {
            return false;
        } else {
            disconnectInput(inputAlreadyConnected);
        }
    }
    
    if ( !Node::connectInput(input, inputNumber) ) {
        computeHash();
    }
    
    return true;
}


void
InspectorNode::setActiveInputAndRefresh(int inputNb)
{
    if ( ( inputNb > (_maxInputs - 1) ) || (inputNb < 0) || (getInput(inputNb) == NULL) ) {
        return;
    }

    computeHash();
    Q_EMIT inputChanged(inputNb);
    onInputChanged(inputNb);

    runInputChangedCallback(inputNb);
    
    
    if ( isOutputNode() ) {
        Natron::OutputEffectInstance* oei = dynamic_cast<Natron::OutputEffectInstance*>( getLiveInstance() );
        assert(oei);
        if (oei) {
            oei->renderCurrentFrame(true);
        }
    }
}

int
InspectorNode::getPreferredInputInternal(bool connected) const
{
    
    bool useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();
    
    ///Find an input named A
    std::string inputNameToFind,otherName;
    if (useInputA) {
        inputNameToFind = "A";
        otherName = "B";
    } else {
        inputNameToFind = "B";
        otherName = "A";
    }
    int foundOther = -1;
    int maxinputs = getMaxInputCount();
    for (int i = 0; i < maxinputs ; ++i) {
        std::string inputLabel = getInputLabel(i);
        if (inputLabel == inputNameToFind ) {
            NodePtr inp = getInput(i);
            if ((connected && inp) || (!connected && !inp)) {
                return i;
            }
        } else if (inputLabel == otherName) {
            foundOther = i;
        }
    }
    if (foundOther != -1) {
        NodePtr inp = getInput(foundOther);
        if ((connected && inp) || (!connected && !inp)) {
            return foundOther;
        }
    }
    
    for (int i = 0; i < _maxInputs; ++i) {
        NodePtr inp = getInput(i);
        if ((!inp && !connected) || (inp && connected)) {
            return i;
        }
    }
    return -1;
}

int
InspectorNode::getPreferredInput() const
{
    return getPreferredInputInternal(true);
}

int
InspectorNode::getPreferredInputForConnection() const
{
    return getPreferredInputInternal(false);
}

