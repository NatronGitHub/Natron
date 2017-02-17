/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_EffectInstancePrivate_h
#define Engine_EffectInstancePrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "EffectInstance.h"

#include <map>
#include <list>
#include <string>

#include <QtCore/QCoreApplication>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>

#include "Global/GlobalDefines.h"

#include "Engine/Image.h"
#include "Engine/ImageStorage.h"
#include "Engine/TLSHolder.h"
#include "Engine/NodeMetadata.h"
#include "Engine/OSGLContext.h"
#include "Engine/ViewIdx.h"
#include "Engine/EffectInstance.h"
#include "Engine/EffectInstanceTLSData.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct RectToRender
{
    // The region in pixels to render
    RectI rect;

    // If this region is identity, this is the
    // input time/view on which we should copy the image
    int identityInputNumber;
    TimeValue identityTime;
    ViewIdx identityView;

    // The type of render for this rectangle
    RenderBackendTypeEnum backendType;

    // The temporary image used to render the rectangle for
    // each plane.
    std::map<ImagePlaneDesc, ImagePtr> tmpRenderPlanes;

    RectToRender()
    : rect()
    , identityInputNumber(-1)
    , identityTime(0)
    , identityView(0)
    , backendType(eRenderBackendTypeCPU)
    , tmpRenderPlanes()
    {

    }
};

struct ChannelSelector
{

    KnobChoiceWPtr layer;


};

struct MaskSelector
{

    KnobBoolWPtr enabled;
    KnobChoiceWPtr channel;

};


struct FormatKnob
{
    KnobIntWPtr size;
    KnobDoubleWPtr par;
    KnobChoiceWPtr formatChoice;
};

struct DefaultKnobs
{
    KnobIntWPtr frameIncrKnob;

    // PyPlug page
    KnobPageWPtr pyPlugPage;
    KnobStringWPtr pyPlugIDKnob, pyPlugDescKnob, pyPlugGroupingKnob, pyPlugLabelKnob;
    KnobFileWPtr pyPlugIconKnob, pyPlugExtPythonScript;
    KnobBoolWPtr pyPlugDescIsMarkdownKnob;
    KnobIntWPtr pyPlugVersionKnob;
    KnobIntWPtr pyPlugShortcutKnob;
    KnobButtonWPtr pyPlugExportButtonKnob;
    KnobButtonWPtr pyPlugConvertToGroupButtonKnob;

    KnobGroupWPtr pyPlugExportDialog;
    KnobFileWPtr pyPlugExportDialogFile;
    KnobButtonWPtr pyPlugExportDialogOkButton, pyPlugExportDialogCancelButton;

    KnobStringWPtr nodeLabelKnob, ofxSubLabelKnob;
    KnobBoolWPtr previewEnabledKnob;
    KnobChoiceWPtr openglRenderingEnabledKnob;
    KnobButtonWPtr keepInAnimationModuleKnob;
    KnobStringWPtr knobChangedCallback;
    KnobStringWPtr inputChangedCallback;
    KnobStringWPtr nodeCreatedCallback;
    KnobStringWPtr nodeRemovalCallback;
    KnobStringWPtr tableSelectionChangedCallback;
    KnobPageWPtr infoPage;
    KnobStringWPtr nodeInfos;
    KnobButtonWPtr refreshInfoButton;
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

    // True if the effect has isHostChannelSelectorSupported() returning true
    bool hostChannelSelectorEnabled;

    DefaultKnobs()
    : hostChannelSelectorEnabled(false)
    {

    }

};

struct RenderDefaultKnobs
{
    KnobBoolWPtr disableNodeKnob;
    KnobIntWPtr lifeTimeKnob;
    KnobBoolWPtr enableLifeTimeKnob;

};

struct DynamicProperties
{
    // The current render safety
    RenderSafetyEnum currentThreadSafety;

    // Does this node currently support tiled rendering
    bool currentSupportTiles;

    // Does this node currently support render scale
    bool currentSupportsRenderScale;

    // Does this node currently supports OpenGL render
    PluginOpenGLRenderSupport currentSupportOpenGLRender;

    // Does this node currently renders sequentially or not
    SequentialPreferenceEnum currentSupportSequentialRender;

    // Does this node can return a distortion function ?
    bool currentCanDistort;
    bool currentDeprecatedTransformSupport;

    DynamicProperties()
    : currentThreadSafety(eRenderSafetyInstanceSafe)
    , currentSupportTiles(false)
    , currentSupportsRenderScale(false)
    , currentSupportOpenGLRender(ePluginOpenGLRenderSupportNone)
    , currentSupportSequentialRender(eSequentialPreferenceNotSequential)
    , currentCanDistort(false)
    , currentDeprecatedTransformSupport(false)
    {

    }
};

