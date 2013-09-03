

//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#include "VideoEngine.h"

#include <iterator>
#include <cassert>
#include <QtCore/QMutex>
#include <QtGui/QVector2D>
#include <QAction>
#include <QtCore/QThread>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <ImfThreading.h>

#include "Gui/Button.h"
#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"
#include "Engine/Settings.h"
#include "Engine/Model.h"
#include "Engine/Hash.h"
#include "Engine/Lut.h"
#include "Engine/ViewerCache.h"
#include "Engine/NodeCache.h"
#include "Engine/Row.h"
#include "Engine/MemoryFile.h"
#include "Engine/Timer.h"
#include "Writers/Writer.h"
#include "Readers/Reader.h"


#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/Timeline.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/ViewerGL.h"

#include "Global/AppManager.h"
#include "Global/MemoryInfo.h"




using namespace std;
using namespace Powiter;

#define gl_viewer currentViewer->getUiContext()->viewer



/**
 *@brief The callback reading the header of the current frame for a reader.
 *@param reader[in] A pointer to the reader that will read the header.
 *@param current_frame[in] The frame number in the sequence to decode.
 */

bool metaReadHeader(Reader* reader,int current_frame){
    if(!reader->readCurrentHeader(current_frame))
        return false;
    return true;
}

/**
 *@brief The callback reading the data of the current frame for a reader.
 *@param reader[in] A pointer to the reader that will read the data.
 *@param current_frame[in] The frame number in the sequence to decode.
 */
void metaReadData(Reader* reader,int current_frame){
    reader->readCurrentData(current_frame);
}

/**
 *@brief The callback cycling through the DAG for one scan-line
 *@param row[in] The row to compute. Note that after that function row will be deleted and cannot be accessed any longer.
 *@param output[in] The output node of the graph.
 */
void metaEnginePerRow(Row* row, Node* output){
    output->engine(row->y(), row->offset(), row->right(), row->channels(), row);
    delete row;
    // QMetaObject::invokeMethod(_engine, "onProgressUpdate", Qt::QueuedConnection, Q_ARG(int, zoomedY));
}

void VideoEngine::render(int frameCount,bool fitFrameToViewer,bool forward,bool sameFrame){
    if (_working) {
        return;
    }
    // cout << "+ STARTING ENGINE " << endl;
    _timer->playState=RUNNING;
   
    _paused = false;
    _aborted = false;
    if(!_dag.validate(false)){ // < validating sequence (mostly getting the same frame range for all nodes).
        stopEngine();
        return;
    }
    double zoomFactor;
    if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        zoomFactor = gl_viewer->getZoomFactor();
        _dag.outputAsViewer()->getUiContext()->play_Forward_Button->setChecked(forward);
        _dag.outputAsViewer()->getUiContext()->play_Backward_Button->setChecked(!forward);
    }else{
        zoomFactor = 1.f;
    }
    
    /*beginRenderAction for all openFX nodes*/
    for (DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
        OfxNode* n = dynamic_cast<OfxNode*>(*it);
        if(n){
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            n->beginRenderAction(0, 25, 1, true,renderScale);
        }
    }
    changeTreeVersion();
    
    _lastRunArgs._zoomFactor = zoomFactor;
    _lastRunArgs._sameFrame = sameFrame;
    _lastRunArgs._fitToViewer = fitFrameToViewer;
    _lastRunArgs._recursiveCall = false;
    _lastRunArgs._forward = forward;
    _lastRunArgs._frameRequestsCount = frameCount;
    _lastRunArgs._frameRequestIndex = 0;
    
    if (!isRunning()) {
        start(HighestPriority);
    } else {
        _startCondition.wakeOne();
    }
}
void VideoEngine::stopEngine(){
    if(_dag.isOutputAViewer()){
        _dag.outputAsViewer()->getUiContext()->play_Forward_Button->setChecked(false);
        _dag.outputAsViewer()->getUiContext()->play_Backward_Button->setChecked(false);
    }
    // cout << "- STOPPING ENGINE"<<endl;
    //  _lastRunArgs._frameRequestsCount = 0;
    _aborted = false;
    _paused = false;
    _working = false;
    _timer->playState=PAUSE;
    
    /*endRenderAction for all openFX nodes*/
    for (DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
        OfxNode* n = dynamic_cast<OfxNode*>(*it);
        if(n){
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            n->endRenderAction(0, 25, 1, true, renderScale);
        }
    }
//    _mutex->lock();
//    _startCondition.wait(_mutex);
//    _mutex->unlock();
    _startCondition.wakeOne();
}

