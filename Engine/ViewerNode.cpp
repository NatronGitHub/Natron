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

#include "Global/AppManager.h"

#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/TimeLineGui.h"

#include "Engine/Row.h"
#include "Engine/ViewerCache.h"
#include "Engine/MemoryFile.h"
#include "Engine/Model.h"
#include "Engine/VideoEngine.h"
#include "Engine/OfxNode.h"

#include "Readers/Reader.h"

using namespace Powiter;

ViewerNode::ViewerNode(Model* model):OutputNode(model),
_viewerInfos(0),
_uiContext(0),
_pboIndex(0),
_inputsCount(1),
_activeInput(0)
{
    connectSlotsToViewerCache();
}

void ViewerNode::connectSlotsToViewerCache(){
    ViewerCache* cache = appPTR->getViewerCache();
    QObject::connect(cache, SIGNAL(addedFrame()), this, SLOT(onCachedFrameAdded()));
    QObject::connect(cache, SIGNAL(removedFrame()), this, SLOT(onCachedFrameAdded()));
    QObject::connect(cache, SIGNAL(clearedInMemoryFrames()), this, SLOT(onViewerCacheCleared()));
}

void ViewerNode::disconnectSlotsToViewerCache(){
    ViewerCache* cache = appPTR->getViewerCache();
    QObject::disconnect(cache, SIGNAL(addedFrame()), this, SLOT(onCachedFrameAdded()));
    QObject::disconnect(cache, SIGNAL(removedFrame()), this, SLOT(onCachedFrameAdded()));
    QObject::disconnect(cache, SIGNAL(clearedInMemoryFrames()), this, SLOT(onViewerCacheCleared()));
}
void ViewerNode::initializeViewerTab(TabWidget* where){
    _uiContext = _model->getApp()->addNewViewerTab(this,where);
}

ViewerNode::~ViewerNode(){
    if(_uiContext && _uiContext->getGui())
        _uiContext->getGui()->removeViewerTab(_uiContext,true);
    if(_viewerInfos)
        delete _viewerInfos;
}

bool ViewerNode::_validate(bool){
    return true;
}
bool ViewerNode::connectInput(Node* input,int inputNumber,bool autoConnection){
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
        _inputs.insert(std::make_pair(inputNumber,input));
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

std::string ViewerNode::description() const {
    return "The Viewer node can display the output of a node graph.";
}

void ViewerNode::engine(int y,int offset,int range,ChannelSet ,Row* out){
    Row* row = input(_activeInput)->get(y,offset,range);
    if(row){
        int zoomedY = out->zoomedY();
        //assert(zoomedY > 0.);
        row->set_zoomedY(zoomedY);
        /*drawRow will fill a portion of the RAM buffer holding the frame.
         This will write at the appropriate offset in the buffer thanks
         to the zoomedY(). Note that this can be called concurrently since
         2 rows do not overlap in memory.*/
        const float* r = NULL;
        const float* g = NULL;
        const float* b = NULL;
        const float* a = NULL;
        r = (*row)[Channel_red];
        g = (*row)[Channel_green];
        b = (*row)[Channel_blue];
        a = (*row)[Channel_alpha];
        _uiContext->viewer->drawRow(r,g,b,a,row->zoomedY());
        row->release();
    }
}

void ViewerNode::makeCurrentViewer(){
    if(_viewerInfos){
        delete _viewerInfos;
    }
    _viewerInfos = new ViewerInfos;
    _viewerInfos->set_channels(info().channels());
    _viewerInfos->set_dataWindow(info().dataWindow());
    _viewerInfos->set_displayWindow(info().displayWindow());
    _viewerInfos->set_firstFrame(info().firstFrame());
    _viewerInfos->set_lastFrame(info().lastFrame());
    
    _uiContext->setCurrentViewerInfos(_viewerInfos,false);
}

int ViewerNode::firstFrame() const{
    return _uiContext->frameSeeker->firstFrame();
}

int ViewerNode::lastFrame() const{
    return _uiContext->frameSeeker->lastFrame();
}

int ViewerNode::currentFrame() const{
    return _uiContext->frameSeeker->currentFrame();
}


void ViewerInfos::reset(){
    set_firstFrame(-1);
    set_lastFrame(-1);
    set_channels(Mask_None);
    _dataWindow.set(0, 0, 0, 0);
    _displayWindow.set(0,0,0,0);
}

void ViewerInfos::merge_displayWindow(const Format& other){
    _displayWindow.merge(other);
    _displayWindow.pixel_aspect(other.pixel_aspect());
    if(_displayWindow.name().empty()){
        _displayWindow.name(other.name());
    }
}

bool ViewerInfos::operator==( const ViewerInfos& other){
	if(other.channels()==this->channels() &&
       other.firstFrame()==this->firstFrame() &&
       other.lastFrame()==this->lastFrame() &&
       other.displayWindow()==this->displayWindow()
       ){
        return true;
	}else{
		return false;
	}
    
}
void ViewerInfos::operator=(const ViewerInfos &other){
    set_channels(other.channels());
    set_firstFrame(other.firstFrame());
    set_lastFrame(other.lastFrame());
    set_displayWindow(other.displayWindow());
    set_dataWindow(other.dataWindow());
    set_rgbMode(other.rgbMode());
}

void ViewerNode::cachedFrameEngine(FrameEntry* frame){
    assert(frame);
    size_t dataSize = 0;
    int w = frame->_textureRect.w;
    int h = frame->_textureRect.h;
    if(frame->_byteMode==1.0){
        dataSize  = w * h * sizeof(U32) ;
    }else{
        dataSize  = w * h  * sizeof(float) * 4;
    }
    ViewerGL* gl_viewer = _uiContext->viewer;
    if(_viewerInfos){
        delete _viewerInfos;
    }
    _viewerInfos = new ViewerInfos;
    _viewerInfos->set_dataWindow(frame->_frameInfo->dataWindow());
    _viewerInfos->set_channels(frame->_frameInfo->channels());
    _viewerInfos->set_displayWindow(frame->_frameInfo->displayWindow());
    _viewerInfos->set_firstFrame(_info.firstFrame());
    _viewerInfos->set_lastFrame(_info.lastFrame());
    _uiContext->setCurrentViewerInfos(_viewerInfos,false);
    /*allocating pbo*/
    void* output = gl_viewer->allocateAndMapPBO(dataSize, gl_viewer->getPBOId(_pboIndex));
    checkGLErrors();
    assert(output); // FIXME: crashes here when using two viewers, each connected to a different reader
    _pboIndex = (_pboIndex+1)%2;
    const char* cachedFrame = frame->getMappedFile()->data();
    assert(cachedFrame);
    _uiContext->viewer->fillPBO(cachedFrame, output, dataSize);
}

void ViewerNode::onCachedFrameAdded(){
    emit addedCachedFrame(currentFrame());
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
        updateDAG(false);
    }
}

