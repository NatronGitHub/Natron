

//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#include "Core/VideoEngine.h"

#include <iterator>
#include <cassert>
#include <QtCore/QMutex>
#include <QtCore/QCoreApplication>
#include <QtGui/QVector2D>
#include <QAction>
#include <QtCore/QThread>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <ImfThreading.h>

#include "Gui/button.h"
#include "Core/inputnode.h"
#include "Core/outputnode.h"
#include "Core/viewerNode.h"
#include "Core/settings.h"
#include "Core/model.h"
#include "Core/hash.h"
#include "Core/Timer.h"
#include "Core/lookUpTables.h"
#include "Core/viewercache.h"
#include "Core/nodecache.h"
#include "Core/row.h"
#include "Core/mappedfile.h"
#include "Writer/Writer.h"

#include "Gui/mainGui.h"
#include "Gui/viewerTab.h"
#include "Gui/timeline.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/GLViewer.h"

#include "Superviser/controler.h"
#include "Superviser/MemoryInfo.h"


/* Here's a drawing that represents how the video Engine works:
 
 1) Without caching
 
 [main thread]                    [OtherThread]
      |                                  |
 - videoEngine
 |------QtConcurrent::Run()>-computeFrameRequest_<
 |< -------------------------**finished()**      |
 -computeTreeForFrame()                  |           |
 |------QtConcurrent::map()--->metaEnginePerRow  |
 |<---------------------------**finished()**     |
 -engineLoop()                           |           |
 |-----------------------QtConcurrent::Run()------
 -updateDisplay()
 
 
 Whenever waiting for a **finished()** signal, the main thread is still executing
 the event loop. All OpenGL code is kept into the main thread and everything is done
 to avoid complications related to OpenGL.
 */

using namespace std;
using namespace Powiter;
#define gl_viewer currentViewer->getUiContext()->viewer

void VideoEngine::videoEngine(int frameCount,bool fitFrameToViewer,bool forward,bool sameFrame){
    if (_working) {
        return;
    }
    // cout << "+ STARTING ENGINE " << endl;
    _timer->playState=RUNNING;
    _frameRequestsCount = frameCount;
    _frameRequestIndex = 0;
    _forward = forward;
    _paused = false;
    _aborted = false;
    _dag.validate(false); // < validating sequence (mostly getting the same frame range for all nodes).
    float zoomFactor;
    if(_dag.isOutputAViewer()){
        zoomFactor = gl_viewer->getZoomFactor();
    }else{
        zoomFactor = 1.f;
    }
    QFuture<void> future = QtConcurrent::run(this,&VideoEngine::computeFrameRequest,zoomFactor,sameFrame,fitFrameToViewer,false);
    _computeFrameWatcher->setFuture(future);
    
}
void VideoEngine::stopEngine(){
    if(_dag.isOutputAViewer()){
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
    }
    // cout << "- STOPPING ENGINE"<<endl;
    _frameRequestsCount = 0;
    _aborted = false;
    _paused = false;
    _lastEngineStatus._returnCode = EngineStatus::ABORTED;
    _working = false;
    _timer->playState=PAUSE;
    
}


