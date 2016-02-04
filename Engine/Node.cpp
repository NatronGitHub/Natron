/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Node.h"

#include <limits>
#include <locale>
#include <algorithm> // min, max
#include <bitset>
#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtCore/QWaitCondition>
#include <QtCore/QTextStream>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <ofxNatron.h>

#include "Global/MemoryInfo.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Backdrop.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Dot.h"
#include "Engine/EffectInstance.h"
#include "Engine/Format.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Log.h"
#include "Engine/Lut.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeGuiI.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/Plugin.h"
#include "Engine/PrecompNode.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Timer.h"
#include "Engine/TLSHolder.h"
#include "Engine/ViewerInstance.h"

///The flickering of edges/nodes in the nodegraph will be refreshed
///at most every...
#define NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS 1

NATRON_NAMESPACE_ENTER;

using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;

namespace { // protect local classes in anonymous namespace
    /*The output node was connected from inputNumber to this...*/
    typedef std::map<NodeWPtr,int > DeactivatedState;
    typedef std::list<Node::KnobLink> KnobLinkList;
    typedef std::vector<NodeWPtr> InputsV;
    
   
    
    class ChannelSelector {
        
    public:
        
        boost::weak_ptr<KnobChoice> layer;
        boost::weak_ptr<KnobString> layerName;
        bool hasAllChoice; // if true, the layer has a "all" entry
        
        mutable QMutex compsMutex;
        
        //Stores the components available at build time of the choice menu
        EffectInstance::ComponentsAvailableMap compsAvailable;
        
        ChannelSelector()
        : layer()
        , layerName()
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
            hasAllChoice = other.hasAllChoice;
            layerName = other.layerName;
            QMutexLocker k(&compsMutex);
            compsAvailable = other.compsAvailable;
        }
    };
    
    class MaskSelector {
        
    public:
        
        boost::weak_ptr<KnobBool> enabled;
        boost::weak_ptr<KnobChoice> channel;
        boost::weak_ptr<KnobString> channelName;
        
        mutable QMutex compsMutex;
        //Stores the components available at build time of the choice menu
      
        std::vector<std::pair<ImageComponents,NodeWPtr > > compsAvailable;
        
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
    
    struct NativeTransformOverlayKnobs
    {
        boost::shared_ptr<KnobDouble> translate;
        boost::shared_ptr<KnobDouble> scale;
        boost::shared_ptr<KnobBool> scaleUniform;
        boost::shared_ptr<KnobDouble> rotate;
        boost::shared_ptr<KnobDouble> skewX;
        boost::shared_ptr<KnobDouble> skewY;
        boost::shared_ptr<KnobChoice> skewOrder;
        boost::shared_ptr<KnobDouble> center;
    };
    
    struct FormatKnob {
        boost::weak_ptr<KnobInt> size;
        boost::weak_ptr<KnobDouble> par;
        boost::weak_ptr<KnobChoice> formatChoice;
    };
}



struct Node::Implementation
{
    Implementation(Node* publicInterface,
                   AppInstance* app_,
                   const boost::shared_ptr<NodeCollection>& collection,
                   Plugin* plugin_)
    : _publicInterface(publicInterface)
    , group(collection)
    , precomp()
    , app(app_)
    , isPartOfProject(true)
    , knobsInitialized(false)
    , inputsInitialized(false)
    , outputsMutex()
    , outputs()
    , guiOutputs()
    , inputsMutex()
    , inputs()
    , guiInputs()
    , mustCopyGuiInputs(false)
    , effect()
    , inputsComponents()
    , outputComponents()
    , inputsLabelsMutex()
    , inputLabels()
    , scriptName()
    , label()
    , cacheID()
    , deactivatedState()
    , activatedMutex()
    , activated(true)
    , plugin(plugin_)
    , pluginPythonModuleMutex()
    , pluginPythonModule()
    , pyplugChangedSinceScript(false)
    , pyPlugID()
    , pyPlugLabel()
    , pyPlugDesc()
    , pyPlugVersion(0)
    , computingPreview(false)
    , computingPreviewMutex()
    , pluginInstanceMemoryUsed(0)
    , memoryUsedMutex()
    , mustQuitPreview(false)
    , mustQuitPreviewMutex()
    , mustQuitPreviewCond()
    , renderInstancesSharedMutex(QMutex::Recursive)
    , knobsAge(0)
    , knobsAgeMutex()
    , masterNodeMutex()
    , masterNode()
    , nodeLinks()
    , frameIncrKnob()
    , nodeSettingsPage()
    , nodeLabelKnob()
    , previewEnabledKnob()
    , disableNodeKnob()
    , infoPage()
    , nodeInfos()
    , refreshInfoButton()
    , useFullScaleImagesWhenRenderScaleUnsupported()
    , forceCaching()
    , hideInputs()
    , beforeFrameRender()
    , beforeRender()
    , afterFrameRender()
    , afterRender()
    , enabledChan()
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
    , keyframesDisplayedOnTimeline(false)
    , timersMutex()
    , lastRenderStartedSlotCallTime()
    , lastInputNRenderStartedSlotCallTime()
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
    , nativeTransformOverlays()
    , nodeCreated(false)
    , createdComponentsMutex()
    , createdComponents()
    , paintStroke()
    , pluginsPropMutex()
    , pluginSafety(eRenderSafetyInstanceSafe)
    , currentThreadSafety(eRenderSafetyInstanceSafe)
    , currentSupportTiles(false)
    , currentSupportOpenGLRender(ePluginOpenGLRenderSupportNone)
    , currentSupportSequentialRender(eSequentialPreferenceNotSequential)
    , currentCanTransform(false)
    , draftModeUsed(false)
    , mustComputeInputRelatedData(true)
    , duringPaintStrokeCreation(false)
    , lastStrokeMovementMutex()
    , strokeBitmapCleared(false)
    , useAlpha0ToConvertFromRGBToRGBA(false)
    , isBeingDestroyedMutex()
    , isBeingDestroyed(false)
    , inputModifiedRecursion(0)
    , inputsModified()
    , refreshIdentityStateRequestsCount(0)
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
                                   const boost::shared_ptr<KnobGroup>& group,
                                   const boost::shared_ptr<KnobPage>& page);
    
    void restoreKnobLinksRecursive(const GroupKnobSerialization* group,
                                   const NodesList & allNodes,
                                   const std::map<std::string,std::string>& oldNewScriptNamesMapping);
    
    void ifGroupForceHashChangeOfInputs();
    
    void runOnNodeCreatedCB(bool userEdited);
    
    void runOnNodeDeleteCB();
    
    void runOnNodeCreatedCBInternal(const std::string& cb,bool userEdited);
    
    void runOnNodeDeleteCBInternal(const std::string& cb);

    
    void appendChild(const NodePtr& child);
    
    void runInputChangedCallback(int index,const std::string& script);
    
    void createChannelSelector(int inputNb,const std::string & inputName, bool isOutput,const boost::shared_ptr<KnobPage>& page);
    
    void onLayerChanged(int inputNb,const ChannelSelector& selector);
    
    void onMaskSelectorChanged(int inputNb,const MaskSelector& selector);
    
    bool getSelectedLayerInternal(int inputNb,const ChannelSelector& selector, ImageComponents* comp) const;
    
    
    Node* _publicInterface;
    
    boost::weak_ptr<NodeCollection> group;
    
    boost::weak_ptr<PrecompNode> precomp;
    
    AppInstance* app; // pointer to the app: needed to access the application's default-project's format
    
    bool isPartOfProject;
    bool knobsInitialized;
    bool inputsInitialized;
    mutable QMutex outputsMutex;
    NodesWList outputs,guiOutputs;
    
    mutable QMutex inputsMutex; //< protects guiInputs so the serialization thread can access them
    
    ///The  inputs are the ones used while rendering and guiInputs the ones used by the gui whenever
    ///the node is currently rendering. Once the render is finished, inputs are refreshed automatically to the value of
    ///guiInputs
    InputsV inputs,guiInputs;
    
    ///Set to true when inputs must be refreshed to reflect the value of guiInputs
    bool mustCopyGuiInputs;
    
    //to the inputs in a thread-safe manner.
    EffectInstPtr  effect; //< the effect hosted by this node
    
    ///These two are also protected by inputsMutex
    std::vector< std::list<ImageComponents> > inputsComponents;
    std::list<ImageComponents> outputComponents;
    
    mutable QMutex nameMutex;
    mutable QMutex inputsLabelsMutex;
    std::vector<std::string> inputLabels; // inputs name
    std::string scriptName; //node name internally and as visible to python
    std::string label; // node label as visible in the GUI
    
    ///The cacheID is the first script name that was given to a node
    ///it is then used in the cache to identify images that belong to this node
    ///In order for the cache to be persistent, the cacheID is serialized with the node
    ///and 2 nodes cannot have the same cacheID.
    std::string cacheID;
    
    DeactivatedState deactivatedState;
    mutable QMutex activatedMutex;
    bool activated;
    
    Plugin* plugin; //< the plugin which stores the function to instantiate the effect
    
    mutable QMutex pluginPythonModuleMutex;
    std::string pluginPythonModule; // the filename of the python script
    
    //Set to true when the user has edited a PyPlug
    bool pyplugChangedSinceScript;
    std::string pyPlugID; //< if this is a pyplug, this is the ID of the Plug-in. This is because the plugin handle will be the one of the Group
    std::string pyPlugLabel;
    std::string pyPlugDesc;
    int pyPlugVersion;
    
    bool computingPreview;
    mutable QMutex computingPreviewMutex;
    
    size_t pluginInstanceMemoryUsed; //< global count on all EffectInstance's of the memory they use.
    QMutex memoryUsedMutex; //< protects _pluginInstanceMemoryUsed
    
    bool mustQuitPreview;
    QMutex mustQuitPreviewMutex;
    QWaitCondition mustQuitPreviewCond;
    
    
    QMutex renderInstancesSharedMutex; //< see eRenderSafetyInstanceSafe in EffectInstance::renderRoI
    //only 1 clone can render at any time
    
    U64 knobsAge; //< the age of the knobs in this effect. It gets incremented every times the effect has its evaluate() function called.
    mutable QReadWriteLock knobsAgeMutex; //< protects knobsAge and hash
    Hash64 hash; //< recomputed everytime knobsAge is changed.
    
    mutable QMutex masterNodeMutex; //< protects masterNode and nodeLinks
    NodeWPtr masterNode; //< this points to the master when the node is a clone
    KnobLinkList nodeLinks; //< these point to the parents of the params links
    
    boost::weak_ptr<KnobInt> frameIncrKnob;
    
    boost::weak_ptr<KnobPage> nodeSettingsPage;
    boost::weak_ptr<KnobString> nodeLabelKnob;
    boost::weak_ptr<KnobBool> previewEnabledKnob;
    boost::weak_ptr<KnobBool> disableNodeKnob;
    boost::weak_ptr<KnobString> knobChangedCallback;
    boost::weak_ptr<KnobString> inputChangedCallback;
    boost::weak_ptr<KnobString> nodeCreatedCallback;
    boost::weak_ptr<KnobString> nodeRemovalCallback;
    
    boost::weak_ptr<KnobPage> infoPage;
    boost::weak_ptr<KnobString> nodeInfos;
    boost::weak_ptr<KnobButton> refreshInfoButton;
    
    boost::weak_ptr<KnobBool> useFullScaleImagesWhenRenderScaleUnsupported;
    boost::weak_ptr<KnobBool> forceCaching;
    boost::weak_ptr<KnobBool> hideInputs;
    
    boost::weak_ptr<KnobString> beforeFrameRender;
    boost::weak_ptr<KnobString> beforeRender;
    boost::weak_ptr<KnobString> afterFrameRender;
    boost::weak_ptr<KnobString> afterRender;
    
    boost::weak_ptr<KnobBool> enabledChan[4];
    boost::weak_ptr<KnobDouble> mixWithSource;
    
    FormatKnob pluginFormatKnobs;
    
    std::map<int,ChannelSelector> channelsSelectors;
    std::map<int,MaskSelector> maskSelectors;
    
    boost::shared_ptr<RotoContext> rotoContext; //< valid when the node has a rotoscoping context (i.e: paint context)
    
    mutable QMutex imagesBeingRenderedMutex;
    QWaitCondition imageBeingRenderedCond;
    std::list< boost::shared_ptr<Image> > imagesBeingRendered; ///< a list of all the images being rendered simultaneously
    
    std::list <ImageBitDepthEnum> supportedDepths;
    
    ///True when several effect instances are represented under the same node.
    bool isMultiInstance;
    NodeWPtr multiInstanceParent;
    mutable QMutex childrenMutex;
    NodesWList children;
    
    ///the name of the parent at the time this node was created
    std::string multiInstanceParentName;
    bool keyframesDisplayedOnTimeline;
    
    ///This is to avoid the slots connected to the main-thread to be called too much
    QMutex timersMutex; //< protects lastRenderStartedSlotCallTime & lastInputNRenderStartedSlotCallTime
    timeval lastRenderStartedSlotCallTime;
    timeval lastInputNRenderStartedSlotCallTime;
    
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
    
    std::list<boost::shared_ptr<KnobDouble> > nativePositionOverlays;
    std::list<NativeTransformOverlayKnobs> nativeTransformOverlays;
    
    
    bool nodeCreated;
    
    mutable QMutex createdComponentsMutex;
    std::list<ImageComponents> createdComponents; // comps created by the user
    
    boost::weak_ptr<RotoDrawableItem> paintStroke;
    
    mutable QMutex pluginsPropMutex;
    RenderSafetyEnum pluginSafety,currentThreadSafety;
    bool currentSupportTiles;
    PluginOpenGLRenderSupport currentSupportOpenGLRender;
    SequentialPreferenceEnum currentSupportSequentialRender;
    bool currentCanTransform;
    bool draftModeUsed,mustComputeInputRelatedData;

    
    bool duringPaintStrokeCreation; // protected by lastStrokeMovementMutex
    mutable QMutex lastStrokeMovementMutex;
    bool strokeBitmapCleared;
    
    
    //This flag is used for the Roto plug-in and for the Merge inside the rotopaint tree
    //so that if the input of the roto node is RGB, it gets converted with alpha = 0, otherwise the user
    //won't be able to paint the alpha channel
    bool useAlpha0ToConvertFromRGBToRGBA;
    
    mutable QMutex isBeingDestroyedMutex;
    bool isBeingDestroyed;
    
    /*
     Used to block render emitions while modifying nodes links
     MT-safe: only accessed/used on main thread
     */
    int inputModifiedRecursion;
    std::set<int> inputsModified;
    
    //For readers, this is the name of the views in the file
    std::vector<std::string> createdViews;
    
    //To concatenate calls to refreshIdentityState, accessed only on main-thread
    int refreshIdentityStateRequestsCount;
    
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
           Plugin* plugin)
: QObject()
, _imp( new Implementation(this,app,group,plugin) )
{
    QObject::connect( this, SIGNAL( pluginMemoryUsageChanged(qint64) ), appPTR, SLOT( onNodeMemoryRegistered(qint64) ) );
    QObject::connect(this, SIGNAL(mustDequeueActions()), this, SLOT(dequeueActions()));
    QObject::connect(this, SIGNAL(mustComputeHashOnMainThread()), this, SLOT(doComputeHashOnMainThread()));
    QObject::connect(this, SIGNAL(refreshIdentityStateRequested()), this, SLOT(onRefreshIdentityStateRequestReceived()), Qt::QueuedConnection);
}

void
Node::createRotoContextConditionnally()
{
    assert(!_imp->rotoContext);
    assert(_imp->effect);
    ///Initialize the roto context if any
    if ( isRotoNode() || isRotoPaintingNode() ) {
        _imp->effect->beginChanges();
        _imp->rotoContext.reset( new RotoContext(shared_from_this()) );
        _imp->effect->endChanges(true);
        _imp->rotoContext->createBaseLayer();
    }
}

const Plugin*
Node::getPlugin() const
{
    return _imp->plugin;
}

void
Node::switchInternalPlugin(Plugin* plugin)
{
    _imp->plugin = plugin;
}

void
Node::setPrecompNode(const boost::shared_ptr<PrecompNode>& precomp)
{
    //QMutexLocker k(&_imp->pluginsPropMutex);
    _imp->precomp = precomp;
}

boost::shared_ptr<PrecompNode>
Node::isPartOfPrecomp() const
{
    //QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->precomp.lock();
}

void
Node::load(const CreateNodeArgs& args)
{
    ///Called from the main thread. MT-safe
    assert( QThread::currentThread() == qApp->thread() );
    
    ///cannot load twice
    assert(!_imp->effect);
    _imp->isPartOfProject = args.addToProject;
    
    boost::shared_ptr<NodeCollection> group = getGroup();

    
    bool isMultiInstanceChild = false;
    if (!args.multiInstanceParentName.empty()) {
        _imp->multiInstanceParentName = args.multiInstanceParentName;
        isMultiInstanceChild = true;
        _imp->isMultiInstance = false;
        fetchParentMultiInstancePointer();
    }
    

    NodePtr thisShared = shared_from_this();

    int renderScaleSupportPreference = appPTR->getCurrentSettings()->getRenderScaleSupportPreference(_imp->plugin);

    LibraryBinary* binary = _imp->plugin->getLibraryBinary();
    std::pair<bool,EffectBuilder> func;
    if (binary) {
        func = binary->findFunction<EffectBuilder>("BuildEffect");
    }
    bool isFileDialogPreviewReader = args.fixedName.contains(NATRON_FILE_DIALOG_PREVIEW_READER_NAME);
    
    bool nameSet = false;
    /*
     If the serialization is not null, we are either pasting a node or loading it from a project.
     */
    if (args.serialization) {
        
        assert(args.reason == eCreateNodeReasonCopyPaste || args.reason == eCreateNodeReasonProjectLoad);
        
        if (group && !group->isCacheIDAlreadyTaken(args.serialization->getCacheID())) {
            QMutexLocker k(&_imp->nameMutex);
            _imp->cacheID = args.serialization->getCacheID();
        }
        if (/*!dontLoadName && */!nameSet && args.fixedName.isEmpty()) {
            const std::string& baseName = args.serialization->getNodeScriptName();
            std::string name = baseName;
            int no = 1;
            do {
                
                if (no > 1) {
                    std::stringstream ss;
                    ss << baseName;
                    ss << '_';
                    ss << no;
                    name = ss.str();
                }
                ++no;
            } while(group && group->checkIfNodeNameExists(name, this));
            
            //This version of setScriptName will not error if the name is invalid or already taken
            //and will not declare to python the node (because effect is not instanced yet)
            setScriptName_no_error_check(name);
            setLabel(args.serialization->getNodeLabel());
            nameSet = true;
        }
    }

    bool hasUsedFileDialog = false;
    bool canOpenFileDialog = args.reason == eCreateNodeReasonUserCreate && !args.serialization && args.paramValues.empty() && !isFileDialogPreviewReader;

    if (func.first) {
        /*
         We are creating a built-in plug-in
         */
        _imp->effect.reset(func.second(thisShared));
        assert(_imp->effect);
        _imp->effect->initializeData();
        
        createRotoContextConditionnally();
        initializeInputs();
        initializeKnobs(renderScaleSupportPreference);
        
        if (args.serialization) {
            loadKnobs(*args.serialization);
        }
        if (!args.paramValues.empty()) {
            setValuesFromSerialization(args.paramValues);
        }
        
        
        std::string images;
        if (_imp->effect->isReader() && canOpenFileDialog) {
            images = getApp()->openImageFileDialog();
        } else if (_imp->effect->isWriter() && canOpenFileDialog) {
            images = getApp()->saveImageFileDialog();
        }
        if (!images.empty()) {
            hasUsedFileDialog = true;
            boost::shared_ptr<KnobSerialization> defaultFile = createDefaultValueForParam(kOfxImageEffectFileParamName, images);
            CreateNodeArgs::DefaultValuesList list;
            list.push_back(defaultFile);
            setValuesFromSerialization(list);
        }
    } else {
            //ofx plugin   
        _imp->effect = appPTR->createOFXEffect(thisShared, args.serialization.get(),args.paramValues,canOpenFileDialog,renderScaleSupportPreference == 1, &hasUsedFileDialog);
            assert(_imp->effect);
    }
    
    _imp->effect->initializeOverlayInteract();
    
    _imp->effect->addSupportedBitDepth(&_imp->supportedDepths);
    
    if ( _imp->supportedDepths.empty() ) {
        //From the spec:
        //The default for a plugin is to have none set, the plugin \em must define at least one in its describe action.
        throw std::runtime_error("Plug-in does not support 8bits, 16bits or 32bits floating point image processing.");
    }
    
    /*
     Set modifiable props
     */
    refreshDynamicProperties();
    
    if (isTrackerNode()) {
        _imp->isMultiInstance = true;
    }
   
    
    if (!nameSet) {
        if (args.fixedName.isEmpty()) {
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
            try {
                group->initNodeName(isMultiInstanceChild ? args.multiInstanceParentName + '_' : pluginLabel.toStdString(),&name);
            } catch (...) {
                
            }
            setNameInternal(name.c_str(), false, true);
            nameSet = true;
        } else {
            try {
                setScriptName(args.fixedName.toStdString());
            } catch (...) {
                appPTR->writeToOfxLog_mt_safe("Could not set node name to " + args.fixedName);
            }
        }
        if (!isMultiInstanceChild && _imp->isMultiInstance) {
            updateEffectLabelKnob( getScriptName().c_str() );
        }
    } else { //nameSet
        //We have to declare the node to Python now since we didn't declare it before
        //with setScriptName_no_error_check
        declareNodeVariableToPython(getFullyQualifiedName());
    }
    if (isMultiInstanceChild && !args.serialization) {
        assert(nameSet);
        updateEffectLabelKnob(getScriptName().c_str());
    }
    if (args.addToProject) {
        declarePythonFields();
        if  (getRotoContext()) {
            declareRotoPythonField();
        }
    }
    
    if (group) {
        group->notifyNodeActivated(thisShared);
    }
    
    //This flag is used for the Roto plug-in and for the Merge inside the rotopaint tree
    //so that if the input of the roto node is RGB, it gets converted with alpha = 0, otherwise the user
    //won't be able to paint the alpha channel
    const QString& pluginID = _imp->plugin->getPluginID();
    if (isRotoPaintingNode() || pluginID == PLUGINID_OFX_ROTO) {
        _imp->useAlpha0ToConvertFromRGBToRGBA = true;
    }
    
    if (!args.serialization) {
        computeHash();
    }
    
    assert(_imp->effect);
    
    _imp->pluginSafety = _imp->effect->renderThreadSafety();
    _imp->currentThreadSafety = _imp->pluginSafety;
    
    _imp->nodeCreated = true;
    
    bool isLoadingPyPlug = getApp()->isCreatingPythonGroup();
    
    if (!getApp()->isCreatingNodeTree()) {
        refreshAllInputRelatedData(!args.serialization);
    }

    _imp->runOnNodeCreatedCB(!args.serialization && !isLoadingPyPlug);
    
    
    ///Now that the instance is created, make sure instanceChangedActino is called for all extra default values
    ///that we set
    double time = getEffectInstance()->getCurrentTime();
    for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = args.paramValues.begin(); it != args.paramValues.end(); ++it) {
        KnobPtr knob = getKnobByName((*it)->getName());
        if (knob) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                knob->evaluateValueChange(i, time, eValueChangedReasonUserEdited);
            }
        } else {
            qDebug() << "WARNING: No such parameter " << (*it)->getName().c_str();
        }
    }
    
    if (hasUsedFileDialog) {
        KnobPtr fileNameKnob = getKnobByName(kOfxImageEffectFileParamName);
        if (fileNameKnob) {
            fileNameKnob->evaluateValueChange(0, time, eValueChangedReasonUserEdited);
        }
    }
    
} // load

bool
Node::usesAlpha0ToConvertFromRGBToRGBA() const
{
    return _imp->useAlpha0ToConvertFromRGBToRGBA;
}

void
Node::setWhileCreatingPaintStroke(bool creating)
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);
    _imp->duringPaintStrokeCreation = creating;
}

bool
Node::isDuringPaintStrokeCreation() const
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);
    return _imp->duringPaintStrokeCreation;
}

void
Node::setRenderThreadSafety(RenderSafetyEnum safety)
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    _imp->currentThreadSafety = safety;
}

RenderSafetyEnum
Node::getCurrentRenderThreadSafety() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->currentThreadSafety;
}

void
Node::revertToPluginThreadSafety()
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    _imp->currentThreadSafety = _imp->pluginSafety;
}

void
Node::setCurrentOpenGLRenderSupport(PluginOpenGLRenderSupport support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    _imp->currentSupportOpenGLRender = support;
}

PluginOpenGLRenderSupport
Node::getCurrentOpenGLRenderSupport() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->currentSupportOpenGLRender;
}

void
Node::setCurrentSequentialRenderSupport(SequentialPreferenceEnum support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    _imp->currentSupportSequentialRender = support;
}

SequentialPreferenceEnum
Node::getCurrentSequentialRenderSupport() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->currentSupportSequentialRender;
}

void
Node::setCurrentSupportTiles(bool support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    _imp->currentSupportTiles = support;
}

bool
Node::getCurrentSupportTiles() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->currentSupportTiles;
}

void
Node::setCurrentCanTransform(bool support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    _imp->currentCanTransform = support;
}

bool
Node::getCurrentCanTransform() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->currentCanTransform;
}

void
Node::refreshDynamicProperties()
{
    setCurrentOpenGLRenderSupport(_imp->effect->supportsOpenGLRender());
    bool tilesSupported = _imp->effect->supportsTiles();
    bool multiResSupported = _imp->effect->supportsMultiResolution();
    bool canTransform = _imp->effect->getCanTransform();
    setCurrentSupportTiles(multiResSupported && tilesSupported);
    setCurrentSequentialRenderSupport(_imp->effect->getSequentialPreference());
    setCurrentCanTransform(canTransform);
}

void
Node::prepareForNextPaintStrokeRender()
{
    
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        _imp->strokeBitmapCleared = false;
    }
    _imp->effect->clearActionsCache();
}

void
Node::setLastPaintStrokeDataNoRotopaint()
{
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        _imp->strokeBitmapCleared = false;
        _imp->duringPaintStrokeCreation = true;
    }
    _imp->effect->setDuringPaintStrokeCreationThreadLocal(true);
}

void
Node::invalidateLastPaintStrokeDataNoRotopaint()
{
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        _imp->duringPaintStrokeCreation = false;
    }

}

RectD
Node::getPaintStrokeRoD_duringPainting() const
{
    return getApp()->getPaintStrokeWholeBbox();
}

void
Node::getPaintStrokeRoD(double time, RectD* bbox) const
{
    bool duringPaintStroke = _imp->effect->isDuringPaintStrokeCreationThreadLocal();
    QMutexLocker k(&_imp->lastStrokeMovementMutex);
    if (duringPaintStroke) {
        *bbox = getPaintStrokeRoD_duringPainting();
    } else {
        boost::shared_ptr<RotoDrawableItem> stroke = _imp->paintStroke.lock();
        if (!stroke) {
            throw std::logic_error("");
        }
        *bbox = stroke->getBoundingBox(time);
    }
    
}



bool
Node::isLastPaintStrokeBitmapCleared() const
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);
    return _imp->strokeBitmapCleared;
}

void
Node::clearLastPaintStrokeRoD()
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);
    _imp->strokeBitmapCleared = true;

}

void
Node::getLastPaintStrokePoints(double time,
                               std::list<std::list<std::pair<Point,double> > >* strokes,
                               int* strokeIndex) const
{
    bool duringPaintStroke;
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        duringPaintStroke = _imp->duringPaintStrokeCreation;
    }
    if (duringPaintStroke) {
        getApp()->getLastPaintStrokePoints(strokes, strokeIndex);
    } else {
        boost::shared_ptr<RotoDrawableItem> item = _imp->paintStroke.lock();
        RotoStrokeItem* stroke = dynamic_cast<RotoStrokeItem*>(item.get());
        assert(stroke);
        if (!stroke) {
            throw std::logic_error("");
        }
        stroke->evaluateStroke(0, time, strokes);
        *strokeIndex = 0;
    }
}


boost::shared_ptr<Image>
Node::getOrRenderLastStrokeImage(unsigned int mipMapLevel,
                                 const RectI& /*roi*/,
                                 double par,
                                 const ImageComponents& components,
                                 ImageBitDepthEnum depth) const
{
    
    QMutexLocker k(&_imp->lastStrokeMovementMutex);
    
    std::list<RectI> restToRender;
    boost::shared_ptr<RotoDrawableItem> item = _imp->paintStroke.lock();
    boost::shared_ptr<RotoStrokeItem> stroke = boost::dynamic_pointer_cast<RotoStrokeItem>(item);
    assert(stroke);
    if (!stroke) {
        throw std::logic_error("");
    }

   // qDebug() << getScriptName_mt_safe().c_str() << "Rendering stroke: " << _imp->lastStrokeMovementBbox.x1 << _imp->lastStrokeMovementBbox.y1 << _imp->lastStrokeMovementBbox.x2 << _imp->lastStrokeMovementBbox.y2;
    
    RectD lastStrokeBbox;
    std::list<std::pair<Point,double> > lastStrokePoints;
    double distNextIn;
    boost::shared_ptr<Image> strokeImage;
    getApp()->getRenderStrokeData(&lastStrokeBbox, &lastStrokePoints, &distNextIn, &strokeImage);
    double distToNextOut = stroke->renderSingleStroke(stroke, lastStrokeBbox, lastStrokePoints, mipMapLevel, par, components, depth, distNextIn, &strokeImage);

    getApp()->updateStrokeImage(strokeImage, distToNextOut, true);
    
    return strokeImage;
}

bool
Node::isNodeCreated() const
{
    return _imp->nodeCreated;
}

void
Node::setProcessChannelsValues(bool doR, bool doG, bool doB, bool doA)
{
    boost::shared_ptr<KnobBool> eR = _imp->enabledChan[0].lock();
    if (eR) {
        eR->setValue(doR, 0);
    }
    boost::shared_ptr<KnobBool> eG = _imp->enabledChan[1].lock();
    if (eG) {
        eG->setValue(doG, 0);
    }
    boost::shared_ptr<KnobBool> eB = _imp->enabledChan[2].lock();
    if (eB) {
        eB->setValue(doB, 0);
    }
    boost::shared_ptr<KnobBool> eA = _imp->enabledChan[3].lock();
    if (eA) {
        eA->setValue(doA, 0);
    }
}

void
Node::declareRotoPythonField()
{
    assert(_imp->rotoContext);
    std::string appID = getApp()->getAppIDString();
    std::string nodeName = getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = nodeFullName + ".roto = " + nodeFullName + ".getRotoContext()\n";
    if (!appPTR->isBackground()) {
        getApp()->printAutoDeclaredVariable(script);
    }
    bool ok = Python::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("Node::declareRotoPythonField(): interpretPythonScript("+script+") failed!");
    }
    _imp->rotoContext->declarePythonFields();
}

