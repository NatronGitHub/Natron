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

 

 



#include "Core/viewerNode.h"

#include <QtConcurrentRun>

#include "Gui/GLViewer.h"
#include "Gui/mainGui.h"
#include "Superviser/controler.h"
#include "Gui/viewerTab.h"
#include "Gui/GLViewer.h"
#include "Core/row.h"
#include "Gui/timeline.h"
#include "Core/viewercache.h"
#include "Reader/Reader.h"
#include "Core/mappedfile.h"
#include "Core/model.h"
#include "Core/VideoEngine.h"
#include "Superviser/controler.h"

using namespace Powiter;
Viewer::Viewer(ViewerCache* cache,TextureCache* textureCache):OutputNode(),
_viewerInfos(0),
_uiContext(0),
_viewerCache(cache),
_textureCache(textureCache),
_pboIndex(0),
_currentZoomFactor(1.f)
{
    _cacheWatcher = new QFutureWatcher<void>;
   QObject::connect(_cacheWatcher, SIGNAL(finished()),ctrlPTR->getModel()->getVideoEngine(), SLOT(engineLoop()));
}

void Viewer::initializeViewerTab(TabWidget* where){
   _uiContext = ctrlPTR->getGui()->addViewerTab(this,where);
}

Viewer::~Viewer(){
    if(ctrlPTR->getGui())
        ctrlPTR->getGui()->removeViewerTab(_uiContext,true);
    if(_viewerInfos)
        delete _viewerInfos;
}

void Viewer::_validate(bool){
    
   // (void)forReal;
   // makeCurrentViewer();
    
}

std::string Viewer::description(){
    return "OutputNode";
}

void Viewer::engine(int y,int offset,int range,ChannelSet channels,Row* out){
    InputRow row;
    input(0)->get(y, offset, range, channels, row);
    Row* internal = row.getInternalRow();
    if(internal){
        internal->zoomedY(out->zoomedY());
        
        /*drawRow will fill a portion of the RAM buffer holding the frame.
         This will write at the appropriate offset in the buffer thanks
         to the zoomedY(). Note that this can be called concurrently since
         2 rows do not overlap in memory.*/
        _uiContext->viewer->drawRow(row[Channel_red],
                                    row[Channel_green],
                                    row[Channel_blue],
                                    row[Channel_alpha],
                                    _currentZoomFactor,
                                    internal->zoomedY());
    }
}

void Viewer::makeCurrentViewer(){
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

int Viewer::firstFrame() const{
    return _uiContext->frameSeeker->firstFrame();
}

int Viewer::lastFrame() const{
    return _uiContext->frameSeeker->lastFrame();
}

int Viewer::currentFrame() const{
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

FrameEntry* Viewer::get(U64 key){
    
  return  _viewerCache->get(key);
}
bool Viewer::isTextureCached(U64 key){
    
    TextureEntry* found = _textureCache->get(key);
    if(found != NULL){
        _uiContext->viewer->setCurrentDisplayTexture(found);
        return true;
    }
    return false;
}

void Viewer::cachedFrameEngine(FrameEntry* frame){
    int w = frame->_actualW ;
    int h = frame->_actualH ;
    size_t dataSize = 0;
    TextureEntry::DataType type;
    if(frame->_byteMode==1.0){
        dataSize  = w * h * sizeof(U32) ;
        type = TextureEntry::BYTE;
    }else{
        dataSize  = w * h  * sizeof(float) * 4;
        type = TextureEntry::FLOAT;
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
    
    
    /*resizing texture if needed, the calls must be made in that order*/
    gl_viewer->getViewerCacheTexture()->allocate(w, h, type);
    gl_viewer->setCurrentDisplayTexture(gl_viewer->getViewerCacheTexture());
    /*allocating pbo*/
    void* output = gl_viewer->allocateAndMapPBO(dataSize, gl_viewer->getPBOId(_pboIndex));
    checkGLErrors();
    _pboIndex = (_pboIndex+1)%2;
    const char* cachedFrame = frame->getMappedFile()->data();
    QFuture<void> future = QtConcurrent::run(this,&Viewer::retrieveCachedFrame,cachedFrame,output,dataSize);
    _cacheWatcher->setFuture(future);
   
    
}
void Viewer::retrieveCachedFrame(const char* cachedFrame,void* dst,size_t dataSize){
    _uiContext->viewer->fillPBO(cachedFrame, dst, dataSize);
}
