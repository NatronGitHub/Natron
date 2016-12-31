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
#include "Engine/TLSHolder.h"
#include "Engine/NodeMetadata.h"
#include "Engine/OSGLContext.h"
#include "Engine/ViewIdx.h"
#include "Engine/EffectInstanceTLSData.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

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



    /**
     * @brief Helper function in the implementation of renderRoI to determine from the planes requested
     * what planes can actually be rendered from this node. Pass-through planes are rendered from upstream
     * nodes.
     **/
    RenderRoIRetCode determinePlanesToRender(const EffectInstance::RenderRoIArgs& args,
                                             std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                             std::list<ImageComponents> *planesToRender,
                                             std::bitset<4>* processChannels,
                                             std::map<ImageComponents, ImagePtr>* outputPlanes);

    /**
     * @brief Helper function in the implementation of renderRoI to handle identity effects
     **/
    RenderRoIRetCode handleIdentityEffect(const EffectInstance::RenderRoIArgs& args,
                                          double par,
                                          const RectD& rod,
                                          const std::list<ImageComponents> &requestedComponents,
                                          const std::map<int, std::list<ImageComponents> >& neededComps,
                                          RenderRoIResults* outputPlanes,
                                          bool *isIdentity);

    /**
     * @brief Helper function in the implementation of renderRoI to handle effects that can concatenate (distorsion etc...)
     **/
    RenderRoIRetCode handleConcatenation(const EffectInstance::RenderRoIArgs& args,
                                         const RenderScale& renderMappedScale,
                                         RenderRoIResults* results,
                                         bool *concatenated);


   
    /**
     * @brief Helper function in the implementation of renderRoI to determine the image backend (OpenGL, CPU...)
     **/
    RenderRoIRetCode resolveRenderBackend(const RenderRoIArgs & args,
                                          const FrameViewRequestPtr& requestPassData,
                                          const RectI& roi,
                                          RenderBackendTypeEnum* renderBackend,
                                          OSGLContextPtr *glRenderContext);

    /**
     * @brief Helper function in the implementation of renderRoI to determine if a render should use the Cache or not.
     * @returns The cache access type, i.e: none, write only or read/write
     **/
    CacheAccessModeEnum shouldRenderUseCache(const RenderRoIArgs & args,
                                             const FrameViewRequestPtr& requestPassData,
                                             RenderBackendTypeEnum backend);

    void fetchOrCreateOutputPlanes(const RenderRoIArgs & args,
                                   const FrameViewRequestPtr& requestPassData,
                                   CacheAccessModeEnum cacheAccess,
                                   const ImagePlanesToRenderPtr &planesToRender,
                                   const std::list<ImageComponents>& requestedComponents,
                                   const RectI& roi,
                                   const RenderScale& renderMappedScale,
                                   std::map<ImageComponents, ImagePtr>* outputPlanes);



    EffectInstance::RenderRoIStatusEnum renderRoILaunchInternalRender(const RenderRoIArgs & args,
                                                                      const ImagePlanesToRenderPtr &planesToRender,
                                                                      const OSGLContextAttacherPtr& glRenderContext,
                                                                      const RectD& rod,
                                                                      const RenderScale& renderMappedScale,
                                                                      const std::bitset<4> &processChannels);

    void ensureImagesToRequestedScale(const RenderRoIArgs & args,
                                      const ImagePlanesToRenderPtr &planesToRender,
                                      const RectI& roi,
                                      std::map<ImageComponents, ImagePtr>* outputPlanes);



    struct TiledRenderingFunctorArgs
    {
        bool renderFullScaleThenDownscale;
        bool isSequentialRender;
        bool isRenderResponseToUserInteraction;
        int preferredInput;
        unsigned int mipMapLevel;
        unsigned int renderMappedMipMapLevel;
        RectD rod;
        TimeValue time;
        ViewIdx view;
        double par;
        ImageBitDepthEnum outputClipPrefDepth;
        ImageComponents outputClipPrefsComps;
        bool byPassCache;
        std::bitset<4> processChannels;
        ImagePlanesToRenderPtr planes;
        OSGLContextPtr glContext;
    };

    void tryShrinkRenderWindow(const EffectInstanceTLSDataPtr &tls,
                               const EffectInstance::RectToRender & rectToRender,
                               const PlaneToRender & firstPlaneToRender,
                               bool renderFullScaleThenDownscale,
                               unsigned int renderMappedMipMapLevel,
                               unsigned int mipMapLevel,
                               double par,
                               const RectD& rod,
                               RectI &renderMappedRectToRender,
                               RectI &downscaledRectToRender,
                               bool *isBeingRenderedElseWhere,
                               bool *bitmapMarkedForRendering);



    RenderingFunctorRetEnum tiledRenderingFunctor(TiledRenderingFunctorArgs & args,  const RectToRender & specificData,
                                                  QThread* callingThread);

    ///These are the image passed to the plug-in to render
    /// - fullscaleMappedImage is the fullscale image remapped to what the plugin can support (components/bitdepth)
    /// - downscaledMappedImage is the downscaled image remapped to what the plugin can support (components/bitdepth wise)
    /// - fullscaleMappedImage is pointing to "image" if the plug-in does support the renderscale, meaning we don't use it.
    /// - Similarily downscaledMappedImage is pointing to "downscaledImage" if the plug-in doesn't support the render scale.
    ///
    /// - renderMappedImage is what is given to the plug-in to render the image into,it is mapped to an image that the plug-in
    ///can render onto (good scale, good components, good bitdepth)
    ///
    /// These are the possible scenarios:
    /// - 1) Plugin doesn't need remapping and doesn't need downscaling
    ///    * We render in downscaledImage always, all image pointers point to it.
    /// - 2) Plugin doesn't need remapping but needs downscaling (doesn't support the renderscale)
    ///    * We render in fullScaleImage, fullscaleMappedImage points to it and then we downscale into downscaledImage.
    ///    * renderMappedImage points to fullScaleImage
    /// - 3) Plugin needs remapping (doesn't support requested components or bitdepth) but doesn't need downscaling
    ///    * renderMappedImage points to downscaledMappedImage
    ///    * We render in downscaledMappedImage and then convert back to downscaledImage with requested comps/bitdepth
    /// - 4) Plugin needs remapping and downscaling
    ///    * renderMappedImage points to fullScaleMappedImage
    ///    * We render in fullScaledMappedImage, then convert into "image" and then downscale into downscaledImage.
    RenderingFunctorRetEnum tiledRenderingFunctor(const RectToRender & rectToRender,
                                                  const OSGLContextPtr& glContext,
                                                  const bool renderFullScaleThenDownscale,
                                                  const bool isSequentialRender,
                                                  const bool isRenderResponseToUserInteraction,
                                                  const int preferredInput,
                                                  const unsigned int mipMapLevel,
                                                  const unsigned int renderMappedMipMapLevel,
                                                  const RectD & rod,
                                                  const TimeValue time,
                                                  const ViewIdx view,
                                                  const double par,
                                                  const bool byPassCache,
                                                  const ImageBitDepthEnum outputClipPrefDepth,
                                                  const ImageComponents & outputClipPrefsComps,
                                                  const std::bitset<4>& processChannels,
                                                  const ImagePlanesToRenderPtr & planes);


    RenderingFunctorRetEnum renderHandlerIdentity(const EffectInstanceTLSDataPtr& tls,
                                                  const RectToRender & rectToRender,
                                                  const OSGLContextPtr& glContext,
                                                  const bool renderFullScaleThenDownscale,
                                                  const RectI & renderMappedRectToRender,
                                                  const RectI & downscaledRectToRender,
                                                  const ImageBitDepthEnum outputClipPrefDepth,
                                                  const TimeValue time,
                                                  const ViewIdx view,
                                                  const unsigned int mipMapLevel,
                                                  const TimeLapsePtr& timeRecorder,
                                                  ImagePlanesToRender & planes);

    RenderingFunctorRetEnum renderHandlerInternal(const EffectInstanceTLSDataPtr& tls,
                                                  const OSGLContextPtr& glContext,
                                                  EffectInstance::RenderActionArgs &actionArgs,
                                                  const ImagePlanesToRenderPtr & planes,
                                                  bool multiPlanar,
                                                  bool bitmapMarkedForRendering,
                                                  const ImageComponents & outputClipPrefsComps,
                                                  const ImageBitDepthEnum outputClipPrefDepth,
                                                  std::map<ImageComponents, PlaneToRender>& outputPlanes,
                                                  boost::shared_ptr<OSGLContextAttacher>* glContextAttacher);

    void setupRenderArgs(const EffectInstanceTLSDataPtr& tls,
                         const OSGLContextPtr& glContext,
                         const TimeValue time,
                         const ViewIdx view,
                         unsigned int mipMapLevel,
                         bool isSequentialRender,
                         bool isRenderResponseToUserInteraction,
                         bool byPassCache,
                         const ImagePlanesToRender & planes,
                         const RectI & renderMappedRectToRender,
                         const std::bitset<4>& processChannels,
                         const InputImagesMap& inputImages,
                         EffectInstance::RenderActionArgs &actionArgs,
                         boost::shared_ptr<OSGLContextAttacher>* glContextAttacher,
                         TimeLapsePtr *timeRecorder);

    void renderHandlerPostProcess(const EffectInstanceTLSDataPtr& tls,
                                  int preferredInput,
                                  const OSGLContextPtr& glContext,
                                  const EffectInstance::RenderActionArgs &actionArgs,
                                  const ImagePlanesToRender & planes,
                                  const RectI& downscaledRectToRender,
                                  const TimeLapsePtr& timeRecorder,
                                  bool renderFullScaleThenDownscale,
                                  unsigned int mipMapLevel,
                                  const std::map<ImageComponents, PlaneToRender>& outputPlanes,
                                  const std::bitset<4>& processChannels);


    void checkMetadata(NodeMetadata &metadata);

    void refreshMetadaWarnings(const NodeMetadata &metadata);
};



NATRON_NAMESPACE_EXIT;

#endif // Engine_EffectInstancePrivate_h
