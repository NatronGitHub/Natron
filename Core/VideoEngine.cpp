
//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include <QtCore/QMutex>
#include <QtCore/qcoreapplication.h>
#include <QtWidgets/QPushButton>
#include <QtGui/QVector2D>
#include <QtWidgets/QAction>
#include <iterator>
#include <cassert>
#include "Core/VideoEngine.h"
#include "Core/inputnode.h"
#include "Core/outputnode.h"
#include "Core/viewerNode.h"
#include "Core/settings.h"
#include "Core/model.h"
#include "Core/hash.h"
#include "Core/Timer.h"
#include "Core/lookUpTables.h"
#include "Core/viewercache.h"


#include "Gui/mainGui.h"
#include "Gui/viewerTab.h"
#include "Gui/timeline.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/GLViewer.h"
#include "Gui/texturecache.h"

#include "Superviser/controler.h"
#include "Superviser/MemoryInfo.h"



#define gl_viewer currentViewer->viewer

/* Here's a drawing that represents how the video Engine works:
 
 [thread pool]                       [OtherThread]
 [main thread]                              |                                   |
 -videoEngine(...)                          |                                   |
 >-computeFrameRequest(...)                 |                                   |
 |    if(!cached)                           |                                   |
 |       -computeTreeForFrame------------>start worker threads                  |
 |              |                           |                                   |
 |              |             <--notify----finished()                           |
 |       -allocatePBO                                                           |
 |       -fillPBO -------------QtConcurrent::Run-------------------------> copy frame data to PBO...
 |           |                                                                  |
 |       -engineLoop <-----------------------------------------------notify---finished()
 |           |
 |           |
 ------------...
 |     else
 |        -cachedFrameEngine(...)
 |          -retrieveFrame(...)
 |          -allocatePBO
 |          -fillPBO -------------QtConcurrent::Run-------------------------> copy frame data to PBO...
 |              |                                                               |
 ----------engineLoop <---------------------------------------------notify---finished()
 
 
 
 */

void VideoEngine::videoEngine(bool outputIsViewer,int frameCount,bool fitFrameToViewer,bool forward,bool sameFrame){
    if (_working || !_enginePostProcessResults->isFinished()) {
        return;
    }
    _timer->playState=RUNNING;
    _frameRequestsCount = frameCount;
    _frameRequestIndex = 0;
    _forward = forward;
    _paused = false;
    _aborted = false;
    _outputIsViewer = outputIsViewer;
    computeFrameRequest(sameFrame,forward,fitFrameToViewer,false);
}
void VideoEngine::stopEngine(){
    if(_outputIsViewer){
        currentViewer->play_Forward_Button->setChecked(false);
        currentViewer->play_Backward_Button->setChecked(false);
    }
    _frameRequestsCount = 0;
    _working = false;
    _aborted = false;
    _paused = false;
    resetReadingBuffers();
    _enginePostProcessResults->waitForFinished();
    _workerThreadsResults->waitForFinished();
    _timer->playState=PAUSE;
    
}
void VideoEngine::resetReadingBuffers(){
    if(_outputIsViewer){
        GLint boundBuffer;
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &boundBuffer);
        if(boundBuffer != 0){
            glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
            glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,0);
        }
    }
    std::vector<InputNode*>& inputs = _dag.getInputs();
    for(int j=0;j<inputs.size();j++){
        InputNode* currentInput=inputs[j];
        if(currentInput->className() == string("Reader")){
            static_cast<Reader*>(currentInput)->removeCachedFramesFromBuffer();
        }
    }
    
}

