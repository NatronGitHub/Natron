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

#include "ViewerInstance.h"

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <QtConcurrentMap>

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

using namespace Natron;
using std::make_pair;
using boost::shared_ptr;


ViewerInstance::ViewerInstance(Node* node):
Natron::OutputEffectInstance(node)
, _uiContext(NULL)
, _pboIndex(0)
,_frameCount(1)
,_forceRenderMutex()
,_forceRender(false)
,_usingOpenGLCond()
,_usingOpenGLMutex()
,_usingOpenGL(false)
,_interThreadInfos()
,_buffer(NULL)
,_mustFreeBuffer(false)
,_renderArgsMutex()
,_exposure(1.)
,_offset(0)
,_colorSpace(Natron::Color::LutManager::sRGBLut())
,_lut(sRGB)
,_channelsMutex()
,_channels(RGB)
,_lastRenderedImage()
,_autoContrastMutex()
,_autoContrast(false)
,_vMinMaxMutex()
,_vmin(0)
,_vmax(0)
{
    connectSlotsToViewerCache();
    connect(this,SIGNAL(doUpdateViewer()),this,SLOT(updateViewer()));
    if(node) {
        connect(node,SIGNAL(nameChanged(QString)),this,SLOT(onNodeNameChanged(QString)));
    }
    _colorSpace->validate();
}

ViewerInstance::~ViewerInstance(){
    if(_uiContext) {
        _uiContext->removeGUI();
    }
    if(_mustFreeBuffer)
        free(_buffer);
}

void ViewerInstance::connectSlotsToViewerCache(){
    Natron::CacheSignalEmitter* emitter = appPTR->getOrActivateViewerCacheSignalEmitter();
    QObject::connect(emitter, SIGNAL(addedEntry()), this, SLOT(onViewerCacheFrameAdded()));
    QObject::connect(emitter, SIGNAL(removedLRUEntry()), this, SIGNAL(removedLRUCachedFrame()));
    QObject::connect(emitter, SIGNAL(clearedInMemoryPortion()), this, SIGNAL(clearedViewerCache()));
}

void ViewerInstance::disconnectSlotsToViewerCache(){
    Natron::CacheSignalEmitter* emitter = appPTR->getOrActivateViewerCacheSignalEmitter();
    QObject::disconnect(emitter, SIGNAL(addedEntry()), this, SLOT(onViewerCacheFrameAdded()));
    QObject::disconnect(emitter, SIGNAL(removedLRUEntry()), this, SIGNAL(removedLRUCachedFrame()));
    QObject::disconnect(emitter, SIGNAL(clearedInMemoryPortion()), this, SIGNAL(clearedViewerCache()));
}

void ViewerInstance::setUiContext(OpenGLViewerI* viewer) {
    _uiContext = viewer;
}

void ViewerInstance::onNodeNameChanged(const QString& name) {
    ///update the gui tab name
    if (_uiContext) {
        _uiContext->onViewerNodeNameChanged(name);
    }
}

void ViewerInstance::cloneExtras(){
    _uiContext = dynamic_cast<ViewerInstance*>(getNode()->getLiveInstance())->getUiContext();
}

int ViewerInstance::activeInput() const{
    return dynamic_cast<InspectorNode*>(getNode())->activeInput();
}

Natron::Status ViewerInstance::getRegionOfDefinition(SequenceTime time,RectI* rod){
    EffectInstance* n = input(activeInput());
    if(n){
        return n->getRegionOfDefinition(time,rod);
    }else{
        return StatFailed;
    }
}

EffectInstance::RoIMap ViewerInstance::getRegionOfInterest(SequenceTime /*time*/,RenderScale /*scale*/,const RectI& renderWindow){
    RoIMap ret;
    EffectInstance* n = input(activeInput());
    if (n) {
        ret.insert(std::make_pair(n, renderWindow));
    }
    return ret;
}

void ViewerInstance::getFrameRange(SequenceTime *first,SequenceTime *last){
    SequenceTime inpFirst = 0,inpLast = 0;
    EffectInstance* n = input(activeInput());
    if(n){
        n->getFrameRange(&inpFirst,&inpLast);
    }
    *first = inpFirst;
    *last = inpLast;
}


