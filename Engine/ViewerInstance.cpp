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

#include "Readers/Reader.h"



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
,_colorSpace(Color::getLut(Color::LUT_DEFAULT_VIEWER))
,_lut(1)
{
    connectSlotsToViewerCache();
    connect(this,SIGNAL(doUpdateViewer()),this,SLOT(updateViewer()));
    
}

ViewerInstance::~ViewerInstance(){
    if(_uiContext && _uiContext->getGui())
        _uiContext->getGui()->removeViewerTab(_uiContext,true);
    if(_mustFreeBuffer)
        free(_buffer);
}

void ViewerInstance::connectSlotsToViewerCache(){
    Natron::CacheSignalEmitter* emitter = appPTR->getViewerCache().activateSignalEmitter();
    QObject::connect(emitter, SIGNAL(addedEntry()), this, SLOT(onCachedFrameAdded()));
    QObject::connect(emitter, SIGNAL(removedEntry()), this, SLOT(onCachedFrameAdded()));
    QObject::connect(emitter, SIGNAL(clearedInMemoryPortion()), this, SLOT(onViewerCacheCleared()));
}

void ViewerInstance::disconnectSlotsToViewerCache(){
    Natron::CacheSignalEmitter* emitter = appPTR->getViewerCache().activateSignalEmitter();
    QObject::disconnect(emitter, SIGNAL(addedEntry()), this, SLOT(onCachedFrameAdded()));
    QObject::disconnect(emitter, SIGNAL(removedEntry()), this, SLOT(onCachedFrameAdded()));
    QObject::disconnect(emitter, SIGNAL(clearedInMemoryPortion()), this, SLOT(onViewerCacheCleared()));
}
void ViewerInstance::initializeViewerTab(TabWidget* where){
    if(isLiveInstance()){
        _uiContext = getNode()->getApp()->addNewViewerTab(this,where);
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


Natron::Status ViewerInstance::renderViewer(SequenceTime time,bool fitToViewer){
    
#ifdef NATRON_LOG
    Natron::Log::beginFunction(getName(),"renderViewer");
    Natron::Log::print(QString("Time "+QString::number(time)).toStdString());
#endif
    
    ViewerGL *viewer = _uiContext->viewer;
    assert(viewer);
    double zoomFactor = viewer->getZoomFactor();
    
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
        viewer->fitToFormat(rod);
        zoomFactor = viewer->getZoomFactor();
    }
    viewer->setRod(rod);
    Format dispW = getApp()->getProjectFormat();
    
    viewer->setDisplayingImage(true);
    
    if(!viewer->isClippingToDisplayWindow()){
        dispW.set(rod);
    }
    
    /*computing the RoI*/
    std::vector<int> rows;
    std::vector<int> columns;
    int bottom = std::max(rod.bottom(),dispW.bottom());
    int top = std::min(rod.top(),dispW.top());
    int left = std::max(rod.left(),dispW.left());
    int right = std::min(rod.right(), dispW.right());
    std::pair<int,int> rowSpan = viewer->computeRowSpan(bottom,top, &rows);
    std::pair<int,int> columnSpan = viewer->computeColumnSpan(left,right, &columns);
    
    TextureRect textureRect(columnSpan.first,rowSpan.first,columnSpan.second,rowSpan.second,columns.size(),rows.size());
#ifdef NATRON_LOG
    Natron::Log::print(QString("Image rect is..."
                               " xmin= "+QString::number(left)+
                               " ymin= "+QString::number(bottom)+
                               " xmax= "+QString::number(right)+
                               " ymax= "+QString::number(top)+
                               " Viewer RoI is...."+
                               " xmin= "+QString::number(textureRect.x)+
                               " ymin= "+QString::number(textureRect.y)+
                               " xmax= "+QString::number(textureRect.r+1)+
                               " ymax= "+QString::number(textureRect.t+1)).toStdString());
#endif
    if(textureRect.w == 0 || textureRect.h == 0){
#ifdef NATRON_LOG
        Natron::Log::print(QString("getRegionOfDefinition returned StatFailed.").toStdString());
        Natron::Log::endFunction(getName(),"renderViewer");
#endif
        return StatFailed;
    }
    _interThreadInfos._textureRect = textureRect;
    _interThreadInfos._bytesCount = _interThreadInfos._textureRect.w * _interThreadInfos._textureRect.h * 4;
    if(viewer->byteMode() == 0 && viewer->hasHardware()){
        _interThreadInfos._bytesCount *= sizeof(float);
    }

    
    int viewsCount = getApp()->getCurrentProjectViewsCount();
    int view = viewsCount > 0 ? _uiContext->getCurrentView() : 0;
    
    FrameKey key(time,
                 hash().value(),
                 zoomFactor,
                 _exposure,
                 _lut,
                 viewer->byteMode(),
                 view,
                 rod,
                 dispW,
                 textureRect);
    
    boost::shared_ptr<const FrameEntry> cachedFrame;
    /*if we want to force a refresh, we by-pass the cache*/
    bool byPassCache = false;
    {
        QMutexLocker forceRenderLocker(&_forceRenderMutex);
        if(!_forceRender){
            cachedFrame = appPTR->getViewerCache().get(key);
        }else{
            byPassCache = true;
            _forceRender = false;
        }
    }
    
    if (cachedFrame) {
        /*Found in viewer cache, we execute the cached engine and leave*/
        _interThreadInfos._ramBuffer = cachedFrame->data();
        if(getNode()->getApp()->shouldAutoSetProjectFormat()){
            getNode()->getApp()->setProjectFormat(dispW);
            getNode()->getApp()->setAutoSetProjectFormat(false);
        }
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
        
        cachedFrame = appPTR->getViewerCache().newEntry(key, _interThreadInfos._bytesCount, 1);
        if(!cachedFrame){
            std::cout << "ViewerCache failed to allocate a new entry...allocating it in RAM instead!" << std::endl;
            _buffer = (unsigned char*)malloc( _interThreadInfos._bytesCount);
            _mustFreeBuffer = true;
        }else{
            _buffer = cachedFrame->data();
        }
        
        _interThreadInfos._ramBuffer = _buffer;
        
        {
            RectI roi(textureRect.x,textureRect.y,textureRect.r+1,textureRect.t+1);
            /*for now we skip the render scale*/
            RenderScale scale;
            scale.x = scale.y = 1.;
            EffectInstance::RoIMap inputsRoi = getRegionOfInterest(time, scale, roi);
            //inputsRoi only contains 1 element
            EffectInstance::RoIMap::const_iterator it = inputsRoi.begin();
            
            boost::shared_ptr<const Natron::Image> inputImage;
            try{
                inputImage = it->first->renderRoI(time, scale,view,it->second,byPassCache);
            }catch(...){
                //plugin should have posted a message
                return StatFailed;
            }
            
            int rowsPerThread = std::ceil((double)rows.size()/(double)QThread::idealThreadCount());
            // group of group of rows where first is image coordinate, second is texture coordinate
            std::vector< std::vector<std::pair<int,int> > > splitRows;
            U32 k = 0;
            while (k < rows.size()) {
                std::vector<std::pair<int,int> > rowsForThread;
                bool shouldBreak = false;
                for (int i = 0; i < rowsPerThread; ++i) {
                    if(k >= rows.size()){
                        shouldBreak = true;
                        break;
                    }
                    rowsForThread.push_back(make_pair(rows[k],k));
                    ++k;
                }
                
                splitRows.push_back(rowsForThread);
                if(shouldBreak){
                    break;
                }
            }
            {
                QMutexLocker locker(&_renderArgsMutex);
                QFuture<void> future = QtConcurrent::map(splitRows,
                                                         boost::bind(&ViewerInstance::renderFunctor,this,inputImage,_1,columns));
                future.waitForFinished();
            }
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

void ViewerInstance::renderFunctor(boost::shared_ptr<const Natron::Image> inputImage,
                                   const std::vector<std::pair<int,int> >& rows,
                                   const std::vector<int>& columns){
    if(aborted()){
        return;
    }
    for(U32 i = 0; i < rows.size();++i){
        const float* data = inputImage->pixelAt(0, rows[i].first);
        if(_uiContext->viewer->byteMode() == 0 && _uiContext->viewer->hasHardware()){
            convertRowToFitTextureBGRA_fp(data,columns,rows[i].second);
        }
        else{
            convertRowToFitTextureBGRA(data,columns,rows[i].second);
        }
    }
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
    
    // updating viewer & pixel aspect ratio if needed
    int width = viewer->width();
    int height = viewer->height();
    double ap = viewer->getDisplayWindow().getPixelAspect();
    if(ap > 1.f){
        glViewport (0, 0, (int)(width*ap), height);
    }else{
        glViewport (0, 0, width, (int)(height/ap));
    }
    viewer->updateColorPicker();
    viewer->updateGL();
    
    _usingOpenGL = false;
    _usingOpenGLCond.wakeOne();
}


void ViewerInstance::onCachedFrameAdded(){
    emit addedCachedFrame(getNode()->getApp()->getTimeLine()->currentFrame());
}
void ViewerInstance::onCachedFrameRemoved(){
    emit removedCachedFrame();
}
void ViewerInstance::onViewerCacheCleared(){
    emit clearedViewerCache();
}


void ViewerInstance::redrawViewer(){
    emit mustRedraw();
}

void ViewerInstance::swapBuffers(){
    emit mustSwapBuffers();
}

void ViewerInstance::pixelScale(double &x,double &y) {
    assert(_uiContext);
    assert(_uiContext->viewer);
    x = _uiContext->viewer->getDisplayWindow().getPixelAspect();
    y = 2. - x;
}

void ViewerInstance::backgroundColor(double &r,double &g,double &b) {
    assert(_uiContext);
    assert(_uiContext->viewer);
    _uiContext->viewer->backgroundColor(r, g, b);
}

void ViewerInstance::viewportSize(double &w,double &h) {
    assert(_uiContext);
    assert(_uiContext->viewer);
    const Format& f = _uiContext->viewer->getDisplayWindow();
    w = f.width();
    h = f.height();
}

void ViewerInstance::drawOverlays() const{
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->drawOverlay();
        }
    }
}

void ViewerInstance::notifyOverlaysPenDown(const QPointF& viewportPos,const QPointF& pos){
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->onOverlayPenDown(viewportPos, pos);
        }
    }
}

