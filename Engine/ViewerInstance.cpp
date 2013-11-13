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
#include "Engine/Timer.h"

#include "Readers/Reader.h"

#define POWITER_FPS_REFRESH_RATE 10


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
,_timerMutex()
,_timer(new Timer)
{
    connectSlotsToViewerCache();
    connect(this,SIGNAL(doUpdateViewer()),this,SLOT(updateViewer()));
    connect(this,SIGNAL(doCachedEngine()),this,SLOT(cachedEngine()));
    connect(this,SIGNAL(doFrameStorageAllocation()),this,SLOT(allocateFrameStorage()));
    connect(this,SIGNAL(doUnmapPBO()),this,SLOT(unMapPBO()));
    _timer->playState = RUNNING; /*activating the timer*/
    
}

ViewerInstance::~ViewerInstance(){
    if(_uiContext && _uiContext->getGui())
        _uiContext->getGui()->removeViewerTab(_uiContext,true);
    _timer->playState = PAUSE;
    
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
        
    int viewsCount = getApp()->getCurrentProjectViewsCount();
    int view = viewsCount > 0 ? _uiContext->getCurrentView() : 0;

    FrameKey key(time,
                 hash().value(),
                 zoomFactor,
                 viewer->getExposure(),
                 viewer->lutType(),
                 viewer->byteMode(),
                 view,
                 rod,
                 dispW,
                 textureRect);
    
    boost::shared_ptr<const FrameEntry> iscached;
    
    /*if we want to force a refresh, we by-pass the cache*/
    bool byPassCache = false;
    {
        QMutexLocker forceRenderLocker(&_forceRenderMutex);
        if(!_forceRender){
            iscached = appPTR->getViewerCache().get(key);
        }else{
            byPassCache = true;
            _forceRender = false;
        }
    }

    
    if (iscached) {
        /*Found in viewer cache, we execute the cached engine and leave*/
        _interThreadInfos._cachedEntry = iscached;
        _interThreadInfos._textureRect = iscached->getKey()._textureRect;
        {
            if(getVideoEngine()->mustQuit()){
                return StatFailed;
            }
            QMutexLocker locker(&_usingOpenGLMutex);
            _usingOpenGL = true;
            emit doCachedEngine();
            while(_usingOpenGL) {
                _usingOpenGLCond.wait(&_usingOpenGLMutex);
            }
        }
#ifdef NATRON_LOG
        Natron::Log::print(QString("The image was found in the ViewerCache with the following hash key: "+
                                                             QString::number(key.getHash())).toStdString());
        Natron::Log::endFunction(getName(),"renderViewer");
#endif
        return StatOK;
    }
    
    /*We didn't find it in the viewer cache, hence we allocate
     the frame storage*/
    
    if(getVideoEngine()->mustQuit()){
        return StatFailed;
    }
    {
        QMutexLocker locker(&_usingOpenGLMutex);
        _usingOpenGL = true;
        emit doFrameStorageAllocation();
        while(_usingOpenGL) {
            _usingOpenGLCond.wait(&_usingOpenGLMutex);
        }
    }
    

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
            QMutexLocker locker(&_usingOpenGLMutex);
            _usingOpenGL = true;
            emit doUnmapPBO();
            while(_usingOpenGL){
                _usingOpenGLCond.wait(&_usingOpenGLMutex);
            }
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
        QFuture<void> future = QtConcurrent::map(splitRows,
                                                 boost::bind(&ViewerInstance::renderFunctor,this,inputImage,_1,columns));
        future.waitForFinished();
        
    }
    //we released the input image and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();
    
    viewer->stopDisplayingProgressBar();
    
    /*we copy the frame to the cache*/
    if(!aborted()){
        assert(sizeof(Natron::Cache<Natron::FrameEntry>::data_t) == 1); // _dataSize is in bytes, so it has to be a byte cache
        size_t bytesToCopy = _interThreadInfos._pixelsCount;
        if(viewer->hasHardware() && !viewer->byteMode()){
            bytesToCopy *= sizeof(float);
        }
        boost::shared_ptr<FrameEntry> cachedFrame = appPTR->getViewerCache().newEntry(key,bytesToCopy, 1);
        if(!cachedFrame){
            setPersistentMessage(Natron::WARNING_MESSAGE, "Failed to cache the frame rendered by the viewer.");
            QMutexLocker locker(&_usingOpenGLMutex);
            _usingOpenGL = true;
            emit doUnmapPBO();
            while(_usingOpenGL){
                _usingOpenGLCond.wait(&_usingOpenGLMutex);
            }
            return StatOK;
        }
       
        memcpy((char*)cachedFrame->data(),viewer->getFrameData(),bytesToCopy);
    }else{
        QMutexLocker locker(&_usingOpenGLMutex);
        _usingOpenGL = true;
        emit doUnmapPBO();
        while(_usingOpenGL){
            _usingOpenGLCond.wait(&_usingOpenGLMutex);
        }
    }
    
    if(getVideoEngine()->mustQuit()){
        QMutexLocker locker(&_usingOpenGLMutex);
        _usingOpenGL = true;
        emit doUnmapPBO();
        while(_usingOpenGL){
            _usingOpenGLCond.wait(&_usingOpenGLMutex);
        }
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
        _uiContext->viewer->drawRow(inputImage->pixelAt(0, rows[i].first), columns, rows[i].second);
    }
}