boost::shared_ptr<NodeCollection>
Node::getGroup() const
{
    return _imp->group.lock();
}

void
Node::Implementation::appendChild(const NodePtr& child)
{
    QMutexLocker k(&childrenMutex);
    for (NodesWList::iterator it = children.begin(); it != children.end(); ++it) {
        if (it->lock() == child) {
            return;
        }
    }
    children.push_back(child);
}

void
Node::fetchParentMultiInstancePointer()
{
    NodesList nodes = _imp->group.lock()->getNodes();
    
    NodePtr thisShared = shared_from_this();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
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

NodePtr
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
Node::getChildrenMultiInstance(NodesList* children) const
{
    QMutexLocker k(&_imp->childrenMutex);
    for (NodesWList::const_iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        children->push_back(it->lock());
    }
}

U64
Node::getHashValue() const
{
    QReadLocker l(&_imp->knobsAgeMutex);
    
    return _imp->hash.value();
}

std::string
Node::getCacheID() const
{
    QMutexLocker k(&_imp->nameMutex);
    return _imp->cacheID;
}

bool
Node::computeHashInternal()
{
    if (!_imp->effect) {
        return false;
    }
    ///Always called in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->inputsInitialized) {
        qDebug() << "Node::computeHash(): inputs not initialized";
    }
    
    U64 oldHash,newHash;
    {
        QWriteLocker l(&_imp->knobsAgeMutex);
        
        oldHash = _imp->hash.value();
        
        ///reset the hash value
        _imp->hash.reset();
        
        ///append the effect's own age
        _imp->hash.append(_imp->knobsAge);
        
        ///append all inputs hash
        boost::shared_ptr<RotoDrawableItem> attachedStroke = _imp->paintStroke.lock();
        NodePtr attachedStrokeContextNode;
        if (attachedStroke) {
            attachedStrokeContextNode = attachedStroke->getContext()->getNode();
        }
        {
            ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->effect.get());
            
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
                        
                        //Since the rotopaint node is connected to the internal nodes of the tree, don't change their hash
                        if (attachedStroke && input == attachedStrokeContextNode) {
                            continue;
                        }
                        ///Add the index of the input to its hash.
                        ///Explanation: if we didn't add this, just switching inputs would produce a similar
                        ///hash.
                        _imp->hash.append(input->getHashValue() + i);
                    }
                }
            }
        }
        
        // We do not append the roto age any longer since now every tool in the RotoContext is backed-up by nodes which
        // have their own age. Instead each action in the Rotocontext is followed by a incrementNodesAge() call so that each
        // node respecitively have their hash correctly set.
        
        //        boost::shared_ptr<RotoContext> roto = attachedStroke ? attachedStroke->getContext() : getRotoContext();
        //        if (roto) {
        //            U64 rotoAge = roto->getAge();
        //            _imp->hash.append(rotoAge);
        //        }
        
        ///Also append the effect's label to distinguish 2 instances with the same parameters
        Hash64_appendQString( &_imp->hash, QString( getScriptName().c_str() ) );
        
        ///Also append the project's creation time in the hash because 2 projects openend concurrently
        ///could reproduce the same (especially simple graphs like Viewer-Reader)
        qint64 creationTime =  getApp()->getProject()->getProjectCreationTime();
        _imp->hash.append(creationTime);
        
        _imp->hash.computeHash();
        
        newHash = _imp->hash.value();
        
    } // QWriteLocker l(&_imp->knobsAgeMutex);
    
    bool hashChanged = oldHash != newHash;

    if (hashChanged) {
        _imp->effect->onNodeHashChanged(newHash);
        if (_imp->nodeCreated && !getApp()->getProject()->isProjectClosing()) {
            /*
             * We changed the node hash. That means all cache entries for this node with a different hash
             * are impossible to re-create again. Just discard them all. This is done in a separate thread.
             */
            removeAllImagesFromCacheWithMatchingIDAndDifferentKey(newHash);
        }
    }

    return hashChanged;
}


void
Node::computeHashRecursive(std::list<Node*>& marked)
{
    if (std::find(marked.begin(), marked.end(), this) != marked.end()) {
        return;
    }
    
    bool hasChanged = computeHashInternal();
    marked.push_back(this);
    if (!hasChanged) {
        //Nothing changed, no need to recurse on outputs
        return;
    }
    
    
    bool isRotoPaint = _imp->effect->isRotoPaintNode();
    
    ///call it on all the outputs
    NodesList outputs;
    getOutputsWithGroupRedirection(outputs);
    for (NodesList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
        assert(*it);
        
        //Since the rotopaint node is connected to the internal nodes of the tree, don't change their hash
        boost::shared_ptr<RotoDrawableItem> attachedStroke = (*it)->getAttachedRotoItem();
        if (isRotoPaint && attachedStroke && attachedStroke->getContext()->getNode().get() == this) {
            continue;
        }
        (*it)->computeHashRecursive(marked);
    }
    
    
    ///If the node has a rotopaint tree, compute the hash of the nodes in the tree
    if (_imp->rotoContext) {
        NodesList allItems;
        _imp->rotoContext->getRotoPaintTreeNodes(&allItems);
        for (NodesList::iterator it = allItems.begin(); it!=allItems.end(); ++it) {
            (*it)->computeHashRecursive(marked);
        }
        
    }
}

void
Node::removeAllImagesFromCacheWithMatchingIDAndDifferentKey(U64 nodeHashKey)
{
    boost::shared_ptr<Project> proj = getApp()->getProject();
    if (proj->isProjectClosing() || proj->isLoadingProject()) {
        return;
    }
    appPTR->removeAllImagesFromCacheWithMatchingIDAndDifferentKey(this, nodeHashKey);
    appPTR->removeAllImagesFromDiskCacheWithMatchingIDAndDifferentKey(this, nodeHashKey);
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->effect.get());
    if (isViewer) {
        //Also remove from viewer cache
        appPTR->removeAllTexturesFromCacheWithMatchingIDAndDifferentKey(this, nodeHashKey);
    }
}

void
Node::removeAllImagesFromCache(bool blocking)
{
    boost::shared_ptr<Project> proj = getApp()->getProject();
    if (proj->isProjectClosing() || proj->isLoadingProject()) {
        return;
    }
    appPTR->removeAllCacheEntriesForHolder(this, blocking);
}

void
Node::doComputeHashOnMainThread()
{
    assert(QThread::currentThread() == qApp->thread());
    computeHash();
}

void
Node::computeHash()
{
    if (QThread::currentThread() != qApp->thread()) {
        Q_EMIT mustComputeHashOnMainThread();
        return;
    }
    std::list<Node*> marked;
    computeHashRecursive(marked);
    
} // computeHash

void
Node::setValuesFromSerialization(const std::list<boost::shared_ptr<KnobSerialization> >& paramValues)
{
    
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->knobsInitialized);
    
    const std::vector< KnobPtr > & nodeKnobs = getKnobs();
    ///for all knobs of the node
    for (U32 j = 0; j < nodeKnobs.size(); ++j) {
        ///try to find a serialized value for this knob
        for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = paramValues.begin(); it != paramValues.end(); ++it) {
            if ( (*it)->getName() == nodeKnobs[j]->getName() ) {
                KnobPtr serializedKnob = (*it)->getKnob();
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
        _imp->createdComponents = serialization.getUserCreatedComponents();
    }
    
    const std::vector< KnobPtr > & nodeKnobs = getKnobs();
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
    
    _imp->effect->onKnobsLoaded();
}

void
Node::loadKnob(const KnobPtr & knob,
               const std::list< boost::shared_ptr<KnobSerialization> > & knobsValues,bool updateKnobGui)
{
    
    ///try to find a serialized value for this knob
    bool found = false;
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        if ( (*it)->getName() == knob->getName() ) {
            found = true;
            // don't load the value if the Knob is not persistant! (it is just the default value in this case)
            ///EDIT: Allow non persistent params to be loaded if we found a valid serialization for them
            //if ( knob->getIsPersistant() ) {
            KnobPtr serializedKnob = (*it)->getKnob();
            
            KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob.get());
            if (isChoice) {
                const TypeExtraData* extraData = (*it)->getExtraData();
                const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>(extraData);
                assert(choiceData);
                
                KnobChoice* choiceSerialized = dynamic_cast<KnobChoice*>(serializedKnob.get());
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
    if (!found) {
        ///Hack for old RGBA checkboxes which have a different name now
        bool isR = knob->getName() == "r";
        bool isG = knob->getName() == "g";
        bool isB = knob->getName() == "b";
        bool isA = knob->getName() == "a";
        if (isR || isG || isB || isA) {
            for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
                if ((isR && (*it)->getName() == kNatronOfxParamProcessR) ||
                    (isG && (*it)->getName() == kNatronOfxParamProcessG) ||
                    (isB && (*it)->getName() == kNatronOfxParamProcessB) ||
                    (isA && (*it)->getName() == kNatronOfxParamProcessA)) {
                    KnobPtr serializedKnob = (*it)->getKnob();
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
            }
        }
    }
}

void
Node::Implementation::restoreKnobLinksRecursive(const GroupKnobSerialization* group,
                                                const NodesList & allNodes,
                                                const std::map<std::string,std::string>& oldNewScriptNamesMapping)
{
    const std::list <boost::shared_ptr<KnobSerializationBase> >&  children = group->getChildren();
    for (std::list <boost::shared_ptr<KnobSerializationBase> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>(it->get());
        KnobSerialization* isRegular = dynamic_cast<KnobSerialization*>(it->get());
        assert(isGrp || isRegular);
        if (isGrp) {
            restoreKnobLinksRecursive(isGrp,allNodes, oldNewScriptNamesMapping);
        } else if (isRegular) {
            KnobPtr knob =  _publicInterface->getKnobByName( isRegular->getName() );
            if (!knob) {
                QString err = _publicInterface->getScriptName_mt_safe().c_str();
                err.append(QObject::tr(": Could not find a parameter named ") );
                err.append(QString((*it)->getName().c_str()));
                appPTR->writeToOfxLog_mt_safe(err);
                continue;
            }
            isRegular->restoreKnobLinks(knob,allNodes, oldNewScriptNamesMapping);
            isRegular->restoreExpressions(knob, oldNewScriptNamesMapping);
            isRegular->restoreTracks(knob,allNodes);

        }
    }
}

void
Node::restoreKnobsLinks(const NodeSerialization & serialization,
                        const NodesList & allNodes,
                        const std::map<std::string,std::string>& oldNewScriptNamesMapping)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    const NodeSerialization::KnobValues & knobsValues = serialization.getKnobsValues();
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        KnobPtr knob = getKnobByName( (*it)->getName() );
        if (!knob) {
            QString err = getScriptName_mt_safe().c_str();
            err.append(QObject::tr(": Could not find a parameter named ") );
            err.append(QString((*it)->getName().c_str()));
            appPTR->writeToOfxLog_mt_safe(err);
            continue;
        }
        (*it)->restoreKnobLinks(knob,allNodes, oldNewScriptNamesMapping);
        (*it)->restoreExpressions(knob,oldNewScriptNamesMapping);
        (*it)->restoreTracks(knob,allNodes);
      
    }
    
    const std::list<boost::shared_ptr<GroupKnobSerialization> >& userKnobs = serialization.getUserPages();
    for (std::list<boost::shared_ptr<GroupKnobSerialization > >::const_iterator it = userKnobs.begin(); it != userKnobs.end(); ++it) {
        _imp->restoreKnobLinksRecursive((*it).get(), allNodes, oldNewScriptNamesMapping);
    }
    
}

void
Node::setPagesOrder(const std::list<std::string>& pages)
{
    //re-order the pages
    std::list<KnobPtr > pagesOrdered;
    
    for (std::list<std::string>::const_iterator it = pages.begin(); it!=pages.end();++it) {
        const KnobsVec &knobs = getKnobs();
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ((*it2)->getName() == *it) {
                pagesOrdered.push_back(*it2);
                _imp->effect->removeKnobFromList(it2->get());
                break;
            }
        }
    }
    int index = 0;
    for (std::list<KnobPtr >::iterator it=  pagesOrdered.begin() ;it!=pagesOrdered.end(); ++it,++index) {
        _imp->effect->insertKnob(index, *it);
    }
}

std::list<std::string>
Node::getPagesOrder() const
{
    const KnobsVec& knobs = getKnobs();
    std::list<std::string> ret;
    for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        KnobPage* ispage = dynamic_cast<KnobPage*>(it->get());
        if (ispage) {
            ret.push_back(ispage->getName());
        }
    }
    return ret;
}

void
Node::restoreUserKnobs(const NodeSerialization& serialization)
{
    const std::list<boost::shared_ptr<GroupKnobSerialization> >& userPages = serialization.getUserPages();
    
    for (std::list<boost::shared_ptr<GroupKnobSerialization> >::const_iterator it = userPages.begin() ; it != userPages.end(); ++it) {
        KnobPtr found = getKnobByName((*it)->getName());
        boost::shared_ptr<KnobPage> page;
        if (!found) {
            page = AppManager::createKnob<KnobPage>(_imp->effect.get(), (*it)->getLabel() , 1, false);
            page->setAsUserKnob();
            page->setName((*it)->getName());
            
        } else {
            page = boost::dynamic_pointer_cast<KnobPage>(found);
        }
        if (page) {
            _imp->restoreUserKnobsRecursive((*it)->getChildren(), boost::shared_ptr<KnobGroup>(), page);
        }
    }
    setPagesOrder(serialization.getPagesOrdered());

}