void ViewerInstance::notifyOverlaysPenMotion(const QPointF& viewportPos,const QPointF& pos){
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->onOverlayPenMotion(viewportPos, pos);
        }
    }
}

void ViewerInstance::notifyOverlaysPenUp(const QPointF& viewportPos,const QPointF& pos){
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->onOverlayPenUp(viewportPos, pos);
        }
    }
}

void ViewerInstance::notifyOverlaysKeyDown(QKeyEvent* e){
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->onOverlayKeyDown(e);
        }
    }
}

void ViewerInstance::notifyOverlaysKeyUp(QKeyEvent* e){
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->onOverlayKeyUp(e);
        }
    }
}

void ViewerInstance::notifyOverlaysKeyRepeat(QKeyEvent* e){
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->onOverlayKeyRepeat(e);
        }
    }
}

void ViewerInstance::notifyOverlaysFocusGained(){
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->onOverlayFocusGained();
        }
    }
}

void ViewerInstance::notifyOverlaysFocusLost(){
    const RenderTree& _dag = getVideoEngine()->getTree();
    if(_dag.getOutput()){
        for (RenderTree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(it->first);
            it->first->getLiveInstance()->onOverlayFocusLost();
        }
    }
}

bool ViewerInstance::isInputOptional(int n) const{
    if(n == activeInput())
        return false;
    else
        return true;
}

