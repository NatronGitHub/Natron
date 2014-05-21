//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#include "EffectInstance.h"

#include <QtConcurrentMap>
#include <QReadWriteLock>
#include <QCoreApplication>
#include <QtConcurrentRun>

#include <boost/bind.hpp>

#include "Engine/AppManager.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Log.h"
#include "Engine/VideoEngine.h"
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
using namespace Natron;


class File_Knob;
class OutputFile_Knob;


struct EffectInstance::RenderArgs {
    RectI _roi; //< The RoI in PIXEL coordinates
    RoIMap _regionOfInterestResults; //< the input RoI's in CANONICAL coordinates
    SequenceTime _time; //< the time to render
    RenderScale _scale; //< the scale to render
    unsigned int _mipMapLevel; //< the mipmap level to render (redundant with scale)
    int _view; //< the view to render
    bool _isSequentialRender; //< is this sequential ?
    bool _isRenderResponseToUserInteraction; //< is this a render due to user interaction ?
    bool _byPassCache; //< use cache lookups  ?
    bool _validArgs; //< are the args valid ?
    U64 _nodeHash;
    U64 _rotoAge;
    
    RenderArgs()
    : _roi()
    , _regionOfInterestResults()
    , _time(0)
    , _scale()
    , _mipMapLevel(0)
    , _view(0)
    , _isSequentialRender(false)
    , _isRenderResponseToUserInteraction(false)
    , _byPassCache(false)
    , _validArgs(false)
    , _nodeHash(0)
    , _rotoAge(0)
    {}
};

struct EffectInstance::Implementation {
    Implementation()
    : renderAbortedMutex()
    , renderAborted(false)
    , renderArgs()
    , previewEnabled(false)
    , beginEndRenderMutex()
    , beginEndRenderCount(0)
    , duringInteractActionMutex()
    , duringInteractAction(false)
    {
    }

    mutable QReadWriteLock renderAbortedMutex;
    bool renderAborted; //< was rendering aborted ?
    
    ThreadStorage<RenderArgs> renderArgs;
    mutable QMutex previewEnabledMutex;
    bool previewEnabled;
    QMutex beginEndRenderMutex;
    int beginEndRenderCount;
    
    mutable QReadWriteLock duringInteractActionMutex; //< protects duringInteractAction
    bool duringInteractAction; //< true when we're running inside an interact action

    void setDuringInteractAction(bool b) {
        QWriteLocker l(&duringInteractActionMutex);
        duringInteractAction = b;
    }
    
    /**
     * @brief Small helper class that set the render args and 
     * invalidate them when it is destroyed.
     **/
    class ScopedRenderArgs {
        
        RenderArgs args;
        ThreadStorage<RenderArgs>* _dst;
    public:
        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,
                         const RectI& roi,
                         const RoIMap& roiMap,
                         SequenceTime time,
                         int view,
                         const RenderScale& scale,
                         unsigned int mipMapLevel,
                         bool sequential,
                         bool userInteraction,
                         bool bypassCache,
                         U64 nodeHash,
                         U64 rotoAge)
        : args()
        , _dst(dst)
        {
            assert(_dst);
            args._roi = roi;
            args._regionOfInterestResults = roiMap;
            args._time = time;
            args._view = view;
            args._scale = scale;
            args._mipMapLevel = mipMapLevel;
            args._isSequentialRender = sequential;
            args._isRenderResponseToUserInteraction = userInteraction;
            args._byPassCache = bypassCache;
            args._nodeHash = nodeHash;
            args._rotoAge = rotoAge;
            args._validArgs = true;
            _dst->setLocalData(args);
        }
        
        ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,const RenderArgs& a)
        : args(a)
        , _dst(dst)
        {
            args._validArgs = true;
            _dst->setLocalData(args);
        }
        
        ~ScopedRenderArgs()
        {
            assert(_dst->hasLocalData());
            args._validArgs = false;
            _dst->setLocalData(args);
        }
        
        /**
         * @brief WARNING: Returns the args that have been passed to the constructor.
         **/
        const RenderArgs& getArgs() const { return args; }
    };
    
};

EffectInstance::EffectInstance(boost::shared_ptr<Node> node)
: KnobHolder(node ? node->getApp() : NULL)
, _node(node)
, _imp(new Implementation)
{
}

EffectInstance::~EffectInstance()
{
}

U64 EffectInstance::hash() const
{
    return getNode()->getHashValue();
}


bool EffectInstance::aborted() const
{
    QReadLocker l(&_imp->renderAbortedMutex);
    return _imp->renderAborted;
}

void EffectInstance::setAborted(bool b)
{
    QWriteLocker l(&_imp->renderAbortedMutex);
    _imp->renderAborted = b;
}

bool EffectInstance::isPreviewEnabled() const
{
    QMutexLocker l(&_imp->previewEnabledMutex);
    return _imp->previewEnabled;
}

U64 EffectInstance::knobsAge() const {
    return _node->getKnobsAge();
}

void EffectInstance::setKnobsAge(U64 age) {
    _node->setKnobsAge(age);
}

const std::string& EffectInstance::getName() const{
    return _node->getName();
}


void EffectInstance::getRenderFormat(Format *f) const
{
    assert(f);
    getApp()->getProject()->getProjectDefaultFormat(f);
}

int EffectInstance::getRenderViewsCount() const{
    return getApp()->getProject()->getProjectViewsCount();
}


bool EffectInstance::hasOutputConnected() const{
    return _node->hasOutputConnected();
}

Natron::EffectInstance* EffectInstance::input(int n) const
{
    
    ///Only called by the main-thread
    
    boost::shared_ptr<Natron::Node> inputNode = _node->input(n);
    if (inputNode) {
        return inputNode->getLiveInstance();
    }
    return NULL;
}

EffectInstance* EffectInstance::input_other_thread(int n) const
{
    boost::shared_ptr<Natron::Node> inputNode = _node->input_other_thread(n);
    if (inputNode) {
        return inputNode->getLiveInstance();
    }
    return NULL;
}

std::string EffectInstance::inputLabel(int inputNb) const
{
    std::string out;
    out.append(1,(char)(inputNb+65));
    return out;
}

