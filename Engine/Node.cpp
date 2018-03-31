/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream> // stringstream

#include "Global/Macros.h"

#include <boost/scoped_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtCore/QWaitCondition>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QRegExp>

#include <ofxNatron.h>

#include "Global/FStreamsSupport.h"

#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Backdrop.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Dot.h"
#include "Engine/EffectInstance.h"
#include "Engine/Format.h"
#include "Engine/FileSystemModel.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Log.h"
#include "Engine/Lut.h"
#include "Engine/MemoryInfo.h" // printAsRAM
#include "Engine/NodeGroup.h"
#include "Engine/NodeGuiI.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/OfxHost.h"
#include "Engine/OneViewNode.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/Plugin.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/PrecompNode.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Timer.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"
#include "Engine/TLSHolder.h"
#include "Engine/UndoCommand.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/WriteNode.h"

///The flickering of edges/nodes in the nodegraph will be refreshed
///at most every...
#define NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS 1

NATRON_NAMESPACE_ENTER

using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;


// protect local classes in anonymous namespace
NATRON_NAMESPACE_ANONYMOUS_ENTER

/*The output node was connected from inputNumber to this...*/
typedef std::map<NodeWPtr, int > DeactivatedState;
typedef std::list<Node::KnobLink> KnobLinkList;
typedef std::vector<NodeWPtr> InputsV;


class ChannelSelector
{
public:

    KnobChoiceWPtr layer;


    ChannelSelector()
        : layer()
    {
    }

    ChannelSelector(const ChannelSelector& other)
    {
        *this = other;
    }

    void operator=(const ChannelSelector& other)
    {
        layer = other.layer;
    }
};

class MaskSelector
{
public:

    KnobBoolWPtr enabled;
    KnobChoiceWPtr channel;
    mutable QMutex compsMutex;
    //Stores the components available at build time of the choice menu
    std::vector<std::pair<ImagePlaneDesc, NodeWPtr> > compsAvailable;

    MaskSelector()
        : enabled()
        , channel()
        , compsMutex()
        , compsAvailable()
    {
    }

    MaskSelector(const MaskSelector& other)
    {
        *this = other;
    }

    void operator=(const MaskSelector& other)
    {
        enabled = other.enabled;
        channel = other.channel;
        QMutexLocker k(&compsMutex);
        compsAvailable = other.compsAvailable;
    }
};


struct FormatKnob
{
    KnobIntWPtr size;
    KnobDoubleWPtr par;
    KnobChoiceWPtr formatChoice;
};

struct PyPlugInfo
{
    std::string pluginPythonModule; // the absolute filename of the python script

    //Set to true when the user has edited a PyPlug
    bool isPyPlug;
    std::string pyPlugID; //< if this is a pyplug, this is the ID of the Plug-in. This is because the plugin handle will be the one of the Group
    std::string pyPlugLabel;
    std::string pyPlugDesc;
    std::string pyPlugIconFilePath;
    std::list<std::string> pyPlugGrouping;
    int pyPlugVersion;

    PyPlugInfo()
    : isPyPlug(false)
    , pyPlugVersion(0)
    {

    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


struct Node::Implementation
{
    Q_DECLARE_TR_FUNCTIONS(Node)

public:
    Implementation(Node* publicInterface,
                   const AppInstancePtr& app_,
                   const NodeCollectionPtr& collection,
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
        , pyPluginInfoMutex()
        , pyPlugInfo()
        , computingPreview(false)
        , previewThreadQuit(false)
        , computingPreviewMutex()
        , pluginInstanceMemoryUsed(0)
        , memoryUsedMutex()
        , mustQuitPreview(0)
        , mustQuitPreviewMutex()
        , mustQuitPreviewCond()
        , renderInstancesSharedMutex(QMutex::Recursive)
        , knobsAge(0)
        , knobsAgeMutex()
        , masterNodeMutex()
        , masterNode()
        , nodeLinks()
#ifdef NATRON_ENABLE_IO_META_NODES
        , ioContainer()
#endif
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
        , trackContext()
        , imagesBeingRenderedMutex()
        , imagesBeingRenderedCond()
        , imagesBeingRendered()
        , supportedDepths()
        , isMultiInstance(false)
        , multiInstanceParent()
        , childrenMutex()
        , children()
        , multiInstanceParentName()
        , keyframesDisplayedOnTimeline(false)
        , lastRenderStartedMutex()
        , lastRenderStartedSlotCallTime()
        , renderStartedCounter(0)
        , inputIsRenderingCounter(0)
        , lastInputNRenderStartedSlotCallTime()
        , nodeIsDequeuing(false)
        , nodeIsDequeuingMutex()
        , nodeIsDequeuingCond()
        , nodeIsRendering(0)
        , nodeIsRenderingMutex()
        , persistentMessage()
        , persistentMessageType(0)
        , persistentMessageMutex()
        , guiPointer()
        , nativeOverlays()
        , nodeCreated(false)
        , wasCreatedSilently(false)
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
        , isRefreshingInputRelatedData(false)
        , streamWarnings()
        , requiresGLFinishBeforeRender(false)
        , hostChannelSelectorEnabled(false)
    {
        ///Initialize timers
        gettimeofday(&lastRenderStartedSlotCallTime, 0);
        gettimeofday(&lastInputNRenderStartedSlotCallTime, 0);
    }

    void abortPreview_non_blocking();

    void abortPreview_blocking(bool allowPreviewRenders);

    bool checkForExitPreview();

    void setComputingPreview(bool v)
    {
        QMutexLocker l(&computingPreviewMutex);

        computingPreview = v;
    }

    void restoreUserKnobsRecursive(const std::list<boost::shared_ptr<KnobSerializationBase> >& knobs,
                                   const boost::shared_ptr<KnobGroup>& group,
                                   const boost::shared_ptr<KnobPage>& page);

    void restoreKnobLinksRecursive(const GroupKnobSerialization* group,
                                   const NodesList & allNodes,
                                   const std::map<std::string, std::string>& oldNewScriptNamesMapping);

    void ifGroupForceHashChangeOfInputs();

    void runOnNodeCreatedCB(bool userEdited);

    void runOnNodeDeleteCB();

    void runOnNodeCreatedCBInternal(const std::string& cb, bool userEdited);

    void runOnNodeDeleteCBInternal(const std::string& cb);


    void appendChild(const NodePtr& child);

    void runInputChangedCallback(int index, const std::string& script);

    void createChannelSelector(int inputNb, const std::string & inputName, bool isOutput, const boost::shared_ptr<KnobPage>& page, KnobIPtr* lastKnobBeforeAdvancedOption);

    void onLayerChanged(int inputNb, const ChannelSelector& selector);

    void onMaskSelectorChanged(int inputNb, const MaskSelector& selector);

    ImagePlaneDesc getSelectedLayerInternal(int inputNb, const std::list<ImagePlaneDesc>& availableLayers, const ChannelSelector& selector) const;


    Node* _publicInterface;
    NodeCollectionWPtr group;
    PrecompNodeWPtr precomp;
    AppInstanceWPtr app; // pointer to the app: needed to access the application's default-project's format
    bool isPartOfProject;
    bool knobsInitialized;
    bool inputsInitialized;
    mutable QMutex outputsMutex;
    NodesWList outputs, guiOutputs;
    mutable QMutex inputsMutex; //< protects guiInputs so the serialization thread can access them

    ///The  inputs are the ones used while rendering and guiInputs the ones used by the gui whenever
    ///the node is currently rendering. Once the render is finished, inputs are refreshed automatically to the value of
    ///guiInputs
    InputsV inputs, guiInputs;

    //to the inputs in a thread-safe manner.
    EffectInstancePtr effect;  //< the effect hosted by this node

    ///The accepted components in input and in output of the plug-in
    ///These two are also protected by inputsMutex
    std::vector<std::list<ImagePlaneDesc> > inputsComponents;
    std::list<ImagePlaneDesc> outputComponents;
    mutable QMutex nameMutex;
    mutable QMutex inputsLabelsMutex;
    std::vector<std::string> inputLabels; // inputs name, protected by inputsLabelsMutex
    std::vector<std::string> inputHints; // protected by inputsLabelsMutex
    std::vector<bool> inputsVisibility; // protected by inputsMutex
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
    mutable QMutex pyPluginInfoMutex;
    PyPlugInfo pyPlugInfo;
    bool computingPreview;
    bool previewThreadQuit;
    mutable QMutex computingPreviewMutex;
    size_t pluginInstanceMemoryUsed; //< global count on all EffectInstance's of the memory they use.
    QMutex memoryUsedMutex; //< protects _pluginInstanceMemoryUsed
    int mustQuitPreview;
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

#ifdef NATRON_ENABLE_IO_META_NODES
    //When creating a Reader or Writer node, this is a pointer to the "bundle" node that the user actually see.
    NodeWPtr ioContainer;
#endif

    KnobIntWPtr frameIncrKnob;
    KnobPageWPtr nodeSettingsPage;
    KnobStringWPtr nodeLabelKnob;
    KnobBoolWPtr previewEnabledKnob;
    KnobBoolWPtr disableNodeKnob;
    KnobChoiceWPtr openglRenderingEnabledKnob;
    KnobIntWPtr lifeTimeKnob;
    KnobBoolWPtr enableLifeTimeKnob;
    KnobStringWPtr knobChangedCallback;
    KnobStringWPtr inputChangedCallback;
    KnobStringWPtr nodeCreatedCallback;
    KnobStringWPtr nodeRemovalCallback;
    KnobPageWPtr infoPage;
    KnobStringWPtr nodeInfos;
    KnobButtonWPtr refreshInfoButton;
    KnobBoolWPtr useFullScaleImagesWhenRenderScaleUnsupported;
    KnobBoolWPtr forceCaching;
    KnobBoolWPtr hideInputs;
    KnobStringWPtr beforeFrameRender;
    KnobStringWPtr beforeRender;
    KnobStringWPtr afterFrameRender;
    KnobStringWPtr afterRender;
    KnobBoolWPtr enabledChan[4];
    KnobStringWPtr premultWarning;
    KnobDoubleWPtr mixWithSource;
    KnobButtonWPtr renderButton; //< render button for writers
    FormatKnob pluginFormatKnobs;
    KnobBoolWPtr processAllLayersKnob;
    std::map<int, ChannelSelector> channelsSelectors;
    std::map<int, MaskSelector> maskSelectors;
    boost::shared_ptr<RotoContext> rotoContext; //< valid when the node has a rotoscoping context (i.e: paint context)
    boost::shared_ptr<TrackerContext> trackContext;
    mutable QMutex imagesBeingRenderedMutex;
    QWaitCondition imagesBeingRenderedCond;
    std::list<boost::shared_ptr<Image> > imagesBeingRendered; ///< a list of all the images being rendered simultaneously
    std::list<ImageBitDepthEnum> supportedDepths;

    ///True when several effect instances are represented under the same node.
    bool isMultiInstance;
    NodeWPtr multiInstanceParent;
    mutable QMutex childrenMutex;
    NodesWList children;

    ///the name of the parent at the time this node was created
    std::string multiInstanceParentName;
    bool keyframesDisplayedOnTimeline;

    ///This is to avoid the slots connected to the main-thread to be called too much
    QMutex lastRenderStartedMutex; //< protects lastRenderStartedSlotCallTime & lastInputNRenderStartedSlotCallTime
    timeval lastRenderStartedSlotCallTime;
    int renderStartedCounter;
    std::vector<int> inputIsRenderingCounter;
    timeval lastInputNRenderStartedSlotCallTime;

    ///True when the node is dequeuing the connectionQueue and no render should be started 'til it is empty
    bool nodeIsDequeuing;
    QMutex nodeIsDequeuingMutex;
    QWaitCondition nodeIsDequeuingCond;

    ///Counter counting how many parallel renders are active on the node
    int nodeIsRendering;
    mutable QMutex nodeIsRenderingMutex;
    QString persistentMessage;
    int persistentMessageType;
    mutable QMutex persistentMessageMutex;
    NodeGuiIWPtr guiPointer;
    std::list<boost::shared_ptr<HostOverlayKnobs> > nativeOverlays;
    bool nodeCreated;
    bool wasCreatedSilently;
    mutable QMutex createdComponentsMutex;
    std::list<ImagePlaneDesc> createdComponents; // comps created by the user
    RotoDrawableItemWPtr paintStroke;

    // These are dynamic props
    mutable QMutex pluginsPropMutex;
    RenderSafetyEnum pluginSafety, currentThreadSafety;
    bool currentSupportTiles;
    PluginOpenGLRenderSupport currentSupportOpenGLRender;
    SequentialPreferenceEnum currentSupportSequentialRender;
    bool currentCanTransform;
    bool draftModeUsed, mustComputeInputRelatedData;
    bool duringPaintStrokeCreation; // protected by lastStrokeMovementMutex
    mutable QMutex lastStrokeMovementMutex;
    bool strokeBitmapCleared;


    //This flag is used for the Roto plug-in and for the Merge inside the rotopaint tree
    //so that if the input of the roto node is RGB, it gets converted with alpha = 0, otherwise the user
    //won't be able to paint the alpha channel
    bool useAlpha0ToConvertFromRGBToRGBA;
    mutable QMutex isBeingDestroyedMutex;
    bool isBeingDestroyed;
    boost::shared_ptr<NodeRenderWatcher> renderWatcher;
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
    int isRefreshingInputRelatedData; // only used by the main thread
    std::map<Node::StreamWarningEnum, QString> streamWarnings;

    // Some plug-ins (mainly Hitfilm Ignite detected for now) use their own OpenGL context that is sharing resources with our OpenGL contexT.
    // as a result if we don't call glFinish() before calling the render action, the plug-in context might use textures that were not finished yet.
    bool requiresGLFinishBeforeRender;

    bool hostChannelSelectorEnabled;
};

class RefreshingInputData_RAII
{
    Node::Implementation *_imp;

public:

    RefreshingInputData_RAII(Node::Implementation* imp)
        : _imp(imp)
    {
        ++_imp->isRefreshingInputRelatedData;
    }

    ~RefreshingInputData_RAII()
    {
        --_imp->isRefreshingInputRelatedData;
    }
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

Node::Node(const AppInstancePtr& app,
           const NodeCollectionPtr& group,
           Plugin* plugin)
    : QObject()
    , _imp( new Implementation(this, app, group, plugin) )
{
    QObject::connect( this, SIGNAL(pluginMemoryUsageChanged(qint64)), appPTR, SLOT(onNodeMemoryRegistered(qint64)) );
    QObject::connect( this, SIGNAL(mustDequeueActions()), this, SLOT(dequeueActions()) );
    QObject::connect( this, SIGNAL(mustComputeHashOnMainThread()), this, SLOT(doComputeHashOnMainThread()) );
    QObject::connect(this, SIGNAL(refreshIdentityStateRequested()), this, SLOT(onRefreshIdentityStateRequestReceived()), Qt::QueuedConnection);

    if (plugin && plugin->getPluginID().startsWith(QLatin1String("com.FXHOME.HitFilm"))) {
        _imp->requiresGLFinishBeforeRender = true;
    }
}

bool
Node::isGLFinishRequiredBeforeRender() const
{
    return _imp->requiresGLFinishBeforeRender;
}

bool
Node::isPartOfProject() const
{
    return _imp->isPartOfProject;
}

void
Node::createRotoContextConditionnally()
{
    assert(!_imp->rotoContext);
    assert(_imp->effect);
    ///Initialize the roto context if any
    if ( isRotoNode() || isRotoPaintingNode() ) {
        _imp->effect->beginChanges();
        _imp->rotoContext = RotoContext::create( shared_from_this() );
        _imp->effect->endChanges(true);
        _imp->rotoContext->createBaseLayer();
    }
}

void
Node::createTrackerContextConditionnally()
{
    assert(!_imp->trackContext);
    assert(_imp->effect);
    ///Initialize the tracker context if any
    if ( _imp->effect->isBuiltinTrackerNode() ) {
        _imp->trackContext = TrackerContext::create( shared_from_this() );
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
Node::initNodeScriptName(const NodeSerialization* serialization, const QString& fixedName)
{
    /*
     If the serialization is not null, we are either pasting a node or loading it from a project.
     */
    NodeCollectionPtr group = getGroup();
    bool isMultiInstanceChild = false;
    if ( !_imp->multiInstanceParentName.empty() ) {
        isMultiInstanceChild = true;
    }

    if (!fixedName.isEmpty()) {

        std::string baseName = fixedName.toStdString();
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
        } while ( group && group->checkIfNodeNameExists(name, this) );

        //This version of setScriptName will not error if the name is invalid or already taken
        setScriptName_no_error_check(name);

    } else if (serialization) {
        if ( group && !group->isCacheIDAlreadyTaken( serialization->getCacheID() ) ) {
            QMutexLocker k(&_imp->nameMutex);
            _imp->cacheID = serialization->getCacheID();
        }
        const std::string& baseName = serialization->getNodeScriptName();
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
        } while ( group && group->checkIfNodeNameExists(name, this) );

        //This version of setScriptName will not error if the name is invalid or already taken
        setScriptName_no_error_check(name);
        setLabel( serialization->getNodeLabel() );

    } else {
        std::string name;
        QString pluginLabel;
        AppManager::AppTypeEnum appType = appPTR->getAppType();
        if (_imp->plugin) {
            if ( ( appType == AppManager::eAppTypeBackground) ||
                 ( appType == AppManager::eAppTypeGui) ||
                 ( appType == AppManager::eAppTypeInterpreter) ) {
                pluginLabel = _imp->plugin->getLabelWithoutSuffix();
            } else {
                pluginLabel = _imp->plugin->getPluginLabel();
            }
        }
        try {
            if (group) {
                group->initNodeName(isMultiInstanceChild ? _imp->multiInstanceParentName + '_' : pluginLabel.toStdString(), &name);
            } else {
                name = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly( pluginLabel.toStdString() );
            }
        } catch (...) {
        }

        setNameInternal(name.c_str(), false);
    }



}

void
Node::load(const CreateNodeArgs& args)
{
    ///Called from the main thread. MT-safe
    assert( QThread::currentThread() == qApp->thread() );

    ///cannot load twice
    assert(!_imp->effect);
    _imp->isPartOfProject = !args.getProperty<bool>(kCreateNodeArgsPropOutOfProject);

#ifdef NATRON_ENABLE_IO_META_NODES
    _imp->ioContainer = args.getProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer);
#endif

    NodeCollectionPtr group = getGroup();
    std::string multiInstanceParentName = args.getProperty<std::string>(kCreateNodeArgsPropMultiInstanceParentName);
    if ( !multiInstanceParentName.empty() ) {
        _imp->multiInstanceParentName = multiInstanceParentName;
        _imp->isMultiInstance = false;
        fetchParentMultiInstancePointer();
    }


    _imp->wasCreatedSilently = args.getProperty<bool>(kCreateNodeArgsPropSilent);

    NodePtr thisShared = shared_from_this();
    LibraryBinary* binary = _imp->plugin->getLibraryBinary();
    std::pair<bool, EffectBuilder> func;
    if (binary) {
        func = binary->findFunction<EffectBuilder>("BuildEffect");
    }


    boost::shared_ptr<NodeSerialization> serialization = args.getProperty<boost::shared_ptr<NodeSerialization> >(kCreateNodeArgsPropNodeSerialization);
    
    bool isSilentCreation = args.getProperty<bool>(kCreateNodeArgsPropSilent);
#ifndef NATRON_ENABLE_IO_META_NODES
    bool hasUsedFileDialog = false;
#endif

    bool hasDefaultFilename = false;
    {
        std::vector<std::string> defaultParamValues = args.getPropertyN<std::string>(kCreateNodeArgsPropNodeInitialParamValues);
        std::vector<std::string>::iterator foundFileName  = std::find(defaultParamValues.begin(), defaultParamValues.end(), std::string(kOfxImageEffectFileParamName));
        if (foundFileName != defaultParamValues.end()) {
            std::string propName(kCreateNodeArgsPropParamValue);
            propName += "_";
            propName += kOfxImageEffectFileParamName;
            hasDefaultFilename = !args.getProperty<std::string>(propName).empty();
        }
    }
    bool canOpenFileDialog = !isSilentCreation && !serialization && _imp->isPartOfProject && !hasDefaultFilename && getGroup();

    std::string argFixedName = args.getProperty<std::string>(kCreateNodeArgsPropNodeInitialName);

    
    if (func.first) {
        /*
           We are creating a built-in plug-in
         */
        _imp->effect.reset( func.second(thisShared) );
        assert(_imp->effect);


#ifdef NATRON_ENABLE_IO_META_NODES
        NodePtr ioContainer = _imp->ioContainer.lock();
        if (ioContainer) {
            ReadNode* isReader = dynamic_cast<ReadNode*>( ioContainer->getEffectInstance().get() );
            if (isReader) {
                isReader->setEmbeddedReader(thisShared);
            } else {
                WriteNode* isWriter = dynamic_cast<WriteNode*>( ioContainer->getEffectInstance().get() );
                assert(isWriter);
                if (isWriter) {
                    isWriter->setEmbeddedWriter(thisShared);
                }
            }
        }
#endif
        initNodeScriptName(serialization.get(), QString::fromUtf8(argFixedName.c_str()));
        
        _imp->effect->initializeData();

        createRotoContextConditionnally();
        createTrackerContextConditionnally();
        initializeInputs();
        initializeKnobs(serialization.get() != 0);

        refreshAcceptedBitDepths();

        _imp->effect->setDefaultMetadata();

        if (serialization) {

            _imp->effect->onKnobsAboutToBeLoaded(serialization);
            loadKnobs(*serialization);
        }
        setValuesFromSerialization(args);
        


#ifndef NATRON_ENABLE_IO_META_NODES
        std::string images;
        if (_imp->effect->isReader() && canOpenFileDialog) {
            images = getApp()->openImageFileDialog();
        } else if (_imp->effect->isWriter() && canOpenFileDialog) {
            images = getApp()->saveImageFileDialog();
        }
        if ( !images.empty() ) {
            hasUsedFileDialog = true;
            boost::shared_ptr<KnobSerialization> defaultFile = createDefaultValueForParam(kOfxImageEffectFileParamName, images);
            CreateNodeArgs::DefaultValuesList list;
            list.push_back(defaultFile);

            std::string canonicalFilename = images;
            getApp()->getProject()->canonicalizePath(canonicalFilename);
            int firstFrame, lastFrame;
            Node::getOriginalFrameRangeForReader(getPluginID(), canonicalFilename, &firstFrame, &lastFrame);
            list.push_back( createDefaultValueForParam(kReaderParamNameOriginalFrameRange, firstFrame, lastFrame) );
            setValuesFromSerialization(args);
        }
#endif
    } else {
        //ofx plugin
#ifndef NATRON_ENABLE_IO_META_NODES
        _imp->effect = appPTR->createOFXEffect(thisShared, args, canOpenFileDialog, &hasUsedFileDialog);
#else
        _imp->effect = appPTR->createOFXEffect(thisShared, args);
#endif
        assert(_imp->effect);
    }

    _imp->effect->initializeOverlayInteract();


    if ( _imp->supportedDepths.empty() ) {
        //From the spec:
        //The default for a plugin is to have none set, the plugin \em must define at least one in its describe action.
        throw std::runtime_error("Plug-in does not support 8bits, 16bits or 32bits floating point image processing.");
    }

    /*
       Set modifiable props
     */
    refreshDynamicProperties();
    onOpenGLEnabledKnobChangedOnProject(getApp()->getProject()->isOpenGLRenderActivated());

    if ( isTrackerNodePlugin() ) {
        _imp->isMultiInstance = true;
    }

    /*if (isMultiInstanceChild && !args.serialization) {
        updateEffectLabelKnob( QString::fromUtf8( getScriptName().c_str() ) );
     }*/
    restoreSublabel();

    declarePythonFields();
    if  ( getRotoContext() ) {
        declareRotoPythonField();
    }
    if ( getTrackerContext() ) {
        declareTrackerPythonField();
    }


    if (group) {
        group->notifyNodeActivated(thisShared);
    }

    //This flag is used for the Roto plug-in and for the Merge inside the rotopaint tree
    //so that if the input of the roto node is RGB, it gets converted with alpha = 0, otherwise the user
    //won't be able to paint the alpha channel
    const QString& pluginID = _imp->plugin->getPluginID();
    if ( isRotoPaintingNode() || ( pluginID == QString::fromUtf8(PLUGINID_OFX_ROTO) ) ) {
        _imp->useAlpha0ToConvertFromRGBToRGBA = true;
    }

    if (!serialization) {
        computeHash();
    }

    assert(_imp->effect);

    _imp->pluginSafety = _imp->effect->renderThreadSafety();
    _imp->currentThreadSafety = _imp->pluginSafety;


    bool isLoadingPyPlug = getApp()->isCreatingPythonGroup();

    _imp->effect->onEffectCreated(canOpenFileDialog, args);

    _imp->nodeCreated = true;

    if ( !getApp()->isCreatingNodeTree() ) {
        refreshAllInputRelatedData(!serialization);
    }


    _imp->runOnNodeCreatedCB(!serialization && !isLoadingPyPlug);
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
    if ( !isMultiThreadingSupportEnabledForPlugin() ) {
        return eRenderSafetyInstanceSafe;
    }
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

    if (_imp->plugin) {
        PluginOpenGLRenderSupport pluginProp = _imp->plugin->getPluginOpenGLRenderSupport();
        if (pluginProp != ePluginOpenGLRenderSupportYes) {
            return pluginProp;
        }
    }

    if (!getApp()->getProject()->isGPURenderingEnabledInProject()) {
        return ePluginOpenGLRenderSupportNone;
    }

    // Ok still turned on, check the value of the opengl support knob in the Node page
    boost::shared_ptr<KnobChoice> openglSupportKnob = _imp->openglRenderingEnabledKnob.lock();
    if (openglSupportKnob) {
        int index = openglSupportKnob->getValue();
        if (index == 1) {
            return ePluginOpenGLRenderSupportNone;
        } else if (index == 2 && getApp()->isBackground()) {
            return ePluginOpenGLRenderSupportNone;
        }
    }

    // Descriptor returned that it supported OpenGL, let's see if it turned off/on in the instance the OpenGL rendering
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
    PluginOpenGLRenderSupport pluginGLSupport = ePluginOpenGLRenderSupportNone;
    if (_imp->plugin) {
        pluginGLSupport = _imp->plugin->getPluginOpenGLRenderSupport();
        if (_imp->plugin->isOpenGLEnabled() && pluginGLSupport == ePluginOpenGLRenderSupportYes) {
            // Ok the plug-in supports OpenGL, figure out now if can be turned on/off by the instance
            pluginGLSupport = _imp->effect->supportsOpenGLRender();
        }
    }


    setCurrentOpenGLRenderSupport(pluginGLSupport);
    bool tilesSupported = _imp->effect->supportsTiles();
    bool multiResSupported = _imp->effect->supportsMultiResolution();
    bool canTransform = _imp->effect->getCanTransform();
    RenderSafetyEnum safety = _imp->effect->renderThreadSafety();
    setRenderThreadSafety(safety);
    setCurrentSupportTiles(multiResSupported && tilesSupported);
    setCurrentSequentialRenderSupport( _imp->effect->getSequentialPreference() );
    setCurrentCanTransform(canTransform);
}

bool
Node::isRenderScaleSupportEnabledForPlugin() const
{
    return _imp->plugin ? _imp->plugin->isRenderScaleEnabled() : true;
}

bool
Node::isMultiThreadingSupportEnabledForPlugin() const
{
    return _imp->plugin ? _imp->plugin->isMultiThreadingEnabled() : true;
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
Node::getPaintStrokeRoD(double time,
                        RectD* bbox) const
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
                               unsigned int mipmapLevel,
                               std::list<std::list<std::pair<Point, double> > >* strokes,
                               int* strokeIndex) const
{
    bool duringPaintStroke;
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        duringPaintStroke = _imp->duringPaintStrokeCreation;
    }

    if (duringPaintStroke) {
        getApp()->getLastPaintStrokePoints(strokes, strokeIndex);
        //adapt to mipmaplevel if needed
        if (mipmapLevel == 0) {
            return;
        }
        int pot = 1 << mipmapLevel;
        for (std::list<std::list<std::pair<Point, double> > >::iterator it = strokes->begin(); it != strokes->end(); ++it) {
            for (std::list<std::pair<Point, double> >::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
                std::pair<Point, double> &p = *it2;
                p.first.x /= pot;
                p.first.y /= pot;
            }
        }
    } else {
        boost::shared_ptr<RotoDrawableItem> item = _imp->paintStroke.lock();
        RotoStrokeItem* stroke = dynamic_cast<RotoStrokeItem*>( item.get() );
        assert(stroke);
        if (!stroke) {
            throw std::logic_error("");
        }
        stroke->evaluateStroke(mipmapLevel, time, strokes);
        *strokeIndex = 0;
    }
}

boost::shared_ptr<Image>
Node::getOrRenderLastStrokeImage(unsigned int mipMapLevel,
                                 double par,
                                 const ImagePlaneDesc& components,
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
    std::list<std::pair<Point, double> > lastStrokePoints;
    double distNextIn = 0.;
    boost::shared_ptr<Image> strokeImage;
    getApp()->getRenderStrokeData(&lastStrokeBbox, &lastStrokePoints, &distNextIn, &strokeImage);
    double distToNextOut = stroke->renderSingleStroke(lastStrokeBbox, lastStrokePoints, mipMapLevel, par, components, depth, distNextIn, &strokeImage);

    getApp()->updateStrokeImage(strokeImage, distToNextOut, true);

    return strokeImage;
}

void
Node::refreshAcceptedBitDepths()
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->effect->addSupportedBitDepth(&_imp->supportedDepths);
}

bool
Node::isNodeCreated() const
{
    return _imp->nodeCreated;
}

void
Node::setProcessChannelsValues(bool doR,
                               bool doG,
                               bool doB,
                               bool doA)
{
    boost::shared_ptr<KnobBool> eR = _imp->enabledChan[0].lock();

    if (eR) {
        eR->setValue(doR);
    }
    boost::shared_ptr<KnobBool> eG = _imp->enabledChan[1].lock();
    if (eG) {
        eG->setValue(doG);
    }
    boost::shared_ptr<KnobBool> eB = _imp->enabledChan[2].lock();
    if (eB) {
        eB->setValue(doB);
    }
    boost::shared_ptr<KnobBool> eA = _imp->enabledChan[3].lock();
    if (eA) {
        eA->setValue(doA);
    }
}

bool
Node::setStreamWarningInternal(StreamWarningEnum warning,
                               const QString& message)
{
    assert( QThread::currentThread() == qApp->thread() );
    std::map<Node::StreamWarningEnum, QString>::iterator found = _imp->streamWarnings.find(warning);
    if ( found == _imp->streamWarnings.end() ) {
        _imp->streamWarnings.insert( std::make_pair(warning, message) );

        return true;
    } else {
        if (found->second != message) {
            found->second = message;

            return true;
        }
    }

    return false;
}

void
Node::setStreamWarning(StreamWarningEnum warning,
                       const QString& message)
{
    if ( setStreamWarningInternal(warning, message) ) {
        Q_EMIT streamWarningsChanged();
    }
}

void
Node::setStreamWarnings(const std::map<StreamWarningEnum, QString>& warnings)
{
    bool changed = false;

    for (std::map<StreamWarningEnum, QString>::const_iterator it = warnings.begin(); it != warnings.end(); ++it) {
        changed |= setStreamWarningInternal(it->first, it->second);
    }
    if (changed) {
        Q_EMIT streamWarningsChanged();
    }
}

void
Node::clearStreamWarning(StreamWarningEnum warning)
{
    assert( QThread::currentThread() == qApp->thread() );
    std::map<Node::StreamWarningEnum, QString>::iterator found = _imp->streamWarnings.find(warning);
    if ( ( found == _imp->streamWarnings.end() ) || found->second.isEmpty() ) {
        return;
    }
    found->second.clear();
    Q_EMIT streamWarningsChanged();
}

void
Node::getStreamWarnings(std::map<StreamWarningEnum, QString>* warnings) const
{
    assert( QThread::currentThread() == qApp->thread() );
    *warnings = _imp->streamWarnings;
}

void
Node::declareRotoPythonField()
{
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    assert(_imp->rotoContext);
    std::string appID = getApp()->getAppIDString();
    std::string nodeName = getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = nodeFullName + ".roto = " + nodeFullName + ".getRotoContext()\n";
    if ( !appPTR->isBackground() ) {
        getApp()->printAutoDeclaredVariable(script);
    }
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("Node::declareRotoPythonField(): interpretPythonScript(" + script + ") failed!");
    }
    _imp->rotoContext->declarePythonFields();
}

void
Node::declareTrackerPythonField()
{
    if (getScriptName_mt_safe().empty()) {
        return;
    }

    assert(_imp->trackContext);
    std::string appID = getApp()->getAppIDString();
    std::string nodeName = getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = nodeFullName + ".tracker = " + nodeFullName + ".getTrackerContext()\n";
    if ( !appPTR->isBackground() ) {
        getApp()->printAutoDeclaredVariable(script);
    }
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("Node::declareTrackerPythonField(): interpretPythonScript(" + script + ") failed!");
    }
    _imp->trackContext->declarePythonFields();
}