void ViewerInstance::convertRowToFitTextureBGRA(const float* data,const std::vector<int>& columnSpan,int yOffset){
    /*Converting one row (float32) to 8bit BGRA portion of texture. We apply a dithering algorithm based on error diffusion.
     This error diffusion will produce stripes in any image that has identical scanlines.
     To prevent this, a random horizontal position is chosen to start the error diffusion at,
     and it proceeds in both directions away from this point.*/
    assert(_buffer);
    assert(!_renderArgsMutex.tryLock());
    U32* output = reinterpret_cast<U32*>(_buffer);
    unsigned int row_width = columnSpan.size();
    yOffset *= row_width;
    output += yOffset;
    
    if(_colorSpace->linear()){
        int start = (int)(rand() % row_width);
        /* go fowards from starting point to end of line: */
        for(unsigned int i = start ; i < row_width; ++i) {
            int col = columnSpan[i]*4;
            double  _a = data[col+3];
            U8 a_,r_,g_,b_;
            a_ = (U8)std::min((int)(_a*256),255);
            r_ = (U8)std::min((int)((data[col])*_a*_exposure*256),255);
            g_ = (U8)std::min((int)((data[col+1])*_a*_exposure*256),255);
            b_ = (U8)std::min((int)((data[col+2])*_a*_exposure*256),255);
            output[i] = toBGRA(r_,g_,b_,a_);
        }
        /* go backwards from starting point to start of line: */
        for(int i = start-1 ; i >= 0 ; --i){
            int col = columnSpan[i]*4;
            double  _a = data[col+3];
            U8 a_,r_,g_,b_;
            a_ = (U8)std::min((int)(_a*256),255);
            r_ = (U8)std::min((int)((data[col])*_a*_exposure*256),255);
            g_ = (U8)std::min((int)((data[col+1])*_a*_exposure*256),255);
            b_ = (U8)std::min((int)((data[col+2])*_a*_exposure*256),255);
            output[i] = toBGRA(r_,g_,b_,a_);
        }
    }else{ // !linear
        /*flaging that we're using the colorspace so it doesn't try to change it in the same time
         if the user requested it*/
        int start = (int)(rand() % row_width);
        unsigned error_r = 0x80;
        unsigned error_g = 0x80;
        unsigned error_b = 0x80;
        /* go fowards from starting point to end of line: */
        _colorSpace->validate();
        for (unsigned int i = start ; i < columnSpan.size() ; ++i) {
            int col = columnSpan[i]*4;
            U8 r_,g_,b_,a_;
            double _a =  data[col+3];
            error_r = (error_r&0xff) + _colorSpace->toFloatFast(data[col]*_a*_exposure);
            error_g = (error_g&0xff) + _colorSpace->toFloatFast(data[col+1]*_a*_exposure);
            error_b = (error_b&0xff) + _colorSpace->toFloatFast(data[col+2]*_a*_exposure);
            a_ = (U8)std::min(_a*256.,255.);
            r_ = (U8)(error_r >> 8);
            g_ = (U8)(error_g >> 8);
            b_ = (U8)(error_b >> 8);
            output[i] = toBGRA(r_,g_,b_,a_);
        }
        /* go backwards from starting point to start of line: */
        error_r = 0x80;
        error_g = 0x80;
        error_b = 0x80;
        
        for (int i = start-1 ; i >= 0 ; --i) {
            int col = columnSpan[i]*4;
            U8 r_,g_,b_,a_;
            double _a =  data[col+3];
            error_r = (error_r&0xff) + _colorSpace->toFloatFast(data[col]*_a*_exposure);
            error_g = (error_g&0xff) + _colorSpace->toFloatFast(data[col+1]*_a*_exposure);
            error_b = (error_b&0xff) + _colorSpace->toFloatFast(data[col+2]*_a*_exposure);
            a_ = (U8)std::min(_a*256.,255.);
            r_ = (U8)(error_r >> 8);
            g_ = (U8)(error_g >> 8);
            b_ = (U8)(error_b >> 8);
            output[i] = toBGRA(r_,g_,b_,a_);
        }
    }
    
}