void VideoEngine::computeFrameRequest(bool sameFrame,bool forward,bool fitFrameToViewer,bool recursiveCall){
    _working = true;
    _sameFrame = sameFrame;
    /*check whether we need to stop the engine*/
    if(!sameFrame && _aborted){
        /*aborted by the user*/
        _waitingTasks.clear();
        stopEngine();
        return;
    }else if(_paused || _frameRequestsCount==0){
        /*paused or frame request count ended.*/
        stopEngine();
        runTasks();
        stopEngine();
        return;
    }else if(recursiveCall && _dag.lastFrame() == _dag.firstFrame() && _frameRequestsCount == -1 && _frameRequestIndex == 1){
        /*1 frame is the sequence and we already compute it*/
        stopEngine();
        runTasks();
        stopEngine();
        return;
    }
    
    /*Determine what is the current frame*/
    int firstFrame = INT_MAX,lastFrame = INT_MIN, currentFrame = 0;
    TimeSlider* frameSeeker = 0;
    if(_outputIsViewer){
        frameSeeker = currentViewer->frameSeeker;
    }
    /*autovalidate will set the frame range correctly for all nodes,but not other infos 
     like channels, bbox etc...since we didn't read files already from Readers.*/
    if(!recursiveCall){
        _outputIsViewer ? currentFrame = frameSeeker->currentFrame() : currentFrame = 0; // FIX THIS FOR WRITERS
        if(!sameFrame){
            /*harmonize node infos across the DAG
             Must be called always after startReading has been called
             since it relies on InputNodes Infos.*/
            _dag.getOutput()->validate(false);
            firstFrame = _dag.firstFrame();
            lastFrame = _dag.lastFrame();
            
            /*clamping the current frame to the range [first,last] if it wasn't*/
            if(currentFrame < firstFrame){
                currentFrame = firstFrame;
            }
            else if(currentFrame > lastFrame){
                currentFrame = lastFrame;
            }
            if (_outputIsViewer) {
                frameSeeker->seek(currentFrame);
            }
        }
        
    }else{ // if the call is recursive, i.e: the next frame in the sequence
        lastFrame = _dag.lastFrame();
        firstFrame = _dag.firstFrame();
        if(_forward){
            currentFrame = currentViewer->frameSeeker->currentFrame()+1;
            if(currentFrame > lastFrame){
                if(_loopMode && _outputIsViewer)
                    currentFrame = firstFrame;
                else{
                    _frameRequestsCount = 0;
                    return;
                }
            }
        }else{
            currentFrame = currentFrame = currentViewer->frameSeeker->currentFrame()-1;
            if(currentFrame < firstFrame){
                if(_loopMode && _outputIsViewer)
                    currentFrame = lastFrame;
                else{
                    _frameRequestsCount = 0;
                    return;
                }
            }
        }
        frameSeeker->seek(currentFrame);
    }
    
    std::vector<Reader*> readers;
    std::vector<InputNode*>& inputs = _dag.getInputs();
    for(int j=0;j<inputs.size();j++){
        InputNode* currentInput=inputs[j];
        if(currentInput->className() == string("Reader")){
            Reader* inp = static_cast<Reader*>(currentInput);
            inp->fitFrameToViewer(fitFrameToViewer);
            readers.push_back(inp);
        }
    }
    
    /*1 slot in the vector corresponds to 1 frame read by a reader. The second member indicates whether the frame
     was in cache or not.*/
    FramesVector readFrames = startReading(readers, true , true);
    
    _dag.getOutput()->validate(true);// < validating infos

    
    for(U32 i = 0; i < readFrames.size();i++){
        ReadFrame readFrame = readFrames[i];
        if (readFrame.second != _viewerCache->end()) {
            cachedFrameEngine(readFrame.second);
            if(_paused){
                stopEngine();
                runTasks();
                stopEngine();
                return;
            }
            if(_aborted){
                _waitingTasks.clear();
                stopEngine();
                return;
            }
            engineLoop();
        }else{
            int frameCountHint = 0;
            if(_outputIsViewer){
                if(_frameRequestsCount==-1){
                    if(_loopMode){
                        frameCountHint = currentViewer->frameSeeker->lastFrame()-currentViewer->frameSeeker->firstFrame()+1;
                    }else{
                        frameCountHint = currentViewer->frameSeeker->lastFrame()-currentViewer->frameSeeker->currentFrame();
                    }
                }else{
                    frameCountHint = _frameRequestsCount+_frameRequestIndex;
                }
            }
            computeTreeForFrame(readFrame.first._filename,_dag.getOutput(),fitFrameToViewer,ceil((double)frameCountHint/10.0));
        }
    }
}
void VideoEngine::finishComputeFrameRequest(){
    _sequenceToWork.clear();
    
    *_enginePostProcessResults = QtConcurrent::run(gl_viewer,
                                                   &ViewerGL::fillPBO,_gpuTransferInfo.src,_gpuTransferInfo.dst,_gpuTransferInfo.byteCount);
    _engineLoopWatcher->setFuture(*_enginePostProcessResults);
}

