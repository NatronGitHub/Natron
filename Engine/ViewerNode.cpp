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

#include "Readers/Reader.h"

using namespace Powiter;
using std::make_pair;
using boost::shared_ptr;

ViewerNode::ViewerNode(AppInstance* app):OutputNode(app),
_uiContext(0),
_inputsCount(1),
_activeInput(0),
_pboIndex(0)
{
    connectSlotsToViewerCache();
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
}

Powiter::Status ViewerNode::getRegionOfDefinition(SequenceTime time,Box2D* rod,Format* displayWindow){
    return input(_activeInput)->getRegionOfDefinition(time,rod,displayWindow);
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

void ViewerNode::renderRow(SequenceTime time,int left,int right,int y,int textureY){
    shared_ptr<const Row> row = input(activeInput())->get(time,y,left,right,_uiContext->displayChannels());
    if (row) {
        /*drawRow will fill a portion of the RAM buffer holding the frame.
         This will write at the appropriate offset in the buffer thanks
         to the zoomedY(). Note that this can be called concurrently since
         2 rows do not overlap in memory.*/
        const float* r = row->begin(Channel_red) ;
        const float* g = row->begin(Channel_green);
        const float* b = row->begin(Channel_blue);
        const float* a = row->begin(Channel_alpha);
        if(r)
            r -= row->left();
        if(g)
            g -= row->left();
        if(b)
            b -= row->left();
        if(a)
            a -= row->left();
        _uiContext->viewer->drawRow(r,g,b,a,textureY);
    }
}

void ViewerNode::cachedFrameEngine(boost::shared_ptr<const FrameEntry> frame){
    assert(frame);
    size_t dataSize = 0;
    int w = frame->getKey()._textureRect.w;
    int h = frame->getKey()._textureRect.h;
    if(frame->getKey()._byteMode==1.0){
        dataSize  = w * h * sizeof(U32) ;
    }else{
        dataSize  = w * h  * sizeof(float) * 4;
    }
    const Box2D& dataW = frame->getKey()._dataWindow;
    Format dispW = frame->getKey()._displayWindow;
    _uiContext->viewer->setRod(dataW);
    if(_app->shouldAutoSetProjectFormat()){
        _app->setProjectFormat(dispW);
        _app->setAutoSetProjectFormat(false);
    }
    /*allocating pbo*/
    void* output = _uiContext->viewer->allocateAndMapPBO(dataSize, _uiContext->viewer->getPBOId(_pboIndex));
    checkGLErrors();
    assert(output); // FIXME: crashes here when using two viewers, each connected to a different reader
    _pboIndex = (_pboIndex+1)%2;
    _uiContext->viewer->fillPBO((const char*)frame->data(), output, dataSize);
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
