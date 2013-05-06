//
//  VideoEngine.cpp
//  PowiterOsX
//
//  Created by Alexandre on 1/16/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//
#include <QtCore/QMutex>
#include <QtCore/qcoreapplication.h>
#include <QtGui/QPushButton>
#include <iterator>
#include <cassert>
#include "Core/VideoEngine.h"
#include "Core/inputnode.h"
#include "Core/outputnode.h"
#include "Core/workerthread.h"
#include "Core/mappedfile.h"
#include "Core/viewerNode.h"
#include "Core/settings.h"
#include "Core/model.h"
#include "Core/hash.h"
#include "Core/Timer.h"

#include "Gui/mainGui.h"
#include "Gui/viewerTab.h"
#include "Gui/timeline.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/GLViewer.h"

#include "Reader/Reader.h"
#include "Reader/Reader.h"
#include "Reader/readerInfo.h"

#include "Superviser/controler.h"
#include "Superviser/MemoryInfo.h"
#include "Superviser/PwStrings.h"


using Powiter_Enums::ROW_RANK;

/* - Computes the tree represented by output and inputs for nbFrames ( if -1, cycles through all frames), starting where it was left off.
 - initViewer is on if we just changed the video sequence
 - sameFrame is on if the call to this function is the result of a zoom operation requested by the user or if this is the
    first time we ran the engine for this frame
 - _runTasks is true when the call has not been made by a task itself. 
 */
void VideoEngine::video_sequence_engine(OutputNode *output,std::vector<InputNode*> inputs,int nbFrames,
                                        bool initViewer,bool sameFrame,bool _runTasks){
    _working=true; // flagging that the engine is currently working
    //cout << "video engine working..." << endl;
    // if  -1, we cycle through all frames
    if(nbFrames==-1)
        nbFrames=gl_viewer->getCurrentReaderInfo()->lastFrame()-gl_viewer->getCurrentReaderInfo()->firstFrame()+1;
    int counter = 0; // counter for nbFrames
    
    if(!sameFrame)
        clearCache();
    
    /*CURRENTLY NOT WORKING FOR SEVERAL INPUTS*/
    _aborted = false; // initialize it to false so we can detect premature abortion
    _paused = false; // same for pause
    
    // setting the timer as RUNNING for regular display ticks
    _timer->playState=RUNNING;
    // if inputs[0] is a reader we make its reader info the current ones
	if(!strcmp(inputs[0]->class_name(),"Reader")){
		makeReaderInfoCurrent(static_cast<Reader*>(inputs[0]));
	}
    IntegerBox dataW = gl_viewer->dataWindow();
    DisplayFormat dispW = gl_viewer->displayWindow();
    
    // if the starting frame is not between FIRST and LAST of the timeslider we put it back into a good range.
    bool adjustedFrameRange = false; // check if we already called setupfornextframe
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
	int first=gl_viewer->getCurrentReaderInfo()->currentFrame();
    if(first < frameSeeker->firstFrame()){
        adjustedFrameRange=true;
        first = frameSeeker->firstFrame();
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
				Reader* readerInput = static_cast<Reader*>(currentInput);
				readerInput->randomFrame(first);
				readerInput->setup_for_next_frame();
			}
		}
        gl_viewer->getCurrentReaderInfo()->currentFrame(first);
    }
	int last=frameSeeker->lastFrame();
    if(first > frameSeeker->lastFrame()){
        adjustedFrameRange=true;
        first = frameSeeker->firstFrame();
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
				Reader* readerInput = static_cast<Reader*>(currentInput);
				readerInput->randomFrame(first);
				readerInput->setup_for_next_frame();
			}
		}
        gl_viewer->getCurrentReaderInfo()->currentFrame(first);
    }
    
    
    
    // we get the current filename for later purpose
    char* currentFrameName=0;
    if(strcmp(inputs[0]->class_name(),"Reader")==0){
        currentFrameName = static_cast<Reader*>(inputs[0])->getCurrentFrameName();
    }
    // check the presence of the frame in the cache
    int consecutiveCachedFrames = 1;
    pair<FramesIterator,bool> iscached = _cache->isCached(currentFrameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                                          gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
    
    // if we didn't call setupfornextframe, call it now in order to initialize the info on the reader
    if(!adjustedFrameRange){
        for(int j=0;j<inputs.size();j++){
            InputNode* currentInput=inputs[j];
            if(strcmp(currentInput->class_name(),"Reader")==0){
                static_cast<Reader*>(currentInput)->setup_for_next_frame();
            }
        }
    }
    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());

    
    if(initViewer){
        // adjusting viewer settings so the frame fit to the viewer
        gl_viewer->initViewer();
    }
    gl_viewer->makeCurrent();

    
    if(!iscached.second){
        int frameHint = gl_viewer->getCurrentReaderInfo()->lastFrame() - gl_viewer->getCurrentReaderInfo()->firstFrame()+1;
        // hint for the cache in case it has to release some disk space: we free about 10%
        computeTreeForFrame(output,inputs[0],ceil((double)frameHint/10.0),true);
        if(_aborted){
            //clearCache();
            return;
        }
        updateDisplay();
        
    }else{ // FOUND IN DISK CACHE, activating double PBO video engine
        // first we need to know how many consecutive frames are stored in cache for maximum playback efficiency
        vector<pair<char*,FramesIterator> > fileNames;
        fileNames.push_back(make_pair(currentFrameName,iscached.first));
        if(strcmp(inputs[0]->class_name(),"Reader")==0){
            int k = gl_viewer->getCurrentReaderInfo()->currentFrame()  +1;
            int c = 1;
            while(k <= last && c < nbFrames){
                char* frameName = static_cast<Reader*>(inputs[0])->getRandomFrameName(k);
                iscached = _cache->isCached(frameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                            gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
                free(frameName);
                if(iscached.second){
                    c++;
                    fileNames.push_back(make_pair(frameName,iscached.first));
                }else{
                    break;
                }
                k++;
            }
        }
        int frameCount = gl_viewer->getCurrentReaderInfo()->lastFrame() - gl_viewer->getCurrentReaderInfo()->firstFrame()+1;
        cached_video_engine(frameCount==1,gl_viewer->getCurrentReaderInfo()->currentFrame(),fileNames,true,true);
        consecutiveCachedFrames = fileNames.size();
    }
    
    counter++;
    counter+= (consecutiveCachedFrames-1);
    // we loop and do the same thing for the rest of the frames
    int i =first+1 + (consecutiveCachedFrames - 1);
    while(i<=last && counter<nbFrames){
        // clearInfos(output);
        bool initTexture = false;
        gl_viewer->currentFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()+1);
		for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
				inp->incrementCurrentFrameIndex();
				inp->setup_for_next_frame();
                /*checking if the dataW/dispW has changed in the reader info in which case we need to
                 make it current again*/
                IntegerBox newDataW = readersInfos[inp]->dataWindow();
                DisplayFormat newDispW = readersInfos[inp]->displayWindow();
                if(newDataW != dataW || newDispW != dispW){
                    dataW = newDataW;
                    dispW = newDispW;
                    initTexture = true;
                    makeReaderInfoCurrent(inp);
                }
                    
			}
		}
        assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
        clearCache();
        
        if(strcmp(inputs[0]->class_name(),"Reader")==0){
            currentFrameName = static_cast<Reader*>(inputs[0])->getCurrentFrameName();
        }
        consecutiveCachedFrames = 1;
        iscached=_cache->isCached(currentFrameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                  gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
        if(!iscached.second){
            int frameHint = gl_viewer->getCurrentReaderInfo()->lastFrame() - gl_viewer->getCurrentReaderInfo()->firstFrame()+1;
            computeTreeForFrame(output,inputs[0],ceil((double)frameHint/10.0),initTexture); // NOT GOOD
            if(_aborted) return;
            _timer->waitUntilNextFrameIsDue();
            updateDisplay();
            
            
        }else{ // FOUND IN DISK CACHE, activating double PBO video engine
            // first we need to know how many consecutive frames are stored in cache for maximum playback effiency
            vector<std::pair<char*,FramesIterator> > fileNames;
            fileNames.push_back(make_pair(currentFrameName,iscached.first));
            if(strcmp(inputs[0]->class_name(),"Reader")==0){
                int k = gl_viewer->getCurrentReaderInfo()->currentFrame() +1;
                int c= 1;
                while(k <= last && c < nbFrames){
                    char* frameName = static_cast<Reader*>(inputs[0])->getRandomFrameName(k);
                    iscached=_cache->isCached(frameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                              gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
                    free(frameName);
                    if(iscached.second){
                        c++;
                        fileNames.push_back(make_pair(frameName,iscached.first));
                    }else{
                        break;
                    }
                    k++;
                }
            }
            cached_video_engine(false,gl_viewer->getCurrentReaderInfo()->currentFrame(),fileNames, false,true);
            consecutiveCachedFrames = fileNames.size();
        }
        
        if(_paused){
            //clearCache();
            _working = false;
            break;
        } // if user pressed pause
		if((i%24)==0){
			//cout << "fps :" << _timer->actualFrameRate() << endl;
            emit fpsChanged(_timer->actualFrameRate());
        }
        i++;
        i+=(consecutiveCachedFrames-1);
        counter++;
        counter+=(consecutiveCachedFrames-1);
    }
    
    _timer->playState=PAUSE;
    _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
    _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
    
    _paused=true;
    _working=false;
    //cout << "video engine finished working..." << endl;
    //cout << row_cache.size() << endl;
    /*Run queuing tasks*/
    if(_runTasks)
        runTasks();

}

