//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "EffectInstance.h"
#include <map>
#include <sstream>
#include <QtConcurrentMap>
#include <QReadWriteLock>
#include <QCoreApplication>
#include <QtConcurrentRun>

#include <boost/bind.hpp>
#include <SequenceParsing.h>

#include "Global/MemoryInfo.h"
#include "Engine/AppManager.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Log.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/KnobFile.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/AppInstance.h"
#include "Engine/ThreadStorage.h"
#include "Engine/Settings.h"
#include "Engine/RotoContext.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Transform.h"
#include "Engine/DiskCacheNode.h"

using namespace Natron;


class File_Knob;
class OutputFile_Knob;



namespace  {
    struct ActionKey {
        double time;
        unsigned int mipMapLevel;
    };
    
    struct IdentityResults {
        int inputIdentityNb;
        double inputIdentityTime;
    };
    
    struct CompareActionsCacheKeys {
        bool operator() (const ActionKey& lhs,const ActionKey& rhs) const {
            if (lhs.time < rhs.time) {
                return true;
            } else if (lhs.time == rhs.time) {
                if (lhs.mipMapLevel < rhs.mipMapLevel) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
            
        }
    };
    
    typedef std::map<ActionKey,IdentityResults,CompareActionsCacheKeys> IdentityCacheMap;
    typedef std::map<ActionKey,RectD,CompareActionsCacheKeys> RoDCacheMap;
    
    /**
     * @brief This class stores all results of the following actions:
     - getRegionOfDefinition (invalidated on hash change, mapped across time + scale)
     - getTimeDomain (invalidated on hash change, only 1 value possible
     - isIdentity (invalidated on hash change,mapped across time + scale)
     * The reason we store them is that the OFX Clip API can potentially call these actions recursively
     * but this is forbidden by the spec:
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id475585
     **/
    class ActionsCache {
        
        mutable QMutex _cacheMutex; //< protects everything in the cache
        
        U64 _cacheHash; //< the effect hash at which the actions were computed
        
        OfxRangeD _timeDomain;
        bool _timeDomainSet;
        
        IdentityCacheMap _identityCache;
        RoDCacheMap _rodCache;
        
    public:
        
        ActionsCache()
        : _cacheMutex()
        , _cacheHash(0)
        , _timeDomain()
        , _timeDomainSet(false)
        , _identityCache()
        , _rodCache()
        {
            
        }
        
        /**
         * @brief Get the hash at which the actions are stored in the cache currently
         **/
        bool getCacheHash() const {
            QMutexLocker l(&_cacheMutex);
            return _cacheHash;
        }
        
        void invalidateAll(U64 newHash) {
            QMutexLocker l(&_cacheMutex);
            _cacheHash = newHash;
            _rodCache.clear();
            _identityCache.clear();
            _timeDomainSet = false;
        }
        
        
        bool getIdentityResult(U64 hash,double time,unsigned int mipMapLevel,int* inputNbIdentity,double* identityTime) {
            QMutexLocker l(&_cacheMutex);
            if (hash != _cacheHash)
                return false;
            
            ActionKey key;
            key.time = time;
            key.mipMapLevel = mipMapLevel;
            
            IdentityCacheMap::const_iterator found = _identityCache.find(key);
            if ( found != _identityCache.end() ) {
                *inputNbIdentity = found->second.inputIdentityNb;
                *identityTime = found->second.inputIdentityTime;
                return true;
            }
            return false;
        }
        
        void setIdentityResult(double time,unsigned int mipMapLevel,int inputNbIdentity,double identityTime)
        {
            QMutexLocker l(&_cacheMutex);
           
            
            ActionKey key;
            key.time = time;
            key.mipMapLevel = mipMapLevel;
            
            IdentityCacheMap::iterator found = _identityCache.find(key);
            if ( found != _identityCache.end() ) {
                found->second.inputIdentityNb = inputNbIdentity;
                found->second.inputIdentityTime = identityTime;
            } else {
                IdentityResults v;
                v.inputIdentityNb = inputNbIdentity;
                v.inputIdentityTime = identityTime;
                _identityCache.insert(std::make_pair(key, v));
            }
            
        }
        
        bool getRoDResult(U64 hash,double time,unsigned int mipMapLevel,RectD* rod) {
            QMutexLocker l(&_cacheMutex);
            if (hash != _cacheHash)
                return false;
            
            ActionKey key;
            key.time = time;
            key.mipMapLevel = mipMapLevel;
            
            RoDCacheMap::const_iterator found = _rodCache.find(key);
            if ( found != _rodCache.end() ) {
                *rod = found->second;
                return true;
            }
            return false;
        }
        
        void setRoDResult(double time,unsigned int mipMapLevel,const RectD& rod)
        {
            QMutexLocker l(&_cacheMutex);
            
            
            ActionKey key;
            key.time = time;
            key.mipMapLevel = mipMapLevel;
            
            RoDCacheMap::iterator found = _rodCache.find(key);
            if ( found != _rodCache.end() ) {
                ///Already set, this is a bug
                return;
            } else {
                _rodCache.insert(std::make_pair(key, rod));
            }
            
        }
        
        bool getTimeDomainResult(U64 hash,double *first,double* last) {
            QMutexLocker l(&_cacheMutex);
            if (hash != _cacheHash || !_timeDomainSet)
                return false;
            
            *first = _timeDomain.min;
            *last = _timeDomain.max;
            return true;
        }
        
        void setTimeDomainResult(double first,double last)
        {
            QMutexLocker l(&_cacheMutex);
            _timeDomainSet = true;
            _timeDomain.min = first;
            _timeDomain.max = last;
        }
        
    };

}

/**
 * @brief These args are local to a renderRoI call and used to retrieve this info 
 * in a thread-safe and thread-local manner in getImage
 **/
struct EffectInstance::RenderArgs
{
    RectD _rod; //!< the effect's RoD in CANONICAL coordinates
    RoIMap _regionOfInterestResults; //< the input RoI's in CANONICAL coordinates
    RectI _renderWindowPixel; //< the current renderWindow in PIXEL coordinates
    SequenceTime _time; //< the time to render
    int _view; //< the view to render
    bool _validArgs; //< are the args valid ?
    int _channelForAlpha;
    bool _isIdentity;
    SequenceTime _identityTime;
    int _identityInputNb;
    boost::shared_ptr<Image> _outputImage;
    
    int _firstFrame,_lastFrame;
    
    RenderArgs()
        : _rod()
          , _regionOfInterestResults()
          , _renderWindowPixel()
          , _time(0)
          , _view(0)
          , _validArgs(false)
          , _channelForAlpha(3)
          , _isIdentity(false)
          , _identityTime(0)
          , _identityInputNb(-1)
          , _outputImage()
          , _firstFrame(0)
          , _lastFrame(0)
    {
    }
    
    RenderArgs(const RenderArgs& o)
    : _rod(o._rod)
    , _regionOfInterestResults(o._regionOfInterestResults)
    , _renderWindowPixel(o._renderWindowPixel)
    , _time(o._time)
    , _view(o._view)
    , _validArgs(o._validArgs)
    , _channelForAlpha(o._channelForAlpha)
    , _isIdentity(o._isIdentity)
    , _identityTime(o._identityTime)
    , _identityInputNb(o._identityInputNb)
    , _outputImage(o._outputImage)
    , _firstFrame(o._firstFrame)
    , _lastFrame(o._lastFrame)
    {
    }
};


struct EffectInstance::Implementation
{
    Implementation()
        : renderAbortedMutex()
          , renderAborted(false)
          , renderArgs()
          , frameRenderArgs()
          , beginEndRenderCount()
          , inputImages()
          , lastRenderArgsMutex()
          , lastRenderHash(0)
          , lastImage()
          , duringInteractActionMutex()
          , duringInteractAction(false)
          , pluginMemoryChunksMutex()
          , pluginMemoryChunks()
          , supportsRenderScale(eSupportsMaybe)
          , actionsCache()
    {
    }

    mutable QReadWriteLock renderAbortedMutex;
    bool renderAborted; //< was rendering aborted ?

    ///Thread-local storage living through the render_public action and used by getImage to retrieve all parameters
    ThreadStorage<RenderArgs> renderArgs;
    
    ///Thread-local storage living through the whole rendering of a frame
    ThreadStorage<ParallelRenderArgs> frameRenderArgs;
    
    ///Keep track of begin/end sequence render calls to make sure they are called in the right order even when
    ///recursive renders are called
    ThreadStorage<int> beginEndRenderCount;

    ///Whenever a render thread is running, it stores here a temp copy used in getImage
    ///to make sure these images aren't cleared from the cache.
    ThreadStorage< std::list< boost::shared_ptr<Natron::Image> > > inputImages;
    

    QMutex lastRenderArgsMutex; //< protects lastImage & lastRenderHash
    U64 lastRenderHash;  //< the last hash given to render
    boost::shared_ptr<Natron::Image> lastImage; //< the last image rendered
    
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
    
    
    void setDuringInteractAction(bool b)
    {
        QWriteLocker l(&duringInteractActionMutex);

        duringInteractAction = b;
    }



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
        RenderArgs args;
        ThreadStorage<RenderArgs>* _dst;

    public:
        
        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,
                         const RoIMap & roiMap,
                         const RectD & rod,
                         const RectI& renderWindow,
                         SequenceTime time,
                         int view,
                         int channelForAlpha,
                         bool isIdentity,
                         SequenceTime identityTime,
                         int inputNbIdentity,
                         const boost::shared_ptr<Image>& outputImage,
                         int firstFrame,
                         int lastFrame)
            : args()
              , _dst(dst)
        {
            assert(_dst);

            args._rod = rod;
            args._renderWindowPixel = renderWindow;
            args._time = time;
            args._view = view;
            args._channelForAlpha = channelForAlpha;
            args._isIdentity = isIdentity;
            args._identityTime = identityTime;
            args._identityInputNb = inputNbIdentity;
            args._outputImage = outputImage;
            args._regionOfInterestResults = roiMap;
            args._firstFrame = firstFrame;
            args._lastFrame = lastFrame;
            args._validArgs = true;
            _dst->setLocalData(args);

        }
        
        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst)
        : args()
        , _dst(dst)
        {
            assert(_dst);
        }

        

        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,
                         const RenderArgs & a)
            : args(a)
              , _dst(dst)
        {
            args._validArgs = true;
            _dst->setLocalData(args);
        }

        ~ScopedRenderArgs()
        {
            assert( _dst->hasLocalData() );
            args._outputImage.reset();
            args._validArgs = false;
            _dst->setLocalData(args);
        }

        /**
         * @brief WARNING: Returns the args that have been passed to the constructor.
         **/
        const RenderArgs & getArgs() const
        {
            return args;
        }
        
        ///Setup the first pass on thread-local storage.
        ///RoIMap and frame range are separated because those actions might need
        ///the thread-storage set up in the first pass to work
        void setArgs_firstPass(const RectD & rod,
                               const RectI& renderWindow,
                               SequenceTime time,
                               int view,
                               int channelForAlpha,
                               bool isIdentity,
                               SequenceTime identityTime,
                               int inputNbIdentity,
                               const boost::shared_ptr<Image>& outputImage)
        {
            args._rod = rod;
            args._renderWindowPixel = renderWindow;
            args._time = time;
            args._view = view;
            args._channelForAlpha = channelForAlpha;
            args._isIdentity = isIdentity;
            args._identityTime = identityTime;
            args._identityInputNb = inputNbIdentity;
            args._outputImage = outputImage;
            args._validArgs = true;
            _dst->setLocalData(args);
        }
        
        void setArgs_secondPass(const RoIMap & roiMap,
                                int firstFrame,
                                int lastFrame) {
            args._regionOfInterestResults = roiMap;
            args._firstFrame = firstFrame;
            args._lastFrame = lastFrame;
            args._validArgs = true;
            _dst->setLocalData(args);
        }
        

    };
    
    void addInputImageTempPointer(const boost::shared_ptr<Natron::Image> & img)
    {
        inputImages.localData().push_back(img);
    }

    void clearInputImagePointers()
    {
        if (inputImages.hasLocalData()) {
            inputImages.localData().clear();
        }
    }
};

class InputImagesHolder_RAII
{
    ThreadStorage< std::list< boost::shared_ptr<Natron::Image> > > *storage;
    
public:
    
    InputImagesHolder_RAII(const std::list<boost::shared_ptr<Natron::Image> >& imgs,ThreadStorage< std::list< boost::shared_ptr<Natron::Image> > >* storage)
    : storage(storage)
    {
        if (!imgs.empty()) {
            if (storage->hasLocalData()) {
                std::list<boost::shared_ptr<Natron::Image> >& data = storage->localData();
                data.insert(data.begin(), imgs.begin(),imgs.end());
            } else {
                storage->setLocalData(imgs);
            }
        } else {
            this->storage = 0;
        }
        
    }
    
    ~InputImagesHolder_RAII()
    {
        if (storage) {
            assert(storage->hasLocalData());
            storage->localData().clear();
        }
    }
};

void
EffectInstance::addThreadLocalInputImageTempPointer(const boost::shared_ptr<Natron::Image> & img)
{
    _imp->addInputImageTempPointer(img);
}

EffectInstance::EffectInstance(boost::shared_ptr<Node> node)
    : NamedKnobHolder(node ? node->getApp() : NULL)
      , _node(node)
      , _imp(new Implementation)
{
}

EffectInstance::~EffectInstance()
{
    clearPluginMemoryChunks();
}


void
EffectInstance::lock(const boost::shared_ptr<Natron::Image>& entry)
{
    _node->lock(entry);
}

void
EffectInstance::unlock(const boost::shared_ptr<Natron::Image>& entry)
{
    _node->unlock(entry);
}

void
EffectInstance::clearPluginMemoryChunks()
{
    int toRemove;
    {
        QMutexLocker l(&_imp->pluginMemoryChunksMutex);
        toRemove = (int)_imp->pluginMemoryChunks.size();
    }

    while (toRemove > 0) {
        PluginMemory* mem;
        {
            QMutexLocker l(&_imp->pluginMemoryChunksMutex);
            mem = ( *_imp->pluginMemoryChunks.begin() );
        }
        delete mem;
        --toRemove;
    }
}

void
EffectInstance::setParallelRenderArgs(int time,
                                      int view,
                                      bool isRenderUserInteraction,
                                      bool isSequential,
                                      bool canAbort,
                                      U64 nodeHash,
                                      U64 rotoAge,
                                      const TimeLine* timeline)
{
    ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
    
    args.time = time;
    args.timeline = timeline;
    args.view = view;
    args.isRenderResponseToUserInteraction = isRenderUserInteraction;
    args.isSequentialRender = isSequential;
    
    args.nodeHash = nodeHash;
    args.rotoAge = rotoAge;
    
    args.canAbort = canAbort;
    
    ++args.validArgs;
    
}

void
EffectInstance::invalidateParallelRenderArgs()
{
    if (_imp->frameRenderArgs.hasLocalData()) {
        ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        --args.validArgs;
    } else {
        qDebug() << "Frame render args thread storage not set, this is probably because the graph changed while rendering.";
    }
}

U64
EffectInstance::getHash() const
{
    return _node->getHashValue();
}

U64
EffectInstance::getRenderHash() const
{
    
    if ( !_imp->frameRenderArgs.hasLocalData() ) {
        return getHash();
    } else {
        ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        if (!args.validArgs) {
            return getHash();
        } else {
            return args.nodeHash;
        }
    }
}

bool
EffectInstance::isAbortedFromPlayback() const
{
    
    ///This flag is set in OutputSchedulerThread::abortRendering
    ///This will be used when playback or rendering on disk
    QReadLocker l(&_imp->renderAbortedMutex);
    return _imp->renderAborted;
    
}

bool
EffectInstance::aborted() const
{
   
     if ( !_imp->frameRenderArgs.hasLocalData() ) {
        
        ///No local data, we're either not rendering or calling this from a thread not controlled by Natron
        return false;
        
    } else {
        
        ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        if (!args.validArgs) {
            ///No valid args, probably not rendering
            return false;
        } else {
            if (args.isRenderResponseToUserInteraction) {
                
                if (args.canAbort) {
                    ///Rendering issued by RenderEngine::renderCurrentFrame, if time or hash changed, abort
                    return args.nodeHash != getHash() ||
                    args.time != args.timeline->currentFrame();
                } else {
                    return false;
                }
                
            } else {
                ///Rendering is playback or render on disk, we rely on the _imp->renderAborted flag for this.

                return isAbortedFromPlayback();
          
            }
        }

    }
}

bool
EffectInstance::shouldCacheOutput() const
{
    return _node->shouldCacheOutput();
}

void
EffectInstance::setAborted(bool b)
{
    {
        QWriteLocker l(&_imp->renderAbortedMutex);
        _imp->renderAborted = b;
    }
}

U64
EffectInstance::getKnobsAge() const
{
    return _node->getKnobsAge();
}

void
EffectInstance::setKnobsAge(U64 age)
{
    _node->setKnobsAge(age);
}

const std::string &
EffectInstance::getName() const
{
    return _node->getName();
}

std::string
EffectInstance::getName_mt_safe() const
{
    return _node->getName_mt_safe();
}

void
EffectInstance::getRenderFormat(Format *f) const
{
    assert(f);
    getApp()->getProject()->getProjectDefaultFormat(f);
}

int
EffectInstance::getRenderViewsCount() const
{
    return getApp()->getProject()->getProjectViewsCount();
}

bool
EffectInstance::hasOutputConnected() const
{
    return _node->hasOutputConnected();
}

