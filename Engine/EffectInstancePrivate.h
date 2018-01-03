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
#include "Engine/EffectDescription.h"
#include "Engine/TLSHolder.h"
#include "Engine/NodeMetadata.h"
#include "Engine/OSGLContext.h"
#include "Engine/ViewIdx.h"
#include "Engine/EffectInstance.h"
#include "Engine/EffectInstanceTLSData.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct RectToRender
{
    // The region in pixels to render
    RectI rect;

    // If this region is identity, this is the
    // input time/view on which we should copy the image
    int identityInputNumber;
    TimeValue identityTime;
    ViewIdx identityView;
    ImagePlaneDesc identityPlane;

    RectToRender()
    : rect()
    , identityInputNumber(-1)
    , identityTime(0)
    , identityView(0)
    , identityPlane()
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
    KnobFileWPtr pyPlugExportDialogFile;
    KnobButtonWPtr pyPlugExportButtonKnob;
    KnobButtonWPtr pyPlugConvertToGroupButtonKnob;

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
    KnobLayersWPtr createPlanesKnob;


    DefaultKnobs()
    {

    }

};

struct RenderDefaultKnobs
{
    KnobBoolWPtr disableNodeKnob;
    KnobIntWPtr lifeTimeKnob;
    KnobDoubleWPtr mixWithSource;
    KnobBoolWPtr enableLifeTimeKnob;

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

    // When true descriptor cannot be changed by the plug-in
    bool descriptorLocked;

    // Properties of the main instance
    EffectDescriptionPtr descriptor;

    // If this node is part of a RotoPaint item implementation
    // this is a pointer to the roto item itself
    boost::weak_ptr<RotoDrawableItem> paintStroke;

    // During painting we keep track of the image that was rendered
    // at the previous step so that we can accumulate the renders
    mutable QMutex accumBufferMutex;
    std::map<ImagePlaneDesc,ImagePtr> accumBuffer;

    // Active Viewer interacts, only accessed on the main thread
    std::list<OverlayInteractBasePtr> interacts;

    // Active Timeline interacts, only accesses on the main thread
    std::list<OverlayInteractBasePtr> timelineInteracts;

    // Weak pointer to the node holding this EffectInstance.
    // The node itself manages the lifetime of the main-thread EffectInstance.
    // However, render clones are managed themselves by the main EffectInstance.
    // To ensure that the node pointer is not NULL suddenly on a render thread because the main-thread is destroying it,
    // we keep another shared pointer for render clones only, in  RenderCloneData
    NodeWPtr node;

    EffectInstanceCommonData()
    : attachedContextsMutex(QMutex::Recursive)
    , attachedContexts()
    , pluginsPropMutex()
    , descriptorLocked(false)
    , descriptor(new EffectDescription)
    , paintStroke()
    , accumBufferMutex()
    , accumBuffer()
    , interacts()
    , timelineInteracts()
    , node()
    {

    }

};

typedef std::map<FrameViewPair, EffectInstanceWPtr, FrameView_compare_less> FrameViewEffectMap;

struct FrameViewKey
{
    unsigned int mipMapLevel;
    RenderScale proxyScale;
    ImagePlaneDesc plane;
};

struct FrameViewKey_Compare
{
    bool operator() (const FrameViewKey& lhs, const FrameViewKey& rhs) const
    {
        if (lhs.mipMapLevel < rhs.mipMapLevel) {
            return true;
        } else if (lhs.mipMapLevel > rhs.mipMapLevel) {
            return false;
        }

        if (lhs.proxyScale.x < rhs.proxyScale.x) {
            return true;
        } else if (lhs.proxyScale.x > rhs.proxyScale.x) {
            return false;
        }

        if (lhs.proxyScale.y < rhs.proxyScale.y) {
            return true;
        } else if (lhs.proxyScale.y > rhs.proxyScale.y) {
            return false;
        }

        // This will compare the planeID: color plane is equal even if e.g: lhs is Alpha and rhs is RGBA
        if (lhs.plane < rhs.plane) {
            return true;
        } else if (lhs.plane > rhs.plane) {
            return false;
        }
        
        
        return false;
    }
};



typedef std::map<FrameViewKey, FrameViewRequestWPtr, FrameViewKey_Compare> FrameViewRequestMap;

// Data specific to a render clone
struct RenderCloneData
{
    // Protects data in this structure accross multiple render threads
    mutable QMutex lock;

    // Used to lock out render instances when the plug-in render thread safety is set to eRenderSafetyInstanceSafe
    mutable QMutex instanceSafeRenderMutex;

    // Frozen state of the main-instance inputs.
    // They can only be used to determine the state of the graph
    // To recurse upstream on render effects, use renderInputs instead
    std::vector<EffectInstanceWPtr> mainInstanceInputs;