void VideoEngine::cachedFrameEngine(FrameEntry* frame){
    int w = frame->second._actualW ;
    int h = frame->second._actualH ;
    size_t dataSize = 0;
    if(frame->second._byteMode==1.0){
        dataSize  = w * h * sizeof(U32) ;
    }else{
        dataSize  = w * h  * sizeof(float) * 4;
    }
    /*resizing texture if needed, the calls must be made in that order*/
    gl_viewer->initTextureBGRA(w,h,gl_viewer->getDefaultTextureID());
    gl_viewer->setCurrentTexture(gl_viewer->getDefaultTextureID());
    gl_viewer->drawing(true);
    QCoreApplication::processEvents();
    
}

void VideoEngine::engineLoop(){
    if(_frameRequestIndex == 0 && _frameRequestsCount == 1 && !_sameFrame){
        _frameRequestsCount = 0;
    }else if(_frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
        _frameRequestsCount--;
    }
    
    _frameRequestIndex++;
    
    if(_outputIsViewer){
        std::pair<int,int> texSize = gl_viewer->getTextureSize();
        gl_viewer->copyPBOtoTexture(texSize.first, texSize.second); // fill texture, returns instantly
    }
    if(_frameRequestsCount!=0 && !_paused){
        std::vector<Reader*> readers;
        std::vector<InputNode*>& inputs = _dag.getInputs();
        for(int j=0;j<inputs.size();j++){
            InputNode* currentInput=inputs[j];
            if(currentInput->className() == string("Reader")){
                Reader* inp =static_cast<Reader*>(currentInput);
                inp->fitFrameToViewer(false);
                readers.push_back(inp);
            }
        }
        startReading(readers , false , true);
    }
    if(_outputIsViewer){
        _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
        if((_frameRequestIndex%24)==0){
            emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
        }
        
        updateDisplay(); // updating viewer & pixel aspect ratio if needed
    }
    computeFrameRequest(false, _forward ,false,true); // recursive call for following frame.
    
}