void
Node::Implementation::restoreUserKnobsRecursive(const std::list<boost::shared_ptr<KnobSerializationBase> >& knobs,
                                                const boost::shared_ptr<KnobGroup>& group,
                                                const boost::shared_ptr<KnobPage>& page)
{
    for (std::list<boost::shared_ptr<KnobSerializationBase> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>(it->get());
        KnobSerialization* isRegular = dynamic_cast<KnobSerialization*>(it->get());
        assert(isGrp || isRegular);
        
        KnobPtr found = _publicInterface->getKnobByName((*it)->getName());
        
        if (isGrp) {
            boost::shared_ptr<KnobGroup> grp;
            if (!found) {
                grp = AppManager::createKnob<KnobGroup>(effect.get(), isGrp->getLabel() , 1, false);
            } else {
                grp = boost::dynamic_pointer_cast<KnobGroup>(found);
                if (!grp) {
                    continue;
                }
            }
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
            KnobPtr sKnob = isRegular->getKnob();
            KnobPtr knob;
            KnobInt* isInt = dynamic_cast<KnobInt*>(sKnob.get());
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>(sKnob.get());
            KnobBool* isBool = dynamic_cast<KnobBool*>(sKnob.get());
            KnobChoice* isChoice = dynamic_cast<KnobChoice*>(sKnob.get());
            KnobColor* isColor = dynamic_cast<KnobColor*>(sKnob.get());
            KnobString* isStr = dynamic_cast<KnobString*>(sKnob.get());
            KnobFile* isFile = dynamic_cast<KnobFile*>(sKnob.get());
            KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>(sKnob.get());
            KnobPath* isPath = dynamic_cast<KnobPath*>(sKnob.get());
            KnobButton* isBtn = dynamic_cast<KnobButton*>(sKnob.get());
            KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(sKnob.get());
            
            assert(isInt || isDbl || isBool || isChoice || isColor || isStr || isFile || isOutFile || isPath || isBtn || isSep);
            
            if (isInt) {
                boost::shared_ptr<KnobInt> k;
                
                if (!found) {
                    k = AppManager::createKnob<KnobInt>(effect.get(), isRegular->getLabel() ,
                                                                             sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobInt>(found);
                    if (!k) {
                        continue;
                    }
                }
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
                boost::shared_ptr<KnobDouble> k;
                if (!found) {
                    k = AppManager::createKnob<KnobDouble>(effect.get(), isRegular->getLabel() ,
                                                        sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobDouble>(found);
                    if (!k) {
                        continue;
                    }
                }
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
                    KnobDouble* isDbl = dynamic_cast<KnobDouble*>(knob.get());
                    if (isDbl) {
                        isDbl->setHasHostOverlayHandle(true);
                    }
                }
                
            } else if (isBool) {
                boost::shared_ptr<KnobBool> k;
                if (!found) {
                    k = AppManager::createKnob<KnobBool>(effect.get(), isRegular->getLabel() ,
                                                      sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobBool>(found);
                    if (!k) {
                        continue;
                    }
                }
                knob = k;
            } else if (isChoice) {
                boost::shared_ptr<KnobChoice> k;
                if (!found) {
                    k = AppManager::createKnob<KnobChoice>(effect.get(), isRegular->getLabel() ,
                                                                               sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobChoice>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>(isRegular->getExtraData());
                assert(data);
                k->populateChoices(data->_entries,data->_helpStrings);
                knob = k;
            } else if (isColor) {
                boost::shared_ptr<KnobColor> k;
                if (!found) {
                    k = AppManager::createKnob<KnobColor>(effect.get(), isRegular->getLabel() ,
                                                       sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobColor>(found);
                    if (!k) {
                        continue;
                    }
                }
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
                boost::shared_ptr<KnobString> k;
                if (!found) {
                    k = AppManager::createKnob<KnobString>(effect.get(), isRegular->getLabel() ,
                                                        sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobString>(found);
                    if (!k) {
                        continue;
                    }
                }
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
                boost::shared_ptr<KnobFile> k;
                if (!found) {
                    k = AppManager::createKnob<KnobFile>(effect.get(), isRegular->getLabel() ,
                                                      sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobFile>(found);
                    if (!k) {
                        continue;
                    }
                }
                const FileExtraData* data = dynamic_cast<const FileExtraData*>(isRegular->getExtraData());
                assert(data);
                if (data->useSequences) {
                    k->setAsInputImage();
                }
                knob = k;
                
            } else if (isOutFile) {
                boost::shared_ptr<KnobOutputFile> k;
                if (!found) {
                    k = AppManager::createKnob<KnobOutputFile>(effect.get(), isRegular->getLabel() ,
                                                            sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobOutputFile>(found);
                    if (!k) {
                        continue;
                    }
                }
                const FileExtraData* data = dynamic_cast<const FileExtraData*>(isRegular->getExtraData());
                assert(data);
                if (data->useSequences) {
                    k->setAsOutputImageFile();
                }
                knob = k;
            } else if (isPath) {
                boost::shared_ptr<KnobPath> k;
                if (!found) {
                    k = AppManager::createKnob<KnobPath>(effect.get(), isRegular->getLabel() ,
                                                                                           sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobPath>(found);
                    if (!k) {
                        continue;
                    }
                }
                const PathExtraData* data = dynamic_cast<const PathExtraData*>(isRegular->getExtraData());
                assert(data);
                if (data->multiPath) {
                    k->setMultiPath(true);
                }
                knob = k;
            } else if (isBtn) {
                boost::shared_ptr<KnobButton> k;
                if (!found) {
                    k = AppManager::createKnob<KnobButton>(effect.get(), isRegular->getLabel() ,
                                                                               sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobButton>(found);
                    if (!k) {
                        continue;
                    }
                }
                knob = k;
            } else if (isSep) {
                boost::shared_ptr<KnobSeparator> k;
                if (!found) {
                    k = AppManager::createKnob<KnobSeparator>(effect.get(), isRegular->getLabel() ,
                                                       sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobSeparator>(found);
                    if (!k) {
                        continue;
                    }
                }
                knob = k;
            }
            
            assert(knob);
            if (!knob) {
                continue;
            }
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
    
    bool changed;
    {
        QWriteLocker l(&_imp->knobsAgeMutex);
        changed = _imp->knobsAge != newAge || !_imp->hash.value();
        if (changed) {
            _imp->knobsAge = newAge;
        }
    }
    if (changed) {
        Q_EMIT knobsAgeChanged(newAge);
        computeHash();
    }
}

void
Node::incrementKnobsAge_internal()
{
    {
        QWriteLocker l(&_imp->knobsAgeMutex);
        ++_imp->knobsAge;
        
        ///if the age of an effect somehow reaches the maximum age (will never happen)
        ///handle it by clearing the cache and resetting the age to 0.
        if ( _imp->knobsAge == std::numeric_limits<U64>::max() ) {
            appPTR->clearAllCaches();
            _imp->knobsAge = 0;
        }
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
    if (!_imp->effect) {
        return false;
    }
    
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        if (nodeGui->hasHostOverlay()) {
            return true;
        }
    }

    return _imp->effect->hasOverlay();
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
        assert(!mustQuitPreview);
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
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( getEffectInstance().get() );
    
    if (isOutput) {
        isOutput->getRenderEngine()->abortRendering(false,true);
    }
    _imp->abortPreview();
}

void
Node::setMustQuitProcessing(bool mustQuit)
{
    
    {
        QMutexLocker k(&_imp->mustQuitProcessingMutex);
        _imp->mustQuitProcessing = mustQuit;
    }
    if (getRotoContext()) {
        NodesList rotopaintNodes;
        getRotoContext()->getRotoPaintTreeNodes(&rotopaintNodes);
        for (NodesList::iterator it = rotopaintNodes.begin(); it!=rotopaintNodes.end(); ++it) {
            (*it)->setMustQuitProcessing(mustQuit);
        }
    }
    //Attempt to wake-up a sleeping thread
    QMutexLocker k(&_imp->nodeIsDequeuingMutex);
    _imp->nodeIsDequeuingCond.wakeAll();
}

void
Node::quitAnyProcessing()
{
    {
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        if (_imp->nodeIsDequeuing) {
            _imp->nodeIsDequeuing = false;
            _imp->nodeIsDequeuingCond.wakeOne();
        }
    }
    
    
    //If this effect has a RenderEngine, make sure it is finished
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>(_imp->effect.get());
    if (isOutput) {
        isOutput->getRenderEngine()->quitEngine();
    }
    
    //Returns when the preview is done computign
    _imp->abortPreview();
    
    if (isRotoPaintingNode()) {
        NodesList rotopaintNodes;
        getRotoContext()->getRotoPaintTreeNodes(&rotopaintNodes);
        for (NodesList::iterator it = rotopaintNodes.begin(); it!=rotopaintNodes.end(); ++it) {
            (*it)->quitAnyProcessing();
        }
    }
    
}

Node::~Node()
{
    destroyNodeInternal(true, false);
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

const NodesWList &
Node::getOutputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->outputs;
}

const NodesWList &
Node::getGuiOutputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->guiOutputs;
}

void
Node::getOutputs_mt_safe(NodesWList& outputs) const
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
    
    QMutexLocker l(&_imp->inputsLabelsMutex);
    assert(_imp->inputs.size() == _imp->inputLabels.size());
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            inputNames.insert(std::make_pair(_imp->inputLabels[i], input->getScriptName_mt_safe()) );
        }
    }
}

int
Node::getPreferredInputInternal(bool connected) const
{
    
    int nInputs = getMaxInputCount();
    if (nInputs == 0) {
        return -1;
    }
    std::vector<NodePtr> inputs(nInputs);
    std::vector<std::string> inputLabels(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        inputLabels[i] = getInputLabel(i);
    }
    
    {
        ///Find an input named Source
        std::string inputNameToFind(kOfxImageEffectSimpleSourceClipName);
        for (int i = 0; i < nInputs ; ++i) {
            if (inputLabels[i] == inputNameToFind) {
                inputs[i] = getInput(i);
                if ((connected && inputs[i]) || (!connected && !inputs[i])) {
                    return i;
                }
            }
        }
    }
    
    bool useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();
    
    ///Find an input named A
    std::string inputNameToFind,otherName;
    if (useInputA || getPluginID() == PLUGINID_OFX_SHUFFLE) {
        inputNameToFind = "A";
        otherName = "B";
    } else {
        inputNameToFind = "B";
        otherName = "A";
    }
    int foundOther = -1;
    for (int i = 0; i < nInputs ; ++i) {
        if (inputLabels[i] == inputNameToFind ) {
            inputs[i] = getInput(i);
            if ((connected && inputs[i]) || (!connected && !inputs[i])) {
                return i;
            }
        } else if (inputLabels[i] == otherName) {
            foundOther = i;
        }
    }
    if (foundOther != -1) {
        inputs[foundOther] = getInput(foundOther);
        if ((connected && inputs[foundOther]) || (!connected && !inputs[foundOther])) {
            return foundOther;
        }
    }
    
    
    for (int i = 0; i < nInputs; ++i) {
        if (!inputs[i]) {
            inputs[i] = getInput(i);
        }
    }

    
    ///we return the first non-optional empty input
    int firstNonOptionalEmptyInput = -1;
    std::list<int> optionalEmptyInputs;
    std::list<int> optionalEmptyMasks;
    
    for (int i = 0; i < nInputs; ++i) {
        if (_imp->effect->isInputRotoBrush(i)) {
            continue;
        }
        if ((connected && inputs[i]) || (!connected && !inputs[i])) {
            if ( !_imp->effect->isInputOptional(i) ) {
                if (firstNonOptionalEmptyInput == -1) {
                    firstNonOptionalEmptyInput = i;
                    break;
                }
            } else {
                if (_imp->effect->isInputMask(i)) {
                    optionalEmptyMasks.push_back(i);
                } else {
                    optionalEmptyInputs.push_back(i);
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
            while ( first != optionalEmptyInputs.rend() && _imp->effect->isInputRotoBrush(*first) ) {
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
Node::getOutputsConnectedToThisNode(std::map<NodePtr,int>* outputs)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    NodePtr thisSHared = shared_from_this();
    for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
        
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        
        int indexOfThis = output->inputIndex(thisSHared);
        assert(indexOfThis != -1);
        if (indexOfThis >= 0) {
            outputs->insert( std::make_pair(output, indexOfThis) );
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

static void prependGroupNameRecursive(const NodePtr& group,std::string& name)
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
Node::getFullyQualifiedNameInternal(const std::string& scriptName) const
{
    std::string ret = scriptName;
    NodePtr parent = getParentMultiInstance();
    if (parent) {
        prependGroupNameRecursive(parent, ret);
    } else {
        boost::shared_ptr<NodeCollection> hasParentGroup = getGroup();
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>(hasParentGroup.get());
        if (isGrp) {
            prependGroupNameRecursive(isGrp->getNode(), ret);
        }
    }
    return ret;

}

std::string
Node::getFullyQualifiedName() const
{
    return getFullyQualifiedNameInternal(getScriptName_mt_safe());
}

void
Node::setLabel(const std::string& label)
{
    assert(QThread::currentThread() == qApp->thread());
    if (dynamic_cast<GroupOutput*>(_imp->effect.get())) {
        return ;
    }
    
    {
        QMutexLocker k(&_imp->nameMutex);
        if (label == _imp->label) {
            return;
        }
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
    setNameInternal(name, false, false);
}



static void insertDependenciesRecursive(Node* node, KnobI::ListenerDimsMap* dependencies)
{
    const KnobsVec & knobs = node->getKnobs();
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobI::ListenerDimsMap dimDeps;
        knobs[i]->getListeners(dimDeps);
        for (KnobI::ListenerDimsMap::iterator it = dimDeps.begin(); it!=dimDeps.end();++it) {
            
            KnobI::ListenerDimsMap::iterator found = dependencies->find(it->first);
            if (found != dependencies->end()) {
                assert(found->second.size() == it->second.size());
                for (std::size_t j = 0; j < found->second.size(); ++j) {
                    if (it->second[j].isExpr) {
                        found->second[j].isListening |= it->second[j].isListening;
                    }
                }
            } else {
                dependencies->insert(*it);
            }
        }
        
    }
    
    NodeGroup* isGroup = node->isEffectGroup();
    if (isGroup) {
        NodesList nodes = isGroup->getNodes();
        for (NodesList::iterator it = nodes.begin(); it!=nodes.end(); ++it) {
            insertDependenciesRecursive(it->get(), dependencies);
        }
    }
}

void
Node::setNameInternal(const std::string& name, bool throwErrors, bool declareToPython)
{
    std::string oldName = getScriptName_mt_safe();
    std::string fullOldName = getFullyQualifiedName();
    std::string newName = name;
    
    boost::shared_ptr<NodeCollection> collection = getGroup();
    if (collection) {
        if (throwErrors) {
            try {
                collection->checkNodeName(this, name,false, false, &newName);
            } catch (const std::exception& e) {
                appPTR->writeToOfxLog_mt_safe(e.what());
                std::cerr << e.what() << std::endl;
                return;
            }
        } else {
            collection->checkNodeName(this, name,false, false, &newName);
        }
    }
    
    
    
    if (oldName == newName) {
        return;
    }


    if (!newName.empty()) {
        bool isAttrDefined = false;
        std::string newPotentialQualifiedName = getApp()->getAppIDString() + "." + getFullyQualifiedNameInternal(newName);
        (void)Python::getAttrRecursive(newPotentialQualifiedName, appPTR->getMainModule(), &isAttrDefined);
        if (isAttrDefined) {
            std::stringstream ss;
            ss << "A Python attribute with the same name (" << newPotentialQualifiedName << ") already exists.";
            if (throwErrors) {
                throw std::runtime_error(ss.str());
            } else {
                std::string err = ss.str();
                appPTR->writeToOfxLog_mt_safe(err.c_str());
                std::cerr << err << std::endl;
                return;
            }
        }
    }
    
    bool mustSetCacheID;
    {
        QMutexLocker l(&_imp->nameMutex);
        _imp->scriptName = newName;
        mustSetCacheID = _imp->cacheID.empty();
        ///Set the label at the same time if the label is empty
        if (_imp->label.empty()) {
            _imp->label = newName;
        }
    }
    std::string fullySpecifiedName = getFullyQualifiedName();

    if (mustSetCacheID) {
        std::string baseName = fullySpecifiedName;
        std::string cacheID = fullySpecifiedName;
        
        
        int i = 1;
        while (getGroup() && getGroup()->isCacheIDAlreadyTaken(cacheID)) {
            std::stringstream ss;
            ss << baseName;
            ss << i;
            cacheID = ss.str();
            ++i;
        }
        QMutexLocker l(&_imp->nameMutex);
        _imp->cacheID = cacheID;
    }
    
    if (declareToPython && collection) {
        if (!oldName.empty()) {
            if (fullOldName != fullySpecifiedName) {
                try {
                    setNodeVariableToPython(fullOldName,fullySpecifiedName);
                } catch (const std::exception& e) {
                    qDebug() << e.what();
                }
            }
        } else { //if (!oldName.empty()) {
            declareNodeVariableToPython(fullySpecifiedName);
        }
        
        
        ///For all knobs that have listeners, change in the expressions of listeners this knob script-name
        KnobI::ListenerDimsMap dependencies;
        insertDependenciesRecursive(this, &dependencies);
        for (KnobI::ListenerDimsMap::iterator it = dependencies.begin(); it!=dependencies.end(); ++it) {
            KnobPtr listener = it->first.lock();
            if (!listener) {
                continue;
            }
            for (std::size_t d = 0; d < it->second.size(); ++d) {
                if (it->second[d].isListening && it->second[d].isExpr) {
                    listener->replaceNodeNameInExpression(d,oldName,newName);
                }
            }
        }
    }
    
    QString qnewName(newName.c_str());
    Q_EMIT scriptNameChanged(qnewName);
    Q_EMIT labelChanged(qnewName);
}

void
Node::setScriptName(const std::string& name)
{
    std::string newName;
    if (getGroup()) {
        getGroup()->checkNodeName(this, name,false, true, &newName);
    } else {
        newName = name;
    }
    //We do not allow setting the script-name of output nodes because we rely on it with NatronRenderer
    if (dynamic_cast<GroupOutput*>(_imp->effect.get())) {
        throw std::runtime_error(QObject::tr("Changing the script-name of an Output node is not a valid operation").toStdString());
        return;
    }
    
    
    setNameInternal(newName, true, true);
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
Node::makeCacheInfo() const
{
    std::size_t ram,disk;
    appPTR->getMemoryStatsForCacheEntryHolder(this, &ram, &disk);
    QString ramSizeStr = printAsRAM((U64)ram);
    QString diskSizeStr = printAsRAM((U64)disk);
    
    std::stringstream ss;
    ss << "<br><b><font color=\"green\">Cache Occupancy:</font></b> RAM: " << ramSizeStr.toStdString() << " / Disk: " << diskSizeStr.toStdString() << "</br>";
    return ss.str();
}

std::string
Node::makeInfoForInput(int inputNumber) const
{
    if (inputNumber < -1 || inputNumber >= getMaxInputCount()) {
        return "";
    }
    const Node* inputNode = 0;
    std::string inputName ;
    if (inputNumber != -1) {
        inputNode = getInput(inputNumber).get();
        inputName = getInputLabel(inputNumber);
    } else {
        inputNode = this;
        inputName = "Output";
    }

    if (!inputNode) {
        return "";
    }
    
    std::list<ImageComponents> comps;
    ImageBitDepthEnum depth;
    ImagePremultiplicationEnum premult;

    EffectInstPtr input = inputNode->getEffectInstance();
    double par = input->getPreferredAspectRatio();
    premult = input->getOutputPremultiplication();
    std::string premultStr;
    switch (premult) {
        case eImagePremultiplicationOpaque:
            premultStr = "opaque";
            break;
        case eImagePremultiplicationPremultiplied:
            premultStr= "premultiplied";
            break;
        case eImagePremultiplicationUnPremultiplied:
            premultStr= "unpremultiplied";
            break;
    }

    _imp->effect->getPreferredDepthAndComponents(inputNumber, &comps, &depth);
    assert(!comps.empty());

    double time = getApp()->getTimeLine()->currentFrame();

    EffectInstance::ComponentsAvailableMap availableComps;
    input->getComponentsAvailable(true, true, time, &availableComps);
    
    RenderScale scale(1.);
    RectD rod;
    bool isProjectFormat;
    StatusEnum stat = input->getRegionOfDefinition_public(getHashValue(),
                                                          time,
                                                          scale, 0, &rod, &isProjectFormat);
    
    double fps = input->getPreferredFrameRate();

    std::stringstream ss;
    ss << "<br><b><font color=\"orange\">"<< inputName << ":" << "</font></b></br>";
    ss << "<br><b>Image Format: </b>";
    
    EffectInstance::ComponentsAvailableMap::iterator next = availableComps.begin();
    if (next != availableComps.end()) {
        ++next;
    }
    for (EffectInstance::ComponentsAvailableMap::iterator it = availableComps.begin(); it != availableComps.end(); ++it) {
        NodePtr origin = it->second.lock();
        if (origin.get() != this || inputNumber == -1) {
            ss << Image::getFormatString(it->first, depth);
            if (origin) {
                ss << ": (" << origin->getLabel_mt_safe() << ")";
            }
        }
        if (next != availableComps.end()) {
            if (origin.get() != this || inputNumber == -1) {
                ss << "</br>";
            }
            ++next;
        }
    }
    
    ss << "<br><b>Alpha premultiplication: </b>" << premultStr << "</br>";
    ss << "<br><b>Pixel aspect ratio: </b>" << par << "</br>";
    ss << "<br><b>Framerate:</b> " << fps << "</br>";
    if (stat != eStatusFailed) {
        ss << "<br><b>Region of Definition:</b> ";
        ss << "left = " << rod.x1 << " bottom = " << rod.y1 << " right = " << rod.x2 << " top = " << rod.y2 << "</br>";
    }
    return ss.str();
}

void
Node::initializeKnobs(int renderScaleSupportPref)
{
    ////Only called by the main-thread
    
    
    
    _imp->effect->beginChanges();
    
    assert( QThread::currentThread() == qApp->thread() );
    assert(!_imp->knobsInitialized);
    
    Backdrop* isBd = dynamic_cast<Backdrop*>(_imp->effect.get());
    Dot* isDot = dynamic_cast<Dot*>(_imp->effect.get());
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->effect.get());
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->effect.get());
    DiskCacheNode* isDiskCache = dynamic_cast<DiskCacheNode*>(_imp->effect.get());
    
    ///For groups, declare the plugin knobs after the node knobs because we want to use the Node page
    if (!isGroup) {
        _imp->effect->initializeKnobsPublic();
    }
    
    InitializeKnobsFlag_RAII __isInitializingKnobsFlag__(_imp->effect.get());

    ///If the effect has a mask, add additionnal mask controls
    int inputsCount = getMaxInputCount();
    
    if (!isDot && !isViewer) {
        
        if (!isBd) {
            
            bool isWriter = _imp->effect->isWriter();
        
            
            bool disableNatronKnobs = _imp->effect->isReader() || isWriter || _imp->effect->isTrackerNode() || dynamic_cast<NodeGroup*>(_imp->effect.get()) || dynamic_cast<GroupInput*>(_imp->effect.get()) ||
            dynamic_cast<GroupOutput*>(_imp->effect.get()) || dynamic_cast<PrecompNode*>(_imp->effect.get());
            
            bool useChannels = !_imp->effect->isMultiPlanar() && !disableNatronKnobs && !isDiskCache;
            
            
            ///find in all knobs a page param to set this param into
            boost::shared_ptr<KnobPage> mainPage;
            const KnobsVec & knobs = _imp->effect->getKnobs();
            
            
            {
                ///Try to find a format param and hijack it to handle it ourselves with the project's formats
                KnobPtr formatKnob;
                for (std::size_t i = 0; i < knobs.size(); ++i) {
                    if (knobs[i]->getName() == kNatronParamFormatChoice) {
                        formatKnob = knobs[i];
                        break;
                    }
                }
                if (formatKnob) {
                    KnobPtr formatSize;
                    for (std::size_t i = 0; i < knobs.size(); ++i) {
                        if (knobs[i]->getName() == kNatronParamFormatSize) {
                            formatSize = knobs[i];
                            break;
                        }
                    }
                    KnobPtr formatPar;
                    for (std::size_t i = 0; i < knobs.size(); ++i) {
                        if (knobs[i]->getName() == kNatronParamFormatPar) {
                            formatPar = knobs[i];
                            break;
                        }
                    }
                    if (formatSize && formatPar) {
                        _imp->pluginFormatKnobs.formatChoice = boost::dynamic_pointer_cast<KnobChoice>(formatKnob);
                        formatSize->setEvaluateOnChange(false);
                        formatPar->setEvaluateOnChange(false);
                        formatSize->setSecret(true);
                        formatSize->setSecretByDefault(true);
                        formatPar->setSecret(true);
                        formatPar->setSecretByDefault(true);
                        _imp->pluginFormatKnobs.size = boost::dynamic_pointer_cast<KnobInt>(formatSize);
                        _imp->pluginFormatKnobs.par = boost::dynamic_pointer_cast<KnobDouble>(formatPar);
                        
                        std::vector<std::string> formats;
                        int defValue;
                        getApp()->getProject()->getProjectFormatEntries(&formats, &defValue);
                        refreshFormatParamChoice(formats, defValue);
                    }
                }
            }
            
            if (!disableNatronKnobs || isWriter) {
                for (std::size_t i = 0; i < knobs.size(); ++i) {
                    boost::shared_ptr<KnobPage> p = boost::dynamic_pointer_cast<KnobPage>(knobs[i]);
                    if ( p && (p->getLabel() != NATRON_PARAMETER_PAGE_NAME_INFO) &&
                        (p->getLabel() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
                        mainPage = p;
                        break;
                    }
                }
                if (!mainPage) {
                    mainPage = AppManager::createKnob<KnobPage>(_imp->effect.get(), "Settings");
                }
                assert(mainPage);
            }
            
            
            if (isWriter) {
                ///Find a  "lastFrame" parameter and add it after it
                boost::shared_ptr<KnobInt> frameIncrKnob = AppManager::createKnob<KnobInt>(_imp->effect.get(), kWriteParamFrameStepLabel, 1 , false);
                frameIncrKnob->setName(kWriteParamFrameStep);
                frameIncrKnob->setHintToolTip(kWriteParamFrameStepHint);
                frameIncrKnob->setAnimationEnabled(false);
                frameIncrKnob->setMinimum(1);
                frameIncrKnob->setDefaultValue(1);
                if (mainPage) {
                    std::vector< KnobPtr > children = mainPage->getChildren();
                    bool foundLastFrame = false;
                    for (std::size_t i = 0; i < children.size(); ++i) {
                        if (children[i]->getName() == "lastFrame") {
                            mainPage->insertKnob(i + 1, frameIncrKnob);
                            foundLastFrame = true;
                            break;
                        }
                    }
                    if (!foundLastFrame) {
                        mainPage->addKnob(frameIncrKnob);
                    }
                }
                _imp->frameIncrKnob = frameIncrKnob;
            }
            
           
            
            ///Pair hasMaskChannelSelector, isMask
            std::vector<std::pair<bool,bool> > hasMaskChannelSelector(inputsCount);
            std::vector<std::string> inputLabels(inputsCount);
            for (int i = 0; i < inputsCount; ++i) {
                inputLabels[i] = _imp->effect->getInputLabel(i);
                
                assert(i < (int)_imp->inputsComponents.size());
                const std::list<ImageComponents>& inputSupportedComps = _imp->inputsComponents[i];
                
                bool isMask = _imp->effect->isInputMask(i);
                bool supportsOnlyAlpha = inputSupportedComps.size() == 1 && inputSupportedComps.front().getNumComponents() == 1;
                
                hasMaskChannelSelector[i].first = false;
                hasMaskChannelSelector[i].second = isMask;
                
                if ((isMask || supportsOnlyAlpha) &&
                    !_imp->effect->isInputRotoBrush(i) ) {
                    hasMaskChannelSelector[i].first = true;
                }
            }
      
           
            if (useChannels) {
                
                
                
                bool useSelectors = !dynamic_cast<RotoPaint*>(_imp->effect.get());
                
                if (useSelectors) {
                    
                    KnobsVec mainPageChildren = mainPage->getChildren();
                    bool skipSeparator = !mainPageChildren.empty() && dynamic_cast<KnobSeparator*>(mainPageChildren.back().get());

                    if (skipSeparator) {
                        boost::shared_ptr<KnobSeparator> sep = AppManager::createKnob<KnobSeparator>(_imp->effect.get(), "Advanced", 1, false);
                        mainPage->addKnob(sep);
                    }
                    
                    ///Create input layer selectors
                    for (int i = 0; i < inputsCount; ++i) {
                        if (!hasMaskChannelSelector[i].first) {
                            _imp->createChannelSelector(i,inputLabels[i], false, mainPage);
                        }
                        
                    }
                    ///Create output layer selectors
                    _imp->createChannelSelector(-1, "Output", true, mainPage);
                }
                

                
                //Try to find R,G,B,A parameters on the plug-in, if found, use them, otherwise create them
                std::string channelLabels[4] = {kNatronOfxParamProcessRLabel, kNatronOfxParamProcessGLabel, kNatronOfxParamProcessBLabel, kNatronOfxParamProcessALabel};
                std::string channelNames[4] = {kNatronOfxParamProcessR, kNatronOfxParamProcessG, kNatronOfxParamProcessB, kNatronOfxParamProcessA};
                std::string channelHints[4] = {kNatronOfxParamProcessRHint, kNatronOfxParamProcessGHint, kNatronOfxParamProcessBHint, kNatronOfxParamProcessAHint};
                
                
                bool pluginDefaultPref[4];
                bool useRGBACheckbox = _imp->effect->isHostChannelSelectorSupported(&pluginDefaultPref[0], &pluginDefaultPref[1], &pluginDefaultPref[2], &pluginDefaultPref[3]);
                boost::shared_ptr<KnobBool> foundEnabled[4];
                for (int i = 0; i < 4; ++i) {
                    boost::shared_ptr<KnobBool> enabled;
                    for (std::size_t j = 0; j < knobs.size(); ++j) {
                        if (knobs[j]->getOriginalName() == channelNames[i]) {
                            foundEnabled[i] = boost::dynamic_pointer_cast<KnobBool>(knobs[j]);
                            break;
                        }
                    }
                }
                
                if (foundEnabled[0] && foundEnabled[1] && foundEnabled[2] && foundEnabled[3]) {
                    
                    for (int i = 0; i < 4; ++i) {
                        if (foundEnabled[i]->getParentKnob() == mainPage) {
                            //foundEnabled[i]->setAddNewLine(i == 3);
                            mainPage->removeKnob(foundEnabled[i].get());
                            mainPage->insertKnob(i,foundEnabled[i]);
                        }
                        _imp->enabledChan[i] = foundEnabled[i];
                    }
                }
#ifdef DEBUG
                if (foundEnabled[0] && foundEnabled[1] && foundEnabled[2] && foundEnabled[3] && useRGBACheckbox) {
                    qDebug() << getScriptName_mt_safe().c_str() << "WARNING: property" << kNatronOfxImageEffectPropChannelSelector << "was not set to" << kOfxImageComponentNone << "but plug-in uses its own checkboxes";
                }
#endif
                if (useRGBACheckbox && (!foundEnabled[0] || !foundEnabled[1] || !foundEnabled[2] || !foundEnabled[3])) {
                    for (int i = 0; i < 4; ++i) {
                        foundEnabled[i] =  AppManager::createKnob<KnobBool>(_imp->effect.get(), channelLabels[i], 1, false);
                        foundEnabled[i]->setName(channelNames[i]);
                        foundEnabled[i]->setAnimationEnabled(false);
                        foundEnabled[i]->setAddNewLine(i == 3);
                        foundEnabled[i]->setDefaultValue(pluginDefaultPref[i]);
                        foundEnabled[i]->setHintToolTip(channelHints[i]);
                        mainPage->insertKnob(i,foundEnabled[i]);
                        _imp->enabledChan[i] = foundEnabled[i];
                    }
                }
                
            } // useChannels
            
            ///Find in the plug-in the Mask/Mix related parameter to re-order them so it is consistent across nodes
            std::vector<std::pair<std::string,KnobPtr > > foundPluginDefaultKnobsToReorder;
            foundPluginDefaultKnobsToReorder.push_back(std::make_pair(kOfxMaskInvertParamName, KnobPtr()));
            foundPluginDefaultKnobsToReorder.push_back(std::make_pair(kOfxMixParamName, KnobPtr()));
            if (mainPage) {
                ///Insert auto-added knobs before mask invert if found
                for (std::size_t i = 0; i < knobs.size(); ++i) {
                    for (std::size_t j = 0; j < foundPluginDefaultKnobsToReorder.size(); ++j) {
                        if (knobs[i]->getName() == foundPluginDefaultKnobsToReorder[j].first) {
                            foundPluginDefaultKnobsToReorder[j].second = knobs[i];
                        }
                    }
                }
            }
            
            ///Create mask selectors
            for (int i = 0; i < inputsCount; ++i) {
                
                if (!hasMaskChannelSelector[i].first) {
                    continue;
                }
                
                
                MaskSelector sel;
                boost::shared_ptr<KnobBool> enabled = AppManager::createKnob<KnobBool>(_imp->effect.get(), inputLabels[i],1,false);
                
                enabled->setDefaultValue(false, 0);
                enabled->setAddNewLine(false);
                if (hasMaskChannelSelector[i].second) {
                    std::string enableMaskName(std::string(kEnableMaskKnobName) + "_" + inputLabels[i]);
                    enabled->setName(enableMaskName);
                    enabled->setHintToolTip(tr("Enable the mask to come from the channel named by the choice parameter on the right. "
                                               "Turning this off will act as though the mask was disconnected.").toStdString());
                } else {
                    std::string enableMaskName(std::string(kEnableInputKnobName) + "_" + inputLabels[i]);
                    enabled->setName(enableMaskName);
                    enabled->setHintToolTip(tr("Enable the image to come from the channel named by the choice parameter on the right. "
                                               "Turning this off will act as though the input was disconnected.").toStdString());
                }
                enabled->setAnimationEnabled(false);
                if (mainPage) {
                    mainPage->addKnob(enabled);
                }
                
                
                sel.enabled = enabled;
                
                boost::shared_ptr<KnobChoice> channel = AppManager::createKnob<KnobChoice>(_imp->effect.get(), "",1,false);
                
                std::vector<std::string> choices;
                choices.push_back("None");
                /*const ImageComponents& rgba = ImageComponents::getRGBAComponents();
                const std::vector<std::string>& channels = rgba.getComponentsNames();
                const std::string& layerName = rgba.getComponentsGlobalName();
                for (std::size_t c = 0; c < channels.size(); ++c) {
                    choices.push_back(layerName + "." + channels[c]);
                }*/
                
                channel->populateChoices(choices);
                channel->setDefaultValue(choices.size() - 1, 0);
                channel->setAnimationEnabled(false);
                channel->setHintToolTip(tr("Use this channel from the original input to mix the output with the original input. "
                                           "Setting this to None is the same as disconnecting the input.").toStdString());
                if (hasMaskChannelSelector[i].second) {
                    std::string channelMaskName(std::string(kMaskChannelKnobName) + "_" + inputLabels[i]);
                    channel->setName(channelMaskName);
                } else {
                    std::string channelMaskName(std::string(kInputChannelKnobName) + "_" + inputLabels[i]);
                    channel->setName(channelMaskName);
                }
                sel.channel = channel;
                channel->setAddNewLine(false);
                if (mainPage) {
                    mainPage->addKnob(channel);
                }
                
                boost::shared_ptr<KnobString> channelName = AppManager::createKnob<KnobString>(_imp->effect.get(), "",1,false);
                channelName->setSecretByDefault(true);
                channelName->setEvaluateOnChange(false);
                channelName->setDefaultValue(choices[choices.size() - 1]);
                if (mainPage) {
                    mainPage->addKnob(channelName);
                }
                sel.channelName = channelName;
                
                //Make sure the first default param in the vector is MaskInvert
                assert(foundPluginDefaultKnobsToReorder.size() > 0 && foundPluginDefaultKnobsToReorder[0].first == kOfxMaskInvertParamName);
                if (foundPluginDefaultKnobsToReorder[0].second) {
                    //If there is a MaskInvert parameter, make it on the same line as the Mask channel parameter
                    channelName->setAddNewLine(false);
                }
                
                
                _imp->maskSelectors[i] = sel;
                
            } // for (int i = 0; i < inputsCount; ++i) {
    
            //Create the host mix if needed
            if (!disableNatronKnobs && _imp->effect->isHostMixingEnabled()) {
                boost::shared_ptr<KnobDouble> mixKnob = AppManager::createKnob<KnobDouble>(_imp->effect.get(), "Mix", 1, false);
                mixKnob->setName("hostMix");
                mixKnob->setHintToolTip("Mix between the source image at 0 and the full effect at 1.");
                mixKnob->setMinimum(0.);
                mixKnob->setMaximum(1.);
                mixKnob->setDefaultValue(1.);
                if (mainPage) {
                    mainPage->addKnob(mixKnob);
                }
                _imp->mixWithSource = mixKnob;
            }
            
            
            /*
             * Reposition the MaskInvert and Mix parameters declared by the plug-in
             */
            if (mainPage) {
                for (std::size_t i = 0; i < foundPluginDefaultKnobsToReorder.size(); ++i) {
                    if (foundPluginDefaultKnobsToReorder[i].second && foundPluginDefaultKnobsToReorder[i].second->getParentKnob() == mainPage) {
                        mainPage->removeKnob(foundPluginDefaultKnobsToReorder[i].second.get());
                        mainPage->addKnob(foundPluginDefaultKnobsToReorder[i].second);
                    }
                }
            }
            
        } // !isBd
        
        _imp->nodeSettingsPage = AppManager::createKnob<KnobPage>(_imp->effect.get(), NATRON_PARAMETER_PAGE_NAME_EXTRA,1,false);

        boost::shared_ptr<KnobString> nodeLabel = AppManager::createKnob<KnobString>(_imp->effect.get(),
                                                              isBd ? tr("Name label").toStdString() : tr("Label").toStdString(),1,false);
        assert(nodeLabel);
        nodeLabel->setName(kUserLabelKnobName);
        nodeLabel->setAnimationEnabled(false);
        nodeLabel->setEvaluateOnChange(false);
        nodeLabel->setAsMultiLine();
        nodeLabel->setUsesRichText(true);
        nodeLabel->setHintToolTip(tr("This label gets appended to the node name on the node graph.").toStdString());
        _imp->nodeSettingsPage.lock()->addKnob(nodeLabel);
        _imp->nodeLabelKnob = nodeLabel;
        
        if (!isBd) {
            
            boost::shared_ptr<KnobBool> hideInputs = AppManager::createKnob<KnobBool>(_imp->effect.get(), "Hide inputs", 1, false);
            hideInputs->setName("hideInputs");
            hideInputs->setDefaultValue(false);
            hideInputs->setAnimationEnabled(false);
            hideInputs->setAddNewLine(false);
            hideInputs->setIsPersistant(true);
            hideInputs->setEvaluateOnChange(false);
            hideInputs->setHintToolTip(tr("When checked, the input arrows of the node in the nodegraph will be hidden").toStdString());
            _imp->hideInputs = hideInputs;
            _imp->nodeSettingsPage.lock()->addKnob(hideInputs);

            
            boost::shared_ptr<KnobBool> fCaching = AppManager::createKnob<KnobBool>(_imp->effect.get(), "Force caching", 1, false);
            fCaching->setName("forceCaching");
            fCaching->setDefaultValue(false);
            fCaching->setAnimationEnabled(false);
            fCaching->setAddNewLine(false);
            fCaching->setIsPersistant(true);
            fCaching->setEvaluateOnChange(false);
            fCaching->setHintToolTip(tr("When checked, the output of this node will always be kept in the RAM cache for fast access of already computed "
                                                  "images.").toStdString());
            _imp->forceCaching = fCaching;
            _imp->nodeSettingsPage.lock()->addKnob(fCaching);
            
            boost::shared_ptr<KnobBool> previewEnabled = AppManager::createKnob<KnobBool>(_imp->effect.get(), tr("Preview").toStdString(),1,false);
            assert(previewEnabled);
            previewEnabled->setDefaultValue( makePreviewByDefault() );
            previewEnabled->setName(kEnablePreviewKnobName);
            previewEnabled->setAnimationEnabled(false);
            previewEnabled->setAddNewLine(false);
            previewEnabled->setIsPersistant(false);
            previewEnabled->setEvaluateOnChange(false);
            previewEnabled->setHintToolTip(tr("Whether to show a preview on the node box in the node-graph.").toStdString());
            _imp->nodeSettingsPage.lock()->addKnob(previewEnabled);
            _imp->previewEnabledKnob = previewEnabled;
            
            boost::shared_ptr<KnobBool> disableNodeKnob = AppManager::createKnob<KnobBool>(_imp->effect.get(), "Disable",1,false);
            assert(disableNodeKnob);
            disableNodeKnob->setAnimationEnabled(false);
            disableNodeKnob->setDefaultValue(false);
            disableNodeKnob->setIsClipPreferencesSlave(true);
            disableNodeKnob->setName(kDisableNodeKnobName);
            disableNodeKnob->setAddNewLine(false);
            disableNodeKnob->setHintToolTip("When disabled, this node acts as a pass through.");
            _imp->nodeSettingsPage.lock()->addKnob(disableNodeKnob);
            _imp->disableNodeKnob = disableNodeKnob;
            
            boost::shared_ptr<KnobBool> useFullScaleImagesWhenRenderScaleUnsupported = AppManager::createKnob<KnobBool>(_imp->effect.get(), tr("Render high def. upstream").toStdString(),1,false);
            useFullScaleImagesWhenRenderScaleUnsupported->setAnimationEnabled(false);
            useFullScaleImagesWhenRenderScaleUnsupported->setDefaultValue(false);
            useFullScaleImagesWhenRenderScaleUnsupported->setName("highDefUpstream");
            useFullScaleImagesWhenRenderScaleUnsupported->setHintToolTip(tr("This node doesn't support rendering images at a scale lower than 1, it "
                                                                                  "can only render high definition images. When checked this parameter controls "
                                                                                  "whether the rest of the graph upstream should be rendered with a high quality too or at "
                                                                                  "the most optimal resolution for the current viewer's viewport. Typically checking this "
                                                                                  "means that an image will be slow to be rendered, but once rendered it will stick in the cache "
                                                                                  "whichever zoom level you're using on the Viewer, whereas when unchecked it will be much "
                                                                                  "faster to render but will have to be recomputed when zooming in/out in the Viewer.").toStdString());
            if (renderScaleSupportPref == 0 && getEffectInstance()->supportsRenderScaleMaybe() == EffectInstance::eSupportsYes) {
                useFullScaleImagesWhenRenderScaleUnsupported->setSecretByDefault(true);
            }
            _imp->nodeSettingsPage.lock()->addKnob(useFullScaleImagesWhenRenderScaleUnsupported);
            _imp->useFullScaleImagesWhenRenderScaleUnsupported = useFullScaleImagesWhenRenderScaleUnsupported;
            
            boost::shared_ptr<KnobString> knobChangedCallback = AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After param changed callback").toStdString());
            knobChangedCallback->setHintToolTip(tr("Set here the name of a function defined in Python which will be called for each  "
                                                         "parameter change. Either define this function in the Script Editor "
                                                         "or in the init.py script or even in the script of a Python group plug-in.\n"
                                                         "The signature of the callback is: callback(thisParam, thisNode, thisGroup, app, userEdited) where:\n"
                                                         "- thisParam: The parameter which just had its value changed\n"
                                                         "- userEdited: A boolean informing whether the change was due to user interaction or "
                                                         "because something internally triggered the change.\n"
                                                         "- thisNode: The node holding the parameter\n"
                                                         "- app: points to the current application instance\n"
                                                         "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)").toStdString());
            knobChangedCallback->setAnimationEnabled(false);
            knobChangedCallback->setName("onParamChanged");
            _imp->nodeSettingsPage.lock()->addKnob(knobChangedCallback);
            _imp->knobChangedCallback = knobChangedCallback;
            
            boost::shared_ptr<KnobString> inputChangedCallback = AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After input changed callback").toStdString());
            inputChangedCallback->setHintToolTip(tr("Set here the name of a function defined in Python which will be called after "
                                                          "each connection is changed for the inputs of the node. "
                                                          "Either define this function in the Script Editor "
                                                          "or in the init.py script or even in the script of a Python group plug-in.\n"
                                                          "The signature of the callback is: callback(inputIndex, thisNode, thisGroup, app):\n"
                                                          "- inputIndex: the index of the input which changed, you can query the node "
                                                          "connected to the input by calling the getInput(...) function.\n"
                                                          "- thisNode: The node holding the parameter\n"
                                                          "- app: points to the current application instance\n"
                                                          "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)").toStdString());
            
            inputChangedCallback->setAnimationEnabled(false);
            inputChangedCallback->setName("onInputChanged");
            _imp->nodeSettingsPage.lock()->addKnob(inputChangedCallback);
            _imp->inputChangedCallback = inputChangedCallback;
            
            if (isGroup) {
                boost::shared_ptr<KnobString> onNodeCreated = AppManager::createKnob<KnobString>(_imp->effect.get(), "After Node Created");
                onNodeCreated->setName("afterNodeCreated");
                onNodeCreated->setHintToolTip("Add here the name of a Python-defined function that will be called each time a node "
                                              "is created in the group. This will be called in addition to the After Node Created "
                                              " callback of the project for the group node and all nodes within it (not recursively).\n"
                                              "The boolean variable userEdited will be set to True if the node was created "
                                              "by the user or False otherwise (such as when loading a project, or pasting a node).\n"
                                              "The signature of the callback is: callback(thisNode, app, userEdited) where:\n"
                                              "- thisNode: the node which has just been created\n"
                                              "- userEdited: a boolean indicating whether the node was created by user interaction or from "
                                              "a script/project load/copy-paste\n"
                                              "- app: points to the current application instance\n");
                onNodeCreated->setAnimationEnabled(false);
                _imp->nodeCreatedCallback = onNodeCreated;
                _imp->nodeSettingsPage.lock()->addKnob(onNodeCreated);
                
                boost::shared_ptr<KnobString> onNodeDeleted = AppManager::createKnob<KnobString>(_imp->effect.get(), "Before Node Removal");
                onNodeDeleted->setName("beforeNodeRemoval");
                onNodeDeleted->setHintToolTip("Add here the name of a Python-defined function that will be called each time a node "
                                              "is about to be deleted. This will be called in addition to the Before Node Removal "
                                              " callback of the project for the group node and all nodes within it (not recursively).\n"
                                              "This function will not be called when the project is closing.\n"
                                              "The signature of the callback is: callback(thisNode, app) where:\n"
                                              "- thisNode: the node about to be deleted\n"
                                              "- app: points to the current application instance\n");
                onNodeDeleted->setAnimationEnabled(false);
                _imp->nodeRemovalCallback = onNodeDeleted;
                _imp->nodeSettingsPage.lock()->addKnob(onNodeDeleted);
            }
            
            
            boost::shared_ptr<KnobPage> infoPage = AppManager::createKnob<KnobPage>(_imp->effect.get(), tr("Info").toStdString(), 1, false);
            infoPage->setName(NATRON_PARAMETER_PAGE_NAME_INFO);
            _imp->infoPage = infoPage;
            
            boost::shared_ptr<KnobString> nodeInfos = AppManager::createKnob<KnobString>(_imp->effect.get(), "", 1, false);
            nodeInfos->setName("nodeInfos");
            nodeInfos->setAnimationEnabled(false);
            nodeInfos->setIsPersistant(false);
            nodeInfos->setAsMultiLine();
            nodeInfos->setAsCustomHTMLText(true);
            nodeInfos->setEvaluateOnChange(false);
            nodeInfos->setHintToolTip(tr("Input and output informations, press Refresh to update them with current values").toStdString());
            infoPage->addKnob(nodeInfos);
            _imp->nodeInfos = nodeInfos;
            
            
            boost::shared_ptr<KnobButton> refreshInfoButton = AppManager::createKnob<KnobButton>(_imp->effect.get(), tr("Refresh Info").toStdString(),1,false);
            refreshInfoButton->setName("refreshButton");
            refreshInfoButton->setEvaluateOnChange(false);
            infoPage->addKnob(refreshInfoButton);
            _imp->refreshInfoButton = refreshInfoButton;
            
            if (_imp->effect->isWriter()) {
                boost::shared_ptr<KnobPage> pythonPage = AppManager::createKnob<KnobPage>(_imp->effect.get(), tr("Python").toStdString(),1,false);
                
                boost::shared_ptr<KnobString> beforeFrameRender =  AppManager::createKnob<KnobString>(_imp->effect.get(), tr("Before frame render").toStdString(), 1 ,false);
                beforeFrameRender->setName("beforeFrameRender");
                beforeFrameRender->setAnimationEnabled(false);
                beforeFrameRender->setHintToolTip(tr("Add here the name of a Python defined function that will be called before rendering "
                                                           "any frame.\n "
                                                           "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                                           "- frame: the frame to be rendered\n"
                                                           "- thisNode: points to the writer node\n"
                                                           "- app: points to the current application instance").toStdString());
                pythonPage->addKnob(beforeFrameRender);
                _imp->beforeFrameRender = beforeFrameRender;
                
                boost::shared_ptr<KnobString> beforeRender =  AppManager::createKnob<KnobString>(_imp->effect.get(), tr("Before render").toStdString(),1,false);
                beforeRender->setName("beforeRender");
                beforeRender->setAnimationEnabled(false);
                beforeRender->setHintToolTip(tr("Add here the name of a Python defined function that will be called once when "
                                                      "starting rendering.\n "
                                                      "The signature of the callback is: callback(thisNode, app) where:\n"
                                                      "- thisNode: points to the writer node\n"
                                                      "- app: points to the current application instance").toStdString());
                pythonPage->addKnob(beforeRender);
                _imp->beforeRender = beforeRender;
                
                boost::shared_ptr<KnobString> afterFrameRender =  AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After frame render").toStdString(),1,false);
                afterFrameRender->setName("afterFrameRender");
                afterFrameRender->setAnimationEnabled(false);
                afterFrameRender->setHintToolTip(tr("Add here the name of a Python defined function that will be called after rendering "
                                                          "any frame.\n "
                                                          "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                                          "- frame: the frame that has been rendered\n"
                                                          "- thisNode: points to the writer node\n"
                                                          "- app: points to the current application instance").toStdString());
                pythonPage->addKnob(afterFrameRender);
                _imp->afterFrameRender = afterFrameRender;
                
                boost::shared_ptr<KnobString> afterRender =  AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After render").toStdString(),1,false);
                afterRender->setName("afterRender");
                afterRender->setAnimationEnabled(false);
                afterRender->setHintToolTip(tr("Add here the name of a Python defined function that will be called once when the rendering "
                                                     "is finished.\n "
                                                     "The signature of the callback is: callback(aborted, thisNode, app) where:\n"
                                                     "- aborted: True if the render ended because it was aborted, False upon completion\n"
                                                     "- thisNode: points to the writer node\n"
                                                     "- app: points to the current application instance").toStdString());
                pythonPage->addKnob(afterRender);
                _imp->afterRender = afterRender;
            }
        }
    }


    if (isGroup) {
        _imp->effect->initializeKnobsPublic();
    }
    
    _imp->knobsInitialized = true;

    _imp->effect->endChanges();
    Q_EMIT knobsInitialized();
} // initializeKnobs

void
Node::Implementation::createChannelSelector(int inputNb,const std::string & inputName,bool isOutput,
                                            const boost::shared_ptr<KnobPage>& page)
{
    
    ChannelSelector sel;
    sel.hasAllChoice = isOutput;
    boost::shared_ptr<KnobChoice> layer = AppManager::createKnob<KnobChoice>(effect.get(), isOutput ? "Output Layer" : inputName + " Layer", 1, false);
    layer->setHostCanAddOptions(isOutput);
    if (!isOutput) {
        layer->setName(inputName + std::string("_") + std::string(kOutputChannelsKnobName));
    } else {
        layer->setName(kOutputChannelsKnobName);
    }
    if (isOutput) {
        layer->setHintToolTip("Select here the layer onto which the processing should occur.");
    } else {
        layer->setHintToolTip("Select here the layer that will be used in input by " + inputName);
    }
    layer->setAnimationEnabled(false);
    layer->setSecretByDefault(!isOutput);
    page->addKnob(layer);
    sel.layer = layer;
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
    layer->populateChoices(baseLayers);
    int defVal;
    if (isOutput && effect->isPassThroughForNonRenderedPlanes() == EffectInstance::ePassThroughRenderAllRequestedPlanes) {
        defVal = 0;
        
        //Hide all other input selectors if choice is All in output
        for (std::map<int,ChannelSelector>::iterator it = channelsSelectors.begin(); it!=channelsSelectors.end(); ++it) {
            it->second.layer.lock()->setSecret(true);
        }
    } else {
        defVal = 1;
    }
    layer->setDefaultValue(defVal);
    
    boost::shared_ptr<KnobString> layerName = AppManager::createKnob<KnobString>(effect.get(), inputName + "_layer_name", 1, false);
    layerName->setSecretByDefault(true);
    layerName->setAnimationEnabled(false);
    layerName->setEvaluateOnChange(false);
    layerName->setDefaultValue(baseLayers[defVal]);
    //layerName->setAddNewLine(!sel.useRGBASelectors);
    page->addKnob(layerName);
    sel.layerName = layerName;
    
    channelsSelectors[inputNb] = sel;
    
}

int
Node::getFrameStepKnobValue() const
{
    boost::shared_ptr<KnobInt> k = _imp->frameIncrKnob.lock();
    if (!k) {
        return 1;
    } else {
        int v = k->getValue();
        return std::max(1, v);
    }
}

bool
Node::handleFormatKnob(KnobI* knob)
{
    boost::shared_ptr<KnobChoice> choice = _imp->pluginFormatKnobs.formatChoice.lock();
    if (!choice) {
        return false;
    }
    if (knob != choice.get()) {
        return false;
    }
    int curIndex = choice->getValue();
    Format f;
    if (!getApp()->getProject()->getProjectFormatAtIndex(curIndex, &f)) {
        assert(false);
        return true;
    }
    
    boost::shared_ptr<KnobInt> size = _imp->pluginFormatKnobs.size.lock();
    boost::shared_ptr<KnobDouble> par = _imp->pluginFormatKnobs.par.lock();
    assert(size && par);
    
    _imp->effect->beginChanges();
    size->setValues(f.width(), f.height(), Natron::eValueChangedReasonNatronInternalEdited);
    par->setValue(f.getPixelAspectRatio(),0);
    _imp->effect->endChanges();
    return true;
}

void
Node::refreshFormatParamChoice(const std::vector<std::string>& entries, int defValue)
{
    boost::shared_ptr<KnobChoice> choice = _imp->pluginFormatKnobs.formatChoice.lock();
    if (!choice) {
        return;
    }
    int curIndex = choice->getValue();
    choice->populateChoices(entries);
    choice->beginChanges();
    choice->setDefaultValue(defValue);
    if (curIndex < (int)entries.size()) {
        choice->setValue(curIndex,0);
    }
    choice->endChanges();
}

QString
Node::makeHTMLDocumentation() const
{
    assert(QThread::currentThread() == qApp->thread());
    
    QString ret;
    QTextStream ts(&ret);
    
    bool isPyPlug;
    QString pluginID,pluginLabel,pluginDescription;
    int majorVersion = getMajorVersion();
    int minorVersion = getMinorVersion();
    
    {
        QMutexLocker k(&_imp->pluginPythonModuleMutex);
        isPyPlug = !_imp->pyPlugID.empty();
        pluginID = _imp->pyPlugID.empty() ? _imp->plugin->getPluginID() : _imp->pyPlugID.c_str();
        pluginLabel = _imp->pyPlugLabel.empty() ? _imp->plugin->getPluginLabel() : _imp->pyPlugLabel.c_str();
        pluginDescription = _imp->pyPlugDesc.empty() ? _imp->effect->getPluginDescription().c_str() : _imp->pyPlugDesc.c_str();
    }
    
    ts << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">";
    ts << "<html>";
    ts << "<head>";
    ts << "<title>" << pluginLabel << "</title>";
    
    ///Stylesheet
    ts << "<style>";
    ts << "body {margin:0;padding:0;}";
    ts << "h3 {font-weight:normal;font-size:xx-large;}";
    ts << "p {text-align:justify;}";
    ts << "table.myTable {width:100%;}";
    ts << "td.myTableValue {background-color:#d3d3d3;}";
    ts << "td.myTableHeader";
    ts << "{font-weight:bold;color:white;background-color:#000;text-align:center;}";
    ts << "div#myFooter {text-align:center;margin:10px;font-size:small;}";
    ts << "div#myHeader {height:150px;background:black;}";
    ts << "div#myHeaderLeft";
    ts << "{height:150px;width:150px;float:left;background:url(natron.png) no-repeat #fff;}";
    ts << "div#myHeaderLeft span {display:none;}";
    ts << "div#myHeaderRight {height:150px;width:200px;float:right;background:green;}";
    ts << "div#myContainer {margin-left:300px;margin-right:30px;}";
    ts << "div#myLeftMenu {float:left;width:250px;background:yellow;}";
    ts << "div#myleftMenu h4 {font-weight:normal;margin-left:5px;}";
    ts << "</style>";
    ts << "</head>";
    
    ts << "<body>";
    ts << "<h3>" << pluginLabel << " version " << majorVersion << "." << minorVersion << "</h3>";
    ts << "<p>" << pluginDescription << "</p>";
    ts << "<h3>" << "Inputs & Controls" << "</h3>";
    
    ts << "<table class=\"knobsTable\">";
    ts << "<td class=\"knobsTableHeader\">Label (UI Name)</td>";
    ts << "<td class=\"knobsTableHeader\">Script-Name</td>";
    ts << "<td class=\"knobsTableHeader\">Default-Value</td>";
    ts << "<td class=\"knobsTableHeader\">Function</td>";
    
    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        
        ts << "<tr>";

        if ((*it)->getDefaultIsSecret()) {
            continue;
        }
        QString knobScriptName((*it)->getName().c_str());
        QString knobLabel((*it)->getLabel().c_str());
        QString knobHint((*it)->getHintToolTip().c_str());
        
        ts << "<td class=\"knobsTableValue\">" << knobLabel << "</td>";
        ts << "<td class=\"knobsTableValue\">" << knobScriptName << "</td>";
        
        QString defValuesStr;

        std::vector<std::pair<QString,QString> > dimsDefaultValueStr;
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(it->get());
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(it->get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(it->get());
        Knob<double>* isDbl = dynamic_cast<Knob<double>*>(it->get());
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(it->get());
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(it->get());
        KnobButton* isBtn = dynamic_cast<KnobButton*>(it->get());
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>(it->get());
        KnobGroup* isGroup = dynamic_cast<KnobGroup*>(it->get());
        KnobPage* isPage = dynamic_cast<KnobPage*>(it->get());
        
        if (!isGroup && !isPage) {
            for (int i = 0; i < (*it)->getDimension(); ++i) {
                QString valueStr;
                
                if (!isBtn && !isSep && !isParametric) {
                    if (isChoice) {
                        int index = isChoice->getDefaultValue(i);
                        std::vector<std::string> entries = isChoice->getEntries_mt_safe();
                        if (index >= 0 && index < (int)entries.size()) {
                            valueStr = entries[index].c_str();
                        }
                    } else if (isInt) {
                        valueStr = QString::number(isInt->getDefaultValue(i));
                    } else if (isDbl) {
                        valueStr = QString::number(isDbl->getDefaultValue(i));
                    } else if (isBool) {
                        valueStr = isBool->getDefaultValue(i) ? "On" : "Off";
                    } else if (isString) {
                        valueStr = isString->getDefaultValue(i).c_str();
                    }
                }
                
                dimsDefaultValueStr.push_back(std::make_pair((*it)->getDimensionName(i).c_str(), valueStr));
            }
            
            for (std::size_t i = 0; i < dimsDefaultValueStr.size(); ++i) {
                if (!dimsDefaultValueStr[i].second.isEmpty()) {
                    if (dimsDefaultValueStr.size() > 1) {
                        defValuesStr.append(dimsDefaultValueStr[i].first);
                        defValuesStr.append(": ");
                    }
                    defValuesStr.append(dimsDefaultValueStr[i].second);
                    if (i < dimsDefaultValueStr.size() -1) {
                        defValuesStr.append(" ");
                    }
                }
            }
            if (defValuesStr.isEmpty()) {
                defValuesStr = "N/A";
            }
        }
        
        
      
        
        ts << "<td class=\"knobsTableValue\">" << defValuesStr << "</td>";
        ts << "<td class=\"knobsTableValue\">" << knobHint << "</td>";

        
        ts << "</tr>";

    } // for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
    
    ts << "</table>";

    //ts << "<div id=\"myFooter\">&copy; 2016 foobar</div>";
    
    ts << "</body>";
    ts << "</html>";
    return ret;
}

bool
Node::isForceCachingEnabled() const
{
    return _imp->forceCaching.lock()->getValue();
}

void
Node::onSetSupportRenderScaleMaybeSet(int support)
{
    if ((EffectInstance::SupportsEnum)support == EffectInstance::eSupportsYes) {
        boost::shared_ptr<KnobBool> b = _imp->useFullScaleImagesWhenRenderScaleUnsupported.lock();
        if (b) {
            b->setSecretByDefault(true);
            b->setSecret(true);
        }
    }
}

bool
Node::useScaleOneImagesWhenRenderScaleSupportIsDisabled() const
{
    return _imp->useFullScaleImagesWhenRenderScaleUnsupported.lock()->getValue();
}

void
Node::beginEditKnobs()
{
    _imp->effect->beginEditKnobs();
}


void
Node::setEffect(const EffectInstPtr& effect)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    _imp->effect = effect;
    _imp->effect->initializeData();
}

EffectInstPtr
Node::getEffectInstance() const
{
    ///Thread safe as it never changes
    return _imp->effect;
}

void
Node::hasViewersConnected(std::list<ViewerInstance* >* viewers) const
{
    ViewerInstance* thisViewer = dynamic_cast<ViewerInstance*>(_imp->effect.get());
    
    if (thisViewer) {
        std::list<ViewerInstance* >::const_iterator alreadyExists = std::find(viewers->begin(), viewers->end(), thisViewer);
        if ( alreadyExists == viewers->end() ) {
            viewers->push_back(thisViewer);
        }
    } else {
        NodesList outputs;
        getOutputsWithGroupRedirection(outputs);
     
        for (NodesList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
            assert(*it);
            (*it)->hasViewersConnected(viewers);
        }
        
    }
}

/**
 * @brief Resolves links of the graph in the case of containers (that do not do any rendering but only contain nodes inside) 
 * so that algorithms that cycle the tree from bottom to top
 * properly visit all nodes in the correct order
 **/
static NodePtr applyNodeRedirectionsUpstream(const NodePtr& node, bool useGuiInput)
{
    if (!node) {
        return node;
    }
    NodeGroup* isGrp = node->isEffectGroup();
    if (isGrp) {
        //The node is a group, instead jump directly to the output node input of the  group
        return applyNodeRedirectionsUpstream(isGrp->getOutputNodeInput(useGuiInput), useGuiInput);
    }
    
    PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>(node->getEffectInstance().get());
    if (isPrecomp) {
        //The node is a precomp, instead jump directly to the output node of the precomp
        return applyNodeRedirectionsUpstream(isPrecomp->getOutputNode(), useGuiInput);
    }
    
    GroupInput* isInput = dynamic_cast<GroupInput*>(node->getEffectInstance().get());
    if (isInput) {
        //The node is a group input,  jump to the corresponding input of the group
        boost::shared_ptr<NodeCollection> collection = node->getGroup();
        assert(collection);
        isGrp = dynamic_cast<NodeGroup*>(collection.get());
        if (isGrp) {
            return applyNodeRedirectionsUpstream(isGrp->getRealInputForInput(useGuiInput,node),useGuiInput);
        }
    }
    
    return node;
}

/**
 * @brief Resolves links of the graph in the case of containers (that do not do any rendering but only contain nodes inside)
 * so that algorithms that cycle the tree from top to bottom
 * properly visit all nodes in the correct order. Note that one node may translate to several nodes since multiple nodes
 * may be connected to the same node.
 **/
static void applyNodeRedirectionsDownstream(int recurseCounter, const NodePtr& node, bool useGuiOutputs, NodesList& translated)
{
    NodeGroup* isGrp = node->isEffectGroup();
    if (isGrp) {
        //The node is a group, meaning it should not be taken into account, instead jump directly to the input nodes output of the group
        NodesList inputNodes;
        isGrp->getInputsOutputs(&inputNodes, useGuiOutputs);
        for (NodesList::iterator it2 = inputNodes.begin(); it2 != inputNodes.end(); ++it2) {
            //Call recursively on them
            applyNodeRedirectionsDownstream(recurseCounter + 1,*it2, useGuiOutputs, translated);
        }
        return;
    }
    
    GroupOutput* isOutput = dynamic_cast<GroupOutput*>(node->getEffectInstance().get());
    if (isOutput) {
        //The node is the output of a group, its outputs are the outputs of the group
        boost::shared_ptr<NodeCollection> collection = isOutput->getNode()->getGroup();
        assert(collection);
        isGrp = dynamic_cast<NodeGroup*>(collection.get());
        if (isGrp) {
            
            NodesWList groupOutputs;
            if (useGuiOutputs) {
                groupOutputs = isGrp->getNode()->getGuiOutputs();
            } else {
                isGrp->getNode()->getOutputs_mt_safe(groupOutputs);
            }
            for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                //Call recursively on them
                NodePtr output = it2->lock();
                if (output) {
                    applyNodeRedirectionsDownstream(recurseCounter + 1, output,useGuiOutputs, translated);
                }
            }
        }
        return;
    }
    
    boost::shared_ptr<PrecompNode> isInPrecomp = node->isPartOfPrecomp();
    if (isInPrecomp && isInPrecomp->getOutputNode() == node) {
        //This node is the output of the precomp, its outputs are the outputs of the precomp node
        NodesWList groupOutputs;
        if (useGuiOutputs) {
            groupOutputs = isInPrecomp->getNode()->getGuiOutputs();
        } else {
            isInPrecomp->getNode()->getOutputs_mt_safe(groupOutputs);
        }
        for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
            //Call recursively on them
            NodePtr output = it2->lock();
            if (output) {
                applyNodeRedirectionsDownstream(recurseCounter + 1, output,useGuiOutputs, translated);
            }
        }
        return;
    }
    
    //Base case: return this node
    if (recurseCounter > 0) {
        translated.push_back(node);
    }
}


void
Node::getOutputsWithGroupRedirection(NodesList& outputs) const
{
    NodesList redirections;
    NodePtr thisShared = boost::const_pointer_cast<Node>(shared_from_this());
    applyNodeRedirectionsDownstream(0, thisShared, false, redirections);
    if (!redirections.empty()) {
        outputs.insert(outputs.begin(), redirections.begin(), redirections.end());
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        for (NodesWList::const_iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it) {
            NodePtr output = it->lock();
            if (output) {
                outputs.push_back(output);
            }
        }
    }
    
}

void
Node::hasOutputNodesConnected(std::list<OutputEffectInstance* >* writers) const
{
    OutputEffectInstance* thisWriter = dynamic_cast<OutputEffectInstance*>(_imp->effect.get());
    
    if (thisWriter && thisWriter->isOutput() && !dynamic_cast<GroupOutput*>(thisWriter)) {
        std::list<OutputEffectInstance* >::const_iterator alreadyExists = std::find(writers->begin(), writers->end(), thisWriter);
        if ( alreadyExists == writers->end() ) {
            writers->push_back(thisWriter);
        }
    } else {
        NodesList outputs;
        getOutputsWithGroupRedirection(outputs);
        
        for (NodesList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
            assert(*it);
            (*it)->hasOutputNodesConnected(writers);
        }
    }
}

int
Node::getMajorVersion() const
{
    {
        QMutexLocker k(&_imp->pluginPythonModuleMutex);
        if (!_imp->pyPlugID.empty()) {
            return _imp->pyPlugVersion;
        }
    }
    if (!_imp->plugin) {
        return 0;
    }
    return _imp->plugin->getMajorVersion();
}

int
Node::getMinorVersion() const
{
    ///Thread safe as it never changes
    if (!_imp->plugin) {
        return 0;
    }
    {
        QMutexLocker k(&_imp->pluginPythonModuleMutex);
        if (!_imp->pyPlugID.empty()) {
            return 0;
        }
    }
    return _imp->plugin->getMinorVersion();
}

void
Node::initializeInputs()
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    int inputCount = getMaxInputCount();
    
    InputsV oldInputs;
    {
        QMutexLocker k(&_imp->inputsLabelsMutex);
        _imp->inputLabels.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputLabels[i] = _imp->effect->getInputLabel(i);
        }
    }
    {
        QMutexLocker l(&_imp->inputsMutex);
        oldInputs = _imp->inputs;
        
        _imp->inputs.resize(inputCount);
        _imp->guiInputs.resize(inputCount);
        ///if we added inputs, just set to NULL the new inputs, and add their label to the labels map
        for (int i = 0; i < inputCount; ++i) {
            if (i < (int)oldInputs.size()) {
                _imp->inputs[i] = oldInputs[i];
                _imp->guiInputs[i] = oldInputs[i];
            } else {
                _imp->inputs[i].reset();
                _imp->guiInputs[i].reset();
            }
        }
        
        
        ///Set the components the plug-in accepts
        _imp->inputsComponents.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputsComponents[i].clear();
            _imp->effect->addAcceptedComponents(i, &_imp->inputsComponents[i]);
        }
        _imp->outputComponents.clear();
        _imp->effect->addAcceptedComponents(-1, &_imp->outputComponents);
    }
    _imp->inputsInitialized = true;
    
    Q_EMIT inputsInitialized();
}

NodePtr
Node::getInput(int index) const
{
    return getInputInternal(false, true, index);
}



NodePtr
Node::getInputInternal(bool useGuiInput, bool useGroupRedirections, int index) const
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
        return NodePtr();
    }
    
    NodePtr ret =  useGuiInput ? _imp->guiInputs[index].lock() : _imp->inputs[index].lock();
    if (ret && useGroupRedirections) {
        ret = applyNodeRedirectionsUpstream(ret, useGuiInput);
    }
    return ret;
}

NodePtr
Node::getGuiInput(int index) const
{
    return getInputInternal(true, true, index);
}

NodePtr
Node::getRealInput(int index) const
{
    return getInputInternal(false, false, index);

}

NodePtr
Node::getRealGuiInput(int index) const
{
    return getInputInternal(true, false, index);
}

int
Node::getInputIndex(const Node* node) const
{
    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock().get() == node) {
            return i;
        }
    }
    return -1;
}

const std::vector<NodeWPtr > &
Node::getInputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getInputs();
    }
    
    return _imp->inputs;
}

const std::vector<NodeWPtr > &
Node::getGuiInputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getGuiInputs();
    }
    
    return _imp->guiInputs;
}

std::vector<NodeWPtr >
Node::getInputs_copy() const
{
    assert(_imp->inputsInitialized);
    
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getInputs();
    }
    
    QMutexLocker l(&_imp->inputsMutex);
    return _imp->inputs;
}

std::string
Node::getInputLabel(int inputNb) const
{
    assert(_imp->inputsInitialized);
    
    QMutexLocker l(&_imp->inputsLabelsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputLabels.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }
    
    return _imp->inputLabels[inputNb];
}

int
Node::getInputNumberFromLabel(const std::string& inputLabel) const
{
    assert(_imp->inputsInitialized);
    QMutexLocker l(&_imp->inputsLabelsMutex);
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
        if (_imp->inputs[i].lock()) {
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
        if (!_imp->inputs[i].lock() && !_imp->effect->isInputOptional(i)) {
            return true;
        }
    }
    return false;
}

bool
Node::hasAllInputsConnected() const
{
    QMutexLocker l(&_imp->inputsMutex);
    
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (!_imp->inputs[i].lock()) {
            return false;
        }
    }
    return true;

}