boost::shared_ptr<Natron::Image> EffectInstance::getImage(int inputNb,SequenceTime time,RenderScale scale,int view)
{
    
    EffectInstance* n  = input_other_thread(inputNb);
   
    boost::shared_ptr<RotoContext> roto = _node->getRotoContext();
    ///The caller thread MUST be a thread owned by Natron. It cannot be a thread from the multi-thread suite.
    ///A call to getImage is forbidden outside an action running in a thread launched by Natron.
    assert(_imp->renderArgs.hasLocalData() && _imp->renderArgs.localData()._validArgs);
    
    ///just call renderRoI which will  do the cache look-up for us and render
    ///the image if it's missing from the cache.
    
    RectI currentEffectRenderWindow = _imp->renderArgs.localData()._roi;
    bool isSequentialRender = _imp->renderArgs.localData()._isSequentialRender;
    bool isRenderUserInteraction = _imp->renderArgs.localData()._isRenderResponseToUserInteraction;
    bool byPassCache = _imp->renderArgs.localData()._byPassCache;
    unsigned int mipMapLevel = _imp->renderArgs.localData()._mipMapLevel;;
    RoIMap inputsRoI = _imp->renderArgs.localData()._regionOfInterestResults;

    
    RoIMap::iterator found = inputsRoI.find(roto ? this : n);
    assert(found != inputsRoI.end());
    
    ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
    RectI roi = found->second;
    
    ///Convert to pixel coordinates (FIXME: take the par into account)
    if (mipMapLevel != 0) {
        roi = roi.downscalePowerOfTwoSmallestEnclosing(mipMapLevel);
    }
    
    
    if (roto && isInputRotoBrush(inputNb)) {
        U64 nodeHash = _imp->renderArgs.localData()._nodeHash;
        U64 rotoAge = _imp->renderArgs.localData()._rotoAge;
        return roto->renderMask(roi, nodeHash,rotoAge,RectI(), time, view, mipMapLevel, byPassCache);
    }
    
    
    //if the node is not connected, return a NULL pointer!
    if(!n){
        return boost::shared_ptr<Natron::Image>();
    }
    
    ///Launch in another thread as the current thread might already have been created by the multi-thread suite,
    ///hence it might have a thread-id.
    QThreadPool::globalInstance()->reserveThread();
    QFuture< boost::shared_ptr<Image > > future = QtConcurrent::run(n,&Natron::EffectInstance::renderRoI,
                RenderRoIArgs(time,scale,mipMapLevel,view,roi,isSequentialRender,isRenderUserInteraction,
                              byPassCache, NULL));
    future.waitForFinished();
    QThreadPool::globalInstance()->releaseThread();
    boost::shared_ptr<Natron::Image> inputImg = future.result();
    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();
    
    ///if the plug-in doesn't support the image components
    if (!isSupportedComponent(inputNb, inputImg->getComponents())) {
        Natron::ImageComponents mappedComp = findClosestSupportedComponents(inputNb, inputImg->getComponents());
        int channelForAlpha = getMaskChannel();
        Natron::Image* remappedImg;
        
        if ((mappedComp == Natron::ImageComponentAlpha) && (channelForAlpha == -1 || !isMaskEnabled())) {
            ///Set the mask to 0's everywhere
            
            remappedImg = new Natron::Image(mappedComp,inputImg->getRoD(),inputImg->getMipMapLevel());
            remappedImg->defaultInitialize(1.,1.);
        } else {
            ///convert the fetched input image
            bool invert = isInputMask(inputNb) && isMaskInverted();
            remappedImg = inputImg->convertToFormat(mappedComp, channelForAlpha,invert);
        }
        
        inputImg.reset(remappedImg);
    }
    
    if (!supportsRenderScale() && inputImgMipMapLevel > 0) {
        RectI upscaledRoD = inputImg->getPixelRoD().upscalePowerOfTwo(inputImgMipMapLevel);
        boost::shared_ptr<Natron::Image> upscaledImg(new Natron::Image(inputImg->getComponents(),upscaledRoD,0));
        inputImg->upscale_mipmap(inputImg->getPixelRoD(), upscaledImg.get(), inputImgMipMapLevel);
        return upscaledImg;
    } else {
        return inputImg;
    }
}

Natron::Status EffectInstance::getRegionOfDefinition(SequenceTime time,const RenderScale& scale,int view,RectI* rod,bool* isProjectFormat) {
    
    Format frmt;
    getRenderFormat(&frmt);
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
        if (input) {
            RectI inputRod;
            Status st = input->getRegionOfDefinition_public(time,scale,view, &inputRod,isProjectFormat);
            if (st == StatFailed) {
                return st;
            }
            
            if (i == 0) {
                *rod = inputRod;
            } else {
                rod->merge(inputRod);
            }

        }
    }
    if (rod->bottom() == frmt.bottom() && rod->top() == frmt.top() && rod->left() == frmt.left() && rod->right() == frmt.right()) {
        *isProjectFormat = true;
    } else {
        *isProjectFormat = false;
    }
    return StatReplyDefault;
}

EffectInstance::RoIMap EffectInstance::getRegionOfInterest(SequenceTime /*time*/,RenderScale /*scale*/,const RectI& renderWindow,
                                                           int /*view*/){
    RoIMap ret;
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
        if (input) {
            ret.insert(std::make_pair(input, renderWindow));
        }
    }
    return ret;
}

EffectInstance::FramesNeededMap EffectInstance::getFramesNeeded(SequenceTime time) {
    EffectInstance::FramesNeededMap ret;
    RangeD defaultRange;
    defaultRange.min = defaultRange.max = time;
    std::vector<RangeD> ranges;
    ranges.push_back(defaultRange);
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
        if (input) {
            ret.insert(std::make_pair(i, ranges));
        }
    }
    return ret;
}