void VideoEngine::run(){
    
    for(;;){ // infinite loop
        _working = true;
        int firstFrame = INT_MAX,lastFrame = INT_MIN, currentFrame = 0;
        TimeLine* frameSeeker = 0;
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            frameSeeker = currentViewer->getUiContext()->frameSeeker;
        }
        Writer* writer = dynamic_cast<Writer*>(_dag.getOutput());
        ViewerNode* viewer = dynamic_cast<ViewerNode*>(_dag.getOutput());
        OfxNode* ofxOutput = dynamic_cast<OfxNode*>(_dag.getOutput());
        if (!_dag.isOutputAViewer()) {
            if(!_dag.isOutputAnOpenFXNode()){
                assert(writer);
                if(!_lastRunArgs._recursiveCall){
                    lastFrame = writer->lastFrame();
                    currentFrame = writer->firstFrame();
                    writer->setCurrentFrameToStart();
                    
                }else{
                    lastFrame = writer->lastFrame();
                    writer->incrementCurrentFrame();
                    currentFrame = writer->currentFrame();
                }
            }else{
                assert(ofxOutput);
                if(!_lastRunArgs._recursiveCall){
                    lastFrame = ofxOutput->lastFrame();
                    currentFrame = ofxOutput->firstFrame();
                    ofxOutput->setCurrentFrameToStart();
                    
                }else{
                    lastFrame = ofxOutput->lastFrame();
                    ofxOutput->incrementCurrentFrame();
                    currentFrame = ofxOutput->currentFrame();
                }
            }
        }
        
        /*check whether we need to stop the engine*/
        if(_aborted){
            /*aborted by the user*/
            stopEngine();
            return;
        }
        if((_dag.isOutputAViewer()
            &&  _lastRunArgs._recursiveCall
            && _dag.lastFrame() == _dag.firstFrame()
            && _lastRunArgs._frameRequestsCount == -1
            && _lastRunArgs._frameRequestIndex == 1)
           || _lastRunArgs._frameRequestsCount == 0
           || _paused){
            /*1 frame in the sequence and we already computed it*/
            stopEngine();
            runTasks();
            return;
        }else if(!_dag.isOutputAViewer() && currentFrame == lastFrame+1){
            /*stoping the engine for writers*/
            stopEngine();
            return;
        }
        
        
        
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){ //openfx viewers are UNSUPPORTED
            /*Determine what is the current frame when output is a viewer*/
            /*!recursiveCall means this is the first time it's called for the sequence.*/
            if(!_lastRunArgs._recursiveCall){
                currentFrame = frameSeeker->currentFrame();
                if(!_lastRunArgs._sameFrame){
                    firstFrame = _dag.firstFrame();
                    lastFrame = _dag.lastFrame();
                    
                    /*clamping the current frame to the range [first,last] if it wasn't*/
                    if(currentFrame < firstFrame){
                        currentFrame = firstFrame;
                    }
                    else if(currentFrame > lastFrame){
                        currentFrame = lastFrame;
                    }
                    frameSeeker->seek_notSlot(currentFrame);
                }
            }else{ // if the call is recursive, i.e: the next frame in the sequence
                /*clear the node cache, as it is very unlikely the user will re-use
                 data from previous frame.*/
                //  NodeCache::getNodeCache()->clear();
                lastFrame = _dag.lastFrame();
                firstFrame = _dag.firstFrame();
                if(_lastRunArgs._forward){
                    currentFrame = currentViewer->currentFrame()+1;
                    if(currentFrame > lastFrame){
                        if(_loopMode)
                            currentFrame = firstFrame;
                        else{
                            stopEngine();
                            return;
                        }
                    }
                }else{
                    currentFrame  = currentViewer->currentFrame()-1;
                    if(currentFrame < firstFrame){
                        if(_loopMode)
                            currentFrame = lastFrame;
                        else{
                            stopEngine();
                            return;
                        }
                    }
                }
                frameSeeker->seek_notSlot(currentFrame);
            }
        }
        
        QList<Reader*> readers;
        
        const std::vector<Node*>& inputs = _dag.getInputs();
        for(U32 j=0;j<inputs.size();++j) {
            Node* currentInput=inputs[j];
            if(currentInput->className() == string("Reader")){
                Reader* inp = static_cast<Reader*>(currentInput);
                inp->fitFrameToViewer(_lastRunArgs._fitToViewer);
                readers << inp;
            }
        }
        
        
        QList<bool> readHeaderResults = QtConcurrent::blockingMapped(readers,boost::bind(metaReadHeader,_1,currentFrame));
        for (int i = 0; i < readHeaderResults.size(); i++) {
            if (readHeaderResults.at(i) == false) {
                stopEngine();
                return;
            }
        }
        
        _dag.validate(true);
        
        const Format &_dispW = _dag.getOutput()->getInfo()->getDisplayWindow();
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode() && _lastRunArgs._fitToViewer){
            gl_viewer->fitToFormat(_dispW);
            _lastRunArgs._zoomFactor = gl_viewer->getZoomFactor();
        }
        /*Now that we called validate we can check if the frame is in the cache
         and return the appropriate EngineStatus code.*/
        vector<int> rows;
        vector<int> columns;
        int x=0,r=0;
        const Box2D& dataW = _dag.getOutput()->getInfo()->getDataWindow();
        FrameEntry* iscached= 0;
        U64 key = 0;
        float lut = 0.f;
        float exposure = 0.f;
        float zoomFactor = 0.f;
        float byteMode = 0.f;
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            gl_viewer->drawing(true);
            
            std::pair<int,int> rowSpan = gl_viewer->computeRowSpan(rows,_dispW);
            std::pair<int,int> columnSpan = gl_viewer->computeColumnSpan(columns, _dispW);
            
            TextureRect textureRect(columnSpan.first,rowSpan.first,columnSpan.second,rowSpan.second,columns.size(),rows.size());
            
            /*Now checking if the frame is already in either the ViewerCache*/
            _lastFrameInfos._textureRect = textureRect;
            if(textureRect.w == 0 || textureRect.h == 0){
                stopEngine();
                return;
            }
            zoomFactor = _lastRunArgs._zoomFactor;
            exposure = gl_viewer->getExposure();
            lut =  gl_viewer->lutType();
            byteMode = gl_viewer->byteMode();
            key = FrameEntry::computeHashKey(currentFrame,
                                             _treeVersion,
                                             zoomFactor,
                                             exposure,
                                             lut,
                                             byteMode,
                                             dataW,
                                             _dispW,
                                             textureRect);
            x = columnSpan.first;
            r = columnSpan.second+1;
            
            
            iscached = viewer->get(key);
            
            /*Found in viewer cache, we execute the cached engine and leave*/
            if(iscached){
                _lastFrameInfos._cachedEntry = iscached;

                /*Checking that the entry retrieve matches absolutely what we
                 asked for.*/
                assert(iscached->_textureRect == textureRect);
                assert(iscached->_treeVers == _treeVersion);
                // assert(iscached->_zoom == _viewerCacheArgs._zoomFactor);
                assert(iscached->_lut == lut);
                assert(iscached->_exposure == exposure);
                assert(iscached->_byteMode == byteMode);
                assert(iscached->_frameInfo->getDisplayWindow() == _dispW);
                assert(iscached->_frameInfo->getDataWindow() == dataW);
                
                _lastFrameInfos._textureRect = iscached->_textureRect;
                
                _mutex->lock();
                emit doCachedEngine();
                _openGLCondition->wait(_mutex);
                _mutex->unlock();
                _lastFrameInfos._cachedEntry->removeReference(); // the cached engine has finished using this frame
                engineLoop();
                continue;
            }
            
        }else{
            for (int i = dataW.y(); i < dataW.top(); ++i) {
                rows.push_back(i);
            }
            x = dataW.x();
            r = dataW.right();
        }
        /*If it reaches here, it means the frame neither belong
         to the ViewerCache, we must
         allocate resources and render the frame.
         If this is a recursive call, we explicitly fallback
         to the viewer cache storage as the texture cache is not
         meant for playback.*/
        _lastFrameInfos._rows = rows;
        
        QtConcurrent::blockingMap(readers,boost::bind(metaReadData,_1,currentFrame));
        if (_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()) {
            viewer->makeCurrentViewer();
            _mutex->lock();
            emit doFrameStorageAllocation();
            _openGLCondition->wait(_mutex);
            _mutex->unlock();
        }
        
        if(!_lastRunArgs._sameFrame){
            NodeCache::getNodeCache()->clear();
        }
        ChannelSet outChannels;
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            outChannels = viewer->getUiContext()->displayChannels();
        }
        else{// channels requested are those requested by the user
            if(!_dag.isOutputAnOpenFXNode()){
                outChannels = writer->requestedChannels();
            }else{
                //openfx outputs can only output RGBA
                outChannels = Mask_RGBA;
            }
        }
        
        int counter = 0;
        gettimeofday(&_lastComputeFrameTime, 0);
        _sequence.clear();
        for (vector<int>::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
            Row* row = new Row(x,*it,r,outChannels);
            row->zoomedY(counter);
            // RowRunnable* worker = new RowRunnable(row,_dag.getOutput());
//            if(counter%10 == 0){
//            // UNCOMMENT to report progress.
//                QObject::connect(worker, SIGNAL(finished(int,int)), this ,SLOT(checkAndDisplayProgress(int,int)),Qt::QueuedConnection);
//            }
            _sequence.push_back(row);
            //  _threadPool->start(worker);
            ++counter;
        }
        _workerThreadsWatcher->setFuture(QtConcurrent::map(_sequence,boost::bind(metaEnginePerRow,_1,_dag.getOutput())));
        _workerThreadsWatcher->waitForFinished();
        //_threadPool->waitForDone();
        
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            //copying the frame data stored into the PBO to the viewer cache if it was a normal engine
            //This is done only if we run a sequence (i.e: playback) because the viewer cache isn't meant for
            //panning/zooming.
            gl_viewer->stopDisplayingProgressBar();
            if(!_lastRunArgs._sameFrame){
                FrameEntry* entry = ViewerCache::getViewerCache()->addFrame(key,
                                                                            _treeVersion,
                                                                            zoomFactor,
                                                                            exposure,
                                                                            lut,
                                                                            byteMode,
                                                                            _lastFrameInfos._textureRect,
                                                                            dataW,
                                                                            _dispW);
                
                if(entry){
                    memcpy(entry->getMappedFile()->data(),gl_viewer->getFrameData(),_lastFrameInfos._dataSize);
                    entry->removeReference(); // removing reference as we're done with the entry.
                }
            }
            
        }else if(!_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            /*if the output is a writer we actually start writing on disk now*/
            _dag.outputAsWriter()->startWriting();
        }
        engineLoop();
        
    }
    
}
void VideoEngine::onProgressUpdate(int i){
    // cout << "progress: index = " << i ;
    if(i < (int)_lastFrameInfos._rows.size()){
        //   cout <<" y = "<< _lastFrameInfos._rows[i] << endl;
        checkAndDisplayProgress(_lastFrameInfos._rows[i],i);
    }
}

