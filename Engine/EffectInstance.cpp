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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "EffectInstance.h"

#include <map>
#include <sstream>
#include <algorithm> // min, max
#include <stdexcept>
#include <fstream>

#include <QtConcurrentMap>
#include <QReadWriteLock>
#include <QCoreApplication>
#include <QtConcurrentRun>
#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif
#include <SequenceParsing.h>

#include "Global/MemoryInfo.h"
#include "Global/QtCompat.h"

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
#include "Engine/Timer.h"
#include "Engine/RenderStats.h"

//#define NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS



using namespace Natron;


class File_Knob;
class OutputFile_Knob;



namespace  {
    struct ActionKey {
        double time;
        int view;
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
    
    typedef std::map<ActionKey,IdentityResults,CompareActionsCacheKeys> IdentityCacheMap;
    typedef std::map<ActionKey,RectD,CompareActionsCacheKeys> RoDCacheMap;
    typedef std::map<ActionKey,FramesNeededMap,CompareActionsCacheKeys> FramesNeededCacheMap;
    
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
        
        struct ActionsCacheInstance
        {
            U64 _hash;
            OfxRangeD _timeDomain;
            bool _timeDomainSet;
            
            IdentityCacheMap _identityCache;
            RoDCacheMap _rodCache;
            FramesNeededCacheMap _framesNeededCache;
            
            ActionsCacheInstance()
            : _hash(0)
            , _timeDomain()
            , _timeDomainSet(false)
            , _identityCache()
            , _rodCache()
            , _framesNeededCache()
            {
                
            }

        };
        
        //In  a list to track the LRU
        std::list<ActionsCacheInstance> _instances;
        std::size_t _maxInstances;
        
        std::list<ActionsCacheInstance>::iterator createActionCacheInternal(U64 newHash)
        {
            if (_instances.size() >= _maxInstances) {
                _instances.pop_front();
            }
            ActionsCacheInstance cache;
            cache._hash = newHash;
            return _instances.insert(_instances.end(),cache);
        }
        
        ActionsCacheInstance& getOrCreateActionCache(U64 newHash)
        {
            std::list<ActionsCacheInstance>::iterator found = _instances.end();
            for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it!=_instances.end(); ++it) {
                if (it->_hash == newHash) {
                    found = it;
                    break;
                }
            }
            if (found == _instances.end()) {
                found = createActionCacheInternal(newHash);
            }
            assert(found != _instances.end());
            return *found;
        }
        
    public:
        
        ActionsCache(int maxAvailableHashes)
        : _cacheMutex()
        , _instances()
        , _maxInstances((std::size_t)maxAvailableHashes)
        {
            
        }
        
        void clearAll()
        {
            QMutexLocker l(&_cacheMutex);
            _instances.clear();
        }
        
        
        
        
        void invalidateAll(U64 newHash) {
            QMutexLocker l(&_cacheMutex);
            createActionCacheInternal(newHash);
        }
        
        bool getIdentityResult(U64 hash,double time, int view, unsigned int mipMapLevel,int* inputNbIdentity,double* identityTime) {
            QMutexLocker l(&_cacheMutex);
            for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it!=_instances.end(); ++it) {
                if (it->_hash == hash) {
                    ActionKey key;
                    key.time = time;
                    key.view = view;
                    key.mipMapLevel = mipMapLevel;
                    
                    IdentityCacheMap::const_iterator found = it->_identityCache.find(key);
                    if ( found != it->_identityCache.end() ) {
                        *inputNbIdentity = found->second.inputIdentityNb;
                        *identityTime = found->second.inputIdentityTime;
                        return true;
                    }
                    return false;
                }
            }
            return false;
     
        }
        
        void setIdentityResult(U64 hash,double time, int view, unsigned int mipMapLevel,int inputNbIdentity,double identityTime)
        {
            
            
            QMutexLocker l(&_cacheMutex);
            ActionsCacheInstance& cache = getOrCreateActionCache(hash);
            
            ActionKey key;
            key.time = time;
            key.view = view;
            key.mipMapLevel = mipMapLevel;
            
            IdentityResults& v = cache._identityCache[key];
            v.inputIdentityNb = inputNbIdentity;
            v.inputIdentityTime = identityTime;
            
        }
       
        bool getRoDResult(U64 hash,double time, int view,unsigned int mipMapLevel,RectD* rod) {
            
            QMutexLocker l(&_cacheMutex);
            for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it!=_instances.end(); ++it) {
                if (it->_hash == hash) {
                    ActionKey key;
                    key.time = time;
                    key.view = view;
                    key.mipMapLevel = mipMapLevel;
                    
                    RoDCacheMap::const_iterator found = it->_rodCache.find(key);
                    if ( found != it->_rodCache.end() ) {
                        *rod = found->second;
                        return true;
                    }

                    return false;
                }
            }
            return false;
        }
        
        
        void setRoDResult(U64 hash,double time, int view, unsigned int mipMapLevel,const RectD& rod)
        {
            
            QMutexLocker l(&_cacheMutex);
            ActionsCacheInstance& cache = getOrCreateActionCache(hash);
            
            ActionKey key;
            key.time = time;
            key.view = view;
            key.mipMapLevel = mipMapLevel;
            
            cache._rodCache[key] = rod;

        }
        
        bool getFramesNeededResult(U64 hash,double time, int view,unsigned int mipMapLevel,FramesNeededMap* framesNeeded) {
            
            QMutexLocker l(&_cacheMutex);
            for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it!=_instances.end(); ++it) {
                if (it->_hash == hash) {
                    ActionKey key;
                    key.time = time;
                    key.view = view;
                    key.mipMapLevel = mipMapLevel;
                    
                    FramesNeededCacheMap::const_iterator found = it->_framesNeededCache.find(key);
                    if ( found != it->_framesNeededCache.end() ) {
                        *framesNeeded = found->second;
                        return true;
                    }
                    
                    return false;
                }
            }
            return false;
        }
        
        
        void setFramesNeededResult(U64 hash,double time, int view, unsigned int mipMapLevel,const FramesNeededMap& framesNeeded)
        {
            
            QMutexLocker l(&_cacheMutex);
            
            ActionsCacheInstance& cache = getOrCreateActionCache(hash);
            
            ActionKey key;
            key.time = time;
            key.view = view;
            key.mipMapLevel = mipMapLevel;
            
            cache._framesNeededCache[key] = framesNeeded;

        }
        
        bool getTimeDomainResult(U64 hash,double *first,double* last) {
            
            QMutexLocker l(&_cacheMutex);
            for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it!=_instances.end(); ++it) {
                if (it->_hash == hash && it->_timeDomainSet) {
                    *first = it->_timeDomain.min;
                    *last = it->_timeDomain.max;
                    return true;
                }
            }
            return false;
         
        }
        
        void setTimeDomainResult(U64 hash,double first,double last)
        {
            
            QMutexLocker l(&_cacheMutex);
            ActionsCacheInstance& cache = getOrCreateActionCache(hash);
            cache._timeDomainSet = true;
            cache._timeDomain.min = first;
            cache._timeDomain.max = last;

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
    double _time; //< the time to render
    int _view; //< the view to render
    bool _validArgs; //< are the args valid ?
    bool _isIdentity;
    double _identityTime;
    int _identityInputNb;
    std::map<Natron::ImageComponents,PlaneToRender> _outputPlanes;
    
    //This is set only when the plug-in has set ePassThroughRenderAllRequestedPlanes
    Natron::ImageComponents _outputPlaneBeingRendered;

    double _firstFrame,_lastFrame;
    
    RenderArgs()
    : _rod()
    , _regionOfInterestResults()
    , _renderWindowPixel()
    , _time(0)
    , _view(0)
    , _validArgs(false)
    , _isIdentity(false)
    , _identityTime(0)
    , _identityInputNb(-1)
    , _outputPlanes()
    , _outputPlaneBeingRendered()
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
    , _isIdentity(o._isIdentity)
    , _identityTime(o._identityTime)
    , _identityInputNb(o._identityInputNb)
    , _outputPlanes(o._outputPlanes)
    , _outputPlaneBeingRendered(o._outputPlaneBeingRendered)
    , _firstFrame(o._firstFrame)
    , _lastFrame(o._lastFrame)
    {
    }
};


struct EffectInstance::Implementation
{
    Implementation(EffectInstance* publicInterface)
    : _publicInterface(publicInterface)
    , renderArgs()
    , frameRenderArgs()
    , beginEndRenderCount()
    , inputImages()
    , lastRenderArgsMutex()
    , lastRenderHash(0)
    , lastPlanesRendered()
    , duringInteractActionMutex()
    , duringInteractAction(false)
    , pluginMemoryChunksMutex()
    , pluginMemoryChunks()
    , supportsRenderScale(eSupportsMaybe)
    , actionsCache(appPTR->getHardwareIdealThreadCount() * 2)
#if NATRON_ENABLE_TRIMAP
    , imagesBeingRenderedMutex()
    , imagesBeingRendered()
#endif
    , componentsAvailableMutex()
    , componentsAvailableDirty(true)
    , outputComponentsAvailable()
    {
    }

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
        
        ImageBeingRendered() : cond(), lock(), refCount(0), renderFailed(false) {}
    };
    QMutex imagesBeingRenderedMutex;
    typedef boost::shared_ptr<ImageBeingRendered> IBRPtr;
    typedef std::map<ImagePtr,IBRPtr > IBRMap;
    IBRMap imagesBeingRendered;
#endif
    
    ///A cache for components available
    mutable QMutex componentsAvailableMutex;
    bool componentsAvailableDirty; /// Set to true when getClipPreferences is called to indicate it must be set again
    EffectInstance::ComponentsAvailableMap outputComponentsAvailable;
    
    void runChangedParamCallback(KnobI* k,bool userEdited,const std::string& callback);
    
    void setDuringInteractAction(bool b)
    {
        QWriteLocker l(&duringInteractActionMutex);

        duringInteractAction = b;
    }

#if NATRON_ENABLE_TRIMAP
    void markImageAsBeingRendered(const boost::shared_ptr<Natron::Image>& img)
    {
        if (!img->usesBitMap()) {
            return;
        }
        QMutexLocker k(&imagesBeingRenderedMutex);
        IBRMap::iterator found = imagesBeingRendered.find(img);
        if (found != imagesBeingRendered.end()) {
            ++(found->second->refCount);
        } else {
            IBRPtr ibr(new Implementation::ImageBeingRendered);
            ++ibr->refCount;
            imagesBeingRendered.insert(std::make_pair(img,ibr));
        }
    }
    
    bool waitForImageBeingRenderedElsewhereAndUnmark(const RectI& roi,const boost::shared_ptr<Natron::Image>& img)
    {
        if (!img->usesBitMap()) {
            return true;
        }
        IBRPtr ibr;
        {
            QMutexLocker k(&imagesBeingRenderedMutex);
            IBRMap::iterator found = imagesBeingRendered.find(img);
            assert(found != imagesBeingRendered.end());
            ibr = found->second;
        }
        
        std::list<RectI> restToRender;
        bool isBeingRenderedElseWhere = false;
        img->getRestToRender_trimap(roi,restToRender, &isBeingRenderedElseWhere);
        
        bool ab = _publicInterface->aborted();
        {
            QMutexLocker kk(&ibr->lock);
            while (!ab && isBeingRenderedElseWhere && !ibr->renderFailed && ibr->refCount > 1) {
                ibr->cond.wait(&ibr->lock);
                isBeingRenderedElseWhere = false;
                img->getRestToRender_trimap(roi, restToRender, &isBeingRenderedElseWhere);
                ab = _publicInterface->aborted();
            }
        }
        
        ///Everything should be rendered now.
        bool hasFailed;
        {
            QMutexLocker k(&imagesBeingRenderedMutex);
            IBRMap::iterator found = imagesBeingRendered.find(img);
            assert(found != imagesBeingRendered.end());
            
            QMutexLocker kk(&ibr->lock);
            hasFailed = ab || ibr->renderFailed || isBeingRenderedElseWhere;
            
            // If it crashes here, that is probably because ibr->refCount == 1, but in this case isBeingRenderedElseWhere should be false.
            // If this assert is triggered, please investigate, this is a serious bug in the trimap system.
            assert(ab || !isBeingRenderedElseWhere || ibr->renderFailed);
            --ibr->refCount;
            found->second->cond.wakeAll();
            if (found != imagesBeingRendered.end() && !ibr->refCount) {
                imagesBeingRendered.erase(found);
            }
        }

        return !hasFailed;
        
    
    }
    
    void unmarkImageAsBeingRendered(const boost::shared_ptr<Natron::Image>& img,bool renderFailed)
    {
        if (!img->usesBitMap()) {
            return;
        }
        QMutexLocker k(&imagesBeingRenderedMutex);
        IBRMap::iterator found = imagesBeingRendered.find(img);
        assert(found != imagesBeingRendered.end());
        
        QMutexLocker kk(&found->second->lock);
        if (renderFailed) {
            found->second->renderFailed = true;
        }
        found->second->cond.wakeAll();
        --found->second->refCount;
        if (!found->second->refCount) {
            kk.unlock(); // < unlock before erase which is going to delete the lock
            imagesBeingRendered.erase(found);
        }
        
    }
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
                         const RectI& renderWindow,
                         double time,
                         int view,
                         bool isIdentity,
                         double identityTime,
                         int inputNbIdentity,
                         const std::map<Natron::ImageComponents,PlaneToRender>& outputPlanes,
                         double firstFrame,
                         double lastFrame)
            : localData(&dst->localData())
            , _dst(dst)
        {
            assert(_dst);

            localData->_rod = rod;
            localData->_renderWindowPixel = renderWindow;
            localData->_time = time;
            localData->_view = view;
            localData->_isIdentity = isIdentity;
            localData->_identityTime = identityTime;
            localData->_identityInputNb = inputNbIdentity;
            localData->_outputPlanes = outputPlanes;
            localData->_regionOfInterestResults = roiMap;
            localData->_firstFrame = firstFrame;
            localData->_lastFrame = lastFrame;
            localData->_validArgs = true;

        }
        
        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst)
        : localData(&dst->localData())
        , _dst(dst)
        {
            assert(_dst);
        }

        

        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,
                         const RenderArgs & a)
            : localData(&dst->localData())
              , _dst(dst)
        {
            *localData = a;
            localData->_validArgs = true;
        }

        ~ScopedRenderArgs()
        {
            assert( _dst->hasLocalData() );
            localData->_outputPlanes.clear();
            localData->_validArgs = false;
        }

        RenderArgs& getLocalData() {
            return *localData;
        }
        
        ///Setup the first pass on thread-local storage.
        ///RoIMap and frame range are separated because those actions might need
        ///the thread-storage set up in the first pass to work
        void setArgs_firstPass(const RectD & rod,
                               const RectI& renderWindow,
                               double time,
                               int view,
                               bool isIdentity,
                               double identityTime,
                               int inputNbIdentity)
        {
            localData->_rod = rod;
            localData->_renderWindowPixel = renderWindow;
            localData->_time = time;
            localData->_view = view;
            localData->_isIdentity = isIdentity;
            localData->_identityTime = identityTime;
            localData->_identityInputNb = inputNbIdentity;
            localData->_validArgs = true;
        }
        
        void setArgs_secondPass(const RoIMap & roiMap,
                                int firstFrame,
                                int lastFrame) {
            localData->_regionOfInterestResults = roiMap;
            localData->_firstFrame = firstFrame;
            localData->_lastFrame = lastFrame;
            localData->_validArgs = true;
        }
        

    };
    
    void addInputImageTempPointer(int inputNb,const boost::shared_ptr<Natron::Image> & img)
    {
        InputImagesMap& tls = inputImages.localData();
        tls[inputNb].push_back(img);
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
    ThreadStorage< EffectInstance::InputImagesMap > *storage;
    EffectInstance::InputImagesMap* localData;
public:
    
    InputImagesHolder_RAII(const EffectInstance::InputImagesMap& imgs,ThreadStorage< EffectInstance::InputImagesMap>* storage)
    : storage(storage)
    , localData(0)
    {
        if (!imgs.empty()) {
            localData = &storage->localData();
            localData->insert(imgs.begin(),imgs.end());
        } else {
            this->storage = 0;
        }
        
    }
    
    ~InputImagesHolder_RAII()
    {
        if (storage) {
            assert(localData);
            localData->clear();
        }
    }
};



EffectPointerThreadProperty_RAII::EffectPointerThreadProperty_RAII(EffectInstance* effect)
{
    QThread::currentThread()->setProperty(kNatronTLSEffectPointerProperty, QVariant::fromValue((QObject*)effect));
}

EffectPointerThreadProperty_RAII::~EffectPointerThreadProperty_RAII()
{
    QThread::currentThread()->setProperty(kNatronTLSEffectPointerProperty, QVariant::fromValue((void*)0));
}


void
EffectInstance::addThreadLocalInputImageTempPointer(int inputNb,const boost::shared_ptr<Natron::Image> & img)
{
    _imp->addInputImageTempPointer(inputNb,img);
}

EffectInstance::EffectInstance(boost::shared_ptr<Node> node)
    : NamedKnobHolder(node ? node->getApp() : NULL)
      , _node(node)
      , _imp(new Implementation(this))
{
}

EffectInstance::~EffectInstance()
{
    clearPluginMemoryChunks();
}



void
EffectInstance::resetTotalTimeSpentRendering()
{
    
    resetTimeSpentRenderingInfos();
}


void
EffectInstance::lock(const boost::shared_ptr<Natron::Image>& entry)
{
    boost::shared_ptr<Node> n = _node.lock();
    n->lock(entry);
}


bool
EffectInstance::tryLock(const boost::shared_ptr<Natron::Image>& entry)
{
    boost::shared_ptr<Node> n = _node.lock();
    return n->tryLock(entry);
}

void
EffectInstance::unlock(const boost::shared_ptr<Natron::Image>& entry)
{
    boost::shared_ptr<Node> n = _node.lock();
    n->unlock(entry);
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
EffectInstance::setParallelRenderArgsTLS(int time,
                                         int view,
                                         bool isRenderUserInteraction,
                                         bool isSequential,
                                         bool canAbort,
                                         U64 nodeHash,
                                         U64 rotoAge,
                                         U64 renderAge,
                                         const boost::shared_ptr<Natron::Node>& treeRoot,
                                         const boost::shared_ptr<NodeFrameRequest>& nodeRequest,
                                         int textureIndex,
                                         const TimeLine* timeline,
                                         bool isAnalysis,
                                         bool isDuringPaintStrokeCreation,
                                         const std::list<boost::shared_ptr<Natron::Node> >& rotoPaintNodes,
                                         Natron::RenderSafetyEnum currentThreadSafety,
                                         bool doNanHandling,
                                         bool draftMode,
                                         bool viewerProgressReportEnabled,
                                         const boost::shared_ptr<RenderStats>& stats)
{
    ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
    args.time = time;
    args.timeline = timeline;
    args.view = view;
    args.isRenderResponseToUserInteraction = isRenderUserInteraction;
    args.isSequentialRender = isSequential;
    args.request = nodeRequest;
    if (nodeRequest) {
        args.nodeHash = nodeRequest->nodeHash;
    } else {
        args.nodeHash = nodeHash;
    }
    args.rotoAge = rotoAge;
    args.canAbort = canAbort;
    args.renderAge = renderAge;
    args.treeRoot = treeRoot;
    args.textureIndex = textureIndex;
    args.isAnalysis = isAnalysis;
    args.isDuringPaintStrokeCreation = isDuringPaintStrokeCreation;
    args.currentThreadSafety = currentThreadSafety;
    args.rotoPaintNodes = rotoPaintNodes;
    args.doNansHandling = doNanHandling;
    args.draftMode = draftMode;
    args.tilesSupported = getNode()->getCurrentSupportTiles();
    args.viewerProgressReportEnabled = viewerProgressReportEnabled;
    args.stats = stats;
    ++args.validArgs;
    
}

bool
EffectInstance::getThreadLocalRotoPaintTreeNodes(std::list<boost::shared_ptr<Natron::Node> >* nodes) const
{
    if (!_imp->frameRenderArgs.hasLocalData()) {
        return false;
    }
    const ParallelRenderArgs& tls = _imp->frameRenderArgs.localData();
    if (!tls.validArgs) {
        return false;
    }
    *nodes = tls.rotoPaintNodes;
    return true;
}

void
EffectInstance::setDuringPaintStrokeCreationThreadLocal(bool duringPaintStroke)
{
    ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
    args.isDuringPaintStrokeCreation = duringPaintStroke;
}

void
EffectInstance::setParallelRenderArgsTLS(const ParallelRenderArgs& args)
{
    assert(args.validArgs);
    ParallelRenderArgs& tls = _imp->frameRenderArgs.localData();
    int curValid = tls.validArgs;
    tls = args;
    tls.validArgs = curValid + 1;
}


void
EffectInstance::invalidateParallelRenderArgsTLS()
{
    if (_imp->frameRenderArgs.hasLocalData()) {
        ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        --args.validArgs;
        if (args.validArgs < 0) {
            args.validArgs = 0;
        }
        
        for (NodeList::iterator it = args.rotoPaintNodes.begin(); it!=args.rotoPaintNodes.end(); ++it) {
            (*it)->getLiveInstance()->invalidateParallelRenderArgsTLS();
        }
    } else {
        //qDebug() << "Frame render args thread storage not set, this is probably because the graph changed while rendering.";
    }
}

const ParallelRenderArgs*
EffectInstance::getParallelRenderArgsTLS() const
{
    if (_imp->frameRenderArgs.hasLocalData()) {
        return &_imp->frameRenderArgs.localData();
    } else {
        //qDebug() << "Frame render args thread storage not set, this is probably because the graph changed while rendering.";
        return 0;
    }
}

bool
EffectInstance::isCurrentRenderInAnalysis() const
{
    if (_imp->frameRenderArgs.hasLocalData()) {
        const ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        return args.validArgs && args.isAnalysis;
    }
    return false;
    
}

U64
EffectInstance::getHash() const
{
    boost::shared_ptr<Node> n = _node.lock();
    return n->getHashValue();
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
            if (args.request) {
                return args.request->nodeHash;
            }
            return args.nodeHash;
        }
    }
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
                
                //Don't allow the plug-in to abort while in analysis.
                //Need to work on this for the tracker in order to be abortable.
                if (args.isAnalysis) {
                    return false;
                }
                ViewerInstance* isViewer  = 0;
                if (args.treeRoot) {
                    
                    isViewer = dynamic_cast<ViewerInstance*>(args.treeRoot->getLiveInstance());
                    //If the viewer is already doing a sequential render, abort
                    if (isViewer && isViewer->isDoingSequentialRender()) {
                        return true;
                    }
                }
                
                if (args.canAbort) {
                    
                    if (isViewer && isViewer->isRenderAbortable(args.textureIndex, args.renderAge)) {
                        return true;
                    }
                    
                    ///Rendering issued by RenderEngine::renderCurrentFrame, if time or hash changed, abort
                    bool ret = !getNode()->isActivated();
                    return ret;
                } else {
                    
                    bool deactivated = !getNode()->isActivated();
                    return deactivated;
                }
                
            } else {
                ///Rendering is playback or render on disk, we rely on the flag set on the node that requested the render
                if (args.treeRoot) {
                    Natron::OutputEffectInstance* effect = dynamic_cast<Natron::OutputEffectInstance*>(args.treeRoot->getLiveInstance());
                    assert(effect);
                    return effect->isSequentialRenderBeingAborted();
                } else {
                    return false;
                }
          
            }
        }

    }
}

bool
EffectInstance::shouldCacheOutput(bool isFrameVaryingOrAnimated) const
{
    boost::shared_ptr<Node> n = _node.lock();
    return n->shouldCacheOutput(isFrameVaryingOrAnimated);
}


U64
EffectInstance::getKnobsAge() const
{
    return getNode()->getKnobsAge();
}

void
EffectInstance::setKnobsAge(U64 age)
{
    getNode()->setKnobsAge(age);
}

const std::string &
EffectInstance::getScriptName() const
{
    return getNode()->getScriptName();
}

std::string
EffectInstance::getScriptName_mt_safe() const
{
    return getNode()->getScriptName_mt_safe();
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
    return getNode()->hasOutputConnected();
}

