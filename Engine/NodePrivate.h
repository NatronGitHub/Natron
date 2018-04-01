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

#ifndef NATRON_ENGINE_NODE_PRIVATE_H
#define NATRON_ENGINE_NODE_PRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Node.h"

#include <QtCore/QWaitCondition>
#include <QtCore/QReadWriteLock>
#include <QtCore/QMutex>

#include "Engine/Hash64.h"

NATRON_NAMESPACE_ENTER

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

struct FormatKnob
{
    KnobIntWPtr size;
    KnobDoubleWPtr par;
    KnobChoiceWPtr formatChoice;
};


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

    void restoreUserKnobsRecursive(const std::list<KnobSerializationBasePtr>& knobs,
                                   const KnobGroupPtr& group,
                                   const KnobPagePtr& page);

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

    void createChannelSelector(int inputNb, const std::string & inputName, bool isOutput, const KnobPagePtr& page, KnobIPtr* lastKnobBeforeAdvancedOption);

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
    RotoContextPtr rotoContext; //< valid when the node has a rotoscoping context (i.e: paint context)
    TrackerContextPtr trackContext;
    mutable QMutex imagesBeingRenderedMutex;
    QWaitCondition imagesBeingRenderedCond;
    std::list<ImagePtr> imagesBeingRendered; ///< a list of all the images being rendered simultaneously
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
    std::list<HostOverlayKnobsPtr> nativeOverlays;
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
    NodeRenderWatcherPtr renderWatcher;
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


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_NODE_PRIVATE_H
