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
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt<<(
#include <QtCore/QFutureWatcher>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)

#include "Global/MemoryInfo.h"
#include "Engine/Node.h"
#include "Engine/ImageInfo.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
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

#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif

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
lutFromColorspace(Natron::ViewerColorSpace cs) WARN_UNUSED_RETURN;

static const Natron::Color::Lut*
lutFromColorspace(Natron::ViewerColorSpace cs)
{
    const Natron::Color::Lut* lut;
    switch (cs) {
        case Natron::sRGB:
            lut = Natron::Color::LutManager::sRGBLut();
            break;
        case Natron::Rec709:
            lut = Natron::Color::LutManager::Rec709Lut();
            break;
        case Natron::Linear:
        default:
            lut = 0;
            break;
    }
    if (lut) {
        lut->validate();
    }
    return lut;
}

namespace {
class MetaTypesRegistration
{
public:
    inline MetaTypesRegistration()
    {
        qRegisterMetaType<boost::shared_ptr<UpdateViewerParams> >("boost::shared_ptr<UpdateViewerParams>");
    }
};
}

static MetaTypesRegistration registration;

Natron::EffectInstance*
ViewerInstance::BuildEffect(boost::shared_ptr<Natron::Node> n)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    return new ViewerInstance(n);
}

ViewerInstance::ViewerInstance(boost::shared_ptr<Node> node)
: Natron::OutputEffectInstance(node)
, _imp(new ViewerInstancePrivate(this))
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    if(node) {
        connect(node.get(),SIGNAL(nameChanged(QString)),this,SLOT(onNodeNameChanged(QString)));
    }
    QObject::connect(this,SIGNAL(disconnectTextureRequest(int)),this,SLOT(executeDisconnectTextureRequestOnMainThread(int)));
    QObject::connect(_imp.get(),SIGNAL(mustRedrawViewer()),this,SLOT(redrawViewer()));
}

ViewerInstance::~ViewerInstance()
{

    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    if (getVideoEngine()) {
        ///Thread must have been killed before.
        assert(!getVideoEngine()->isThreadRunning());
    }
    
    // If _imp->updateViewerRunning is true, that means that the next updateViewer call was
    // not yet processed. Since we're in the main thread and it is processed in the main thread,
    // there is no way to wait for it (locking the mutex would cause a deadlock).
    // We don't care, after all.
    //{
    //    QMutexLocker locker(&_imp->updateViewerMutex);
    //    assert(!_imp->updateViewerRunning);
    //}
    if (_imp->uiContext) {
        _imp->uiContext->removeGUI();
    }

    if (_imp->bufferAllocated) {
        free(_imp->buffer);
        _imp->bufferAllocated = 0;
    }
    
    U64 sizeToUnregister = 0;
    if (_imp->lastRenderedImage[0]) {
        sizeToUnregister += _imp->lastRenderedImage[0]->size();
    }
    if (_imp->lastRenderedImage[1]) {
        sizeToUnregister += _imp->lastRenderedImage[1]->size();
    }
    if (sizeToUnregister != 0) {
        unregisterPluginMemory(sizeToUnregister);
    }
}

void
ViewerInstance::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_IMAGE);
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
ViewerInstance::setUiContext(OpenGLViewerI* viewer)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    // VideoEngine must not be created (or there would be a race condition)
    assert(!_imp->threadIdVideoEngine);

    _imp->uiContext = viewer;
}

void
ViewerInstance::invalidateUiContext()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    _imp->uiContext = NULL;
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
    return dynamic_cast<InspectorNode*>(getNode().get())->activeInput(); // not MT-SAFE!
}

int
ViewerInstance::getMaxInputCount() const
{
    // runs in the VideoEngine thread or in the main thread
    //_imp->assertVideoEngine();
    // but is it really MT-SAFE?

    return getNode()->getMaxInputCount();
}

void
ViewerInstance::getFrameRange(SequenceTime *first,
                              SequenceTime *last)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    SequenceTime inpFirst = 0,inpLast = 0;
    int activeInputs[2];
    getActiveInputs(activeInputs[0], activeInputs[1]);
    EffectInstance* n1 = input_other_thread(activeInputs[0]);
    if (n1) {
        n1->getFrameRange_public(&inpFirst,&inpLast);
    }
    *first = inpFirst;
    *last = inpLast;
    
    inpFirst = 0;
    inpLast = 0;
    
    EffectInstance* n2 = input_other_thread(activeInputs[1]);
    if (n2) {
        n2->getFrameRange_public(&inpFirst,&inpLast);
        if (inpFirst < *first) {
            *first = inpFirst;
        }
        
        if (inpLast > *last) {
            *last = inpLast;
        }
    }
    
   
}


void ViewerInstance::executeDisconnectTextureRequestOnMainThread(int index)
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->uiContext) {
        _imp->uiContext->disconnectInputTexture(index);
    }
}

Natron::Status
ViewerInstance::renderViewer(SequenceTime time,
                             bool singleThreaded,bool isSequentialRender)
{
    if (!_imp->uiContext) {
        return StatReplyDefault;
    }
    Natron::Status ret[2] = { StatOK,StatOK };
    for (int i = 0; i < 2; ++i) {
        if (i == 1 && _imp->uiContext->getCompositingOperator() == Natron::OPERATOR_NONE) {
            break;
        }
        ret[i] = renderViewer_internal(time, singleThreaded, isSequentialRender, i);
        if (ret[i] == StatFailed) {
            emit disconnectTextureRequest(i);
        }
    }
    
    _imp->redrawViewer();
    
    if (ret[0] == StatFailed && ret[1] == StatFailed) {
        return StatFailed;
    }
    return StatOK;
}