EffectInstance*
EffectInstance::getInput(int n) const
{
    boost::shared_ptr<Natron::Node> inputNode = getNode()->getInput(n);

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
EffectInstance::retrieveGetImageDataUponFailure(const double time,
                                                const int view,
                                                const RenderScale& scale,
                                                const RectD* optionalBoundsParam,
                                                U64* nodeHash_p,
                                                U64* rotoAge_p,
                                                bool* isIdentity_p,
                                                double* identityTime,
                                                int* identityInputNb_p,
                                                bool* duringPaintStroke_p,
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
//        qDebug() << getNode()->getScriptName_mt_safe().c_str() << " is trying to call clipGetImage during an unauthorized time. "
//        "Developers of that plug-in should fix it. \n Reminder from the OpenFX spec: \n "
//        "Images may be fetched from an attached clip in the following situations... \n"
//        "- in the kOfxImageEffectActionRender action\n"
//        "- in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason or kOfxChangeUserEdited";
//    }
//#endif
    
    ///Try to compensate for the mistake
    
    *nodeHash_p = getHash();
    *rotoAge_p = getNode()->getRotoAge();
    *duringPaintStroke_p = getNode()->isDuringPaintStrokeCreation();
    const U64& nodeHash = *nodeHash_p;

    {
        RECURSIVE_ACTION();
        Natron::StatusEnum stat = getRegionOfDefinition(nodeHash, time, scale, view, rod_p);
        if (stat == eStatusFailed) {
            return false;
        }
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
    RectI pixelRod;
    rod.toPixelEnclosing(scale, getPreferredAspectRatio(), &pixelRod);
    try {
        *isIdentity_p = isIdentity_public(true, nodeHash,time, scale, pixelRod, view, identityTime, identityInputNb_p);
    } catch (...) {
        return false;
    }


    return true;
}

void
EffectInstance::getThreadLocalInputImages(EffectInstance::InputImagesMap* images) const
{
    if (_imp->inputImages.hasLocalData()) {
        *images = _imp->inputImages.localData();
    }
}

bool
EffectInstance::getThreadLocalRegionsOfInterests(RoIMap& roiMap) const
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


ImagePtr
EffectInstance::getImage(int inputNb,
                         const double time,
                         const RenderScale & scale,
                         const int view,
                         const RectD *optionalBoundsParam, //!< optional region in canonical coordinates
                         const ImageComponents& comp,
                         const Natron::ImageBitDepthEnum depth,
                         const double par,
                         const bool dontUpscale,
                         RectI* roiPixel)
{
   
    ///The input we want the image from
    EffectInstance* n = getInput(inputNb);
    
    ///Is this input a mask or not
    bool isMask = isInputMask(inputNb);
    
    ///If the input is a mask, this is the channel index in the layer of the mask channel
    int channelForMask = -1;
    
    ///Is this node a roto node or not. If so, find out if this input is the roto-brush
    boost::shared_ptr<RotoContext> roto;
    boost::shared_ptr<RotoDrawableItem> attachedStroke = getNode()->getAttachedRotoItem();
    if (attachedStroke) {
        roto = attachedStroke->getContext();
    } else {
        roto = getNode()->getRotoContext();
    }
    bool useRotoInput = false;
    if (roto) {
        useRotoInput = isMask || isInputRotoBrush(inputNb);
    }
    
    ///This is the actual layer that we are fetching in input, note that it is different than "comp" which is pointing to Alpha
    ImageComponents maskComps;
    //if (isMask) {
        if (!isMaskEnabled(inputNb)) {
            ///This is last resort, the plug-in should've checked getConnected() before, which would have returned false.
            return ImagePtr();
        }
        NodePtr maskInput;
        channelForMask = getMaskChannel(inputNb,&maskComps,&maskInput);
        if (maskInput && channelForMask != -1) {
            n = maskInput->getLiveInstance();
        }
        
        //Invalid mask
        if (isMask && (channelForMask == -1 || maskComps.getNumComponents() == 0)) {
            return ImagePtr();
        }
    //}
    
    
 
    
    if ((!roto || (roto && !useRotoInput)) && !n) {
        //Disconnected input
        return ImagePtr();
    }
    
    std::list<ImageComponents> outputClipPrefComps;
    ImageBitDepthEnum outputDepth;
    getPreferredDepthAndComponents(inputNb, &outputClipPrefComps, &outputDepth);
    assert(outputClipPrefComps.size() >= 1);
    const ImageComponents& prefComps = outputClipPrefComps.front();
    
    ///If optionalBounds have been set, use this for the RoI instead of the data int the TLS
    RectD optionalBounds;
    if (optionalBoundsParam) {
        optionalBounds = *optionalBoundsParam;
    }
    
    /*
     * These are the data fields stored in the TLS from the on-going render action or instance changed action
     */
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    RoIMap inputsRoI;
    RectD rod;
    bool isIdentity;
    int inputNbIdentity;
    double inputIdentityTime;
    U64 nodeHash;
    U64 rotoAge;
    bool duringPaintStroke;
    /// Never by-pass the cache here because we already computed the image in renderRoI and by-passing the cache again can lead to
    /// re-computing of the same image many many times
    bool byPassCache = false;
    
    ///The caller thread MUST be a thread owned by Natron. It cannot be a thread from the multi-thread suite.
    ///A call to getImage is forbidden outside an action running in a thread launched by Natron.
    
    /// From http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsImagesAndClipsUsingClips
    //    Images may be fetched from an attached clip in the following situations...
    //    in the kOfxImageEffectActionRender action
    //    in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason of kOfxChangeUserEdited
    RectD roi;
    bool roiWasInRequestPass = false;
    
    if (!_imp->renderArgs.hasLocalData() || !_imp->frameRenderArgs.hasLocalData()) {
        
        if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &rotoAge, &isIdentity, &inputIdentityTime, &inputNbIdentity, &duringPaintStroke, &rod, &inputsRoI, &optionalBounds) ) {
            return ImagePtr();
        }
        
    } else {
        
        RenderArgs& renderArgs = _imp->renderArgs.localData();
        ParallelRenderArgs& frameRenderArgs = _imp->frameRenderArgs.localData();
        
        if (!renderArgs._validArgs || !frameRenderArgs.validArgs) {
            if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &rotoAge, &isIdentity, &inputIdentityTime, &inputNbIdentity, &duringPaintStroke, &rod, &inputsRoI, &optionalBounds) ) {
                return ImagePtr();
            }
            
        } else {
            if (n) {
                const ParallelRenderArgs* inputFrameArgs = n->getParallelRenderArgsTLS();
                const FrameViewRequest* request = 0;
                if (inputFrameArgs && inputFrameArgs->request) {
                    request = inputFrameArgs->request->getFrameViewRequest(time, view);
                }
                if (request) {
                    roiWasInRequestPass = true;
                    roi = request->finalData.finalRoi;
                }
            }
            if (!roiWasInRequestPass) {
                inputsRoI = renderArgs._regionOfInterestResults;
            }
            rod = renderArgs._rod;
            isIdentity = renderArgs._isIdentity;
            inputIdentityTime = renderArgs._identityTime;
            inputNbIdentity = renderArgs._identityInputNb;
            nodeHash = frameRenderArgs.nodeHash;
            rotoAge = frameRenderArgs.rotoAge;
            duringPaintStroke = frameRenderArgs.isDuringPaintStrokeCreation;
        }
        
        
    }
    
    if (optionalBoundsParam) {
        roi = optionalBounds;
    } else if (!roiWasInRequestPass) {
        EffectInstance* inputToFind = 0;
        if (useRotoInput) {
            if (getNode()->getRotoContext()) {
                inputToFind = this; 
            } else {
                assert(attachedStroke);
                inputToFind = attachedStroke->getContext()->getNode()->getLiveInstance();
            }
        } else {
            inputToFind = n;
        }
        RoIMap::iterator found = inputsRoI.find(inputToFind);
        if ( found != inputsRoI.end() ) {
            ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
            roi = found->second;
        } else {
            ///Oops, we didn't find the roi in the thread-storage... use  the RoD instead...
            roi = rod;
        }
    }
    
    if (roi.isNull()) {
        return ImagePtr();
    }
    
    
    if (isIdentity) {
        assert(inputNbIdentity != -2);
        ///If the effect is an identity but it didn't ask for the effect's image of which it is identity
        ///return a null image
        if (inputNbIdentity != inputNb) {
            ImagePtr();
        }
        
    }
    
    
    ///Does this node supports images at a scale different than 1
    bool renderFullScaleThenDownscale = (!supportsRenderScale() && mipMapLevel != 0);
    
    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    
    unsigned int renderMappedMipMapLevel = mipMapLevel;
    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
        if (renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            renderMappedMipMapLevel = 0;
        }
    }
    
    ///Both the result of getRegionsOfInterest and optionalBounds are in canonical coordinates, we have to convert in both cases
    ///Convert to pixel coordinates
    RectI pixelRoI;
    roi.toPixelEnclosing(renderScaleOneUpstreamIfRenderScaleSupportDisabled ? 0 : mipMapLevel, par, &pixelRoI);
    
    ///Try to find in the input images thread local storage if we already pre-computed the image
    InputImagesMap inputImagesThreadLocal;
    if (_imp->inputImages.hasLocalData()) {
        inputImagesThreadLocal = _imp->inputImages.localData();
    }
    
    ImagePtr inputImg;
    
    ///For the roto brush, we do things separatly and render the mask with the RotoContext.
    if (useRotoInput) {
        
        
        ///Usage of roto outside of the rotopaint node is no longer handled
        assert(attachedStroke);
        if (attachedStroke) {
            if (duringPaintStroke) {
                inputImg = getNode()->getOrRenderLastStrokeImage(mipMapLevel, pixelRoI, par, prefComps, depth);
            } else {
                inputImg = roto->renderMaskFromStroke(attachedStroke, pixelRoI, rotoAge, nodeHash, prefComps,
                                                      time, view, depth, mipMapLevel);
                if (roto->isDoingNeatRender()) {
                    getNode()->updateStrokeImage(inputImg);
                }
            }
        }
        if (roiPixel) {
            *roiPixel = pixelRoI;
        }
        
        if (!pixelRoI.intersects(inputImg->getBounds())) {
            //The RoI requested does not intersect with the bounds of the input image, return a NULL image.
#ifdef DEBUG
            qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": The RoI requested to the roto mask does not intersect with the bounds of the input image";
#endif
            return ImagePtr();
        }
        
        if (inputImagesThreadLocal.empty()) {
            ///If the effect is analysis (e.g: Tracker) there's no input images in the tread local storage, hence add it
            _imp->addInputImageTempPointer(inputNb,inputImg);
        }
        return inputImg;
    }
    
    
    /// The node is connected.
    assert(n);
    
    std::list<ImageComponents> requestedComps;
    requestedComps.push_back(isMask ? maskComps : comp);
    ImageList inputImages;
    RenderRoIRetCode retCode = n->renderRoI(RenderRoIArgs(time,
                                                          scale,
                                                          renderMappedMipMapLevel,
                                                          view,
                                                          byPassCache,
                                                          pixelRoI,
                                                          RectD(),
                                                          requestedComps,
                                                          depth,
                                                          this,
                                                          inputImagesThreadLocal), &inputImages);
    
    if (inputImages.empty() || retCode != eRenderRoIRetCodeOk) {
        return ImagePtr();
    }
    assert(inputImages.size() == 1);
    
    inputImg = inputImages.front();
    
    if (!pixelRoI.intersects(inputImg->getBounds())) {
        //The RoI requested does not intersect with the bounds of the input image, return a NULL image.
#ifdef DEBUG
        qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": The RoI requested to" << n->getScriptName_mt_safe().c_str() << "does not intersect with the bounds of the input image";
#endif
        return ImagePtr();
    }
    
    /*
     * From now on this is the generic part. We first call renderRoI and then convert to the appropriate scale/components if needed.
     * Note that since the image has been pre-rendered before by the recursive nature of the algorithm, the call to renderRoI will be
     * instantaneous thanks to the image cache.
     */
    
    ///Check that the rendered image contains what we requested.
    assert((!isMask && inputImg->getComponents() == comp) || (isMask && inputImg->getComponents() == maskComps));
    
    if (roiPixel) {
        *roiPixel = pixelRoI;
    }
    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();
    
    if (std::abs(inputImg->getPixelAspectRatio() - par) > 1e-6) {
        qDebug() << "WARNING: " << getScriptName_mt_safe().c_str() << " requested an image with a pixel aspect ratio of " << par <<
        " but " << n->getScriptName_mt_safe().c_str() << " rendered an image with a pixel aspect ratio of " << inputImg->getPixelAspectRatio();
    }
    
    ///If the plug-in doesn't support the render scale, but the image is downscaled, up-scale it.
    ///Note that we do NOT cache it because it is really low def!
    if ( !dontUpscale  && renderFullScaleThenDownscale && inputImgMipMapLevel != 0 ) {
        assert(inputImgMipMapLevel != 0);
        ///Resize the image according to the requested scale
        Natron::ImageBitDepthEnum bitdepth = inputImg->getBitDepth();
        RectI bounds;
        inputImg->getRoD().toPixelEnclosing(0, par, &bounds);
        ImagePtr rescaledImg( new Natron::Image(inputImg->getComponents(), inputImg->getRoD(),
                                                                        bounds, 0, par, bitdepth) );
        inputImg->upscaleMipMap(inputImg->getBounds(), inputImgMipMapLevel, 0, rescaledImg.get());
        if (roiPixel) {
            RectD canonicalPixelRoI;
            pixelRoI.toCanonical(inputImgMipMapLevel, par, rod, &canonicalPixelRoI);
            canonicalPixelRoI.toPixelEnclosing(0, par, roiPixel);
        }
        
        inputImg = rescaledImg;
        
    }
    

    
    if (prefComps.getNumComponents() != inputImg->getComponents().getNumComponents()) {
        ImagePtr remappedImg;
        {
            Image::ReadAccess acc = inputImg->getReadRights();
            
            remappedImg.reset( new Image(prefComps, inputImg->getRoD(), inputImg->getBounds(), inputImg->getMipMapLevel(),inputImg->getPixelAspectRatio(), inputImg->getBitDepth(), false) );
            
            Natron::ViewerColorSpaceEnum colorspace = getApp()->getDefaultColorSpaceForBitDepth(inputImg->getBitDepth());
            
            bool unPremultIfNeeded = getOutputPremultiplication() == eImagePremultiplicationPremultiplied &&
            inputImg->getComponents().getNumComponents() == 4 && prefComps.getNumComponents() == 3;
            inputImg->convertToFormat(inputImg->getBounds(),
                                      colorspace, colorspace,
                                      channelForMask, false, unPremultIfNeeded, remappedImg.get());
        }
        inputImg = remappedImg;
    }

    
    
    if (inputImagesThreadLocal.empty()) {
        ///If the effect is analysis (e.g: Tracker) there's no input images in the tread local storage, hence add it
        _imp->addInputImageTempPointer(inputNb,inputImg);
    }
    return inputImg;
    
    
    
} // getImage

void
EffectInstance::calcDefaultRegionOfDefinition(U64 /*hash*/,double /*time*/,
                                              int /*view*/,
                                              const RenderScale & /*scale*/,
                                              RectD *rod)
{
    Format projectDefault;

    getRenderFormat(&projectDefault);
    *rod = RectD( projectDefault.left(), projectDefault.bottom(), projectDefault.right(), projectDefault.top() );
}

