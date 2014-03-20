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

#include "ViewerInstancePrivate.h"

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QtGlobal>
#include <QtCore/QtConcurrentMap>
#include <QtCore/QFutureWatcher>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)

#include "Engine/Node.h"
#include "Engine/ImageInfo.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/FrameEntry.h"
#include "Engine/MemoryFile.h"
#include "Engine/VideoEngine.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/ImageInfo.h"
#include "Engine/TimeLine.h"
#include "Engine/Cache.h"
#include "Engine/Log.h"
#include "Engine/Lut.h"
#include "Engine/Settings.h"
#include "Engine/Project.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/Image.h"

#include "Gui/Texture.h" /// ?

using namespace Natron;
using std::make_pair;
using boost::shared_ptr;




static void
scaleToTexture8bits(std::pair<int,int> yRange,
                    const RenderViewerArgs& args,
                    U32* output);

static void
scaleToTexture32bits(std::pair<int,int> yRange,
                     const RenderViewerArgs& args,
                     float *output);

static std::pair<double, double>
findAutoContrastVminVmax(boost::shared_ptr<const Natron::Image> inputImage,
                         ViewerInstance::DisplayChannels channels,
                         const RectI& rect);

static void
renderFunctor(std::pair<int,int> yRange,
              const RenderViewerArgs& args,
              void *buffer);