void VideoEngine::backward_video_sequence_engine(OutputNode *output,std::vector<InputNode*> inputs,int nbFrames,bool _runTasks){
    _working=true; // flagging that the engine is currently working

    if(nbFrames==-1)
        nbFrames=gl_viewer->getCurrentReaderInfo()->lastFrame()+1;
    int counter = 0; // counter for nbFrames
    _aborted = false; // initialize it to false so we can detect premature abortion
    _paused = false;
    
    clearCache();
	// we compute the first frame
    
    _timer->playState=RUNNING;
    //	clearRowCache(output); // DEBUG THE CLEARROWCACHE !!!
	if(!strcmp(inputs[0]->class_name(),"Reader")){
		makeReaderInfoCurrent(static_cast<Reader*>(inputs[0]));
	}
    
    bool adjustedFrameRange=false;
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
	int first=gl_viewer->getCurrentReaderInfo()->currentFrame();
    if(first > frameSeeker->lastFrame()){
        adjustedFrameRange=true;
        first = frameSeeker->lastFrame();
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* readerInput = static_cast<Reader*>(currentInput);
				readerInput->randomFrame(first);
				readerInput->setup_for_next_frame();
			}
		}
        gl_viewer->getCurrentReaderInfo()->currentFrame(first);
    }
	int last=frameSeeker->firstFrame();
    if(first < frameSeeker->firstFrame()){
        adjustedFrameRange=true;
        for(int j=0;j<inputs.size();j++){
            first = frameSeeker->lastFrame();
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
				Reader* readerInput = static_cast<Reader*>(currentInput);
				readerInput->randomFrame(first);
				readerInput->setup_for_next_frame();
			}
		}
        gl_viewer->getCurrentReaderInfo()->currentFrame(first);
    }
    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
    
    IntegerBox dataW = gl_viewer->dataWindow();
    DisplayFormat dispW = gl_viewer->displayWindow();
    
    char* currentFrameName=0;
    if(strcmp(inputs[0]->class_name(),"Reader")==0){
        currentFrameName = static_cast<Reader*>(inputs[0])->getCurrentFrameName();
    }
    
    
    // if we didn't call setupfornextframe, call it now
    if(!adjustedFrameRange){
        for(int j=0;j<inputs.size();j++){
            InputNode* currentInput=inputs[j];
            if(strcmp(currentInput->class_name(),"Reader")==0){
                static_cast<Reader*>(currentInput)->setup_for_next_frame();
            }
        }
    }
    
    int consecutiveCachedFrames = 1;
    pair<FramesIterator,bool> iscached = _cache->isCached(currentFrameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                                          gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
    if(!iscached.second){
        int frameHint = gl_viewer->getCurrentReaderInfo()->lastFrame() - gl_viewer->getCurrentReaderInfo()->firstFrame()+1;
        computeTreeForFrame(output,inputs[0],ceil((double)frameHint/10.0),true); // NOT GOOD
        if(_aborted){
            //clearCache();
            return;
        }
        updateDisplay();
        
        
        
    }else{ // FOUND IN DISK CACHE, activating double PBO video engine
        // first we need to know how many consecutive frames are stored in cache for maximum playback effiency
        vector<pair<char*,FramesIterator> > fileNames;
        fileNames.push_back(make_pair(currentFrameName,iscached.first));
        
        if(strcmp(inputs[0]->class_name(),"Reader")==0){
            int k = gl_viewer->getCurrentReaderInfo()->currentFrame() -1;
            int c= 1;
            while(k >= last && c < nbFrames){
                char* frameName = static_cast<Reader*>(inputs[0])->getRandomFrameName(k);
                iscached=_cache->isCached(frameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                          gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
                free(frameName);
                if(iscached.second){
                    c++;
                    fileNames.push_back(make_pair(frameName,iscached.first));
                }else{
                    break;
                }
                k--;
            }
        }
        int frameCount = gl_viewer->getCurrentReaderInfo()->lastFrame() - gl_viewer->getCurrentReaderInfo()->firstFrame()+1;
        cached_video_engine(frameCount==1,gl_viewer->getCurrentReaderInfo()->currentFrame(),fileNames, true,false);
        consecutiveCachedFrames=fileNames.size();
    }
    
    
    counter++;
    counter+=(consecutiveCachedFrames-1);
    
    int i =first - ( consecutiveCachedFrames ); // we already computed the first one + we ran the cached_video_engine on a few frames
    while(i>=last && counter<nbFrames){
        bool initTexture = false;
        gl_viewer->currentFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()-1);
		for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
				inp->decrementCurrentFrameIndex();
				inp->setup_for_next_frame();
                /*checking if the dataW/dispW has changed in the reader info in which case we need to
                 make it current again*/
                IntegerBox newDataW = readersInfos[inp]->dataWindow();
                DisplayFormat newDispW = readersInfos[inp]->displayWindow();
                if(newDataW != dataW || newDispW != dispW){
                    dataW = newDataW;
                    dispW = newDispW;
                    initTexture = true;
                    makeReaderInfoCurrent(inp);
                }
			}
		}
        assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
        clearCache();
        if(strcmp(inputs[0]->class_name(),"Reader")==0){
            currentFrameName = static_cast<Reader*>(inputs[0])->getCurrentFrameName();
        }
        consecutiveCachedFrames = 1;
        iscached = _cache->isCached(currentFrameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                    gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
        if(!iscached.second){
            
            int frameHint = gl_viewer->getCurrentReaderInfo()->lastFrame() - gl_viewer->getCurrentReaderInfo()->firstFrame()+1;
            computeTreeForFrame(output,inputs[0],ceil((double)frameHint/10.0),initTexture); // NOT GOOD
            if(_aborted) return;
            _timer->waitUntilNextFrameIsDue();
            updateDisplay();
            
        }else{ // FOUND IN DISK CACHE, activating double PBO video engine
            // first we need to know how many consecutive frames are stored in cache for maximum playback effiency
            vector<pair<char*,FramesIterator> > fileNames;
            fileNames.push_back(make_pair(currentFrameName,iscached.first));
            
            if(strcmp(inputs[0]->class_name(),"Reader")==0){
                int k = gl_viewer->getCurrentReaderInfo()->currentFrame() -1;
                int c= 1;
                while(k >= last && c < nbFrames){
                    char* frameName = static_cast<Reader*>(inputs[0])->getRandomFrameName(k);
                    iscached = _cache->isCached(frameName,_treeVersion.getHashValue(),gl_viewer->currentBuiltinZoom(),
                                                gl_viewer->getExposure(),gl_viewer->lutType(),gl_viewer->_byteMode);
                    free(frameName);
                    if(iscached.second){
                        c++;
                        fileNames.push_back(make_pair(frameName,iscached.first));
                    }else{
                        break;
                    }
                    k--;
                }
            }
            cached_video_engine(false,gl_viewer->getCurrentReaderInfo()->currentFrame(),fileNames, false,false);
            consecutiveCachedFrames=fileNames.size();
        }
        
        if(_paused){
            //clearCache();
            _working = false;
            break;
        } // if user pressed pause
		if((i%24)==0){
			//cout << "fps :" << _timer->actualFrameRate() << endl;
            emit fpsChanged(_timer->actualFrameRate());
        }
        i--;
        i-=(consecutiveCachedFrames-1);
        counter++;
        counter+=(consecutiveCachedFrames-1);
    }
    //clearCache();
    //clearInfos(output);
    _timer->playState=PAUSE;
    _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
    _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
    
    _paused=true;
    _working=false;
    
    /*Run queuing tasks*/
    if(_runTasks)
        runTasks();
}