NodeCollectionPtr
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
        if ( (*it)->getScriptName() == _imp->multiInstanceParentName ) {
            ///no need to store the boost pointer because the main instance lives the same time
            ///as the child
            _imp->multiInstanceParent = *it;
            (*it)->_imp->appendChild(thisShared);
            QObject::connect( it->get(), SIGNAL(inputChanged(int)), this, SLOT(onParentMultiInstanceInputChanged(int)) );
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
        children->push_back( it->lock() );
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

    U64 oldHash, newHash;
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
            ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( _imp->effect.get() );

            if (isViewer) {
                int activeInput[2];
                isViewer->getActiveInputs(activeInput[0], activeInput[1]);

                for (int i = 0; i < 2; ++i) {
                    NodePtr input = getInput(activeInput[i]);
                    if (input) {
                        _imp->hash.append( input->getHashValue() );
                    }
                }
            } else {
                for (U32 i = 0; i < _imp->inputs.size(); ++i) {
                    NodePtr input = getInput(i);
                    if (input) {
                        //Since the rotopaint node is connected to the internal nodes of the tree, don't change their hash
                        if ( attachedStroke && (input == attachedStrokeContextNode) ) {
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
        Hash64_appendQString( &_imp->hash, QString::fromUtf8( getScriptName().c_str() ) );

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
        if ( _imp->nodeCreated && !getApp()->getProject()->isProjectClosing() ) {
            /*
             * We changed the node hash. That means all cache entries for this node with a different hash
             * are impossible to re-create again. Just discard them all. This is done in a separate thread.
             */
            removeAllImagesFromCacheWithMatchingIDAndDifferentKey(newHash);
        }
    }

    return hashChanged;
} // Node::computeHashInternal

void
Node::computeHashRecursive(std::list<Node*>& marked)
{
    if ( std::find(marked.begin(), marked.end(), this) != marked.end() ) {
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
        if ( isRotoPaint && attachedStroke && (attachedStroke->getContext()->getNode().get() == this) ) {
            continue;
        }
        (*it)->computeHashRecursive(marked);
    }


    ///If the node has a rotopaint tree, compute the hash of the nodes in the tree
    if (_imp->rotoContext) {
        NodesList allItems;
        _imp->rotoContext->getRotoPaintTreeNodes(&allItems);
        for (NodesList::iterator it = allItems.begin(); it != allItems.end(); ++it) {
            (*it)->computeHashRecursive(marked);
        }
    }
}

void
Node::removeAllImagesFromCacheWithMatchingIDAndDifferentKey(U64 nodeHashKey)
{
    AppInstancePtr app = getApp();

    if (!app) {
        return;
    }
    boost::shared_ptr<Project> proj = app->getProject();

    if ( proj->isProjectClosing() || proj->isLoadingProject() ) {
        return;
    }
    appPTR->removeAllImagesFromCacheWithMatchingIDAndDifferentKey(this, nodeHashKey);
    appPTR->removeAllImagesFromDiskCacheWithMatchingIDAndDifferentKey(this, nodeHashKey);
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( _imp->effect.get() );
    if (isViewer) {
        //Also remove from viewer cache
        appPTR->removeAllTexturesFromCacheWithMatchingIDAndDifferentKey(this, nodeHashKey);
    }
}

void
Node::removeAllImagesFromCache(bool blocking)
{
    AppInstancePtr app = getApp();

    if (!app) {
        return;
    }
    boost::shared_ptr<Project> proj = app->getProject();

    if ( proj->isProjectClosing() || proj->isLoadingProject() ) {
        return;
    }
    appPTR->removeAllCacheEntriesForHolder(this, blocking);
}

void
Node::doComputeHashOnMainThread()
{
    assert( QThread::currentThread() == qApp->thread() );
    computeHash();
}

void
Node::computeHash()
{
    if ( QThread::currentThread() != qApp->thread() ) {
        Q_EMIT mustComputeHashOnMainThread();

        return;
    }
    std::list<Node*> marked;
    computeHashRecursive(marked);
} // computeHash

void
Node::setValuesFromSerialization(const CreateNodeArgs& args)
{
    
    std::vector<std::string> params = args.getPropertyN<std::string>(kCreateNodeArgsPropNodeInitialParamValues);
    
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->knobsInitialized);
    const std::vector<KnobIPtr> & nodeKnobs = getKnobs();

    for (std::size_t i = 0; i < params.size(); ++i) {
        for (U32 j = 0; j < nodeKnobs.size(); ++j) {
            if (nodeKnobs[j]->getName() == params[i]) {
                
                KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(nodeKnobs[j].get());
                KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(nodeKnobs[j].get());
                KnobDoubleBase* isDbl = dynamic_cast<KnobDoubleBase*>(nodeKnobs[j].get());
                KnobStringBase* isStr = dynamic_cast<KnobStringBase*>(nodeKnobs[j].get());
                int nDims = nodeKnobs[j]->getDimension();

                std::string propName = kCreateNodeArgsPropParamValue;
                propName += "_";
                propName += params[i];
                if (isBool) {
                    std::vector<bool> v = args.getPropertyN<bool>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isBool->setValue(v[d], ViewSpec(0), d);
                    }
                } else if (isInt) {
                    std::vector<int> v = args.getPropertyN<int>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isInt->setValue(v[d], ViewSpec(0), d);
                    }
                } else if (isDbl) {
                    std::vector<double> v = args.getPropertyN<double>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isDbl->setValue(v[d], ViewSpec(0), d );
                    }
                } else if (isStr) {
                    std::vector<std::string> v = args.getPropertyN<std::string>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isStr->setValue(v[d],ViewSpec(0), d );
                    }
                }
                break;
            }
        }
    }
    
    
}

void
Node::loadKnobs(const NodeSerialization & serialization,
                bool updateKnobGui)
{
    ///Only called from the main thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->knobsInitialized);
    if ( serialization.isNull() ) {
        return;
    }


    {
        QMutexLocker k(&_imp->createdComponentsMutex);
        _imp->createdComponents = serialization.getUserCreatedComponents();
    }

    const std::vector<KnobIPtr> & nodeKnobs = getKnobs();
    ///for all knobs of the node
    for (U32 j = 0; j < nodeKnobs.size(); ++j) {
        loadKnob(nodeKnobs[j], serialization, updateKnobGui);
    }
    ///now restore the roto context if the node has a roto context
    if (serialization.hasRotoContext() && _imp->rotoContext) {
        _imp->rotoContext->load( serialization.getRotoContext() );
    }

    if (serialization.hasTrackerContext() && _imp->trackContext) {
        _imp->trackContext->load( serialization.getTrackerContext() );
    }

    restoreUserKnobs(serialization);

    setKnobsAge( serialization.getKnobsAge() );


    _imp->effect->onKnobsLoaded();
}

void
Node::restoreSublabel()
{
    //Check if natron custom tags are present and insert them if needed
    /// If the node has a sublabel, restore it in the label
    boost::shared_ptr<KnobString> labelKnob = _imp->nodeLabelKnob.lock();

    if (labelKnob) {
        QString labeltext = QString::fromUtf8( labelKnob->getValue().c_str() );
        int foundNatronCustomTag = labeltext.indexOf( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START) );
        if (foundNatronCustomTag == -1) {
            KnobIPtr sublabelKnob = getKnobByName(kNatronOfxParamStringSublabelName);
            if (sublabelKnob) {
                KnobString* sublabelKnobIsString = dynamic_cast<KnobString*>( sublabelKnob.get() );
                if (sublabelKnobIsString) {
                    QString sublabel = QString::fromUtf8( sublabelKnobIsString->getValue(0).c_str() );
                    if ( !sublabel.isEmpty() ) {
                        int fontEndTagFound = labeltext.lastIndexOf( QString::fromUtf8(kFontEndTag) );
                        if (fontEndTagFound == -1) {
                            labeltext.append( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START) );
                            labeltext.append( QLatin1Char('(') + sublabel + QLatin1Char(')') );
                            labeltext.append( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END) );
                        } else {
                            QString toAppend( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START) );
                            toAppend += QLatin1Char('(');
                            toAppend += sublabel;
                            toAppend += QLatin1Char(')');
                            toAppend += QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END);
                            labeltext.insert(fontEndTagFound, toAppend);
                        }
                        labelKnob->setValue( labeltext.toStdString() );
                    }
                }
            }
        }
    }
}

void
Node::loadKnob(const KnobIPtr & knob,
               const NodeSerialization & serialization,
               bool /*updateKnobGui*/)
{

    const NodeSerialization::KnobValues& knobsValues = serialization.getKnobsValues();

    ///try to find a serialized value for this knob
    const ProjectBeingLoadedInfo projectInfos = getApp()->getProjectBeingLoadedInfo();

    std::string pluginID = getPluginID();

    {
        // Use the plug-in ID of the encoder/decoder for reader and writer
        ReadNode* isReadNode = dynamic_cast<ReadNode*>( getEffectInstance().get() );
        WriteNode* isWriteNode = dynamic_cast<WriteNode*>( getEffectInstance().get() );
        if (isReadNode) {
            NodePtr p = isReadNode->getEmbeddedReader();
            if (p) {
                pluginID = p->getPluginID();
            }
        } else if (isWriteNode) {
            NodePtr p = isWriteNode->getEmbeddedWriter();
            if (p) {
                pluginID = p->getPluginID();
            }
        }
    }


    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knob.get() );
    if (isChoice) {

        if (projectInfos.vMajor == 2 && projectInfos.vMinor < 3) {
            // Before Natron 2.2.3, all dynamic choice parameters for multiplane had a string parameter.
            // The string parameter had the same name as the choice parameter plus "Choice" appended.
            // If we found such a parameter, retrieve the string from it.
            std::string stringParamName = isChoice->getName() + "Choice";
            for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
                if ( (*it)->getName() == stringParamName ) {
                    boost::shared_ptr<KnobString> stringKnob = boost::dynamic_pointer_cast<KnobString>((*it)->getKnob());
                    if (stringKnob) {
                        std::string serializedString = stringKnob->getValue();
                        if ((*it)->_version < KNOB_SERIALIZATION_CHANGE_PLANES_SERIALIZATION) {
                            filterKnobChoiceOptionCompat(getPluginID(), getMajorVersion(), getMinorVersion(), projectInfos.vMajor, projectInfos.vMinor, projectInfos.vRev, isChoice->getName(), &serializedString);
                        }
                        isChoice->setActiveEntry(ChoiceOption(serializedString));
                    }
                    break;
                }
            }

        }
    }

    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {

        std::string serializedName = (*it)->getName();
        bool foundMatch = false;
        if (serializedName == knob->getName()) {
            foundMatch = true;
        } else {
            if (filterKnobNameCompat(getPluginID(), getMajorVersion(), getMinorVersion(), projectInfos.vMajor, projectInfos.vMinor, projectInfos.vRev, &serializedName)) {
                if (serializedName == knob->getName()) {
                    foundMatch = true;
                }
            }

        }
        if (!foundMatch) {
            continue;
        }

        // don't load the value if the Knob is not persistent! (it is just the default value in this case)
        ///EDIT: Allow non persistent params to be loaded if we found a valid serialization for them
        //if ( knob->getIsPersistent() ) {
        KnobIPtr serializedKnob = (*it)->getKnob();

        // A knob might change its type between versions, do not load it
        if ( knob->typeName() != serializedKnob->typeName() ) {
            continue;
        }

        knob->cloneDefaultValues( serializedKnob.get() );

        if (isChoice) {
            const TypeExtraData* extraData = (*it)->getExtraData();
            const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>(extraData);
            assert(choiceData);
            if (choiceData) {
                KnobChoice* choiceSerialized = dynamic_cast<KnobChoice*>( serializedKnob.get() );
                assert(choiceSerialized);
                if (choiceSerialized) {
                    std::string optionID = choiceData->_choiceString;
                    // first, try to get the id the easy way ( see choiceMatch() )
                    int id = isChoice->choiceRestorationId(choiceSerialized, optionID);
                    if (id < 0) {
                        // no luck, try the filters
                        filterKnobChoiceOptionCompat(getPluginID(), serialization.getPluginMajorVersion(), serialization.getPluginMinorVersion(), projectInfos.vMajor, projectInfos.vMinor, projectInfos.vRev, serializedName, &optionID);
                        id = isChoice->choiceRestorationId(choiceSerialized, optionID);
                    }
                    isChoice->choiceRestoration(choiceSerialized, optionID, id);
                    //if (id >= 0) {
                    //    choiceData->_choiceString = isChoice->getEntry(id).id;
                    //}
                }
            }
        } else {
            // There is a case where the dimension of a parameter might have changed between versions, e.g:
            // the size parameter of the Blur node was previously a Double1D and has become a Double2D to control
            // both dimensions.
            // For compatibility, we do not load only the first dimension, otherwise the result wouldn't be the same,
            // instead we replicate the last dimension of the serialized knob to all other remaining dimensions to fit the
            // knob's dimensions.
            if ( serializedKnob->getDimension() < knob->getDimension() ) {
                int nSerDims = serializedKnob->getDimension();
                for (int i = 0; i < nSerDims; ++i) {
                    knob->cloneAndUpdateGui(serializedKnob.get(), i);
                }
                for (int i = nSerDims; i < knob->getDimension(); ++i) {
                    knob->cloneAndUpdateGui(serializedKnob.get(), i, nSerDims - 1);
                }
            } else {
                knob->cloneAndUpdateGui( serializedKnob.get() );
            }
            knob->setSecret( serializedKnob->getIsSecret() );
            if ( knob->getDimension() == serializedKnob->getDimension() ) {
                for (int i = 0; i < knob->getDimension(); ++i) {
                    knob->setEnabled( i, serializedKnob->isEnabled(i) );
                }
            }
        }

        if (knob->getName() == kOfxImageEffectFileParamName) {
            computeFrameRangeForReader( knob.get() );
        }

        //}
        break;

    }
  
} // Node::loadKnob

void
Node::Implementation::restoreKnobLinksRecursive(const GroupKnobSerialization* group,
                                                const NodesList & allNodes,
                                                const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    const std::list<boost::shared_ptr<KnobSerializationBase> >&  children = group->getChildren();

    for (std::list<boost::shared_ptr<KnobSerializationBase> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>( it->get() );
        KnobSerialization* isRegular = dynamic_cast<KnobSerialization*>( it->get() );
        assert(isGrp || isRegular);
        if (isGrp) {
            restoreKnobLinksRecursive(isGrp, allNodes, oldNewScriptNamesMapping);
        } else if (isRegular) {
            KnobIPtr knob =  _publicInterface->getKnobByName( isRegular->getName() );
            if (!knob) {
                LogEntry::LogEntryColor c;
                if (_publicInterface->getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }

                QString err = tr("Could not find a parameter named %1").arg( QString::fromUtf8( (*it)->getName().c_str() ) );
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( _publicInterface->getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), err, false, c);
                continue;
            }
            isRegular->restoreKnobLinks(knob, allNodes, oldNewScriptNamesMapping);
            isRegular->restoreExpressions(knob, oldNewScriptNamesMapping);
        }
    }
}

void
Node::restoreKnobsLinks(const NodeSerialization & serialization,
                        const NodesList & allNodes,
                        const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    const NodeSerialization::KnobValues & knobsValues = serialization.getKnobsValues();
    ///try to find a serialized value for this knob
    for (NodeSerialization::KnobValues::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        KnobIPtr knob = getKnobByName( (*it)->getName() );
        if (!knob) {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }

            QString err = tr("Could not find a parameter named %1").arg( QString::fromUtf8( (*it)->getName().c_str() ) );
            appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), err, false, c);
            continue;
        }
        (*it)->restoreKnobLinks(knob, allNodes, oldNewScriptNamesMapping);
        (*it)->restoreExpressions(knob, oldNewScriptNamesMapping);
    }

    const std::list<boost::shared_ptr<GroupKnobSerialization> >& userKnobs = serialization.getUserPages();
    for (std::list<boost::shared_ptr<GroupKnobSerialization > >::const_iterator it = userKnobs.begin(); it != userKnobs.end(); ++it) {
        _imp->restoreKnobLinksRecursive( (*it).get(), allNodes, oldNewScriptNamesMapping );
    }
}

void
Node::setPagesOrder(const std::list<std::string>& pages)
{
    //re-order the pages
    std::list<KnobIPtr> pagesOrdered;

    for (std::list<std::string>::const_iterator it = pages.begin(); it != pages.end(); ++it) {
        const KnobsVec &knobs = getKnobs();
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ( (*it2)->getName() == *it ) {
                pagesOrdered.push_back(*it2);
                _imp->effect->removeKnobFromList( it2->get() );
                break;
            }
        }
    }
    int index = 0;
    for (std::list<KnobIPtr>::iterator it =  pagesOrdered.begin(); it != pagesOrdered.end(); ++it, ++index) {
        _imp->effect->insertKnob(index, *it);
    }
}

std::list<std::string>
Node::getPagesOrder() const
{
    const KnobsVec& knobs = getKnobs();
    std::list<std::string> ret;

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPage* ispage = dynamic_cast<KnobPage*>( it->get() );
        if (ispage) {
            ret.push_back( ispage->getName() );
        }
    }

    return ret;
}

void
Node::restoreUserKnobs(const NodeSerialization& serialization)
{
    const std::list<boost::shared_ptr<GroupKnobSerialization> >& userPages = serialization.getUserPages();

    for (std::list<boost::shared_ptr<GroupKnobSerialization> >::const_iterator it = userPages.begin(); it != userPages.end(); ++it) {
        KnobIPtr found = getKnobByName( (*it)->getName() );
        boost::shared_ptr<KnobPage> page;
        if (!found) {
            page = AppManager::createKnob<KnobPage>(_imp->effect.get(), (*it)->getLabel(), 1, false);
            page->setAsUserKnob(true);
            page->setName( (*it)->getName() );
        } else {
            page = boost::dynamic_pointer_cast<KnobPage>(found);
        }
        if (page) {
            _imp->restoreUserKnobsRecursive( (*it)->getChildren(), boost::shared_ptr<KnobGroup>(), page );
        }
    }
    setPagesOrder( serialization.getPagesOrdered() );
}

void
Node::Implementation::restoreUserKnobsRecursive(const std::list<boost::shared_ptr<KnobSerializationBase> >& knobs,
                                                const boost::shared_ptr<KnobGroup>& group,
                                                const boost::shared_ptr<KnobPage>& page)
{
    for (std::list<boost::shared_ptr<KnobSerializationBase> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>( it->get() );
        KnobSerialization* isRegular = dynamic_cast<KnobSerialization*>( it->get() );
        assert(isGrp || isRegular);

        KnobIPtr found = _publicInterface->getKnobByName( (*it)->getName() );

        if (isGrp) {
            boost::shared_ptr<KnobGroup> grp;
            if (!found) {
                grp = AppManager::createKnob<KnobGroup>(effect.get(), isGrp->getLabel(), 1, false);
            } else {
                grp = boost::dynamic_pointer_cast<KnobGroup>(found);
                if (!grp) {
                    continue;
                }
            }
            grp->setAsUserKnob(true);
            grp->setName( (*it)->getName() );
            if ( isGrp && isGrp->isSetAsTab() ) {
                grp->setAsTab();
            }
            page->addKnob(grp);
            if (group) {
                group->addKnob(grp);
            }
            grp->setValue( isGrp->isOpened() );
            restoreUserKnobsRecursive(isGrp->getChildren(), grp, page);
        } else if (isRegular) {
            assert( isRegular->isUserKnob() );
            KnobIPtr sKnob = isRegular->getKnob();
            KnobIPtr knob;
            KnobInt* isInt = dynamic_cast<KnobInt*>( sKnob.get() );
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>( sKnob.get() );
            KnobBool* isBool = dynamic_cast<KnobBool*>( sKnob.get() );
            KnobChoice* isChoice = dynamic_cast<KnobChoice*>( sKnob.get() );
            KnobColor* isColor = dynamic_cast<KnobColor*>( sKnob.get() );
            KnobString* isStr = dynamic_cast<KnobString*>( sKnob.get() );
            KnobFile* isFile = dynamic_cast<KnobFile*>( sKnob.get() );
            KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>( sKnob.get() );
            KnobPath* isPath = dynamic_cast<KnobPath*>( sKnob.get() );
            KnobButton* isBtn = dynamic_cast<KnobButton*>( sKnob.get() );
            KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( sKnob.get() );
            KnobParametric* isParametric = dynamic_cast<KnobParametric*>( sKnob.get() );

            assert(isInt || isDbl || isBool || isChoice || isColor || isStr || isFile || isOutFile || isPath || isBtn || isSep || isParametric);

            if (isInt) {
                boost::shared_ptr<KnobInt> k;

                if (!found) {
                    k = AppManager::createKnob<KnobInt>(effect.get(), isRegular->getLabel(),
                                                        sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobInt>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    std::vector<int> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < k->getDimension(); ++i) {
                        minimums.push_back(data->min);
                        maximums.push_back(data->max);
                        dminimums.push_back(data->dmin);
                        dmaximums.push_back(data->dmax);
                    }
                    k->setMinimumsAndMaximums(minimums, maximums);
                    k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                }
                knob = k;
            } else if (isDbl) {
                boost::shared_ptr<KnobDouble> k;
                if (!found) {
                    k = AppManager::createKnob<KnobDouble>(effect.get(), isRegular->getLabel(),
                                                           sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobDouble>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    std::vector<double> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < k->getDimension(); ++i) {
                        minimums.push_back(data->min);
                        maximums.push_back(data->max);
                        dminimums.push_back(data->dmin);
                        dmaximums.push_back(data->dmax);
                    }
                    k->setMinimumsAndMaximums(minimums, maximums);
                    k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                }
                knob = k;

                if ( isRegular->getUseHostOverlayHandle() ) {
                    KnobDouble* isDbl = dynamic_cast<KnobDouble*>( knob.get() );
                    if (isDbl) {
                        isDbl->setHasHostOverlayHandle(true);
                    }
                }
            } else if (isBool) {
                boost::shared_ptr<KnobBool> k;
                if (!found) {
                    k = AppManager::createKnob<KnobBool>(effect.get(), isRegular->getLabel(),
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
                    k = AppManager::createKnob<KnobChoice>(effect.get(), isRegular->getLabel(),
                                                           sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobChoice>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    std::vector<ChoiceOption> options(data->_entries.size());
                    for (std::size_t i =  0; i < options.size(); ++i) {
                        options[i].id = data->_entries[i];
                        if (i < data->_helpStrings.size()) {
                            options[i].tooltip = data->_helpStrings[i];
                        }
                    }
                    k->populateChoices(options);
                }
                knob = k;
            } else if (isColor) {
                boost::shared_ptr<KnobColor> k;
                if (!found) {
                    k = AppManager::createKnob<KnobColor>(effect.get(), isRegular->getLabel(),
                                                          sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobColor>(found);
                    if (!k) {
                        continue;
                    }
                }
                const ValueExtraData* data = dynamic_cast<const ValueExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    std::vector<double> minimums, maximums, dminimums, dmaximums;
                    for (int i = 0; i < k->getDimension(); ++i) {
                        minimums.push_back(data->min);
                        maximums.push_back(data->max);
                        dminimums.push_back(data->dmin);
                        dmaximums.push_back(data->dmax);
                    }
                    k->setMinimumsAndMaximums(minimums, maximums);
                    k->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                }
                knob = k;
            } else if (isStr) {
                boost::shared_ptr<KnobString> k;
                if (!found) {
                    k = AppManager::createKnob<KnobString>(effect.get(), isRegular->getLabel(),
                                                           sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobString>(found);
                    if (!k) {
                        continue;
                    }
                }
                const TextExtraData* data = dynamic_cast<const TextExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data) {
                    if (data->label) {
                        k->setAsLabel();
                    } else if (data->multiLine) {
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
                    k = AppManager::createKnob<KnobFile>(effect.get(), isRegular->getLabel(),
                                                         sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobFile>(found);
                    if (!k) {
                        continue;
                    }
                }
                const FileExtraData* data = dynamic_cast<const FileExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data && data->useSequences) {
                    k->setAsInputImage();
                }
                knob = k;
            } else if (isOutFile) {
                boost::shared_ptr<KnobOutputFile> k;
                if (!found) {
                    k = AppManager::createKnob<KnobOutputFile>(effect.get(), isRegular->getLabel(),
                                                               sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobOutputFile>(found);
                    if (!k) {
                        continue;
                    }
                }
                const FileExtraData* data = dynamic_cast<const FileExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data && data->useSequences) {
                    k->setAsOutputImageFile();
                }
                knob = k;
            } else if (isPath) {
                boost::shared_ptr<KnobPath> k;
                if (!found) {
                    k = AppManager::createKnob<KnobPath>(effect.get(), isRegular->getLabel(),
                                                         sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobPath>(found);
                    if (!k) {
                        continue;
                    }
                }
                const PathExtraData* data = dynamic_cast<const PathExtraData*>( isRegular->getExtraData() );
                assert(data);
                if (data && data->multiPath) {
                    k->setMultiPath(true);
                }
                knob = k;
            } else if (isBtn) {
                boost::shared_ptr<KnobButton> k;
                if (!found) {
                    k = AppManager::createKnob<KnobButton>(effect.get(), isRegular->getLabel(),
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
                    k = AppManager::createKnob<KnobSeparator>(effect.get(), isRegular->getLabel(),
                                                              sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobSeparator>(found);
                    if (!k) {
                        continue;
                    }
                }
                knob = k;
            } else if (isParametric) {
                boost::shared_ptr<KnobParametric> k;
                if (!found) {
                    k = AppManager::createKnob<KnobParametric>(effect.get(), isRegular->getLabel(), sKnob->getDimension(), false);
                } else {
                    k = boost::dynamic_pointer_cast<KnobParametric>(found);
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
            knob->cloneDefaultValues( sKnob.get() );
            if (isChoice) {
                const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>( isRegular->getExtraData() );
                assert(choiceData);
                KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knob.get() );
                assert(isChoice);
                if (choiceData && isChoice) {
                    KnobChoice* choiceSerialized = dynamic_cast<KnobChoice*>( sKnob.get() );
                    if (choiceSerialized) {
                        std::string optionID = choiceData->_choiceString;
                        // first, try to get the id the easy way ( see choiceMatch() )
                        int id = isChoice->choiceRestorationId(choiceSerialized, optionID);
#pragma message WARN("TODO: choice id filters")
                        //if (id < 0) {
                        //    // no luck, try the filters
                        //    filterKnobChoiceOptionCompat(getPluginID(), serialization.getPluginMajorVersion(), serialization.getPluginMinorVersion(), projectInfos.vMajor, projectInfos.vMinor, projectInfos.vRev, serializedName, &optionID);
                        //    id = isChoice->choiceRestorationId(choiceSerialized, optionID);
                        //}
                        isChoice->choiceRestoration(choiceSerialized, optionID, id);
                    }
                }
            } else {
                knob->clone( sKnob.get() );
            }
            knob->setAsUserKnob(true);
            if (group) {
                group->addKnob(knob);
            } else if (page) {
                page->addKnob(knob);
            }

            knob->setIsPersistent( isRegular->isPersistent() );
            knob->setAnimationEnabled( isRegular->isAnimationEnabled() );
            knob->setEvaluateOnChange( isRegular->getEvaluatesOnChange() );
            knob->setName( isRegular->getName() );
            knob->setHintToolTip( isRegular->getHintToolTip() );
            if ( !isRegular->triggerNewLine() ) {
                knob->setAddNewLine(false);
            }
        }
    }
} // Node::Implementation::restoreUserKnobsRecursive

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

    NodeGuiIPtr nodeGui = getNodeGui();
    if (nodeGui) {
        if ( nodeGui->hasHostOverlay() ) {
            return true;
        }
    }

    return _imp->effect->hasOverlay();
}

void
Node::Implementation::abortPreview_non_blocking()
{
    bool computing;
    {
        QMutexLocker locker(&computingPreviewMutex);
        computing = computingPreview;
    }

    if (computing) {
        QMutexLocker l(&mustQuitPreviewMutex);
        ++mustQuitPreview;
    }
}

void
Node::Implementation::abortPreview_blocking(bool allowPreviewRenders)
{
    bool computing;
    {
        QMutexLocker locker(&computingPreviewMutex);
        computing = computingPreview;
        previewThreadQuit = !allowPreviewRenders;
    }

    if (computing) {
        QMutexLocker l(&mustQuitPreviewMutex);
        assert(!mustQuitPreview);
        ++mustQuitPreview;
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
        if (mustQuitPreview || previewThreadQuit) {
            mustQuitPreview = 0;
            mustQuitPreviewCond.wakeOne();

            return true;
        }

        return false;
    }
}

bool
Node::areAllProcessingThreadsQuit() const
{
    {
        QMutexLocker locker(&_imp->mustQuitPreviewMutex);
        if (!_imp->previewThreadQuit) {
            return false;
        }
    }

    //If this effect has a RenderEngine, make sure it is finished
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( _imp->effect.get() );

    if (isOutput) {
        if ( isOutput->getRenderEngine()->hasThreadsAlive() ) {
            return false;
        }
    }

    boost::shared_ptr<TrackerContext> trackerContext = getTrackerContext();
    if (trackerContext) {
        if ( !trackerContext->hasTrackerThreadQuit() ) {
            return false;
        }
    }

    return true;
}

void
Node::quitAnyProcessing_non_blocking()
{
    //If this effect has a RenderEngine, make sure it is finished
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( _imp->effect.get() );

    if (isOutput) {
        isOutput->getRenderEngine()->quitEngine(true);
    }

    //Returns when the preview is done computign
    _imp->abortPreview_non_blocking();

    boost::shared_ptr<TrackerContext> trackerContext = getTrackerContext();
    if (trackerContext) {
        trackerContext->quitTrackerThread_non_blocking();
    }

    if ( isRotoPaintingNode() ) {
        NodesList rotopaintNodes;
        getRotoContext()->getRotoPaintTreeNodes(&rotopaintNodes);
        for (NodesList::iterator it = rotopaintNodes.begin(); it != rotopaintNodes.end(); ++it) {
            (*it)->quitAnyProcessing_non_blocking();
        }
    }
}

void
Node::quitAnyProcessing_blocking(bool allowThreadsToRestart)
{
    {
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        if (_imp->nodeIsDequeuing) {
            _imp->nodeIsDequeuing = false;

            //Attempt to wake-up  sleeping threads of the thread pool
            _imp->nodeIsDequeuingCond.wakeAll();
        }
    }


    //If this effect has a RenderEngine, make sure it is finished
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( _imp->effect.get() );

    if (isOutput) {
        boost::shared_ptr<RenderEngine> engine = isOutput->getRenderEngine();
        assert(engine);
        engine->quitEngine(allowThreadsToRestart);
        engine->waitForEngineToQuit_enforce_blocking();
    }

    //Returns when the preview is done computign
    _imp->abortPreview_blocking(allowThreadsToRestart);

    boost::shared_ptr<TrackerContext> trackerContext = getTrackerContext();
    if (trackerContext) {
        trackerContext->quitTrackerThread_blocking(allowThreadsToRestart);
    }

    if ( isRotoPaintingNode() ) {
        NodesList rotopaintNodes;
        getRotoContext()->getRotoPaintTreeNodes(&rotopaintNodes);
        for (NodesList::iterator it = rotopaintNodes.begin(); it != rotopaintNodes.end(); ++it) {
            (*it)->quitAnyProcessing_blocking(allowThreadsToRestart);
        }
    }
}

void
Node::abortAnyProcessing_non_blocking()
{
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( getEffectInstance().get() );

    if (isOutput) {
        isOutput->getRenderEngine()->abortRenderingNoRestart();
    }

    boost::shared_ptr<TrackerContext> trackerContext = getTrackerContext();
    if (trackerContext) {
        trackerContext->abortTracking();
    }

    _imp->abortPreview_non_blocking();
}

void
Node::abortAnyProcessing_blocking()
{
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( getEffectInstance().get() );

    if (isOutput) {
        boost::shared_ptr<RenderEngine> engine = isOutput->getRenderEngine();
        assert(engine);
        engine->abortRenderingNoRestart();
        engine->waitForAbortToComplete_enforce_blocking();
    }

    boost::shared_ptr<TrackerContext> trackerContext = getTrackerContext();
    if (trackerContext) {
        trackerContext->abortTracking_blocking();
    }

    _imp->abortPreview_blocking(false);
}

Node::~Node()
{
    destroyNode(true, false);
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
Node::getInputNames(std::map<std::string, std::string> & inputNames) const
{
    ///This is called by the serialization thread.
    ///We use the guiInputs because we want to serialize exactly how the tree was to the user

    NodePtr parent = _imp->multiInstanceParent.lock();

    if (parent) {
        parent->getInputNames(inputNames);

        return;
    }

    QMutexLocker l(&_imp->inputsLabelsMutex);
    assert( _imp->inputs.size() == _imp->inputLabels.size() );
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            inputNames.insert( std::make_pair( _imp->inputLabels[i], input->getScriptName_mt_safe() ) );
        }
    }
}

int
Node::getPreferredInputInternal(bool connected) const
{
    int nInputs = getNInputs();

    if (nInputs == 0) {
        return -1;
    }
    std::vector<NodePtr> inputs(nInputs);
    std::vector<std::string> inputLabels(nInputs);
    int inputA = -1;
    int inputB = -1;
    {
        // fill input labels, and if one is called "Source", return it
        // if it's "A" or "B", keep the index.
        for (int i = 0; i < nInputs; ++i) {
            std::string inputLabel = getInputLabel(i);
            //printf("%d->%s\n", i, inputLabel.c_str());
            if (inputLabel == kOfxImageEffectSimpleSourceClipName) {
                inputs[i] = getInput(i);
                if ( (connected && inputs[i]) || (!connected && !inputs[i]) ) {
                    return i;
                }
            } else if (inputLabel == "A") {
                inputA = i;
            } else if (inputLabel == "B") {
                inputB = i;
            }
        }
    }
    bool useInputA = false;
    if (!connected) {
        // For the merge node, use the preference (only when not connected)
        useInputA = appPTR->getCurrentSettings()->useInputAForMergeAutoConnect();
    }

    ///Find an input named A
    int inputToFind = -1, foundOther = -1;
    if ( useInputA || (getPluginID() == PLUGINID_OFX_SHUFFLE && getMajorVersion() < 3) ) {
        inputToFind = inputA;
        foundOther = inputB;
    } else {
        inputToFind = inputB;
        foundOther = inputA;
    }
    if (inputToFind != -1) {
        inputs[inputToFind] = getInput(inputToFind);
        if ( (connected && inputs[inputToFind]) || (!connected && !inputs[inputToFind]) ) {
            return inputToFind;
        }
    }
    if (foundOther != -1) {
        inputs[foundOther] = getInput(foundOther);
        if ( (connected && inputs[foundOther]) || (!connected && !inputs[foundOther]) ) {
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
        if ( _imp->effect->isInputRotoBrush(i) ) {
            continue;
        }
        if ( (connected && inputs[i]) || (!connected && !inputs[i]) ) {
            if ( !_imp->effect->isInputOptional(i) ) {
                if (firstNonOptionalEmptyInput == -1) {
                    firstNonOptionalEmptyInput = i;
                    break;
                }
            } else {
                if ( _imp->effect->isInputMask(i) ) {
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
            //We return the first optional empty input
            std::list<int>::iterator first = optionalEmptyInputs.begin();
            while ( first != optionalEmptyInputs.end() && _imp->effect->isInputRotoBrush(*first) ) {
                ++first;
            }
            if ( first == optionalEmptyInputs.end() ) {
                return -1;
            } else {
                return *first;
            }
        } else if ( !optionalEmptyMasks.empty() ) {
            return optionalEmptyMasks.front();
        } else {
            return -1;
        }
    }
} // Node::getPreferredInputInternal

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

NodePtr
Node::getPreferredInputNode() const
{
    GroupInput* isInput = dynamic_cast<GroupInput*>( _imp->effect.get() );
    PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>( _imp->effect.get() );

    if (isInput) {
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>( getGroup().get() );
        assert(isGroup);
        if (!isGroup) {
            return NodePtr();
        }
        int inputNb = -1;
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs, false);
        for (std::size_t i = 0; i < groupInputs.size(); ++i) {
            if (groupInputs[i].get() == this) {
                inputNb = i;
                break;
            }
        }
        if (inputNb != -1) {
            NodePtr input = isGroup->getNode()->getInput(inputNb);

            return input;
        }
    } else if (isPrecomp) {
        return isPrecomp->getOutputNode();
    } else {
        int idx = getPreferredInput();
        if (idx != -1) {
            return getInput(idx);
        }
    }

    return NodePtr();
}

void
Node::getOutputsConnectedToThisNode(std::map<NodePtr, int>* outputs)
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

static void
prependGroupNameRecursive(const NodePtr& group,
                          std::string& name)
{
    name.insert(0, ".");
    name.insert( 0, group->getScriptName_mt_safe() );
    NodeCollectionPtr hasParentGroup = group->getGroup();
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
        NodeCollectionPtr hasParentGroup = getGroup();
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>( hasParentGroup.get() );
        if (isGrp) {
            NodePtr grpNode = isGrp->getNode();
            if (grpNode) {
                prependGroupNameRecursive(grpNode, ret);
            }
        }
    }

    return ret;
}

std::string
Node::getFullyQualifiedName() const
{
    return getFullyQualifiedNameInternal( getScriptName_mt_safe() );
}

void
Node::setLabel(const std::string& label)
{
    assert( QThread::currentThread() == qApp->thread() );

    {
        QMutexLocker k(&_imp->nameMutex);
        if (label == _imp->label) {
            return;
        }
        _imp->label = label;
    }
    NodeCollectionPtr collection = getGroup();
    if (collection) {
        collection->notifyNodeNameChanged( shared_from_this() );
    }
    Q_EMIT labelChanged( QString::fromUtf8( label.c_str() ) );
}

const std::string&
Node::getLabel() const
{
    assert( QThread::currentThread() == qApp->thread() );
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
    setNameInternal(name, false);
}

static void
insertDependenciesRecursive(Node* node,
                            KnobI::ListenerDimsMap* dependencies)
{
    const KnobsVec & knobs = node->getKnobs();

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobI::ListenerDimsMap dimDeps;
        knobs[i]->getListeners(dimDeps);
        for (KnobI::ListenerDimsMap::iterator it = dimDeps.begin(); it != dimDeps.end(); ++it) {
            KnobI::ListenerDimsMap::iterator found = dependencies->find(it->first);
            if ( found != dependencies->end() ) {
                assert( found->second.size() == it->second.size() );
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
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            insertDependenciesRecursive(it->get(), dependencies);
        }
    }
}

void
Node::setNameInternal(const std::string& name,
                      bool throwErrors)
{
    std::string oldName = getScriptName_mt_safe();
    std::string fullOldName = getFullyQualifiedName();
    std::string newName = name;
    bool onlySpaces = true;

    for (std::size_t i = 0; i < name.size(); ++i) {
        if (name[i] != '_') {
            onlySpaces = false;
            break;
        }
    }
    if (onlySpaces) {
        QString err = tr("The name must at least contain a character");
        if (throwErrors) {
            throw std::runtime_error(err.toStdString());
        } else {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }
            appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getFullyQualifiedName().c_str()), QDateTime::currentDateTime(), err, false, c);
            std::cerr << err.toStdString() << std::endl;

            return;
        }
    }

    NodeCollectionPtr collection = getGroup();
    if (collection) {
        if (throwErrors) {
            try {
                collection->checkNodeName(this, name, false, false, &newName);
            } catch (const std::exception& e) {
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getFullyQualifiedName().c_str()), QDateTime::currentDateTime(), QString::fromUtf8( e.what() ), false, c );
                std::cerr << e.what() << std::endl;

                return;
            }
        } else {
            collection->checkNodeName(this, name, false, false, &newName);
        }
    }


    if (oldName == newName) {
        return;
    }


    if ( !newName.empty() ) {
        bool isAttrDefined = false;
        std::string newPotentialQualifiedName = getApp()->getAppIDString() + "." + getFullyQualifiedNameInternal(newName);
        PyObject* obj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(newPotentialQualifiedName, appPTR->getMainModule(), &isAttrDefined);
        Q_UNUSED(obj);
        if (isAttrDefined) {
            std::stringstream ss;
            ss << "A Python attribute with the same name (" << newPotentialQualifiedName << ") already exists.";
            if (throwErrors) {
                throw std::runtime_error( ss.str() );
            } else {
                std::string err = ss.str();
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(oldName.c_str()), QDateTime::currentDateTime(), QString::fromUtf8( err.c_str() ), false, c );
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
        if ( _imp->label.empty() ) {
            _imp->label = newName;
        }
    }
    std::string fullySpecifiedName = getFullyQualifiedName();

    if (mustSetCacheID) {
        std::string baseName = fullySpecifiedName;
        std::string cacheID = fullySpecifiedName;
        int i = 1;
        while ( getGroup() && getGroup()->isCacheIDAlreadyTaken(cacheID) ) {
            std::stringstream ss;
            ss << baseName;
            ss << i;
            cacheID = ss.str();
            ++i;
        }
        QMutexLocker l(&_imp->nameMutex);
        _imp->cacheID = cacheID;
    }

    if (collection) {
        if ( !oldName.empty() ) {
            if (fullOldName != fullySpecifiedName) {
                try {
                    setNodeVariableToPython(fullOldName, fullySpecifiedName);
                } catch (const std::exception& e) {
                    qDebug() << e.what();
                }
            }
        } else { //if (!oldName.empty()) {
            declareNodeVariableToPython(fullySpecifiedName);
        }

        if (_imp->nodeCreated) {
            ///For all knobs that have listeners, change in the expressions of listeners this knob script-name
            KnobI::ListenerDimsMap dependencies;
            insertDependenciesRecursive(this, &dependencies);
            for (KnobI::ListenerDimsMap::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
                KnobIPtr listener = it->first.lock();
                if (!listener) {
                    continue;
                }
                for (std::size_t d = 0; d < it->second.size(); ++d) {
                    if (it->second[d].isListening && it->second[d].isExpr) {
                        listener->replaceNodeNameInExpression(d, oldName, newName);
                    }
                }
            }
        }
    }

    QString qnewName = QString::fromUtf8( newName.c_str() );
    Q_EMIT scriptNameChanged(qnewName);
    Q_EMIT labelChanged(qnewName);
} // Node::setNameInternal

void
Node::setScriptName(const std::string& name)
{
    std::string newName;

    if ( getGroup() ) {
        getGroup()->checkNodeName(this, name, false, true, &newName);
    } else {
        newName = name;
    }
    //We do not allow setting the script-name of output nodes because we rely on it with NatronRenderer
    if ( dynamic_cast<GroupOutput*>( _imp->effect.get() ) ) {
        throw std::runtime_error( tr("Changing the script-name of an Output node is not a valid operation.").toStdString() );

        return;
    }


    setNameInternal(newName, true);
}

AppInstancePtr
Node::getApp() const
{
    return _imp->app.lock();
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
    std::size_t ram, disk;

    appPTR->getMemoryStatsForCacheEntryHolder(this, &ram, &disk);
    QString ramSizeStr = printAsRAM( (U64)ram );
    QString diskSizeStr = printAsRAM( (U64)disk );
    std::stringstream ss;
    ss << "<b><font color=\"green\">Cache occupancy:</font></b> RAM: <font color=#c8c8c8>" << ramSizeStr.toStdString() << "</font> / Disk: <font color=#c8c8c8>" << diskSizeStr.toStdString() << "</font>";

    return ss.str();
}

std::string
Node::makeInfoForInput(int inputNumber) const
{
    if ( (inputNumber < -1) || ( inputNumber >= getNInputs() ) ) {
        return "";
    }
    EffectInstancePtr input;
    if (inputNumber != -1) {
        input = _imp->effect->getInput(inputNumber);
        /*if (input) {
            input = input->getNearestNonIdentity( getApp()->getTimeLine()->currentFrame() );
        }*/
    } else {
        input = _imp->effect;
    }

    if (!input) {
        return "";
    }


    double time = getApp()->getTimeLine()->currentFrame();
    std::stringstream ss;
    { // input name
        QString inputName;
        if (inputNumber != -1) {
            inputName = QString::fromUtf8( getInputLabel(inputNumber).c_str() );
        } else {
            inputName = tr("Output");
        }
        ss << "<b><font color=\"orange\">" << tr("%1:").arg(inputName).toStdString() << "</font></b><br />";
    }
    { // image format
        ss << "<b>" << tr("Layers:").toStdString() << "</b> <font color=#c8c8c8>";
        std::list<ImagePlaneDesc> availableLayers;
        input->getAvailableLayers(time, ViewIdx(0), -1, &availableLayers); // get the layers in the input's output (thus the -1)!
        std::list<ImagePlaneDesc>::iterator next = availableLayers.begin();
        if ( next != availableLayers.end() ) {
            ++next;
        }
        for (std::list<ImagePlaneDesc>::iterator it = availableLayers.begin(); it != availableLayers.end(); ++it) {

            ss << " "  << it->getPlaneLabel() << '.' << it->getChannelsLabel();
            if ( next != availableLayers.end() ) {
                ss << ", ";
                ++next;
            }
        }
        ss << "</font><br />";
    }
    {
        ImageBitDepthEnum depth = _imp->effect->getBitDepth(inputNumber);
        QString depthStr = tr("unknown");
        switch (depth) {
            case eImageBitDepthByte:
                depthStr = tr("8u");
                break;
            case eImageBitDepthShort:
                depthStr = tr("16u");
                break;
            case eImageBitDepthFloat:
                depthStr = tr("32fp");
                break;
            case eImageBitDepthHalf:
                depthStr = tr("16fp");
            case eImageBitDepthNone:
                break;
        }
        ss << "<b>" << tr("BitDepth:").toStdString() << "</b> <font color=#c8c8c8>" << depthStr.toStdString() << "</font><br />";
    }
    { // premult
        ImagePremultiplicationEnum premult = input->getPremult();
        QString premultStr = tr("unknown");
        switch (premult) {
        case eImagePremultiplicationOpaque:
            premultStr = tr("opaque");
            break;
        case eImagePremultiplicationPremultiplied:
            premultStr = tr("premultiplied");
            break;
        case eImagePremultiplicationUnPremultiplied:
            premultStr = tr("unpremultiplied");
            break;
        }
        ss << "<b>" << tr("Alpha premultiplication:").toStdString() << "</b> <font color=#c8c8c8>" << premultStr.toStdString() << "</font><br />";
    }
    {
        RectI format = input->getOutputFormat();
        if ( !format.isNull() ) {
            ss << "<b>" << tr("Format (pixels):").toStdString() << "</b> <font color=#c8c8c8>";
            ss << tr("left = %1 bottom = %2 right = %3 top = %4").arg(format.x1).arg(format.y1).arg(format.x2).arg(format.y2).toStdString() << "</font><br />";
        }
    }
    { // par
        double par = input->getAspectRatio(-1);
        ss << "<b>" << tr("Pixel aspect ratio:").toStdString() << "</b> <font color=#c8c8c8>" << par << "</font><br />";
    }
    { // fps
        double fps = input->getFrameRate();
        ss << "<b>" << tr("Frame rate:").toStdString() << "</b> <font color=#c8c8c8>" << tr("%1fps").arg(fps).toStdString() << "</font><br />";
    }
    {
        double first = 1., last = 1.;
        input->getFrameRange_public(getHashValue(), &first, &last);
        ss << "<b>" << tr("Frame range:").toStdString() << "</b> <font color=#c8c8c8>" << first << " - " << last << "</font><br />";
    }
    {
        RenderScale scale(1.);
        RectD rod;
        bool isProjectFormat;
        StatusEnum stat = input->getRegionOfDefinition_public(getHashValue(),
                                                              time,
                                                              scale, ViewIdx(0), &rod, &isProjectFormat);
        if (stat != eStatusFailed) {
            ss << "<b>" << tr("Region of Definition (at t=%1):").arg(time).toStdString() << "</b> <font color=#c8c8c8>";
            ss << tr("left = %1 bottom = %2 right = %3 top = %4").arg(rod.x1).arg(rod.y1).arg(rod.x2).arg(rod.y2).toStdString() << "</font><br />";
        }
    }

    return ss.str();
} // Node::makeInfoForInput

void
Node::findPluginFormatKnobs()
{
    findPluginFormatKnobs(getKnobs(), true);
}

void
Node::findRightClickMenuKnob(const KnobsVec& knobs)
{
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getName() == kNatronOfxParamRightClickMenu) {
            KnobIPtr rightClickKnob = knobs[i];
            KnobChoice* isChoice = dynamic_cast<KnobChoice*>( rightClickKnob.get() );
            if (isChoice) {
                QObject::connect( isChoice, SIGNAL(populated()), this, SIGNAL(rightClickMenuKnobPopulated()) );
            }
            break;
        }
    }
}