void VideoEngine::computeFrameRequest(float zoomFactor,bool sameFrame,bool fitFrameToViewer,bool recursiveCall){
    //cout << "     _computeFrameRequest()" << endl;
    _working = true;
    _sameFrame = sameFrame;
    EngineStatus::RetCode returnCode;
    int firstFrame = INT_MAX,lastFrame = INT_MIN, currentFrame = 0;
    TimeLine* frameSeeker = 0;
    if(_dag.isOutputAViewer()){
        frameSeeker = currentViewer->getUiContext()->frameSeeker;
    }
    Writer* writer = dynamic_cast<Writer*>(_dag.getOutput());
    Viewer* viewer = dynamic_cast<Viewer*>(_dag.getOutput());
    if (!_dag.isOutputAViewer()) {
        assert(writer);
        if(!recursiveCall){
            lastFrame = writer->lastFrame();
            currentFrame = writer->firstFrame();
            writer->setCurrentFrameToStart();
            
        }else{
            lastFrame = writer->lastFrame();
            writer->incrementCurrentFrame();
            currentFrame = writer->currentFrame();
        }
    }
    
    /*check whether we need to stop the engine*/
    if(_aborted){
        /*aborted by the user*/
        stopEngine();
        _lastEngineStatus._returnCode = EngineStatus::ABORTED;
        return;
    }else if((_dag.isOutputAViewer()
             &&  recursiveCall
             && _dag.lastFrame() == _dag.firstFrame()
             && _frameRequestsCount == -1
             && _frameRequestIndex == 1)
             || _frameRequestsCount == 0
             || _paused){
        /*1 frame in the sequence and we already computed it*/
        stopEngine();
        runTasks();
        _lastEngineStatus._returnCode = EngineStatus::ABORTED;
        return;
    }else if(!_dag.isOutputAViewer() && currentFrame == lastFrame+1){
        /*stoping the engine for writers*/
        stopEngine();
        _lastEngineStatus._returnCode = EngineStatus::ABORTED;
        return;
    }
    
    if(_dag.isOutputAViewer()){
        /*Determine what is the current frame when output is a viewer*/
        /*!recursiveCall means this is the first time it's called for the sequence.*/
        if(!recursiveCall){
            currentFrame = frameSeeker->currentFrame();
            if(!sameFrame){
                firstFrame = _dag.firstFrame();
                lastFrame = _dag.lastFrame();
                
                /*clamping the current frame to the range [first,last] if it wasn't*/
                if(currentFrame < firstFrame){
                    currentFrame = firstFrame;
                }
                else if(currentFrame > lastFrame){
                    currentFrame = lastFrame;
                }
                QMetaObject::invokeMethod(frameSeeker, "seek", Qt::QueuedConnection, Q_ARG(int,currentFrame));
            }
        }else{ // if the call is recursive, i.e: the next frame in the sequence
            /*clear the node cache, as it is very unlikely the user will re-use
             data from previous frame.*/
            //  NodeCache::getNodeCache()->clear();
            lastFrame = _dag.lastFrame();
            firstFrame = _dag.firstFrame();
            if(_forward){
                currentFrame = currentViewer->currentFrame()+1;
                if(currentFrame > lastFrame){
                    if(_loopMode)
                        currentFrame = firstFrame;
                    else{
                        _frameRequestsCount = 0;
                        _lastEngineStatus._returnCode = EngineStatus::ABORTED;
                        return;
                    }
                }
            }else{
                currentFrame  = currentViewer->currentFrame()-1;
                if(currentFrame < firstFrame){
                    if(_loopMode)
                        currentFrame = lastFrame;
                    else{
                        _frameRequestsCount = 0;
                        _lastEngineStatus._returnCode = EngineStatus::ABORTED;
                        return;
                    }
                }
            }
            QMetaObject::invokeMethod(frameSeeker, "seek", Qt::QueuedConnection, Q_ARG(int,currentFrame));
        }
    }
    
    std::vector<Reader*> readers;
    const std::vector<InputNode*>& inputs = _dag.getInputs();
    for(U32 j=0;j<inputs.size();j++){
        InputNode* currentInput=inputs[j];
        if(currentInput->className() == string("Reader")){
            Reader* inp = static_cast<Reader*>(currentInput);
            inp->fitFrameToViewer(fitFrameToViewer);
            readers.push_back(inp);
        }
    }
    
    QtConcurrent::blockingMap(readers,boost::bind(&VideoEngine::metaReadHeader,_1,currentFrame));
    
    if(!_dag.validate(true)){
        stopEngine();
        _lastEngineStatus._returnCode = EngineStatus::ABORTED;
        return;
    }
    const Format &_dispW = _dag.getOutput()->getInfo()->getDisplayWindow();
    if(_dag.isOutputAViewer() && fitFrameToViewer){
        gl_viewer->fitToFormat(_dispW);
        zoomFactor = gl_viewer->getZoomFactor();
    }
    /*Now that we called validate we can check if the frame is in the cache
     and return the appropriate EngineStatus code.*/
    vector<int> rows;
    vector<int> columns;
    const Box2D& dataW = _dag.getOutput()->getInfo()->getDataWindow();
    FrameEntry* iscached= 0;
    U64 key = 0;
    if(_dag.isOutputAViewer()){
        
        std::pair<int,int> rowSpan = gl_viewer->computeRowSpan(rows,_dispW);
        std::pair<int,int> columnSpan = gl_viewer->computeColumnSpan(columns, _dispW);
        
        TextureRect textureRect(columnSpan.first,rowSpan.first,columnSpan.second,rowSpan.second,columns.size(),rows.size());
        /*Now checking if the frame is already in either the ViewerCache*/
        _viewerCacheArgs._zoomFactor = zoomFactor;
        _viewerCacheArgs._exposure = gl_viewer->getExposure();
        _viewerCacheArgs._lut = gl_viewer->lutType();
        _viewerCacheArgs._byteMode = gl_viewer->byteMode();
        _viewerCacheArgs._dataWindow = dataW;
        _viewerCacheArgs._displayWindow = _dispW;
        _viewerCacheArgs._textureRect = textureRect;
        if(textureRect.w == 0 || textureRect.h == 0){
            _lastEngineStatus._returnCode = EngineStatus::ABORTED;
            stopEngine();
            return;
        }
        key = FrameEntry::computeHashKey(currentFrame,
                                         _treeVersion.getHashValue(),
                                         _viewerCacheArgs._zoomFactor,
                                         _viewerCacheArgs._exposure,
                                         _viewerCacheArgs._lut,
                                         _viewerCacheArgs._byteMode,
                                         dataW,
                                         _dispW,
                                         textureRect);
        _lastEngineStatus._x = columnSpan.first;
        _lastEngineStatus._r = columnSpan.second;
        
//        _lastEngineStatus._x = dataW.x();
//        _lastEngineStatus._r = dataW.right();

        _viewerCacheArgs._hashKey = key;
        iscached = viewer->get(key);
        
        /*Found in viewer cache, we execute the cached engine and leave*/
        if(iscached){
            returnCode = EngineStatus::CACHED_ENGINE;
            goto stop;
        }
        
        
        
        /*checking whether it is texture cached (only when not in playback)*/
        if(!recursiveCall && viewer->isTextureCached(key)){
            returnCode = EngineStatus::TEXTURE_CACHED_ENGINE;
            goto stop;
        }
        
    }else{
        for (int i = dataW.y(); i < dataW.top(); i++) {
            rows.push_back(i);
        }
        _lastEngineStatus._x = dataW.x();
        _lastEngineStatus._r = dataW.right();
    }
    /*If it reaches here, it means the frame neither belong
     to the ViewerCache nor to the TextureCache, we must
     allocate resources and render the frame.
     If this is a recursive call, we explicitly fallback
     to the viewer cache storage as the texture cache is not
     meant for playback.*/
    QtConcurrent::blockingMap(readers,boost::bind(&VideoEngine::metaReadData,_1,currentFrame));
    returnCode = EngineStatus::NORMAL_ENGINE;

stop:
    _lastEngineStatus._cachedEntry = iscached;
    _lastEngineStatus._key = key;
    _lastEngineStatus._returnCode = returnCode;
    _lastEngineStatus._rows = rows;
}

