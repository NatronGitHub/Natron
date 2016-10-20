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


#ifndef NATRON_ENGINE_NODE_PRIVATE_H
#define NATRON_ENGINE_NODE_PRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <list>
#include "Engine/EngineFwd.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/GenericSchedulerThreadWatcher.h"


#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>



#define kNodeParamProcessAllLayers "processAllLayers"
#define kNodeParamProcessAllLayersLabel "All Layers"
#define kNodeParamProcessAllLayersHint "When checked all layers in input will be processed and output to the same layer as in input. It is useful for example to apply a Transform effect on all layers."


NATRON_NAMESPACE_ENTER


/*The output node was connected from inputNumber to this...*/
typedef std::map<NodeWPtr, int > DeactivatedState;
typedef std::list<Node::KnobLink> KnobLinkList;
typedef std::vector<NodeWPtr> InputsV;


class ChannelSelector
{
public:

    KnobChoiceWPtr layer;
    mutable QMutex compsMutex;

    //Stores the components available at build time of the choice menu
    EffectInstance::ComponentsAvailableMap compsAvailable;

    ChannelSelector()
    : layer()
    , compsMutex()
    , compsAvailable()
    {
    }

    ChannelSelector(const ChannelSelector& other)
    {
        *this = other;
    }

    void operator=(const ChannelSelector& other)
    {
        layer = other.layer;
        QMutexLocker k(&compsMutex);
        compsAvailable = other.compsAvailable;
    }
};

class MaskSelector
{
public:

    KnobBoolWPtr enabled;
    KnobChoiceWPtr channel;
    mutable QMutex compsMutex;
    //Stores the components available at build time of the choice menu
    std::vector<std::pair<ImageComponents, NodeWPtr > > compsAvailable;

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


struct NodePrivate
{
    Q_DECLARE_TR_FUNCTIONS(Node)

public:
    NodePrivate(Node* publicInterface,
                   const AppInstancePtr& app_,
                   const NodeCollectionPtr& collection,
                const PluginPtr& plugin_);

    void abortPreview_non_blocking();

    void abortPreview_blocking(bool allowPreviewRenders);

    bool checkForExitPreview();

    void setComputingPreview(bool v)
    {
        QMutexLocker l(&computingPreviewMutex);

        computingPreview = v;
    }

    void runOnNodeCreatedCB(bool userEdited);

    void runOnNodeDeleteCB();

    bool figureOutCallbackName(const std::string& inCallback, std::string* outCallback);

    void runChangedParamCallback(const std::string& cb, const KnobIPtr& k, bool userEdited);

    void runOnNodeCreatedCBInternal(const std::string& cb, bool userEdited);

    void runOnNodeDeleteCBInternal(const std::string& cb);

    void runInputChangedCallback(int index, const std::string& script);

    void createChannelSelector(int inputNb, const std::string & inputName, bool isOutput, const KnobPagePtr& page, KnobIPtr* lastKnobBeforeAdvancedOption);

    void onLayerChanged(bool isOutput);

    void onMaskSelectorChanged(int inputNb, const MaskSelector& selector);

    ImageComponents getSelectedLayerInternal(int inputNb, const ChannelSelector& selector) const;

    void refreshDefaultPagesOrder();

    void refreshDefaultViewerKnobsOrder();


    ////////////////////////////////////////////////

    // Ptr to public interface, can not be a smart ptr
    Node* _publicInterface;

    // The group containing this node
    boost::weak_ptr<NodeCollection> group;

    // If this node is part of a precomp, this is a pointer to it
    boost::weak_ptr<PrecompNode> precomp;

    // pointer to the app: needed to access project stuff
    AppInstanceWPtr app;

    // If true, the node is serialized
    bool isPersistent;
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
    std::vector< std::list<ImageComponents> > inputsComponents;
    std::list<ImageComponents> outputComponents;
    mutable QMutex nameMutex;
    mutable QMutex inputsLabelsMutex;
    std::vector<std::string> inputLabels; // inputs name, protected by inputsLabelsMutex
    std::vector<std::string> inputHints; // protected by inputsLabelsMutex
    std::vector<bool> inputsVisibility; // protected by inputsMutex
    std::string scriptName; //node name internally and as visible to python
    std::string label; // node label as visible in the GUI

    DeactivatedState deactivatedState;
    mutable QMutex activatedMutex;
    bool activated;

    // the plugin which stores the function to instantiate the effect
    PluginWPtr plugin;

    PluginWPtr pyPlugHandle;
    bool isPyPlug;

