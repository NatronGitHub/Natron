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
 * @brief These args are local to a renderRoI call and used to retrieve these infos 
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
        if ( inputImages.hasLocalData() ) {
            inputImages.localData().push_back(img);
        } else {
            std::list< boost::shared_ptr<Natron::Image> > newList;
            newList.push_back(img);
            inputImages.localData() = newList;
        }
    }

    void clearInputImagePointers()
    {
        if ( inputImages.hasLocalData() ) {
            inputImages.localData().clear();
        }
    }
};

EffectInstance::EffectInstance(boost::shared_ptr<Node> node)
    : NamedKnobHolder(node ? node->getApp() : NULL)
      , _node(node)
      , _imp(new Implementation)
{
}

EffectInstance::~EffectInstance()
{
    if (_node) {
        appPTR->removeAllImagesFromCacheWithMatchingKey( getHash() );
    }
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
                           bool byPassCache,
                           U64 nodeHash,
                           U64 rotoAge)
{
    ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
    
    args.time = time;
    args.view = view;
    args.isRenderResponseToUserInteraction = isRenderUserInteraction;
    args.isSequentialRender = isSequential;
    
    if ( !byPassCache && isWriter() ) {
        args.byPassCache = true;
    } else {
        args.byPassCache = byPassCache;
    }
    
    args.nodeHash = nodeHash;
    args.rotoAge = rotoAge;
    
    ++args.validArgs;
    
}

void
EffectInstance::invalidateParallelRenderArgs()
{
    assert(_imp->frameRenderArgs.hasLocalData());
    ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
    --args.validArgs;
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
EffectInstance::aborted() const
{
    {
        ///This flag is set in OutputSchedulerThread::abortRendering
        ///This will be used when playback or rendering on disk
        QReadLocker l(&_imp->renderAbortedMutex);
        if (_imp->renderAborted) {
            return true;
        }
    }
    
    ///Now check thread-local storage to find out in the case of renders made upon user interaction
    ///(i.e: issued by RenderEngine::renderCurrentFrame) whether the hash or time has changed
    
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
                ///Rendering issued by RenderEngine::renderCurrentFrame, if time or hash changed, abort
                return args.nodeHash != getHash() ||
                args.time != getApp()->getTimeLine()->currentFrame();
                
            } else {
                ///Rendering is playback or render on disk, we rely on the _imp->renderAborted flag for this.
                return false;
                
            }
        }

    }
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
                                                int* identityInputNb_p,
                                                int* identityTime_p,
                                                RectD* rod_p,
                                                RoIMap* inputRois_p, //!< output, only set if optionalBoundsParam != NULL
                                                RectD* optionalBounds_p) //!< output, only set if optionalBoundsParam != NULL
{
    /////Update 09/02/14
    /// We now AUTHORIZE GetRegionOfDefinition and isIdentity and getRegionsOfInterest to be called recursively.
    /// It didn't make much sense to forbid them from being recursive.
    
#ifdef DEBUG
    if (QThread::currentThread() != qApp->thread()) {
        ///This is a bad plug-in
        qDebug() << getNode()->getName_mt_safe().c_str() << " is trying to call clipGetImage during an unauthorized time. "
        "Developers of that plug-in should fix it. \n Reminder from the OpenFX spec: \n "
        "Images may be fetched from an attached clip in the following situations... \n"
        "- in the kOfxImageEffectActionRender action\n"
        "- in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason or kOfxChangeUserEdited";
    }
#endif
    
    ///Try to compensate for the mistake
    
    *nodeHash_p = getHash();
    const U64& nodeHash = *nodeHash_p;
    boost::shared_ptr<RotoContext> roto =  getNode()->getRotoContext();
    if (roto) {
        *rotoAge_p = roto->getAge();
    } else {
        *rotoAge_p = 0;
    }
    
    Natron::Status stat = getRegionOfDefinition(nodeHash, time, scale, view, rod_p);
    if (stat == StatFailed) {
        return false;
    }
    const RectD& rod = *rod_p;
    
    
    if (!optionalBoundsParam) {
#pragma message WARN("The actual contents pointed to by optionalBoundsParam is never used, is this normal? please comment.")
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
        *inputRois_p = getRegionsOfInterest(time, scale, optionalBounds, optionalBounds, 0);
    }
    
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );
    try {
        *isIdentity_p = isIdentity_public(nodeHash, time, scale, rod, view, identityTime_p, identityInputNb_p);
    } catch (...) {
        return false;
    }


    return true;
}

