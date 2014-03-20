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
using namespace Natron;


class File_Knob;
class OutputFile_Knob;


struct EffectInstance::RenderArgs {
    RectI _roi;
    SequenceTime _time;
    RenderScale _scale;
    int _view;
    bool _isSequentialRender;
    bool _isRenderResponseToUserInteraction;
    bool _byPassCache;
};

struct EffectInstance::Implementation {
    Implementation()
    : renderAbortedMutex()
    , renderAborted(false)
    , hashValue()
    , hashAge(0)
    , inputs()
    , renderArgs()
    , previewEnabled(false)
    , markedByTopologicalSort(false)
    , beginEndRenderMutex()
    , beginEndRenderCount(0)
    {
    }

    mutable QReadWriteLock renderAbortedMutex;
    bool renderAborted; //< was rendering aborted ?
    Hash64 hashValue;//< The hash value of this effect
    int hashAge;//< to check if the hash has the same age than the project's age
    //or a render instance (i.e a snapshot of the live instance at a given time)

    Inputs inputs;//< all the inputs of the effect. Watch out, some might be NULL if they aren't connected
    ThreadStorage<RenderArgs> renderArgs;
    mutable QMutex previewEnabledMutex;
    bool previewEnabled;
    bool markedByTopologicalSort;
    QMutex beginEndRenderMutex;
    int beginEndRenderCount;
};

EffectInstance::EffectInstance(Node* node)
: KnobHolder(node ? node->getApp() : NULL)
, _node(node)
, _imp(new Implementation)
{
}

EffectInstance::~EffectInstance()
{
}

void EffectInstance::setMarkedByTopologicalSort(bool marked) const {_imp->markedByTopologicalSort = marked;}

bool EffectInstance::isMarkedByTopologicalSort() const {return _imp->markedByTopologicalSort;}

bool EffectInstance::isLiveInstance() const
{
    return !isClone();
}

const Hash64& EffectInstance::hash() const
{
    return _imp->hashValue;
}

const EffectInstance::Inputs& EffectInstance::getInputs() const
{
    return _imp->inputs;
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

void EffectInstance::clone(){
    if(!isClone())
        return;
    cloneKnobs(*(_node->getLiveInstance()));
    //refreshAfterTimeChange(time);
    cloneExtras();
    _imp->previewEnabled = _node->getLiveInstance()->isPreviewEnabled();
    if(isOpenFX()){
        dynamic_cast<OfxEffectInstance*>(this)->effectInstance()->syncPrivateDataAction();
    }
}


bool EffectInstance::isHashValid() const {
    //The hash is valid only if the age is the same than the project's age and the hash has been computed at least once.
    return _imp->hashAge == getAppAge() && _imp->hashValue.valid();
}
int EffectInstance::hashAge() const{
    return _imp->hashAge;
}

U64 EffectInstance::knobsAge() const {
    return _node->getKnobsAge();
}

void EffectInstance::setKnobsAge(U64 age) {
    _node->setKnobsAge(age);
}


U64 EffectInstance::cloneKnobsAndComputeHashAndClearPersistentMessage(int knobsAge,bool forceHashComputation) {
   
    ///if the effect is visited again, isHashValid will return true and the call is cheap
    
    if (!isHashValid() || forceHashComputation) {
        
        std::vector<U64> inputHashs;
        for (Inputs::iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
            if (*it) {
                inputHashs.push_back((*it)->cloneKnobsAndComputeHashAndClearPersistentMessage(knobsAge,forceHashComputation));
            }
        }
        
        clearPersistentMessage();
        
        ///clone all the knobs
        clone();
        
        ///set the hash age to be the global app age
        _imp->hashAge = knobsAge;
        
        ///reset the hash value
        _imp->hashValue.reset();
        
        ///append the effect's own age
        _imp->hashValue.append(getNode()->getKnobsAge());

        ///append all inputs hash
        for (U32 i =0; i < inputHashs.size(); ++i) {
            _imp->hashValue.append(inputHashs[i]);
        }
        
        ///Also append the effect's label to distinguish 2 instances with the same parameters
        ::Hash64_appendQString(&_imp->hashValue, QString(getName().c_str()));
        
        
        ///Also append the project's creation time in the hash because 2 projects openend concurrently
        ///could reproduce the same (especially simple graphs like Viewer-Reader)
        _imp->hashValue.append(getApp()->getProject()->getProjectCreationTime());

        _imp->hashValue.computeHash();
        
        return _imp->hashValue.value();
    } else {
        return _imp->hashValue.value();
    }

}

const std::string& EffectInstance::getName() const{
    return _node->getName();
}


void EffectInstance::getRenderFormat(Format *f) const
{
    assert(f);
    _node->getRenderFormatForEffect(this, f);
}

int EffectInstance::getRenderViewsCount() const{
    return _node->getRenderViewsCountForEffect(this);
}


bool EffectInstance::hasOutputConnected() const{
    return _node->hasOutputConnected();
}

Natron::EffectInstance* EffectInstance::input(int n) const{
    if (n < (int)_imp->inputs.size()) {
        return _imp->inputs[n];
    }
    return NULL;
}

int EffectInstance::inputIndex(EffectInstance* input) const {
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i] == input) {
            return i;
        }
    }
    return -1;
}