bool
Node::hasOutputConnected() const
{
    ////Only called by the main-thread
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->hasOutputConnected();
    }
    if (isOutputNode()) {
        return true;
    }
    if ( QThread::currentThread() == qApp->thread() ) {
        if (_imp->outputs.size() == 1) {

            NodePtr output = _imp->outputs.front().lock();
            return !(output->isTrackerNode() && output->isMultiInstance());

        } else if (_imp->outputs.size() > 1) {

            return true;
        }

    } else {
        QMutexLocker l(&_imp->outputsMutex);
        if (_imp->outputs.size() == 1) {

            NodePtr output = _imp->outputs.front().lock();
            return !(output->isTrackerNode() && output->isMultiInstance());

        } else if (_imp->outputs.size() > 1) {

            return true;
        }
    }

    return false;
}

bool
Node::checkIfConnectingInputIsOk(Node* input) const
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
Node::isNodeUpstream(const Node* input,
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
        if (_imp->inputs[i].lock().get() == input) {
            *ok = true;
            
            return;
        }
    }
    *ok = false;
    for (U32 i = 0; i  < _imp->inputs.size(); ++i) {
        NodePtr in = _imp->inputs[i].lock();
        if (in) {
            in->isNodeUpstream(input, ok);
            if (*ok) {
                return;
            }
        }
    }
}

static Node::CanConnectInputReturnValue checkCanConnectNoMultiRes(const Node* output, const NodePtr& input)
{
    //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
    //Check that the input has the same RoD that another input and that its rod is set to 0,0
    RenderScale scale(1.);
    RectD rod;
    bool isProjectFormat;
    StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(input->getHashValue(), output->getApp()->getTimeLine()->currentFrame(), scale, 0, &rod, &isProjectFormat);
    if (stat == eStatusFailed && !rod.isNull()) {
        return Node::eCanConnectInput_givenNodeNotConnectable;
    }
    if (rod.x1 != 0 || rod.y1 != 0) {
        return Node::eCanConnectInput_multiResNotSupported;
    }
    
  /*  RectD outputRod;
    stat = output->getEffectInstance()->getRegionOfDefinition_public(output->getHashValue(), output->getApp()->getTimeLine()->currentFrame(), scale, 0, &outputRod, &isProjectFormat);
    if (stat == eStatusFailed && !outputRod.isNull()) {
        return Node::eCanConnectInput_givenNodeNotConnectable;
    }
    
    if (rod != outputRod) {
        return Node::eCanConnectInput_multiResNotSupported;
    }*/
    
    for (int i = 0; i < output->getMaxInputCount(); ++i) {
        NodePtr inputNode = output->getInput(i);
        if (inputNode) {
            
            RectD inputRod;
            stat = inputNode->getEffectInstance()->getRegionOfDefinition_public(inputNode->getHashValue(), output->getApp()->getTimeLine()->currentFrame(), scale, 0, &inputRod, &isProjectFormat);
            if (stat == eStatusFailed && !inputRod.isNull()) {
                return Node::eCanConnectInput_givenNodeNotConnectable;
            }
            if (inputRod != rod) {
                return Node::eCanConnectInput_multiResNotSupported;
            }
            
        }
    }
    return Node::eCanConnectInput_ok;
}

Node::CanConnectInputReturnValue
Node::canConnectInput(const NodePtr& input,int inputNumber) const
{
   
    
    
    ///No-one is allowed to connect to the other node
    if (!input || !input->canOthersConnectToThisNode()) {
        return eCanConnectInput_givenNodeNotConnectable;
    }
    
    ///Check for invalid index
    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber >= (int)_imp->guiInputs.size() )) {
            return eCanConnectInput_indexOutOfRange;
        }
        if (_imp->guiInputs[inputNumber].lock()) {
            return eCanConnectInput_inputAlreadyConnected;
        }
    }
    
    NodeGroup* isGrp = input->isEffectGroup();
    if (isGrp && !isGrp->getOutputNode(true)) {
        return eCanConnectInput_groupHasNoOutput;
    }
    
    if (getParentMultiInstance() || input->getParentMultiInstance()) {
        return eCanConnectInput_inputAlreadyConnected;
    }
    
    ///Applying this connection would create cycles in the graph
    if (!checkIfConnectingInputIsOk(input.get())) {
        return eCanConnectInput_graphCycles;
    }
    
    if (_imp->effect->isInputRotoBrush(inputNumber)) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";
        return eCanConnectInput_indexOutOfRange;
    }
    
    if (!_imp->effect->supportsMultiResolution()) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return ret;
        }
    }
    
    {
        ///Check for invalid pixel aspect ratio if the node doesn't support multiple clip PARs
        if (!_imp->effect->supportsMultipleClipsPAR()) {
            
            double inputPAR = input->getEffectInstance()->getPreferredAspectRatio();
            
            double inputFPS = input->getEffectInstance()->getPreferredFrameRate();
            
            QMutexLocker l(&_imp->inputsMutex);

            for (InputsV::const_iterator it = _imp->guiInputs.begin(); it != _imp->guiInputs.end(); ++it) {
                NodePtr node = it->lock();
                if (node) {
                    if (node->getEffectInstance()->getPreferredAspectRatio() != inputPAR) {
                        return eCanConnectInput_differentPars;
                    }
                    
                    if (std::abs(node->getEffectInstance()->getPreferredFrameRate() - inputFPS) > 0.01) {
                        return eCanConnectInput_differentFPS;
                    }
                    
                }
            }
        }
    }
    
    return eCanConnectInput_ok;
}