void EffectInstance::getFrameRange(SequenceTime *first,SequenceTime *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
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



boost::shared_ptr<Natron::Image> EffectInstance::renderRoI(const RenderRoIArgs& args)
{
#ifdef NATRON_LOG
    Natron::Log::beginFunction(getName(),"renderRoI");
    Natron::Log::print(QString("Time "+QString::number(time)+
                                                      " Scale ("+QString::number(scale.x)+
                                                      ","+QString::number(scale.y)
                        +") View " + QString::number(view) + " RoI: xmin= "+ QString::number(renderWindow.left()) +
                        " ymin= " + QString::number(renderWindow.bottom()) + " xmax= " + QString::number(renderWindow.right())
                        + " ymax= " + QString::number(renderWindow.top())).toStdString());
#endif
    
    ///The effect caching policy might forbid caching (Readers could use this when going out of the original frame range.)
    ///For writer we never want to cache otherwise the next time we want to render it will skip writing the image on disk!
    bool byPassCache = args.byPassCache;
    if (getCachePolicy(args.time) == NEVER_CACHE || isWriter()) {
        byPassCache = true;
    }
    
    U64 nodeHash = hash();
    
    boost::shared_ptr<const ImageParams> cachedImgParams;
    boost::shared_ptr<Image> image;

    Natron::ImageKey key = Natron::Image::makeKey(nodeHash, args.time,args.mipMapLevel ,args.view);
    
    /// First-off look-up the cache and see if we can find the cached actions results and cached image.
    bool isCached = Natron::getImageFromCache(key, &cachedImgParams,&image);
    
    if (isCached) {
        assert(cachedImgParams);
        
        if (cachedImgParams->isRodProjectFormat()) {
            ////If the image was cached with a RoD dependent on the project format, but the project format changed,
            ////just discard this entry
            Format projectFormat;
            getRenderFormat(&projectFormat);
            if (dynamic_cast<RectI&>(projectFormat) != cachedImgParams->getRoD()) {
                isCached = false;
                appPTR->removeFromNodeCache(image);
                cachedImgParams.reset();
                image.reset();
            }
        }
        
        if (isCached && byPassCache) {
            ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
            ///we're sure renderRoIInternal will compute the whole image again.
            ///We must use the cache facility anyway because we rely on it for caching the results
            ///of actions which is necessary to avoid recursive actions.
            image->clearBitmap();
        }

    }
    
    
#pragma message WARN("Specify image components here")
    ImageComponents components = Natron::ImageComponentRGBA;
    
    boost::shared_ptr<Natron::Image> downscaledImage = image;
    
    if (!isCached) {
        
        ///first-off check whether the effect is identity, in which case we don't want
        /// to cache anything or render anything for this effect.
        SequenceTime inputTimeIdentity;
        int inputNbIdentity;
        RectI rod;
        FramesNeededMap framesNeeded;
        bool isProjectFormat = false;
        
        bool identity = isIdentity_public(args.time,args.scale,args.roi,args.view,&inputTimeIdentity,&inputNbIdentity);
        if (identity) {
            RectI canonicalRoI = args.roi.upscalePowerOfTwo(args.mipMapLevel);
            RoIMap inputsRoI = getRegionOfInterest_public(args.time, args.scale, canonicalRoI, args.view);
            Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,
                                                        args.roi,
                                                        inputsRoI,
                                                        args.time,
                                                        args.view,
                                                        args.scale,
                                                        args.mipMapLevel,
                                                        args.isSequentialRender,
                                                        args.isRenderUserInteraction,
                                                        byPassCache,
                                                        nodeHash,
                                                        0);
            
            ///we don't need to call getRegionOfDefinition and getFramesNeeded if the effect is an identity
            image = getImage(inputNbIdentity,inputTimeIdentity,args.scale,args.view);
            
            ///if we bypass the cache, don't cache the result of isIdentity
            if (byPassCache) {
                return image;
            }
        } else {
            ///set it to -1 so the cache knows it's not an identity
            inputNbIdentity = -1;
            
            ///if the rod is already passed as parameter, just use it and don't call getRegionOfDefinition
            if (args.preComputedRoD) {
                rod = *args.preComputedRoD;
            } else {
                ///before allocating it we must fill the RoD of the image we want to render
                if(getRegionOfDefinition_public(args.time,args.scale,args.view, &rod,&isProjectFormat) == StatFailed){
                    ///if getRoD fails, just return a NULL ptr
                    return boost::shared_ptr<Natron::Image>();
                }
            }
            
            // why should the rod be empty here?
            assert(!rod.isNull());
            
            framesNeeded = getFramesNeeded_public(args.time);
          
        }
    
        
        int cost = 0;
        /*should data be stored on a physical device ?*/
        if (shouldRenderedDataBePersistent()) {
            cost = 1;
        }
        
        if (identity) {
            cost = -1;
        }
      
        cachedImgParams = Natron::Image::makeParams(cost, rod,args.mipMapLevel,isProjectFormat,
                                                    components,
                                                    inputNbIdentity, inputTimeIdentity,
                                                    framesNeeded);
    
        ///even though we called getImage before and it returned false, it may now
        ///return true if another thread created the image in the cache, so we can't
        ///make any assumption on the return value of this function call.
        ///
        ///!!!Note that if isIdentity is true it will allocate an empty image object with 0 bytes of data.
        boost::shared_ptr<Image> newImage;
        bool cached = appPTR->getImageOrCreate(key, cachedImgParams, &newImage);
        assert(newImage);
        
        
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
        
        if (!supportsRenderScale() && args.mipMapLevel != 0) {
            ///Allocate the upscaled image
            image.reset(new Natron::Image(components,rod,0));
        }
        
        
        assert(cachedImgParams);

    } else {
#ifdef NATRON_LOG
        Natron::Log::print(QString("The image was found in the NodeCache with the following hash key: "+
                                   QString::number(key.getHash())).toStdString());
#endif
        assert(cachedImgParams);
        assert(image);
        

        ///if it was cached, first thing to check is to see if it is an identity
        int inputNbIdentity = cachedImgParams->getInputNbIdentity();
        if (inputNbIdentity != -1) {
            SequenceTime inputTimeIdentity = cachedImgParams->getInputTimeIdentity();
            RectI canonicalRoI = args.roi.upscalePowerOfTwo(args.mipMapLevel);
            RoIMap inputsRoI = getRegionOfInterest_public(args.time, args.scale, canonicalRoI, args.view);
            Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,
                                                        args.roi,
                                                        inputsRoI,
                                                        args.time,
                                                        args.view,
                                                        args.scale,
                                                        args.mipMapLevel,
                                                        args.isSequentialRender,
                                                        args.isRenderUserInteraction,
                                                        byPassCache,
                                                        nodeHash,
                                                        0);
            return getImage(inputNbIdentity, inputTimeIdentity, args.scale, args.view);
        }
        
#ifdef NATRON_DEBUG
        ///If the precomputed rod parameter was set, assert that it is the same than the image in cache.
        if (inputNbIdentity != -1 && args.preComputedRoD) {
            assert(*args.preComputedRoD == cachedImgParams->getRoD());
        }
#endif

        ///For effects that don't support the render scale we have to upscale this cached image,
        ///render the parts we are interested in and then downscale again
        ///Before doing that we verify if everything we want is already rendered in which case we
        ///dont degrade the image
        if (!supportsRenderScale() && args.mipMapLevel != 0) {
            
            RectI intersection;
            args.roi.intersect(image->getPixelRoD(), &intersection);
            std::list<RectI> rectsRendered = image->getRestToRender(intersection);
            if (rectsRendered.empty()) {
                return image;
            }
        
            downscaledImage = image;
            
            ///Allocate the upscaled image
            boost::shared_ptr<Natron::Image> upscaledImage(new Natron::Image(components,cachedImgParams->getRoD(),0));
            downscaledImage->scale_box_generic(downscaledImage->getPixelRoD(),upscaledImage.get());
            image = upscaledImage;
        }
        
    }
    

    ///If we reach here, it can be either because the image is cached or not, either way
    ///the image is NOT an identity, and it may have some content left to render.
    bool success = renderRoIInternal(args.time, args.scale,args.mipMapLevel, args.view, args.roi, cachedImgParams, image,
                                     downscaledImage,args.isSequentialRender,args.isRenderUserInteraction ,byPassCache,nodeHash);
    
    if(aborted() || !success){
        //if render was aborted, remove the frame from the cache as it contains only garbage
        appPTR->removeFromNodeCache(image);
    }