EffectInstance*
EffectInstance::getInput(int n) const
{
    boost::shared_ptr<Natron::Node> inputNode = _node->getInput(n);

    if (inputNode) {
        return inputNode->getLiveInstance();
    }

    return NULL;
}

std::string
EffectInstance::getInputLabel(int inputNb) const
{
    std::string out;

    out.append( 1,(char)(inputNb + 65) );

    return out;
}

bool
EffectInstance::retrieveGetImageDataUponFailure(const int time,
                                                const int view,
                                                const RenderScale& scale,
                                                const RectD* optionalBoundsParam,
                                                U64* nodeHash_p,
                                                U64* rotoAge_p,
                                                bool* isIdentity_p,
                                                int* identityTime,
                                                int* identityInputNb_p,
                                                RectD* rod_p,
                                                RoIMap* inputRois_p, //!< output, only set if optionalBoundsParam != NULL
                                                RectD* optionalBounds_p) //!< output, only set if optionalBoundsParam != NULL
{
    /////Update 09/02/14
    /// We now AUTHORIZE GetRegionOfDefinition and isIdentity and getRegionsOfInterest to be called recursively.
    /// It didn't make much sense to forbid them from being recursive.
    
//#ifdef DEBUG
//    if (QThread::currentThread() != qApp->thread()) {
//        ///This is a bad plug-in
//        qDebug() << getNode()->getName_mt_safe().c_str() << " is trying to call clipGetImage during an unauthorized time. "
//        "Developers of that plug-in should fix it. \n Reminder from the OpenFX spec: \n "
//        "Images may be fetched from an attached clip in the following situations... \n"
//        "- in the kOfxImageEffectActionRender action\n"
//        "- in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason or kOfxChangeUserEdited";
//    }
//#endif
    
    ///Try to compensate for the mistake
    
    *nodeHash_p = getHash();
    const U64& nodeHash = *nodeHash_p;
    boost::shared_ptr<RotoContext> roto =  getNode()->getRotoContext();
    if (roto) {
        *rotoAge_p = roto->getAge();
    } else {
        *rotoAge_p = 0;
    }
    
    Natron::StatusEnum stat = getRegionOfDefinition(nodeHash, time, scale, view, rod_p);
    if (stat == eStatusFailed) {
        return false;
    }
    const RectD& rod = *rod_p;
    
    ///OptionalBoundsParam is the optional rectangle passed to getImage which may be NULL, in which case we use the RoD.
    if (!optionalBoundsParam) {
        ///// We cannot recover the RoI, we just assume the plug-in wants to render the full RoD.
        *optionalBounds_p = rod;
        ifInfiniteApplyHeuristic(nodeHash, time, scale, view, optionalBounds_p);
        const RectD& optionalBounds = *optionalBounds_p;
        
        /// If the region parameter is not set to NULL, then it will be clipped to the clip's
        /// Region of Definition for the given time. The returned image will be m at m least as big as this region.
        /// If the region parameter is not set, then the region fetched will be at least the Region of Interest
        /// the effect has previously specified, clipped the clip's Region of Definition.
        /// (renderRoI will do the clipping for us).
        
        
        ///// This code is wrong but executed ONLY IF THE PLUG-IN DOESN'T RESPECT THE SPECIFICATIONS. Recursive actions
        ///// should never happen.
        getRegionsOfInterest(time, scale, optionalBounds, optionalBounds, 0,inputRois_p);
    }
    
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );
    try {
        *isIdentity_p = isIdentity_public(nodeHash, time, scale, rod, getPreferredAspectRatio(), view, identityTime, identityInputNb_p);
    } catch (...) {
        return false;
    }


    return true;
}

void
EffectInstance::getThreadLocalInputImages(std::list<boost::shared_ptr<Natron::Image> >* images) const
{
    if (_imp->inputImages.hasLocalData()) {
        *images = _imp->inputImages.localData();
    }
}

bool
EffectInstance::getThreadLocalRegionsOfInterests(EffectInstance::RoIMap& roiMap) const
{
    if (!_imp->renderArgs.hasLocalData()) {
        return false;
    }
    RenderArgs& renderArgs = _imp->renderArgs.localData();
    if (!renderArgs._validArgs) {
        return false;
    }
    roiMap = renderArgs._regionOfInterestResults;
    return true;
}


boost::shared_ptr<Natron::Image>
EffectInstance::getImage(int inputNb,
                         const SequenceTime time,
                         const RenderScale & scale,
                         const int view,
                         const RectD *optionalBoundsParam, //!< optional region in canonical coordinates
                         const Natron::ImageComponentsEnum comp,
                         const Natron::ImageBitDepthEnum depth,
                         const double par,
                         const bool dontUpscale,
                         RectI* roiPixel)
{
   
    ///The input we want the image from
    EffectInstance* n = getInput(inputNb);
    
    
    bool isMask = isInputMask(inputNb);
    
    if ( isMask && !isMaskEnabled(inputNb) ) {
        ///This is last resort, the plug-in should've checked getConnected() before, which would have returned false.
        return boost::shared_ptr<Natron::Image>();
    }
    boost::shared_ptr<RotoContext> roto = _node->getRotoContext();
    bool useRotoInput = false;
    if (roto) {
        useRotoInput = isInputRotoBrush(inputNb);
    }
    if ( ( !roto || (roto && !useRotoInput) ) && !n ) {
        return boost::shared_ptr<Natron::Image>();
    }
    
    RectD optionalBounds;
    if (optionalBoundsParam) {
        optionalBounds = *optionalBoundsParam;
    }
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    RoIMap inputsRoI;
    RectD rod;
    bool isIdentity;
    int inputNbIdentity;
    int inputIdentityTime;
    U64 nodeHash;
    U64 rotoAge;
    
    /// Never by-pass the cache here because we already computed the image in renderRoI and by-passing the cache again can lead to
    /// re-computing of the same image many many times
    bool byPassCache = false;
    
    ///The caller thread MUST be a thread owned by Natron. It cannot be a thread from the multi-thread suite.
    ///A call to getImage is forbidden outside an action running in a thread launched by Natron.
    
    /// From http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsImagesAndClipsUsingClips
    //    Images may be fetched from an attached clip in the following situations...
    //    in the kOfxImageEffectActionRender action
    //    in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason of kOfxChangeUserEdited
    
    if ( !_imp->renderArgs.hasLocalData() || !_imp->frameRenderArgs.hasLocalData() ) {
        
        if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &rotoAge, &isIdentity, &inputIdentityTime, &inputNbIdentity, &rod, &inputsRoI, &optionalBounds) ) {
            return boost::shared_ptr<Image>();
        }
        
    } else {
        
        RenderArgs& renderArgs = _imp->renderArgs.localData();
        ParallelRenderArgs& frameRenderArgs = _imp->frameRenderArgs.localData();
        
        if (!renderArgs._validArgs || !frameRenderArgs.validArgs) {
            if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &rotoAge, &isIdentity, &inputIdentityTime, &inputNbIdentity, &rod, &inputsRoI, &optionalBounds) ) {
                return boost::shared_ptr<Image>();
            }
            
        } else {
            inputsRoI = renderArgs._regionOfInterestResults;
            rod = renderArgs._rod;
            isIdentity = renderArgs._isIdentity;
            inputIdentityTime = renderArgs._identityTime;
            inputNbIdentity = renderArgs._identityInputNb;
            nodeHash = frameRenderArgs.nodeHash;
            rotoAge = frameRenderArgs.rotoAge;
        }
        
        
    }
    
    RectD roi;
    if (!optionalBoundsParam) {
        RoIMap::iterator found = inputsRoI.find(useRotoInput ? this : n);
        if ( found != inputsRoI.end() ) {
            ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
            roi = found->second;
        } else {
            ///Oops, we didn't find the roi in the thread-storage... use  the RoD instead...
            roi = rod;
        }
    } else {
        roi = optionalBounds;
    }
    
    
    
    if ( isIdentity) {
        assert(inputNbIdentity != -2);
        ///If the effect is an identity but it didn't ask for the effect's image of which it is identity
        ///return a null image
        if (inputNbIdentity != inputNb) {
            return boost::shared_ptr<Image>();
        }
        
    }
    
    
    bool renderFullScaleThenDownscale = (!supportsRenderScale() && mipMapLevel != 0);
    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    unsigned int renderMappedMipMapLevel = mipMapLevel;
    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = _node->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
        if (renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            renderMappedMipMapLevel = 0;
        }
    }
    
    ///Both the result of getRegionsOfInterest and optionalBounds are in canonical coordinates, we have to convert in both cases
    ///Convert to pixel coordinates
    RectI pixelRoI;
    roi.toPixelEnclosing(renderScaleOneUpstreamIfRenderScaleSupportDisabled ? 0 : mipMapLevel, par, &pixelRoI);
    
    //Try to find in the input images thread local storage if we already pre-computed the image
    std::list<boost::shared_ptr<Image> > inputImagesThreadLocal;
    if (_imp->inputImages.hasLocalData()) {
        inputImagesThreadLocal = _imp->inputImages.localData();
    }
    
    
    int channelForAlpha = !isMask ? -1 : getMaskChannel(inputNb);
    
    if (useRotoInput) {
        
        Natron::ImageComponentsEnum outputComps;
        Natron::ImageBitDepthEnum outputDepth;
        getPreferredDepthAndComponents(-1, &outputComps, &outputDepth);
        boost::shared_ptr<Natron::Image> mask =  roto->renderMask(true,pixelRoI, outputComps, nodeHash,rotoAge,
                                                                  RectD(), time, depth, view, mipMapLevel, inputImagesThreadLocal, byPassCache);
        if (inputImagesThreadLocal.empty()) {
            ///If the effect is analysis (e.g: Tracker) there's no input images in the tread local storage, hence add it
            _imp->addInputImageTempPointer(mask);
        }
        if (roiPixel) {
            *roiPixel = pixelRoI;
        }
        return mask;
    }
    
    
    //if the node is not connected, return a NULL pointer!
    if (!n) {
        return boost::shared_ptr<Natron::Image>();
    }
    
    
    boost::shared_ptr<Image > inputImg = n->renderRoI( RenderRoIArgs(time,
                                                                     scale,
                                                                     renderMappedMipMapLevel,
                                                                     view,
                                                                     byPassCache,
                                                                     pixelRoI,
                                                                     RectD(),
                                                                     comp,
                                                                     depth,
                                                                     channelForAlpha,
                                                                     true,
                                                                     inputImagesThreadLocal) );
    
    if (!inputImg) {
        return inputImg;
    }
    if (roiPixel) {
        *roiPixel = pixelRoI;
    }
    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();
    
    if (inputImg->getPixelAspectRatio() != par) {
        qDebug() << "WARNING: " << getName_mt_safe().c_str() << " requested an image with a pixel aspect ratio of " << par <<
        " but " << n->getName_mt_safe().c_str() << " rendered an image with a pixel aspect ratio of " << inputImg->getPixelAspectRatio();
    }
    
    ///If the plug-in doesn't support the render scale, but the image is downscaled, up-scale it.
    ///Note that we do NOT cache it because it is really low def!
    if ( !dontUpscale  && renderFullScaleThenDownscale && !renderScaleOneUpstreamIfRenderScaleSupportDisabled ) {
        assert(inputImgMipMapLevel != 0);
        ///Resize the image according to the requested scale
        Natron::ImageBitDepthEnum bitdepth = inputImg->getBitDepth();
        RectI bounds;
        inputImg->getRoD().toPixelEnclosing(0, par, &bounds);
        boost::shared_ptr<Natron::Image> rescaledImg( new Natron::Image(inputImg->getComponents(), inputImg->getRoD(),
                                                                        bounds, 0, par, bitdepth) );
        inputImg->upscaleMipMap(inputImg->getBounds(), inputImgMipMapLevel, 0, rescaledImg.get());
        if (roiPixel) {
            RectD canonicalPixelRoI;
            pixelRoI.toCanonical(inputImgMipMapLevel, par, rod, &canonicalPixelRoI);
            canonicalPixelRoI.toPixelEnclosing(0, par, roiPixel);
        }
        //inputImg->scaleBox( inputImg->getBounds(), rescaledImg.get() );
        return rescaledImg;
        
        
    } else {
        
        if (inputImagesThreadLocal.empty()) {
            ///If the effect is analysis (e.g: Tracker) there's no input images in the tread local storage, hence add it
            _imp->addInputImageTempPointer(inputImg);
        }
        return inputImg;
        
    }
    
} // getImage 

void
EffectInstance::calcDefaultRegionOfDefinition(U64 /*hash*/,SequenceTime /*time*/,
                                              int /*view*/,
                                              const RenderScale & /*scale*/,
                                              RectD *rod)
{
    Format projectDefault;

    getRenderFormat(&projectDefault);
    *rod = RectD( projectDefault.left(), projectDefault.bottom(), projectDefault.right(), projectDefault.top() );
}

Natron::StatusEnum
EffectInstance::getRegionOfDefinition(U64 hash,SequenceTime time,
                                      const RenderScale & scale,
                                      int view,
                                      RectD* rod) //!< rod is in canonical coordinates
{
    bool firstInput = true;
    RenderScale renderMappedScale = scale;

    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    for (int i = 0; i < getMaxInputCount(); ++i) {
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            RectD inputRod;
            bool isProjectFormat;
            StatusEnum st = input->getRegionOfDefinition_public(hash,time, renderMappedScale, view, &inputRod, &isProjectFormat);
            assert(inputRod.x2 >= inputRod.x1 && inputRod.y2 >= inputRod.y1);
            if (st == eStatusFailed) {
                return st;
            }

            if (firstInput) {
                *rod = inputRod;
                firstInput = false;
            } else {
                rod->merge(inputRod);
            }
            assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
        }
    }

    return eStatusReplyDefault;
}

bool
EffectInstance::ifInfiniteApplyHeuristic(U64 hash,
                                         SequenceTime time,
                                         const RenderScale & scale,
                                         int view,
                                         RectD* rod) //!< input/output
{
    /*If the rod is infinite clip it to the project's default*/

    Format projectFormat;
    getRenderFormat(&projectFormat);
    RectD projectDefault = projectFormat.toCanonicalFormat();
    /// FIXME: before removing the assert() (I know you are tempted) please explain (here: document!) if the format rectangle can be empty and in what situation(s)
    assert( !projectDefault.isNull() );

    assert(rod);
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
    bool x1Infinite = rod->x1 <= kOfxFlagInfiniteMin;
    bool y1Infinite = rod->y1 <= kOfxFlagInfiniteMin;
    bool x2Infinite = rod->x2 >= kOfxFlagInfiniteMax;
    bool y2Infinite = rod->y2 >= kOfxFlagInfiniteMax;

    ///Get the union of the inputs.
    RectD inputsUnion;

    ///Do the following only if one coordinate is infinite otherwise we wont need the RoD of the input
    if (x1Infinite || y1Infinite || x2Infinite || y2Infinite) {
        // initialize with the effect's default RoD, because inputs may not be connected to other effects (e.g. Roto)
        calcDefaultRegionOfDefinition(hash,time,view, scale, &inputsUnion);
        bool firstInput = true;
        for (int i = 0; i < getMaxInputCount(); ++i) {
            Natron::EffectInstance* input = getInput(i);
            if (input) {
                RectD inputRod;
                bool isProjectFormat;
                RenderScale inputScale = scale;
                if (input->supportsRenderScaleMaybe() == eSupportsNo) {
                    inputScale.x = inputScale.y = 1.;
                }
                StatusEnum st = input->getRegionOfDefinition_public(hash,time, inputScale, view, &inputRod, &isProjectFormat);
                if (st != eStatusFailed) {
                    if (firstInput) {
                        inputsUnion = inputRod;
                        firstInput = false;
                    } else {
                        inputsUnion.merge(inputRod);
                    }
                }
            }
        }
    }
    ///If infinite : clip to inputsUnion if not null, otherwise to project default

    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    bool isProjectFormat = false;
    if (x1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x1 = std::min(inputsUnion.x1, projectDefault.x1);
        } else {
            rod->x1 = projectDefault.x1;
            isProjectFormat = true;
        }
        rod->x2 = std::max(rod->x1, rod->x2);
    }
    if (y1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y1 = std::min(inputsUnion.y1, projectDefault.y1);
        } else {
            rod->y1 = projectDefault.y1;
            isProjectFormat = true;
        }
        rod->y2 = std::max(rod->y1, rod->y2);
    }
    if (x2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x2 = std::max(inputsUnion.x2, projectDefault.x2);
        } else {
            rod->x2 = projectDefault.x2;
            isProjectFormat = true;
        }
        rod->x1 = std::min(rod->x1, rod->x2);
    }
    if (y2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y2 = std::max(inputsUnion.y2, projectDefault.y2);
        } else {
            rod->y2 = projectDefault.y2;
            isProjectFormat = true;
        }
        rod->y1 = std::min(rod->y1, rod->y2);
    }
    if ( isProjectFormat && !isGenerator() ) {
        isProjectFormat = false;
    }
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

    return isProjectFormat;
} // ifInfiniteApplyHeuristic

void
EffectInstance::getRegionsOfInterest(SequenceTime /*time*/,
                                     const RenderScale & /*scale*/,
                                     const RectD & /*outputRoD*/, //!< the RoD of the effect, in canonical coordinates
                                     const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                     int /*view*/,
                                     EffectInstance::RoIMap* ret)
{
    for (int i = 0; i < getMaxInputCount(); ++i) {
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            ret->insert( std::make_pair(input, renderWindow) );
        }
    }
}