bool
Node::connectInput(const NodePtr & input,
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
    if (_imp->effect->isInputRotoBrush(inputNumber)) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";
        return false;
    }
    
    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if (!_imp->effect->supportsMultiResolution()) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }
    
    bool useGuiInputs = isNodeRendering();
    _imp->effect->abortAnyEvaluation();
    
    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if (inputNumber < 0 ||
            inputNumber >= (int)_imp->inputs.size() ||
            (!useGuiInputs && _imp->inputs[inputNumber].lock()) ||
            (useGuiInputs && _imp->guiInputs[inputNumber].lock())) {
            return false;
        }
        
        ///Set the input
        if (!useGuiInputs) {
            _imp->inputs[inputNumber] = input;
            _imp->guiInputs[inputNumber] = input;
        } else {
            _imp->guiInputs[inputNumber] = input;
            _imp->mustCopyGuiInputs = true;
        }
        input->connectOutput(useGuiInputs,shared_from_this());
    }
    
    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL( labelChanged(QString) ), this, SLOT( onInputLabelChanged(QString) ) );
    
    ///Notify the GUI
    Q_EMIT inputChanged(inputNumber);
    
    bool mustCallEnd = false;
    
    if (!useGuiInputs) {
        ///Call the instance changed action with a reason clip changed
        beginInputEdition();
        mustCallEnd = true;
        onInputChanged(inputNumber);
    }
    
    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }
    
    _imp->ifGroupForceHashChangeOfInputs();
    
    std::string inputChangedCB = getInputChangedCallback();
    if (!inputChangedCB.empty()) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }
    
    if (mustCallEnd) {
        endInputEdition(true);
    }
    
    return true;
}

void
Node::Implementation::ifGroupForceHashChangeOfInputs()
{
    ///If the node is a group, force a change of the outputs of the GroupInput nodes so the hash of the tree changes downstream
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(effect.get());
    if (isGrp && !isGrp->getApp()->isCreatingNodeTree()) {
        NodesList inputsOutputs;
        isGrp->getInputsOutputs(&inputsOutputs, false);
        for (NodesList::iterator it = inputsOutputs.begin(); it != inputsOutputs.end(); ++it) {
            (*it)->incrementKnobsAge_internal();
            (*it)->computeHash();
        }
    }
}

bool
Node::replaceInput(const NodePtr& input,int inputNumber)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    assert(input);
    
    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }
    if (_imp->effect->isInputRotoBrush(inputNumber)) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";
        return false;
    }
    
    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if (!_imp->effect->supportsMultiResolution()) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }
    
    bool useGuiInputs = isNodeRendering();
    _imp->effect->abortAnyEvaluation();
    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber > (int)_imp->inputs.size() ) ) {
            return false;
        }
    }

    {
        QMutexLocker l(&_imp->inputsMutex);
        ///Set the input
        
        if (!useGuiInputs) {
            NodePtr curIn = _imp->inputs[inputNumber].lock();
            if (curIn) {
                QObject::connect( curIn.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)));
                curIn->disconnectOutput(useGuiInputs, this);
            }
            _imp->inputs[inputNumber] = input;
            _imp->guiInputs[inputNumber] = input;
        } else {
            NodePtr curIn = _imp->guiInputs[inputNumber].lock();
            if (curIn) {
                QObject::connect(curIn.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)));
                curIn->disconnectOutput(useGuiInputs, this);
            }
            _imp->guiInputs[inputNumber] = input;
            _imp->mustCopyGuiInputs = true;
        }
        input->connectOutput(useGuiInputs, shared_from_this());
    }
    
    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL( labelChanged(QString) ), this, SLOT( onInputLabelChanged(QString) ) );
    
    ///Notify the GUI
    Q_EMIT inputChanged(inputNumber);

    bool mustCallEnd = false;
    if (!useGuiInputs) {
        beginInputEdition();
        mustCallEnd = true;
        ///Call the instance changed action with a reason clip changed
        onInputChanged(inputNumber);
    }
    
    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }
    
    _imp->ifGroupForceHashChangeOfInputs();
    
    std::string inputChangedCB = getInputChangedCallback();
    if (!inputChangedCB.empty()) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    if (mustCallEnd) {
        endInputEdition(true);
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
        if ( !_imp->effect->isInputMask(i) ) {
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
        if ( !_imp->effect->isInputMask(j) ) {
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
    
    bool useGuiInputs = isNodeRendering();
    _imp->effect->abortAnyEvaluation();
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        assert( inputAIndex < (int)_imp->inputs.size() && inputBIndex < (int)_imp->inputs.size() );
        NodePtr input0;
        
        if (!useGuiInputs) {
            input0 = _imp->inputs[inputAIndex].lock();
            _imp->inputs[inputAIndex] = _imp->inputs[inputBIndex];
            _imp->inputs[inputBIndex] = input0;
            _imp->guiInputs[inputAIndex] = _imp->inputs[inputAIndex];
            _imp->guiInputs[inputBIndex] = _imp->inputs[inputBIndex];
        } else {
            input0 = _imp->guiInputs[inputAIndex].lock();
            _imp->guiInputs[inputAIndex] = _imp->guiInputs[inputBIndex];
            _imp->guiInputs[inputBIndex] = input0;
            _imp->mustCopyGuiInputs = true;
        }
    }
    Q_EMIT inputChanged(inputAIndex);
    Q_EMIT inputChanged(inputBIndex);
    bool mustCallEnd = false;
    if (!useGuiInputs) {
        beginInputEdition();
        mustCallEnd = true;
        onInputChanged(inputAIndex);
        onInputChanged(inputBIndex);
        
    }
    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }
    
    std::string inputChangedCB = getInputChangedCallback();
    if (!inputChangedCB.empty()) {
        _imp->runInputChangedCallback(inputAIndex, inputChangedCB);
        _imp->runInputChangedCallback(inputBIndex, inputChangedCB);
    }

    
    _imp->ifGroupForceHashChangeOfInputs();
    
    if (mustCallEnd) {
        endInputEdition(true);
    }

} // switchInput0And1

void
Node::onInputLabelChanged(const QString & name)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    Node* inp = dynamic_cast<Node*>( sender() );
    assert(inp);
    if (!inp) {
        // coverity[dead_error_line]
        return;
    }
    int inputNb = -1;
    ///No need to lock, inputs is only written to by the mainthread
    
    for (U32 i = 0; i < _imp->guiInputs.size(); ++i) {
        if (_imp->guiInputs[i].lock().get() == inp) {
            inputNb = i;
            break;
        }
    }
    
    if (inputNb != -1) {
        Q_EMIT inputLabelChanged(inputNb, name);
    }
}

void
Node::connectOutput(bool useGuiValues,const NodePtr& output)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(output);
    
    {
        QMutexLocker l(&_imp->outputsMutex);
        if (!useGuiValues) {
            _imp->outputs.push_back(output);
            _imp->guiOutputs.push_back(output);
        } else {
            _imp->guiOutputs.push_back(output);
        }
    }
    Q_EMIT outputsChanged();
}

int
Node::disconnectInput(int inputNumber)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    
    NodePtr inputShared;
    bool useGuiValues = isNodeRendering();
    
    bool destroyed;
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        destroyed = _imp->isBeingDestroyed;
    }
    if (!destroyed) {
        _imp->effect->abortAnyEvaluation();
    }
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        if (inputNumber < 0 ||
            inputNumber > (int)_imp->inputs.size() ||
            (!useGuiValues && !_imp->inputs[inputNumber].lock()) ||
            (useGuiValues && !_imp->guiInputs[inputNumber].lock())) {
            return -1;
        }
        inputShared = useGuiValues ? _imp->guiInputs[inputNumber].lock() : _imp->inputs[inputNumber].lock();
    }
    

    
    QObject::disconnect( inputShared.get(), SIGNAL( labelChanged(QString) ), this, SLOT( onInputLabelChanged(QString) ) );
    inputShared->disconnectOutput(useGuiValues,this);
    
    {
        QMutexLocker l(&_imp->inputsMutex);
        if (!useGuiValues) {
            _imp->inputs[inputNumber].reset();
            _imp->guiInputs[inputNumber].reset();
        } else {
            _imp->guiInputs[inputNumber].reset();
            _imp->mustCopyGuiInputs = true;
        }
    }
    
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        if (_imp->isBeingDestroyed) {
            return -1;
        }
    }
    
    Q_EMIT inputChanged(inputNumber);
    bool mustCallEnd = false;
    if (!useGuiValues) {
        beginInputEdition();
        mustCallEnd= true;
        onInputChanged(inputNumber);
    }
    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }
    
    _imp->ifGroupForceHashChangeOfInputs();
    
    std::string inputChangedCB = getInputChangedCallback();
    if (!inputChangedCB.empty()) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }
    if (mustCallEnd) {
        endInputEdition(true);
    }
    return inputNumber;
}

int
Node::disconnectInput(Node* input)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    int found = -1;
    bool useGuiValues = isNodeRendering();
    _imp->effect->abortAnyEvaluation();
    NodePtr inputShared;
    {
        QMutexLocker l(&_imp->inputsMutex);
        if (!useGuiValues) {
            for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
                NodePtr curInput = _imp->inputs[i].lock();
                if (curInput.get() == input) {
                    inputShared = curInput;
                    found = (int)i;
                    break;
                }
            }
        } else {
            for (std::size_t i = 0; i < _imp->guiInputs.size(); ++i) {
                NodePtr curInput = _imp->guiInputs[i].lock();
                if (curInput.get() == input) {
                    inputShared = curInput;
                    found = (int)i;
                    break;
                }
            }
        }
    }
    if (found != -1) {
        
        {
            QMutexLocker l(&_imp->inputsMutex);
            if (!useGuiValues) {
                _imp->inputs[found].reset();
                _imp->guiInputs[found].reset();
            } else {
                _imp->guiInputs[found].reset();
                _imp->mustCopyGuiInputs = true;
            }
        }
        input->disconnectOutput(useGuiValues,this);
        Q_EMIT inputChanged(found);
        bool mustCallEnd = false;
        if (!useGuiValues) {
            beginInputEdition();
            mustCallEnd = true;
            onInputChanged(found);
        }
        bool creatingNodeTree = getApp()->isCreatingNodeTree();
        if (!creatingNodeTree) {
            ///Recompute the hash
            if (!getApp()->getProject()->isProjectClosing()) {
                computeHash();
            }
        }

        
        _imp->ifGroupForceHashChangeOfInputs();
        
        std::string inputChangedCB = getInputChangedCallback();
        if (!inputChangedCB.empty()) {
            _imp->runInputChangedCallback(found, inputChangedCB);
        }
        
        if (mustCallEnd) {
            endInputEdition(true);
        }
        
        return found;
    }
    
    return -1;
}

int
Node::disconnectOutput(bool useGuiValues,const Node* output)
{
    assert(output);
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    int ret = -1;
    {
        QMutexLocker l(&_imp->outputsMutex);
        if (!useGuiValues) {
            
            int ret = 0;
            for (NodesWList::iterator it = _imp->outputs.begin(); it!=_imp->outputs.end(); ++it,++ret) {
                if (it->lock().get() == output) {
                    _imp->outputs.erase(it);
                    break;
                }
            }
        }
        int ret = 0;
        for (NodesWList::iterator it = _imp->guiOutputs.begin(); it!=_imp->guiOutputs.end(); ++it,++ret) {
            if (it->lock().get() == output) {
                _imp->guiOutputs.erase(it);
                break;
            }
        }
        
    }
    
    //Will just refresh the gui
    Q_EMIT outputsChanged();
    
    return ret;
}


int
Node::inputIndex(const NodePtr& n) const
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
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock() == n) {
            return i;
        }
    }
    
    
    return -1;
}

void
Node::clearLastRenderedImage()
{
    _imp->effect->clearLastRenderedImage();
}

/*After this call this node still knows the link to the old inputs/outputs
 but no other node knows this node.*/
void
Node::deactivate(const std::list< NodePtr > & outputsToDisconnect,
                 bool disconnectAll,
                 bool reconnect,
                 bool hideGui,
                 bool triggerRender)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    if (!_imp->effect || !isActivated()) {
        return;
    }
    //first tell the gui to clear any persistent message linked to this node
    clearPersistentMessage(false);
    
    boost::shared_ptr<NodeCollection> parentCol = getGroup();
    NodeGroup* isParentGroup = dynamic_cast<NodeGroup*>(parentCol.get());
    
    ///For all knobs that have listeners, kill expressions
    const KnobsVec & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobI::ListenerDimsMap listeners;
        knobs[i]->getListeners(listeners);
        for (KnobI::ListenerDimsMap::iterator it = listeners.begin(); it != listeners.end(); ++it) {
            KnobPtr listener = it->first.lock();
            if (!listener) {
                continue;
            }
            KnobHolder* holder = listener->getHolder();
            if (!holder) {
                continue;
            }
            if (holder == _imp->effect.get() || holder == isParentGroup) {
                continue;
            }
            
            EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
            if (!isEffect) {
                continue;
            }
            
            boost::shared_ptr<NodeCollection> effectParent = isEffect->getNode()->getGroup();
            assert(effectParent);
            NodeGroup* isEffectParentGroup = dynamic_cast<NodeGroup*>(effectParent.get());
            if (isEffectParentGroup && isEffectParentGroup == _imp->effect.get()) {
                continue;
            }
            
            isEffect->beginChanges();
            for (int dim = 0; dim < listener->getDimension(); ++dim) {
                std::pair<int, KnobPtr > master = listener->getMaster(dim);
                if (master.second == knobs[i]) {
                    listener->unSlave(dim, true);
                }
                
                std::string hasExpr = listener->getExpression(dim);
                if (!hasExpr.empty()) {
                    listener->clearExpression(dim,true);
                }
            }
            isEffect->endChanges(true);
        }
    }
    
    ///if the node has 1 non-optional input, attempt to connect the outputs to the input of the current node
    ///this node is the node the outputs should attempt to connect to
    NodePtr inputToConnectTo;
    NodePtr firstOptionalInput;
    int firstNonOptionalInput = -1;
    if (reconnect) {
        bool hasOnlyOneInputConnected = false;
        
        ///No need to lock guiInputs is only written to by the mainthread
        for (std::size_t i = 0; i < _imp->guiInputs.size(); ++i) {
            NodePtr input = _imp->guiInputs[i].lock();
            if (input) {
                if ( !_imp->effect->isInputOptional(i) ) {
                    if (firstNonOptionalInput == -1) {
                        firstNonOptionalInput = i;
                        hasOnlyOneInputConnected = true;
                    } else {
                        hasOnlyOneInputConnected = false;
                    }
                } else if (!firstOptionalInput) {
                    firstOptionalInput = input;
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
                inputToConnectTo = getRealGuiInput(firstNonOptionalInput);
            } else if (firstOptionalInput) {
                inputToConnectTo = firstOptionalInput;
            }
        }
    }
    /*Removing this node from the output of all inputs*/
    _imp->deactivatedState.clear();
    
    
    std::vector<NodePtr > inputsQueueCopy;


    ///For multi-instances, if we deactivate the main instance without hiding the GUI (the default state of the tracker node)
    ///then don't remove it from outputs of the inputs
    if (hideGui || !_imp->isMultiInstance) {
        for (std::size_t i = 0; i < _imp->guiInputs.size(); ++i) {
            NodePtr input = _imp->guiInputs[i].lock();
            if (input) {
                input->disconnectOutput(false,this);
            }
        }
    }
    
    
    ///For each output node we remember that the output node  had its input number inputNb connected
    ///to this node
    NodesWList outputsQueueCopy;
    {
        QMutexLocker l(&_imp->outputsMutex);
        outputsQueueCopy = _imp->guiOutputs;
    }
    
    
    for (NodesWList::iterator it = outputsQueueCopy.begin(); it != outputsQueueCopy.end(); ++it) {
        
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        bool dc = false;
        if (disconnectAll) {
            dc = true;
        } else {
            
            for (NodesList::const_iterator found = outputsToDisconnect.begin(); found != outputsToDisconnect.end(); ++found) {
                if (*found == output) {
                    dc = true;
                    break;
                }
            }
        }
        if (dc) {
            int inputNb = output->getInputIndex(this);
            if (inputNb != -1) {
                _imp->deactivatedState.insert( make_pair(*it, inputNb) );
                
                ///reconnect if inputToConnectTo is not null
                if (inputToConnectTo) {
                    output->replaceInput(inputToConnectTo, inputNb);
                } else {
                    ignore_result(output->disconnectInput(this));
                }
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
    ///_imp->effect->clearPluginMemoryChunks();
    clearLastRenderedImage();
    
    bool beingDestroyed;
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        beingDestroyed = _imp->isBeingDestroyed;
    }
    if (parentCol && !beingDestroyed) {
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
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(_imp->effect.get());
    if (isGrp) {
        isGrp->setIsDeactivatingGroup(true);
        NodesList nodes = isGrp->getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->deactivate(std::list< NodePtr >(),false,false,true,false);
        }
        isGrp->setIsDeactivatingGroup(false);
    }
    
    ///If the node has children (i.e it is a multi-instance), deactivate its children
    for (NodesWList::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        it->lock()->deactivate(std::list< NodePtr >(),false,false,true,false);
    }
    

    
    if (!getApp()->getProject()->isProjectClosing()) {
        _imp->runOnNodeDeleteCB();
    }
    
} // deactivate

void
Node::activate(const std::list< NodePtr > & outputsToRestore,
               bool restoreAll,
               bool triggerRender)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->effect || isActivated()) {
        return;
    }

    
    ///No need to lock, guiInputs is only written to by the main-thread
    NodePtr thisShared = shared_from_this();

    ///for all inputs, reconnect their output to this node
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            input->connectOutput(false,thisShared);
        }
    }
    
    
    
    ///Restore all outputs that was connected to this node
    for (std::map<NodeWPtr,int >::iterator it = _imp->deactivatedState.begin();
         it != _imp->deactivatedState.end(); ++it) {
        
        NodePtr output = it->first.lock();
        if (!output) {
            continue;
        }
        
        bool restore = false;
        if (restoreAll) {
            restore = true;
        } else {
            for (NodesList::const_iterator found = outputsToRestore.begin(); found != outputsToRestore.end(); ++found) {
                if (*found == output) {
                    restore = true;
                    break;
                }
            }
        }
        
        if (restore) {
            ///before connecting the outputs to this node, disconnect any link that has been made
            ///between the outputs by the user. This should normally never happen as the undo/redo
            ///stack follow always the same order.
            NodePtr outputHasInput = output->getInput(it->second);
            if (outputHasInput) {
                bool ok = getApp()->getProject()->disconnectNodes(outputHasInput, output);
                assert(ok);
                Q_UNUSED(ok);
            }
            
            ///and connect the output to this node
            output->connectInput(thisShared, it->second);
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
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(_imp->effect.get());
    if (isGrp) {
        isGrp->setIsActivatingGroup(true);
        NodesList nodes = isGrp->getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->activate(std::list< NodePtr >(),false,false);
        }
        isGrp->setIsActivatingGroup(false);
    }
    
    ///If the node has children (i.e it is a multi-instance), activate its children
    for (NodesWList::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        it->lock()->activate(std::list< NodePtr >(),false,false);
    }
    
    _imp->runOnNodeCreatedCB(true);
} // activate

void
Node::destroyNodeInternal(bool fromDest, bool autoReconnect)
{
    assert(QThread::currentThread() == qApp->thread());
    
    if (!_imp->effect) {
        return;
    }
    
    {
        QMutexLocker k(&_imp->activatedMutex);
        _imp->isBeingDestroyed = true;
    }
    
    quitAnyProcessing();
    
    
    ///Remove the node from the project
    deactivate(NodesList(),
               true,
               autoReconnect,
               true,
               false);
    
    {
        boost::shared_ptr<NodeGuiI> guiPtr = _imp->guiPointer.lock();
        if (guiPtr) {
            guiPtr->destroyGui();
        }
    }
    
    ///If its a group, clear its nodes
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(_imp->effect.get());
    if (isGrp) {
        isGrp->clearNodes(true);
    }
    
    
    ///Quit any rendering
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>(_imp->effect.get());
    if (isOutput) {
        isOutput->getRenderEngine()->quitEngine();
    }
    
    ///Remove all images in the cache associated to this node
    ///This will not remove from the disk cache if the project is closing
    removeAllImagesFromCache(false);
    
    ///Remove the Python node
    deleteNodeVariableToPython(getFullyQualifiedName());
    
    ///Disconnect all inputs
    /*int maxInputs = getMaxInputCount();
     for (int i = 0; i < maxInputs; ++i) {
     disconnectInput(i);
     }*/
    
    ///Kill the effect
    _imp->effect.reset();
    
    ///If inside the group, remove it from the group
    ///the use_count() after the call to removeNode should be 2 and should be the shared_ptr held by the caller and the
    ///thisShared ptr
    
    ///If not inside a gorup or inside fromDest the shared_ptr is probably invalid at this point
    if (!fromDest) {
        NodePtr thisShared = shared_from_this();
        if (getGroup()) {
            getGroup()->removeNode(thisShared);
        }
    }
    
    
}

void
Node::destroyNode(bool autoReconnect)
{
    destroyNodeInternal(false, autoReconnect);
}

KnobPtr
Node::getKnobByName(const std::string & name) const
{
    ///MT-safe, never changes
    assert(_imp->knobsInitialized);
    
    return _imp->effect->getKnobByName(name);
}

namespace {
    ///output is always RGBA with alpha = 255
    template<typename PIX,int maxValue, int srcNComps>
    void
    renderPreview(const Image & srcImg,
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
        
        Image::ReadAccess acc = srcImg.getReadRights();
        
        
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
                        float rFilt = src_pixels[xi * srcNComps] / (float)maxValue;
                        float gFilt = srcNComps < 2 ? 0 : src_pixels[xi * srcNComps + 1] / (float)maxValue;
                        float bFilt = srcNComps < 3 ? 0 : src_pixels[xi * srcNComps + 2] / (float)maxValue;
                        if (srcNComps == 1) {
                            gFilt = bFilt = rFilt;
                        }
                        int r = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(rFilt) : rFilt);
                        int g = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(gFilt) : gFilt);
                        int b = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(bFilt) : bFilt);
                        dst_pixels[j] = toBGRA(r, g, b, 255);
                    }
                }
            }
        }
    } // renderPreview
    
    ///output is always RGBA with alpha = 255
    template<typename PIX,int maxValue>
    void
    renderPreviewForDepth(const Image & srcImg,
                          int elemCount,
                          int *dstWidth,
                          int *dstHeight,
                          bool convertToSrgb,
                          unsigned int* dstPixels) {
        switch (elemCount) {
            case 0:
                return;
            case 1:
                renderPreview<PIX,maxValue,1>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
                break;
            case 2:
                renderPreview<PIX,maxValue,2>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
                break;
            case 3:
                renderPreview<PIX,maxValue,3>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
                break;
            case 4:
                renderPreview<PIX,maxValue,4>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
                break;
            default:
                break;
        }
    }
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
        
        (void)_imp->checkForExitPreview();
    }
};