/**
 *@brief Actually converting to ARGB... but it is called BGRA by
 the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int
toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;

unsigned int
toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static const Natron::Color::Lut*
lutFromColorspace(ViewerInstance::ViewerColorSpace cs) WARN_UNUSED_RETURN;

static const Natron::Color::Lut*
lutFromColorspace(ViewerInstance::ViewerColorSpace cs)
{
    const Natron::Color::Lut* lut;
    switch (cs) {
        case ViewerInstance::Linear:
            return 0;
        case ViewerInstance::sRGB:
            lut = Natron::Color::LutManager::sRGBLut();
            break;
        case ViewerInstance::Rec709:
            lut = Natron::Color::LutManager::Rec709Lut();
            break;
    }
    lut->validate();
    return lut;
}


Natron::EffectInstance*
ViewerInstance::BuildEffect(Natron::Node* n)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    return new ViewerInstance(n);
}

ViewerInstance::ViewerInstance(Node* node)
: Natron::OutputEffectInstance(node)
, _imp(new ViewerInstancePrivate(this))
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    connectSlotsToViewerCache();
    if(node) {
        connect(node,SIGNAL(nameChanged(QString)),this,SLOT(onNodeNameChanged(QString)));
    }
}

ViewerInstance::~ViewerInstance()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    if (_imp->bufferAllocated) {
        free(_imp->buffer);
        _imp->bufferAllocated = 0;
    }
}

OpenGLViewerI*
ViewerInstance::getUiContext() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    return _imp->uiContext;
}



void
ViewerInstance::forceFullComputationOnNextFrame()
{
    // this is called by the GUI when the user presses the "Refresh" button.
    // It set the flag forceRender to true, meaning no cache will be used.
    
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    QMutexLocker forceRenderLocker(&_imp->forceRenderMutex);
    _imp->forceRender = true;
}

void
ViewerInstance::connectSlotsToViewerCache()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    Natron::CacheSignalEmitter* emitter = appPTR->getOrActivateViewerCacheSignalEmitter();
    QObject::connect(emitter, SIGNAL(addedEntry()), this, SLOT(onViewerCacheFrameAdded()));
    QObject::connect(emitter, SIGNAL(removedLRUEntry()), this, SIGNAL(removedLRUCachedFrame()));
    QObject::connect(emitter, SIGNAL(clearedInMemoryPortion()), this, SIGNAL(clearedViewerCache()));
}

void
ViewerInstance::disconnectSlotsToViewerCache()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    Natron::CacheSignalEmitter* emitter = appPTR->getOrActivateViewerCacheSignalEmitter();
    QObject::disconnect(emitter, SIGNAL(addedEntry()), this, SLOT(onViewerCacheFrameAdded()));
    QObject::disconnect(emitter, SIGNAL(removedLRUEntry()), this, SIGNAL(removedLRUCachedFrame()));
    QObject::disconnect(emitter, SIGNAL(clearedInMemoryPortion()), this, SIGNAL(clearedViewerCache()));
}

void
ViewerInstance::setUiContext(OpenGLViewerI* viewer)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    // VideoEngine must not be created (or there would be a race condition)
    assert(!_imp->threadIdVideoEngine);

    _imp->uiContext = viewer;
}

void
ViewerInstance::onNodeNameChanged(const QString& name)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    ///update the gui tab name
    if (_imp->uiContext) {
        _imp->uiContext->onViewerNodeNameChanged(name);
    }
}


int
ViewerInstance::activeInput() const
{
    //    InspectorNode::activeInput()  is MT-safe
    return dynamic_cast<InspectorNode*>(getNode())->activeInput(); // not MT-SAFE!
}

int
ViewerInstance::maximumInputs() const
{
    // always running in the VideoEngine thread
    _imp->assertVideoEngine();

    return getNode()->maximumInputs();
}

Natron::Status
ViewerInstance::getRegionOfDefinition(SequenceTime time,RectI* rod,bool* isProjectFormat)
{
    // always running in the VideoEngine thread
    _imp->assertVideoEngine();

    ///Return the RoD of the active input
    EffectInstance* n = input(activeInput());
    if (n) {
        return n->getRegionOfDefinition(time,rod,isProjectFormat);
    } else {
        return StatFailed;
    }
}


void
ViewerInstance::getFrameRange(SequenceTime *first,
                              SequenceTime *last)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    SequenceTime inpFirst = 0,inpLast = 0;
    EffectInstance* n = input(activeInput());
    if (n) {
        n->getFrameRange(&inpFirst,&inpLast);
    }
    *first = inpFirst;
    *last = inpLast;
}


Natron::Status
ViewerInstance::renderViewer(SequenceTime time,
                             bool singleThreaded)
{
    // always running in the VideoEngine thread
    _imp->assertVideoEngine();

#ifdef NATRON_LOG
    Natron::Log::beginFunction(getName(),"renderViewer");
    Natron::Log::print(QString("Time "+QString::number(time)).toStdString());
#endif

    
    if (aborted()) {
        return StatFailed;
    }
    
    double zoomFactor = _imp->uiContext->getZoomFactor();

#pragma message WARN("Make use of the render scale here")
    RenderScale scale;
    scale.x = scale.y = 1.;
    
    Format dispW;
    getRenderFormat(&dispW);
    int viewsCount = getRenderViewsCount();
    int view = viewsCount > 0 ? _imp->uiContext->getCurrentView() : 0;

    EffectInstance* activeInputToRender = input(activeInput());
    assert(activeInputToRender);
    
    bool forceRender;
    {
        QMutexLocker forceRenderLocker(&_imp->forceRenderMutex);
        forceRender = _imp->forceRender;
        _imp->forceRender = false;
    }
    
    ///instead of calling getRegionOfDefinition on the active input, check the image cache
    ///to see whether the result of getRegionOfDefinition is already present. A cache lookup
    ///might be much cheaper than a call to getRegionOfDefinition.
    ///
    ///Note that we can't yet use the texture cache because we would need the TextureRect identifyin
    ///the texture in order to retrieve from the cache, but to make the TextureRect we need the RoD!
    boost::shared_ptr<const ImageParams> cachedImgParams;
    boost::shared_ptr<Image> inputImage;
    bool isInputImgCached = false;
    Natron::ImageKey inputImageKey = Natron::Image::makeKey(activeInputToRender->hash().value(), time, scale,view);
    RectI rod;
    bool isRodProjectFormat = false;
    int inputIdentityNumber = -1;
    SequenceTime inputIdentityTime;
    if (!forceRender) {
        isInputImgCached = Natron::getImageFromCache(inputImageKey, &cachedImgParams,&inputImage);
        if (isInputImgCached) {
            inputIdentityNumber = cachedImgParams->getInputNbIdentity();
            inputIdentityTime = cachedImgParams->getInputTimeIdentity();
        }
    }
    
    ////While the inputs are identity get the RoD of the first non identity input
    while (!forceRender && inputIdentityNumber != -1 && isInputImgCached) {
        EffectInstance* recursiveInput = activeInputToRender->input(inputIdentityNumber);
        if (recursiveInput) {
            inputImageKey = Natron::Image::makeKey(recursiveInput->hash().value(), inputIdentityTime, scale,view);
            isInputImgCached = Natron::getImageFromCache(inputImageKey, &cachedImgParams,&inputImage);
            if (isInputImgCached) {
                inputIdentityNumber = cachedImgParams->getInputNbIdentity();
                inputIdentityTime = cachedImgParams->getInputTimeIdentity();
            }
            
        } else {
            isInputImgCached = false;
        }
    }
    
    if (isInputImgCached) {
        
        ////If the image was cached with a RoD dependent on the project format, but the project format changed,
        ////just discard this entry.
        //// We do this ONLY if the effect is not an identity, because otherwise the isRoDProjectFormat is meaningless
        //// because we fetched an image upstream anyway.
        if (inputIdentityNumber == -1 && cachedImgParams->isRodProjectFormat()) {
            if (dynamic_cast<RectI&>(dispW) != cachedImgParams->getRoD()) {
                isInputImgCached = false;
                appPTR->removeFromNodeCache(inputImage);
                inputImage.reset();
                cachedImgParams.reset();
            }
        }
    }
    if (isInputImgCached) {
        rod = inputImage->getRoD();
        isRodProjectFormat = cachedImgParams->isRodProjectFormat();
        _imp->lastRenderedImage = inputImage;

    } else {
        Status stat = getRegionOfDefinition(time, &rod,&isRodProjectFormat);
        if(stat == StatFailed){
#ifdef NATRON_LOG
            Natron::Log::print(QString("getRegionOfDefinition returned StatFailed.").toStdString());
            Natron::Log::endFunction(getName(),"renderViewer");
#endif
            return stat;
        }
        isRodProjectFormat = ifInfiniteclipRectToProjectDefault(&rod);
    }
    

    emit rodChanged(rod);

        
    if (!_imp->uiContext->isClippingImageToProjectWindow()) {
        dispW.set(rod);
    }
    
    /*computing the RoI*/
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow(2,-std::ceil(std::log(zoomFactor) / std::log(2.)));

    RectI roi = _imp->uiContext->getImageRectangleDisplayed(rod);

    RectI texRect;
    double tileSize = std::pow(2., (double)appPTR->getCurrentSettings()->getViewerTilesPowerOf2());
    texRect.x1 = std::floor(((double)roi.x1 / closestPowerOf2) / tileSize) * tileSize;
    texRect.y1 = std::floor(((double)roi.y1 / closestPowerOf2) / tileSize) * tileSize;
    texRect.x2 = std::ceil(((double)roi.x2 / closestPowerOf2) / tileSize) * tileSize;
    texRect.y2 = std::ceil(((double)roi.y2 / closestPowerOf2) / tileSize) * tileSize;
    
    if (texRect.width() == 0 || texRect.height() == 0) {
        return StatOK;
    }
    
    RectI texRectClipped = texRect;
    texRectClipped.x1 *= closestPowerOf2;
    texRectClipped.x2 *= closestPowerOf2;
    texRectClipped.y1 *= closestPowerOf2;
    texRectClipped.y2 *= closestPowerOf2;
    texRectClipped.intersect(rod, &texRectClipped);
    
    int texW = texRect.width() > rod.width() ? rod.width() : texRect.width();
    int texH = texRect.height() > rod.height() ? rod.height() : texRect.height();
    
    TextureRect textureRect(texRectClipped.x1,texRectClipped.y1,texRectClipped.x2,
                            texRectClipped.y2,texW,texH,closestPowerOf2);
    
    //  std::cout << "ViewerInstance: x1: " << textureRect.x1 << " x2: " << textureRect.x2 << " y1: " << textureRect.y1 <<
    //" y2: " << textureRect.y2 << " w: " << textureRect.w << " h: " << textureRect.h << " po2: " << textureRect.closestPo2 << std::endl;
    