// Data shared accross all clones
struct EffectInstanceCommonData
{
    mutable QMutex attachedContextsMutex;

    // A list of context that are currently attached (i.e attachOpenGLContext() has been called on them but not yet dettachOpenGLContext).
    // If a plug-in returns false to supportsConcurrentOpenGLRenders() then whenever trying to attach a context, we take a lock in attachOpenGLContext
    // that is released in dettachOpenGLContext so that there can only be a single attached OpenGL context at any time.
    std::map<OSGLContextWPtr, EffectOpenGLContextDataPtr> attachedContexts;


    // Protects all plug-in properties
    mutable QMutex pluginsPropMutex;

    // pluginSafety is the render thread safety declared by the plug-in.
    // The currentThreadSafety is either pointing to the same value as pluginSafety
    // or a lower value. This is used for example by Natron when painting with
    // a brush to ensure only 1 render thread is running.
    RenderSafetyEnum pluginSafety;

    // When true pluginSafety cannot be changed by the plug-in
    bool pluginSafetyLocked;

    DynamicProperties props;


    // Protects supportedInputComponents & supportedOutputComponents
    mutable QMutex supportedComponentsMutex;

    // The accepted number of components in input and in output of the plug-in
    // These two are also protected by inputsMutex
    // This is a bitset: each bit tells whether the plug-in supports N comps
    std::vector< std::bitset<4> > supportedInputComponents;
    std::bitset<4> supportedOutputComponents;


    // List of supported bitdepth by the plug-in
    std::list <ImageBitDepthEnum> supportedDepths;

    // Protects created component
    mutable QMutex createdPlanesMutex;

    // Comps created by the user in the stream.
    std::list<ImagePlaneDesc> createdPlanes;

    // If this node is part of a RotoPaint item implementation
    // this is a pointer to the roto item itself
    boost::weak_ptr<RotoDrawableItem> paintStroke;

    // During painting we keep track of the image that was rendered
    // at the previous step so that we can accumulate the renders
    mutable QMutex accumBufferMutex;
    ImagePtr accumBuffer;


    EffectInstanceCommonData()
    : attachedContextsMutex(QMutex::Recursive)
    , attachedContexts()
    , pluginsPropMutex()
    , pluginSafety(eRenderSafetyInstanceSafe)
    , pluginSafetyLocked(false)
    , props()
    , supportedComponentsMutex()
    , supportedInputComponents()
    , supportedOutputComponents()
    , supportedDepths()
    , createdPlanesMutex()
    , createdPlanes()
    , paintStroke()
    , accumBufferMutex()
    , accumBuffer()
    {

    }

};

typedef std::map<FrameViewPair, FrameViewRequestWPtr, FrameView_compare_less> NodeFrameViewRequestData;


// Data specific to a render clone
struct RenderCloneData
{
    // Protects data in this structure accross multiple render threads
    mutable QMutex lock;

    // Used to lock out render instances when the plug-in render thread safety is set to eRenderSafetyInstanceSafe
    mutable QMutex instanceSafeRenderMutex;

    // Render-local inputs vector
    std::vector<EffectInstanceWPtr> inputs;

    // This is the current frame/view being requested or rendered by the main render thread.
    FrameViewRequestWPtr currentFrameView;

    // Contains data for all frame/view pair that are going to be computed
    // for this frame/view pair with the overall RoI to avoid rendering several times with this node.
    // We don't hold a strong reference here so that when all listeners to the frame/view are done listening
    // the FrameViewRequest (hence the embedded image) is destroyed and resources released.
    NodeFrameViewRequestData frames;

    // The results of the get frame range action for this render
    GetFrameRangeResultsPtr frameRangeResults;

    // The time invariant metadas for the render
    GetTimeInvariantMetaDatasResultsPtr metadatasResults;

    // This is the hash used to cache time and view invariant stuff
    U64 timeViewInvariantHash;

    // Hash used for metadata (depdending only for parameters that
    // are metadata dependent)
    U64 metadataTimeInvariantHash;

    // Time/view variant hash
    FrameViewHashMap timeViewVariantHash;

    // Properties, local to this render
    DynamicProperties props;

    // True if hash is valid
    bool timeViewInvariantHashValid;

    bool metadataTimeInvariantHashValid;

    RenderCloneData()
    : lock()
    , instanceSafeRenderMutex()
    , inputs()
    , currentFrameView()
    , frames()
    , frameRangeResults()
    , metadatasResults()
    , timeViewInvariantHash(0)
    , metadataTimeInvariantHash(0)
    , timeViewVariantHash()
    , props()
    , timeViewInvariantHashValid(false)
    , metadataTimeInvariantHashValid(false)
    {

    }

