//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ViewerNode.h"

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
#include "Engine/OfxNode.h"
#include "Engine/ImageInfo.h"
#include "Engine/TimeLine.h"
#include "Engine/Cache.h"
#include "Engine/Timer.h"

#include "Readers/Reader.h"

#define POWITER_FPS_REFRESH_RATE 10


using namespace Powiter;
using std::make_pair;
using boost::shared_ptr;

ViewerNode::ViewerNode(AppInstance* app):OutputNode(app)
,_uiContext(0)
,_inputsCount(1)
,_activeInput(0)
,_pboIndex(0)
,_frameCount(1)
,_forceRenderMutex()
,_forceRender(false)
,_pboUnMappedCondition()
,_pboUnMappedMutex()
,_pboUnMappedCount(0)
,_interThreadInfos()
,_timerMutex()
,_timer(new Timer)
{
    connectSlotsToViewerCache();
    
    connect(this,SIGNAL(doUpdateViewer()),this,SLOT(updateViewer()));
    connect(this,SIGNAL(doCachedEngine()),this,SLOT(cachedEngine()));
    connect(this,SIGNAL(doFrameStorageAllocation()),this,SLOT(allocateFrameStorage()));

    _timer->playState = RUNNING; /*activating the timer*/

}

void ViewerNode::connectSlotsToViewerCache(){
    Powiter::CacheSignalEmitter* emitter = appPTR->getViewerCache().activateSignalEmitter();
    QObject::connect(emitter, SIGNAL(addedEntry()), this, SLOT(onCachedFrameAdded()));
    QObject::connect(emitter, SIGNAL(removedEntry()), this, SLOT(onCachedFrameAdded()));
    QObject::connect(emitter, SIGNAL(clearedInMemoryPortion()), this, SLOT(onViewerCacheCleared()));
}

void ViewerNode::disconnectSlotsToViewerCache(){
    Powiter::CacheSignalEmitter* emitter = appPTR->getViewerCache().activateSignalEmitter();
    QObject::disconnect(emitter, SIGNAL(addedEntry()), this, SLOT(onCachedFrameAdded()));
    QObject::disconnect(emitter, SIGNAL(removedEntry()), this, SLOT(onCachedFrameAdded()));
    QObject::disconnect(emitter, SIGNAL(clearedInMemoryPortion()), this, SLOT(onViewerCacheCleared()));
}
void ViewerNode::initializeViewerTab(TabWidget* where){
    _uiContext = getApp()->addNewViewerTab(this,where);
}

ViewerNode::~ViewerNode(){
    if(_uiContext && _uiContext->getGui())
        _uiContext->getGui()->removeViewerTab(_uiContext,true);
    _timer->playState = PAUSE;

}

Powiter::Status ViewerNode::getRegionOfDefinition(SequenceTime time,Box2D* rod){
    Node* n = input(_activeInput);
    if(n){
        return n->getRegionOfDefinition(time,rod);
    }else{
        return StatFailed;
    }
}

Node::RoIMap ViewerNode::getRegionOfInterest(SequenceTime /*time*/,RenderScale /*scale*/,const Box2D& renderWindow){
    RoIMap ret;
    Node* n = input(_activeInput);
    if (n) {
        ret.insert(std::make_pair(n, renderWindow));
    }
    return ret;
}

void ViewerNode::getFrameRange(SequenceTime *first,SequenceTime *last){
    SequenceTime inpFirst = 0,inpLast = 0;
    Node* n = input(_activeInput);
    if(n){
        n->getFrameRange(&inpFirst,&inpLast);
    }
    *first = inpFirst;
    *last = inpLast;
}