#pragma message WARN("Specify image components here")
    size_t bytesCount = textureRect.w * textureRect.h * 4;
    
    OpenGLViewerI::BitDepth bitDepth = _imp->uiContext->getBitDepth();
    
    //half float is not supported yet so it is the same as float
    if (bitDepth == OpenGLViewerI::FLOAT || bitDepth == OpenGLViewerI::HALF_FLOAT) {
        bytesCount *= sizeof(float);
    }
    
    ///make a copy of the auto contrast enabled state, so render threads only refer to that copy
    double gain;
    double offset = 0.; // 0 except for autoContrast
    bool autoContrast;
    ViewerInstance::ViewerColorSpace lut;
    ViewerInstance::DisplayChannels channels;
    {
        QMutexLocker locker(&_imp->viewerParamsMutex);
        gain = _imp->viewerParamsGain;
        lut = _imp->viewerParamsLut;
        autoContrast = _imp->viewerParamsAutoContrast;
        channels = _imp->viewerParamsChannels;
    }

    FrameKey key(time,
                 hash().value(),
                 gain,
                 lut,
                 (int)bitDepth,
                 channels,
                 view,
                 textureRect);

    /////////////////////////////////////
    // start cachedFrame scope
    //
    boost::shared_ptr<FrameEntry> cachedFrame; //!< this pointer is at least valid until this function exits, the the cache entry cannot be released
    boost::shared_ptr<const FrameParams> cachedFrameParams;
    bool isCached = false;
    
    ///if we want to force a refresh, we by-pass the cache
    bool byPassCache = false;
    if (!forceRender) {
        ///we never use the texture cache when the user RoI is enabled, otherwise we would have
        ///zillions of textures in the cache, each a few pixels different.
        if (!_imp->uiContext->isUserRegionOfInterestEnabled() && !autoContrast) {
            isCached = Natron::getTextureFromCache(key,&cachedFrameParams,&cachedFrame);
        }
    } else {
        byPassCache = true;
    }

    unsigned char* ramBuffer = NULL;

    if (isCached) {
        
        assert(cachedFrameParams);
        /*Found in viewer cache, we execute the cached engine and leave*/

        // how do you make sure cachedFrame->data() is not freed after this line?
        ///It is not freed as long as the cachedFrame shared_ptr has a used_count greater than 1.
        ///Since it is used during the whole function scope it is guaranteed not to be freed before
        ///The viewer is actually done with it.
        /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
        ramBuffer = cachedFrame->data();
#ifdef NATRON_LOG
        Natron::Log::print(QString("The image was found in the ViewerCache with the following hash key: "+
                                   QString::number(key.getHash())).toStdString());
        Natron::Log::endFunction(getName(),"renderViewer");
#endif
    } else { // !isCached
        /*We didn't find it in the viewer cache, hence we render
         the frame*/
        ///If the user RoI is enabled, the odds that we find a texture containing exactly the same portion
        ///is very low, we better render again (and let the NodeCache do the work) rather than just
        ///overload the ViewerCache which may become slowe
        if (byPassCache || _imp->uiContext->isUserRegionOfInterestEnabled() || autoContrast) {
            assert(!cachedFrame);
            // don't reallocate if we need less memory (avoid fragmentation)
            if (_imp->bufferAllocated < bytesCount) {
                if (_imp->bufferAllocated > 0) {
                    free(_imp->buffer);
                }
                _imp->bufferAllocated = bytesCount;
                _imp->buffer = (unsigned char*)malloc(_imp->bufferAllocated);
                if (!_imp->buffer) {
                    _imp->bufferAllocated = 0;
                    throw std::bad_alloc();
                }
                ramBuffer = (unsigned char*)_imp->buffer;
            }
        } else {
            cachedFrameParams = FrameEntry::makeParams(rod, key.getBitDepth(), textureRect.w, textureRect.h);
            bool success = Natron::getTextureFromCacheOrCreate(key, cachedFrameParams, &cachedFrame);
            ///note that unlike  getImageFromCacheOrCreate in EffectInstance::renderRoI, we
            ///are sure that this time the image was not in the cache and we created it because this functino
            ///is not multi-threaded.
            assert(!success);
            assert(cachedFrame);
            // how do you make sure cachedFrame->data() is not freed after this line?
            ///It is not freed as long as the cachedFrame shared_ptr has a used_count greater than 1.
            ///Since it is used during the whole function scope it is guaranteed not to be freed before
            ///The viewer is actually done with it.
            /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
            ramBuffer = cachedFrame->data();
        }
        assert(ramBuffer);

        if (!activeInputToRender->supportsTiles()) {
            texRectClipped.intersect(rod, &texRectClipped);
        }


        int inputIndex = activeInput();
        _node->notifyInputNIsRendering(inputIndex);

        // If an exception occurs here it is probably fatal, since
        // it comes from Natron itself. All exceptions from plugins are already caught
        // by the HostSupport library.
        // We catch it  and rethrow it just to notify the rendering is done.
        try {
            if (isInputImgCached) {
                ///if the input image is cached, call the shorter version of renderRoI which doesn't do all the
                ///cache lookup things because we already did it ourselves.
                activeInputToRender->renderRoI(time, scale, view, texRectClipped, cachedImgParams, inputImage);
            } else {
                _imp->lastRenderedImage = activeInputToRender->renderRoI(time, scale,view,texRectClipped,byPassCache,&rod);
            }
        } catch (...) {
            _node->notifyInputNIsFinishedRendering(inputIndex);
            throw;
        }

        _node->notifyInputNIsFinishedRendering(inputIndex);


        if (!_imp->lastRenderedImage) {
            //if render was aborted, remove the frame from the cache as it contains only garbage
            appPTR->removeFromViewerCache(cachedFrame);
            return StatFailed;
        }

        //  Natron::debugImage(_lastRenderedImage.get(),"img.png");

        if (aborted()) {
            //if render was aborted, remove the frame from the cache as it contains only garbage
            appPTR->removeFromViewerCache(cachedFrame);
            return StatOK;
        }

        if (singleThreaded) {
            if (autoContrast) {
                double vmin, vmax;
                std::pair<double,double> vMinMax = findAutoContrastVminVmax(_imp->lastRenderedImage, channels, roi);
                vmin = vMinMax.first;
                vmax = vMinMax.second;

                ///if vmax - vmin is greater than 1 the gain will be really small and we won't see
                ///anything in the image
                if (vmin == vmax) {
                    vmin = vmax - 1.;
                }
                gain = 1 / (vmax - vmin);
                offset = -vmin / ( vmax - vmin);
            }

            const RenderViewerArgs args(_imp->lastRenderedImage,
                                        textureRect,
                                        channels,
                                        closestPowerOf2,
                                        bitDepth,
                                        gain,
                                        offset,
                                        lutFromColorspace(lut));

            renderFunctor(std::make_pair(texRectClipped.y1,texRectClipped.y2),
                          args,
                          ramBuffer);
        } else {

            int rowsPerThread = std::ceil((double)(texRectClipped.x2 - texRectClipped.x1) / (double)QThread::idealThreadCount());
            // group of group of rows where first is image coordinate, second is texture coordinate
            QList< std::pair<int, int> > splitRows;
            int k = texRectClipped.y1;
            while (k < texRectClipped.y2) {
                int top = k + rowsPerThread;
                int realTop = top > texRectClipped.y2 ? texRectClipped.y2 : top;
                splitRows.push_back(std::make_pair(k,realTop));
                k += rowsPerThread;
            }

            ///if autoContrast is enabled, find out the vmin/vmax before rendering and mapping against new values
            if (autoContrast) {

                rowsPerThread = std::ceil((double)(roi.width()) / (double)QThread::idealThreadCount());
                std::vector<RectI> splitRects;
                k = roi.y1;
                while (k < roi.y2) {
                    int top = k + rowsPerThread;
                    int realTop = top > roi.top() ? roi.top() : top;
                    splitRects.push_back(RectI(roi.left(), k, roi.right(), realTop));
                    k += rowsPerThread;
                }

                QFuture<std::pair<double,double> > future = QtConcurrent::mapped(splitRects,
                                                                                 boost::bind(findAutoContrastVminVmax,
                                                                                             _imp->lastRenderedImage,
                                                                                             channels,
                                                                                             _1));
                future.waitForFinished();
                double vmin = std::numeric_limits<double>::infinity();
                double vmax = -std::numeric_limits<double>::infinity();

                std::pair<double,double> vMinMax;
                foreach (vMinMax, future.results()) {
                    if (vMinMax.first < vmin) {
                        vmin = vMinMax.first;
                    }
                    if (vMinMax.second > vmax) {
                        vmax = vMinMax.second;
                    }
                }
                if (vmax == vmin) {
                    vmin = vmax - 1.;
                }

                gain = 1 / (vmax - vmin);
                offset =  -vmin / (vmax - vmin);
            }

            const RenderViewerArgs args(_imp->lastRenderedImage,
                                        textureRect,
                                        channels,
                                        closestPowerOf2,
                                        bitDepth,
                                        gain,
                                        offset,
                                        lutFromColorspace(lut));

            QtConcurrent::map(splitRows,
                              boost::bind(&renderFunctor,
                                          _1,
                                          args,
                                          ramBuffer)).waitForFinished();

        }
        if (aborted()) {
            //if render was aborted, remove the frame from the cache as it contains only garbage
            appPTR->removeFromViewerCache(cachedFrame);
            return StatOK;
        }
        //we released the input image and force the cache to clear exceeding entries
        appPTR->clearExceedingEntriesFromNodeCache();

    } // !isCached

    if(getVideoEngine()->mustQuit()){
        return StatFailed;
    }

    /////////////////////////////////////////
    // call updateViewer()

    {
        QMutexLocker locker(&_imp->updateViewerMutex);
        assert(!_imp->updateViewerRunning);
        _imp->updateViewerRunning = true;
        assert(_imp->updateViewerParams.ramBuffer == NULL);
        assert(_imp->updateViewerParams.bytesCount == 0);
        _imp->updateViewerParams.ramBuffer = ramBuffer;
        _imp->updateViewerParams.textureRect = textureRect;
        _imp->updateViewerParams.bytesCount = bytesCount;
        _imp->updateViewerParams.gain = gain;
        _imp->updateViewerParams.offset = offset;
        _imp->updateViewerParams.lut = lut;

        if (!aborted()) {
            if (singleThreaded) {
                locker.unlock();
                _imp->updateViewer();
                locker.relock();
                assert(!_imp->updateViewerRunning);
            } else {
                _imp->updateViewerVideoEngine();
                // wait until updateViewer finishes
                while (_imp->updateViewerRunning) {
                    _imp->updateViewerCond.wait(&_imp->updateViewerMutex);
                }
                assert(!_imp->updateViewerRunning);
            }
        }
        // cachedFrame may be freed here, since updateViewer() has finished!
        // invalidate _imp->updateViewerParams
        _imp->updateViewerParams.ramBuffer = NULL;
        _imp->updateViewerParams.textureRect.reset();
        _imp->updateViewerParams.bytesCount = 0;
    }
    // end of cachedFrame scope
    ////////////////////////////////////
    return StatOK;
}