void ViewerInstance::wakeUpAnySleepingThread(){
    _usingOpenGL = false;
    _usingOpenGLCond.wakeAll();
}
void ViewerInstance::allocateFrameStorage(){
    QMutexLocker locker(&_usingOpenGLMutex);
    _interThreadInfos._pixelsCount = _interThreadInfos._textureRect.w * _interThreadInfos._textureRect.h * 4;
    _uiContext->viewer->allocateFrameStorage(_interThreadInfos._pixelsCount);
    _usingOpenGL = false;
    _usingOpenGLCond.wakeOne();
    
}
void ViewerInstance::updateViewer(){
    QMutexLocker locker(&_usingOpenGLMutex);
    
    ViewerGL* viewer = _uiContext->viewer;
    
    if(!aborted()){
        viewer->copyPBOToRenderTexture(_interThreadInfos._textureRect); // returns instantly
    }else{
        viewer->unMapPBO();
        viewer->unBindPBO();
    }
    
    {
        QMutexLocker timerLocker(&_timerMutex);
        _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
    }
    if((_frameCount%POWITER_FPS_REFRESH_RATE)==0){
        emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
        _frameCount = 1; //reseting to 1
    }else{
        ++_frameCount;
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

void ViewerInstance::unMapPBO(){
     QMutexLocker locker(&_usingOpenGLMutex);
    _uiContext->viewer->unMapPBO();
    _usingOpenGL = false;
    _usingOpenGLCond.wakeOne();
}

void ViewerInstance::cachedEngine(){
    QMutexLocker locker(&_usingOpenGLMutex);
    
    assert(_interThreadInfos._cachedEntry);
    size_t dataSize = 0;
    int w = _interThreadInfos._textureRect.w;
    int h = _interThreadInfos._textureRect.h;
    dataSize  = w * h * 4 ;
    const RectI& dataW = _interThreadInfos._cachedEntry->getKey()._dataWindow;
    Format dispW = _interThreadInfos._cachedEntry->getKey()._displayWindow;
    _uiContext->viewer->setRod(dataW);
    if(getNode()->getApp()->shouldAutoSetProjectFormat()){
        getNode()->getApp()->setProjectFormat(dispW);
        getNode()->getApp()->setAutoSetProjectFormat(false);
    }
    /*allocating pbo*/
    void* output = _uiContext->viewer->allocateAndMapPBO(dataSize, _uiContext->viewer->getPBOId(_pboIndex));
    assert(output); 
    _pboIndex = (_pboIndex+1)%2;
    _uiContext->viewer->fillPBO((const char*)_interThreadInfos._cachedEntry->data(), output, dataSize);

    locker.unlock();
    updateViewer();
}



void ViewerInstance::setDesiredFPS(double d){
    QMutexLocker timerLocker(&_timerMutex);
    _timer->setDesiredFrameRate(d);
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