    // These are the render clones in input for each frame/view
    std::vector<FrameViewEffectMap> renderInputs;

    // All requests made on the clone
    FrameViewRequestMap requests;

    // The results of the get frame range action for this render
    GetFrameRangeResultsPtr frameRangeResults;

    // The time invariant metadas for the render
    GetTimeInvariantMetadataResultsPtr metadataResults;

    // A shared pointer to the node, to ensure it does not get deleted while rendering
    NodePtr node;

    RenderCloneData()
    : lock()
    , instanceSafeRenderMutex()
    , mainInstanceInputs()
    , renderInputs()
    , requests()
    , frameRangeResults()
    , metadataResults()
    , node()
    {

    }

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

    // Pointer to the effect description. For the effect main instance, this points to the description in the common structure.
    // For a render clone ,this is a copy of the main instance data.
    EffectDescriptionPtr descriptionPtr;

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
    
    void fetchSubLabelKnob();


    /**
     * @brief Same as getFrameViewRequest excepts that if it does not exist it will create it.
     * @returns True if it was created, false otherwise
     **/
    FrameViewRequestPtr createFrameViewRequest(TimeValue time,
                                ViewIdx view,
                                const RenderScale& proxyScale,
                                unsigned int mipMapLevel,
                                const ImagePlaneDesc& plane);


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
    void setTimeInvariantMetadataResults(const GetTimeInvariantMetadataResultsPtr& metadata);

    /**
     * @brief Get the results of the getFrameRange action for this render
     **/
    GetTimeInvariantMetadataResultsPtr getTimeInvariantMetadataResults() const;
    

    /**
     * @brief Helper function in the implementation of renderRoI to determine from the planes requested
     * what planes can actually be rendered from this node. Pass-through planes are rendered from upstream
     * nodes.
     **/
    ActionRetCodeEnum handlePassThroughPlanes(const FrameViewRequestPtr& requestData,
                                              const TreeRenderExecutionDataPtr& requestPassSharedData,
                                              const RectD& roiCanonical,
                                              AcceptedRequestConcatenationFlags concatenationFlags,
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
                                           const TreeRenderExecutionDataPtr& requestPassSharedData,
                                           AcceptedRequestConcatenationFlags concatenationFlags,
                                           bool *isIdentity);

    /**
     * @brief Helper function in the implementation of renderRoI to handle effects that can concatenate (distortion etc...)
     **/
    ActionRetCodeEnum handleConcatenation(const TreeRenderExecutionDataPtr& requestPassSharedData,
                                          const FrameViewRequestPtr& requestData,
                                          AcceptedRequestConcatenationFlags concatenationFlags,
                                          const RenderScale& renderScale,
                                          const RectD& canonicalRoi,
                                          bool draftRender,
                                          bool *concatenated);


    /**
     * @brief Returns accepted concatenations with the given input nb
     **/
    AcceptedRequestConcatenationFlags getConcatenationFlagsForInput(int inputNb) const;


   
    /**
     * @brief Helper function in the implementation of renderRoI to determine the image backend (OpenGL, CPU...)
     **/
    ActionRetCodeEnum resolveRenderBackend(const TreeRenderExecutionDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestPassData, const RectI& roi, CacheAccessModeEnum *cachePolicy, RenderBackendTypeEnum* renderBackend);

    /**
     * @brief Helper function in the implementation of renderRoI to determine if a render should use the Cache or not.
     * @returns The cache access type, i.e: none, write only or read/write
     **/
    CacheAccessModeEnum shouldRenderUseCache(const TreeRenderExecutionDataPtr& requestPassSharedData, const FrameViewRequestPtr& requestPassData);

    /**
     * @brief If a plug-in is using host frame-threading, potentially concurrent threads are calling getImagePlane().
     * If they all need to perform modification to the same image, we need to return individual images (and FrameViewRequest).
     **/
    ActionRetCodeEnum launchIsolatedRender(const FrameViewRequestPtr& requestPassData);

    ActionRetCodeEnum handleUpstreamFramesNeeded(const TreeRenderExecutionDataPtr& requestPassSharedData,
                                                 const FrameViewRequestPtr& requestPassData,
                                                 const RenderScale& proxyScale,
                                                 unsigned int mipMapLevel,
                                                 const RectD& roi,
                                                 const std::map<int, std::list<ImagePlaneDesc> >& neededInputLayers);


    bool canSplitRenderWindowWithIdentityRectangles(const RenderScale& renderMappedScale,
                                                    RectD* inputRoDIntersection);

    static RenderBackendTypeEnum storageModeToBackendType(StorageModeEnum storage);

    static StorageModeEnum storageModeFromBackendType(RenderBackendTypeEnum backend);