void VideoEngine::computeTreeForFrame(std::string filename,OutputNode *output,bool fitFrameToViewer,int followingComputationsNb){
    if(fitFrameToViewer && currentViewer && _outputIsViewer){
        gl_viewer->fitToFormat(gl_viewer->displayWindow());
    }
    output->request(gl_viewer->displayChannels());
    const Format &_dispW = output->getInfo()->getDisplayWindow();
    //const Box2D &_dataW = output->getInfo()->getDataWindow();
    // AT THIS POINT EVERY OPERATOR HAS ITS INFO SET!! AS WELL AS REQUESTED_BOX AND REQUESTED_CHANNELS
    
    //outChannels are the intersection between what the viewer requests and the ones available in the viewer node
    // in case of a writer, it's just the output channels
    ChannelSet outChannels;
    if(_outputIsViewer)
        outChannels  = output->getRequestedChannels() & output->getInfo()->channels();
    else
        outChannels = output->getInfo()->channels();
    
    float zoomFactor = gl_viewer->getZoomFactor();
    std::map<int,int> rows = gl_viewer->computeRowSpan(_dispW, zoomFactor);
    
    // selecting the right anchor of the row
    int right = _dispW.right();
  //  _dataW.right() > _dispW.right() ? right = _dataW.right() : right = _dispW.right();
    
    // selecting the left anchor of the row
	int offset= _dispW.x();
   // _dataW.x() < _dispW.x() ? offset = _dataW.x() : offset = _dispW.x();
    map<int,int>::iterator it = rows.begin();
    map<int,int>::iterator last = rows.end();
    ViewerGL::CACHING_MODE mode = ViewerGL::TEXTURE_CACHE;
    if(rows.size() > 0){
        last--;
        int firstRow = it->first;
        int lastRow = last->first;
        gl_viewer->setRowSpan(make_pair(firstRow, lastRow));
        if(rows.size() >= 2){
            it++;
            int gap = it->first - firstRow; // gap between first and second rows
            if (firstRow <= _dispW.y()+gap && lastRow >= _dispW.h()-1-gap) {
                mode = ViewerGL::VIEWER_CACHE;
            }
            it--; // setting back the iterator to begin
            
        }
    }else{
        gl_viewer->setRowSpan(make_pair(_dispW.y(), _dispW.h()-1));
    }
    int w = zoomFactor <= 1.f ? _dispW.w() * zoomFactor : _dispW.w();
    int h = rows.size();
    //starting viewer pre-process : i.e initialize the cached frame
    bool isTextureCached = gl_viewer->handleTextureAndViewerCache(filename,followingComputationsNb,w,h,mode);
    /*if a texture was found in cache, notify the viewer and skip immediately to the loop*/
    if(isTextureCached){
        QCoreApplication::processEvents();
        engineLoop();
        return;
    }
    int counter = 0;
    for(; it!=rows.end() ; it++){
        if(_aborted){
            _waitingTasks.clear();
            stopEngine();
            return;
        }else if(_paused){
            _workerThreadsWatcher->cancel();
            stopEngine();
            runTasks();
            stopEngine();
            return;
        }
        Row* row = 0;
        int y = it->first;
        row=new Row(offset,y,right,outChannels);
        row->zoomedY(counter);
        _sequenceToWork.push_back(row);
        counter++;
    }
    size_t dataSize = 0;
    if(gl_viewer->byteMode() == 1 || !gl_viewer->hasHardware()){
		dataSize =  w*h*sizeof(U32);
	}else{
		dataSize = w*h*sizeof(float)*4;
	}
    gl_viewer->drawing(true);
    void* gpuMappedBuffer = gl_viewer->allocateAndMapPBO(dataSize,gl_viewer->getPBOId(0));
    _gpuTransferInfo.set(gl_viewer->getFrameData(), gpuMappedBuffer, dataSize);
    
    *_workerThreadsResults = QtConcurrent::map(_sequenceToWork,boost::bind(&VideoEngine::metaEnginePerRow,this,_1,output));
    _workerThreadsWatcher->setFuture(*_workerThreadsResults);
    
}
VideoEngine::FramesVector VideoEngine::startReading(std::vector<Reader*>& readers,bool useMainThread,bool useOtherThread){
    
    FramesVector frames;
    if(readers.size() == 0) return frames;
    
    Reader::DecodeMode mode = Reader::DEFAULT_DECODE;
    // if(isStereo ) mode = stereo
    
    if(useMainThread){
        bool useOtherThreadOnNextLoop = false;
        for(U32 i = 0;i < readers.size(); i++){
            Reader* reader = readers[i];
            std::string currentFrameName = reader->getRandomFrameName(currentViewer->frameSeeker->currentFrame());
            std::vector<Reader::Buffer::DecodedFrameDescriptor> ret;
            pair<ViewerCache::FramesIterator,bool> iscached =
            _viewerCache->isCached(currentFrameName,
                                   _treeVersion.getHashValue(),
                                   gl_viewer->getZoomFactor(),
                                   gl_viewer->getExposure(),
                                   gl_viewer->lutType(),
                                   gl_viewer->_byteMode);
            if(!iscached.second){
                if(useOtherThreadOnNextLoop && useOtherThread){
                    ret = reader->decodeFrames(mode, false, true , _forward);
                }else{
                    ret = reader->decodeFrames(mode, true, false , _forward);
                }
                useOtherThreadOnNextLoop = !useOtherThreadOnNextLoop;
                for(U32 j = 0; j < ret.size() ; j ++){
                    frames.push_back(make_pair(ret[j],_viewerCache->end()));
                }
            }else{
                int cur = currentViewer->frameSeeker->currentFrame();
                int frameNb = _frameRequestsCount==1 && reader->firstFrame() == reader->lastFrame() ? 0 : cur;
                frames.push_back(make_pair(reader->openCachedFrame(iscached.first,frameNb,false),
                                           iscached.first));
            }
        }
        for(U32 i = 0 ;i < frames.size() ;i++){
            Reader::Buffer::DecodedFrameDescriptor it = frames[i].first;
            if(it._asynchTask && !it._asynchTask->isFinished()){
                it._asynchTask->waitForFinished();
            }
        }
    }else{
        if(!useOtherThread) return frames;
        int cur = currentViewer->frameSeeker->currentFrame();
        std::vector<Reader::Buffer::DecodedFrameDescriptor> ret;
        Reader* reader = readers[0];
        if(reader->firstFrame() == reader->lastFrame()) return frames;
        int followingFrame = cur;
        _forward ? followingFrame++ : followingFrame--;
        if(followingFrame > reader->lastFrame()) followingFrame = reader->firstFrame();
        if(followingFrame < reader->firstFrame()) followingFrame = reader->lastFrame();
        
        std::string followingFrameName = reader->getRandomFrameName(followingFrame);
        pair<ViewerCache::FramesIterator,bool> iscached = _viewerCache->isCached(followingFrameName,
                                                                                 _treeVersion.getHashValue(),
                                                                                 gl_viewer->getZoomFactor(),
                                                                                 gl_viewer->getExposure(),
                                                                                 gl_viewer->lutType(),
                                                                                 gl_viewer->_byteMode);
        if(!iscached.second){
            ret = reader->decodeFrames(mode, false, true, _forward);
            for(U32 j = 0; j < ret.size() ; j ++){
                frames.push_back(make_pair(ret[j],_viewerCache->end()));
            }
        }else{
            int frameNb = _frameRequestsCount==1 && reader->firstFrame() == reader->lastFrame() ? 0 : followingFrame;
            frames.push_back(make_pair(reader->openCachedFrame(iscached.first,frameNb,true),
                                       iscached.first));
        }
        
    }
    return frames;
}