void ViewerNode::redrawViewer(){
    emit mustRedraw();
}

void ViewerNode::swapBuffers(){
    emit mustSwapBuffers();
}

void ViewerNode::pixelScale(double &x,double &y){
    x = _uiContext->viewer->displayWindow().pixel_aspect();
    y = 2. - x;
}

void ViewerNode::backgroundColor(double &r,double &g,double &b){
    _uiContext->viewer->backgroundColor(r, g, b);
}
void ViewerNode::viewportSize(double &w,double &h){
    const Format& f = _uiContext->viewer->displayWindow();
    w = f.width();
    h = f.height();
}

void ViewerNode::drawOverlays() const{
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->drawOverlay();
        }
    }
}

void ViewerNode::notifyOverlaysPenDown(const QPointF& viewportPos,const QPointF& pos){
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayPenDown(viewportPos, pos);
        }
    }
}

void ViewerNode::notifyOverlaysPenMotion(const QPointF& viewportPos,const QPointF& pos){
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayPenMotion(viewportPos, pos);
        }
    }
}

void ViewerNode::notifyOverlaysPenUp(const QPointF& viewportPos,const QPointF& pos){
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayPenUp(viewportPos, pos);
        }
    }
}

void ViewerNode::notifyOverlaysKeyDown(QKeyEvent* e){
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayKeyDown(e);
        }
    }
}

void ViewerNode::notifyOverlaysKeyUp(QKeyEvent* e){
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayKeyUp(e);
        }
    }
}

void ViewerNode::notifyOverlaysKeyRepeat(QKeyEvent* e){
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayKeyRepeat(e);
        }
    }
}

void ViewerNode::notifyOverlaysFocusGained(){
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayFocusGained();
        }
    }
}

void ViewerNode::notifyOverlaysFocusLost(){
    const VideoEngine::DAG& _dag = getVideoEngine()->getCurrentDAG();
    if(_dag.getOutput()){
        for (VideoEngine::DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            assert(*it);
            (*it)->onOverlayFocusLost();
        }
    }
}