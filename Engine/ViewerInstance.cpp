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

#include "Global/AppManager.h"

#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/TimeLineGui.h"
#include "Gui/TabWidget.h"

#include "Engine/Row.h"
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
,_colorSpace(Natron::Color::LutManager::sRGBLut())
,_lut(sRGB)
,_channels(RGBA)
{
    connectSlotsToViewerCache();
    connect(this,SIGNAL(doUpdateViewer()),this,SLOT(updateViewer()));
    if(node) {
        connect(node,SIGNAL(nameChanged(QString)),this,SLOT(onNodeNameChanged(QString)));
    }
}

ViewerInstance::~ViewerInstance(){
    if(_uiContext && _uiContext->getGui())
        _uiContext->getGui()->removeViewerTab(_uiContext,true);
    if(_mustFreeBuffer)
        free(_buffer);
}

void ViewerInstance::connectSlotsToViewerCache(){
    Natron::CacheSignalEmitter* emitter = appPTR->getViewerCache().activateSignalEmitter();
    QObject::connect(emitter, SIGNAL(addedEntry()), this, SLOT(onViewerCacheFrameAdded()));
    QObject::connect(emitter, SIGNAL(removedLRUEntry()), this, SIGNAL(removedLRUCachedFrame()));
    QObject::connect(emitter, SIGNAL(clearedInMemoryPortion()), this, SIGNAL(clearedViewerCache()));
}

void ViewerInstance::disconnectSlotsToViewerCache(){
    Natron::CacheSignalEmitter* emitter = appPTR->getViewerCache().activateSignalEmitter();
    QObject::disconnect(emitter, SIGNAL(addedEntry()), this, SLOT(onViewerCacheFrameAdded()));
    QObject::disconnect(emitter, SIGNAL(removedLRUEntry()), this, SIGNAL(removedLRUCachedFrame()));
    QObject::disconnect(emitter, SIGNAL(clearedInMemoryPortion()), this, SIGNAL(clearedViewerCache()));
}
void ViewerInstance::initializeViewerTab(TabWidget* where){
    if(isLiveInstance()){
        _uiContext = getNode()->getApp()->addNewViewerTab(this,where);
    }
}