EffectInstance::FramesNeededMap
EffectInstance::getFramesNeeded(SequenceTime time)
{
    EffectInstance::FramesNeededMap ret;
    RangeD defaultRange;
    
    defaultRange.min = defaultRange.max = time;
    std::vector<RangeD> ranges;
    ranges.push_back(defaultRange);
    for (int i = 0; i < getMaxInputCount(); ++i) {
        if (isInputRotoBrush(i)) {
            ret.insert( std::make_pair(i, ranges) );
        } else {
            Natron::EffectInstance* input = getInput(i);
            if (input) {
                ret.insert( std::make_pair(i, ranges) );
            }
        }
    }

    return ret;
}

void
EffectInstance::getFrameRange(SequenceTime *first,
                              SequenceTime *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    for (int i = 0; i < getMaxInputCount(); ++i) {
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            SequenceTime inpFirst,inpLast;
            input->getFrameRange(&inpFirst, &inpLast);
            if (i == 0) {
                *first = inpFirst;
                *last = inpLast;
            } else {
                if (inpFirst < *first) {
                    *first = inpFirst;
                }
                if (inpLast > *last) {
                    *last = inpLast;
                }
            }
        }
    }
}

EffectInstance::NotifyRenderingStarted_RAII::NotifyRenderingStarted_RAII(Node* node)
: _node(node)
{
    _didEmit = node->notifyRenderingStarted();
}

EffectInstance::NotifyRenderingStarted_RAII::~NotifyRenderingStarted_RAII()
{
    if (_didEmit) {
        _node->notifyRenderingEnded();
    }
}

EffectInstance::NotifyInputNRenderingStarted_RAII::NotifyInputNRenderingStarted_RAII(Node* node,int inputNumber)
: _node(node)
, _inputNumber(inputNumber)
{
    _didEmit = node->notifyInputNIsRendering(inputNumber);
}

EffectInstance::NotifyInputNRenderingStarted_RAII::~NotifyInputNRenderingStarted_RAII()
{
    if (_didEmit) {
        _node->notifyInputNIsFinishedRendering(_inputNumber);
    }
}

void
EffectInstance::getImageFromCacheAndConvertIfNeeded(bool useCache,
                                                    bool useDiskCache,
                                                    const Natron::ImageKey& key,
                                                    unsigned int mipMapLevel,
                                                    Natron::ImageBitDepthEnum bitdepth,
                                                    Natron::ImageComponentsEnum components,
                                                    Natron::ImageBitDepthEnum nodePrefDepth,
                                                    Natron::ImageComponentsEnum nodePrefComps,
                                                    int /*channelForAlpha*/,
                                                    const RectD& rod,
                                                    const std::list<boost::shared_ptr<Natron::Image> >& inputImages,
                                                    boost::shared_ptr<Natron::Image>* image)
{
    ImageList cachedImages;
    bool isCached = false;
    
    ///Find first something in the input images list
    if (!inputImages.empty()) {
        for (std::list<boost::shared_ptr<Image> >::const_iterator it = inputImages.begin(); it != inputImages.end(); ++it) {
            const ImageKey& imgKey = (*it)->getKey();
            if (imgKey == key) {
                cachedImages.push_back(*it);
                isCached = true;
            }
        }
    }
    
    if (!isCached) {
        isCached = !useDiskCache ? Natron::getImageFromCache(key,&cachedImages) : Natron::getImageFromDiskCache(key, &cachedImages);
    }
    
    if (isCached) {
        
        ///A ptr to a higher resolution of the image or an image with different comps/bitdepth
        ImagePtr imageToConvert;
        
        bool imageConversionNeeded = false;
        
        for (ImageList::iterator it = cachedImages.begin(); it!=cachedImages.end(); ++it) {
            unsigned int imgMMlevel = (*it)->getMipMapLevel();
            ImageComponentsEnum imgComps = (*it)->getComponents();
            ImageBitDepthEnum imgDepth = (*it)->getBitDepth();
            
            if ( (*it)->getParams()->isRodProjectFormat() ) {
                ////If the image was cached with a RoD dependent on the project format, but the project format changed,
                ////just discard this entry
                Format projectFormat;
                getRenderFormat(&projectFormat);
                RectD canonicalProject = projectFormat.toCanonicalFormat();
                if ( canonicalProject != (*it)->getRoD() ) {
                    appPTR->removeFromNodeCache(*it);
                    continue;
                }
            }
            
            ///Throw away images that are not even what the node want to render
            if (imgComps != nodePrefComps || imgDepth != nodePrefDepth) {
                appPTR->removeFromNodeCache(*it);
                continue;
            }
            
            if (imgMMlevel == mipMapLevel && Image::hasEnoughDataToConvert(imgComps,components) &&
            getSizeOfForBitDepth(imgDepth) >= getSizeOfForBitDepth(bitdepth)/* && imgComps == components && imgDepth == bitdepth*/) {
                
                ///We found  a matching image
                
                *image = *it;
                break;
            } else {
                
                
                if (imgMMlevel > mipMapLevel || !Image::hasEnoughDataToConvert(imgComps,components) ||
                    getSizeOfForBitDepth(imgDepth) < getSizeOfForBitDepth(bitdepth)) {
                    ///Either smaller resolution or not enough components or bit-depth is not as deep, don't use the image
                    continue;
                }
                
                
                ///Same resolution but different comps/bitdepth
                if (imgMMlevel == mipMapLevel) {
                    imageConversionNeeded = true;
                    imageToConvert = *it;
                    break;
                }
                
                assert(imgMMlevel < mipMapLevel);
                
                if (!imageToConvert) {
                    imageToConvert = *it;
                    imageConversionNeeded = false;//imgComps != components || imgDepth != bitdepth;

                } else {
                    if (imgMMlevel < imageToConvert->getMipMapLevel()) {
                        imageToConvert = *it;
                        imageConversionNeeded = false;//imgComps != components || imgDepth != bitdepth;

                    }
                }
                
                
            }
        }
        
        if (imageToConvert && !*image) {

            //Take the lock after getting the image from the cache
            ///to make sure a thread will not attempt to write to the image while its being allocated.
            ///When calling allocateMemory() on the image, the cache already has the lock since it added it
            ///so taking this lock now ensures the image will be allocated completetly

            ImageLocker locker(this, imageToConvert);

            ///Edit: don't try to convert now, instead continue rendering the original image and convert afterwards
//            if (imageConversionNeeded) {
//                
//                boost::shared_ptr<ImageParams> imageParams(new ImageParams(*imageToConvert->getParams()));
//                imageParams->setComponents(components);
//                imageParams->setBitDepth(bitdepth);
//                
//                
//                ///Allocate the image in the cache, it may be useful later
//                ImageLocker imageLock(this);
//                
//                boost::shared_ptr<Image> img;
//                if (useCache) {
//                    bool cached = !useDiskCache ? Natron::getImageFromCacheOrCreate(key, imageParams, &imageLock, &img) :
//                                                  Natron::getImageFromDiskCacheOrCreate(key, imageParams, &imageLock, &img);
//                    assert(img);
//                    
//                    if (!cached) {
//                        img->allocateMemory();
//                    } else {
//                        ///lock the image because it might not be allocated yet
//                        imageLock.lock(img);
//                    }
//                } else {
//                    img.reset(new Image(key,imageParams));
//                }
//                
//                bool unPremultIfNeeded = getOutputPremultiplication() == eImagePremultiplicationPremultiplied;
//                
//                imageToConvert->convertToFormat(imageToConvert->getBounds(),
//                                                               getApp()->getDefaultColorSpaceForBitDepth( imageToConvert->getBitDepth() ),
//                                                               getApp()->getDefaultColorSpaceForBitDepth(bitdepth), channelForAlpha, false,
//                                                useCache  && imageToConvert->usesBitMap(),
//                                                unPremultIfNeeded, img.get());
//                
//                imageToConvert = img;
//                
//            }
            
            if (imageToConvert->getMipMapLevel() != mipMapLevel) {
                boost::shared_ptr<ImageParams> oldParams = imageToConvert->getParams();
                
                boost::shared_ptr<ImageParams> imageParams = Image::makeParams(oldParams->getCost(),
                                                                               rod,
                                                                               oldParams->getPixelAspectRatio(),
                                                                               mipMapLevel,
                                                                               oldParams->isRodProjectFormat(),
                                                                               oldParams->getComponents(),
                                                                               oldParams->getBitDepth(),
                                                                               oldParams->getFramesNeeded());
                
                imageParams->setMipMapLevel(mipMapLevel);
                
                
                ///Allocate the image in the cache, it may be useful later
                ImageLocker imageLock(this);
                
                boost::shared_ptr<Image> img;
                if (useCache) {
                    bool cached = !useDiskCache ? Natron::getImageFromCacheOrCreate(key, imageParams, &imageLock, &img) :
                                                  Natron::getImageFromDiskCacheOrCreate(key, imageParams, &imageLock, &img);
                    assert(img);
                    if (!cached) {
                        img->allocateMemory();
                    } else {
                        ///lock the image because it might not be allocated yet
                        imageLock.lock(img);
                    }
                } else {
                    img.reset(new Image(key, imageParams));
                }
                imageToConvert->downscaleMipMap(imageToConvert->getBounds(),
                                                imageToConvert->getMipMapLevel(), img->getMipMapLevel() ,
                                                useCache && imageToConvert->usesBitMap(),
                                                img.get());
                
                imageToConvert = img;
                
            }
            
            *image = imageToConvert;
            
        } else if (*image) { //  else if (imageToConvert && !*image)
            
            //Take the lock after getting the image from the cache
            ///to make sure a thread will not attempt to write to the image while its being allocated.
            ///When calling allocateMemory() on the image, the cache already has the lock since it added it
            ///so taking this lock now ensures the image will be allocated completetly

            ImageLocker locker(this,*image);
            assert(*image);
        }
        
    }

}

bool
EffectInstance::tryConcatenateTransforms(const RenderRoIArgs& args,
                                         int* inputTransformNb,
                                         Natron::EffectInstance** newInputEffect,
                                         int *newInputNbToFetchFrom,
                                         boost::shared_ptr<Transform::Matrix3x3>* cat,
                                         bool* isResultIdentity)
{
    
    bool canTransform = getCanTransform();
    Natron::EffectInstance* inputTransformEffect = 0;
    
    //An effect might not be able to concatenate transforms but can still apply a transform (e.g CornerPinMasked)
    bool canApplyTransform = getCanApplyTransform(&inputTransformEffect);
    
    if (canTransform || canApplyTransform) {
        
        Transform::Matrix3x3 thisNodeTransform;
        
        *inputTransformNb = getInputNumber(inputTransformEffect);
        assert(*inputTransformNb != -1);
        
        assert(inputTransformEffect);
        
        std::list<Transform::Matrix3x3> matricesByOrder; // from downstream to upstream
        
        Natron::EffectInstance* inputToTransform = 0;
        
        if (canTransform) {
            inputToTransform = 0;
            Natron::StatusEnum stat = getTransform_public(args.time, args.scale, args.view, &inputToTransform, &thisNodeTransform);
            if (stat == eStatusOK) {
                inputTransformEffect = inputToTransform;
            }
        }
        
        *newInputEffect = inputTransformEffect;
        *newInputNbToFetchFrom = *inputTransformNb;
       
        // recursion upstream
        
        bool inputCanTransform = false;
        bool inputIsDisabled  =  inputTransformEffect->getNode()->isNodeDisabled();
        
        if (!inputIsDisabled) {
            inputCanTransform = inputTransformEffect->getCanTransform();
        }
        
        
        while (inputTransformEffect && (inputCanTransform || inputIsDisabled)) {
            //input is either disabled, or identity or can concatenate a transform too
            
            if (inputIsDisabled) {
                
                int prefInput;
                inputTransformEffect = inputTransformEffect->getNearestNonDisabledPrevious(&prefInput);
                if (prefInput == -1) {
                    *newInputEffect = 0;
                    *newInputNbToFetchFrom = -1;
                    *inputTransformNb = -1;
                    return false;
                }
                
                *newInputEffect = inputTransformEffect;
                *newInputNbToFetchFrom = prefInput;
                inputTransformEffect = inputTransformEffect->getInput(prefInput);
                            
            } else if (inputCanTransform) {
                Transform::Matrix3x3 m;
                inputToTransform = 0;
                Natron::StatusEnum stat = inputTransformEffect->getTransform_public(args.time, args.scale, args.view, &inputToTransform, &m);
                if (stat == eStatusOK) {
                    matricesByOrder.push_back(m);
                    *newInputNbToFetchFrom = inputTransformEffect->getInputNumber(inputToTransform);
                    *newInputEffect = inputTransformEffect;
                    inputTransformEffect = inputToTransform;
                } else {
                    break;
                }
            } else  {
                assert(false);
            }
            
            
            if (inputTransformEffect) {
                inputIsDisabled = inputTransformEffect->getNode()->isNodeDisabled();
                if (!inputIsDisabled) {
                    inputCanTransform = inputTransformEffect->getCanTransform();
                }
            }
        }
        
        
        if (inputTransformEffect && !matricesByOrder.empty()) {
            
            assert(!inputCanTransform && !inputIsDisabled);
            assert(*newInputEffect);
            
            ///Now actually concatenate matrices together
            cat->reset(new Transform::Matrix3x3);
            std::list<Transform::Matrix3x3>::reverse_iterator it = matricesByOrder.rbegin();
            **cat = *it;
            ++it;
            while (it != matricesByOrder.rend()) {
                **cat = Transform::matMul(**cat, *it);
                ++it;
            }
            
            if (canTransform) {
                //Check if the result of all the matrices is an identity
                Transform::Matrix3x3 tmp = Transform::matMul(**cat, thisNodeTransform);
                *isResultIdentity = tmp.isIdentity();
            } else {
                *isResultIdentity = false;
            }
            
            
            return true;
        }
    }
    
    *newInputEffect = 0;
    *newInputNbToFetchFrom = -1;
    *inputTransformNb = -1;

    return false;

}

class TransformReroute_RAII
{
    int inputNb;
    Natron::EffectInstance* self;
public:
    
    TransformReroute_RAII(EffectInstance* self,int inputNb,Natron::EffectInstance* input,int newInputNb,const boost::shared_ptr<Transform::Matrix3x3>& matrix)
    : inputNb(inputNb)
    , self(self)
    {
        self->rerouteInputAndSetTransform(inputNb, input, newInputNb, *matrix);
    }
    
    ~TransformReroute_RAII()
    {
        self->clearTransform(inputNb);
    }
};


