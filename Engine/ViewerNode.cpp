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

#include "Gui/ViewerGL.h"
#include "Gui/Gui.h"
#include "Global/AppManager.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Engine/Row.h"
#include "Gui/Timeline.h"
#include "Engine/ViewerCache.h"
#include "Readers/Reader.h"
#include "Engine/MemoryFile.h"
#include "Engine/Model.h"
#include "Engine/VideoEngine.h"
#include "Global/AppManager.h"

using namespace Powiter;

ViewerNode::ViewerNode(ViewerCache* cache,Model* model):OutputNode(model),
_viewerInfos(0),
_uiContext(0),
_viewerCache(cache),
_pboIndex(0)
{
}

void ViewerNode::initializeViewerTab(TabWidget* where){
   _uiContext = _model->getApp()->addNewViewerTab(this,where);
}

ViewerNode::~ViewerNode(){
    if(_uiContext->getGui())
        _uiContext->getGui()->removeViewerTab(_uiContext,true);
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