void ViewerInstance::onNodeNameChanged(const QString& name) {
    ///update the gui tab name
    if (_uiContext) {
        _uiContext->getGui()->unregisterTab(_uiContext);
        TabWidget* parent = dynamic_cast<TabWidget*>(_uiContext->parentWidget());
        if ( parent ) {
            parent->setTabName(_uiContext, name);
        }
        _uiContext->getGui()->registerTab(_uiContext);
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


Natron::Status ViewerInstance::renderViewer(SequenceTime time,bool fitToViewer)
{
#ifdef NATRON_LOG
    Natron::Log::beginFunction(getName(),"renderViewer");
    Natron::Log::print(QString("Time "+QString::number(time)).toStdString());
#endif
    ViewerGL *viewer = _uiContext->viewer;
    assert(viewer);
    double zoomFactor = viewer->getZoomFactor();
    
    if(aborted()){
        return StatFailed;
    }
    
    RectI rod;
    Status stat = getRegionOfDefinition(time, &rod);
    if(stat == StatFailed){
#ifdef NATRON_LOG
        Natron::Log::print(QString("getRegionOfDefinition returned StatFailed.").toStdString());
        Natron::Log::endFunction(getName(),"renderViewer");
#endif
        return stat;
    }
    
    
    
    ifInfiniteclipRectToProjectDefault(&rod);
    if(fitToViewer){
        viewer->fitToFormat(Format(rod));
        zoomFactor = viewer->getZoomFactor();
    }
    viewer->setRod(rod);
    Format dispW = getApp()->getProjectFormat();
    
    viewer->setDisplayingImage(true);
    
    if(!viewer->isClippingToDisplayWindow()){
        dispW.set(rod);
    }
    
    /*computing the RoI*/
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow(2,-std::ceil(std::log(zoomFactor) / std::log(2)));

    RectI roi = viewer->getImageRectangleDisplayed(rod);

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
    
    //  std::cout << "x1: " << textureRect.x1 << " x2: " << textureRect.x2 << " y1: " << textureRect.y1 <<
    //" y2: " << textureRect.y2 << " w: " << textureRect.w << " h: " << textureRect.h << " po2: " << textureRect.closestPo2 << std::endl;
    
    _interThreadInfos._textureRect = textureRect;
    _interThreadInfos._bytesCount = textureRect.w * textureRect.h * 4;
    
    //half float is not supported yet so it is the same as float
    if(viewer->bitDepth() == FLOAT || viewer->bitDepth() == HALF_FLOAT){
        _interThreadInfos._bytesCount *= sizeof(float);
    }

    
    int viewsCount = getApp()->getProjectViewsCount();
    int view = viewsCount > 0 ? _uiContext->getCurrentView() : 0;
    
    FrameKey key(time,
                 hash().value(),
                 _exposure,
                 _lut,
                 viewer->bitDepth(),
                 _channels,
                 view,
                 rod,
                 dispW,
                 textureRect);
    
    boost::shared_ptr<FrameEntry> cachedFrame;
    bool isCached = false;
    /*if we want to force a refresh, we by-pass the cache*/
    bool byPassCache = false;
    {
        QMutexLocker forceRenderLocker(&_forceRenderMutex);
        if(!_forceRender){
            isCached = Natron::getTextureFromCache(key, &cachedFrame);
        }else{
            byPassCache = true;
            _forceRender = false;
        }
    }
    
    if (isCached) {
                
        /*Found in viewer cache, we execute the cached engine and leave*/
        _interThreadInfos._ramBuffer = cachedFrame->data();
        Format dispW;
        getApp()->setOrAddProjectFormat(dispW,true);
#ifdef NATRON_LOG
        Natron::Log::print(QString("The image was found in the ViewerCache with the following hash key: "+
                                   QString::number(key.getHash())).toStdString());
        Natron::Log::endFunction(getName(),"renderViewer");
#endif
    }else{
    
        /*We didn't find it in the viewer cache, hence we render
         the frame*/
        
        if(_mustFreeBuffer){
            free(_buffer);
            _mustFreeBuffer = false;
        }
        
        if(byPassCache){
            assert(!cachedFrame);
            _buffer = (unsigned char*)malloc( _interThreadInfos._bytesCount);
            _mustFreeBuffer = true;
        }else{
            assert(cachedFrame);
            _buffer = cachedFrame->data();
        }
        
        _interThreadInfos._ramBuffer = _buffer;
        
        {
            /*for now we skip the render scale*/
            RenderScale scale;
            scale.x = scale.y = 1.;
            EffectInstance::RoIMap inputsRoi = getRegionOfInterest(time, scale, roi);
            //inputsRoi only contains 1 element
            EffectInstance::RoIMap::const_iterator it = inputsRoi.begin();
            
            boost::shared_ptr<Natron::Image> inputImage;

            // Do not catch exceptions: if an exception occurs here it is probably fatal, since
            // it comes from Natron itself. All exceptions from plugins are already caught
            // by the HostSupport library.
            int inputIndex = activeInput();
            _node->notifyInputNIsRendering(inputIndex);
            inputImage = it->first->renderRoI(time, scale,view,it->second,byPassCache);
            _node->notifyInputNIsFinishedRendering(inputIndex);;
            
            if(aborted()){
                //if render was aborted, remove the frame from the cache as it contains only garbage
                appPTR->removeFromViewerCache(cachedFrame);
                return StatOK;
            }
            
            int rowsPerThread = std::ceil((double)texRect.height()/(double)QThread::idealThreadCount());
            // group of group of rows where first is image coordinate, second is texture coordinate
            std::vector< std::pair<int, int> > splitRows;
            int k = textureRect.y1;
            while (k < textureRect.y2) {
                int top = k + rowsPerThread;
                int realTop = top > textureRect.y2 ? textureRect.y2 : top;
                splitRows.push_back(std::make_pair(k,realTop));
                k += rowsPerThread;
            }
            {
                QMutexLocker locker(&_renderArgsMutex);
                QFuture<void> future = QtConcurrent::map(splitRows,
                                                         boost::bind(&ViewerInstance::renderFunctor,this,inputImage,_1,textureRect,closestPowerOf2));
                future.waitForFinished();
            }
        }
        if(aborted()){
            //if render was aborted, remove the frame from the cache as it contains only garbage
            appPTR->removeFromViewerCache(cachedFrame);
            return StatOK;
        }
        //we released the input image and force the cache to clear exceeding entries
        appPTR->clearExceedingEntriesFromNodeCache();
        
        viewer->stopDisplayingProgressBar();
        
    }
    
    if(getVideoEngine()->mustQuit()){
        return StatFailed;
    }

    QMutexLocker locker(&_usingOpenGLMutex);
    _usingOpenGL = true;
    emit doUpdateViewer();
    while(_usingOpenGL) {
        _usingOpenGLCond.wait(&_usingOpenGLMutex);
    }
    return StatOK;
}
void ViewerInstance::renderFunctor(boost::shared_ptr<const Natron::Image> inputImage,std::pair<int,int> yRange,
                                   const TextureRect& texRect,int closestPowerOf2) {

    if(aborted()){
        return;
    }
    
    int rOffset = 0,gOffset = 0,bOffset = 0;
    bool luminance = false;
    switch (_channels) {
        case RGBA:
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
    if(_uiContext->viewer->bitDepth() == FLOAT || _uiContext->viewer->bitDepth() == HALF_FLOAT){
        // image is stored as linear, the OpenGL shader with do gamma/sRGB/Rec709 decompression
        scaleToTexture32bits(inputImage,yRange,texRect,closestPowerOf2,rOffset,gOffset,bOffset,luminance);
    }
    else{
        // texture is stored as sRGB/Rec709 compressed 8-bit RGBA
        scaleToTexture8bits(inputImage,yRange,texRect,closestPowerOf2,rOffset,gOffset,bOffset,luminance);
    }

}

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
        
        int start = (int)(rand() % texRect.w);
        
        const float* src_pixels = (const float*)inputImage->pixelAt(texRect.x1, y);

        U32* dst_pixels = output + dstY * texRect.w;

        if (!_colorSpace) { //< linear
            /* go fowards from starting point to end of line: */
            for(int i = start; i < texRect.w; ++i) {
                int srcIndex = (i  * 4 * closestPowerOf2);
                if (srcIndex > texRect.x2 * 4) {
                    dst_pixels[i] = toBGRA(0,0,0,255);
                    continue;
                }
                
                double r = src_pixels[srcIndex + rOffset] * _exposure;
                double g = src_pixels[srcIndex + gOffset] * _exposure;
                double b = src_pixels[srcIndex + bOffset] * _exposure;
                if(luminance){
                    r = 0.299 * r + 0.587 * g + 0.114 * b;
                    g = r;
                    b = r;
                }
                dst_pixels[i] = toBGRA((U8)std::min((int)( r * 256 ),255),
                                   (U8)std::min((int)( g * 256 ),255),
                                   (U8)std::min((int)( b * 256 ),255),
                                   255);
            }
            
            /* go backwards from starting point to start of line: */
            for(int i = start-1 ; i >= 0 ; --i){
                int srcIndex = (i  * 4 * closestPowerOf2);
                if (srcIndex > texRect.x2 * 4) {
                    dst_pixels[i] = toBGRA(0,0,0,255);
                    continue;
                }
                
                double r = src_pixels[srcIndex + rOffset] * _exposure;
                double g = src_pixels[srcIndex + gOffset] * _exposure;
                double b = src_pixels[srcIndex + bOffset] * _exposure;
                if(luminance){
                    r = 0.299 * r + 0.587 * g + 0.114 * b;
                    g = r;
                    b = r;
                }
                dst_pixels[i] = toBGRA((U8)std::min((int)( r * 256 ),255),
                                       (U8)std::min((int)( g * 256 ),255),
                                       (U8)std::min((int)( b * 256 ),255),
                                       255);
            }

        } else {
            
            _colorSpace->validate();

            unsigned error_r = 0x80;
            unsigned error_g = 0x80;
            unsigned error_b = 0x80;
            
            /* go fowards from starting point to end of line: */
            for(int i = start ; i < texRect.w; ++i) {
                int srcIndex =  (i  * 4 * closestPowerOf2);
                if (srcIndex > ( texRect.x2 - texRect.x1 ) * 4) {
                    dst_pixels[i] = toBGRA(0,0,0,255);
                    continue;
                }
                
                double r = src_pixels[srcIndex + rOffset] * _exposure;
                double g = src_pixels[srcIndex + gOffset] * _exposure;
                double b = src_pixels[srcIndex + bOffset] * _exposure;
                if(luminance){
                    r = 0.299 * r + 0.587 * g + 0.114 * b;
                    g = r;
                    b = r;
                }
                
                error_r = (error_r&0xff) + _colorSpace->toColorSpaceShortFromLinearFloatFast(Natron::Color::clamp(r,0.,1.));
                error_g = (error_g&0xff) + _colorSpace->toColorSpaceShortFromLinearFloatFast(Natron::Color::clamp(g,0.,1.));
                error_b = (error_b&0xff) + _colorSpace->toColorSpaceShortFromLinearFloatFast(Natron::Color::clamp(b,0.,1.));
                
                dst_pixels[i] = toBGRA((U8)(error_r >> 8),
                                       (U8)(error_g >> 8),
                                       (U8)(error_b >> 8),
                                       255);
            }
            
            error_r = 0x80;
            error_g = 0x80;
            error_b = 0x80;
            
            /* go backwards from starting point to start of line: */
            for(int i = start-1 ; i >= 0 ; --i){
                int srcIndex =  (i  * 4 * closestPowerOf2);
                if (srcIndex > ( texRect.x2 - texRect.x1 ) * 4) {
                    dst_pixels[i] = toBGRA(0,0,0,255);
                    continue;
                }
                
                double r = src_pixels[srcIndex + rOffset] * _exposure;
                double g = src_pixels[srcIndex + gOffset] * _exposure;
                double b = src_pixels[srcIndex + bOffset] * _exposure;
                if(luminance){
                    r = 0.299 * r + 0.587 * g + 0.114 * b;
                    g = r;
                    b = r;
                }
                
                error_r = (error_r&0xff) + _colorSpace->toColorSpaceShortFromLinearFloatFast(Natron::Color::clamp(r,0.,1.));
                error_g = (error_g&0xff) + _colorSpace->toColorSpaceShortFromLinearFloatFast(Natron::Color::clamp(g,0.,1.));
                error_b = (error_b&0xff) + _colorSpace->toColorSpaceShortFromLinearFloatFast(Natron::Color::clamp(b,0.,1.));
                
                dst_pixels[i] = toBGRA((U8)(error_r >> 8),
                                       (U8)(error_g >> 8),
                                       (U8)(error_b >> 8),
                                       255);
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
    ViewerGL* viewer = _uiContext->viewer;
    viewer->makeCurrent();
    if(!aborted()){
        viewer->transferBufferFromRAMtoGPU(_interThreadInfos._ramBuffer,
                                           _interThreadInfos._bytesCount,
                                           _interThreadInfos._textureRect,
                                           _pboIndex);
        _pboIndex = (_pboIndex+1)%2;
    }
    
    viewer->updateColorPicker();
    viewer->update();
    
    _usingOpenGL = false;
    _usingOpenGLCond.wakeOne();
}


bool ViewerInstance::isInputOptional(int n) const{
    return n != activeInput();
}
void ViewerInstance::onExposureChanged(double exp){
    QMutexLocker l(&_renderArgsMutex);
    _exposure = exp;
    
    if((_uiContext->viewer->bitDepth() == BYTE  || !_uiContext->viewer->supportsGLSL())
       && input(activeInput()) != NULL) {
        refreshAndContinueRender();
    } else {
        emit mustRedraw();
    }
    
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
    
    if((_uiContext->viewer->bitDepth() == BYTE  || !_uiContext->viewer->supportsGLSL())
       && input(activeInput()) != NULL){
        refreshAndContinueRender();
    }else{
        emit mustRedraw();
    }
}

void ViewerInstance::onViewerCacheFrameAdded(){
    emit addedCachedFrame(getApp()->getTimeLine()->currentFrame());
}

void ViewerInstance::setDisplayChannels(DisplayChannels channels) {
    _channels = channels;
    refreshAndContinueRender();
}

void ViewerInstance::disconnectViewer(){
    getVideoEngine()->abortRendering(); // aborting current work
    emit viewerDisconnected();
}

void ViewerInstance::getColorAt(int x,int y,float* r,float* g,float* b,float* a,bool forceLinear){
    
    _uiContext->viewer->getColorAt(x, y, r, g, b, a);
    if(forceLinear && _colorSpace){
        float from[3];
        from[0] = *r;
        from[1] = *g;
        from[2] = *b;
        float to[3];
        _colorSpace->from_float_planar(to, from, 3);
        *r = to[0];
        *g = to[1];
        *b = to[2];
    }
}

bool ViewerInstance::supportsGLSL() const{
    return _uiContext->viewer->supportsGLSL();
}

void ViewerInstance::redrawViewer(){
    emit mustRedraw();
}