Natron::StatusEnum
EffectInstance::getRegionOfDefinition(U64 hash,double time,
                                      const RenderScale & scale,
                                      int view,
                                      RectD* rod) //!< rod is in canonical coordinates
{
    bool firstInput = true;
    RenderScale renderMappedScale = scale;

    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    for (int i = 0; i < getMaxInputCount(); ++i) {
        if (isInputMask(i)) {
            continue;
        }
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

    // if rod was not set, return default, else return OK
    return firstInput ? eStatusReplyDefault : eStatusOK;
}

bool
EffectInstance::ifInfiniteApplyHeuristic(U64 hash,
                                         double time,
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
EffectInstance::getRegionsOfInterest(double /*time*/,
                                     const RenderScale & /*scale*/,
                                     const RectD & /*outputRoD*/, //!< the RoD of the effect, in canonical coordinates
                                     const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                     int /*view*/,
                                     RoIMap* ret)
{
    for (int i = 0; i < getMaxInputCount(); ++i) {
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            ret->insert( std::make_pair(input, renderWindow) );
        }
    }
}

FramesNeededMap
EffectInstance::getFramesNeeded(double time, int view)
{
    FramesNeededMap ret;
    RangeD defaultRange;
    
    defaultRange.min = defaultRange.max = time;
    std::vector<RangeD> ranges;
    ranges.push_back(defaultRange);
    std::map<int,std::vector<RangeD> > defViewRange;
    defViewRange.insert(std::make_pair(view, ranges));
    for (int i = 0; i < getMaxInputCount(); ++i) {
        if (isInputRotoBrush(i)) {
            ret.insert( std::make_pair(i, defViewRange) );
        } else {
            Natron::EffectInstance* input = getInput(i);
            if (input) {
                ret.insert( std::make_pair(i, defViewRange) );
            }
        }
    }

    return ret;
}

void
EffectInstance::getFrameRange(double *first,
                              double *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    for (int i = 0; i < getMaxInputCount(); ++i) {
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            double inpFirst,inpLast;
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

static void getOrCreateFromCacheInternal(const ImageKey& key,
                                         const boost::shared_ptr<ImageParams>& params,
                                         bool useCache,
                                         bool useDiskCache,
                                         ImagePtr* image)
{
    
    
    if (useCache) {
        !useDiskCache ? Natron::getImageFromCacheOrCreate(key, params, image) :
        Natron::getImageFromDiskCacheOrCreate(key, params, image);
        
        if (!*image) {
            std::stringstream ss;
            ss << "Failed to allocate an image of ";
            ss << printAsRAM( params->getElementsCount() * sizeof(Image::data_t) ).toStdString();
            Natron::errorDialog( QObject::tr("Out of memory").toStdString(),ss.str() );
            return;
        }
    
        /*
         * Note that at this point the image is already exposed to other threads and another one might already have allocated it.
         * This function does nothing if it has been reallocated already.
         */
        (*image)->allocateMemory();
        
        
        
        /*
         * Another thread might have allocated the same image in the cache but with another RoI, make sure
         * it is big enough for us, or resize it to our needs.
         */
        

        (*image)->ensureBounds(params->getBounds());
        
        
    } else {
        image->reset(new Image(key, params));
    }
}

void
EffectInstance::getImageFromCacheAndConvertIfNeeded(bool useCache,
                                                    bool useDiskCache,
                                                    const Natron::ImageKey& key,
                                                    unsigned int mipMapLevel,
                                                    const RectI* boundsParam,
                                                    const RectD* rodParam,
                                                    Natron::ImageBitDepthEnum bitdepth,
                                                    const Natron::ImageComponents& components,
                                                    Natron::ImageBitDepthEnum nodePrefDepth,
                                                    const Natron::ImageComponents& nodePrefComps,
                                                    const EffectInstance::InputImagesMap& inputImages,
                                                    const boost::shared_ptr<RenderStats>& stats,
                                                    boost::shared_ptr<Natron::Image>* image)
{
    ImageList cachedImages;
    bool isCached = false;
    
    ///Find first something in the input images list
    if (!inputImages.empty()) {
        for (EffectInstance::InputImagesMap::const_iterator it = inputImages.begin(); it != inputImages.end(); ++it) {
            for (ImageList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                if (!it2->get()) {
                    continue;
                }
                const ImageKey& imgKey = (*it2)->getKey();
                if (imgKey == key) {
                    cachedImages.push_back(*it2);
                    isCached = true;
                }

            }
        }
    }
    
    if (!isCached) {
        isCached = !useDiskCache ? Natron::getImageFromCache(key,&cachedImages) : Natron::getImageFromDiskCache(key, &cachedImages);
    }
    
    if (stats && stats->isInDepthProfilingEnabled() && !isCached) {
        stats->addCacheInfosForNode(getNode(), true, false);
    }
    
    if (isCached) {
        
        ///A ptr to a higher resolution of the image or an image with different comps/bitdepth
        ImagePtr imageToConvert;
        
        for (ImageList::iterator it = cachedImages.begin(); it != cachedImages.end(); ++it) {
            unsigned int imgMMlevel = (*it)->getMipMapLevel();
            const Natron::ImageComponents& imgComps = (*it)->getComponents();
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
            if ((imgComps.isColorPlane() && nodePrefComps.isColorPlane() && imgComps != nodePrefComps) || imgDepth != nodePrefDepth) {
                appPTR->removeFromNodeCache(*it);
                continue;
            }
            
            bool convertible = imgComps.isConvertibleTo(components);
            if (imgMMlevel == mipMapLevel && convertible &&
            getSizeOfForBitDepth(imgDepth) >= getSizeOfForBitDepth(bitdepth)/* && imgComps == components && imgDepth == bitdepth*/) {
                
                ///We found  a matching image
                
                *image = *it;
                break;
            } else {
                
                
                if (imgMMlevel >= mipMapLevel || !convertible ||
                    getSizeOfForBitDepth(imgDepth) < getSizeOfForBitDepth(bitdepth)) {
                    ///Either smaller resolution or not enough components or bit-depth is not as deep, don't use the image
                    continue;
                }
                
                assert(imgMMlevel < mipMapLevel);
                
                if (!imageToConvert) {
                    imageToConvert = *it;

                } else {
                    ///We found an image which scale is closer to the requested mipmap level we want, use it instead
                    if (imgMMlevel > imageToConvert->getMipMapLevel()) {
                        imageToConvert = *it;
                    }
                }
                
                
            }
        } //end for
        
        if (imageToConvert && !*image) {

            ///Ensure the image is allocated
            (imageToConvert)->allocateMemory();

            
            if (imageToConvert->getMipMapLevel() != mipMapLevel) {
                boost::shared_ptr<ImageParams> oldParams = imageToConvert->getParams();
                
                assert(imageToConvert->getMipMapLevel() < mipMapLevel);
                
                RectI imgToConvertBounds = imageToConvert->getBounds();
                
                const RectD& rod = rodParam ? *rodParam : oldParams->getRoD();
                
                RectD imgToConvertCanonical;
                imgToConvertBounds.toCanonical(imageToConvert->getMipMapLevel(), imageToConvert->getPixelAspectRatio(), rod, &imgToConvertCanonical);
                RectI downscaledBounds;
                
                imgToConvertCanonical.toPixelEnclosing(imageToConvert->getMipMapLevel(), imageToConvert->getPixelAspectRatio(), &imgToConvertBounds);
                imgToConvertCanonical.toPixelEnclosing(mipMapLevel, imageToConvert->getPixelAspectRatio(), &downscaledBounds);
                
                if (boundsParam) {
                    downscaledBounds.merge(*boundsParam);
                }
                
                RectI pixelRoD;
                rod.toPixelEnclosing(mipMapLevel, oldParams->getPixelAspectRatio(), &pixelRoD);
                downscaledBounds.intersect(pixelRoD, &downscaledBounds);
                
                boost::shared_ptr<ImageParams> imageParams = Image::makeParams(oldParams->getCost(),
                                                                               rod,
                                                                               downscaledBounds,
                                                                               oldParams->getPixelAspectRatio(),
                                                                               mipMapLevel,
                                                                               oldParams->isRodProjectFormat(),
                                                                               oldParams->getComponents(),
                                                                               oldParams->getBitDepth(),
                                                                               oldParams->getFramesNeeded());
                
                
                imageParams->setMipMapLevel(mipMapLevel);
                
                
                boost::shared_ptr<Image> img;
                getOrCreateFromCacheInternal(key,imageParams,useCache,useDiskCache,&img);
                if (!img) {
                    return;
                }
                
                if (imgToConvertBounds.area() > 1) {
                    imageToConvert->downscaleMipMap(rod,
                                                    imgToConvertBounds,
                                                    imageToConvert->getMipMapLevel(), img->getMipMapLevel() ,
                                                    useCache && imageToConvert->usesBitMap(),
                                                    img.get());
                } else {
                    img->pasteFrom(*imageToConvert, imgToConvertBounds);
                }
                
                imageToConvert = img;

            }
            
            *image = imageToConvert;
            //assert(imageToConvert->getBounds().contains(bounds));
            
            if (stats && stats->isInDepthProfilingEnabled()) {
                stats->addCacheInfosForNode(getNode(), false, true);
            }
            
        } else if (*image) { //  else if (imageToConvert && !*image)
            
            ///Ensure the image is allocated
            (*image)->allocateMemory();
            
            if (stats && stats->isInDepthProfilingEnabled()) {
                stats->addCacheInfosForNode(getNode(), false, false);
            }

        } else {
            if (stats && stats->isInDepthProfilingEnabled()) {
                stats->addCacheInfosForNode(getNode(), true, false);
            }
        }
        
    } // isCached

}

void
EffectInstance::tryConcatenateTransforms(double time,
                                         int view,
                                         const RenderScale& scale,
                                         InputMatrixMap* inputTransforms)
{
    
    bool canTransform = getCanTransform();
    
    //An effect might not be able to concatenate transforms but can still apply a transform (e.g CornerPinMasked)
    std::list<int> inputHoldingTransforms;
    bool canApplyTransform = getInputsHoldingTransform(&inputHoldingTransforms);
    assert(inputHoldingTransforms.empty() || canApplyTransform);
    
    Transform::Matrix3x3 thisNodeTransform;
    Natron::EffectInstance* inputToTransform = 0;
    
    bool getTransformSucceeded = false;
    
    if (canTransform) {
        /*
         * If getting the transform does not succeed, then this effect is treated as any other ones.
         */
        Natron::StatusEnum stat = getTransform_public(time, scale, view, &inputToTransform, &thisNodeTransform);
        if (stat == eStatusOK) {
            getTransformSucceeded = true;
        }
    }

    
    if ((canTransform && getTransformSucceeded) || (!canTransform && canApplyTransform && !inputHoldingTransforms.empty())) {
        
        for (std::list<int>::iterator it = inputHoldingTransforms.begin(); it != inputHoldingTransforms.end(); ++it) {
            
            EffectInstance* input = getInput(*it);
            if (!input) {
                continue;
            }
            std::list<Transform::Matrix3x3> matricesByOrder; // from downstream to upstream

            InputMatrix im;
            im.newInputEffect = input;
            im.newInputNbToFetchFrom = *it;

            
            // recursion upstream
            bool inputCanTransform = false;
            bool inputIsDisabled  =  input->getNode()->isNodeDisabled();
            
            if (!inputIsDisabled) {
                inputCanTransform = input->getCanTransform();
            }
            
            
            while (input && (inputCanTransform || inputIsDisabled)) {
                
                //input is either disabled, or identity or can concatenate a transform too
                if (inputIsDisabled) {
                    
                    int prefInput;
                    input = input->getNearestNonDisabled();
                    prefInput = input ? input->getNode()->getPreferredInput() : -1;
                    if (prefInput == -1) {
                        break;
                    }
                                        
                    if (input) {
                        im.newInputNbToFetchFrom = prefInput;
                        im.newInputEffect = input;
                    }
                    
                } else if (inputCanTransform) {
                    Transform::Matrix3x3 m;
                    inputToTransform = 0;
                    Natron::StatusEnum stat = input->getTransform_public(time, scale, view, &inputToTransform, &m);
                    if (stat == eStatusOK) {
                        matricesByOrder.push_back(m);
                        if (inputToTransform) {
                            im.newInputNbToFetchFrom = input->getInputNumber(inputToTransform);
                            im.newInputEffect = input;
                            input = inputToTransform;
                        }
                    } else {
                        break;
                    }
                } else {
                    assert(false);
                }
                
                if (input) {
                    inputIsDisabled = input->getNode()->isNodeDisabled();
                    if (!inputIsDisabled) {
                        inputCanTransform = input->getCanTransform();
                    }
                }
            }
            
            if (input && !matricesByOrder.empty()) {
                assert(im.newInputEffect);
                
                ///Now actually concatenate matrices together
                im.cat.reset(new Transform::Matrix3x3);
                std::list<Transform::Matrix3x3>::iterator it2 = matricesByOrder.begin();
                *im.cat = *it2;
                ++it2;
                while (it2 != matricesByOrder.end()) {
                    *im.cat = Transform::matMul(*im.cat, *it2);
                    ++it2;
                }
                
                inputTransforms->insert(std::make_pair(*it,im));
            }
            
        } //  for (std::list<int>::iterator it = inputHoldingTransforms.begin(); it != inputHoldingTransforms.end(); ++it)

    } // if ((canTransform && getTransformSucceeded) || (canApplyTransform && !inputHoldingTransforms.empty()))

}

class TransformReroute_RAII
{
    EffectInstance* self;
    InputMatrixMap transforms;
public:
    
    TransformReroute_RAII(EffectInstance* self,const InputMatrixMap& inputTransforms)
    : self(self)
    , transforms(inputTransforms)
    {
        self->rerouteInputAndSetTransform(inputTransforms);
    }
    
    ~TransformReroute_RAII()
    {
        for (InputMatrixMap::iterator it = transforms.begin(); it != transforms.end(); ++it) {
            self->clearTransform(it->first);
        }
        
    }
};


bool
EffectInstance::allocateImagePlane(const ImageKey& key,
                                   const RectD& rod,
                                   const RectI& downscaleImageBounds,
                                   const RectI& fullScaleImageBounds,
                                   bool isProjectFormat,
                                   const FramesNeededMap& framesNeeded,
                                   const Natron::ImageComponents& components,
                                   Natron::ImageBitDepthEnum depth,
                                   double par,
                                   unsigned int mipmapLevel,
                                   bool renderFullScaleThenDownscale,
                                   bool renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                   bool useDiskCache,
                                   bool createInCache,
                                   boost::shared_ptr<Natron::Image>* fullScaleImage,
                                   boost::shared_ptr<Natron::Image>* downscaleImage)
{
    //Controls whether images are stored on disk or in RAM, 0 = RAM, 1 = mmap
    int cost = useDiskCache ? 1 : 0;
    
    //If we're rendering full scale and with input images at full scale, don't cache the downscale image since it is cheap to
    //recreate, instead cache the full-scale image
    if (renderFullScaleThenDownscale && renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
        
        downscaleImage->reset( new Natron::Image(components, rod, downscaleImageBounds, mipmapLevel, par, depth, true) );
        
    } else {
        
        ///Cache the image with the requested components instead of the remapped ones
        boost::shared_ptr<Natron::ImageParams> cachedImgParams = Natron::Image::makeParams(cost,
                                                                                           rod,
                                                                                           downscaleImageBounds,
                                                                                           par,
                                                                                           mipmapLevel,
                                                                                           isProjectFormat,
                                                                                           components,
                                                                                           depth,
                                                                                           framesNeeded);
        
        //Take the lock after getting the image from the cache or while allocating it
        ///to make sure a thread will not attempt to write to the image while its being allocated.
        ///When calling allocateMemory() on the image, the cache already has the lock since it added it
        ///so taking this lock now ensures the image will be allocated completetly
        
        getOrCreateFromCacheInternal(key,cachedImgParams,createInCache,useDiskCache,fullScaleImage);
        if (!*fullScaleImage) {
            return false;
        }
        
        
        *downscaleImage = *fullScaleImage;
    }
    
    if (renderFullScaleThenDownscale) {
        
        if (!renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            
            ///The upscaled image will be rendered using input images at lower def... which means really crappy results, don't cache this image!
            fullScaleImage->reset( new Natron::Image(components, rod, fullScaleImageBounds, 0, par, depth, true) );
            
        } else {
            
            boost::shared_ptr<Natron::ImageParams> upscaledImageParams = Natron::Image::makeParams(cost,
                                                                                                   rod,
                                                                                                   fullScaleImageBounds,
                                                                                                   par,
                                                                                                   0,
                                                                                                   isProjectFormat,
                                                                                                   components,
                                                                                                   depth,
                                                                                                   framesNeeded);
            
            //The upscaled image will be rendered with input images at full def, it is then the best possibly rendered image so cache it!
            
            fullScaleImage->reset();
            getOrCreateFromCacheInternal(key,upscaledImageParams,createInCache,useDiskCache,fullScaleImage);
            
            if (!*fullScaleImage) {
                return false;
            }
        }
        
    }
    return true;
}


/*
 * @brief Split all rects to render in smaller rects and check if each one of them is identity.
 * For identity rectangles, we just call renderRoI again on the identity input in the tiledRenderingFunctor.
 * For non-identity rectangles, compute the bounding box of them and render it
 */
static void optimizeRectsToRender(Natron::EffectInstance* self,
                                  const RectI& inputsRoDIntersection,
                                  const std::list<RectI>& rectsToRender,
                                  const double time,
                                  const int view,
                                  const RenderScale& renderMappedScale,
                                  std::list<EffectInstance::RectToRender>* finalRectsToRender)
{
    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        
        std::vector<RectI> splits = it->splitIntoSmallerRects(0);
        EffectInstance::RectToRender nonIdentityRect;
        nonIdentityRect.isIdentity = false;
        nonIdentityRect.identityInput = 0;
        nonIdentityRect.rect.x1 = INT_MAX;
        nonIdentityRect.rect.x2 = INT_MIN;
        nonIdentityRect.rect.y1 = INT_MAX;
        nonIdentityRect.rect.y2 = INT_MIN;
        
        bool nonIdentityRectSet = false;
        for (std::size_t i = 0; i < splits.size(); ++i) {
            double identityInputTime;
            int identityInputNb;
            bool identity;
            
            if (!splits[i].intersects(inputsRoDIntersection)) {
                identity = self->isIdentity_public(false, 0, time, renderMappedScale, splits[i], view, &identityInputTime, &identityInputNb);
            } else {
                identity = false;
            }
            
            if (identity) {
                EffectInstance::RectToRender r;
                r.isIdentity = true;
                
                //Walk along the identity branch until we find the non identity input, or NULL in we case we will
                //just render black and transparant
                Natron::EffectInstance* identityInput = self->getInput(identityInputNb);
                if (identityInput) {
                    for(;;) {
                        
                        identity = identityInput->isIdentity_public(false, 0, time, renderMappedScale, splits[i], view, &identityInputTime, &identityInputNb);
                        if (!identity || identityInputNb == -2) {
                            break;
                        }
                        Natron::EffectInstance* subIdentityInput = identityInput->getInput(identityInputNb);
                        if (subIdentityInput == identityInput) {
                            break;
                        }
                        
                        identityInput = subIdentityInput;
                        if (!subIdentityInput) {
                            break;
                        }
                    }
                }
                r.identityInput = identityInput;
                r.identityTime = identityInputTime;
                r.rect = splits[i];
                finalRectsToRender->push_back(r);
                
            } else {
                nonIdentityRectSet = true;
                nonIdentityRect.rect.x1 = std::min(splits[i].x1,nonIdentityRect.rect.x1);
                nonIdentityRect.rect.x2 = std::max(splits[i].x2,nonIdentityRect.rect.x2);
                nonIdentityRect.rect.y1 = std::min(splits[i].y1,nonIdentityRect.rect.y1);
                nonIdentityRect.rect.y2 = std::max(splits[i].y2,nonIdentityRect.rect.y2);
            }
        }
        if (nonIdentityRectSet) {
            finalRectsToRender->push_back(nonIdentityRect);
        }
    }

}

EffectInstance::RenderRoIRetCode EffectInstance::renderRoI(const RenderRoIArgs & args,ImageList* outputPlanes)
{
   
    //Do nothing if no components were requested
    if (args.components.empty() || args.roi.isNull()) {
        qDebug() << getScriptName_mt_safe().c_str() << "renderRoi: Early bail-out components requested empty or RoI is NULL";
        return eRenderRoIRetCodeOk;
    }
    
    ParallelRenderArgs& frameRenderArgs = _imp->frameRenderArgs.localData();
    if (!frameRenderArgs.validArgs) {
        qDebug() << "Thread-storage for the render of the frame was not set, this is a bug.";
        frameRenderArgs.time = args.time;
        frameRenderArgs.nodeHash = getHash();
        frameRenderArgs.view = args.view;
        frameRenderArgs.isSequentialRender = false;
        frameRenderArgs.isRenderResponseToUserInteraction = true;
        frameRenderArgs.validArgs = true;
    } else {
        //The hash must not have changed if we did a pre-pass.
        assert(!frameRenderArgs.request || frameRenderArgs.nodeHash == frameRenderArgs.request->nodeHash);
    }
    
    ///The args must have been set calling setParallelRenderArgs
    assert(frameRenderArgs.validArgs);
    
    ///For writer we never want to cache otherwise the next time we want to render it will skip writing the image on disk!
    bool byPassCache = args.byPassCache;

    ///Use the hash at this time, and then copy it to the clips in the thread local storage to use the same value
    ///through all the rendering of this frame.
    U64 nodeHash = frameRenderArgs.nodeHash;
 
    const double par = getPreferredAspectRatio();


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

        renderScaleOneUpstreamIfRenderScaleSupportDisabled = getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
        
        ///For multi-resolution we want input images with exactly the same size as the output image
        if (!renderScaleOneUpstreamIfRenderScaleSupportDisabled && !supportsMultiResolution()) {
            renderScaleOneUpstreamIfRenderScaleSupportDisabled = true;
        }
    }
    
    ViewInvarianceLevel viewInvariance = isViewInvariant();
    
    const FrameViewRequest* requestPassData = 0;
    if (frameRenderArgs.request) {
        requestPassData = frameRenderArgs.request->getFrameViewRequest(args.time, args.view);
    }
 
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Get the RoD ///////////////////////////////////////////////////////////////
    
    ///if the rod is already passed as parameter, just use it and don't call getRegionOfDefinition
    if ( !args.preComputedRoD.isNull() ) {
        rod = args.preComputedRoD;
    } else {
        
        ///Check if the pre-pass already has the RoD
        if (requestPassData) {
            rod = requestPassData->globalData.rod;
            isProjectFormat = requestPassData->globalData.isProjectFormat;
        } else {
            assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
            StatusEnum stat = getRegionOfDefinition_public(nodeHash,args.time, renderMappedScale, args.view, &rod, &isProjectFormat);
            
            ///The rod might be NULL for a roto that has no beziers and no input
            if (stat == eStatusFailed) {
                ///if getRoD fails, this might be because the RoD is null after all (e.g: an empty Roto node), we don't want the render to fail
                return eRenderRoIRetCodeOk;
            } else if (rod.isNull()) {
                //Nothing to render
                return eRenderRoIRetCodeOk;
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
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End get RoD ///////////////////////////////////////////////////////////////

    
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
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Check if effect is identity ///////////////////////////////////////////////////////////////
    {
        double inputTimeIdentity = 0.;
        int inputNbIdentity;
        
        assert( !( (supportsRS == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        bool identity;
        
        RectI pixelRod;
        rod.toPixelEnclosing(args.mipMapLevel, par, &pixelRod);
        
        if (args.view != 0 && viewInvariance == eViewInvarianceAllViewsInvariant) {
            identity = true;
            inputNbIdentity = -2;
            inputTimeIdentity = args.time;
        } else {
            try {
                if (requestPassData) {
                    inputTimeIdentity = requestPassData->globalData.inputIdentityTime;
                    inputNbIdentity = requestPassData->globalData.identityInputNb;
                    identity = inputNbIdentity != -1;
                } else {
                    identity = isIdentity_public(true, nodeHash, args.time, renderMappedScale, pixelRod, args.view, &inputTimeIdentity, &inputNbIdentity);
                }
            } catch (...) {
                return eRenderRoIRetCodeFailed;
            }
        }
        
        if ( (supportsRS == eSupportsMaybe) && (renderMappedMipMapLevel != 0) ) {
            // supportsRenderScaleMaybe may have changed, update it
            supportsRS = supportsRenderScaleMaybe();
            renderFullScaleThenDownscale = true;
            renderMappedScale.x = renderMappedScale.y = 1.;
            renderMappedMipMapLevel = 0;
            
        }
        
        if (identity) {
            ///The effect is an identity but it has no inputs
            if (inputNbIdentity == -1) {
                return eRenderRoIRetCodeOk;
            } else if (inputNbIdentity == -2) {
                // there was at least one crash if you set the first frame to a negative value
                assert(inputTimeIdentity != args.time || viewInvariance == eViewInvarianceAllViewsInvariant);
                
                // be safe in release mode otherwise we hit an infinite recursion
                if (inputTimeIdentity != args.time || viewInvariance == eViewInvarianceAllViewsInvariant) {
                    
                    ///This special value of -2 indicates that the plugin is identity of itself at another time
                    RenderRoIArgs argCpy = args;
                    argCpy.time = inputTimeIdentity;
                    
                    if (viewInvariance == eViewInvarianceAllViewsInvariant) {
                        argCpy.view = 0;
                    }
                    
                    argCpy.preComputedRoD.clear(); //< clear as the RoD of the identity input might not be the same (reproducible with Blur)
                    
                    return renderRoI(argCpy,outputPlanes);
                }
            }
            
            double firstFrame,lastFrame;
            getFrameRange_public(nodeHash, &firstFrame, &lastFrame);
            
            RectD canonicalRoI;
            ///WRONG! We can't clip against the RoD of *this* effect. We should clip against the RoD of the input effect, but this is done
            ///later on for us already.
            //args.roi.toCanonical(args.mipMapLevel, rod, &canonicalRoI);
            args.roi.toCanonical_noClipping(args.mipMapLevel, par,  &canonicalRoI);
            
            Natron::EffectInstance* inputEffectIdentity = getInput(inputNbIdentity);
            if (inputEffectIdentity) {
                
                if (frameRenderArgs.stats && frameRenderArgs.stats->isInDepthProfilingEnabled()) {
                    frameRenderArgs.stats->setNodeIdentity(getNode(), inputEffectIdentity->getNode());
                }

                
                RenderRoIArgs inputArgs = args;
                inputArgs.time = inputTimeIdentity;
                
                // Make sure we do not hold the RoD for this effect
                inputArgs.preComputedRoD.clear();
                
                return inputEffectIdentity->renderRoI(inputArgs, outputPlanes);
                
            } else {
                assert(outputPlanes->empty());
            }
            
            return eRenderRoIRetCodeOk;
            
        } // if (identity)
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End identity check ///////////////////////////////////////////////////////////////
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Handle pass-through for planes //////////////////////////////////////////////////////////
    ComponentsAvailableMap componentsAvailables;
    
    //Available planes/components is view agnostic
    getComponentsAvailable(args.time, &componentsAvailables);
    
    ComponentsNeededMap neededComps;
    bool processAllComponentsRequested;
    bool processChannels[4];
    ComponentsNeededMap::iterator foundOutputNeededComps;
    
    {
        SequenceTime ptTime;
        int ptView;
        boost::shared_ptr<Natron::Node> ptInput;
        getComponentsNeededAndProduced_public(args.time, args.view, &neededComps, &processAllComponentsRequested, &ptTime, &ptView, processChannels, &ptInput);
        
        foundOutputNeededComps = neededComps.find(-1);
        if (foundOutputNeededComps == neededComps.end()) {
            return eRenderRoIRetCodeOk;
        }
        
        if (processAllComponentsRequested) {
            std::vector<ImageComponents> compVec;
            for (std::list<Natron::ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
                
                bool found = false;
                for (std::vector<Natron::ImageComponents>::const_iterator it2 = foundOutputNeededComps->second.begin(); it2 != foundOutputNeededComps->second.end(); ++it2) {
                    if ((it2->isColorPlane() && it->isColorPlane())) {
                        compVec.push_back(*it2);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    compVec.push_back(*it);
                }
            }
            for (ComponentsNeededMap::iterator it = neededComps.begin(); it != neededComps.end(); ++it) {
                it->second = compVec;
            }
            //neededComps[-1] = compVec;
        }
    }
    const std::vector<Natron::ImageComponents>& outputComponents = foundOutputNeededComps->second;
    
    /*
     * For all requested planes, check which components can be produced in output by this node. 
     * If the components are from the color plane, if another set of components of the color plane is present
     * we try to render with those instead.
     */
    std::list<Natron::ImageComponents> requestedComponents;
    
    ComponentsAvailableMap componentsToFetchUpstream;
    for (std::list<Natron::ImageComponents>::const_iterator it = args.components.begin(); it != args.components.end(); ++it) {
        
        assert(it->getNumComponents() > 0);
        
        bool isColorComponents = it->isColorPlane();
        
        ComponentsAvailableMap::iterator found = componentsAvailables.end();
        for (ComponentsAvailableMap::iterator it2 = componentsAvailables.begin(); it2 != componentsAvailables.end(); ++it2) {
            if (it2->first == *it) {
                found = it2;
                break;
            } else {
                if (isColorComponents && it2->first.isColorPlane() && isSupportedComponent(-1, it2->first)) {
                    //We found another set of components in the color plane, take it
                    found = it2;
                    break;
                }
            }
        }
        
        // If  the requested component is not present, then it will just return black and transparant to the plug-in.
        if (found != componentsAvailables.end()) {
            if (found->second.lock() == getNode()) {
                requestedComponents.push_back(*it);
            } else {
                //The component is not available directly from this node, fetch it upstream
                componentsToFetchUpstream.insert(std::make_pair(*it, found->second.lock()));
            }
            
        }
    }
    
    //Render planes that we are not able to render on this node from upstream
    for (ComponentsAvailableMap::iterator it = componentsToFetchUpstream.begin(); it != componentsToFetchUpstream.end(); ++it) {
        NodePtr node = it->second.lock();
        if (node) {
            RenderRoIArgs inArgs = args;
            inArgs.components.clear();
            inArgs.components.push_back(it->first);
            ImageList inputPlanes;
            RenderRoIRetCode inputRetCode = node->getLiveInstance()->renderRoI(inArgs,&inputPlanes);
            assert(inputPlanes.size() == 1 || inputPlanes.empty());
            if (inputRetCode == eRenderRoIRetCodeAborted || inputRetCode == eRenderRoIRetCodeFailed || inputPlanes.empty()) {
                return inputRetCode;
            }
            outputPlanes->push_back(inputPlanes.front());
        }
    }
    
    ///There might be only planes to render that were fetched from upstream
    if (requestedComponents.empty()) {
        return eRenderRoIRetCodeOk;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End pass-through for planes //////////////////////////////////////////////////////////

    // At this point, if only the pass through planes are view variant and the rendered view is different than 0,
    // just call renderRoI again for the components left to render on the view 0.    
    if (args.view != 0 && viewInvariance == eViewInvarianceOnlyPassThroughPlanesVariant) {
        RenderRoIArgs argCpy = args;
        argCpy.view = 0;
        argCpy.preComputedRoD.clear();
        return renderRoI(argCpy,outputPlanes);
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Transform concatenations ///////////////////////////////////////////////////////////////
    ///Try to concatenate transform effects
    InputMatrixMap inputsToTransform;
    bool useTransforms;
    if (requestPassData) {
        inputsToTransform = requestPassData->globalData.transforms;
        useTransforms = !inputsToTransform.empty();
    } else {
        bool useTransforms = appPTR->getCurrentSettings()->isTransformConcatenationEnabled();
        if (useTransforms) {
            tryConcatenateTransforms(args.time, args.view, args.scale, &inputsToTransform);
        }
    }
    
    ///Ok now we have the concatenation of all matrices, set it on the associated clip and reroute the tree
    boost::shared_ptr<TransformReroute_RAII> transformConcatenationReroute;
    if (!inputsToTransform.empty()) {
        transformConcatenationReroute.reset(new TransformReroute_RAII(this,inputsToTransform));
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End transform concatenations//////////////////////////////////////////////////////////
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Compute RoI depending on render scale ///////////////////////////////////////////////////
    
    
    RectI downscaledImageBoundsNc,upscaledImageBoundsNc;
    rod.toPixelEnclosing(args.mipMapLevel, par, &downscaledImageBoundsNc);
    rod.toPixelEnclosing(0, par, &upscaledImageBoundsNc);
    
    
    
    ///Make sure the RoI falls within the image bounds
    ///Intersection will be in pixel coordinates
    if (frameRenderArgs.tilesSupported) {
        if (useImageAsOutput) {
            if (!roi.intersect(upscaledImageBoundsNc, &roi)) {
                return eRenderRoIRetCodeOk;
            }
            assert(roi.x1 >= upscaledImageBoundsNc.x1 && roi.y1 >= upscaledImageBoundsNc.y1 &&
                   roi.x2 <= upscaledImageBoundsNc.x2 && roi.y2 <= upscaledImageBoundsNc.y2);
            
        } else {
            if (!roi.intersect(downscaledImageBoundsNc, &roi)) {
                return eRenderRoIRetCodeOk;
            }
            assert(roi.x1 >= downscaledImageBoundsNc.x1 && roi.y1 >= downscaledImageBoundsNc.y1 &&
                   roi.x2 <= downscaledImageBoundsNc.x2 && roi.y2 <= downscaledImageBoundsNc.y2);
        }
#ifndef NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS
        ///just allocate the roi
        upscaledImageBoundsNc.intersect(roi, &upscaledImageBoundsNc);
        downscaledImageBoundsNc.intersect(args.roi, &downscaledImageBoundsNc);
#endif
    } else {
        roi = useImageAsOutput ? upscaledImageBoundsNc : downscaledImageBoundsNc;
    }
    
    const RectI& downscaledImageBounds = downscaledImageBoundsNc;
    const RectI& upscaledImageBounds = upscaledImageBoundsNc;
    
    RectD canonicalRoI;
    if (useImageAsOutput) {
        roi.toCanonical(0, par, rod, &canonicalRoI);
    } else {
        roi.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Compute RoI /////////////////////////////////////////////////////////////////////////
    
    
    
    bool isFrameVaryingOrAnimated = isFrameVaryingOrAnimated_Recursive();
    bool createInCache = shouldCacheOutput(isFrameVaryingOrAnimated);

    Natron::ImageKey key = Natron::Image::makeKey(nodeHash, isFrameVaryingOrAnimated, args.time, args.view, frameRenderArgs.draftMode);

    bool useDiskCacheNode = dynamic_cast<DiskCacheNode*>(this) != NULL;


    
    /*
     * Get the bitdepth and output components that the plug-in expects to render. The cached image does not necesserarily has the bitdepth
     * that the plug-in expects.
     */
    Natron::ImageBitDepthEnum outputDepth;
    std::list<Natron::ImageComponents> outputClipPrefComps;
    getPreferredDepthAndComponents(-1, &outputClipPrefComps, &outputDepth);
    assert(!outputClipPrefComps.empty());
    

    
    ImagePlanesToRender planesToRender;
    FramesNeededMap framesNeeded;
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Look-up the cache ///////////////////////////////////////////////////////////////
    
    {
        //If one plane is missing from cache, we will have to render it all. For all other planes, either they have nothing
        //left to render, otherwise we render them for all the roi again.
        bool missingPlane = false;
        
        for (std::list<ImageComponents>::iterator it = requestedComponents.begin(); it != requestedComponents.end(); ++it) {
            
            PlaneToRender plane;
            
            /*
             * If the plane is the color plane, we might have to convert between components, hence we always
             * try to find in the cache the "preferred" components of this node for the color plane.
             * For all other planes, just consider this set of components, we do not allow conversion.
             */
            const ImageComponents* components = 0;
            if (!it->isColorPlane()) {
                components = &(*it);
            } else {
                for (std::vector<Natron::ImageComponents>::const_iterator it2 = outputComponents.begin(); it2 != outputComponents.end(); ++it2) {
                    if (it2->isColorPlane()) {
                        components = &(*it2);
                        break;
                    }
                }
            }
            assert(components);
            getImageFromCacheAndConvertIfNeeded(createInCache, useDiskCacheNode, key, renderMappedMipMapLevel,
                                                useImageAsOutput ? &upscaledImageBounds : &downscaledImageBounds,
                                                &rod,
                                                args.bitdepth, *it,
                                                outputDepth,
                                                *components,
                                                args.inputImagesList,
                                                frameRenderArgs.stats,
                                                &plane.fullscaleImage);
            
            
            if (byPassCache) {
                if (plane.fullscaleImage) {
                    appPTR->removeFromNodeCache(key.getHash());
                    plane.fullscaleImage.reset();
                }
                //For writers, we always want to call the render action, but we still want to use the cache for nodes upstream
                if (isWriter()) {
                    byPassCache = false;
                }
            }
            if (plane.fullscaleImage) {
                
                if (missingPlane) {
                    std::list<RectI> restToRender;
                    plane.fullscaleImage->getRestToRender(roi, restToRender);
                    if (!restToRender.empty()) {
                        appPTR->removeFromNodeCache(plane.fullscaleImage);
                        plane.fullscaleImage.reset();
                    } else {
                        outputPlanes->push_back(plane.fullscaleImage);
                        continue;
                    }
                } else {
                    
                    //Overwrite the RoD with the RoD contained in the image.
                    //This is to deal with the situation with an image rendered at scale 1 in the cache, but a new render asking for the same
                    //image at scale 0.5. The RoD will then be slightly larger at scale 0.5 thus re-rendering a few pixels. If the effect
                    //wouldn't support tiles, then it'b problematic as it would need to render the whole frame again just for a few pixels.
//                    if (!tilesSupported) {
//                        rod = plane.fullscaleImage->getRoD();
//                    }
                    framesNeeded = plane.fullscaleImage->getParams()->getFramesNeeded();
                }
            } else {
                if (!missingPlane) {
                    missingPlane = true;
                    //Ensure that previous planes are either already rendered or otherwise render them  again
                    std::map<ImageComponents, PlaneToRender> newPlanes;
                    for (std::map<ImageComponents, PlaneToRender>::iterator it2 = planesToRender.planes.begin();
                         it2 != planesToRender.planes.end(); ++it2) {
                        if (it2->second.fullscaleImage) {
                            std::list<RectI> restToRender;
                            it2->second.fullscaleImage->getRestToRender(roi, restToRender);
                            if (!restToRender.empty()) {
                                appPTR->removeFromNodeCache(it2->second.fullscaleImage);
                                it2->second.fullscaleImage.reset();
                                it2->second.downscaleImage.reset();
                                newPlanes.insert(*it2);
                            } else {
                                outputPlanes->push_back(it2->second.fullscaleImage);
                            }
                        } else {
                            newPlanes.insert(*it2);
                        }
                        
                    }
                    planesToRender.planes = newPlanes;
                }
            }
            
            plane.downscaleImage = plane.fullscaleImage;
            plane.isAllocatedOnTheFly = false;
            planesToRender.planes.insert(std::make_pair(*it, plane));
        }
    }
    
    assert(!planesToRender.planes.empty());
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////End cache lookup//////////////////////////////////////////////////////////
    
    if (framesNeeded.empty()) {
        if (requestPassData) {
            framesNeeded = requestPassData->globalData.frameViewsNeeded;
        } else {
            framesNeeded = getFramesNeeded_public(nodeHash, args.time, args.view, renderMappedMipMapLevel);
        }
    }
  
    
    ///In the event where we had the image from the cache, but it wasn't completly rendered over the RoI but the cache was almost full,
    ///we don't hold a pointer to it, allowing the cache to free it.
    ///Hence after rendering all the input images, we redo a cache look-up to check whether the image is still here
    bool redoCacheLookup = false;
    bool cacheAlmostFull = appPTR->isNodeCacheAlmostFull();
    
    
    ImagePtr isPlaneCached;
    
    if (!planesToRender.planes.empty()) {
        isPlaneCached = planesToRender.planes.begin()->second.fullscaleImage;
    }
    
    if (!isPlaneCached && args.roi.isNull()) {
        ///Empty RoI and nothing in the cache with matching args, return empty planes.
        return eRenderRoIRetCodeFailed;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Determine rectangles left to render /////////////////////////////////////////////////////
    
    std::list<RectI> rectsLeftToRender;
    bool isDuringPaintStroke = isDuringPaintStrokeCreationThreadLocal();
    bool fillGrownBoundsWithZeroes = false;
    RectI lastStrokePixelRoD;
    if (isDuringPaintStroke && args.inputImagesList.empty()) {
        RectD lastStrokeRoD;
        NodePtr node = getNode();
        if (!node->isLastPaintStrokeBitmapCleared()) {
            node->getLastPaintStrokeRoD(&lastStrokeRoD);
            node->clearLastPaintStrokeRoD();
           // qDebug() << getScriptName_mt_safe().c_str() << "last stroke RoD: " << lastStrokeRoD.x1 << lastStrokeRoD.y1 << lastStrokeRoD.x2 << lastStrokeRoD.y2;
            lastStrokeRoD.toPixelEnclosing(mipMapLevel, par, &lastStrokePixelRoD);
        }
        
    }

    if (isPlaneCached) {

        if (isDuringPaintStroke && !lastStrokePixelRoD.isNull()) {
            
            fillGrownBoundsWithZeroes = true;
            //Clear the bitmap of the cached image in the portion of the last stroke to only recompute what's needed
            for (std::map<ImageComponents, PlaneToRender>::iterator it2 = planesToRender.planes.begin();
                 it2 != planesToRender.planes.end(); ++it2) {
                it2->second.fullscaleImage->clearBitmap(lastStrokePixelRoD);
                
                /*
                 * This is useful to optimize the bitmap checking
                 * when we are sure multiple threads are not using the image and we have a very small RoI to render.
                 * For now it's only used for the rotopaint while painting.
                 */
                it2->second.fullscaleImage->setBitmapDirtyZone(lastStrokePixelRoD);
                
                
            }
        }
        
        ///We check what is left to render.
#if NATRON_ENABLE_TRIMAP
        if (!frameRenderArgs.canAbort && frameRenderArgs.isRenderResponseToUserInteraction) {
#ifndef DEBUG
            isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender.isBeingRenderedElsewhere);
#else
       // in debug mode, check that the result of getRestToRender_trimap and getRestToRender is the same if the image
       // is not currently rendered concurrently
            QMutexLocker k(&_imp->imagesBeingRenderedMutex);
            EffectInstance::Implementation::IBRMap::const_iterator found = _imp->imagesBeingRendered.find(isPlaneCached);
            if (found == _imp->imagesBeingRendered.end() || !found->second->refCount) {
                
                Image::ReadAccess racc(isPlaneCached.get());
                isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender.isBeingRenderedElsewhere);
                std::list<RectI> tmpRects;
                isPlaneCached->getRestToRender(roi, tmpRects);
                
                //If it crashes here that means the image is no longer being rendered but its bitmap still contains PIXEL_UNAVAILABLE pixels.
                //The other thread should have removed that image from the cache or marked the image as rendered.
                assert(!planesToRender.isBeingRenderedElsewhere);
                assert(rectsLeftToRender.size() == tmpRects.size());
                
                std::list<RectI>::iterator oIt = rectsLeftToRender.begin();
                for (std::list<RectI>::iterator it = tmpRects.begin(); it!=tmpRects.end(); ++it,++oIt) {
                    assert(*it == *oIt);
                }
            } else {
                isPlaneCached->getRestToRender_trimap(roi, rectsLeftToRender, &planesToRender.isBeingRenderedElsewhere);
            }
#endif
        } else {
            isPlaneCached->getRestToRender(roi, rectsLeftToRender);
        }
#else
        isPlaneCached->getRestToRender(roi, rectsLeftToRender);
#endif
        if (isDuringPaintStroke && !rectsLeftToRender.empty() && !lastStrokePixelRoD.isNull()) {
            rectsLeftToRender.clear();
            RectI intersection;
            if (downscaledImageBounds.intersect(lastStrokePixelRoD, &intersection)) {
                rectsLeftToRender.push_back(intersection);
            }
        }
        
        if (!rectsLeftToRender.empty() && cacheAlmostFull) {
            ///The node cache is almost full and we need to render  something in the image, if we hold a pointer to this image here
            ///we might recursively end-up in this same situation at each level of the render tree, ending with all images of each level
            ///being held in memory.
            ///Our strategy here is to clear the pointer, hence allowing the cache to remove the image, and ask the inputs to render the full RoI
            ///instead of the rest to render. This way, even if the image is cleared from the cache we already have rendered the full RoI anyway.
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(roi);
            for (std::map<ImageComponents, PlaneToRender>::iterator it2 = planesToRender.planes.begin(); it2 != planesToRender.planes.end(); ++it2) {
                //Keep track of the original cached image for the re-lookup afterward, if the pointer doesn't match the first look-up, don't consider
                //the image because the region to render might have changed and we might have to re-trigger a render on inputs again.
                
                ///Make sure to never dereference originalCachedImage! We only compare it (that's why it s a void*)
                it2->second.originalCachedImage = it2->second.fullscaleImage.get();
                it2->second.fullscaleImage.reset();
                it2->second.downscaleImage.reset();
            }
            isPlaneCached.reset();
            redoCacheLookup = true;
        }
        
        
        
        ///If the effect doesn't support tiles and it has something left to render, just render the bounds again
        ///Note that it should NEVER happen because if it doesn't support tiles in the first place, it would
        ///have rendered the rod already.
        if (!frameRenderArgs.tilesSupported && !rectsLeftToRender.empty() && isPlaneCached) {
            ///if the effect doesn't support tiles, just render the whole rod again even though
            rectsLeftToRender.clear();
            rectsLeftToRender.push_back(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds);
        }
    } else {
        if (frameRenderArgs.tilesSupported) {
            rectsLeftToRender.push_back(roi);
        } else {
            rectsLeftToRender.push_back(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds);
        }
    }

    /*
     * If the effect has multiple inputs (such as masks) try to call isIdentity if the RoDs do not intersect the RoI
     */
    bool tryIdentityOptim = false;
    RectI inputsRoDIntersectionPixel;
    if (frameRenderArgs.tilesSupported && !rectsLeftToRender.empty()
        && (frameRenderArgs.viewerProgressReportEnabled || isDuringPaintStroke)) {
        
        RectD inputsIntersection;
        bool inputsIntersectionSet = false;
        bool hasDifferentRods = false;
        int maxInput = getMaxInputCount();
        bool hasMask = false;
        
        boost::shared_ptr<RotoDrawableItem> attachedStroke = getNode()->getAttachedRotoItem();
        for (int i = 0; i < maxInput; ++i) {
            
            bool isMask = isInputMask(i) || isInputRotoBrush(i);
            
            RectD inputRod;
            if (attachedStroke && isMask) {
                getNode()->getPaintStrokeRoD(args.time, &inputRod);
                hasMask = true;
            } else {
                
                EffectInstance* input = getInput(i);
                if (!input) {
                    continue;
                }
                bool isProjectFormat;
                const ParallelRenderArgs* inputFrameArgs = input->getParallelRenderArgsTLS();
                U64 inputHash = (inputFrameArgs && inputFrameArgs->validArgs) ? inputFrameArgs->nodeHash : input->getHash();
                Natron::StatusEnum stat = input->getRegionOfDefinition_public(inputHash, args.time, args.scale, args.view, &inputRod, &isProjectFormat);
                if (stat != eStatusOK && !inputRod.isNull()) {
                    break;
                }
                if (isMask) {
                    hasMask = true;
                }
            }
            if (!inputsIntersectionSet) {
                inputsIntersection = inputRod;
                inputsIntersectionSet = true;
            } else {
                if (!hasDifferentRods) {
                    if (inputRod != inputsIntersection) {
                        hasDifferentRods = true;
                    }
                }
                inputsIntersection.intersect(inputRod, &inputsIntersection);
            }
        }
        
        /*
         If the effect has 1 or more inputs and:
         - An input is a mask OR
         - Several inputs have different region of definition
         Try to split the rectangles to render in smaller rectangles, we have great chances that these smaller rectangles
         are identity over one of the input effect, thus avoiding pixels to render.
         */
        if (inputsIntersectionSet && (hasMask || hasDifferentRods)) {
            inputsIntersection.toPixelEnclosing(mipMapLevel, par, &inputsRoDIntersectionPixel);
            tryIdentityOptim = true;
        }
        
    }
    
    if (tryIdentityOptim) {
        optimizeRectsToRender(this, inputsRoDIntersectionPixel, rectsLeftToRender, args.time, args.view, renderMappedScale, &planesToRender.rectsToRender);
    } else {
        for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it!=rectsLeftToRender.end(); ++it) {
            RectToRender r;
            r.rect = *it;
            r.identityInput = 0;
            r.isIdentity = false;
            planesToRender.rectsToRender.push_back(r);
        }
    }
    
    bool hasSomethingToRender = !planesToRender.rectsToRender.empty();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Determine rectangles left to render /////////////////////////////////////////////////
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Pre-render input images ////////////////////////////////////////////////////////////////
    
    ///Pre-render input images before allocating the image if we need to render
    {
        const ImageComponents& outComp = outputComponents.front();
        if (outComp.isColorPlane()) {
            planesToRender.outputPremult = getOutputPremultiplication();
        } else {
            planesToRender.outputPremult = eImagePremultiplicationOpaque;
        }
    }
    for (std::list<RectToRender>::iterator it = planesToRender.rectsToRender.begin(); it != planesToRender.rectsToRender.end(); ++it) {
        
        if (it->isIdentity) {
            continue;
        }
        
        RectD canonicalRoI;
        if (useImageAsOutput) {
            it->rect.toCanonical(0, par, rod, &canonicalRoI);
        } else {
            it->rect.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
        }

        RenderRoIRetCode inputCode = renderInputImagesForRoI(requestPassData,
                                                             useTransforms,
                                                             args.time,
                                                             args.view,
                                                             par,
                                                             rod,
                                                             canonicalRoI,
                                                             inputsToTransform,
                                                             args.mipMapLevel,
                                                             args.scale,
                                                             renderMappedScale,
                                                             renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                             byPassCache,
                                                             framesNeeded,
                                                             neededComps,
                                                             &it->imgs,
                                                             &it->inputRois);
        
        if (planesToRender.inputPremult.empty()) {
            for (InputImagesMap::iterator it2 = it->imgs.begin(); it2 != it->imgs.end(); ++it2) {
                EffectInstance* input = getInput(it2->first);
                if (input) {
                    Natron::ImagePremultiplicationEnum inputPremult = input->getOutputPremultiplication();
                    if (!it2->second.empty()) {
                        const ImageComponents& comps = it2->second.front()->getComponents();
                        if (!comps.isColorPlane()) {
                            inputPremult = eImagePremultiplicationOpaque;
                        }
                    }
                    
                    planesToRender.inputPremult[it2->first] = inputPremult;
                }
            }
        }
        
        //Render was aborted
        if (inputCode != eRenderRoIRetCodeOk) {
            return inputCode;
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End Pre-render input images ////////////////////////////////////////////////////////////

    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Redo - cache lookup if memory almost full //////////////////////////////////////////////

    
    if (redoCacheLookup) {
        
        for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
            
            /*
             * If the plane is the color plane, we might have to convert between components, hence we always
             * try to find in the cache the "preferred" components of this node for the color plane.
             * For all other planes, just consider this set of components, we do not allow conversion.
             */
            const ImageComponents* components = 0;
            if (!it->first.isColorPlane()) {
                components = &(it->first);
            } else {
                for (std::vector<Natron::ImageComponents>::const_iterator it = outputComponents.begin(); it != outputComponents.end(); ++it) {
                    if (it->isColorPlane()) {
                        components = &(*it);
                        break;
                    }
                }
            }

            assert(components);
            getImageFromCacheAndConvertIfNeeded(createInCache, useDiskCacheNode, key, renderMappedMipMapLevel,
                                                useImageAsOutput ? &upscaledImageBounds : &downscaledImageBounds,
                                                &rod,
                                                args.bitdepth, it->first,
                                                outputDepth,*components,
                                                args.inputImagesList, frameRenderArgs.stats, &it->second.fullscaleImage);
            
            ///We must retrieve from the cache exactly the originally retrieved image, otherwise we might have to call  renderInputImagesForRoI
            ///again, which could create a vicious cycle.
            if (it->second.fullscaleImage && it->second.fullscaleImage.get() == it->second.originalCachedImage) {
                it->second.downscaleImage = it->second.fullscaleImage;
            } else {
                for (std::map<ImageComponents, PlaneToRender>::iterator it2 = planesToRender.planes.begin(); it2 != planesToRender.planes.end(); ++it2) {
                    it2->second.fullscaleImage.reset();
                    it2->second.downscaleImage.reset();
                }
                break;
            }
            
            
        }
        
        isPlaneCached = planesToRender.planes.begin()->second.fullscaleImage;
        
        if (!isPlaneCached) {
            planesToRender.rectsToRender.clear();
            rectsLeftToRender.clear();
            if (frameRenderArgs.tilesSupported) {
                rectsLeftToRender.push_back(roi);
            } else {
                rectsLeftToRender.push_back(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds);
            }
            

            if (tryIdentityOptim && !rectsLeftToRender.empty()) {
                optimizeRectsToRender(this, inputsRoDIntersectionPixel, rectsLeftToRender, args.time, args.view, renderMappedScale, &planesToRender.rectsToRender);
            } else {
                for (std::list<RectI>::iterator it = rectsLeftToRender.begin(); it!=rectsLeftToRender.end(); ++it) {
                    RectToRender r;
                    r.rect = *it;
                    r.identityInput = 0;
                    r.isIdentity = false;
                    planesToRender.rectsToRender.push_back(r);
                }
            }
            
            ///We must re-copute input images because we might not have rendered what's needed
            for (std::list<RectToRender>::iterator it = planesToRender.rectsToRender.begin();
                 it != planesToRender.rectsToRender.end(); ++it) {
                if (it->isIdentity) {
                    continue;
                }
                
                RectD canonicalRoI;
                if (useImageAsOutput) {
                    it->rect.toCanonical(0, par, rod, &canonicalRoI);
                } else {
                    it->rect.toCanonical(args.mipMapLevel, par, rod, &canonicalRoI);
                }
                
                RenderRoIRetCode inputRetCode = renderInputImagesForRoI(requestPassData,
                                                                        useTransforms,
                                                                        args.time,
                                                                        args.view,
                                                                        par,
                                                                        rod,
                                                                        canonicalRoI,
                                                                        inputsToTransform,
                                                                        args.mipMapLevel,
                                                                        args.scale,
                                                                        renderMappedScale,
                                                                        renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                                                        byPassCache,
                                                                        framesNeeded,
                                                                        neededComps,
                                                                        &it->imgs,
                                                                        &it->inputRois);
                    //Render was aborted
                if (inputRetCode != eRenderRoIRetCodeOk) {
                    return inputRetCode;
                }
            }
        }
        
    } // if (redoCacheLookup) {
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End 2nd cache lookup ///////////////////////////////////////////////////////////////////
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// Allocate planes in the cache ////////////////////////////////////////////////////////////
    
    ///For all planes, if needed allocate the associated image
    if (hasSomethingToRender) {
        for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin();
             it != planesToRender.planes.end(); ++it) {
            
            const ImageComponents *components = 0;
            
            if (!it->first.isColorPlane()) {
                //This plane is not color, there can only be a single set of components
                components = &(it->first);
            } else {
                //Find color plane from clip preferences
                for (std::vector<Natron::ImageComponents>::const_iterator it = outputComponents.begin(); it != outputComponents.end(); ++it) {
                    if (it->isColorPlane()) {
                        components = &(*it);
                        break;
                    }
                }
            }
            assert(components);
            
            if (!it->second.fullscaleImage) {
                ///The image is not cached
                allocateImagePlane(key, rod, downscaledImageBounds, upscaledImageBounds, isProjectFormat, framesNeeded, *components, args.bitdepth, par, args.mipMapLevel, renderFullScaleThenDownscale, renderScaleOneUpstreamIfRenderScaleSupportDisabled, useDiskCacheNode, createInCache, &it->second.fullscaleImage, &it->second.downscaleImage);
                
            } else {
                
                /*
                 * There might be a situation  where the RoD of the cached image
                 * is not the same as this RoD even though the hash is the same.
                 * This seems to happen with the Roto node. This hack just updates the
                 * image's RoD to prevent an assert from triggering in the call to ensureBounds() below.
                 */
                RectD oldRod = it->second.fullscaleImage->getRoD();
                if (oldRod != rod) {
                    oldRod.merge(rod);
                    it->second.fullscaleImage->setRoD(oldRod);
                }
                
                
                /*
                 * Another thread might have allocated the same image in the cache but with another RoI, make sure
                 * it is big enough for us, or resize it to our needs.
                 */
                bool hasResized = it->second.fullscaleImage->ensureBounds(useImageAsOutput ? upscaledImageBounds : downscaledImageBounds,
                                                                          fillGrownBoundsWithZeroes, fillGrownBoundsWithZeroes);
                
                
                /*
                 * Note that the image has been resized and the bitmap explicitly set to 1 in the newly allocated portions (for rotopaint purpose).
                 * We must reset it back to 0 in the last stroke tick RoD.
                 */
                if (hasResized && fillGrownBoundsWithZeroes) {
                    it->second.fullscaleImage->clearBitmap(lastStrokePixelRoD);
                }
                
                if (renderFullScaleThenDownscale && it->second.fullscaleImage->getMipMapLevel() == 0) {
                    //Allocate a downscale image that will be cheap to create
                    ///The upscaled image will be rendered using input images at lower def... which means really crappy results, don't cache this image!
                    RectI bounds;
                    rod.toPixelEnclosing(args.mipMapLevel, par, &bounds);
                    it->second.downscaleImage.reset( new Natron::Image(*components, rod, downscaledImageBounds, args.mipMapLevel, it->second.fullscaleImage->getPixelAspectRatio(), outputDepth, true) );
                    it->second.fullscaleImage->downscaleMipMap(rod,it->second.fullscaleImage->getBounds(), 0, args.mipMapLevel, true, it->second.downscaleImage.get());
                }
            }
            
            ///The image and downscaled image are pointing to the same image in 2 cases:
            ///1) Proxy mode is turned off
            ///2) Proxy mode is turned on but plug-in supports render scale
            ///Subsequently the image and downscaled image are different only if the plug-in
            ///does not support the render scale and the proxy mode is turned on.
            assert( (it->second.fullscaleImage == it->second.downscaleImage && !renderFullScaleThenDownscale) ||
                   ((it->second.fullscaleImage != it->second.downscaleImage || it->second.fullscaleImage->getMipMapLevel() == it->second.downscaleImage->getMipMapLevel()) && renderFullScaleThenDownscale) );
        }
    } // hasSomethingToRender
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////// End allocation of planes ///////////////////////////////////////////////////////////////
    
    
    //There should always be at least 1 plane to render (The color plane)
    assert(!planesToRender.planes.empty());

    ///If we reach here, it can be either because the planes are cached or not, either way
    ///the planes are NOT a total identity, and they may have some content left to render.
    EffectInstance::RenderRoIStatusEnum renderRetCode = eRenderRoIStatusImageAlreadyRendered;
    
    bool renderAborted;

    if (!hasSomethingToRender && !planesToRender.isBeingRenderedElsewhere) {
        renderAborted = aborted();
    } else {
    
#if NATRON_ENABLE_TRIMAP
        ///Only use trimap system if the render cannot be aborted.
        if (!frameRenderArgs.canAbort && frameRenderArgs.isRenderResponseToUserInteraction) {
            for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
                _imp->markImageAsBeingRendered(useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage);
            }
        }
#endif

        if (hasSomethingToRender) {
            
            {
                ///If the last rendered image had a different hash key (i.e a parameter changed or an input changed)
                ///just remove the old image from the cache to recycle memory.
                ///We also do this if the mipmap level is different (e.g: the user is zooming in/out) because
                ///anyway the ViewerCache will have the texture cached and it would be redundant to keep this image
                ///in the cache since the ViewerCache already has it ready.
                
                
                ///Note that if 2 threads concurrently are rendering the same frame with 2 different keys, this would not
                ///interfere with the cache system for the other thread because if another thread uses this image, the
                ///attempt to remove it from the cache will fail because of use_count > 1
                bool isLastPlanesEmpty;
                U64 lastRenderHash;
                {
                    QMutexLocker l(&_imp->lastRenderArgsMutex);
                    isLastPlanesEmpty = _imp->lastPlanesRendered.empty();
                    lastRenderHash = _imp->lastRenderHash;
                }
                if ( !isLastPlanesEmpty && lastRenderHash != nodeHash ) {
                    ///once we got it remove it from the cache
                    if (!useDiskCacheNode) {
                        appPTR->removeAllImagesFromCacheWithMatchingKey(true, lastRenderHash);
                    } else {
                        appPTR->removeAllImagesFromDiskCacheWithMatchingKey(true, lastRenderHash);
                    }
                    {
                        QMutexLocker l(&_imp->lastRenderArgsMutex);
                        _imp->lastPlanesRendered.clear();
                    }
                }
            }
            
            
            // eRenderSafetyInstanceSafe means that there is at most one render per instance
            // NOTE: the per-instance lock should probably be shared between
            // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
            // and read-write on it.
            // It is probably safer to assume that several clones may write to the same output image only in the eRenderSafetyFullySafe case.
            
            // eRenderSafetyFullySafe means that there is only one render per FRAME : the lock is by image and handled in Node.cpp
            ///locks belongs to an instance)
            
            boost::shared_ptr<QMutexLocker> locker;
            Natron::RenderSafetyEnum safety = getCurrentThreadSafetyThreadLocal();
            if (safety == eRenderSafetyInstanceSafe) {
                locker.reset(new QMutexLocker( &getNode()->getRenderInstancesSharedMutex()));
            } else if (safety == eRenderSafetyUnsafe) {
                const Natron::Plugin* p = getNode()->getPlugin();
                assert(p);
                locker.reset(new QMutexLocker(p->getPluginLock()));
            }
            ///For eRenderSafetyFullySafe, don't take any lock, the image already has a lock on itself so we're sure it can't be written to by 2 different threads.
            
            if (frameRenderArgs.stats && frameRenderArgs.stats->isInDepthProfilingEnabled()) {
                frameRenderArgs.stats->setGlobalRenderInfosForNode(getNode(), rod, planesToRender.outputPremult, processChannels, frameRenderArgs.tilesSupported, !renderFullScaleThenDownscale, renderMappedMipMapLevel);
            }
            
# ifdef DEBUG

            /*{
                const std::list<RectToRender>& rectsToRender = planesToRender.rectsToRender;
                qDebug() <<'('<<QThread::currentThread()<<")--> "<< getNode()->getScriptName_mt_safe().c_str() << ": render view: " << args.view << ", time: " << args.time << " No. tiles: " << rectsToRender.size() << " rectangles";
                for (std::list<RectToRender>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
                    qDebug() << "rect: " << "x1= " <<  it->rect.x1 << " , y1= " << it->rect.y1 << " , x2= " << it->rect.x2 << " , y2= " << it->rect.y2 << "(identity:" << it->isIdentity << ")";
                }
                for (std::map<Natron::ImageComponents, PlaneToRender> ::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
                    qDebug() << "plane: " <<  it->second.downscaleImage.get() << it->first.getLayerName().c_str();
                }
                qDebug() << "Cached:" << (isPlaneCached.get() != 0) << "Rendered elsewhere:" << planesToRender.isBeingRenderedElsewhere;
                
            }*/
# endif
            renderRetCode = renderRoIInternal(args.time,
                                              frameRenderArgs,
                                              safety,
                                              args.mipMapLevel,
                                              args.view,
                                              rod,
                                              par,
                                              planesToRender,
                                              frameRenderArgs.isSequentialRender,
                                              frameRenderArgs.isRenderResponseToUserInteraction,
                                              nodeHash,
                                              renderFullScaleThenDownscale,
                                              renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                              byPassCache,
                                              outputDepth,
                                              outputClipPrefComps,
                                              processChannels);
        } // if (hasSomethingToRender) {
        
        renderAborted = aborted();
#if NATRON_ENABLE_TRIMAP
        
        if (!frameRenderArgs.canAbort && frameRenderArgs.isRenderResponseToUserInteraction) {
            ///Only use trimap system if the render cannot be aborted.
            ///If we were aborted after all (because the node got deleted) then return a NULL image and empty the cache
            ///of this image
            for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
                if (!renderAborted) {
                    if (renderRetCode == eRenderRoIStatusRenderFailed || !planesToRender.isBeingRenderedElsewhere) {
                        _imp->unmarkImageAsBeingRendered(useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage,
                                                         renderRetCode == eRenderRoIStatusRenderFailed);
                    } else {
                        if (!_imp->waitForImageBeingRenderedElsewhereAndUnmark(roi,
                                                                               useImageAsOutput ? it->second.fullscaleImage: it->second.downscaleImage)) {
                            renderAborted = true;
                        }
                    }
                } else {
                    appPTR->removeFromNodeCache(useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage);
                    _imp->unmarkImageAsBeingRendered(useImageAsOutput ? it->second.fullscaleImage : it->second.downscaleImage,true);
                    return eRenderRoIRetCodeAborted;
                }
            }
        }
#endif
    } // if (!hasSomethingToRender && !planesToRender.isBeingRenderedElsewhere) {
    
    
    if (renderAborted && renderRetCode != eRenderRoIStatusImageAlreadyRendered) {
        
        ///Return a NULL image
        
        if (isDuringPaintStroke) {
            //We know the image will never be used ever again
            appPTR->removeAllImagesFromCacheWithMatchingKey(true, nodeHash);
        }
        return eRenderRoIRetCodeAborted;
        
    } else if (renderRetCode == eRenderRoIStatusRenderFailed) {
        
        ///Throwing this exception will ensure the render stops.
        ///This is slightly clumsy since we already have a render rect code indicating it, we should
        ///use the ret code instead.
        throw std::runtime_error("Rendering Failed");
    }
    
    

#if DEBUG
    if (hasSomethingToRender && renderRetCode != eRenderRoIStatusRenderFailed && !renderAborted) {
        // Kindly check that everything we asked for is rendered!
        
        for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
            
            if (!frameRenderArgs.tilesSupported) {
                //assert that bounds are consistent with the RoD if tiles are not supported
                const RectD & srcRodCanonical = useImageAsOutput ? it->second.fullscaleImage->getRoD() : it->second.downscaleImage->getRoD();
                RectI srcBounds;
                srcRodCanonical.toPixelEnclosing(useImageAsOutput ? it->second.fullscaleImage->getMipMapLevel() : it->second.downscaleImage->getMipMapLevel(), par, &srcBounds);
                RectI srcRealBounds = useImageAsOutput ? it->second.fullscaleImage->getBounds() : it->second.downscaleImage->getBounds();
                assert(srcRealBounds.x1 == srcBounds.x1);
                assert(srcRealBounds.x2 == srcBounds.x2);
                assert(srcRealBounds.y1 == srcBounds.y1);
                assert(srcRealBounds.y2 == srcBounds.y2);
            }
            
            std::list<RectI> restToRender;
            if (useImageAsOutput) {
                it->second.fullscaleImage->getRestToRender(roi,restToRender);
            } else {
                it->second.downscaleImage->getRestToRender(roi,restToRender);
            }
            assert(restToRender.empty());
        }
    }
#endif
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// Make sure all planes rendered have the requested mipmap level and format ///////////////////////////
    
    bool useAlpha0ForRGBToRGBAConversion = args.caller ? args.caller->getNode()->usesAlpha0ToConvertFromRGBToRGBA() : false;

    for (std::map<ImageComponents, PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
        //We have to return the downscale image, so make sure it has been computed
        if (renderRetCode != eRenderRoIStatusRenderFailed && renderFullScaleThenDownscale && renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            assert(it->second.fullscaleImage->getMipMapLevel() == 0);
            roi.intersect(it->second.fullscaleImage->getBounds(), &roi);
            if (it->second.downscaleImage == it->second.fullscaleImage) {
                it->second.downscaleImage.reset(new Image(it->second.fullscaleImage->getComponents(),
                                                          it->second.fullscaleImage->getRoD(),
                                                          downscaledImageBounds,
                                                          args.mipMapLevel,
                                                          it->second.fullscaleImage->getPixelAspectRatio(),
                                                          it->second.fullscaleImage->getBitDepth(),
                                                          false));
            }
            
            it->second.fullscaleImage->downscaleMipMap(it->second.fullscaleImage->getRoD(),roi, 0, args.mipMapLevel, false, it->second.downscaleImage.get());
        }
        ///The image might need to be converted to fit the original requested format
        bool imageConversionNeeded = it->first != it->second.downscaleImage->getComponents() || args.bitdepth != it->second.downscaleImage->getBitDepth();
        
        if (imageConversionNeeded && renderRetCode != eRenderRoIStatusRenderFailed) {
            
            /**
             * Lock the downscaled image so it cannot be resized while creating the temp image and calling convertToFormat.
             **/
            boost::shared_ptr<Image> tmp;
            {
                Image::ReadAccess acc = it->second.downscaleImage->getReadRights();
                RectI bounds = it->second.downscaleImage->getBounds();

                tmp.reset( new Image(it->first, it->second.downscaleImage->getRoD(), bounds, mipMapLevel,it->second.downscaleImage->getPixelAspectRatio(), args.bitdepth, false) );
                
                bool unPremultIfNeeded = planesToRender.outputPremult == eImagePremultiplicationPremultiplied && it->second.downscaleImage->getComponentsCount() == 4 && tmp->getComponentsCount() == 3;
                
                if (useAlpha0ForRGBToRGBAConversion) {
                    it->second.downscaleImage->convertToFormatAlpha0(bounds,
                                                               getApp()->getDefaultColorSpaceForBitDepth(it->second.downscaleImage->getBitDepth()),
                                                               getApp()->getDefaultColorSpaceForBitDepth(args.bitdepth),
                                                               -1, false, unPremultIfNeeded, tmp.get());
                } else {
                    it->second.downscaleImage->convertToFormat(bounds,
                                                               getApp()->getDefaultColorSpaceForBitDepth(it->second.downscaleImage->getBitDepth()),
                                                               getApp()->getDefaultColorSpaceForBitDepth(args.bitdepth),
                                                               -1, false, unPremultIfNeeded, tmp.get());
                }
            }
            it->second.downscaleImage = tmp;
        }
        assert(it->second.downscaleImage->getComponents() == it->first && it->second.downscaleImage->getBitDepth() == args.bitdepth);
        outputPlanes->push_back(it->second.downscaleImage);

    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// End requested format convertion ////////////////////////////////////////////////////////////////////
  
    
    ///// Termination, update last rendered planes
    assert(!outputPlanes->empty());
    {
        ///flag that this is the last planes we rendered
        QMutexLocker l(&_imp->lastRenderArgsMutex);
        _imp->lastRenderHash = nodeHash;
        _imp->lastPlanesRendered = *outputPlanes;
    }
    return eRenderRoIRetCodeOk;
} // renderRoI

void
EffectInstance::transformInputRois(Natron::EffectInstance* self,
                                   const InputMatrixMap& inputTransforms,
                                   double par,
                                   const RenderScale& scale,
                                   RoIMap* inputsRoi,
                                   std::map<int, EffectInstance*>* reroutesMap)
{
    //Transform the RoIs by the inverse of the transform matrix (which is in pixel coordinates)
    for (InputMatrixMap::const_iterator it = inputTransforms.begin(); it != inputTransforms.end(); ++it) {
        
        RectD transformedRenderWindow;
        
        Natron::EffectInstance* effectInTransformInput = self->getInput(it->first);
        assert(effectInTransformInput);
        
        
        
        RoIMap::iterator foundRoI = inputsRoi->find(effectInTransformInput);
        if (foundRoI == inputsRoi->end()) {
            //There might be no RoI because it was null
            continue;
        }
        
        // invert it
        Transform::Matrix3x3 invertTransform;
        double det = Transform::matDeterminant(*it->second.cat);
        if (det != 0.) {
            invertTransform = Transform::matInverse(*it->second.cat, det);
        }
        
        Transform::Matrix3x3 canonicalToPixel = Transform::matCanonicalToPixel(par, scale.x,
                                                                               scale.y, false);
        Transform::Matrix3x3 pixelToCanonical = Transform::matPixelToCanonical(par,  scale.x,
                                                                               scale.y, false);
        
        invertTransform = Transform::matMul(Transform::matMul(pixelToCanonical, invertTransform), canonicalToPixel);
        Transform::transformRegionFromRoD(foundRoI->second, invertTransform, transformedRenderWindow);
        
        //Replace the original RoI by the transformed RoI
        inputsRoi->erase(foundRoI);
        inputsRoi->insert(std::make_pair(it->second.newInputEffect->getInput(it->second.newInputNbToFetchFrom), transformedRenderWindow));
        reroutesMap->insert(std::make_pair(it->first, it->second.newInputEffect));
        
    }
}


EffectInstance::RenderRoIRetCode
EffectInstance::renderInputImagesForRoI(const FrameViewRequest* request,
                                        bool useTransforms,
                                        double time,
                                        int view,
                                        double par,
                                        const RectD& rod,
                                        const RectD& canonicalRenderWindow,
                                        const InputMatrixMap& inputTransforms,
                                        unsigned int mipMapLevel,
                                        const RenderScale & scale,
                                        const RenderScale& renderMappedScale,
                                        bool useScaleOneInputImages,
                                        bool byPassCache,
                                        const FramesNeededMap& framesNeeded,
                                        const ComponentsNeededMap& neededComps,
                                        InputImagesMap *inputImages,
                                        RoIMap* inputsRoi)
{
    if (!request) {
        getRegionsOfInterest_public(time, renderMappedScale, rod, canonicalRenderWindow, view, inputsRoi);
    }
#ifdef DEBUG
    if (!inputsRoi->empty() && framesNeeded.empty() && !isReader() && !isRotoPaintNode()) {
        qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": getRegionsOfInterestAction returned 1 or multiple input RoI(s) but returned "
        << "an empty list with getFramesNeededAction";
    }
#endif
    
    std::map<int, EffectInstance*> reroutesMap;
    if (!request) {
        transformInputRois(this, inputTransforms,par,scale,inputsRoi,&reroutesMap);
    } else {
        reroutesMap = request->globalData.reroutesMap;
    }
   
    return treeRecurseFunctor(true,
                              getNode(),
                              framesNeeded,
                              *inputsRoi,
                              reroutesMap,
                              useTransforms,
                              0,
                              mipMapLevel,
                              NodePtr(),
                              0,
                              inputImages,
                              &neededComps,
                              useScaleOneInputImages,
                              byPassCache);
}


EffectInstance::RenderRoIStatusEnum
EffectInstance::renderRoIInternal(double time,
                                  const ParallelRenderArgs& frameArgs,
                                  Natron::RenderSafetyEnum safety,
                                  unsigned int mipMapLevel,
                                  int view,
                                  const RectD & rod, //!< effect rod in canonical coords
                                  const double par,
                                  ImagePlanesToRender& planesToRender,
                                  bool isSequentialRender,
                                  bool isRenderMadeInResponseToUserInteraction,
                                  U64 nodeHash,
                                  bool renderFullScaleThenDownscale,
                                  bool useScaleOneInputImages,
                                  bool byPassCache,
                                  Natron::ImageBitDepthEnum outputClipPrefDepth,
                                  const std::list<Natron::ImageComponents>& outputClipPrefsComps,
                                  bool* processChannels)
{
    EffectInstance::RenderRoIStatusEnum retCode;
    
    assert(!planesToRender.planes.empty());
    
    ///Add the window to the project's available formats if the effect is a reader
    ///This is the only reliable place where I could put these lines...which don't seem to feel right here.
    ///Plus setOrAddProjectFormat will actually set the project format the first time we read an image in the project
    ///hence ask for a new render... which can be expensive!
    ///Any solution how to work around this ?
    ///Edit: do not do this if in the main-thread (=noRenderThread = -1) otherwise we will change the parallel render args TLS
    ///which will lead to asserts down the stream
    if ( isReader() && QThread::currentThread() != qApp->thread()) {
        Format frmt;
        RectI pixelRoD;
        rod.toPixelEnclosing(0, par, &pixelRoD);
        frmt.set(pixelRoD);
        frmt.setPixelAspectRatio(par);
        getApp()->getProject()->setOrAddProjectFormat(frmt);
    }
    
    RenderScale renderMappedScale;
    unsigned int renderMappedMipMapLevel = 0;

    for (std::map<ImageComponents,PlaneToRender>::iterator it = planesToRender.planes.begin(); it != planesToRender.planes.end(); ++it) {
        it->second.renderMappedImage = renderFullScaleThenDownscale ? it->second.fullscaleImage : it->second.downscaleImage;
        if (it == planesToRender.planes.begin()) {
            renderMappedMipMapLevel = it->second.renderMappedImage->getMipMapLevel();
        }
    }
    
    renderMappedScale.x = Image::getScaleFromMipMapLevel(renderMappedMipMapLevel);
    renderMappedScale.y = renderMappedScale.x;
    


    RenderingFunctorRetEnum renderStatus = eRenderingFunctorRetOK;
    if (planesToRender.rectsToRender.empty()) {
        retCode = EffectInstance::eRenderRoIStatusImageAlreadyRendered;
    } else {
        retCode = EffectInstance::eRenderRoIStatusImageRendered;
    }


    ///Notify the gui we're rendering
    boost::shared_ptr<NotifyRenderingStarted_RAII> renderingNotifier;
    if (!planesToRender.rectsToRender.empty()) {
        renderingNotifier.reset(new NotifyRenderingStarted_RAII(getNode().get()));
    }

    ///depending on the thread-safety of the plug-in we render with a different
    ///amount of threads.
    ///If the project lock is already locked at this point, don't start any other thread
    ///as it would lead to a deadlock when the project is loading.
    ///Just fall back to Fully_safe
    int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
    if (safety == eRenderSafetyFullySafeFrame) {
        ///If the plug-in is eRenderSafetyFullySafeFrame that means it wants the host to perform SMP aka slice up the RoI into chunks
        ///but if the effect doesn't support tiles it won't work.
        ///Also check that the number of threads indicating by the settings are appropriate for this render mode.
        if ( !frameArgs.tilesSupported || (nbThreads == -1) || (nbThreads == 1) ||
            ( (nbThreads == 0) && (appPTR->getHardwareIdealThreadCount() == 1) ) ||
            ( QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount() ) ||
            isRotoPaintNode()) {
            safety = eRenderSafetyFullySafe;
        }
    }
   
    
    std::map<boost::shared_ptr<Natron::Node>,ParallelRenderArgs > tlsCopy;
    if (safety == eRenderSafetyFullySafeFrame) {
        /*
         * Since we're about to start new threads potentially, copy all the thread local storage on all nodes (any node may be involved in
         * expressions, and we need to retrieve the exact local time of render).
         */
        getApp()->getProject()->getParallelRenderArgs(tlsCopy);
    }
    
    double firstFrame, lastFrame;
    getFrameRange_public(nodeHash, &firstFrame, &lastFrame);

    
    ///We only need to call begin if we've not already called it.
    bool callBegin = false;
    
    /// call beginsequenceRender here if the render is sequential
    
    Natron::SequentialPreferenceEnum pref = getSequentialPreference();
    if (!isWriter() || pref == eSequentialPreferenceNotSequential) {
        callBegin = true;
    }
    
    
    
    if (callBegin) {
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        if (beginSequenceRender_public(time, time, 1, !appPTR->isBackground(), renderMappedScale, isSequentialRender,
                                       isRenderMadeInResponseToUserInteraction, view) == eStatusFailed) {
            renderStatus = eRenderingFunctorRetFailed;
        }
    }
    
   
    
    /*
     * All channels will be taken from this input if some channels are marked to be not processed
     */
    int preferredInput = getNode()->getPreferredInput();
    if (preferredInput != -1 && isInputMask(preferredInput)) {
        preferredInput = -1;
    }
    
    const QThread* currentThread = QThread::currentThread();
    
    if (renderStatus != eRenderingFunctorRetFailed) {
        
        if (safety == eRenderSafetyFullySafeFrame) {
            TiledRenderingFunctorArgs tiledArgs;
            tiledArgs.frameArgs = frameArgs;
            tiledArgs.frameTLS = tlsCopy;
            tiledArgs.renderFullScaleThenDownscale = renderFullScaleThenDownscale;
            tiledArgs.renderUseScaleOneInputs = useScaleOneInputImages;
            tiledArgs.isRenderResponseToUserInteraction = isRenderMadeInResponseToUserInteraction;
            tiledArgs.firstFrame = firstFrame;
            tiledArgs.lastFrame = lastFrame;
            tiledArgs.preferredInput = preferredInput;
            tiledArgs.mipMapLevel = mipMapLevel;
            tiledArgs.renderMappedMipMapLevel = renderMappedMipMapLevel;
            tiledArgs.rod = rod;
            tiledArgs.time = time;
            tiledArgs.view = view;
            tiledArgs.par = par;
            tiledArgs.byPassCache = byPassCache;
            tiledArgs.outputClipPrefDepth = outputClipPrefDepth;
            tiledArgs.outputClipPrefsComps = outputClipPrefsComps;
            tiledArgs.processChannels = processChannels;
            tiledArgs.planes = planesToRender;
            
            
#ifdef NATRON_HOSTFRAMETHREADING_SEQUENTIAL
            std::vector<EffectInstance::RenderingFunctorRetEnum> ret(tiledData.size());
            int i = 0;
            for (std::list<RectToRender>::const_iterator it = planesToRender.rectsToRender.begin(); it!= planesToRender.rectsToRender.end(); ++it,++i) {
                ret[i] = tiledRenderingFunctor(tiledArgs,
                                               *it,
                                               currentThread);
            }
            std::vector<EffectInstance::RenderingFunctorRetEnum>::const_iterator it2;

#else
       
            
            QFuture<RenderingFunctorRetEnum> ret = QtConcurrent::mapped(planesToRender.rectsToRender,
                                                                           boost::bind(&EffectInstance::tiledRenderingFunctor,
                                                                                       this,
                                                                                       tiledArgs,
                                                                                       _1,
                                                                                       currentThread));
            ret.waitForFinished();
            QFuture<EffectInstance::RenderingFunctorRetEnum>::const_iterator it2;

#endif
            for (it2 = ret.begin(); it2 != ret.end(); ++it2) {
                if ( (*it2) == EffectInstance::eRenderingFunctorRetFailed ) {
                    renderStatus = eRenderingFunctorRetFailed;
                    break;
                }
#if NATRON_ENABLE_TRIMAP
                else if ((*it2) == EffectInstance::eRenderingFunctorRetTakeImageLock) {
                    planesToRender.isBeingRenderedElsewhere = true;
                }
#endif
                else if ((*it2) == EffectInstance::eRenderingFunctorRetAborted) {
                    renderStatus = eRenderingFunctorRetFailed;
                    break;
                }
            }
   
        } else {
            for (std::list<RectToRender>::const_iterator it = planesToRender.rectsToRender.begin(); it != planesToRender.rectsToRender.end(); ++it) {
                
                
                
                RenderingFunctorRetEnum functorRet = tiledRenderingFunctor(currentThread, frameArgs, *it, tlsCopy, renderFullScaleThenDownscale, useScaleOneInputImages, isSequentialRender, isRenderMadeInResponseToUserInteraction, firstFrame, lastFrame, preferredInput, mipMapLevel, renderMappedMipMapLevel, rod, time, view, par, byPassCache, outputClipPrefDepth, outputClipPrefsComps, processChannels, planesToRender);
                
                if (functorRet == eRenderingFunctorRetFailed || functorRet == eRenderingFunctorRetAborted) {
                    renderStatus = functorRet;
                    break;
                }
                
                if  (functorRet == eRenderingFunctorRetTakeImageLock) {
                    renderStatus = eRenderingFunctorRetOK;
#if NATRON_ENABLE_TRIMAP
                    planesToRender.isBeingRenderedElsewhere = true;
#endif
                }
            } // for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        }
    } // if (renderStatus != eRenderingFunctorRetFailed) {
    
    ///never call endsequence render here if the render is sequential
    if (callBegin) {
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(renderMappedScale.x == 1. && renderMappedScale.y == 1.) ) );
        if (endSequenceRender_public(time, time, time, false, renderMappedScale,
                                     isSequentialRender,
                                     isRenderMadeInResponseToUserInteraction,
                                     view) == eStatusFailed) {
            renderStatus = eRenderingFunctorRetFailed;
        }
    }
    
    if (renderStatus != eRenderingFunctorRetOK) {
        retCode = eRenderRoIStatusRenderFailed;
    }

    return retCode;
} // renderRoIInternal

EffectInstance::RenderingFunctorRetEnum
EffectInstance::tiledRenderingFunctor( TiledRenderingFunctorArgs& args, const RectToRender& specificData, const QThread* callingThread)
{
   return tiledRenderingFunctor(callingThread,
                                args.frameArgs,
                                specificData,
                                args.frameTLS,
                                args.renderFullScaleThenDownscale,
                                args.renderUseScaleOneInputs,
                                args.isSequentialRender,
                                args.isRenderResponseToUserInteraction,
                                args.firstFrame,
                                args.lastFrame,
                                args.preferredInput,
                                args.mipMapLevel,
                                args.renderMappedMipMapLevel,
                                args.rod,
                                args.time,
                                args.view,
                                args.par,
                                args.byPassCache,
                                args.outputClipPrefDepth,
                                args.outputClipPrefsComps,
                                args.processChannels,
                                args.planes);
}

EffectInstance::RenderingFunctorRetEnum
EffectInstance::tiledRenderingFunctor(const QThread* callingThread,
                                      const ParallelRenderArgs& frameArgs,
                                      const RectToRender& rectToRender,
                                      const std::map<boost::shared_ptr<Natron::Node>,ParallelRenderArgs >& frameTLS,
                                      bool renderFullScaleThenDownscale,
                                      bool renderUseScaleOneInputs,
                                      bool isSequentialRender,
                                      bool isRenderResponseToUserInteraction,
                                      int firstFrame,int lastFrame,
                                      int preferredInput,
                                      unsigned int mipMapLevel,
                                      unsigned int renderMappedMipMapLevel,
                                      const RectD& rod,
                                      double time,
                                      int view,
                                      const double par,
                                      bool byPassCache,
                                      Natron::ImageBitDepthEnum outputClipPrefDepth,
                                      const std::list<Natron::ImageComponents>& outputClipPrefsComps,
                                      bool* processChannels,
                                      ImagePlanesToRender& planes) // when MT, planes is a copy so there's is no data race
{
    assert(!rectToRender.rect.isNull());
    
    bool outputUseImage = renderFullScaleThenDownscale && renderUseScaleOneInputs;
    
    ///Make the thread-storage live as long as the render action is called if we're in a newly launched thread in eRenderSafetyFullySafeFrame mode
    boost::shared_ptr<ParallelRenderArgsSetter> scopedFrameArgs;
    if (!frameTLS.empty() && callingThread != QThread::currentThread()) {
        scopedFrameArgs.reset( new ParallelRenderArgsSetter(frameTLS));
    }
    
    ///We hold our input images in thread-storage, so that the getImage function can find them afterwards, even if the node doesn't cache its output.
    boost::shared_ptr<InputImagesHolder_RAII> inputImagesHolder;
    if (!rectToRender.imgs.empty()) {
        inputImagesHolder.reset(new InputImagesHolder_RAII(rectToRender.imgs,&_imp->inputImages));
    }
    
    /*
     * downscaledRectToRender is in bitmap mipmap level. If outputUseImage,
     * then mipmapLevel = 0, otherwise mipmapLevel = mipMapLevel (arg)
     */
    RectI downscaledRectToRender = rectToRender.rect;
    
    /*
     * renderMappedRectToRender is in renderMappedMipMapLevel
     */
    RectI renderMappedRectToRender = downscaledRectToRender;

    ///Upscale the RoI to a region in the full scale image so it is in canonical coordinates
    RectD canonicalRectToRender;
    downscaledRectToRender.toCanonical(mipMapLevel, par, rod, &canonicalRectToRender);
    if (!outputUseImage && mipMapLevel > 0 && renderMappedMipMapLevel != mipMapLevel) {
        canonicalRectToRender.toPixelEnclosing(renderMappedMipMapLevel, par, &renderMappedRectToRender);
    }
    
    const PlaneToRender& firstPlaneToRender = planes.planes.begin()->second;
    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    RectI  renderBounds = firstPlaneToRender.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
# endif
    
    bool isBeingRenderedElseWhere = false;
    ///At this point if we're in eRenderSafetyFullySafeFrame mode, we are a thread that might have been launched way after
    ///the time renderRectToRender was computed. We recompute it to update the portion to render.
    ///Note that if it is bigger than the initial rectangle, we don't render the bigger rectangle since we cannot
    ///now make the preliminaries call to handle that region (getRoI etc...) so just stick with the old rect to render
    
    // check the bitmap!
    if (frameArgs.tilesSupported) {
        if (outputUseImage) {
            
            //The renderMappedImage is cached , read bitmap from it
            canonicalRectToRender.toPixelEnclosing(0, par, &downscaledRectToRender);
            downscaledRectToRender.intersect(firstPlaneToRender.renderMappedImage->getBounds(), &downscaledRectToRender);
            
            RectI initialRenderRect = downscaledRectToRender;
            
#if NATRON_ENABLE_TRIMAP
            if (!frameArgs.canAbort && frameArgs.isRenderResponseToUserInteraction) {
                downscaledRectToRender = firstPlaneToRender.renderMappedImage->getMinimalRect_trimap(downscaledRectToRender,&isBeingRenderedElseWhere);
            } else {
                downscaledRectToRender = firstPlaneToRender.renderMappedImage->getMinimalRect(downscaledRectToRender);
            }
#else
            reducedDownscaledRectToRender = renderMappedImage->getMinimalRect(renderRectToRender);
#endif
            
            ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
            ///we stick to what was requested
            if (!initialRenderRect.contains(downscaledRectToRender)) {
                downscaledRectToRender = initialRenderRect;
            }
            
            assert(downscaledRectToRender.isNull() || (renderBounds.x1 <= downscaledRectToRender.x1 && downscaledRectToRender.x2 <= renderBounds.x2 &&
                                                       renderBounds.y1 <= downscaledRectToRender.y1 && downscaledRectToRender.y2 <= renderBounds.y2));
            renderMappedRectToRender = downscaledRectToRender;
            
        } else {
            //The downscaled image is cached, read bitmap from it
#if NATRON_ENABLE_TRIMAP
            RectI downscaledRectToRenderMinimal;
            if (!frameArgs.canAbort && frameArgs.isRenderResponseToUserInteraction) {
                downscaledRectToRenderMinimal = firstPlaneToRender.downscaleImage->getMinimalRect_trimap(downscaledRectToRender,&isBeingRenderedElseWhere);
            } else {
                downscaledRectToRenderMinimal = firstPlaneToRender.downscaleImage->getMinimalRect(downscaledRectToRender);
            }
#else
            const RectI downscaledRectToRenderMinimal = downscaledImage->getMinimalRect(downscaledRectToRender);
#endif
            
            assert(downscaledRectToRenderMinimal.isNull() || (renderBounds.x1 <= downscaledRectToRenderMinimal.x1 && downscaledRectToRenderMinimal.x2 <= renderBounds.x2 &&
                                                              renderBounds.y1 <= downscaledRectToRenderMinimal.y1 && downscaledRectToRenderMinimal.y2 <= renderBounds.y2));
            
            
            
            if (renderFullScaleThenDownscale) {
                
                
                ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
                ///we stick to what was requested
                RectD canonicalrenderRectToRender;
                if (downscaledRectToRender.contains(downscaledRectToRenderMinimal)) {
                    downscaledRectToRenderMinimal.toCanonical(mipMapLevel, par, rod, &canonicalrenderRectToRender);
                    downscaledRectToRender = downscaledRectToRenderMinimal;
                } else {
                    downscaledRectToRender.toCanonical(mipMapLevel, par, rod, &canonicalrenderRectToRender);
                }
                canonicalrenderRectToRender.toPixelEnclosing(0, par, &renderMappedRectToRender);
                renderMappedRectToRender.intersect(firstPlaneToRender.renderMappedImage->getBounds(), &renderMappedRectToRender);
            } else {
                
                ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
                ///we stick to what was requested
                if (downscaledRectToRender.contains(downscaledRectToRenderMinimal)) {
                    downscaledRectToRender = downscaledRectToRenderMinimal;
                    renderMappedRectToRender = downscaledRectToRender;
                }
            }
            
        }
        
    } // tilesSupported
    ///It might have been already rendered now
    if (downscaledRectToRender.isNull()) {
        return isBeingRenderedElseWhere ? eRenderingFunctorRetTakeImageLock : eRenderingFunctorRetOK;
    }

    ///There cannot be the same thread running 2 concurrent instances of renderRoI on the same effect.
    assert(!_imp->renderArgs.hasLocalData() || !_imp->renderArgs.localData()._validArgs);
    
    
    
    Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs);
    scopedArgs.setArgs_firstPass(rod,
                                 renderMappedRectToRender,
                                 time,
                                 view,
                                 false, //< if we reached here the node is not an identity!
                                 0.,
                                 -1);
    
    
    
    ///The scoped args will maintain the args set for this thread during the
    ///whole time the render action is called, so they can be fetched in the
    ///getImage() call.
    /// @see EffectInstance::getImage
    scopedArgs.setArgs_secondPass(rectToRender.inputRois,firstFrame,lastFrame);
    RenderArgs & args = scopedArgs.getLocalData();
    
    ImagePtr originalInputImage, maskImage;
    Natron::ImagePremultiplicationEnum originalImagePremultiplication;
    InputImagesMap::const_iterator foundPrefInput = rectToRender.imgs.find(preferredInput);
    InputImagesMap::const_iterator foundMaskInput = rectToRender.imgs.end();
    
    if (isHostMaskingEnabled()) {
        foundMaskInput = rectToRender.imgs.find(getMaxInputCount() - 1);
    }
    if (foundPrefInput != rectToRender.imgs.end() && !foundPrefInput->second.empty()) {
        originalInputImage = foundPrefInput->second.front();
    }
    std::map<int,Natron::ImagePremultiplicationEnum>::const_iterator foundPrefPremult = planes.inputPremult.find(preferredInput);
    if (foundPrefPremult != planes.inputPremult.end() && originalInputImage) {
        originalImagePremultiplication = foundPrefPremult->second;
    } else {
        originalImagePremultiplication = Natron::eImagePremultiplicationOpaque;
    }
    
   
    if (foundMaskInput != rectToRender.imgs.end() && !foundMaskInput->second.empty()) {
        maskImage = foundMaskInput->second.front();
    }
    
#ifndef NDEBUG
    RenderScale scale;
    scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;
    // check the dimensions of all input and output images
    for (InputImagesMap::const_iterator it = rectToRender.imgs.begin();
         it != rectToRender.imgs.end();
         ++it) {
        for (ImageList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            assert(outputUseImage || (*it2)->getMipMapLevel() == mipMapLevel);
            const RectD & srcRodCanonical = (*it2)->getRoD();
            RectI srcBounds;
            srcRodCanonical.toPixelEnclosing((*it2)->getMipMapLevel(), (*it2)->getPixelAspectRatio(), &srcBounds); // compute srcRod at level 0
            const RectD & dstRodCanonical = firstPlaneToRender.renderMappedImage->getRoD();
            RectI dstBounds;
            dstRodCanonical.toPixelEnclosing(firstPlaneToRender.renderMappedImage->getMipMapLevel(), par, &dstBounds); // compute dstRod at level 0
            
            if (!frameArgs.tilesSupported) {
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
                RectI srcRealBounds = (*it2)->getBounds();
                RectI dstRealBounds = firstPlaneToRender.renderMappedImage->getBounds();
                
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
            
        } // end for
    } //end for
    
    if (supportsRenderScaleMaybe() == eSupportsNo) {
        assert(firstPlaneToRender.renderMappedImage->getMipMapLevel() == 0);
        assert(renderMappedMipMapLevel == 0);
    }
#     endif // DEBUG
    
    RenderingFunctorRetEnum handlerRet =  renderHandler(args,
                                                        frameArgs,
                                                        rectToRender.imgs,
                                                        rectToRender.isIdentity,
                                                        rectToRender.identityTime,
                                                        rectToRender.identityInput,
                                                        renderFullScaleThenDownscale,
                                                        renderUseScaleOneInputs,
                                                        isSequentialRender,
                                                        isRenderResponseToUserInteraction,
                                                        renderMappedRectToRender,
                                                        downscaledRectToRender,
                                                        byPassCache,
                                                        outputClipPrefDepth,
                                                        outputClipPrefsComps,
                                                        processChannels,
                                                        originalInputImage,
                                                        maskImage,
                                                        originalImagePremultiplication,
                                                        planes);
    if (handlerRet == eRenderingFunctorRetOK) {
        if (isBeingRenderedElseWhere) {
            return eRenderingFunctorRetTakeImageLock;
        } else {
            return eRenderingFunctorRetOK;
        }
    } else {
        return handlerRet;
    }
}

EffectInstance::RenderingFunctorRetEnum
EffectInstance::renderHandler(RenderArgs & args,
                              const ParallelRenderArgs& frameArgs,
                              const InputImagesMap& inputImages,
                              bool identity,
                              double identityTime,
                              Natron::EffectInstance* identityInput,
                              bool renderFullScaleThenDownscale,
                              bool renderUseScaleOneInputs,
                              bool isSequentialRender,
                              bool isRenderResponseToUserInteraction,
                              const RectI& renderMappedRectToRender,
                              const RectI & downscaledRectToRender,
                              bool byPassCache,
                              Natron::ImageBitDepthEnum outputClipPrefDepth,
                              const std::list<Natron::ImageComponents>& outputClipPrefsComps,
                              bool* processChannels,
                              const boost::shared_ptr<Natron::Image>& originalInputImage,
                              const boost::shared_ptr<Natron::Image>& maskImage,
                              Natron::ImagePremultiplicationEnum originalImagePremultiplication,
                              ImagePlanesToRender& planes)
{
    boost::shared_ptr<TimeLapse> timeRecorder;
    if (frameArgs.stats) {
        timeRecorder.reset(new TimeLapse());
    }
    
    const PlaneToRender& firstPlane = planes.planes.begin()->second;
    
    const double time = args._time;
    int mipMapLevel = firstPlane.downscaleImage->getMipMapLevel();
    const int view = args._view;

    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    RectI  renderBounds = firstPlane.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
# endif
    
    bool outputUseImage = renderUseScaleOneInputs && renderFullScaleThenDownscale;
    
    RenderActionArgs actionArgs;
    actionArgs.byPassCache = byPassCache;
    for (int i = 0; i < 4; ++i) {
        actionArgs.processChannels[i] = processChannels[i];
    }
    actionArgs.mappedScale.x = actionArgs.mappedScale.y = Image::getScaleFromMipMapLevel( firstPlane.renderMappedImage->getMipMapLevel() );
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(actionArgs.mappedScale.x == 1. && actionArgs.mappedScale.y == 1.) ) );
    actionArgs.originalScale.x = firstPlane.downscaleImage->getScale();
    actionArgs.originalScale.y = actionArgs.originalScale.x;
    actionArgs.draftMode = frameArgs.draftMode;
    
    std::list<std::pair<ImageComponents,ImagePtr> > tmpPlanes;
    bool multiPlanar = isMultiPlanar();

    actionArgs.roi = renderMappedRectToRender;
    
    assert(!outputClipPrefsComps.empty());
    
    if (identity) {
        
        std::list<Natron::ImageComponents> comps;
        for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
            comps.push_back(it->second.renderMappedImage->getComponents());
        }
        assert(!comps.empty());
        ImageList identityPlanes;
        RenderRoIArgs renderArgs(identityTime,
                                 actionArgs.originalScale,
                                 mipMapLevel,
                                 view,
                                 false,
                                 downscaledRectToRender,
                                 RectD(),
                                 comps,
                                 outputClipPrefDepth,
                                 this);
        if (!identityInput) {
            for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                if (outputUseImage) {
                    it->second.fullscaleImage->fillZero(downscaledRectToRender);
                    it->second.fullscaleImage->markForRendered(downscaledRectToRender);
                } else {
                    it->second.downscaleImage->fillZero(downscaledRectToRender);
                    it->second.downscaleImage->markForRendered(downscaledRectToRender);
                }
                if (frameArgs.stats && frameArgs.stats->isInDepthProfilingEnabled()) {
                    frameArgs.stats->addRenderInfosForNode(getNode(),  NodePtr(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation());
                }
            }
            
            return eRenderingFunctorRetOK;
        } else {
        
            EffectInstance::RenderRoIRetCode renderOk = identityInput->renderRoI(renderArgs, &identityPlanes);
            if (renderOk == eRenderRoIRetCodeAborted) {
                return eRenderingFunctorRetAborted;
            } else if (renderOk == eRenderRoIRetCodeFailed) {
                return eRenderingFunctorRetFailed;
            } else if (identityPlanes.empty()) {
                for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                    if (outputUseImage) {
                        it->second.fullscaleImage->fillZero(downscaledRectToRender);
                        it->second.fullscaleImage->markForRendered(downscaledRectToRender);
                    } else {
                        it->second.downscaleImage->fillZero(downscaledRectToRender);
                        it->second.downscaleImage->markForRendered(downscaledRectToRender);
                    }
                    if (frameArgs.stats && frameArgs.stats->isInDepthProfilingEnabled()) {
                        frameArgs.stats->addRenderInfosForNode(getNode(),  identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation());
                    }
                }
                return eRenderingFunctorRetOK;
            } else {
                
                assert(identityPlanes.size() == planes.planes.size());
                
                ImageList::iterator idIt = identityPlanes.begin();
                for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it,++idIt) {
                    
                    if (outputUseImage && (*idIt)->getMipMapLevel() > it->second.fullscaleImage->getMipMapLevel()) {
                        
                        if (!(*idIt)->getBounds().contains(downscaledRectToRender)) {
                            ///Fill the RoI with 0's as the identity input image might have bounds contained into the RoI
                            it->second.fullscaleImage->fillZero(downscaledRectToRender);
                        }
                        
                        ///Convert format first if needed
                        ImagePtr sourceImage;
                        if (it->second.fullscaleImage->getComponents() != (*idIt)->getComponents() || it->second.fullscaleImage->getBitDepth() != (*idIt)->getBitDepth()) {
                            sourceImage.reset(new Image(it->second.fullscaleImage->getComponents(),(*idIt)->getRoD(),(*idIt)->getBounds(),
                                                        (*idIt)->getMipMapLevel(),(*idIt)->getPixelAspectRatio(),it->second.fullscaleImage->getBitDepth(),false));
                            Natron::ViewerColorSpaceEnum colorspace = getApp()->getDefaultColorSpaceForBitDepth((*idIt)->getBitDepth());
                            Natron::ViewerColorSpaceEnum dstColorspace = getApp()->getDefaultColorSpaceForBitDepth(it->second.fullscaleImage->getBitDepth());
                            (*idIt)->convertToFormat((*idIt)->getBounds(), colorspace, dstColorspace, 3, false, false, sourceImage.get());
                        } else {
                            sourceImage = *idIt;
                        }
                        
                        ///then upscale
                        const RectD& rod = sourceImage->getRoD();
                        RectI bounds;
                        rod.toPixelEnclosing(it->second.renderMappedImage->getMipMapLevel(), it->second.renderMappedImage->getPixelAspectRatio(), &bounds);
                        ImagePtr  inputPlane(new Image(it->first,rod,bounds,it->second.renderMappedImage->getMipMapLevel(),
                                                   it->second.renderMappedImage->getPixelAspectRatio(),it->second.renderMappedImage->getBitDepth(),false));
                        sourceImage->upscaleMipMap(sourceImage->getBounds(), sourceImage->getMipMapLevel(), inputPlane->getMipMapLevel(), inputPlane.get());
                        it->second.fullscaleImage->pasteFrom(*inputPlane,downscaledRectToRender, false);
                        it->second.fullscaleImage->markForRendered(downscaledRectToRender);
                    } else {
                        
                        if (!(*idIt)->getBounds().contains(downscaledRectToRender)) {
                            ///Fill the RoI with 0's as the identity input image might have bounds contained into the RoI
                            it->second.downscaleImage->fillZero(downscaledRectToRender);
                        }
                        
                        ///Convert format if needed or copy
                        if (it->second.downscaleImage->getComponents() != (*idIt)->getComponents() || it->second.downscaleImage->getBitDepth() != (*idIt)->getBitDepth()) {
                            Natron::ViewerColorSpaceEnum colorspace = getApp()->getDefaultColorSpaceForBitDepth((*idIt)->getBitDepth());
                            Natron::ViewerColorSpaceEnum dstColorspace = getApp()->getDefaultColorSpaceForBitDepth(it->second.fullscaleImage->getBitDepth());
                            
                            RectI convertWindow;
                            (*idIt)->getBounds().intersect(downscaledRectToRender, &convertWindow);
                            (*idIt)->convertToFormat(convertWindow, colorspace, dstColorspace, 3, false, false, it->second.downscaleImage.get());
                        } else {
                            it->second.downscaleImage->pasteFrom(**idIt,downscaledRectToRender, false);
                        }
                        it->second.downscaleImage->markForRendered(downscaledRectToRender);
                    }
                    
                    if (frameArgs.stats && frameArgs.stats->isInDepthProfilingEnabled() ) {
                        frameArgs.stats->addRenderInfosForNode(getNode(),  identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation());
                    }
                    
                }
                return eRenderingFunctorRetOK;
            } // if (renderOk == eRenderRoIRetCodeAborted) {
        }  //  if (!identityInput) {
        
    } // if (identity) {
    
    args._outputPlanes = planes.planes;
    for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = args._outputPlanes.begin(); it != args._outputPlanes.end(); ++it) {
        /*
         * When using the cache, allocate a local temporary buffer onto which the plug-in will render, and then safely
         * copy this buffer to the shared (among threads) image.
         * This is also needed if the plug-in does not support the number of components of the renderMappedImage
         */
        Natron::ImageComponents prefComp;
        if (multiPlanar) {
            prefComp = getNode()->findClosestSupportedComponents(-1, it->second.renderMappedImage->getComponents());
        } else {
            prefComp = Node::findClosestInList(it->second.renderMappedImage->getComponents(), outputClipPrefsComps, multiPlanar);
            
        }
        
        if ((it->second.renderMappedImage->usesBitMap() || prefComp != it->second.renderMappedImage->getComponents() ||
            outputClipPrefDepth != it->second.renderMappedImage->getBitDepth()) && !isPaintingOverItselfEnabled()) {
            it->second.tmpImage.reset(new Image(prefComp,
                                                it->second.renderMappedImage->getRoD(),
                                                actionArgs.roi,
                                                it->second.renderMappedImage->getMipMapLevel(),
                                                it->second.renderMappedImage->getPixelAspectRatio(),
                                                outputClipPrefDepth,
                                                false)); //< no bitmap
            
        } else {
            it->second.tmpImage = it->second.renderMappedImage;
        }
        tmpPlanes.push_back(std::make_pair(it->second.renderMappedImage->getComponents(),it->second.tmpImage));
    }
    
    
#if NATRON_ENABLE_TRIMAP
    if (!frameArgs.canAbort && frameArgs.isRenderResponseToUserInteraction) {
        for (std::map<Natron::ImageComponents,PlaneToRender>::iterator it = args._outputPlanes.begin(); it != args._outputPlanes.end(); ++it) {
            if (outputUseImage) {
                it->second.fullscaleImage->markForRendering(downscaledRectToRender);
            } else {
                it->second.downscaleImage->markForRendering(downscaledRectToRender);
            }
        }
        
    }
#endif
    
    
    /// Render in the temporary image
    
    
    actionArgs.time = time;
    actionArgs.view = view;
    actionArgs.isSequentialRender = isSequentialRender;
    actionArgs.isRenderResponseToUserInteraction = isRenderResponseToUserInteraction;
    actionArgs.inputImages = inputImages;
    
    std::list< std::list<std::pair<ImageComponents,ImagePtr> > > planesLists;
    if (!multiPlanar) {
        for (std::list<std::pair<ImageComponents,ImagePtr> >::iterator it = tmpPlanes.begin(); it != tmpPlanes.end(); ++it) {
            std::list<std::pair<ImageComponents,ImagePtr> > tmp;
            tmp.push_back(*it);
            planesLists.push_back(tmp);
        }
    } else {
        planesLists.push_back(tmpPlanes);
    }
    
    bool renderAborted = false;
    std::map<Natron::ImageComponents,PlaneToRender> outputPlanes;
    for (std::list<std::list<std::pair<ImageComponents,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {
        if (!multiPlanar) {
            assert(!it->empty());
            args._outputPlaneBeingRendered = it->front().first;
        }
        actionArgs.outputPlanes = *it;
        
        Natron::StatusEnum st = render_public(actionArgs);
        
        renderAborted = aborted();
        
        /*
         * Since new planes can have been allocated on the fly by allocateImagePlaneAndSetInThreadLocalStorage(), refresh
         * the planes map from the thread local storage once the render action is finished
         */
        if (it == planesLists.begin()) {
            outputPlanes = args._outputPlanes;
            assert(!outputPlanes.empty());
        }
        
        if (st != eStatusOK || renderAborted) {
#if NATRON_ENABLE_TRIMAP
            if (!frameArgs.canAbort && frameArgs.isRenderResponseToUserInteraction) {
                
                /*
                 At this point, another thread might have already gotten this image from the cache and could end-up
                 using it while it has still pixels marked to PIXEL_UNAVAILABLE, hence clear the bitmap
                 */
                for (std::map<ImageComponents,PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
                    if (outputUseImage) {
                        it->second.fullscaleImage->clearBitmap(downscaledRectToRender);
                    } else {
                        it->second.downscaleImage->clearBitmap(downscaledRectToRender);
                    }
                }
                
            }
#endif
            
            return st != eStatusOK ? eRenderingFunctorRetFailed : eRenderingFunctorRetAborted;
            
            
        } // if (st != eStatusOK || renderAborted) {
    } // for (std::list<std::list<std::pair<ImageComponents,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)

    assert(!renderAborted);
    
    bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = isHostMaskingEnabled() || isHostMixingEnabled();
    double mix = useMaskMix ? getNode()->getHostMixingValue(time) : 1.;
    bool doMask = useMaskMix ? getNode()->isMaskEnabled(getMaxInputCount() - 1) : false;
    
    //Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImageComponents,PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
        
        bool unPremultRequired = unPremultIfNeeded && it->second.tmpImage->getComponentsCount() == 4 && it->second.renderMappedImage->getComponentsCount() == 3;
        
        if (frameArgs.doNansHandling && it->second.tmpImage->checkForNaNs(actionArgs.roi)) {
            QString warning(getNode()->getScriptName_mt_safe().c_str());
            warning.append(": ");
            warning.append(tr("rendered rectangle ("));
            warning.append(QString::number(actionArgs.roi.x1));
            warning.append(',');
            warning.append(QString::number(actionArgs.roi.y1));
            warning.append(")-(");
            warning.append(QString::number(actionArgs.roi.x2));
            warning.append(',');
            warning.append(QString::number(actionArgs.roi.y2));
            warning.append(") ");
            warning.append(tr("contains NaN values. They have been converted to 1."));
            setPersistentMessage(Natron::eMessageTypeWarning, warning.toStdString());
        }
        if (it->second.isAllocatedOnTheFly) {
            ///Plane allocated on the fly only have a temp image if using the cache and it is defined over the render window only
            if (it->second.tmpImage != it->second.renderMappedImage) {
                assert(it->second.tmpImage->getBounds() == actionArgs.roi);
                
                if (it->second.renderMappedImage->getComponents() != it->second.tmpImage->getComponents() ||
                    it->second.renderMappedImage->getBitDepth() != it->second.tmpImage->getBitDepth()) {
                    
                    it->second.tmpImage->convertToFormat(it->second.tmpImage->getBounds(),
                                                         getApp()->getDefaultColorSpaceForBitDepth(it->second.tmpImage->getBitDepth()),
                                                         getApp()->getDefaultColorSpaceForBitDepth(it->second.renderMappedImage->getBitDepth()),
                                                         -1, false, unPremultRequired, it->second.renderMappedImage.get());
                } else {
                    it->second.renderMappedImage->pasteFrom(*(it->second.tmpImage), it->second.tmpImage->getBounds(), false);
                }
            }
            it->second.renderMappedImage->markForRendered(actionArgs.roi);
            
        } else {
            
            if (renderFullScaleThenDownscale) {
                ///copy the rectangle rendered in the full scale image to the downscaled output
                
                /* If we're using renderUseScaleOneInputs, the full scale image is cached.
                 We might have been asked to render only a portion.
                 Hence we're not sure that the whole part of the image will be downscaled.
                 Instead we do all the downscale at once at the end of renderRoI().
                 If !renderUseScaleOneInputs the image is not cached.
                 Hence we know it will be rendered completly so it is safe to do this here and take advantage of the multi-threading.*/
                if (mipMapLevel != 0 && !renderUseScaleOneInputs) {
                    
                    assert(it->second.fullscaleImage != it->second.downscaleImage && it->second.renderMappedImage == it->second.fullscaleImage);
                    
                    
                    if (it->second.downscaleImage->getComponents() != it->second.tmpImage->getComponents() ||
                        it->second.downscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth()) {
                        
                        /*
                         * BitDepth/Components conversion required as well as downscaling, do conversion to a tmp buffer
                         */
                        ImagePtr tmp(new Image(it->second.downscaleImage->getComponents(), it->second.tmpImage->getRoD(), it->second.tmpImage->getBounds(), mipMapLevel,it->second.tmpImage->getPixelAspectRatio(), it->second.downscaleImage->getBitDepth(), false) );
                        
                        it->second.tmpImage->convertToFormat(it->second.tmpImage->getBounds(),
                                                             getApp()->getDefaultColorSpaceForBitDepth(it->second.tmpImage->getBitDepth()),
                                                             getApp()->getDefaultColorSpaceForBitDepth(it->second.downscaleImage->getBitDepth()),
                                                             -1, false, unPremultRequired, tmp.get());
                        tmp->downscaleMipMap(it->second.tmpImage->getRoD(),
                                             actionArgs.roi, 0, mipMapLevel, false,it->second.downscaleImage.get() );
                    } else {
                        
                        /*
                         *  Downscaling required only
                         */
                        it->second.tmpImage->downscaleMipMap(it->second.tmpImage->getRoD(),
                                                             actionArgs.roi, 0, mipMapLevel, false,it->second.downscaleImage.get() );
                        
                    }
                    
                    it->second.downscaleImage->copyUnProcessedChannels(downscaledRectToRender, planes.outputPremult, originalImagePremultiplication,  processChannels, originalInputImage);
                    if (useMaskMix) {
                        it->second.downscaleImage->applyMaskMix(downscaledRectToRender, maskImage.get(), originalInputImage.get(), doMask, false, mix);
                    }
                    it->second.downscaleImage->markForRendered(downscaledRectToRender);
                } else { // if (mipMapLevel != 0 && !renderUseScaleOneInputs) {
                    
                    assert(it->second.renderMappedImage == it->second.fullscaleImage);
                    if (it->second.tmpImage != it->second.renderMappedImage) {
                        
                        if (it->second.fullscaleImage->getComponents() != it->second.tmpImage->getComponents() ||
                            it->second.fullscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth()) {
                            
                            /*
                             * BitDepth/Components conversion required
                             */
                            
                            it->second.tmpImage->copyUnProcessedChannels(it->second.tmpImage->getBounds(), planes.outputPremult, originalImagePremultiplication, processChannels, originalInputImage);
                            if (useMaskMix) {
                                it->second.tmpImage->applyMaskMix(actionArgs.roi, maskImage.get(), originalInputImage.get(), doMask, false, mix);
                            }
                            it->second.tmpImage->convertToFormat(it->second.tmpImage->getBounds(),
                                                                 getApp()->getDefaultColorSpaceForBitDepth(it->second.tmpImage->getBitDepth()),
                                                                 getApp()->getDefaultColorSpaceForBitDepth(it->second.fullscaleImage->getBitDepth()),
                                                                 -1, false, unPremultRequired, it->second.fullscaleImage.get());
                            
                        } else {
                            
                            /*
                             * No conversion required, copy to output
                             */
                            int prefInput = getNode()->getPreferredInput();
                            RectI roiPixel;
                            ImagePtr originalInputImageFullScale;
                            if (prefInput != -1) {
                                originalInputImageFullScale = getImage(prefInput, time, actionArgs.mappedScale, view, NULL, originalInputImage->getComponents(), originalInputImage->getBitDepth(), originalInputImage->getPixelAspectRatio(), false, &roiPixel);
                            }
                            
                            
                            if (originalInputImageFullScale) {
                                
                                it->second.fullscaleImage->copyUnProcessedChannels(actionArgs.roi,planes.outputPremult, originalImagePremultiplication,  processChannels, originalInputImageFullScale);
                                if (useMaskMix) {
                                    ImagePtr originalMaskFullScale = getImage(getMaxInputCount() - 1, time, actionArgs.mappedScale, view, NULL, ImageComponents::getAlphaComponents(), originalInputImage->getBitDepth(), originalInputImage->getPixelAspectRatio(), false, &roiPixel);
                                    if (originalMaskFullScale) {
                                        it->second.fullscaleImage->applyMaskMix(actionArgs.roi, originalMaskFullScale.get(), originalInputImageFullScale.get(), doMask, false, mix);
                                    }
                                }
                            }
                            it->second.fullscaleImage->pasteFrom(*it->second.tmpImage, actionArgs.roi, false);
                        }
                        
                        
                    }
                    it->second.fullscaleImage->markForRendered(actionArgs.roi);
                }
            } else { // if (renderFullScaleThenDownscale) {
                
                ///Copy the rectangle rendered in the downscaled image
                if (it->second.tmpImage != it->second.downscaleImage) {
                    
                    
                    if (it->second.downscaleImage->getComponents() != it->second.tmpImage->getComponents() ||
                        it->second.downscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth()) {
                        
                        /*
                         * BitDepth/Components conversion required
                         */
                        
                        
                        it->second.tmpImage->convertToFormat(it->second.tmpImage->getBounds(),
                                                             getApp()->getDefaultColorSpaceForBitDepth(it->second.tmpImage->getBitDepth()),
                                                             getApp()->getDefaultColorSpaceForBitDepth(it->second.downscaleImage->getBitDepth()),
                                                             -1, false, unPremultRequired, it->second.downscaleImage.get());
                    } else {
                        
                        /*
                         * No conversion required, copy to output
                         */
                        
                        it->second.downscaleImage->pasteFrom(*(it->second.tmpImage), it->second.downscaleImage->getBounds(), false);
                    }
                    
                }
                
                it->second.downscaleImage->copyUnProcessedChannels(actionArgs.roi,planes.outputPremult, originalImagePremultiplication, processChannels, originalInputImage);
                if (useMaskMix) {
                    it->second.downscaleImage->applyMaskMix(actionArgs.roi, maskImage.get(), originalInputImage.get(), doMask, false, mix);
                }
                it->second.downscaleImage->markForRendered(downscaledRectToRender);
                
            } // if (renderFullScaleThenDownscale) {
        } // if (it->second.isAllocatedOnTheFly) {
        
        if (frameArgs.stats && frameArgs.stats->isInDepthProfilingEnabled()) {
            frameArgs.stats->addRenderInfosForNode(getNode(),  NodePtr(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation());
        }
        
    } // for (std::map<ImageComponents,PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
    
    
    return eRenderingFunctorRetOK;
} // tiledRenderingFunctor

ImagePtr
EffectInstance::allocateImagePlaneAndSetInThreadLocalStorage(const Natron::ImageComponents& plane)
{
    /*
     * The idea here is that we may have asked the plug-in to render say motion.forward, but it can only render both fotward
     * and backward at a time.
     * So it needs to allocate motion.backward and store it in the cache for efficiency.
     * Note that when calling this, the plug-in is already in the render action, hence in case of Host frame threading,
     * this function will be called as many times as there were thread used by the host frame threading.
     * For all other planes, there was a local temporary image, shared among all threads for the calls to render.
     * Since we may be in a thread of the host frame threading, only allocate a temporary image of the size of the rectangle
     * to render and mark that we're a plane allocated on the fly so that the tiledRenderingFunctor can know this is a plane
     * to handle specifically.
     */
    
    if (_imp->renderArgs.hasLocalData()) {
        RenderArgs& args = _imp->renderArgs.localData();
        if (args._validArgs) {
            
            assert(!args._outputPlanes.empty());
            
            const PlaneToRender& firstPlane = args._outputPlanes.begin()->second;
            
            bool useCache = firstPlane.fullscaleImage->usesBitMap() || firstPlane.downscaleImage->usesBitMap();
            
            const ImagePtr& img = firstPlane.fullscaleImage->usesBitMap() ? firstPlane.fullscaleImage : firstPlane.downscaleImage;
            
            boost::shared_ptr<ImageParams> params = img->getParams();
            
            PlaneToRender p;
            bool ok = allocateImagePlane(img->getKey(), img->getRoD(), img->getBounds(), img->getBounds(), false, params->getFramesNeeded(), plane, img->getBitDepth(), img->getPixelAspectRatio(), img->getMipMapLevel(), false, false, false, useCache, &p.fullscaleImage, &p.downscaleImage);
            if (!ok) {
                return ImagePtr();
            } else {
                
                p.renderMappedImage = p.downscaleImage;
                p.isAllocatedOnTheFly = true;
                
                /*
                 * Allocate a temporary image for rendering only if using cache
                 */
                if (useCache) {
                    p.tmpImage.reset(new Image(p.renderMappedImage->getComponents(),
                                               p.renderMappedImage->getRoD(),
                                               args._renderWindowPixel,
                                               p.renderMappedImage->getMipMapLevel(),
                                               p.renderMappedImage->getPixelAspectRatio(),
                                               p.renderMappedImage->getBitDepth(),
                                               false));
                } else {
                    p.tmpImage = p.renderMappedImage;
                }
                args._outputPlanes.insert(std::make_pair(plane,p));
                return p.downscaleImage;
            }
            
        } else {
            return ImagePtr();
        }
    } else {
        return ImagePtr();
    }
} // allocateImagePlaneAndSetInThreadLocalStorage

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
EffectInstance::evaluate(KnobI* knob,
                         bool isSignificant,
                         Natron::ValueChangedReasonEnum /*reason*/)
{

    ////If the node is currently modifying its input, to ask for a render
    ////because at then end of the inputChanged handler, it will ask for a refresh
    ////and a rebuild of the inputs tree.
    NodePtr node = getNode();
    if ( node->duringInputChangedAction() ) {
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
                if ( node->hasSequentialOnlyNodeUpstream(sequentialNode) ) {
                    if (node->getApp()->getProject()->getProjectViewsCount() > 1) {
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
                getApp()->startWritersRendering(getApp()->isRenderStatsActionChecked(),works);

                return;
            }
        }
    }

    ///increments the knobs age following a change
    if (!button && isSignificant) {
        node->incrementKnobsAge();
    }
    
    
    double time = getCurrentTime();
    
    
    std::list<ViewerInstance* > viewers;
    node->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        if (isSignificant) {
            (*it)->renderCurrentFrame(true);
        } else {
            (*it)->redrawViewer();
        }
    }
    
    getNode()->refreshPreviewsRecursivelyDownstream(time);
} // evaluate

bool
EffectInstance::message(Natron::MessageTypeEnum type,
                        const std::string & content) const
{
    return getNode()->message(type,content);
}

void
EffectInstance::setPersistentMessage(Natron::MessageTypeEnum type,
                                     const std::string & content)
{
    getNode()->setPersistentMessage(type, content);
}

void
EffectInstance::clearPersistentMessage(bool recurse)
{
    getNode()->clearPersistentMessage(recurse);
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
    NodePtr node = getNode();
    if (node) {
        node->onSetSupportRenderScaleMaybeSet((int)s);
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
    PluginMemory* ret = new PluginMemory( getNode()->getLiveInstance() ); //< hack to get "this" as a shared ptr
    bool wasntLocked = ret->alloc(nBytes);

    assert(wasntLocked);
    Q_UNUSED(wasntLocked);

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
    getNode()->registerPluginMemory(nBytes);
}

void
EffectInstance::unregisterPluginMemory(size_t nBytes)
{
    getNode()->unregisterPluginMemory(nBytes);
}

void
EffectInstance::onAllKnobsSlaved(bool isSlave,
                                 KnobHolder* master)
{
    getNode()->onAllKnobsSlaved(isSlave,master);
}

void
EffectInstance::onKnobSlaved(KnobI* slave,KnobI* master,
                             int dimension,
                             bool isSlave)
{
    getNode()->onKnobSlaved(slave,master,dimension,isSlave);
}

void
EffectInstance::setCurrentViewportForOverlays_public(OverlaySupport* viewport)
{
    getNode()->setCurrentViewportForDefaultOverlays(viewport);
    setCurrentViewportForOverlays(viewport);
}

void
EffectInstance::drawOverlay_public(double time,
                                   double scaleX,
                                   double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasDefaultOverlay() ) {
        return;
    }

    RECURSIVE_ACTION();

    _imp->setDuringInteractAction(true);
    drawOverlay(time, scaleX,scaleY);
    getNode()->drawDefaultOverlay(time, scaleX, scaleY);
    _imp->setDuringInteractAction(false);
}

bool
EffectInstance::onOverlayPenDown_public(double time,
                                        double scaleX,
                                        double scaleY,
                                        const QPointF & viewportPos,
                                        const QPointF & pos,
                                        double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }
    
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayPenDown(time, scaleX, scaleY, viewportPos, pos, pressure);
        if (!ret) {
            ret |= getNode()->onOverlayPenDownDefault(scaleX, scaleY, viewportPos, pos, pressure);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();
    
    return ret;
}

bool
EffectInstance::onOverlayPenMotion_public(double time,
                                          double scaleX,
                                          double scaleY,
                                          const QPointF & viewportPos,
                                          const QPointF & pos,
                                          double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }
    

    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenMotion(time, scaleX, scaleY, viewportPos, pos, pressure);
    if (!ret) {
        ret |= getNode()->onOverlayPenMotionDefault(scaleX, scaleY, viewportPos, pos, pressure);
    }
    _imp->setDuringInteractAction(false);
    //Don't chek if render is needed on pen motion, wait for the pen up

    //checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayPenUp_public(double time,
                                      double scaleX,
                                      double scaleY,
                                      const QPointF & viewportPos,
                                      const QPointF & pos,
                                      double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayPenUp(time, scaleX, scaleY, viewportPos, pos, pressure);
        if (!ret) {
            ret |= getNode()->onOverlayPenUpDefault(scaleX, scaleY, viewportPos, pos, pressure);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();
    
    return ret;
}

bool
EffectInstance::onOverlayKeyDown_public(double time,
                                        double scaleX,
                                        double scaleY,
                                        Natron::Key key,
                                        Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyDown(time, scaleX,scaleY,key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyDownDefault(scaleX, scaleY, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();
    
    return ret;
}

bool
EffectInstance::onOverlayKeyUp_public(double time,
                                      double scaleX,
                                      double scaleY,
                                      Natron::Key key,
                                      Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }
    
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyUp(time, scaleX, scaleY, key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyUpDefault(scaleX, scaleY, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyRepeat_public(double time,
                                          double scaleX,
                                          double scaleY,
                                          Natron::Key key,
                                          Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }
    
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyRepeat(time, scaleX,scaleY,key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyRepeatDefault(scaleX, scaleY, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();
    
    return ret;
}

bool
EffectInstance::onOverlayFocusGained_public(double time,
                                            double scaleX,
                                            double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasDefaultOverlay()  ) {
        return false;
    }
    
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayFocusGained(time, scaleX,scaleY);
        if (!ret) {
            ret |= getNode()->onOverlayFocusGainedDefault(scaleX, scaleY);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();
    
    return ret;
}

bool
EffectInstance::onOverlayFocusLost_public(double time,
                                          double scaleX,
                                          double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasDefaultOverlay()  ) {
        return false;
    }
    bool ret;
    {
        
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayFocusLost(time, scaleX,scaleY);
        if (!ret) {
            ret |= getNode()->onOverlayFocusLostDefault(scaleX, scaleY);
        }
        _imp->setDuringInteractAction(false);
    }
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
EffectInstance::render_public(const RenderActionArgs& args)
{
    NON_RECURSIVE_ACTION();
    EffectPointerThreadProperty_RAII propHolder_raii(this);
    return render(args);

}

Natron::StatusEnum
EffectInstance::getTransform_public(double time,
                                    const RenderScale& renderScale,
                                    int view,
                                    Natron::EffectInstance** inputToTransform,
                                    Transform::Matrix3x3* transform)
{
    RECURSIVE_ACTION();
    assert(getCanTransform());
    return getTransform(time, renderScale, view, inputToTransform, transform);
}

bool
EffectInstance::isIdentity_public(bool useIdentityCache, // only set to true when calling for the whole image (not for a subrect)
                                  U64 hash,
                                  double time,
                                  const RenderScale & scale,
                                  const RectI& renderWindow,
                                  int view,
                                  double* inputTime,
                                  int* inputNb)
{
    
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );
    
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);

    if (useIdentityCache) {
        double timeF = 0.;
        bool foundInCache = _imp->actionsCache.getIdentityResult(hash, time, view, mipMapLevel, inputNb, &timeF);
        if (foundInCache) {
            *inputTime = timeF;
            return *inputNb >= 0 || *inputNb == -2;
        }
    }
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
    boost::shared_ptr<RotoDrawableItem> rotoItem = getNode()->getAttachedRotoItem();
    if (rotoItem && !rotoItem->isActivated(time)) {
        ret = true;
        *inputNb = getNode()->getPreferredInput();
        *inputTime = time;
    } else if (appPTR->isBackground() && dynamic_cast<DiskCacheNode*>(this) != NULL) {
        ret = true;
        *inputNb = 0;
        *inputTime = time;
    } else if ( getNode()->isNodeDisabled() || !getNode()->hasAtLeastOneChannelToProcess()) {
        
        ret = true;
        *inputTime = time;
        *inputNb = -1;
        *inputNb = getNode()->getPreferredInput();
        
    } else {
        /// Don't call isIdentity if plugin is sequential only.
        if (getSequentialPreference() != Natron::eSequentialPreferenceOnlySequential) {
            try {
                ret = isIdentity(time, scale,renderWindow, view, inputTime, inputNb);
            } catch (...) {
                throw;
            }
        }
    }
    if (!ret) {
        *inputNb = -1;
        *inputTime = time;
    }
    
    if (useIdentityCache) {
        _imp->actionsCache.setIdentityResult(hash, time, view, mipMapLevel, *inputNb, *inputTime);
    }
    return ret;
    
}

void
EffectInstance::onInputChanged(int /*inputNo*/)
{
    if ( !getApp()->getProject()->isLoadingProject() ) {
        RenderScale s;
        s.x = s.y = 1.;
        checkOFXClipPreferences_public(getCurrentTime(), s, kOfxChangeUserEdited,true, true);
    }
}

Natron::StatusEnum
EffectInstance::getRegionOfDefinition_publicInternal(U64 hash,
                                                        double time,
                                                        const RenderScale & scale,
                                                        const RectI& renderWindow,
                                                        bool useRenderWindow,
                                                        int view,
                                                        RectD* rod,
                                                        bool* isProjectFormat)
{
    if (!isEffectCreated()) {
        return eStatusFailed;
    }
    
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    
    if (useRenderWindow) {
        double inputTimeIdentity;
        int inputNbIdentity;
        bool isIdentity = isIdentity_public(true, hash, time, scale, renderWindow, view, &inputTimeIdentity, &inputNbIdentity);
        if (isIdentity) {
            if (inputNbIdentity >= 0) {
                Natron::EffectInstance* input = getInput(inputNbIdentity);
                if (input) {
                    return input->getRegionOfDefinition_public(input->getRenderHash(), inputTimeIdentity, scale, view, rod, isProjectFormat);
                }
            } else if (inputNbIdentity == -2) {
                return getRegionOfDefinition_public(hash, inputTimeIdentity, scale, view, rod, isProjectFormat);
            } else {
                return eStatusFailed;
            }
        }
    }
    
    bool foundInCache = _imp->actionsCache.getRoDResult(hash, time, view, mipMapLevel, rod);
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
                //if (!isDuringStrokeCreation) {
                _imp->actionsCache.invalidateAll(hash);
                _imp->actionsCache.setRoDResult(hash, time, view, mipMapLevel, RectD());
                // }
                return ret;
            }
            
            if (rod->isNull()) {
                //if (!isDuringStrokeCreation) {
                _imp->actionsCache.invalidateAll(hash);
                _imp->actionsCache.setRoDResult(hash, time, view, mipMapLevel, RectD());
                //}
                return eStatusFailed;
            }
            
            assert( (ret == eStatusOK || ret == eStatusReplyDefault) && (rod->x1 <= rod->x2 && rod->y1 <= rod->y2) );
            
        }
        *isProjectFormat = ifInfiniteApplyHeuristic(hash,time, scale, view, rod);
        assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
        
        //if (!isDuringStrokeCreation) {
        _imp->actionsCache.setRoDResult(hash, time, view,  mipMapLevel, *rod);
        //}
        return ret;
    }

}

Natron::StatusEnum
EffectInstance::getRegionOfDefinitionWithIdentityCheck_public(U64 hash,
                                                              double time,
                                                              const RenderScale & scale,
                                                              const RectI& renderWindow,
                                                              int view,
                                                              RectD* rod,
                                                              bool* isProjectFormat)
{
    return getRegionOfDefinition_publicInternal(hash, time, scale, renderWindow, true, view, rod, isProjectFormat);
}

Natron::StatusEnum
EffectInstance::getRegionOfDefinition_public(U64 hash,
                                             double time,
                                             const RenderScale & scale,
                                             int view,
                                             RectD* rod,
                                             bool* isProjectFormat)
{
    return getRegionOfDefinition_publicInternal(hash, time, scale, RectI(), false, view, rod, isProjectFormat);
}

void
EffectInstance::getRegionsOfInterest_public(double time,
                                            const RenderScale & scale,
                                            const RectD & outputRoD, //!< effect RoD in canonical coordinates
                                            const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            int view,
                                            RoIMap* ret)
{
    NON_RECURSIVE_ACTION();
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);

    getRegionsOfInterest(time, scale, outputRoD, renderWindow, view,ret);
}

FramesNeededMap
EffectInstance::getFramesNeeded_public(U64 hash, double time,int view, unsigned int mipMapLevel)
{
    NON_RECURSIVE_ACTION();
    FramesNeededMap framesNeeded;
    bool foundInCache = _imp->actionsCache.getFramesNeededResult(hash, time, view, mipMapLevel, &framesNeeded);
    if (foundInCache) {
        return framesNeeded;
    }
    
    framesNeeded = getFramesNeeded(time, view);
    _imp->actionsCache.setFramesNeededResult(hash, time, view, mipMapLevel, framesNeeded);
    
    return framesNeeded;
}

void
EffectInstance::getFrameRange_public(U64 hash,
                                     double *first,
                                     double *last,
                                     bool bypasscache)
{
    
    double fFirst = 0.,fLast = 0.;
    bool foundInCache = false;
    if (!bypasscache) {
        foundInCache = _imp->actionsCache.getTimeDomainResult(hash, &fFirst, &fLast);
    }
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
        _imp->actionsCache.setTimeDomainResult(hash, *first, *last);
    }
}