boost::shared_ptr<Natron::Image>
EffectInstance::renderRoI(const RenderRoIArgs & args)
{
   
    ParallelRenderArgs& frameRenderArgs = _imp->frameRenderArgs.localData();
    if (!frameRenderArgs.validArgs) {
        qDebug() << "Thread-storage for the render of the frame was not set, this is a bug.";
        frameRenderArgs.time = args.time;
        frameRenderArgs.nodeHash = getHash();
        frameRenderArgs.view = args.view;
        frameRenderArgs.isSequentialRender = false;
        frameRenderArgs.isRenderResponseToUserInteraction = true;
        boost::shared_ptr<RotoContext> roto = _node->getRotoContext();
        if (roto) {
            frameRenderArgs.rotoAge = roto->getAge();
        } else {
            frameRenderArgs.rotoAge = 0;
        }
        frameRenderArgs.validArgs = true;
    }
    
    ///The args must have been set calling setParallelRenderArgs
    assert(frameRenderArgs.validArgs);
    
    ///For writer we never want to cache otherwise the next time we want to render it will skip writing the image on disk!
    bool byPassCache = args.byPassCache;

    ///Use the hash at this time, and then copy it to the clips in the thread local storage to use the same value
    ///through all the rendering of this frame.
    U64 nodeHash = frameRenderArgs.nodeHash;
 
    const double par = getPreferredAspectRatio();

    boost::shared_ptr<Image> image;
    RectD rod; //!< rod is in canonical coordinates
    bool isProjectFormat = false;
    const unsigned int mipMapLevel = args.mipMapLevel;
    SupportsEnum supportsRS = supportsRenderScaleMaybe();
    ///This flag is relevant only when the mipMapLevel is different than 0. We use it to determine
    ///wether the plug-in should render in the full scale image, and then we downscale afterwards or
    ///if the plug-in can just use the downscaled image to render.
    bool renderFullScaleThenDownscale = (supportsRS == eSupportsNo && mipMapLevel != 0);
    unsigned int renderMappedMipMapLevel;
    if (renderFullScaleThenDownscale) {
        renderMappedMipMapLevel = 0;
    } else {
        renderMappedMipMapLevel = args.mipMapLevel;
    }
    RenderScale renderMappedScale;
    renderMappedScale.x = renderMappedScale.y = Image::getScaleFromMipMapLevel(renderMappedMipMapLevel);
    assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );

    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = _node->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
    }
    
 
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Get the RoD ///////////////////////////////////////////////////////////////
    
    ///if the rod is already passed as parameter, just use it and don't call getRegionOfDefinition
    if ( !args.preComputedRoD.isNull() ) {
        rod = args.preComputedRoD;
    } else {
        ///before allocating it we must fill the RoD of the image we want to render
        assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        StatusEnum stat = getRegionOfDefinition_public(nodeHash,args.time, renderMappedScale, args.view, &rod, &isProjectFormat);

        ///The rod might be NULL for a roto that has no beziers and no input
        if ( (stat == eStatusFailed) || rod.isNull() ) {
            ///if getRoD fails, just return a NULL ptr
            return boost::shared_ptr<Natron::Image>();
        }
        if ( (supportsRS == eSupportsMaybe) && (renderMappedMipMapLevel != 0) ) {
            // supportsRenderScaleMaybe may have changed, update it
            supportsRS = supportsRenderScaleMaybe();
            renderFullScaleThenDownscale = (supportsRS == eSupportsNo && mipMapLevel != 0);
            if (renderFullScaleThenDownscale) {
                renderMappedScale.x = renderMappedScale.y = 1.;
                renderMappedMipMapLevel = 0;
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End get RoD ///////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check if effect is identity ///////////////////////////////////////////////////////////////
    {
        SequenceTime inputTimeIdentity = 0.;
        int inputNbIdentity;
        
        assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        bool identity;
        try {
            identity = isIdentity_public(nodeHash,args.time, renderMappedScale, rod, par, args.view, &inputTimeIdentity, &inputNbIdentity);
        } catch (...) {
            return boost::shared_ptr<Natron::Image>();
        }
        
        if ( (supportsRS == eSupportsMaybe) && (renderMappedMipMapLevel != 0) ) {
            // supportsRenderScaleMaybe may have changed, update it
            supportsRS = supportsRenderScaleMaybe();
            renderFullScaleThenDownscale = (supportsRS == eSupportsNo && mipMapLevel != 0);
            if (renderFullScaleThenDownscale) {
                renderMappedScale.x = renderMappedScale.y = 1.;
                renderMappedMipMapLevel = 0;
            }
        }
        
        if (identity) {
            ///The effect is an identity but it has no inputs
            if (inputNbIdentity == -1) {
                return boost::shared_ptr<Natron::Image>();
            } else if (inputNbIdentity == -2) {
                // there was at least one crash if you set the first frame to a negative value
                assert(inputTimeIdentity != args.time);
                if (inputTimeIdentity != args.time) { // be safe in release mode!
                    ///This special value of -2 indicates that the plugin is identity of itself at another time
                    RenderRoIArgs argCpy = args;
                    argCpy.time = inputTimeIdentity;
                    
                    return renderRoI(argCpy);
                }
            }
            
            int firstFrame,lastFrame;
            getFrameRange_public(nodeHash, &firstFrame, &lastFrame);
            
            RectD canonicalRoI;
            ///WRONG! We can't clip against the RoD of *this* effect. We should clip against the RoD of the input effect, but this is done
            ///later on for us already.
            //args.roi.toCanonical(args.mipMapLevel, rod, &canonicalRoI);
            args.roi.toCanonical_noClipping(args.mipMapLevel, par,  &canonicalRoI);
            RoIMap inputsRoI;
            inputsRoI.insert( std::make_pair(getInput(inputNbIdentity), canonicalRoI) );
            Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,
                                                        inputsRoI,
                                                        rod,
                                                        args.roi,
                                                        args.time,
                                                        args.view,
                                                        args.channelForAlpha,
                                                        identity,
                                                        inputTimeIdentity,
                                                        inputNbIdentity,
                                                        boost::shared_ptr<Image>(),
                                                        firstFrame,
                                                        lastFrame);
            Natron::EffectInstance* inputEffectIdentity = getInput(inputNbIdentity);
            if (inputEffectIdentity) {
                ///we don't need to call getRegionOfDefinition and getFramesNeeded if the effect is an identity
                image = getImage(inputNbIdentity,
                                 inputTimeIdentity,
                                 args.scale,
                                 args.view,
                                 &canonicalRoI,
                                 args.components,
                                 args.bitdepth,
                                 par,
                                 true,
                                 NULL);

            }
            
            return image;
            
        } // if (identity)
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End identity check ///////////////////////////////////////////////////////////////

    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Transform concatenations ///////////////////////////////////////////////////////////////
    ///Try to concatenate transform effects
    boost::shared_ptr<Transform::Matrix3x3> transformMatrix;
    Natron::EffectInstance* newInputAfterConcat = 0;
    int transformInputNb = -1;
    int newInputNb = -1;
    bool isResultingTransformIdentity;
    bool hasConcat;
    
    if (appPTR->getCurrentSettings()->isTransformConcatenationEnabled()) {
        hasConcat = tryConcatenateTransforms(args, &transformInputNb, &newInputAfterConcat, &newInputNb, &transformMatrix, &isResultingTransformIdentity);
    } else {
        hasConcat = false;
    }
    
    
    assert((!hasConcat && (transformInputNb == -1) && (newInputAfterConcat == 0) && (newInputNb == -1)) ||
           (hasConcat && transformMatrix && (transformInputNb != -1) && newInputAfterConcat && (newInputNb != -1)));
    
    boost::shared_ptr<TransformReroute_RAII> transformConcatenationReroute;
    
    if (hasConcat) {
        if (isResultingTransformIdentity) {
            ///The transform matrix is identity! fetch directly the image from inputTransformEffect
            return newInputAfterConcat->renderRoI(args);
        }
        ///Ok now we have the concatenation of all matrices, set it on the associated clip and reroute the tree
        transformConcatenationReroute.reset(new TransformReroute_RAII(this, transformInputNb, newInputAfterConcat, newInputNb, transformMatrix));
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End transform concatenations//////////////////////////////////////////////////////////
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Look-up the cache ///////////////////////////////////////////////////////////////
    bool createInCache = shouldCacheOutput();

    bool isFrameVaryingOrAnimated = isFrameVaryingOrAnimated_Recursive();
    Natron::ImageKey key = Natron::Image::makeKey(nodeHash, isFrameVaryingOrAnimated, args.time, args.view);
    
    bool useDiskCacheNode = dynamic_cast<DiskCacheNode*>(this) != NULL;

    {
        ///If the last rendered image had a different hash key (i.e a parameter changed or an input changed)
        ///just remove the old image from the cache to recycle memory.
        ///We also do this if the mipmap level is different (e.g: the user is zooming in/out) because
        ///anyway the ViewerCache will have the texture cached and it would be redundant to keep this image
        ///in the cache since the ViewerCache already has it ready.
        boost::shared_ptr<Image> lastRenderedImage;
        U64 lastRenderHash;
        {
            QMutexLocker l(&_imp->lastRenderArgsMutex);
            lastRenderedImage = _imp->lastImage;
            lastRenderHash = _imp->lastRenderHash;
        }
        if ( lastRenderedImage && lastRenderHash != nodeHash ) {
            ///once we got it remove it from the cache
            if (!useDiskCacheNode) {
                appPTR->removeAllImagesFromCacheWithMatchingKey(lastRenderHash);
            } else {
                appPTR->removeAllImagesFromDiskCacheWithMatchingKey(lastRenderHash);
            }
            {
                QMutexLocker l(&_imp->lastRenderArgsMutex);
                _imp->lastImage.reset();
            }
        }
    }
    
    Natron::ImageBitDepthEnum outputDepth;
    Natron::ImageComponentsEnum outputComponents;
    getPreferredDepthAndComponents(-1, &outputComponents, &outputDepth);

    boost::shared_ptr<ImageParams> cachedImgParams;
    getImageFromCacheAndConvertIfNeeded(createInCache, useDiskCacheNode, key, renderMappedMipMapLevel,args.bitdepth, args.components,
                                        outputDepth, outputComponents,args.channelForAlpha,rod,args.inputImagesList, &image);

    
    if (byPassCache) {
        if (image) {
            appPTR->removeFromNodeCache(key.getHash());
            image.reset();
        }
        //For writers, we always want to call the render action, but we still want to use the cache for nodes upstream
        if (isWriter()) {
            byPassCache = false;
        }
    }
    if (image) {
        cachedImgParams = image->getParams();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End cache lookup//////////////////////////////////////////////////////////

    boost::shared_ptr<Natron::Image> downscaledImage = image;
    
    FramesNeededMap framesNeeded;
    if (image) {
        framesNeeded = cachedImgParams->getFramesNeeded();
    } else {
        framesNeeded = getFramesNeeded_public(args.time);
    }
    
    
    RoIMap inputsRoi;
    std::list <boost::shared_ptr<Image> > inputImages;

    /*We pass the 2 images (image & downscaledImage). Depending on the context we want to render in one or the other one:
     If (renderFullScaleThenDownscale and renderScaleOneUpstreamIfRenderScaleSupportDisabled)
     the image that is held by the cache will be 'image' and it will then be downscaled if needed.
     However if the render scale is not supported but input images are not rendered at full-scale  ,
     we don't want to cache the full-scale image because it will be low res. Instead in that case we cache the downscaled image
     */
    bool useImageAsOutput;
    RectI roi;
    
    if (renderFullScaleThenDownscale && renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
        
        //We cache 'image', hence the RoI should be expressed in its coordinates
        //renderRoIInternal should check the bitmap of 'image' and not downscaledImage!
        RectD canonicalRoI;
        args.roi.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
        canonicalRoI.toPixelEnclosing(0, par, &roi);
        useImageAsOutput = true;
    } else {
        
        //In that case the plug-in either supports render scale or doesn't support render scale but uses downscaled inputs
        //renderRoIInternal should check the bitmap of downscaledImage and not 'image'!
        roi = args.roi;
        useImageAsOutput = false;
    }
    
    
    RectI downscaledImageBounds,upscaledImageBounds;
    rod.toPixelEnclosing(args.mipMapLevel, par, &downscaledImageBounds);
    rod.toPixelEnclosing(0, par, &upscaledImageBounds);
    

    bool tilesSupported = supportsTiles();

    ///Make sure the RoI falls within the image bounds
    ///Intersection will be in pixel coordinates
    if (tilesSupported) {
        if (useImageAsOutput) {
            if (!roi.intersect(upscaledImageBounds, &roi)) {
                return ImagePtr();
            }
            if (!createInCache) {
                ///If we don't cache the image, just allocate the roi
                upscaledImageBounds.intersect(roi, &upscaledImageBounds);
            }
            assert(roi.x1 >= upscaledImageBounds.x1 && roi.y1 >= upscaledImageBounds.y1 &&
                   roi.x2 <= upscaledImageBounds.x2 && roi.y2 <= upscaledImageBounds.y2);
            
        } else {
            if (!roi.intersect(downscaledImageBounds, &roi)) {
                return ImagePtr();
            }
            if (!createInCache) {
                ///If we don't cache the image, just allocate the roi
                downscaledImageBounds.intersect(roi, &downscaledImageBounds);
            }
            assert(roi.x1 >= downscaledImageBounds.x1 && roi.y1 >= downscaledImageBounds.y1 &&
                   roi.x2 <= downscaledImageBounds.x2 && roi.y2 <= downscaledImageBounds.y2);
        }
    } else {
        roi = useImageAsOutput ? upscaledImageBounds : downscaledImageBounds;
    }
    
    RectD canonicalRoI;
    if (useImageAsOutput) {
        roi.toCanonical(0, par, rod, &canonicalRoI);
    } else {
        roi.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
    }
    
    /// If the list is empty then we already rendered it all
    /// Each rect in this list is in pixel coordinate (downscaled)
    std::list<RectI> rectsToRender;
    
    ///In the event where we had the image from the cache, but it wasn't completly rendered over the RoI but the cache was almost full,
    ///we don't hold a pointer to it, allowing the cache to free it.
    ///Hence after rendering all the input images, we redo a cache look-up to check whether the image is still here
    bool redoCacheLookup = false;
    

    if (image) {
        
        ///We check what is left to render.
        rectsToRender = image->getRestToRender(roi);
        
        if (!rectsToRender.empty() && appPTR->isNodeCacheAlmostFull()) {
            ///The node cache is almost full and we need to render  something in the image, if we hold a pointer to this image here
            ///we might recursively end-up in this same situation at each level of the render tree, ending with all images of each level
            ///being held in memory.
            ///Our strategy here is to clear the pointer, hence allowing the cache to remove the image, and ask the inputs to render the full RoI
            ///instead of the rest to render. This way, even if the image is cleared from the cache we already have rendered the full RoI anyway.
            image.reset();
            cachedImgParams.reset();
            rectsToRender.clear();
            rectsToRender.push_back(roi);
            redoCacheLookup = true;
        }
        
        
        
        ///If the effect doesn't support tiles and it has something left to render, just render the bounds again
        ///Note that it should NEVER happen because if it doesn't support tiles in the first place, it would
        ///have rendered the rod already.
        if (!tilesSupported && !rectsToRender.empty()) {
            ///if the effect doesn't support tiles, just render the whole rod again even though
            rectsToRender.clear();
            rectsToRender.push_back( image->getBounds() );
        }
        
    } else {
        
        if (tilesSupported) {
            rectsToRender.push_back(roi);
        } else {
            rectsToRender.push_back(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds);
        }
    }

    if (redoCacheLookup) {
        getImageFromCacheAndConvertIfNeeded(createInCache, useDiskCacheNode, key, renderMappedMipMapLevel,
                                            args.bitdepth, args.components,
                                            outputDepth,outputComponents,
                                            args.channelForAlpha,rod,args.inputImagesList, &image);
        if (image) {
            cachedImgParams = image->getParams();
            ///We check what is left to render.
            rectsToRender = image->getRestToRender(roi);
        }
    }
    
    ///Pre-render input images before allocating the image if we need to render
    if (!rectsToRender.empty()) {
        if (!renderInputImagesForRoI(createInCache,
                                     args.time,
                                     args.view,
                                     par,
                                     nodeHash,
                                     frameRenderArgs.rotoAge,
                                     rod,
                                     roi,
                                     canonicalRoI,
                                     transformMatrix,
                                     transformInputNb,
                                     newInputNb,
                                     newInputAfterConcat,
                                     args.mipMapLevel,
                                     args.scale,
                                     renderMappedScale,
                                     renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                     byPassCache,
                                     framesNeeded,
                                     &inputImages,
                                     &inputsRoi)) {
            return ImagePtr();
        }
    }
    
    ///We hold our input images in thread-storage, so that the getImage function can find them afterwards, even if the node doesn't cache its output.
    boost::shared_ptr<InputImagesHolder_RAII> inputImagesHolder;
    if (!rectsToRender.empty() && !inputImages.empty()) {
        inputImagesHolder.reset(new InputImagesHolder_RAII(inputImages,&_imp->inputImages));
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Allocate image in the cache ///////////////////////////////////////////////////////////////
    if (!image) {
        
        
        ///The image is not cached
        
        // why should the rod be empty here?
        assert( !rod.isNull() );
        
        
        //Controls whether images are stored on disk or in RAM, 0 = RAM, 1 = mmap
        int cost = useDiskCacheNode ? 1 : 0;
    
        //If we're rendering full scale and with input images at full scale, don't cache the downscale image since it is cheap to
        //recreate, instead cache the full-scale image
        if (useImageAsOutput) {
            
            downscaledImage.reset( new Natron::Image(args.components, rod, downscaledImageBounds, args.mipMapLevel, par, args.bitdepth, true) );
            
        } else {

            ///Cache the image with the requested components instead of the remapped ones
            cachedImgParams = Natron::Image::makeParams(cost,
                                                        rod,
                                                        downscaledImageBounds,
                                                        par,
                                                        args.mipMapLevel,
                                                        isProjectFormat,
                                                        args.components,
                                                        args.bitdepth,
                                                        framesNeeded);
            
            //Take the lock after getting the image from the cache or while allocating it
            ///to make sure a thread will not attempt to write to the image while its being allocated.
            ///When calling allocateMemory() on the image, the cache already has the lock since it added it
            ///so taking this lock now ensures the image will be allocated completetly
            
            ImageLocker imageLock(this);
            
            ImagePtr newImage;
            if (createInCache) {
                bool cached = !useDiskCacheNode ? Natron::getImageFromCacheOrCreate(key, cachedImgParams, &imageLock, &newImage) :
                                                  Natron::getImageFromDiskCacheOrCreate(key, cachedImgParams, &imageLock, &newImage);
                
                if (!newImage) {
                    std::stringstream ss;
                    ss << "Failed to allocate an image of ";
                    ss << printAsRAM( cachedImgParams->getElementsCount() * sizeof(Image::data_t) ).toStdString();
                    Natron::errorDialog( QObject::tr("Out of memory").toStdString(),ss.str() );
                    
                    return newImage;
                }
                
                assert(newImage);
                assert(newImage->getRoD() == rod);
                
                if (!cached) {
                    newImage->allocateMemory();
                } else {
                    ///lock the image because it might not be allocated yet
                    imageLock.lock(newImage);
                }
            } else {
                newImage.reset(new Natron::Image(key, cachedImgParams));
            }
            
            image = newImage;
            downscaledImage = image;
        }
        
        if (renderFullScaleThenDownscale) {
            
            ///Allocate the upscaled image
            assert(renderMappedMipMapLevel == 0);

            if (!useImageAsOutput) {
                
                ///The upscaled image will be rendered using input images at lower def... which means really crappy results, don't cache this image!
                image.reset( new Natron::Image(args.components, rod, upscaledImageBounds, renderMappedMipMapLevel, downscaledImage->getPixelAspectRatio(), args.bitdepth, true) );
                
            } else {
                
                boost::shared_ptr<Natron::ImageParams> upscaledImageParams = Natron::Image::makeParams(cost,
                                                                                                       rod,
                                                                                                       upscaledImageBounds,
                                                                                                       downscaledImage->getPixelAspectRatio(),
                                                                                                       0,
                                                                                                       isProjectFormat,
                                                                                                       args.components,
                                                                                                       args.bitdepth,
                                                                                                       framesNeeded);

                if (createInCache) {
                    //The upscaled image will be rendered with input images at full def, it is then the best possibly rendered image so cache it!
                    ImageLocker upscaledImageLock(this);
                    
                    image.reset();
                    
                    bool cached = !useDiskCacheNode ? Natron::getImageFromCacheOrCreate(key, upscaledImageParams, &upscaledImageLock, &image) :
                                                      Natron::getImageFromDiskCacheOrCreate(key, upscaledImageParams, &upscaledImageLock, &image) ;
                    if (!cached) {
                        image->allocateMemory();
                    } else {
                        ///lock the image because it might not be allocated yet
                        upscaledImageLock.lock(image);
                    }
                } else {
                    image.reset(new Natron::Image(key, upscaledImageParams));
                }
            }
            
        }
        
        
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////// End allocation of image in cache ///////////////////////////////////////////////////////////////
    } // if (!image) (the image is not cached)
    else {
        if (renderFullScaleThenDownscale && image->getMipMapLevel() == 0) {
            //Allocate a downscale image that will be cheap to create
            ///The upscaled image will be rendered using input images at lower def... which means really crappy results, don't cache this image!
            RectI bounds;
            rod.toPixelEnclosing(args.mipMapLevel, par, &bounds);
            downscaledImage.reset( new Natron::Image(args.components, rod, downscaledImageBounds, args.mipMapLevel, image->getPixelAspectRatio(), args.bitdepth, true) );
        }
    }
    
    
    
    ///Check if the requested components and bitdepth are supported by the output clip
    
    bool imageConversionNeeded = outputComponents != image->getComponents() || outputDepth != image->getBitDepth();
    
    assert( isSupportedBitDepth(outputDepth) && isSupportedComponent(-1, outputComponents) );
    
    ///The image and downscaled image are pointing to the same image in 2 cases:
    ///1) Proxy mode is turned off
    ///2) Proxy mode is turned on but plug-in supports render scale
    ///Subsequently the image and downscaled image are different only if the plug-in
    ///does not support the render scale and the proxy mode is turned on.
    assert( (image == downscaledImage && !renderFullScaleThenDownscale) ||
           ((image != downscaledImage || image->getMipMapLevel() == downscaledImage->getMipMapLevel()) && renderFullScaleThenDownscale) );

    ///If we reach here, it can be either because the image is cached or not, either way
    ///the image is NOT an identity, and it may have some content left to render.
    EffectInstance::RenderRoIStatusEnum renderRetCode = eRenderRoIStatusImageAlreadyRendered;
    
    if (!rectsToRender.empty()) {
        
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
        boost::shared_ptr<Image> fullScaleMappedImage, downscaledMappedImage, renderMappedImage;
        if ( !rectsToRender.empty() ) {
            if (imageConversionNeeded) {
                if (renderFullScaleThenDownscale) {
                    // TODO: as soon as partial images are supported (RoD != bounds): no need to allocate a full image...
                    // one rectangle should be allocated for each rendered rectangle
                    RectD rod = image->getRoD();
                    RectI bounds;
                    rod.toPixelEnclosing(renderMappedMipMapLevel, par, &bounds);
                    fullScaleMappedImage.reset( new Image(outputComponents, rod, bounds, renderMappedMipMapLevel, image->getPixelAspectRatio(), outputDepth, false) );
                    downscaledMappedImage = downscaledImage;
                    assert( downscaledMappedImage->getBounds() == downscaledImage->getBounds() );
                    assert( fullScaleMappedImage->getBounds() == image->getBounds() );
                } else {
                    RectD rod = downscaledImage->getRoD();
                    RectI bounds;
                    rod.toPixelEnclosing(mipMapLevel, par, &bounds);
                    downscaledMappedImage.reset( new Image(outputComponents, rod, image->getBounds(), mipMapLevel,downscaledImage->getPixelAspectRatio(), outputDepth, false) );
                    fullScaleMappedImage = image;
                    assert( downscaledMappedImage->getBounds() == downscaledImage->getBounds() );
                    assert( fullScaleMappedImage->getBounds() == image->getBounds() );
                }
            } else {
                fullScaleMappedImage = image;
                downscaledMappedImage = downscaledImage;
            }
            if (renderFullScaleThenDownscale) {
                renderMappedImage = fullScaleMappedImage;
            } else {
                renderMappedImage = downscaledMappedImage;
            }
            assert( renderMappedMipMapLevel == renderMappedImage->getMipMapLevel() );
            if (renderFullScaleThenDownscale) {
                assert( renderMappedImage->getBounds() == image->getBounds() );
            } else {
                assert( renderMappedImage->getBounds() == downscaledImage->getBounds() );
            }
            assert( downscaledMappedImage->getBounds() == downscaledImage->getBounds() );
            assert( fullScaleMappedImage->getBounds() == image->getBounds() );
        }

        
        renderRetCode = renderRoIInternal(args.time,
                                          args.mipMapLevel,
                                          args.view,
                                          rectsToRender,
                                          rod,
                                          par,
                                          image,
                                          downscaledImage,
                                          fullScaleMappedImage,
                                          downscaledMappedImage,
                                          renderMappedImage,
                                          useImageAsOutput,
                                          frameRenderArgs.isSequentialRender,
                                          frameRenderArgs.isRenderResponseToUserInteraction,
                                          nodeHash,
                                          args.channelForAlpha,
                                          renderFullScaleThenDownscale,
                                          renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                          inputsRoi,
                                          inputImages);
    }
    ///The image might be in the cache but might need a conversion
    else if (imageConversionNeeded) {
        boost::shared_ptr<Image> tmp( new Image(outputComponents, rod, image->getBounds(), mipMapLevel,image->getPixelAspectRatio(), outputDepth, false) );
        image = tmp;
    }
    
    
    
#ifdef DEBUG
    if (renderRetCode != eRenderRoIStatusRenderFailed && !aborted()) {
        // Kindly check that everything we asked for is rendered!
        std::list<RectI> restToRender = useImageAsOutput ? image->getRestToRender(roi) : downscaledImage->getRestToRender(roi);
        assert(restToRender.empty());
    }
#endif
    
    //We have to return the downscale image, so make sure it has been computed
    if (renderRetCode != eRenderRoIStatusRenderFailed && renderFullScaleThenDownscale && renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
        assert(image->getMipMapLevel() == 0);
        roi.intersect(image->getBounds(), &roi);
        image->downscaleMipMap(roi, 0, args.mipMapLevel, false, downscaledImage.get() );
    }

    if ( aborted() && renderRetCode != eRenderRoIStatusImageAlreadyRendered) {
        
        ///Return a NULL image if the render call was not issued by the result of a call of a plug-in to clipGetImage
        if (!args.calledFromGetImage) {
            return boost::shared_ptr<Image>();
        }
        
    } else if (renderRetCode == eRenderRoIStatusRenderFailed) {
        throw std::runtime_error("Rendering Failed");
    }

    {
        ///flag that this is the last image we rendered
        QMutexLocker l(&_imp->lastRenderArgsMutex);
        _imp->lastRenderHash = nodeHash;
        _imp->lastImage = downscaledImage;
    }

    return downscaledImage;
} // renderRoI


bool
EffectInstance::renderInputImagesForRoI(bool createImageInCache,
                                        SequenceTime time,
                                        int view,
                                        double par,
                                        U64 nodeHash,
                                        U64 rotoAge,
                                        const RectD& rod,
                                        const RectI& downscaledRenderWindow,
                                        const RectD& canonicalRenderWindow,
                                        const boost::shared_ptr<Transform::Matrix3x3>& transformMatrix,
                                        int transformInputNb,
                                        int newTransformedInputNb,
                                        Natron::EffectInstance* transformRerouteInput,
                                        unsigned int mipMapLevel,
                                        const RenderScale & scale,
                                        const RenderScale& renderMappedScale,
                                        bool useScaleOneInputImages,
                                        bool byPassCache,
                                        const FramesNeededMap& framesNeeded,
                                        std::list< boost::shared_ptr<Natron::Image> > *inputImages,
                                        RoIMap* inputsRoi)
{
    getRegionsOfInterest_public(time, renderMappedScale, rod, canonicalRenderWindow, view,inputsRoi);
#ifdef DEBUG
    if (!inputsRoi->empty() && framesNeeded.empty() && !isReader()) {
        qDebug() << getNode()->getName_mt_safe().c_str() << ": getRegionsOfInterestAction returned 1 or multiple input RoI(s) but returned "
        << "an empty list with getFramesNeededAction";
    }
#endif
    if (transformMatrix) {
        //Transform the RoIs by the inverse of the transform matrix (which is in pixel coordinates)
        
        RectD transformedRenderWindow;
        
        
        Natron::EffectInstance* effectInTransformInput = getInput(transformInputNb);
        assert(effectInTransformInput);
        
        RoIMap::iterator foundRoI = inputsRoi->find(effectInTransformInput);
        assert(foundRoI != inputsRoi->end());
        
        // invert it
        Transform::Matrix3x3 invertTransform;
        double det = Transform::matDeterminant(*transformMatrix);
        if (det != 0.) {
            invertTransform = Transform::matInverse(*transformMatrix, det);
        }
        
        Transform::Matrix3x3 canonicalToPixel = Transform::matCanonicalToPixel(par, scale.x,
                                                                               scale.y, false);
        Transform::Matrix3x3 pixelToCanonical = Transform::matPixelToCanonical(par,  scale.x,
                                                                               scale.y, false);
        
        invertTransform = Transform::matMul(Transform::matMul(pixelToCanonical, invertTransform), canonicalToPixel);
        Transform::transformRegionFromRoD(foundRoI->second, invertTransform, transformedRenderWindow);
        
        //Replace the original RoI by the transformed RoI
        inputsRoi->erase(foundRoI);
        inputsRoi->insert(std::make_pair(transformRerouteInput->getInput(newTransformedInputNb), transformedRenderWindow));
        
    }
    
    for (FramesNeededMap::const_iterator it2 = framesNeeded.begin(); it2 != framesNeeded.end(); ++it2) {
        ///We have to do this here because the enabledness of a mask is a feature added by Natron.
        bool inputIsMask = isInputMask(it2->first);
        if ( inputIsMask && !isMaskEnabled(it2->first) ) {
            continue;
        }
        
        EffectInstance* inputEffect;
        if (it2->first == transformInputNb) {
            assert(transformRerouteInput);
            inputEffect = transformRerouteInput->getInput(it2->first);
        } else {
            inputEffect = getInput(it2->first);
        }
        if (inputEffect) {
            ///What region are we interested in for this input effect ? (This is in Canonical coords)
            RoIMap::iterator foundInputRoI = inputsRoi->find(inputEffect);
            assert( foundInputRoI != inputsRoi->end() );
            
            ///Convert to pixel coords the RoI
            if ( foundInputRoI->second.isInfinite() ) {
                throw std::runtime_error(std::string("Plugin ") + this->getPluginLabel() + " asked for an infinite region of interest!");
            }
            
            const double inputPar = inputEffect->getPreferredAspectRatio();
            
            RectI inputRoIPixelCoords;
            foundInputRoI->second.toPixelEnclosing(useScaleOneInputImages ? 0 : mipMapLevel, inputPar, &inputRoIPixelCoords);
            
            ///Notify the node that we're going to render something with the input
            assert(it2->first != -1); //< see getInputNumber
            
            {
                NotifyInputNRenderingStarted_RAII inputNIsRendering_RAII(_node.get(),it2->first);
                
                ///For all frames requested for this node, render the RoI requested.
                for (U32 range = 0; range < it2->second.size(); ++range) {
                    for (int f = std::floor(it2->second[range].min + 0.5); f <= std::floor(it2->second[range].max + 0.5); ++f) {
                        Natron::ImageComponentsEnum inputPrefComps;
                        Natron::ImageBitDepthEnum inputPrefDepth;
                        getPreferredDepthAndComponents(it2->first, &inputPrefComps, &inputPrefDepth);
                        
                        int channelForAlphaInput = inputIsMask ? getMaskChannel(it2->first) : 3;
                        RenderScale scaleOne;
                        scaleOne.x = scaleOne.y = 1.;
                        
                        boost::shared_ptr<Natron::Image> inputImg =
                        inputEffect->renderRoI( RenderRoIArgs(f, //< time
                                                              useScaleOneInputImages ? scaleOne : scale, //< scale
                                                              useScaleOneInputImages ? 0 : mipMapLevel, //< mipmapLevel (redundant with the scale)
                                                              view, //< view
                                                              byPassCache,
                                                              inputRoIPixelCoords, //< roi in pixel coordinates
                                                              RectD(), // < did we precompute any RoD to speed-up the call ?
                                                              inputPrefComps, //< requested comps
                                                              inputPrefDepth,
                                                              channelForAlphaInput) ); //< requested bitdepth
                        
                        if (inputImg) {
                            inputImages->push_back(inputImg);
                        }
                    }
                }
                
            } // NotifyInputNRenderingStarted_RAII inputNIsRendering_RAII(_node.get(),it2->first);
            
            if ( aborted() ) {
                return false;
            }
        }
    }

    
    ///if the node has a roto context, pre-render the roto mask too
    boost::shared_ptr<RotoContext> rotoCtx = _node->getRotoContext();
    if (rotoCtx) {
        Natron::ImageComponentsEnum inputPrefComps;
        Natron::ImageBitDepthEnum inputPrefDepth;
        int rotoIndex = getRotoBrushInputIndex();
        assert(rotoIndex != -1);
        getPreferredDepthAndComponents(rotoIndex, &inputPrefComps, &inputPrefDepth);
        boost::shared_ptr<Natron::Image> mask = rotoCtx->renderMask(createImageInCache,
                                                                    downscaledRenderWindow,
                                                                    inputPrefComps,
                                                                    nodeHash,
                                                                    rotoAge,
                                                                    rod,
                                                                    time,
                                                                    inputPrefDepth,
                                                                    view,
                                                                    mipMapLevel,
                                                                    ImageList(),
                                                                    byPassCache);
        assert(mask);
        inputImages->push_back(mask);
    }
    
    return true;
}


EffectInstance::RenderRoIStatusEnum
EffectInstance::renderRoIInternal(SequenceTime time,
                                  unsigned int mipMapLevel,
                                  int view,
                                  const std::list<RectI>& rectsToRender,
                                  const RectD & rod, //!< effect rod in canonical coords
                                  const double par,
                                  const boost::shared_ptr<Image> & image,
                                  const boost::shared_ptr<Image> & downscaledImage,
                                  const boost::shared_ptr<Image>& fullScaleMappedImage,
                                  const boost::shared_ptr<Image>& downscaledMappedImage,
                                  const boost::shared_ptr<Image>& renderMappedImage,
                                  bool outputUseImage, //< whether we output to image or downscaledImage
                                  bool isSequentialRender,
                                  bool isRenderMadeInResponseToUserInteraction,
                                  U64 nodeHash,
                                  int channelForAlpha,
                                  bool renderFullScaleThenDownscale,
                                  bool useScaleOneInputImages,
                                  const RoIMap& inputsRoi,
                                  const std::list<boost::shared_ptr<Natron::Image> >& inputImages)
{
    EffectInstance::RenderRoIStatusEnum retCode;

    
    ///Add the window to the project's available formats if the effect is a reader
    ///This is the only reliable place where I could put these lines...which don't seem to feel right here.
    ///Plus setOrAddProjectFormat will actually set the project format the first time we read an image in the project
    ///hence ask for a new render... which can be expensive!
    ///Any solution how to work around this ?
    if ( isReader() ) {
        Format frmt;
        RectI pixelRoD;
        rod.toPixelEnclosing(0, par, &pixelRoD);
        frmt.set(pixelRoD);
        frmt.setPixelAspectRatio(par);
        getApp()->getProject()->setOrAddProjectFormat(frmt);
    }
    
#ifndef NDEBUG
    RenderScale renderMappedScale;
    renderMappedScale.x = Image::getScaleFromMipMapLevel(renderMappedImage->getMipMapLevel());
    renderMappedScale.y = renderMappedScale.x;
#endif


    bool tilesSupported = supportsTiles();

    ///We check what is left to render.

    ///Here we fetch the age of the roto context if there's any to pass it through
    ///the tree and remember it when the plugin calls getImage() afterwards
    Natron::StatusEnum renderStatus = eStatusOK;

    if ( rectsToRender.empty() ) {
        retCode = EffectInstance::eRenderRoIStatusImageAlreadyRendered;
    } else {
        retCode = EffectInstance::eRenderRoIStatusImageRendered;
    }


    ///Notify the gui we're rendering
    boost::shared_ptr<NotifyRenderingStarted_RAII> renderingNotifier;
    if (!rectsToRender.empty()) {
        renderingNotifier.reset(new NotifyRenderingStarted_RAII(_node.get()));
    }

    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        
        const RectI & downscaledRectToRender = *it; // please leave it as const, copy it if necessary

        ///Upscale the RoI to a region in the full scale image so it is in canonical coordinates
        RectD canonicalRectToRender;
        downscaledRectToRender.toCanonical(outputUseImage ? image->getMipMapLevel() : downscaledImage->getMipMapLevel(), par, rod, &canonicalRectToRender);

        ///the getRegionsOfInterest call will not be cached because it would be unnecessary
        ///To put that information (which depends on the RoI) into the cache. That's why we
        ///store it into the render args (thread-storage) so the getImage() function can retrieve the results.
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );

        
        ///There cannot be the same thread running 2 concurrent instances of renderRoI on the same effect.
        assert(!_imp->renderArgs.hasLocalData() || !_imp->renderArgs.localData()._validArgs);

        RectI renderMappedRectToRender;
        
        if (renderFullScaleThenDownscale) {
            canonicalRectToRender.toPixelEnclosing(0, par, &renderMappedRectToRender);
            renderMappedRectToRender.intersect(renderMappedImage->getBounds(), &renderMappedRectToRender);
        } else {
            renderMappedRectToRender = downscaledRectToRender;
        }
        
        Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs);
        scopedArgs.setArgs_firstPass(rod,
                                     renderMappedRectToRender,
                                     time,
                                     view,
                                     channelForAlpha,
                                     false, //< if we reached here the node is not an identity!
                                     0.,
                                     -1,
                                     renderMappedImage);
        
        
        int firstFrame, lastFrame;
        getFrameRange_public(nodeHash, &firstFrame, &lastFrame);
        
        ///The scoped args will maintain the args set for this thread during the
        ///whole time the render action is called, so they can be fetched in the
        ///getImage() call.
        /// @see EffectInstance::getImage
        scopedArgs.setArgs_secondPass(inputsRoi,firstFrame,lastFrame);
        const RenderArgs & args = scopedArgs.getArgs();


#     ifndef NDEBUG
        RenderScale scale;
        scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
        scale.y = scale.x;
        // check the dimensions of all input and output images
        for (std::list< boost::shared_ptr<Natron::Image> >::const_iterator it = inputImages.begin();
             it != inputImages.end();
             ++it) {

            assert(useScaleOneInputImages || (*it)->getMipMapLevel() == mipMapLevel);
            const RectD & srcRodCanonical = (*it)->getRoD();
            RectI srcBounds;
            srcRodCanonical.toPixelEnclosing((*it)->getMipMapLevel(), (*it)->getPixelAspectRatio(), &srcBounds); // compute srcRod at level 0
            const RectD & dstRodCanonical = renderMappedImage->getRoD();
            RectI dstBounds;
            dstRodCanonical.toPixelEnclosing(renderMappedImage->getMipMapLevel(), par, &dstBounds); // compute dstRod at level 0

            if (!tilesSupported) {
                // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
                //  If a clip or plugin does not support tiled images, then the host should supply full RoD images to the effect whenever it fetches one.

                ///Note: The renderRoI() function returns an image according to the mipMapLevel given in parameters.
                ///For effects that DO NOT SUPPORT TILES they are expected an input image to be the full RoD.
                ///Hence the resulting image of the renderRoI call made on the input has to be upscaled to its full RoD.
                ///The reason why this upscale is done externally to renderRoI is because renderRoI is "local" to an effect:
                ///The effect has no way to know that the caller (downstream effect) doesn't support tiles. We would have to
                ///pass this in parameters to the renderRoI function and would make it less clear to the caller.
                ///
                ///Another point is that we don't cache the resulting upscaled image (@see getImage()).
                ///The reason why we don't do this is because all images in the NodeCache have a key identifying them.
                ///Part of the key is the mipmapLevel of the image, hence
                ///2 images with different mipmapLevels have different keys. Now if we were to put those "upscaled" images in the cache
                ///they would take the same priority as the images that were REALLY rendered at scale 1. But those upcaled images have poor
                ///quality compared to the images rendered at scale 1, hence we don't cache them.
                ///If we were to cache them, we would need to change the way the cache works and return a list of potential images instead.
                ///This way we could add a "quality" identifier to images and pick the best one from the list returned by the cache.
                RectI srcRealBounds = (*it)->getBounds();
                RectI dstRealBounds = renderMappedImage->getBounds();
                
                assert(srcRealBounds.x1 == srcBounds.x1);
                assert(srcRealBounds.x2 == srcBounds.x2);
                assert(srcRealBounds.y1 == srcBounds.y1);
                assert(srcRealBounds.y2 == srcBounds.y2);
                assert(dstRealBounds.x1 == dstBounds.x1);
                assert(dstRealBounds.x2 == dstBounds.x2);
                assert(dstRealBounds.y1 == dstBounds.y1);
                assert(dstRealBounds.y2 == dstBounds.y2);
            }
            if ( !supportsMultiResolution() ) {
                // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
                //   Multiple resolution images mean...
                //    input and output images can be of any size
                //    input and output images can be offset from the origin
                assert(srcBounds.x1 == 0);
                assert(srcBounds.y1 == 0);
                assert(srcBounds.x1 == dstBounds.x1);
                assert(srcBounds.x2 == dstBounds.x2);
                assert(srcBounds.y1 == dstBounds.y1);
                assert(srcBounds.y2 == dstBounds.y2);
            }
        } //end for
        
        if (supportsRenderScaleMaybe() == eSupportsNo) {
            assert(renderMappedImage->getMipMapLevel() == 0);
            assert(renderMappedScale.x == 1. && renderMappedScale.y == 1.);
        }
#     endif // DEBUG

       

        ///We only need to call begin if we've not already called it.
        bool callBegin = false;

        ///neer call beginsequenceRender here if the render is sequential
        
        Natron::SequentialPreferenceEnum pref = getSequentialPreference();
        if (!isWriter() || pref == eSequentialPreferenceNotSequential) {
            callBegin = true;
        }

        if (callBegin) {
            assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
            if (beginSequenceRender_public(time, time, 1, !appPTR->isBackground(), renderMappedScale, isSequentialRender,
                                           isRenderMadeInResponseToUserInteraction, view) == eStatusFailed) {
                renderStatus = eStatusFailed;
                break;
            }
        }

        /*depending on the thread-safety of the plug-in we render with a different
           amount of threads*/
        EffectInstance::RenderSafetyEnum safety = renderThreadSafety();

        ///if the project lock is already locked at this point, don't start any other thread
        ///as it would lead to a deadlock when the project is loading.
        ///Just fall back to Fully_safe
        int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
        if (safety == eRenderSafetyFullySafeFrame) {
            ///If the plug-in is eRenderSafetyFullySafeFrame that means it wants the host to perform SMP aka slice up the RoI into chunks
            ///but if the effect doesn't support tiles it won't work.
            ///Also check that the number of threads indicating by the settings are appropriate for this render mode.
            if ( !tilesSupported || (nbThreads == -1) || (nbThreads == 1) ||
                ( (nbThreads == 0) && (appPTR->getHardwareIdealThreadCount() == 1) ) ||
                 ( QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount() ) ) {
                safety = eRenderSafetyFullySafe;
            } else {
                if ( !getApp()->getProject()->tryLock() ) {
                    safety = eRenderSafetyFullySafe;
                } else {
                    getApp()->getProject()->unlock();
                }
            }
        }
        
        assert(_imp->frameRenderArgs.hasLocalData());
        const ParallelRenderArgs& frameArgs = _imp->frameRenderArgs.localData();

        switch (safety) {
        case eRenderSafetyFullySafeFrame: {     // the plugin will not perform any per frame SMP threading
            // we can split the frame in tiles and do per frame SMP threading (see kOfxImageEffectPluginPropHostFrameThreading)
            if (nbThreads == 0) {
                nbThreads = QThreadPool::globalInstance()->maxThreadCount();
            }
            std::vector<RectI> splitRects = RectI::splitRectIntoSmallerRect(downscaledRectToRender, nbThreads);
            
            TiledRenderingFunctorArgs tiledArgs;
            tiledArgs.args = &args;
            tiledArgs.isSequentialRender = isSequentialRender;
            tiledArgs.inputImages = inputImages;
            tiledArgs.renderUseScaleOneInputs = useScaleOneInputImages;
            tiledArgs.isRenderResponseToUserInteraction = isRenderMadeInResponseToUserInteraction;
            tiledArgs.downscaledImage = downscaledImage;
            tiledArgs.downscaledMappedImage = downscaledMappedImage;
            tiledArgs.fullScaleImage = image;
            tiledArgs.fullScaleMappedImage = fullScaleMappedImage;
            tiledArgs.renderMappedImage = renderMappedImage;
            tiledArgs.par = par;
            tiledArgs.renderFullScaleThenDownscale = renderFullScaleThenDownscale;
            
            // the bitmap is checked again at the beginning of EffectInstance::tiledRenderingFunctor()
            QFuture<Natron::StatusEnum> ret = QtConcurrent::mapped( splitRects,
                                                                boost::bind(&EffectInstance::tiledRenderingFunctor,
                                                                            this,
                                                                            tiledArgs,
                                                                            frameArgs,
                                                                            true,
                                                                            _1) );
            ret.waitForFinished();

            ///never call endsequence render here if the render is sequential

            if (callBegin) {
                assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
                if (endSequenceRender_public(time, time, time, false, renderMappedScale,
                                             isSequentialRender,
                                             isRenderMadeInResponseToUserInteraction,
                                             view) == eStatusFailed) {
                    renderStatus = eStatusFailed;
                    break;
                }
            }
            for (QFuture<Natron::StatusEnum>::const_iterator it2 = ret.begin(); it2 != ret.end(); ++it2) {
                if ( (*it2) == Natron::eStatusFailed ) {
                    renderStatus = *it2;
                    break;
                }
            }
            break;
        }

        case eRenderSafetyInstanceSafe:     // indicating that any instance can have a single 'render' call at any one time,
        case eRenderSafetyFullySafe:        // indicating that any instance of a plugin can have multiple renders running simultaneously
        case eRenderSafetyUnsafe: {     // indicating that only a single 'render' call can be made at any time amoung all instances
            // eRenderSafetyInstanceSafe means that there is at most one render per instance
            // NOTE: the per-instance lock should probably be shared between
            // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
            // and read-write on it.
            // It is probably safer to assume that several clones may write to the same output image only in the eRenderSafetyFullySafe case.

            // eRenderSafetyFullySafe means that there is only one render per FRAME : the lock is by image and handled in Node.cpp
            ///locks belongs to an instance)

            QMutexLocker *locker = 0;

            if (safety == eRenderSafetyInstanceSafe) {
                 locker = new QMutexLocker( &getNode()->getRenderInstancesSharedMutex() );
            } else if (safety == eRenderSafetyUnsafe) {
                locker = new QMutexLocker( appPTR->getMutexForPlugin( getPluginID().c_str() ) );
            }
            ///For eRenderSafetyFullySafe, don't take any lock, the image already has a lock on itself so we're sure it can't be written to by 2 different threads.
            
            
            renderStatus = tiledRenderingFunctor(args,
                                                 frameArgs,
                                                 inputImages,
                                                 false,
                                                 renderFullScaleThenDownscale,
                                                 useScaleOneInputImages,
                                                 isSequentialRender,
                                                 isRenderMadeInResponseToUserInteraction,
                                                 downscaledRectToRender,
                                                 par,
                                                 downscaledImage,
                                                 image,
                                                 downscaledMappedImage,
                                                 fullScaleMappedImage,
                                                 renderMappedImage);

            delete locker;
            break;
        }
        } // switch
 
        

        if (renderStatus != eStatusOK) {
            break;
        }
    } // for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
    
    if (renderStatus != eStatusOK) {
        retCode = eRenderRoIStatusRenderFailed;
    }

    return retCode;
} // renderRoIInternal

Natron::StatusEnum
EffectInstance::tiledRenderingFunctor(const TiledRenderingFunctorArgs& args,
                                      const ParallelRenderArgs& frameArgs,
                                     bool setThreadLocalStorage,
                                     const RectI & roi )
{
    return tiledRenderingFunctor(*args.args,
                                 frameArgs,
                                 args.inputImages,
                                 setThreadLocalStorage,
                                 args.renderFullScaleThenDownscale,
                                 args.renderUseScaleOneInputs,
                                 args.isSequentialRender,
                                 args.isRenderResponseToUserInteraction,
                                 roi,
                                 args.par,
                                 args.downscaledImage,
                                 args.fullScaleImage,
                                 args.downscaledMappedImage,
                                 args.fullScaleMappedImage,
                                 args.renderMappedImage);
}

Natron::StatusEnum
EffectInstance::tiledRenderingFunctor(const RenderArgs & args,
                                      const ParallelRenderArgs& frameArgs,
                                      const std::list<boost::shared_ptr<Natron::Image> >& inputImages,
                                      bool setThreadLocalStorage,
                                      bool renderFullScaleThenDownscale,
                                      bool renderUseScaleOneInputs,
                                      bool isSequentialRender,
                                      bool isRenderResponseToUserInteraction,
                                      const RectI & downscaledRectToRender,
                                      const double par,
                                      const boost::shared_ptr<Natron::Image> & downscaledImage,
                                      const boost::shared_ptr<Natron::Image> & fullScaleImage,
                                      const boost::shared_ptr<Natron::Image> & downscaledMappedImage,
                                      const boost::shared_ptr<Natron::Image> & fullScaleMappedImage,
                                      const boost::shared_ptr<Natron::Image> & renderMappedImage)
{
    assert(downscaledMappedImage && fullScaleMappedImage && renderMappedImage);
    if (renderFullScaleThenDownscale) {
        assert( renderMappedImage->getBounds() == fullScaleImage->getBounds() );
    } else {
        assert( renderMappedImage->getBounds() == downscaledImage->getBounds() );
    }
    assert( downscaledMappedImage->getBounds() == downscaledImage->getBounds() );
    assert( fullScaleMappedImage->getBounds() == fullScaleImage->getBounds() );
    
    
    
    
    const SequenceTime time = args._time;
    int mipMapLevel = downscaledImage->getMipMapLevel();
    const int view = args._view;
    const int channelForAlpha = args._channelForAlpha;

    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    const RectI & renderBounds = renderMappedImage->getBounds();
# endif
    assert(renderBounds.x1 <= downscaledRectToRender.x1 && downscaledRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= downscaledRectToRender.y1 && downscaledRectToRender.y2 <= renderBounds.y2);

   
    
    
    RectI renderRectToRender; // rectangle to render, in renderMappedImage's pixel coordinates
    
    RenderScale renderMappedScale;
    renderMappedScale.x = renderMappedScale.y = Image::getScaleFromMipMapLevel( renderMappedImage->getMipMapLevel() );
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
    
    
    ///Make the thread-storage live as long as the render action is called if we're in a newly launched thread in eRenderSafetyFullySafeFrame mode
    boost::shared_ptr<Implementation::ScopedRenderArgs> scopedArgs;
    boost::shared_ptr<Node::ParallelRenderArgsSetter> scopedFrameArgs;
    
    boost::shared_ptr<InputImagesHolder_RAII> scopedInputImages;
    
    if (!setThreadLocalStorage) {
        renderRectToRender = args._renderWindowPixel;
    } else {

        ///At this point if we're in eRenderSafetyFullySafeFrame mode, we are a thread that might have been launched way after
        ///the time renderRectToRender was computed. We recompute it to update the portion to render
        
        // check the bitmap!
        if (renderFullScaleThenDownscale && renderUseScaleOneInputs) {
            //The renderMappedImage is cached , read bitmap from it
            RectD canonicalrenderRectToRender;
            downscaledRectToRender.toCanonical(mipMapLevel, par, args._rod, &canonicalrenderRectToRender);
            canonicalrenderRectToRender.toPixelEnclosing(0, par, &renderRectToRender);
            renderRectToRender.intersect(renderMappedImage->getBounds(), &renderRectToRender);
            
            renderRectToRender = renderMappedImage->getMinimalRect(renderRectToRender);
            
            assert(renderBounds.x1 <= renderRectToRender.x1 && renderRectToRender.x2 <= renderBounds.x2 &&
                   renderBounds.y1 <= renderRectToRender.y1 && renderRectToRender.y2 <= renderBounds.y2);
        } else {
            //THe downscaled image is cached, read bitmap from it
            const RectI downscaledRectToRenderMinimal = downscaledMappedImage->getMinimalRect(downscaledRectToRender);
            
            assert(renderBounds.x1 <= downscaledRectToRenderMinimal.x1 && downscaledRectToRenderMinimal.x2 <= renderBounds.x2 &&
                   renderBounds.y1 <= downscaledRectToRenderMinimal.y1 && downscaledRectToRenderMinimal.y2 <= renderBounds.y2);
            
            if (renderFullScaleThenDownscale) {
                RectD canonicalrenderRectToRender;
                downscaledRectToRenderMinimal.toCanonical(mipMapLevel, par, args._rod, &canonicalrenderRectToRender);
                canonicalrenderRectToRender.toPixelEnclosing(0, par, &renderRectToRender);
                renderRectToRender.intersect(renderMappedImage->getBounds(), &renderRectToRender);
            } else {
                renderRectToRender = downscaledRectToRenderMinimal;
            }
        }
        
        RenderArgs argsCpy(args);
        ///Update the renderWindow which might have changed
        argsCpy._renderWindowPixel = renderRectToRender;
        
        scopedArgs.reset( new Implementation::ScopedRenderArgs(&_imp->renderArgs,argsCpy) );
        scopedFrameArgs.reset( new Node::ParallelRenderArgsSetter(_node.get(),
                                                                  frameArgs.time,
                                                                  frameArgs.view,
                                                                  frameArgs.isRenderResponseToUserInteraction,
                                                                  frameArgs.isSequentialRender,
                                                                  frameArgs.canAbort,
                                                                  frameArgs.nodeHash,
                                                                  frameArgs.timeline) );
        
        scopedInputImages.reset(new InputImagesHolder_RAII(inputImages,&_imp->inputImages));
    }
    if ( renderRectToRender.isNull() ) {
        ///We've got nothing to do
        return eStatusOK;
    }
    
    RenderScale originalScale;
    originalScale.x = downscaledImage->getScale();
    originalScale.y = originalScale.x;

    Natron::StatusEnum st = render_public(time,
                                          originalScale,
                                          renderMappedScale,
                                          renderRectToRender, view,
                                          isSequentialRender,
                                          isRenderResponseToUserInteraction,
                                          renderMappedImage);
    if (st != eStatusOK) {
        return st;
    }
    
    if ( !aborted() ) {
        
        
        //Check for NaNs
        renderMappedImage->checkForNaNs(renderRectToRender);
        
        ///copy the rectangle rendered in the full scale image to the downscaled output
        if (renderFullScaleThenDownscale) {
            ///First demap the fullScaleMappedImage to the original comps/bitdepth if it needs to
            if ( (renderMappedImage == fullScaleMappedImage) && (fullScaleMappedImage != fullScaleImage) ) {
                bool unPremultIfNeeded = getOutputPremultiplication() == eImagePremultiplicationPremultiplied;
                renderMappedImage->convertToFormat( renderRectToRender,
                                                   getApp()->getDefaultColorSpaceForBitDepth( renderMappedImage->getBitDepth() ),
                                                   getApp()->getDefaultColorSpaceForBitDepth( fullScaleMappedImage->getBitDepth() ),
                                                   channelForAlpha, false, false,unPremultIfNeeded,
                                                   fullScaleImage.get() );
            }
            
            ///If we're using renderUseScaleOneInputs, the full scale image is cached, hence we're not sure that the whole part of the image will be downscaled.
            ///Instead we do all the downscale at once at the end of renderRoI().
            ///If !renderUseScaleOneInputs the image is not cached and we know it will be rendered completly so it is safe to do this here and take advantage
            ///of the multi-threading.
            if (mipMapLevel != 0 && !renderUseScaleOneInputs) {
                assert(fullScaleImage != downscaledImage);
                fullScaleImage->downscaleMipMap( renderRectToRender, 0, mipMapLevel, false, downscaledImage.get() );
                downscaledImage->markForRendered(downscaledRectToRender);
            } else {
                fullScaleImage->markForRendered(renderRectToRender);
            }
        } else {
            assert(renderMappedImage == downscaledMappedImage);
            if (renderMappedImage != downscaledImage) {
                bool unPremultIfNeeded = getOutputPremultiplication() == eImagePremultiplicationPremultiplied;
                renderMappedImage->convertToFormat( renderRectToRender,
                                                   getApp()->getDefaultColorSpaceForBitDepth( renderMappedImage->getBitDepth() ),
                                                   getApp()->getDefaultColorSpaceForBitDepth( downscaledMappedImage->getBitDepth() ),
                                                   channelForAlpha, false,
                                                   false,unPremultIfNeeded,
                                                   downscaledImage.get() );
                
            }
            downscaledImage->markForRendered(downscaledRectToRender);
        }
        
    }
  

    return eStatusOK;
} // tiledRenderingFunctor

void
EffectInstance::openImageFileKnob()
{
    const std::vector< boost::shared_ptr<KnobI> > & knobs = getKnobs();

    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->typeName() == File_Knob::typeNameStatic() ) {
            boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
            assert(fk);
            if ( fk->isInputImageFile() ) {
                std::string file = fk->getValue();
                if ( file.empty() ) {
                    fk->open_file();
                }
                break;
            }
        } else if ( knobs[i]->typeName() == OutputFile_Knob::typeNameStatic() ) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if ( fk->isOutputImageFile() ) {
                std::string file = fk->getValue();
                if ( file.empty() ) {
                    fk->open_file();
                }
                break;
            }
        }
    }
}