void
Node::findPluginFormatKnobs(const KnobsVec & knobs,
                            bool loadingSerialization)
{
    ///Try to find a format param and hijack it to handle it ourselves with the project's formats
    KnobIPtr formatKnob;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getName() == kNatronParamFormatChoice) {
            formatKnob = knobs[i];
            break;
        }
    }
    if (formatKnob) {
        KnobIPtr formatSize;
        for (std::size_t i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->getName() == kNatronParamFormatSize) {
                formatSize = knobs[i];
                break;
            }
        }
        KnobIPtr formatPar;
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

            std::vector<ChoiceOption> formats;
            int defValue;
            getApp()->getProject()->getProjectFormatEntries(&formats, &defValue);
            refreshFormatParamChoice(formats, defValue, loadingSerialization);
        }
    }
}

void
Node::createNodePage(const boost::shared_ptr<KnobPage>& settingsPage)
{
    boost::shared_ptr<KnobBool> hideInputs = AppManager::createKnob<KnobBool>(_imp->effect.get(), tr("Hide inputs"), 1, false);

    hideInputs->setName("hideInputs");
    hideInputs->setDefaultValue(false);
    hideInputs->setAnimationEnabled(false);
    hideInputs->setAddNewLine(false);
    hideInputs->setIsPersistent(true);
    hideInputs->setEvaluateOnChange(false);
    hideInputs->setHintToolTip( tr("When checked, the input arrows of the node in the nodegraph will be hidden") );
    _imp->hideInputs = hideInputs;
    settingsPage->addKnob(hideInputs);


    boost::shared_ptr<KnobBool> fCaching = AppManager::createKnob<KnobBool>(_imp->effect.get(), tr("Force caching"), 1, false);
    fCaching->setName("forceCaching");
    fCaching->setDefaultValue(false);
    fCaching->setAnimationEnabled(false);
    fCaching->setAddNewLine(false);
    fCaching->setIsPersistent(true);
    fCaching->setEvaluateOnChange(false);
    fCaching->setHintToolTip( tr("When checked, the output of this node will always be kept in the RAM cache for fast access of already computed "
                                 "images.") );
    _imp->forceCaching = fCaching;
    settingsPage->addKnob(fCaching);

    boost::shared_ptr<KnobBool> previewEnabled = AppManager::createKnob<KnobBool>(_imp->effect.get(), tr("Preview"), 1, false);
    assert(previewEnabled);
    previewEnabled->setDefaultValue( makePreviewByDefault() );
    previewEnabled->setName(kEnablePreviewKnobName);
    previewEnabled->setAnimationEnabled(false);
    previewEnabled->setAddNewLine(false);
    previewEnabled->setIsPersistent(false);
    previewEnabled->setEvaluateOnChange(false);
    previewEnabled->setHintToolTip( tr("Whether to show a preview on the node box in the node-graph.") );
    settingsPage->addKnob(previewEnabled);
    _imp->previewEnabledKnob = previewEnabled;

    boost::shared_ptr<KnobBool> disableNodeKnob = AppManager::createKnob<KnobBool>(_imp->effect.get(), tr("Disable"), 1, false);
    assert(disableNodeKnob);
    disableNodeKnob->setAnimationEnabled(false);
    disableNodeKnob->setIsMetadataSlave(true);
    disableNodeKnob->setName(kDisableNodeKnobName);
    disableNodeKnob->setAddNewLine(false);
    disableNodeKnob->setHintToolTip( tr("When disabled, this node acts as a pass through.") );
    settingsPage->addKnob(disableNodeKnob);
    _imp->disableNodeKnob = disableNodeKnob;



    boost::shared_ptr<KnobBool> useFullScaleImagesWhenRenderScaleUnsupported = AppManager::createKnob<KnobBool>(_imp->effect.get(), tr("Render high def. upstream"), 1, false);
    useFullScaleImagesWhenRenderScaleUnsupported->setAnimationEnabled(false);
    useFullScaleImagesWhenRenderScaleUnsupported->setDefaultValue(false);
    useFullScaleImagesWhenRenderScaleUnsupported->setName("highDefUpstream");
    useFullScaleImagesWhenRenderScaleUnsupported->setHintToolTip( tr("This node does not support rendering images at a scale lower than 1, it "
                                                                     "can only render high definition images. When checked this parameter controls "
                                                                     "whether the rest of the graph upstream should be rendered with a high quality too or at "
                                                                     "the most optimal resolution for the current viewer's viewport. Typically checking this "
                                                                     "means that an image will be slow to be rendered, but once rendered it will stick in the cache "
                                                                     "whichever zoom level you are using on the Viewer, whereas when unchecked it will be much "
                                                                     "faster to render but will have to be recomputed when zooming in/out in the Viewer.") );
    if ( ( isRenderScaleSupportEnabledForPlugin() ) && (getEffectInstance()->supportsRenderScaleMaybe() == EffectInstance::eSupportsYes) ) {
        useFullScaleImagesWhenRenderScaleUnsupported->setSecretByDefault(true);
    }
    settingsPage->addKnob(useFullScaleImagesWhenRenderScaleUnsupported);
    _imp->useFullScaleImagesWhenRenderScaleUnsupported = useFullScaleImagesWhenRenderScaleUnsupported;


    boost::shared_ptr<KnobInt> lifeTimeKnob = AppManager::createKnob<KnobInt>(_imp->effect.get(), tr("Lifetime Range"), 2, false);
    assert(lifeTimeKnob);
    lifeTimeKnob->setAnimationEnabled(false);
    lifeTimeKnob->setIsMetadataSlave(true);
    lifeTimeKnob->setName(kLifeTimeNodeKnobName);
    lifeTimeKnob->setAddNewLine(false);
    lifeTimeKnob->setHintToolTip( tr("This is the frame range during which the node will be active if Enable Lifetime is checked") );
    settingsPage->addKnob(lifeTimeKnob);
    _imp->lifeTimeKnob = lifeTimeKnob;


    boost::shared_ptr<KnobBool> enableLifetimeNodeKnob = AppManager::createKnob<KnobBool>(_imp->effect.get(), tr("Enable Lifetime"), 1, false);
    assert(enableLifetimeNodeKnob);
    enableLifetimeNodeKnob->setAnimationEnabled(false);
    enableLifetimeNodeKnob->setDefaultValue(false);
    enableLifetimeNodeKnob->setIsMetadataSlave(true);
    enableLifetimeNodeKnob->setName(kEnableLifeTimeNodeKnobName);
    enableLifetimeNodeKnob->setHintToolTip( tr("When checked, the node is only active during the specified frame range by the Lifetime Range parameter. "
                                               "Outside of this frame range, it behaves as if the Disable parameter is checked") );
    settingsPage->addKnob(enableLifetimeNodeKnob);
    _imp->enableLifeTimeKnob = enableLifetimeNodeKnob;

    PluginOpenGLRenderSupport glSupport = ePluginOpenGLRenderSupportNone;
    if (_imp->plugin && _imp->plugin->isOpenGLEnabled()) {
        glSupport = _imp->plugin->getPluginOpenGLRenderSupport();
    }
    if (glSupport != ePluginOpenGLRenderSupportNone) {
        boost::shared_ptr<KnobChoice> openglRenderingKnob = AppManager::createKnob<KnobChoice>(_imp->effect.get(), tr("GPU Rendering"), 1, false);
        assert(openglRenderingKnob);
        openglRenderingKnob->setAnimationEnabled(false);
        {
            std::vector<ChoiceOption> entries;
            entries.push_back(ChoiceOption("Enabled", "", tr("If a plug-in support GPU rendering, prefer rendering using the GPU if possible.").toStdString() ));
            entries.push_back(ChoiceOption("Disabled", "", tr("Disable GPU rendering for all plug-ins.").toStdString()));
            entries.push_back(ChoiceOption("Disabled if background", "", tr("Disable GPU rendering when rendering with NatronRenderer but not in GUI mode.").toStdString()));
            openglRenderingKnob->populateChoices(entries);

        }

        openglRenderingKnob->setName("enableGPURendering");
        openglRenderingKnob->setHintToolTip( tr("Select when to activate GPU rendering for this node. Note that if the GPU Rendering parameter in the Project settings is set to disabled then GPU rendering will not be activated regardless of that value.") );
        settingsPage->addKnob(openglRenderingKnob);
        _imp->openglRenderingEnabledKnob = openglRenderingKnob;
    }


    boost::shared_ptr<KnobString> knobChangedCallback = AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After param changed callback"), 1, false);
    knobChangedCallback->setHintToolTip( tr("Set here the name of a function defined in Python which will be called for each  "
                                            "parameter change. Either define this function in the Script Editor "
                                            "or in the init.py script or even in the script of a Python group plug-in.\n"
                                            "The signature of the callback is: callback(thisParam, thisNode, thisGroup, app, userEdited) where:\n"
                                            "- thisParam: The parameter which just had its value changed\n"
                                            "- userEdited: A boolean informing whether the change was due to user interaction or "
                                            "because something internally triggered the change.\n"
                                            "- thisNode: The node holding the parameter\n"
                                            "- app: points to the current application instance\n"
                                            "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)") );
    knobChangedCallback->setAnimationEnabled(false);
    knobChangedCallback->setName("onParamChanged");
    settingsPage->addKnob(knobChangedCallback);
    _imp->knobChangedCallback = knobChangedCallback;

    boost::shared_ptr<KnobString> inputChangedCallback = AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After input changed callback"), 1, false);
    inputChangedCallback->setHintToolTip( tr("Set here the name of a function defined in Python which will be called after "
                                             "each connection is changed for the inputs of the node. "
                                             "Either define this function in the Script Editor "
                                             "or in the init.py script or even in the script of a Python group plug-in.\n"
                                             "The signature of the callback is: callback(inputIndex, thisNode, thisGroup, app):\n"
                                             "- inputIndex: the index of the input which changed, you can query the node "
                                             "connected to the input by calling the getInput(...) function.\n"
                                             "- thisNode: The node holding the parameter\n"
                                             "- app: points to the current application instance\n"
                                             "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)") );

    inputChangedCallback->setAnimationEnabled(false);
    inputChangedCallback->setName("onInputChanged");
    settingsPage->addKnob(inputChangedCallback);
    _imp->inputChangedCallback = inputChangedCallback;

    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGroup) {
        boost::shared_ptr<KnobString> onNodeCreated = AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After Node Created"), 1, false);
        onNodeCreated->setName("afterNodeCreated");
        onNodeCreated->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                          "is created in the group. This will be called in addition to the After Node Created "
                                          " callback of the project for the group node and all nodes within it (not recursively).\n"
                                          "The boolean variable userEdited will be set to True if the node was created "
                                          "by the user or False otherwise (such as when loading a project, or pasting a node).\n"
                                          "The signature of the callback is: callback(thisNode, app, userEdited) where:\n"
                                          "- thisNode: the node which has just been created\n"
                                          "- userEdited: a boolean indicating whether the node was created by user interaction or from "
                                          "a script/project load/copy-paste\n"
                                          "- app: points to the current application instance.") );
        onNodeCreated->setAnimationEnabled(false);
        _imp->nodeCreatedCallback = onNodeCreated;
        settingsPage->addKnob(onNodeCreated);

        boost::shared_ptr<KnobString> onNodeDeleted = AppManager::createKnob<KnobString>(_imp->effect.get(), tr("Before Node Removal"), 1, false);
        onNodeDeleted->setName("beforeNodeRemoval");
        onNodeDeleted->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                          "is about to be deleted. This will be called in addition to the Before Node Removal "
                                          " callback of the project for the group node and all nodes within it (not recursively).\n"
                                          "This function will not be called when the project is closing.\n"
                                          "The signature of the callback is: callback(thisNode, app) where:\n"
                                          "- thisNode: the node about to be deleted\n"
                                          "- app: points to the current application instance.") );
        onNodeDeleted->setAnimationEnabled(false);
        _imp->nodeRemovalCallback = onNodeDeleted;
        settingsPage->addKnob(onNodeDeleted);
    }
    if (_imp->effect->isWriter()
#ifdef NATRON_ENABLE_IO_META_NODES
        && !getIOContainer()
#endif
        ) {
        boost::shared_ptr<KnobString> beforeFrameRender =  AppManager::createKnob<KnobString>(_imp->effect.get(), tr("Before frame render"), 1, false);

        beforeFrameRender->setName("beforeFrameRender");
        beforeFrameRender->setAnimationEnabled(false);
        beforeFrameRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called before rendering "
                                              "any frame.\n "
                                              "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                              "- frame: the frame to be rendered\n"
                                              "- thisNode: points to the writer node\n"
                                              "- app: points to the current application instance") );
        settingsPage->addKnob(beforeFrameRender);
        _imp->beforeFrameRender = beforeFrameRender;

        boost::shared_ptr<KnobString> beforeRender =  AppManager::createKnob<KnobString>(_imp->effect.get(), tr("Before render"), 1, false);
        beforeRender->setName("beforeRender");
        beforeRender->setAnimationEnabled(false);
        beforeRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called once when "
                                         "starting rendering.\n "
                                         "The signature of the callback is: callback(thisNode, app) where:\n"
                                         "- thisNode: points to the writer node\n"
                                         "- app: points to the current application instance") );
        settingsPage->addKnob(beforeRender);
        _imp->beforeRender = beforeRender;

        boost::shared_ptr<KnobString> afterFrameRender =  AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After frame render"), 1, false);
        afterFrameRender->setName("afterFrameRender");
        afterFrameRender->setAnimationEnabled(false);
        afterFrameRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called after rendering "
                                             "any frame.\n "
                                             "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                             "- frame: the frame that has been rendered\n"
                                             "- thisNode: points to the writer node\n"
                                             "- app: points to the current application instance") );
        settingsPage->addKnob(afterFrameRender);
        _imp->afterFrameRender = afterFrameRender;

        boost::shared_ptr<KnobString> afterRender =  AppManager::createKnob<KnobString>(_imp->effect.get(), tr("After render"), 1, false);
        afterRender->setName("afterRender");
        afterRender->setAnimationEnabled(false);
        afterRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called once when the rendering "
                                        "is finished.\n "
                                        "The signature of the callback is: callback(aborted, thisNode, app) where:\n"
                                        "- aborted: True if the render ended because it was aborted, False upon completion\n"
                                        "- thisNode: points to the writer node\n"
                                        "- app: points to the current application instance") );
        settingsPage->addKnob(afterRender);
        _imp->afterRender = afterRender;
    }
} // Node::createNodePage

void
Node::createInfoPage()
{
    boost::shared_ptr<KnobPage> infoPage = AppManager::createKnob<KnobPage>(_imp->effect.get(), tr("Info").toStdString(), 1, false);

    infoPage->setName(NATRON_PARAMETER_PAGE_NAME_INFO);
    _imp->infoPage = infoPage;

    boost::shared_ptr<KnobString> nodeInfos = AppManager::createKnob<KnobString>(_imp->effect.get(), std::string(), 1, false);
    nodeInfos->setName("nodeInfos");
    nodeInfos->setAnimationEnabled(false);
    nodeInfos->setIsPersistent(false);
    nodeInfos->setAsMultiLine();
    nodeInfos->setAsCustomHTMLText(true);
    nodeInfos->setEvaluateOnChange(false);
    nodeInfos->setHintToolTip( tr("Input and output informations, press Refresh to update them with current values") );
    infoPage->addKnob(nodeInfos);
    _imp->nodeInfos = nodeInfos;


    boost::shared_ptr<KnobButton> refreshInfoButton = AppManager::createKnob<KnobButton>(_imp->effect.get(), tr("Refresh Info"), 1, false);
    refreshInfoButton->setName("refreshButton");
    refreshInfoButton->setEvaluateOnChange(false);
    infoPage->addKnob(refreshInfoButton);
    _imp->refreshInfoButton = refreshInfoButton;
}

void
Node::createHostMixKnob(const boost::shared_ptr<KnobPage>& mainPage)
{
    boost::shared_ptr<KnobDouble> mixKnob = AppManager::createKnob<KnobDouble>(_imp->effect.get(), tr("Mix"), 1, false);

    mixKnob->setName("hostMix");
    mixKnob->setHintToolTip( tr("Mix between the source image at 0 and the full effect at 1.") );
    mixKnob->setMinimum(0.);
    mixKnob->setMaximum(1.);
    mixKnob->setDefaultValue(1.);
    if (mainPage) {
        mainPage->addKnob(mixKnob);
    }
    _imp->mixWithSource = mixKnob;
}

void
Node::createMaskSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                          const std::vector<std::string>& inputLabels,
                          const boost::shared_ptr<KnobPage>& mainPage,
                          bool addNewLineOnLastMask,
                          KnobIPtr* lastKnobCreated)
{
    assert( hasMaskChannelSelector.size() == inputLabels.size() );

    for (std::size_t i = 0; i < hasMaskChannelSelector.size(); ++i) {
        if (!hasMaskChannelSelector[i].first) {
            continue;
        }


        MaskSelector sel;
        boost::shared_ptr<KnobBool> enabled = AppManager::createKnob<KnobBool>(_imp->effect.get(), inputLabels[i], 1, false);

        enabled->setDefaultValue(false, 0);
        enabled->setAddNewLine(false);
        if (hasMaskChannelSelector[i].second) {
            std::string enableMaskName(std::string(kEnableMaskKnobName) + "_" + inputLabels[i]);
            enabled->setName(enableMaskName);
            enabled->setHintToolTip( tr("Enable the mask to come from the channel named by the choice parameter on the right. "
                                        "Turning this off will act as though the mask was disconnected.") );
        } else {
            std::string enableMaskName(std::string(kEnableInputKnobName) + "_" + inputLabels[i]);
            enabled->setName(enableMaskName);
            enabled->setHintToolTip( tr("Enable the image to come from the channel named by the choice parameter on the right. "
                                        "Turning this off will act as though the input was disconnected.") );
        }
        enabled->setAnimationEnabled(false);
        if (mainPage) {
            mainPage->addKnob(enabled);
        }


        sel.enabled = enabled;

        boost::shared_ptr<KnobChoice> channel = AppManager::createKnob<KnobChoice>(_imp->effect.get(), std::string(), 1, false);
        // By default if connected it should be filled with None, Color.R, Color.G, Color.B, Color.A (@see refreshChannelSelectors)
        channel->setDefaultValue(4);
        channel->setAnimationEnabled(false);
        channel->setHintToolTip( tr("Use this channel from the original input to mix the output with the original input. "
                                    "Setting this to None is the same as disconnecting the input.") );
        if (hasMaskChannelSelector[i].second) {
            std::string channelMaskName(std::string(kMaskChannelKnobName) + "_" + inputLabels[i]);
            channel->setName(channelMaskName);
        } else {
            std::string channelMaskName(std::string(kInputChannelKnobName) + "_" + inputLabels[i]);
            channel->setName(channelMaskName);
        }
        sel.channel = channel;
        if (mainPage) {
            mainPage->addKnob(channel);
        }

        //Make sure the first default param in the vector is MaskInvert

        if (!addNewLineOnLastMask) {
            //If there is a MaskInvert parameter, make it on the same line as the Mask channel parameter
            channel->setAddNewLine(false);
        }
        if (!*lastKnobCreated) {
            *lastKnobCreated = enabled;
        }

        _imp->maskSelectors[i] = sel;
    } // for (int i = 0; i < inputsCount; ++i) {
} // Node::createMaskSelectors