// nbbytesoutput is the size in bytes of 1 channel for the row
void ViewerInstance::convertRowToFitTextureBGRA_fp(const float* data,const std::vector<int>& columnSpan,int yOffset){
    assert(_buffer);
    float* output = reinterpret_cast<float*>(_buffer);
    // offset in the buffer : (y)*(w) where y is the zoomedY of the row and w=nbbytes/sizeof(float)*4 = nbbytes
    yOffset *= columnSpan.size()*sizeof(float);
    output += yOffset;
    int index = 0;
    for (unsigned int i = 0 ; i < columnSpan.size(); ++i) {
        int col = columnSpan[i]*4;
        output[index++] = data[col];
        output[index++] = data[col+1];
        output[index++] = data[col+2];
        output[index++] = data[col+3];
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

void ViewerInstance::onExposureChanged(double exp){
    QMutexLocker l(&_renderArgsMutex);
    _exposure = exp;
    
    if((_uiContext->viewer->byteMode() == 1  || !_uiContext->viewer->hasHardware()) && input(activeInput()) != NULL){
        refreshAndContinueRender();
    }else{
        emit mustRedraw();
    }
    
}

void ViewerInstance::onColorSpaceChanged(const QString& colorspaceName){
    QMutexLocker l(&_renderArgsMutex);
    
    if (colorspaceName == "Linear(None)") {
        if(_lut != 0){ // if it wasnt already this setting
            _colorSpace = Color::getLut(Color::LUT_DEFAULT_FLOAT);
        }
        _lut = 0;
    }else if(colorspaceName == "sRGB"){
        if(_lut != 1){ // if it wasnt already this setting
            _colorSpace = Color::getLut(Color::LUT_DEFAULT_VIEWER);
        }
        
        _lut = 1;
    }else if(colorspaceName == "Rec.709"){
        if(_lut != 2){ // if it wasnt already this setting
            _colorSpace = Color::getLut(Color::LUT_DEFAULT_MONITOR);
        }
        _lut = 2;
    }
    
    if((_uiContext->viewer->byteMode() == 1  || !_uiContext->viewer->hasHardware()) && input(activeInput()) != NULL){
        refreshAndContinueRender();
    }else{
        emit mustRedraw();
    }
}