void VideoEngine::computeTreeForFrame(OutputNode *output,InputNode* input,int followingComputationsNb,bool initTexture){

    IntegerBox renderBox = gl_viewer->displayWindow(); // the display window held by the data
    
	int y=renderBox.y(); // bottom
    
    // REQUEST THE CHANNELS CONTAINED IN THE VIEWER TAB COMBOBOX IN THE GUI !!
    output->request(y,renderBox.top(),0,renderBox.range(),gl_viewer->displayChannels());
    
	
    // AT THIS POINT EVERY OPERATOR HAS ITS INFO SET!! AS WELL AS REQUESTED_BOX AND REQUESTED_CHANNELS
	
	if(gl_viewer->Ydirection()<0){ // Ydirection < 0 means we cycle from top to bottom
        y=renderBox.top()-1;
	}
	else{ // cycling from bottom to top
        y=renderBox.y();
	}
    
    //outChannels are the intersection between what the viewer requests and the ones available in the viewer node
    ChannelSet outChannels = gl_viewer->displayChannels() & output->getInfo()->channels();
    
    /*below are stuff to optimize zoom caching*/
    float zoomFactor =  closestBuiltinZoom(gl_viewer->getZoomFactor());
    gl_viewer->setCurrentBuiltInZoom(zoomFactor);
    int zoomedHeight = floor((float)gl_viewer->displayWindow().h()*zoomFactor);
    float incrementNew = builtInZooms[zoomFactor].first;
    float incrementFullsize = builtInZooms[zoomFactor].second;
    gl_viewer->setZoomIncrement(make_pair(incrementNew,incrementFullsize));
    
    if(initTexture)
        gl_viewer->initTextures();

#ifdef __POWITER_WIN32__
	gl_viewer->doneCurrent(); // windows only, openGL needs this to deal with concurrency
#endif

    // selecting the right anchor of the row
    int range = 0;
        gl_viewer->dataWindow().range() > gl_viewer->displayWindow().range() ?
        range = gl_viewer->dataWindow().range() : range = gl_viewer->displayWindow().range();
    
    // selection the left anchor of the row
	int offset=0;
    	gl_viewer->dataWindow().x() < gl_viewer->displayWindow().x() ?
        offset = gl_viewer->dataWindow().x() : offset = gl_viewer->displayWindow().x();
    
    //starting viewer preprocess : i.e initialize the cached frame
    gl_viewer->preProcess(followingComputationsNb);
    
	if(gl_viewer->Ydirection()<0){
        int rowY = zoomedHeight -1;
		while (y>=renderBox.y()){
            int k =y;
            while(k > (y -incrementNew) && rowY>=0 && k>=(renderBox.y())){
                if(_aborted){
                    _abort();
                }
                
                bool cached = false;
                Row* row ;
                map<int,Row*>::iterator foundCached = contained_in_cache(k);
                if(foundCached!=row_cache.end()){
                    row= foundCached->second;
                    cached = true;
                }else{
                    row=new Row(offset,k,range,outChannels);
                    addToRowCache(row);
                }
                row->zoomedY(rowY);
                WorkerThread* task;
                if((k==renderBox.y() || rowY==0) && !strcmp(output->class_name(),"Viewer")){
                    task =new WorkerThread(row,input,output,LAST,cached);
                }else if(k==(renderBox.top()-1) && !strcmp(output->class_name(),"Viewer")){
                    task =new WorkerThread(row,input,output,FIRST,cached);
                }else{
                    task =new WorkerThread(row,input,output,ND,cached);
                }
                rowY--;
                k--;
                pool->start(task);
                
            }
            y-=incrementFullsize;
        }
        
    }else{
        int rowY = renderBox.y(); // zoomed index
        while(y<renderBox.top()){ // y = full size index
            for(int k = y; k<incrementNew+y;k++){
                bool cached = false;
                Row* row ;
                if(_aborted){
                    _abort();
                }
                map<int,Row*>::iterator foundCached = contained_in_cache(k);
                if(foundCached!=row_cache.end()){
                    row= foundCached->second;
                    cached = true;
                }else{
                    row=new Row(offset,k,range,outChannels);
                    addToRowCache(row);
                }
                row->zoomedY(rowY);
                WorkerThread* task;
                if(k==(renderBox.top()-1) && !strcmp(output->class_name(),"Viewer")){
                    task =new WorkerThread(row,input,output,LAST,cached);
                }else if(k==renderBox.y() && !strcmp(output->class_name(),"Viewer")){
                    task =new WorkerThread(row,input,output,FIRST,cached);
                }
                else{
                    task =new WorkerThread(row,input,output,ND,cached);
                }
                pool->start(task);
                
                rowY++;
            }
            y+=incrementFullsize;
        }
        
        
    }
    pool->waitForDone();
    
    //finalize caching
    gl_viewer->postProcess();
        
    QCoreApplication::processEvents();
}
void VideoEngine::drawOverlay(){
    if(output)
        _drawOverlay(output);
}

