/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QtCore/QRecursiveMutex>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/Image.h"
#include "Engine/TLSHolder.h"
#include "Engine/NodeMetadata.h"
#include "Engine/OSGLContext.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct ActionKey
{
    double time;
    ViewIdx view;
    unsigned int mipmapLevel;
};

struct IdentityResults
{
    int inputIdentityNb;
    double inputIdentityTime;
    ViewIdx inputView;
};

struct ComponentsNeededResults
{
    EffectInstance::ComponentsNeededMap neededComps;
    std::bitset<4> processChannels;
    bool processAll;
    std::list<ImagePlaneDesc> passThroughPlanes;
    int passThroughInputNb;
    double passThroughTime;
    ViewIdx passThroughView;
};

struct CompareActionsCacheKeys
{
    bool operator() (const ActionKey & lhs,
                     const ActionKey & rhs) const
    {
        if (lhs.time < rhs.time) {
            return true;
        } else if (lhs.time == rhs.time) {
            if (lhs.mipmapLevel < rhs.mipmapLevel) {
                return true;
            } else if (lhs.mipmapLevel == rhs.mipmapLevel) {
                if (lhs.view < rhs.view) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
};

typedef std::map<ActionKey, IdentityResults, CompareActionsCacheKeys> IdentityCacheMap;
typedef std::map<ActionKey, RectD, CompareActionsCacheKeys> RoDCacheMap;
typedef std::map<ActionKey, FramesNeededMap, CompareActionsCacheKeys> FramesNeededCacheMap;
typedef std::map<ActionKey, ComponentsNeededResults, CompareActionsCacheKeys> ComponentsNeededCacheMap;

/**
 * @brief This class stores all results of the following actions:
   - getRegionOfDefinition (invalidated on hash change, mapped across time + scale)
   - getTimeDomain (invalidated on hash change, only 1 value possible
   - isIdentity (invalidated on hash change,mapped across time + scale)
 * The reason we store them is that the OFX Clip API can potentially call these actions recursively
 * but this is forbidden by the spec:
 * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id475585
 **/
class ActionsCache
{
public:
    ActionsCache(int maxAvailableHashes);

    void clearAll();

    void invalidateAll(U64 newHash);

    bool getIdentityResult(U64 hash, double time, ViewIdx view, int* inputNbIdentity, ViewIdx *inputView, double* identityTime);

    void setIdentityResult(U64 hash, double time, ViewIdx view, int inputNbIdentity, ViewIdx inputView, double identityTime);

    bool getComponentsNeededResults(U64 hash, double time, ViewIdx view, EffectInstance::ComponentsNeededMap* neededComps, std::bitset<4> *processChannels, bool *processAll,
                                    std::list<ImagePlaneDesc> *passThroughPlanes, int* passThroughInputNb, ViewIdx *passThroughView, double* passThroughTime);

    void setComponentsNeededResults(U64 hash, double time, ViewIdx view, const EffectInstance::ComponentsNeededMap& neededComps, std::bitset<4> processChannels,  bool processAll,
                                    const std::list<ImagePlaneDesc>& passThroughPlanes,int passThroughInputNb, ViewIdx passThroughView, double passThroughTime);

    bool getRoDResult(U64 hash, double time, ViewIdx view, unsigned int mipmapLevel, RectD* rod);

    void setRoDResult(U64 hash, double time, ViewIdx view, unsigned int mipmapLevel, const RectD & rod);

    bool getFramesNeededResult(U64 hash, double time, ViewIdx view, unsigned int mipmapLevel, FramesNeededMap* framesNeeded);

    void setFramesNeededResult(U64 hash, double time, ViewIdx view, unsigned int mipmapLevel, const FramesNeededMap & framesNeeded);

    bool getTimeDomainResult(U64 hash, double *first, double* last);

    void setTimeDomainResult(U64 hash, double first, double last);

private:
    mutable QMutex _cacheMutex; //< protects everything in the cache
    struct ActionsCacheInstance
    {
        U64 _hash;
        OfxRangeD _timeDomain;
        bool _timeDomainSet;
        IdentityCacheMap _identityCache;
        RoDCacheMap _rodCache;
        FramesNeededCacheMap _framesNeededCache;
        ComponentsNeededCacheMap _componentsNeededCache;

        ActionsCacheInstance();
    };

    //In  a list to track the LRU
    std::list<ActionsCacheInstance> _instances;
    std::size_t _maxInstances;
    std::list<ActionsCacheInstance>::iterator createActionCacheInternal(U64 newHash);
    ActionsCacheInstance & getOrCreateActionCache(U64 newHash);
};


class EffectInstance::Implementation
{
    Q_DECLARE_TR_FUNCTIONS(EffectInstance)

public:
    Implementation(EffectInstance* publicInterface);

    Implementation(const Implementation& other);

public:
    EffectInstance* _publicInterface;

    ///Thread-local storage living through the render_public action and used by getImage to retrieve all parameters
    std::shared_ptr<TLSHolder<EffectInstance::EffectTLSData> > tlsData;
    mutable QReadWriteLock duringInteractActionMutex; //< protects duringInteractAction
    bool duringInteractAction; //< true when we're running inside an interact action

    ///Current chunks of memory held by the plug-in
    mutable QMutex pluginMemoryChunksMutex;
    std::list<PluginMemoryWPtr> pluginMemoryChunks;

    ///Does this plug-in supports render scale ?
    QMutex supportsRenderScaleMutex;
    SupportsEnum supportsRenderScale;

    /// Mt-Safe actions cache
    ActionsCachePtr actionsCache;

#if NATRON_ENABLE_TRIMAP
    ///Store all images being rendered to avoid 2 threads rendering the same portion of an image
    struct ImageBeingRendered
    {
        QWaitCondition cond;
        QMutex lock;
        int refCount;
        bool failed;

        ImageBeingRendered()
            : cond(), lock(), refCount(0), failed(false)
        {
        }
    };

    QMutex imagesBeingRenderedMutex;
    typedef std::shared_ptr<ImageBeingRendered> ImageBeingRenderedPtr;
    typedef std::map<ImagePtr, ImageBeingRenderedPtr> ImageBeingRenderedMap;
    ImageBeingRenderedMap imagesBeingRendered;
#endif

    ///A cache for components available
    std::list<KnobIWPtr> overlaySlaves;
    mutable QMutex metadataMutex;
    NodeMetadata metadata;
    bool runningClipPreferences; //only used on main thread

    // set during interact actions on main-thread
    OverlaySupport* overlaysViewport;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    mutable QRecursiveMutex attachedContextsMutex;
#else
    mutable QMutex attachedContextsMutex;
#endif
    // A list of context that are currently attached (i.e attachOpenGLContext() has been called on them but not yet dettachOpenGLContext).
    // If a plug-in returns false to supportsConcurrentOpenGLRenders() then whenever trying to attach a context, we take a lock in attachOpenGLContext
    // that is released in dettachOpenGLContext so that there can only be a single attached OpenGL context at any time.
    EffectInstance::OpenGLContextEffectsMap attachedContexts;

    // Render clones are very small copies holding just pointers to Knobs that are used to render plug-ins that are only
    // eRenderSafetyInstanceSafe or lower
    EffectInstance* mainInstance; // pointer to the main-instance if this instance is a clone
    bool isDoingInstanceSafeRender; // true if this instance is rendering
    mutable QMutex renderClonesMutex;
    std::list<EffectInstancePtr> renderClonesPool;
    bool mustSyncPrivateData; //!< true if the effect's knobs were changed but instanceChanged could not be called (e.g. when loading a PyPlug), so that syncPrivateData should be called in getPreferredMetadata_public before calling getPreferredMetadata
    mutable QMutex mustSyncPrivateDataMutex; //!< protects mustSyncPrivateData

public:
    void runChangedParamCallback(KnobI* k, bool userEdited, const std::string & callback);

    void setDuringInteractAction(bool b);

#if NATRON_ENABLE_TRIMAP
    void markImageAsBeingRendered(const ImagePtr & img, const RectI& roi, std::list<RectI>* restToRender, bool *renderedElsewhere);

    bool waitForImageBeingRenderedElsewhere(const RectI & roi, const ImagePtr & img);

    void unmarkImageAsBeingRendered(const ImagePtr & img, const std::list<RectI>& rects, bool renderFailed);
#endif

    /**
     * @brief This function sets on the thread storage given in parameter all the arguments which
     * are used to render an image.
     * This is used exclusively on the render thread in the renderRoI function or renderRoIInternal function.
     * The reason we use thread-storage is because the OpenFX API doesn't give all the parameters to the
     * ImageEffect suite functions except the desired time. That is the Host has to maintain an internal state to "guess" what are the
     * expected parameters in order to respond correctly to the function call. This state is maintained throughout the render thread work
     * for all these actions:
     *
       - getRegionsOfInterest
       - getFrameRange
       - render
       - beginRender
       - endRender
       - isIdentity
     *
     * The object that will need to know these datas is OfxClipInstance, more precisely in the following functions:
       - OfxClipInstance::getRegionOfDefinition
       - OfxClipInstance::getImage
     *
     * We don't provide these datas for the getRegionOfDefinition with these render args because this action can be called way
     * prior we have all the other parameters. getRegionOfDefinition only needs the current render view and mipmapLevel if it is
     * called on a render thread or during an analysis. We provide it by setting those 2 parameters directly on a thread-storage
     * object local to the clip.
     *
     * For getImage, all the ScopedRenderArgs are active (except for analysis). The view and mipmapLevel parameters will be retrieved
     * on the clip that needs the image. All the other parameters will be retrieved in EffectInstance::getImage on the ScopedRenderArgs.
     *
     * During an analysis effect we don't set any ScopedRenderArgs and call some actions recursively if needed.
     * WARNING: analysis effect's are set the current view and mipmapLevel to 0 in the OfxEffectInstance::knobChanged function
     * If we were to have analysis that perform on different views we would have to change that.
     **/
    class ScopedRenderArgs
    {
        EffectTLSDataPtr tlsData;

public:
        ScopedRenderArgs(const EffectTLSDataPtr& tlsData,
                         const RectD & rod,
                         const RectI & renderWindow,
                         double time,
                         ViewIdx view,
                         bool isIdentity,
                         double identityTime,
                         const EffectInstancePtr& identityInput,
                         const ComponentsNeededMapPtr& compsNeeded,
                         const EffectInstance::InputImagesMap& inputImages,
                         const RoIMap & roiMap,
                         int firstFrame,
                         int lastFrame,
                         bool isDoingOpenGLRender);

        ScopedRenderArgs(const EffectTLSDataPtr& tlsData,
                         const EffectTLSDataPtr& otherThreadData);

        ~ScopedRenderArgs();
    };

    void addInputImageTempPointer(int inputNb, const ImagePtr & img);

    void clearInputImagePointers();


    struct TiledRenderingFunctorArgs
    {
        bool renderFullScaleThenDownscale;
        bool isSequentialRender;
        bool isRenderResponseToUserInteraction;
        int firstFrame;
        int lastFrame;
        int preferredInput;
        unsigned int mipmapLevel;
        unsigned int renderMappedMipmapLevel;
        RectD rod;
        double time;
        ViewIdx view;
        double par;
        ImageBitDepthEnum outputClipPrefDepth;
        ComponentsNeededMapPtr  compsNeeded;
        ImagePlaneDesc outputClipPrefsComps;
        bool byPassCache;
        std::bitset<4> processChannels;
        ImagePlanesToRenderPtr planes;
    };

    RenderingFunctorRetEnum tiledRenderingFunctor(TiledRenderingFunctorArgs & args,  const RectToRender & specificData,
                                                  QThread* callingThread);

    RenderingFunctorRetEnum tiledRenderingFunctor(const RectToRender & rectToRender,
                                                  const bool renderFullScaleThenDownscale,
                                                  const bool isSequentialRender,
                                                  const bool isRenderResponseToUserInteraction,
                                                  const int firstFrame, const int lastFrame,
                                                  const int preferredInput,
                                                  const unsigned int mipmapLevel,
                                                  const unsigned int renderMappedMipmapLevel,
                                                  const RectD & rod,
                                                  const double time,
                                                  const ViewIdx view,
                                                  const double par,
                                                  const bool byPassCache,
                                                  const ImageBitDepthEnum outputClipPrefDepth,
                                                  const ImagePlaneDesc & outputClipPrefsComps,
                                                  const ComponentsNeededMapPtr & compsNeeded,
                                                  const std::bitset<4>& processChannels,
                                                  const ImagePlanesToRenderPtr & planes);


    ///These are the image passed to the plug-in to render
    /// - fullscaleMappedImage is the fullscale image remapped to what the plugin can support (components/bitdepth)
    /// - downscaledMappedImage is the downscaled image remapped to what the plugin can support (components/bitdepth wise)
    /// - fullscaleMappedImage is pointing to "image" if the plug-in does support the renderscale, meaning we don't use it.
    /// - Similarly downscaledMappedImage is pointing to "downscaledImage" if the plug-in doesn't support the render scale.
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
    RenderingFunctorRetEnum renderHandler(const EffectTLSDataPtr& tls,
                                          const unsigned int mipmapLevel,
                                          const bool renderFullScaleThenDownscale,
                                          const bool isSequentialRender,
                                          const bool isRenderResponseToUserInteraction,
                                          const RectI & renderMappedRectToRender,
                                          const RectI & downscaledRectToRender,
                                          const bool byPassCache,
                                          const ImageBitDepthEnum outputClipPrefDepth,
                                          const ImagePlaneDesc & outputClipPrefsComps,
                                          const std::bitset<4>& processChannels,
                                          const ImagePtr & originalInputImage,
                                          const ImagePtr & maskImage,
                                          const ImagePremultiplicationEnum originalImagePremultiplication,
                                          ImagePlanesToRender & planes);

    static bool aborted(bool isRenderResponseToUserInteraction,
                        const AbortableRenderInfoPtr& abortInfo,
                        const EffectInstancePtr& treeRoot)  WARN_UNUSED_RETURN;

    void checkMetadata(NodeMetadata &metadata);
};


NATRON_NAMESPACE_EXIT

#endif // Engine_EffectInstancePrivate_h