    ImagePtr createCachedImage(const RectI& roiPixels,
                               const std::vector<RectI>& perMipMapPixelRoD,
                               unsigned int mappedMipMapLevel,
                               const RenderScale& proxyScale,
                               const ImagePlaneDesc& plane,
                               RenderBackendTypeEnum backend,
                               CacheAccessModeEnum cachePolicy,
                               bool delayAllocation);




    /**
     * @brief Check if the given request has stuff left to render in the image plane or not.
     * @param renderRects In output the rectangles left to render (identity or plain render).
     * @param hasPendingTiles True if some tiles are pending from another render
     **/
    ActionRetCodeEnum checkRestToRender(bool updateTilesStateFromCache,
                                        const FrameViewRequestPtr& requestData,
                                        const RectI& renderMappedRoI,
                                        const RenderScale& renderMappedScale,
                                        const std::map<ImagePlaneDesc, ImagePtr>& producedImagePlanes,
                                        std::list<RectToRender>* renderRects,
                                        bool* hasPendingTiles);


    ActionRetCodeEnum launchRenderForSafetyAndBackend(const FrameViewRequestPtr& requestData,
                                                      const RenderScale& combinedScale,
                                                      RenderBackendTypeEnum backendType,
                                                      const std::list<RectToRender>& renderRects,
                                                      const std::map<ImagePlaneDesc, ImagePtr>& cachedPlanes);


    ActionRetCodeEnum launchPluginRenderAndHostFrameThreading(const FrameViewRequestPtr& requestData,
                                                              const OSGLContextPtr& glContext,
                                                              const EffectOpenGLContextDataPtr& glContextData,
                                                              const RenderScale& combinedScale,
                                                              RenderBackendTypeEnum backendType,
                                                              const std::list<RectToRender>& renderRects,
                                                              const std::map<ImagePlaneDesc, ImagePtr>& cachedPlanes);


    ActionRetCodeEnum lookupCachedImage(unsigned int mipMapLevel,
                                        const RenderScale& proxyScale,
                                        const ImagePlaneDesc& plane,
                                        const std::vector<RectI>& perMipMapPixelRoD,
                                        const RectI& pixelRoi,
                                        CacheAccessModeEnum cachePolicy,
                                        RenderBackendTypeEnum backend,
                                        bool readOnly,
                                        ImagePtr* image,
                                        bool* hasPendingTiles,
                                        bool* hasUnrenderedTiles);

    struct IdentityPlaneKey
    {
        int identityInputNb;
        ViewIdx identityView;
        TimeValue identityTime;
        ImagePlaneDesc identityPlane;
    };

    struct IdentityPlaneKeyCompare
    {
        bool operator()(const IdentityPlaneKey& lhs, const IdentityPlaneKey& rhs) const
        {
            if (lhs.identityInputNb < rhs.identityInputNb) {
                return true;
            } else if (lhs.identityInputNb > rhs.identityInputNb) {
                return false;
            }
            if (lhs.identityView < rhs.identityView) {
                return true;
            } else if (lhs.identityView > rhs.identityView) {
                return false;
            }
            if (lhs.identityTime < rhs.identityTime) {
                return true;
            } else if (lhs.identityTime > rhs.identityTime) {
                return false;
            }
            return lhs.identityPlane < rhs.identityPlane;
        }
    };

    typedef std::map<IdentityPlaneKey, ImagePtr, IdentityPlaneKeyCompare> IdentityPlanesMap;

    struct TiledRenderingFunctorArgs
    {
        FrameViewRequestPtr requestData;
        OSGLContextPtr glContext;
        EffectOpenGLContextDataPtr glContextData;
        RenderBackendTypeEnum backendType;
        std::map<ImagePlaneDesc, ImagePtr> cachedPlanes;

        // For identity rectangles, this is the input identity image for each plane
        IdentityPlanesMap identityPlanes;
    };

    ActionRetCodeEnum tiledRenderingFunctor(const RectToRender & rectToRender,
                                            const TiledRenderingFunctorArgs& args);


    ActionRetCodeEnum renderHandlerIdentity(const RectToRender & rectToRender,
                                            const TiledRenderingFunctorArgs& args);

    ActionRetCodeEnum renderHandlerPlugin(const RectToRender & rectToRender,
                                          const TiledRenderingFunctorArgs& args);

    ActionRetCodeEnum renderHandlerPostProcess(const RectToRender & rectToRender,
                                  const TiledRenderingFunctorArgs& args);


    void checkMetadata(NodeMetadata &metadata);

    void onMaskSelectorChanged(int inputNb, const MaskSelector& selector);

    ImagePlaneDesc getSelectedLayerInternal(const std::list<ImagePlaneDesc>& availableLayers,
                                            const ChannelSelector& selector) const;

    void onLayerChanged(bool isOutput);

    
};


NATRON_NAMESPACE_EXIT

#endif // Engine_EffectInstancePrivate_h