void VideoEngine::_drawOverlay(Node *output){
    output->drawOverlay();
    foreach(Node* n,output->getParents()){
        _drawOverlay(n);
    }
    
}

void VideoEngine::computeFrameRequest(){
    video_sequence_engine(output, inputs, 1,false,true);
}

void VideoEngine::updateProgressBar(){
    //update progress bar
}
void VideoEngine::updateDisplay(){
    gl_viewer->resizeEvent(new QResizeEvent(gl_viewer->size(),gl_viewer->size()));
    gl_viewer->updateGL();
}

void VideoEngine::startEngineWithOption(int nbFrames,bool initViewer){
    video_sequence_engine(output, inputs,nbFrames,initViewer);
}
void VideoEngine::startEngine(int nbFrames){
    video_sequence_engine(output, inputs,nbFrames,true, nbFrames == 1);
}

VideoEngine::VideoEngine(ViewerGL *gl_viewer,Model* engine,QMutex* lock):
QObject(engine),_working(false),_aborted(false),_paused(true),_readerInfoHasChanged(false){
    this->_coreEngine = engine;
    this->_lock= lock;
    this->gl_viewer = gl_viewer;
    gl_viewer->setVideoEngine(this);
    _timer=new Timer();
    pool = new QThreadPool(this);
    (QThread::idealThreadCount()) > 1 ? pool->setMaxThreadCount((QThread::idealThreadCount())) : pool->setMaxThreadCount(1);
    fillBuilInZoomsLut();
    output = 0;
    foreach(InputNode* i,inputs){
        i = 0;
    }
    FeedBackSpinBox* fpsBox = engine->getControler()->getGui()->viewer_tab->fpsBox;
    TimeSlider* frameSeeker = engine->getControler()->getGui()->viewer_tab->frameSeeker;
    FeedBackSpinBox* frameNumber = engine->getControler()->getGui()->viewer_tab->frameNumberBox;
    QObject::connect(fpsBox, SIGNAL(valueChanged(double)),this, SLOT(setDesiredFPS(double)));
    QObject::connect(this, SIGNAL(fpsChanged(double)), fpsBox, SLOT(setValue(double)));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->play_Forward_Button,SIGNAL(toggled(bool)),this,SLOT(startPause(bool)));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->stop_Button,SIGNAL(clicked()),this,SLOT(pause()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->play_Backward_Button,SIGNAL(toggled(bool)),this,SLOT(startBackward(bool)));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->previousFrame_Button,SIGNAL(clicked()),this,SLOT(previousFrame()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->nextFrame_Button,SIGNAL(clicked()),this,SLOT(nextFrame()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->previousIncrement_Button,SIGNAL(clicked()),this,SLOT(previousIncrement()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->nextIncrement_Button,SIGNAL(clicked()),this,SLOT(nextIncrement()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->firstFrame_Button,SIGNAL(clicked()),this,SLOT(firstFrame()));
    QObject::connect(engine->getControler()->getGui()->viewer_tab->lastFrame_Button,SIGNAL(clicked()),this,SLOT(lastFrame()));
    QObject::connect(frameNumber,SIGNAL(valueChanged(double)),this,SLOT(seekRandomFrame(double)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), this, SLOT(seekRandomFrame(int)));
    QObject::connect(frameSeeker,SIGNAL(positionChanged(int)), frameNumber, SLOT(setValue(int)));
    QObject::connect(frameNumber, SIGNAL(valueChanged(double)), frameSeeker, SLOT(seek(double)));
    qint64 maxDiskCacheSize = engine->getCurrentProject()->maxDiskCache;
    qint64 maxMemoryCacheSize = (double)getSystemTotalRAM() * engine->getCurrentProject()->maxCacheMemoryPercent;
    _cache = new DiskCache(gl_viewer,maxDiskCacheSize, maxMemoryCacheSize);
    QObject::connect(engine->getControler()->getGui()->actionClearDiskCache, SIGNAL(triggered()),this,SLOT(clearDiskCache()));
    QObject::connect(engine->getControler()->getGui()->actionClearPlayBackCache, SIGNAL(triggered()),this,SLOT(clearPlayBackCache()));
    QObject::connect(engine->getControler()->getGui()->actionClearBufferCache, SIGNAL(triggered()),this,SLOT(clearCache()));
    
}

void VideoEngine::clearDiskCache(){
    _cache->clearCache();
}

VideoEngine::~VideoEngine(){
    clearCache();
    delete _cache;
    delete pool;
    delete _timer;
}

void VideoEngine::fillBuilInZoomsLut(){
    builtInZooms[1.f/10.f] = make_pair(1,10);
    builtInZooms[1.f/4.f] = make_pair(1,4);
    builtInZooms[1.f/2.f] = make_pair(1,2);
    builtInZooms[3.f/4.f] = make_pair(3,4);
    builtInZooms[9.f/10.f]= make_pair(9,10);
    builtInZooms[1.f] = make_pair(1,1);
}
float VideoEngine::closestBuiltinZoom(float v){
    std::map<float, pair<int,int> >::iterator it = builtInZooms.begin();
    std::map<float, pair<int,int> >::iterator suiv = it;
    ++suiv;
    if( v < it->first)
        return it->first;
    for(;it!=builtInZooms.end();it++){
        if(it->first == v)
            return it->first;
        else if(suiv!=builtInZooms.end() && it->first <v && suiv->first >= v)
            return suiv->first;
        else if(suiv==builtInZooms.end() && it->first < v  )
            return 1.f;
        suiv++;
    }
    return -1.f;
}
float VideoEngine::inferiorBuiltinZoom(float v){
    map<float, pair<int,int> >::iterator it=builtInZooms.begin(); it++;
    map<float, pair<int,int> >::iterator prec =builtInZooms.begin();
    for(;it!=builtInZooms.end();it++){
        float value = it->first;
        if( value == v &&  value != 1.f/20.f)
            return prec->first;
        else if(value==v && value==1.f/20.f)
            return value;
        prec++;
    }
    return -1.f;
}
float VideoEngine::superiorBuiltinZoom(float v){
    map<float, pair<int,int> >::iterator suiv=builtInZooms.begin(); suiv++;
    map<float, pair<int,int> >::iterator it =builtInZooms.begin();
    for(;it!=builtInZooms.end();it++){
        float value = it->first;
        if( value == v &&  value != 19.f/20.f)
            return suiv->first;
        else if(value==v && value==19.f/20.f)
            return 1.f;
        suiv++;
    }
    return -1.f;
}

void VideoEngine::addToRowCache(Row* row){
    QMutexLocker lock(_lock);
	row_cache[row->y()]=row;
}

std::map<int,Row*>::iterator VideoEngine::contained_in_cache(int rowIndex){
    QMutexLocker lock(_lock);
	return row_cache.find(rowIndex);
}

void VideoEngine::clearInfos(Node* out){
    out->clear_info();
    foreach(Node* c,out->getParents()){
        clearInfos(c);
    }
}

void VideoEngine::pushReaderInfo(ReaderInfo* info,Reader* reader){
    std::map<Reader*, ReaderInfo* >::iterator it = readersInfos.find(reader);
    if(it!=readersInfos.end()){
        if(*(it->second) == *info){
            it->second->currentFrame(info->currentFrame());
            it->second->currentFrameName(info->currentFrameName());
            delete info;
            return;
        }
        it->second->copy(info);
//        it->second = info;
    }else{
        readersInfos.insert(make_pair(reader ,info));
    }
    _readerInfoHasChanged = true;
}
void VideoEngine::popReaderInfo(Reader* reader){
    std::map<Reader*, ReaderInfo* >::iterator it = readersInfos.find(reader);
    if(it!=readersInfos.end()){
        delete it->second;
        readersInfos.erase(it);
    }
}
void VideoEngine::makeReaderInfoCurrent(Reader* reader){
    std::map<Reader*, ReaderInfo* >::iterator it = readersInfos.find(reader);
    if(it != readersInfos.end() && it->second)
    {
        gl_viewer->setCurrentReaderInfo(it->second,false,_readerInfoHasChanged);
        _readerInfoHasChanged = false;
    }
    
}
void VideoEngine::setDesiredFPS(double d){
    _timer->setDesiredFrameRate(d);
}

void VideoEngine::_abort(){
    pool->waitForDone(); // stopping worker threads
    clearCache(); // clearing row cache
    QCoreApplication::processEvents(); // load events that may have been ignored
}
void VideoEngine::abort(){
    _aborted=true;
    _timer->playState=PAUSE; // pausing timer
    _working=false; // engine idling
}
void VideoEngine::pause(){
    _paused=true;
    clearCache();
    _timer->playState=PAUSE; // pausing timer
    _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
    _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
}
void VideoEngine::startPause(bool c){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
        return;
    }
    
    
    if(c && output && inputs.size()>0){
        startEngineWithOption(-1,false);
    }
    else if(!output || inputs.size()==0){
        _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
        _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::startBackward(bool c){
    
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()){
        pause();
        return;
    }
    if(c && output && inputs.size()>0){
        backward_video_sequence_engine(output, inputs, -1);
    }else if(!output || inputs.size()==0){
        _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->setChecked(false);
        _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::previousFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
        while(_working){};
    }
    if(!_working)
        _previousFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()-1, 1, false);
    else
        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()-1, 1, false,&VideoEngine::_previousFrame);
}