boost::shared_ptr<Natron::Image>
EffectInstance::getImage(int inputNb,
                         const SequenceTime time,
                         const RenderScale & scale,
                         const int view,
                         const RectD *optionalBoundsParam, //!< optional region in canonical coordinates
                         const Natron::ImageComponents comp,
                         const Natron::ImageBitDepth depth,
                         const bool dontUpscale,
                         RectI* roiPixel)
{
    bool isMask = isInputMask(inputNb);

    if ( isMask && !isMaskEnabled(inputNb) ) {
        ///This is last resort, the plug-in should've checked getConnected() before, which would have returned false.
        return boost::shared_ptr<Natron::Image>();
    }

    ///The input we want the image from
    EffectInstance* n = getInput(inputNb);
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
    SequenceTime identityTime;
    int inputNbIdentity;
    U64 nodeHash;
    U64 rotoAge;
    bool byPassCache = false;
    
    ///The caller thread MUST be a thread owned by Natron. It cannot be a thread from the multi-thread suite.
    ///A call to getImage is forbidden outside an action running in a thread launched by Natron.

    /// From http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsImagesAndClipsUsingClips
    //    Images may be fetched from an attached clip in the following situations...
    //    in the kOfxImageEffectActionRender action
    //    in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason of kOfxChangeUserEdited

    if ( !_imp->renderArgs.hasLocalData() || !_imp->frameRenderArgs.hasLocalData() ) {
        
        if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &rotoAge, &isIdentity, &inputNbIdentity, &identityTime, &rod, &inputsRoI, &optionalBounds) ) {
            return boost::shared_ptr<Image>();
        }
       
    } else {
        
        RenderArgs& renderArgs = _imp->renderArgs.localData();
        ParallelRenderArgs& frameRenderArgs = _imp->frameRenderArgs.localData();
        
        if (!renderArgs._validArgs || !frameRenderArgs.validArgs) {
#pragma message WARN("Value stored to 'identityTime is never read' - please fix and comment")
            if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &rotoAge, &isIdentity, &inputNbIdentity, &identityTime, &rod, &inputsRoI, &optionalBounds) ) {
                return boost::shared_ptr<Image>();
            }
            
        } else {
            inputsRoI = renderArgs._regionOfInterestResults;
            rod = renderArgs._rod;
            isIdentity = renderArgs._isIdentity;
            identityTime = renderArgs._identityTime;
            inputNbIdentity = renderArgs._identityInputNb;
            nodeHash = frameRenderArgs.nodeHash;
            rotoAge = frameRenderArgs.rotoAge;
            byPassCache = frameRenderArgs.byPassCache;
        }
        
        
    }

    RectD roi;
    if (!optionalBoundsParam) {
        RoIMap::iterator found = inputsRoI.find(useRotoInput ? this : n);
        assert( found != inputsRoI.end() );
        ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
        roi = found->second;
    } else {
        roi = optionalBounds;
    }


    ///If the effect is an identity but it didn't ask for the effect's image of which it is identity
    ///return a null image
    if ( isIdentity && (inputNbIdentity != inputNb) ) {
        return boost::shared_ptr<Image>();
    }

    ///Both the result of getRegionsOfInterest and optionalBounds are in canonical coordinates, we have to convert in both cases
    ///Convert to pixel coordinates
    RectI pixelRoI;
    roi.toPixelEnclosing(scale, &pixelRoI);

    int channelForAlpha = !isMask ? -1 : getMaskChannel(inputNb);

    if (useRotoInput) {
        
        Natron::ImageComponents outputComps;
        Natron::ImageBitDepth outputDepth;
        getPreferredDepthAndComponents(-1, &outputComps, &outputDepth);
        boost::shared_ptr<Natron::Image> mask =  roto->renderMask(pixelRoI, outputComps, nodeHash,rotoAge,
                                                                  RectD(), time, depth, view, mipMapLevel, byPassCache);
        _imp->addInputImageTempPointer(mask);
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
                                                                    mipMapLevel,
                                                                    view,
                                                                    pixelRoI,
                                                                    RectD(),
                                                                    comp,
                                                                    depth,
                                                                    channelForAlpha) );

    if (!inputImg) {
        return inputImg;
    }
    if (roiPixel) {
        *roiPixel = pixelRoI;
    }
    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();

    ///If the plug-in doesn't support the render scale, but the image is downscale, up-scale it.
    ///Note that we do NOT cache it
    if ( !dontUpscale && (inputImgMipMapLevel != 0) && !supportsRenderScale() ) {
        Natron::ImageBitDepth bitdepth = inputImg->getBitDepth();
        int mipMapLevel = 0;
        RectI bounds;
        inputImg->getRoD().toPixelEnclosing(mipMapLevel, &bounds);
        boost::shared_ptr<Natron::Image> upscaledImg( new Natron::Image(inputImg->getComponents(), inputImg->getRoD(),
                                                                        bounds, mipMapLevel, bitdepth) );
        //inputImg->upscaleMipMap(inputImg->getBounds(), inputImgMipMapLevel, mipMapLevel, upscaledImg.get());
        inputImg->scaleBox( inputImg->getBounds(), upscaledImg.get() );

        return upscaledImg;
    } else {
        _imp->addInputImageTempPointer(inputImg);

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

Natron::Status
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
            Status st = input->getRegionOfDefinition_public(hash,time, renderMappedScale, view, &inputRod, &isProjectFormat);
            assert(inputRod.x2 >= inputRod.x1 && inputRod.y2 >= inputRod.y1);
            if (st == StatFailed) {
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

    return StatReplyDefault;
}

bool
EffectInstance::ifInfiniteApplyHeuristic(U64 hash,
                                         SequenceTime time,
                                         const RenderScale & scale,
                                         int view,
                                         RectD* rod) //!< input/output
{
    /*If the rod is infinite clip it to the project's default*/

    Format projectDefault;

    getRenderFormat(&projectDefault);
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
                Status st = input->getRegionOfDefinition_public(hash,time, inputScale, view, &inputRod, &isProjectFormat);
                if (st != StatFailed) {
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
    }
    if (y1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y1 = std::min(inputsUnion.y1, projectDefault.y1);
        } else {
            rod->y1 = projectDefault.y1;
            isProjectFormat = true;
        }
    }
    if (x2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x2 = std::max(inputsUnion.x2, projectDefault.x2);
        } else {
            rod->x2 = projectDefault.x2;
            isProjectFormat = true;
        }
    }
    if (y2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y2 = std::max(inputsUnion.y2, projectDefault.y2);
        } else {
            rod->y2 = projectDefault.y2;
            isProjectFormat = true;
        }
    }
    if ( isProjectFormat && !isGenerator() ) {
        isProjectFormat = false;
    }
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

    return isProjectFormat;
} // ifInfiniteApplyHeuristic

EffectInstance::RoIMap
EffectInstance::getRegionsOfInterest(SequenceTime /*time*/,
                                     const RenderScale & /*scale*/,
                                     const RectD & /*outputRoD*/, //!< the RoD of the effect, in canonical coordinates
                                     const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                     int /*view*/)
{
    RoIMap ret;

    for (int i = 0; i < getMaxInputCount(); ++i) {
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            ret.insert( std::make_pair(input, renderWindow) );
        }
    }

    return ret;
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
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            ret.insert( std::make_pair(i, ranges) );
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