void VideoEngine::drawOverlay(){
    if(_dag.getOutput())
        _drawOverlay(_dag.getOutput());
}

void VideoEngine::_drawOverlay(Node *output){
    output->drawOverlay();
    foreach(Node* n,output->getParents()){
        _drawOverlay(n);
    }
    
}

void VideoEngine::metaEnginePerRow(Row* row, OutputNode* output){
    row->allocate();
    if((output->getOutputChannels() & output->getInfo()->channels())){
        output->engine(row->y(), row->offset(), row->right(), output->getRequestedChannels(), row);
    }
    delete row;
//    for(DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); it++){
//        Node* node = *it;
//        if((node->getOutputChannels() & node->getInfo()->channels())){
//            
//            if(!row->cached()){
//                node->engine(row->y(),row->offset(),row->right(),node->getRequestedChannels() & node->getInfo()->channels(),row);
//            }else{
//                if(node == output){
//                    node->engine(row->y(),row->offset(),row->right(),
//                                 node->getRequestedChannels() & node->getInfo()->channels(),row);
//                }
//            }
//        }
//    }
}

void VideoEngine::updateProgressBar(){
    //update progress bar
}
void VideoEngine::updateDisplay(){
    int width = gl_viewer->width();
    int height = gl_viewer->height();
    float ap = gl_viewer->displayWindow().pixel_aspect();
    if(ap > 1.f){
        glViewport (0, 0, width*ap, height);
    }else{
        glViewport (0, 0, width, height/ap);
    }
    gl_viewer->updateGL();
}

void VideoEngine::startEngine(int nbFrames){
    videoEngine(true,nbFrames,true,true);
}