bool
Node::makePreviewImage(SequenceTime time,
                       int *width,
                       int *height,
                       unsigned int* buf)
{
    assert(_imp->knobsInitialized);
    
    
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        if (_imp->isBeingDestroyed) {
            return false;
        }
    }
    
    if (_imp->checkForExitPreview()) {
        return false;
    }
    
    /// prevent 2 previews to occur at the same time since there's only 1 preview instance
    ComputingPreviewSetter_RAII computingPreviewRAII(_imp.get());
    
    RectD rod;
    bool isProjectFormat;
    RenderScale scale(1.);
    U64 nodeHash = getHashValue();
    
    EffectInstance* effect = 0;
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->effect.get());
    if (isGroup) {
        effect = isGroup->getOutputNode(false)->getEffectInstance().get();
    } else {
        effect = _imp->effect.get();
    }
    
    if (!_imp->effect) {
        return false;
    }
    
    StatusEnum stat = effect->getRegionOfDefinition_public(nodeHash,time, scale, 0, &rod, &isProjectFormat);
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
    
    scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;
    
    const double par = effect->getPreferredAspectRatio();
    
    RectI renderWindow;
    rod.toPixelEnclosing(mipMapLevel, par, &renderWindow);
    
    NodePtr thisNode = shared_from_this();
    
    RenderingFlagSetter flagIsRendering(this);

    
    FrameRequestMap request;
    stat = EffectInstance::computeRequestPass(time, 0, mipMapLevel, rod, thisNode, request);
    if (stat == eStatusFailed) {
        return false;
    }
    
    {
        ParallelRenderArgsSetter frameRenderArgs(time,
                                                 0, //< preview only renders view 0 (left)
                                                 true, //<isRenderUserInteraction
                                                 false, //isSequential
                                                 true, //can abort
                                                 0, //render Age
                                                 thisNode, // viewer requester
                                                 &request,
                                                 0, //texture index
                                                 getApp()->getTimeLine().get(), // timeline
                                                 NodePtr(), //rotoPaint node
                                                 false, // isAnalysis
                                                 true, // isDraft
                                                 false, // enableProgress
                                                 boost::shared_ptr<RenderStats>());
        
        std::list<ImageComponents> requestedComps;
        ImageBitDepthEnum depth;
        getEffectInstance()->getPreferredDepthAndComponents(-1, &requestedComps, &depth);
        
        
        
        // Exceptions are caught because the program can run without a preview,
        // but any exception in renderROI is probably fatal.
        ImageList planes;
        try {
            EffectInstance::RenderRoIRetCode retCode =
            effect->renderRoI( EffectInstance::RenderRoIArgs( time,
                                                             scale,
                                                             mipMapLevel,
                                                             0, //< preview only renders view 0 (left)
                                                             false,
                                                             renderWindow,
                                                             rod,
                                                             requestedComps, //< preview is always rgb...
                                                             depth, false, effect) ,&planes);
            if (retCode != EffectInstance::eRenderRoIRetCodeOk) {
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
        bool convertToSrgb = getApp()->getDefaultColorSpaceForBitDepth( img->getBitDepth() ) == eViewerColorSpaceLinear;
        
        switch ( img->getBitDepth() ) {
            case eImageBitDepthByte: {
                renderPreviewForDepth<unsigned char, 255>(*img, elemCount, width, height,convertToSrgb, buf);
                break;
            }
            case eImageBitDepthShort: {
                renderPreviewForDepth<unsigned short, 65535>(*img, elemCount, width, height,convertToSrgb, buf);
                break;
            }
            case eImageBitDepthHalf:
                break;
            case eImageBitDepthFloat: {
                renderPreviewForDepth<float, 1>(*img, elemCount, width, height,convertToSrgb, buf);
                break;
            }
            case eImageBitDepthNone:
                break;
        }
    } // ParallelRenderArgsSetter
    
    ///Exit of the thread
    appPTR->getAppTLS()->cleanupTLSForThread();
    
    return true;
} // makePreviewImage

bool
Node::isInputNode() const
{
    ///MT-safe, never changes
    return _imp->effect->isGenerator();
}

bool
Node::isOutputNode() const
{   ///MT-safe, never changes
    return _imp->effect->isOutput();
}

bool
Node::isOpenFXNode() const
{
    ///MT-safe, never changes
    return _imp->effect->isOpenFX();
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
    return _imp->effect ? _imp->effect->isRotoPaintNode() : false;
}

ViewerInstance*
Node::isEffectViewer() const
{
    return dynamic_cast<ViewerInstance*>(_imp->effect.get());
}

NodeGroup*
Node::isEffectGroup() const
{
    return dynamic_cast<NodeGroup*>(_imp->effect.get());
}

boost::shared_ptr<RotoContext>
Node::getRotoContext() const
{
    return _imp->rotoContext;
}

U64
Node::getRotoAge() const
{
    if (_imp->rotoContext) {
        return _imp->rotoContext->getAge();
    }
    
    boost::shared_ptr<RotoDrawableItem> item = _imp->paintStroke.lock();
    if (item) {
        return item->getContext()->getAge();
    }
    return 0;
    
}


const KnobsVec &
Node::getKnobs() const
{
    ///MT-safe from EffectInstance::getKnobs()
    return _imp->effect->getKnobs();
}

void
Node::setKnobsFrozen(bool frozen)
{
    ///MT-safe from EffectInstance::setKnobsFrozen
    _imp->effect->setKnobsFrozen(frozen);
    
    QMutexLocker l(&_imp->inputsMutex);
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            input->setKnobsFrozen(frozen);
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
    if (!_imp->plugin) {
        return std::string();
    }
    
    {
        QMutexLocker k(&_imp->pluginPythonModuleMutex);
        if (!_imp->pyPlugID.empty()) {
            return _imp->pyPlugID;
        }
    }
    
    return _imp->plugin->getPluginID().toStdString();
    
}

std::string
Node::getPluginLabel() const
{
    {
        QMutexLocker k(&_imp->pluginPythonModuleMutex);
        if (!_imp->pyPlugLabel.empty()) {
            return _imp->pyPlugLabel;
        }
    }
    return _imp->effect->getPluginLabel();
}

std::string
Node::getPluginDescription() const
{
    {
        QMutexLocker k(&_imp->pluginPythonModuleMutex);
        if (!_imp->pyPlugDesc.empty()) {
            return _imp->pyPlugDesc;
        }
    }
    return _imp->effect->getPluginDescription();
}

int
Node::getMaxInputCount() const
{
    ///MT-safe, never changes
    assert(_imp->effect);
    
    return _imp->effect->getMaxInputCount();
}

bool
Node::makePreviewByDefault() const
{
    ///MT-safe, never changes
    assert(_imp->effect);
    
    return _imp->effect->makePreviewByDefault();
}

void
Node::togglePreview()
{
    ///MT-safe from Knob
    assert(_imp->knobsInitialized);
    boost::shared_ptr<KnobBool> b = _imp->previewEnabledKnob.lock();
    if (!b) {
        return;
    }
    b->setValue(!b->getValue(),0);
}

bool
Node::isPreviewEnabled() const
{
    ///MT-safe from EffectInstance
    if (!_imp->knobsInitialized) {
        qDebug() << "Node::isPreviewEnabled(): knobs not initialized (including previewEnabledKnob)";
    }
    boost::shared_ptr<KnobBool> b = _imp->previewEnabledKnob.lock();
    if (!b) {
        return false;
    }
    return b->getValue();
    
}

bool
Node::aborted() const
{
    ///MT-safe from EffectInstance
    assert(_imp->effect);
    
    return _imp->effect->aborted();
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
    if (_imp->nodeIsDequeuing) {
        _imp->nodeIsDequeuing = false;
        _imp->nodeIsDequeuingCond.wakeOne();
    }
//    }
    
}

bool
Node::message(MessageTypeEnum type,
              const std::string & content) const
{
    ///If the node was aborted, don't transmit any message because we could cause a deadlock
    if ( _imp->effect->aborted() ) {
        return false;
    }
    
    switch (type) {
        case eMessageTypeInfo:
            Dialogs::informationDialog(getLabel_mt_safe(), content);
            
            return true;
        case eMessageTypeWarning:
            Dialogs::warningDialog(getLabel_mt_safe(), content);
            
            return true;
        case eMessageTypeError:
            Dialogs::errorDialog(getLabel_mt_safe(), content);
            
            return true;
        case eMessageTypeQuestion:
            
            return Dialogs::questionDialog(getLabel_mt_safe(), content, false) == eStandardButtonYes;
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
            QString mess(content.c_str());
            if (mess == _imp->persistentMessage) {
                return;
            }
            _imp->persistentMessageType = (int)type;
            _imp->persistentMessage = mess;
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
Node::getPersistentMessage(QString* message,int* type,bool prefixLabelAndType) const
{
    QMutexLocker k(&_imp->persistentMessageMutex);
    *type = _imp->persistentMessageType;
    
    if (prefixLabelAndType && !_imp->persistentMessage.isEmpty()) {
        message->append( getLabel_mt_safe().c_str() );
        if (*type == eMessageTypeError) {
            message->append(" error: ");
        } else if (*type == eMessageTypeWarning) {
            message->append(" warning: ");
        }
    }
    message->append(_imp->persistentMessage);
}

void
Node::clearPersistentMessageRecursive(std::list<Node*>& markedNodes)
{
    if (std::find(markedNodes.begin(), markedNodes.end(), this) != markedNodes.end()) {
        return;
    }
    markedNodes.push_back(this);
    clearPersistentMessageInternal();
    
    int nInputs = getMaxInputCount();
    ///No need to lock, guiInputs is only written to by the main-thread
    for (int i = 0; i < nInputs; ++i) {
        NodePtr input = getInput(i);
        if (input) {
            input->clearPersistentMessageRecursive(markedNodes);
        }
    }
}

void
Node::clearPersistentMessageInternal()
{
    
    bool changed;
    {
        QMutexLocker k(&_imp->persistentMessageMutex);
        changed = !_imp->persistentMessage.isEmpty();
        if (changed) {
            _imp->persistentMessage.clear();
        }
    }
    if (changed) {
        Q_EMIT persistentMessageChanged();
    }
}

void
Node::clearPersistentMessage(bool recurse)
{
    if (getApp()->isBackground()) {
        return;
    }
    if (recurse) {
        std::list<Node*> markedNodes;
        clearPersistentMessageRecursive(markedNodes);
    } else {
        clearPersistentMessageInternal();
    }
    
}

void
Node::purgeAllInstancesCaches()
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->effect);
    _imp->effect->purgeCaches();
}

bool
Node::notifyInputNIsRendering(int inputNb)
{
    if (!getApp() || getApp()->isGuiFrozen()) {
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
    if (!getApp() || getApp()->isGuiFrozen()) {
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
    assert(_imp->effect);
    _imp->effect->setOutputFilesForWriter(pattern);
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

static void refreshPreviewsRecursivelyUpstreamInternal(double time,Node* node,std::list<Node*>& marked)
{
    if (std::find(marked.begin(), marked.end(), node) != marked.end()) {
        return;
    }

    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }
    
    marked.push_back(node);
    
    std::vector<NodeWPtr > inputs = node->getInputs_copy();
    
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        NodePtr input = inputs[i].lock();
        if (input) {
            input->refreshPreviewsRecursivelyUpstream(time);
        }
    }

}

void
Node::refreshPreviewsRecursivelyUpstream(double time)
{
    std::list<Node*> marked;
    refreshPreviewsRecursivelyUpstreamInternal(time,this,marked);
}

static void refreshPreviewsRecursivelyDownstreamInternal(double time,Node* node,std::list<Node*>& marked)
{
    if (std::find(marked.begin(), marked.end(), node) != marked.end()) {
        return;
    }
    
    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }
    
    marked.push_back(node);
    
    NodesWList outputs;
    node->getOutputs_mt_safe(outputs);
    for (NodesWList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (output) {
            output->refreshPreviewsRecursivelyDownstream(time);
        }
    }

}

void
Node::refreshPreviewsRecursivelyDownstream(double time)
{
    if (!getNodeGui()) {
        return;
    }
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
        EffectInstance* effect = dynamic_cast<EffectInstance*>(master);
        assert(effect);
        NodePtr masterNode = effect->getNode();
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
    NodePtr parentNode  = isEffect->getNode();
    bool changed = false;
    {
        QMutexLocker l(&_imp->masterNodeMutex);
        KnobLinkList::iterator found = _imp->nodeLinks.end();
        for (KnobLinkList::iterator it = _imp->nodeLinks.begin(); it != _imp->nodeLinks.end(); ++it) {
            if (it->masterNode.lock() == parentNode) {
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
    if (!_imp->effect) {
        return;
    }
    _imp->effect->unslaveAllKnobs();
}

NodePtr
Node::getMasterNode() const
{
    QMutexLocker l(&_imp->masterNodeMutex);
    
    return _imp->masterNode.lock();
}

bool
Node::isSupportedComponent(int inputNb,
                           const ImageComponents& comp) const
{
    QMutexLocker l(&_imp->inputsMutex);
    
    if (inputNb >= 0) {
        assert( inputNb < (int)_imp->inputsComponents.size() );
        std::list<ImageComponents>::const_iterator found =
        std::find(_imp->inputsComponents[inputNb].begin(),_imp->inputsComponents[inputNb].end(),comp);
        
        return found != _imp->inputsComponents[inputNb].end();
    } else {
        assert(inputNb == -1);
        std::list<ImageComponents>::const_iterator found =
        std::find(_imp->outputComponents.begin(),_imp->outputComponents.end(),comp);
        
        return found != _imp->outputComponents.end();
    }
}

ImageComponents
Node::findClosestInList(const ImageComponents& comp,
                        const std::list<ImageComponents> &components,
                        bool multiPlanar)
{
    if ( components.empty() ) {
        return ImageComponents::getNoneComponents();
    }
    std::list<ImageComponents>::const_iterator closestComp = components.end();
    for (std::list<ImageComponents>::const_iterator it = components.begin(); it != components.end(); ++it) {
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
                if (diff > 0 && diff < diffSoFar) {
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

ImageComponents
Node::findClosestSupportedComponents(int inputNb,
                                     const ImageComponents& comp) const
{
    std::list<ImageComponents> comps;
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
    return findClosestInList(comp, comps, _imp->effect->isMultiPlanar());
}



int
Node::getMaskChannel(int inputNb,ImageComponents* comps,NodePtr* maskInput) const
{
    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);
    if ( it != _imp->maskSelectors.end() ) {
        int index =  it->second.channel.lock()->getValue();
        if (index == 0) {
            *comps = ImageComponents::getNoneComponents();
            maskInput->reset();
            return -1;
        } else {
            index -= 1; // None choice
            QMutexLocker locker(&it->second.compsMutex);
            int k = 0;
            for (std::size_t i = 0; i < it->second.compsAvailable.size(); ++i) {
                if (index >= k && index < (it->second.compsAvailable[i].first.getNumComponents() + k)) {
                    int compIndex = index - k;
                    assert(compIndex >= 0 && compIndex <= 3);
                    *comps = it->second.compsAvailable[i].first;
                    *maskInput = it->second.compsAvailable[i].second.lock();
                    return compIndex;
                }
                k += it->second.compsAvailable[i].first.getNumComponents();
            }
            
        }
        //Default to alpha
        *comps = ImageComponents::getAlphaComponents();
        return 0;
    }
    return -1;
    
}

bool
Node::isMaskEnabled(int inputNb) const
{
    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);
    if ( it != _imp->maskSelectors.end() ) {
        return it->second.enabled.lock()->getValue();
    } else {
        return true;
    }
}

void
Node::lock(const boost::shared_ptr<Image> & image)
{
    
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<boost::shared_ptr<Image> >::iterator it =
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
Node::tryLock(const boost::shared_ptr<Image> & image)
{
    
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<boost::shared_ptr<Image> >::iterator it =
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
Node::unlock(const boost::shared_ptr<Image> & image)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<boost::shared_ptr<Image> >::iterator it =
    std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    ///The image must exist, otherwise this is a bug
    assert( it != _imp->imagesBeingRendered.end() );
    _imp->imagesBeingRendered.erase(it);
    ///Notify all waiting threads that we're finished
    _imp->imageBeingRenderedCond.wakeAll();
}

boost::shared_ptr<Image>
Node::getImageBeingRendered(double time,
                            unsigned int mipMapLevel,
                            int view)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    for (std::list<boost::shared_ptr<Image> >::iterator it = _imp->imagesBeingRendered.begin();
         it != _imp->imagesBeingRendered.end(); ++it) {
        const ImageKey &key = (*it)->getKey();
        if ( (key._view == view) && ((*it)->getMipMapLevel() == mipMapLevel) && (key._time == time) ) {
            return *it;
        }
    }
    return boost::shared_ptr<Image>();
}

void
Node::beginInputEdition()
{
    assert( QThread::currentThread() == qApp->thread() );
    ++_imp->inputModifiedRecursion;
}

void
Node::endInputEdition(bool triggerRender)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->inputModifiedRecursion > 0) {
        --_imp->inputModifiedRecursion;
    }
    
    if (!_imp->inputModifiedRecursion) {
        
        bool hasChanged = !_imp->inputsModified.empty();
        _imp->inputsModified.clear();

        if (hasChanged) {
            if (!getApp()->isCreatingNodeTree()) {
                forceRefreshAllInputRelatedData();
            }
            refreshDynamicProperties();
        }
        
        triggerRender = triggerRender && hasChanged;
        
        if (triggerRender) {
            std::list<ViewerInstance* > viewers;
            hasViewersConnected(&viewers);
            for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
                (*it2)->renderCurrentFrame(true);
            }
        }
    }
}

void
Node::onInputChanged(int inputNb)
{
    if (getApp()->getProject()->isProjectClosing()) {
        return;
    }
    assert( QThread::currentThread() == qApp->thread() );

    bool mustCallEndInputEdition = _imp->inputModifiedRecursion == 0;
    if (mustCallEndInputEdition) {
        beginInputEdition();
    }
    
    refreshMaskEnabledNess(inputNb);
    refreshLayersChoiceSecretness(inputNb);
    
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(_imp->effect.get());
    if (isViewer) {
        isViewer->refreshActiveInputs(inputNb);
    }
    
    bool shouldDoInputChanged = (!getApp()->getProject()->isProjectClosing() && !getApp()->isCreatingNodeTree()) ||
    _imp->effect->isRotoPaintNode();
    
    if (shouldDoInputChanged) {
  
        ///When loading a group (or project) just wait until everything is setup to actually compute input
        ///related data such as clip preferences
        ///Exception for the Rotopaint node which needs to setup its own graph internally
        
        /**
         * The plug-in might call getImage, set a valid thread storage on the tree.
         **/
        double time = getApp()->getTimeLine()->currentFrame();
        ParallelRenderArgsSetter frameRenderArgs(time,
                                                 0 /*view*/,
                                                 true,
                                                 false,
                                                 false,
                                                 0,
                                                 shared_from_this(),
                                                 0,
                                                 0, //texture index
                                                 getApp()->getTimeLine().get(),
                                                 NodePtr(),
                                                 false,
                                                 false,
                                                 false,
                                                 boost::shared_ptr<RenderStats>());
        
        
        ///Don't do clip preferences while loading a project, they will be refreshed globally once the project is loaded.
        _imp->effect->onInputChanged(inputNb);
        _imp->inputsModified.insert(inputNb);
        
        //A knob value might have changed recursively, redraw  any overlay
        if (!_imp->effect->isDequeueingValuesSet() &&
            _imp->effect->getRecursionLevel() == 0 && _imp->effect->checkIfOverlayRedrawNeeded()) {
            _imp->effect->redrawOverlayInteract();
        }
    }
   
    /*
     If this is a group, also notify the output nodes of the GroupInput node inside the Group corresponding to
     the this inputNb
     */
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->effect.get());
    if (isGroup) {
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs, false);
        if (inputNb >= 0 && inputNb < (int)groupInputs.size() && groupInputs[inputNb]) {
            std::map<NodePtr,int> inputOutputs;
            groupInputs[inputNb]->getOutputsConnectedToThisNode(&inputOutputs);
            for (std::map<NodePtr,int> ::iterator it = inputOutputs.begin(); it!=inputOutputs.end(); ++it) {
                it->first->onInputChanged(it->second);
            }
        }
    }
    
    /*
     If this is an output node, notify the Group output nodes that their input have changed.
     */
    GroupOutput* isOutput = dynamic_cast<GroupOutput*>(_imp->effect.get());
    if (isOutput) {
        NodeGroup* containerGroup = dynamic_cast<NodeGroup*>(isOutput->getNode()->getGroup().get());
        if (containerGroup) {
            std::map<NodePtr,int> groupOutputs;
            containerGroup->getNode()->getOutputsConnectedToThisNode(&groupOutputs);
            for (std::map<NodePtr,int> ::iterator it = groupOutputs.begin(); it!=groupOutputs.end(); ++it) {
                it->first->onInputChanged(it->second);
            }
        }
    }
    
    /*
     * If this node is the output of a pre-comp, notify the precomp output nodes that their input have changed
     */
    boost::shared_ptr<PrecompNode> isInPrecomp = isPartOfPrecomp();
    if (isInPrecomp && isInPrecomp->getOutputNode().get() == this) {
        std::map<NodePtr,int> inputOutputs;
        isInPrecomp->getNode()->getOutputsConnectedToThisNode(&inputOutputs);
        for (std::map<NodePtr,int> ::iterator it = inputOutputs.begin(); it!=inputOutputs.end(); ++it) {
            it->first->onInputChanged(it->second);
        }

    }
    
    if (mustCallEndInputEdition) {
        endInputEdition(true);
    }
   
}

void
Node::onParentMultiInstanceInputChanged(int input)
{
    ++_imp->inputModifiedRecursion;
    _imp->effect->onInputChanged(input);
    --_imp->inputModifiedRecursion;
}


bool
Node::duringInputChangedAction() const
{
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->inputModifiedRecursion > 0;
}

void
Node::computeFrameRangeForReader(const KnobI* fileKnob)
{
    int leftBound = INT_MIN;
    int rightBound = INT_MAX;
    ///Set the originalFrameRange parameter of the reader if it has one.
    KnobPtr knob = getKnobByName("originalFrameRange");
    if (knob) {
        KnobInt* originalFrameRange = dynamic_cast<KnobInt*>(knob.get());
        if (originalFrameRange && originalFrameRange->getDimension() == 2) {
            
            const KnobFile* isFile = dynamic_cast<const KnobFile*>(fileKnob);
            assert(isFile);
            if (!isFile) {
                throw std::logic_error("Node::computeFrameRangeForReader");
            }
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
Node::canHandleRenderScaleForOverlays() const
{
    return _imp->effect->canHandleRenderScaleForOverlays();
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
Node::drawHostOverlay(double time, const RenderScale & renderScale)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        nodeGui->drawHostOverlay(time, renderScale);
    }
}

bool
Node::onOverlayPenDownDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayPenDownDefault(renderScale, viewportPos, pos, pressure);
    }
    return false;
}

bool
Node::onOverlayPenMotionDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayPenMotionDefault(renderScale, viewportPos, pos, pressure);
    }
    return false;
}

bool
Node::onOverlayPenUpDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayPenUpDefault(renderScale, viewportPos, pos, pressure);
    }
    return false;
}

bool
Node::onOverlayKeyDownDefault(const RenderScale & renderScale, Key key, KeyboardModifiers modifiers)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayKeyDownDefault(renderScale, key, modifiers);
    }
    return false;
}

bool
Node::onOverlayKeyUpDefault(const RenderScale & renderScale, Key key, KeyboardModifiers modifiers)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayKeyUpDefault(renderScale, key, modifiers);
    }
    return false;
}

bool
Node::onOverlayKeyRepeatDefault(const RenderScale & renderScale, Key key, KeyboardModifiers modifiers)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayKeyRepeatDefault(renderScale, key, modifiers);
    }
    return false;
}

bool
Node::onOverlayFocusGainedDefault(const RenderScale & renderScale)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayFocusGainedDefault(renderScale);
    }
    return false;
}

bool
Node::onOverlayFocusLostDefault(const RenderScale & renderScale)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        return nodeGui->onOverlayFocusLostDefault(renderScale);
    }
    return false;
}

void
Node::removePositionHostOverlay(KnobI* knob)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui) {
        nodeGui->removePositionHostOverlay(knob);
    }
}

void
Node::addDefaultPositionOverlay(const boost::shared_ptr<KnobDouble>& position)
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
Node::addTransformInteract(const boost::shared_ptr<KnobDouble>& translate,
                          const boost::shared_ptr<KnobDouble>& scale,
                          const boost::shared_ptr<KnobBool>& scaleUniform,
                          const boost::shared_ptr<KnobDouble>& rotate,
                          const boost::shared_ptr<KnobDouble>& skewX,
                          const boost::shared_ptr<KnobDouble>& skewY,
                          const boost::shared_ptr<KnobChoice>& skewOrder,
                          const boost::shared_ptr<KnobDouble>& center)
{
    assert(QThread::currentThread() == qApp->thread());
    if (appPTR->isBackground()) {
        return;
    }
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (!nodeGui) {
        NativeTransformOverlayKnobs t;
        t.translate = translate;
        t.scale = scale;
        t.scaleUniform = scaleUniform;
        t.rotate = rotate;
        t.skewX = skewX;
        t.skewY = skewY;
        t.skewOrder = skewOrder;
        t.center = center;
        _imp->nativeTransformOverlays.push_back(t);
    } else {
        nodeGui->addTransformInteract(translate, scale, scaleUniform, rotate, skewX, skewY, skewOrder, center);
    }

}

void
Node::initializeHostOverlays()
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (!nodeGui) {
        return;
    }
    for (std::list<boost::shared_ptr<KnobDouble> > ::iterator it = _imp->nativePositionOverlays.begin(); it != _imp->nativePositionOverlays.end(); ++it)
    {
        nodeGui->addDefaultPositionInteract(*it);
    }
    _imp->nativePositionOverlays.clear();
    for (std::list<NativeTransformOverlayKnobs> ::iterator it = _imp->nativeTransformOverlays.begin(); it != _imp->nativeTransformOverlays.end(); ++it)
    {
        nodeGui->addTransformInteract(it->translate, it->scale, it->scaleUniform, it->rotate, it->skewX, it->skewY, it->skewOrder, it->center);
    }
    _imp->nativeTransformOverlays.clear();
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
    
    assert(QThread::currentThread() == qApp->thread());
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (!nodeGui) {
        return;
    }
    
    {
        QMutexLocker k(&_imp->pluginPythonModuleMutex);
        _imp->pyPlugVersion = version;
        _imp->pyPlugID = pluginID;
        _imp->pyPlugLabel = pluginLabel;
    }
    
    nodeGui->setPluginIDAndVersion(pluginLabel,pluginID, version);
    
}

bool
Node::hasPyPlugBeenEdited() const
{
    QMutexLocker k(&_imp->pluginPythonModuleMutex);
    return _imp->pyplugChangedSinceScript || _imp->pluginPythonModule.empty();
}

void
Node::setPyPlugEdited(bool edited)
{
    QMutexLocker k(&_imp->pluginPythonModuleMutex);
    _imp->pyplugChangedSinceScript = edited;
    _imp->pyPlugID.clear();
    _imp->pyPlugLabel.clear();
    _imp->pyPlugDesc.clear();
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
Node::hasHostOverlayForParam(const KnobI* knob) const
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui && nodeGui->hasHostOverlayForParam(knob)) {
        return true;
    }
    return false;

}

bool
Node::hasHostOverlay() const
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui && nodeGui->hasHostOverlay()) {
        return true;
    }
    return false;
}

void
Node::setCurrentViewportForHostOverlays(OverlaySupport* viewPort)
{
    boost::shared_ptr<NodeGuiI> nodeGui = getNodeGui();
    if (nodeGui && nodeGui->hasHostOverlay()) {
        nodeGui->setCurrentViewportForHostOverlays(viewPort);
    }
}

const std::vector<std::string>&
Node::getCreatedViews() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->createdViews;
}

void
Node::refreshCreatedViews()
{
    KnobPtr knob = getKnobByName(kReadOIIOAvailableViewsKnobName);
    if (knob) {
        refreshCreatedViews(knob.get());
    }
}

void
Node::refreshCreatedViews(KnobI* knob)
{
    assert(QThread::currentThread() == qApp->thread());
    
    KnobString* availableViewsKnob = dynamic_cast<KnobString*>(knob);
    if (!availableViewsKnob) {
        return;
    }
    QString value(availableViewsKnob->getValue().c_str());
    QStringList views = value.split(',');
    
    _imp->createdViews.clear();
    
    const std::vector<std::string>& projectViews = getApp()->getProject()->getProjectViewNames();
    QStringList qProjectViews;
    for (std::size_t i = 0; i < projectViews.size(); ++i) {
        qProjectViews.push_back(projectViews[i].c_str());
    }
    
    QStringList missingViews;
    for (QStringList::Iterator it = views.begin(); it!=views.end(); ++it) {
        if (!qProjectViews.contains(*it,Qt::CaseInsensitive) && !it->isEmpty()) {
            missingViews.push_back(*it);
        }
        _imp->createdViews.push_back(it->toStdString());
    }
    
    if (!missingViews.isEmpty()) {
        
        
        KnobPtr fileKnob = getKnobByName(kOfxImageEffectFileParamName);
        KnobFile* inputFileKnob = dynamic_cast<KnobFile*>(fileKnob.get());
        if (inputFileKnob) {
            
            std::string filename = inputFileKnob->getValue();
            
            std::stringstream ss;
            for (int i = 0; i < missingViews.size(); ++i) {
                ss << missingViews[i].toStdString();
                if (i < missingViews.size() - 1) {
                    ss << ", ";
                }
            }
            ss << std::endl;
            ss << std::endl;
            ss << QObject::tr("These views are in").toStdString() << ' ' << filename << ' '
            << QObject::tr("but do not exist in the project.").toStdString() << std::endl;
            ss << QObject::tr("Would you like to create them?").toStdString();
            std::string question  = ss.str();
            StandardButtonEnum rep = Dialogs::questionDialog("Views available", question, false, StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonYes);
            if (rep == eStandardButtonYes) {
                std::vector<std::string> viewsToCreate;
                for (QStringList::Iterator it = missingViews.begin(); it!=missingViews.end(); ++it) {
                    viewsToCreate.push_back(it->toStdString());
                }
                getApp()->getProject()->createProjectViews(viewsToCreate);
            }
        }
        
     
    }
    
    Q_EMIT availableViewsChanged();
    
}

bool
Node::getHideInputsKnobValue() const
{
    boost::shared_ptr<KnobBool> k = _imp->hideInputs.lock();
    if (!k) {
        return false;
    }
    return k->getValue();
}

void
Node::setHideInputsKnobValue(bool hidden)
{
    boost::shared_ptr<KnobBool> k = _imp->hideInputs.lock();
    if (!k) {
        return;
    }
    k->setValue(hidden,0);
}

void
Node::onRefreshIdentityStateRequestReceived()
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->refreshIdentityStateRequestsCount == 0 || !_imp->effect) {
        //was already processed
        return;
    }
    _imp->refreshIdentityStateRequestsCount = 0;
    
    boost::shared_ptr<Project> project = getApp()->getProject();
    double time = project->currentFrame();
    RenderScale scale(1.);
    
    double inputTime = 0;
    
    U64 hash = getHashValue();
    
    bool viewAware =  _imp->effect->isViewAware();
    
    
    int nViews = !viewAware ? 1 : project->getProjectViewsCount();
    
    Format frmt;
    project->getProjectDefaultFormat(&frmt);
    
    bool isIdentity = false;
    int inputNb = -1;
    for (int i = 0; i < nViews; ++i) {
        int identityInputNb = -1;
        bool isIdentityView = _imp->effect->isIdentity_public(true, hash, time, scale, frmt, i, &inputTime, &identityInputNb);
        if (i > 0 && (isIdentityView != isIdentity || identityInputNb != inputNb)) {
            isIdentity = false;
            inputNb = -1;
            break;
        }
        isIdentity |= isIdentityView;
        inputNb = identityInputNb;
        if (!isIdentity) {
            break;
        }
        
    }
    
   
    //Check for consistency across views or then say the effect is not identity since the UI cannot display 2 different states
    //depending on the view
    
    
    boost::shared_ptr<NodeGuiI> nodeUi = _imp->guiPointer.lock();
    assert(nodeUi);
    nodeUi->onIdentityStateChanged(isIdentity ? inputNb : -1);

}

void
Node::refreshIdentityState()
{
    assert(QThread::currentThread() == qApp->thread());
    
    if (!_imp->guiPointer.lock()) {
        return;
    }
    
    //Post a new request
    ++_imp->refreshIdentityStateRequestsCount;
    Q_EMIT refreshIdentityStateRequested();
}

/*
 This is called AFTER the instanceChanged action has been called on the plug-in
 */
void
Node::onEffectKnobValueChanged(KnobI* what,
                               ValueChangedReasonEnum reason)
{
    if (!what) {
        return;
    }
    for (std::map<int, MaskSelector >::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        if (it->second.channel.lock().get() == what) {
            _imp->onMaskSelectorChanged(it->first, it->second);
            break;
        }
    }
    
    if ( what == _imp->previewEnabledKnob.lock().get() ) {
        if ( (reason == eValueChangedReasonUserEdited) || (reason == eValueChangedReasonSlaveRefresh) ) {
            Q_EMIT previewKnobToggled();
        }
    } else if ( ( what == _imp->disableNodeKnob.lock().get() ) && !_imp->isMultiInstance && !_imp->multiInstanceParent.lock() ) {
        Q_EMIT disabledKnobToggled( _imp->disableNodeKnob.lock()->getValue() );
        getApp()->redrawAllViewers();
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->effect.get());
        if (isGroup) {
            ///When a group is disabled we have to force a hash change of all nodes inside otherwise the image will stay cached
            
            NodesList nodes = isGroup->getNodes();
            std::list<Node*> markedNodes;
            for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                //This will not trigger a hash recomputation
                (*it)->incrementKnobsAge_internal();
                (*it)->computeHashRecursive(markedNodes);
            }
        }
        
    } else if ( what == _imp->nodeLabelKnob.lock().get() ) {
        Q_EMIT nodeExtraLabelChanged( _imp->nodeLabelKnob.lock()->getValue().c_str() );
    } else if (what->getName() == kNatronOfxParamStringSublabelName) {
        //special hack for the merge node and others so we can retrieve the sublabel and display it in the node's label
        KnobString* strKnob = dynamic_cast<KnobString*>(what);
        if (strKnob) {
            QString operation = strKnob->getValue().c_str();
            if (!operation.isEmpty()) {
                operation.prepend("(");
                operation.append(")");
            }
            replaceCustomDataInlabel(operation);
        }
    } else if (what == _imp->hideInputs.lock().get()) {
        Q_EMIT hideInputsKnobChanged(_imp->hideInputs.lock()->getValue());
    } else if (what->getName() == kOfxImageEffectFileParamName && reason != eValueChangedReasonTimeChanged) {
        
        if (_imp->effect->isReader()) {
            ///Refresh the preview automatically if the filename changed
            incrementKnobsAge(); //< since evaluate() is called after knobChanged we have to do this  by hand
            //computePreviewImage( getApp()->getTimeLine()->currentFrame() );
            
            ///union the project frame range if not locked with the reader frame range
            bool isLocked = getApp()->getProject()->isFrameRangeLocked();
            if (!isLocked) {
                double leftBound = INT_MIN,rightBound = INT_MAX;
                _imp->effect->getFrameRange_public(getHashValue(), &leftBound, &rightBound, true);
                
                if (leftBound != INT_MIN && rightBound != INT_MAX) {
                    bool isFileDialogPreviewReader = getScriptName().find(NATRON_FILE_DIALOG_PREVIEW_READER_NAME) != std::string::npos;
                    if (!isFileDialogPreviewReader) {
                        getApp()->getProject()->unionFrameRangeWith(leftBound, rightBound);
                    }
                }
            }
        } else if (_imp->effect->isWriter()) {
            /*
             Check if the filename param has a %V in it, in which case make sure to hide the Views parameter
             */
            KnobOutputFile* fileParam = dynamic_cast<KnobOutputFile*>(what);
            if (fileParam) {
                std::string pattern = fileParam->getValue();
                std::size_t foundViewPattern = pattern.find_first_of("%v");
                if (foundViewPattern == std::string::npos) {
                    foundViewPattern = pattern.find_first_of("%V");
                }
                if (foundViewPattern != std::string::npos) {
                    //We found view pattern
                    KnobPtr viewsKnob = getKnobByName(kWriteOIIOParamViewsSelector);
                    if (viewsKnob) {
                        KnobChoice* viewsSelector = dynamic_cast<KnobChoice*>(viewsKnob.get());
                        if (viewsSelector) {
                            viewsSelector->setSecret(true);
                        }
                    }
                }
            }
        }
    } else if (_imp->effect->isReader() && what->getName() == kReadOIIOAvailableViewsKnobName) {
        refreshCreatedViews(what);
    } else if ( what == _imp->refreshInfoButton.lock().get() ) {
        int maxinputs = getMaxInputCount();
        std::stringstream ssinfo;
        for (int i = 0; i < maxinputs; ++i) {
            std::string inputInfo = makeInfoForInput(i);
            if (!inputInfo.empty()) {
                ssinfo << inputInfo << "<br/>";
            }
        }
        std::string outputInfo = makeInfoForInput(-1);
        ssinfo << outputInfo << "<br/>";
        std::string cacheInfo = makeCacheInfo();
        ssinfo << cacheInfo << "<br/>";
        _imp->nodeInfos.lock()->setValue(ssinfo.str(), 0);
    }
    
    for (std::map<int,ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
        if (it->second.layer.lock().get() == what) {
            _imp->onLayerChanged(it->first, it->second);
        }
    }
    
    GroupInput* isInput = dynamic_cast<GroupInput*>(_imp->effect.get());
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
Node::getSelectedLayerChoiceRaw(int inputNb,std::string& layer) const
{
    std::map<int,ChannelSelector>::iterator found = _imp->channelsSelectors.find(inputNb);
    if (found == _imp->channelsSelectors.end()) {
        return false;
    }
    boost::shared_ptr<KnobChoice> layerKnob = found->second.layer.lock();
    layer = layerKnob->getActiveEntryText_mt_safe();
    return true;
}