boost::shared_ptr<Natron::Image>
EffectInstance::renderRoI(const RenderRoIArgs & args)
{
   
    assert(_imp->frameRenderArgs.hasLocalData() && _imp->frameRenderArgs.localData().validArgs);
    ParallelRenderArgs& frameRenderArgs = _imp->frameRenderArgs.localData();
    
    ///The args must have been set calling setParallelRenderArgs
    assert(frameRenderArgs.validArgs);
    
    ///For writer we never want to cache otherwise the next time we want to render it will skip writing the image on disk!
    bool byPassCache = frameRenderArgs.byPassCache;

    ///Use the hash at this time, and then copy it to the clips in the thread local storage to use the same value
    ///through all the rendering of this frame.
    U64 nodeHash = frameRenderArgs.nodeHash;
 

    boost::shared_ptr<ImageParams> cachedImgParams;
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

    ///if the rod is already passed as parameter, just use it and don't call getRegionOfDefinition
    if ( !args.preComputedRoD.isNull() ) {
        rod = args.preComputedRoD;
    } else {
        ///before allocating it we must fill the RoD of the image we want to render
        assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        Status stat = getRegionOfDefinition_public(nodeHash,args.time, renderMappedScale, args.view, &rod, &isProjectFormat);

        ///The rod might be NULL for a roto that has no beziers and no input
        if ( (stat == StatFailed) || rod.isNull() ) {
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

    Natron::ImageKey key = Natron::Image::makeKey(nodeHash, args.time, args.mipMapLevel, args.view);
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
        if ( lastRenderedImage &&
             (lastRenderHash != nodeHash) ) {
            ///try to obtain the lock for the last rendered image as another thread might still rely on it in the cache
            ImageLocker imgLocker(this,lastRenderedImage);
            ///once we got it remove it from the cache
            appPTR->removeAllImagesFromCacheWithMatchingKey(lastRenderHash);
            {
                QMutexLocker l(&_imp->lastRenderArgsMutex);
                _imp->lastImage.reset();
            }
        }
    }

    /// First-off look-up the cache and see if we can find the cached actions results and cached image.
    bool isCached = Natron::getImageFromCache(key, &cachedImgParams,&image);

    ////Lock the output image so that multiple threads do not access for writing at the same time.
    ////When it goes out of scope the lock will be released automatically
    boost::shared_ptr<ImageLocker> imageLock;

    if (isCached) {
        assert(cachedImgParams && image);

        imageLock.reset( new ImageLocker(this,image) );

        if ( cachedImgParams->isRodProjectFormat() ) {
            ////If the image was cached with a RoD dependent on the project format, but the project format changed,
            ////just discard this entry
            Format projectFormat;
            getRenderFormat(&projectFormat);
            if ( static_cast<RectD &>(projectFormat) != cachedImgParams->getRoD() ) {
                isCached = false;
                appPTR->removeFromNodeCache(image);
                cachedImgParams.reset();
                imageLock.reset(); //< release the lock after cleaning it from the cache
                image.reset();
            }
        }

        //Do the following only if we're not an identity
        if ( image && (cachedImgParams->getInputNbIdentity() == -1) ) {
            ///If components are different but convertible without damage, or bit depth is different, keep this image, convert it
            ///and continue render on it. This is in theory still faster than ignoring the image and doing a full render again.
            if ( ( (image->getComponents() != args.components) && Image::hasEnoughDataToConvert(image->getComponents(),args.components) ) ||
                 ( image->getBitDepth() != args.bitdepth) ) {
                ///Convert the image to the requested components
                assert( !rod.isNull() );
                RectI bounds;
                rod.toPixelEnclosing(args.mipMapLevel, &bounds);
                boost::shared_ptr<Image> remappedImage( new Image(args.components, rod, bounds, args.mipMapLevel, args.bitdepth) );
                if (!byPassCache) {
                    
                    bool unPremultIfNeeded = getOutputPremultiplication() == ImagePremultiplied;
                    
                    image->convertToFormat( args.roi,
                                            getApp()->getDefaultColorSpaceForBitDepth( image->getBitDepth() ),
                                            getApp()->getDefaultColorSpaceForBitDepth(args.bitdepth),
                                            args.channelForAlpha, false, true,unPremultIfNeeded,
                                            remappedImage.get() );
                }
                ///switch the pointer
                image = remappedImage;
            } else if (image->getComponents() != args.components) {
                assert( !Image::hasEnoughDataToConvert(image->getComponents(),args.components) );
                ///we cannot convert without loosing data of some channels, we better off render everything again
                isCached = false;
                appPTR->removeFromNodeCache(image);
                cachedImgParams.reset();
                imageLock.reset(); //< release the lock after cleaning it from the cache
                image.reset();
            }

            if (isCached && byPassCache) {
                ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
                ///we're sure renderRoIInternal will compute the whole image again.
                ///We must use the cache facility anyway because we rely on it for caching the results
                ///of actions which is necessary to avoid recursive actions.
                image->clearBitmap();
            }
        }
    }

    boost::shared_ptr<Natron::Image> downscaledImage = image;

    if (!isCached) {
        ///first-off check whether the effect is identity, in which case we don't want
        /// to cache anything or render anything for this effect.
        SequenceTime inputTimeIdentity = 0.;
        int inputNbIdentity;
        FramesNeededMap framesNeeded;

        assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        bool identity;
        try {
            identity = isIdentity_public(nodeHash,args.time, renderMappedScale, rod, args.view, &inputTimeIdentity, &inputNbIdentity);
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
            args.roi.toCanonical_noClipping(args.mipMapLevel, &canonicalRoI);
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
                                 true,
                                 NULL);
                ///Clear input images pointer because getImage has stored the image .
                _imp->clearInputImagePointers();
            } else {
                return image;
            }

            ///if we bypass the cache, don't cache the result of isIdentity
            if (byPassCache) {
                return image;
            }
        } else {
            ///set it to -1 so the cache knows it's not an identity
            inputNbIdentity = -1;

            // why should the rod be empty here?
            assert( !rod.isNull() );

            framesNeeded = getFramesNeeded_public(args.time);
        }


        int cost = 0;
        /*should data be stored on a physical device ?*/
        if ( shouldRenderedDataBePersistent() ) {
            cost = 1;
        }

        if (identity) {
            cost = -1;
        }


        ///Cache the image with the requested components instead of the remapped ones
        cachedImgParams = Natron::Image::makeParams(cost,
                                                    rod,
                                                    args.mipMapLevel,
                                                    isProjectFormat,
                                                    args.components,
                                                    args.bitdepth,
                                                    inputNbIdentity,
                                                    inputTimeIdentity,
                                                    framesNeeded);

        imageLock.reset( new ImageLocker(this) );
        
        ///even though we called getImage before and it returned false, it may now
        ///return true if another thread created the image in the cache, so we can't
        ///make any assumption on the return value of this function call.
        ///
        ///!!!Note that if isIdentity is true it will allocate an empty image object with 0 bytes of data.
        boost::shared_ptr<Image> newImage;
        bool cached = appPTR->getImageOrCreate(key, cachedImgParams, imageLock.get(), &newImage);
        if (!newImage) {
            std::stringstream ss;
            ss << "Failed to allocate an image of ";
            ss << printAsRAM( cachedImgParams->getElementsCount() * sizeof(Image::data_t) ).toStdString();
            Natron::errorDialog( QObject::tr("Out of memory").toStdString(),ss.str() );

            return newImage;
        }
        assert(newImage);
        assert(newImage->getRoD() == rod);
        
        ///Take the lock for the output image flagging that we're accessing it.
        ///If !cached the lock has already been taken, we just have to allocate memory for the image
        if (cached) {
            imageLock->lock(newImage);
        } else {
            newImage->allocateMemory();
        }

        if (cached && byPassCache) {
            ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
            ///we're sure renderRoIInternal will compute the whole image again.
            ///We must use the cache facility anyway because we rely on it for caching the results
            ///of actions which is necessary to avoid recursive actions.
            newImage->clearBitmap();
        }

        ///if the plugin is an identity we just inserted in the cache the identity params, we can now return.
        if (identity) {
            ///don't return the empty allocated image but the input effect image instead!
            return image;
        }
        image = newImage;
        downscaledImage = image;

        if (renderFullScaleThenDownscale) {
            ///Allocate the upscaled image
            assert(renderMappedMipMapLevel == 0);
            RectI bounds;
            rod.toPixelEnclosing(renderMappedMipMapLevel, &bounds);
            image.reset( new Natron::Image(args.components, rod, bounds, renderMappedMipMapLevel, args.bitdepth) );
        }


        assert(cachedImgParams);
    } else {

        assert(cachedImgParams);
        assert(image);


        ///if it was cached, first thing to check is to see if it is an identity
        int inputNbIdentity = cachedImgParams->getInputNbIdentity();
        if (inputNbIdentity != -1) {
            SequenceTime inputTimeIdentity = cachedImgParams->getInputTimeIdentity();
            RectD canonicalRoI;
            
            
            int firstFrame,lastFrame;
            getFrameRange_public(nodeHash, &firstFrame, &lastFrame);
            
            assert( !rod.isNull() );
            ///WRONG! We can't clip against the RoD of *this* effect. We should clip against the RoD of the input effect, but this is done
            ///later on for us already.
            //args.roi.toCanonical(args.mipMapLevel, rod, &canonicalRoI);
            args.roi.toCanonical_noClipping(args.mipMapLevel, &canonicalRoI);
            RoIMap inputsRoI;
            inputsRoI.insert( std::make_pair(getInput(inputNbIdentity), canonicalRoI) );
            Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,
                                                        inputsRoI,
                                                        rod,
                                                        args.roi,
                                                        args.time,
                                                        args.view,
                                                        args.channelForAlpha,
                                                        true,
                                                        inputTimeIdentity,
                                                        inputNbIdentity,
                                                        boost::shared_ptr<Image>(),
                                                        firstFrame,
                                                        lastFrame);
            Natron::EffectInstance* inputEffectIdentity = getInput(inputNbIdentity);
            if (inputEffectIdentity) {
                boost::shared_ptr<Image> ret =  getImage(inputNbIdentity, inputTimeIdentity,
                                                         args.scale, args.view, NULL, args.components, args.bitdepth, true,NULL);
                ///Clear input images pointer because getImage has stored ret .
                _imp->clearInputImagePointers();

                return ret;
            } else {
                return boost::shared_ptr<Image>();
            }
        }