VideoEngine::VideoEngine(Model* engine,QMutex* lock):
_working(false),_aborted(false),_paused(true),
_forward(true),_frameRequestsCount(0),_frameRequestIndex(0),_loopMode(true),_sameFrame(false){
    
    _engineLoopWatcher = new QFutureWatcher<void>;
    _enginePostProcessResults = new QFuture<void>;
    _workerThreadsResults = new QFuture<void>;
    _workerThreadsWatcher = new QFutureWatcher<void>;
    connect(_workerThreadsWatcher,SIGNAL(finished()),this,SLOT(finishComputeFrameRequest()));
    connect(_engineLoopWatcher, SIGNAL(finished()), this, SLOT(engineLoop()));
    this->_coreEngine = engine;
    this->_lock= lock;
    
    _timer=new Timer();
    
    qint64 maxDiskCacheSize = Settings::getPowiterCurrentSettings()->_cacheSettings.maxDiskCache;
    qint64 maxMemoryCacheSize = (double)getSystemTotalRAM() *
    Settings::getPowiterCurrentSettings()->_cacheSettings.maxCacheMemoryPercent;
    _viewerCache = new ViewerCache(maxDiskCacheSize/2, maxMemoryCacheSize);
    
}

VideoEngine::~VideoEngine(){
    _enginePostProcessResults->waitForFinished();
    _workerThreadsResults->waitForFinished();
    delete _workerThreadsResults;
    delete _workerThreadsWatcher;
    delete _engineLoopWatcher;
    delete _enginePostProcessResults;
    delete _viewerCache;
    delete _timer;
}



void VideoEngine::clearInfos(Node* out){
    out->clear_info();
    foreach(Node* c,out->getParents()){
        clearInfos(c);
    }
}

void VideoEngine::setDesiredFPS(double d){
    _timer->setDesiredFrameRate(d);
}