#ifndef NATRON_ENABLE_IO_META_NODES
void
Node::createWriterFrameStepKnob(const boost::shared_ptr<KnobPage>& mainPage)
{
    ///Find a  "lastFrame" parameter and add it after it
    boost::shared_ptr<KnobInt> frameIncrKnob = AppManager::createKnob<KnobInt>(_imp->effect.get(), tr(kNatronWriteParamFrameStepLabel), 1, false);

    frameIncrKnob->setName(kNatronWriteParamFrameStep);
    frameIncrKnob->setHintToolTip( tr(kNatronWriteParamFrameStepHint) );
    frameIncrKnob->setAnimationEnabled(false);
    frameIncrKnob->setMinimum(1);
    frameIncrKnob->setDefaultValue(1);
    if (mainPage) {
        std::vector<KnobIPtr> children = mainPage->getChildren();
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

#endif

boost::shared_ptr<KnobPage>
Node::getOrCreateMainPage()
{
    const KnobsVec & knobs = _imp->effect->getKnobs();
    boost::shared_ptr<KnobPage> mainPage;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        boost::shared_ptr<KnobPage> p = boost::dynamic_pointer_cast<KnobPage>(knobs[i]);
        if ( p && (p->getLabel() != NATRON_PARAMETER_PAGE_NAME_INFO) &&
             (p->getLabel() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
            mainPage = p;
            break;
        }
    }
    if (!mainPage) {
        mainPage = AppManager::createKnob<KnobPage>( _imp->effect.get(), tr("Settings") );
    }

    return mainPage;
}

void
Node::createLabelKnob(const boost::shared_ptr<KnobPage>& settingsPage,
                      const std::string& label)
{
    boost::shared_ptr<KnobString> nodeLabel = AppManager::createKnob<KnobString>(_imp->effect.get(), label, 1, false);

    assert(nodeLabel);
    nodeLabel->setName(kUserLabelKnobName);
    nodeLabel->setAnimationEnabled(false);
    nodeLabel->setEvaluateOnChange(false);
    nodeLabel->setAsMultiLine();
    nodeLabel->setUsesRichText(true);
    nodeLabel->setHintToolTip( tr("This label gets appended to the node name on the node graph.") );
    settingsPage->addKnob(nodeLabel);
    _imp->nodeLabelKnob = nodeLabel;
}

void
Node::findOrCreateChannelEnabled(const boost::shared_ptr<KnobPage>& mainPage)
{
    //Try to find R,G,B,A parameters on the plug-in, if found, use them, otherwise create them
    static const std::string channelLabels[4] = {kNatronOfxParamProcessRLabel, kNatronOfxParamProcessGLabel, kNatronOfxParamProcessBLabel, kNatronOfxParamProcessALabel};
    static const std::string channelNames[4] = {kNatronOfxParamProcessR, kNatronOfxParamProcessG, kNatronOfxParamProcessB, kNatronOfxParamProcessA};
    static const std::string channelHints[4] = {kNatronOfxParamProcessRHint, kNatronOfxParamProcessGHint, kNatronOfxParamProcessBHint, kNatronOfxParamProcessAHint};
    boost::shared_ptr<KnobBool> foundEnabled[4];
    const KnobsVec & knobs = _imp->effect->getKnobs();

    for (int i = 0; i < 4; ++i) {
        boost::shared_ptr<KnobBool> enabled;
        for (std::size_t j = 0; j < knobs.size(); ++j) {
            if (knobs[j]->getOriginalName() == channelNames[i]) {
                foundEnabled[i] = boost::dynamic_pointer_cast<KnobBool>(knobs[j]);
                break;
            }
        }
    }

    bool foundAll = foundEnabled[0] && foundEnabled[1] && foundEnabled[2] && foundEnabled[3];
    bool isWriter = _imp->effect->isWriter();

    if (foundAll) {
        for (int i = 0; i < 4; ++i) {
            // Writers already have their checkboxes places correctly
            if (!isWriter) {
                if (foundEnabled[i]->getParentKnob() == mainPage) {
                    //foundEnabled[i]->setAddNewLine(i == 3);
                    mainPage->removeKnob( foundEnabled[i].get() );
                    mainPage->insertKnob(i, foundEnabled[i]);
                }
            }
            _imp->enabledChan[i] = foundEnabled[i];
        }
    }

    bool pluginDefaultPref[4];
    _imp->hostChannelSelectorEnabled = _imp->effect->isHostChannelSelectorSupported(&pluginDefaultPref[0], &pluginDefaultPref[1], &pluginDefaultPref[2], &pluginDefaultPref[3]);


    if (_imp->hostChannelSelectorEnabled) {
        if (foundAll) {
            std::cerr << getScriptName_mt_safe() << ": WARNING: property " << kNatronOfxImageEffectPropChannelSelector << " is different of " << kOfxImageComponentNone << " but uses its own checkboxes" << std::endl;
        } else {
            //Create the selectors
            for (int i = 0; i < 4; ++i) {
                foundEnabled[i] =  AppManager::createKnob<KnobBool>(_imp->effect.get(), channelLabels[i], 1, false);
                foundEnabled[i]->setName(channelNames[i]);
                foundEnabled[i]->setAnimationEnabled(false);
                foundEnabled[i]->setAddNewLine(i == 3);
                foundEnabled[i]->setDefaultValue(pluginDefaultPref[i]);
                foundEnabled[i]->setHintToolTip(channelHints[i]);
                mainPage->insertKnob(i, foundEnabled[i]);
                _imp->enabledChan[i] = foundEnabled[i];
            }
            foundAll = true;
        }
    }
    if ( !isWriter && foundAll && !getApp()->isBackground() ) {
        _imp->enabledChan[3].lock()->setAddNewLine(false);
        boost::shared_ptr<KnobString> premultWarning = AppManager::createKnob<KnobString>(_imp->effect.get(), std::string(), 1, false);
        premultWarning->setIconLabel("dialog-warning");
        premultWarning->setSecretByDefault(true);
        premultWarning->setAsLabel();
        premultWarning->setEvaluateOnChange(false);
        premultWarning->setIsPersistent(false);
        premultWarning->setHintToolTip( tr("The alpha checkbox is checked and the RGB "
                                           "channels in output are alpha-premultiplied. Any of the unchecked RGB channel "
                                           "may be incorrect because the alpha channel changed but their value did not. "
                                           "To fix this, either check all RGB channels (or uncheck alpha) or unpremultiply the "
                                           "input image first.").toStdString() );
        mainPage->insertKnob(4, premultWarning);
        _imp->premultWarning = premultWarning;
    }
} // Node::findOrCreateChannelEnabled

void
Node::createChannelSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                             const std::vector<std::string>& inputLabels,
                             const boost::shared_ptr<KnobPage>& mainPage,
                             KnobIPtr* lastKnobBeforeAdvancedOption)
{
    ///Create input layer selectors
    for (std::size_t i = 0; i < inputLabels.size(); ++i) {
        if (!hasMaskChannelSelector[i].first) {
            _imp->createChannelSelector(i, inputLabels[i], false, mainPage, lastKnobBeforeAdvancedOption);
        }
    }
    ///Create output layer selectors
    _imp->createChannelSelector(-1, "Output", true, mainPage, lastKnobBeforeAdvancedOption);
}

void
Node::initializeDefaultKnobs(bool loadingSerialization)
{
    //Readers and Writers don't have default knobs since these knobs are on the ReadNode/WriteNode itself
#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr ioContainer = getIOContainer();
#endif

    //Add the "Node" page
    boost::shared_ptr<KnobPage> settingsPage = AppManager::createKnob<KnobPage>(_imp->effect.get(), tr(NATRON_PARAMETER_PAGE_NAME_EXTRA), 1, false);
    _imp->nodeSettingsPage = settingsPage;

    //Create the "Label" knob
    Backdrop* isBackdropNode = dynamic_cast<Backdrop*>( _imp->effect.get() );
    QString labelKnobLabel = isBackdropNode ? tr("Name label") : tr("Label");
    createLabelKnob( settingsPage, labelKnobLabel.toStdString() );

    if (isBackdropNode) {
        //backdrops just have a label
        return;
    }


    ///find in all knobs a page param to set this param into
    const KnobsVec & knobs = _imp->effect->getKnobs();

    findPluginFormatKnobs(knobs, loadingSerialization);
    findRightClickMenuKnob(knobs);

    boost::shared_ptr<KnobPage> mainPage;


    // Scan all inputs to find masks and get inputs labels
    //Pair hasMaskChannelSelector, isMask
    int inputsCount = getNInputs();
    std::vector<std::pair<bool, bool> > hasMaskChannelSelector(inputsCount);
    std::vector<std::string> inputLabels(inputsCount);
    for (int i = 0; i < inputsCount; ++i) {
        inputLabels[i] = _imp->effect->getInputLabel(i);

        assert( i < (int)_imp->inputsComponents.size() );
        bool isMask = _imp->effect->isInputMask(i);
        bool supportsOnlyAlpha = isInputOnlyAlpha(i);

        hasMaskChannelSelector[i].first = false;
        hasMaskChannelSelector[i].second = isMask;

        if ( (isMask || supportsOnlyAlpha) &&
             !_imp->effect->isInputRotoBrush(i) ) {
            hasMaskChannelSelector[i].first = true;
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }
        }
    }

    bool requiresLayerShuffle = _imp->effect->getCreateChannelSelectorKnob();

    // Create the Output Layer choice if needed plus input layers selectors
    KnobIPtr lastKnobBeforeAdvancedOption;
    if (requiresLayerShuffle) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        createChannelSelectors(hasMaskChannelSelector, inputLabels, mainPage, &lastKnobBeforeAdvancedOption);
    }

    if (!mainPage) {
        mainPage = getOrCreateMainPage();
    }
    findOrCreateChannelEnabled(mainPage);

    ///Find in the plug-in the Mask/Mix related parameter to re-order them so it is consistent across nodes
    std::vector<std::pair<std::string, KnobIPtr> > foundPluginDefaultKnobsToReorder;
    foundPluginDefaultKnobsToReorder.push_back( std::make_pair( kOfxMaskInvertParamName, KnobIPtr() ) );
    foundPluginDefaultKnobsToReorder.push_back( std::make_pair( kOfxMixParamName, KnobIPtr() ) );

    ///Insert auto-added knobs before mask invert if found
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        for (std::size_t j = 0; j < foundPluginDefaultKnobsToReorder.size(); ++j) {
            if (knobs[i]->getName() == foundPluginDefaultKnobsToReorder[j].first) {
                foundPluginDefaultKnobsToReorder[j].second = knobs[i];
            }
        }
    }


    assert(foundPluginDefaultKnobsToReorder.size() > 0 && foundPluginDefaultKnobsToReorder[0].first == kOfxMaskInvertParamName);

    createMaskSelectors(hasMaskChannelSelector, inputLabels, mainPage, !foundPluginDefaultKnobsToReorder[0].second.get(), &lastKnobBeforeAdvancedOption);


    //Create the host mix if needed
    if ( _imp->effect->isHostMixingEnabled() ) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        createHostMixKnob(mainPage);
    }


    /*
     * Reposition the MaskInvert and Mix parameters declared by the plug-in
     */
    for (std::size_t i = 0; i < foundPluginDefaultKnobsToReorder.size(); ++i) {
        if (foundPluginDefaultKnobsToReorder[i].second) {
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }
            if (foundPluginDefaultKnobsToReorder[i].second->getParentKnob() == mainPage) {
                mainPage->removeKnob( foundPluginDefaultKnobsToReorder[i].second.get() );
                mainPage->addKnob(foundPluginDefaultKnobsToReorder[i].second);
            }
        }
    }


    if (lastKnobBeforeAdvancedOption && mainPage) {
        
        KnobsVec mainPageChildren = mainPage->getChildren();
        int i = 0;
        for (KnobsVec::iterator it = mainPageChildren.begin(); it != mainPageChildren.end(); ++it, ++i) {
            if (*it == lastKnobBeforeAdvancedOption) {
                if (i > 0) {
                    int j = i - 1;
                    KnobsVec::iterator prev = it;
                    --prev;
                    while (j >= 0 && (*prev)->getIsSecret()) {
                        --j;
                        --prev;
                    }

                    if ( j >= 0 && !dynamic_cast<KnobSeparator*>( prev->get() ) ) {
                        boost::shared_ptr<KnobSeparator> sep = AppManager::createKnob<KnobSeparator>(_imp->effect.get(), std::string(), 1, false);
                        sep->setName("advancedSep");
                        mainPage->insertKnob(i, sep);
                    }
                }

                break;
            }
        }
    }


    createNodePage(settingsPage);

    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->effect.get());
    if (!isGroup && !isBackdropNode) {
        createInfoPage();
    }

    if (_imp->effect->isWriter()
#ifdef NATRON_ENABLE_IO_META_NODES
        && !ioContainer
#endif
        ) {
        //Create a frame step parameter for writers, and control it in OutputSchedulerThread.cpp
#ifndef NATRON_ENABLE_IO_META_NODES
        createWriterFrameStepKnob(mainPage);
#endif
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }

        boost::shared_ptr<KnobButton> renderButton = AppManager::createKnob<KnobButton>(_imp->effect.get(), tr("Render"), 1, false);
        renderButton->setHintToolTip( tr("Starts rendering the specified frame range.") );
        renderButton->setAsRenderButton();
        renderButton->setName(kNatronWriteParamStartRender);
        renderButton->setEvaluateOnChange(false);
        _imp->renderButton = renderButton;
        mainPage->addKnob(renderButton);
        createInfoPage();
    }
} // Node::initializeDefaultKnobs

void
Node::initializeKnobs(bool loadingSerialization)
{
    ////Only called by the main-thread


    _imp->effect->beginChanges();

    assert( QThread::currentThread() == qApp->thread() );
    assert(!_imp->knobsInitialized);

    ///For groups, declare the plugin knobs after the node knobs because we want to use the Node page
    bool effectIsGroup = getPluginID() == PLUGINID_NATRON_GROUP;

    if (!effectIsGroup) {
        //Initialize plug-in knobs
        _imp->effect->initializeKnobsPublic();
    }

    InitializeKnobsFlag_RAII __isInitializingKnobsFlag__( _imp->effect.get() );

    if ( _imp->effect->getMakeSettingsPanel() ) {
        //initialize default knobs added by Natron
        initializeDefaultKnobs(loadingSerialization);
    }

    if (effectIsGroup) {
        _imp->effect->initializeKnobsPublic();
    }
    _imp->effect->endChanges();

    _imp->knobsInitialized = true;

    Q_EMIT knobsInitialized();
} // initializeKnobs

void
Node::Implementation::createChannelSelector(int inputNb,
                                            const std::string & inputName,
                                            bool isOutput,
                                            const boost::shared_ptr<KnobPage>& page,
                                            KnobIPtr* lastKnobBeforeAdvancedOption)
{
    ChannelSelector sel;

    boost::shared_ptr<KnobChoice> layer = AppManager::createKnob<KnobChoice>(effect.get(), isOutput ? tr("Output Layer") : tr("%1 Layer").arg( QString::fromUtf8( inputName.c_str() ) ), 1, false);
    layer->setHostCanAddOptions(isOutput);
    if (!isOutput) {
        layer->setName( inputName + std::string("_") + std::string(kOutputChannelsKnobName) );
    } else {
        layer->setName(kOutputChannelsKnobName);
    }
    if (isOutput) {
        layer->setHintToolTip( tr("Select here the layer onto which the processing should occur.") );
    } else {
        layer->setHintToolTip( tr("Select here the layer that will be used in input by %1.").arg( QString::fromUtf8( inputName.c_str() ) ) );
    }
    layer->setAnimationEnabled(false);
    layer->setSecretByDefault(!isOutput);
    page->addKnob(layer);
    sel.layer = layer;

    if (isOutput) {
        layer->setAddNewLine(false);
        boost::shared_ptr<KnobBool> processAllKnob = AppManager::createKnob<KnobBool>(effect.get(), tr(kNodeParamProcessAllLayersLabel), 1, false);
        processAllKnob->setName(kNodeParamProcessAllLayers);
        processAllKnob->setHintToolTip(tr(kNodeParamProcessAllLayersHint));
        processAllKnob->setAnimationEnabled(false);
        processAllKnob->setIsMetadataSlave(true);
        page->addKnob(processAllKnob);

        // If the effect wants by default to render all planes set default value
        if ( isOutput && (effect->isPassThroughForNonRenderedPlanes() == EffectInstance::ePassThroughRenderAllRequestedPlanes) ) {
            processAllKnob->setDefaultValue(true);
            //Hide all other input selectors if choice is All in output
            for (std::map<int, ChannelSelector>::iterator it = channelsSelectors.begin(); it != channelsSelectors.end(); ++it) {
                it->second.layer.lock()->setSecret(true);
            }
        }
        processAllLayersKnob = processAllKnob;
    }


    layer->setDefaultValue(isOutput ? 0 : 1);

    if (!*lastKnobBeforeAdvancedOption) {
        *lastKnobBeforeAdvancedOption = layer;
    }

    channelsSelectors[inputNb] = sel;
} // Node::Implementation::createChannelSelector

int
Node::getFrameStepKnobValue() const
{
    KnobIPtr knob = getKnobByName(kNatronWriteParamFrameStep);
    if (!knob) {
        return 1;
    }
    KnobInt* k = dynamic_cast<KnobInt*>(knob.get());

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

    if ( knob != choice.get() ) {
        return false;
    }
    if (choice->getIsSecret()) {
        return true;
    }
    int curIndex = choice->getValue();
    Format f;
    if ( !getApp()->getProject()->getProjectFormatAtIndex(curIndex, &f) ) {
        assert(false);

        return true;
    }

    boost::shared_ptr<KnobInt> size = _imp->pluginFormatKnobs.size.lock();
    boost::shared_ptr<KnobDouble> par = _imp->pluginFormatKnobs.par.lock();
    assert(size && par);

    _imp->effect->beginChanges();
    size->blockValueChanges();
    size->setValues(f.width(), f.height(), ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
    size->unblockValueChanges();
    par->blockValueChanges();
    par->setValue( f.getPixelAspectRatio() );
    par->unblockValueChanges();

    _imp->effect->endChanges();

    return true;
}

void
Node::refreshFormatParamChoice(const std::vector<ChoiceOption>& entries,
                               int defValue,
                               bool loadingProject)
{
    boost::shared_ptr<KnobChoice> choice = _imp->pluginFormatKnobs.formatChoice.lock();

    if (!choice) {
        return;
    }
    int curIndex = choice->getValue();
    choice->populateChoices(entries);
    choice->beginChanges();
    choice->setDefaultValueWithoutApplying(defValue);
    if (!loadingProject) {
        //changedKnob was not called because we are initializing knobs
        handleFormatKnob( choice.get() );
    } else {
        if ( curIndex < (int)entries.size() ) {
            choice->setValue(curIndex);
        }
    }

    choice->endChanges();
}

void
Node::refreshPreviewsAfterProjectLoad()
{
    computePreviewImage( getApp()->getTimeLine()->currentFrame() );
    Q_EMIT s_refreshPreviewsAfterProjectLoadRequested();
}

// those parameters should be ignored (they are always secret in Natron)
#define kOCIOParamInputSpace "ocioInputSpace"
#define kOCIOParamOutputSpace "ocioOutputSpace"

// those parameters should not have their options in the help file if generating markdown,
// because the options are dinamlically constructed at run-time from the OCIO config.
#define kOCIOParamInputSpaceChoice "ocioInputSpaceIndex"
#define kOCIOParamOutputSpaceChoice "ocioOutputSpaceIndex"

#define kOCIODisplayPluginIdentifier "fr.inria.openfx.OCIODisplay"
#define kOCIODisplayParamDisplay "display"
#define kOCIODisplayParamDisplayChoice "displayIndex"
#define kOCIODisplayParamView "view"
#define kOCIODisplayParamViewChoice "viewIndex"

// not yet implemented (see OCIOCDLTransform.cpp)
//#define kOCIOCDLTransformPluginIdentifier "fr.inria.openfx.OCIOCDLTransform"
//#define kOCIOCDLTransformParamCCCID "cccId"
//#define kOCIOCDLTransformParamCCCIDChoice "cccIdIndex"

#define kOCIOFileTransformPluginIdentifier "fr.inria.openfx.OCIOFileTransform"
#define kOCIOFileTransformParamCCCID "cccId"
#define kOCIOFileTransformParamCCCIDChoice "cccIdIndex"

// genHTML: true for live HTML output for the internal web-server, false for markdown output
QString
Node::makeDocumentation(bool genHTML) const
{
    QString ret;
    QString markdown;
    QTextStream ts(&ret);
    QTextStream ms(&markdown);

    QString pluginID, pluginLabel, pluginDescription, pluginIcon;
    int majorVersion = getMajorVersion();
    int minorVersion = getMinorVersion();
    QStringList pluginGroup;
    bool pluginDescriptionIsMarkdown = false;
    QVector<QStringList> inputs;
    QVector<QStringList> items;

    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if (_imp->pyPlugInfo.isPyPlug) {
            pluginID = QString::fromUtf8( _imp->pyPlugInfo.pyPlugID.c_str() );
            pluginLabel =  QString::fromUtf8( _imp->pyPlugInfo.pyPlugLabel.c_str() );
            pluginDescription = QString::fromUtf8( _imp->pyPlugInfo.pyPlugDesc.c_str() );
            pluginIcon = QString::fromUtf8( _imp->pyPlugInfo.pyPlugIconFilePath.c_str() );
            for (std::list<std::string>::const_iterator it = _imp->pyPlugInfo.pyPlugGrouping.begin() ; it != _imp->pyPlugInfo.pyPlugGrouping.end(); ++it) {
                pluginGroup.push_back(QString::fromUtf8(it->c_str()));
            }
        } else {
            pluginID = _imp->plugin->getPluginID();
            pluginLabel =  Plugin::makeLabelWithoutSuffix( _imp->plugin->getPluginLabel() );
            pluginDescription =  QString::fromUtf8( _imp->effect->getPluginDescription().c_str() );
            pluginIcon = _imp->plugin->getIconFilePath();
            pluginGroup = _imp->plugin->getGrouping();
            pluginDescriptionIsMarkdown = _imp->effect->isPluginDescriptionInMarkdown();
        }

        for (int i = 0; i < _imp->effect->getNInputs(); ++i) {
            QStringList input;
            input << convertFromPlainTextToMarkdown( QString::fromStdString( _imp->effect->getInputLabel(i) ), genHTML, true );
            input << convertFromPlainTextToMarkdown( QString::fromStdString( _imp->effect->getInputHint(i) ), genHTML, true );
            input << ( _imp->effect->isInputOptional(i) ? tr("Yes") : tr("No") );
            inputs.push_back(input);

            // Don't show more than doc for 4 inputs otherwise it will just clutter the page
            if (i == 3) {
                break;
            }
        }
    }

    // check for plugin icon
    QString pluginIconUrl;
    if ( !pluginIcon.isEmpty() ) {
        QFile iconFile(pluginIcon);
        if ( iconFile.exists() ) {
            if (genHTML) {
                pluginIconUrl.append( QString::fromUtf8("/LOCAL_FILE/") );
                pluginIconUrl.append(pluginIcon);
                pluginIconUrl.replace( QString::fromUtf8("\\"), QString::fromUtf8("/") );
            } else {
                pluginIconUrl.append(pluginID);
                pluginIconUrl.append(QString::fromUtf8(".png"));
            }
        }
    }

    // check for extra markdown file
    QString extraMarkdown;
    QString pluginMD = pluginIcon;
    pluginMD.replace( QString::fromUtf8(".png"), QString::fromUtf8(".md") );
    QFile pluginMarkdownFile(pluginMD);
    if ( pluginMarkdownFile.exists() ) {
        if ( pluginMarkdownFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
            extraMarkdown = QString::fromUtf8( pluginMarkdownFile.readAll() );
            pluginMarkdownFile.close();
        }
    }

    // generate knobs info
    KnobsVec knobs = getEffectInstance()->getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {

        if ( (*it)->getDefaultIsSecret() ) {
            continue;
        }

        if ( !(*it)->isDeclaredByPlugin() && !( isPyPlug() && (*it)->isUserKnob() ) ) {
            continue;
        }

        // do not escape characters in the scriptName, since it will be put between backquotes
        QString knobScriptName = /*NATRON_NAMESPACE::convertFromPlainTextToMarkdown(*/ QString::fromUtf8( (*it)->getName().c_str() )/*, genHTML, true)*/;
        QString knobLabel = NATRON_NAMESPACE::convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getLabel().c_str() ), genHTML, true);
        QString knobHint = NATRON_NAMESPACE::convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getHintToolTip().c_str() ), genHTML, true);

        // totally ignore the documentation for these parameters (which are always secret in Natron)
        if ( knobScriptName.startsWith( QString::fromUtf8("NatronOfxParam") ) ||
             knobScriptName == QString::fromUtf8("exportAsPyPlug") ||
             knobScriptName == QString::fromUtf8(kOCIOParamInputSpace) ||
             knobScriptName == QString::fromUtf8(kOCIOParamOutputSpace) ||
             ( ( pluginID == QString::fromUtf8(kOCIODisplayPluginIdentifier) ) &&
               ( knobScriptName == QString::fromUtf8(kOCIODisplayParamDisplay) ) ) ||
             ( ( pluginID == QString::fromUtf8(kOCIODisplayPluginIdentifier) ) &&
               ( knobScriptName == QString::fromUtf8(kOCIODisplayParamView) ) ) ||
             //( ( pluginID == QString::fromUtf8(kOCIOCDLTransformPluginIdentifier) ) &&
             //  ( knobScriptName == QString::fromUtf8(kOCIOCDLTransformParamCCCID) ) ) ||
             ( ( pluginID == QString::fromUtf8(kOCIOFileTransformPluginIdentifier) ) &&
               ( knobScriptName == QString::fromUtf8(kOCIOFileTransformParamCCCID) ) ) ||
             false ) {
            continue;
        }

        QString defValuesStr, knobType;
        std::vector<std::pair<QString, QString> > dimsDefaultValueStr;
        KnobInt* isInt = dynamic_cast<KnobInt*>( it->get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( it->get() );
        KnobBool* isBool = dynamic_cast<KnobBool*>( it->get() );
        KnobDouble* isDbl = dynamic_cast<KnobDouble*>( it->get() );
        KnobString* isString = dynamic_cast<KnobString*>( it->get() );
        bool isLabel = isString && isString->isLabel();
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( it->get() );
        KnobButton* isBtn = dynamic_cast<KnobButton*>( it->get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( it->get() );
        KnobGroup* isGroup = dynamic_cast<KnobGroup*>( it->get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
        KnobColor* isColor = dynamic_cast<KnobColor*>( it->get() );

        if (isInt) {
            knobType = tr("Integer");
        } else if (isChoice) {
            knobType = tr("Choice");
        } else if (isBool) {
            knobType = tr("Boolean");
        } else if (isDbl) {
            knobType = tr("Double");
        } else if (isString) {
            if (isLabel) {
                knobType = tr("Label");
            } else {
                knobType = tr("String");
            }
        } else if (isSep) {
            knobType = tr("Separator");
        } else if (isBtn) {
            knobType = tr("Button");
        } else if (isParametric) {
            knobType = tr("Parametric");
        } else if (isGroup) {
            knobType = tr("Group");
        } else if (isPage) {
            knobType = tr("Page");
        } else if (isColor) {
            knobType = tr("Color");
        } else {
            knobType = tr("N/A");
        }

        if (!isGroup && !isPage) {
            for (int i = 0; i < (*it)->getDimension(); ++i) {
                QString valueStr;

                if (!isBtn && !isSep && !isParametric) {
                    // If this is a ChoiceParam and we are not generating live HTML doc,
                    // only add the list of entries and their halp if this node should not be
                    // ignored (eg. OCIO colorspace knobs).
                    if ( isChoice &&
                         (genHTML || !( knobScriptName == QString::fromUtf8(kOCIOParamInputSpaceChoice) ||
                                        knobScriptName == QString::fromUtf8(kOCIOParamOutputSpaceChoice) ||
                                        ( ( pluginID == QString::fromUtf8(kOCIODisplayPluginIdentifier) ) &&
                                          ( knobScriptName == QString::fromUtf8(kOCIODisplayParamDisplayChoice) ) ) ||
                                        ( ( pluginID == QString::fromUtf8(kOCIODisplayPluginIdentifier) ) &&
                                          ( knobScriptName == QString::fromUtf8(kOCIODisplayParamViewChoice) ) ) ||
                                        //( ( pluginID == QString::fromUtf8(kOCIOCDLTransformPluginIdentifier) ) &&
                                        //   ( knobScriptName == QString::fromUtf8(kOCIOCDLTransformParamCCCIDChoice) ) ) ||
                                        ( ( pluginID == QString::fromUtf8(kOCIOFileTransformPluginIdentifier) ) &&
                                          ( knobScriptName == QString::fromUtf8(kOCIOFileTransformParamCCCIDChoice) ) ) ||
                                        ( ( pluginID == QString::fromUtf8("net.fxarena.openfx.Text") ) &&
                                          ( knobScriptName == QString::fromUtf8("name") ) ) || // font family from Text plugin
                                        ( ( pluginID == QString::fromUtf8("net.fxarena.openfx.Polaroid") ) &&
                                          ( knobScriptName == QString::fromUtf8("font") ) ) || // font family from Polaroid plugin
                                        ( ( pluginID == QString::fromUtf8(PLUGINID_NATRON_PRECOMP) ) &&
                                          ( knobScriptName == QString::fromUtf8("writeNode") ) ) ||
                                        ( ( pluginID == QString::fromUtf8(PLUGINID_NATRON_ONEVIEW) ) &&
                                          ( knobScriptName == QString::fromUtf8("view") ) ) ) ) ) {
                        // see also KnobChoice::getHintToolTipFull()
                        int index = isChoice->getDefaultValue(i);
                        std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();
                        if ( (index >= 0) && ( index < (int)entries.size() ) ) {
                            valueStr = QString::fromUtf8( entries[index].label.c_str() );
                        }
                        bool first = true;
                        for (size_t i = 0; i < entries.size(); i++) {
                            QString entryHelp = QString::fromUtf8( entries[i].tooltip.c_str() );
                            QString entry;
                            if (entries[i].id != entries[i].label) {
                                entry = QString::fromUtf8( "%1 (%2)" ).arg(QString::fromUtf8( entries[i].label.c_str() )).arg(QString::fromUtf8( entries[i].id.c_str() ));
                            } else {
                                entry = QString::fromUtf8( entries[i].label.c_str() );
                            }
                            if (!entry.isEmpty()) {
                                if (first) {
                                    // empty line before the option descriptions
                                    if (genHTML) {
                                        if ( !knobHint.isEmpty() ) {
                                            knobHint.append( QString::fromUtf8("<br />") );
                                        }
                                        // "Possible values:" clutters the documentation.
                                        //knobHint.append( tr("Possible values:") + QString::fromUtf8("<br />") );
                                    } else {
                                        // we do a hack for multiline elements, because the markdown->rst conversion by pandoc doesn't use the line block syntax.
                                        // what we do here is put a supplementary dot at the beginning of each line, which is then converted to a pipe '|' in the
                                        // genStaticDocs.sh script by a simple sed command after converting to RsT
                                        if ( !knobHint.isEmpty() ) {
                                            if (!knobHint.startsWith( QString::fromUtf8(". ") )) {
                                                knobHint.prepend( QString::fromUtf8(". ") );
                                            }
                                            knobHint.append( QString::fromUtf8("\\\n") );
                                        }
                                        // "Possible values:" clutters the documentation.
                                        //knobHint.append( QString::fromUtf8(". ") + tr("Possible values:") +  QString::fromUtf8("\\\n") );
                                    }
                                    first = false;
                                }
                                if (genHTML) {
                                    knobHint.append( QString::fromUtf8("<br />") );
                                } else {
                                    knobHint.append( QString::fromUtf8("\\\n") );
                                    // we do a hack for multiline elements, because the markdown->rst conversion by pandoc doesn't use the line block syntax.
                                    // what we do here is put a supplementary dot at the beginning of each line, which is then converted to a pipe '|' in the
                                    // genStaticDocs.sh script by a simple sed command after converting to RsT
                                    knobHint.append( QString::fromUtf8(". ") );
                                }
                                if (entryHelp.isEmpty()) {
                                    knobHint.append( QString::fromUtf8("**%1**").arg( convertFromPlainTextToMarkdown(entry, genHTML, true) ) );
                                } else {
                                    knobHint.append( QString::fromUtf8("**%1**: %2").arg( convertFromPlainTextToMarkdown(entry, genHTML, true) ).arg( convertFromPlainTextToMarkdown(entryHelp, genHTML, true) ) );
                                }
                            }
                        }

                    } else if (isInt) {
                        valueStr = QString::number( isInt->getDefaultValue(i) );
                    } else if (isDbl) {
                        valueStr = QString::number( isDbl->getDefaultValue(i) );
                    } else if (isBool) {
                        valueStr = isBool->getDefaultValue(i) ? tr("On") : tr("Off");
                    } else if (isString) {
                        valueStr = QString::fromUtf8( isString->getDefaultValue(i).c_str() );
                    } else if (isColor) {
                        valueStr = QString::number( isColor->getDefaultValue(i) );
                    }
                }

                dimsDefaultValueStr.push_back( std::make_pair(convertFromPlainTextToMarkdown( QString::fromUtf8( (*it)->getDimensionName(i).c_str() ), genHTML, true ),
                                                              convertFromPlainTextToMarkdown(valueStr, genHTML, true)) );
            }

            for (std::size_t i = 0; i < dimsDefaultValueStr.size(); ++i) {
                if ( !dimsDefaultValueStr[i].second.isEmpty() ) {
                    if (dimsDefaultValueStr.size() > 1) {
                        defValuesStr.append(dimsDefaultValueStr[i].first);
                        defValuesStr.append( QString::fromUtf8(": ") );
                    }
                    defValuesStr.append(dimsDefaultValueStr[i].second);
                    if (i < dimsDefaultValueStr.size() - 1) {
                        defValuesStr.append( QString::fromUtf8(" ") );
                    }
                }
            }
            if ( defValuesStr.isEmpty() ) {
                defValuesStr = tr("N/A");
            }
        }

        if (!isPage && !isSep && !isGroup && !isLabel) {
            QStringList row;
            row << knobLabel << knobScriptName << knobType << defValuesStr << knobHint;
            items.append(row);
        }
    } // for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {


    // generate plugin info
    ms << tr("%1 node").arg(pluginLabel) << "\n==========\n\n";

    // a hack to avoid repeating the documentation for the various merge plugins
    if ( pluginID.startsWith( QString::fromUtf8("net.sf.openfx.Merge") ) ) {
        std::string id = pluginID.toStdString();
        std::string op;
        if (id == PLUGINID_OFX_MERGE) {
            // do nothing
        } else if (id == "net.sf.openfx.MergeDifference") {
            op = "difference (a.k.a. absminus)";
        } else if (id == "net.sf.openfx.MergeIn") {
            op = "in";
        } else if (id == "net.sf.openfx.MergeMatte") {
            op = "matte";
        } else if (id == "net.sf.openfx.MergeMax") {
            op = "max";
        } else if (id == "net.sf.openfx.MergeMin") {
            op = "min";
        } else if (id == "net.sf.openfx.MergeMultiply") {
            op = "multiply";
        } else if (id == "net.sf.openfx.MergeOut") {
            op = "out";
        } else if (id == "net.sf.openfx.MergePlus") {
            op = "plus";
        } else if (id == "net.sf.openfx.MergeScreen") {
            op = "screen";
        }
        if ( !op.empty() ) {
            // we should use the custom link "[Merge node](|http::/plugins/" PLUGINID_OFX_MERGE ".html||rst::net.sf.openfx.MergePlugin|)"
            // but pandoc borks it
            ms << tr("The *%1* node is a convenience node identical to the %2, except that the operator is set to *%3* by default.")
            .arg(pluginLabel)
            .arg(genHTML ? QString::fromUtf8("<a href=\"" PLUGINID_OFX_MERGE ".html\">Merge node</a>") :
                 QString::fromUtf8(":ref:`" PLUGINID_OFX_MERGE "`")
                 //QString::fromUtf8("[Merge node](http::/plugins/" PLUGINID_OFX_MERGE ".html)")
                 )
            .arg( QString::fromUtf8( op.c_str() ) );
            goto OUTPUT;
        }

    }

    if (!pluginIconUrl.isEmpty()) {
        // add a nonbreaking space so that pandoc doesn't use the alt-text as a caption
        // http://pandoc.org/MANUAL.html#images
        ms << "![pluginIcon](" << pluginIconUrl << ")";
        if (!genHTML) {
            // specify image width for pandoc-generated printed doc
            // (for hoedown-generated HTML, this handled by the CSS using the alt=pluginIcon attribute)
            // see http://pandoc.org/MANUAL.html#images
            // note that only % units are understood both by pandox and sphinx
            ms << "{ width=10% }";
        }
        ms << "&nbsp;\n\n"; // &nbsp; required so that there is no legend when converted to rst by pandoc
    }
    ms << tr("*This documentation is for version %2.%3 of %1.*").arg(pluginLabel).arg(majorVersion).arg(minorVersion) << "\n\n";

    ms << "\n" << tr("Description") << "\n--------------------------------------------------------------------------------\n\n";

    if (!pluginDescriptionIsMarkdown) {
        if (genHTML) {
            pluginDescription = NATRON_NAMESPACE::convertFromPlainText(pluginDescription, NATRON_NAMESPACE::WhiteSpaceNormal);

            // replace URLs with links
            QRegExp re( QString::fromUtf8("((http|ftp|https)://([\\w_-]+(?:(?:\\.[\\w_-]+)+))([\\w.,@?^=%&:/~+#-]*[\\w@?^=%&/~+#-])?)") );
            pluginDescription.replace( re, QString::fromUtf8("<a href=\"\\1\">\\1</a>") );
        } else {
            pluginDescription = convertFromPlainTextToMarkdown(pluginDescription, genHTML, false);
        }
    }

    ms << pluginDescription << "\n";

    // create markdown table
    ms << "\n" << tr("Inputs") << "\n--------------------------------------------------------------------------------\n\n";
    ms << tr("Input") << " | " << tr("Description") << " | " << tr("Optional") << "\n";
    ms << "--- | --- | ---\n";
    if (inputs.size() > 0) {
        Q_FOREACH(const QStringList &input, inputs) {
            QString inputName = input.at(0);
            QString inputDesc = input.at(1);
            QString inputOpt = input.at(2);

            ms << inputName << " | " << inputDesc << " | " << inputOpt << "\n";
        }
    }
    ms << "\n" << tr("Controls") << "\n--------------------------------------------------------------------------------\n\n";
    if (!genHTML) {
        // insert a special marker to be replaced in rst by the genStaticDocs.sh script)
        ms << "CONTROLSTABLEPROPS\n\n";
    }
    ms << tr("Parameter / script name") << " | " << tr("Type") << " | " << tr("Default") << " | " << tr("Function") << "\n";
    ms << "--- | --- | --- | ---\n";
    if (items.size() > 0) {
        Q_FOREACH(const QStringList &item, items) {
            QString itemLabel = item.at(0);
            QString itemScript = item.at(1);
            QString itemType = item.at(2);
            QString itemDefault = item.at(3);
            QString itemFunction = item.at(4);

            ms << itemLabel << " / `" << itemScript << "` | " << itemType << " | " << itemDefault << " | " << itemFunction << "\n";
        }
    }

    // add extra markdown if available
    if ( !extraMarkdown.isEmpty() ) {
        ms << "\n\n";
        ms << extraMarkdown;
    }

OUTPUT:
    // output
    if (genHTML) {
        // use hoedown to convert to HTML

        ts << ("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
               "  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
               "\n"
               "\n"
               "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
               "  <head>\n"
               "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
               "    \n"
               "    <title>") << tr("%1 node").arg(pluginLabel) << " &#8212; NATRON_DOCUMENTATION</title>\n";
        ts << ("    \n"
               "    <link rel=\"stylesheet\" href=\"_static/markdown.css\" type=\"text/css\" />\n"
               "    \n"
               "    <script type=\"text/javascript\" src=\"_static/jquery.js\"></script>\n"
               "    <script type=\"text/javascript\" src=\"_static/dropdown.js\"></script>\n"
               "    <link rel=\"index\" title=\"Index\" href=\"genindex.html\" />\n"
               "    <link rel=\"search\" title=\"Search\" href=\"search.html\" />\n"
               "  </head>\n"
               "  <body role=\"document\">\n"
               "    <div class=\"related\" role=\"navigation\" aria-label=\"related navigation\">\n"
               "      <h3>") << tr("Navigation") << "</h3>\n";
        ts << ("      <ul>\n"
               "        <li class=\"right\" style=\"margin-right: 10px\">\n"
               "          <a href=\"genindex.html\" title=\"General Index\"\n"
               "             accesskey=\"I\">") << tr("index") << "</a></li>\n";
        ts << ("        <li class=\"right\" >\n"
               "          <a href=\"py-modindex.html\" title=\"Python Module Index\"\n"
               "             >") << tr("modules") << "</a> |</li>\n";
        ts << ("        <li class=\"nav-item nav-item-0\"><a href=\"index.html\">NATRON_DOCUMENTATION</a> &#187;</li>\n"
               "          <li class=\"nav-item nav-item-1\"><a href=\"_group.html\" >") << tr("Reference Guide") << "</a> &#187;</li>\n";
        if ( !pluginGroup.isEmpty() ) {
            QString group = pluginGroup.at(0);
            if (!group.isEmpty()) {
                ts << "          <li class=\"nav-item nav-item-2\"><a href=\"_group.html?id=" << group << "\">" << group << " nodes</a> &#187;</li>";
            }
        }
        ts << ("      </ul>\n"
               "    </div>  \n"
               "\n"
               "    <div class=\"document\">\n"
               "      <div class=\"documentwrapper\">\n"
               "          <div class=\"body\" role=\"main\">\n"
               "            \n"
               "  <div class=\"section\">\n");
        QString html = Markdown::convert2html(markdown);
        ts << Markdown::fixNodeHTML(html);
        ts << ("</div>\n"
               "\n"
               "\n"
               "          </div>\n"
               "      </div>\n"
               "      <div class=\"clearer\"></div>\n"
               "    </div>\n"
               "\n"
               "    <div class=\"footer\" role=\"contentinfo\">\n"
               "        &#169; Copyright 2013-2018 The Natron documentation authors, licensed under CC BY-SA 4.0.\n"
               "    </div>\n"
               "  </body>\n"
               "</html>");
    } else {
        // this markdown will be processed externally by pandoc

        ts << markdown;
    }

    return ret;
} // Node::makeDocumentation

bool
Node::isForceCachingEnabled() const
{
    boost::shared_ptr<KnobBool> b = _imp->forceCaching.lock();

    return b ? b->getValue() : false;
}

void
Node::onSetSupportRenderScaleMaybeSet(int support)
{
    if ( (EffectInstance::SupportsEnum)support == EffectInstance::eSupportsYes ) {
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
    boost::shared_ptr<KnobBool> b =  _imp->useFullScaleImagesWhenRenderScaleUnsupported.lock();

    return b ? b->getValue() : false;
}

void
Node::beginEditKnobs()
{
    _imp->effect->beginEditKnobs();
}

void
Node::setEffect(const EffectInstancePtr& effect)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    _imp->effect = effect;
    _imp->effect->initializeData();


#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr thisShared = shared_from_this();
    NodePtr ioContainer = _imp->ioContainer.lock();
    if (ioContainer) {
        ReadNode* isReader = dynamic_cast<ReadNode*>( ioContainer->getEffectInstance().get() );
        if (isReader) {
            isReader->setEmbeddedReader(thisShared);
        } else {
            WriteNode* isWriter = dynamic_cast<WriteNode*>( ioContainer->getEffectInstance().get() );
            assert(isWriter);
            if (isWriter) {
                isWriter->setEmbeddedWriter(thisShared);
            }
        }
    }
#endif
}

EffectInstancePtr
Node::getEffectInstance() const
{
    ///Thread safe as it never changes
    return _imp->effect;
}

void
Node::hasViewersConnectedInternal(std::list<ViewerInstance* >* viewers,
                                 std::list<const Node*>* markedNodes) const
{

    if (std::find(markedNodes->begin(), markedNodes->end(), this) != markedNodes->end()) {
        return;
    }

    markedNodes->push_back(this);
    ViewerInstance* thisViewer = dynamic_cast<ViewerInstance*>( _imp->effect.get() );

    if (thisViewer) {
        viewers->push_back(thisViewer);
    } else {
        NodesList outputs;
        getOutputsWithGroupRedirection(outputs);

        for (NodesList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
            assert(*it);
            (*it)->hasViewersConnectedInternal(viewers, markedNodes);
        }
    }
}

void
Node::hasOutputNodesConnectedInternal(std::list<OutputEffectInstance* >* writers,
                                     std::list<const Node*>* markedNodes) const
{
    if (std::find(markedNodes->begin(), markedNodes->end(), this) != markedNodes->end()) {
        return;
    }

    markedNodes->push_back(this);

    OutputEffectInstance* thisWriter = dynamic_cast<OutputEffectInstance*>( _imp->effect.get() );

    if ( thisWriter && thisWriter->isOutput() && !dynamic_cast<GroupOutput*>(thisWriter) ) {
        writers->push_back(thisWriter);
    } else {
        NodesList outputs;
        getOutputsWithGroupRedirection(outputs);

        for (NodesList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
            assert(*it);
            (*it)->hasOutputNodesConnectedInternal(writers, markedNodes);
        }
    }
}

void
Node::hasOutputNodesConnected(std::list<OutputEffectInstance* >* writers) const
{
    std::list<const Node*> m;
    hasOutputNodesConnectedInternal(writers, &m);
}

void
Node::hasViewersConnected(std::list<ViewerInstance* >* viewers) const
{

    std::list<const Node*> m;
    hasViewersConnectedInternal(viewers, &m);

}

/**
 * @brief Resolves links of the graph in the case of containers (that do not do any rendering but only contain nodes inside)
 * so that algorithms that cycle the tree from bottom to top
 * properly visit all nodes in the correct order
 **/
static NodePtr
applyNodeRedirectionsUpstream(const NodePtr& node,
                              bool useGuiInput)
{
    if (!node) {
        return node;
    }
    NodeGroup* isGrp = node->isEffectGroup();
    if (isGrp) {
        //The node is a group, instead jump directly to the output node input of the  group
        return applyNodeRedirectionsUpstream(isGrp->getOutputNodeInput(useGuiInput), useGuiInput);
    }

    PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>( node->getEffectInstance().get() );
    if (isPrecomp) {
        //The node is a precomp, instead jump directly to the output node of the precomp
        return applyNodeRedirectionsUpstream(isPrecomp->getOutputNode(), useGuiInput);
    }

    GroupInput* isInput = dynamic_cast<GroupInput*>( node->getEffectInstance().get() );
    if (isInput) {
        //The node is a group input,  jump to the corresponding input of the group
        NodeCollectionPtr collection = node->getGroup();
        assert(collection);
        isGrp = dynamic_cast<NodeGroup*>( collection.get() );
        if (isGrp) {
            return applyNodeRedirectionsUpstream(isGrp->getRealInputForInput(useGuiInput, node), useGuiInput);
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
static void
applyNodeRedirectionsDownstream(int recurseCounter,
                                const NodePtr& node,
                                bool useGuiOutputs,
                                NodesList& translated)
{
    NodeGroup* isGrp = node->isEffectGroup();

    if (isGrp) {
        //The node is a group, meaning it should not be taken into account, instead jump directly to the input nodes output of the group
        NodesList inputNodes;
        isGrp->getInputsOutputs(&inputNodes, useGuiOutputs);
        for (NodesList::iterator it2 = inputNodes.begin(); it2 != inputNodes.end(); ++it2) {
            //Call recursively on them
            applyNodeRedirectionsDownstream(recurseCounter + 1, *it2, useGuiOutputs, translated);
        }

        return;
    }

    GroupOutput* isOutput = dynamic_cast<GroupOutput*>( node->getEffectInstance().get() );
    if (isOutput) {
        //The node is the output of a group, its outputs are the outputs of the group
        NodeCollectionPtr collection = isOutput->getNode()->getGroup();
        if (!collection) {
            return;
        }
        isGrp = dynamic_cast<NodeGroup*>( collection.get() );
        if (isGrp) {
            NodesWList groupOutputs;
            if (useGuiOutputs) {
                groupOutputs = isGrp->getNode()->getGuiOutputs();
            } else {
                NodePtr grpNode = isGrp->getNode();
                if (grpNode) {
                    grpNode->getOutputs_mt_safe(groupOutputs);
                }
            }
            for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                //Call recursively on them
                NodePtr output = it2->lock();
                if (output) {
                    applyNodeRedirectionsDownstream(recurseCounter + 1, output, useGuiOutputs, translated);
                }
            }
        }

        return;
    }

    boost::shared_ptr<PrecompNode> isInPrecomp = node->isPartOfPrecomp();
    if ( isInPrecomp && (isInPrecomp->getOutputNode() == node) ) {
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
                applyNodeRedirectionsDownstream(recurseCounter + 1, output, useGuiOutputs, translated);
            }
        }

        return;
    }

    //Base case: return this node
    if (recurseCounter > 0) {
        translated.push_back(node);
    }
} // applyNodeRedirectionsDownstream

void
Node::getOutputsWithGroupRedirection(NodesList& outputs) const
{
    NodesList redirections;
    NodePtr thisShared = boost::const_pointer_cast<Node>( shared_from_this() );

    applyNodeRedirectionsDownstream(0, thisShared, false, redirections);
    if ( !redirections.empty() ) {
        outputs.insert( outputs.begin(), redirections.begin(), redirections.end() );
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        for (NodesWList::const_iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
            NodePtr output = it->lock();
            if (output) {
                outputs.push_back(output);
            }
        }
    }
}



int
Node::getMajorVersion() const
{
    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if (_imp->pyPlugInfo.isPyPlug) {
            return _imp->pyPlugInfo.pyPlugVersion;
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
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if ( _imp->pyPlugInfo.isPyPlug ) {
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
    const int inputCount = getNInputs();
    InputsV oldInputs;
    {
        QMutexLocker k(&_imp->inputsLabelsMutex);
        _imp->inputLabels.resize(inputCount);
        _imp->inputHints.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputLabels[i] = _imp->effect->getInputLabel(i);
            _imp->inputHints[i] = _imp->effect->getInputHint(i);
        }
    }
    {
        QMutexLocker l(&_imp->lastRenderStartedMutex);
        _imp->inputIsRenderingCounter.resize(inputCount);
    }
    {
        QMutexLocker l(&_imp->inputsMutex);
        oldInputs = _imp->inputs;

        std::vector<bool> oldInputsVisibility = _imp->inputsVisibility;
        _imp->inputIsRenderingCounter.resize(inputCount);
        _imp->inputs.resize(inputCount);
        _imp->guiInputs.resize(inputCount);
        _imp->inputsVisibility.resize(inputCount);
        ///if we added inputs, just set to NULL the new inputs, and add their label to the labels map
        for (int i = 0; i < inputCount; ++i) {
            if ( i < (int)oldInputs.size() ) {
                _imp->inputs[i] = oldInputs[i];
                _imp->guiInputs[i] = oldInputs[i];
            } else {
                _imp->inputs[i].reset();
                _imp->guiInputs[i].reset();
            }
            if (i < (int) oldInputsVisibility.size()) {
                _imp->inputsVisibility[i] = oldInputsVisibility[i];
            } else {
                _imp->inputsVisibility[i] = true;
            }
        }


        ///Set the components the plug-in accepts
        _imp->inputsComponents.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputsComponents[i].clear();
            if ( _imp->effect->isInputMask(i) ) {
                //Force alpha for masks
                _imp->inputsComponents[i].push_back( ImagePlaneDesc::getAlphaComponents() );
            } else {
                _imp->effect->addAcceptedComponents(i, &_imp->inputsComponents[i]);
            }
        }
        _imp->outputComponents.clear();
        _imp->effect->addAcceptedComponents(-1, &_imp->outputComponents);
    }
    _imp->inputsInitialized = true;

    Q_EMIT inputsInitialized();
} // Node::initializeInputs

NodePtr
Node::getInput(int index) const
{
    return getInputInternal(false, true, index);
}

NodePtr
Node::getInputInternal(bool useGuiInput,
                       bool useGroupRedirections,
                       int index) const
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

const std::vector<NodeWPtr> &
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

const std::vector<NodeWPtr> &
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

std::vector<NodeWPtr>
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

std::string
Node::getInputHint(int inputNb) const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsLabelsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputHints.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }

    return _imp->inputHints[inputNb];
}