#     ifdef DEBUG
        ///If the precomputed rod parameter was set, assert that it is the same than the image in cache.
        if ( (inputNbIdentity != -1) && !args.preComputedRoD.isNull() ) {
            assert( args.preComputedRoD == cachedImgParams->getRoD() );
        }
#     endif // DEBUG

        ///For effects that don't support the render scale we have to upscale this cached image,
        ///render the parts we are interested in and then downscale again
        ///Before doing that we verify if everything we want is already rendered in which case we
        ///dont degrade the image
        if (renderFullScaleThenDownscale) {
            RectI intersection;
            args.roi.intersect(image->getBounds(), &intersection);
            std::list<RectI> rectsRendered = image->getRestToRender(intersection);
            if ( rectsRendered.empty() ) {
                return image;
            }

            downscaledImage = image;

            assert(renderMappedMipMapLevel == 0);
            RectD rod = cachedImgParams->getRoD();
            RectI bounds;
            rod.toPixelEnclosing(renderMappedMipMapLevel, &bounds);
            ///Allocate the upscaled image
            boost::shared_ptr<Natron::Image> upscaledImage( new Natron::Image(args.components, rod, bounds, renderMappedMipMapLevel, args.bitdepth) );
            downscaledImage->scaleBox( downscaledImage->getBounds(),upscaledImage.get() );
            image = upscaledImage;
        }
    }

    ////The lock must be taken here otherwise this could lead to corruption if multiple threads
    ////are accessing the output image.
    assert(imageLock);

    ///If we reach here, it can be either because the image is cached or not, either way
    ///the image is NOT an identity, and it may have some content left to render.
    EffectInstance::RenderRoIStatus renderRetCode = renderRoIInternal(args.time,
                                                                      args.scale,
                                                                      args.mipMapLevel,
                                                                      args.view,
                                                                      args.roi,
                                                                      rod,
                                                                      cachedImgParams,
                                                                      image,
                                                                      downscaledImage,
                                                                      frameRenderArgs.isSequentialRender,
                                                                      frameRenderArgs.isRenderResponseToUserInteraction,
                                                                      byPassCache,
                                                                      nodeHash,
                                                                      args.channelForAlpha,
                                                                      renderFullScaleThenDownscale);


    if ( aborted() && renderRetCode != eImageAlreadyRendered) {
        //if render was aborted, remove the frame from the cache as it contains only garbage
        appPTR->removeFromNodeCache(image);
    } else if (renderRetCode == eImageRenderFailed) {
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

void
EffectInstance::renderRoI(SequenceTime time,
                          const RenderScale & scale,
                          unsigned int mipMapLevel,
                          int view,
                          const RectI & renderWindow,
                          const RectD & rod, //!< effect rod in canonical coords
                          const boost::shared_ptr<ImageParams> & cachedImgParams,
                          const boost::shared_ptr<Image> & image,
                          const boost::shared_ptr<Image> & downscaledImage)
{
    
    assert(_imp->frameRenderArgs.hasLocalData());
    ParallelRenderArgs& frameRenderArgs = _imp->frameRenderArgs.localData();
    
    SupportsEnum supportsRS = supportsRenderScaleMaybe();
    ///This flag is relevant only when the mipMapLevel is different than 0. We use it to determine
    ///wether the plug-in should render in the full scale image, and then we downscale afterwards or
    ///if the plug-in can just use the downscaled image to render.
    bool renderFullScaleThenDownscale = (supportsRS == eSupportsNo && mipMapLevel != 0);
    EffectInstance::RenderRoIStatus renderRetCode = renderRoIInternal(time,
                                                                      scale,
                                                                      mipMapLevel,
                                                                      view,
                                                                      renderWindow,
                                                                      rod,
                                                                      cachedImgParams,
                                                                      image,
                                                                      downscaledImage,
                                                                      frameRenderArgs.isSequentialRender,
                                                                      frameRenderArgs.isRenderResponseToUserInteraction,
                                                                      frameRenderArgs.byPassCache,
                                                                      frameRenderArgs.nodeHash,
                                                                      3,
                                                                      renderFullScaleThenDownscale);

    if ( (renderRetCode == eImageRenderFailed) && !aborted() ) {
        throw std::runtime_error("Rendering Failed");
    }
}

EffectInstance::RenderRoIStatus
EffectInstance::renderRoIInternal(SequenceTime time,
                                  const RenderScale & scale,
                                  unsigned int mipMapLevel,
                                  int view,
                                  const RectI & renderWindow, //!< seems to be in downscaledImage's pixel coordinates ??
                                  const RectD & rod, //!< effect rod in canonical coords
                                  const boost::shared_ptr<ImageParams> & cachedImgParams,
                                  const boost::shared_ptr<Image> & image,
                                  const boost::shared_ptr<Image> & downscaledImage,
                                  bool isSequentialRender,
                                  bool isRenderMadeInResponseToUserInteraction,
                                  bool byPassCache,
                                  U64 nodeHash,
                                  int channelForAlpha,
                                  bool renderFullScaleThenDownscale)
{
    EffectInstance::RenderRoIStatus retCode;

    ///First off check if the requested components and bitdepth are supported by the output clip
    Natron::ImageBitDepth outputDepth;
    Natron::ImageComponents outputComponents;

    getPreferredDepthAndComponents(-1, &outputComponents, &outputDepth);
    bool imageConversionNeeded = outputComponents != image->getComponents() || outputDepth != image->getBitDepth();

    assert( isSupportedBitDepth(outputDepth) && isSupportedComponent(-1, outputComponents) );

    unsigned int renderMappedMipMapLevel;
    if (renderFullScaleThenDownscale) {
        renderMappedMipMapLevel = image->getMipMapLevel();
    } else {
        renderMappedMipMapLevel = downscaledImage->getMipMapLevel();
    }
    RenderScale renderMappedScale;
    renderMappedScale.x = renderMappedScale.y = Image::getScaleFromMipMapLevel(renderMappedMipMapLevel);

    ///The image and downscaled image are pointing to the same image in 2 cases:
    ///1) Proxy mode is turned off
    ///2) Proxy mode is turned on but plug-in supports render scale
    ///Subsequently the image and downscaled image are different only if the plug-in
    ///does not support the render scale and the proxy mode is turned on.
    assert( (image == downscaledImage && !renderFullScaleThenDownscale) ||
            (image != downscaledImage && renderFullScaleThenDownscale) );


    ///Add the window to the project's available formats if the effect is a reader
    ///This is the only reliable place where I could put these lines...which don't seem to feel right here.
    ///Plus setOrAddProjectFormat will actually set the project format the first time we read an image in the project
    ///hence ask for a new render... which can be expensive!
    ///Any solution how to work around this ?
    if ( isReader() ) {
        Format frmt;
        frmt.set( cachedImgParams->getRoD() );
        ///FIXME: what about the pixel aspect ratio ?
        getApp()->getProject()->setOrAddProjectFormat(frmt);
    }


    bool tilesSupported = supportsTiles();

    ///We check what is left to render.

    ///intersect the image render window to the actual image region of definition.
    ///Intersection will be in pixel coordinates (downscaled).
    RectI intersection;

    // make sure the renderWindow falls within the image bounds
    renderWindow.intersect(image->getBounds(), &intersection);

    /// If the list is empty then we already rendered it all
    /// Each rect in this list is in pixel coordinate (downscaled)
    std::list<RectI> rectsToRender = downscaledImage->getRestToRender(intersection);

    ///if the effect doesn't support tiles and it has something left to render, just render the rod again
    ///note that it should NEVER happen because if it doesn't support tiles in the first place, it would
    ///have rendered the rod already.
    if ( !tilesSupported && !rectsToRender.empty() ) {
        ///if the effect doesn't support tiles, just render the whole rod again even though
        rectsToRender.clear();
        rectsToRender.push_back( downscaledImage->getBounds() );
    }
    
    ///Here we fetch the age of the roto context if there's any to pass it through
    ///the tree and remember it when the plugin calls getImage() afterwards
    boost::shared_ptr<RotoContext> rotoContext = _node->getRotoContext();
    U64 rotoAge = rotoContext ? rotoContext->getAge() : 0;
    Natron::Status renderStatus = StatOK;

    if ( rectsToRender.empty() ) {
        retCode = EffectInstance::eImageAlreadyRendered;
    } else {
        retCode = EffectInstance::eImageRendered;
    }


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
                rod.toPixelEnclosing(renderMappedMipMapLevel, &bounds);
                fullScaleMappedImage.reset( new Image(outputComponents, rod, bounds, renderMappedMipMapLevel, outputDepth) );
                downscaledMappedImage = downscaledImage;
                assert( downscaledMappedImage->getBounds() == downscaledImage->getBounds() );
                assert( fullScaleMappedImage->getBounds() == image->getBounds() );
            } else {
                RectD rod = downscaledImage->getRoD();
                RectI bounds;
                rod.toPixelEnclosing(mipMapLevel, &bounds);
                downscaledMappedImage.reset( new Image(outputComponents, rod, bounds, mipMapLevel, outputDepth) );
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

    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        const RectI & downscaledRectToRender = *it; // please leave it as const, copy it if necessary

        ///Upscale the RoI to a region in the full scale image so it is in canonical coordinates
        RectD canonicalRectToRender;
        downscaledRectToRender.toCanonical(mipMapLevel, rod, &canonicalRectToRender);

        ///the getRegionsOfInterest call will not be cached because it would be unnecessary
        ///To put that information (which depends on the RoI) into the cache. That's why we
        ///store it into the render args (thread-storage) so the getImage() function can retrieve the results.
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );

        
        ///There cannot be the same thread running 2 concurrent instances of renderRoI on the same effect.
        assert(!_imp->renderArgs.hasLocalData() || !_imp->renderArgs.localData()._validArgs);

        RectI renderMappedRectToRender;
        
        if (renderFullScaleThenDownscale) {
            canonicalRectToRender.toPixelEnclosing(0, &renderMappedRectToRender);
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
        
        RoIMap inputsRoi = getRegionsOfInterest_public(time, renderMappedScale, rod, canonicalRectToRender, view);

       
        ///If the effect is a writer, byPassCache was set to true to make sure the image
        ///we got from the cache would get its bitmap cleared (indicating we would have to call render again)
        ///but for the render args, we set byPassCache to false otherwise each input will not benefit of the
        ///cache, which would drastically reduce effiency.
        if ( isWriter() ) {
            byPassCache = false;
        }

        int firstFrame, lastFrame;
        getFrameRange_public(nodeHash, &firstFrame, &lastFrame);
        
        ///The scoped args will maintain the args set for this thread during the
        ///whole time the render action is called, so they can be fetched in the
        ///getImage() call.
        /// @see EffectInstance::getImage
        scopedArgs.setArgs_secondPass(inputsRoi,firstFrame,lastFrame);
        const RenderArgs & args = scopedArgs.getArgs();


        ///Get the frames needed.
        const FramesNeededMap & framesNeeeded = cachedImgParams->getFramesNeeded();

        ///We render each input first and stash their image in the inputImages list
        ///in order to maintain a shared_ptr use_count > 1 so the cache doesn't attempt
        ///to remove them.
        std::list< boost::shared_ptr<Natron::Image> > inputImages;

        for (FramesNeededMap::const_iterator it2 = framesNeeeded.begin(); it2 != framesNeeeded.end(); ++it2) {
            ///We have to do this here because the enabledness of a mask is a feature added by Natron.
            bool inputIsMask = isInputMask(it2->first);
            if ( inputIsMask && !isMaskEnabled(it2->first) ) {
                continue;
            }

            EffectInstance* inputEffect = getInput(it2->first);
            if (inputEffect) {
                ///What region are we interested in for this input effect ? (This is in Canonical coords)
                RoIMap::iterator foundInputRoI = inputsRoi.find(inputEffect);
                assert( foundInputRoI != inputsRoi.end() );

                ///Convert to pixel coords the RoI
                if ( foundInputRoI->second.isInfinite() ) {
                    throw std::runtime_error(std::string("Plugin ") + this->getPluginLabel() + " asked for an infinite region of interest!");
                }

                RectI inputRoIPixelCoords;
                foundInputRoI->second.toPixelEnclosing(scale, &inputRoIPixelCoords);

                ///Notify the node that we're going to render something with the input
                assert(it2->first != -1); //< see getInputNumber
                
                bool didEmitSignalInputNRendering = _node->notifyInputNIsRendering(it2->first);

                ///For all frames requested for this node, render the RoI requested.
                for (U32 range = 0; range < it2->second.size(); ++range) {
                    for (U32 f = it2->second[range].min; f <= it2->second[range].max; ++f) {
                        Natron::ImageComponents inputPrefComps;
                        Natron::ImageBitDepth inputPrefDepth;
                        getPreferredDepthAndComponents(it2->first, &inputPrefComps, &inputPrefDepth);

                        int channelForAlphaInput = inputIsMask ? getMaskChannel(it2->first) : 3;
                        boost::shared_ptr<Natron::Image> inputImg =
                            inputEffect->renderRoI( RenderRoIArgs(f, //< time
                                                                  scale, //< scale
                                                                  mipMapLevel, //< mipmapLevel (redundant with the scale)
                                                                  view, //< view
                                                                  inputRoIPixelCoords, //< roi in pixel coordinates
                                                                  RectD(), // < did we precompute any RoD to speed-up the call ?
                                                                  inputPrefComps, //< requested comps
                                                                  inputPrefDepth,
                                                                  channelForAlphaInput) ); //< requested bitdepth

                        if (inputImg) {
                            inputImages.push_back(inputImg);
                        }
                    }
                }
                
                if (didEmitSignalInputNRendering) {
                    _node->notifyInputNIsFinishedRendering(it2->first);
                }

                if ( aborted() ) {
                    //if render was aborted, remove the frame from the cache as it contains only garbage
                    appPTR->removeFromNodeCache(image);

                    return eImageRendered;
                }
            }
        }

        ///if the node has a roto context, pre-render the roto mask too
        boost::shared_ptr<RotoContext> rotoCtx = _node->getRotoContext();
        if (rotoCtx) {
            Natron::ImageComponents inputPrefComps;
            Natron::ImageBitDepth inputPrefDepth;
            int rotoIndex = getRotoBrushInputIndex();
            assert(rotoIndex != -1);
            getPreferredDepthAndComponents(rotoIndex, &inputPrefComps, &inputPrefDepth);
            boost::shared_ptr<Natron::Image> mask = rotoCtx->renderMask(downscaledRectToRender,
                                                                        inputPrefComps,
                                                                        nodeHash,
                                                                        rotoAge,
                                                                        rod,
                                                                        time,
                                                                        inputPrefDepth,
                                                                        view,
                                                                        mipMapLevel,
                                                                        byPassCache);
            assert(mask);
            inputImages.push_back(mask);
        }

#     ifdef DEBUG
        RenderScale scale;
        scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
        scale.y = scale.x;
        // check the dimensions of all input and output images
        for (std::list< boost::shared_ptr<Natron::Image> >::const_iterator it = inputImages.begin();
             it != inputImages.end();
             ++it) {
            RectI srcBounds = (*it)->getBounds();
            const RectD & srcRodCanonical = (*it)->getRoD();
            RectI srcRod;
            srcRodCanonical.toPixelEnclosing(0, &srcRod);
            const RectI & dstBounds = renderMappedImage->getBounds();
            const RectD & dstRodCanonical = renderMappedImage->getRoD();
            RectI dstRod;
            dstRodCanonical.toPixelEnclosing(scale, &dstRod);

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
                if ( (*it)->getMipMapLevel() != 0 ) {
                    srcBounds = srcBounds.upscalePowerOfTwo( (*it)->getMipMapLevel() );
                    srcBounds.intersect(srcRod, &srcBounds);
                }
                assert(srcRod.x1 == srcBounds.x1);
                assert(srcRod.x2 == srcBounds.x2);
                assert(srcRod.y1 == srcBounds.y1);
                assert(srcRod.y2 == srcBounds.y2);
                assert(dstRod.x1 == dstBounds.x1);
                assert(dstRod.x2 == dstBounds.x2);
                assert(dstRod.y1 == dstBounds.y1);
                assert(dstRod.y2 == dstBounds.y2);
            }
            if ( !supportsMultiResolution() ) {
                // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
                //   Multiple resolution images mean...
                //    input and output images can be of any size
                //    input and output images can be offset from the origin
                assert(srcRod.x1 == 0);
                assert(srcRod.y1 == 0);
                assert(srcRod.x1 == dstRod.x1);
                assert(srcRod.x2 == dstRod.x2);
                assert(srcRod.y1 == dstRod.y1);
                assert(srcRod.y2 == dstRod.y2);
            }
        }
        if (supportsRenderScaleMaybe() == eSupportsNo) {
            assert(renderMappedMipMapLevel == 0);
            assert(renderMappedScale.x == 1. && renderMappedScale.y == 1.);
        }
#     endif // DEBUG

        ///notify the node we're starting a render
        bool didEmitRenderingStartedSignal = _node->notifyRenderingStarted();

        ///We only need to call begin if we've not already called it.
        bool callBegin = false;

        ///neer call beginsequenceRender here if the render is sequential
        
        Natron::SequentialPreference pref = getSequentialPreference();
        if (!isWriter() || pref == EFFECT_NOT_SEQUENTIAL) {
            callBegin = true;
        }

        if (callBegin) {
            assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
            if (beginSequenceRender_public(time, time, 1, !appPTR->isBackground(), renderMappedScale, isSequentialRender,
                                           isRenderMadeInResponseToUserInteraction, view) == StatFailed) {
                renderStatus = StatFailed;
                break;
            }
        }

        /*depending on the thread-safety of the plug-in we render with a different
           amount of threads*/
        EffectInstance::RenderSafety safety = renderThreadSafety();

        ///if the project lock is already locked at this point, don't start any other thread
        ///as it would lead to a deadlock when the project is loading.
        ///Just fall back to Fully_safe
        int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
        if (safety == FULLY_SAFE_FRAME) {
            ///If the plug-in is FULLY_SAFE_FRAME that means it wants the host to perform SMP aka slice up the RoI into chunks
            ///but if the effect doesn't support tiles it won't work.
            ///Also check that the number of threads indicating by the settings are appropriate for this render mode.
            if ( !tilesSupported || (nbThreads == -1) || (nbThreads == 1) ||
                ( (nbThreads == 0) && (appPTR->getHardwareIdealThreadCount() == 1) ) ||
                 ( QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount() ) ) {
                safety = FULLY_SAFE;
            } else {
                if ( !getApp()->getProject()->tryLock() ) {
                    safety = FULLY_SAFE;
                } else {
                    getApp()->getProject()->unlock();
                }
            }
        }
        
        assert(_imp->frameRenderArgs.hasLocalData());
        const ParallelRenderArgs& frameArgs = _imp->frameRenderArgs.localData();

        switch (safety) {
        case FULLY_SAFE_FRAME: {     // the plugin will not perform any per frame SMP threading
            // we can split the frame in tiles and do per frame SMP threading (see kOfxImageEffectPluginPropHostFrameThreading)
            if (nbThreads == 0) {
                nbThreads = QThreadPool::globalInstance()->maxThreadCount();
            }
            std::vector<RectI> splitRects = RectI::splitRectIntoSmallerRect(downscaledRectToRender, nbThreads);
            
            TiledRenderingFunctorArgs tiledArgs;
            tiledArgs.args = &args;
            tiledArgs.isSequentialRender = isSequentialRender;
            tiledArgs.isRenderResponseToUserInteraction = isRenderMadeInResponseToUserInteraction;
            tiledArgs.downscaledImage = downscaledImage;
            tiledArgs.downscaledMappedImage = downscaledMappedImage;
            tiledArgs.fullScaleImage = image;
            tiledArgs.fullScaleMappedImage = fullScaleMappedImage;
            tiledArgs.renderMappedImage = renderMappedImage;
            
            
            // the bitmap is checked again at the beginning of EffectInstance::tiledRenderingFunctor()
            QFuture<Natron::Status> ret = QtConcurrent::mapped( splitRects,
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
                                             view) == StatFailed) {
                    renderStatus = StatFailed;
                    break;
                }
            }
            for (QFuture<Natron::Status>::const_iterator it2 = ret.begin(); it2 != ret.end(); ++it2) {
                if ( (*it2) == Natron::StatFailed ) {
                    renderStatus = *it2;
                    break;
                }
            }
            break;
        }

        case INSTANCE_SAFE:     // indicating that any instance can have a single 'render' call at any one time,
        case FULLY_SAFE:        // indicating that any instance of a plugin can have multiple renders running simultaneously
        case UNSAFE: {     // indicating that only a single 'render' call can be made at any time amoung all instances
            // INSTANCE_SAFE means that there is at most one render per instance
            // NOTE: the per-instance lock should probably be shared between
            // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
            // and read-write on it.
            // It is probably safer to assume that several clones may write to the same output image only in the FULLY_SAFE case.

            // FULLY_SAFE means that there is only one render per FRAME : the lock is by image and handled in Node.cpp
            ///locks belongs to an instance)

            QMutexLocker *locker = 0;

            if (safety == INSTANCE_SAFE) {
                 locker = new QMutexLocker( &getNode()->getRenderInstancesSharedMutex() );
            } else if (safety == UNSAFE) {
                locker = new QMutexLocker( appPTR->getMutexForPlugin( getPluginID().c_str() ) );
            }
            ///For FULLY_SAFE, don't take any lock, the image already has a lock on itself so we're sure it can't be written to by 2 different threads.
            
            
            renderStatus = tiledRenderingFunctor(args,
                                                 frameArgs,
                                                 false,
                                                 renderFullScaleThenDownscale,
                                                 isSequentialRender,
                                                 isRenderMadeInResponseToUserInteraction,
                                                 downscaledRectToRender,
                                                 downscaledImage,
                                                 image,
                                                 downscaledMappedImage,
                                                 fullScaleMappedImage,
                                                 renderMappedImage);

            delete locker;
            break;
        }
        } // switch

        ///notify the node we've finished rendering
        
        if (didEmitRenderingStartedSignal) {
            _node->notifyRenderingEnded();
        }

        if (renderStatus != StatOK) {
            break;
        }
    }

    if (renderStatus != StatOK) {
        retCode = eImageRenderFailed;
    }
    //we released the input images and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();

    return retCode;
} // renderRoIInternal