void
EffectInstance::createKnobDynamically()
{
    _node->createKnobDynamically();
}

void
EffectInstance::evaluate(KnobI* knob,
                         bool isSignificant,
                         Natron::ValueChangedReasonEnum /*reason*/)
{
    assert(_node);

    ////If the node is currently modifying its input, to ask for a render
    ////because at then end of the inputChanged handler, it will ask for a refresh
    ////and a rebuild of the inputs tree.
    if ( _node->duringInputChangedAction() ) {
        return;
    }

    if ( getApp()->getProject()->isLoadingProject() ) {
        return;
    }


    Button_Knob* button = dynamic_cast<Button_Knob*>(knob);

    /*if this is a writer (openfx or built-in writer)*/
    if ( isWriter() ) {
        /*if this is a button and it is a render button,we're safe to assume the plug-ins wants to start rendering.*/
        if (button) {
            if ( button->isRenderButton() ) {
                std::string sequentialNode;
                if ( _node->hasSequentialOnlyNodeUpstream(sequentialNode) ) {
                    if (_node->getApp()->getProject()->getProjectViewsCount() > 1) {
                        Natron::StandardButtonEnum answer =
                        Natron::questionDialog( QObject::tr("Render").toStdString(),
                                               sequentialNode + QObject::tr(" can only "
                                                                            "render in sequential mode. Due to limitations in the "
                                                                            "OpenFX standard that means that %1"
                                                                            " will not be able "
                                                                            "to render all the views of the project. "
                                                                            "Only the main view of the project will be rendered, you can "
                                                                            "change the main view in the project settings. Would you like "
                                                                            "to continue ?").arg(NATRON_APPLICATION_NAME).toStdString(),false );
                        if (answer != Natron::eStandardButtonYes) {
                            return;
                        }
                    }
                }
                AppInstance::RenderWork w;
                w.writer = dynamic_cast<OutputEffectInstance*>(this);
                w.firstFrame = INT_MIN;
                w.lastFrame = INT_MAX;
                std::list<AppInstance::RenderWork> works;
                works.push_back(w);
                getApp()->startWritersRendering(works);

                return;
            }
        }
    }

    ///increments the knobs age following a change
    if (!button && isSignificant) {
        _node->incrementKnobsAge();
    }

    std::list<ViewerInstance* > viewers;
    _node->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        if (isSignificant) {
            (*it)->renderCurrentFrame(true);
        } else {
            (*it)->redrawViewer();
        }
    }

    getNode()->refreshPreviewsRecursivelyDownstream(getApp()->getTimeLine()->currentFrame());
} // evaluate