void
Node::setInputLabel(int inputNb, const std::string& label)
{
    {
        QMutexLocker l(&_imp->inputsLabelsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputLabels.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputLabels[inputNb] = label;
    }
    std::map<int, MaskSelector>::iterator foundMask = _imp->maskSelectors.find(inputNb);
    if (foundMask != _imp->maskSelectors.end()) {
        foundMask->second.channel.lock()->setLabel(label);
    }

    std::map<int, ChannelSelector>::iterator foundChannel = _imp->channelsSelectors.find(inputNb);
    if (foundChannel != _imp->channelsSelectors.end()) {
        foundChannel->second.layer.lock()->setLabel(label + std::string(" Layer"));
    }

    Q_EMIT inputEdgeLabelChanged(inputNb, QString::fromUtf8(label.c_str()));
}

void
Node::setInputHint(int inputNb, const std::string& hint)
{
    {
        QMutexLocker l(&_imp->inputsLabelsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputHints.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputHints[inputNb] = hint;
    }
}

bool
Node::isInputVisible(int inputNb) const
{
    QMutexLocker k(&_imp->inputsMutex);
    if (inputNb >= 0 && inputNb < (int)_imp->inputsVisibility.size()) {
        return _imp->inputsVisibility[inputNb];
    } else {
        throw std::invalid_argument("Index out of range");
    }
    return false;
}

void
Node::setInputVisible(int inputNb, bool visible)
{
    {
        QMutexLocker k(&_imp->inputsMutex);
        if (inputNb >= 0 && inputNb < (int)_imp->inputsVisibility.size()) {
            _imp->inputsVisibility[inputNb] = visible;
        } else {
            throw std::invalid_argument("Index out of range");
        }
    }
    Q_EMIT inputVisibilityChanged(inputNb);
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
Node::isInputOnlyAlpha(int inputNb) const
{
    assert(_imp->inputsInitialized);

    const std::list<ImagePlaneDesc>& inputSupportedComps = _imp->inputsComponents[inputNb];
    return (inputSupportedComps.size() == 1 && inputSupportedComps.front().getNumComponents() == 1);
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
        if ( _imp->inputs[i].lock() ) {
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
        if ( !_imp->inputs[i].lock() && !_imp->effect->isInputOptional(i) ) {
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
        if ( !_imp->inputs[i].lock() ) {
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
    if ( isOutputNode() ) {
        return true;
    }
    if ( QThread::currentThread() == qApp->thread() ) {
        if (_imp->outputs.size() == 1) {
            NodePtr output = _imp->outputs.front().lock();

            return !( output->isTrackerNodePlugin() && output->isMultiInstance() );
        } else if (_imp->outputs.size() > 1) {
            return true;
        }
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        if (_imp->outputs.size() == 1) {
            NodePtr output = _imp->outputs.front().lock();

            return !( output->isTrackerNodePlugin() && output->isMultiInstance() );
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
    if (!input || input == this) {
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

static Node::CanConnectInputReturnValue
checkCanConnectNoMultiRes(const Node* output,
                          const NodePtr& input)
{
    //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
    //Check that the input has the same RoD that another input and that its rod is set to 0,0
    RenderScale scale(1.);
    RectD rod;
    bool isProjectFormat;
    StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(input->getHashValue(),
                                                                               output->getApp()->getTimeLine()->currentFrame(),
                                                                               scale,
                                                                               ViewIdx(0),
                                                                               &rod, &isProjectFormat);

    if ( (stat == eStatusFailed) && !rod.isNull() ) {
        return Node::eCanConnectInput_givenNodeNotConnectable;
    }
    if ( (rod.x1 != 0) || (rod.y1 != 0) ) {
        return Node::eCanConnectInput_multiResNotSupported;
    }

    // Commented-out: Some Furnace plug-ins from The Foundry (e.g F_Steadiness) are not supporting multi-resolution but actually produce an output
    // with a RoD different from the input

    /*RectD outputRod;
       stat = output->getEffectInstance()->getRegionOfDefinition_public(output->getHashValue(), output->getApp()->getTimeLine()->currentFrame(), scale, ViewIdx(0), &outputRod, &isProjectFormat);
       Q_UNUSED(stat);

       if ( !outputRod.isNull() && (rod != outputRod) ) {
        return Node::eCanConnectInput_multiResNotSupported;
       }*/

    for (int i = 0; i < output->getNInputs(); ++i) {
        NodePtr inputNode = output->getInput(i);
        if (inputNode) {
            RectD inputRod;
            stat = inputNode->getEffectInstance()->getRegionOfDefinition_public(inputNode->getHashValue(),
                                                                                output->getApp()->getTimeLine()->currentFrame(),
                                                                                scale,
                                                                                ViewIdx(0),
                                                                                &inputRod, &isProjectFormat);
            if ( (stat == eStatusFailed) && !inputRod.isNull() ) {
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
Node::canConnectInput(const NodePtr& input,
                      int inputNumber) const
{
    ///No-one is allowed to connect to the other node
    if ( !input || !input->canOthersConnectToThisNode() ) {
        return eCanConnectInput_givenNodeNotConnectable;
    }

    ///Check for invalid index
    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber >= (int)_imp->guiInputs.size() ) ) {
            return eCanConnectInput_indexOutOfRange;
        }
        if ( _imp->guiInputs[inputNumber].lock() ) {
            return eCanConnectInput_inputAlreadyConnected;
        }
    }

    NodeGroup* isGrp = input->isEffectGroup();
    if ( isGrp && !isGrp->getOutputNode(true) ) {
        return eCanConnectInput_groupHasNoOutput;
    }

    if ( getParentMultiInstance() || input->getParentMultiInstance() ) {
        return eCanConnectInput_inputAlreadyConnected;
    }

    ///Applying this connection would create cycles in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return eCanConnectInput_graphCycles;
    }

    if ( _imp->effect->isInputRotoBrush(inputNumber) ) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";

        return eCanConnectInput_indexOutOfRange;
    }

    if ( !_imp->effect->supportsMultiResolution() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return ret;
        }
    }

    {
        ///Check for invalid pixel aspect ratio if the node doesn't support multiple clip PARs

        double inputPAR = input->getEffectInstance()->getAspectRatio(-1);
        double inputFPS = input->getEffectInstance()->getFrameRate();
        QMutexLocker l(&_imp->inputsMutex);

        for (InputsV::const_iterator it = _imp->guiInputs.begin(); it != _imp->guiInputs.end(); ++it) {
            NodePtr node = it->lock();
            if (node) {
                if ( !_imp->effect->supportsMultipleClipPARs() ) {
                    if (node->getEffectInstance()->getAspectRatio(-1) != inputPAR) {
                        return eCanConnectInput_differentPars;
                    }
                }

                if (std::abs(node->getEffectInstance()->getFrameRate() - inputFPS) > 0.01) {
                    return eCanConnectInput_differentFPS;
                }
            }
        }
    }

    return eCanConnectInput_ok;
} // Node::canConnectInput

bool
Node::connectInput(const NodePtr & input,
                   int inputNumber)
{
    assert(_imp->inputsInitialized);
    assert(input);

    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }
    if ( _imp->effect->isInputRotoBrush(inputNumber) ) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";

        return false;
    }

    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if ( !_imp->effect->supportsMultiResolution() && !getApp()->getProject()->isLoadingProject() ) {
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
        if ( (inputNumber < 0) ||
             ( inputNumber >= (int)_imp->inputs.size() ) ||
             ( !useGuiInputs && _imp->inputs[inputNumber].lock() ) ||
             ( useGuiInputs && _imp->guiInputs[inputNumber].lock() ) ) {
            return false;
        }

        ///Set the input
        if (!useGuiInputs) {
            _imp->inputs[inputNumber] = input;
            _imp->guiInputs[inputNumber] = input;
        } else {
            _imp->guiInputs[inputNumber] = input;
        }
        input->connectOutput( useGuiInputs, shared_from_this() );
    }

    getApp()->recheckInvalidExpressions();

    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );

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
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    if (mustCallEnd) {
        endInputEdition(true);
    }

    return true;
} // Node::connectInput

void
Node::Implementation::ifGroupForceHashChangeOfInputs()
{
    ///If the node is a group, force a change of the outputs of the GroupInput nodes so the hash of the tree changes downstream
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( effect.get() );

    if ( isGrp && !isGrp->getApp()->isCreatingNodeTree() ) {
        NodesList inputsOutputs;
        isGrp->getInputsOutputs(&inputsOutputs, false);
        for (NodesList::iterator it = inputsOutputs.begin(); it != inputsOutputs.end(); ++it) {
            (*it)->incrementKnobsAge_internal();
            (*it)->computeHash();
        }
    }
}

bool
Node::replaceInputInternal(const NodePtr& input, int inputNumber, bool useGuiInputs)
{
    assert(_imp->inputsInitialized);
    assert(input);
    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }
    if ( _imp->effect->isInputRotoBrush(inputNumber) ) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";

        return false;
    }

    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if ( !_imp->effect->supportsMultiResolution() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }

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
                QObject::connect( curIn.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );
                curIn->disconnectOutput(useGuiInputs, this);
            }
            _imp->inputs[inputNumber] = input;
            _imp->guiInputs[inputNumber] = input;
        } else {
            NodePtr curIn = _imp->guiInputs[inputNumber].lock();
            if (curIn) {
                QObject::connect( curIn.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );
                curIn->disconnectOutput(useGuiInputs, this);
            }
            _imp->guiInputs[inputNumber] = input;
        }
        input->connectOutput( useGuiInputs, shared_from_this() );
    }

    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );

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
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    if (mustCallEnd) {
        endInputEdition(true);
    }
    
    return true;
}

bool
Node::replaceInput(const NodePtr& input,
                   int inputNumber)
{


    bool useGuiInputs = isNodeRendering();
    _imp->effect->abortAnyEvaluation();
    return replaceInputInternal(input, inputNumber, useGuiInputs);
} // Node::replaceInput

void
Node::switchInput0And1()
{
    assert(_imp->inputsInitialized);
    int maxInputs = getNInputs();
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
    if ( !inputChangedCB.empty() ) {
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
Node::connectOutput(bool useGuiValues,
                    const NodePtr& output)
{
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
        if ( (inputNumber < 0) ||
             ( inputNumber > (int)_imp->inputs.size() ) ||
             ( !useGuiValues && !_imp->inputs[inputNumber].lock() ) ||
             ( useGuiValues && !_imp->guiInputs[inputNumber].lock() ) ) {
            return -1;
        }
        inputShared = useGuiValues ? _imp->guiInputs[inputNumber].lock() : _imp->inputs[inputNumber].lock();
    }


    QObject::disconnect( inputShared.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );
    inputShared->disconnectOutput(useGuiValues, this);

    {
        QMutexLocker l(&_imp->inputsMutex);
        if (!useGuiValues) {
            _imp->inputs[inputNumber].reset();
            _imp->guiInputs[inputNumber].reset();
        } else {
            _imp->guiInputs[inputNumber].reset();
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
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }
    if (mustCallEnd) {
        endInputEdition(true);
    }

    return inputNumber;
} // Node::disconnectInput

int
Node::disconnectInputInternal(Node* input, bool useGuiValues)
{
    assert(_imp->inputsInitialized);
    int found = -1;
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
            }
        }
        input->disconnectOutput(useGuiValues, this);
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
            if ( !getApp()->getProject()->isProjectClosing() ) {
                computeHash();
            }
        }


        _imp->ifGroupForceHashChangeOfInputs();

        std::string inputChangedCB = getInputChangedCallback();
        if ( !inputChangedCB.empty() ) {
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
Node::disconnectInput(Node* input)
{

    bool useGuiValues = isNodeRendering();
    _imp->effect->abortAnyEvaluation();
    return disconnectInputInternal(input, useGuiValues);
} // Node::disconnectInput

int
Node::disconnectOutput(bool useGuiValues,
                       const Node* output)
{
    assert(output);
    int ret = -1;
    {
        QMutexLocker l(&_imp->outputsMutex);
        if (!useGuiValues) {
            int ret = 0;
            for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it, ++ret) {
                if (it->lock().get() == output) {
                    _imp->outputs.erase(it);
                    break;
                }
            }
        }
        int ret = 0;
        for (NodesWList::iterator it = _imp->guiOutputs.begin(); it != _imp->guiOutputs.end(); ++it, ++ret) {
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
Node::deactivate(const std::list<NodePtr> & outputsToDisconnect,
                 bool disconnectAll,
                 bool reconnect,
                 bool hideGui,
                 bool triggerRender,
                 bool unslaveKnobs)
{

    if ( !_imp->effect || !isActivated() ) {
        return;
    }
    //first tell the gui to clear any persistent message linked to this node
    clearPersistentMessage(false);



    bool beingDestroyed;
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        beingDestroyed = _imp->isBeingDestroyed;
    }


    ///kill any thread it could have started
    ///Commented-out: If we were to undo the deactivate we don't want all threads to be
    ///exited, just exit them when the effect is really deleted instead
    //quitAnyProcessing();
    if (!beingDestroyed) {
        _imp->effect->abortAnyEvaluation(false /*keepOldestRender*/);
        _imp->abortPreview_non_blocking();
    }

    NodeCollectionPtr parentCol = getGroup();


    if (unslaveKnobs) {
        ///For all knobs that have listeners, invalidate expressions
        NodeGroup* isParentGroup = dynamic_cast<NodeGroup*>( parentCol.get() );
        KnobsVec  knobs = _imp->effect->getKnobs_mt_safe();
        for (U32 i = 0; i < knobs.size(); ++i) {
            KnobI::ListenerDimsMap listeners;
            knobs[i]->getListeners(listeners);
            for (KnobI::ListenerDimsMap::iterator it = listeners.begin(); it != listeners.end(); ++it) {
                KnobIPtr listener = it->first.lock();
                if (!listener) {
                    continue;
                }
                KnobHolder* holder = listener->getHolder();
                if (!holder) {
                    continue;
                }
                if ( ( holder == _imp->effect.get() ) || (holder == isParentGroup) ) {
                    continue;
                }

                EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
                if (!isEffect) {
                    continue;
                }

                NodeCollectionPtr effectParent = isEffect->getNode()->getGroup();
                if (!effectParent) {
                    continue;
                }
                NodeGroup* isEffectParentGroup = dynamic_cast<NodeGroup*>( effectParent.get() );
                if ( isEffectParentGroup && ( isEffectParentGroup == _imp->effect.get() ) ) {
                    continue;
                }

                isEffect->beginChanges();
                for (int dim = 0; dim < listener->getDimension(); ++dim) {
                    std::pair<int, KnobIPtr> master = listener->getMaster(dim);
                    if (master.second == knobs[i]) {
                        listener->unSlave(dim, true);
                    }

                    std::string hasExpr = listener->getExpression(dim);
                    if ( !hasExpr.empty() ) {
                        std::stringstream ss;
                        ss << tr("Missing node ").toStdString();
                        ss << getFullyQualifiedName();
                        ss << ' ';
                        ss << tr("in expression.").toStdString();
                        listener->setExpressionInvalid( dim, false, ss.str() );
                    }
                }
                isEffect->endChanges(true);
            }
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


    std::vector<NodePtr> inputsQueueCopy;


    ///For multi-instances, if we deactivate the main instance without hiding the GUI (the default state of the tracker node)
    ///then don't remove it from outputs of the inputs
    if (hideGui || !_imp->isMultiInstance) {
        for (std::size_t i = 0; i < _imp->guiInputs.size(); ++i) {
            NodePtr input = _imp->guiInputs[i].lock();
            if (input) {
                input->disconnectOutput(false, this);
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
                    output->replaceInputInternal(inputToConnectTo, inputNb, false);
                } else {
                    ignore_result( output->disconnectInputInternal(this, false) );
                }
            }
        }
    }

    // If the effect was doing OpenGL rendering and had context(s) bound, dettach them.
    _imp->effect->dettachAllOpenGLContexts();


#if NATRON_VERSION_ENCODED < NATRON_VERSION_ENCODE(3,0,0)
    // In Natron 2, the meta Read node is NOT a Group, hence the internal decoder node is not a child of the Read node.
    // As a result of it, if the user deleted the meta Read node, the internal decoder is node destroyed
    ReadNode* isReader = dynamic_cast<ReadNode*>(_imp->effect.get());
    if (isReader) {
        NodePtr internalDecoder = isReader->getEmbeddedReader();
        if (internalDecoder) {
            internalDecoder->deactivate();
        }
    }
#endif

    ///Free all memory used by the plug-in.

    ///COMMENTED-OUT: Don't do this, the node may still be rendering here since the abort call is not blocking.
    ///_imp->effect->clearPluginMemoryChunks();
    clearLastRenderedImage();

    if (parentCol && !beingDestroyed) {
        parentCol->notifyNodeDeactivated( shared_from_this() );
    }

    if (hideGui && !beingDestroyed) {
        Q_EMIT deactivated(triggerRender);
    }
    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = false;
    }


    ///If the node is a group, deactivate all nodes within the group
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGrp) {
        isGrp->setIsDeactivatingGroup(true);
        NodesList nodes = isGrp->getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->deactivate(std::list<NodePtr>(), false, false, true, false);
        }
        isGrp->setIsDeactivatingGroup(false);
    }

    ///If the node has children (i.e it is a multi-instance), deactivate its children
    for (NodesWList::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        it->lock()->deactivate(std::list<NodePtr>(), false, false, true, false);
    }


    AppInstancePtr app = getApp();
    if ( app && !app->getProject()->isProjectClosing() ) {
        _imp->runOnNodeDeleteCB();
    }

    deleteNodeVariableToPython( getFullyQualifiedName() );
} // deactivate

void
Node::activate(const std::list<NodePtr> & outputsToRestore,
               bool restoreAll,
               bool triggerRender)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !_imp->effect || isActivated() ) {
        return;
    }


    ///No need to lock, guiInputs is only written to by the main-thread
    NodePtr thisShared = shared_from_this();

    ///for all inputs, reconnect their output to this node
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            input->connectOutput(false, thisShared);
        }
    }


    ///Restore all outputs that was connected to this node
    for (std::map<NodeWPtr, int >::iterator it = _imp->deactivatedState.begin();
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

#if NATRON_VERSION_ENCODED < NATRON_VERSION_ENCODE(3,0,0)
    // In Natron 2, the meta Read node is NOT a Group, hence the internal decoder node is not a child of the Read node.
    // As a result of it, if the user deleted the meta Read node, the internal decoder is node destroyed
    ReadNode* isReader = dynamic_cast<ReadNode*>(_imp->effect.get());
    if (isReader) {
        NodePtr internalDecoder = isReader->getEmbeddedReader();
        if (internalDecoder) {
            internalDecoder->activate();
        }
    }
#endif

    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = true; //< flag it true before notifying the GUI because the gui rely on this flag (espcially the Viewer)
    }

    NodeCollectionPtr group = getGroup();
    if (group) {
        group->notifyNodeActivated( shared_from_this() );
    }
    Q_EMIT activated(triggerRender);


    declareAllPythonAttributes();
    getApp()->recheckInvalidExpressions();


    ///If the node is a group, activate all nodes within the group first
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGrp) {
        isGrp->setIsActivatingGroup(true);
        NodesList nodes = isGrp->getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->activate(std::list<NodePtr>(), false, false);
        }
        isGrp->setIsActivatingGroup(false);
    }

    ///If the node has children (i.e it is a multi-instance), activate its children
    for (NodesWList::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        it->lock()->activate(std::list<NodePtr>(), false, false);
    }

    _imp->runOnNodeCreatedCB(true);
} // activate

class NodeDestroyNodeInternalArgs
    : public GenericWatcherCallerArgs
{
public:

    bool autoReconnect;

    NodeDestroyNodeInternalArgs()
        : GenericWatcherCallerArgs()
        , autoReconnect(false)
    {}

    virtual ~NodeDestroyNodeInternalArgs() {}
};

void
Node::onProcessingQuitInDestroyNodeInternal(int taskID,
                                            const WatcherCallerArgsPtr& args)
{
    assert(_imp->renderWatcher);
    assert(taskID == (int)NodeRenderWatcher::eBlockingTaskQuitAnyProcessing);
    Q_UNUSED(taskID);
    assert(args);
    NodeDestroyNodeInternalArgs* thisArgs = dynamic_cast<NodeDestroyNodeInternalArgs*>( args.get() );
    assert(thisArgs);
    doDestroyNodeInternalEnd(thisArgs ? thisArgs->autoReconnect : false);
    _imp->renderWatcher.reset();
}

void
Node::doDestroyNodeInternalEnd(bool autoReconnect)
{
    ///Remove the node from the project
    deactivate(NodesList(),
               true,
               autoReconnect,
               true,
               false);


    {
        NodeGuiIPtr guiPtr = _imp->guiPointer.lock();
        if (guiPtr) {
            guiPtr->destroyGui();
        }
    }

    ///If its a group, clear its nodes
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGrp) {
        isGrp->clearNodesBlocking();
    }


    ///Quit any rendering
    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( _imp->effect.get() );
    if (isOutput) {
        isOutput->getRenderEngine()->quitEngine(true);
    }

    ///Remove all images in the cache associated to this node
    ///This will not remove from the disk cache if the project is closing
    removeAllImagesFromCache(false);

    AppInstancePtr app = getApp();
    if (app) {
        app->recheckInvalidExpressions();
    }

    ///Remove the Python node
    deleteNodeVariableToPython( getFullyQualifiedName() );

    ///Disconnect all inputs
    /*int maxInputs = getNInputs();
       for (int i = 0; i < maxInputs; ++i) {
       disconnectInput(i);
       }*/

    ///Kill the effect
    if (_imp->effect) {
        _imp->effect->clearPluginMemoryChunks();
    }
    _imp->effect.reset();

    ///If inside the group, remove it from the group
    ///the use_count() after the call to removeNode should be 2 and should be the shared_ptr held by the caller and the
    ///thisShared ptr

    ///If not inside a gorup or inside fromDest the shared_ptr is probably invalid at this point
    NodeCollectionPtr thisGroup = getGroup();
    if ( thisGroup ) {
        thisGroup->removeNode(this);
    }

} // Node::doDestroyNodeInternalEnd

void
Node::destroyNode(bool blockingDestroy, bool autoReconnect)
{
    if (!_imp->effect) {
        return;
    }

    {
        QMutexLocker k(&_imp->activatedMutex);
        _imp->isBeingDestroyed = true;
    }

    bool allProcessingQuit = areAllProcessingThreadsQuit();
    if (allProcessingQuit || blockingDestroy) {
        if (!allProcessingQuit) {
            quitAnyProcessing_blocking(false);
        }
        doDestroyNodeInternalEnd(false);
    } else {
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>( _imp->effect.get() );
        NodesList nodesToWatch;
        nodesToWatch.push_back( shared_from_this() );
        if (isGrp) {
            isGrp->getNodes_recursive(nodesToWatch, false);
        }
        _imp->renderWatcher.reset( new NodeRenderWatcher(nodesToWatch) );
        QObject::connect( _imp->renderWatcher.get(), SIGNAL(taskFinished(int,WatcherCallerArgsPtr)), this, SLOT(onProcessingQuitInDestroyNodeInternal(int,WatcherCallerArgsPtr)) );
        boost::shared_ptr<NodeDestroyNodeInternalArgs> args = boost::make_shared<NodeDestroyNodeInternalArgs>();
        args->autoReconnect = autoReconnect;
        _imp->renderWatcher->scheduleBlockingTask(NodeRenderWatcher::eBlockingTaskQuitAnyProcessing, args);

    }
} // Node::destroyNodeInternal