Natron::Status
EffectInstance::tiledRenderingFunctor(const TiledRenderingFunctorArgs& args,
                                      const ParallelRenderArgs& frameArgs,
                                     bool setThreadLocalStorage,
                                     const RectI & roi )
{
    return tiledRenderingFunctor(*args.args,
                                 frameArgs,
                                 setThreadLocalStorage,
                                 args.renderFullScaleThenDownscale,
                                 args.isSequentialRender,
                                 args.isRenderResponseToUserInteraction,
                                 roi,
                                 args.downscaledImage,
                                 args.fullScaleImage,
                                 args.downscaledMappedImage,
                                 args.fullScaleMappedImage,
                                 args.renderMappedImage);
}

Natron::Status
EffectInstance::tiledRenderingFunctor(const RenderArgs & args,
                                      const ParallelRenderArgs& frameArgs,
                                      bool setThreadLocalStorage,
                                      bool renderFullScaleThenDownscale,
                                      bool isSequentialRender,
                                      bool isRenderResponseToUserInteraction,
                                      const RectI & downscaledRectToRender,
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
    
    
    ///Make the thread-storage live as long as the render action is called if we're in a newly launched thread in FULLY_SAFE_FRAME mode
    boost::shared_ptr<Implementation::ScopedRenderArgs> scopedArgs;
    boost::shared_ptr<Node::ParallelRenderArgsSetter> scopedFrameArgs;
    
    if (setThreadLocalStorage) {
        
        ///At this point if we're in FULLY_SAFE_FRAME mode, we are a thread that might have been launched way after
        ///the time renderRectToRender was computed. We recompute it to update the portion to render
        
        // check the bitmap!
        const RectI downscaledRectToRenderMinimal = downscaledMappedImage->getMinimalRect(downscaledRectToRender);
        
        assert(renderBounds.x1 <= downscaledRectToRenderMinimal.x1 && downscaledRectToRenderMinimal.x2 <= renderBounds.x2 &&
               renderBounds.y1 <= downscaledRectToRenderMinimal.y1 && downscaledRectToRenderMinimal.y2 <= renderBounds.y2);
        
        if (renderFullScaleThenDownscale) {
            RectD canonicalrenderRectToRender;
            downscaledRectToRenderMinimal.toCanonical(mipMapLevel, args._rod, &canonicalrenderRectToRender);
            canonicalrenderRectToRender.toPixelEnclosing(0, &renderRectToRender);
            renderRectToRender.intersect(renderMappedImage->getBounds(), &renderRectToRender);
        } else {
            renderRectToRender = downscaledRectToRenderMinimal;
        }
        
        
        
        RenderArgs argsCpy = args;
        ///Update the renderWindow which might have changed
        argsCpy._renderWindowPixel = renderRectToRender;
        
        scopedArgs.reset( new Implementation::ScopedRenderArgs(&_imp->renderArgs,args) );
        scopedFrameArgs.reset( new Node::ParallelRenderArgsSetter(_node.get(),
                                                           frameArgs.time,
                                                           frameArgs.view,
                                                           frameArgs.isRenderResponseToUserInteraction,
                                                           frameArgs.isSequentialRender,
                                                           frameArgs.byPassCache,
                                                           frameArgs.nodeHash) );
        
    } else {
        renderRectToRender = args._renderWindowPixel;
    }
    
    if ( renderRectToRender.isNull() ) {
        ///We've got nothing to do
        return StatOK;
    }
    
    Natron::Status st = render_public(time, renderMappedScale, renderRectToRender, view,
                                      isSequentialRender,
                                      isRenderResponseToUserInteraction,
                                      renderMappedImage);
    if (st != StatOK) {
        return st;
    }
    
    if ( !aborted() ) {
        renderMappedImage->markForRendered(renderRectToRender);
    }
    
    ///copy the rectangle rendered in the full scale image to the downscaled output
    if (renderFullScaleThenDownscale) {
        ///First demap the fullScaleMappedImage to the original comps/bitdepth if it needs to
        if ( (renderMappedImage == fullScaleMappedImage) && (fullScaleMappedImage != fullScaleImage) ) {
            bool unPremultIfNeeded = getOutputPremultiplication() == ImagePremultiplied;
            renderMappedImage->convertToFormat( renderRectToRender,
                                               getApp()->getDefaultColorSpaceForBitDepth( renderMappedImage->getBitDepth() ),
                                               getApp()->getDefaultColorSpaceForBitDepth( fullScaleMappedImage->getBitDepth() ),
                                               channelForAlpha, false, true,unPremultIfNeeded,
                                               fullScaleImage.get() );
        }
        if (mipMapLevel != 0) {
            assert(fullScaleImage != downscaledImage);
            fullScaleImage->downscaleMipMap( renderRectToRender, 0, mipMapLevel, downscaledImage.get() );
        }
    } else {
        assert(renderMappedImage == downscaledMappedImage);
        if (renderMappedImage != downscaledImage) {
            bool unPremultIfNeeded = getOutputPremultiplication() == ImagePremultiplied;
            renderMappedImage->convertToFormat( renderRectToRender,
                                               getApp()->getDefaultColorSpaceForBitDepth( renderMappedImage->getBitDepth() ),
                                               getApp()->getDefaultColorSpaceForBitDepth( downscaledMappedImage->getBitDepth() ),
                                               channelForAlpha, false, true,unPremultIfNeeded,
                                               downscaledImage.get() );
        }
    }
    

    return StatOK;
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
                         Natron::ValueChangedReason /*reason*/)
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
                        Natron::StandardButton answer =
                        Natron::questionDialog( QObject::tr("Render").toStdString(),
                                               sequentialNode + QObject::tr(" can only "
                                                                            "render in sequential mode. Due to limitations in the "
                                                                            "OpenFX standard that means that %1"
                                                                            " will not be able "
                                                                            "to render all the views of the project. "
                                                                            "Only the main view of the project will be rendered, you can "
                                                                            "change the main view in the project settings. Would you like "
                                                                            "to continue ?").arg(NATRON_APPLICATION_NAME).toStdString() );
                        if (answer != Natron::Yes) {
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
    if (!button) {
        _node->incrementKnobsAge();
    }

    std::list<ViewerInstance* > viewers;
    _node->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        if (isSignificant) {
            (*it)->renderCurrentFrame();
        } else {
            (*it)->redrawViewer();
        }
    }

    getNode()->refreshPreviewsRecursivelyDownstream(getApp()->getTimeLine()->currentFrame());
} // evaluate