void
renderFunctor(std::pair<int,int> yRange,
              const RenderViewerArgs& args,
              void *buffer)
{
    assert(args.texRect.y1 <= yRange.first && yRange.first <= yRange.second && yRange.second <= args.texRect.y2);
    
    if (args.bitDepth == OpenGLViewerI::FLOAT || args.bitDepth == OpenGLViewerI::HALF_FLOAT) {
        // image is stored as linear, the OpenGL shader with do gamma/sRGB/Rec709 decompression, as well as gain and offset
        scaleToTexture32bits(yRange, args, (float*)buffer);
    }
    else{
        // texture is stored as sRGB/Rec709 compressed 8-bit RGBA
        scaleToTexture8bits(yRange, args, (U32*)buffer);
    }

}

std::pair<double, double>
findAutoContrastVminVmax(boost::shared_ptr<const Natron::Image> inputImage,
                         ViewerInstance::DisplayChannels channels,
                         const RectI& rect)
{
    double localVmin = std::numeric_limits<double>::infinity();
    double localVmax = -std::numeric_limits<double>::infinity();
    for (int y = rect.bottom(); y < rect.top(); ++y) {
        const float* src_pixels = (const float*)inputImage->pixelAt(rect.left(),y);
        ///we fill the scan-line with all the pixels of the input image
        for (int x = rect.left(); x < rect.right(); ++x) {
            double r = src_pixels[0];
            double g = src_pixels[1];
            double b = src_pixels[2];
            double a = src_pixels[3];
            
            double mini,maxi;
            switch (channels) {
                case ViewerInstance::RGB:
                    mini = std::min(std::min(r,g),b);
                    maxi = std::max(std::max(r,g),b);
                    break;
                case ViewerInstance::LUMINANCE:
                    mini = r = 0.299 * r + 0.587 * g + 0.114 * b;
                    maxi = mini;
                    break;
                case ViewerInstance::R:
                    mini = r;
                    maxi = mini;
                    break;
                case ViewerInstance::G:
                    mini = g;
                    maxi = mini;
                    break;
                case ViewerInstance::B:
                    mini = b;
                    maxi = mini;
                    break;
                case ViewerInstance::A:
                    mini = a;
                    maxi = mini;
                    break;
                default:
                    break;
            }
            if (mini < localVmin) {
                localVmin = mini;
            }
            if (maxi > localVmax) {
                localVmax = maxi;
            }
            
            src_pixels +=  4;
        }
    }
    
    return std::make_pair(localVmin, localVmax);
}


