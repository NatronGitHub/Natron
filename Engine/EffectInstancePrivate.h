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
    ActionRetCodeEnum determinePlanesToRender(const EffectInstance::RenderRoIArgs& args,
                                             std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                             std::list<ImageComponents> *planesToRender,
                                             std::bitset<4>* processChannels,
                                             std::map<ImageComponents, ImagePtr>* outputPlanes);

    /**
     * @brief Helper function in the implementation of renderRoI to handle identity effects
     **/
    ActionRetCodeEnum handleIdentityEffect(const EffectInstance::RenderRoIArgs& args,
                                          double par,
                                          const RectD& rod,
                                          const RenderScale& scale,
                                          const std::list<ImageComponents> &requestedComponents,
                                          const std::map<int, std::list<ImageComponents> >& neededComps,
                                          RenderRoIResults* outputPlanes,
                                          bool *isIdentity);

    /**
     * @brief Helper function in the implementation of renderRoI to handle effects that can concatenate (distorsion etc...)
     **/
    ActionRetCodeEnum handleConcatenation(const EffectInstance::RenderRoIArgs& args,
                                         const RenderScale& renderScale,
                                         RenderRoIResults* results,
                                         bool *concatenated);


   
    /**
     * @brief Helper function in the implementation of renderRoI to determine the image backend (OpenGL, CPU...)
     **/
    ActionRetCodeEnum resolveRenderBackend(const RenderRoIArgs & args,
                                          const FrameViewRequestPtr& requestPassData,
                                          const RectI& roi,
                                          RenderBackendTypeEnum* renderBackend,
                                          OSGLContextPtr *glRenderContext);

    /**
     * @brief Helper function in the implementation of renderRoI to determine if a render should use the Cache or not.
     * @returns The cache access type, i.e: none, write only or read/write
     **/
    CacheAccessModeEnum shouldRenderUseCache(const RenderRoIArgs & args,
                                             const FrameViewRequestPtr& requestPassData);

    bool canSplitRenderWindowWithIdentityRectangles(const RenderRoIArgs& args,
                                                    const RenderScale& renderMappedScale,
                                                    RectD* inputRoDIntersection);

    void fetchOrCreateOutputPlanes(const RenderRoIArgs & args,
                                   const FrameViewRequestPtr& requestPassData,
                                   CacheAccessModeEnum cacheAccess,
                                   const ImagePlanesToRenderPtr &planesToRender,
                                   const std::list<ImageComponents>& requestedComponents,
                                   const RectI& roi,
                                   const RenderScale& mappedProxyScale,
                                   unsigned int mappedMipMapLevel,
                                   std::map<ImageComponents, ImagePtr>* outputPlanes);


    void checkPlanesToRenderAndComputeRectanglesToRender(const RenderRoIArgs & args,
                                                         const ImagePlanesToRenderPtr &planesToRender,
                                                         CacheAccessModeEnum cacheAccess,
                                                         const RectI& roi,
                                                         std::map<ImageComponents, ImagePtr>* outputPlanes);

    void computeRectanglesToRender(const RenderRoIArgs& args, const ImagePlanesToRenderPtr &planesToRender, const RectI& renderWindow);


    ActionRetCodeEnum launchRenderAndWaitForPendingTiles(const RenderRoIArgs & args,
                                                         const ImagePlanesToRenderPtr &planesToRender,
                                                         const OSGLContextAttacherPtr& glRenderContext,
                                                         CacheAccessModeEnum cacheAccess,
                                                         const RectI& roi,
                                                         const RenderScale& renderMappedScale,
                                                         const std::bitset<4> &processChannels,
                                                         const std::map<int, std::list<ImageComponents> >& neededInputLayers,
                                                         std::map<ImageComponents, ImagePtr>* outputPlanes);

    ActionRetCodeEnum renderRoILaunchInternalRender(const RenderRoIArgs & args,
                                                    const ImagePlanesToRenderPtr &planesToRender,
                                                    const OSGLContextAttacherPtr& glRenderContext,
                                                    const RenderScale& renderMappedScale,
                                                    const std::bitset<4> &processChannels,
                                                    const std::map<int, std::list<ImageComponents> >& neededInputLayers);



    struct TiledRenderingFunctorArgs
    {
        const RenderRoIArgs* args;
        RenderScale renderMappedScale;
        ImagePtr mainInputImage;
        std::bitset<4> processChannels;
        ImagePlanesToRenderPtr planesToRender;
        OSGLContextAttacherPtr glContext;
    };



    ActionRetCodeEnum tiledRenderingFunctor(TiledRenderingFunctorArgs & args,
                                              const RectToRender & specificData,
                                              QThread* callingThread);


    ActionRetCodeEnum tiledRenderingFunctor(const RectToRender & rectToRender,
                                              const RenderRoIArgs* args,
                                              RenderScale renderMappedScale,
                                              std::bitset<4> processChannels,
                                              const ImagePtr& mainInputImage,
                                              ImagePlanesToRenderPtr planesToRender,
                                              OSGLContextAttacherPtr glContext);


    ActionRetCodeEnum renderHandlerIdentity(const RectToRender & rectToRender,
                                              const RenderRoIArgs* args,
                                              const RenderScale &renderMappedScale,
                                              const ImagePlanesToRenderPtr& planes);

    ActionRetCodeEnum renderHandlerInternal(const EffectInstanceTLSDataPtr& tls,
                                              const RectToRender & rectToRender,
                                              const RenderRoIArgs* args,
                                              const RenderScale& renderMappedScale,
                                              const OSGLContextAttacherPtr glContext,
                                              const ImagePlanesToRenderPtr& planes,
                                              std::bitset<4> processChannels);

    void renderHandlerPostProcess(const RectToRender & rectToRender,
                                  const RenderRoIArgs* args,
                                  const RenderScale& renderMappedScale,
                                  const ImagePlanesToRenderPtr& planes,
                                  const ImagePtr& mainInputImage,
                                  std::bitset<4> processChannels);


    void checkMetadata(NodeMetadata &metadata);

};



NATRON_NAMESPACE_EXIT;

#endif // Engine_EffectInstancePrivate_h