Natron::Status ViewerInstance::renderViewer(SequenceTime time,bool singleThreaded)
{
#ifdef NATRON_LOG
    Natron::Log::beginFunction(getName(),"renderViewer");
    Natron::Log::print(QString("Time "+QString::number(time)).toStdString());
#endif

    
    if(aborted()){
        return StatFailed;
    }
    
    double zoomFactor = _uiContext->getZoomFactor();

#pragma message WARN("Make use of the render scale here")
    RenderScale scale;
    scale.x = scale.y = 1.;
    
    int viewsCount = getApp()->getProject()->getProjectViewsCount();
    int view = viewsCount > 0 ? _uiContext->getCurrentView() : 0;

    EffectInstance* activeInputToRender = input(activeInput());
    assert(activeInputToRender);
    
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
    int inputIdentityNumber = -1;
    SequenceTime inputIdentityTime;
    if (!_forceRender) {
        isInputImgCached = Natron::getImageFromCache(inputImageKey, &cachedImgParams,&inputImage);
        if (isInputImgCached) {
            inputIdentityNumber = cachedImgParams->getInputNbIdentity();
            inputIdentityTime = cachedImgParams->getInputTimeIdentity();
        }
    }
    
    ////While the inputs are identity get the RoD of the first non identity input
    while (!_forceRender && inputIdentityNumber != -1 && isInputImgCached) {
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
        rod = cachedImgParams->getRoD();
        _lastRenderedImage = inputImage;
    } else {
        Status stat = getRegionOfDefinition(time, &rod);
        if(stat == StatFailed){
#ifdef NATRON_LOG
            Natron::Log::print(QString("getRegionOfDefinition returned StatFailed.").toStdString());
            Natron::Log::endFunction(getName(),"renderViewer");
#endif
            return stat;
        }
        
    }
    
    ifInfiniteclipRectToProjectDefault(&rod);

    emit rodChanged(rod);
    
    Format dispW = getApp()->getProject()->getProjectDefaultFormat();
        
    if(!_uiContext->isClippingImageToProjectWindow()){
        dispW.set(rod);
    }
    
    /*computing the RoI*/
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow(2,-std::ceil(std::log(zoomFactor) / std::log(2.)));

    RectI roi = _uiContext->getImageRectangleDisplayed(rod);

    RectI texRect;
    double tileSize = std::pow(2., (double)appPTR->getCurrentSettings()->getViewerTilesPowerOf2());
    texRect.x1 = std::floor(((double)roi.x1 / closestPowerOf2) / tileSize) * tileSize;
    texRect.y1 = std::floor(((double)roi.y1 / closestPowerOf2) / tileSize) * tileSize;
    texRect.x2 = std::ceil(((double)roi.x2 / closestPowerOf2) / tileSize) * tileSize;
    texRect.y2 = std::ceil(((double)roi.y2 / closestPowerOf2) / tileSize) * tileSize;
    
    if(texRect.width() == 0 || texRect.height() == 0){
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
    
    _interThreadInfos._textureRect = textureRect;
#pragma message WARN("Specify image components here")
    _interThreadInfos._bytesCount = textureRect.w * textureRect.h * 4;
    
    _interThreadInfos._bitDepth = _uiContext->getBitDepth();
    
    //half float is not supported yet so it is the same as float
    if(_interThreadInfos._bitDepth == OpenGLViewerI::FLOAT || _interThreadInfos._bitDepth == OpenGLViewerI::HALF_FLOAT){
        _interThreadInfos._bytesCount *= sizeof(float);
    }
    
    ///make a copy of the auto contrast enabled state, so render threads only refer to that copy
    _interThreadInfos._autoContrast = isAutoContrastEnabled();
    
    {
        QMutexLocker expLocker(&_renderArgsMutex);
        _interThreadInfos._exposure = _exposure;
        _interThreadInfos._offset = _offset;
    }
    
    {
        QMutexLocker channelsLocker(&_channelsMutex);
        _interThreadInfos._channels = _channels;
    }
    
    FrameKey key(time,
                 hash().value(),
                 _interThreadInfos._exposure,
                 _lut,
                 (int)_interThreadInfos._bitDepth,
                 _interThreadInfos._channels,
                 view,
                 textureRect);
    
    
    boost::shared_ptr<FrameEntry> cachedFrame;
    boost::shared_ptr<const FrameParams> cachedFrameParams;
    bool isCached = false;
    
    ///if we want to force a refresh, we by-pass the cache
    bool byPassCache = false;
    {
        QMutexLocker forceRenderLocker(&_forceRenderMutex);
        if (!_forceRender) {
            
            ///we never use the texture cache when the user RoI is enabled, otherwise we would have
            ///zillions of textures in the cache, each a few pixels different.
            if (!_uiContext->isUserRegionOfInterestEnabled() && !_interThreadInfos._autoContrast) {
                isCached = Natron::getTextureFromCache(key,&cachedFrameParams,&cachedFrame);
            }
        } else {
            byPassCache = true;
            _forceRender = false;
        }
    }
    
    if (isCached) {
        
        assert(cachedFrameParams);
        /*Found in viewer cache, we execute the cached engine and leave*/
        _interThreadInfos._ramBuffer = cachedFrame->data();
#ifdef NATRON_LOG
        Natron::Log::print(QString("The image was found in the ViewerCache with the following hash key: "+
                                   QString::number(key.getHash())).toStdString());
        Natron::Log::endFunction(getName(),"renderViewer");
#endif
    } else {
        /*We didn't find it in the viewer cache, hence we render
         the frame*/
        
        if (_mustFreeBuffer) {
            free(_buffer);
            _mustFreeBuffer = false;
        }
        
        ///If the user RoI is enabled, the odds that we find a texture containing exactly the same portion
        ///is very low, we better render again (and let the NodeCache do the work) rather than just
        ///overload the ViewerCache which may become slowe
        if (byPassCache || _uiContext->isUserRegionOfInterestEnabled() || _interThreadInfos._autoContrast) {
            assert(!cachedFrame);
            _buffer = (unsigned char*)malloc( _interThreadInfos._bytesCount);
            _mustFreeBuffer = true;
        } else {
            
            cachedFrameParams = FrameEntry::makeParams(rod, key._bitDepth, textureRect.w, textureRect.h);
            bool success = Natron::getTextureFromCacheOrCreate(key, cachedFrameParams, &cachedFrame);
            ///note that unlike  getImageFromCacheOrCreate in EffectInstance::renderRoI, we
            ///are sure that this time the image was not in the cache and we created it because this functino
            ///is not multi-threaded.
            assert(!success);
            
            assert(cachedFrame);
            _buffer = cachedFrame->data();
        }
        
        _interThreadInfos._ramBuffer = _buffer;
        
        {
      
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
                    _lastRenderedImage = activeInputToRender->renderRoI(time, scale,view,texRectClipped,byPassCache);
                }
            } catch (const std::exception& e) {
                _node->notifyInputNIsFinishedRendering(inputIndex);
                throw e;
            }
            
            _node->notifyInputNIsFinishedRendering(inputIndex);

            
            if (!_lastRenderedImage) {
                //if render was aborted, remove the frame from the cache as it contains only garbage
                appPTR->removeFromViewerCache(cachedFrame);
                return StatFailed;
            }
            
            //  Natron::debugImage(_lastRenderedImage.get(),"img.png");
            
            if(aborted()){
                //if render was aborted, remove the frame from the cache as it contains only garbage
                appPTR->removeFromViewerCache(cachedFrame);
                return StatOK;
            }
            
            if (singleThreaded) {
                if (_interThreadInfos._autoContrast) {
                    _vmin = INT_MAX;
                    _vmax = INT_MIN;
                    findAutoContrastVminVmax(_lastRenderedImage,roi);
                    
                    ///if vmax - vmin is greater than 1 the exposure will be really small and we won't see
                    ///anything in the image
                    if ((_vmax - _vmin) > 1.) {
                        _vmax = 1.;
                        _vmin = 0.;
                    }
                    _interThreadInfos._exposure = 1 / (_vmax - _vmin);
                    _interThreadInfos._offset =  - _vmin / ( _vmax - _vmin );
                    _exposure = _interThreadInfos._exposure;
                    _offset = _interThreadInfos._offset;

                    emit exposureChanged(_interThreadInfos._exposure);
                }
                
                renderFunctor(_lastRenderedImage, std::make_pair(texRectClipped.y1,texRectClipped.y2), textureRect, closestPowerOf2);
            } else {
                
                int rowsPerThread = std::ceil((double)(texRectClipped.x2 - texRectClipped.x1) / (double)QThread::idealThreadCount());
                // group of group of rows where first is image coordinate, second is texture coordinate
                std::vector< std::pair<int, int> > splitRows;
                int k = texRectClipped.y1;
                while (k < texRectClipped.y2) {
                    int top = k + rowsPerThread;
                    int realTop = top > texRectClipped.y2 ? texRectClipped.y2 : top;
                    splitRows.push_back(std::make_pair(k,realTop));
                    k += rowsPerThread;
                }
                
                ///if autocontrast is enabled, find out the vmin/vmax before rendering and mapping against new values
                if (_interThreadInfos._autoContrast) {
                    
                    rowsPerThread = std::ceil((double)(roi.width()) / (double)QThread::idealThreadCount());
                    std::vector<RectI> splitRects;
                    k = roi.y1;
                    while (k < roi.y2) {
                        int top = k + rowsPerThread;
                        int realTop = top > roi.top() ? roi.top() : top;
                        splitRects.push_back(RectI(roi.left(), k, roi.right(), realTop));
                        k += rowsPerThread;
                    }

                    _vmin = INT_MAX;
                    _vmax = INT_MIN;
                    QtConcurrent::map(splitRects,boost::bind(&ViewerInstance::findAutoContrastVminVmax,this,_lastRenderedImage,_1))
                                      .waitForFinished();
                    if ((_vmax - _vmin) > 1.) {
                        _vmax = 1.;
                        _vmin = 0.;
                    }
                    _interThreadInfos._exposure = 1 / (_vmax - _vmin);
                    _interThreadInfos._offset =  - _vmin / ( _vmax - _vmin );
                    {
                        QMutexLocker l(&_renderArgsMutex);
                        _exposure = _interThreadInfos._exposure;
                        _offset = _interThreadInfos._offset;
                    }
                    emit exposureChanged(_interThreadInfos._exposure);
                }
                
                QtConcurrent::map(splitRows,
                                  boost::bind(&ViewerInstance::renderFunctor,this,_lastRenderedImage,_1,textureRect,closestPowerOf2)).waitForFinished();
                
            }
        }
        if(aborted()){
            //if render was aborted, remove the frame from the cache as it contains only garbage
            appPTR->removeFromViewerCache(cachedFrame);
            return StatOK;
        }
        //we released the input image and force the cache to clear exceeding entries
        appPTR->clearExceedingEntriesFromNodeCache();
        
    }
    
    if(getVideoEngine()->mustQuit()){
        return StatFailed;
    }
    
    if(!aborted()) {
        
        if (singleThreaded) {
            updateViewer();
        } else {
            QMutexLocker locker(&_usingOpenGLMutex);
            _usingOpenGL = true;
            emit doUpdateViewer();
            while(_usingOpenGL) {
                _usingOpenGLCond.wait(&_usingOpenGLMutex);
            }
        }
    }
    return StatOK;
}
void ViewerInstance::renderFunctor(boost::shared_ptr<const Natron::Image> inputImage,std::pair<int,int> yRange,
                                   const TextureRect& texRect,int closestPowerOf2) {

    assert(texRect.y1 <= yRange.first && yRange.first <= yRange.second && yRange.second <= texRect.y2);
    
    if(aborted()){
        return;
    }
    
    int rOffset = 0,gOffset = 0,bOffset = 0;
    bool luminance = false;
    switch (_interThreadInfos._channels) {
        case RGB:
            rOffset = 0;
            gOffset = 1;
            bOffset = 2;
            break;
        case LUMINANCE:
            luminance = true;
            break;
        case R:
            rOffset = 0;
            gOffset = 0;
            bOffset = 0;
            break;
        case G:
            rOffset = 1;
            gOffset = 1;
            bOffset = 1;
            break;
        case B:
            rOffset = 2;
            gOffset = 2;
            bOffset = 2;
            break;
        case A:
            rOffset = 3;
            gOffset = 3;
            bOffset = 3;
            break;
    }
    if(_interThreadInfos._bitDepth == OpenGLViewerI::FLOAT || _interThreadInfos._bitDepth == OpenGLViewerI::HALF_FLOAT){
        // image is stored as linear, the OpenGL shader with do gamma/sRGB/Rec709 decompression
        scaleToTexture32bits(inputImage,yRange,texRect,closestPowerOf2,rOffset,gOffset,bOffset,luminance);
    }
    else{
        // texture is stored as sRGB/Rec709 compressed 8-bit RGBA
        scaleToTexture8bits(inputImage,yRange,texRect,closestPowerOf2,rOffset,gOffset,bOffset,luminance);
    }

}