void VideoEngine::dispatchEngine(){
    //cout << "     _dispatchEngine()" << endl;
    if(_lastEngineStatus._returnCode == EngineStatus::NORMAL_ENGINE) {
        if (_dag.isOutputAViewer()) {
            Viewer* viewer = _dag.outputAsViewer();
            viewer->makeCurrentViewer();
            _viewerCacheArgs._dataSize = gl_viewer->allocateFrameStorage(_viewerCacheArgs._textureRect.w,
                                                                                _viewerCacheArgs._textureRect.h);
        }
        //   cout << "     _computeTreForFrame()" << endl;
        computeTreeForFrame(_lastEngineStatus._rows,_lastEngineStatus._x,_lastEngineStatus._r,_dag.getOutput());
        
    }else if(_lastEngineStatus._returnCode == EngineStatus::CACHED_ENGINE){
        //cout << "    _cachedFrameEngine()" << endl;
        Viewer* viewer = _dag.outputAsViewer();
        viewer->cachedFrameEngine(_lastEngineStatus._cachedEntry);
    }
    else if(_lastEngineStatus._returnCode == EngineStatus::TEXTURE_CACHED_ENGINE){
        //  cout << "    _textureCachedEngine()" << endl;
        engineLoop();
    }//else{
     //    cout <<"__really stopping__" << endl;
     // }
    
}