#pragma message WARN("Adjust the 8bits and 32bits functions to take into account the image components.")
void
scaleToTexture8bits(std::pair<int,int> yRange,
                    const RenderViewerArgs& args,
                    U32* output)
{
    assert(output);

    int rOffset, gOffset, bOffset;

    const bool luminance = (args.channels == ViewerInstance::LUMINANCE);
    switch (args.channels) {
        case ViewerInstance::RGB:
        case ViewerInstance::LUMINANCE:
            rOffset = 0;
            gOffset = 1;
            bOffset = 2;
            break;
        case ViewerInstance::R:
            rOffset = 0;
            gOffset = 0;
            bOffset = 0;
            break;
        case ViewerInstance::G:
            rOffset = 1;
            gOffset = 1;
            bOffset = 1;
            break;
        case ViewerInstance::B:
            rOffset = 2;
            gOffset = 2;
            bOffset = 2;
            break;
        case ViewerInstance::A:
            rOffset = 3;
            gOffset = 3;
            bOffset = 3;
            break;
    }

    ///the base output buffer
    //U32* output = reinterpret_cast<U32*>(_imp->buffer);

    ///offset the output buffer at the starting point
    output += ((yRange.first - args.texRect.y1) / args.closestPowerOf2) * args.texRect.w;
    
    ///iterating over the scan-lines of the input image
    int dstY = 0;
    for (int y = yRange.first; y < yRange.second; y += args.closestPowerOf2) {
        
        int start = (int)(rand() % ((args.texRect.x2 - args.texRect.x1)/args.closestPowerOf2));
        const float* src_pixels = (const float*)args.inputImage->pixelAt(args.texRect.x1, y);
        
        U32* dst_pixels = output + dstY * args.texRect.w;

        /* go fowards from starting point to end of line: */
        for (int backward = 0;backward < 2; ++backward) {
            
            int dstIndex = backward ? start - 1 : start;
            int srcIndex = dstIndex * args.closestPowerOf2; //< offset from src_pixels
            assert(backward == 1 || (srcIndex >= 0 && srcIndex < (args.texRect.x2 - args.texRect.x1)));
            
            unsigned error_r = 0x80;
            unsigned error_g = 0x80;
            unsigned error_b = 0x80;
            
            while (dstIndex < args.texRect.w && dstIndex >= 0) {
                
                if (srcIndex >= ( args.texRect.x2 - args.texRect.x1)) {
                    break;
                    //dst_pixels[dstIndex] = toBGRA(0,0,0,255);
                } else {
                    double r = src_pixels[srcIndex * 4 + rOffset] * args.gain + args.offset;
                    double g = src_pixels[srcIndex * 4 + gOffset] * args.gain + args.offset;
                    double b = src_pixels[srcIndex * 4 + bOffset] * args.gain + args.offset;
                    if (luminance) {
                        r = 0.299 * r + 0.587 * g + 0.114 * b;
                        g = r;
                        b = r;
                    }
                    
                    if (!args.colorSpace) {
                        dst_pixels[dstIndex] = toBGRA(Color::floatToInt<256>(r),
                                                      Color::floatToInt<256>(g),
                                                      Color::floatToInt<256>(b),
                                                      255);
                    } else {
                        error_r = (error_r&0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(r);
                        error_g = (error_g&0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(g);
                        error_b = (error_b&0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(b);
                        assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
                        dst_pixels[dstIndex] = toBGRA((U8)(error_r >> 8),
                                               (U8)(error_g >> 8),
                                               (U8)(error_b >> 8),
                                               255);
                    }
                }
                if (backward) {
                    --dstIndex;
                    srcIndex -= args.closestPowerOf2;
                } else {
                    ++dstIndex;
                    srcIndex += args.closestPowerOf2;
                }
                
            }
        }
        ++dstY;
    }
}

void
scaleToTexture32bits(std::pair<int,int> yRange,
                     const RenderViewerArgs& args,
                     float *output)
{
    assert(output);

    int rOffset, gOffset, bOffset;

    const bool luminance = (args.channels == ViewerInstance::LUMINANCE);
    switch (args.channels) {
        case ViewerInstance::RGB:
        case ViewerInstance::LUMINANCE:
            rOffset = 0;
            gOffset = 1;
            bOffset = 2;
            break;
        case ViewerInstance::R:
            rOffset = 0;
            gOffset = 0;
            bOffset = 0;
            break;
        case ViewerInstance::G:
            rOffset = 1;
            gOffset = 1;
            bOffset = 1;
            break;
        case ViewerInstance::B:
            rOffset = 2;
            gOffset = 2;
            bOffset = 2;
            break;
        case ViewerInstance::A:
            rOffset = 3;
            gOffset = 3;
            bOffset = 3;
            break;
    }

    ///the base output buffer
    //float* output = reinterpret_cast<float*>(_imp->buffer);
    
    ///the width of the output buffer multiplied by the channels count
    int dst_width = args.texRect.w * 4;
    
    ///offset the output buffer at the starting point
    output += ((yRange.first - args.texRect.y1) / args.closestPowerOf2) * dst_width;
    
    ///iterating over the scan-lines of the input image
    int dstY = 0;
    for (int y = yRange.first; y < yRange.second; y += args.closestPowerOf2) {
        
        const float* src_pixels = (const float*)args.inputImage->pixelAt(args.texRect.x1, y);
        float* dst_pixels = output + dstY * dst_width;
        
        ///we fill the scan-line with all the pixels of the input image
        for (int x = args.texRect.x1; x < args.texRect.x2; x += args.closestPowerOf2) {
            double r = src_pixels[rOffset];
            double g = src_pixels[gOffset];
            double b = src_pixels[bOffset];
            if(luminance){
                r = 0.299 * r + 0.587 * g + 0.114 * b;
                g = r;
                b = r;
            }
            *dst_pixels++ = r;
            *dst_pixels++ = g;
            *dst_pixels++ = b;
            *dst_pixels++ = 1.;
            
            src_pixels += args.closestPowerOf2 * 4;
        }
        ++dstY;
    }
    
}



void
ViewerInstance::wakeUpAnySleepingThread()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    QMutexLocker locker(&_imp->updateViewerMutex);
    _imp->updateViewerRunning = false;
    _imp->updateViewerCond.wakeAll();
}

void
ViewerInstance::ViewerInstancePrivate::updateViewerVideoEngine()
{
    // always running in the VideoEngine thread
    assertVideoEngine();

    emit doUpdateViewer();
}

void
ViewerInstance::ViewerInstancePrivate::updateViewer()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    QMutexLocker locker(&updateViewerMutex);
    assert(updateViewerRunning);
    uiContext->makeOpenGLcontextCurrent();
    if (!instance->aborted()) {
        // how do you make sure _imp->updateViewerParams.ramBuffer is not freed during this operation?
        /// It is not freed as long as the cachedFrame shared_ptr in renderViewer has a used_count greater than 1.
        /// i.e. until renderViewer exits.
        /// Since updateViewer() is in the scope of cachedFrame, and renderViewer waits for the completion
        /// of updateViewer(), it is guaranteed not to be freed before the viewer is actually done with it.
        /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
        uiContext->transferBufferFromRAMtoGPU(updateViewerParams.ramBuffer,
                                              updateViewerParams.bytesCount,
                                              updateViewerParams.textureRect,
                                              updateViewerParams.gain,
                                              updateViewerParams.offset,
                                              updateViewerParams.lut,
                                              updateViewerPboIndex);
        updateViewerPboIndex = (updateViewerPboIndex+1)%2;
    }
    
    uiContext->updateColorPicker();
    uiContext->redraw();
    
    updateViewerRunning = false;
    updateViewerCond.wakeOne();
}


bool
ViewerInstance::isInputOptional(int n) const
{
    //activeInput() is MT-safe
    return n != activeInput();
}

void
ViewerInstance::onGainChanged(double exp)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        _imp->viewerParamsGain = exp;
    }
    if((_imp->uiContext->getBitDepth() == OpenGLViewerI::BYTE  || !_imp->uiContext->supportsGLSL())
       && input(activeInput()) != NULL && !getApp()->getProject()->isLoadingProject()) {
        refreshAndContinueRender(false);
    } else {
        emit mustRedraw();
    }
    
}