void VideoEngine::nextFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _nextFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false);
    else
        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false,&VideoEngine::_nextFrame);
}

void VideoEngine::firstFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
    if(!_working)
        _firstFrame(frameSeeker->firstFrame(), 1, false);
    else
        appendTask(frameSeeker->firstFrame(), 1, false, &VideoEngine::_firstFrame);
}

void VideoEngine::lastFrame(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
    if(!_working)
        _lastFrame(frameSeeker->lastFrame(), 1, false);
    else
        appendTask(frameSeeker->lastFrame(), 1, false, &VideoEngine::_lastFrame);
}

void VideoEngine::previousIncrement(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = gl_viewer->getCurrentReaderInfo()->currentFrame()-_coreEngine->getControler()->getGui()->viewer_tab->incrementSpinBox->value();
    if(!_working){
        _previousIncrement(frame, 1, false);
    }else{
        appendTask(frame,1, false, &VideoEngine::_previousIncrement);
    }
        
    
}

void VideoEngine::nextIncrement(){
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = gl_viewer->getCurrentReaderInfo()->currentFrame()+_coreEngine->getControler()->getGui()->viewer_tab->incrementSpinBox->value();
    if(!_working)
        _nextIncrement(frame, 1, false);
    else
        appendTask(frame,1, false, &VideoEngine::_nextIncrement);
}