void VideoEngine::engineLoop(){
    if(_lastRunArgs._frameRequestIndex == 0 && _lastRunArgs._frameRequestsCount == 1 && !_lastRunArgs._sameFrame){
        _lastRunArgs._frameRequestsCount = 0;
    }else if(_lastRunArgs._frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
        --_lastRunArgs._frameRequestsCount;
    }
    ++_lastRunArgs._frameRequestIndex;//incrementing the frame counter
    
    if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        _mutex->lock();
        emit doUpdateViewer();
        _openGLCondition->wait(_mutex);
        _mutex->unlock();
        
    }
    _lastRunArgs._fitToViewer = false;
    _lastRunArgs._recursiveCall = true;
    if(_autoSaveOnNextRun){
        _autoSaveOnNextRun = false;
        appPTR->autoSave();
    }

}

void VideoEngine::updateViewer(){
    _mutex->lock();
    gl_viewer->copyPBOToRenderTexture(_lastFrameInfos._textureRect); // returns instantly
    _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
    if((_lastRunArgs._frameRequestIndex%24)==0){
        emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
    }
    _lastRunArgs._zoomFactor = gl_viewer->getZoomFactor();
    
    updateDisplay(); // updating viewer & pixel aspect ratio if needed
    _openGLCondition->wakeOne();
    _mutex->unlock();
}
void VideoEngine::cachedEngine(){
    _mutex->lock();
    _dag.outputAsViewer()->cachedFrameEngine(_lastFrameInfos._cachedEntry);
    _openGLCondition->wakeOne();
    _mutex->unlock();

}
void VideoEngine::allocateFrameStorage(){
    _mutex->lock();
    _lastFrameInfos._dataSize = gl_viewer->allocateFrameStorage(_lastFrameInfos._textureRect.w,
                                               _lastFrameInfos._textureRect.h);
    _openGLCondition->wakeOne();
    _mutex->unlock();
}