void
ViewerInstance::onAutoContrastChanged(bool autoContrast,bool refresh)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        _imp->viewerParamsAutoContrast = autoContrast;
    }
    if (refresh && input(activeInput()) != NULL && !getApp()->getProject()->isLoadingProject()){
        refreshAndContinueRender(false);
    }
}

bool
ViewerInstance::isAutoContrastEnabled() const
{
    // MT-safe
    QMutexLocker l(&_imp->viewerParamsMutex);
    return _imp->viewerParamsAutoContrast;
}

void
ViewerInstance::onColorSpaceChanged(const QString& colorspaceName)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    QMutexLocker l(&_imp->viewerParamsMutex);
    
    if (colorspaceName == "Linear(None)") {
        _imp->viewerParamsLut = Linear;
    } else if (colorspaceName == "sRGB") {
        _imp->viewerParamsLut = sRGB;
    } else if (colorspaceName == "Rec.709") {
        _imp->viewerParamsLut = Rec709;
    }

    if ((_imp->uiContext->getBitDepth() == OpenGLViewerI::BYTE  || !_imp->uiContext->supportsGLSL())
       && input(activeInput()) != NULL) {
        refreshAndContinueRender(false);
    } else {
        emit mustRedraw();
    }
}

void
ViewerInstance::onViewerCacheFrameAdded()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    emit addedCachedFrame(getApp()->getTimeLine()->currentFrame());
}