Natron::StatusEnum
EffectInstance::beginSequenceRender_public(double first,
                                           double last,
                                           double step,
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
EffectInstance::endSequenceRender_public(double first,
                                         double last,
                                         double step,
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
                                     const Natron::ImageComponents& comp) const
{
    return getNode()->isSupportedComponent(inputNb, comp);
}

Natron::ImageBitDepthEnum
EffectInstance::getBitDepth() const
{
    return getNode()->getBitDepth();
}

bool
EffectInstance::isSupportedBitDepth(Natron::ImageBitDepthEnum depth) const
{
    return getNode()->isSupportedBitDepth(depth);
}

Natron::ImageComponents
EffectInstance::findClosestSupportedComponents(int inputNb,
                                               const Natron::ImageComponents& comp) const
{
    return getNode()->findClosestSupportedComponents(inputNb,comp);
}

void
EffectInstance::getPreferredDepthAndComponents(int inputNb,
                                               std::list<Natron::ImageComponents>* comp,
                                               Natron::ImageBitDepthEnum* depth) const
{
    EffectInstance* inp = 0;
    
    std::list<Natron::ImageComponents> inputComps;
    if (inputNb != -1) {
        inp = getInput(inputNb);
        if (inp) {
            Natron::ImageBitDepthEnum depth;
            inp->getPreferredDepthAndComponents(-1, &inputComps, &depth);
        }
    } else {
        
        int index = getNode()->getPreferredInput();
        if (index != -1) {
            Natron::EffectInstance* input = getInput(index);
            if (input) {
                Natron::ImageBitDepthEnum inputDepth;
                input->getPreferredDepthAndComponents(-1, &inputComps, &inputDepth);

            }
        }
    }
    if (inputComps.empty()) {
        inputComps.push_back(ImageComponents::getNoneComponents());
    }
    for (std::list<Natron::ImageComponents>::iterator it = inputComps.begin(); it != inputComps.end(); ++it) {
        comp->push_back(findClosestSupportedComponents(inputNb, *it));
    }
    

    ///find deepest bitdepth
    *depth = getBitDepth();
}

void
EffectInstance::clearActionsCache()
{
    _imp->actionsCache.clearAll();
}

void
EffectInstance::setComponentsAvailableDirty(bool dirty)
{
    QMutexLocker k(&_imp->componentsAvailableMutex);
    _imp->componentsAvailableDirty = dirty;
}

void
EffectInstance::getNonMaskInputsAvailableComponents(double time,
                                                    int view,
                                                    bool preferExistingComponents,
                                                    ComponentsAvailableMap* comps,
                                                    std::list<Natron::EffectInstance*>* markedNodes)
{
    
    NodePtr node  = getNode();
    if (!node) {
        return;
    }
    int preferredInput = node->getPreferredInput();
    
    //Call recursively on all inputs which are not masks
    int maxInputs = getMaxInputCount();
    for (int i = 0; i < maxInputs; ++i) {
        if (!isInputMask(i) && !isInputRotoBrush(i)) {
            EffectInstance* input = getInput(i);
            if (input) {
                ComponentsAvailableMap inputAvailComps;
                input->getComponentsAvailableRecursive(time, view, &inputAvailComps, markedNodes);
                for (ComponentsAvailableMap::iterator it = inputAvailComps.begin(); it!=inputAvailComps.end(); ++it) {
                    //If the component is already present in the 'comps' map, only add it if we are the preferred input
                    ComponentsAvailableMap::iterator colorMatch = comps->end();
                    bool found = false;
                    for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                        if (it2->first == it->first) {
                            if (i == preferredInput && !preferExistingComponents) {
                                it2->second = node;
                            }
                            found = true;
                            break;
                        } else if (it2->first.isColorPlane()) {
                            colorMatch = it2;
                        }
                    }
                    if (!found) {
                        if (colorMatch != comps->end() && it->first.isColorPlane()) {
                            //we found another color components type, skip
                            continue;
                        } else {
                            comps->insert(*it);
                        }
                    }
                }
            }
        }
    }
}