void VideoEngine::copyFrameToCache(const char* src){
    
    FrameEntry* entry = ViewerCache::getViewerCache()->addFrame(_viewerCacheArgs._hashKey,
                                                                _treeVersion.getHashValue(),
                                                                _viewerCacheArgs._zoomFactor,
                                                                _viewerCacheArgs._exposure,
                                                                _viewerCacheArgs._lut,
                                                                _viewerCacheArgs._byteMode,
                                                                _viewerCacheArgs._textureRect,
                                                                _viewerCacheArgs._dataWindow,
                                                                _viewerCacheArgs._displayWindow);
    
    if(entry){
        memcpy(entry->getMappedFile()->data(),src,_viewerCacheArgs._dataSize);
    }else{
#ifdef PW_DEBUG
        cout << "WARNING: caching does not seem to work properly..failing to add the entry." << endl;
#endif
    }
}
void VideoEngine::computeTreeForFrame(const std::vector<int>& rows,int x,int r,OutputNode *output){
    ChannelSet outChannels;
    if(_dag.isOutputAViewer()){
        outChannels = currentViewer->getUiContext()->displayChannels();
    }
    else{// channels requested are those requested by the user
        outChannels = static_cast<Writer*>(_dag.getOutput())->requestedChannels();
    }
    int counter = 0;
    for(vector<int>::const_iterator it = rows.begin(); it!=rows.end() ; it++){
        Row* row = new Row(x,*it,r,outChannels);
        row->zoomedY(counter);
        _sequenceToWork.push_back(row);
        counter++;
    }
    *_workerThreadsResults = QtConcurrent::map(_sequenceToWork,boost::bind(&VideoEngine::metaEnginePerRow,_1,output));
    _workerThreadsWatcher->setFuture(*_workerThreadsResults);
    
}


void VideoEngine::engineLoop(){
    //cout << "     _engineLoop()" << endl;
    if(_frameRequestIndex == 0 && _frameRequestsCount == 1 && !_sameFrame){
        _frameRequestsCount = 0;
    }else if(_frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
        _frameRequestsCount--;
    }
    _frameRequestIndex++;//incrementing the frame counter
    
    //clearing the Row objects used by the QtConcurrent::map call, note that all Row's already have been destroyed.
    _sequenceToWork.clear();
    float zoomFactor;
    if(_dag.isOutputAViewer()){
        
        
        gl_viewer->drawing(true);

        //copying the frame data stored into the PBO to the viewer cache if it was a normal engine
        if(_lastEngineStatus._returnCode == EngineStatus::NORMAL_ENGINE){
            copyFrameToCache(gl_viewer->getFrameData());
            // now that the texture is full we can cache it
            TextureEntry* texture = new TextureEntry;
            texture->setHashKey(_viewerCacheArgs._hashKey);
            gl_viewer->copyPBOToNewTexture(texture, _viewerCacheArgs._textureRect);
            
        }
        else if(_lastEngineStatus._returnCode == EngineStatus::CACHED_ENGINE){
            //copying the content of the PBO to the rendering texture.
            gl_viewer->copyPBOToExistingTexture(); // returns instantly
        }
        
        
        
        _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
        if((_frameRequestIndex%24)==0){
            emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
        }
        zoomFactor = gl_viewer->getZoomFactor();
    }else{
        /*if the output is a writer we actually start writing on disk now*/
        _dag.outputAsWriter()->startWriting();
        zoomFactor = 1.f;
    }
    
    // recursive call, before the updateDisplay (swapBuffer) so it can run concurrently
    QFuture<void> future = QtConcurrent::run(this,&VideoEngine::computeFrameRequest,zoomFactor,false,false,true);
    _computeFrameWatcher->setFuture(future);
    if(_dag.isOutputAViewer()){
        updateDisplay(); // updating viewer & pixel aspect ratio if needed
    }
}



void VideoEngine::metaReadHeader(Reader* reader,int current_frame){
    reader->readCurrentHeader(current_frame);
}

void VideoEngine::metaReadData(Reader* reader,int current_frame){
    reader->readCurrentData(current_frame);
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
    output->engine(row->y(), row->offset(), row->right(), row->channels(), row);
    delete row;
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
    if (dagHasInputs()) {
        videoEngine(nbFrames,true,true);
        
    }
}
void VideoEngine::repeatSameFrame(){
    if (dagHasInputs()) {
        if(_working){
            appendTask(currentViewer->currentFrame(), 1, false,_forward,true, _dag.getOutput(),&VideoEngine::_startEngine);
        }else{
            videoEngine(1,false,true,true);
        }
    }
}