bool ViewerNode::connectInput(Node* input,int inputNumber,bool autoConnection) {
    assert(input);
    InputMap::iterator found = _inputs.find(inputNumber);
    /*If the selected node is a viewer itself, return*/
    if(input->className() == "Viewer" && found!=_inputs.end() && !found->second){
        return false;
    }
    /*Adding all empty edges so it creates at least the inputNB'th one.*/
    while(_inputsCount <= inputNumber){
        tryAddEmptyInput();
    }
    //#1: first case, If the inputNB of the viewer is already connected & this is not
    // an autoConnection, just refresh it*/
    InputMap::iterator inputAlreadyConnected = _inputs.end();
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (it->second == input) {
            inputAlreadyConnected = it;
            break;
        }
    }
    
    if(found!=_inputs.end() && found->second && !autoConnection &&
       ((inputAlreadyConnected!=_inputs.end()) || (inputAlreadyConnected==_inputs.end() && input->className() == "Viewer"))){
        setActiveInputAndRefresh(found->first);
        return false;
    }
    /*#2:second case: Before connecting the appropriate edge we search for any other edge connected with the
     selected node, in which case we just refresh the already connected edge.*/
    for (InputMap::const_iterator i = _inputs.begin(); i!=_inputs.end(); ++i) {
        if(i->second && i->second == input){
            setActiveInputAndRefresh(i->first);
            return false;
        }
    }
    if (found != _inputs.end()) {
        _inputs.erase(found);
        _inputs.insert(make_pair(inputNumber,input));
        emit inputChanged(inputNumber);
        tryAddEmptyInput();
        return true;
    }
    return false;
}
bool ViewerNode::tryAddEmptyInput(){
    if(_inputs.size() <= 10){
        if(_inputs.size() > 0){
            InputMap::const_iterator it = _inputs.end();
            --it;
            if(it->second != NULL){
                addEmptyInput();
                return true;
            }
        }else{
            addEmptyInput();
            return true;
        }
    }
    return false;
}
void ViewerNode::addEmptyInput(){
    _activeInput = _inputsCount-1;
    ++_inputsCount;
    initializeInputs();
}

void ViewerNode::removeEmptyInputs(){
    /*While there're NULL inputs at the tail of the map,remove them.
     Stops at the first non-NULL input.*/
    while (_inputs.size() > 1) {
        InputMap::iterator it = _inputs.end();
        --it;
        if(it->second == NULL){
            InputMap::iterator it2 = it;
            --it2;
            if(it2->second!=NULL)
                break;
            //int inputNb = it->first;
            _inputs.erase(it);
            --_inputsCount;
            emit inputsInitialized();
        }else{
            break;
        }
    }
}
int ViewerNode::disconnectInput(int inputNumber){
    int ret = Node::disconnectInput(inputNumber);
    if(ret!=-1){
        removeEmptyInputs();
        _activeInput = _inputs.size()-1;
        initializeInputs();
    }
    return ret;
}

int ViewerNode::disconnectInput(Node* input){
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if(it->second == input){
            return disconnectInput(it->first);
        }
    }
    return -1;
}