KnobIPtr
Node::getKnobByName(const std::string & name) const
{
    ///MT-safe, never changes
    assert(_imp->knobsInitialized);
    if (!_imp->effect) {
        return KnobIPtr();
    }

    return _imp->effect->getKnobByName(name);
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

///output is always RGBA with alpha = 255
template<typename PIX, int maxValue, int srcNComps>
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
                int xi = std::floor(x + 0.5) - srcBounds.x1;     // round to nearest
                if ( (xi < 0) || ( xi >= (srcBounds.x2 - srcBounds.x1) ) ) {
#ifndef __NATRON_WIN32__
                    dst_pixels[j] = toBGRA(0, 0, 0, 0);
#else
                    dst_pixels[j] = toBGRA(0, 0, 0, 255);
#endif
                } else {
                    float rFilt = src_pixels[xi * srcNComps] * (1.f / maxValue);
                    float gFilt = srcNComps < 2 ? 0 : src_pixels[xi * srcNComps + 1] * (1.f / maxValue);
                    float bFilt = srcNComps < 3 ? 0 : src_pixels[xi * srcNComps + 2] * (1.f / maxValue);
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
}     // renderPreview

///output is always RGBA with alpha = 255
template<typename PIX, int maxValue>
void
renderPreviewForDepth(const Image & srcImg,
                      int elemCount,
                      int *dstWidth,
                      int *dstHeight,
                      bool convertToSrgb,
                      unsigned int* dstPixels)
{
    switch (elemCount) {
    case 0:

        return;
    case 1:
        renderPreview<PIX, maxValue, 1>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
        break;
    case 2:
        renderPreview<PIX, maxValue, 2>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
        break;
    case 3:
        renderPreview<PIX, maxValue, 3>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
        break;
    case 4:
        renderPreview<PIX, maxValue, 4>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
        break;
    default:
        break;
    }
}     // renderPreviewForDepth

NATRON_NAMESPACE_ANONYMOUS_EXIT


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

        bool mustQuitPreview = _imp->checkForExitPreview();
        Q_UNUSED(mustQuitPreview);
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

    if ( _imp->checkForExitPreview() ) {
        return false;
    }

    /// prevent 2 previews to occur at the same time since there's only 1 preview instance
    ComputingPreviewSetter_RAII computingPreviewRAII( _imp.get() );
    RectD rod;
    bool isProjectFormat;
    RenderScale scale(1.);
    U64 nodeHash = getHashValue();
    EffectInstance* effect = 0;
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGroup) {
        effect = isGroup->getOutputNode(false)->getEffectInstance().get();
    } else {
        effect = _imp->effect.get();
    }

    if (!_imp->effect) {
        return false;
    }

    effect->clearPersistentMessage(false);

    StatusEnum stat = effect->getRegionOfDefinition_public(nodeHash, time, scale, ViewIdx(0), &rod, &isProjectFormat);
    if ( (stat == eStatusFailed) || rod.isNull() ) {
        return false;
    }
    assert( !rod.isNull() );
    double yZoomFactor = (double)*height / (double)rod.height();
    double xZoomFactor = (double)*width / (double)rod.width();
    double closestPowerOf2X = xZoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil( std::log(xZoomFactor) / std::log(2.) ) );
    double closestPowerOf2Y = yZoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil( std::log(yZoomFactor) / std::log(2.) ) );
    int closestPowerOf2 = std::max(closestPowerOf2X, closestPowerOf2Y);
    unsigned int mipMapLevel = std::min(std::log( (double)closestPowerOf2 ) / std::log(2.), 5.);

    scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;

    const double par = effect->getAspectRatio(-1);
    RectI renderWindow;
    rod.toPixelEnclosing(mipMapLevel, par, &renderWindow);

    NodePtr thisNode = shared_from_this();
    RenderingFlagSetter flagIsRendering(thisNode);


    {
        AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(true, 0);
        const bool isRenderUserInteraction = true;
        const bool isSequentialRender = false;
        AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( QThread::currentThread() );
        if (isAbortable) {
            isAbortable->setAbortInfo( isRenderUserInteraction, abortInfo, thisNode->getEffectInstance() );
        }
        ParallelRenderArgsSetter frameRenderArgs( time,
                                                  ViewIdx(0), //< preview only renders view 0 (left)
                                                  isRenderUserInteraction, //<isRenderUserInteraction
                                                  isSequentialRender, //isSequential
                                                  abortInfo, // abort info
                                                  thisNode, // viewer requester
                                                  0, //texture index
                                                  getApp()->getTimeLine().get(), // timeline
                                                  NodePtr(), //rotoPaint node
                                                  false, // isAnalysis
                                                  true, // isDraft
                                                  boost::shared_ptr<RenderStats>() );
        FrameRequestMap request;
        stat = EffectInstance::computeRequestPass(time, ViewIdx(0), mipMapLevel, rod, thisNode, request);
        if (stat == eStatusFailed) {
            return false;
        }

        frameRenderArgs.updateNodesRequest(request);

        std::list<ImagePlaneDesc> requestedComps;
        ImageBitDepthEnum depth = effect->getBitDepth(-1);
        {
            ImagePlaneDesc plane, pairedPlane;
            effect->getMetadataComponents(-1, &plane, &pairedPlane);
            requestedComps.push_back(plane);
        }


        // Exceptions are caught because the program can run without a preview,
        // but any exception in renderROI is probably fatal.
        std::map<ImagePlaneDesc, ImagePtr> planes;
        try {
            boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs( new EffectInstance::RenderRoIArgs(time,
                                                                                                           scale,
                                                                                                           mipMapLevel,
                                                                                                           ViewIdx(0), //< preview only renders view 0 (left)
                                                                                                           false,
                                                                                                           renderWindow,
                                                                                                           rod,
                                                                                                           requestedComps, //< preview is always rgb...
                                                                                                           depth,
                                                                                                           false,
                                                                                                           effect,
                                                                                                           eStorageModeRAM /*returnStorage*/,
                                                                                                           time /*callerRenderTime*/) );
            EffectInstance::RenderRoIRetCode retCode;
            retCode = effect->renderRoI(*renderArgs, &planes);
            if (retCode != EffectInstance::eRenderRoIRetCodeOk) {
                return false;
            }
        } catch (...) {
            return false;
        }

        if ( planes.empty() ) {
            return false;
        }

        const ImagePtr& img = planes.begin()->second;
        const ImagePlaneDesc& components = img->getComponents();
        int elemCount = components.getNumComponents();

        ///we convert only when input is Linear.
        //Rec709 and srGB is acceptable for preview
        bool convertToSrgb = getApp()->getDefaultColorSpaceForBitDepth( img->getBitDepth() ) == eViewerColorSpaceLinear;

        switch ( img->getBitDepth() ) {
        case eImageBitDepthByte: {
            renderPreviewForDepth<unsigned char, 255>(*img, elemCount, width, height, convertToSrgb, buf);
            break;
        }
        case eImageBitDepthShort: {
            renderPreviewForDepth<unsigned short, 65535>(*img, elemCount, width, height, convertToSrgb, buf);
            break;
        }
        case eImageBitDepthHalf:
            break;
        case eImageBitDepthFloat: {
            renderPreviewForDepth<float, 1>(*img, elemCount, width, height, convertToSrgb, buf);
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
    return _imp->effect->isGenerator() && ! _imp->effect->isFilter();
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
    return dynamic_cast<ViewerInstance*>( _imp->effect.get() );
}

NodeGroup*
Node::isEffectGroup() const
{
    return dynamic_cast<NodeGroup*>( _imp->effect.get() );
}

boost::shared_ptr<RotoContext>
Node::getRotoContext() const
{
    return _imp->rotoContext;
}

boost::shared_ptr<TrackerContext>
Node::getTrackerContext() const
{
    return _imp->trackContext;
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
    if (!_imp->plugin) {
        return std::string();
    }

    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if ( !_imp->pyPlugInfo.pyPlugIconFilePath.empty() ) {
            return _imp->pyPlugInfo.pyPlugIconFilePath;
        }
    }
    return _imp->plugin->getIconFilePath().toStdString();
}

std::string
Node::getPluginID() const
{
    if (!_imp->plugin) {
        return std::string();
    }

    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if ( _imp->pyPlugInfo.isPyPlug ) {
            return _imp->pyPlugInfo.pyPlugID;
        }
    }

    return _imp->plugin->getPluginID().toStdString();
}

std::string
Node::getPluginLabel() const
{
    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if ( !_imp->pyPlugInfo.pyPlugLabel.empty() ) {
            return _imp->pyPlugInfo.pyPlugLabel;
        }
    }

    return _imp->effect->getPluginLabel();
}

std::string
Node::getPluginResourcesPath() const
{
    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if ( !_imp->pyPlugInfo.pluginPythonModule.empty() ) {
            std::size_t foundSlash = _imp->pyPlugInfo.pluginPythonModule.find_last_of("/");
            if (foundSlash != std::string::npos) {
                return _imp->pyPlugInfo.pluginPythonModule.substr(0, foundSlash);
            }
        }
    }

    return _imp->plugin->getResourcesPath().toStdString();
}

std::string
Node::getPluginDescription() const
{
    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if ( _imp->pyPlugInfo.isPyPlug ) {
            return _imp->pyPlugInfo.pyPlugDesc;
        }
    }

    // if this is a Read or Write plugin, return the description from the embedded plugin
    std::string pluginID = getPluginID();
    if (pluginID == PLUGINID_NATRON_READ ||
        pluginID == PLUGINID_NATRON_WRITE) {
        EffectInstancePtr effectInstance = getEffectInstance();
        if ( effectInstance && effectInstance->isReader() ) {
            ReadNode* isReadNode = dynamic_cast<ReadNode*>( effectInstance.get() );

            if (isReadNode) {
                NodePtr subnode = isReadNode->getEmbeddedReader();
                if (subnode) {
                    return subnode->getPluginDescription();
                }
            }
        } else if ( effectInstance && effectInstance->isWriter() ) {
            WriteNode* isWriteNode = dynamic_cast<WriteNode*>( effectInstance.get() );

            if (isWriteNode) {
                NodePtr subnode = isWriteNode->getEmbeddedWriter();
                if (subnode) {
                    return subnode->getPluginDescription();
                }
            }
        }
    }

    return _imp->effect->getPluginDescription();
}

void
Node::getPluginGrouping(std::list<std::string>* grouping) const
{
    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        if ( !_imp->pyPlugInfo.pyPlugGrouping.empty() ) {
            *grouping =  _imp->pyPlugInfo.pyPlugGrouping;
            return;
        }
    }
    _imp->effect->getPluginGrouping(grouping);
}

std::string
Node::getPyPlugID() const
{
    return _imp->pyPlugInfo.pyPlugID;
}

std::string
Node::getPyPlugLabel() const
{
    return _imp->pyPlugInfo.pyPlugLabel;
}

std::string
Node::getPyPlugDescription() const
{
    return _imp->pyPlugInfo.pyPlugDesc;
}

void
Node::getPyPlugGrouping(std::list<std::string>* grouping) const
{
    *grouping = _imp->pyPlugInfo.pyPlugGrouping;
}


std::string
Node::getPyPlugIconFilePath() const
{
    return _imp->pyPlugInfo.pyPlugIconFilePath;
}

int
Node::getPyPlugMajorVersion() const
{
    return _imp->pyPlugInfo.pyPlugVersion;
}

int
Node::getNInputs() const
{
    ///MT-safe, never changes
    assert(_imp->effect);

    return _imp->effect->getNInputs();
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
    b->setValue( !b->getValue() );
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

bool
Node::message(MessageTypeEnum type,
              const std::string & content) const
{
    ///If the node was aborted, don't transmit any message because we could cause a deadlock
    if ( _imp->effect->aborted() || (!_imp->nodeCreated && _imp->wasCreatedSilently)) {
        std::cerr << getScriptName_mt_safe() << " message: " << content << std::endl;
        return false;
    }

    // See https://github.com/MrKepzie/Natron/issues/1313
    // Messages posted from a separate thread should be logged and not show a pop-up
    if ( QThread::currentThread() != qApp->thread() ) {

        LogEntry::LogEntryColor c;
        if (getColor(&c.r, &c.g, &c.b)) {
            c.colorSet = true;
        }
        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( getLabel_mt_safe().c_str() ), QDateTime::currentDateTime(), QString::fromUtf8( content.c_str() ), false, c);
        if (type == eMessageTypeError) {
            appPTR->showErrorLog();
        }
        return true;
    }

#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        ioContainer->message(type, content);
        return true;
    }
#endif
    // Some nodes may be hidden from the user but may still report errors (such that the group is in fact hidden to the user)
    if ( !isPartOfProject() && getGroup() ) {
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>( getGroup().get() );
        if (isGroup) {
            isGroup->message(type, content);
        }
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
    if (!_imp->nodeCreated && _imp->wasCreatedSilently) {
        std::cerr << getScriptName_mt_safe() << " message: " << content << std::endl;
        return;
    }

    if ( !appPTR->isBackground() ) {
        //if the message is just an information, display a popup instead.
#ifdef NATRON_ENABLE_IO_META_NODES
        NodePtr ioContainer = getIOContainer();
        if (ioContainer) {
            ioContainer->setPersistentMessage(type, content);

            return;
        }
#endif
        // Some nodes may be hidden from the user but may still report errors (such that the group is in fact hidden to the user)
        if ( !isPartOfProject() && getGroup() ) {
            NodeGroup* isGroup = dynamic_cast<NodeGroup*>( getGroup().get() );
            if (isGroup) {
                isGroup->setPersistentMessage(type, content);
            }
        }

        if (type == eMessageTypeInfo) {
            message(type, content);

            return;
        }

        {
            QMutexLocker k(&_imp->persistentMessageMutex);
            QString mess = QString::fromUtf8( content.c_str() );
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
#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        return ioContainer->hasPersistentMessage();
    }
#endif
    QMutexLocker k(&_imp->persistentMessageMutex);

    return !_imp->persistentMessage.isEmpty();
}

bool
Node::hasAnyPersistentMessage() const
{
    QMutexLocker k(&_imp->persistentMessageMutex);
    return !_imp->persistentMessage.isEmpty();
}

void
Node::getPersistentMessage(QString* message,
                           int* type,
                           bool prefixLabelAndType) const
{
#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        return ioContainer->getPersistentMessage(message, type, prefixLabelAndType);
    }
#endif
    QMutexLocker k(&_imp->persistentMessageMutex);

    *type = _imp->persistentMessageType;

    if ( prefixLabelAndType && !_imp->persistentMessage.isEmpty() ) {
        message->append( QString::fromUtf8( getLabel_mt_safe().c_str() ) );
        if (*type == eMessageTypeError) {
            message->append( QString::fromUtf8(" error: ") );
        } else if (*type == eMessageTypeWarning) {
            message->append( QString::fromUtf8(" warning: ") );
        }
    }
    message->append(_imp->persistentMessage);
}

void
Node::clearPersistentMessageRecursive(std::list<Node*>& markedNodes)
{
    if ( std::find(markedNodes.begin(), markedNodes.end(), this) != markedNodes.end() ) {
        return;
    }
    markedNodes.push_back(this);
    clearPersistentMessageInternal();

    int nInputs = getNInputs();
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
#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        ioContainer->clearPersistentMessageInternal();

        return;
    }
#endif
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
    AppInstancePtr app = getApp();

    if (!app) {
        return;
    }
    if ( app->isBackground() ) {
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
    if ( !getApp() || getApp()->isGuiFrozen() ) {
        return false;
    }

    timeval now;


    gettimeofday(&now, 0);

    QMutexLocker l(&_imp->lastRenderStartedMutex);
    double t =  now.tv_sec  - _imp->lastInputNRenderStartedSlotCallTime.tv_sec +
               (now.tv_usec - _imp->lastInputNRenderStartedSlotCallTime.tv_usec) * 1e-6f;

    assert( inputNb >= 0 && inputNb < (int)_imp->inputIsRenderingCounter.size() );

    if ( (t > NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS) || (_imp->inputIsRenderingCounter[inputNb] == 0) ) {
        _imp->lastInputNRenderStartedSlotCallTime = now;
        ++_imp->inputIsRenderingCounter[inputNb];

        l.unlock();

        Q_EMIT inputNIsRendering(inputNb);

        return true;
    }

    return false;
}

void
Node::notifyInputNIsFinishedRendering(int inputNb)
{
    {
        QMutexLocker l(&_imp->lastRenderStartedMutex);
        assert( inputNb >= 0 && inputNb < (int)_imp->inputIsRenderingCounter.size() );
        --_imp->inputIsRenderingCounter[inputNb];
    }
    Q_EMIT inputNIsFinishedRendering(inputNb);
}

bool
Node::notifyRenderingStarted()
{
    if ( !getApp() || getApp()->isGuiFrozen() ) {
        return false;
    }

    timeval now;

    gettimeofday(&now, 0);


    QMutexLocker l(&_imp->lastRenderStartedMutex);
    double t =  now.tv_sec  - _imp->lastRenderStartedSlotCallTime.tv_sec +
               (now.tv_usec - _imp->lastRenderStartedSlotCallTime.tv_usec) * 1e-6f;

    if ( (t > NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS) || (_imp->renderStartedCounter == 0) ) {
        _imp->lastRenderStartedSlotCallTime = now;
        ++_imp->renderStartedCounter;


        l.unlock();

        Q_EMIT renderingStarted();

        return true;
    }

    return false;
}

void
Node::notifyRenderingEnded()
{
    {
        QMutexLocker l(&_imp->lastRenderStartedMutex);
        --_imp->renderStartedCounter;
    }
    Q_EMIT renderingEnded();
}

int
Node::getIsInputNRenderingCounter(int inputNb) const
{
    QMutexLocker l(&_imp->lastRenderStartedMutex);

    assert( inputNb >= 0 && inputNb < (int)_imp->inputIsRenderingCounter.size() );

    return _imp->inputIsRenderingCounter[inputNb];
}

int
Node::getIsNodeRenderingCounter() const
{
    QMutexLocker l(&_imp->lastRenderStartedMutex);

    return _imp->renderStartedCounter;
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

static void
refreshPreviewsRecursivelyUpstreamInternal(double time,
                                           Node* node,
                                           std::list<Node*>& marked)
{
    if ( std::find(marked.begin(), marked.end(), node) != marked.end() ) {
        return;
    }

    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }

    marked.push_back(node);

    std::vector<NodeWPtr> inputs = node->getInputs_copy();

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

    refreshPreviewsRecursivelyUpstreamInternal(time, this, marked);
}

static void
refreshPreviewsRecursivelyDownstreamInternal(double time,
                                             Node* node,
                                             std::list<Node*>& marked)
{
    if ( std::find(marked.begin(), marked.end(), node) != marked.end() ) {
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
    if ( !getNodeGui() ) {
        return;
    }
    std::list<Node*> marked;
    refreshPreviewsRecursivelyDownstreamInternal(time, this, marked);
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
        if (effect) {
            NodePtr masterNode = effect->getNode();
            {
                QMutexLocker l(&_imp->masterNodeMutex);
                _imp->masterNode = masterNode;
            }
            QObject::connect( masterNode.get(), SIGNAL(deactivated(bool)), this, SLOT(onMasterNodeDeactivated()) );
            QObject::connect( masterNode.get(), SIGNAL(knobsAgeChanged(U64)), this, SLOT(setKnobsAge(U64)) );
            QObject::connect( masterNode.get(), SIGNAL(previewImageChanged(double)), this, SLOT(refreshPreviewImage(double)) );
        }
    } else {
        NodePtr master = getMasterNode();
        QObject::disconnect( master.get(), SIGNAL(deactivated(bool)), this, SLOT(onMasterNodeDeactivated()) );
        QObject::disconnect( master.get(), SIGNAL(knobsAgeChanged(U64)), this, SLOT(setKnobsAge(U64)) );
        QObject::disconnect( master.get(), SIGNAL(previewImageChanged(double)), this, SLOT(refreshPreviewImage(double)) );
        {
            QMutexLocker l(&_imp->masterNodeMutex);
            _imp->masterNode.reset();
        }
    }

    Q_EMIT allKnobsSlaved(isSlave);
}

void
Node::onKnobSlaved(const KnobIPtr& slave,
                   const KnobIPtr& master,
                   int dimension,
                   bool isSlave)
{
    ///ignore the call if the node is a clone
    {
        QMutexLocker l(&_imp->masterNodeMutex);
        if ( _imp->masterNode.lock() ) {
            return;
        }
    }

    assert( master->getHolder() );


    ///If the holder isn't an effect, ignore it too
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>( master->getHolder() );
    NodePtr parentNode;
    if (!isEffect) {
        TrackMarker* isMarker = dynamic_cast<TrackMarker*>( master->getHolder() );
        if (isMarker) {
            parentNode = isMarker->getContext()->getNode();
        }
    } else {
        parentNode = isEffect->getNode();
    }

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

#ifdef NATRON_ENABLE_IO_META_NODES
NodePtr
Node::getIOContainer() const
{
    return _imp->ioContainer.lock();
}

#endif

NodePtr
Node::getMasterNode() const
{
    QMutexLocker l(&_imp->masterNodeMutex);

    return _imp->masterNode.lock();
}

bool
Node::isSupportedComponent(int inputNb,
                           const ImagePlaneDesc& comp) const
{
    QMutexLocker l(&_imp->inputsMutex);

    if (inputNb >= 0) {
        assert( inputNb < (int)_imp->inputsComponents.size() );
        std::list<ImagePlaneDesc>::const_iterator found =
            std::find(_imp->inputsComponents[inputNb].begin(), _imp->inputsComponents[inputNb].end(), comp);

        return found != _imp->inputsComponents[inputNb].end();
    } else {
        assert(inputNb == -1);
        std::list<ImagePlaneDesc>::const_iterator found =
            std::find(_imp->outputComponents.begin(), _imp->outputComponents.end(), comp);

        return found != _imp->outputComponents.end();
    }
}

ImagePlaneDesc
Node::findClosestInList(const ImagePlaneDesc& comp,
                        const std::list<ImagePlaneDesc> &components,
                        bool multiPlanar)
{
    if ( components.empty() ) {
        return ImagePlaneDesc::getNoneComponents();
    }
    std::list<ImagePlaneDesc>::const_iterator closestComp = components.end();
    for (std::list<ImagePlaneDesc>::const_iterator it = components.begin(); it != components.end(); ++it) {
        if ( closestComp == components.end() ) {
            if ( multiPlanar && ( it->getNumComponents() == comp.getNumComponents() ) ) {
                return comp;
            }
            closestComp = it;
        } else {
            if ( it->getNumComponents() == comp.getNumComponents() ) {
                if (multiPlanar) {
                    return comp;
                }
                closestComp = it;
                break;
            } else {
                int diff = it->getNumComponents() - comp.getNumComponents();
                int diffSoFar = closestComp->getNumComponents() - comp.getNumComponents();
                if ( (diff > 0) && (diff < diffSoFar) ) {
                    closestComp = it;
                }
            }
        }
    }
    if ( closestComp == components.end() ) {
        return ImagePlaneDesc::getNoneComponents();
    }

    return *closestComp;
}

ImagePlaneDesc
Node::findClosestSupportedComponents(int inputNb,
                                     const ImagePlaneDesc& comp) const
{
    std::list<ImagePlaneDesc> comps;
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

    return findClosestInList( comp, comps, _imp->effect->isMultiPlanar() );
}

int
Node::isMaskChannelKnob(const KnobI* knob) const
{
    for (std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        if (it->second.channel.lock().get() == knob) {
            return it->first;
        }
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
        _imp->imagesBeingRenderedCond.wait(&_imp->imagesBeingRenderedMutex);
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
    _imp->imagesBeingRenderedCond.wakeAll();
}

boost::shared_ptr<Image>
Node::getImageBeingRendered(double time,
                            unsigned int mipMapLevel,
                            ViewIdx view)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);

    for (std::list<boost::shared_ptr<Image> >::iterator it = _imp->imagesBeingRendered.begin();
         it != _imp->imagesBeingRendered.end(); ++it) {
        const ImageKey &key = (*it)->getKey();
        if ( (key._view == view) && ( (*it)->getMipMapLevel() == mipMapLevel ) && (key._time == time) ) {
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
            if ( !getApp()->isCreatingNodeTree() ) {
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
Node::onInputChanged(int inputNb,
                     bool isInputA)
{
    if ( getApp()->getProject()->isProjectClosing() ) {
        return;
    }
    assert( QThread::currentThread() == qApp->thread() );

    bool mustCallEndInputEdition = _imp->inputModifiedRecursion == 0;
    if (mustCallEndInputEdition) {
        beginInputEdition();
    }

    refreshMaskEnabledNess(inputNb);
    refreshLayersChoiceSecretness(inputNb);

    InspectorNode* isInspector = dynamic_cast<InspectorNode*>(this);
    if (isInspector) {
        isInspector->refreshActiveInputs(inputNb, isInputA);
    }

    bool shouldDoInputChanged = ( !getApp()->getProject()->isProjectClosing() && !getApp()->isCreatingNodeTree() ) ||
                                _imp->effect->isRotoPaintNode();

    if (shouldDoInputChanged) {
        ///When loading a group (or project) just wait until everything is setup to actually compute input
        ///related data such as clip preferences
        ///Exception for the Rotopaint node which needs to setup its own graph internally

        /**
         * The plug-in might call getImage, set a valid thread storage on the tree.
         **/
        double time = getApp()->getTimeLine()->currentFrame();
        AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
        const bool isRenderUserInteraction = true;
        const bool isSequentialRender = false;
        AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( QThread::currentThread() );
        if (isAbortable) {
            isAbortable->setAbortInfo( isRenderUserInteraction, abortInfo, getEffectInstance() );
        }
        ParallelRenderArgsSetter frameRenderArgs( time,
                                                  ViewIdx(0),
                                                  isRenderUserInteraction,
                                                  isSequentialRender,
                                                  abortInfo,
                                                  shared_from_this(),
                                                  0, //texture index
                                                  getApp()->getTimeLine().get(),
                                                  NodePtr(),
                                                  false,
                                                  false,
                                                  boost::shared_ptr<RenderStats>() );


        ///Don't do clip preferences while loading a project, they will be refreshed globally once the project is loaded.
        _imp->effect->onInputChanged(inputNb);
        _imp->inputsModified.insert(inputNb);

        // If the effect has render clones, kill them as the plug-in might have changed its internal state
        _imp->effect->clearRenderInstances();

        //A knob value might have changed recursively, redraw  any overlay
        if ( !_imp->effect->isDequeueingValuesSet() &&
             ( _imp->effect->getRecursionLevel() == 0) && _imp->effect->checkIfOverlayRedrawNeeded() ) {
            _imp->effect->redrawOverlayInteract();
        }
    }

    /*
       If this is a group, also notify the output nodes of the GroupInput node inside the Group corresponding to
       the this inputNb
     */
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGroup) {
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs, false);
        if ( (inputNb >= 0) && ( inputNb < (int)groupInputs.size() ) && groupInputs[inputNb] ) {
            std::map<NodePtr, int> inputOutputs;
            groupInputs[inputNb]->getOutputsConnectedToThisNode(&inputOutputs);
            for (std::map<NodePtr, int> ::iterator it = inputOutputs.begin(); it != inputOutputs.end(); ++it) {
                it->first->onInputChanged(it->second);
            }
        }
    }

    /*
       If this is an output node, notify the Group output nodes that their input have changed.
     */
    GroupOutput* isOutput = dynamic_cast<GroupOutput*>( _imp->effect.get() );
    if (isOutput) {
        NodeGroup* containerGroup = dynamic_cast<NodeGroup*>( isOutput->getNode()->getGroup().get() );
        if (containerGroup) {
            std::map<NodePtr, int> groupOutputs;
            containerGroup->getNode()->getOutputsConnectedToThisNode(&groupOutputs);
            for (std::map<NodePtr, int> ::iterator it = groupOutputs.begin(); it != groupOutputs.end(); ++it) {
                it->first->onInputChanged(it->second);
            }
        }
    }

    /*
     * If this node is the output of a pre-comp, notify the precomp output nodes that their input have changed
     */
    boost::shared_ptr<PrecompNode> isInPrecomp = isPartOfPrecomp();
    if ( isInPrecomp && (isInPrecomp->getOutputNode().get() == this) ) {
        std::map<NodePtr, int> inputOutputs;
        isInPrecomp->getNode()->getOutputsConnectedToThisNode(&inputOutputs);
        for (std::map<NodePtr, int> ::iterator it = inputOutputs.begin(); it != inputOutputs.end(); ++it) {
            it->first->onInputChanged(it->second);
        }
    }

    if (mustCallEndInputEdition) {
        endInputEdition(true);
    }
} // Node::onInputChanged

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
Node::onFileNameParameterChanged(KnobI* fileKnob)
{
    if ( _imp->effect->isReader() ) {
        ///Refresh the preview automatically if the filename changed
        incrementKnobsAge(); //< since evaluate() is called after knobChanged we have to do this  by hand
        //computePreviewImage( getApp()->getTimeLine()->currentFrame() );

        ///union the project frame range if not locked with the reader frame range
        bool isLocked = getApp()->getProject()->isFrameRangeLocked();
        if (!isLocked) {
            double leftBound = INT_MIN, rightBound = INT_MAX;
            _imp->effect->getFrameRange_public(getHashValue(), &leftBound, &rightBound, true);

            if ( (leftBound != INT_MIN) && (rightBound != INT_MAX) ) {
                if ( getGroup() || getIOContainer() ) {
                    getApp()->getProject()->unionFrameRangeWith(leftBound, rightBound);
                }
            }
        }
    } else if ( _imp->effect->isWriter() ) {
        KnobIPtr sublabelKnob = getKnobByName(kNatronOfxParamStringSublabelName);
        KnobOutputFile* isFile = dynamic_cast<KnobOutputFile*>(fileKnob);
        if (isFile && sublabelKnob) {
            KnobStringBase* isString = dynamic_cast<KnobStringBase*>( sublabelKnob.get() );

            std::string pattern = isFile->getValue();
            if (isString) {
                std::size_t foundSlash = pattern.find_last_of("/");
                if (foundSlash != std::string::npos) {
                    pattern = pattern.substr(foundSlash + 1);
                }

                isString->setValue(pattern);
            }
        }

        /*
           Check if the filename param has a %V in it, in which case make sure to hide the Views parameter
         */
        KnobOutputFile* fileParam = dynamic_cast<KnobOutputFile*>(fileKnob);
        if (fileParam) {
            std::string pattern = fileParam->getValue();
            std::size_t foundViewPattern = pattern.find_first_of("%v");
            if (foundViewPattern == std::string::npos) {
                foundViewPattern = pattern.find_first_of("%V");
            }
            if (foundViewPattern != std::string::npos) {
                //We found view pattern
                KnobIPtr viewsKnob = getKnobByName(kWriteOIIOParamViewsSelector);
                if (viewsKnob) {
                    KnobChoice* viewsSelector = dynamic_cast<KnobChoice*>( viewsKnob.get() );
                    if (viewsSelector) {
                        viewsSelector->setSecret(true);
                    }
                }
            }
        }
    }
} // Node::onFileNameParameterChanged

void
Node::getOriginalFrameRangeForReader(const std::string& pluginID,
                                     const std::string& canonicalFileName,
                                     int* firstFrame,
                                     int* lastFrame)
{
    if ( ReadNode::isVideoReader(pluginID) ) {
        ///If the plug-in is a video, only ffmpeg may know how many frames there are
        *firstFrame = INT_MIN;
        *lastFrame = INT_MAX;
    } else {
        SequenceParsing::SequenceFromPattern seq;
        FileSystemModel::filesListFromPattern(canonicalFileName, &seq);
        if ( seq.empty() || (seq.size() == 1) ) {
            *firstFrame = 1;
            *lastFrame = 1;
        } else if (seq.size() > 1) {
            *firstFrame = seq.begin()->first;
            *lastFrame = seq.rbegin()->first;
        }
    }
}

void
Node::computeFrameRangeForReader(KnobI* fileKnob)
{
    /*
       We compute the original frame range of the sequence for the plug-in
       because the plug-in does not have access to the exact original pattern
       hence may not exactly end-up with the same file sequence as what the user
       selected from the file dialog.
     */
    ReadNode* isReadNode = dynamic_cast<ReadNode*>( _imp->effect.get() );
    std::string pluginID;

    if (isReadNode) {
        NodePtr embeddedPlugin = isReadNode->getEmbeddedReader();
        if (embeddedPlugin) {
            pluginID = embeddedPlugin->getPluginID();
        }
    } else {
        pluginID = getPluginID();
    }

    int leftBound = INT_MIN;
    int rightBound = INT_MAX;
    ///Set the originalFrameRange parameter of the reader if it has one.
    KnobIPtr knob = getKnobByName(kReaderParamNameOriginalFrameRange);
    if (knob) {
        KnobInt* originalFrameRange = dynamic_cast<KnobInt*>( knob.get() );
        if ( originalFrameRange && (originalFrameRange->getDimension() == 2) ) {
            KnobFile* isFile = dynamic_cast<KnobFile*>(fileKnob);
            assert(isFile);
            if (!isFile) {
                throw std::logic_error("Node::computeFrameRangeForReader");
            }

            if ( ReadNode::isVideoReader(pluginID) ) {
                ///If the plug-in is a video, only ffmpeg may know how many frames there are
                originalFrameRange->setValues(INT_MIN, INT_MAX, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
            } else {
                std::string pattern = isFile->getValue();
                getApp()->getProject()->canonicalizePath(pattern);
                SequenceParsing::SequenceFromPattern seq;
                FileSystemModel::filesListFromPattern(pattern, &seq);
                if ( seq.empty() || (seq.size() == 1) ) {
                    leftBound = 1;
                    rightBound = 1;
                } else if (seq.size() > 1) {
                    leftBound = seq.begin()->first;
                    rightBound = seq.rbegin()->first;
                }
                originalFrameRange->setValues(leftBound, rightBound, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
            }
        }
    }
}

bool
Node::canHandleRenderScaleForOverlays() const
{
    return _imp->effect->canHandleRenderScaleForOverlays();
}

bool
Node::getOverlayColor(double* r,
                      double* g,
                      double* b) const
{
    NodeGuiIPtr gui_i = getNodeGui();

    if (!gui_i) {
        return false;
    }

    return gui_i->getOverlayColor(r, g, b);
}

bool
Node::shouldDrawOverlay() const
{
    if ( !hasOverlay() && !getRotoContext() ) {
        return false;
    }

    if (!_imp->isPartOfProject) {
        return false;
    }

    if ( !isActivated() ) {
        return false;
    }

    if ( isNodeDisabled() ) {
        return false;
    }

    if ( getParentMultiInstance() ) {
        return false;
    }

    if ( !isSettingsPanelVisible() ) {
        return false;
    }

    if ( isSettingsPanelMinimized() ) {
        return false;
    }


    return true;
}

void
Node::drawHostOverlay(double time,
                      const RenderScale& renderScale,
                      ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->drawHostOverlay(time, renderScale, view);
    }
}

bool
Node::onOverlayPenDownDefault(double time,
                              const RenderScale& renderScale,
                              ViewIdx view,
                              const QPointF & viewportPos,
                              const QPointF & pos,
                              double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenDownDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}

bool
Node::onOverlayPenDoubleClickedDefault(double time,
                                       const RenderScale& renderScale,
                                       ViewIdx view,
                                       const QPointF & viewportPos,
                                       const QPointF & pos)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenDoubleClickedDefault(time, renderScale, view, viewportPos, pos);
    }

    return false;
}

bool
Node::onOverlayPenMotionDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenMotionDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}

bool
Node::onOverlayPenUpDefault(double time,
                            const RenderScale& renderScale,
                            ViewIdx view,
                            const QPointF & viewportPos,
                            const QPointF & pos,
                            double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenUpDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}

bool
Node::onOverlayKeyDownDefault(double time,
                              const RenderScale& renderScale,
                              ViewIdx view,
                              Key key,
                              KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyDownDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}

bool
Node::onOverlayKeyUpDefault(double time,
                            const RenderScale& renderScale,
                            ViewIdx view,
                            Key key,
                            KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyUpDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}

bool
Node::onOverlayKeyRepeatDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view,
                                Key key,
                                KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyRepeatDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}

bool
Node::onOverlayFocusGainedDefault(double time,
                                  const RenderScale& renderScale,
                                  ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayFocusGainedDefault(time, renderScale, view);
    }

    return false;
}

bool
Node::onOverlayFocusLostDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayFocusLostDefault(time, renderScale, view);
    }

    return false;
}

void
Node::removePositionHostOverlay(KnobI* knob)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->removePositionHostOverlay(knob);
    }
}