void VideoEngine::seekRandomFrame(int f){
    if(!output || inputs.size()==0) return;
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _seekRandomFrame(f, 1, false);
    else
        appendTask(f, 1, false, &VideoEngine::_seekRandomFrame);
}

void VideoEngine::_previousFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB >= frameSeeker->firstFrame()){
            gl_viewer->currentFrame(frameNB);
            for(int j=0;j<inputs.size();j++){
                InputNode* currentInput=inputs[j];
                if(strcmp(currentInput->class_name(),"Reader")==0){
                    Reader* inp = static_cast<Reader*>(currentInput);
                    int f =inp->currentFrame();
                    inp->decrementCurrentFrameIndex();
                    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                    if(inp->currentFrame()!=f){ // if it has successfully decremented, i.e: we weren't on the first frame
                        static_cast<Reader*>(currentInput)->setup_for_next_frame();
                    }
                }
            }
            backward_video_sequence_engine(output, inputs, frameCount,false);
        }
        
    }

}
void VideoEngine::_nextFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB <= frameSeeker->lastFrame()){
            gl_viewer->currentFrame(frameNB);
            for(int j=0;j<inputs.size();j++){
                InputNode* currentInput=inputs[j];
                if(strcmp(currentInput->class_name(),"Reader")==0){
                    Reader* inp = static_cast<Reader*>(currentInput);
                    int f =inp->currentFrame();
                    inp->incrementCurrentFrameIndex();
                    assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                    if(inp->currentFrame()!=f){ // if it has successfully incremented, i.e: we weren't on the last frame
                        static_cast<Reader*>(currentInput)->setup_for_next_frame();
                    }
                }
            }
            video_sequence_engine(output, inputs, frameCount,initViewer,false,false);
        }
    }

}
void VideoEngine::_previousIncrement(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB > frameSeeker->firstFrame())
            gl_viewer->currentFrame(frameNB);
        else
            gl_viewer->currentFrame(frameSeeker->firstFrame());
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                if(frameNB < frameSeeker->firstFrame()){
                    inp->randomFrame(frameNB);
                }else{
                    inp->randomFrame(frameSeeker->firstFrame());
                }
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                inp->setup_for_next_frame();
			}
		}
        backward_video_sequence_engine(output, inputs, frameCount,false);
    }

}
void VideoEngine::_nextIncrement(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB < frameSeeker->lastFrame())
            gl_viewer->currentFrame(frameNB);
        else
            gl_viewer->currentFrame(frameSeeker->lastFrame());
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                if(frameNB < frameSeeker->lastFrame()){
                    inp->randomFrame(frameNB);
                }else{
                    inp->randomFrame(frameSeeker->lastFrame());
                }
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                inp->setup_for_next_frame();
			}
		}
        video_sequence_engine(output, inputs, frameCount,initViewer,false,false);
    }

}
void VideoEngine::_firstFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        gl_viewer->currentFrame(frameNB);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                static_cast<Reader*>(currentInput)->setup_for_next_frame();
			}
		}
        backward_video_sequence_engine(output, inputs, frameCount,false);
    }

}
void VideoEngine::_lastFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        gl_viewer->currentFrame(frameNB);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
				inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                static_cast<Reader*>(currentInput)->setup_for_next_frame();
                
			}
		}
        video_sequence_engine(output, inputs, frameCount,initViewer,false,false);
    }

}
void VideoEngine::_seekRandomFrame(int frameNB,int frameCount,bool initViewer){
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(frameNB < frameSeeker->firstFrame() || frameNB > frameSeeker->lastFrame())
            return;
        gl_viewer->currentFrame(frameNB);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->randomFrame(frameNB);
                assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
                inp->setup_for_next_frame();
            }
		}
        video_sequence_engine(output, inputs, frameCount,initViewer,false,false);
        
    }

}
void VideoEngine::appendTask(int frameNB, int frameCount, bool initViewer, VengineFunction func){
    _waitingTasks.push_back(Task(frameNB,frameCount,initViewer,func));
}