Powiter::Status ViewerNode::renderViewer(SequenceTime time,bool fitToViewer){
    
    ViewerGL *viewer = _uiContext->viewer;
    assert(viewer);
    double zoomFactor = viewer->getZoomFactor();
    
    Box2D rod;
    Status stat = getRegionOfDefinition(time, &rod);
    if(stat == StatFailed){
        return stat;
    }
    ifInfiniteclipBox2DToProjectDefault(&rod);
    if(fitToViewer){
        viewer->fitToFormat(rod);
        zoomFactor = viewer->getZoomFactor();
    }
    viewer->setRod(rod);
    Format dispW = getProjectDefaultFormat();

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
    if(textureRect.w == 0 || textureRect.h == 0){
        return StatFailed;
    }
    _interThreadInfos._textureRect = textureRect;
    FrameKey key(time,
                 _hashValue.value(),
                 zoomFactor,
                 viewer->getExposure(),
                 viewer->lutType(),
                 viewer->byteMode(),
                 rod,
                 dispW,
                 textureRect);
    
    boost::shared_ptr<const FrameEntry> iscached;
    
    /*if we want to force a refresh, we by-pass the cache*/
    {
        QMutexLocker forceRenderLocker(&_forceRenderMutex);
        if(!_forceRender){
            iscached = appPTR->getViewerCache().get(key);
        }else{
            _forceRender = false;
        }
    }

    
    if (iscached) {
        /*Found in viewer cache, we execute the cached engine and leave*/
        _interThreadInfos._cachedEntry = iscached;
        _interThreadInfos._textureRect = iscached->getKey()._textureRect;
        {
            QMutexLocker locker(&_pboUnMappedMutex);
            emit doCachedEngine();
            while(_pboUnMappedCount <= 0) {
                _pboUnMappedCondition.wait(&_pboUnMappedMutex);
            }
            --_pboUnMappedCount;
        }
        {
            QMutexLocker locker(&_pboUnMappedMutex);
            emit doUpdateViewer();
            while(_pboUnMappedCount <= 0) {
                _pboUnMappedCondition.wait(&_pboUnMappedMutex);
            }
            --_pboUnMappedCount;
        }
        return StatOK;
    }
    
    /*We didn't find it in the viewer cache, hence we allocate
     the frame storage*/
    {
        QMutexLocker locker(&_pboUnMappedMutex);
        emit doFrameStorageAllocation();
        while(_pboUnMappedCount <= 0) {
            _pboUnMappedCondition.wait(&_pboUnMappedMutex);
        }
        --_pboUnMappedCount;
    }
    

    {
        Box2D roi(textureRect.x,textureRect.y,textureRect.r+1,textureRect.t+1);
        /*for now we skip the render scale*/
        RenderScale scale;
        scale.x = scale.y = 1.;
        RoIMap inputsRoi = getRegionOfInterest(time, scale, roi);
        //inputsRoi only contains 1 element
        RoIMap::const_iterator it = inputsRoi.begin();
#warning "Rendering only a single view for now, we need to pass the project's views count."
        boost::shared_ptr<const Powiter::Image> inputImage = it->first->renderRoI(time, scale,0 ,it->second);
        
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
                                  boost::bind(&ViewerNode::renderFunctor,this,inputImage,_1,columns));
        future.waitForFinished();
    }
    //we released the input image and force the cache to clear exceeding entries
    appPTR->clearExceedingEntriesFromNodeCache();
    
    viewer->stopDisplayingProgressBar();
    
    /*we copy the frame to the cache*/
    if(!_renderAborted){
        assert(sizeof(Powiter::Cache<Powiter::FrameEntry>::data_t) == 1); // _dataSize is in bytes, so it has to be a byte cache
        size_t bytesToCopy = _interThreadInfos._pixelsCount;
        boost::shared_ptr<FrameEntry> cachedFrame = appPTR->getViewerCache().newEntry(key,bytesToCopy, 1);
        if(!cachedFrame){
            std::cout << "Failed to cache the frame rendered by the viewer." << std::endl;
            return StatOK;
        }
        if(viewer->hasHardware() && !viewer->byteMode()){
            bytesToCopy *= sizeof(float);
        }
        memcpy((char*)cachedFrame->data(),viewer->getFrameData(),bytesToCopy);
    }
    
    QMutexLocker locker(&_pboUnMappedMutex);
    emit doUpdateViewer();
    while(_pboUnMappedCount <= 0) {
        _pboUnMappedCondition.wait(&_pboUnMappedMutex);
    }
    --_pboUnMappedCount;
    return StatOK;
}

void ViewerNode::renderFunctor(boost::shared_ptr<const Powiter::Image> inputImage,
                               const std::vector<std::pair<int,int> >& rows,
                               const std::vector<int>& columns){
    for(U32 i = 0; i < rows.size();++i){
        _uiContext->viewer->drawRow(inputImage->pixelAt(0, rows[i].first), columns, rows[i].second);
    }
}