VideoEngine::VideoEngine(Model* engine,QMutex* lock):
_coreEngine(engine),
_working(false),
_lock(lock),
_aborted(false),
_paused(true),
_frameRequestsCount(0),
_frameRequestIndex(0),
_forward(true),
_loopMode(true),
_sameFrame(false)
{
    
    _workerThreadsResults = new QFuture<void>;
    _workerThreadsWatcher = new QFutureWatcher<void>;
    connect(_workerThreadsWatcher,SIGNAL(finished()),this,SLOT(engineLoop()));
    _computeFrameWatcher = new QFutureWatcher<void>;
    connect(_computeFrameWatcher,SIGNAL(finished()),this,SLOT(dispatchEngine()));
    //  connect(_workerThreadsWatcher,SIGNAL(canceled()),this,SLOT(stopEngine()));
    /*Adjusting multi-threading for OpenEXR library.*/
    Imf::setGlobalThreadCount(QThread::idealThreadCount());
    
    _timer=new Timer();
    
}

VideoEngine::~VideoEngine(){
    _workerThreadsResults->waitForFinished();
    delete _workerThreadsResults;
    delete _workerThreadsWatcher;
    delete _timer;
    delete _computeFrameWatcher;
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
    _workerThreadsResults->cancel();
    if(currentViewer){
        gl_viewer->forceUnmapPBO();
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
    }
}
void VideoEngine::pause(){
    _paused=true;
    
}
void VideoEngine::startPause(bool c){
    if(currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        abort();
        return;
    }
    
    
    if(c && _dag.getOutput()){
        videoEngine(-1,false,true);
    }
    else if(!_dag.getOutput() || _dag.getInputs().size()==0){
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::startBackward(bool c){
    
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()){
        pause();
        return;
    }
    if(c && _dag.getOutput()){
        videoEngine(-1,false,false);
    }
    else if(!_dag.getOutput() || _dag.getInputs().size()==0){
        currentViewer->getUiContext()->play_Forward_Button->setChecked(false);
        currentViewer->getUiContext()->play_Backward_Button->setChecked(false);
        
    }else{
        pause();
    }
}
void VideoEngine::previousFrame(){
    if( currentViewer->getUiContext()->play_Forward_Button->isChecked()
       || currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->currentFrame()-1, 1, false,false,false);
    //    else
    //        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()-1, 1, false,&VideoEngine::_previousFrame);
}

void VideoEngine::nextFrame(){
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()
       || currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    
    if(!_working)
        _startEngine(currentViewer->currentFrame()+1, 1, false,true,false);
    //    else
    //        appendTask(gl_viewer->getCurrentReaderInfo()->currentFrame()+1, 1, false,&VideoEngine::_nextFrame);
}

void VideoEngine::firstFrame(){
    if( currentViewer->getUiContext()->play_Forward_Button->isChecked()
       || currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    
    if(!_working)
        _startEngine(currentViewer->firstFrame(), 1, false,false,false);
    //    else
    //        appendTask(frameSeeker->firstFrame(), 1, false, &VideoEngine::_firstFrame);
}

void VideoEngine::lastFrame(){
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()
       ||  currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->lastFrame(), 1, false,true,false);
    //    else
    //        appendTask(frameSeeker->lastFrame(), 1, false, &VideoEngine::_lastFrame);
}

void VideoEngine::previousIncrement(){
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()
       ||  currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = currentViewer->currentFrame()- currentViewer->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,false,false);
    //    else{
    //        appendTask(frame,1, false, &VideoEngine::_previousIncrement);
    //    }
    
    
}

void VideoEngine::nextIncrement(){
    if(currentViewer->getUiContext()->play_Forward_Button->isChecked()
       ||  currentViewer->getUiContext()->play_Backward_Button->isChecked()){
        pause();
    }
    int frame = currentViewer->currentFrame()+currentViewer->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,true,false);
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
        _startEngine(f, 1, false,true,false);
    else
        appendTask(f, -1, false,_forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
}




void VideoEngine::changeDAGAndStartEngine(OutputNode* output){
    pause();
    if(!_working)
        _changeDAGAndStartEngine(currentViewer->currentFrame(), -1, false,true,false,output);
    else
        appendTask(currentViewer->currentFrame(), -1, false,true,false, output, &VideoEngine::_changeDAGAndStartEngine);
}