void
EffectInstance::getComponentsAvailableRecursive(double time, int view, ComponentsAvailableMap* comps,
                                                std::list<Natron::EffectInstance*>* markedNodes)
{
    if (std::find(markedNodes->begin(), markedNodes->end(), this) != markedNodes->end()) {
        return;
    }
    
    {
        QMutexLocker k(&_imp->componentsAvailableMutex);
        if (!_imp->componentsAvailableDirty) {
            comps->insert(_imp->outputComponentsAvailable.begin(), _imp->outputComponentsAvailable.end());
            return;
        }
    }
    
    
    NodePtr node  = getNode();
    if (!node) {
        return;
    }
    ComponentsNeededMap neededComps;
    SequenceTime ptTime;
    int ptView;
    NodePtr ptInput;
    bool processAll;
    bool processChannels[4];
    getComponentsNeededAndProduced_public(time, view, &neededComps, &processAll, &ptTime, &ptView, processChannels, &ptInput);
    
    
    ///If the plug-in is not pass-through, only consider the components processed by the plug-in in output,
    ///so we do not need to recurse.
    PassThroughEnum passThrough = isPassThroughForNonRenderedPlanes();
    if (passThrough == ePassThroughPassThroughNonRenderedPlanes || passThrough == ePassThroughRenderAllRequestedPlanes) {
        
        bool doHeuristicForPassThrough = false;
        if (isMultiPlanar()) {
            if (!ptInput) {
                doHeuristicForPassThrough = true;
            }
        } else {
            doHeuristicForPassThrough = true;
        }
        
        if (doHeuristicForPassThrough) {
            
            getNonMaskInputsAvailableComponents(time, view, false, comps, markedNodes);
            
        } else {
            if (ptInput) {
                ptInput->getLiveInstance()->getComponentsAvailableRecursive(time, view, comps, markedNodes);
            }
        }
        
        
        
    }
    if (processAll) {
        //The node makes available everything available upstream
        for (ComponentsAvailableMap::iterator it = comps->begin(); it != comps->end(); ++it) {
            if (it->second.lock()) {
                it->second = node;
            }
        }
    }


    
    ComponentsNeededMap::iterator foundOutput = neededComps.find(-1);
    if (foundOutput != neededComps.end()) {
        
        ///Foreach component produced by the node at the given (view,time),  try
        ///to add it to the components available. Since we already handled upstream nodes, it is probably
        ///already in there, in which case we mark that this node is producing the component instead
        for (std::vector<Natron::ImageComponents>::iterator it = foundOutput->second.begin();
             it != foundOutput->second.end(); ++it) {
            
            
            ComponentsAvailableMap::iterator alreadyExisting = comps->end();
            
            if (it->isColorPlane()) {
                
                ComponentsAvailableMap::iterator colorMatch = comps->end();
                
                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    } else if (it2->first.isColorPlane()) {
                        colorMatch = it2;
                    }
                }
                
                if (alreadyExisting == comps->end() && colorMatch != comps->end()) {
                    comps->erase(colorMatch);
                }
            } else {
                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    }
                }
            }
            
            
            if (alreadyExisting == comps->end()) {
                comps->insert(std::make_pair(*it, node));
            } else {
                //If the component already exists from upstream in the tree, mark that we produce it instead
                alreadyExisting->second = node;
            }
        }
        
        
        std::list<ImageComponents> userComps;
        node->getUserComponents(&userComps);
        ///Foreach user component, add it as an available component, but use this node only if it is also
        ///in the "needed components" list
        for (std::list<ImageComponents>::iterator it = userComps.begin(); it != userComps.end(); ++it) {
            
            bool found = false;
            for (std::vector<Natron::ImageComponents>::iterator it2 = foundOutput->second.begin();
                 it2 != foundOutput->second.end(); ++it2) {
                if (*it2 == *it) {
                    found = true;
                    break;
                }
            }
 
           
            ComponentsAvailableMap::iterator alreadyExisting = comps->end();
            
            if (it->isColorPlane()) {
                
                ComponentsAvailableMap::iterator colorMatch = comps->end();
                
                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    } else if (it2->first.isColorPlane()) {
                        colorMatch = it2;
                    }
                }
                
                if (alreadyExisting == comps->end() && colorMatch != comps->end()) {
                    comps->erase(colorMatch);
                }
            } else {
                alreadyExisting = comps->find(*it);
            }
            
            //If the component already exists from above in the tree, do not add it
            if (alreadyExisting == comps->end()) {
                comps->insert(std::make_pair(*it, found ? node : NodePtr() ));
            } else {
                alreadyExisting->second = node ;
            }

        }
    }
    markedNodes->push_back(this);
    
    
    {
        QMutexLocker k(&_imp->componentsAvailableMutex);
        _imp->componentsAvailableDirty = false;
        _imp->outputComponentsAvailable = *comps;
    }
    
    
    
}

