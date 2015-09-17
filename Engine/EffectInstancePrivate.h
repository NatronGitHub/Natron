/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef _Engine_EffectInstancePrivate_h_
#define _Engine_EffectInstancePrivate_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "EffectInstance.h"

#include <map>
#include <list>
#include <string>

#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>

#include "Global/GlobalDefines.h"

#include "Engine/Image.h"
#include "Engine/ThreadStorage.h"

namespace Natron {

struct ActionKey
{
    double time;
    int view;
    unsigned int mipMapLevel;
};

struct IdentityResults
{
    int inputIdentityNb;
    double inputIdentityTime;
};

struct CompareActionsCacheKeys
{
    bool operator() (const ActionKey & lhs,
                     const ActionKey & rhs) const
    {
        if (lhs.time < rhs.time) {
            return true;
        } else if (lhs.time == rhs.time) {
            if (lhs.mipMapLevel < rhs.mipMapLevel) {
                return true;
            } else if (lhs.mipMapLevel == rhs.mipMapLevel) {
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

    bool getIdentityResult(U64 hash, double time, int view, unsigned int mipMapLevel, int* inputNbIdentity, double* identityTime);

    void setIdentityResult(U64 hash, double time, int view, unsigned int mipMapLevel, int inputNbIdentity, double identityTime);

    bool getRoDResult(U64 hash, double time, int view, unsigned int mipMapLevel, RectD* rod);

    void setRoDResult(U64 hash, double time, int view, unsigned int mipMapLevel, const RectD & rod);

    bool getFramesNeededResult(U64 hash, double time, int view, unsigned int mipMapLevel, FramesNeededMap* framesNeeded);

    void setFramesNeededResult(U64 hash, double time, int view, unsigned int mipMapLevel, const FramesNeededMap & framesNeeded);

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

        ActionsCacheInstance();
    };

    //In  a list to track the LRU
    std::list<ActionsCacheInstance> _instances;
    std::size_t _maxInstances;
    std::list<ActionsCacheInstance>::iterator createActionCacheInternal(U64 newHash);
    ActionsCacheInstance & getOrCreateActionCache(U64 newHash);
};

/**
 * @brief These args are local to a renderRoI call and used to retrieve this info
 * in a thread-safe and thread-local manner in getImage
 **/
struct EffectInstance::RenderArgs
{
    RectD _rod; //!< the effect's RoD in CANONICAL coordinates
    RoIMap _regionOfInterestResults; //< the input RoI's in CANONICAL coordinates
    RectI _renderWindowPixel; //< the current renderWindow in PIXEL coordinates
    double _time; //< the time to render
    int _view; //< the view to render
    bool _validArgs; //< are the args valid ?
    bool _isIdentity;
    double _identityTime;
    int _identityInputNb;
    std::map<ImageComponents, PlaneToRender> _outputPlanes;

    //This is set only when the plug-in has set ePassThroughRenderAllRequestedPlanes
    ImageComponents _outputPlaneBeingRendered;
    double _firstFrame, _lastFrame;

    RenderArgs();

    RenderArgs(const RenderArgs & o);
};

struct EffectInstance::Implementation
{
    Implementation(EffectInstance* publicInterface);

    EffectInstance* _publicInterface;

    ///Thread-local storage living through the render_public action and used by getImage to retrieve all parameters
    ThreadStorage<RenderArgs> renderArgs;

    ///Thread-local storage living through the whole rendering of a frame
    ThreadStorage<ParallelRenderArgs> frameRenderArgs;

    ///Keep track of begin/end sequence render calls to make sure they are called in the right order even when
    ///recursive renders are called
    ThreadStorage<int> beginEndRenderCount;

    ///Whenever a render thread is running, it stores here a temp copy used in getImage
    ///to make sure these images aren't cleared from the cache.
    ThreadStorage< InputImagesMap > inputImages;


    QMutex lastRenderArgsMutex; //< protects lastImage & lastRenderHash
    U64 lastRenderHash;  //< the last hash given to render
    ImageList lastPlanesRendered; //< the last image planes rendered
    mutable QReadWriteLock duringInteractActionMutex; //< protects duringInteractAction
    bool duringInteractAction; //< true when we're running inside an interact action

    ///Current chuncks of memory held by the plug-in
    mutable QMutex pluginMemoryChunksMutex;
    std::list<PluginMemory*> pluginMemoryChunks;

    ///Does this plug-in supports render scale ?
    QMutex supportsRenderScaleMutex;
    SupportsEnum supportsRenderScale;

    /// Mt-Safe actions cache
    ActionsCache actionsCache;

#if NATRON_ENABLE_TRIMAP
    ///Store all images being rendered to avoid 2 threads rendering the same portion of an image
    struct ImageBeingRendered
    {
        QWaitCondition cond;
        QMutex lock;
        int refCount;
        bool renderFailed;

        ImageBeingRendered() : cond(), lock(), refCount(0), renderFailed(false)
        {
        }
    };

    QMutex imagesBeingRenderedMutex;
    typedef boost::shared_ptr<ImageBeingRendered> IBRPtr;
    typedef std::map<ImagePtr, IBRPtr > IBRMap;
    IBRMap imagesBeingRendered;
#endif

    ///A cache for components available
    mutable QMutex componentsAvailableMutex;
    bool componentsAvailableDirty; /// Set to true when getClipPreferences is called to indicate it must be set again
    EffectInstance::ComponentsAvailableMap outputComponentsAvailable;

    void runChangedParamCallback(KnobI* k, bool userEdited, const std::string & callback);

    void setDuringInteractAction(bool b);

#if NATRON_ENABLE_TRIMAP
    void markImageAsBeingRendered(const boost::shared_ptr<Image> & img);

    bool waitForImageBeingRenderedElsewhereAndUnmark(const RectI & roi, const boost::shared_ptr<Image> & img);

    void unmarkImageAsBeingRendered(const boost::shared_ptr<Image> & img, bool renderFailed);
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
     * prior we have all the other parameters. getRegionOfDefinition only needs the current render view and mipMapLevel if it is
     * called on a render thread or during an analysis. We provide it by setting those 2 parameters directly on a thread-storage
     * object local to the clip.
     *
     * For getImage, all the ScopedRenderArgs are active (except for analysis). The view and mipMapLevel parameters will be retrieved
     * on the clip that needs the image. All the other parameters will be retrieved in EffectInstance::getImage on the ScopedRenderArgs.
     *
     * During an analysis effect we don't set any ScopedRenderArgs and call some actions recursively if needed.
     * WARNING: analysis effect's are set the current view and mipmapLevel to 0 in the OfxEffectInstance::knobChanged function
     * If we were to have analysis that perform on different views we would have to change that.
     **/
    class ScopedRenderArgs
    {
        RenderArgs* localData;
        ThreadStorage<RenderArgs>* _dst;

public:

        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,
                         const RoIMap & roiMap,
                         const RectD & rod,
                         const RectI & renderWindow,
                         double time,
                         int view,
                         bool isIdentity,
                         double identityTime,
                         int inputNbIdentity,
                         const std::map<ImageComponents, PlaneToRender> & outputPlanes,
                         double firstFrame,
                         double lastFrame);

        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst);


        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,
                         const RenderArgs & a);

        ~ScopedRenderArgs();

        RenderArgs & getLocalData();

        ///Setup the first pass on thread-local storage.
        ///RoIMap and frame range are separated because those actions might need
        ///the thread-storage set up in the first pass to work
        void setArgs_firstPass(const RectD & rod,
                               const RectI & renderWindow,
                               double time,
                               int view,
                               bool isIdentity,
                               double identityTime,
                               int inputNbIdentity);

        void setArgs_secondPass(const RoIMap & roiMap,
                                int firstFrame,
                                int lastFrame);
    };

    void addInputImageTempPointer(int inputNb, const boost::shared_ptr<Image> & img);

    void clearInputImagePointers();
};


class InputImagesHolder_RAII
{
    ThreadStorage< EffectInstance::InputImagesMap > *storage;
    EffectInstance::InputImagesMap* localData;

public:

    InputImagesHolder_RAII(const EffectInstance::InputImagesMap & imgs, ThreadStorage< EffectInstance::InputImagesMap>* storage);

    ~InputImagesHolder_RAII();
};

} // Namespace Natron

#endif // _Engine_EffectInstancePrivate_h_
