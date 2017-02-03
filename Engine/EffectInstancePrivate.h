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

class EffectInstance::Implementation
{
    Q_DECLARE_TR_FUNCTIONS(EffectInstance)

public:
    Implementation(EffectInstance* publicInterface);

    Implementation(const Implementation& other);

    ~Implementation()
    {
        
    }

    // can not be a smart ptr
    EffectInstance* _publicInterface;

    mutable QMutex attachedContextsMutex;
    // A list of context that are currently attached (i.e attachOpenGLContext() has been called on them but not yet dettachOpenGLContext).
    // If a plug-in returns false to supportsConcurrentOpenGLRenders() then whenever trying to attach a context, we take a lock in attachOpenGLContext
    // that is released in dettachOpenGLContext so that there can only be a single attached OpenGL context at any time.
    std::map<OSGLContextWPtr, EffectOpenGLContextDataPtr> attachedContexts;

    // Render clones are very small copies holding just pointers to Knobs that are used to render plug-ins that are only
    // eRenderSafetyInstanceSafe or lower
    EffectInstancePtr mainInstance; // pointer to the main-instance if this instance is a clone

    // True if this intance is rendering. Only used if the plug-in is instance thread safe.
    // Protected by renderClonesMutex
    bool isDoingInstanceSafeRender;

    // Protects renderClonesPool
    mutable QMutex renderClonesMutex;

    // List of render clones if the main instance is not multi-thread safe
    std::list<EffectInstancePtr> renderClonesPool;

    // Thread local storage is requied to ensure Knob values and node inputs do not change during a single render.
    // In the future we should make copies of EffectInstance but for now this is the only way to deal with it.
    boost::shared_ptr<TLSHolder<EffectInstanceTLSData> > tlsData;


    static RenderScale getCombinedScale(unsigned int mipMapLevel, const RenderScale& proxyScale);

    /**
     * @brief Helper function in the implementation of renderRoI to determine from the planes requested
     * what planes can actually be rendered from this node. Pass-through planes are rendered from upstream
     * nodes.
     **/
    ActionRetCodeEnum handlePassThroughPlanes(const FrameViewRequestPtr& requestData,
                                              unsigned int mipMapLevel,
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
                                           bool *isIdentity);

    /**
     * @brief Helper function in the implementation of renderRoI to handle effects that can concatenate (distortion etc...)
     **/
    ActionRetCodeEnum handleConcatenation(const FrameViewRequestPtr& requestData,
                                          const FrameViewRequestPtr& requester,
                                          int inputNbInRequester,
                                          const RenderScale& renderScale,
                                          const RectD& canonicalRoi,
                                          bool *concatenated);


   
    /**
     * @brief Helper function in the implementation of renderRoI to determine the image backend (OpenGL, CPU...)
     **/
    ActionRetCodeEnum resolveRenderBackend(const FrameViewRequestPtr& requestPassData, const RectI& roi, RenderBackendTypeEnum* renderBackend);

    /**
     * @brief Helper function in the implementation of renderRoI to determine if a render should use the Cache or not.
     * @returns The cache access type, i.e: none, write only or read/write
     **/
    CacheAccessModeEnum shouldRenderUseCache(const FrameViewRequestPtr& requestPassData);

    ActionRetCodeEnum handleUpstreamFramesNeeded(const FrameViewRequestPtr& requestPassData,
                                                        const RenderScale& combinedScale,
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


    ActionRetCodeEnum allocateRenderBackendStorageForRenderRects(const FrameViewRequestPtr& requestData,
                                                                 const RectI& roiPixels,
                                                                 unsigned int mipMapLevel,
                                                                 const RenderScale& combinedScale,
                                                                 std::map<ImagePlaneDesc, ImagePtr> *producedPlanes,
                                                                 std::list<RectToRender>* renderRects);

    ActionRetCodeEnum launchInternalRender(const FrameViewRequestPtr& requestData,
                                           const RenderScale& combinedScale,
                                           const std::list<RectToRender>& renderRects,
                                           const std::map<ImagePlaneDesc, ImagePtr>& producedImagePlanes);


    ActionRetCodeEnum renderForClone(const FrameViewRequestPtr& requestData,
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

    ActionRetCodeEnum tiledRenderingFunctorInSeparateThread(const RectToRender & rectToRender,
                                                            const TiledRenderingFunctorArgs& args,
                                                            QThread* spawnerThread);


    ActionRetCodeEnum tiledRenderingFunctor(const RectToRender & rectToRender,
                                            const TiledRenderingFunctorArgs& args);


    ActionRetCodeEnum renderHandlerIdentity(const RectToRender & rectToRender,
                                            const RenderScale& combinedScale,
                                            const TiledRenderingFunctorArgs& args);

    ActionRetCodeEnum renderHandlerPlugin(const EffectInstanceTLSDataPtr& tls,
                                            const RectToRender & rectToRender,
                                            const RenderScale& combinedScale,
                                            const TiledRenderingFunctorArgs& args);

    void renderHandlerPostProcess(const RectToRender & rectToRender,
                                  const RenderScale& combinedScale,
                                  const TiledRenderingFunctorArgs& args);


    void checkMetadata(NodeMetadata &metadata);

};



NATRON_NAMESPACE_EXIT;

#endif // Engine_EffectInstancePrivate_h