void ViewerNode::updateViewer(){
    QMutexLocker locker(&_pboUnMappedMutex);
    
    ViewerGL* viewer = _uiContext->viewer;
    
    if(!_renderAborted){
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
    
    
    ++_pboUnMappedCount;
    _pboUnMappedCondition.wakeOne();
}

void ViewerNode::cachedEngine(){
    QMutexLocker locker(&_pboUnMappedMutex);
    
    assert(_interThreadInfos._cachedEntry);
    size_t dataSize = 0;
    int w = _interThreadInfos._textureRect.w;
    int h = _interThreadInfos._textureRect.h;
    dataSize  = w * h * 4 ;
    const Box2D& dataW = _interThreadInfos._cachedEntry->getKey()._dataWindow;
    Format dispW = _interThreadInfos._cachedEntry->getKey()._displayWindow;
    _uiContext->viewer->setRod(dataW);
    if(_app->shouldAutoSetProjectFormat()){
        _app->setProjectFormat(dispW);
        _app->setAutoSetProjectFormat(false);
    }
    /*allocating pbo*/
    void* output = _uiContext->viewer->allocateAndMapPBO(dataSize, _uiContext->viewer->getPBOId(_pboIndex));
    assert(output); 
    _pboIndex = (_pboIndex+1)%2;
    _uiContext->viewer->fillPBO((const char*)_interThreadInfos._cachedEntry->data(), output, dataSize);

    ++_pboUnMappedCount;
    _pboUnMappedCondition.wakeOne();
}

void ViewerNode::allocateFrameStorage(){
    QMutexLocker locker(&_pboUnMappedMutex);
    _interThreadInfos._pixelsCount = _interThreadInfos._textureRect.w * _interThreadInfos._textureRect.h * 4;
    _uiContext->viewer->allocateFrameStorage(_interThreadInfos._pixelsCount);
    ++_pboUnMappedCount;
    _pboUnMappedCondition.wakeOne();
    
}

void ViewerNode::setDesiredFPS(double d){
    QMutexLocker timerLocker(&_timerMutex);
    _timer->setDesiredFrameRate(d);
}

void ViewerNode::onCachedFrameAdded(){
    emit addedCachedFrame(_app->getTimeLine()->currentFrame());
}
void ViewerNode::onCachedFrameRemoved(){
    emit removedCachedFrame();
}
void ViewerNode::onViewerCacheCleared(){
    emit clearedViewerCache();
}
void ViewerNode::setActiveInputAndRefresh(int inputNb){
    InputMap::iterator it = _inputs.find(inputNb);
    if(it!=_inputs.end() && it->second!=NULL){
        _activeInput = inputNb;
        updateTreeAndRender();
    }
}

void ViewerNode::redrawViewer(){
    emit mustRedraw();
}

void ViewerNode::swapBuffers(){
    emit mustSwapBuffers();
}

void ViewerNode::pixelScale(double &x,double &y) {
    assert(_uiContext);
    assert(_uiContext->viewer);
    x = _uiContext->viewer->getDisplayWindow().getPixelAspect();
    y = 2. - x;
}

void ViewerNode::backgroundColor(double &r,double &g,double &b) {
    assert(_uiContext);
    assert(_uiContext->viewer);
    _uiContext->viewer->backgroundColor(r, g, b);
}

void ViewerNode::viewportSize(double &w,double &h) {
    assert(_uiContext);
    assert(_uiContext->viewer);
    const Format& f = _uiContext->viewer->getDisplayWindow();
    w = f.width();
    h = f.height();
}

void ViewerNode::drawOverlays() const{
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock(); /*it might be locked already if a node forced a re-render*/
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->drawOverlay();
        }
    }
    _dag.unlock();
}

void ViewerNode::notifyOverlaysPenDown(const QPointF& viewportPos,const QPointF& pos){
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock();
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayPenDown(viewportPos, pos);
        }
    }
    _dag.unlock();
}

void ViewerNode::notifyOverlaysPenMotion(const QPointF& viewportPos,const QPointF& pos){
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock();
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayPenMotion(viewportPos, pos);
        }
    }
    _dag.unlock();
}

void ViewerNode::notifyOverlaysPenUp(const QPointF& viewportPos,const QPointF& pos){
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock();
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayPenUp(viewportPos, pos);
        }
    }
    _dag.unlock();
}

void ViewerNode::notifyOverlaysKeyDown(QKeyEvent* e){
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock();
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayKeyDown(e);
        }
    }
    _dag.unlock();
}

void ViewerNode::notifyOverlaysKeyUp(QKeyEvent* e){
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock();
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayKeyUp(e);
        }
    }
    _dag.unlock();
}

void ViewerNode::notifyOverlaysKeyRepeat(QKeyEvent* e){
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock();
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayKeyRepeat(e);
        }
    }
    _dag.unlock();
}

void ViewerNode::notifyOverlaysFocusGained(){
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock();
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayFocusGained();
        }
    }
    _dag.unlock();
}

void ViewerNode::notifyOverlaysFocusLost(){
    const Tree& _dag = getVideoEngine()->getTree();
    _dag.lock();
    if(_dag.getOutput()){
        for (Tree::TreeIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayFocusLost();
        }
    }
    _dag.unlock();
}