void
EffectInstance::getComponentsAvailable(double time, ComponentsAvailableMap* comps, std::list<Natron::EffectInstance*>* markedNodes)
{
    getComponentsAvailableRecursive(time, 0, comps, markedNodes);
}

void
EffectInstance::getComponentsAvailable(double time, ComponentsAvailableMap* comps)
{
   
    //int nViews = getApp()->getProject()->getProjectViewsCount();
    
    ///Union components over all views
    //for (int view = 0; view < nViews; ++view) {
    ///Edit: Just call for 1 view, it should not matter as this should be view agnostic.
        std::list<Natron::EffectInstance*> marks;
        getComponentsAvailableRecursive(time, 0, comps, &marks);
        
    //}
   
}

void
EffectInstance::getComponentsNeededAndProduced(double time, int view,
                                    ComponentsNeededMap* comps,
                                    SequenceTime* passThroughTime,
                                    int* passThroughView,
                                    boost::shared_ptr<Natron::Node>* passThroughInput)
{
    *passThroughTime = time;
    *passThroughView = view;
    
    std::list<Natron::ImageComponents> outputComp;
    Natron::ImageBitDepthEnum outputDepth;
    getPreferredDepthAndComponents(-1, &outputComp , &outputDepth);
    
    std::vector<Natron::ImageComponents> outputCompVec;
    for (std::list<Natron::ImageComponents>::iterator it = outputComp.begin(); it != outputComp.end(); ++it) {
        outputCompVec.push_back(*it);
    }
    
    
    comps->insert(std::make_pair(-1, outputCompVec));
    
    NodePtr firstConnectedOptional;
    for (int i = 0; i < getMaxInputCount(); ++i) {
        NodePtr node = getNode()->getInput(i);
        if (!node) {
            continue;
        }
        if (isInputRotoBrush(i)) {
            continue;
        }
        
        std::list<Natron::ImageComponents> comp;
        Natron::ImageBitDepthEnum depth;
        getPreferredDepthAndComponents(-1, &comp, &depth);
        
        std::vector<Natron::ImageComponents> compVect;
        for (std::list<Natron::ImageComponents>::iterator it = comp.begin(); it != comp.end(); ++it) {
            compVect.push_back(*it);
        }
        comps->insert(std::make_pair(i, compVect));
        
        if (!isInputOptional(i)) {
            *passThroughInput = node;
        } else {
            firstConnectedOptional = node;
        }
    }
    if (!*passThroughInput) {
        *passThroughInput = firstConnectedOptional;
    }
    
}