void
Node::addPositionInteract(const boost::shared_ptr<KnobDouble>& position,
                          const boost::shared_ptr<KnobBool>& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }

    boost::shared_ptr<HostOverlayKnobsPosition> knobs = boost::make_shared<HostOverlayKnobsPosition>();
    knobs->addKnob(position, HostOverlayKnobsPosition::eKnobsEnumerationPosition);
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsPosition::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
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
                           const boost::shared_ptr<KnobDouble>& center,
                           const boost::shared_ptr<KnobBool>& invert,
                           const boost::shared_ptr<KnobBool>& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }

    boost::shared_ptr<HostOverlayKnobsTransform> knobs = boost::make_shared<HostOverlayKnobsTransform>();
    knobs->addKnob(translate, HostOverlayKnobsTransform::eKnobsEnumerationTranslate);
    knobs->addKnob(scale, HostOverlayKnobsTransform::eKnobsEnumerationScale);
    knobs->addKnob(scaleUniform, HostOverlayKnobsTransform::eKnobsEnumerationUniform);
    knobs->addKnob(rotate, HostOverlayKnobsTransform::eKnobsEnumerationRotate);
    knobs->addKnob(center, HostOverlayKnobsTransform::eKnobsEnumerationCenter);
    knobs->addKnob(skewX, HostOverlayKnobsTransform::eKnobsEnumerationSkewx);
    knobs->addKnob(skewY, HostOverlayKnobsTransform::eKnobsEnumerationSkewy);
    knobs->addKnob(skewOrder, HostOverlayKnobsTransform::eKnobsEnumerationSkewOrder);
    if (invert) {
        knobs->addKnob(invert, HostOverlayKnobsTransform::eKnobsEnumerationInvert);
    }
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsTransform::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
    }
}

void
Node::addCornerPinInteract(const boost::shared_ptr<KnobDouble>& from1,
                           const boost::shared_ptr<KnobDouble>& from2,
                           const boost::shared_ptr<KnobDouble>& from3,
                           const boost::shared_ptr<KnobDouble>& from4,
                           const boost::shared_ptr<KnobDouble>& to1,
                           const boost::shared_ptr<KnobDouble>& to2,
                           const boost::shared_ptr<KnobDouble>& to3,
                           const boost::shared_ptr<KnobDouble>& to4,
                           const boost::shared_ptr<KnobBool>& enable1,
                           const boost::shared_ptr<KnobBool>& enable2,
                           const boost::shared_ptr<KnobBool>& enable3,
                           const boost::shared_ptr<KnobBool>& enable4,
                           const boost::shared_ptr<KnobChoice>& overlayPoints,
                           const boost::shared_ptr<KnobBool>& invert,
                           const boost::shared_ptr<KnobBool>& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }
    boost::shared_ptr<HostOverlayKnobsCornerPin> knobs = boost::make_shared<HostOverlayKnobsCornerPin>();
    knobs->addKnob(from1, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom1);
    knobs->addKnob(from2, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom2);
    knobs->addKnob(from3, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom3);
    knobs->addKnob(from4, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom4);

    knobs->addKnob(to1, HostOverlayKnobsCornerPin::eKnobsEnumerationTo1);
    knobs->addKnob(to2, HostOverlayKnobsCornerPin::eKnobsEnumerationTo2);
    knobs->addKnob(to3, HostOverlayKnobsCornerPin::eKnobsEnumerationTo3);
    knobs->addKnob(to4, HostOverlayKnobsCornerPin::eKnobsEnumerationTo4);

    knobs->addKnob(enable1, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable1);
    knobs->addKnob(enable2, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable2);
    knobs->addKnob(enable3, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable3);
    knobs->addKnob(enable4, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable4);

    knobs->addKnob(overlayPoints, HostOverlayKnobsCornerPin::eKnobsEnumerationOverlayPoints);

    if (invert) {
        knobs->addKnob(invert, HostOverlayKnobsCornerPin::eKnobsEnumerationInvert);
    }
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsCornerPin::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
    }
}

void
Node::initializeHostOverlays()
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (!nodeGui) {
        return;
    }
    for (std::list<boost::shared_ptr<HostOverlayKnobs> > ::iterator it = _imp->nativeOverlays.begin(); it != _imp->nativeOverlays.end(); ++it) {
        nodeGui->addDefaultInteract(*it);
    }
    _imp->nativeOverlays.clear();
}

void
Node::setPluginIDAndVersionForGui(const std::list<std::string>& grouping,
                                  const std::string& pluginLabel,
                                  const std::string& pluginID,
                                  const std::string& pluginDesc,
                                  const std::string& pluginIconFilePath,
                                  unsigned int version)
{
    assert( QThread::currentThread() == qApp->thread() );
    NodeGuiIPtr nodeGui = getNodeGui();

    setPyPlugEdited(false);
    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);
        _imp->pyPlugInfo.pyPlugVersion = version;
        _imp->pyPlugInfo.pyPlugID = pluginID;
        _imp->pyPlugInfo.pyPlugDesc = pluginDesc;
        _imp->pyPlugInfo.pyPlugLabel = pluginLabel;
        _imp->pyPlugInfo.pyPlugGrouping = grouping;
        _imp->pyPlugInfo.pyPlugIconFilePath = pluginIconFilePath;
    }

    if (!nodeGui) {
        return;
    }

    nodeGui->setPluginIDAndVersion(grouping, pluginLabel, pluginID, pluginDesc, pluginIconFilePath, version);
}

bool
Node::hasPyPlugBeenEdited() const
{
    QMutexLocker k(&_imp->pyPluginInfoMutex);

    return !_imp->pyPlugInfo.isPyPlug || _imp->pyPlugInfo.pluginPythonModule.empty();
}

void
Node::setPyPlugEdited(bool edited)
{
    {
        QMutexLocker k(&_imp->pyPluginInfoMutex);

        _imp->pyPlugInfo.isPyPlug = !edited;
    }
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(_imp->effect.get());
    if (isGroup) {
        boost::shared_ptr<KnobButton> convertToGroupButton = isGroup->getConvertToGroupButton();
        boost::shared_ptr<KnobButton> exportPyPlugButton = isGroup->getExportAsPyPlugButton();
        if (convertToGroupButton) {
            convertToGroupButton->setSecret(edited);
        }
        if (exportPyPlugButton) {
            exportPyPlugButton->setSecret(!edited);
        }

    }
}

bool
Node::isPyPlug() const
{
    return _imp->pyPlugInfo.isPyPlug;
}

void
Node::setPluginPythonModule(const std::string& pythonModule)
{
    QMutexLocker k(&_imp->pyPluginInfoMutex);

    _imp->pyPlugInfo.pluginPythonModule = pythonModule;
}

std::string
Node::getPluginPythonModule() const
{
    QMutexLocker k(&_imp->pyPluginInfoMutex);

    return _imp->pyPlugInfo.pluginPythonModule;
}

bool
Node::hasHostOverlayForParam(const KnobI* knob) const
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlayForParam(knob) ) {
        return true;
    }

    return false;
}

bool
Node::hasHostOverlay() const
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlay() ) {
        return true;
    }

    return false;
}

void
Node::pushUndoCommand(const UndoCommandPtr& command)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->pushUndoCommand(command);
    } else {
        command->redo();
    }
}

void
Node::setCurrentCursor(CursorEnum defaultCursor)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->setCurrentCursor(defaultCursor);
    }
}

bool
Node::setCurrentCursor(const QString& customCursorFilePath)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->setCurrentCursor(customCursorFilePath);
    }

    return false;
}

void
Node::setCurrentViewportForHostOverlays(OverlaySupport* viewPort)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlay() ) {
        nodeGui->setCurrentViewportForHostOverlays(viewPort);
    }
}

const std::vector<std::string>&
Node::getCreatedViews() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->createdViews;
}

void
Node::refreshCreatedViews(bool silent)
{
    KnobIPtr knob = getKnobByName(kReadOIIOAvailableViewsKnobName);

    if (knob) {
        refreshCreatedViews( knob.get(), silent );
    }
}

void
Node::refreshCreatedViews(KnobI* knob, bool silent)
{
    assert( QThread::currentThread() == qApp->thread() );

    KnobString* availableViewsKnob = dynamic_cast<KnobString*>(knob);
    if (!availableViewsKnob) {
        return;
    }
    QString value = QString::fromUtf8( availableViewsKnob->getValue().c_str() );
    QStringList views = value.split( QLatin1Char(',') );

    _imp->createdViews.clear();

    const std::vector<std::string>& projectViews = getApp()->getProject()->getProjectViewNames();
    QStringList qProjectViews;
    for (std::size_t i = 0; i < projectViews.size(); ++i) {
        qProjectViews.push_back( QString::fromUtf8( projectViews[i].c_str() ) );
    }

    QStringList missingViews;
    for (QStringList::Iterator it = views.begin(); it != views.end(); ++it) {
        if ( !qProjectViews.contains(*it, Qt::CaseInsensitive) && !it->isEmpty() ) {
            missingViews.push_back(*it);
        }
        _imp->createdViews.push_back( it->toStdString() );
    }

    if ( !missingViews.isEmpty() && !silent ) {
        KnobIPtr fileKnob = getKnobByName(kOfxImageEffectFileParamName);
        KnobFile* inputFileKnob = dynamic_cast<KnobFile*>( fileKnob.get() );
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
            ss << tr("These views are in %1 but do not exist in the project.\nWould you like to create them?").arg( QString::fromUtf8( filename.c_str() ) ).toStdString();
            std::string question  = ss.str();
            StandardButtonEnum rep = Dialogs::questionDialog(tr("Views available").toStdString(), question, false, StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonYes);
            if (rep == eStandardButtonYes) {
                std::vector<std::string> viewsToCreate;
                for (QStringList::Iterator it = missingViews.begin(); it != missingViews.end(); ++it) {
                    viewsToCreate.push_back( it->toStdString() );
                }
                getApp()->getProject()->createProjectViews(viewsToCreate);
            }
        }
    }

    Q_EMIT availableViewsChanged();
} // Node::refreshCreatedViews

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
    k->setValue(hidden);
}

void
Node::onRefreshIdentityStateRequestReceived()
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( (_imp->refreshIdentityStateRequestsCount == 0) || !_imp->effect ) {
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
    // Do not test identity on the project format! intermediate images may be much bigger!
    // see https://github.com/MrKepzie/Natron/issues/1578#issuecomment-377640892
    //Format frmt;
    //project->getProjectDefaultFormat(&frmt);
    RectI frmt;
    frmt.x1 = frmt.y1 = kOfxFlagInfiniteMin;
    frmt.x2 = frmt.y2 = kOfxFlagInfiniteMax;

    //The one view node might report it is identity, but we do not want it to display it


    bool isIdentity = false;
    int inputNb = -1;
    OneViewNode* isOneView = dynamic_cast<OneViewNode*>( _imp->effect.get() );
    if (!isOneView) {
        for (int i = 0; i < nViews; ++i) {
            int identityInputNb = -1;
            ViewIdx identityView;
            bool isViewIdentity = _imp->effect->isIdentity_public(true, hash, time, scale, frmt, ViewIdx(i), &inputTime, &identityView, &identityInputNb);
            if ( (i > 0) && ( (isViewIdentity != isIdentity) || (identityInputNb != inputNb) || (identityView.value() != i) ) ) {
                isIdentity = false;
                inputNb = -1;
                break;
            }
            isIdentity |= isViewIdentity;
            inputNb = identityInputNb;
            if (!isIdentity) {
                break;
            }
        }
    }

    //Check for consistency across views or then say the effect is not identity since the UI cannot display 2 different states
    //depending on the view


    NodeGuiIPtr nodeUi = _imp->guiPointer.lock();
    assert(nodeUi);
    nodeUi->onIdentityStateChanged(isIdentity ? inputNb : -1);
} // Node::onRefreshIdentityStateRequestReceived

void
Node::refreshIdentityState()
{
    assert( QThread::currentThread() == qApp->thread() );

    if ( !_imp->guiPointer.lock() ) {
        return;
    }

    //Post a new request
    ++_imp->refreshIdentityStateRequestsCount;
    Q_EMIT refreshIdentityStateRequested();
}

/*
   This is called AFTER the instanceChanged action has been called on the plug-in
 */
bool
Node::onEffectKnobValueChanged(KnobI* what,
                               ValueChangedReasonEnum reason)
{
    if (!what) {
        return false;
    }
    for (std::map<int, MaskSelector >::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        if (it->second.channel.lock().get() == what) {
            _imp->onMaskSelectorChanged(it->first, it->second);
            break;
        }
    }

    bool ret = true;
    if ( what == _imp->previewEnabledKnob.lock().get() ) {
        if ( (reason == eValueChangedReasonUserEdited) || (reason == eValueChangedReasonSlaveRefresh) ) {
            Q_EMIT previewKnobToggled();
        }
    } else if ( what == _imp->renderButton.lock().get() ) {
        if ( getEffectInstance()->isWriter() ) {
            /*if this is a button and it is a render button,we're safe to assume the plug-ins wants to start rendering.*/
            AppInstance::RenderWork w;
            w.writer = dynamic_cast<OutputEffectInstance*>( _imp->effect.get() );
            w.firstFrame = INT_MIN;
            w.lastFrame = INT_MAX;
            w.frameStep = INT_MIN;
            w.useRenderStats = getApp()->isRenderStatsActionChecked();
            std::list<AppInstance::RenderWork> works;
            works.push_back(w);
            getApp()->startWritersRendering(false, works);
        }
    } else if ( ( what == _imp->disableNodeKnob.lock().get() ) && !_imp->isMultiInstance && !_imp->multiInstanceParent.lock() ) {
        Q_EMIT disabledKnobToggled( _imp->disableNodeKnob.lock()->getValue() );
        if ( QThread::currentThread() == qApp->thread() ) {
            getApp()->redrawAllViewers();
        }
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>( _imp->effect.get() );
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
        Q_EMIT nodeExtraLabelChanged( QString::fromUtf8( _imp->nodeLabelKnob.lock()->getValue().c_str() ) );
    } else if (what->getName() == kNatronOfxParamStringSublabelName) {
        //special hack for the merge node and others so we can retrieve the sublabel and display it in the node's label
        KnobString* strKnob = dynamic_cast<KnobString*>(what);
        if (strKnob) {
            QString operation = QString::fromUtf8( strKnob->getValue().c_str() );
            if ( !operation.isEmpty() ) {
                operation.prepend( QString::fromUtf8("(") );
                operation.append( QString::fromUtf8(")") );
            }
            replaceCustomDataInlabel(operation);
        }
    } else if ( what == _imp->hideInputs.lock().get() ) {
        Q_EMIT hideInputsKnobChanged( _imp->hideInputs.lock()->getValue() );
    } else if ( _imp->effect->isReader() && (what->getName() == kReadOIIOAvailableViewsKnobName) ) {
        refreshCreatedViews(what, false /*silent*/);
    } else if ( what == _imp->refreshInfoButton.lock().get() ||
               (what == _imp->infoPage.lock().get() && reason == eValueChangedReasonUserEdited) ) {
        std::stringstream ssinfo;
        int maxinputs = getNInputs();
        for (int i = 0; i < maxinputs; ++i) {
            std::string inputInfo = makeInfoForInput(i);
            if ( !inputInfo.empty() ) {
                ssinfo << inputInfo << "<br />";
            }
        }
        std::string outputInfo = makeInfoForInput(-1);
        ssinfo << outputInfo << "<br />";
        std::string cacheInfo = makeCacheInfo();
        ssinfo << cacheInfo << "<br />";
        ssinfo << "<b>" << tr("Supports tiles:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( getCurrentSupportTiles() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        if (_imp->effect) {
            ssinfo << "<b>" << tr("Supports multiresolution:").toStdString() << "</b> <font color=#c8c8c8>";
            ssinfo << ( _imp->effect->supportsMultiResolution() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
            ssinfo << "<b>" << tr("Supports renderscale:").toStdString() << "</b> <font color=#c8c8c8>";
            switch ( _imp->effect->supportsRenderScaleMaybe() ) {
                case EffectInstance::eSupportsMaybe:
                    ssinfo << tr("Maybe").toStdString();
                    break;

                case EffectInstance::eSupportsNo:
                    ssinfo << tr("No").toStdString();
                    break;

                case EffectInstance::eSupportsYes:
                    ssinfo << tr("Yes").toStdString();
                    break;
            }
            ssinfo << "</font><br />";
            ssinfo << "<b>" << tr("Supports multiple clip PARs:").toStdString() << "</b> <font color=#c8c8c8>";
            ssinfo << ( _imp->effect->supportsMultipleClipPARs() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
            ssinfo << "<b>" << tr("Supports multiple clip depths:").toStdString() << "</b> <font color=#c8c8c8>";
            ssinfo << ( _imp->effect->supportsMultipleClipDepths() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        }
        ssinfo << "<b>" << tr("Render thread safety:").toStdString() << "</b> <font color=#c8c8c8>";
        switch ( getCurrentRenderThreadSafety() ) {
            case eRenderSafetyUnsafe:
                ssinfo << tr("Unsafe").toStdString();
                break;

            case eRenderSafetyInstanceSafe:
                ssinfo << tr("Safe").toStdString();
                break;

            case eRenderSafetyFullySafe:
                ssinfo << tr("Fully safe").toStdString();
                break;

            case eRenderSafetyFullySafeFrame:
                ssinfo << tr("Fully safe frame").toStdString();
                break;
        }
        ssinfo << "</font><br />";
        ssinfo << "<b>" << tr("OpenGL Rendering Support:").toStdString() << "</b>: <font color=#c8c8c8>";
        PluginOpenGLRenderSupport glSupport = getCurrentOpenGLRenderSupport();
        switch (glSupport) {
            case ePluginOpenGLRenderSupportNone:
                ssinfo << tr("No").toStdString();
                break;
            case ePluginOpenGLRenderSupportNeeded:
                ssinfo << tr("Yes but CPU rendering is not supported").toStdString();
                break;
            case ePluginOpenGLRenderSupportYes:
                ssinfo << tr("Yes").toStdString();
                break;
            default:
                break;
        }
        ssinfo << "</font>";
        _imp->nodeInfos.lock()->setValue( ssinfo.str() );
    } else if ( what == _imp->openglRenderingEnabledKnob.lock().get() ) {
        bool enabled = true;
        int thisKnobIndex = _imp->openglRenderingEnabledKnob.lock()->getValue();
        if (thisKnobIndex == 1 || (thisKnobIndex == 2 && getApp()->isBackground())) {
            enabled = false;
        }
        if (enabled) {
            // Check value on project now
            if (!getApp()->getProject()->isOpenGLRenderActivated()) {
                enabled = false;
            }
        }
        _imp->effect->onEnableOpenGLKnobValueChanged(enabled);
    } else if (what == _imp->processAllLayersKnob.lock().get() ) {
        
        std::map<int, ChannelSelector>::iterator foundOutput = _imp->channelsSelectors.find(-1);
        if (foundOutput != _imp->channelsSelectors.end()) {
            _imp->onLayerChanged(foundOutput->first, foundOutput->second);
        }
    } else {
        ret = false;
    }

    if (!ret) {
        KnobGroup* isGroup = dynamic_cast<KnobGroup*>(what);
        if ( isGroup && isGroup->getIsDialog() ) {
            NodeGuiIPtr gui = getNodeGui();
            if (gui) {
                gui->showGroupKnobAsDialog(isGroup);
                ret = true;
            }
        }
    }

    if (!ret) {
        for (std::map<int, ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
            if (it->second.layer.lock().get() == what) {
                _imp->onLayerChanged(it->first, it->second);
                ret = true;
                break;
            }
        }
    }

    if (!ret) {
        for (int i = 0; i < 4; ++i) {
            boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
            if (!enabled) {
                break;
            }
            if (enabled.get() == what) {
                checkForPremultWarningAndCheckboxes();
                ret = true;
                break;
            }
        }
    }

    if (!ret) {
        GroupInput* isInput = dynamic_cast<GroupInput*>( _imp->effect.get() );
        if (isInput) {
            if ( (what->getName() == kNatronGroupInputIsOptionalParamName)
                 || ( what->getName() == kNatronGroupInputIsMaskParamName) ) {
                NodeCollectionPtr col = isInput->getNode()->getGroup();
                assert(col);
                NodeGroup* isGrp = dynamic_cast<NodeGroup*>( col.get() );
                assert(isGrp);
                if (isGrp) {
                    ///Refresh input arrows of the node to reflect the state
                    isGrp->getNode()->initializeInputs();
                    ret = true;
                }
            }
        }
    }

    return ret;
} // Node::onEffectKnobValueChanged

void
Node::onOpenGLEnabledKnobChangedOnProject(bool activated)
{
    bool enabled = activated;
    boost::shared_ptr<KnobChoice> k = _imp->openglRenderingEnabledKnob.lock();
    if (enabled) {
        if (k) {
            k->setAllDimensionsEnabled(true);
            int thisKnobIndex = k->getValue();
            if (thisKnobIndex == 1 || (thisKnobIndex == 2 && getApp()->isBackground())) {
                enabled = false;
            }
        }
    } else {
        if (k) {
            k->setAllDimensionsEnabled(true);
        }
    }
    _imp->effect->onEnableOpenGLKnobValueChanged(enabled);
    
}

bool
Node::getSelectedLayerChoiceRaw(int inputNb,
                                std::string& layer) const
{
    std::map<int, ChannelSelector>::iterator found = _imp->channelsSelectors.find(inputNb);

    if ( found == _imp->channelsSelectors.end() ) {
        return false;
    }
    boost::shared_ptr<KnobChoice> layerKnob = found->second.layer.lock();
    layer = layerKnob->getActiveEntry().label;

    return true;
}

ImagePlaneDesc
Node::Implementation::getSelectedLayerInternal(int inputNb,
                                               const std::list<ImagePlaneDesc>& availableLayers,
                                               const ChannelSelector& selector) const
{
    NodePtr node;

    assert(_publicInterface);
    if (!_publicInterface) {
        return ImagePlaneDesc();
    }
    if (inputNb == -1) {
        node = _publicInterface->shared_from_this();
    } else {
        node = _publicInterface->getInput(inputNb);
    }

    boost::shared_ptr<KnobChoice> layerKnob = selector.layer.lock();
    if (!layerKnob) {
        return ImagePlaneDesc();
    }
    ChoiceOption layerID = layerKnob->getActiveEntry();

    for (std::list<ImagePlaneDesc>::const_iterator it2 = availableLayers.begin(); it2 != availableLayers.end(); ++it2) {

        const std::string& layerName = it2->getPlaneID();
        if (layerID.id == layerName) {
            return *it2;
        }
    }
    return ImagePlaneDesc();
} // Node::Implementation::getSelectedLayerInternal

void
Node::Implementation::onLayerChanged(int inputNb,
                                     const ChannelSelector& selector)
{
    boost::shared_ptr<KnobChoice> layerKnob = selector.layer.lock();
    bool outputIsAll = processAllLayersKnob.lock()->getValue();

    if (inputNb == -1) {

        ///Disable all input selectors as it doesn't make sense to edit them whilst output is All
        for (std::map<int, ChannelSelector>::iterator it = channelsSelectors.begin(); it != channelsSelectors.end(); ++it) {
            
            NodePtr inp;
            if (it->first >= 0) {
                inp = _publicInterface->getInput(it->first);
            }
            bool mustBeSecret = (it->first >= 0 && !inp.get()) || outputIsAll;
            it->second.layer.lock()->setSecret(mustBeSecret);
            
        }
    }
    if (!isRefreshingInputRelatedData) {
        ///Clip preferences have changed
        RenderScale s(1.);
        effect->refreshMetadata_public(true);
    }
    if ( !enabledChan[0].lock() ) {
        return;
    }

    if (inputNb == -1) {
        if (outputIsAll) {
            for (int i = 0; i < 4; ++i) {
                enabledChan[i].lock()->setSecret(true);
            }
        } else {
            std::list<ImagePlaneDesc> availablePlanes;
            effect->getAvailableLayers(_publicInterface->getApp()->getTimeLine()->currentFrame(), ViewIdx(0), inputNb, &availablePlanes);

            ImagePlaneDesc comp = getSelectedLayerInternal(inputNb, availablePlanes, selector);
            _publicInterface->refreshEnabledKnobsLabel(comp);
        }

        _publicInterface->s_outputLayerChanged();
    }
}

void
Node::refreshEnabledKnobsLabel(const ImagePlaneDesc& comp)
{
    const std::vector<std::string>& channels = comp.getChannels();
    if (!_imp->enabledChan[0].lock()) {
        return;
    }
    switch ( channels.size() ) {
    case 1: {
        for (int i = 0; i < 3; ++i) {
            boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
            enabled->setSecret(true);
        }
        boost::shared_ptr<KnobBool> alpha = _imp->enabledChan[3].lock();
        alpha->setSecret(false);
        alpha->setLabel(channels[0]);
        break;
    }
    case 2: {
        for (int i = 2; i < 4; ++i) {
            boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
            enabled->setSecret(true);
        }
        for (int i = 0; i < 2; ++i) {
            boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
            enabled->setSecret(false);
            enabled->setLabel(channels[i]);
        }
        break;
    }
    case 3: {
        for (int i = 3; i < 4; ++i) {
            boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
            enabled->setSecret(true);
        }
        for (int i = 0; i < 3; ++i) {
            boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
            enabled->setSecret(false);
            enabled->setLabel(channels[i]);
        }
        break;
    }
    case 4: {
        for (int i = 0; i < 4; ++i) {
            boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
            enabled->setSecret(false);
            enabled->setLabel(channels[i]);
        }
        break;
    }

    case 0:
    default: {
        for (int i = 0; i < 4; ++i) {
            boost::shared_ptr<KnobBool> enabled = _imp->enabledChan[i].lock();
            enabled->setSecret(true);
        }
        break;
    }
    } // switch
} // Node::refreshEnabledKnobsLabel

void
Node::Implementation::onMaskSelectorChanged(int inputNb,
                                            const MaskSelector& selector)
{
    boost::shared_ptr<KnobChoice> channel = selector.channel.lock();
    int index = channel->getValue();
    boost::shared_ptr<KnobBool> enabled = selector.enabled.lock();

    if ( (index == 0) && enabled->isEnabled(0) ) {
        enabled->setValue(false);
        enabled->setEnabled(0, false);
    } else if ( !enabled->isEnabled(0) ) {
        enabled->setEnabled(0, true);
        if ( _publicInterface->getInput(inputNb) ) {
            enabled->setValue(true);
        }
    }

    if (!isRefreshingInputRelatedData) {
        ///Clip preferences have changed
        RenderScale s(1.);
        effect->refreshMetadata_public(true);
    }
}

bool
Node::isPluginUsingHostChannelSelectors() const
{
    return _imp->hostChannelSelectorEnabled;
}

bool
Node::getProcessChannel(int channelIndex) const
{
    if (!isPluginUsingHostChannelSelectors()) {
        return true;
    }
    assert(channelIndex >= 0 && channelIndex < 4);
    boost::shared_ptr<KnobBool> k = _imp->enabledChan[channelIndex].lock();
    if (k) {
        return k->getValue();
    }

    return true;
}

bool
Node::getSelectedLayer(int inputNb,
                       const std::list<ImagePlaneDesc>& availableLayers,
                       std::bitset<4> *processChannels,
                       bool* isAll,
                       ImagePlaneDesc* layer) const
{
    // If there's a mask channel selector, fetch the mask layer
    int chanIndex = getMaskChannel(inputNb, availableLayers, layer);
    if (chanIndex != -1) {
        *isAll = false;
        Q_UNUSED(chanIndex);
        if (processChannels) {
            (*processChannels)[0] = true;
            (*processChannels)[1] = true;
            (*processChannels)[2] = true;
            (*processChannels)[3] = true;
        }

        return true;
    }


    std::map<int, ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(inputNb);

    if ( foundSelector == _imp->channelsSelectors.end() ) {
        // Fetch in input what the user has set for the output
        foundSelector = _imp->channelsSelectors.find(-1);
    }


    // Check if the checkbox "All layers" is checked or not
    boost::shared_ptr<KnobBool> processAllKnob = _imp->processAllLayersKnob.lock();
    *isAll = false;
    if (processAllKnob) {
        *isAll = processAllKnob->getValue();
    }

    if (!*isAll && foundSelector != _imp->channelsSelectors.end()) {
        *layer = _imp->getSelectedLayerInternal(inputNb, availableLayers, foundSelector->second);
    }

    if (processChannels) {
        if (_imp->hostChannelSelectorEnabled &&  _imp->enabledChan[0].lock() ) {
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
    }
    return foundSelector != _imp->channelsSelectors.end();
}

bool
Node::hasAtLeastOneChannelToProcess() const
{
    std::map<int, ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(-1);

    if ( foundSelector == _imp->channelsSelectors.end() ) {
        return true;
    }
    if ( _imp->enabledChan[0].lock() ) {
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
    QString label = QString::fromUtf8( labelKnob->getValue().c_str() );
    ///Since the label is html encoded, find the text's start
    int foundFontTag = label.indexOf( QString::fromUtf8("<font") );
    bool htmlPresent =  (foundFontTag != -1);
    ///we're sure this end tag is the one of the font tag
    QString endFont( QString::fromUtf8("\">") );
    int endFontTag = label.indexOf(endFont, foundFontTag);
    QString customTagStart( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START) );
    QString customTagEnd( QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END) );
    int foundNatronCustomDataTag = label.indexOf(customTagStart, endFontTag == -1 ? 0 : endFontTag);
    if (foundNatronCustomDataTag != -1) {
        ///remove the current custom data
        int foundNatronEndTag = label.indexOf(customTagEnd, foundNatronCustomDataTag);
        assert(foundNatronEndTag != -1);

        foundNatronEndTag += customTagEnd.size();
        label.remove(foundNatronCustomDataTag, foundNatronEndTag - foundNatronCustomDataTag);
    }

    int i = htmlPresent ? endFontTag + endFont.size() : 0;
    label.insert(i, customTagStart);
    label.insert(i + customTagStart.size(), data);
    label.insert(i + customTagStart.size() + data.size(), customTagEnd);
    labelKnob->setValue( label.toStdString() );
}

boost::shared_ptr<KnobBool>
Node::getDisabledKnob() const
{
    return _imp->disableNodeKnob.lock();
}

bool
Node::isLifetimeActivated(int *firstFrame,
                          int *lastFrame) const
{
    boost::shared_ptr<KnobBool> enableLifetimeKnob = _imp->enableLifeTimeKnob.lock();

    if (!enableLifetimeKnob) {
        return false;
    }
    if ( !enableLifetimeKnob->getValue() ) {
        return false;
    }
    boost::shared_ptr<KnobInt> lifetimeKnob = _imp->lifeTimeKnob.lock();
    *firstFrame = lifetimeKnob->getValue(0);
    *lastFrame = lifetimeKnob->getValue(1);

    return true;
}

bool
Node::isNodeDisabled() const
{
    boost::shared_ptr<KnobBool> b = _imp->disableNodeKnob.lock();
    bool thisDisabled = b ? b->getValue() : false;
    NodeGroup* isContainerGrp = dynamic_cast<NodeGroup*>( getGroup().get() );

    if (isContainerGrp) {
        return thisDisabled || isContainerGrp->getNode()->isNodeDisabled();
    }
#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        return ioContainer->isNodeDisabled();
    }
#endif

    int lifeTimeFirst, lifeTimeEnd;
    bool lifeTimeEnabled = isLifetimeActivated(&lifeTimeFirst, &lifeTimeEnd);
    double curFrame = _imp->effect->getCurrentTime();
    bool enabled = ( !lifeTimeEnabled || (curFrame >= lifeTimeFirst && curFrame <= lifeTimeEnd) ) && !thisDisabled;

    return !enabled;
}

void
Node::setNodeDisabled(bool disabled)
{
    boost::shared_ptr<KnobBool> b = _imp->disableNodeKnob.lock();

    if (b) {
        b->setValue(disabled);

        // Clear the actions cache because if this function is called from another thread, the hash will not be incremented
        _imp->effect->clearActionsCache();
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

bool
Node::hasAnimatedKnob() const
{
    const KnobsVec & knobs = getKnobs();
    bool hasAnimation = false;

    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->canAnimate() ) {
            for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                if ( knobs[i]->isAnimated(j) ) {
                    hasAnimation = true;
                    break;
                }
            }
        }
        if (hasAnimation) {
            break;
        }
    }

    return hasAnimation;
}

void
Node::getAllKnobsKeyframes(std::list<SequenceTime>* keyframes)
{
    assert(keyframes);
    const KnobsVec & knobs = getKnobs();

    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->getIsSecret() || !knobs[i]->getIsPersistent() ) {
            continue;
        }
        if ( !knobs[i]->canAnimate() ) {
            continue;
        }
        int dim = knobs[i]->getDimension();
        KnobFile* isFile = dynamic_cast<KnobFile*>( knobs[i].get() );
        if (isFile) {
            ///skip file knobs
            continue;
        }
        for (int j = 0; j < dim; ++j) {
            if ( knobs[i]->canAnimate() && knobs[i]->isAnimated( j, ViewIdx(0) ) ) {
                KeyFrameSet kfs = knobs[i]->getCurve(ViewIdx(0), j)->getKeyFrames_mt_safe();
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
            if ( input && input->hasSequentialOnlyNodeUpstream(nodeName) && input->getEffectInstance()->isWriter() ) {
                nodeName = input->getScriptName_mt_safe();

                return true;
            }
        }

        return false;
    }
}

bool
Node::isTrackerNodePlugin() const
{
    return _imp->effect->isTrackerNodePlugin();
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
    KnobIPtr knob = getKnobByName(kNatronOfxParamStringSublabelName);
    KnobString* strKnob = dynamic_cast<KnobString*>( knob.get() );
    if (strKnob) {
        strKnob->setValue( name.toStdString() );
    }
}

bool
Node::canOthersConnectToThisNode() const
{
    if ( dynamic_cast<Backdrop*>( _imp->effect.get() ) ) {
        return false;
    } else if ( dynamic_cast<GroupOutput*>( _imp->effect.get() ) ) {
        return false;
    } else if ( _imp->effect->isWriter() && (_imp->effect->getSequentialPreference() == eSequentialPreferenceOnlySequential) ) {
        return false;
    }
    ///In debug mode only allow connections to Writer nodes
# ifdef DEBUG

    return dynamic_cast<const ViewerInstance*>( _imp->effect.get() ) == NULL;
# else // !DEBUG
    return dynamic_cast<const ViewerInstance*>( _imp->effect.get() ) == NULL /* && !_imp->effect->isWriter()*/;
# endif // !DEBUG
}

void
Node::setNodeIsRenderingInternal(std::list<NodeWPtr>& markedNodes)
{
    ///If marked, we alredy set render args
    for (std::list<NodeWPtr>::iterator it = markedNodes.begin(); it != markedNodes.end(); ++it) {
        if (it->lock().get() == this) {
            return;
        }
    }

    ///Wait for the main-thread to be done dequeuing the connect actions queue
    if ( QThread::currentThread() != qApp->thread() ) {
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        while ( _imp->nodeIsDequeuing && !aborted() ) {
            _imp->nodeIsDequeuingCond.wait(&_imp->nodeIsDequeuingMutex);
        }
    }

    ///Increment the node is rendering counter
    {
        QMutexLocker nrLocker(&_imp->nodeIsRenderingMutex);
        ++_imp->nodeIsRendering;
    }


    ///mark this
    markedNodes.push_back( shared_from_this() );

    ///Call recursively

    int maxInpu = getNInputs();
    for (int i = 0; i < maxInpu; ++i) {
        NodePtr input = getInput(i);
        if (input) {
            input->setNodeIsRenderingInternal(markedNodes);
        }
    }
}

RenderingFlagSetter::RenderingFlagSetter(const NodePtr& n)
    : node(n)
    , nodes()
{
    n->setNodeIsRendering(nodes);
}

RenderingFlagSetter::~RenderingFlagSetter()
{
    for (std::list<NodeWPtr>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr n = it->lock();
        if (!n) {
            continue;
        }
        n->unsetNodeIsRendering();
    }
}

void
Node::setNodeIsRendering(std::list<NodeWPtr>& nodes)
{
    setNodeIsRenderingInternal(nodes);
}