#ifdef NATRON_LOG
    Natron::Log::endFunction(getName(),"renderRoI");
#endif
    if (!success) {
        throw std::runtime_error("Rendering Failed");
    }

    return downscaledImage;
}


void EffectInstance::renderRoI(SequenceTime time,const RenderScale& scale,unsigned int mipMapLevel,
                               int view,const RectI& renderWindow,
                               const boost::shared_ptr<const ImageParams>& cachedImgParams,
                               const boost::shared_ptr<Image>& image,
                               const boost::shared_ptr<Image>& downscaledImage,
                               bool isSequentialRender,
                               bool isRenderMadeInResponseToUserInteraction,
                               bool byPassCache,
                               U64 nodeHash) {
    bool success = renderRoIInternal(time, scale,mipMapLevel, view, renderWindow, cachedImgParams, image,downscaledImage,isSequentialRender,isRenderMadeInResponseToUserInteraction, byPassCache,nodeHash);
    if (!success) {
        throw std::runtime_error("Rendering Failed");
    }
}

bool EffectInstance::renderRoIInternal(SequenceTime time,const RenderScale& scale,unsigned int mipMapLevel,
                                       int view,const RectI& renderWindow,
                                       const boost::shared_ptr<const ImageParams>& cachedImgParams,
                                       const boost::shared_ptr<Image>& image,
                                       const boost::shared_ptr<Image>& downscaledImage,
                                       bool isSequentialRender,
                                       bool isRenderMadeInResponseToUserInteraction,
                                       bool byPassCache,
                                       U64 nodeHash) {
    
    ///add the window to the project's available formats if the effect is a reader
    if ( mipMapLevel == 0 && isReader()) {
        Format frmt;
        frmt.set(cachedImgParams->getRoD());
        ///FIXME: what about the pixel aspect ratio ?
        getApp()->getProject()->setOrAddProjectFormat(frmt);
    }

    bool useFullResImage = !supportsRenderScale() && (mipMapLevel != 0);

//    _node->addImageBeingRendered(useFullResImage ? image : downscaledImage, time, view,mipMapLevel);
    
    ///We check what is left to render.
    
    ///intersect the image render window to the actual image region of definition.
    RectI intersection;
    renderWindow.intersect(downscaledImage->getPixelRoD(), &intersection);
    
    /// If the list is empty then we already rendered it all
    std::list<RectI> rectsToRender = downscaledImage->getRestToRender(intersection);
    
    ///if the effect doesn't support tiles and it has something left to render, just render the rod again
    ///note that it should NEVER happen because if it doesn't support tiles in the first place, it would
    ///have rendered the rod already.
    if (!supportsTiles() && !rectsToRender.empty()) {
        ///if the effect doesn't support tiles, just render the whole rod again even though
        rectsToRender.clear();
        rectsToRender.push_back(cachedImgParams->getPixelRoD());
    }
#ifdef NATRON_LOG
    else if (rectsToRender.empty()) {
        Natron::Log::print(QString("Everything is already rendered in this image.").toStdString());
    }
#endif
    
    
    boost::shared_ptr<RotoContext> rotoContext = _node->getRotoContext();
    U64 rotoAge = rotoContext ? rotoContext->getAge() : 0;
    
    bool renderSucceeded = true;


    for (std::list<RectI>::const_iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        
        const RectI &rectToRender = *it;
        
        ///Upscale the RoI to a region in the unscaled image so it is in canonical coordinates
        ///this is actually not entirely true since we don't care about pixel aspect ratio here
        RectI canonicalRectToRender = rectToRender.upscalePowerOfTwo(mipMapLevel);


        
#ifdef NATRON_LOG
        Natron::Log::print(QString("Rect left to render in the image... xmin= "+
                                   QString::number(rectToRender.left())+" ymin= "+
                                   QString::number(rectToRender.bottom())+ " xmax= "+
                                   QString::number(rectToRender.right())+ " ymax= "+
                                   QString::number(rectToRender.top())).toStdString());
#endif
        
        ///the getRegionOfInterest call will not be cached because it would be unnecessary
        ///To put that information (which depends on the RoI) into the cache. That's why we
        ///store it into the render args so the getImage() function can retrieve the results.
        RoIMap inputsRoi = getRegionOfInterest_public(time, scale, canonicalRectToRender,view);
        
        /*we can set the render args*/
        assert(!_imp->renderArgs.hasLocalData() || !_imp->renderArgs.localData()._validArgs);

        Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,
                                                    rectToRender,
                                                    inputsRoi,
                                                    time,
                                                    view,
                                                    scale,
                                                    mipMapLevel,
                                                    isSequentialRender,
                                                    isRenderMadeInResponseToUserInteraction,
                                                    byPassCache,
                                                    nodeHash,
                                                    rotoAge);
        const RenderArgs& args = scopedArgs.getArgs();
    
       
        ///get the cached frames needed or the one we just computed earlier.
        const FramesNeededMap& framesNeeeded = cachedImgParams->getFramesNeeded();
        
        std::list< boost::shared_ptr<Natron::Image> > inputImages;
        

        /*we render each input first and store away their image in the inputImages list
         in order to maintain a shared_ptr use_count > 1 so the cache doesn't attempt
         to remove them.*/
        for (FramesNeededMap::const_iterator it2 = framesNeeeded.begin(); it2 != framesNeeeded.end(); ++it2) {
            EffectInstance* inputEffect = input_other_thread(it2->first);
            if (inputEffect) {
                RoIMap::iterator foundInputRoI = inputsRoi.find(inputEffect);
                assert(foundInputRoI != inputsRoi.end());
                
                ///convert to pixel coords
                RectI inputRoIPixelCoords = foundInputRoI->second.downscalePowerOfTwoSmallestEnclosing(mipMapLevel);
                
                ///notify the node that we're going to render something with the input
                assert(it2->first != -1); //< see getInputNumber
                _node->notifyInputNIsRendering(it2->first);
                
                for (U32 range = 0; range < it2->second.size(); ++range) {
                    for (U32 f = it2->second[range].min; f <= it2->second[range].max; ++f) {
                        boost::shared_ptr<Natron::Image> inputImg =
                        inputEffect->renderRoI(RenderRoIArgs(f, //< time
                                                             scale, //< scale
                                                             mipMapLevel, //< mipmapLevel (redundant with the scale)
                                                             view, //< view
                                                             inputRoIPixelCoords, //< roi in pixel coordinates
                                                             isSequentialRender, //< sequential render ?
                                                             isRenderMadeInResponseToUserInteraction, // < user interaction ?
                                                             byPassCache, //< look-up the cache for existing images ?
                                                             NULL)); // < did we precompute any RoD to speed-up the call ?
                        
                        if (inputImg) {
                            inputImages.push_back(inputImg);
                        }
                    }
                }
                _node->notifyInputNIsFinishedRendering(it2->first);
                
                if (aborted()) {
                    //if render was aborted, remove the frame from the cache as it contains only garbage
                    appPTR->removeFromNodeCache(image);
//                    _node->removeImageBeingRendered(time, view,mipMapLevel);
                    return true;
                }
            }
        }
        
        ///if the node has a roto context, pre-render the roto mask too
        boost::shared_ptr<RotoContext> rotoCtx = _node->getRotoContext();
        if (rotoCtx) {
            boost::shared_ptr<Natron::Image> mask = rotoCtx->renderMask(rectToRender, nodeHash,rotoAge,
                                                                        cachedImgParams->getRoD() ,time, view, mipMapLevel, byPassCache);
            inputImages.push_back(mask);
        }
        
        ///notify the node we're starting a render
        _node->notifyRenderingStarted();
        
        bool callBegin = false;
        {
            QMutexLocker locker(&_imp->beginEndRenderMutex);
            if (_imp->beginEndRenderCount == 0) {
                callBegin = true;
            }
            ++_imp->beginEndRenderCount;
        }
        if (callBegin) {
            beginSequenceRender_public(time, time, 1, !appPTR->isBackground(), scale,isSequentialRender,
                                       isRenderMadeInResponseToUserInteraction,view);
        }
        
        /*depending on the thread-safety of the plug-in we render with a different
         amount of threads*/
        EffectInstance::RenderSafety safety = renderThreadSafety();
        
        ///if the project lock is already locked at this point, don't start any othter thread
        ///as it would lead to a deadlock when the project is loading.
        ///Just fall back to Fully_safe
        int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
        if (safety == FULLY_SAFE_FRAME) {
            
            if (nbThreads == -1 || nbThreads == 1 || (nbThreads == 0 && QThread::idealThreadCount() == 1) ||
                QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount()) {
                safety = FULLY_SAFE;
            } else {
                if (!getApp()->getProject()->tryLock()) {
                    safety = FULLY_SAFE;
                } else {
                    getApp()->getProject()->unlock();
                }
            }
        }

        switch (safety) {
            case FULLY_SAFE_FRAME: // the plugin will not perform any per frame SMP threading
            {
                // we can split the frame in tiles and do per frame SMP threading (see kOfxImageEffectPluginPropHostFrameThreading)
                if (nbThreads == 0) {
                    nbThreads = QThreadPool::globalInstance()->maxThreadCount();
                }
                std::vector<RectI> splitRects = RectI::splitRectIntoSmallerRect(rectToRender, nbThreads);
                // the bitmap is checked again at the beginning of EffectInstance::tiledRenderingFunctor()
                QFuture<Natron::Status> ret = QtConcurrent::mapped(splitRects,
                                                                   boost::bind(&EffectInstance::tiledRenderingFunctor,
                                                                               this,args,_1,downscaledImage,image));
                ret.waitForFinished();
                
                bool callEndRender = false;
                {
                    QMutexLocker locker(&_imp->beginEndRenderMutex);
                    --_imp->beginEndRenderCount;
                    assert(_imp->beginEndRenderCount >= 0);
                    if (_imp->beginEndRenderCount == 0) {
                        callEndRender = true;
                    }
                }
                if (callEndRender) {
                    endSequenceRender_public(time, time, time, false, scale,
                                             isSequentialRender,isRenderMadeInResponseToUserInteraction,view);
                }
                
                for (QFuture<Natron::Status>::const_iterator it2 = ret.begin(); it2!=ret.end(); ++it2) {
                    if ((*it2) == Natron::StatFailed) {
                        renderSucceeded = false;
                        break;
                    }
                }
            } break;
                
            case INSTANCE_SAFE: // indicating that any instance can have a single 'render' call at any one time,
            {
                // NOTE: the per-instance lock should probably be shared between
                // all clones of the same instance, because an InstanceSafe plugin may assume it is the sole owner of the output image,
                // and read-write on it.
                // It is probably safer to assume that several clones may write to the same output image only in the FULLY_SAFE case.
                
                QMutexLocker l(&getNode()->getRenderInstancesSharedMutex());
                
                // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
                RectI minimalRectToRender = downscaledImage->getMinimalRect(rectToRender);
                canonicalRectToRender = minimalRectToRender;
                if (useFullResImage) {
                    canonicalRectToRender = minimalRectToRender.upscalePowerOfTwo(mipMapLevel);
                    canonicalRectToRender.intersect(image->getPixelRoD(), &canonicalRectToRender);
                }
                
                if (!canonicalRectToRender.isNull()) {
                    Natron::Status st = render_public(time, scale, canonicalRectToRender,view,isSequentialRender,
                                               isRenderMadeInResponseToUserInteraction,useFullResImage ? image : downscaledImage);
                    ///copy the rectangle rendered in the full scale image to the downscaled output
                    if (useFullResImage) {
                        image->downscale_mipmap(canonicalRectToRender,downscaledImage.get(), args._mipMapLevel);
                    }
                    bool callEndRender = false;
                    {
                        QMutexLocker locker(&_imp->beginEndRenderMutex);
                        --_imp->beginEndRenderCount;
                        assert(_imp->beginEndRenderCount >= 0);
                        if (_imp->beginEndRenderCount == 0) {
                            callEndRender = true;
                        }
                    }
                    if (callEndRender) {
                        endSequenceRender_public(time, time, time, false, scale,isSequentialRender,
                                                 isRenderMadeInResponseToUserInteraction,view);
                    }
                    
                    if (st != Natron::StatOK) {
                        renderSucceeded = false;
                    }
                    if (!aborted()) {
                        downscaledImage->markForRendered(minimalRectToRender);
                    }
                }
            } break;
            case FULLY_SAFE:    // indicating that any instance of a plugin can have multiple renders running simultaneously
            {
                ///FULLY_SAFE means that there is only one render per FRAME for a given instance take a per-frame lock here (the map of per-frame
                ///locks belongs to an instance)
                
                QMutexLocker l(&getNode()->getFrameMutex(time));
                
                // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
                RectI minimalRectToRender = downscaledImage->getMinimalRect(rectToRender);
                canonicalRectToRender = minimalRectToRender;
                if (useFullResImage) {
                    canonicalRectToRender = minimalRectToRender.upscalePowerOfTwo(mipMapLevel);
                    canonicalRectToRender.intersect(image->getPixelRoD(), &canonicalRectToRender);
                }
                
                if (!canonicalRectToRender.isNull()) {
                    Natron::Status st = render_public(time,scale, canonicalRectToRender,view,isSequentialRender,
                                               isRenderMadeInResponseToUserInteraction, useFullResImage ? image : downscaledImage);
                    ///copy the rectangle rendered in the full scale image to the downscaled output
                    if (useFullResImage) {
                        image->downscale_mipmap(canonicalRectToRender,downscaledImage.get(), args._mipMapLevel);
                    }
                    bool callEndRender = false;
                    {
                        QMutexLocker locker(&_imp->beginEndRenderMutex);
                        --_imp->beginEndRenderCount;
                        assert(_imp->beginEndRenderCount >= 0);
                        if (_imp->beginEndRenderCount == 0) {
                            callEndRender = true;
                        }
                    }
                    if (callEndRender) {
                        endSequenceRender_public(time, time, time, false, scale,isSequentialRender,
                                                 isRenderMadeInResponseToUserInteraction,view);
                    }
                    
                    if (st != Natron::StatOK) {
                        renderSucceeded = false;
                    }
                    if (!aborted()) {
                        downscaledImage->markForRendered(minimalRectToRender);
                    }
                }
            } break;
                
                
            case UNSAFE: // indicating that only a single 'render' call can be made at any time amoung all instances
            default:
            {
                QMutexLocker lock(appPTR->getMutexForPlugin(pluginID().c_str()));
                // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
                RectI minimalRectToRender = downscaledImage->getMinimalRect(rectToRender);
                canonicalRectToRender = minimalRectToRender;
                if (useFullResImage) {
                    canonicalRectToRender = minimalRectToRender.upscalePowerOfTwo(mipMapLevel);
                    canonicalRectToRender.intersect(image->getPixelRoD(), &canonicalRectToRender);
                }
                
                if (!canonicalRectToRender.isNull()) {
                    Natron::Status st = render_public(time,scale, canonicalRectToRender,view,isSequentialRender,
                                               isRenderMadeInResponseToUserInteraction, useFullResImage ? image : downscaledImage);
                    
                    ///copy the rectangle rendered in the full scale image to the downscaled output
                    if (useFullResImage) {
                        image->downscale_mipmap(canonicalRectToRender,downscaledImage.get(), args._mipMapLevel);
                    }
                    bool callEndRender = false;
                    {
                        QMutexLocker locker(&_imp->beginEndRenderMutex);
                        --_imp->beginEndRenderCount;
                        assert(_imp->beginEndRenderCount >= 0);
                        if (_imp->beginEndRenderCount == 0) {
                            callEndRender = true;
                        }
                    }
                    if (callEndRender) {
                        endSequenceRender_public(time, time, time, false, scale,isSequentialRender,
                                                 isRenderMadeInResponseToUserInteraction,view);
                    }
                    
                    if(st != Natron::StatOK){
                        renderSucceeded = false;
                    }
                    if (!aborted()) {
                        downscaledImage->markForRendered(minimalRectToRender);
                    }
                }
            } break;
        }
        //appPTR->debugImage(downscaledImage.get(), QString(QString(getNode()->getName_mt_safe().c_str()) + ".png"));
        ///notify the node we've finished rendering
        _node->notifyRenderingEnded();
        
        if (!renderSucceeded) {
            break;
        }
    }