bool
Node::Implementation::getSelectedLayerInternal(int inputNb,const ChannelSelector& selector, ImageComponents* comp) const
{
    Node* node = 0;
    if (inputNb == -1) {
        node = _publicInterface;
    } else {
        node = _publicInterface->getInput(inputNb).get();
    }
    
    boost::shared_ptr<KnobChoice> layerKnob = selector.layer.lock();
    assert(layerKnob);
    int index = layerKnob->getValue();
    std::vector<std::string> entries = layerKnob->getEntries_mt_safe();
    if (entries.empty()) {
        return false;
    }
    if (index < 0 || index >= (int)entries.size()) {
        return false;
    }
    const std::string& layer = entries[index];
    if (layer == "All") {
        return false;
    }
    std::string mappedLayerName = ImageComponents::mapUserFriendlyPlaneNameToNatronInternalPlaneName(layer);
    
    bool isCurLayerColorComp = mappedLayerName == kNatronRGBAComponentsName || mappedLayerName == kNatronRGBComponentsName || mappedLayerName == kNatronAlphaComponentsName;
    
    EffectInstance::ComponentsAvailableMap compsAvailable;
    {
        QMutexLocker k(&selector.compsMutex);
        compsAvailable = selector.compsAvailable;
    }
    if (node) {
        for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2!= compsAvailable.end(); ++it2) {
            if (it2->first.isColorPlane()) {
                if (isCurLayerColorComp) {
                    *comp = it2->first;
                    break;
                    
                }
            } else {
                if (it2->first.getLayerName() == mappedLayerName) {
                    *comp = it2->first;
                    break;
                    
                }
            }
        }
    }
    if (comp->getNumComponents() == 0) {
        if (mappedLayerName == kNatronRGBAComponentsName) {
            *comp = ImageComponents::getRGBAComponents();
        } else if (mappedLayerName == kNatronDisparityLeftPlaneName) {
            *comp = ImageComponents::getDisparityLeftComponents();
        } else if (mappedLayerName == kNatronDisparityRightPlaneName) {
            *comp = ImageComponents::getDisparityRightComponents();
        } else if (mappedLayerName == kNatronBackwardMotionVectorsPlaneName) {
            *comp = ImageComponents::getBackwardMotionComponents();
        } else if (mappedLayerName == kNatronForwardMotionVectorsPlaneName) {
            *comp = ImageComponents::getForwardMotionComponents();
        }
    }
    return true;
    
    
}

void
Node::Implementation::onLayerChanged(int inputNb,const ChannelSelector& selector)
{
    
    boost::shared_ptr<KnobChoice> layerKnob = selector.layer.lock();
    std::vector<std::string> entries = layerKnob->getEntries_mt_safe();
    int curLayer_i = layerKnob->getValue();
    assert(curLayer_i >= 0 && curLayer_i < (int)entries.size());
    selector.layerName.lock()->setValue(entries[curLayer_i], 0);
    
    if (inputNb == -1) {
        bool outputIsAll = entries[curLayer_i] == "All";
        
        ///Disable all input selectors as it doesn't make sense to edit them whilst output is All
        for (std::map<int,ChannelSelector>::iterator it = channelsSelectors.begin(); it != channelsSelectors.end(); ++it) {
            if (it->first >= 0) {
                NodePtr inp = _publicInterface->getInput(it->first);
                bool mustBeSecret = !inp.get() || outputIsAll;
                it->second.layer.lock()->setSecret(mustBeSecret);
            }
        }
       
    }
    {
        ///Clip preferences have changed
        RenderScale s(1.);
        effect->refreshClipPreferences_public(_publicInterface->getApp()->getTimeLine()->currentFrame(),
                                                     s,
                                                     eValueChangedReasonUserEdited,
                                                     true, true);
    }
    if (!enabledChan[0].lock()) {
        return;
    }
    
    ImageComponents comp ;
    if (!getSelectedLayerInternal(inputNb, selector, &comp)) {
        for (int i = 0; i < 4; ++i) {
            enabledChan[i].lock()->setSecret(true);
        }

    } else {
        _publicInterface->refreshEnabledKnobsLabel(comp);
    }
    
    if (inputNb == -1) {
        _publicInterface->s_outputLayerChanged();
    }
}

void
Node::refreshEnabledKnobsLabel(const ImageComponents& comp)
{
    const std::vector<std::string>& channels = comp.getComponentsNames();
    switch (channels.size()) {
        
        case 1:
        {
            for (int i = 0; i < 3; ++i) {
                boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
                enabled->setSecret(true);
            }
            boost::shared_ptr<KnobBool> alpha = _imp->enabledChan[3].lock();
            alpha->setSecret(false);
            alpha->setLabel(channels[0]);
        } break;
        case 2:
        {
            for (int i = 2; i < 4; ++i) {
                boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
                enabled->setSecret(true);
            }
            for (int i = 0; i < 2; ++i) {
                boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
                enabled->setSecret(false);
                enabled->setLabel(channels[i]);
            }

            
        } break;
        case 3:
        {
            for (int i = 3; i < 4; ++i) {
                boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
                enabled->setSecret(true);
            }
            for (int i = 0; i < 3; ++i) {
                boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
                enabled->setSecret(false);
                enabled->setLabel(channels[i]);
            }
        } break;
        case 4:
        {
         
            for (int i = 0; i < 4; ++i) {
                boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
                enabled->setSecret(false);
                enabled->setLabel(channels[i]);
            }
        } break;
            
        case 0:
        default:
        {
            for (int i = 0; i < 4; ++i) {
                boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
                enabled->setSecret(true);
            }
        } break;
    }

}

void
Node::Implementation::onMaskSelectorChanged(int inputNb,const MaskSelector& selector)
{
    
    boost::shared_ptr<KnobChoice> channel = selector.channel.lock();
    int index = channel->getValue();
    boost::shared_ptr<KnobBool> enabled = selector.enabled.lock();
    if ( (index == 0) && enabled->isEnabled(0) ) {
        enabled->setValue(false, 0);
        enabled->setEnabled(0, false);
    } else if ( !enabled->isEnabled(0) ) {
        enabled->setEnabled(0, true);
        if ( _publicInterface->getInput(inputNb) ) {
            enabled->setValue(true, 0);
        }
    }
    
    std::vector<std::string> entries = channel->getEntries_mt_safe();
    int curChan_i = channel->getValue();
    if (curChan_i < 0 || curChan_i >= (int)entries.size()) {
        _publicInterface->refreshChannelSelectors();
        return;
    }
    selector.channelName.lock()->setValue(entries[curChan_i], 0);
    {
        ///Clip preferences have changed
        RenderScale s(1.);
        effect->refreshClipPreferences_public(_publicInterface->getApp()->getTimeLine()->currentFrame(),
                                                     s,
                                                     eValueChangedReasonUserEdited,
                                                     true, true);
    }
}

bool
Node::getProcessChannel(int channelIndex) const
{
    assert(channelIndex >= 0 && channelIndex < 4);
    boost::shared_ptr<KnobBool> k = _imp->enabledChan[channelIndex].lock();
    if (k) {
        return k->getValue();
    }
    return true;
}

bool
Node::getSelectedLayer(int inputNb,
                       std::bitset<4> *processChannels,
                       bool* isAll,
                       ImageComponents* layer) const
{
    //If the effect is multi-planar, it is expected to handle itself all the planes
    assert(!_imp->effect->isMultiPlanar());
    
    std::map<int,ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(inputNb);
    NodePtr maskInput;
    int chanIndex = getMaskChannel(inputNb,layer,&maskInput);
    bool hasChannelSelector = true;
    if (chanIndex != -1) {
        
        *isAll = false;
        Q_UNUSED(chanIndex);
        (*processChannels)[0] = true;
        (*processChannels)[1] = true;
        (*processChannels)[2] = true;
        (*processChannels)[3] = true;
        
        return true;
    } else {
        if (foundSelector == _imp->channelsSelectors.end()) {
            //Fetch in input what the user has set for the output
            foundSelector = _imp->channelsSelectors.find(-1);
        }
        if (foundSelector == _imp->channelsSelectors.end()) {
            hasChannelSelector = false;
        }
    }
    if (hasChannelSelector) {
        *isAll = !_imp->getSelectedLayerInternal(inputNb, foundSelector->second, layer);
    } else {
        *isAll = false;
    }
    if (_imp->enabledChan[0].lock()) {
        (*processChannels)[0] = _imp->enabledChan[0].lock()->getValue();
        (*processChannels)[1] = _imp->enabledChan[1].lock()->getValue();
        (*processChannels)[2] = _imp->enabledChan[2].lock()->getValue();
        (*processChannels)[3] = _imp->enabledChan[3].lock()->getValue();
    } else {
        (*processChannels)[0] = true;
        (*processChannels)[1] = true;
        (*processChannels)[2] = true;
        (*processChannels)[3] = true;
    }
 
    return hasChannelSelector;
}

bool
Node::hasAtLeastOneChannelToProcess() const
{
    std::map<int,ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(-1);
    if (foundSelector == _imp->channelsSelectors.end()) {
        return true;
    }
    if (_imp->enabledChan[0].lock()) {
        std::bitset<4> processChannels;
        processChannels[0] = _imp->enabledChan[0].lock()->getValue();
        processChannels[1] = _imp->enabledChan[1].lock()->getValue();
        processChannels[2] = _imp->enabledChan[2].lock()->getValue();
        processChannels[3] = _imp->enabledChan[3].lock()->getValue();
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
    boost::shared_ptr<KnobString> labelKnob = _imp->nodeLabelKnob.lock();
    if (!labelKnob) {
        return;
    }
    QString label = labelKnob->getValue().c_str();
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
    labelKnob->setValue(label.toStdString(), 0);
}

boost::shared_ptr<KnobBool>
Node::getDisabledKnob() const
{
    return _imp->disableNodeKnob.lock();
}

bool
Node::isNodeDisabled() const
{
    boost::shared_ptr<KnobBool> b = _imp->disableNodeKnob.lock();
    bool thisDisabled = b ? b->getValue() : false;
    NodeGroup* isContainerGrp = dynamic_cast<NodeGroup*>(getGroup().get());
    if (isContainerGrp) {
        return thisDisabled || isContainerGrp->getNode()->isNodeDisabled();
    }
    return thisDisabled;
}

void
Node::setNodeDisabled(bool disabled)
{
    boost::shared_ptr<KnobBool> b = _imp->disableNodeKnob.lock();
    if (b) {
        b->setValue(disabled, 0);
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
    getApp()->addMultipleKeyframeIndicatorsAdded(keys, emitSignal);
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
    getApp()->removeMultipleKeyframeIndicator(keys, emitSignal);
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
    const KnobsVec & knobs = getKnobs();
    
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->getIsSecret() || !knobs[i]->getIsPersistant()) {
            continue;
        }
        if (!knobs[i]->canAnimate()) {
            continue;
        }
        int dim = knobs[i]->getDimension();
        KnobFile* isFile = dynamic_cast<KnobFile*>( knobs[i].get() );
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

ImageBitDepthEnum
Node::getClosestSupportedBitDepth(ImageBitDepthEnum depth)
{
    bool foundShort = false;
    bool foundByte = false;
    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->supportedDepths.begin(); it != _imp->supportedDepths.end(); ++it) {
        if (*it == depth) {
            return depth;
        } else if (*it == eImageBitDepthFloat) {
            return eImageBitDepthFloat;
        } else if (*it == eImageBitDepthShort) {
            foundShort = true;
        } else if (*it == eImageBitDepthByte) {
            foundByte = true;
        }
    }
    if (foundShort) {
        return eImageBitDepthShort;
    } else if (foundByte) {
        return eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);
        
        return eImageBitDepthNone;
    }
}

ImageBitDepthEnum
Node::getBestSupportedBitDepth() const
{
    bool foundShort = false;
    bool foundByte = false;
    
    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->supportedDepths.begin(); it != _imp->supportedDepths.end(); ++it) {
        switch (*it) {
            case eImageBitDepthByte:
                foundByte = true;
                break;

            case eImageBitDepthShort:
                foundShort = true;
                break;

            case eImageBitDepthHalf:
                break;

            case eImageBitDepthFloat:
                return eImageBitDepthFloat;

            case eImageBitDepthNone:
                break;
        }
    }
    
    if (foundShort) {
        return eImageBitDepthShort;
    } else if (foundByte) {
        return eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);
        
        return eImageBitDepthNone;
    }
}

bool
Node::isSupportedBitDepth(ImageBitDepthEnum depth) const
{
    return std::find(_imp->supportedDepths.begin(), _imp->supportedDepths.end(), depth) != _imp->supportedDepths.end();
}

std::string
Node::getNodeExtraLabel() const
{
    boost::shared_ptr<KnobString> s = _imp->nodeLabelKnob.lock();
    if (s) {
        return s->getValue();
    } else {
        return std::string();
    }
}

bool
Node::hasSequentialOnlyNodeUpstream(std::string & nodeName) const
{
    ///Just take into account sequentiallity for writers
    if ( (_imp->effect->getSequentialPreference() == eSequentialPreferenceOnlySequential) && _imp->effect->isWriter() ) {
        nodeName = getScriptName_mt_safe();
        
        return true;
    } else {
        
        
        QMutexLocker l(&_imp->inputsMutex);
        
        for (InputsV::iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
            NodePtr input = it->lock();
            if (input && input->hasSequentialOnlyNodeUpstream(nodeName) && input->getEffectInstance()->isWriter() ) {
                nodeName = input->getScriptName_mt_safe();
                
                return true;
            }
        }
        
        return false;
    }
}

bool
Node::isTrackerNode() const
{
    return _imp->effect->isTrackerNode();
}

bool
Node::isPointTrackerNode() const
{
    return getPluginID() == PLUGINID_OFX_TRACKERPM;
}

bool
Node::isBackdropNode() const
{
    return getPluginID() == PLUGINID_NATRON_BACKDROP;
}

void
Node::updateEffectLabelKnob(const QString & name)
{
    if (!_imp->effect) {
        return;
    }
    KnobPtr knob = getKnobByName(kNatronOfxParamStringSublabelName);
    KnobString* strKnob = dynamic_cast<KnobString*>( knob.get() );
    if (strKnob) {
        strKnob->setValue(name.toStdString(), 0);
    }
}

bool
Node::canOthersConnectToThisNode() const
{
    if (dynamic_cast<Backdrop*>(_imp->effect.get())) {
        return false;
    } else if (dynamic_cast<GroupOutput*>(_imp->effect.get())) {
        return false;
    } else if (_imp->effect->isWriter() && _imp->effect->getSequentialPreference() == eSequentialPreferenceOnlySequential) {
        return false;
    }
    ///In debug mode only allow connections to Writer nodes
# ifdef DEBUG
    
    return dynamic_cast<const ViewerInstance*>(_imp->effect.get()) == NULL;
# else // !DEBUG
    return dynamic_cast<const ViewerInstance*>(_imp->effect.get()) == NULL/* && !_imp->effect->isWriter()*/;
# endif // !DEBUG
}

void
Node::setNodeIsRenderingInternal(std::list<Node*>& markedNodes)
{
    ///If marked, we alredy set render args
    std::list<Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), this);
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
    {
        QMutexLocker nrLocker(&_imp->nodeIsRenderingMutex);
        ++_imp->nodeIsRendering;
    }
    
    
    ///mark this
    markedNodes.push_back(this);
    
    ///Call recursively
    
    int maxInpu = getMaxInputCount();
    for (int i = 0; i < maxInpu; ++i) {
        NodePtr input = getInput(i);
        if (input) {
            input->setNodeIsRenderingInternal(markedNodes);
            
        }
    }
}

void
Node::setNodeIsNoLongerRenderingInternal(std::list<Node*>& markedNodes)
{
    
    ///If marked, we alredy set render args
    std::list<Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), this);
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

        
        
        Q_EMIT mustDequeueActions();
    }
    
    
    ///mark this
    markedNodes.push_back(this);
    
    ///Call recursively
    
    int maxInpu = getMaxInputCount();
    for (int i = 0; i < maxInpu; ++i) {
        NodePtr input = getInput(i);
        if (input) {
            input->setNodeIsNoLongerRenderingInternal(markedNodes);
            
        }
    }


}

void
Node::setNodeIsRendering()
{
    std::list<Node*> marked;
    setNodeIsRenderingInternal(marked);
}

void
Node::unsetNodeIsRendering()
{
    std::list<Node*> marked;
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
    
    ///Flag that the node is dequeuing.
    {
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        assert(!_imp->nodeIsDequeuing);
        _imp->nodeIsDequeuing = true;
    }
    bool hasChanged = false;
    if (_imp->effect) {
        hasChanged |= _imp->effect->dequeueValuesSet();
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->effect.get());
        if (isGroup) {
            isGroup->dequeueConnexions();
        }
    }
    if (_imp->rotoContext) {
        _imp->rotoContext->dequeueGuiActions();
    }

    
    std::set<int> inputChanges;
    {
        QMutexLocker k(&_imp->inputsMutex);
        assert(_imp->guiInputs.size() == _imp->inputs.size());

        for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
            
            NodePtr inp = _imp->inputs[i].lock();
            NodePtr guiInp = _imp->guiInputs[i].lock();
            if (inp != guiInp) {
                inputChanges.insert(i);
                _imp->inputs[i] = guiInp;
            }
        }
    }
    {
        QMutexLocker k(&_imp->outputsMutex);
        _imp->outputs = _imp->guiOutputs;
    }
    
    beginInputEdition();
    hasChanged |= !inputChanges.empty();
    for (std::set<int>::iterator it = inputChanges.begin(); it!=inputChanges.end(); ++it) {
        onInputChanged(*it);
    }
    endInputEdition(true);
    
    if (hasChanged) {
        refreshIdentityState();
    }

    {
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        assert(_imp->nodeIsDequeuing);
        _imp->nodeIsDequeuing = false;
        _imp->nodeIsDequeuingCond.wakeOne();
    }
}

static void addIdentityNodesRecursively(const Node* caller,
                                        const Node* node,
                                        double time,
                                        int view,
                                        std::list<const Node*>* outputs,
                                        std::list<const Node*>* markedNodes)
{
    if (std::find(markedNodes->begin(), markedNodes->end(), node) != markedNodes->end()) {
        return;
    }
    
    markedNodes->push_back(node);

    
    if (caller != node) {
        
        boost::shared_ptr<ParallelRenderArgs> inputFrameArgs = node->getEffectInstance()->getParallelRenderArgsTLS();
        const FrameViewRequest* request = 0;
        bool isIdentity = false;
        if (inputFrameArgs && inputFrameArgs->request) {
            request = inputFrameArgs->request->getFrameViewRequest(time, view);
            if (request) {
                isIdentity = request->globalData.identityInputNb != -1;
            }
        }
        
        if (!request) {
            
            /*
             Very unlikely that there's no request pass. But we still check
             */
            RenderScale scale(1.);
            double inputTimeId;
            int inputNbId;
            U64 renderHash;
            
            renderHash = node->getEffectInstance()->getRenderHash();
            
            RectD rod;
            bool isProj;
            StatusEnum stat = node->getEffectInstance()->getRegionOfDefinition_public(renderHash, time, scale, view, &rod, &isProj);
            if (stat == eStatusFailed) {
                isIdentity = false;
            } else {
                RectI pixelRod;
                rod.toPixelEnclosing(scale, node->getEffectInstance()->getPreferredAspectRatio(), &pixelRod);
                isIdentity = node->getEffectInstance()->isIdentity_public(true, renderHash, time, scale, pixelRod, view, &inputTimeId, &inputNbId);
            }
        }
        
        
        if (!isIdentity) {
            outputs->push_back(node);
            return;
        }
    }
    
    ///Append outputs of this node instead
    NodesWList nodeOutputs;
    node->getOutputs_mt_safe(nodeOutputs);
    NodesWList outputsToAdd;
    for (NodesWList::iterator it = nodeOutputs.begin(); it != nodeOutputs.end(); ++it) {
        
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        GroupOutput* isOutputNode = dynamic_cast<GroupOutput*>(output->getEffectInstance().get());
        //If the node is an output node, add all the outputs of the group node instead
        if (isOutputNode) {
            boost::shared_ptr<NodeCollection> collection = output->getGroup();
            assert(collection);
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(collection.get());
            if (isGrp) {
                NodesWList groupOutputs;
                isGrp->getNode()->getOutputs_mt_safe(groupOutputs);
                for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                    outputsToAdd.push_back(*it2);
                }
            }
        }
    }
    nodeOutputs.insert(nodeOutputs.end(), outputsToAdd.begin(),outputsToAdd.end());
    for (NodesWList::iterator it = nodeOutputs.begin(); it!=nodeOutputs.end(); ++it) {
        NodePtr node = it->lock();
        if (node) {
            addIdentityNodesRecursively(caller,node.get(),time,view,outputs, markedNodes);
        }
    }
}

bool
Node::shouldCacheOutput(bool isFrameVaryingOrAnimated, double time, int view) const
{
    /*
     * Here is a list of reasons when caching is enabled for a node:
     * - It is references multiple times below in the graph
     * - Its single output has its settings panel opened,  meaning the user is actively editing the output
     * - The force caching parameter in the "Node" tab is checked
     * - The aggressive caching preference of Natron is checked
     * - We are in a recursive action (such as an analysis)
     * - The plug-in does temporal clip access 
     * - Preview image is enabled (and Natron is not running in background)
     * - The node is a direct input of a viewer, this is to overcome linear graphs where all nodes would not be cached 
     * - The node is not frame varying, meaning it will always produce the same image at any time
     * - The node is a roto node and it is being edited
     * - The node does not support tiles
     */

    std::list<const Node*> outputs;
    {
        std::list<const Node*> markedNodes;
        addIdentityNodesRecursively(this, this, time, view,&outputs,&markedNodes);
    }

    
    std::size_t sz = outputs.size();
    if (sz > 1) {
        ///The node is referenced multiple times below, cache it
        return true;
    } else {
        if (sz == 1) {
          
            const Node* output = outputs.front();
            
            ViewerInstance* isViewer = output->isEffectViewer();
            if (isViewer) {
                int activeInputs[2];
                isViewer->getActiveInputs(activeInputs[0], activeInputs[1]);
                if (output->getInput(activeInputs[0]).get() == this ||
                    output->getInput(activeInputs[1]).get() == this) {
                    ///The node is a direct input of the viewer. Cache it because it is likely the user will make
                    ///changes to the viewer that will need this image.
                    return true;
                }
            }
        
            if (!isFrameVaryingOrAnimated) {
                //This image never changes, cache it once.
                return true;
            }
            if (output->isSettingsPanelOpened()) {
                //Output node has panel opened, meaning the user is likely to be heavily editing
                //that output node, hence requesting this node a lot. Cache it.
                return true;
            }
            if (_imp->effect->doesTemporalClipAccess()) {
                //Very heavy to compute since many frames are fetched upstream. Cache it.
                return true;
            }
            if (!_imp->effect->supportsTiles()) {
                //No tiles, image is going to be produced fully, cache it to prevent multiple access
                //with different RoIs
                return true;
            }
            if (_imp->effect->getRecursionLevel() > 0) {
                //We are in a call from getImage() and the image needs to be computed, so likely in an
                //analysis pass. Cache it because the image is likely to get asked for severla times.
                return true;
            }
            if (isForceCachingEnabled()) {
                //Users wants it cached
                return true;
            }
            NodeGroup* parentIsGroup = dynamic_cast<NodeGroup*>(getGroup().get());
            if (parentIsGroup && parentIsGroup->getNode()->isForceCachingEnabled() && parentIsGroup->getOutputNodeInput(false).get() == this) {
                //if the parent node is a group and it has its force caching enabled, cache the output of the Group Output's node input.
                return true;
            }
            
            if (appPTR->isAggressiveCachingEnabled()) {
                ///Users wants all nodes cached
                return true;
            }
            
            if (isPreviewEnabled() && !appPTR->isBackground()) {
               ///The node has a preview, meaning the image will be computed several times between previews & actual renders. Cache it.
                return true;
            }
            
            if (isRotoPaintingNode() && isSettingsPanelOpened()) {
                ///The Roto node is being edited, cache its output (special case because Roto has an internal node tree)
                return true;
            }
            
            boost::shared_ptr<RotoDrawableItem> attachedStroke = _imp->paintStroke.lock();
            if (attachedStroke && attachedStroke->getContext()->getNode()->isSettingsPanelOpened()) {
                ///Internal RotoPaint tree and the Roto node has its settings panel opened, cache it.
                return true;
            }
            
        } else {
            // outputs == 0, never cache, unless explicitly set or rotopaint internal node
            boost::shared_ptr<RotoDrawableItem> attachedStroke = _imp->paintStroke.lock();
            return isForceCachingEnabled() || appPTR->isAggressiveCachingEnabled() ||
            (attachedStroke && attachedStroke->getContext()->getNode()->isSettingsPanelOpened());
        }
    }
    
    return false;
}

bool
Node::refreshLayersChoiceSecretness(int inputNb)
{
    std::map<int,ChannelSelector>::iterator foundChan = _imp->channelsSelectors.find(inputNb);
    NodePtr inp = getInput(inputNb);
    if ( foundChan != _imp->channelsSelectors.end() ) {
        std::map<int,ChannelSelector>::iterator foundOuptut = _imp->channelsSelectors.find(-1);
        bool outputIsAll = false;
        if (foundOuptut != _imp->channelsSelectors.end()) {
            boost::shared_ptr<KnobChoice> outputChoice = foundOuptut->second.layer.lock();
            if (outputChoice) {
                outputIsAll = outputChoice->getActiveEntryText_mt_safe() == "All";
            }
        }
        boost::shared_ptr<KnobChoice> chanChoice = foundChan->second.layer.lock();
        if (chanChoice) {
            bool isSecret = chanChoice->getIsSecret();
            bool mustBeSecret = !inp.get() || outputIsAll;
            bool changed = isSecret != mustBeSecret;
            if (changed) {
                chanChoice->setSecret(mustBeSecret);
                return true;
            }
        }
    }
    return false;
}

bool
Node::refreshMaskEnabledNess(int inputNb)
{
    std::map<int,MaskSelector>::iterator found = _imp->maskSelectors.find(inputNb);
    NodePtr inp = getInput(inputNb);
    bool changed = false;
    if ( found != _imp->maskSelectors.end() ) {
        boost::shared_ptr<KnobBool> enabled = found->second.enabled.lock();
        assert(enabled);
        enabled->blockValueChanges();
        bool curValue = enabled->getValue();
        bool newValue = inp ? true : false;
        changed = curValue != newValue;
        if (changed) {
            enabled->setValue(newValue, 0);
        }
        enabled->unblockValueChanges();
    }
    return changed;
}

bool
Node::refreshDraftFlagInternal(const std::vector<NodeWPtr >& inputs)
{
    bool hasDraftInput = false;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        NodePtr input = inputs[i].lock();
        if (input) {
            hasDraftInput |= input->isDraftModeUsed();
        }
    }
    hasDraftInput |= _imp->effect->supportsRenderQuality();
    bool changed;
    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        changed = _imp->draftModeUsed != hasDraftInput;
        _imp->draftModeUsed = hasDraftInput;
    }
    return changed;
}

void
Node::refreshAllInputRelatedData(bool canChangeValues)
{
    refreshAllInputRelatedData(canChangeValues,getInputs_copy());
}

bool
Node::refreshAllInputRelatedData(bool /*canChangeValues*/,const std::vector<NodeWPtr >& inputs)
{
    bool hasChanged = false;
    hasChanged |= refreshDraftFlagInternal(inputs);

    ///if all non optional clips are connected, call getClipPrefs
    ///The clip preferences action is never called until all non optional clips have been attached to the plugin.
    if (!hasMandatoryInputDisconnected()) {
        
        
        if (getApp()->getProject()->isLoadingProject()) {
            //Nb: we clear the action cache because when creating the node many calls to getRoD and stuff might have returned
            //empty rectangles, but since we force the hash to remain what was in the project file, we might then get wrong RoDs returned
            _imp->effect->clearActionsCache();
        }
        
        double time = (double)getApp()->getTimeLine()->currentFrame();
        
        RenderScale scaleOne(1.);
        ///Render scale support might not have been set already because getRegionOfDefinition could have failed until all non optional inputs were connected
        if (_imp->effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsMaybe) {
            RectD rod;
            
            StatusEnum stat = _imp->effect->getRegionOfDefinition(getHashValue(), time, scaleOne, 0, &rod);
            if (stat != eStatusFailed) {
                RenderScale scale(0.5);
                stat = _imp->effect->getRegionOfDefinition(getHashValue(), time, scale, 0, &rod);
                if (stat != eStatusFailed) {
                    _imp->effect->setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);
                } else {
                    _imp->effect->setSupportsRenderScaleMaybe(EffectInstance::eSupportsNo);
                }
            }
            
        }
        hasChanged |= _imp->effect->refreshClipPreferences(time, scaleOne, eValueChangedReasonUserEdited, true);
    }
    
    hasChanged |= refreshChannelSelectors();
    
    refreshIdentityState();
    
    if (getApp()->getProject()->isLoadingProject()) {
        //When loading the project, refresh the hash of the nodes in a recursive manner in the proper order
        //for the disk cache to work
        hasChanged |= computeHashInternal();
    }

    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        _imp->mustComputeInputRelatedData = false;
    }
    
    return hasChanged;
}

bool
Node::refreshInputRelatedDataInternal(std::list<Node*>& markedNodes)
{
    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        if (!_imp->mustComputeInputRelatedData) {
            //We didn't change
            return false;
        }
    }
    
    std::list<Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), this);
    if (found != markedNodes.end()) {
        return false;
    }
    
    ///Check if inputs must be refreshed first
 
    int maxInputs = getMaxInputCount();
    std::vector<NodeWPtr > inputsCopy(maxInputs);
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr input = getInput(i);
        inputsCopy[i] = input;
        if (input && input->isInputRelatedDataDirty()) {
            input->refreshInputRelatedDataInternal(markedNodes);
        }
    }
    

    markedNodes.push_back(this);
    
    bool hasChanged = refreshAllInputRelatedData(false, inputsCopy);
    
    if (isRotoPaintingNode()) {
        boost::shared_ptr<RotoContext> roto = getRotoContext();
        assert(roto);
        NodePtr bottomMerge = roto->getRotoPaintBottomMergeNode();
        if (bottomMerge) {
            bottomMerge->refreshInputRelatedDataRecursiveInternal(markedNodes);
        }
        
    }
    
    return hasChanged;
}

bool
Node::isInputRelatedDataDirty() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->mustComputeInputRelatedData;
}

void
Node::forceRefreshAllInputRelatedData()
{
    markInputRelatedDataDirtyRecursive();
    
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->effect.get());
    if (isGroup) {
        NodesList inputs;
        isGroup->getInputsOutputs(&inputs, false);
        for (NodesList::iterator it = inputs.begin(); it != inputs.end(); ++it) {
            if ((*it)) {
                (*it)->refreshInputRelatedDataRecursive();
            }
        }
    } else {
        refreshInputRelatedDataRecursive();
    }
}

void
Node::markAllInputRelatedDataDirty()
{
    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        _imp->mustComputeInputRelatedData = true;
    }
    if (isRotoPaintingNode()) {
        boost::shared_ptr<RotoContext> roto = getRotoContext();
        assert(roto);
        NodesList rotoNodes;
        roto->getRotoPaintTreeNodes(&rotoNodes);
        for (NodesList::iterator it = rotoNodes.begin(); it!=rotoNodes.end(); ++it) {
            (*it)->markAllInputRelatedDataDirty();
        }
    }
    
}