void VideoEngine::drawOverlay() const{
    if(_dag.getOutput())
        _drawOverlay(_dag.getOutput());
}

void VideoEngine::_drawOverlay(Node *output) const{
    output->drawOverlay();
    foreach(Node* n,output->getParents()){
        _drawOverlay(n);
    }
    
}


void RowRunnable::run() {
    _output->engine(_row->y(), _row->offset(), _row->right(), _row->channels(), _row);
    emit finished(_row->y(),_row->zoomedY());
    delete _row;
}


void VideoEngine::updateProgressBar(){
    //update progress bar
}
void VideoEngine::updateDisplay(){
    int width = gl_viewer->width();
    int height = gl_viewer->height();
    double ap = gl_viewer->displayWindow().pixel_aspect();
    if(ap > 1.f){
        glViewport (0, 0, width*ap, height);
    }else{
        glViewport (0, 0, width, height/ap);
    }
    gl_viewer->updateGL();
}

void VideoEngine::startEngine(int nbFrames){
    if (dagHasInputs()) {
        render(nbFrames,true,true);
        
    }
}
void VideoEngine::repeatSameFrame(){
    if (dagHasInputs()) {
        if(_working){
            appendTask(currentViewer->currentFrame(), 1, false,_lastRunArgs._forward,true, _dag.getOutput(),&VideoEngine::_startEngine);
        }else{
            render(1,false,true,true);
        }
    }
}