//    _node->removeImageBeingRendered(time, view,mipMapLevel);
    
    //we released the input images and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();
    return renderSucceeded;

}

//boost::shared_ptr<Natron::Image> EffectInstance::getImageBeingRendered(SequenceTime time,int view,unsigned int mipMapLevel) const{
//    return _node->getImageBeingRendered(time, view,mipMapLevel);
//}

Natron::Status EffectInstance::tiledRenderingFunctor(const RenderArgs& args,
                                                     const RectI& roi,
                                                     boost::shared_ptr<Natron::Image> downscaledOutput,
                                                     boost::shared_ptr<Natron::Image> output)
{
    Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs,args);
    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
    RectI rectToRender = downscaledOutput->getMinimalRect(roi);
    bool useFullResImage = !supportsRenderScale() && args._mipMapLevel != 0;
    
    RectI upscaledRoi = rectToRender;
    if (useFullResImage) {
        upscaledRoi = rectToRender.upscalePowerOfTwo(args._mipMapLevel);
        upscaledRoi.intersect(output->getPixelRoD(), &upscaledRoi);
    }
    
    if (!upscaledRoi.isNull()) {
        
        Natron::Status st = render_public(args._time,args._scale, upscaledRoi, args._view,
                                   args._isSequentialRender,args._isRenderResponseToUserInteraction,
                                   useFullResImage ? output : downscaledOutput);
        if(st != StatOK){
            return st;
        }
        
        
        
        if (!aborted()) {
            downscaledOutput->markForRendered(rectToRender);
        }
        ///copy the rectangle rendered in the full scale image to the downscaled output
        if (useFullResImage) {
            output->downscale_mipmap(upscaledRoi,downscaledOutput.get(), args._mipMapLevel);
        }

    }
    
    return StatOK;
}