void
Node::markInputRelatedDataDirtyRecursiveInternal(std::list<Node*>& markedNodes,bool recurse) {
    std::list<Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), this);
    if (found != markedNodes.end()) {
        return;
    }
    markAllInputRelatedDataDirty();
    markedNodes.push_back(this);
    if (recurse) {
        NodesList  outputs;
        getOutputsWithGroupRedirection(outputs);
        for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            (*it)->markInputRelatedDataDirtyRecursiveInternal( markedNodes, true );
        }
    }
    


}

void
Node::markInputRelatedDataDirtyRecursive()
{
    std::list<Node*> marked;
    markInputRelatedDataDirtyRecursiveInternal(marked, true);
}

void
Node::refreshInputRelatedDataRecursiveInternal(std::list<Node*>& markedNodes)
{
    if (getApp()->isCreatingNodeTree()) {
        return;
    }
    refreshInputRelatedDataInternal(markedNodes);
    
    ///Now notify outputs we have changed
    NodesList  outputs;
    getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->refreshInputRelatedDataRecursiveInternal( markedNodes );
    }
    
}

void
Node::refreshInputRelatedDataRecursive()
{
    std::list<Node*> markedNodes;
    refreshInputRelatedDataRecursiveInternal(markedNodes);
}

bool
Node::isDraftModeUsed() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->draftModeUsed;
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
Node::isUserSelected() const
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (!gui) {
        return false;
    }
    return gui->isUserSelected();
}

bool
Node::isSettingsPanelOpenedInternal(std::set<const Node*>& recursionList) const
{
    boost::shared_ptr<NodeGuiI> gui = _imp->guiPointer.lock();
    if (!gui) {
        return false;
    }
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->isSettingsPanelOpened();
    }
    
    if (recursionList.find(this) != recursionList.end()) {
        return false;
    }
    recursionList.insert(this);
    
    {
        NodePtr master = getMasterNode();
        if (master) {
            return master->isSettingsPanelOpened();
        }
        for (KnobLinkList::iterator it = _imp->nodeLinks.begin(); it != _imp->nodeLinks.end(); ++it) {
            NodePtr masterNode = it->masterNode.lock();
            if (masterNode && masterNode.get() != this && masterNode->isSettingsPanelOpenedInternal(recursionList)) {
                return true;
            }
        }
    }
    return gui->isSettingsPanelOpened();
}

bool
Node::isSettingsPanelOpened() const
{
    std::set<const Node*> tmplist;
    return isSettingsPanelOpenedInternal(tmplist);
}



void
Node::attachRotoItem(const boost::shared_ptr<RotoDrawableItem>& stroke)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->paintStroke = stroke;
    _imp->useAlpha0ToConvertFromRGBToRGBA = true;
    setProcessChannelsValues(true, true, true, true);
}

void
Node::setUseAlpha0ToConvertFromRGBToRGBA(bool use)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->useAlpha0ToConvertFromRGBToRGBA = use;
}

boost::shared_ptr<RotoDrawableItem>
Node::getAttachedRotoItem() const
{
    return _imp->paintStroke.lock();
}



void
Node::declareNodeVariableToPython(const std::string& nodeName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return;
#endif
    if (!_imp->isPartOfProject) {
        return;
    }
    PythonGILLocker pgl;
    
    PyObject* mainModule = appPTR->getMainModule();
    assert(mainModule);
    
    std::string appID = getApp()->getAppIDString();
    
    std::string nodeFullName = appID + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = Python::getAttrRecursive(nodeFullName, mainModule, &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);

    if (!alreadyDefined) {
        std::string script = nodeFullName + " = " + appID + ".getNode(\"";
        script.append(nodeName);
        script.append("\")\n");
        std::string err;
        if (!appPTR->isBackground()) {
            getApp()->printAutoDeclaredVariable(script);
        }
        if (!Python::interpretPythonScript(script, &err, 0)) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::setNodeVariableToPython(const std::string& oldName,const std::string& newName)
{
    if (!_imp->isPartOfProject) {
        return;
    }
    QString appID(getApp()->getAppIDString().c_str());
    QString str = QString(appID + ".%1 = " + appID + ".%2\ndel " + appID + ".%2\n").arg(newName.c_str()).arg(oldName.c_str());
    std::string script = str.toStdString();
    std::string err;
    if (!appPTR->isBackground()) {
        getApp()->printAutoDeclaredVariable(script);
    }
    if (!Python::interpretPythonScript(script, &err, 0)) {
        qDebug() << err.c_str();
    }
    
}

void
Node::deleteNodeVariableToPython(const std::string& nodeName)
{
    if (!_imp->isPartOfProject || nodeName.empty()) {
        return;
    }
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
    if (!Python::interpretPythonScript(script, &err, 0)) {
        qDebug() << err.c_str();
    }
}




void
Node::declarePythonFields()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return;
#endif
    if (!_imp->isPartOfProject) {
        return;
    }
    PythonGILLocker pgl;
    
    if (!getGroup()) {
        return ;
    }
    
    std::locale locale;
    std::string nodeName = getFullyQualifiedName();
    
    std::string appID = getApp()->getAppIDString();
    bool alreadyDefined = false;
    
    std::string nodeFullName = appID + "." + nodeName;
    PyObject* nodeObj = Python::getAttrRecursive(nodeFullName, Python::getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (!alreadyDefined) {
        qDebug() << QString("declarePythonFields(): attribute ") + nodeFullName.c_str() + " is not defined";
        throw std::logic_error(std::string("declarePythonFields(): attribute ") + nodeFullName + " is not defined");
    }
    
    
    std::stringstream ss;
    const KnobsVec& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        const std::string& knobName = knobs[i]->getName();
        if (!knobName.empty() && knobName.find(" ") == std::string::npos && !std::isdigit(knobName[0],locale)) {
            
            if (PyObject_HasAttrString(nodeObj, knobName.c_str())) {
                continue;
            }
            ss << nodeFullName <<  "." << knobName << " = ";
            ss << nodeFullName << ".getParam(\"" << knobName << "\")\n";
            
        }
    }
    
    std::string script = ss.str();
    if (!script.empty()) {
        if (!appPTR->isBackground()) {
            getApp()->printAutoDeclaredVariable(script);
        }
        std::string err;
        if (!Python::interpretPythonScript(script, &err, 0)) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::removeParameterFromPython(const std::string& parameterName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
    return;
#endif
    if (!_imp->isPartOfProject) {
        return;
    }
    PythonGILLocker pgl;
    std::string appID = getApp()->getAppIDString();
    std::string nodeName = getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    bool alreadyDefined = false;
    
    PyObject* nodeObj = Python::getAttrRecursive(nodeFullName, Python::getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (!alreadyDefined) {
        qDebug() << QString("declarePythonFields(): attribute ") + nodeFullName.c_str() + " is not defined";
        throw std::logic_error(std::string("declarePythonFields(): attribute ") + nodeFullName + " is not defined");
    }
    assert(PyObject_HasAttrString(nodeObj, parameterName.c_str()));
    std::string script = "del " + nodeFullName + "." + parameterName;
    if (!appPTR->isBackground()) {
        getApp()->printAutoDeclaredVariable(script);
    }
    std::string err;
    if (!Python::interpretPythonScript(script, &err, 0)) {
        qDebug() << err.c_str();
    }
}



std::string
Node::getKnobChangedCallback() const
{
    boost::shared_ptr<KnobString> s = _imp->knobChangedCallback.lock();
    return s ? s->getValue() : std::string();
}

std::string
Node::getInputChangedCallback() const
{
    boost::shared_ptr<KnobString> s = _imp->inputChangedCallback.lock();
    return s ? s->getValue() : std::string();
}

void
Node::Implementation::runOnNodeCreatedCBInternal(const std::string& cb,bool userEdited)
{
    std::vector<std::string> args;
    std::string error;
    try {
        Python::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor(std::string("Failed to run onNodeCreated callback: ")
                                                         + e.what());
        return;
    }
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
    if (!Python::interpretPythonScript(script, &error, &output)) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + error);
    } else if (!output.empty()) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }

}

void
Node::Implementation::runOnNodeDeleteCBInternal(const std::string& cb)
{
    std::vector<std::string> args;
    std::string error;
    try {
        Python::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor(std::string("Failed to run onNodeDeletion callback: ")
                                                         + e.what());
        return;
    }
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
    if (!Python::interpretPythonScript(ss.str(), &err, &output)) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + err);
    } else if (!output.empty()) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }

}

void
Node::Implementation::runOnNodeCreatedCB(bool userEdited)
{
    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeCreatedCB();
    boost::shared_ptr<NodeCollection> group = _publicInterface->getGroup();
    if (!group) {
        return;
    }
    if (!cb.empty()) {
        runOnNodeCreatedCBInternal(cb, userEdited);
    }
    
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(group.get());
    boost::shared_ptr<KnobString> nodeCreatedCbKnob = nodeCreatedCallback.lock();
    if (!nodeCreatedCbKnob && isGroup) {
        cb = isGroup->getNode()->getAfterNodeCreatedCallback();
    } else  if (nodeCreatedCbKnob) {
        cb = nodeCreatedCbKnob->getValue();
    }
    if (!cb.empty()) {
        runOnNodeCreatedCBInternal(cb, userEdited);
    }
}

void
Node::Implementation::runOnNodeDeleteCB()
{
    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeDeleteCB();
    boost::shared_ptr<NodeCollection> group = _publicInterface->getGroup();
    if (!group) {
        return;
    }
    if (!cb.empty()) {
        runOnNodeDeleteCBInternal(cb);
    }

    
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(group.get());
    boost::shared_ptr<KnobString> nodeDeletedKnob = nodeRemovalCallback.lock();
    if (!nodeDeletedKnob && isGroup) {
        cb = isGroup->getNode()->getBeforeNodeRemovalCallback();
    } else  if (nodeDeletedKnob) {
        cb = nodeDeletedKnob->getValue();
    }
    if (!cb.empty()) {
        runOnNodeDeleteCBInternal(cb);
    }
}

std::string
Node::getBeforeRenderCallback() const
{
    boost::shared_ptr<KnobString> s = _imp->beforeRender.lock();
    return s ? s->getValue() : std::string();
}

std::string
Node::getBeforeFrameRenderCallback() const
{
    boost::shared_ptr<KnobString> s = _imp->beforeFrameRender.lock();
    return s ? s->getValue() : std::string();
}

std::string
Node::getAfterRenderCallback() const
{
    boost::shared_ptr<KnobString> s = _imp->afterRender.lock();
    return s ? s->getValue() : std::string();
}

std::string
Node::getAfterFrameRenderCallback() const
{
    boost::shared_ptr<KnobString> s = _imp->afterFrameRender.lock();
    return s ? s->getValue() : std::string();
}

std::string
Node::getAfterNodeCreatedCallback() const
{
    boost::shared_ptr<KnobString> s = _imp->nodeCreatedCallback.lock();
    return s ? s->getValue() : std::string();
}

std::string
Node::getBeforeNodeRemovalCallback() const
{
    boost::shared_ptr<KnobString> s = _imp->nodeRemovalCallback.lock();
    return s ? s->getValue() : std::string();
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
    try {
        Python::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor(std::string("Failed to run onInputChanged callback: ")
                                                         + e.what());
        return;
    }
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
        std::string nodeName = isParentGrp->getNode()->getFullyQualifiedName();
        std::string nodeFullName = appID + "." + nodeName;
        thisGroupVar = nodeFullName;
    } else {
        thisGroupVar = appID;
    }
    
    std::stringstream ss;
    ss << cb << "(" << index << "," << appID << "." << _publicInterface->getFullyQualifiedName() << "," << thisGroupVar << "," << appID << ")\n";
    
    std::string script = ss.str();
    std::string output;
    if (!Python::interpretPythonScript(script, &error,&output)) {
        _publicInterface->getApp()->appendToScriptEditor(QObject::tr("Failed to execute callback: ").toStdString() + error);
    } else {
        if (!output.empty()) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }

}

boost::shared_ptr<KnobChoice>
Node::getChannelSelectorKnob(int inputNb) const
{
    std::map<int,ChannelSelector>::const_iterator found = _imp->channelsSelectors.find(inputNb);
    if (found == _imp->channelsSelectors.end()) {
        if (inputNb == -1) {
            ///The effect might be multi-planar and supply its own
            KnobPtr knob = getKnobByName(kNatronOfxParamOutputChannels);
            if (!knob) {
                return boost::shared_ptr<KnobChoice>();
            }
            return boost::dynamic_pointer_cast<KnobChoice>(knob);
        }
        return boost::shared_ptr<KnobChoice>();
    }
    return found->second.layer.lock();
}

bool
Node::refreshChannelSelectors()
{
    if (!isNodeCreated()) {
        return false;
    }
    _imp->effect->setComponentsAvailableDirty(true);
    
    double time = getApp()->getTimeLine()->currentFrame();
    
    bool hasChanged = false;
    for (std::map<int,ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
        
        NodePtr node;
        if (it->first == -1) {
            node = shared_from_this();
        } else {
            node = getInput(it->first);
        }
        
        boost::shared_ptr<KnobChoice> layerKnob = it->second.layer.lock();
        const std::vector<std::string> currentLayerEntries = layerKnob->getEntries_mt_safe();
        const std::string curLayer_internalName = it->second.layerName.lock()->getValue();
        const std::string curLayer = ImageComponents::mapUserFriendlyPlaneNameToNatronInternalPlaneName(curLayer_internalName);
        
        bool isCurLayerColorComp = curLayer == kNatronAlphaComponentsName || curLayer == kNatronRGBAComponentsName || curLayer == kNatronRGBComponentsName;
        
        std::vector<std::string> choices;
        if (it->second.hasAllChoice) {
            choices.push_back("All");
        } else {
            choices.push_back("None");
        }
        int gotColor = -1;

        ImageComponents colorComp,selectedComp;

        /*
         These are default layers that we always display in the layer selector.
         If one of them is found in the clip preferences, we set the default value to it.
         */
        std::map<std::string, int > defaultLayers;
        {
            int i = 0;
            while (ImageComponents::defaultComponents[i][0] != 0) {
                std::string layer = ImageComponents::defaultComponents[i][0];
                if (layer != kNatronAlphaComponentsName && layer != kNatronRGBAComponentsName && layer != kNatronRGBComponentsName) {
                    //Do not add the color plane, because it is handled in a separate case to make sure it is always the first choice
                    defaultLayers[layer] = -1;
                }
                ++i;
            }
        }
        
        int foundCurLayerChoice = -1;
        if (curLayer == "All" || curLayer == "None") {
            foundCurLayerChoice = 0;
        }
        
        if (node) {
            EffectInstance::ComponentsAvailableMap compsAvailable;
            node->getEffectInstance()->getComponentsAvailable(it->first != -1, true, time, &compsAvailable);
            {
                QMutexLocker k(&it->second.compsMutex);
                it->second.compsAvailable = compsAvailable;
            }
            for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2!= compsAvailable.end(); ++it2) {
                if (it2->first.isColorPlane()) {
                    int numComp = it2->first.getNumComponents();
                    colorComp = it2->first;
                    
                    assert(choices.size() > 0);
                    std::vector<std::string>::iterator pos = choices.begin();
                    ++pos; // bypass the "None" choice
                    gotColor = 1;


                    std::string colorCompName;
                    if (numComp == 1) {
                        colorCompName = kNatronAlphaPlaneUserName;
                    } else if (numComp == 3) {
                        colorCompName = kNatronRGBPlaneUserName;
                    } else if (numComp == 4) {
                        colorCompName = kNatronRGBAPlaneUserName;
                    } else {
                        assert(false);
                    }
                    choices.insert(pos,colorCompName);
                    
                    if (foundCurLayerChoice == -1 && isCurLayerColorComp) {
                        selectedComp = it2->first;
                        foundCurLayerChoice = 1;
                    }

                    
                    ///Increment all default indexes
                    for (std::map<std::string, int >::iterator it = defaultLayers.begin() ;it!=defaultLayers.end(); ++it) {
                        if (it->second != -1) {
                            ++it->second;
                        }
                    }
                } else {
                    
                    std::string choiceName = ImageComponents::mapNatronInternalPlaneNameToUserFriendlyPlaneName(it2->first.getLayerName());
                    std::map<std::string, int>::iterator foundDefaultLayer = defaultLayers.find(it2->first.getLayerName());
                    if (foundDefaultLayer != defaultLayers.end()) {
                        foundDefaultLayer->second = choices.size() -1;
                    }
                    
                   
                    choices.push_back(choiceName);
                    
                    if (foundCurLayerChoice == -1 && it2->first.getLayerName() == curLayer) {
                        selectedComp = it2->first;
                        foundCurLayerChoice = choices.size()-1;
                    }
                    
                }
            }
        } // if (node) {
        
        if (gotColor == -1) {
            assert(choices.size() > 0);
            std::vector<std::string>::iterator pos = choices.begin();
            ++pos;
            gotColor = 1;
            ///Increment all default indexes
            for (std::map<std::string, int>::iterator it = defaultLayers.begin() ;it!=defaultLayers.end(); ++it) {
                if (it->second != -1) {
                    ++it->second;
                }
            }
            colorComp = ImageComponents::getRGBAComponents();
            choices.insert(pos,kNatronRGBAPlaneUserName);
            
        }
        for (std::map<std::string, int>::iterator itl = defaultLayers.begin(); itl != defaultLayers.end(); ++itl) {
            if (itl->second == -1) {
                std::string choiceName = ImageComponents::mapNatronInternalPlaneNameToUserFriendlyPlaneName(itl->first);
                choices.push_back(choiceName);
                
                if (foundCurLayerChoice == -1 && itl->first == curLayer) {
                    selectedComp = ImageComponents::getDefaultComponent(itl->first);
                    foundCurLayerChoice = choices.size()-1;
                }

            }
        }

        if (choices.size() != currentLayerEntries.size()) {
            hasChanged = true;
        } else {
            for (std::size_t i = 0; i < currentLayerEntries.size(); ++i) {
                if (currentLayerEntries[i] != choices[i]) {
                    hasChanged = true;
                    break;
                }
            }
        }
        
        layerKnob->populateChoices(choices);
        if (hasChanged) {
            s_outputLayerChanged();
        }
        
        
        if (!curLayer.empty() && foundCurLayerChoice != -1) {
            //We already had a choice and it was found in the current layers
            assert(foundCurLayerChoice >= 0 && foundCurLayerChoice < (int)choices.size());
            layerKnob->blockValueChanges();
            _imp->effect->beginChanges();
            layerKnob->setValue(foundCurLayerChoice, 0);
            _imp->effect->endChanges(true);
            layerKnob->unblockValueChanges();
            if (it->first == -1 && _imp->enabledChan[0].lock()) {
                refreshEnabledKnobsLabel(selectedComp);
            }
            
        } else {
            //fallback
            bool foundLayer = false;
            if (!curLayer_internalName.empty()) {
                //check if the layer is in the available options: since we had default layers that may not be produced, it may be in the list
                //but not in the components presents
                for (std::size_t i = 0; i < currentLayerEntries.size(); ++i) {
                    if (currentLayerEntries[i] == curLayer_internalName) {
                        layerKnob->setValue(i, 0);
                        it->second.layerName.lock()->setValue(choices[i], 0);
                        foundLayer = true;
                        break;
                    }
                }

            }
            if (!foundLayer) {
                if (it->second.hasAllChoice &&
                    _imp->effect->isPassThroughForNonRenderedPlanes() == EffectInstance::ePassThroughRenderAllRequestedPlanes) {
                    layerKnob->setValue(0, 0);
                    it->second.layerName.lock()->setValue(choices[0], 0);
                } else {
                    
                    int defaultIndex;
                    if (gotColor != -1) {
                        defaultIndex = gotColor;
                    } else {
                        defaultIndex = -1;
                        for (std::map<std::string, int>::iterator itl = defaultLayers.begin(); itl != defaultLayers.end(); ++itl) {
                            if (itl->second != -1) {
                                defaultIndex = itl->second;
                                break;
                            }
                        }
                    }
                    
                    assert(defaultIndex != -1 && defaultIndex >= 0 && defaultIndex < (int)choices.size());
                    layerKnob->setValue(defaultIndex,0);
                    it->second.layerName.lock()->setValue(choices[defaultIndex], 0);
                }
            } // !foundLayer
        }
    } // for (std::map<int,ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
    
    NodePtr prefInputNode;
    if (!_imp->maskSelectors.empty()) {
        int prefInputNb = getPreferredInput();
        if (prefInputNb != -1) {
            prefInputNode = getInput(prefInputNb);
        }
    }
    
    for (std::map<int,MaskSelector>::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        NodePtr node;
        if (it->first == -1) {
            node = shared_from_this();
        } else {
            node = getInput(it->first);
        }
        
        boost::shared_ptr<KnobChoice> channelKnob = it->second.channel.lock();
        boost::shared_ptr<KnobString> channelNameKnob = it->second.channelName.lock();

        const std::vector<std::string> currentLayerEntries = channelKnob->getEntries_mt_safe();
        const std::string curLayer = channelNameKnob->getValue();
        std::string curLayerName = curLayer,curChannelName;
        {
            std::size_t foundLastDot = curLayer.find_last_of(".");
            if (foundLastDot != std::string::npos) {
                curLayerName = curLayer.substr(0, foundLastDot);
                std::size_t foundPrevDot = curLayerName.find_first_of(".");
                if (foundPrevDot != std::string::npos) {
                    //Remove the node name
                    curLayerName = curLayerName.substr(foundPrevDot + 1);
                }
                curChannelName = curLayer.substr(foundLastDot + 1);
            }
        }
        
        bool isCurLayerColor = curLayerName == kNatronRGBAComponentsName ||
        curLayerName == kNatronRGBComponentsName ||
        curLayerName == kNatronAlphaComponentsName;
        
        std::vector<std::string> choices;
        choices.push_back("None");
        int alphaIndex = 0;
        
        //Get the mask input components
        EffectInstance::ComponentsAvailableMap compsAvailable;
        std::list<EffectInstance*> markedNodes;
        if (node) {
            node->getEffectInstance()->getComponentsAvailable(true, true, time, &compsAvailable,&markedNodes);
        }
        
        //Also get the node's preferred input (the "main" input) components
        EffectInstance::ComponentsAvailableMap prefInputAvailComps;
        
        if (prefInputNode) {
            prefInputNode->getEffectInstance()->getComponentsAvailable(true, true, time, &prefInputAvailComps, &markedNodes);
            
            //Merge the 2 components available maps, but preferring channels coming from the Mask input
            for (EffectInstance::ComponentsAvailableMap::iterator it = prefInputAvailComps.begin(); it != prefInputAvailComps.end(); ++it) {
                //If the component is already present in the 'comps' map, only add it if we are the preferred input
                EffectInstance::ComponentsAvailableMap::iterator colorMatch = compsAvailable.end();
                bool found = false;
                for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2 != compsAvailable.end(); ++it2) {
                    if (it2->first == it->first) {
                        found = true;
                        break;
                    } else if ( it2->first.isColorPlane() ) {
                        colorMatch = it2;
                    }
                }
                if (!found) {
                    if ( ( colorMatch != compsAvailable.end() ) && it->first.isColorPlane() ) {
                        //we found another color components type, skip
                        continue;
                    } else {
                        compsAvailable.insert(*it);
                    }
                }
            }
        } // if (prefInputNode)
        
        
        std::vector<std::pair<ImageComponents,NodeWPtr > > compsOrdered;
        for (EffectInstance::ComponentsAvailableMap::iterator comp = compsAvailable.begin(); comp != compsAvailable.end(); ++comp) {
            if (comp->first.isColorPlane()) {
                compsOrdered.insert(compsOrdered.begin(), std::make_pair(comp->first,comp->second));
            } else {
                compsOrdered.push_back(*comp);
            }
        }
        {
            
            QMutexLocker k(&it->second.compsMutex);
            it->second.compsAvailable = compsOrdered;
        }
        int foundLastLayerChoice = -1;
        if (curLayer == "None") {
            foundLastLayerChoice = 0;
        }
        for (std::vector<std::pair<ImageComponents,NodeWPtr > >::iterator it2 = compsOrdered.begin(); it2!= compsOrdered.end(); ++it2) {
            
            const std::vector<std::string>& channels = it2->first.getComponentsNames();
            const std::string& layerName = it2->first.isColorPlane() ? it2->first.getComponentsGlobalName() : it2->first.getLayerName();
            bool isLastLayer = false;
            bool isColorPlane = it2->first.isColorPlane();
            
            if (foundLastLayerChoice == -1 && (layerName == curLayerName || (isCurLayerColor && isColorPlane))) {
                isLastLayer = true;
            }
            
            std::string nodeName = it2->second.lock()->getScriptName_mt_safe();
            
            for (std::size_t i = 0; i < channels.size(); ++i) {
                choices.push_back(nodeName + "." + layerName + "." + channels[i]);
                if (isLastLayer && channels[i] == curChannelName) {
                    foundLastLayerChoice = choices.size() - 1;
                }
            }
            
            if (it2->first.isColorPlane()) {
                if (channels.size() == 1 || channels.size() == 4) {
                    alphaIndex = choices.size() - 1;
                } else {
                    alphaIndex = 0;
                }
            }
        }
        
        
        /*if (!gotColor) {
            const ImageComponents& rgba = ImageComponents::getRGBAComponents();
            const std::vector<std::string>& channels = rgba.getComponentsNames();
            const std::string& layerName = rgba.getComponentsGlobalName();
            for (std::size_t i = 0; i < channels.size(); ++i) {
                choices.push_back(layerName + "." + channels[i]);
            }
            alphaIndex = choices.size() - 1;
        }*/
        
        if (choices.size() != currentLayerEntries.size()) {
            hasChanged = true;
        } else {
            for (std::size_t i = 0; i < currentLayerEntries.size(); ++i) {
                if (currentLayerEntries[i] != choices[i]) {
                    hasChanged = true;
                    break;
                }
            }
        }
        channelKnob->populateChoices(choices);
        
        
        /*if (setValues) {
            assert(alphaIndex != -1 && alphaIndex >= 0 && alphaIndex < (int)choices.size());
            channelKnob->setValue(alphaIndex,0);
            channelNameKnob->setValue(choices[alphaIndex], 0);
        } else {*/
            if (foundLastLayerChoice != -1 && foundLastLayerChoice != 0) {
                channelKnob->blockValueChanges();
                _imp->effect->beginChanges();
                channelKnob->setValue(foundLastLayerChoice, 0);
                channelKnob->unblockValueChanges();
                channelNameKnob->setValue(choices[foundLastLayerChoice], 0);
                _imp->effect->endChanges();
                
            } else {
                assert(alphaIndex != -1 && alphaIndex >= 0 && alphaIndex < (int)choices.size());
                channelKnob->setValue(alphaIndex,0);
                channelNameKnob->setValue(choices[alphaIndex], 0);
            }
        //}
    }
    
    //Notify the effect channels have changed (the viewer needs this)
    _imp->effect->onChannelsSelectorRefreshed();
    
    return hasChanged;
    
} // Node::refreshChannelSelectors()

bool
Node::addUserComponents(const ImageComponents& comps)
{
    ///The node has node channel selector, don't allow adding a custom plane.
    KnobPtr outputLayerKnob = getKnobByName(kNatronOfxParamOutputChannels);
    if (_imp->channelsSelectors.empty() && !outputLayerKnob) {
        return false;
    }
    
    if (!outputLayerKnob) {
        //The effect does not have kNatronOfxParamOutputChannels but maybe the selector provided by Natron
        std::map<int,ChannelSelector>::iterator found = _imp->channelsSelectors.find(-1);
        if (found == _imp->channelsSelectors.end()) {
            return false;
        }
        outputLayerKnob = found->second.layer.lock();
    }
    
    {
        QMutexLocker k(&_imp->createdComponentsMutex);
        for (std::list<ImageComponents>::iterator it = _imp->createdComponents.begin(); it != _imp->createdComponents.end(); ++it) {
            if (it->getLayerName() == comps.getLayerName()) {
                return false;
            }
        }
        
        _imp->createdComponents.push_back(comps);
    }
    {
        ///Clip preferences have changed
        RenderScale s(1.);
        getEffectInstance()->refreshClipPreferences_public(getApp()->getTimeLine()->currentFrame(),
                                                          s,
                                                          eValueChangedReasonUserEdited,
                                                          true, true);
    }
    {
        ///Set the selector to the new channel
        KnobChoice* layerChoice = dynamic_cast<KnobChoice*>(outputLayerKnob.get());
        if (layerChoice) {
            layerChoice->setValueFromLabel(comps.getLayerName(), 0);
        }
    }
    return true;
}

void
Node::getUserCreatedComponents(std::list<ImageComponents>* comps)
{
    QMutexLocker k(&_imp->createdComponentsMutex);
    *comps = _imp->createdComponents;
}

double
Node::getHostMixingValue(double time) const
{
    boost::shared_ptr<KnobDouble> mix = _imp->mixWithSource.lock();
    return mix ? mix->getValueAtTime(time) : 1.;
}

//////////////////////////////////

InspectorNode::InspectorNode(AppInstance* app,
                             const boost::shared_ptr<NodeCollection>& group,
                             Plugin* plugin,
                             int maxInputs)
: Node(app,group,plugin)
, _maxInputs(maxInputs)
{
}

InspectorNode::~InspectorNode()
{
}

bool
InspectorNode::connectInput(const NodePtr& input,
                            int inputNumber)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    if (!isEffectViewer()) {
        return connectInputBase(input, inputNumber);
    }
    
    ///cannot connect more than _maxInputs inputs.
    assert(inputNumber <= _maxInputs);
    
    assert(input);
    
    if (!checkIfConnectingInputIsOk(input.get())) {
        return false;
    }
    
    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if (!getEffectInstance()->supportsMultiResolution()) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }
    
    ///If the node 'input' is already to an input of the inspector, find it.
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
    
    if ( !Node::connectInput(input, inputNumber) ) {
        bool creatingNodeTree = getApp()->isCreatingNodeTree();
        if (!creatingNodeTree) {
            ///Recompute the hash
            computeHash();
        }

    }
    
    return true;
}


void
InspectorNode::setActiveInputAndRefresh(int inputNb, bool /*fromViewer*/)
{
    assert(QThread::currentThread() == qApp->thread());
    
    if ( ( inputNb > (_maxInputs - 1) ) || (inputNb < 0) || (getInput(inputNb) == NULL) ) {
        return;
    }

    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }

    Q_EMIT inputChanged(inputNb);
    onInputChanged(inputNb);

    runInputChangedCallback(inputNb);
    
    
    if ( isOutputNode() ) {
        OutputEffectInstance* oei = dynamic_cast<OutputEffectInstance*>( getEffectInstance().get() );
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
    if (useInputA || getPluginID() == PLUGINID_OFX_SHUFFLE) {
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

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Node.cpp"
