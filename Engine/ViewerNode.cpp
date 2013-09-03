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

#include <QtConcurrentRun>

#include "Gui/ViewerGL.h"
#include "Gui/Gui.h"
#include "Global/Controler.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Engine/Row.h"
#include "Gui/Timeline.h"
#include "Engine/ViewerCache.h"
#include "Readers/Reader.h"
#include "Engine/MemoryFile.h"
#include "Engine/Model.h"
#include "Engine/VideoEngine.h"
#include "Global/Controler.h"

using namespace Powiter;

ViewerNode::ViewerNode(ViewerCache* cache):Node(),
_viewerInfos(0),
_uiContext(0),
_viewerCache(cache),
_pboIndex(0)
{
    _cacheWatcher = new QFutureWatcher<void>;
   QObject::connect(_cacheWatcher, SIGNAL(finished()),ctrlPTR->getModel()->getVideoEngine(), SLOT(engineLoop()));
}

void ViewerNode::initializeViewerTab(TabWidget* where){
   _uiContext = ctrlPTR->getGui()->addNewViewerTab(this,where);
}

ViewerNode::~ViewerNode(){
    if(ctrlPTR->getGui())
        ctrlPTR->getGui()->removeViewerTab(_uiContext,true);
    if(_viewerInfos)
        delete _viewerInfos;
}

bool ViewerNode::_validate(bool){
    
   // (void)forReal;
   // makeCurrentViewer();
    return true;
}

std::string ViewerNode::description(){
    return "OutputNode";
}

void ViewerNode::engine(int y,int offset,int range,ChannelSet ,Row* out){
    Row* row = input(0)->get(y,offset,range);
    if(row){
        row->zoomedY(out->zoomedY());
        
        /*drawRow will fill a portion of the RAM buffer holding the frame.
         This will write at the appropriate offset in the buffer thanks
         to the zoomedY(). Note that this can be called concurrently since
         2 rows do not overlap in memory.*/
        //  const ChannelSet& uiChannels = _uiContext->displayChannels();
        const float* r = NULL;
        const float* g = NULL;
        const float* b = NULL;
        const float* a = NULL;
        
        //        if (uiChannels & Channel_red)
            r = (*row)[Channel_red];
        //        if (uiChannels & Channel_green)
            g = (*row)[Channel_green];
        //        if (uiChannels & Channel_blue)
            b = (*row)[Channel_blue];
        //        if (uiChannels & Channel_alpha)
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
    _viewerInfos->set(dynamic_cast<const Box2D&>(*_info));
    _viewerInfos->setChannels(_info->channels());
    _viewerInfos->setDisplayWindow(_info->getDisplayWindow());
    _viewerInfos->firstFrame(_info->firstFrame());
    _viewerInfos->lastFrame(_info->lastFrame());
    
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
    _firstFrame = -1;
    _lastFrame = -1;
    _channels = Mask_None;
    set(0, 0, 0, 0);
    _displayWindow.set(0,0,0,0);
}

void ViewerInfos::mergeDisplayWindow(const Format& other){
    _displayWindow.merge(other);
    _displayWindow.pixel_aspect(other.pixel_aspect());
    if(_displayWindow.name().empty()){
        _displayWindow.name(other.name());
    }
}

bool ViewerInfos::operator==( const ViewerInfos& other){
	if(other.channels()==this->channels() &&
       other.firstFrame()==this->_firstFrame &&
       other.lastFrame()==this->_lastFrame &&
       other.getDisplayWindow()==this->_displayWindow
       ){
        return true;
	}else{
		return false;
	}
    
}
void ViewerInfos::operator=(const ViewerInfos &other){
    _channels = other._channels;
    _firstFrame = other._firstFrame;
    _lastFrame = other._lastFrame;
    _displayWindow = other._displayWindow;
    set(other);
    rgbMode(other._rgbMode);
}

FrameEntry* ViewerNode::get(U64 key){
    
  return  _viewerCache->get(key);
}


void ViewerNode::cachedFrameEngine(FrameEntry* frame){
    assert(frame);
    size_t dataSize = 0;
    int w = frame->_textureRect.w;
    int h = frame->_textureRect.h;
    //Texture::DataType type;
    if(frame->_byteMode==1.0){
        dataSize  = w * h * sizeof(U32) ;
        //type = Texture::BYTE;
    }else{
        dataSize  = w * h  * sizeof(float) * 4;
        //type = Texture::FLOAT;
    }
    ViewerGL* gl_viewer = _uiContext->viewer;
    if(_viewerInfos){
        delete _viewerInfos;
    }
    _viewerInfos = new ViewerInfos;
    _viewerInfos->set(dynamic_cast<const Box2D&>(*frame->_frameInfo));
    _viewerInfos->setChannels(frame->_frameInfo->channels());
    _viewerInfos->setDisplayWindow(frame->_frameInfo->getDisplayWindow());
    _viewerInfos->firstFrame(_info->firstFrame());
    _viewerInfos->lastFrame(_info->lastFrame());
    _uiContext->setCurrentViewerInfos(_viewerInfos,false);
    
    
    /*allocating pbo*/
    void* output = gl_viewer->allocateAndMapPBO(dataSize, gl_viewer->getPBOId(_pboIndex));
    checkGLErrors();
    assert(output); // FIXME: crashes here when using two viewers, each connected to a different reader
    _pboIndex = (_pboIndex+1)%2;
    const char* cachedFrame = frame->getMappedFile()->data();
    assert(cachedFrame);
    QFuture<void> future = QtConcurrent::run(this,&ViewerNode::retrieveCachedFrame,cachedFrame,output,dataSize);
    _cacheWatcher->setFuture(future);
   
    
}
void ViewerNode::retrieveCachedFrame(const char* cachedFrame,void* dst,size_t dataSize){
    assert(dst);
    assert(cachedFrame);
    _uiContext->viewer->fillPBO(cachedFrame, dst, dataSize);
}