Natron::Status
ViewerInstance::renderViewer_internal(SequenceTime time,bool singleThreaded,bool isSequentialRender,
                                     int textureIndex)
{
    // always running in the VideoEngine thread
    _imp->assertVideoEngine();

#ifdef NATRON_LOG
    Natron::Log::beginFunction(getName(),"renderViewer");
    Natron::Log::print(QString("Time "+QString::number(time)).toStdString());
#endif

    
    if (aborted()) {
        return StatOK;
    }
    

    Format dispW;
    getRenderFormat(&dispW);
    int viewsCount = getRenderViewsCount();
    int view = (viewsCount > 0  && _imp->uiContext) ? _imp->uiContext->getCurrentView() : 0;

    int activeInputIndex;
    if (textureIndex == 0) {
        QMutexLocker l(&_imp->activeInputsMutex);
        activeInputIndex =  _imp->activeInputs[0];
    } else {
        QMutexLocker l(&_imp->activeInputsMutex);
        activeInputIndex =  _imp->activeInputs[1];
    }
    
    
    ///Iterate while nodes are disabled to find the first active input
    EffectInstance* activeInputToRender = input_other_thread(activeInputIndex);
    while (activeInputToRender && activeInputToRender->getNode()->isNodeDisabled()) {
        ///we forward this node to the last connected non-optional input
        ///if there's only optional inputs connected, we return the last optional input
        int lastOptionalInput = -1;
        int inputNb = -1;
        for (int i = activeInputToRender->getMaxInputCount() - 1; i >= 0; --i) {
            bool optional = activeInputToRender->isInputOptional(i);
            if (!optional && activeInputToRender->getNode()->input_other_thread(i)) {
                inputNb = i;
                break;
            } else if (optional && lastOptionalInput == -1) {
                lastOptionalInput = i;
            }
        }
        if (inputNb == -1) {
            inputNb = lastOptionalInput;
        }
        activeInputToRender = activeInputToRender->input_other_thread(inputNb);
    }
    if (!activeInputToRender) {
        return StatFailed;
    }
    

    
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
    
    RenderScale scale;
    int mipMapLevel;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        scale.x = Natron::Image::getScaleFromMipMapLevel(_imp->viewerMipMapLevel);
        scale.y = scale.x;
        mipMapLevel = _imp->viewerMipMapLevel;
    }
     
    assert(_imp->uiContext);
    double zoomFactor = _imp->uiContext->getZoomFactor();
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow(2,-std::ceil(std::log(zoomFactor) / M_LN2));
    mipMapLevel = std::max((double)mipMapLevel,std::log(closestPowerOf2) / M_LN2);

    // If it's eSupportsMaybe and mipMapLevel!=0, don't forget to update
    // this after the first call to getRegionOfDefinition().
    EffectInstance::SupportsEnum supportsRS = activeInputToRender->supportsRenderScaleMaybe();
    if (supportsRS == eSupportsNo) {
        scale.x = scale.y = 1.;
    } else {
        scale.x = scale.y = Natron::Image::getScaleFromMipMapLevel(mipMapLevel);
    }
    closestPowerOf2 = 1 << mipMapLevel;
    
    ImageComponents components;
    ImageBitDepth imageDepth;
    activeInputToRender->getPreferredDepthAndComponents(-1, &components, &imageDepth);
    
    emit imageFormatChanged(textureIndex,components, imageDepth);
    
    U64 inputNodeHash = activeInputToRender->getHash();
        
    Natron::ImageKey inputImageKey = Natron::Image::makeKey(inputNodeHash, time, mipMapLevel,view);
    RectD rod;
    RectI bounds;
    bool isRodProjectFormat = false;
    int inputIdentityNumber = -1;
    SequenceTime inputIdentityTime = time;
    
    
    bool isInputImgCached = Natron::getImageFromCache(inputImageKey, &cachedImgParams,&inputImage);
    
    ////Lock the output image so that multiple threads do not access for writing at the same time.
    ////When it goes out of scope the lock will be released automatically
    boost::shared_ptr<OutputImageLocker> imageLock;
    
    if (isInputImgCached) {
        assert(inputImage);
        imageLock.reset(new OutputImageLocker(getNode().get(),inputImage));
        
        inputIdentityNumber = cachedImgParams->getInputNbIdentity();
        inputIdentityTime = cachedImgParams->getInputTimeIdentity();
        if (forceRender) {
            ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
            ///we're sure renderRoIInternal will compute the whole image again.
            ///We must use the cache facility anyway because we rely on it for caching the results
            ///of actions which is necessary to avoid recursive actions.
            inputImage->clearBitmap();
        }
        
        ///If components are different but convertible without damage, or bit depth is different, keep this image, convert it
        ///and continue render on it. This is in theory still faster than ignoring the image and doing a full render again.
        if ((inputImage->getComponents() != components &&
             Image::hasEnoughDataToConvert(inputImage->getComponents(),components)) ||
            inputImage->getBitDepth() != imageDepth) {
            ///Convert the image to the requested components
            boost::shared_ptr<Image> remappedImage(new Image(components,
                                                             inputImage->getRoD(),
                                                             inputImage->getBounds(),
                                                             mipMapLevel,
                                                             imageDepth));
            inputImage->convertToFormat(inputImage->getBounds(),
                                        getApp()->getDefaultColorSpaceForBitDepth(inputImage->getBitDepth()),
                                        getApp()->getDefaultColorSpaceForBitDepth(imageDepth),
                                        3, false, true,
                                        remappedImage.get());
            
            ///switch the pointer
            inputImage = remappedImage;
        } else if (inputImage->getComponents() != components) {
            assert(!Image::hasEnoughDataToConvert(inputImage->getComponents(),components));
            ///we cannot convert without loosing data of some channels, we better off render everything again
            isInputImgCached = false;
            appPTR->removeFromNodeCache(inputImage);
            cachedImgParams.reset();
            imageLock.reset();
            inputImage.reset();
        }
    }
    
    
    ////While the inputs are identity get the RoD of the first non identity input
    while (inputIdentityNumber != -1 && isInputImgCached) {
        EffectInstance* recursiveInput = activeInputToRender->input_other_thread(inputIdentityNumber);
        if (recursiveInput) {
            inputImageKey = Natron::Image::makeKey(recursiveInput->getHash(), inputIdentityTime, mipMapLevel,view);
            isInputImgCached = Natron::getImageFromCache(inputImageKey, &cachedImgParams,&inputImage);
            if (isInputImgCached) {
                inputIdentityNumber = cachedImgParams->getInputNbIdentity();
                inputIdentityTime = cachedImgParams->getInputTimeIdentity();
                if (forceRender) {
                    ///If we want to by-pass the cache, we will just zero-out the bitmap of the image, so
                    ///we're sure renderRoIInternal will compute the whole image again.
                    ///We must use the cache facility anyway because we rely on it for caching the results
                    ///of actions which is necessary to avoid recursive actions.
                    inputImage->clearBitmap();
                }
            }
            activeInputToRender = recursiveInput;
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
        
        rod = inputImage->getRoD();
        bounds = inputImage->getBounds();
        isRodProjectFormat = cachedImgParams->isRodProjectFormat();
        
        ///since we are going to render a new image, decrease the current memory use of the viewer by
        ///the amount of the current image, and increase it after we rendered the new image.
        bool registerMem = false;
        {
            QMutexLocker l(&_imp->lastRenderedImageMutex);
            if (_imp->lastRenderedImage[textureIndex] != inputImage) {
                if (_imp->lastRenderedImage[textureIndex]) {
                    unregisterPluginMemory(_imp->lastRenderedImage[textureIndex]->size());
                }
                _imp->lastRenderedImage[textureIndex] = inputImage;
                registerMem = true;
            }
        }
        if (registerMem) {
            ///notify that the viewer is actually using that much memory.
            ///It will never be freed unless we delete the node completely from the undo/redo stack.
            registerPluginMemory(inputImage->size());
        }
        
    }  else {
        Status stat = activeInputToRender->getRegionOfDefinition_public(time, scale, view, &rod, &isRodProjectFormat);
        if(stat == StatFailed){
#ifdef NATRON_LOG
            Natron::Log::print(QString("getRegionOfDefinition returned StatFailed.").toStdString());
            Natron::Log::endFunction(getName(),"renderViewer");
#endif
            return stat;
        }
        // update scale after the first call to getRegionOfDefinition
        if (supportsRS == eSupportsMaybe && mipMapLevel != 0) {
            supportsRS = activeInputToRender->supportsRenderScaleMaybe();
            if (supportsRS == eSupportsNo) {
                scale.x = scale.y = 1.;
            }
        }
        isRodProjectFormat = ifInfiniteclipRectToProjectDefault(&rod);

        // For the viewer, we need the enclosing rectangle to avoid black borders.
        // Do this here to avoid infinity values.
        rod.toPixelEnclosing(mipMapLevel, &bounds);
    }

    emit rodChanged(rod, textureIndex);

    assert(_imp->uiContext);
    bool isClippingToProjectWindow = _imp->uiContext->isClippingImageToProjectWindow();
    if (!isClippingToProjectWindow) {
        dispW.set(rod);
    }
    
    /*computing the RoI*/

    ///The RoI of the viewer, given the bounds (which takes into account the current render scale).
    ///The roi is then in pixel coordinates.
    assert(_imp->uiContext);
    RectI roi = _imp->uiContext->getImageRectangleDisplayed(bounds,mipMapLevel);
    
    
    ///Clip the roi  the project window (in pixel coordinates)
    RectI pixelDispW;
    if (isClippingToProjectWindow) {
        dispW.toPixelEnclosing(mipMapLevel, &pixelDispW);
        roi.intersect(pixelDispW, &roi);
    }

    ////Texrect is the coordinates of the 4 corners of the texture in the bounds with the current zoom
    ////factor taken into account.
    RectI texRect;
    double tileSize = std::pow(2., (double)appPTR->getCurrentSettings()->getViewerTilesPowerOf2());
    texRect.x1 = std::floor(((double)roi.x1) / tileSize) * tileSize;
    texRect.y1 = std::floor(((double)roi.y1) / tileSize) * tileSize;
    texRect.x2 = std::ceil(((double)roi.x2) / tileSize) * tileSize;
    texRect.y2 = std::ceil(((double)roi.y2) / tileSize) * tileSize;
    
    if (texRect.width() == 0 || texRect.height() == 0) {
        return StatOK;
    }
    
    ///TexRectClipped is the same as texRect but without the zoom factor taken into account (in pixel coords)
    RectI texRectClipped ;
    
    ///Make sure the bounds of the area to render in the texture lies in the bounds
    texRect.intersect(bounds, &texRectClipped);
    ///Clip again against the project window
    if (isClippingToProjectWindow) {
        ///it has already been computed in the previous clip above
        assert(!pixelDispW.isNull());
        texRectClipped.intersect(pixelDispW, &texRectClipped);
    }
    
    ///The width and height of the texture must at least contain the roi
    // RectI texRectClippedDownscaled = texRectClipped.downscalePowerOfTwoSmallestEnclosing(std::log(closestPowerOf2) / M_LN2);
    
    
    ///Texture rect contains the pixel coordinates in the image to be rendered
    TextureRect textureRect(texRectClipped.x1,texRectClipped.y1,texRectClipped.x2,
                            texRectClipped.y2,texRectClipped.width(),texRectClipped.height(),closestPowerOf2);

    size_t bytesCount = textureRect.w * textureRect.h * 4;
    if (bytesCount == 0) {
        return StatOK;
    }
    
    assert(_imp->uiContext);
    OpenGLViewerI::BitDepth bitDepth = _imp->uiContext->getBitDepth();
    
    //half float is not supported yet so it is the same as float
    if (bitDepth == OpenGLViewerI::FLOAT || bitDepth == OpenGLViewerI::HALF_FLOAT) {
        bytesCount *= sizeof(float);
    }
    
    ///make a copy of the auto contrast enabled state, so render threads only refer to that copy
    double gain;
    double offset = 0.; // 0 except for autoContrast
    bool autoContrast;
    Natron::ViewerColorSpace lut;
    ViewerInstance::DisplayChannels channels;
    {
        QMutexLocker locker(&_imp->viewerParamsMutex);
        gain = _imp->viewerParamsGain;
        lut = _imp->viewerParamsLut;
        autoContrast = _imp->viewerParamsAutoContrast;
        channels = _imp->viewerParamsChannels;
    }

    std::string inputToRenderName = activeInputToRender->getNode()->getName_mt_safe();
    U64 nodeHash = getHash();
    FrameKey key(time,
                 nodeHash,
                 gain,
                 lut,
                 (int)bitDepth,
                 channels,
                 view,
                 textureRect,
                 scale,
                 inputToRenderName);

    /////////////////////////////////////
    // start UpdateViewerParams scope
    //
    boost::shared_ptr<UpdateViewerParams> params(new UpdateViewerParams);
    bool isCached = false;
    
    ///if we want to force a refresh, we by-pass the cache
    bool byPassCache = false;
    if (!forceRender) {
        ///we never use the texture cache when the user RoI is enabled, otherwise we would have
        ///zillions of textures in the cache, each a few pixels different.
        assert(_imp->uiContext);
        if (!_imp->uiContext->isUserRegionOfInterestEnabled() && !autoContrast) {
            boost::shared_ptr<const Natron::FrameParams> cachedFrameParams;
            isCached = Natron::getTextureFromCache(key, &cachedFrameParams, &params->cachedFrame);
            assert(!isCached || cachedFrameParams);
            
            ///The user changed a parameter or the tree, just clear the cache
            ///it has no point keeping the cache because we will never find these entries again.
            U64 lastRenderHash;
            boost::shared_ptr<Natron::FrameEntry> lastRenderedTex;
            {
                
                QMutexLocker l(&_imp->lastRenderedTextureMutex);
                lastRenderHash = _imp->lastRenderHash;
                lastRenderedTex = _imp->lastRenderedTexture;
                    
            }
            if (lastRenderedTex && lastRenderHash != nodeHash) {
                appPTR->removeAllTexturesFromCacheWithMatchingKey(lastRenderHash);
                {
                    QMutexLocker l(&_imp->lastRenderedTextureMutex);
                    _imp->lastRenderedTexture.reset();
                }
            }

            
        }
    } else {
        byPassCache = true;
    }

    unsigned char* ramBuffer = NULL;

    bool usingRAMBuffer = false;
    
    if (isCached) {
        /*Found in viewer cache, we execute the cached engine and leave*/

        // how do you make sure cachedFrame->data() is not freed after this line?
        ///It is not freed as long as the cachedFrame shared_ptr has a used_count greater than 1.
        ///Since it is used during the whole function scope it is guaranteed not to be freed before
        ///The viewer is actually done with it.
        /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
        ramBuffer = params->cachedFrame->data();

        {
            QMutexLocker l(&_imp->lastRenderedTextureMutex);
            _imp->lastRenderedTexture = params->cachedFrame;
            _imp->lastRenderHash = nodeHash;
        }
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
        assert(_imp->uiContext);
        if (byPassCache || _imp->uiContext->isUserRegionOfInterestEnabled() || autoContrast) {
            assert(!params->cachedFrame);
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
            }
            ramBuffer = (unsigned char*)_imp->buffer;
            usingRAMBuffer = true;

        } else {
            boost::shared_ptr<const Natron::FrameParams> cachedFrameParams =
            FrameEntry::makeParams(bounds, key.getBitDepth(), textureRect.w, textureRect.h);
            
            bool textureIsCached = Natron::getTextureFromCacheOrCreate(key, cachedFrameParams, &params->cachedFrame);
            if (!params->cachedFrame) {
                std::stringstream ss;
                ss << "Failed to allocate a texture of ";
                ss << printAsRAM(cachedFrameParams->getElementsCount() * sizeof(FrameEntry::data_t)).toStdString();
                Natron::errorDialog(QObject::tr("Out of memory").toStdString(),ss.str());
                return StatFailed;
            }
            ///note that unlike  getImageFromCacheOrCreate in EffectInstance::renderRoI, we
            ///are sure that this time the image was not in the cache and we created it because this functino
            ///is not multi-threaded.
            assert(!textureIsCached);
            
            assert(params->cachedFrame);
            // how do you make sure cachedFrame->data() is not freed after this line?
            ///It is not freed as long as the cachedFrame shared_ptr has a used_count greater than 1.
            ///Since it is used during the whole function scope it is guaranteed not to be freed before
            ///The viewer is actually done with it.
            /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
            ramBuffer = params->cachedFrame->data();
            
            {
                QMutexLocker l(&_imp->lastRenderedTextureMutex);
                _imp->lastRenderedTexture = params->cachedFrame;
                _imp->lastRenderHash = nodeHash;
            }
        }
        assert(ramBuffer);

        ///intersect the image render window to the actual image region of definition.
        texRectClipped.intersect(bounds, &texRectClipped);
        
        boost::shared_ptr<Natron::Image> originalInputImage = inputImage;
        
        bool renderedCompletely = false;
        boost::shared_ptr<Natron::Image> downscaledImage = inputImage;
        boost::shared_ptr<Natron::Image> lastRenderedImage;

        ///If the plug-in doesn't support the render scale and we found an image cached but which still
        ///contains some stuff to render we don't want to use it, instead we need to upscale the image

        if (!activeInputToRender->supportsRenderScale() && isInputImgCached && mipMapLevel != 0) {

            /// If the list is empty then we already rendered it all
            /// Otherwise we have to upscale the found image, render what we need and downscale it again
            std::list<RectI> rectsToRender = inputImage->getRestToRender(texRectClipped);
            RectI bounds;
            unsigned int mipMapLevel = 0;
            rod.toPixelEnclosing(mipMapLevel, &bounds);
            if (!rectsToRender.empty()) {
                boost::shared_ptr<Natron::Image> upscaledImage(new Natron::Image(components,
                                                                                 rod,
                                                                                 bounds,
                                                                                 mipMapLevel,
                                                                                 downscaledImage->getBitDepth()));
                downscaledImage->scaleBox(downscaledImage->getBounds(), upscaledImage.get());
                inputImage = upscaledImage;
                if (isInputImgCached) {
                    lastRenderedImage = downscaledImage;
                }
            } else {
                renderedCompletely = true;
            }
            
        } else {
            if (isInputImgCached) {
                lastRenderedImage = inputImage;
            }
        }
        _node->notifyInputNIsRendering(activeInputIndex);

      
        
       
        
        
        if (!renderedCompletely) {
            // If an exception occurs here it is probably fatal, since
            // it comes from Natron itself. All exceptions from plugins are already caught
            // by the HostSupport library.
            // We catch it  and rethrow it just to notify the rendering is done.
            try {
                if (isInputImgCached) {
                    ///if the input image is cached, call the shorter version of renderRoI which doesn't do all the
                    ///cache lookup things because we already did it ourselves.
                        activeInputToRender->renderRoI(time, scale, mipMapLevel, view, texRectClipped, rod, cachedImgParams, inputImage,downscaledImage,isSequentialRender,true,byPassCache,inputNodeHash);
                    
                } else {
                    
                    lastRenderedImage = activeInputToRender->renderRoI(
                    EffectInstance::RenderRoIArgs(time,
                                                  scale,
                                                  mipMapLevel,
                                                  view,
                                                  texRectClipped,
                                                  isSequentialRender,
                                                  true,
                                                  byPassCache,
                                                  rod,
                                                  components,
                                                  imageDepth)); //< render the input depth as the viewer can handle it
                    
                    if (!lastRenderedImage) {
                         _node->notifyInputNIsFinishedRendering(activeInputIndex);
                        return StatFailed;
                    }
                    
                    ///since we are going to render a new image, decrease the current memory use of the viewer by
                    ///the amount of the current image, and increase it after we rendered the new image.
                    bool registerMem = false;
                    {
                        QMutexLocker l(&_imp->lastRenderedImageMutex);
                        if (_imp->lastRenderedImage[textureIndex] != lastRenderedImage) {
                            if (_imp->lastRenderedImage[textureIndex]) {
                                unregisterPluginMemory(_imp->lastRenderedImage[textureIndex]->size());
                            }
                            if (!activeInputToRender->aborted()) {
                                _imp->lastRenderedImage[textureIndex] = lastRenderedImage;
                            } else {
                                _imp->lastRenderedImage[textureIndex].reset();
                            }
                            registerMem = true;
                        }
                    }
                    if (registerMem && lastRenderedImage) {
                        ///notify that the viewer is actually using that much memory.
                        ///It will never be freed unless we delete the node completely from the undo/redo stack.
                        registerPluginMemory(lastRenderedImage->size());
                    }
                }
            } catch (...) {
                _node->notifyInputNIsFinishedRendering(activeInputIndex);
                appPTR->removeFromViewerCache(params->cachedFrame);
                
                ///If the plug-in was aborted, this is probably not a failure due to render but because of abortion.
                ///Don't forward the exception in that case.
                if (!activeInputToRender->aborted()) {
                    throw;
                } else {
                    return StatOK;
                }
            }
            
        }
        _node->notifyInputNIsFinishedRendering(activeInputIndex);
        
        
        if (!lastRenderedImage) {
            //if render was aborted, remove the frame from the cache as it contains only garbage
            appPTR->removeFromViewerCache(params->cachedFrame);
            return StatFailed;
        }
        
        
        
        if (activeInputToRender->aborted()) {
            //if render was aborted, remove the frame from the cache as it contains only garbage
            appPTR->removeFromViewerCache(params->cachedFrame);
            return StatOK;
        }
        
        ViewerColorSpace srcColorSpace = getApp()->getDefaultColorSpaceForBitDepth(lastRenderedImage->getBitDepth());
        
        if (singleThreaded) {
            if (autoContrast) {
                double vmin, vmax;
                std::pair<double,double> vMinMax = findAutoContrastVminVmax(lastRenderedImage, channels, roi);
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

            const RenderViewerArgs args(lastRenderedImage,
                                        textureRect,
                                        channels,
                                        1,
                                        bitDepth,
                                        gain,
                                        offset,
                                        lutFromColorspace(srcColorSpace),
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
                                                                                             lastRenderedImage,
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

            const RenderViewerArgs args(lastRenderedImage,
                                        textureRect,
                                        channels,
                                        1,
                                        bitDepth,
                                        gain,
                                        offset,
                                        lutFromColorspace(srcColorSpace),
                                        lutFromColorspace(lut));

            QtConcurrent::map(splitRows,
                              boost::bind(&renderFunctor,
                                          _1,
                                          args,
                                          ramBuffer)).waitForFinished();

        }
        if (activeInputToRender->aborted()) {
            //if render was aborted, remove the frame from the cache as it contains only garbage
            appPTR->removeFromViewerCache(params->cachedFrame);
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

    if (!activeInputToRender->aborted()) {
        QMutexLocker locker(&_imp->updateViewerMutex);
        // wait until previous updateViewer (if any) finishes
        
        if (!usingRAMBuffer) {
            while (_imp->updateViewerRunning) {
                _imp->updateViewerCond.wait(&_imp->updateViewerMutex);
            }
        }
        assert(!_imp->updateViewerRunning);
        _imp->updateViewerRunning = true;
        params->ramBuffer = ramBuffer;
        params->textureRect = textureRect;
        params->bytesCount = bytesCount;
        params->gain = gain;
        params->offset = offset;
        params->lut = lut;
        params->mipMapLevel = (unsigned int)mipMapLevel;
        params->textureIndex = textureIndex;
        if (!activeInputToRender->aborted()) {
            if (singleThreaded) {
                locker.unlock();
                _imp->updateViewer(params);
                locker.relock();
                assert(!_imp->updateViewerRunning);
            } else {
                _imp->updateViewerVideoEngine(params);
            }
        } else {
            _imp->updateViewerRunning = false;
        }
        if (usingRAMBuffer) {
            while (_imp->updateViewerRunning) {
                _imp->updateViewerCond.wait(&_imp->updateViewerMutex);
            }
        }
    }
    // end of boost::shared_ptr<UpdateUserParams> scope... but it still lives inside updateViewer()
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
            
            double mini, maxi;
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
                    mini = 0.;
                    maxi = 0.;
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


template <typename PIX,int maxValue>
void scaleToTexture8bits_internal(const std::pair<int,int>& yRange,
                                  const RenderViewerArgs& args,
                                  U32* output,
                                  int rOffset,int gOffset,int bOffset,int nComps)
{
    
    Natron::ImageComponents comps = args.inputImage->getComponents();
    const bool luminance = (args.channels == ViewerInstance::LUMINANCE);

    ///offset the output buffer at the starting point
    output += ((yRange.first - args.texRect.y1) / args.closestPowerOf2) * args.texRect.w;
    
    ///iterating over the scan-lines of the input image
    int dstY = 0;
    for (int y = yRange.first; y < yRange.second; y += args.closestPowerOf2) {
        
        int start = (int)(rand() % std::max(((args.texRect.x2 - args.texRect.x1)/args.closestPowerOf2),1));
        const PIX* src_pixels = (const PIX*)args.inputImage->pixelAt(args.texRect.x1, y);
        
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
                } else {
                    
                    double r,g,b;
                    int a;
                    switch (comps) {
                        case Natron::ImageComponentRGBA:
                            r = (src_pixels ? src_pixels[srcIndex * nComps + rOffset] : 0.);
                            g = (src_pixels ? src_pixels[srcIndex * nComps + gOffset] : 0.);
                            b = (src_pixels ? src_pixels[srcIndex * nComps + bOffset] : 0.) ;
                            a = (src_pixels ? Color::floatToInt<256>(src_pixels[srcIndex * nComps + 3]) : 0);
                            break;
                        case Natron::ImageComponentRGB:
                            r = (src_pixels ? src_pixels[srcIndex * nComps + rOffset] : 0.);
                            g = (src_pixels ? src_pixels[srcIndex * nComps + gOffset] : 0.);
                            b = (src_pixels ? src_pixels[srcIndex * nComps + bOffset] : 0.);
                            a = (src_pixels ? 255 : 0);
                            break;
                        case Natron::ImageComponentAlpha:
                            r = src_pixels ? src_pixels[srcIndex] : 0.;
                            g = b = r;
                            a = src_pixels ? 255 : 0;
                            break;
                        default:
                            assert(false);
                            break;
                    }
                    switch (args.inputImage->getBitDepth()) {
                        case Natron::IMAGE_BYTE:
                            if (args.srcColorSpace) {
                                r = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast((unsigned char)r);
                                g = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast((unsigned char)g);
                                b = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast((unsigned char)b);
                            } else {
                                r = (double)convertPixelDepth<unsigned char, float>((unsigned char)r);
                                g = (double)convertPixelDepth<unsigned char, float>((unsigned char)g);
                                b = (double)convertPixelDepth<unsigned char, float>((unsigned char)b);
                            }
                            break;
                        case Natron::IMAGE_SHORT:
                            if (args.srcColorSpace) {
                                r = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast((unsigned short)r);
                                g = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast((unsigned short)g);
                                b = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast((unsigned short)b);
                            } else {
                                r = (double)convertPixelDepth<unsigned short, float>((unsigned char)r);
                                g = (double)convertPixelDepth<unsigned short, float>((unsigned char)g);
                                b = (double)convertPixelDepth<unsigned short, float>((unsigned char)b);
                            }
                            break;
                        case Natron::IMAGE_FLOAT:
                            if (args.srcColorSpace) {
                                r = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(r);
                                g = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(g);
                                b = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(b);
                            }
                            break;
                        case Natron::IMAGE_NONE:
                            break;
                    }

                    r =  r * args.gain + args.offset;
                    g =  g * args.gain + args.offset;
                    b =  b * args.gain + args.offset;
                    
                    if (luminance) {
                        r = 0.299 * r + 0.587 * g + 0.114 * b;
                        g = r;
                        b = r;
                    }
                    
                    if (!args.colorSpace) {
                        dst_pixels[dstIndex] = toBGRA(Color::floatToInt<256>(r),
                                                      Color::floatToInt<256>(g),
                                                      Color::floatToInt<256>(b),
                                                      a);
                    } else {
                        error_r = (error_r&0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(r);
                        error_g = (error_g&0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(g);
                        error_b = (error_b&0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(b);
                        assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
                        dst_pixels[dstIndex] = toBGRA((U8)(error_r >> 8),
                                                      (U8)(error_g >> 8),
                                                      (U8)(error_b >> 8),
                                                      a);
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
scaleToTexture8bits(std::pair<int,int> yRange,
                    const RenderViewerArgs& args,
                    U32* output)
{
    assert(output);

    int rOffset, gOffset, bOffset;
    int nComps = (int)args.inputImage->getComponentsCount();
    switch (args.channels) {
        case ViewerInstance::RGB:
        case ViewerInstance::LUMINANCE:
            rOffset = 0;
            gOffset = nComps < 2 ? 0 : 1;
            bOffset = nComps < 3 ? 0 : 2;
            break;
        case ViewerInstance::R:
            rOffset = 0;
            gOffset = 0;
            bOffset = 0;
            break;
        case ViewerInstance::G:
            rOffset = nComps < 2 ? 0 : 1;
            gOffset = nComps < 2 ? 0 : 1;
            bOffset = nComps < 2 ? 0 : 1;
            break;
        case ViewerInstance::B:
            rOffset = nComps < 3 ? 0 : 2;
            gOffset = nComps < 3 ? 0 : 2;
            bOffset = nComps < 3 ? 0 : 2;
            break;
        case ViewerInstance::A:
            rOffset = nComps < 4 ? 0 : 3;
            gOffset = nComps < 4 ? 0 : 3;
            bOffset = nComps < 4 ? 0 : 3;
            break;
        default:
            rOffset = 0;
            gOffset = 0;
            bOffset = 0;
            break;
    }
    switch (args.inputImage->getBitDepth()) {
        case Natron::IMAGE_FLOAT:
            scaleToTexture8bits_internal<float, 1>(yRange, args, output, rOffset, gOffset, bOffset, nComps);
            break;
        case Natron::IMAGE_BYTE:
            scaleToTexture8bits_internal<unsigned char, 255>(yRange, args, output, rOffset, gOffset, bOffset, nComps);
            break;
        case Natron::IMAGE_SHORT:
            scaleToTexture8bits_internal<unsigned short, 65535>(yRange, args, output, rOffset, gOffset, bOffset, nComps);
            break;
            
        case Natron::IMAGE_NONE:
            break;
    }
}

template <typename PIX,int maxValue>
void scaleToTexture32bitsInternal(const std::pair<int,int>& yRange,
                                  const RenderViewerArgs& args,
                                  float *output,
                                  int rOffset,int gOffset,int bOffset,int nComps)
{
    
    Natron::ImageComponents comps = args.inputImage->getComponents();
    const bool luminance = (args.channels == ViewerInstance::LUMINANCE);

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
            
            double r,g,b,a;
            
            switch (comps) {
                case Natron::ImageComponentRGBA:
                    r = (double)(src_pixels[rOffset]);
                    g = (double)src_pixels[gOffset] ;
                    b = (double)src_pixels[bOffset] ;
                    a = (nComps < 4) ? 1. : src_pixels[3];
                    break;
                case Natron::ImageComponentRGB:
                    r = (double)(src_pixels[rOffset]) ;
                    g = (double)src_pixels[gOffset] ;
                    b = (double)src_pixels[bOffset] ;
                    a = 1.;
                    break;
                case Natron::ImageComponentAlpha:
                    a = (nComps < 4) ? 1. : *src_pixels;
                    r = g = b = a;
                    a = 1.;
                    break;
                default:
                    assert(false);
                    break;
            }
            
            switch (args.inputImage->getBitDepth()) {
                case Natron::IMAGE_BYTE:
                    if (args.srcColorSpace) {
                        r = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast((unsigned char)r);
                        g = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast((unsigned char)g);
                        b = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast((unsigned char)b);
                    } else {
                        r = (double)convertPixelDepth<unsigned char, float>((unsigned char)r);
                        g = (double)convertPixelDepth<unsigned char, float>((unsigned char)g);
                        b = (double)convertPixelDepth<unsigned char, float>((unsigned char)b);
                    }
                    break;
                case Natron::IMAGE_SHORT:
                    if (args.srcColorSpace) {
                        r = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast((unsigned short)r);
                        g = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast((unsigned short)g);
                        b = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast((unsigned short)b);
                    } else {
                        r = (double)convertPixelDepth<unsigned short, float>((unsigned char)r);
                        g = (double)convertPixelDepth<unsigned short, float>((unsigned char)g);
                        b = (double)convertPixelDepth<unsigned short, float>((unsigned char)b);
                    }
                    break;
                case Natron::IMAGE_FLOAT:
                    if (args.srcColorSpace) {
                        r = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(r);
                        g = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(g);
                        b = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(b);
                    }
                    break;
                case Natron::IMAGE_NONE:
                    break;
            }


            
            if (luminance) {
                r = 0.299 * r + 0.587 * g + 0.114 * b;
                g = r;
                b = r;
            }
            *dst_pixels++ = r;
            *dst_pixels++ = g;
            *dst_pixels++ = b;
            *dst_pixels++ = a;
            
            src_pixels += args.closestPowerOf2 * 4;
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

    int nComps = (int)args.inputImage->getComponentsCount();
    switch (args.channels) {
        case ViewerInstance::RGB:
        case ViewerInstance::LUMINANCE:
            rOffset = 0;
            gOffset = nComps < 2 ? 0 : 1;
            bOffset = nComps < 3 ? 0 : 2;
            break;
        case ViewerInstance::R:
            rOffset = 0;
            gOffset = 0;
            bOffset = 0;
            break;
        case ViewerInstance::G:
            rOffset = nComps < 2 ? 0 : 1;
            gOffset = nComps < 2 ? 0 : 1;
            bOffset = nComps < 2 ? 0 : 1;
            break;
        case ViewerInstance::B:
            rOffset = nComps < 3 ? 0 : 2;
            gOffset = nComps < 3 ? 0 : 2;
            bOffset = nComps < 3 ? 0 : 2;
            break;
        case ViewerInstance::A:
            rOffset = nComps < 4 ? 0 : 3;
            gOffset = nComps < 4 ? 0 : 3;
            bOffset = nComps < 4 ? 0 : 3;
            break;
        default:
            rOffset = 0;
            gOffset = 0;
            bOffset = 0;
            break;
    }
    switch (args.inputImage->getBitDepth()) {
        case Natron::IMAGE_FLOAT:
            scaleToTexture32bitsInternal<float, 1>(yRange, args, output, rOffset, gOffset, bOffset,nComps);
            break;
        case Natron::IMAGE_BYTE:
            scaleToTexture32bitsInternal<unsigned char, 255>(yRange, args, output, rOffset, gOffset, bOffset,nComps);
            break;
        case Natron::IMAGE_SHORT:
            scaleToTexture32bitsInternal<unsigned short, 65535>(yRange, args, output, rOffset, gOffset, bOffset,nComps);
            break;
        case Natron::IMAGE_NONE:
            break;
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
ViewerInstance::ViewerInstancePrivate::updateViewerVideoEngine(const boost::shared_ptr<UpdateViewerParams> &params)
{
    // always running in the VideoEngine thread
    assertVideoEngine();

    emit doUpdateViewer(params);
}

void
ViewerInstance::ViewerInstancePrivate::updateViewer(boost::shared_ptr<UpdateViewerParams> params)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    QMutexLocker locker(&updateViewerMutex);
    if (updateViewerRunning) { // updateViewerRunning may have been reset, e.g. by wakeUpAnySleepingThread()
        uiContext->makeOpenGLcontextCurrent();
        if (!instance->aborted()) {
            // how do you make sure params->ramBuffer is not freed during this operation?
            /// It is not freed as long as the cachedFrame shared_ptr in renderViewer has a used_count greater than 1.
            /// i.e. until renderViewer exits.
            /// Since updateViewer() is in the scope of cachedFrame, and renderViewer waits for the completion
            /// of updateViewer(), it is guaranteed not to be freed before the viewer is actually done with it.
            /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
            uiContext->transferBufferFromRAMtoGPU(params->ramBuffer,
                                                  params->bytesCount,
                                                  params->textureRect,
                                                  params->gain,
                                                  params->offset,
                                                  params->lut,
                                                  updateViewerPboIndex,
                                                  params->mipMapLevel,
                                                  params->textureIndex);
            updateViewerPboIndex = (updateViewerPboIndex+1)%2;
        }

        uiContext->updateColorPicker(params->textureIndex);
        
        updateViewerRunning = false;
    }
    updateViewerCond.wakeOne();
}


bool
ViewerInstance::isInputOptional(int n) const
{
    //activeInput() is MT-safe
    int activeInputs[2];
    getActiveInputs(activeInputs[0], activeInputs[1]);
    if (n == 0 && activeInputs[0] == -1 && activeInputs[1] == -1) {
        return false;
    }
    return (n != activeInputs[0] && n != activeInputs[1]);
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
    assert(_imp->uiContext);
    if ((_imp->uiContext->getBitDepth() == OpenGLViewerI::BYTE  || !_imp->uiContext->supportsGLSL())
       && input(activeInput()) != NULL && !getApp()->getProject()->isLoadingProject()) {
        refreshAndContinueRender(false,true);
    } else {
        _imp->uiContext->redraw();
    }
    
}

void
ViewerInstance::onMipMapLevelChanged(int level)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerMipMapLevel == (unsigned int)level) {
            return;
        }
        _imp->viewerMipMapLevel = level;
    }
    if(input(activeInput()) != NULL && !getApp()->getProject()->isLoadingProject()) {
        refreshAndContinueRender(false,true);
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
        refreshAndContinueRender(false,true);
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
ViewerInstance::onColorSpaceChanged(Natron::ViewerColorSpace colorspace)
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    QMutexLocker l(&_imp->viewerParamsMutex);
    
    _imp->viewerParamsLut = colorspace;

    assert(_imp->uiContext);
    if ((_imp->uiContext->getBitDepth() == OpenGLViewerI::BYTE  || !_imp->uiContext->supportsGLSL())
       && input(activeInput()) != NULL && !getApp()->getProject()->isLoadingProject()) {
        refreshAndContinueRender(false,true);
    } else {
        _imp->uiContext->redraw();
    }
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
        refreshAndContinueRender(false,true);
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

template <typename PIX,int maxValue>
static
bool getColorAtInternal(Natron::Image* image,
                        int x, int y, // in pixel coordinates
                        bool forceLinear,
                        const Natron::Color::Lut* srcColorSpace,
                        const Natron::Color::Lut* dstColorSpace,
                        float* r, float* g, float* b, float* a)
{
    const PIX* pix = (const PIX*)image->pixelAt(x, y);
    if (!pix) {
        return false;
    }

    Natron::ImageComponents comps = image->getComponents();
    switch (comps) {
        case Natron::ImageComponentRGBA:
            *r = pix[0] / (float)maxValue;
            *g = pix[1] / (float)maxValue;
            *b = pix[2] / (float)maxValue;
            *a = pix[3] / (float)maxValue;
            break;
        case Natron::ImageComponentRGB:
            *r = pix[0] / (float)maxValue;
            *g = pix[1] / (float)maxValue;
            *b = pix[2] / (float)maxValue;
            *a = 1.;
            break;
        case Natron::ImageComponentAlpha:
            *r = 0.;
            *g = 0.;
            *b = 0.;
            *a = pix[3] / (float)maxValue;
            break;
        default:
            assert(false);
            break;
    }

    
    ///convert to linear
    if (srcColorSpace) {
        *r = srcColorSpace->fromColorSpaceFloatToLinearFloat(*r);
        *g = srcColorSpace->fromColorSpaceFloatToLinearFloat(*g);
        *b = srcColorSpace->fromColorSpaceFloatToLinearFloat(*b);
    }
    
    if (!forceLinear && dstColorSpace) {
        ///convert to dst color space
        float from[3];
        from[0] = *r;
        from[1] = *g;
        from[2] = *b;
        float to[3];
        dstColorSpace->to_float_planar(to, from, 3);
        *r = to[0];
        *g = to[1];
        *b = to[2];
    }
    return true;
}

bool
ViewerInstance::getColorAt(double x, double y, // x and y in canonical coordinates
                           bool forceLinear, int textureIndex,
                           float* r, float* g, float* b, float* a) // output values
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(r && g && b && a);
    assert(textureIndex == 0 || textureIndex == 1);

    boost::shared_ptr<Image> img;
    {
        QMutexLocker l(&_imp->lastRenderedImageMutex);
        img = _imp->lastRenderedImage[textureIndex];
    }

    ViewerColorSpace lut;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        lut = _imp->viewerParamsLut;
    }

    unsigned int mipMapLevel = (unsigned int)getMipMapLevelCombinedToZoomFactor();
    if (!img || img->getMipMapLevel() != mipMapLevel) {
        double colorGPU[4];
        _imp->uiContext->getTextureColorAt(x, y, &colorGPU[0], &colorGPU[1], &colorGPU[2], &colorGPU[3]);
        *a = colorGPU[3];
        if (forceLinear && lut != Linear) {
            const Natron::Color::Lut* srcColorSpace = lutFromColorspace(lut);

            *r = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[0]);
            *g = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[1]);
            *b = srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[2]);
            
        } else {
            *r = colorGPU[0];
            *g = colorGPU[1];
            *b = colorGPU[2];
        }
        return true;
    }

    Natron::ImageBitDepth depth = img->getBitDepth();
    ViewerColorSpace srcCS = getApp()->getDefaultColorSpaceForBitDepth(depth);
    const Natron::Color::Lut* dstColorSpace;
    const Natron::Color::Lut* srcColorSpace;
    if ((srcCS == lut) && (lut == Linear || !forceLinear)) {
        // identity transform
        srcColorSpace = 0;
        dstColorSpace = 0;
    } else {
        srcColorSpace = lutFromColorspace(srcCS);
        dstColorSpace = lutFromColorspace(lut);
    }

    ///Convert to pixel coords
    int xPixel = int(std::floor(x)) >> mipMapLevel;
    int yPixel = int(std::floor(y)) >> mipMapLevel;

    bool gotval;
    switch (depth) {
        case IMAGE_BYTE:
            gotval = getColorAtInternal<unsigned char, 255>(img.get(),
                                                            xPixel, yPixel,
                                                            forceLinear,
                                                            srcColorSpace,
                                                            dstColorSpace,
                                                            r, g, b, a);
            break;
        case IMAGE_SHORT:
            gotval = getColorAtInternal<unsigned short, 65535>(img.get(),
                                                               xPixel, yPixel,
                                                               forceLinear,
                                                               srcColorSpace,
                                                               dstColorSpace,
                                                               r, g, b, a);
            break;
        case IMAGE_FLOAT:
            gotval = getColorAtInternal<float, 1>(img.get(),
                                                  xPixel, yPixel,
                                                  forceLinear,
                                                  srcColorSpace,
                                                  dstColorSpace,
                                                  r, g, b, a);
            break;
        case IMAGE_NONE:
            gotval = false;
            break;
    }

    return gotval;
}

bool
ViewerInstance::getColorAtRect(const RectD &rect, // rectangle in canonical coordinates
                               bool forceLinear, int textureIndex,
                               float* r, float* g, float* b, float* a) // output values
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(r && g && b && a);
    assert(textureIndex == 0 || textureIndex == 1);

    boost::shared_ptr<Image> img;
    {
        QMutexLocker l(&_imp->lastRenderedImageMutex);
        img = _imp->lastRenderedImage[textureIndex];
    }

    ViewerColorSpace lut;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        lut = _imp->viewerParamsLut;
    }

    unsigned int mipMapLevel = (unsigned int)getMipMapLevelCombinedToZoomFactor();
    ///Convert to pixel coords
    RectI rectPixel;
    rectPixel.set_left(  int(std::floor(rect.left())  ) >> mipMapLevel);
    rectPixel.set_right( int(std::floor(rect.right()) ) >> mipMapLevel);
    rectPixel.set_bottom(int(std::floor(rect.bottom())) >> mipMapLevel);
    rectPixel.set_top(   int(std::floor(rect.top())   ) >> mipMapLevel);
    assert(rect.bottom() <= rect.top() && rect.left() <= rect.right());
    assert(rectPixel.bottom() <= rectPixel.top() && rectPixel.left() <= rectPixel.right());
    double rSum = 0.;
    double gSum = 0.;
    double bSum = 0.;
    double aSum = 0.;
    if (!img || img->getMipMapLevel() != mipMapLevel) {
        double colorGPU[4];
        for (int yPixel = rectPixel.bottom(); yPixel < rectPixel.top(); ++yPixel) {
            for (int xPixel = rectPixel.left(); xPixel < rectPixel.right(); ++xPixel) {
                _imp->uiContext->getTextureColorAt(xPixel << mipMapLevel, yPixel << mipMapLevel,
                                                   &colorGPU[0], &colorGPU[1], &colorGPU[2], &colorGPU[3]);
                aSum += colorGPU[3];
                if (forceLinear && lut != Linear) {
                    const Natron::Color::Lut* srcColorSpace = lutFromColorspace(lut);

                    rSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[0]);
                    gSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[1]);
                    bSum += srcColorSpace->fromColorSpaceFloatToLinearFloat(colorGPU[2]);

                } else {
                    rSum += colorGPU[0];
                    gSum += colorGPU[1];
                    bSum += colorGPU[2];
                }
            }
        }
        *r = rSum / rectPixel.area();
        *g = gSum / rectPixel.area();
        *b = bSum / rectPixel.area();
        *a = aSum / rectPixel.area();
        return true;
    }


    Natron::ImageBitDepth depth = img->getBitDepth();
    ViewerColorSpace srcCS = getApp()->getDefaultColorSpaceForBitDepth(depth);
    const Natron::Color::Lut* dstColorSpace;
    const Natron::Color::Lut* srcColorSpace;
    if ((srcCS == lut) && (lut == Linear || !forceLinear)) {
        // identity transform
        srcColorSpace = 0;
        dstColorSpace = 0;
    } else {
        srcColorSpace = lutFromColorspace(srcCS);
        dstColorSpace = lutFromColorspace(lut);
    }

    unsigned long area = 0;
    for (int yPixel = rectPixel.bottom(); yPixel < rectPixel.top(); ++yPixel) {
        for (int xPixel = rectPixel.left(); xPixel < rectPixel.right(); ++xPixel) {
            float rPix, gPix, bPix, aPix;
            bool gotval = false;
            switch (depth) {
                case IMAGE_BYTE:
                    gotval = getColorAtInternal<unsigned char, 255>(img.get(),
                                                                    xPixel, yPixel,
                                                                    forceLinear,
                                                                    srcColorSpace,
                                                                    dstColorSpace,
                                                                    &rPix, &gPix, &bPix, &aPix);
                    break;
                case IMAGE_SHORT:
                    gotval = getColorAtInternal<unsigned short, 65535>(img.get(),
                                                                       xPixel, yPixel,
                                                                       forceLinear,
                                                                       srcColorSpace,
                                                                       dstColorSpace,
                                                                       &rPix, &gPix, &bPix, &aPix);
                    break;
                case IMAGE_FLOAT:
                    gotval = getColorAtInternal<float, 1>(img.get(),
                                                          xPixel, yPixel,
                                                          forceLinear,
                                                          srcColorSpace,
                                                          dstColorSpace,
                                                           &rPix, &gPix, &bPix, &aPix);
                    break;
                case IMAGE_NONE:
                    break;
            }
            if (gotval) {
                rSum += rPix;
                gSum += gPix;
                bSum += bPix;
                aSum += aPix;
                ++area;
            }
        }
    }

    if (area > 0) {
        *r = rSum / area;
        *g = gSum / area;
        *b = bSum / area;
        *a = aSum / area;
        return true;
    }
    
    return false;
}

bool
ViewerInstance::supportsGLSL() const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    ///This is a short-cut, this is primarily used when the user switch the
    /// texture mode in the preferences menu. If the hardware doesn't support GLSL
    /// it returns false, true otherwise. @see Settings::onKnobValueChanged
    assert(_imp->uiContext);
    return _imp->uiContext->supportsGLSL();
}