void EffectInstance::openImageFileKnob() {
    const std::vector< boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
            boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
            assert(fk);
            if (fk->isInputImageFile()) {
                std::string file = fk->getValue();
                if (file.empty()) {
                    fk->open_file();
                }
                break;
            }
        } else if(knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if (fk->isOutputImageFile()) {
                std::string file = fk->getValue();
                if(file.empty()){
                    fk->open_file();
                }
                break;
            }

        }
    }
}


void EffectInstance::createKnobDynamically(){
    _node->createKnobDynamically();
}

void EffectInstance::evaluate(KnobI* knob, bool isSignificant)
{
    
    assert(_node);
    
    if (getApp()->getProject()->isLoadingProject()) {
        return;
    }
    
    
    Button_Knob* button = dynamic_cast<Button_Knob*>(knob);

    /*if this is a writer (openfx or built-in writer)*/
    if (isWriter()) {
        /*if this is a button and it is a render button,we're safe to assume the plug-ins wants to start rendering.*/
        if (button) {
            if (button->isRenderButton()) {
                QStringList list;
                list << getName().c_str();
                getApp()->startWritersRendering(list);
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
    bool forcePreview = getApp()->getProject()->isAutoPreviewEnabled();
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it) {
        if (isSignificant) {
            (*it)->refreshAndContinueRender(forcePreview);
        } else {
            (*it)->redrawViewer();
        }
    }
    
    getNode()->refreshPreviewsRecursively();
}

void EffectInstance::togglePreview() {
    QMutexLocker l(&_imp->previewEnabledMutex);
    _imp->previewEnabled = !_imp->previewEnabled;
}

bool EffectInstance::message(Natron::MessageType type,const std::string& content) const{
    return _node->message(type,content);
}

void EffectInstance::setPersistentMessage(Natron::MessageType type,const std::string& content){
    _node->setPersistentMessage(type, content);
}

void EffectInstance::clearPersistentMessage() {
    _node->clearPersistentMessage();
}

int EffectInstance::getInputNumber(Natron::EffectInstance* inputEffect) const {
    for (int i = 0; i < maximumInputs(); ++i) {
        if (input_other_thread(i) == inputEffect) {
            return i;
        }
    }
    return -1;
}


void EffectInstance::setInputFilesForReader(const std::vector<std::string>& files) {
    
    if (!isReader()) {
        return;
    }
    
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
            boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
            assert(fk);
            if (fk->isInputImageFile()) {
                if (files.size() > 1 && ! fk->isAnimationEnabled()) {
                    throw std::invalid_argument("This reader does not support image sequences. Please provide a single file.");
                }
                fk->setFiles(files);
                break;
            }
        }
    }
}