    /**
     * @brief Get the time/view variant hash
     **/
    bool getFrameViewHash(TimeValue time, ViewIdx view, U64* hash) const;
    void setFrameViewHash(TimeValue time, ViewIdx view, U64 hash);


    /**
     * @brief Get the time and view invariant hash
     **/
    bool getTimeViewInvariantHash(U64* hash) const;
    void setTimeViewInvariantHash(U64 hash);


    /**
     * @brief Get the time and view invariant hash used for metadatas
     **/
    void setTimeInvariantMetadataHash(U64 hash);
    bool getTimeInvariantMetadataHash(U64* hash) const;


    

};

class EffectInstance::Implementation
{
    Q_DECLARE_TR_FUNCTIONS(EffectInstance)

public:


    // can not be a smart ptr
    EffectInstance* _publicInterface;

    // Common data shared accross the main instance and all render instances.
    boost::shared_ptr<EffectInstanceCommonData> common;

    // Data specific to a render clone. Each render clone is tied to a single render but these datas may be
    // accessed by multiple threads in the render.
    boost::scoped_ptr<RenderCloneData> renderData;

    // Default implementation knobs. They are shared with the main instance implementation.
    boost::shared_ptr<DefaultKnobs> defKnobs;

    // Knobs specific to each render clone
    RenderDefaultKnobs renderKnobs;

    // Register memory chunks by the plug-in. Kept here to avoid plug-in memory leaks
    mutable QMutex pluginMemoryChunksMutex;
    std::list<PluginMemoryPtr> pluginMemoryChunks;

public:

    Implementation(EffectInstance* publicInterface);

    Implementation(EffectInstance* publicInterface, const Implementation& other);

    ~Implementation();


    /**
     * @brief Returns a previously requested frame/view request from requestRender. This contains most actions
     * results for the frame/view as well as the RoI required to render on the effect for this particular frame/view pair.
     * The time passed in parameter should always be rounded for effects that are not continuous.
     **/
    FrameViewRequestPtr getFrameViewRequest(TimeValue time, ViewIdx view) const;

    /**
     * @brief Same as getFrameViewRequest excepts that if it does not exist it will create it.
     * @returns True if it was created, false otherwise
     **/
    bool getOrCreateFrameViewRequest(TimeValue time,
                                     ViewIdx view,
                                     const RenderScale& proxyScale,
                                     unsigned int mipMapLevel,
                                     const ImagePlaneDesc& plane,
                                     FrameViewRequestPtr* request);


    /**
     * @brief Set the results of the getFrameRange action for this render
     **/
    void setFrameRangeResults(const GetFrameRangeResultsPtr& range);

    /**
     * @brief Get the results of the getFrameRange action for this render
     **/
    GetFrameRangeResultsPtr getFrameRangeResults() const;

    /**
     * @brief Set the results of the getMetadata action for this render
     **/
    void setTimeInvariantMetadataResults(const GetTimeInvariantMetaDatasResultsPtr& metadatas);

    /**
     * @brief Get the results of the getFrameRange action for this render
     **/
    GetTimeInvariantMetaDatasResultsPtr getTimeInvariantMetadataResults() const;
    

    static RenderScale getCombinedScale(unsigned int mipMapLevel, const RenderScale& proxyScale);

    /**
     * @brief Helper function in the implementation of renderRoI to determine from the planes requested
     * what planes can actually be rendered from this node. Pass-through planes are rendered from upstream
     * nodes.
     **/
    ActionRetCodeEnum handlePassThroughPlanes(const FrameViewRequestPtr& requestData,
                                              const RequestPassSharedDataPtr& requestPassSharedData,
                                              const RectD& roiCanonical,
                                              std::map<int, std::list<ImagePlaneDesc> >* neededComp,
                                              bool *isPassThrough);

    /**
     * @brief Helper function in the implementation of renderRoI to handle identity effects
     **/
    ActionRetCodeEnum handleIdentityEffect(double par,
                                           const RectD& rod,
                                           const RenderScale& combinedScale,
                                           const RectD& canonicalRoi,
                                           const FrameViewRequestPtr& requestData,
                                           const RequestPassSharedDataPtr& requestPassSharedData,
                                           bool *isIdentity);