    bool computingPreview;
    bool previewThreadQuit;
    mutable QMutex computingPreviewMutex;
    size_t pluginInstanceMemoryUsed; //< global count on all EffectInstance's of the memory they use.
    QMutex memoryUsedMutex; //< protects _pluginInstanceMemoryUsed
    int mustQuitPreview;
    QMutex mustQuitPreviewMutex;
    QWaitCondition mustQuitPreviewCond;
    QMutex renderInstancesSharedMutex; //< see eRenderSafetyInstanceSafe in EffectInstance::renderRoI
    mutable QMutex masterNodeMutex; //< protects masterNode and nodeLinks
    NodeWPtr masterNode; //< this points to the master when the node is a clone
    KnobLinkList nodeLinks; //< these point to the parents of the params links

    //When creating a Reader or Writer node, this is a pointer to the "bundle" node that the user actually see.
    NodeWPtr ioContainer;


    KnobIntWPtr frameIncrKnob;

    // PyPlug page
    KnobPageWPtr pyPlugPage;
    KnobStringWPtr pyPlugIDKnob, pyPlugDescKnob, pyPlugGroupingKnob, pyPlugLabelKnob;
    KnobFileWPtr pyPlugIconKnob, pyPlugExtPythonScript;
    KnobBoolWPtr pyPlugDescIsMarkdownKnob;
    KnobIntWPtr pyPlugVersionKnob;
    KnobIntWPtr pyPlugShortcutKnob;
    KnobButtonWPtr pyPlugExportButtonKnob;

    KnobGroupWPtr pyPlugExportDialog;
    KnobFileWPtr pyPlugExportDialogFile;
    KnobButtonWPtr pyPlugExportDialogOkButton, pyPlugExportDialogCancelButton;

    KnobStringWPtr nodeLabelKnob, ofxSubLabelKnob;
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
    std::map<int, ChannelSelector> channelsSelectors;
    KnobBoolWPtr processAllLayersKnob;
    std::map<int, MaskSelector> maskSelectors;
    mutable QMutex imagesBeingRenderedMutex;
    QWaitCondition imageBeingRenderedCond;
    std::list< ImagePtr > imagesBeingRendered; ///< a list of all the images being rendered simultaneously
    std::list <ImageBitDepthEnum> supportedDepths;

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
    boost::weak_ptr<NodeGuiI> guiPointer;
    std::list<HostOverlayKnobsPtr> nativeOverlays;
    bool nodeCreated;
    bool wasCreatedSilently;
    mutable QMutex createdComponentsMutex;
    std::list<ImageComponents> createdComponents; // comps created by the user
    boost::weak_ptr<RotoDrawableItem> paintStroke;

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

    // During painting this is the buffer we use
    ImagePtr paintBuffer;

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

    // UI
    mutable QMutex nodeUIDataMutex;
    double nodePositionCoords[2]; // x,y  X=Y=INT_MIN if there is no position info
    double nodeSize[2]; // width, height, W=H=-1 if there is no size info
    double nodeColor[3]; // node color (RGB), between 0. and 1. If R=G=B=-1 then no color
    double overlayColor[3]; // overlay color (RGB), between 0. and 1. If R=G=B=-1 then no color
    bool nodeIsSelected; // is this node selected by the user ?

    // The name of the preset with which this node was created
    mutable QMutex nodePresetMutex;
    std::string initialNodePreset;

    // This is a list of KnobPage script-names defining the ordering of the pages in the settings panel.
    // This is used to determine if the ordering has changed or not for serialization purpose
    std::list<std::string> defaultPagesOrder;

    // This is a list of Knob script-names which are by default in the viewer interface.
    // This is used to determine if the ordering has changed or not for serialization purpose
    std::list<std::string> defaultViewerKnobsOrder;

    bool restoringDefaults;

    // Used to determine which knobs are presets, so that if the user switch preset we remove them
    bool isLoadingPreset;
    std::list<KnobIWPtr> presetKnobs;

    bool hostChannelSelectorEnabled;
};

class RefreshingInputData_RAII
{
    NodePrivate *_imp;

public:

    RefreshingInputData_RAII(NodePrivate* imp)
    : _imp(imp)
    {
        ++_imp->isRefreshingInputRelatedData;
    }

    ~RefreshingInputData_RAII()
    {
        --_imp->isRefreshingInputRelatedData;
    }
};



class ComputingPreviewSetter_RAII
{
    NodePrivate* _imp;

public:
    ComputingPreviewSetter_RAII(NodePrivate* imp)
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

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_NODE_PRIVATE_H