void EffectInstance::setOutputFilesForWriter(const std::string& pattern) {
    
    if (!isWriter()) {
        return;
    }
    
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if (fk->isOutputImageFile()) {
                fk->setValue(pattern,0);
                break;
            }
        }
    }
}

PluginMemory* EffectInstance::newMemoryInstance(size_t nBytes) {
    
    PluginMemory* ret = new PluginMemory(_node->getLiveInstance()); //< hack to get "this" as a shared ptr
    bool wasntLocked = ret->alloc(nBytes);
    assert(wasntLocked);
    return ret;
}


void EffectInstance::registerPluginMemory(size_t nBytes) {
    _node->registerPluginMemory(nBytes);
}

void EffectInstance::unregisterPluginMemory(size_t nBytes) {
    _node->unregisterPluginMemory(nBytes);
}

void EffectInstance::onSlaveStateChanged(bool isSlave,KnobHolder* master) {
    _node->onSlaveStateChanged(isSlave,master);
}


void EffectInstance::drawOverlay_public(double scaleX,double scaleY)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    
    ///Recursive action, must not call assertActionIsNotRecursive()
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    drawOverlay(scaleX,scaleY);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
}

bool EffectInstance::onOverlayPenDown_public(double scaleX,double scaleY,const QPointF& viewportPos, const QPointF& pos)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenDown(scaleX,scaleY,viewportPos, pos);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;
}

bool EffectInstance::onOverlayPenMotion_public(double scaleX,double scaleY,const QPointF& viewportPos, const QPointF& pos)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenMotion(scaleX,scaleY,viewportPos, pos);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    //Don't chek if render is needed on pen motion, wait for the pen up
    //checkIfRenderNeeded();
    return ret;
}

bool EffectInstance::onOverlayPenUp_public(double scaleX,double scaleY,const QPointF& viewportPos, const QPointF& pos)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    bool ret = onOverlayPenUp(scaleX,scaleY,viewportPos, pos);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;
}

bool EffectInstance::onOverlayKeyDown_public(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyDown(scaleX,scaleY,key, modifiers);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;
}

bool EffectInstance::onOverlayKeyUp_public(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyUp(scaleX,scaleY,key, modifiers);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;

}

bool EffectInstance::onOverlayKeyRepeat_public(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayKeyRepeat(scaleX,scaleY,key, modifiers);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;

}

bool EffectInstance::onOverlayFocusGained_public(double scaleX,double scaleY)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayFocusGained(scaleX,scaleY);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;

}

bool EffectInstance::onOverlayFocusLost_public(double scaleX,double scaleY)
{
    ///cannot be run in another thread
    assert(QThread::currentThread() == qApp->thread());
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayFocusLost(scaleX,scaleY);
    _imp->setDuringInteractAction(false);
    decrementRecursionLevel();
    checkIfRenderNeeded();
    return ret;

}

bool EffectInstance::isDoingInteractAction() const
{
    QReadLocker l(&_imp->duringInteractActionMutex);
    return _imp->duringInteractAction;
}

Natron::Status EffectInstance::render_public(SequenceTime time, RenderScale scale, const RectI& roi, int view,
                             bool isSequentialRender,bool isRenderResponseToUserInteraction,
                             boost::shared_ptr<Natron::Image> output)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    Natron::Status ret = render(time, scale, roi, view, isSequentialRender, isRenderResponseToUserInteraction, output);
    decrementRecursionLevel();
    return ret;
}

bool EffectInstance::isIdentity_public(SequenceTime time,RenderScale scale,const RectI& roi,
                       int view,SequenceTime* inputTime,int* inputNb)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    bool ret = isIdentity(time, scale, roi, view, inputTime, inputNb);
    decrementRecursionLevel();
    return ret;
}