std::string EffectInstance::inputLabel(int inputNb) const {
    std::string out;
    out.append(1,(char)(inputNb+65));
    return out;
}

boost::shared_ptr<Natron::Image> EffectInstance::getImage(int inputNb,SequenceTime time,RenderScale scale,int view){
    
    EffectInstance* n  = input(inputNb);
    
    //if the node is not connected, return a NULL pointer!
    if(!n){
        return boost::shared_ptr<Natron::Image>();
    }

    ///just call renderRoI which will  do the cache look-up for us and render
    ///the image if it's missing from the cache.
    RectI roi;
    RectI precomputedRoD;
    bool isSequentialRender = false;
    bool isRenderUserInteraction = true;
    bool byPassCache = false;
    if (_imp->renderArgs.hasLocalData()) {
        roi = _imp->renderArgs.localData()._roi;//if the thread was spawned by us we take the last render args
        isSequentialRender = _imp->renderArgs.localData()._isSequentialRender;
        isRenderUserInteraction = _imp->renderArgs.localData()._isRenderResponseToUserInteraction;
        byPassCache = _imp->renderArgs.localData()._byPassCache;
    } else {
        bool isProjectFormat;
        Natron::Status stat = n->getRegionOfDefinition(time, &roi,&isProjectFormat);
        precomputedRoD = roi;
        if(stat == Natron::StatFailed) {//we have no choice but compute the full region of definition
            return boost::shared_ptr<Natron::Image>();
        }
    }
    boost::shared_ptr<Image > entry = n->renderRoI(time, scale, view,roi,isSequentialRender,isRenderUserInteraction,false, precomputedRoD.isNull() ? NULL : &precomputedRoD);

    return entry;
}