void
EffectInstance::getComponentsNeededAndProduced_public(double time, int view,
                                                      ComponentsNeededMap* comps,
                                                      bool* processAllRequested,
                                                      SequenceTime* passThroughTime,
                                                      int* passThroughView,
                                                      bool* processChannels,
                                                      boost::shared_ptr<Natron::Node>* passThroughInput)

{
    RECURSIVE_ACTION();
    
    if (isMultiPlanar()) {
        processChannels[0] = processChannels[1] = processChannels[2] = processChannels[3] = true;
        getComponentsNeededAndProduced(time, view, comps, passThroughTime, passThroughView, passThroughInput);
        *processAllRequested = false;
    } else {
        *passThroughTime = time;
        *passThroughView = view;
        int idx = getNode()->getPreferredInput();
        *passThroughInput = getNode()->getInput(idx);
        
        {
            ImageComponents layer;
            std::vector<ImageComponents> compVec;
            bool ok = getNode()->getUserComponents(-1, processChannels,processAllRequested, &layer);
            if (ok) {
             
                if (!layer.isColorPlane() && layer.getNumComponents() != 0) {
                    compVec.push_back(layer);
                } else {
                    //Use regular clip preferences
                    ImageBitDepthEnum depth;
                    std::list<ImageComponents> components;
                    getPreferredDepthAndComponents(-1, &components, &depth);
                    for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                        //if (it->isColorPlane()) {
                            compVec.push_back(*it);
                        //}
                    }

                }
            } else  {
                //Use regular clip preferences
                ImageBitDepthEnum depth;
                std::list<ImageComponents> components;
                getPreferredDepthAndComponents(-1, &components, &depth);
                for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                    //if (it->isColorPlane()) {
                        compVec.push_back(*it);
                    //}
                }
            }
            comps->insert(std::make_pair(-1, compVec));

        }
        
        int maxInput = getMaxInputCount();
        for (int i = 0; i < maxInput; ++i) {
            EffectInstance* input = getInput(i);
            if (input) {
                std::vector<ImageComponents> compVec;
                bool inputProcChannels[4];
                ImageComponents layer;
                bool isAll;
                bool ok = getNode()->getUserComponents(i, inputProcChannels, &isAll, &layer);
                
                ImageComponents maskComp;
                NodePtr maskInput;
                int channelMask = getNode()->getMaskChannel(i, &maskComp,&maskInput);
                
                if (ok && !isAll) {
                    if (!layer.isColorPlane()) {
                        compVec.push_back(layer);
                    } else {
                        //Use regular clip preferences
                        ImageBitDepthEnum depth;
                        std::list<ImageComponents> components;
                        getPreferredDepthAndComponents(i, &components, &depth);
                        for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                            //if (it->isColorPlane()) {
                                compVec.push_back(*it);
                            //}
                        }
                        
                    }
                } else if (channelMask != -1 && maskComp.getNumComponents() > 0) {
                    std::vector<ImageComponents> compVec;
                    compVec.push_back(maskComp);
                    comps->insert(std::make_pair(i, compVec));
                    
                } else {
                    //Use regular clip preferences
                    ImageBitDepthEnum depth;
                    std::list<ImageComponents> components;
                    getPreferredDepthAndComponents(i, &components, &depth);
                    for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                        //if (it->isColorPlane()) {
                            compVec.push_back(*it);
                        //}
                    }
                }
                comps->insert(std::make_pair(i, compVec));
            }

        }
    }
}