bool
EffectInstance::message(Natron::MessageTypeEnum type,
                        const std::string & content) const
{
    return _node->message(type,content);
}

void
EffectInstance::setPersistentMessage(Natron::MessageTypeEnum type,
                                     const std::string & content)
{
    _node->setPersistentMessage(type, content);
}

void
EffectInstance::clearPersistentMessage(bool recurse)
{
    _node->clearPersistentMessage(recurse);
}

int
EffectInstance::getInputNumber(Natron::EffectInstance* inputEffect) const
{
    for (int i = 0; i < getMaxInputCount(); ++i) {
        if (getInput(i) == inputEffect) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Does this effect supports rendering at a different scale than 1 ?
 * There is no OFX property for this purpose. The only solution found for OFX is that if a isIdentity
 * with renderscale != 1 fails, the host retries with renderscale = 1 (and upscaled images).
 * If the renderScale support was not set, this throws an exception.
 **/
bool
EffectInstance::supportsRenderScale() const
{
    if (_imp->supportsRenderScale == eSupportsMaybe) {
        qDebug() << "EffectInstance::supportsRenderScale should be set before calling supportsRenderScale(), or use supportsRenderScaleMaybe() instead";
        throw std::runtime_error("supportsRenderScale not set");
    }

    return _imp->supportsRenderScale == eSupportsYes;
}

EffectInstance::SupportsEnum
EffectInstance::supportsRenderScaleMaybe() const
{
    QMutexLocker l(&_imp->supportsRenderScaleMutex);

    return _imp->supportsRenderScale;
}

/// should be set during effect initialization, but may also be set by the first getRegionOfDefinition that succeeds
void
EffectInstance::setSupportsRenderScaleMaybe(EffectInstance::SupportsEnum s) const
{
    {
        QMutexLocker l(&_imp->supportsRenderScaleMutex);
        
        _imp->supportsRenderScale = s;
    }
    if (_node) {
        _node->onSetSupportRenderScaleMaybeSet((int)s);
    }
}

void
EffectInstance::setOutputFilesForWriter(const std::string & pattern)
{
    if ( !isWriter() ) {
        return;
    }

    const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->typeName() == OutputFile_Knob::typeNameStatic() ) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if ( fk->isOutputImageFile() ) {
                fk->setValue(pattern,0);
                break;
            }
        }
    }
}

