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

    // An effect instance is only allowed to render if it has a render data pointer.
    // When rendering Natron creates a copy of the "main instance" in a similar fashion as render clones
    // but the image effect itself is not copied. 
    TreeRenderNodeArgsPtr renderData;


    void
    determineRectsToRender(ImagePtr& isPlaneCached,
                           const ParallelRenderArgsPtr& frameArgs,
                           TimeValue time,
                           ViewIdx view,
                           RenderSafetyEnum safety,
                           const RenderScale& argsScale,
                           const RenderScale& renderMappedScale,
                           const RectI& roi,
                           const RectI& upscaledImageBounds,
                           const RectI& downscaledImageBounds,
                           bool renderFullScaleThenDownscale,
                           bool isDuringPaintStroke,
                           unsigned int mipMapLevel,
                           double par,
                           bool cacheIsAlmostFull,
                           const InputImagesMap& argsInputImageList,
                           StorageModeEnum storage,
                           const OSGLContextAttacherPtr& glContextLocker,
                           const EffectInstance::ImagePlanesToRenderPtr& planesToRender,
                           bool *redoCacheLookup,
                           bool *fillGrownBoundsWithZeroes,
                           bool *tryIdentityOptim,
                           RectI *lastStrokePixelRoD,
                           RectI *inputsRoDIntersectionPixel);

    RenderRoIRetCode determinePlanesToRender(const EffectInstance::RenderRoIArgs& args, std::list<ImageComponents> *requestedComponents, std::map<ImageComponents, ImagePtr>* outputPlanes);

    RenderRoIRetCode handleIdentityEffect(const EffectInstance::RenderRoIArgs& args,
                                          const ParallelRenderArgsPtr& frameArgs,
                                          const FrameViewRequest* requestPassData,
                                          RenderScaleSupportEnum supportsRS,
                                          U64 frameViewHash,
                                          double par,
                                          unsigned int mipMapLevel,
                                          ImagePremultiplicationEnum thisEffectOutputPremult,
                                          const RectD& rod,
                                          const std::list<ImageComponents> &requestedComponents,
                                          const std::vector<ImageComponents>& outputComponents,
                                          const ComponentsNeededMapPtr& neededComps,
                                          RenderScale& renderMappedScale,
                                          unsigned int &renderMappedMipMapLevel,
                                          bool &renderFullScaleThenDownscale,
                                          RenderRoIResults* outputPlanes,
                                          bool *isIdentity);

    bool handleConcatenation(const EffectInstance::RenderRoIArgs& args, const RenderScale& renderMappedScale, U64 frameViewHash, RenderRoIResults* results, RenderRoIRetCode* retCode);

    bool setupRenderRoIParams(const RenderRoIArgs & args,
                              EffectTLSDataPtr* tls,
                              U64 *frameViewHash,
                              AbortableRenderInfoPtr *abortInfo,
                              ParallelRenderArgsPtr* frameArgs,
                              OSGLContextPtr *glGpuContext,
                              OSGLContextPtr *glCpuContext,
                              double *par,
                              ImageFieldingOrderEnum *fieldingOrder,
                              ImagePremultiplicationEnum *thisEffectOutputPremult,
                              RenderScaleSupportEnum *supportsRS,
                              bool *renderFullScaleThenDownscale,
                              unsigned int *renderMappedMipMapLevel,
                              RenderScale *renderMappedScale,
                              const FrameViewRequest** requestPassData,
                              RectD* rod,
                              RectI* roi,
                              ComponentsNeededMapPtr *neededComps,
                              std::bitset<4> *processChannels,
                              const std::vector<ImageComponents>** outputComponents);

    bool resolveRenderDevice(const RenderRoIArgs & args,
                             const ParallelRenderArgsPtr& frameArgs,
                             const OSGLContextPtr& glGpuContext,
                             const OSGLContextPtr& glCpuContext,
                             const RectI& downscaleImageBounds,
                             RenderScale* renderMappedScale,
                             unsigned int *renderMappedMipMapLevel,
                             bool *renderFullScaleThenDownscale,
                             RectI* roi,
                             StorageModeEnum* storage,
                             OSGLContextPtr *glRenderContext,
                             OSGLContextAttacherPtr *glContextLocker,
                             bool *useOpenGL,
                             RenderRoIRetCode* resolvedError);

    bool renderRoILookupCacheFirstTime(const RenderRoIArgs & args,
                                       const ParallelRenderArgsPtr& frameArgs,
                                       const U64 frameViewHash,
                                       StorageModeEnum storage,
                                       const OSGLContextPtr& glRenderContext,
                                       const OSGLContextAttacherPtr& glContextLocker,
                                       const ImagePlanesToRenderPtr &planesToRender,
                                       bool isDuringPaintStrokeDrawing,
                                       const std::list<ImageComponents>& requestedComponents,
                                       const std::vector<ImageComponents>& outputComponents,
                                       const RectD& rod,
                                       const RectI& roi,
                                       const RectI& upscaledImageBounds,
                                       const RectI& downscaledImageBounds,
                                       bool renderFullScaleThenDownscale,
                                       unsigned int renderMipMapLevel,
                                       bool* createInCache,
                                       bool *renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                       ImagePtr *isPlaneCached,
                                       boost::scoped_ptr<ImageKey>* key,
                                       std::map<ImageComponents, ImagePtr>* outputPlanes);


    RenderRoIRetCode renderRoIRenderInputImages(const RenderRoIArgs & args,
                                                                const U64 frameViewHash,
                                                                const ComponentsNeededMapPtr& neededComps,
                                                                const FrameViewRequest* requestPassData,
                                                                const ImagePlanesToRenderPtr &planesToRender,
                                                                StorageModeEnum storage,
                                                                const std::vector<ImageComponents>& outputComponents,
                                                                ImagePremultiplicationEnum thisEffectOutputPremult,
                                                                bool renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                                boost::shared_ptr<FramesNeededMap>* framesNeeded);

    RenderRoIRetCode renderRoISecondCacheLookup(const RenderRoIArgs & args,
                                                                const ParallelRenderArgsPtr& frameArgs,
                                                                const ImagePlanesToRenderPtr &planesToRender,
                                                                const OSGLContextAttacherPtr& glContextLocker,
                                                                bool isDuringPaintStrokeDrawing,
                                                                RenderSafetyEnum safety,
                                                                StorageModeEnum storage,
                                                                const std::vector<ImageComponents>& outputComponents,
                                                                const RectD& rod,
                                                                const RectI& roi,
                                                                const RectI& upscaledImageBounds,
                                                                const RectI& downscaledImageBounds,
                                                                const RectI& inputsRoDIntersectionPixel,
                                                                bool tryIdentityOptim,
                                                                bool renderFullScaleThenDownscale,
                                                                bool createInCache,
                                                                unsigned int renderMappedMipMapLevel,
                                                                const RenderScale& renderMappedScale,
                                                                const boost::scoped_ptr<ImageKey>& key,
                                                                ImagePtr *isPlaneCached);

    void renderRoIAllocateOutputPlanes(const RenderRoIArgs & args,
                                       const ImagePlanesToRenderPtr &planesToRender,
                                       const OSGLContextAttacherPtr& glContextLocker,
                                       const OSGLContextPtr& glRenderContext,
                                       ImageFieldingOrderEnum fieldingOrder,
                                       ImageBitDepthEnum outputDepth,
                                       bool fillGrownBoundsWithZeroes,
                                       StorageModeEnum storage,
                                       const std::vector<ImageComponents>& outputComponents,
                                       const RectD& rod,
                                       const RectI& upscaledImageBounds,
                                       const RectI& downscaledImageBounds,
                                       const RectI& lastStrokePixelRoD,
                                       double par,
                                       bool renderFullScaleThenDownscale,
                                       bool createInCache,
                                       const boost::scoped_ptr<ImageKey>& key);


    EffectInstance::RenderRoIStatusEnum renderRoILaunchInternalRender(const RenderRoIArgs & args,
                                                                      const ParallelRenderArgsPtr& frameArgs,
                                                                      const ImagePlanesToRenderPtr &planesToRender,
                                                                      const ComponentsNeededMapPtr& neededComps,
                                                                      const OSGLContextPtr& glRenderContext,
                                                                      RenderSafetyEnum safety,
                                                                      ImageBitDepthEnum outputDepth,
                                                                      const ImageComponents &outputClipPrefComps,
                                                                      bool hasSomethingToRender,
                                                                      bool isDuringPaintStrokeDrawing,
                                                                      StorageModeEnum storage,
                                                                      const U64 frameViewHash,
                                                                      const RectD& rod,
                                                                      const RectI& roi,
                                                                      unsigned int renderMappedMipMapLevel,
                                                                      const std::bitset<4> &processChannels,
                                                                      double par,
                                                                      bool renderFullScaleThenDownscale,
                                                                      bool *renderAborted);

    void renderRoITermination(const RenderRoIArgs & args,
                              const ParallelRenderArgsPtr& frameArgs,
                              const ImagePlanesToRenderPtr &planesToRender,
                              const OSGLContextPtr& glGpuContext,
                              bool hasSomethingToRender,
                              bool isDuringPaintStrokeDrawing,
                              const RectD& rod,
                              const RectI& roi,
                              const RectI& downscaledImageBounds,
                              const RectI& originalRoI,
                              double par,
                              bool renderFullScaleThenDownscale,
                              bool renderAborted,
                              EffectInstance::RenderRoIStatusEnum renderRetCode,
                              std::map<ImageComponents, ImagePtr>* outputPlanes,
                              OSGLContextAttacherPtr* glContextLocker);



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
        ComponentsNeededMapPtr  compsNeeded;
        ImageComponents outputClipPrefsComps;
        bool byPassCache;
        std::bitset<4> processChannels;
        ImagePlanesToRenderPtr planes;
        OSGLContextPtr glContext;
    };

    void tryShrinkRenderWindow(const EffectTLSDataPtr &tls,
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
                                                  const ComponentsNeededMapPtr & compsNeeded,
                                                  const std::bitset<4>& processChannels,
                                                  const ImagePlanesToRenderPtr & planes);


    RenderingFunctorRetEnum renderHandlerIdentity(const EffectTLSDataPtr& tls,
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

    RenderingFunctorRetEnum renderHandlerInternal(const EffectTLSDataPtr& tls,
                                                  const OSGLContextPtr& glContext,
                                                  EffectInstance::RenderActionArgs &actionArgs,
                                                  const ImagePlanesToRenderPtr & planes,
                                                  bool multiPlanar,
                                                  bool bitmapMarkedForRendering,
                                                  const ImageComponents & outputClipPrefsComps,
                                                  const ImageBitDepthEnum outputClipPrefDepth,
                                                  const ComponentsNeededMapPtr & compsNeeded,
                                                  std::map<ImageComponents, PlaneToRender>& outputPlanes,
                                                  boost::shared_ptr<OSGLContextAttacher>* glContextAttacher);

    void setupRenderArgs(const EffectTLSDataPtr& tls,
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

    void renderHandlerPostProcess(const EffectTLSDataPtr& tls,
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