void VideoEngine::abort(){
    _aborted=true;
}
void VideoEngine::pause(){
    _paused=true;
    
}
void VideoEngine::startPause(bool c){
    if(currentViewer->play_Backward_Button->isChecked()){
        abort();
        return;
    }
    
    if(c && _dag.getOutput()){
        videoEngine(true,-1,false,true);
    }
    else if(!_dag.getOutput() || _dag.getInputs().size()==0){
        currentViewer->play_Forward_Button->setChecked(false);
        currentViewer->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::startBackward(bool c){
    
    if(currentViewer->play_Forward_Button->isChecked()){
        pause();
        return;
    }
    if(c && _dag.getOutput()){
        videoEngine(true,-1,false,false);
    }
    else if(!_dag.getOutput() || _dag.getInputs().size()==0){
        currentViewer->play_Forward_Button->setChecked(false);
        currentViewer->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::previousFrame(){
    if( currentViewer->play_Forward_Button->isChecked()
       || currentViewer->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->frameSeeker->currentFrame()-1, 1, false,false);
    //    else
    //        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()-1, 1, false,&VideoEngine::_previousFrame);
}

void VideoEngine::nextFrame(){
    if(currentViewer->play_Forward_Button->isChecked()
       || currentViewer->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->frameSeeker->currentFrame()+1, 1, false,true);
    //    else
    //        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false,&VideoEngine::_nextFrame);
}

void VideoEngine::firstFrame(){
    if( currentViewer->play_Forward_Button->isChecked()
       || currentViewer->play_Backward_Button->isChecked()){
        pause();
    }
    TimeSlider* frameSeeker = currentViewer->frameSeeker;
    if(!_working)
        _startEngine(frameSeeker->firstFrame(), 1, false,false);
    //    else
    //        appendTask(frameSeeker->firstFrame(), 1, false, &VideoEngine::_firstFrame);
}

void VideoEngine::lastFrame(){
    if(currentViewer->play_Forward_Button->isChecked()
       ||  currentViewer->play_Backward_Button->isChecked()){
        pause();
    }
    TimeSlider* frameSeeker = currentViewer->frameSeeker;
    if(!_working)
        _startEngine(frameSeeker->lastFrame(), 1, false,true);
    //    else
    //        appendTask(frameSeeker->lastFrame(), 1, false, &VideoEngine::_lastFrame);
}

void VideoEngine::previousIncrement(){
    if(currentViewer->play_Forward_Button->isChecked()
       ||  currentViewer->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = currentViewer->frameSeeker->currentFrame()- currentViewer->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,false);
    //    else{
    //        appendTask(frame,1, false, &VideoEngine::_previousIncrement);
    //    }
    
    
}

void VideoEngine::nextIncrement(){
    if(currentViewer->play_Forward_Button->isChecked()
       ||  currentViewer->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = currentViewer->frameSeeker->currentFrame()+currentViewer->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,true);
    //    else
    //        appendTask(frame,1, false, &VideoEngine::_nextIncrement);
}

void VideoEngine::seekRandomFrame(int f){
    if(!_dag.getOutput() || _dag.getInputs().size()==0) return;
    //if( ctrlPTR->getGui()->viewer_tab->play_Forward_Button->isChecked()
    //  ||  ctrlPTR->getGui()->viewer_tab->play_Backward_Button->isChecked()){
    pause();
    // }
    if(!_working)
        _startEngine(f, 1, false,true);
    else
        appendTask(f, -1, false,true, _dag.getOutput(),&VideoEngine::_startEngine);
}




void VideoEngine::changeDAGAndStartEngine(OutputNode* output){
    pause();
    if(!_working)
        _changeDAGAndStartEngine(currentViewer->frameSeeker->currentFrame(), -1, false,true,output);
    else
        appendTask(currentViewer->frameSeeker->currentFrame(), -1, false,true, output, &VideoEngine::_changeDAGAndStartEngine);
}

void VideoEngine::appendTask(int frameNB, int frameCount, bool initViewer,bool forward,OutputNode* output, VengineFunction func){
    _waitingTasks.push_back(Task(frameNB,frameCount,initViewer,forward,output,func));
}

void VideoEngine::runTasks(){
    for(unsigned int i=0; i < _waitingTasks.size(); i++){
        Task _t = _waitingTasks[i];
        VengineFunction f = _t._func;
        VideoEngine *vengine = this;
        _waitingTasks.clear();
        (*vengine.*f)(_t._newFrameNB,_t._frameCount,_t._initViewer,_t._forward,_t._output);
    }
}

void VideoEngine::_startEngine(int frameNB,int frameCount,bool initViewer,bool forward,OutputNode* output){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        TimeSlider* frameSeeker = currentViewer->frameSeeker;
        if(frameNB < frameSeeker->firstFrame() || frameNB > frameSeeker->lastFrame())
            return;
        frameSeeker->seek(frameNB);
        videoEngine(true,frameCount,initViewer,forward);
        
    }
}

void VideoEngine::_changeDAGAndStartEngine(int frameNB, int frameCount, bool initViewer,bool forward,OutputNode* output){
    _dag.resetAndSort(output);
    changeTreeVersion();
    videoEngine(true,-1,initViewer,_forward);
}

void VideoEngine::debugTree(){
    int nb=0;
    _debugTree(_dag.getOutput(),&nb);
    cout << "The tree contains " << nb << " nodes. " << endl;
}
void VideoEngine::_debugTree(Node* n,int* nb){
    *nb = *nb+1;
    cout << *n << endl;
    foreach(Node* c,n->getParents()){
        _debugTree(c,nb);
    }
}
void VideoEngine::computeTreeHash(std::vector< std::pair<std::string,U64> > &alreadyComputed, Node *n){
    for(int i =0; i < alreadyComputed.size();i++){
        if(alreadyComputed[i].first == n->getName().toStdString())
            return;
    }
    std::vector<std::string> v;
    n->computeTreeHash(v);
    U64 hashVal = n->getHash()->getHashValue();
    alreadyComputed.push_back(make_pair(n->getName().toStdString(),hashVal));
    foreach(Node* parent,n->getParents()){
        computeTreeHash(alreadyComputed, parent);
    }
    
    
}

void VideoEngine::changeTreeVersion(){
    std::vector< std::pair<std::string,U64> > nodeHashs;
    _treeVersion.reset();
    if(!_dag.getOutput()){
        return;
    }
    computeTreeHash(nodeHashs, _dag.getOutput());
    for(int i =0 ;i < nodeHashs.size();i++){
        _treeVersion.appendNodeHashToHash(nodeHashs[i].second);
    }
    _treeVersion.computeHash();
    
}


void VideoEngine::DAG::fillGraph(Node* n){
    if(std::find(_graph.begin(),_graph.end(),n)==_graph.end()){
        n->setMarked(false);
        _graph.push_back(n);
        if(n->isInputNode()){
            _inputs.push_back(static_cast<InputNode*>(n));
        }
    }
    foreach(Node* p,n->getParents()){
        fillGraph(p);
    }
}
void VideoEngine::DAG::clearGraph(){
    _graph.clear();
    _sorted.clear();
    _inputs.clear();
    
}
void VideoEngine::DAG::topologicalSort(){
    for(U32 i = 0 ; i < _graph.size(); i++){
        Node* n = _graph[i];
        if(!n->isMarked())
            _depthCycle(n);
    }
    
}
void VideoEngine::DAG::_depthCycle(Node* n){
    n->setMarked(true);
    foreach(Node* p, n->getParents()){
        if(!p->isMarked()){
            _depthCycle(p);
        }
    }
    _sorted.push_back(n);
}
void VideoEngine::DAG::resetAndSort(OutputNode* out){
    _output = out;
    _validate = &VideoEngine::DAG::validate;
    _hasValidated = false;
    clearGraph();
    if(!_output){
        return;
    }
    fillGraph(dynamic_cast<Node*>(out));
    
    topologicalSort();
}
void VideoEngine::DAG::debug(){
    cout << "Topological ordering of the DAG is..." << endl;
    for(DAG::DAGIterator it = begin(); it != end() ;it++){
        cout << (*it)->getName().toStdString() << endl;
    }
}

/*sets infos accordingly across all the DAG*/
void VideoEngine::DAG::validate(bool forReal){
    _output->validate(forReal);
    _hasValidated = true;
    _validate = &VideoEngine::DAG::validateInputs;
}

/*same as validate(), but it refreshes info only for inputNodes.
 This*/
void VideoEngine::DAG::validateInputs(bool forReal){
    foreach(InputNode* i,_inputs) i->validate(forReal);
}
int VideoEngine::DAG::firstFrame(){
    return _output->getInfo()->firstFrame();
}
int VideoEngine::DAG::lastFrame(){
    return _output->getInfo()->lastFrame();
}

void VideoEngine::debugRowSequence(){
    int h = _sequenceToWork.size();
    int w = _sequenceToWork[0]->right() - _sequenceToWork[0]->offset();
    if(h == 0 || w == 0) cout << "empty img" << endl;
    QImage img(w,h,QImage::Format_ARGB32);
    for(int i = 0; i < h ; i++){
        Row* from = _sequenceToWork[i];
        const float* r = (*from)[Channel_red];
        const float* g = (*from)[Channel_green];
        const float* b = (*from)[Channel_blue];
        const float* a = (*from)[Channel_alpha];
        for(int j = 0 ; j < w ; j++){
            QColor c(r ? Lut::clamp((*r++))*255 : 0,
                     g ? Lut::clamp((*g++))*255 : 0,
                     b ? Lut::clamp((*b++))*255 : 0,
                     a? Lut::clamp((*a++))*255 : 255);
            img.setPixel(j, i, c.rgba());
        }
    }
    string name("debug_");
    char tmp[60];
    sprintf(tmp,"%i",w);
    name.append(tmp);
    name.append("x");
    sprintf(tmp, "%i",h);
    name.append(tmp);
    name.append(".png");
    img.save(name.c_str());
}