int
EffectInstance::getMaskChannel(int inputNb,Natron::ImageComponents* comps,boost::shared_ptr<Natron::Node>* maskInput) const
{
    return getNode()->getMaskChannel(inputNb,comps,maskInput);
}

bool
EffectInstance::isMaskEnabled(int inputNb) const
{
    return getNode()->isMaskEnabled(inputNb);
}

void
EffectInstance::onKnobValueChanged(KnobI* /*k*/,
                                   Natron::ValueChangedReasonEnum /*reason*/,
                                   SequenceTime /*time*/,
                                   bool /*originatedFromMainThread*/)
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
    
    if (_imp->frameRenderArgs.hasLocalData()) {
        const ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.time;
        }
    }
    return getApp()->getTimeLine()->currentFrame();
}

bool
EffectInstance::getThreadLocalRenderedPlanes(std::map<Natron::ImageComponents,PlaneToRender> *outputPlanes,
                                             Natron::ImageComponents* planeBeingRendered,
                                             RectI* renderWindow) const
{
    if (_imp->renderArgs.hasLocalData()) {
        const RenderArgs& args = _imp->renderArgs.localData();
        if (args._validArgs) {
            assert(!args._outputPlanes.empty());
            *planeBeingRendered = args._outputPlaneBeingRendered;
            *outputPlanes = args._outputPlanes;
            *renderWindow = args._renderWindowPixel;
            return true;
        }
    }
    return false;
}

void
EffectInstance::updateThreadLocalRenderTime(double time)
{
    if (QThread::currentThread() != qApp->thread() && _imp->renderArgs.hasLocalData()) {
         RenderArgs& args = _imp->renderArgs.localData();
        if (args._validArgs) {
            args._time = time;
        }
    }
}

bool
EffectInstance::isDuringPaintStrokeCreationThreadLocal() const
{
    if (_imp->frameRenderArgs.hasLocalData()) {
        const ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.isDuringPaintStrokeCreation;
        }
    }
    return getNode()->isDuringPaintStrokeCreation();
}

Natron::RenderSafetyEnum
EffectInstance::getCurrentThreadSafetyThreadLocal() const
{
    if (_imp->frameRenderArgs.hasLocalData()) {
        const ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.currentThreadSafety;
        }
    }
    return getNode()->getCurrentRenderThreadSafety();
}

void
EffectInstance::Implementation::runChangedParamCallback(KnobI* k,bool userEdited,const std::string& callback)
{
    std::vector<std::string> args;
    std::string error;
    
    if (!k || k->getName() == "onParamChanged") {
        return;
    }
    
    Natron::getFunctionArguments(callback, &error, &args);
    if (!error.empty()) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + error);
        return;
    }
    
    std::string signatureError;
    signatureError.append("The param changed callback supports the following signature(s):\n");
    signatureError.append("- callback(thisParam,thisNode,thisGroup,app,userEdited)");
    if (args.size() != 5) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + signatureError);
        return;
    }
    
    if ((args[0] != "thisParam" || args[1] != "thisNode" || args[2] != "thisGroup" || args[3] != "app" || args[4] != "userEdited")) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + signatureError);
        return;
    }
    
    std::string appID = _publicInterface->getApp()->getAppIDString();
    
    assert(k);
    std::string thisNodeVar = appID + ".";
    thisNodeVar.append(_publicInterface->getNode()->getFullyQualifiedName());
    
    boost::shared_ptr<NodeCollection> collection = _publicInterface->getNode()->getGroup();
    assert(collection);
    if (!collection) {
        return;
    }
    
    std::string thisGroupVar;
    NodeGroup* isParentGrp = dynamic_cast<NodeGroup*>(collection.get());
    if (isParentGrp) {
        thisGroupVar = appID + "." + isParentGrp->getNode()->getFullyQualifiedName();
    } else {
        thisGroupVar = appID;
    }

    
    std::stringstream ss;
    ss << callback << "(" << thisNodeVar << "." << k->getName() << "," << thisNodeVar << "," << thisGroupVar << "," << appID
    << ",";
    if (userEdited) {
        ss << "True";
    } else {
        ss << "False";
    }
    ss << ")\n";
   
    std::string script = ss.str();
    std::string err;
    std::string output;
    if (!Natron::interpretPythonScript(script, &err,&output)) {
        _publicInterface->getApp()->appendToScriptEditor(QObject::tr("Failed to execute onParamChanged callback: ").toStdString() + err);
    } else {
        if (!output.empty()) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }
}

void
EffectInstance::onKnobValueChanged_public(KnobI* k,
                                          Natron::ValueChangedReasonEnum reason,
                                          SequenceTime time,
                                          bool originatedFromMainThread)
{

    NodePtr node = getNode();
//    if (!node->isNodeCreated()) {
//        return;
//    }

    if (isReader() && k->getName() == kOfxImageEffectFileParamName) {
        node->computeFrameRangeForReader(k);
    }

    
    KnobHelper* kh = dynamic_cast<KnobHelper*>(k);
    assert(kh);
    if ( kh && kh->isDeclaredByPlugin() ) {
        ////We set the thread storage render args so that if the instance changed action
        ////tries to call getImage it can render with good parameters.
        
        
        ParallelRenderArgsSetter frameRenderArgs(getApp()->getProject().get(),
                                                 time,
                                                 0, /*view*/
                                                 true,
                                                 false,
                                                 false,
                                                 0,
                                                 node,
                                                 0, // request
                                                 0, //texture index
                                                 getApp()->getTimeLine().get(),
                                                 NodePtr(),
                                                 true,
                                                 false,
                                                 false,
                                                 boost::shared_ptr<RenderStats>());

        RECURSIVE_ACTION();
        EffectPointerThreadProperty_RAII propHolder_raii(this);
        knobChanged(k, reason, /*view*/ 0, time, originatedFromMainThread);
    }
    
    node->onEffectKnobValueChanged(k, reason);
    
    ///If there's a knobChanged Python callback, run it
    std::string pythonCB = getNode()->getKnobChangedCallback();
    
    if (!pythonCB.empty()) {
        bool userEdited = reason == eValueChangedReasonNatronGuiEdited ||
        reason == eValueChangedReasonUserEdited;
        _imp->runChangedParamCallback(k,userEdited,pythonCB);
    }

    ///Refresh the dynamic properties that can be changed during the instanceChanged action
    node->refreshDynamicProperties();
    
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
        _imp->lastPlanesRendered.clear();
    }
}

void
EffectInstance::aboutToRestoreDefaultValues()
{
    ///Invalidate the cache by incrementing the age
    NodePtr node = getNode();
    node->incrementKnobsAge();

    if ( node->areKeyframesVisibleOnTimeline() ) {
        node->hideKeyframesFromTimeline(true);
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
    NodePtr node = getNode();
    if ( !node->isNodeDisabled() ) {
        return node->getLiveInstance();
    } else {
        ///Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<Natron::EffectInstance*> nonOptionalInputs;
        std::list<Natron::EffectInstance*> optionalInputs;
        
        bool useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();
        
        ///Find an input named A
        std::string inputNameToFind,otherName;
        if (useInputA) {
            inputNameToFind = "A";
            otherName = "B";
        } else {
            inputNameToFind = "B";
            otherName = "A";
        }
        int foundOther = -1;
        int maxinputs = getMaxInputCount();
        for (int i = 0; i < maxinputs ; ++i) {
            std::string inputLabel = getInputLabel(i);
            if (inputLabel == inputNameToFind ) {
                EffectInstance* inp = getInput(i);
                if (inp) {
                    nonOptionalInputs.push_front(inp);
                    break;
                }
            } else if (inputLabel == otherName) {
                foundOther = i;
            }
        }
        
        if (foundOther != -1 && nonOptionalInputs.empty()) {
            EffectInstance* inp = getInput(foundOther);
            if (inp) {
                nonOptionalInputs.push_front(inp);
            }
        }
        
        ///If we found A or B so far, cycle through them
        for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        
        ///We cycle in reverse by default. It should be a setting of the application.
        ///In this case it will return input B instead of input A of a merge for example.
        for (int i = 0; i < maxinputs; ++i) {
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
    assert(getNode()->isNodeDisabled());
    
    ///Test all inputs recursively, going from last to first, preferring non optional inputs.
    std::list<Natron::EffectInstance*> nonOptionalInputs;
    std::list<Natron::EffectInstance*> optionalInputs;
    int localPreferredInput = -1;

    bool useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();
    ///Find an input named A
    std::string inputNameToFind,otherName;
    if (useInputA) {
        inputNameToFind = "A";
        otherName = "B";
    } else {
        inputNameToFind = "B";
        otherName = "A";
    }
    int foundOther = -1;
    int maxinputs = getMaxInputCount();
    for (int i = 0; i < maxinputs ; ++i) {
        std::string inputLabel = getInputLabel(i);
        if (inputLabel == inputNameToFind ) {
            EffectInstance* inp = getInput(i);
            if (inp) {
                nonOptionalInputs.push_front(inp);
                localPreferredInput = i;
                break;
            }
        } else if (inputLabel == otherName) {
            foundOther = i;
        }
    }
    
    if (foundOther != -1 && nonOptionalInputs.empty()) {
        EffectInstance* inp = getInput(foundOther);
        if (inp) {
            nonOptionalInputs.push_front(inp);
            localPreferredInput = foundOther;
        }
    }
    
    ///If we found A or B so far, cycle through them
    for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        if ((*it)->getNode()->isNodeDisabled()) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }
    
    
    ///We cycle in reverse by default. It should be a setting of the application.
    ///In this case it will return input B instead of input A of a merge for example.
    for (int i = 0; i < maxinputs; ++i) {
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
EffectInstance::getNearestNonIdentity(double time)
{
    U64 hash = getRenderHash();

    RenderScale scale;
    scale.x = scale.y = 1.;
    
    RectD rod;
    bool isProjectFormat;
    Natron::StatusEnum stat = getRegionOfDefinition_public(hash, time, scale, 0, &rod, &isProjectFormat);
    
    ///Ignore the result of getRoD if it failed
    Q_UNUSED(stat);
    
    double inputTimeIdentity;
    int inputNbIdentity;
    
    RectI pixelRoi;
    rod.toPixelEnclosing(scale, getPreferredAspectRatio(), &pixelRoi);
    if ( !isIdentity_public(true, hash,time, scale, pixelRoi, 0, &inputTimeIdentity, &inputNbIdentity) ) {
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
EffectInstance::restoreClipPreferences()
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

void
EffectInstance::onNodeHashChanged(U64 hash)
{
    
    ///Invalidate actions cache
    _imp->actionsCache.invalidateAll(hash);
    
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        for (int i = 0; i < (*it)->getDimension(); ++i) {
            (*it)->clearExpressionsResults(i);
        }
    }
}

bool
EffectInstance::canSetValue() const
{
    return !getNode()->isNodeRendering() || appPTR->isBackground();
}

void
EffectInstance::abortAnyEvaluation()
{
    ///Just change the hash
    NodePtr node = getNode();
    assert(node);
    node->incrementKnobsAge();
    std::list<ViewerInstance*> viewers;
    node->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance*>::const_iterator it = viewers.begin(); it!=viewers.end(); ++it) {
        (*it)->markAllOnRendersAsAborted();
    }
}

double
EffectInstance::getCurrentTime() const
{
    return getThreadLocalRenderTime();
}

int
EffectInstance::getCurrentView() const
{
    if (_imp->renderArgs.hasLocalData()) {
        const RenderArgs& args = _imp->renderArgs.localData();
        if (args._validArgs) {
            return args._view;
        }
    }
    
    return 0;
}

SequenceTime
EffectInstance::getFrameRenderArgsCurrentTime() const
{
    if (_imp->frameRenderArgs.hasLocalData()) {
        const ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.time;
        }
    }
    return getApp()->getTimeLine()->currentFrame();
}

int
EffectInstance::getFrameRenderArgsCurrentView() const
{
    if (_imp->frameRenderArgs.hasLocalData()) {
        const ParallelRenderArgs& args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.view;
        }
    }
    
    return 0;
}

#ifdef DEBUG
void
EffectInstance::checkCanSetValueAndWarn() const
{
    if (!checkCanSetValue()) {
        qDebug() << getScriptName_mt_safe().c_str() << ": setValue()/setValueAtTime() was called during an action that is not allowed to call this function.";
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

bool
EffectInstance::isPaintingOverItselfEnabled() const
{
    return isDuringPaintStrokeCreationThreadLocal();
}

OutputEffectInstance::OutputEffectInstance(boost::shared_ptr<Node> node)
: Natron::EffectInstance(node)
, _writerCurrentFrame(0)
, _writerFirstFrame(0)
, _writerLastFrame(0)
, _outputEffectDataLock(new QMutex)
, _renderController(0)
, _engine(0)
, _timeSpentPerFrameRendered()
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
    _engine->renderCurrentFrame(getApp()->isRenderStatsActionChecked(), canAbort);
}

void
OutputEffectInstance::renderCurrentFrameWithRenderStats(bool canAbort)
{
    _engine->renderCurrentFrame(true, canAbort);
}

void
OutputEffectInstance::renderFromCurrentFrameUsingCurrentDirection(bool enableRenderStats)
{
    _engine->renderFromCurrentFrameUsingCurrentDirection(enableRenderStats);
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
OutputEffectInstance::renderFullSequence(bool enableRenderStats,BlockingBackgroundRender* renderController,int first,int last)
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
    _engine->renderFrameRange(enableRenderStats,first,last,OutputSchedulerThread::eRenderDirectionForward);

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

bool
OutputEffectInstance::isSequentialRenderBeingAborted() const
{
    return _engine ? _engine->isSequentialRenderBeingAborted() : false;
}

bool
OutputEffectInstance::isDoingSequentialRender() const
{
    return _engine ? _engine->isDoingSequentialRender() : false;
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
    NodePtr node = getNode();
    std::list<Natron::Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), node.get());
    if (found != markedNodes.end()) {
        return;
    }
    

    checkOFXClipPreferences(time, scale, reason, forceGetClipPrefAction);
    
    getNode()->refreshChannelSelectors(false);

    markedNodes.push_back(node.get());
    
    std::list<Natron::Node*>  outputs;
    node->getOutputsWithGroupRedirection(outputs);
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

void
OutputEffectInstance::updateRenderTimeInfos(double lastTimeSpent, double *averageTimePerFrame, double *totalTimeSpent)
{
    assert(totalTimeSpent && averageTimePerFrame);
    
    *totalTimeSpent = 0;
    
    QMutexLocker k(_outputEffectDataLock);
    _timeSpentPerFrameRendered.push_back(lastTimeSpent);
    
    for (std::list<double>::iterator it = _timeSpentPerFrameRendered.begin(); it!= _timeSpentPerFrameRendered.end(); ++it) {
        *totalTimeSpent += *it;
    }
    size_t c = _timeSpentPerFrameRendered.size();
    *averageTimePerFrame = (c == 0 ? 0 : *totalTimeSpent / c);
}

void
OutputEffectInstance::resetTimeSpentRenderingInfos()
{
    QMutexLocker k(_outputEffectDataLock);
    _timeSpentPerFrameRendered.clear();
}

void
OutputEffectInstance::reportStats(int time, int view, double wallTime, const std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >& stats)
{
    std::string filename;
    boost::shared_ptr<KnobI> fileKnob = getKnobByName(kOfxImageEffectFileParamName);
    if (fileKnob) {
        OutputFile_Knob* strKnob = dynamic_cast<OutputFile_Knob*>(fileKnob.get());
        if  (strKnob) {
            QString qfileName(SequenceParsing::generateFileNameFromPattern(strKnob->getValue(0), time, view).c_str());
            Natron::removeFileExtension(qfileName);
            qfileName.append("-stats.txt");
            filename = qfileName.toStdString();
        }
    }
    
    //If there's no filename knob, do not write anything
    if (filename.empty()) {
        std::cout << QObject::tr("Cannot write render statistics file: ").toStdString() << getScriptName_mt_safe() <<
        QObject::tr(" does not seem to have a parameter named \"filename\" to determine the location where to write the stats file.").toStdString() << std::endl;
        return;
    }
    
    std::ofstream ofile;
    ofile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try {
        ofile.open(filename.c_str(),std::ofstream::out);
    } catch (const std::ios_base::failure & e) {
        std::cout << QObject::tr("Failure to write render statistics file: ").toStdString() << e.what() << std::endl;
        return;
    }
    
    if ( !ofile.good() ) {
        std::cout << QObject::tr("Failure to write render statistics file.").toStdString() << std::endl;
        return;
    }
    
    ofile << "Time spent to render frame (wall clock time): " << Timer::printAsTime(wallTime,false).toStdString() << std::endl;
    for (std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >::const_iterator it = stats.begin(); it!=stats.end(); ++it) {
        ofile << "------------------------------- " << it->first->getScriptName_mt_safe() << "------------------------------- " << std::endl;
        ofile << "Time spent rendering: " << Timer::printAsTime(it->second.getTotalTimeSpentRendering(), false).toStdString() << std::endl;
        const RectD& rod = it->second.getRoD();
        ofile << "Region of definition: x1 = " << rod.x1  << " y1 = " << rod.y1 << " x2 = " << rod.x2 << " y2 = " << rod.y2 << std::endl;
        ofile << "Is Identity to Effect? ";
        NodePtr identity = it->second.getInputImageIdentity();
        if (identity) {
            ofile << "Yes, to " << identity->getScriptName_mt_safe() << std::endl;
        } else {
            ofile << "No" << std::endl;
        }
        ofile << "Has render-scale support? " ;
        if (it->second.isRenderScaleSupportEnabled()) {
            ofile << "Yes";
        } else {
            ofile << "No";
        }
        ofile << std::endl;
        ofile << "Has tiles support? " ;
        if (it->second.isTilesSupportEnabled()) {
            ofile << "Yes";
        } else {
            ofile << "No";
        }
        ofile << std::endl;
        ofile << "Channels processed: ";
        
        bool r,g,b,a;
        it->second.getChannelsRendered(&r, &g, &b, &a);
        if (r) {
            ofile << "red ";
        }
        if (g) {
            ofile << "green ";
        }
        if (b) {
            ofile << "blue ";
        }
        if (a) {
            ofile << "alpha";
        }
        ofile << std::endl;
        
        ofile << "Output alpha premultiplication: ";
        switch (it->second.getOutputPremult()) {
            case Natron::eImagePremultiplicationOpaque:
                ofile << "opaque";
                break;
            case Natron::eImagePremultiplicationPremultiplied:
                ofile << "premultiplied";
                break;
            case Natron::eImagePremultiplicationUnPremultiplied:
                ofile << "unpremultiplied";
                break;
        }
        ofile << std::endl;
        
        ofile << "Mipmap level(s) rendered: ";
        for (std::set<unsigned int>::const_iterator it2 = it->second.getMipMapLevelsRendered().begin(); it2!=it->second.getMipMapLevelsRendered().end(); ++it2) {
            ofile << *it2 << ' ';
        }
        ofile << std::endl;
        int nbCacheMiss,nbCacheHit, nbCacheHitButDownscaled;
        it->second.getCacheAccessInfos(&nbCacheMiss, &nbCacheHit, &nbCacheHitButDownscaled);
        ofile << "Nb cache hit: " << nbCacheMiss << std::endl;
        ofile << "Nb cache miss: " << nbCacheMiss << std::endl;
        ofile << "Nb cache hit requiring mipmap downscaling: " << nbCacheHitButDownscaled << std::endl;
        
        const std::set<std::string>& planes = it->second.getPlanesRendered();
        ofile << "Plane(s) rendered: ";
        for (std::set<std::string>::const_iterator it2 = planes.begin(); it2 != planes.end(); ++it2) {
            ofile << *it2 << ' ';
        }
        ofile << std::endl;
        
        std::list<std::pair<RectI,boost::shared_ptr<Natron::Node> > > identityRectangles = it->second.getIdentityRectangles();
        const std::list<RectI>& renderedRectangles = it->second.getRenderedRectangles();
        
        ofile << "Identity rectangles: " << identityRectangles.size() << std::endl;
        for (std::list<std::pair<RectI,boost::shared_ptr<Natron::Node> > > ::iterator it2 = identityRectangles.begin(); it2 != identityRectangles.end(); ++it2) {
            ofile << "Origin: " << it2->second->getScriptName_mt_safe() << ", rect: x1 = " << it2->first.x1
            << " y1 = " << it2->first.y1 << " x2 = " << it2->first.x2 << " y2 = " << it2->first.y2 << std::endl;
        }
        
        ofile << "Rectangles rendered: " << renderedRectangles.size() << std::endl;
        for ( std::list<RectI>::const_iterator it2 = renderedRectangles.begin(); it2 != renderedRectangles.end(); ++it2) {
            ofile << "x1 = " << it2->x1 << " y1 = " << it2->y1 << " x2 = " << it2->x2 << " y2 = " << it2->y2 << std::endl;
        }
    }
}