void
Node::unsetNodeIsRendering()
{
    bool mustDequeue;
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
    assert( QThread::currentThread() == qApp->thread() );

    ///Flag that the node is dequeuing.
    {
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        assert(!_imp->nodeIsDequeuing);
        _imp->nodeIsDequeuing = true;
    }
    bool hasChanged = false;
    if (_imp->effect) {
        hasChanged |= _imp->effect->dequeueValuesSet();
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>( _imp->effect.get() );
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
        assert( _imp->guiInputs.size() == _imp->inputs.size() );

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

    if ( !inputChanges.empty() ) {
        beginInputEdition();
        hasChanged = true;
        for (std::set<int>::iterator it = inputChanges.begin(); it != inputChanges.end(); ++it) {
            onInputChanged(*it);
        }
        endInputEdition(true);
    }
    if (hasChanged) {
        computeHash();
        refreshIdentityState();
    }

    {
        QMutexLocker k(&_imp->nodeIsDequeuingMutex);
        //Another slots in this thread might have aborted the dequeuing
        if (_imp->nodeIsDequeuing) {
            _imp->nodeIsDequeuing = false;

            //There might be multiple threads waiting
            _imp->nodeIsDequeuingCond.wakeAll();
        }
    }
} // Node::dequeueActions

static void
addIdentityNodesRecursively(const Node* caller,
                            const Node* node,
                            double time,
                            ViewIdx view,
                            std::list<const Node*>* outputs,
                            std::list<const Node*>* markedNodes)
{
    if ( std::find(markedNodes->begin(), markedNodes->end(), node) != markedNodes->end() ) {
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
            ViewIdx identityView;
            int inputNbId;
            U64 renderHash;

            renderHash = node->getEffectInstance()->getRenderHash();

            RectI format = node->getEffectInstance()->getOutputFormat();

            isIdentity = node->getEffectInstance()->isIdentity_public(true, renderHash, time, scale, format, view, &inputTimeId, &identityView, &inputNbId);
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
        GroupOutput* isOutputNode = dynamic_cast<GroupOutput*>( output->getEffectInstance().get() );
        //If the node is an output node, add all the outputs of the group node instead
        if (isOutputNode) {
            NodeCollectionPtr collection = output->getGroup();
            assert(collection);
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>( collection.get() );
            if (isGrp) {
                NodesWList groupOutputs;
                isGrp->getNode()->getOutputs_mt_safe(groupOutputs);
                for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                    outputsToAdd.push_back(*it2);
                }
            }
        }

        //If the node is a group, add all its inputs
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>(output->getEffectInstance().get());
        if (isGrp) {
            NodesList inputOutputs;
            isGrp->getInputsOutputs(&inputOutputs, false);
            for (NodesList::iterator it2 = inputOutputs.begin(); it2 != inputOutputs.end(); ++it2) {
                outputsToAdd.push_back(*it2);
            }

        }

    }
    nodeOutputs.insert( nodeOutputs.end(), outputsToAdd.begin(), outputsToAdd.end() );
    for (NodesWList::iterator it = nodeOutputs.begin(); it != nodeOutputs.end(); ++it) {
        NodePtr node = it->lock();
        if (node) {
            addIdentityNodesRecursively(caller, node.get(), time, view, outputs, markedNodes);
        }
    }
} // addIdentityNodesRecursively

bool
Node::shouldCacheOutput(bool isFrameVaryingOrAnimated,
                        double time,
                        ViewIdx view,
                        int /*visitsCount*/) const
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
        addIdentityNodesRecursively(this, this, time, view, &outputs, &markedNodes);
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
                if ( (output->getInput(activeInputs[0]).get() == this) ||
                     ( output->getInput(activeInputs[1]).get() == this) ) {
                    ///The node is a direct input of the viewer. Cache it because it is likely the user will make

                    ///changes to the viewer that will need this image.
                    return true;
                }
            }

            RotoPaint* isRoto = dynamic_cast<RotoPaint*>(output->getEffectInstance().get());
            if (isRoto) {
                // THe roto internally makes multiple references to the input so cache it
                return true;
            }

            if (!isFrameVaryingOrAnimated) {
                //This image never changes, cache it once.
                return true;
            }
            if ( output->isSettingsPanelVisible() ) {
                //Output node has panel opened, meaning the user is likely to be heavily editing

                //that output node, hence requesting this node a lot. Cache it.
                return true;
            }
            if ( _imp->effect->doesTemporalClipAccess() ) {
                //Very heavy to compute since many frames are fetched upstream. Cache it.
                return true;
            }
            if ( !_imp->effect->supportsTiles() ) {
                //No tiles, image is going to be produced fully, cache it to prevent multiple access

                //with different RoIs
                return true;
            }
            if (_imp->effect->getRecursionLevel() > 0) {
                //We are in a call from getImage() and the image needs to be computed, so likely in an

                //analysis pass. Cache it because the image is likely to get asked for severla times.
                return true;
            }
            if ( isForceCachingEnabled() ) {
                //Users wants it cached
                return true;
            }
            NodeGroup* parentIsGroup = dynamic_cast<NodeGroup*>( getGroup().get() );
            if ( parentIsGroup && parentIsGroup->getNode()->isForceCachingEnabled() && (parentIsGroup->getOutputNodeInput(false).get() == this) ) {
                //if the parent node is a group and it has its force caching enabled, cache the output of the Group Output's node input.
                return true;
            }

            if ( appPTR->isAggressiveCachingEnabled() ) {
                ///Users wants all nodes cached
                return true;
            }

            if ( isPreviewEnabled() && !appPTR->isBackground() ) {
                ///The node has a preview, meaning the image will be computed several times between previews & actual renders. Cache it.
                return true;
            }

            if ( isRotoPaintingNode() && isSettingsPanelVisible() ) {
                ///The Roto node is being edited, cache its output (special case because Roto has an internal node tree)
                return true;
            }

            boost::shared_ptr<RotoDrawableItem> attachedStroke = _imp->paintStroke.lock();
            if ( attachedStroke && attachedStroke->getContext()->getNode()->isSettingsPanelVisible() ) {
                ///Internal RotoPaint tree and the Roto node has its settings panel opened, cache it.
                return true;
            }
        } else {
            // outputs == 0, never cache, unless explicitly set or rotopaint internal node
            boost::shared_ptr<RotoDrawableItem> attachedStroke = _imp->paintStroke.lock();

            return isForceCachingEnabled() || appPTR->isAggressiveCachingEnabled() ||
                   ( attachedStroke && attachedStroke->getContext()->getNode()->isSettingsPanelVisible() );
        }
    }

    return false;
} // Node::shouldCacheOutput

bool
Node::refreshLayersChoiceSecretness(int inputNb)
{
    std::map<int, ChannelSelector>::iterator foundChan = _imp->channelsSelectors.find(inputNb);
    NodePtr inp = getInputInternal(false /*useGuiInput*/, false /*useGroupRedirections*/, inputNb);

    if ( foundChan != _imp->channelsSelectors.end() ) {
        std::map<int, ChannelSelector>::iterator foundOuptut = _imp->channelsSelectors.find(-1);
        bool outputIsAll = false;
        if ( foundOuptut != _imp->channelsSelectors.end() ) {
            boost::shared_ptr<KnobChoice> outputChoice = foundOuptut->second.layer.lock();
            if (outputChoice) {
                outputIsAll = _imp->processAllLayersKnob.lock()->getValue();
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
    std::map<int, MaskSelector>::iterator found = _imp->maskSelectors.find(inputNb);
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
            enabled->setValue(newValue);
        }
        enabled->unblockValueChanges();
    }

    return changed;
}

bool
Node::refreshDraftFlagInternal(const std::vector<NodeWPtr>& inputs)
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
    refreshAllInputRelatedData( canChangeValues, getInputs_copy() );
}

bool
Node::refreshAllInputRelatedData(bool /*canChangeValues*/,
                                 const std::vector<NodeWPtr>& inputs)
{
    assert( QThread::currentThread() == qApp->thread() );
    RefreshingInputData_RAII _refreshingflag( _imp.get() );
    bool hasChanged = false;
    hasChanged |= refreshDraftFlagInternal(inputs);

    bool loadingProject = getApp()->getProject()->isLoadingProject();
    ///if all non optional clips are connected, call getClipPrefs
    ///The clip preferences action is never called until all non optional clips have been attached to the plugin.
    /// EDIT: we allow calling getClipPreferences even if some non optional clip is not connected so that layer menus get properly created
    const bool canCallRefreshMetadata = true; // = !hasMandatoryInputDisconnected();

    if (canCallRefreshMetadata) {
        if (loadingProject) {
            //Nb: we clear the action cache because when creating the node many calls to getRoD and stuff might have returned
            //empty rectangles, but since we force the hash to remain what was in the project file, we might then get wrong RoDs returned
            _imp->effect->clearActionsCache();
        }

        double time = (double)getApp()->getTimeLine()->currentFrame();
        RenderScale scaleOne(1.);
        ///Render scale support might not have been set already because getRegionOfDefinition could have failed until all non optional inputs were connected
        if (_imp->effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsMaybe) {
            RectD rod;
            StatusEnum stat = _imp->effect->getRegionOfDefinition(getHashValue(), time, scaleOne, ViewIdx(0), &rod);
            if (stat != eStatusFailed) {
                RenderScale scale(0.5);
                stat = _imp->effect->getRegionOfDefinition(getHashValue(), time, scale, ViewIdx(0), &rod);
                if (stat != eStatusFailed) {
                    _imp->effect->setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);
                } else {
                    _imp->effect->setSupportsRenderScaleMaybe(EffectInstance::eSupportsNo);
                }
            }
        }
        hasChanged |= _imp->effect->refreshMetadata_public(false);
    }

    hasChanged |= refreshChannelSelectors();

    refreshIdentityState();

    if (loadingProject) {
        //When loading the project, refresh the hash of the nodes in a recursive manner in the proper order
        //for the disk cache to work
        hasChanged |= computeHashInternal();
    }

    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        _imp->mustComputeInputRelatedData = false;
    }

    return hasChanged;
} // Node::refreshAllInputRelatedData

bool
Node::refreshInputRelatedDataInternal(bool domarking, std::set<Node*>& markedNodes)
{
    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        if (!_imp->mustComputeInputRelatedData) {
            //We didn't change
            return false;
        }
    }

    if (domarking) {
        std::set<Node*>::iterator found = markedNodes.find(this);

        if ( found != markedNodes.end() ) {
            return false;
        }
    }

    ///Check if inputs must be refreshed first

    int maxInputs = getNInputs();
    std::vector<NodeWPtr> inputsCopy(maxInputs);
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr input = getInput(i);
        inputsCopy[i] = input;
        if ( input && input->isInputRelatedDataDirty() ) {
            input->refreshInputRelatedDataInternal(true, markedNodes);
        }
    }

    if (domarking) {
        markedNodes.insert(this);
    }

    bool hasChanged = refreshAllInputRelatedData(false, inputsCopy);

    if ( isRotoPaintingNode() ) {
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

    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGroup) {
        NodesList inputs;
        isGroup->getInputsOutputs(&inputs, false);
        for (NodesList::iterator it = inputs.begin(); it != inputs.end(); ++it) {
            if ( (*it) ) {
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
    if ( isRotoPaintingNode() ) {
        boost::shared_ptr<RotoContext> roto = getRotoContext();
        assert(roto);
        NodesList rotoNodes;
        roto->getRotoPaintTreeNodes(&rotoNodes);
        for (NodesList::iterator it = rotoNodes.begin(); it != rotoNodes.end(); ++it) {
            (*it)->markAllInputRelatedDataDirty();
        }
    }
}

void
Node::markInputRelatedDataDirtyRecursiveInternal(std::list<Node*>& markedNodes,
                                                 bool recurse)
{
    std::list<Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), this);

    if ( found != markedNodes.end() ) {
        return;
    }
    markAllInputRelatedDataDirty();
    markedNodes.push_back(this);
    if (recurse) {
        NodesList outputs;
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
Node::refreshInputRelatedDataRecursiveInternal(std::set<Node*>& markedNodes)
{
    if ( getApp()->isCreatingNodeTree() ) {
        return;
    }
    std::set<Node*>::iterator found = markedNodes.find(this);

    if ( found != markedNodes.end() ) {
        return;
    }

    markedNodes.insert(this);
    refreshInputRelatedDataInternal(false, markedNodes);

    ///Now notify outputs we have changed
    NodesList outputs;
    getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->refreshInputRelatedDataRecursiveInternal( markedNodes );
    }
}

void
Node::refreshInputRelatedDataRecursive()
{
    std::set<Node*> markedNodes;

    refreshInputRelatedDataRecursiveInternal(markedNodes);
}

bool
Node::isDraftModeUsed() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->draftModeUsed;
}

void
Node::setPosition(double x,
                  double y)
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->setPosition(x, y);
    }
}

void
Node::getPosition(double *x,
                  double *y) const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->getPosition(x, y);
    } else {
        *x = 0.;
        *y = 0.;
    }
}

void
Node::setSize(double w,
              double h)
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->setSize(w, h);
    }
}

void
Node::getSize(double* w,
              double* h) const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->getSize(w, h);
    } else {
        *w = 0.;
        *h = 0.;
    }
}

bool
Node::getColor(double* r,
               double *g,
               double* b) const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->getColor(r, g, b);
        return true;
    } else {
        *r = 0.;
        *g = 0.;
        *b = 0.;
        return false;
    }
}

void
Node::setColor(double r,
               double g,
               double b)
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->setColor(r, g, b);
    }
}

void
Node::setNodeGuiPointer(const NodeGuiIPtr& gui)
{
    assert( !_imp->guiPointer.lock() );
    assert( QThread::currentThread() == qApp->thread() );
    _imp->guiPointer = gui;
}

NodeGuiIPtr
Node::getNodeGui() const
{
    return _imp->guiPointer.lock();
}

bool
Node::isUserSelected() const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (!gui) {
        return false;
    }

    return gui->isUserSelected();
}

bool
Node::isSettingsPanelMinimized() const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (!gui) {
        return false;
    }

    return gui->isSettingsPanelMinimized();
}

bool
Node::isSettingsPanelVisibleInternal(std::set<const Node*>& recursionList) const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (!gui) {
        return false;
    }
    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->isSettingsPanelVisible();
    }

    if ( recursionList.find(this) != recursionList.end() ) {
        return false;
    }
    recursionList.insert(this);

    {
        NodePtr master = getMasterNode();
        if (master) {
            return master->isSettingsPanelVisible();
        }
        for (KnobLinkList::iterator it = _imp->nodeLinks.begin(); it != _imp->nodeLinks.end(); ++it) {
            NodePtr masterNode = it->masterNode.lock();
            if ( masterNode && (masterNode.get() != this) && masterNode->isSettingsPanelVisibleInternal(recursionList) ) {
                return true;
            }
        }
    }

    return gui->isSettingsPanelVisible();
}

bool
Node::isSettingsPanelVisible() const
{
    std::set<const Node*> tmplist;

    return isSettingsPanelVisibleInternal(tmplist);
}

void
Node::attachRotoItem(const boost::shared_ptr<RotoDrawableItem>& stroke)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->paintStroke = stroke;
    _imp->useAlpha0ToConvertFromRGBToRGBA = true;
    setProcessChannelsValues(true, true, true, true);
}

void
Node::setUseAlpha0ToConvertFromRGBToRGBA(bool use)
{
    assert( QThread::currentThread() == qApp->thread() );
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
    if (getScriptName_mt_safe().empty()) {
        return;
    }


    PythonGILLocker pgl;
    PyObject* mainModule = appPTR->getMainModule();
    assert(mainModule);

    std::string appID = getApp()->getAppIDString();
    std::string nodeFullName = appID + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, mainModule, &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);

    if (!alreadyDefined) {
        std::stringstream ss;
        ss << nodeFullName << " = " << appID << ".getNode(\"" << nodeName << "\")\n";
#ifdef DEBUG
        ss << "if not " << nodeFullName << ":\n";
        ss << "    print \"[BUG]: " << nodeFullName << " does not exist!\"";
#endif
        std::string script = ss.str();
        std::string output;
        std::string err;
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::setNodeVariableToPython(const std::string& oldName,
                              const std::string& newName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    QString appID = QString::fromUtf8( getApp()->getAppIDString().c_str() );
    QString str = QString( appID + QString::fromUtf8(".%1 = ") + appID + QString::fromUtf8(".%2\ndel ") + appID + QString::fromUtf8(".%2\n") ).arg( QString::fromUtf8( newName.c_str() ) ).arg( QString::fromUtf8( oldName.c_str() ) );
    std::string script = str.toStdString();
    std::string err;
    if ( !appPTR->isBackground() ) {
        getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        qDebug() << err.c_str();
    }
}

void
Node::deleteNodeVariableToPython(const std::string& nodeName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
   if (getScriptName_mt_safe().empty()) {
        return;
    }
    if ( getParentMultiInstance() ) {
        return;
    }

    AppInstancePtr app = getApp();
    if (!app) {
        return;
    }
    QString appID = QString::fromUtf8( getApp()->getAppIDString().c_str() );
    std::string nodeFullName = appID.toStdString() + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, appPTR->getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (alreadyDefined) {
        std::string script = "del " + nodeFullName;
        std::string err;
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::declarePythonFields()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    PythonGILLocker pgl;

    if ( !getGroup() ) {
        return;
    }

    std::locale locale;
    std::string nodeName;
    if (getIOContainer()) {
        nodeName = getIOContainer()->getFullyQualifiedName();
    } else {
        nodeName = getFullyQualifiedName();
    }
    std::string appID = getApp()->getAppIDString();
    bool alreadyDefined = false;
    std::string nodeFullName = appID + "." + nodeName;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (!alreadyDefined) {
        qDebug() << QString::fromUtf8("declarePythonFields(): attribute ") + QString::fromUtf8( nodeFullName.c_str() ) + QString::fromUtf8(" is not defined");
        throw std::logic_error(std::string("declarePythonFields(): attribute ") + nodeFullName + " is not defined");
    }


    std::stringstream ss;
#ifdef DEBUG
    ss << "if not " << nodeFullName << ":\n";
    ss << "    print \"[BUG]: " << nodeFullName << " is not defined!\"\n";
#endif
    const KnobsVec& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        const std::string& knobName = knobs[i]->getName();
        if ( !knobName.empty() && (knobName.find(" ") == std::string::npos) && !std::isdigit(knobName[0], locale) ) {
            if ( PyObject_HasAttrString( nodeObj, knobName.c_str() ) ) {
                continue;
            }
            ss << nodeFullName <<  "." << knobName << " = ";
            ss << nodeFullName << ".getParam(\"" << knobName << "\")\n";
        }
    }

    std::string script = ss.str();
    if ( !script.empty() ) {
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            qDebug() << err.c_str();
        }
    }
} // Node::declarePythonFields

void
Node::removeParameterFromPython(const std::string& parameterName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    PythonGILLocker pgl;
    std::string appID = getApp()->getAppIDString();
    std::string nodeName;
    if (getIOContainer()) {
        nodeName = getIOContainer()->getFullyQualifiedName();
    } else {
        nodeName = getFullyQualifiedName();
    }
    std::string nodeFullName = appID + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (!alreadyDefined) {
        qDebug() << QString::fromUtf8("removeParameterFromPython(): attribute ") + QString::fromUtf8( nodeFullName.c_str() ) + QString::fromUtf8(" is not defined");
        throw std::logic_error(std::string("removeParameterFromPython(): attribute ") + nodeFullName + " is not defined");
    }
    assert( PyObject_HasAttrString( nodeObj, parameterName.c_str() ) );
    std::string script = "del " + nodeFullName + "." + parameterName;
    if ( !appPTR->isBackground() ) {
        getApp()->printAutoDeclaredVariable(script);
    }
    std::string err;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        qDebug() << err.c_str();
    }
}

void
Node::declareAllPythonAttributes()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    try {
        declareNodeVariableToPython( getFullyQualifiedName() );
        declarePythonFields();
        if (_imp->rotoContext) {
            declareRotoPythonField();
        }
        if (_imp->trackContext) {
            declareTrackerPythonField();
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
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
Node::Implementation::runOnNodeCreatedCBInternal(const std::string& cb,
                                                 bool userEdited)
{
    std::vector<std::string> args;
    std::string error;
    if (_publicInterface->getScriptName_mt_safe().empty()) {
        return;
    }
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onNodeCreated callback: ")
                                                          + e.what() );

        return;
    }

    if ( !error.empty() ) {
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

    if ( (args[0] != "thisNode") || (args[1] != "app") || (args[2] != "userEdited") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    std::string scriptName = _publicInterface->getScriptName_mt_safe();
    if ( scriptName.empty() ) {
        return;
    }
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
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeCreated callback: " + error);
    } else if ( !output.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }
} // Node::Implementation::runOnNodeCreatedCBInternal

void
Node::Implementation::runOnNodeDeleteCBInternal(const std::string& cb)
{
    std::vector<std::string> args;
    std::string error;

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onNodeDeletion callback: ")
                                                          + e.what() );

        return;
    }

    if ( !error.empty() ) {
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

    if ( (args[0] != "thisNode") || (args[1] != "app") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    std::stringstream ss;
    ss << cb << "(" << appID << "." << _publicInterface->getFullyQualifiedName() << "," << appID << ")\n";

    std::string err;
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(ss.str(), &err, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onNodeDeletion callback: " + err);
    } else if ( !output.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor(output);
    }
}

void
Node::Implementation::runOnNodeCreatedCB(bool userEdited)
{
    if (!isPartOfProject) {
        return;
    }
    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeCreatedCB();
    NodeCollectionPtr group = _publicInterface->getGroup();

    if (!group) {
        return;
    }
    if ( !cb.empty() ) {
        runOnNodeCreatedCBInternal(cb, userEdited);
    }

    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( group.get() );
    boost::shared_ptr<KnobString> nodeCreatedCbKnob = nodeCreatedCallback.lock();
    if (!nodeCreatedCbKnob && isGroup) {
        cb = isGroup->getNode()->getAfterNodeCreatedCallback();
    } else if (nodeCreatedCbKnob) {
        cb = nodeCreatedCbKnob->getValue();
    }
    if ( !cb.empty() ) {
        runOnNodeCreatedCBInternal(cb, userEdited);
    }
}

void
Node::Implementation::runOnNodeDeleteCB()
{
    if (!isPartOfProject) {
        return;
    }

    if (_publicInterface->getScriptName_mt_safe().empty()) {
        return;
    }
    std::string cb = _publicInterface->getApp()->getProject()->getOnNodeDeleteCB();
    NodeCollectionPtr group = _publicInterface->getGroup();

    if (!group) {
        return;
    }
    if ( !cb.empty() ) {
        runOnNodeDeleteCBInternal(cb);
    }


    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( group.get() );
    boost::shared_ptr<KnobString> nodeDeletedKnob = nodeRemovalCallback.lock();
    if (!nodeDeletedKnob && isGroup) {
        NodePtr grpNode = isGroup->getNode();
        if (grpNode) {
            cb = grpNode->getBeforeNodeRemovalCallback();
        }
    } else if (nodeDeletedKnob) {
        cb = nodeDeletedKnob->getValue();
    }
    if ( !cb.empty() ) {
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

    if ( !cb.empty() ) {
        _imp->runInputChangedCallback(index, cb);
    }
}

void
Node::Implementation::runInputChangedCallback(int index,
                                              const std::string& cb)
{
    std::vector<std::string> args;
    std::string error;

    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(cb, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( std::string("Failed to run onInputChanged callback: ")
                                                          + e.what() );

        return;
    }

    if ( !error.empty() ) {
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

    if ( (args[0] != "inputIndex") || (args[1] != "thisNode") || (args[2] != "thisGroup") || (args[3] != "app") ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onInputChanged callback: " + signatureError);

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();
    NodeCollectionPtr collection = _publicInterface->getGroup();
    assert(collection);
    if (!collection) {
        return;
    }

    std::string thisGroupVar;
    NodeGroup* isParentGrp = dynamic_cast<NodeGroup*>( collection.get() );
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
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to execute callback: %1").arg( QString::fromUtf8( error.c_str() ) ).toStdString() );
    } else {
        if ( !output.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }
} // Node::Implementation::runInputChangedCallback

boost::shared_ptr<KnobChoice>
Node::getChannelSelectorKnob(int inputNb) const
{
    std::map<int, ChannelSelector>::const_iterator found = _imp->channelsSelectors.find(inputNb);

    if ( found == _imp->channelsSelectors.end() ) {
        if (inputNb == -1) {
            ///The effect might be multi-planar and supply its own
            KnobIPtr knob = getKnobByName(kNatronOfxParamOutputChannels);
            if (!knob) {
                return boost::shared_ptr<KnobChoice>();
            }

            return boost::dynamic_pointer_cast<KnobChoice>(knob);
        }

        return boost::shared_ptr<KnobChoice>();
    }

    return found->second.layer.lock();
}

boost::shared_ptr<KnobBool>
Node::getProcessAllLayersKnob() const
{
    return _imp->processAllLayersKnob.lock();
}

void
Node::checkForPremultWarningAndCheckboxes()
{
    if ( isOutputNode() ) {
        return;
    }
    boost::shared_ptr<KnobBool> chans[4];
    boost::shared_ptr<KnobString> premultWarn = _imp->premultWarning.lock();
    if (!premultWarn) {
        return;
    }
    NodePtr prefInput = getPreferredInputNode();

    //Do not display a warning for Roto paint
    if ( !prefInput || _imp->effect->isRotoPaintNode() ) {
        //No input, do not warn
        premultWarn->setSecret(true);

        return;
    }
    for (int i = 0; i < 4; ++i) {
        chans[i] = _imp->enabledChan[i].lock();

        //No checkboxes
        if (!chans[i]) {
            premultWarn->setSecret(true);

            return;
        }

        //not RGBA
        if ( chans[i]->getIsSecret() ) {
            return;
        }
    }

    ImagePremultiplicationEnum premult = _imp->effect->getPremult();

    //not premult
    if (premult != eImagePremultiplicationPremultiplied) {
        premultWarn->setSecret(true);

        return;
    }

    bool checked[4];
    checked[3] = chans[3]->getValue();

    //alpha unchecked
    if (!checked[3]) {
        premultWarn->setSecret(true);

        return;
    }
    for (int i = 0; i < 3; ++i) {
        checked[i] = chans[i]->getValue();
        if (!checked[i]) {
            premultWarn->setSecret(false);

            return;
        }
    }

    //RGB checked
    premultWarn->setSecret(true);
} // Node::checkForPremultWarningAndCheckboxes

int
Node::getMaskChannel(int inputNb, const std::list<ImagePlaneDesc>& availableLayers, ImagePlaneDesc* comps) const
{
    *comps = ImagePlaneDesc::getNoneComponents();

    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);

    if ( it == _imp->maskSelectors.end() ) {
        return -1;
    }
    ChoiceOption maskChannelID =  it->second.channel.lock()->getActiveEntry();

    for (std::list<ImagePlaneDesc>::const_iterator it2 = availableLayers.begin(); it2 != availableLayers.end(); ++it2) {

        std::size_t nChans = (std::size_t)it2->getNumComponents();
        for (std::size_t c = 0; c < nChans; ++c) {
            ChoiceOption channelOption = it2->getChannelOption(c);
            if (channelOption.id == maskChannelID.id) {
                *comps = *it2;
                return c;
            }
        }
    }
    return -1;
}

bool
Node::refreshChannelSelectors()
{
    if ( !isNodeCreated() ) {
        return false;
    }

    double time = getApp()->getTimeLine()->currentFrame();
    // Refresh each layer selector (input and output)
    bool hasChanged = false;
    for (std::map<int, ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {

        int inputNb = it->first;


        // The Output Layer menu has a All choice, input layers menus have a None choice.
        std::vector<ChoiceOption> choices;
        if (inputNb >= 0) {
            choices.push_back(ChoiceOption("None", "", ""));
        }


        std::list<ImagePlaneDesc> availableComponents;
        _imp->effect->getAvailableLayers(time, ViewIdx(0), inputNb,  &availableComponents);

        for (std::list<ImagePlaneDesc>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {
            ChoiceOption layerOption = it2->getPlaneOption();
            choices.push_back(layerOption);
        }

        {
            boost::shared_ptr<KnobChoice> layerKnob = it->second.layer.lock();

            bool menuChanged = layerKnob->populateChoices(choices);
            if (menuChanged) {
                hasChanged = true;
                if (inputNb == -1) {
                    s_outputLayerChanged();
                }
            }
        }
    }  // for each layer selector

    // Refresh each mask channel selector

    for (std::map<int, MaskSelector>::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {

        int inputNb = it->first;
        std::vector<ChoiceOption> choices;
        choices.push_back(ChoiceOption("None", "",""));

        // Get the mask input components
        std::list<ImagePlaneDesc> availableComponents;

        _imp->effect->getAvailableLayers(time, ViewIdx(0), inputNb,  &availableComponents);


        for (std::list<ImagePlaneDesc>::const_iterator it2 = availableComponents.begin(); it2 != availableComponents.end(); ++it2) {

            std::size_t nChans = (std::size_t)it2->getNumComponents();
            for (std::size_t c = 0; c < nChans; ++c) {
                choices.push_back(it2->getChannelOption(c));
            }
        }


        boost::shared_ptr<KnobChoice> channelKnob = it->second.channel.lock();
        
        hasChanged |= channelKnob->populateChoices(choices);
        
    }
    //Notify the effect channels have changed (the viewer needs this)
    _imp->effect->onChannelsSelectorRefreshed();

    return hasChanged;
} // Node::refreshChannelSelectors()

bool
Node::addUserComponents(const ImagePlaneDesc& comps)
{
    ///The node has node channel selector, don't allow adding a custom plane.
    KnobIPtr outputLayerKnob = getKnobByName(kNatronOfxParamOutputChannels);

    if (_imp->channelsSelectors.empty() && !outputLayerKnob) {
        return false;
    }

    if (!outputLayerKnob) {
        //The effect does not have kNatronOfxParamOutputChannels but maybe the selector provided by Natron
        std::map<int, ChannelSelector>::iterator found = _imp->channelsSelectors.find(-1);
        if ( found == _imp->channelsSelectors.end() ) {
            return false;
        }
        outputLayerKnob = found->second.layer.lock();
    }

    {
        QMutexLocker k(&_imp->createdComponentsMutex);
        for (std::list<ImagePlaneDesc>::iterator it = _imp->createdComponents.begin(); it != _imp->createdComponents.end(); ++it) {
            if ( it->getPlaneID() == comps.getPlaneID() ) {
                return false;
            }
        }

        _imp->createdComponents.push_back(comps);
    }
    if (!_imp->isRefreshingInputRelatedData) {
        ///Clip preferences have changed
        RenderScale s(1.);
        getEffectInstance()->refreshMetadata_public(true);
    }
    {
        ///Set the selector to the new channel
        KnobChoice* layerChoice = dynamic_cast<KnobChoice*>( outputLayerKnob.get() );
        if (layerChoice) {
            layerChoice->setValueFromID(comps.getPlaneID(), 0);
        }
    }

    return true;
}

void
Node::getUserCreatedComponents(std::list<ImagePlaneDesc>* comps)
{
    QMutexLocker k(&_imp->createdComponentsMutex);

    *comps = _imp->createdComponents;
}

double
Node::getHostMixingValue(double time,
                         ViewIdx view) const
{
    boost::shared_ptr<KnobDouble> mix = _imp->mixWithSource.lock();

    return mix ? mix->getValueAtTime(time, 0, view) : 1.;
}

//////////////////////////////////

InspectorNode::InspectorNode(const AppInstancePtr& app,
                             const NodeCollectionPtr& group,
                             Plugin* plugin)
    : Node(app, group, plugin)
{
    for (int i = 0; i < 2; ++i) {
        _activeInputs[i] = -1;
    }
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

    if ( !isEffectViewer() ) {
        return connectInputBase(input, inputNumber);
    }

    ///cannot connect more than _maxInputs inputs.
    assert( inputNumber <= getNInputs() );

    assert(input);

    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }

    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if ( !getEffectInstance()->supportsMultiResolution() ) {
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
InspectorNode::setActiveInputAndRefresh(int inputNb,
                                        bool isASide)
{
    assert( QThread::currentThread() == qApp->thread() );

    int maxInputs = getNInputs();
    if ( ( inputNb > (maxInputs - 1) ) || (inputNb < 0) || ( !getInput(inputNb) ) ) {
        return;
    }

    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }

    Q_EMIT inputChanged(inputNb);
    onInputChanged(inputNb, isASide);

    runInputChangedCallback(inputNb);


    if ( isOutputNode() ) {
        OutputEffectInstance* oei = dynamic_cast<OutputEffectInstance*>( getEffectInstance().get() );
        assert(oei);
        if (oei) {
            oei->renderCurrentFrame(true);
        }
    }
}

void
InspectorNode::refreshActiveInputs(int inputNbChanged,
                                   bool isASide)
{
    assert( QThread::currentThread() == qApp->thread() );
    NodePtr inputNode = getRealInput(inputNbChanged);
    {
        QMutexLocker l(&_activeInputsMutex);
        if (!inputNode) {
            ///check if the input was one of the active ones if so set to -1
            if (_activeInputs[0] == inputNbChanged) {
                _activeInputs[0] = -1;
            } else if (_activeInputs[1] == inputNbChanged) {
                _activeInputs[1] = -1;
            }
        } else {
            if ( !isASide && (_activeInputs[0] != -1) ) {
                ViewerInstance* isViewer = isEffectViewer();
                if (isViewer) {
                    OpenGLViewerI* viewerUI = isViewer->getUiContext();
                    if (viewerUI) {
                        ViewerCompositingOperatorEnum op = viewerUI->getCompositingOperator();
                        if (op == eViewerCompositingOperatorNone) {
                            viewerUI->setCompositingOperator(eViewerCompositingOperatorWipeUnder);
                        }
                    }
                }
                _activeInputs[1] = inputNbChanged;
            } else {
                _activeInputs[0] = inputNbChanged;
                if (_activeInputs[1] == -1) {
                    _activeInputs[1] = inputNbChanged;
                }
            }
        }
    }
    Q_EMIT activeInputsChanged();
    Q_EMIT refreshOptionalState();
}

int
InspectorNode::getPreferredInputInternal(bool connected) const
{
    bool useInputA = false;
    if (!connected) {
        // For the merge node, use the preference (only when not connected)
        useInputA = appPTR->getCurrentSettings()->useInputAForMergeAutoConnect();
    }

    ///Find an input named A
    std::string inputNameToFind, otherName;

    if ( useInputA || (getPluginID() == PLUGINID_OFX_SHUFFLE && getMajorVersion() < 3) ) {
        inputNameToFind = "A";
        otherName = "B";
    } else {
        inputNameToFind = "B";
        otherName = "A";
    }
    int foundOther = -1;
    int maxinputs = getNInputs();
    for (int i = 0; i < maxinputs; ++i) {
        std::string inputLabel = getInputLabel(i);
        if (inputLabel == inputNameToFind) {
            NodePtr inp = getInput(i);
            if ( (connected && inp) || (!connected && !inp) ) {
                return i;
            }
        } else if (inputLabel == otherName) {
            foundOther = i;
        }
    }
    if (foundOther != -1) {
        NodePtr inp = getInput(foundOther);
        if ( (connected && inp) || (!connected && !inp) ) {
            return foundOther;
        }
    }

    int maxInputs = getNInputs();
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr inp = getInput(i);
        if ( (!inp && !connected) || (inp && connected) ) {
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

void
InspectorNode::getActiveInputs(int & a,
                               int &b) const
{
    QMutexLocker l(&_activeInputsMutex);

    a = _activeInputs[0];
    b = _activeInputs[1];
}

void
InspectorNode::setInputA(int inputNb)
{
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&_activeInputsMutex);
        _activeInputs[0] = inputNb;
    }
    Q_EMIT refreshOptionalState();
}

void
InspectorNode::setInputB(int inputNb)
{
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&_activeInputsMutex);
        _activeInputs[1] = inputNb;
    }
    Q_EMIT refreshOptionalState();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Node.cpp"