void ViewerInstance::findAutoContrastVminVmax(boost::shared_ptr<const Natron::Image> inputImage,const RectI& rect) {
    
    double localVmin = INT_MAX,localVmax = INT_MIN;
    for (int y = rect.bottom(); y < rect.top(); ++y) {
        const float* src_pixels = (const float*)inputImage->pixelAt(rect.left(),y);
        ///we fill the scan-line with all the pixels of the input image
        for (int x = rect.left(); x < rect.right(); ++x) {
            double r = src_pixels[0];
            double g = src_pixels[1];
            double b = src_pixels[2];
            double a = src_pixels[3];
            
            double mini,maxi;
            switch (_interThreadInfos._channels) {
                case RGB:
                    mini = std::min(std::min(r,g),b);
                    maxi = std::max(std::max(r,g),b);
                    break;
                case LUMINANCE:
                    mini = r = 0.299 * r + 0.587 * g + 0.114 * b;
                    maxi = mini;
                    break;
                case R:
                    mini = r;
                    maxi = mini;
                    break;
                case G:
                    mini = g;
                    maxi = mini;
                    break;
                case B:
                    mini = b;
                    maxi = mini;
                    break;
                case A:
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
    
    QMutexLocker vminVmaxLocker(&_vMinMaxMutex);
    _vmin = std::min(_vmin, localVmin);
    _vmax = std::max(_vmax, localVmax);
    
}


#pragma message WARN("Adjust the 8bits and 32bits functions to take into account the image components.")
void ViewerInstance::scaleToTexture8bits(boost::shared_ptr<const Natron::Image> inputImage,std::pair<int,int> yRange,
                                         const TextureRect& texRect,int closestPowerOf2,int rOffset,int gOffset,int bOffset,bool luminance) {
    assert(_buffer);
    
    ///the base output buffer
    U32* output = reinterpret_cast<U32*>(_buffer);

    ///offset the output buffer at the starting point
    output += ((yRange.first - texRect.y1) / closestPowerOf2) * texRect.w;
    
    ///iterating over the scan-lines of the input image
    int dstY = 0;
    for (int y = yRange.first; y < yRange.second; y+=closestPowerOf2) {
        
        int start = (int)(rand() % ((texRect.x2 - texRect.x1)/closestPowerOf2));
        const float* src_pixels = (const float*)inputImage->pixelAt(texRect.x1, y);
        
        U32* dst_pixels = output + dstY * texRect.w;

        /* go fowards from starting point to end of line: */
        for (int backward = 0;backward < 2; ++backward) {
            
            int dstIndex = backward ? start - 1 : start;
            int srcIndex = dstIndex * closestPowerOf2; //< offset from src_pixels
            assert(backward == 1 || (srcIndex >= 0 && srcIndex < (texRect.x2 - texRect.x1)));
            
            unsigned error_r = 0x80;
            unsigned error_g = 0x80;
            unsigned error_b = 0x80;
            
            while(dstIndex < texRect.w && dstIndex >= 0) {
                
                if (srcIndex >= ( texRect.x2 - texRect.x1)) {
                    break;
                    //dst_pixels[dstIndex] = toBGRA(0,0,0,255);
                } else {
                    
                    double r = src_pixels[srcIndex * 4 + rOffset] * _interThreadInfos._exposure + _interThreadInfos._offset;
                    double g = src_pixels[srcIndex * 4 + gOffset] * _interThreadInfos._exposure + _interThreadInfos._offset;
                    double b = src_pixels[srcIndex * 4 + bOffset] * _interThreadInfos._exposure + _interThreadInfos._offset;
                    if(luminance){
                        r = 0.299 * r + 0.587 * g + 0.114 * b;
                        g = r;
                        b = r;
                    }
                    
                    if (!_colorSpace) {
                        
                        dst_pixels[dstIndex] = toBGRA(Color::floatToInt<256>(r),
                                                      Color::floatToInt<256>(g),
                                                      Color::floatToInt<256>(b),
                                                      255);
                        
                    } else {
                        error_r = (error_r&0xff) + _colorSpace->toColorSpaceUint8xxFromLinearFloatFast(r);
                        error_g = (error_g&0xff) + _colorSpace->toColorSpaceUint8xxFromLinearFloatFast(g);
                        error_b = (error_b&0xff) + _colorSpace->toColorSpaceUint8xxFromLinearFloatFast(b);
                        assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
                        dst_pixels[dstIndex] = toBGRA((U8)(error_r >> 8),
                                               (U8)(error_g >> 8),
                                               (U8)(error_b >> 8),
                                               255);
                        
                    }
                }
                if (backward) {
                    --dstIndex;
                    srcIndex -= closestPowerOf2;
                } else {
                    ++dstIndex;
                    srcIndex += closestPowerOf2;
                }
                
            }
        }
        ++dstY;
    }
}

void ViewerInstance::scaleToTexture32bits(boost::shared_ptr<const Natron::Image> inputImage,std::pair<int,int> yRange,const TextureRect& texRect,
                                          int closestPowerOf2,int rOffset,int gOffset,int bOffset,bool luminance) {
    assert(_buffer);
    
    ///the base output buffer
    float* output = reinterpret_cast<float*>(_buffer);
    
    ///the width of the output buffer multiplied by the channels count
    int dst_width = texRect.w * 4;
    
    ///offset the output buffer at the starting point
    output += ((yRange.first - texRect.y1) / closestPowerOf2) * dst_width;
    
    ///iterating over the scan-lines of the input image
    int dstY = 0;
    for (int y = yRange.first; y < yRange.second; y+=closestPowerOf2) {
        
        const float* src_pixels = (const float*)inputImage->pixelAt(texRect.x1, y);
        float* dst_pixels = output + dstY * dst_width;
        
        ///we fill the scan-line with all the pixels of the input image
        for (int x = texRect.x1; x < texRect.x2; x+=closestPowerOf2) {
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
            
            src_pixels += closestPowerOf2 * 4;
        }
        ++dstY;
    }
    
}

U32 ViewerInstance::toBGRA(U32 r,U32 g,U32 b,U32 a){
    U32 res = 0x0;
    res |= b;
    res |= (g << 8);
    res |= (r << 16);
    res |= (a << 24);
    return res;
}


void ViewerInstance::wakeUpAnySleepingThread(){
    _usingOpenGL = false;
    _usingOpenGLCond.wakeAll();
}

void ViewerInstance::updateViewer(){
    QMutexLocker locker(&_usingOpenGLMutex);
    _uiContext->makeOpenGLcontextCurrent();
    if(!aborted()){
        _uiContext->transferBufferFromRAMtoGPU(_interThreadInfos._ramBuffer,
                                           _interThreadInfos._bytesCount,
                                           _interThreadInfos._textureRect,
                                           _pboIndex);
        _pboIndex = (_pboIndex+1)%2;
    }
    
    _uiContext->updateColorPicker();
    _uiContext->redraw();
    
    _usingOpenGL = false;
    _usingOpenGLCond.wakeOne();
}


bool ViewerInstance::isInputOptional(int n) const{
    return n != activeInput();
}
void ViewerInstance::onExposureChanged(double exp){
    {
        QMutexLocker l(&_renderArgsMutex);
        _exposure = exp;
    }
    if((_uiContext->getBitDepth() == OpenGLViewerI::BYTE  || !_uiContext->supportsGLSL())
       && input(activeInput()) != NULL && !getApp()->getProject()->isLoadingProject()) {
        refreshAndContinueRender(false);
    } else {
        emit mustRedraw();
    }
    
}

void ViewerInstance::onAutoContrastChanged(bool autoContrast) {
    {
        QMutexLocker l(&_autoContrastMutex);
        _autoContrast = autoContrast;
    }
    if (input(activeInput()) != NULL && !getApp()->getProject()->isLoadingProject()){
        refreshAndContinueRender(false);
    }
}

bool ViewerInstance::isAutoContrastEnabled() const {
    QMutexLocker l(&_autoContrastMutex);
    return _autoContrast;
}

void ViewerInstance::onColorSpaceChanged(const QString& colorspaceName){
    QMutexLocker l(&_renderArgsMutex);
    
    if (colorspaceName == "Linear(None)") {
        if(_lut != Linear){ // if it wasnt already this setting
            _colorSpace = 0;
        }
        _lut = Linear;
    }else if(colorspaceName == "sRGB"){
        if(_lut != sRGB){ // if it wasnt already this setting
            _colorSpace = Natron::Color::LutManager::sRGBLut();
        }
        
        _lut = sRGB;
    }else if(colorspaceName == "Rec.709"){
        if(_lut != Rec709){ // if it wasnt already this setting
            _colorSpace = Natron::Color::LutManager::Rec709Lut();
        }
        _lut = Rec709;
    }
    
    if (_colorSpace) {
        _colorSpace->validate();
    }
    
    if((_uiContext->getBitDepth() == OpenGLViewerI::BYTE  || !_uiContext->supportsGLSL())
       && input(activeInput()) != NULL){
        refreshAndContinueRender(false);
    }else{
        emit mustRedraw();
    }
}

void ViewerInstance::onViewerCacheFrameAdded(){
    emit addedCachedFrame(getApp()->getTimeLine()->currentFrame());
}

void ViewerInstance::setDisplayChannels(DisplayChannels channels) {
    {
        QMutexLocker l(&_channelsMutex);
        _channels = channels;
    }
    if (!getApp()->getProject()->isLoadingProject()) {
        refreshAndContinueRender(false);
    }
}

void ViewerInstance::disconnectViewer()
{
    if (getVideoEngine()->isWorking()) {
        getVideoEngine()->abortRendering(false); // aborting current work
    }
    //_lastRenderedImage.reset(); // if you uncomment this, _lastRenderedImage is not set back when you reconnect the viewer immediately after disconnecting
    emit viewerDisconnected();
}

bool ViewerInstance::getColorAt(int x,int y,float* r,float* g,float* b,float* a,bool forceLinear)
{
    if (!_lastRenderedImage) {
        return false;
    }
    
    const RectI& bbox = _lastRenderedImage->getRoD();
    
    if (x < bbox.x1 || x >= bbox.x2 || y < bbox.y1 || y >= bbox.y2) {
        return false;
    }
    
    const float* pix = _lastRenderedImage->pixelAt(x, y);
    *r = *pix;
    *g = *(pix + 1);
    *b = *(pix + 2);
    *a = *(pix + 3);
    if(!forceLinear && _colorSpace){
        float from[3];
        from[0] = *r;
        from[1] = *g;
        from[2] = *b;
        float to[3];
        _colorSpace->to_float_planar(to, from, 3);
        *r = to[0];
        *g = to[1];
        *b = to[2];
    }
    return true;
}

bool ViewerInstance::supportsGLSL() const{
    return _uiContext->supportsGLSL();
}

void ViewerInstance::redrawViewer(){
    emit mustRedraw();
}

boost::shared_ptr<Natron::Image> ViewerInstance::getLastRenderedImage() const {
    if (!getNode()->isActivated()) {
        return boost::shared_ptr<Natron::Image>();
    }
    return _lastRenderedImage;
}

int ViewerInstance::getLutType() const  {
    QMutexLocker l(&_renderArgsMutex);
    return _lut;
}

double ViewerInstance::getExposure() const {
    QMutexLocker l(&_renderArgsMutex);
    return _exposure;
}

const Natron::Color::Lut* ViewerInstance::getLut() const {
    QMutexLocker l(&_renderArgsMutex);
    return _colorSpace;
}

double ViewerInstance::getOffset() const {
    QMutexLocker l(&_renderArgsMutex);
    return _offset;
}