void VideoEngine::runTasks(){
    for(unsigned int i=0; i < _waitingTasks.size(); i++){
        Task _t = _waitingTasks[i];
        VengineFunction f = _t._func;
        VideoEngine *vengine = this;
        (*vengine.*f)(_t._newFrameNB,_t._frameCount,_t._initViewer);
    }
    _waitingTasks.clear();
}


/*code needs to be reviewed*/
void VideoEngine::seekRandomFrameWithStart(int f){
    if(!output || inputs.size()==0) return;
    if( _coreEngine->getControler()->getGui()->viewer_tab->play_Forward_Button->isChecked()
       ||  _coreEngine->getControler()->getGui()->viewer_tab->play_Backward_Button->isChecked()){
        pause();
    }
    if(output && inputs.size()>0){
        TimeSlider* frameSeeker = _coreEngine->getControler()->getGui()->viewer_tab->frameSeeker;
        if(f < frameSeeker->firstFrame() || f > frameSeeker->lastFrame())
            return;
        gl_viewer->currentFrame(f);
        for(int j=0;j<inputs.size();j++){
			InputNode* currentInput=inputs[j];
			if(strcmp(currentInput->class_name(),"Reader")==0){
                Reader* inp = static_cast<Reader*>(currentInput);
                int o =inp->currentFrame();
                if(o < f){
                    while(o <f){
                        inp->incrementCurrentFrameIndex();
                        o++;
                    }
                }else{
                    while(o >f){
                        inp->decrementCurrentFrameIndex();
                        o--;
                    }
                }
                static_cast<Reader*>(currentInput)->setup_for_next_frame();
                
            }
		}
        video_sequence_engine(output, inputs, -1,false);
    }
}

void VideoEngine::debugTree(){
    int nb=0;
    _debugTree(output,&nb);
    cout << "The tree contains " << nb << " nodes. " << endl;
}
void VideoEngine::_debugTree(Node* n,int* nb){
    *nb = *nb+1;
    cout << *n << endl;
    foreach(Node* c,n->getParents()){
        _debugTree(c,nb);
    }
}
void VideoEngine::computeTreeHash(std::vector< std::pair<char*,U32> > &alreadyComputed, Node *n){
    for(int i =0; i < alreadyComputed.size();i++){
        if(!strcmp(alreadyComputed[i].first,n->getName()))
            return;
    }
    std::vector<char*> v;
    n->computeTreeHash(v);
    U32 hashVal = n->getHash()->getHashValue();
    char* name = QstringCpy(n->getName());
    alreadyComputed.push_back(make_pair(name,hashVal));
    foreach(Node* parent,n->getParents()){
        computeTreeHash(alreadyComputed, parent);
    }
    
    
}