void
ViewerInstance::redrawViewer()
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());
    assert(_imp->uiContext);
    _imp->uiContext->redraw();
}

boost::shared_ptr<Natron::Image>
ViewerInstance::getLastRenderedImage(int textureIndex) const
{
    // always running in the main thread
    assert(qApp && qApp->thread() == QThread::currentThread());

    if (!getNode()->isActivated()) {
        return boost::shared_ptr<Natron::Image>();
    }
    QMutexLocker l(&_imp->lastRenderedImageMutex);
    return _imp->lastRenderedImage[textureIndex];
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

int
ViewerInstance::getMipMapLevel() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread
    
    QMutexLocker l(&_imp->viewerParamsMutex);
    return _imp->viewerMipMapLevel;

}

int
ViewerInstance::getMipMapLevelCombinedToZoomFactor() const
{
    int mmLvl = getMipMapLevel();
    assert(_imp->uiContext);
    double factor = _imp->uiContext->getZoomFactor();
    if (factor > 1) {
        factor = 1;
    }
    mmLvl = std::max((double)mmLvl,-std::ceil(std::log(factor)/M_LN2));
    return mmLvl;
}

ViewerInstance::DisplayChannels
ViewerInstance::getChannels() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);
    return _imp->viewerParamsChannels;
}