    /**
     * @brief Helper function in the implementation of renderRoI to handle effects that can concatenate (distortion etc...)
     **/
    ActionRetCodeEnum handleConcatenation(const RequestPassSharedDataPtr& requestPassSharedData,
                                          const FrameViewRequestPtr& requestData,
                                          const FrameViewRequestPtr& requester,
                                          int inputNbInRequester,
                                          const RenderScale& renderScale,
                                          const RectD& canonicalRoi,
                                          bool *concatenated);


   
    /**
     * @brief Helper function in the implementation of renderRoI to determine the image backend (OpenGL, CPU...)
     **/
    ActionRetCodeEnum resolveRenderBackend(const RequestPassSharedDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestPassData, const RectI& roi, RenderBackendTypeEnum* renderBackend);

    /**
     * @brief Helper function in the implementation of renderRoI to determine if a render should use the Cache or not.
     * @returns The cache access type, i.e: none, write only or read/write
     **/
    CacheAccessModeEnum shouldRenderUseCache(const RequestPassSharedDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestPassData);

    ActionRetCodeEnum handleUpstreamFramesNeeded(const RequestPassSharedDataPtr& requestPassSharedData,
                                                 const FrameViewRequestPtr& requestPassData,
                                                 const RenderScale& proxyScale,
                                                 unsigned int mipMapLevel,
                                                 const RectD& roi,
                                                 const std::map<int, std::list<ImagePlaneDesc> >& neededInputLayers);


    bool canSplitRenderWindowWithIdentityRectangles(const FrameViewRequestPtr& requestPassData,
                                                    const RenderScale& renderMappedScale,
                                                    RectD* inputRoDIntersection);


    ImagePtr fetchCachedTiles(const FrameViewRequestPtr& requestPassData,
                              const RectI& roiPixels,
                              unsigned int mappedMipMapLevel,
                              const ImagePlaneDesc& plane,
                              bool delayAllocation);




    /**
     * @brief Check if the given request has stuff left to render in the image plane or not.
     * @param renderRects In output the rectangles left to render (identity or plain render).
     * @param hasPendingTiles True if some tiles are pending from another render
     **/
    void checkRestToRender(const FrameViewRequestPtr& requestData,
                           const RectI& renderMappedRoI,
                           const RenderScale& renderMappedScale,
                           std::list<RectToRender>* renderRects,
                           bool* hasPendingTiles);


    ActionRetCodeEnum allocateRenderBackendStorageForRenderRects(const RequestPassSharedDataPtr& requestPassSharedData,
                                                                 const FrameViewRequestPtr& requestData,
                                                                 const RectI& roiPixels,
                                                                 unsigned int mipMapLevel,
                                                                 const RenderScale& combinedScale,
                                                                 std::map<ImagePlaneDesc, ImagePtr> *producedPlanes,
                                                                 std::list<RectToRender>* renderRects);

    ActionRetCodeEnum launchRenderForSafetyAndBackend(const FrameViewRequestPtr& requestData,
                                                      const RenderScale& combinedScale,
                                                      const std::list<RectToRender>& renderRects,
                                                      const std::map<ImagePlaneDesc, ImagePtr>& producedImagePlanes);


    ActionRetCodeEnum launchPluginRenderAndHostFrameThreading(const FrameViewRequestPtr& requestData,
                                                              const OSGLContextPtr& glContext,
                                                              const EffectOpenGLContextDataPtr& glContextData,
                                                              const RenderScale& combinedScale,
                                                              const std::list<RectToRender>& renderRects,
                                                              const std::map<ImagePlaneDesc, ImagePtr>& producedImagePlanes);
    
    struct TiledRenderingFunctorArgs
    {
        FrameViewRequestPtr requestData;
        OSGLContextPtr glContext;
        EffectOpenGLContextDataPtr glContextData;
        std::map<ImagePlaneDesc, ImagePtr> producedImagePlanes;
    };


    ActionRetCodeEnum tiledRenderingFunctor(const RectToRender & rectToRender,
                                            const TiledRenderingFunctorArgs& args);


    ActionRetCodeEnum renderHandlerIdentity(const RectToRender & rectToRender,
                                            const TiledRenderingFunctorArgs& args);

    ActionRetCodeEnum renderHandlerPlugin(const RectToRender & rectToRender,
                                          const RenderScale& combinedScale,
                                          const TiledRenderingFunctorArgs& args);

    void renderHandlerPostProcess(const RectToRender & rectToRender,
                                  const TiledRenderingFunctorArgs& args);


    void checkMetadata(NodeMetadata &metadata);

    void onMaskSelectorChanged(int inputNb, const MaskSelector& selector);

    ImagePlaneDesc getSelectedLayerInternal(const std::list<ImagePlaneDesc>& availableLayers,
                                            const ChannelSelector& selector) const;

    void onLayerChanged(bool isOutput);

    
};



NATRON_NAMESPACE_EXIT;

#endif // Engine_EffectInstancePrivate_h