PluginMemory*
EffectInstance::newMemoryInstance(size_t nBytes)
{
    PluginMemory* ret = new PluginMemory( _node->getLiveInstance() ); //< hack to get "this" as a shared ptr
    bool wasntLocked = ret->alloc(nBytes);

    assert(wasntLocked);
    (void)wasntLocked;

    return ret;
}

void
EffectInstance::addPluginMemoryPointer(PluginMemory* mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);

    _imp->pluginMemoryChunks.push_back(mem);
}

void
EffectInstance::removePluginMemoryPointer(PluginMemory* mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);
    std::list<PluginMemory*>::iterator it = std::find(_imp->pluginMemoryChunks.begin(),_imp->pluginMemoryChunks.end(),mem);

    if ( it != _imp->pluginMemoryChunks.end() ) {
        _imp->pluginMemoryChunks.erase(it);
    }
}

void
EffectInstance::registerPluginMemory(size_t nBytes)
{
    _node->registerPluginMemory(nBytes);
}

void
EffectInstance::unregisterPluginMemory(size_t nBytes)
{
    _node->unregisterPluginMemory(nBytes);
}

void
EffectInstance::onAllKnobsSlaved(bool isSlave,
                                 KnobHolder* master)
{
    _node->onAllKnobsSlaved(isSlave,master);
}

void
EffectInstance::onKnobSlaved(const boost::shared_ptr<KnobI> & knob,
                             int dimension,
                             bool isSlave,
                             KnobHolder* master)
{
    _node->onKnobSlaved(knob,dimension,isSlave,master);
}

void
EffectInstance::drawOverlay_public(double scaleX,
                                   double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return;
    }

    RECURSIVE_ACTION();

    _imp->setDuringInteractAction(true);
    drawOverlay(scaleX,scaleY);
    _imp->setDuringInteractAction(false);
}

bool
EffectInstance::onOverlayPenDown_public(double scaleX,
                                        double scaleY,
                                        const QPointF & viewportPos,
                                        const QPointF & pos)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return false;
    }

    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenDown(scaleX,scaleY,viewportPos, pos);
    _imp->setDuringInteractAction(false);
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayPenMotion_public(double scaleX,
                                          double scaleY,
                                          const QPointF & viewportPos,
                                          const QPointF & pos)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return false;
    }
    

    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenMotion(scaleX,scaleY,viewportPos, pos);
    _imp->setDuringInteractAction(false);
    //Don't chek if render is needed on pen motion, wait for the pen up

    //checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayPenUp_public(double scaleX,
                                      double scaleY,
                                      const QPointF & viewportPos,
                                      const QPointF & pos)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return false;
    }
    
    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenUp(scaleX,scaleY,viewportPos, pos);
    _imp->setDuringInteractAction(false);
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyDown_public(double scaleX,
                                        double scaleY,
                                        Natron::Key key,
                                        Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return false;
    }

    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyDown(scaleX,scaleY,key, modifiers);
    _imp->setDuringInteractAction(false);
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyUp_public(double scaleX,
                                      double scaleY,
                                      Natron::Key key,
                                      Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return false;
    }

    NON_RECURSIVE_ACTION();

    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyUp(scaleX, scaleY, key, modifiers);
    _imp->setDuringInteractAction(false);
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyRepeat_public(double scaleX,
                                          double scaleY,
                                          Natron::Key key,
                                          Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return false;
    }

    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyRepeat(scaleX,scaleY,key, modifiers);
    _imp->setDuringInteractAction(false);
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayFocusGained_public(double scaleX,
                                            double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return false;
    }

    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayFocusGained(scaleX,scaleY);
    _imp->setDuringInteractAction(false);
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayFocusLost_public(double scaleX,
                                          double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() ) {
        return false;
    }


    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayFocusLost(scaleX,scaleY);
    _imp->setDuringInteractAction(false);
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::isDoingInteractAction() const
{
    QReadLocker l(&_imp->duringInteractActionMutex);

    return _imp->duringInteractAction;
}

Natron::StatusEnum
EffectInstance::render_public(SequenceTime time,
                              const RenderScale& originalScale,
                              const RenderScale & mappedScale,
                              const RectI & roi,
                              int view,
                              bool isSequentialRender,
                              bool isRenderResponseToUserInteraction,
                              boost::shared_ptr<Natron::Image> output)
{
    NON_RECURSIVE_ACTION();
    return render(time, originalScale, mappedScale, roi, view, isSequentialRender, isRenderResponseToUserInteraction, output);

}

Natron::StatusEnum
EffectInstance::getTransform_public(SequenceTime time,
                                    const RenderScale& renderScale,
                                    int view,
                                    Natron::EffectInstance** inputToTransform,
                                    Transform::Matrix3x3* transform)
{
    NON_RECURSIVE_ACTION();
    return getTransform(time, renderScale, view, inputToTransform, transform);
}

bool
EffectInstance::isIdentity_public(U64 hash,
                                  SequenceTime time,
                                  const RenderScale & scale,
                                  const RectD& rod,
                                  const double par,
                                  int view,
                                  SequenceTime* inputTime,
                                  int* inputNb)
{
    
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    
    double timeF;
    bool foundInCache = _imp->actionsCache.getIdentityResult(hash, time, mipMapLevel, inputNb, &timeF);
    if (foundInCache) {
        *inputTime = timeF;
        return *inputNb >= 0 || *inputNb == -2;
    } else {
        
        ///If this is running on a render thread, attempt to find the info in the thread local storage.
        if (QThread::currentThread() != qApp->thread() && _imp->renderArgs.hasLocalData()) {
            const RenderArgs& args = _imp->renderArgs.localData();
            if (args._validArgs) {
                *inputNb = args._identityInputNb;
                *inputTime = args._identityTime;
                return *inputNb != -1 ;
            }
        }
        
        ///EDIT: We now allow isIdentity to be called recursively.
        RECURSIVE_ACTION();
        
        bool ret = false;
        
        if (appPTR->isBackground() && dynamic_cast<DiskCacheNode*>(this) != NULL) {
            ret = true;
            *inputNb = 0;
            *inputTime = time;
        } else if ( _node->isNodeDisabled() ) {
            ret = true;
            *inputTime = time;
            *inputNb = -1;
            ///we forward this node to the last connected non-optional input
            ///if there's only optional inputs connected, we return the last optional input
            int lastOptionalInput = -1;
            for (int i = getMaxInputCount() - 1; i >= 0; --i) {
                bool optional = isInputOptional(i);
                if ( !optional && _node->getInput(i) ) {
                    *inputNb = i;
                    break;
                } else if ( optional && (lastOptionalInput == -1) ) {
                    lastOptionalInput = i;
                }
            }
            if (*inputNb == -1) {
                *inputNb = lastOptionalInput;
            }
        } else {
            /// Don't call isIdentity if plugin is sequential only.
            if (getSequentialPreference() != Natron::eSequentialPreferenceOnlySequential) {
                try {
                    ret = isIdentity(time, scale,rod, par, view, inputTime, inputNb);
                } catch (...) {
                    throw;
                }
            }
        }
        if (!ret) {
            *inputNb = -1;
            *inputTime = time;
        }
        _imp->actionsCache.setIdentityResult(time, mipMapLevel, *inputNb, *inputTime);
        return ret;
    }
}

Natron::StatusEnum
EffectInstance::getRegionOfDefinition_public(U64 hash,
                                             SequenceTime time,
                                             const RenderScale & scale,
                                             int view,
                                             RectD* rod,
                                             bool* isProjectFormat)
{
    if (!isEffectCreated()) {
        return eStatusFailed;
    }
    
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    bool foundInCache = _imp->actionsCache.getRoDResult(hash, time, mipMapLevel, rod);
    if (foundInCache) {
        *isProjectFormat = false;
        if (rod->isNull()) {
            return Natron::eStatusFailed;
        }
        return Natron::eStatusOK;
    } else {
        
        
        ///If this is running on a render thread, attempt to find the RoD in the thread local storage.
        if (QThread::currentThread() != qApp->thread() && _imp->renderArgs.hasLocalData()) {
            const RenderArgs& args = _imp->renderArgs.localData();
            if (args._validArgs) {
                *rod = args._rod;
                *isProjectFormat = false;
                return Natron::eStatusOK;
            }
        }
        
        Natron::StatusEnum ret;
        RenderScale scaleOne;
        scaleOne.x = scaleOne.y = 1.;
        {
            RECURSIVE_ACTION();
            ret = getRegionOfDefinition(hash,time, supportsRenderScaleMaybe() == eSupportsNo ? scaleOne : scale, view, rod);
            
            if ( (ret != eStatusOK) && (ret != eStatusReplyDefault) ) {
                // rod is not valid
                _imp->actionsCache.invalidateAll(hash);
                _imp->actionsCache.setRoDResult(time, mipMapLevel, RectD());
                return ret;
            }
            
            if (rod->isNull()) {
                _imp->actionsCache.invalidateAll(hash);
                _imp->actionsCache.setRoDResult(time, mipMapLevel, RectD());
                return eStatusFailed;
            }
            
            assert( (ret == eStatusOK || ret == eStatusReplyDefault) && (rod->x1 <= rod->x2 && rod->y1 <= rod->y2) );
            
        }
        *isProjectFormat = ifInfiniteApplyHeuristic(hash,time, scale, view, rod);
        assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

        _imp->actionsCache.setRoDResult( time, mipMapLevel, *rod);
        return ret;
    }
}