Natron::Status EffectInstance::getRegionOfDefinition_public(SequenceTime time,const RenderScale& scale,int view,
                                            RectI* rod,bool* isProjectFormat)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    Natron::Status ret = getRegionOfDefinition(time, scale,view ,rod, isProjectFormat);
    decrementRecursionLevel();
    return ret;
}

EffectInstance::RoIMap EffectInstance::getRegionOfInterest_public(SequenceTime time,RenderScale scale,const RectI& renderWindow,int view)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    EffectInstance::RoIMap ret = getRegionOfInterest(time, scale, renderWindow, view);
    decrementRecursionLevel();
    return ret;
}

EffectInstance::FramesNeededMap EffectInstance::getFramesNeeded_public(SequenceTime time)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    EffectInstance::FramesNeededMap ret = getFramesNeeded(time);
    decrementRecursionLevel();
    return ret;
}

void EffectInstance::getFrameRange_public(SequenceTime *first,SequenceTime *last)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    getFrameRange(first, last);
    decrementRecursionLevel();
}

void EffectInstance::beginSequenceRender_public(SequenceTime first,SequenceTime last,
                                SequenceTime step,bool interactive,RenderScale scale,
                                bool isSequentialRender,bool isRenderResponseToUserInteraction,
                                int view)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    beginSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, view);
    decrementRecursionLevel();
}

void EffectInstance::endSequenceRender_public(SequenceTime first,SequenceTime last,
                              SequenceTime step,bool interactive,RenderScale scale,
                              bool isSequentialRender,bool isRenderResponseToUserInteraction,
                              int view)
{
    assertActionIsNotRecursive();
    incrementRecursionLevel();
    endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, view);
    decrementRecursionLevel();
}


bool EffectInstance::isSupportedComponent(int inputNb,Natron::ImageComponents comp) const
{
    return _node->isSupportedComponent(inputNb, comp);
}

Natron::ImageComponents EffectInstance::findClosestSupportedComponents(int inputNb,Natron::ImageComponents comp) const
{
    return _node->findClosestSupportedComponents(inputNb,comp);
}

int EffectInstance::getMaskChannel() const
{
    return _node->getMaskChannel();
}


bool EffectInstance::isMaskEnabled() const
{
    return _node->isMaskEnabled();
}


bool EffectInstance::isMaskInverted() const
{
    return _node->isMaskInverted();
}

OutputEffectInstance::OutputEffectInstance(boost::shared_ptr<Node> node)
: Natron::EffectInstance(node)
, _videoEngine(node ? new VideoEngine(this) : 0)
, _writerCurrentFrame(0)
, _writerFirstFrame(0)
, _writerLastFrame(0)
, _doingFullSequenceRender()
, _outputEffectDataLock(new QMutex)
, _renderController(0)
{
}

OutputEffectInstance::~OutputEffectInstance(){
    if(_videoEngine){
        _videoEngine->quitEngineThread();
    }
    delete _outputEffectDataLock;
}

void OutputEffectInstance::updateTreeAndRender(){
    _videoEngine->updateTreeAndContinueRender();
}
void OutputEffectInstance::refreshAndContinueRender(bool forcePreview){
    _videoEngine->refreshAndContinueRender(forcePreview);
}

bool OutputEffectInstance::ifInfiniteclipRectToProjectDefault(RectI* rod) const{
    if(!getApp()->getProject()){
        return false;
    }
    /*If the rod is infinite clip it to the project's default*/
    Format projectDefault;
    getRenderFormat(&projectDefault);
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    // an int can not be equal to (or compared to) std::numeric_limits<double>::infinity()
    bool isRodProjctFormat = false;
    if (rod->left() == kOfxFlagInfiniteMin || rod->left() == std::numeric_limits<int>::min()) {
        rod->set_left(projectDefault.left());
        isRodProjctFormat = true;
    }
    if (rod->bottom() == kOfxFlagInfiniteMin || rod->bottom() == std::numeric_limits<int>::min()) {
        rod->set_bottom(projectDefault.bottom());
        isRodProjctFormat = true;
    }
    if (rod->right() == kOfxFlagInfiniteMax || rod->right() == std::numeric_limits<int>::max()) {
        rod->set_right(projectDefault.right());
        isRodProjctFormat = true;
    }
    if (rod->top() == kOfxFlagInfiniteMax || rod->top()  == std::numeric_limits<int>::max()) {
        rod->set_top(projectDefault.top());
        isRodProjctFormat = true;
    }
    return isRodProjctFormat;
}

void OutputEffectInstance::renderFullSequence(BlockingBackgroundRender* renderController) {
    _renderController = renderController;
    assert(pluginID() != "Viewer"); //< this function is not meant to be called for rendering on the viewer
    getVideoEngine()->refreshTree();
    getVideoEngine()->render(-1, //< frame count
                             true, //< seek timeline
                             true, //< refresh tree
                             true, //< forward
                             false,
                             false); //< same frame
    
}

void OutputEffectInstance::notifyRenderFinished() {
    if (_renderController) {
        _renderController->notifyFinished();
        _renderController = 0;
    }
}

int OutputEffectInstance::getCurrentFrame() const {
    QMutexLocker l(_outputEffectDataLock);
    return _writerCurrentFrame;
}

void OutputEffectInstance::setCurrentFrame(int f) {
    QMutexLocker l(_outputEffectDataLock);
    _writerCurrentFrame = f;
}

int OutputEffectInstance::getFirstFrame() const {
    QMutexLocker l(_outputEffectDataLock);
    return _writerFirstFrame;
}

void OutputEffectInstance::setFirstFrame(int f) {
    QMutexLocker l(_outputEffectDataLock);
    _writerFirstFrame = f;
}

int OutputEffectInstance::getLastFrame() const {
    QMutexLocker l(_outputEffectDataLock);
    return _writerLastFrame;
}

void OutputEffectInstance::setLastFrame(int f) {
    QMutexLocker l(_outputEffectDataLock);
    _writerLastFrame = f;
}

void OutputEffectInstance::setDoingFullSequenceRender(bool b) {
    QMutexLocker l(_outputEffectDataLock);
    _doingFullSequenceRender = b;
}

bool OutputEffectInstance::isDoingFullSequenceRender() const {
    QMutexLocker l(_outputEffectDataLock);
    return _doingFullSequenceRender;
}