void VideoEngine::changeTreeVersion(){
    std::vector< std::pair<char*,U32> > nodeHashs;
    _treeVersion.reset();
    if(!output){
        return;
    }
    computeTreeHash(nodeHashs, output);
    for(int i =0 ;i < nodeHashs.size();i++){
        _treeVersion.appendNodeHashToHash(nodeHashs[i].second);
        //        free(nodeHashs[i].first);
    }
    _treeVersion.computeHash();
    
}

char* VideoEngine::currentFrameName(){
    if(strcmp(inputs[0]->class_name(),"Reader")==0){
        return static_cast<Reader*>(inputs[0])->getCurrentFrameName();
    }
    return 0;
}

void VideoEngine::cached_video_engine(bool oneFrame,int startNb,std::vector<std::pair<char *,FramesIterator> > fileNames,bool firstFrame,bool forward){
    int index =0;
    int nextIndex = 0;
    bool first_time = true;
    gl_viewer->_drawing= true;
    //cout << "cached engine called for " << fileNames.size() << " frame(s)." << endl;
    if(firstFrame){
        float zoomFactor =  closestBuiltinZoom(gl_viewer->getZoomFactor());
        gl_viewer->setCurrentBuiltInZoom(zoomFactor);
        gl_viewer->initTextures();
    }
    std::pair<int,int> frame_size ;
    if(oneFrame){
        frame_size =  _cache->retrieveFrame(0,fileNames[0].second, index);
        gl_viewer->doEmitFrameChanged(0);
    }else{
        frame_size =  _cache->retrieveFrame(startNb,fileNames[0].second, index);
        gl_viewer->doEmitFrameChanged(startNb);
    }
    
    if(fileNames.size() == 1){ // No asynchroneous data upload CPU/GPU
        glBindTexture(GL_TEXTURE_2D,gl_viewer->texId[0]);
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, gl_viewer->texBuffer[index]);
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        frame_size.first,
                        frame_size.second,
                        GL_RGBA,
                        GL_FLOAT,
                        0);
        glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
        _timer->waitUntilNextFrameIsDue();
        updateDisplay();
        QCoreApplication::processEvents();
        if(_aborted) return;
//        if(_paused){
//            //clearCache();
//            
//        } // if user pressed pause
    }else{ // dual PBO mode, fast asynchroneous upload
        int i =0;
        while( i < fileNames.size()){
            if(!first_time){
                index = (index + 1) % 2;
                nextIndex = (index + 1) % 2;
            }else{
                first_time = false;
                nextIndex=1;
                glBindTexture(GL_TEXTURE_2D,gl_viewer->texId[0]);
            }
            
            
            
            glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, gl_viewer->texBuffer[index]);
            glBindTexture(GL_TEXTURE_2D,gl_viewer->texId[0]);
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            frame_size.first,
                            frame_size.second,
                            GL_RGBA,
                            GL_FLOAT,
                            0);
            std::pair<int,int> newFrameSize = _cache->retrieveFrame(startNb,fileNames[i].second, nextIndex);
            if(newFrameSize.first != frame_size.first || newFrameSize.second != frame_size.second){
                float zoomFactor =  closestBuiltinZoom(gl_viewer->getZoomFactor());
                gl_viewer->setCurrentBuiltInZoom(zoomFactor);
                gl_viewer->initTextures();
            }
            frame_size =newFrameSize;
            gl_viewer->doEmitFrameChanged(startNb);
            if(forward)
                startNb++;
            else
                startNb--;
            
            glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
            _timer->waitUntilNextFrameIsDue();
            updateDisplay();
            QCoreApplication::processEvents();
            if((i%24)==0){
                emit fpsChanged(_timer->actualFrameRate());
            }
            if(_aborted) return;
            if(_paused){
                break;
            }
            i++;
        }
        if(forward){
            gl_viewer->currentFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()+i-1);
            for(int j=0;j<inputs.size();j++){
                InputNode* currentInput=inputs[j];
                if(strcmp(currentInput->class_name(),"Reader")==0){
                    Reader* inp = static_cast<Reader*>(currentInput);
                    inp->randomFrame(inp->currentFrame()+i-1);
                    inp->setup_for_next_frame();
                }
            }
            
        }else{
            gl_viewer->currentFrame(gl_viewer->getCurrentReaderInfo()->currentFrame()-i+1);
            for(int j=0;j<inputs.size();j++){
                InputNode* currentInput=inputs[j];
                if(strcmp(currentInput->class_name(),"Reader")==0){
                    Reader* inp = static_cast<Reader*>(currentInput);
                    inp->randomFrame(inp->currentFrame()-i+1);
                    inp->setup_for_next_frame();
                    
                }
            }
            
        }
        assert(gl_viewer->getCurrentReaderInfo()->currentFrame() == static_cast<Reader*>(inputs[0])->currentFrame());
    }
    //cout << "end cached engine" << endl;
    
}


std::pair<char*,FrameID> VideoEngine::mapNewFrame(int frameNb,char* filename,int width,int height,int nbFrameHint){
    return _cache->mapNewFrame(frameNb,filename, width, height, nbFrameHint,_treeVersion.getHashValue());}

void VideoEngine::closeMappedFile(){_cache->closeMappedFile();}

void VideoEngine::clearPlayBackCache(){_cache->clearPlayBackCache();}