void
EffectInstance::getRegionsOfInterest_public(SequenceTime time,
                                            const RenderScale & scale,
                                            const RectD & outputRoD, //!< effect RoD in canonical coordinates
                                            const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            int view,
                                            EffectInstance::RoIMap* ret)
{
    NON_RECURSIVE_ACTION();
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);
    getRegionsOfInterest(time, scale, outputRoD, renderWindow, view,ret);
    
}

EffectInstance::FramesNeededMap
EffectInstance::getFramesNeeded_public(SequenceTime time)
{
    NON_RECURSIVE_ACTION();

    return getFramesNeeded(time);
}

void
EffectInstance::getFrameRange_public(U64 hash,
                                     SequenceTime *first,
                                     SequenceTime *last)
{
    double fFirst,fLast;
    bool foundInCache = _imp->actionsCache.getTimeDomainResult(hash, &fFirst, &fLast);
    if (foundInCache) {
        *first = std::floor(fFirst+0.5);
        *last = std::floor(fLast+0.5);
    } else {
        
        ///If this is running on a render thread, attempt to find the info in the thread local storage.
        if (QThread::currentThread() != qApp->thread() && _imp->renderArgs.hasLocalData()) {
            const RenderArgs& args = _imp->renderArgs.localData();
            if (args._validArgs) {
                *first = args._firstFrame;
                *last = args._lastFrame;
                return;
            }
        }
        
        NON_RECURSIVE_ACTION();
        getFrameRange(first, last);
        _imp->actionsCache.setTimeDomainResult(*first, *last);
    }
}

Natron::StatusEnum
EffectInstance::beginSequenceRender_public(SequenceTime first,
                                           SequenceTime last,
                                           SequenceTime step,
                                           bool interactive,
                                           const RenderScale & scale,
                                           bool isSequentialRender,
                                           bool isRenderResponseToUserInteraction,
                                           int view)
{
    NON_RECURSIVE_ACTION();
    {
        if ( !_imp->beginEndRenderCount.hasLocalData() ) {
            _imp->beginEndRenderCount.localData() = 1;
        } else {
            ++_imp->beginEndRenderCount.localData();
        }
    }

    return beginSequenceRender(first, last, step, interactive, scale,
                               isSequentialRender, isRenderResponseToUserInteraction, view);
}

Natron::StatusEnum
EffectInstance::endSequenceRender_public(SequenceTime first,
                                         SequenceTime last,
                                         SequenceTime step,
                                         bool interactive,
                                         const RenderScale & scale,
                                         bool isSequentialRender,
                                         bool isRenderResponseToUserInteraction,
                                         int view)
{
    NON_RECURSIVE_ACTION();
    {
        assert( _imp->beginEndRenderCount.hasLocalData() );
        --_imp->beginEndRenderCount.localData();
        assert(_imp->beginEndRenderCount.localData() >= 0);
    }

    return endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, view);
}

bool
EffectInstance::isSupportedComponent(int inputNb,
                                     Natron::ImageComponentsEnum comp) const
{
    return _node->isSupportedComponent(inputNb, comp);
}

Natron::ImageBitDepthEnum
EffectInstance::getBitDepth() const
{
    return _node->getBitDepth();
}

bool
EffectInstance::isSupportedBitDepth(Natron::ImageBitDepthEnum depth) const
{
    return _node->isSupportedBitDepth(depth);
}

Natron::ImageComponentsEnum
EffectInstance::findClosestSupportedComponents(int inputNb,
                                               Natron::ImageComponentsEnum comp) const
{
    return _node->findClosestSupportedComponents(inputNb,comp);
}

void
EffectInstance::getPreferredDepthAndComponents(int inputNb,
                                               Natron::ImageComponentsEnum* comp,
                                               Natron::ImageBitDepthEnum* depth) const
{
    ///find closest to RGBA
    *comp = findClosestSupportedComponents(inputNb, Natron::eImageComponentRGBA);

    ///find deepest bitdepth
    *depth = getBitDepth();
}

int
EffectInstance::getMaskChannel(int inputNb) const
{
    return _node->getMaskChannel(inputNb);
}

bool
EffectInstance::isMaskEnabled(int inputNb) const
{
    return _node->isMaskEnabled(inputNb);
}

void
EffectInstance::onKnobValueChanged(KnobI* /*k*/,
                                   Natron::ValueChangedReasonEnum /*reason*/,
                                   SequenceTime /*time*/)
{
}

int
EffectInstance::getThreadLocalRenderTime() const
{
    if (_imp->renderArgs.hasLocalData()) {
        const RenderArgs& args = _imp->renderArgs.localData();
        if (args._validArgs) {
            return args._time;
        }
    }

    return getApp()->getTimeLine()->currentFrame();
}

bool
EffectInstance::getThreadLocalRenderedImage(boost::shared_ptr<Natron::Image>* image,RectI* renderWindow) const
{
    if (_imp->renderArgs.hasLocalData()) {
        const RenderArgs& args = _imp->renderArgs.localData();
        if (args._validArgs) {
            *image = args._outputImage;
            *renderWindow = args._renderWindowPixel;
            return true;
        }
    }
    return false;
}

void
EffectInstance::updateThreadLocalRenderTime(int time)
{
    if (QThread::currentThread() != qApp->thread() && _imp->renderArgs.hasLocalData()) {
         RenderArgs& args = _imp->renderArgs.localData();
        if (args._validArgs) {
            args._time = time;
        }
    }
}


void
EffectInstance::onKnobValueChanged_public(KnobI* k,
                                          Natron::ValueChangedReasonEnum reason,
                                          SequenceTime time)
{

    if ( isEvaluationBlocked() ) {
        return;
    }

    _node->onEffectKnobValueChanged(k, reason);
    
    KnobHelper* kh = dynamic_cast<KnobHelper*>(k);
    assert(kh);
    if ( kh && kh->isDeclaredByPlugin() ) {
        ////We set the thread storage render args so that if the instance changed action
        ////tries to call getImage it can render with good parameters.
        
        Node::ParallelRenderArgsSetter frameRenderArgs(_node.get(),
                                                       time,
                                                       0, /*view*/
                                                       true,
                                                       false,
                                                       false,
                                                       getHash(),
                                                       getApp()->getTimeLine().get());

        RECURSIVE_ACTION();
        knobChanged(k, reason, /*view*/ 0, time);
    }
    
    ///Clear input images pointers that were stored in getImage() for the main-thread.
    ///This is safe to do so because if this is called while in render() it won't clear the input images
    ///pointers for the render thread. This is helpful for analysis effects which call getImage() on the main-thread
    ///and whose render() function is never called.
    _imp->clearInputImagePointers();
    
} // onKnobValueChanged_public

void
EffectInstance::clearLastRenderedImage()
{
    {
        QMutexLocker l(&_imp->lastRenderArgsMutex);
        _imp->lastImage.reset();
    }
}

void
EffectInstance::aboutToRestoreDefaultValues()
{
    ///Invalidate the cache by incrementing the age
    _node->incrementKnobsAge();

    if ( _node->areKeyframesVisibleOnTimeline() ) {
        _node->hideKeyframesFromTimeline(true);
    }
}

/**
 * @brief Returns a pointer to the first non disabled upstream node.
 * When cycling through the tree, we prefer non optional inputs and we span inputs
 * from last to first.
 **/
Natron::EffectInstance*
EffectInstance::getNearestNonDisabled() const
{
    if ( !_node->isNodeDisabled() ) {
        return _node->getLiveInstance();
    } else {
        ///Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<Natron::EffectInstance*> nonOptionalInputs;
        std::list<Natron::EffectInstance*> optionalInputs;
        int maxInp = getMaxInputCount();

        ///We cycle in reverse by default. It should be a setting of the application.
        ///In this case it will return input B instead of input A of a merge for example.
        for (int i = maxInp - 1; i >= 0; --i) {
            Natron::EffectInstance* inp = getInput(i);
            bool optional = isInputOptional(i);
            if (inp) {
                if (optional) {
                    optionalInputs.push_back(inp);
                } else {
                    nonOptionalInputs.push_back(inp);
                }
            }
        }

        ///Cycle through all non optional inputs first
        for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        ///Cycle through optional inputs...
        for (std::list<Natron::EffectInstance*> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        ///We didn't find anything upstream, return
        return NULL;
    }
}

Natron::EffectInstance*
EffectInstance::getNearestNonDisabledPrevious(int* inputNb)
{
    assert(_node->isNodeDisabled());
    
    ///Test all inputs recursively, going from last to first, preferring non optional inputs.
    std::list<Natron::EffectInstance*> nonOptionalInputs;
    std::list<Natron::EffectInstance*> optionalInputs;
    int maxInp = getMaxInputCount();
    
    
    int localPreferredInput = -1;
    
    ///We cycle in reverse by default. It should be a setting of the application.
    ///In this case it will return input B instead of input A of a merge for example.
    for (int i = maxInp - 1; i >= 0; --i) {
        Natron::EffectInstance* inp = getInput(i);
        bool optional = isInputOptional(i);
        if (inp) {
            if (optional) {
                if (localPreferredInput == -1) {
                    localPreferredInput = i;
                }
                optionalInputs.push_back(inp);
            } else {
                if (localPreferredInput == -1) {
                    localPreferredInput = i;
                }
                nonOptionalInputs.push_back(inp);
            }
        }
    }
    
    
    ///Cycle through all non optional inputs first
    for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        if ((*it)->getNode()->isNodeDisabled()) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }
    
    ///Cycle through optional inputs...
    for (std::list<Natron::EffectInstance*> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
        if ((*it)->getNode()->isNodeDisabled()) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }
    
    *inputNb = localPreferredInput;
    return this;
    
}

Natron::EffectInstance*
EffectInstance::getNearestNonIdentity(int time)
{
    
    U64 hash = getHash();
    RenderScale scale;
    scale.x = scale.y = 1.;
    
    RectD rod;
    bool isProjectFormat;
    Natron::StatusEnum stat = getRegionOfDefinition_public(hash, time, scale, 0, &rod, &isProjectFormat);
    
    double par = getPreferredAspectRatio();
    
    ///Ignore the result of getRoD if it failed
    (void)stat;
    
    SequenceTime inputTimeIdentity;
    int inputNbIdentity;
    
    if ( !isIdentity_public(hash, time, scale, rod, par, 0, &inputTimeIdentity, &inputNbIdentity) ) {
        return this;
    } else {
        
        if (inputNbIdentity < 0) {
            return this;
        }
        Natron::EffectInstance* effect = getInput(inputNbIdentity);
        return effect ? effect->getNearestNonIdentity(time) : this;
    }

}

void
EffectInstance::onNodeHashChanged(U64 hash)
{
    
    ///Always running in the MAIN THREAD
    assert(QThread::currentThread() == qApp->thread());
    
    ///Invalidate actions cache
    _imp->actionsCache.invalidateAll(hash);
}

bool
EffectInstance::canSetValue() const
{
    return !_node->isNodeRendering() || appPTR->isBackground();
}

SequenceTime
EffectInstance::getCurrentTime() const
{
    return getThreadLocalRenderTime();
}

#ifdef DEBUG
void
EffectInstance::checkCanSetValueAndWarn() const
{
    if (!checkCanSetValue()) {
        qDebug() << getName_mt_safe().c_str() << ": setValue()/setValueAtTime() was called during an action that is not allowed to call this function.";
    }
}
#endif

static
void isFrameVaryingOrAnimated_impl(const Natron::EffectInstance* node,bool *ret)
{
    if (node->isFrameVarying() || node->getHasAnimation() || node->getNode()->getRotoContext()) {
        *ret = true;
    } else {
        int maxInputs = node->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            Natron::EffectInstance* input = node->getInput(i);
            if (input) {
                isFrameVaryingOrAnimated_impl(input,ret);
                if (*ret) {
                    return;
                }
            }
        }
    }
}

bool
EffectInstance::isFrameVaryingOrAnimated_Recursive() const
{
    bool ret = false;
    isFrameVaryingOrAnimated_impl(this,&ret);
    return ret;
}

OutputEffectInstance::OutputEffectInstance(boost::shared_ptr<Node> node)
    : Natron::EffectInstance(node)
      , _writerCurrentFrame(0)
      , _writerFirstFrame(0)
      , _writerLastFrame(0)
      , _outputEffectDataLock(new QMutex)
      , _renderController(0)
      , _engine(0)
{
}

OutputEffectInstance::~OutputEffectInstance()
{
    if (_engine) {
        ///Thread must have been killed before.
        assert( !_engine->hasThreadsAlive() );
    }
    delete _engine;
    delete _outputEffectDataLock;
}

void
OutputEffectInstance::renderCurrentFrame(bool canAbort)
{
    _engine->renderCurrentFrame(canAbort);
}

bool
OutputEffectInstance::ifInfiniteclipRectToProjectDefault(RectD* rod) const
{
    if ( !getApp()->getProject() ) {
        return false;
    }
    /*If the rod is infinite clip it to the project's default*/
    Format projectDefault;
    getRenderFormat(&projectDefault);
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    // an int can not be equal to (or compared to) std::numeric_limits<double>::infinity()
    bool isRodProjctFormat = false;
    if (rod->left() <= kOfxFlagInfiniteMin) {
        rod->set_left( projectDefault.left() );
        isRodProjctFormat = true;
    }
    if (rod->bottom() <= kOfxFlagInfiniteMin) {
        rod->set_bottom( projectDefault.bottom() );
        isRodProjctFormat = true;
    }
    if (rod->right() >= kOfxFlagInfiniteMax) {
        rod->set_right( projectDefault.right() );
        isRodProjctFormat = true;
    }
    if (rod->top() >= kOfxFlagInfiniteMax) {
        rod->set_top( projectDefault.top() );
        isRodProjctFormat = true;
    }

    return isRodProjctFormat;
}

void
OutputEffectInstance::renderFullSequence(BlockingBackgroundRender* renderController,int first,int last)
{
    _renderController = renderController;
    
    ///Make sure that the file path exists
    boost::shared_ptr<KnobI> fileParam = getKnobByName(kOfxImageEffectFileParamName);
    if (fileParam) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(fileParam.get());
        if (isString) {
            std::string pattern = isString->getValue();
            std::string path = SequenceParsing::removePath(pattern);
            std::map<std::string,std::string> env;
            getApp()->getProject()->getEnvironmentVariables(env);
            Project::expandVariable(env, path);
            QDir().mkpath(path.c_str());
        }
    }
    ///If you want writers to render backward (from last to first), just change the flag in parameter here
    _engine->renderFrameRange(first,last,OutputSchedulerThread::RENDER_FORWARD);

}

void
OutputEffectInstance::notifyRenderFinished()
{
    if (_renderController) {
        _renderController->notifyFinished();
        _renderController = 0;
    }
}

int
OutputEffectInstance::getCurrentFrame() const
{
    QMutexLocker l(_outputEffectDataLock);

    return _writerCurrentFrame;
}

void
OutputEffectInstance::setCurrentFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);

    _writerCurrentFrame = f;
}

void
OutputEffectInstance::incrementCurrentFrame()
{
    QMutexLocker l(_outputEffectDataLock);
    ++_writerCurrentFrame;
}

void
OutputEffectInstance::decrementCurrentFrame()
{
    QMutexLocker l(_outputEffectDataLock);
    --_writerCurrentFrame;
}

int
OutputEffectInstance::getFirstFrame() const
{
    QMutexLocker l(_outputEffectDataLock);

    return _writerFirstFrame;
}

void
OutputEffectInstance::setFirstFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);

    _writerFirstFrame = f;
}

int
OutputEffectInstance::getLastFrame() const
{
    QMutexLocker l(_outputEffectDataLock);

    return _writerLastFrame;
}

void
OutputEffectInstance::setLastFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);

    _writerLastFrame = f;
}

void
OutputEffectInstance::initializeData()
{
    _engine= createRenderEngine();
}

RenderEngine*
OutputEffectInstance::createRenderEngine()
{
    return new RenderEngine(this);
}

double
EffectInstance::getPreferredFrameRate() const
{
    return getApp()->getProjectFrameRate();
}

void
EffectInstance::checkOFXClipPreferences_recursive(double time,
                                       const RenderScale & scale,
                                       const std::string & reason,
                                       bool forceGetClipPrefAction,
                                       std::list<Natron::Node*>& markedNodes)
{
    std::list<Natron::Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), _node.get());
    if (found != markedNodes.end()) {
        return;
    }
    
    checkOFXClipPreferences(time, scale, reason, forceGetClipPrefAction);
    markedNodes.push_back(_node.get());
    
    const std::list<Natron::Node*> & outputs = _node->getOutputs();
    for (std::list<Natron::Node*>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->getLiveInstance()->checkOFXClipPreferences_recursive(time, scale, reason, forceGetClipPrefAction,markedNodes);
    }
}

void
EffectInstance::checkOFXClipPreferences_public(double time,
                                    const RenderScale & scale,
                                    const std::string & reason,
                                    bool forceGetClipPrefAction,
                                    bool recurse)
{
    assert(QThread::currentThread() == qApp->thread());
    
    if (recurse) {
        std::list<Natron::Node*> markedNodes;
        checkOFXClipPreferences_recursive(time, scale, reason, forceGetClipPrefAction, markedNodes);
    } else {
        checkOFXClipPreferences(time, scale, reason, forceGetClipPrefAction);
    }
}