Natron::Status EffectInstance::getRegionOfDefinition(SequenceTime time,RectI* rod,bool* isProjectFormat) {
    
    Format frmt;
    getRenderFormat(&frmt);
    for(Inputs::const_iterator it = _imp->inputs.begin() ; it != _imp->inputs.end() ; ++it){
        if (*it) {
            RectI inputRod;
            Status st = (*it)->getRegionOfDefinition(time, &inputRod,isProjectFormat);
            if(st == StatFailed)
                return st;
            if (it == _imp->inputs.begin()) {
                *rod = inputRod;
            }else{
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

EffectInstance::RoIMap EffectInstance::getRegionOfInterest(SequenceTime /*time*/,RenderScale /*scale*/,const RectI& renderWindow){
    RoIMap ret;
    for( Inputs::const_iterator it = _imp->inputs.begin() ; it != _imp->inputs.end() ; ++it) {
        if (*it) {
            ret.insert(std::make_pair(*it, renderWindow));
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
    int i = 0;
    for(Inputs::const_iterator it = _imp->inputs.begin() ; it != _imp->inputs.end() ; ++it) {
        if (*it) {
            ret.insert(std::make_pair(i, ranges));
        }
        ++i;
    }
    return ret;
}

void EffectInstance::getFrameRange(SequenceTime *first,SequenceTime *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    for (Inputs::const_iterator it = _imp->inputs.begin() ; it != _imp->inputs.end() ; ++it) {
        if (*it) {
            SequenceTime inpFirst,inpLast;
            (*it)->getFrameRange(&inpFirst, &inpLast);
            if (it == _imp->inputs.begin()) {
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

boost::shared_ptr<Natron::Image> EffectInstance::renderRoI(SequenceTime time,RenderScale scale,
                                                           int view,const RectI& renderWindow,
                                                           bool isSequentialRender,
                                                           bool isRenderMadeInResponseToUserInteraction,
                                                           bool byPassCache,const RectI* preComputedRoD)
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
    
    /// First-off look-up the cache and see if we can find the cached actions results and cached image.
    boost::shared_ptr<const ImageParams> cachedImgParams;
    boost::shared_ptr<Image> image;
    bool isCached = false;
    Natron::ImageKey key = Natron::Image::makeKey(_imp->hashValue.value(), time, scale,view);
    
    ///The effect caching policy might forbid caching (Readers could use this when going out of the original frame range.)
    if (getCachePolicy(time) == NEVER_CACHE || isWriter()) {
        byPassCache = true;
    }
    
    if (!byPassCache) {
        isCached = Natron::getImageFromCache(key, &cachedImgParams,&image);
        
        ////If the image was cached with a RoD dependent on the project format, but the project format changed,
        ////just discard this entry
        if (isCached) {
            assert(cachedImgParams);
            if (cachedImgParams->isRodProjectFormat()) {
                Format projectFormat;
                getRenderFormat(&projectFormat);
                if (dynamic_cast<RectI&>(projectFormat) != cachedImgParams->getRoD()) {
                    isCached = false;
                    appPTR->removeFromNodeCache(image);
                    cachedImgParams.reset();
                    image.reset();
                }
            }
        }
    }
    
    
    if (!isCached) {
        
        ///first-off check whether the effect is identity, in which case we don't want
        /// to cache anything or render anything for this effect.
        SequenceTime inputTimeIdentity;
        int inputNbIdentity;
        RectI rod;
        FramesNeededMap framesNeeded;
        bool isProjectFormat = false;
        bool identity = isIdentity(time,scale,renderWindow,view,&inputTimeIdentity,&inputNbIdentity);
        if (identity) {
           
            ///we don't need to call getRegionOfDefinition and getFramesNeeded if the effect is an identity
            image = getImage(inputNbIdentity,inputTimeIdentity,scale,view);

            ///if we bypass the cache, don't cache the result of isIdentity
            if (byPassCache) {
                return image;
            }
        } else {
            ///set it to -1 so the cache know its not an identity
            inputNbIdentity = -1;
            
            ///if the rod is already passed as parameter, just use it and don't call getRegionOfDefinition
            if (preComputedRoD) {
                rod = *preComputedRoD;
            } else {
                ///before allocating it we must fill the RoD of the image we want to render
                if(getRegionOfDefinition(time, &rod,&isProjectFormat) == StatFailed){
                    ///if getRoD fails, just return a NULL ptr
                    return boost::shared_ptr<Natron::Image>();
                }
            }
            // why should the rod be empty here?
            assert(!rod.isNull());
            
            ///add the window to the project's available formats if the effect is a reader
            if (isReader()) {
                Format frmt;
                frmt.set(rod);
                ///FIXME: what about the pixel aspect ratio ?
                getApp()->getProject()->setOrAddProjectFormat(frmt);
            }
            
            framesNeeded = getFramesNeeded(time);
          
        }
        
#pragma message WARN("Specify image components here")
        ImageComponents components = Natron::ImageComponentRGBA;
        
        int cost = 0;
        /*should data be stored on a physical device ?*/
        if (shouldRenderedDataBePersistent()) {
            cost = 1;
        }
        
        if (identity) {
            cost = -1;
        }
        
        cachedImgParams = Natron::Image::makeParams(cost, rod,isProjectFormat,
                                                    components,
                                                    inputNbIdentity, inputTimeIdentity,
                                                    framesNeeded);
        
        if (byPassCache) {
            ///if we bypass the cache, allocate the image ourselves
            assert(!image);
            assert(!identity);
            image.reset(new Natron::Image(components,rod,scale,time));
            
        } else {

            ///even though we called getImage before and it returned false, it may now
            ///return true if another thread created the image in the cache, so we can't
            ///make any assumption on the return value of this function call.
            ///
            ///!!!Note that if isIdentity is true it will allocate an empty image object with 0 bytes of data.
            boost::shared_ptr<Image> newImage;
            appPTR->getImageOrCreate(key, cachedImgParams, &newImage);
            assert(newImage);
            
            ///if the plugin is an identity we just inserted in the cache the identity params, we can now return.
            if (identity) {
                ///don't return the empty allocated image but the input effect image instead!
                return image;
            } else {
                image = newImage;
            }
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
            return getImage(inputNbIdentity, inputTimeIdentity, scale, view);
        }
        
#ifdef NATRON_DEBUG
        ///If the precomputed rod parameter was set, assert that it is the same than the image in cache.
        if (inputNbIdentity != -1 && preComputedRoD) {
            assert(*preComputedRoD == cachedImgParams->getRoD());
        }
#endif

    }

    ///If we reach here, it can be either because the image is cached or not, either way
    ///the image is NOT an identity, and it may have some content left to render.
    bool success = renderRoIInternal(time, scale, view, renderWindow, cachedImgParams, image,isSequentialRender,isRenderMadeInResponseToUserInteraction ,byPassCache);
    
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
    
    return image;
}


void EffectInstance::renderRoI(SequenceTime time,RenderScale scale,
                               int view,const RectI& renderWindow,
                               const boost::shared_ptr<const ImageParams>& cachedImgParams,
                               const boost::shared_ptr<Image>& image,
                               bool isSequentialRender,
                               bool isRenderMadeInResponseToUserInteraction,
                               bool byPassCache) {
    bool success = renderRoIInternal(time, scale, view, renderWindow, cachedImgParams, image,isSequentialRender,isRenderMadeInResponseToUserInteraction, byPassCache);
    if (!success) {
        throw std::runtime_error("Rendering Failed");
    }
}

bool EffectInstance::renderRoIInternal(SequenceTime time,RenderScale scale,
                                       int view,const RectI& renderWindow,
                                       const boost::shared_ptr<const ImageParams>& cachedImgParams,
                                       const boost::shared_ptr<Image>& image,
                                       bool isSequentialRender,
                                       bool isRenderMadeInResponseToUserInteraction,
                                       bool byPassCache) {
    _node->addImageBeingRendered(image, time, view);
    
    ///We check what is left to render.
    
    ///intersect the image render window to the actual image region of definition.
    RectI intersection;
    renderWindow.intersect(image->getRoD(), &intersection);
    
    /// If the list contains only null rects then we already rendered it all
    std::list<RectI> rectsToRender = image->getRestToRender(intersection);
    
    ///if the effect doesn't support tiles and it has something left to render, just render the rod again
    ///note that it should NEVER happen because if it doesn't support tiles in the first place, it would
    ///have rendered the rod already.
    if (!supportsTiles() && !rectsToRender.empty()) {
        ///if the effect doesn't support tiles, just render the whole rod again even though
        rectsToRender.clear();
        rectsToRender.push_back(cachedImgParams->getRoD());
    }
#ifdef NATRON_LOG
    else if (rectsToRender.empty()) {
        Natron::Log::print(QString("Everything is already rendered in this image.").toStdString());
    }
#endif
    
    bool renderSucceeded = true;

    
    for (std::list<RectI>::iterator it = rectsToRender.begin(); it != rectsToRender.end(); ++it) {
        
        RectI& rectToRender = *it;
        
#ifdef NATRON_LOG
        Natron::Log::print(QString("Rect left to render in the image... xmin= "+
                                   QString::number(rectToRender.left())+" ymin= "+
                                   QString::number(rectToRender.bottom())+ " xmax= "+
                                   QString::number(rectToRender.right())+ " ymax= "+
                                   QString::number(rectToRender.top())).toStdString());
#endif
        /*we can set the render args*/
        RenderArgs args;
        args._roi = rectToRender;
        args._time = time;
        args._view = view;
        args._scale = scale;
        args._isSequentialRender = isSequentialRender;
        args._isRenderResponseToUserInteraction = isRenderMadeInResponseToUserInteraction;
        _imp->renderArgs.setLocalData(args);
        args._byPassCache = byPassCache;
        
        
        ///the getRegionOfInterest call CANNOT be cached because it depends of the render window.
        RoIMap inputsRoi = getRegionOfInterest(time, scale, rectToRender);
        
        ///get the cached frames needed or the one we just computed earlier.
        const FramesNeededMap& framesNeeeded = cachedImgParams->getFramesNeeded();
        
        std::list< boost::shared_ptr<Natron::Image> > inputImages;
        

        /*we render each input first and store away their image in the inputImages list
         in order to maintain a shared_ptr use_count > 1 so the cache doesn't attempt
         to remove them.*/
        for (FramesNeededMap::const_iterator it2 = framesNeeeded.begin(); it2 != framesNeeeded.end(); ++it2) {
            EffectInstance* inputEffect = input(it2->first);
            if (inputEffect) {
                RoIMap::iterator foundInputRoI = inputsRoi.find(inputEffect);
                assert(foundInputRoI != inputsRoi.end());
                
                ///notify the node that we're going to render something with the input
                assert(it2->first != -1); //< see getInputNumber
                _node->notifyInputNIsRendering(it2->first);
                
                for (U32 range = 0; range < it2->second.size(); ++range) {
                    for (U32 f = it2->second[range].min; f < it2->second[range].max; ++f) {
                        boost::shared_ptr<Natron::Image> inputImg = inputEffect->renderRoI(f, scale,view, foundInputRoI->second,
                                                                isSequentialRender,isRenderMadeInResponseToUserInteraction,byPassCache);
                        if (inputImg) {
                            inputImages.push_back(inputImg);
                        }
                    }
                }
                _node->notifyInputNIsFinishedRendering(it2->first);
                
                if (aborted()) {
                    //if render was aborted, remove the frame from the cache as it contains only garbage
                    appPTR->removeFromNodeCache(image);
                    _node->removeImageBeingRendered(time, view);
                    return true;
                }
            }
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
            beginSequenceRender(time, time, 1, !appPTR->isBackground(), scale,isSequentialRender,isRenderMadeInResponseToUserInteraction,view);
        }
        
        /*depending on the thread-safety of the plug-in we render with a different
         amount of threads*/
        EffectInstance::RenderSafety safety = renderThreadSafety();
        
        ///if the project lock is already locked at this point, don't start any othter thread
        ///as it would lead to a deadlock when the project is loading.
        ///Just fall back to Fully_safe
        if (safety == FULLY_SAFE_FRAME) {
            int nbThreads = appPTR->getCurrentSettings()->getNumberOfThreads();
            if (nbThreads == -1 || nbThreads == 1 || (nbThreads == 0 && QThread::idealThreadCount() == 1)) {
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
            case FULLY_SAFE_FRAME: // the plugin will perform any per frame SMP threading
            {
                // we can split the frame in tiles and do per frame SMP threading (see kOfxImageEffectPluginPropHostFrameThreading)
                std::vector<RectI> splitRects = RectI::splitRectIntoSmallerRect(rectToRender, QThread::idealThreadCount());
                // the bitmap is checked again at the beginning of EffectInstance::tiledRenderingFunctor()
                QFuture<Natron::Status> ret = QtConcurrent::mapped(splitRects,
                                                                   boost::bind(&EffectInstance::tiledRenderingFunctor,
                                                                               this,args,_1,image));
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
                    endSequenceRender(time, time, time, false, scale,isSequentialRender,isRenderMadeInResponseToUserInteraction,view);
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
                rectToRender = image->getMinimalRect(rectToRender);
                if (!rectToRender.isNull()) {
                    Natron::Status st = render(time, scale, rectToRender,view,isSequentialRender,isRenderMadeInResponseToUserInteraction, image);
                    
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
                        endSequenceRender(time, time, time, false, scale,isSequentialRender,isRenderMadeInResponseToUserInteraction,view);
                    }
                    
                    if (st != Natron::StatOK) {
                        renderSucceeded = false;
                    }
                    if (!aborted()) {
                        image->markForRendered(rectToRender);
                    }
                }
            } break;
            case FULLY_SAFE:    // indicating that any instance of a plugin can have multiple renders running simultaneously
            {
                ///FULLY_SAFE means that there is only one render per FRAME for a given instance take a per-frame lock here (the map of per-frame
                ///locks belongs to an instance)
                
                QMutexLocker l(&getNode()->getFrameMutex(time));
                
                // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
                rectToRender = image->getMinimalRect(rectToRender);
                if (!rectToRender.isNull()) {
                    Natron::Status st = render(time, scale, rectToRender,view,isSequentialRender,isRenderMadeInResponseToUserInteraction, image);
                    
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
                        endSequenceRender(time, time, time, false, scale,isSequentialRender,isRenderMadeInResponseToUserInteraction,view);
                    }
                    
                    if (st != Natron::StatOK) {
                        renderSucceeded = false;
                    }
                    if (!aborted()) {
                        image->markForRendered(rectToRender);
                    }
                }
            } break;
                
                
            case UNSAFE: // indicating that only a single 'render' call can be made at any time amoung all instances
            default:
            {
                QMutexLocker lock(appPTR->getMutexForPlugin(pluginID().c_str()));
                // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
                rectToRender = image->getMinimalRect(rectToRender);
                if (!rectToRender.isNull()) {
                    Natron::Status st = render(time, scale, rectToRender,view,isSequentialRender,isRenderMadeInResponseToUserInteraction, image);
                    
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
                        endSequenceRender(time, time, time, false, scale,isSequentialRender,isRenderMadeInResponseToUserInteraction,view);
                    }
                    
                    if(st != Natron::StatOK){
                        renderSucceeded = false;
                    }
                    if (!aborted()) {
                        image->markForRendered(rectToRender);
                    }
                }
            } break;
        }
        
        ///notify the node we've finished rendering
        _node->notifyRenderingEnded();
        
        if (!renderSucceeded) {
            break;
        }
    }
    _node->removeImageBeingRendered(time, view);
    
    //we released the input images and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();
    
    return renderSucceeded;

}

boost::shared_ptr<Natron::Image> EffectInstance::getImageBeingRendered(SequenceTime time,int view) const{
    return _node->getImageBeingRendered(time, view);
}

Natron::Status EffectInstance::tiledRenderingFunctor(const RenderArgs& args,
                                                     const RectI& roi,
                                                     boost::shared_ptr<Natron::Image> output)
{
    _imp->renderArgs.setLocalData(args);
    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
    RectI rectToRender = output->getMinimalRect(roi);
    if (!rectToRender.isNull()) {
        Natron::Status st = render(args._time, args._scale, rectToRender, args._view,
                                   args._isSequentialRender,args._isRenderResponseToUserInteraction, output);
        if(st != StatOK){
            return st;
        }
        if(!aborted()){
            output->markForRendered(roi);
        }
    }
    return StatOK;
}

void EffectInstance::openImageFileKnob() {
    const std::vector< boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
            boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
            assert(fk);
            if (fk->isInputImageFile()) {
                QString file = fk->getValue<QString>();
                if (file.isEmpty()) {
                    fk->open_file();
                }
                break;
            }
        } else if(knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if (fk->isOutputImageFile()) {
                QString file = fk->getValue<QString>();
                if(file.isEmpty()){
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

void EffectInstance::evaluate(Knob* knob, bool isSignificant)
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
    
    std::list<ViewerInstance*> viewers;
    _node->hasViewersConnected(&viewers);
    bool forcePreview = getApp()->getProject()->isAutoPreviewEnabled();
    for (std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it) {
        if (isSignificant) {
            (*it)->refreshAndContinueRender(forcePreview);
        } else {
            (*it)->redrawViewer();
        }
    }
    
    getNode()->refreshPreviewsRecursively();
}


void EffectInstance::abortRendering(){
    if (isClone()) {
        _node->abortRenderingForEffect(this);
    }else if(isOutput()){
        dynamic_cast<OutputEffectInstance*>(this)->getVideoEngine()->abortRendering(true);
    }
}

void EffectInstance::togglePreview() {
    QMutexLocker l(&_imp->previewEnabledMutex);
    _imp->previewEnabled = !_imp->previewEnabled;
}

void EffectInstance::updateInputs(RenderTree* tree) {
    Inputs inputsCopy = _imp->inputs;
    _imp->inputs.clear();
    const Node::InputMap& inputs = _node->getInputs();
    InspectorNode* insp = dynamic_cast<InspectorNode*>(_node);
    
    for (Node::InputMap::const_iterator it = inputs.begin(); it!=inputs.end(); ++it) {
        EffectInstance* inputInstance = NULL;
        if (it->second) {
            if (insp) {
                Node* activeInput = insp->input(insp->activeInput());
                if(it->second == activeInput){
                    if(tree){
                        inputInstance = tree->getEffectForNode(it->second);
                    }else{
                        inputInstance = it->second->getLiveInstance();
                    }
                    assert(inputInstance);
                }
            } else {
                if(tree){
                    inputInstance = tree->getEffectForNode(it->second);
                }else{
                    inputInstance = it->second->getLiveInstance();
                }
                assert(inputInstance);
            }
        }
        _imp->inputs.push_back(inputInstance);
    }
    if (!insp) {
        if (!inputsCopy.empty()) {
            bool hasChanged = false;
            assert(_imp->inputs.size() == inputsCopy.size());
            for (unsigned int i = 0; i < inputsCopy.size(); ++i) {
                if (_imp->inputs[i] != inputsCopy[i]) {
                    onInputChanged(i);
                    hasChanged = true;
                }
            }
            if (hasChanged) {
                onMultipleInputsChanged();
            }
        } else {
            bool hasChanged = false;
            for (unsigned int i = 0; i < inputsCopy.size(); ++i) {
                onInputChanged(i);
                hasChanged = true;
            }
            if (hasChanged) {
                onMultipleInputsChanged();
            }
            
        }
    }
    
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
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i] == inputEffect) {
            return i;
        }
    }
    return -1;
}


void EffectInstance::setInputFilesForReader(const QStringList& files) {
    
    if (!isReader()) {
        return;
    }
    
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
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

void EffectInstance::setOutputFilesForWriter(const QString& pattern) {
    
    if (!isWriter()) {
        return;
    }
    
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            assert(fk);
            if (fk->isOutputImageFile()) {
                fk->setValue<QString>(pattern);
                break;
            }
        }
    }
}

PluginMemory* EffectInstance::newMemoryInstance(size_t nBytes) {
    PluginMemory* ret = new PluginMemory(this);
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

OutputEffectInstance::OutputEffectInstance(Node* node)
: Natron::EffectInstance(node)
, _videoEngine(node?new VideoEngine(this):0)
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