void ViewerInstance::addAcceptedComponents(int /*inputNb*/,std::list<Natron::ImageComponents>* comps)
{
    ///Viewer only supports RGBA for now.
    comps->push_back(Natron::ImageComponentRGBA);
}

int ViewerInstance::getCurrentView() const
{
    assert(_imp->uiContext);
    return _imp->uiContext->getCurrentView();
}

void ViewerInstance::onInputChanged(int inputNb)
{
    assert(QThread::currentThread() == qApp->thread());
    EffectInstance* inp = input(inputNb);
    {
        QMutexLocker l(&_imp->activeInputsMutex);
        if (!inp) {
            ///check if the input was one of the active ones if so set to -1
            if (_imp->activeInputs[0] == inputNb) {
                _imp->activeInputs[0] = -1;
            } else if (_imp->activeInputs[1] == inputNb) {
                _imp->activeInputs[1] = -1;
            }
        } else {
            if (_imp->activeInputs[0] == -1) {
                _imp->activeInputs[0] = inputNb;
            } else {
                _imp->activeInputs[1] = inputNb;
            }
        }
    }
    emit activeInputsChanged();
}

void ViewerInstance::addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const
{
    depths->push_back(IMAGE_FLOAT);
    depths->push_back(IMAGE_SHORT);
    depths->push_back(IMAGE_BYTE);
}

void ViewerInstance::getActiveInputs(int& a,int &b) const
{
    QMutexLocker l(&_imp->activeInputsMutex);
    a = _imp->activeInputs[0];
    b = _imp->activeInputs[1];
}

void ViewerInstance::setInputA(int inputNb)
{
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->activeInputsMutex);
    _imp->activeInputs[0] = inputNb;
}

void ViewerInstance::setInputB(int inputNb)
{
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker l(&_imp->activeInputsMutex);
    _imp->activeInputs[1] = inputNb;
}

bool ViewerInstance::isFrameRangeLocked() const
{
    return _imp->uiContext ? _imp->uiContext->isFrameRangeLocked() : true;
}

void ViewerInstance::clearLastRenderedTexture()
{
    {
        QMutexLocker l(&_imp->lastRenderedTextureMutex);
        _imp->lastRenderedTexture.reset();
    }
    {
        QMutexLocker l(&_imp->lastRenderedImageMutex);
        _imp->lastRenderedImage[0].reset();
        _imp->lastRenderedImage[1].reset();
    }
}