bool
EffectInstance::message(Natron::MessageType type,
                        const std::string & content) const
{
    return _node->message(type,content);
}

void
EffectInstance::setPersistentMessage(Natron::MessageType type,
                                     const std::string & content)
{
    _node->setPersistentMessage(type, content);
}

void
EffectInstance::clearPersistentMessage()
{
    _node->clearPersistentMessage();
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
    QMutexLocker l(&_imp->supportsRenderScaleMutex);

    _imp->supportsRenderScale = s;
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

Natron::Status
EffectInstance::render_public(SequenceTime time,
                              const RenderScale & scale,
                              const RectI & roi,
                              int view,
                              bool isSequentialRender,
                              bool isRenderResponseToUserInteraction,
                              boost::shared_ptr<Natron::Image> output)
{
    NON_RECURSIVE_ACTION();

    ///Clear any previous input image which may be left
    _imp->clearInputImagePointers();

    Natron::Status stat;

    try {
        stat = render(time, scale, roi, view, isSequentialRender, isRenderResponseToUserInteraction, output);
    } catch (const std::exception & e) {
        ///Also clear images when catching an exception
        _imp->clearInputImagePointers();
        throw e;
    }

    ///Clear any previous input image which may be left
    _imp->clearInputImagePointers();

    return stat;
}

bool
EffectInstance::isIdentity_public(U64 hash,
                                  SequenceTime time,
                                  const RenderScale & scale,
                                  const RectD& rod,
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
        
        NON_RECURSIVE_ACTION();
        bool ret = false;
        if ( _node->isNodeDisabled() ) {
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
            if (getSequentialPreference() != Natron::EFFECT_ONLY_SEQUENTIAL) {
                try {
                    ret = isIdentity(time, scale,rod, view, inputTime, inputNb);
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

Natron::Status
EffectInstance::getRegionOfDefinition_public(U64 hash,
                                             SequenceTime time,
                                             const RenderScale & scale,
                                             int view,
                                             RectD* rod,
                                             bool* isProjectFormat)
{
    if (!isEffectCreated()) {
        return StatFailed;
    }
    
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    bool foundInCache = _imp->actionsCache.getRoDResult(hash, time, mipMapLevel, rod);
    if (foundInCache) {
        *isProjectFormat = false;
        return Natron::StatOK;
    } else {
        
        
        ///If this is running on a render thread, attempt to find the RoD in the thread local storage.
        if (QThread::currentThread() != qApp->thread() && _imp->renderArgs.hasLocalData()) {
            const RenderArgs& args = _imp->renderArgs.localData();
            if (args._validArgs) {
                *rod = args._rod;
                *isProjectFormat = false;
                return Natron::StatOK;
            }
        }
        
        Natron::Status ret;
        RenderScale scaleOne;
        scaleOne.x = scaleOne.y = 1.;
        {
            NON_RECURSIVE_ACTION();
            ret = getRegionOfDefinition(hash,time, supportsRenderScaleMaybe() == eSupportsNo ? scaleOne : scale, view, rod);
            
            if ( (ret != StatOK) && (ret != StatReplyDefault) ) {
                // rod is not valid
                return ret;
            }
            assert( (ret == StatOK || ret == StatReplyDefault) && (rod->x1 <= rod->x2 && rod->y1 <= rod->y2) );
            
        }
        *isProjectFormat = ifInfiniteApplyHeuristic(hash,time, scale, view, rod);
        assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

        _imp->actionsCache.setRoDResult( time, mipMapLevel, *rod);
        return ret;
    }
}

EffectInstance::RoIMap
EffectInstance::getRegionsOfInterest_public(SequenceTime time,
                                            const RenderScale & scale,
                                            const RectD & outputRoD, //!< effect RoD in canonical coordinates
                                            const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            int view)
{
    NON_RECURSIVE_ACTION();
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);
    EffectInstance::RoIMap ret;
    try {
        ret = getRegionsOfInterest(time, scale, outputRoD, renderWindow, view);
    } catch (const std::exception & e) {
        throw e;
    }

    return ret;
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

Natron::Status
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

Natron::Status
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
                                     Natron::ImageComponents comp) const
{
    return _node->isSupportedComponent(inputNb, comp);
}

Natron::ImageBitDepth
EffectInstance::getBitDepth() const
{
    return _node->getBitDepth();
}

bool
EffectInstance::isSupportedBitDepth(Natron::ImageBitDepth depth) const
{
    return _node->isSupportedBitDepth(depth);
}

Natron::ImageComponents
EffectInstance::findClosestSupportedComponents(int inputNb,
                                               Natron::ImageComponents comp) const
{
    return _node->findClosestSupportedComponents(inputNb,comp);
}

void
EffectInstance::getPreferredDepthAndComponents(int inputNb,
                                               Natron::ImageComponents* comp,
                                               Natron::ImageBitDepth* depth) const
{
    ///find closest to RGBA
    *comp = findClosestSupportedComponents(inputNb, Natron::ImageComponentRGBA);

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
                                   Natron::ValueChangedReason /*reason*/,
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
                                          Natron::ValueChangedReason reason,
                                          SequenceTime time)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );


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
                                                       getHash());

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
EffectInstance::getNearestNonIdentity(int time)
{
    
    U64 hash = getHash();
    RenderScale scale;
    scale.x = scale.y = 1.;
    
    RectD rod;
    bool isProjectFormat;
    Natron::Status stat = getRegionOfDefinition_public(hash, time, scale, 0, &rod, &isProjectFormat);
    
    ///Ignore the result of getRoD if it failed
    (void)stat;
    
    SequenceTime inputTimeIdentity;
    int inputNbIdentity;
    
    if ( !isIdentity_public(hash, time, scale, rod, 0, &inputTimeIdentity, &inputNbIdentity) ) {
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
OutputEffectInstance::renderCurrentFrame(bool abortPrevious)
{
    _engine->renderCurrentFrame(abortPrevious);
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