void VideoEngine::appendTask(int frameNB, int frameCount, bool initViewer,bool forward,bool sameFrame,OutputNode* output, VengineFunction func){
    _waitingTasks.push_back(Task(frameNB,frameCount,initViewer,forward,sameFrame,output,func));
}

void VideoEngine::runTasks(){
    if(_waitingTasks.size() > 0){
        Task _t = _waitingTasks.back();
        VengineFunction f = _t._func;
        VideoEngine *vengine = this;
        (*vengine.*f)(_t._newFrameNB,_t._frameCount,_t._initViewer,_t._forward,_t._sameFrame,_t._output);
        _waitingTasks.clear();
    }
    
}

void VideoEngine::_startEngine(int frameNB,int frameCount,bool initViewer,bool forward,bool sameFrame,OutputNode* ){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        if(frameNB < currentViewer->firstFrame() || frameNB > currentViewer->lastFrame())
            return;
        QMetaObject::invokeMethod(currentViewer->getUiContext()->frameSeeker, "seek", Qt::DirectConnection, Q_ARG(int,frameNB));
        videoEngine(frameCount,initViewer,forward,sameFrame);
        
    }
}

void VideoEngine::_changeDAGAndStartEngine(int, int, bool initViewer,bool,bool sameFrame,OutputNode* output){
    _dag.resetAndSort(output,true);
    bool hasFrames = false;
    bool hasInputDifferentThanReader = false;
    for (U32 i = 0; i< _dag.getInputs().size(); i++) {
        Reader* r = static_cast<Reader*>(_dag.getInputs()[i]);
        if (r) {
            if (r->hasFrames()) {
                hasFrames = true;
            }
        }else{
            hasInputDifferentThanReader = true;
        }
    }
    changeTreeVersion();
    if(hasInputDifferentThanReader || hasFrames)
        videoEngine(-1,initViewer,_forward,sameFrame);
}


void VideoEngine::computeTreeHash(std::vector< std::pair<std::string,U64> > &alreadyComputed, Node *n){
    for(U32 i =0; i < alreadyComputed.size();i++){
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
    for(U32 i =0 ;i < nodeHashs.size();i++){
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

void VideoEngine::DAG::reset(){
    _output = 0;
    
    clearGraph();
}

Viewer* VideoEngine::DAG::outputAsViewer() const {
    if(_output && _isViewer)
        return dynamic_cast<Viewer*>(_output);
    else
        return NULL;
}

Writer* VideoEngine::DAG::outputAsWriter() const {
    if(_output && !_isViewer)
        return dynamic_cast<Writer*>(_output);
    else
        return NULL;
}

void VideoEngine::DAG::resetAndSort(OutputNode* out,bool isViewer){
    _output = out;
    _isViewer = isViewer;
    
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
bool VideoEngine::DAG::validate(bool forReal){
    /*Validating the DAG in topological order*/
    for (DAGIterator it = begin(); it!=end(); it++) {
        if (!(*it)->validate(forReal)) {
            return false;
        }
    }
    return true;
}


int VideoEngine::DAG::firstFrame() const {
    return _output->getInfo()->firstFrame();
}
int VideoEngine::DAG::lastFrame() const{
    return _output->getInfo()->lastFrame();
}

void VideoEngine::debugRowSequence(){
    int h = _sequenceToWork.size();
    int w = _sequenceToWork[0]->right() - _sequenceToWork[0]->offset();
    if(h == 0 || w == 0) cout << "empty img" << endl;
    QImage img(w,h,QImage::Format_ARGB32);
    for(int i = 0; i < h ; i++){
        Row* from = _sequenceToWork[i];
        const float* r = (*from)[Channel_red] + from->offset();
        const float* g = (*from)[Channel_green]+ from->offset();
        const float* b = (*from)[Channel_blue]+ from->offset();
        const float* a = (*from)[Channel_alpha]+ from->offset();
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

void VideoEngine::resetAndMakeNewDag(OutputNode* output,bool isViewer){
    _dag.resetAndSort(output,isViewer);
}
#ifdef PW_DEBUG
bool VideoEngine::rangeCheck(const std::vector<int>& columns,int x,int r){
    for (unsigned int i = 0; i < columns.size(); i++) {
        if(columns[i] < x || columns[i] > r){
            return false;
        }
    }
    return true;
}
#endif