void
ViewerInstance::setDisplayChannels(DisplayChannels channels)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        _imp->viewerParamsChannels = channels;
    }
    if (!getApp()->getProject()->isLoadingProject()) {
        refreshAndContinueRender(false);
    }
}

void
ViewerInstance::disconnectViewer()
{
    // always running in the VideoEngine thread
    _imp->assertVideoEngine();

    if (getVideoEngine()->isWorking()) {
        getVideoEngine()->abortRendering(false); // aborting current work
    }
    //_lastRenderedImage.reset(); // if you uncomment this, _lastRenderedImage is not set back when you reconnect the viewer immediately after disconnecting
    emit viewerDisconnected();
}

bool
ViewerInstance::getColorAt(int x,int y,float* r,float* g,float* b,float* a,bool forceLinear)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    if (!_imp->lastRenderedImage) {
        return false;
    }
    
    const RectI& bbox = _imp->lastRenderedImage->getRoD();
    
    if (x < bbox.x1 || x >= bbox.x2 || y < bbox.y1 || y >= bbox.y2) {
        return false;
    }
    
    const float* pix = _imp->lastRenderedImage->pixelAt(x, y);
    *r = *pix;
    *g = *(pix + 1);
    *b = *(pix + 2);
    *a = *(pix + 3);
    ViewerColorSpace lut;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        lut = _imp->viewerParamsLut;
    }
    const Natron::Color::Lut* colorSpace = lutFromColorspace(lut);
    if (!forceLinear && colorSpace) {
        float from[3];
        from[0] = *r;
        from[1] = *g;
        from[2] = *b;
        float to[3];
        colorSpace->to_float_planar(to, from, 3);
        *r = to[0];
        *g = to[1];
        *b = to[2];
    }
    return true;
}

bool
ViewerInstance::supportsGLSL() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    ///This is a short-cut, this is primarily used when the user switch the
    /// texture mode in the preferences menu. If the hardware doesn't support GLSL
    /// it returns false, true otherwise. @see Settings::onKnobValueChanged
    return _imp->uiContext->supportsGLSL();
}

void
ViewerInstance::redrawViewer()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    emit mustRedraw();
}

boost::shared_ptr<Natron::Image>
ViewerInstance::getLastRenderedImage() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    if (!getNode()->isActivated()) {
        return boost::shared_ptr<Natron::Image>();
    }
    return _imp->lastRenderedImage;
}

int
ViewerInstance::getLutType() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);
    return _imp->viewerParamsLut;
}

double
ViewerInstance::getGain() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);
    return _imp->viewerParamsGain;
}


ViewerInstance::DisplayChannels
ViewerInstance::getChannels() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);
    return _imp->viewerParamsChannels;
}