VideoEngine::VideoEngine(QWaitCondition* openGLCondition,QMutex* mutex,QObject* parent):QThread(parent),
_working(false),
_aborted(false),
_paused(true),
_loopMode(true),
_autoSaveOnNextRun(false),
_openGLCondition(openGLCondition),
_mutex(mutex)
{
    
    connect(this,SIGNAL(doUpdateViewer()),this,SLOT(updateViewer()));
    connect(this,SIGNAL(doCachedEngine()),this,SLOT(cachedEngine()));
    connect(this,SIGNAL(doFrameStorageAllocation()),this,SLOT(allocateFrameStorage()));
    
    _workerThreadsWatcher = new QFutureWatcher<void>;
    connect(_workerThreadsWatcher, SIGNAL(progressValueChanged(int)), this, SLOT(onProgressUpdate(int)),Qt::QueuedConnection);
    /*Adjusting multi-threading for OpenEXR library.*/
    Imf::setGlobalThreadCount(QThread::idealThreadCount());
    
    _timer=new Timer();
    //_threadPool = new QThreadPool;
    
    
    
}

VideoEngine::~VideoEngine(){
    _workerThreadsWatcher->waitForFinished();
    //  _threadPool->waitForDone();
    _mutex->lock();
    _aborted = true;
    _startCondition.wakeOne();
    _mutex->unlock();
    wait();
    delete _workerThreadsWatcher;
    //  delete _threadPool;
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
    _workerThreadsWatcher->cancel();
    _mutex->lock();
    _aborted=true;
    quit();
    // _startCondition.wakeOne();
    _mutex->unlock();
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
        render(-1,false,true);
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
        render(-1,false,false);
    }else{
        pause();
    }
}
void VideoEngine::previousFrame(){
    if( _working){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->currentFrame()-1, 1, false,false,false);
    else
        appendTask(currentViewer->currentFrame()-1, 1,  false,_lastRunArgs._forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
}

void VideoEngine::nextFrame(){
    if(_working){
        pause();
    }
    
    if(!_working)
        _startEngine(currentViewer->currentFrame()+1, 1, false,true,false);
    else
        appendTask(currentViewer->currentFrame()+1,  1,false,_lastRunArgs._forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
}

void VideoEngine::firstFrame(){
    if( _working){
        pause();
    }
    
    if(!_working)
        _startEngine(currentViewer->firstFrame(), 1, false,false,false);
    else
        appendTask(currentViewer->firstFrame(), 1,  false,_lastRunArgs._forward,false,_dag.getOutput(),  &VideoEngine::_startEngine);
}

void VideoEngine::lastFrame(){
    if(_working){
        pause();
    }
    if(!_working)
        _startEngine(currentViewer->lastFrame(), 1, false,true,false);
    else
        appendTask(currentViewer->lastFrame(), 1,  false,_lastRunArgs._forward,false,_dag.getOutput(),  &VideoEngine::_startEngine);
}

void VideoEngine::previousIncrement(){
    if(_working){
        pause();
    }
    int frame = currentViewer->currentFrame()- currentViewer->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,false,false);
    else{
        appendTask(frame,1, false,_lastRunArgs._forward,false,_dag.getOutput(), &VideoEngine::_startEngine);
    }
    
    
}

void VideoEngine::nextIncrement(){
    if(_working){
        pause();
    }
    int frame = currentViewer->currentFrame()+currentViewer->getUiContext()->incrementSpinBox->value();
    if(!_working)
        _startEngine(frame, 1, false,true,false);
    else
        appendTask(frame,1, false,_lastRunArgs._forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
}

void VideoEngine::seekRandomFrame(int f){
    if(!_dag.getOutput() || _dag.getInputs().size()==0) return;
    
//            if(_lastRunArgs._frameRequestsCount == -1){
//                _startEngine(f, -1, false,_lastRunArgs._forward,false);
//            }else{
//                _startEngine(f, 1, false,_lastRunArgs._forward,false);
//            }
    if(_working){
        pause();
    }
    
    if(!_working){
        if(_lastRunArgs._frameRequestsCount == -1){
            _startEngine(f, -1, false,_lastRunArgs._forward,false);
        }else{
            _startEngine(f, 1, false,_lastRunArgs._forward,false);
        }
    }
    else{
        if(_lastRunArgs._frameRequestsCount == -1){
            appendTask(f, -1, false,_lastRunArgs._forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
        }else{
            appendTask(f, 1, false,_lastRunArgs._forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
        }
    }
}
void VideoEngine::recenterViewer(){
    if(_working){
        pause();
    }
    if(!_working){
        if(_lastRunArgs._frameRequestsCount == -1)
            _startEngine(currentViewer->currentFrame(), -1, true,_lastRunArgs._forward,false);
        else
            _startEngine(currentViewer->currentFrame(), 1, true,_lastRunArgs._forward,false);
    }else{
        appendTask(currentViewer->currentFrame(), -1, true,_lastRunArgs._forward,false, _dag.getOutput(),&VideoEngine::_startEngine);
    }
}



void VideoEngine::changeDAGAndStartEngine(Node* output){
    pause();
    if(!_working){
        if(_dag.getOutput()){
            if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
                _changeDAGAndStartEngine(currentViewer->currentFrame(), -1, false,true,false,output);
            }else if(!_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
                _changeDAGAndStartEngine(_dag.outputAsWriter()->currentFrame(), -1, false,true,false,output);
            }else if(_dag.isOutputAnOpenFXNode()){
                _changeDAGAndStartEngine(_dag.outputAsOpenFXNode()->currentFrame(), -1, false,true,false,output);
            }
        }else{
            _changeDAGAndStartEngine(0, -1, false,true,false,output);
        }
    }else{
        if(_dag.getOutput()){
            if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
                appendTask(currentViewer->currentFrame(), 1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
            }else if(!_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
                appendTask(_dag.outputAsWriter()->currentFrame(),1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
            }else if(_dag.isOutputAnOpenFXNode()){
                appendTask(_dag.outputAsOpenFXNode()->currentFrame(),1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
            }
        }else{
            appendTask(0,1, false,true,true, output, &VideoEngine::_changeDAGAndStartEngine);
        }
    }
}

void VideoEngine::appendTask(int frameNB, int frameCount, bool initViewer,bool forward,bool sameFrame,Node* output, VengineFunction func){
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

void VideoEngine::_startEngine(int frameNB,int frameCount,bool initViewer,bool forward,bool sameFrame,Node* ){
    if(_dag.getOutput() && _dag.getInputs().size()>0){
        if(frameNB < currentViewer->firstFrame() || frameNB > currentViewer->lastFrame())
            return;
        currentViewer->getUiContext()->frameSeeker->seek_notSlot(frameNB);
        render(frameCount,initViewer,forward,sameFrame);
        
    }
}

void VideoEngine::_changeDAGAndStartEngine(int , int frameCount, bool initViewer,bool,bool sameFrame,Node* output){
    bool isViewer = false;
    if(dynamic_cast<ViewerNode*>(output)){
        isViewer = true;
    }
    _dag.resetAndSort(output,isViewer);
    const vector<Node*>& inputs = _dag.getInputs();
    bool start = false;
    for (U32 i = 0 ; i < inputs.size(); ++i) {
        if(inputs[i]->className() == "Reader"){
            Reader* reader = dynamic_cast<Reader*>(inputs[i]);
            if(reader->hasFrames()) start = true;
            else{
                start = false;
                break;
            }
        }else{
            if(inputs[0]->isInputNode()){
                start = true;
            }
        }
    }
    if(start)
        render(frameCount,initViewer,_lastRunArgs._forward,sameFrame);
}



void VideoEngine::changeTreeVersion(){
    
    if(!_dag.getOutput()){
        return;
    }
    Node* output = _dag.getOutput();
    std::vector<std::string> v;
    output->computeTreeHash(v);
    _treeVersion = output->getHash()->getHashValue();
    
}


void VideoEngine::DAG::fillGraph(Node* n){
    if(std::find(_graph.begin(),_graph.end(),n)==_graph.end()){
        n->setMarked(false);
        _graph.push_back(n);
        if(n->isInputNode()){
            _inputs.push_back(n);
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
    for(U32 i = 0 ; i < _graph.size(); ++i) {
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

ViewerNode* VideoEngine::DAG::outputAsViewer() const {
    if(_output && _isViewer)
        return dynamic_cast<ViewerNode*>(_output);
    else
        return NULL;
}

Writer* VideoEngine::DAG::outputAsWriter() const {
    if(_output && !_isViewer)
        return dynamic_cast<Writer*>(_output);
    else
        return NULL;
}
OfxNode* VideoEngine::DAG::outputAsOpenFXNode() const{
    if(_output && _isOutputOpenFXNode){
        return dynamic_cast<OfxNode*>(_output);
    }else{
        return NULL;
    }
}

void VideoEngine::DAG::resetAndSort(Node* out,bool isViewer){
    _output = out;
    _isViewer = isViewer;
    if(out && out->isOpenFXNode()){
        _isOutputOpenFXNode = true;
    }else{
        _isOutputOpenFXNode = false;
    }
    clearGraph();
    if(!_output){
        return;
    }
    fillGraph(out);
    
    topologicalSort();
}
void VideoEngine::DAG::debug(){
    cout << "Topological ordering of the DAG is..." << endl;
    for(DAG::DAGIterator it = begin(); it != end() ;++it) {
        cout << (*it)->getName() << endl;
    }
}

/*sets infos accordingly across all the DAG*/
bool VideoEngine::DAG::validate(bool forReal){
    /*Validating the DAG in topological order*/
    for (DAGIterator it = begin(); it!=end(); ++it) {
        if(!(*it)->validate(forReal))
            return false;
    }
    return true;
}


int VideoEngine::DAG::firstFrame() const {
    return _output->getInfo()->firstFrame();
}
int VideoEngine::DAG::lastFrame() const{
    return _output->getInfo()->lastFrame();
}



void VideoEngine::resetAndMakeNewDag(Node* output,bool isViewer){
    _dag.resetAndSort(output,isViewer);
}
#ifdef POWITER_DEBUG
bool VideoEngine::rangeCheck(const std::vector<int>& columns,int x,int r){
    for (unsigned int i = 0; i < columns.size(); ++i) {
        if(columns[i] < x || columns[i] > r){
            return false;
        }
    }
    return true;
}
#endif



bool VideoEngine::checkAndDisplayProgress(int y,int zoomedY){
    timeval now;
    gettimeofday(&now, 0);
    double t =  now.tv_sec  - _lastComputeFrameTime.tv_sec +
    (now.tv_usec - _lastComputeFrameTime.tv_usec) * 1e-6f;
    if(t >= 0.3){
        //   cout << zoomedY << endl;
        gl_viewer->updateProgressOnViewer(_lastFrameInfos._textureRect, y,zoomedY);
        return true;
    }else{
        return false;
    }
}
