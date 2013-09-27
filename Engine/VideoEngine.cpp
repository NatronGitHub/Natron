

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

#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Model.h"
#include "Engine/Hash64.h"
#include "Engine/Lut.h"
#include "Engine/ViewerCache.h"
#include "Engine/NodeCache.h"
#include "Engine/Row.h"
#include "Engine/MemoryFile.h"
#include "Engine/Timer.h"
#include "Engine/TimeLine.h"
#include "Writers/Writer.h"
#include "Readers/Reader.h"


#include "Gui/Gui.h"
#include "Gui/ViewerTab.h"
#include "Gui/SpinBox.h"
#include "Gui/ViewerGL.h"
#include "Gui/Button.h"


#include "Global/AppManager.h"
#include "Global/MemoryInfo.h"



using namespace std;
using namespace Powiter;

VideoEngine::VideoEngine(Model* model,QObject* parent)
: QThread(parent)
, _model(model)
, _dag()
, _timer(new Timer)
, _abortBeingProcessedLock()
, _abortBeingProcessed(false)
, _abortedRequestedCondition()
, _abortedRequestedMutex()
, _abortRequested(0)
, _mustQuitMutex()
, _mustQuit(false)
, _treeVersion(0)
, _treeVersionValid(false)
, _loopMode(true)
, _restart(true)
, _forceRender(false)
, _workerThreadsWatcher(new QFutureWatcher<void>)
, _pboUnMappedCondition()
, _pboUnMappedMutex()
, _pboUnMappedCount(0)
, _startCondition()
, _startMutex()
, _startCount(0)
, _lastRunArgs()
, _lastFrameInfos()
, _lastComputeFrameTime()
{
    
    connect(this,SIGNAL(doUpdateViewer()),this,SLOT(updateViewer()));
    connect(this,SIGNAL(doCachedEngine()),this,SLOT(cachedEngine()));
    connect(this,SIGNAL(doFrameStorageAllocation()),this,SLOT(allocateFrameStorage()));
    connect(_workerThreadsWatcher, SIGNAL(progressValueChanged(int)), this, SLOT(onProgressUpdate(int)));
    
}

VideoEngine::~VideoEngine() {
    assert(_workerThreadsWatcher);
    _workerThreadsWatcher->cancel();
    _workerThreadsWatcher->waitForFinished();
    {
        QMutexLocker locker(&_abortedRequestedMutex);
        ++_abortRequested;
        for (DAG::DAGIterator it = _dag.begin(); it != _dag.end(); ++it) {
            (*it)->setAborted(true);
        }
        _abortedRequestedCondition.wakeOne();
    }
    {
        QMutexLocker locker(&_startMutex);
        ++_startCount;
        _startCondition.wakeOne();
    }
    wait();
    delete _workerThreadsWatcher;
}


/**
 *@brief The callback reading the header of the current frame for a reader.
 *@param reader[in] A pointer to the reader that will read the header.
 *@param current_frame[in] The frame number in the sequence to decode.
 */

static bool metaReadHeader(Reader* reader,int current_frame) {
    assert(reader);
    if(!reader->readCurrentHeader(current_frame))
        return false;
    return true;
}

/**
 *@brief The callback reading the data of the current frame for a reader.
 *@param reader[in] A pointer to the reader that will read the data.
 *@param current_frame[in] The frame number in the sequence to decode.
 */
static void metaReadData(Reader* reader,int current_frame) {
    assert(reader);
    reader->readCurrentData(current_frame);
}

/**
 *@brief The callback cycling through the DAG for one scan-line
 *@param row[in] The row to compute. Note that after that function row will be deleted and cannot be accessed any longer.
 *@param output[in] The output node of the graph.
 */
static void metaEnginePerRow(Row* row, Node* output){
    assert(row);
    assert(output);
    row->lock();
    output->engine(row->y(), row->offset(), row->right(), row->channels(), row);
    delete row;
    // QMetaObject::invokeMethod(_engine, "onProgressUpdate", Qt::QueuedConnection, Q_ARG(int, zoomedY));
}

void VideoEngine::render(OutputNode* output, int startingFrame, int frameCount, bool fitFrameToViewer, bool forward, bool sameFrame) {
    assert(output);
    {
        QMutexLocker startedLocker(&_startMutex);
        /*aborting any rendering on-going*/
        if(_startCount > 0)
            abort();
    }
    /*seek the timeline to the starting frame*/
    output->seekFrame(startingFrame);
    
    double zoomFactor;
    if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
        ViewerNode* viewer = _dag.outputAsViewer();
        assert(viewer);
        assert(viewer->getUiContext());
        assert(viewer->getUiContext()->viewer);
        zoomFactor = viewer->getUiContext()->viewer->getZoomFactor();

    }else{
        zoomFactor = 1.f;
    }
    
    /*setting the run args that are used by the run function*/
    _lastRunArgs._zoomFactor = zoomFactor;
    _lastRunArgs._sameFrame = sameFrame;
    _lastRunArgs._fitToViewer = fitFrameToViewer;
    _lastRunArgs._recursiveCall = false;
    _lastRunArgs._forward = forward;
    _lastRunArgs._frameRequestsCount = frameCount;
    _lastRunArgs._frameRequestIndex = 0;
    _lastRunArgs._output = output;
    
    /*Starting or waking-up the thread*/
    if (!isRunning()) {
        start(HighestPriority);
    } else {
        QMutexLocker locker(&_startMutex);
        ++_startCount;
        _startCondition.wakeOne();
    }
}
void VideoEngine::startEngine(){
    /*wait if something is already running until it's aborted*/
    {
        QMutexLocker abortProcessLocker(&_abortBeingProcessedLock);
        assert(!_abortBeingProcessed);
        {
            QMutexLocker l(&_abortedRequestedMutex);
            while(_abortRequested > 0) {
                cout << "waiting for an aborted render to finish" << endl;
                _abortedRequestedCondition.wait(&_abortedRequestedMutex);
            }
        }
    }
    
    _restart = false; /*we just called startEngine,we don't want to recall this function for the next frame in the sequence*/
    _timer->playState = RUNNING; /*activating the timer*/
    _dag.resetAndSort(_lastRunArgs._output);/*rebuilding the dag from the output provided*/
    
    
    ViewerNode* viewer = dynamic_cast<ViewerNode*>(_dag.getOutput()); /*viewer might be NULL if the output is smthing else*/
    const std::vector<Node*>& inputs = _dag.getInputs();
    bool hasFrames = false;
    bool hasInputDifferentThanReader = false;
    for (U32 i = 0; i< inputs.size(); ++i) {
        assert(inputs[i]);
        Reader* r = dynamic_cast<Reader*>(inputs[i]);
        if (r) {
            if (r->hasFrames()) {
                hasFrames = true;
            }
        }else{
            hasInputDifferentThanReader = true;
        }
    }
    /*if there's no inputs or there's no input with frames to provide, disconnect the viewer associated*/
    if(!hasInputDifferentThanReader && !hasFrames){
        if(viewer)
            viewer->disconnectViewer();
        stopEngine();
        return;
    }
    
    if(!_dag.validate(false)){ // < validating sequence (mostly getting the same frame range for all nodes).
        stopEngine();
        return;
    }

    /*update the tree hash */
    bool oldVersionValid = _treeVersionValid;
    U64 oldVersion;
    if (oldVersionValid) {
        oldVersion = getCurrentTreeVersion();
    }
    updateTreeVersion();
    
    /*If the DAG changed we clear the playback cache.*/
    if(!oldVersionValid || (_treeVersion != oldVersion)){
         // FIXME: the playback cache seems to be global to the application, why should we clear it here?
        //because the DAG changed, so the frame stored in memory are now useless for the playback.
        _model->getApp()->clearPlaybackCache();
    }
    emit engineStarted(_lastRunArgs._forward);

}
void VideoEngine::stopEngine() {
    /*reset the abort flag and wake up any thread waiting*/
    {
        QMutexLocker abortProcessLocker(&_abortBeingProcessedLock);
        _abortBeingProcessed = true;
        QMutexLocker l(&_abortedRequestedMutex);
        _abortRequested = 0;
        cout << "abort processed" << endl;
        for (DAG::DAGIterator it = _dag.begin(); it != _dag.end(); ++it) {
            (*it)->setAborted(false);
        }
        _abortedRequestedCondition.wakeOne();
        _abortBeingProcessed = false;
    }
    
    emit engineStopped();
    _lastRunArgs._frameRequestsCount = 0;
    _timer->playState=PAUSE;
    _model->getGeneralMutex()->unlock();
    _restart = true;
    
    /*pause the thread if needed*/
    {
        QMutexLocker locker(&_startMutex);
        while(_startCount <= 0) {
            _startCondition.wait(&_startMutex);
        }
        --_startCount;
    }
   
    
}

void VideoEngine::run(){
    
    for(;;){ // infinite loop
        {
            /*First-off, check if the node holding this engine got deleted
             in which case we must quit the engine.*/
            QMutexLocker locker(&_mustQuitMutex);
            if(_mustQuit) {
                return;
            }
        }
        
        /*Locking out other rendering tasks so 1 VideoEngine gets access to all
         nodes.That means only 1 frame can be rendered at any time. We would have to copy
         all the nodes that have a varying state (such as Readers/Writers) for every VideoEngine
         running simultaneously which is not very efficient and adds the burden to synchronize
         states of nodes etc...
         To be lock free you can move a rendering task to a new project and start rendering in the new window.*/
        _model->getGeneralMutex()->lock();
        
        /*If restart is on, start the engine. Restart is on for the 1st frame
         rendered of a sequence.*/
        if(_restart){
            startEngine();
        }

        /*beginRenderAction for all openFX nodes*/
        for (DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            OfxNode* n = dynamic_cast<OfxNode*>(*it);
            if(n) {
                OfxPointD renderScale;
                renderScale.x = renderScale.y = 1.0;
                assert(n->effectInstance());
                OfxStatus stat;
                stat = n->effectInstance()->beginRenderAction(0, 25, 1, true,renderScale);
                assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            }
        }
        
        /*get the frame range*/
        int firstFrame = INT_MAX,lastFrame = INT_MIN, currentFrame = 0;
        Writer* writer = _dag.outputAsWriter();
        ViewerNode* viewer = _dag.outputAsViewer();
        OfxNode* ofxOutput = _dag.outputAsOpenFXNode();
        
        /*If the output is not a Viewer, get the frame range*/
        if (!_dag.isOutputAViewer()) {
            if(!_dag.isOutputAnOpenFXNode()){
                assert(writer);
                if (!_lastRunArgs._recursiveCall) {
                    lastFrame = writer->lastFrame();
                    currentFrame = writer->firstFrame();
                    writer->seekFrame(writer->firstFrame());
                } else {
                    lastFrame = writer->lastFrame();
                    writer->incrementCurrentFrame();
                    currentFrame = writer->currentFrame();
                }
            } else {
                assert(ofxOutput);
                if (!_lastRunArgs._recursiveCall) {
                    lastFrame = ofxOutput->lastFrame();
                    currentFrame = ofxOutput->firstFrame();
                    ofxOutput->seekFrame(ofxOutput->firstFrame());
                } else {
                    lastFrame = ofxOutput->lastFrame();
                    ofxOutput->incrementCurrentFrame();
                    currentFrame = ofxOutput->currentFrame();
                }
            }
        }
        
        /*Check whether we need to stop the engine or not for various reasons.
         */
        if(_abortRequested || // #1 aborted by the user
           
           (_dag.isOutputAViewer() // #2 the DAG contains only 1 frame and we rendered it
            &&  _lastRunArgs._recursiveCall
            && _dag.lastFrame() == _dag.firstFrame()
            && _lastRunArgs._frameRequestsCount == -1
            && _lastRunArgs._frameRequestIndex == 1)
           
           || _lastRunArgs._frameRequestsCount == 0 // #3 the sequence ended and it was not an infinite run
           || (!_dag.isOutputAViewer() && currentFrame == lastFrame+1)){//#4 the sequence ended and it was an infinite run (for writers)

            stopEngine();
            continue;
        }
        
        
        /*Getting the frame range if it was not a writer*/
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){ //openfx viewers are UNSUPPORTED
            assert(viewer);
            /*Determine what is the current frame when output is a viewer*/
            /*!recursiveCall means this is the first time it's called for the sequence.*/
            if (!_lastRunArgs._recursiveCall) {
                currentFrame = viewer->currentFrame();
                firstFrame = _dag.firstFrame();
                lastFrame = _dag.lastFrame();
                
                /*clamping the current frame to the range [first,last] if it wasn't*/
                if(currentFrame < firstFrame){
                    currentFrame = firstFrame;
                }
                else if(currentFrame > lastFrame){
                    currentFrame = lastFrame;
                }
                viewer->seekFrame(currentFrame);
                
            } else { // if the call is recursive, i.e: the next frame in the sequence
                /*clear the node cache, as it is very unlikely the user will re-use
                 data from previous frame.*/
                lastFrame = _dag.lastFrame();
                firstFrame = _dag.firstFrame();
                if (_lastRunArgs._forward) {
                    currentFrame = viewer->currentFrame()+1;
                    if(currentFrame > lastFrame){
                        if(_loopMode)
                            currentFrame = firstFrame;
                        else{
                            stopEngine();
                            return;
                        }
                    }
                } else {
                    currentFrame  = viewer->currentFrame()-1;
                    if(currentFrame < firstFrame){
                        if(_loopMode)
                            currentFrame = lastFrame;
                        else{
                            stopEngine();
                            return;
                        }
                    }
                }
                viewer->seekFrame(currentFrame);
            }
        }
        
        /*Getting all readers of the DAG + flagging that we'll want to fit the frame to the viewer*/
        QList<Reader*> readers;
        const std::vector<Node*>& inputs = _dag.getInputs();
        for(U32 j=0;j<inputs.size();++j) {
            Node* currentInput=inputs[j];
            assert(currentInput);
            if(currentInput->className() == string("Reader")){
                Reader* inp = static_cast<Reader*>(currentInput);
                assert(inp);
                inp->fitFrameToViewer(_lastRunArgs._fitToViewer);
                readers << inp;
            }
        }
        
        /*Read all the headers of all the files to read*/
        QList<bool> readHeaderResults = QtConcurrent::blockingMapped(readers,boost::bind(metaReadHeader,_1,currentFrame));
        for (int i = 0; i < readHeaderResults.size(); i++) {
            if (readHeaderResults.at(i) == false) {
                stopEngine();
                {
                    QMutexLocker locker(&_startMutex);
                    while(_startCount <= 0) {
                        _startCondition.wait(&_startMutex);
                    }
                    --_startCount;
                }
                continue;
            }
        }
        
        /*Validate the infos that has been read and pass 'em down the tree*/
        _dag.validate(true);
        
        /*Fit the frame to the viewer if this was requested by the call to render()*/
        OutputNode* outputNode = _dag.getOutput();
        assert(outputNode);
        const Format &_dispW = outputNode->info().displayWindow();
        if (_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()) {
            assert(viewer);
            if (_lastRunArgs._fitToViewer) {
                assert(viewer);
                assert(viewer->getUiContext());
                ViewerGL* viewerGL = viewer->getUiContext()->viewer;
                assert(viewerGL);
                assert(_dispW.height() > 0. && _dispW.width() > 0);
                viewerGL->fitToFormat(_dispW);
                _lastRunArgs._zoomFactor = viewerGL->getZoomFactor();
            }
            
        }
        
        /*Now that we called validate we can check if the frame is in the cache (only if the output is a Viewer)*/
        vector<int> rows;
        vector<int> columns;
        int x=0,r=0;
        const Box2D& dataW = outputNode->info().dataWindow();
        FrameEntry* iscached= 0;
        U64 key = 0;
        float lut = 0.f;
        float exposure = 0.f;
        float zoomFactor = 0.f;
        float byteMode = 0.f;
        QString inputFileNames;
        if(_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()){
            assert(viewer);
            assert(viewer->getUiContext());
            ViewerGL* viewerGL = viewer->getUiContext()->viewer;
            assert(viewerGL);
            viewerGL->setDisplayingImage(true);
            
            std::pair<int,int> rowSpan = viewerGL->computeRowSpan(_dispW, &rows);
            std::pair<int,int> columnSpan = viewerGL->computeColumnSpan(_dispW, &columns);
            
            TextureRect textureRect(columnSpan.first,rowSpan.first,columnSpan.second,rowSpan.second,columns.size(),rows.size());
            
            /*Now checking if the frame is already in either the ViewerCache*/
            _lastFrameInfos._textureRect = textureRect;
            if(textureRect.w == 0 || textureRect.h == 0){
                stopEngine();
                return;
            }
            zoomFactor = _lastRunArgs._zoomFactor;
            exposure = viewerGL->getExposure();
            lut =  viewerGL->lutType();
            byteMode = viewerGL->byteMode();
            inputFileNames = _dag.generateConcatenationOfAllReadersFileNames();
            key = FrameEntry::computeHashKey(currentFrame,
                                             inputFileNames,
                                             getCurrentTreeVersion(),
                                             zoomFactor,
                                             exposure,
                                             lut,
                                             byteMode,
                                             dataW,
                                             _dispW,
                                             textureRect);
            x = columnSpan.first;
            r = columnSpan.second+1;
            
            if(!_forceRender){
                iscached = appPTR->getViewerCache()->get(key);
            }else{
                _forceRender = false;
            }
            /*Found in viewer cache, we execute the cached engine and leave*/
            if (iscached) {
                _lastFrameInfos._cachedEntry = iscached;
                
                /*Checking that the entry retrieve matches absolutely what we
                 asked for.*/
                assert(iscached->_textureRect == textureRect);
                assert(iscached->_treeVers == getCurrentTreeVersion());
                // assert(iscached->_zoom == _viewerCacheArgs._zoomFactor);
                assert(iscached->_lut == lut);
                assert(iscached->_exposure == exposure);
                assert(iscached->_byteMode == byteMode);
                assert(iscached->_frameInfo->displayWindow() == _dispW);
                assert(iscached->_frameInfo->dataWindow() == dataW);
                assert(iscached->_frameInfo->getCurrentFrameName() == inputFileNames.toStdString());
                
                _lastFrameInfos._textureRect = iscached->_textureRect;
                
                {
                    QMutexLocker locker(&_pboUnMappedMutex);
                    emit doCachedEngine();
                    while(_pboUnMappedCount <= 0) {
                        _pboUnMappedCondition.wait(&_pboUnMappedMutex);
                    }
                    --_pboUnMappedCount;
                }
                assert(iscached);
                iscached->removeReference(); // the cached engine has finished using this frame
                iscached->unlock();
                emit frameRendered(currentFrame);
                engineLoop();
                _model->getGeneralMutex()->unlock();
                continue;
            }
            
        }else{
            for (int i = dataW.bottom(); i < dataW.top(); ++i) {
                rows.push_back(i);
            }
            x = dataW.left();
            r = dataW.right();
        }
        /*If it reaches here, it means the frame doesn't belong
         to the ViewerCache, we must
         allocate resources and render the frame.*/
        /*****************************************************************************************************/
        /*****************************COMPUTING FRAME*********************************************************/
        /*****************************************************************************************************/
        _lastFrameInfos._rows = rows;
        /*Read the data of all the files to read*/
        QtConcurrent::blockingMap(readers,boost::bind(metaReadData,_1,currentFrame));
        /*Allocate the output buffer if the output is a viewer (i.e: allocating the PBO)*/
        if (_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()) {
            QMutexLocker locker(&_pboUnMappedMutex);
            emit doFrameStorageAllocation();
            while(_pboUnMappedCount <= 0) {
                _pboUnMappedCondition.wait(&_pboUnMappedMutex);
            }
            --_pboUnMappedCount;
        }
        
        /*If the frame is not the same than the last frame, we clear the node cache because
         we will NOT get the same results for another frame in the playback sequence.*/
        if (!_lastRunArgs._sameFrame) {
            assert(appPTR->getNodeCache());
            appPTR->getNodeCache()->clear();
        }
        
        /*What are the output channels ?*/
        ChannelSet outChannels;
        if (_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()) {
            assert(viewer);
            assert(viewer->getUiContext());
            outChannels = viewer->getUiContext()->displayChannels();
        } else {// channels requested are those requested by the user
            if (!_dag.isOutputAnOpenFXNode()) {
                assert(writer);
                outChannels = writer->requestedChannels();
            } else {
                //openfx outputs can only output RGBA
                outChannels = Mask_RGBA;
            }
        }
        /*get the time at which we started rendering the frame*/
        gettimeofday(&_lastComputeFrameTime, 0);
        {
            int counter = 0;
            QVector<Row*> sequence;
            sequence.reserve(rows.size());
            /*Creating the sequence of rows*/
            for (vector<int>::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
                Row* row = new Row(x,*it,r,outChannels);
                assert(row);
                row->lock();
                row->set_zoomedY(counter);
                row->unlock();
                // RowRunnable* worker = new RowRunnable(row,_dag.getOutput());
                //            if(counter%10 == 0){
                //            // UNCOMMENT to report progress.
                //                QObject::connect(worker, SIGNAL(finished(int,int)), this ,SLOT(checkAndDisplayProgress(int,int)),Qt::QueuedConnection);
                //            }
                sequence.push_back(row);
                //  _threadPool->start(worker);
                ++counter;
            }
            
            /*Does the rendering*/
            _workerThreadsWatcher->setFuture(QtConcurrent::map(sequence,boost::bind(metaEnginePerRow,_1,_dag.getOutput())));
            _workerThreadsWatcher->waitForFinished();
            //_threadPool->waitForDone();
        }
        {
            /*The frame is now fully rendered, if it is a viewer we want to stash it to the cache.*/
            QMutexLocker locker(&_abortedRequestedMutex);
            if (_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode() && !_abortRequested) {
                locker.unlock();
                //copying the frame data stored into the PBO to the viewer cache if it was a normal engine
                //This is done only if we run a sequence (i.e: playback) because the viewer cache isn't meant for
                //panning/zooming.
                assert(viewer);
                assert(viewer->getUiContext());
                ViewerGL* viewerGL = viewer->getUiContext()->viewer;
                assert(viewerGL);
                viewerGL->stopDisplayingProgressBar();
                assert(appPTR->getViewerCache());
                FrameEntry* entry = appPTR->getViewerCache()->addFrame(key,
                                                                       inputFileNames,
                                                                       _treeVersion,
                                                                       zoomFactor,
                                                                       exposure,
                                                                       lut,
                                                                       byteMode,
                                                                       _lastFrameInfos._textureRect,
                                                                       dataW,
                                                                       _dispW);

                if(entry){
                    assert(entry->getMappedFile());
                    assert(entry->getMappedFile()->data());
                    assert(viewer->getUiContext());
                    assert(viewerGL);
                    assert(viewerGL->getFrameData());
                    memcpy(entry->getMappedFile()->data(),viewerGL->getFrameData(),_lastFrameInfos._dataSize);
                    entry->removeReference(); // removing reference as we're done with the entry.
                    entry->unlock();
                }

            } else if (!_dag.isOutputAViewer() && !_dag.isOutputAnOpenFXNode()) {
                /*if the output is a writer we actually start writing on disk now*/
                assert(_dag.outputAsWriter());
                if(!_abortRequested) {
                    locker.unlock();
                    _dag.outputAsWriter()->startWriting();
                }
            }
        }
        /*The frame has been rendered and cached properly, we call engineLoop() which will reset all the flags,
         update viewers
         and appropriately increment counters for the next frame in the sequence.*/
        emit frameRendered(currentFrame);
        engineLoop();

        /*endRenderAction for all openFX nodes*/
        for (DAG::DAGIterator it = _dag.begin(); it!=_dag.end(); ++it) {
            OfxNode* n = dynamic_cast<OfxNode*>(*it);
            if(n){
                OfxPointD renderScale;
                renderScale.x = renderScale.y = 1.0;
                assert(n->effectInstance());
                OfxStatus stat;
                stat = n->effectInstance()->endRenderAction(0, 25, 1, true, renderScale);
                assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            }
        }
        _model->getGeneralMutex()->unlock();
        
    } // end for(;;)
    
}
void VideoEngine::onProgressUpdate(int i){
    // cout << "progress: index = " << i ;
    if(i < (int)_lastFrameInfos._rows.size()){
        //  cout <<" y = "<< _lastFrameInfos._rows[i] << endl;
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
        QMutexLocker locker(&_pboUnMappedMutex);
        emit doUpdateViewer();
        while(_pboUnMappedCount <= 0) {
            _pboUnMappedCondition.wait(&_pboUnMappedMutex);
        }
        --_pboUnMappedCount;
    }
    _lastRunArgs._fitToViewer = false;
    _lastRunArgs._recursiveCall = true;
}

void VideoEngine::updateViewer(){
    QMutexLocker locker(&_pboUnMappedMutex);
    
    ViewerGL* viewer = _dag.outputAsViewer()->getUiContext()->viewer;
    /*This should remove the flickering on the Viewer. This is because we're trying to
     fill a texture with a buffer not necessarily filled correctly if aborted is true.
     
     ||| Somehow calling unMapPBO() leads to a race condition. Commenting out
     while this is resolved.*/
    {
        QMutexLocker l(&_abortedRequestedMutex);
        if(!_abortRequested){
            viewer->copyPBOToRenderTexture(_lastFrameInfos._textureRect); // returns instantly
        }else{
            viewer->unMapPBO();
            viewer->unBindPBO();
        }
    }
    _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
    if((_lastRunArgs._frameRequestIndex%24)==0){
        emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
    }
    _lastRunArgs._zoomFactor = viewer->getZoomFactor();
    updateDisplay(); // updating viewer & pixel aspect ratio if needed
    ++_pboUnMappedCount;
    _pboUnMappedCondition.wakeOne();
}

void VideoEngine::cachedEngine(){
     QMutexLocker locker(&_pboUnMappedMutex);
    _dag.outputAsViewer()->cachedFrameEngine(_lastFrameInfos._cachedEntry);
//    while(_pboUnMappedCount <= 0) {
//        _pboUnMappedCondition.wait(&_pboUnMappedMutex);
//    }
//    --_pboUnMappedCount;
    ++_pboUnMappedCount;
    _pboUnMappedCondition.wakeOne();
}

void VideoEngine::allocateFrameStorage(){
    QMutexLocker locker(&_pboUnMappedMutex);
    _lastFrameInfos._dataSize = _dag.outputAsViewer()->getUiContext()->viewer->allocateFrameStorage(_lastFrameInfos._textureRect.w,
                                                                                                    _lastFrameInfos._textureRect.h);
//    while(_pboUnMappedCount <= 0) {
//        _pboUnMappedCondition.wait(&_pboUnMappedMutex);
//    }
//    --_pboUnMappedCount;
    ++_pboUnMappedCount;
    _pboUnMappedCondition.wakeOne();
   
}

void RowRunnable::run() {
    assert(_row);
    assert(_output);
    _output->engine(_row->y(), _row->offset(), _row->right(), _row->channels(), _row);
    emit finished(_row->y(),_row->zoomedY());
    delete _row;
}


void VideoEngine::updateDisplay(){
    ViewerGL* viewer  = _dag.outputAsViewer()->getUiContext()->viewer;
    int width = viewer->width();
    int height = viewer->height();
    double ap = viewer->displayWindow().pixel_aspect();
    if(ap > 1.f){
        glViewport (0, 0, (int)(width*ap), height);
    }else{
        glViewport (0, 0, width, (int)(height/ap));
    }
    viewer->updateColorPicker();
    viewer->updateGL();
}



void VideoEngine::setDesiredFPS(double d){
    _timer->setDesiredFrameRate(d);
}


void VideoEngine::abort(){
    
    assert(_workerThreadsWatcher);
    _workerThreadsWatcher->cancel();
    _workerThreadsWatcher->waitForFinished();
    {
        QMutexLocker locker(&_abortedRequestedMutex);
        cout << "abort requested" << endl;

        ++_abortRequested;
        for (DAG::DAGIterator it = _dag.begin(); it != _dag.end(); ++it) {
            (*it)->setAborted(true);
        }
        // _abortedCondition.wakeOne();
    }
}


void VideoEngine::seek(int frame){
    if(frame >= _dag.getOutput()->firstFrame() && frame <= _dag.getOutput()->lastFrame())
        refreshAndContinueRender(false, _dag.getOutput(),frame);
}

void VideoEngine::refreshAndContinueRender(bool initViewer,OutputNode* output,int startingFrame){

    ViewerNode* viewer = dynamic_cast<ViewerNode*>(output);
    bool wasPlaybackRunning;
    {
        QMutexLocker startedLocker(&_startMutex);
        wasPlaybackRunning = _startCount > 0 && _lastRunArgs._frameRequestsCount == -1;
    }
    if(!viewer || wasPlaybackRunning){
        render(output,startingFrame,-1,initViewer,_lastRunArgs._forward,false);
    }else{
        render(output,startingFrame,1,initViewer,_lastRunArgs._forward,true);
    }
}


void VideoEngine::updateTreeVersion(){
    
    if(!_dag.getOutput()){
        return;
    }
    Node* output = _dag.getOutput();
    std::vector<std::string> v;
    output->computeTreeHash(v);
    _treeVersion = output->hash().value();
    _treeVersionValid = true;
}


void VideoEngine::DAG::fillGraph(Node* n){
    if(std::find(_graph.begin(),_graph.end(),n)==_graph.end()){
        n->setMarked(false);
        _graph.push_back(n);
        if(n->isInputNode()){
            _inputs.push_back(n);
        }
    }
    
    const Node::InputMap& inputs = n->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second){
            if(n->className() == "Viewer"){
                ViewerNode* v = dynamic_cast<ViewerNode*>(n);
                if (it->first!=v->activeInput()) {
                    continue;
                }
            }
            fillGraph(it->second);
        }
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
    const Node::InputMap& inputs = n->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second && !it->second->isMarked()){
            _depthCycle(it->second);
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

void VideoEngine::DAG::resetAndSort(OutputNode* out){
    
    _output = out;
    _isViewer = dynamic_cast<ViewerNode*>(out) != NULL;
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
bool VideoEngine::DAG::validate(bool doFullWork){
    /*Validating the DAG in topological order*/
    for (DAGIterator it = begin(); it!=end(); ++it) {
        assert(*it);
        if(!(*it)->validate(doFullWork)){
            return false;
        }else{
            (*it)->setExecutingEngine(_output->getVideoEngine());
        }
    }
    return true;
}


int VideoEngine::DAG::firstFrame() const {
    return _output->info().firstFrame();
}
int VideoEngine::DAG::lastFrame() const {
    return _output->info().lastFrame();
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
    if(t >= 0.5){
        if(_dag.isOutputAViewer()){
            _dag.outputAsViewer()->getUiContext()->viewer->updateProgressOnViewer(_lastFrameInfos._textureRect, y,zoomedY);
        }else{
            emit progressChanged(floor(((double)y/(double)_lastFrameInfos._rows.size())*100));
        }
        return true;
    }else{
        return false;
    }
}
void VideoEngine::quitEngineThread(){
    {
        QMutexLocker locker(&_mustQuitMutex);
        _mustQuit = true;
    }
    {
        QMutexLocker locker(&_startMutex);
        ++_startCount;
        _startCondition.wakeOne();
    }
}

void VideoEngine::toggleLoopMode(bool b){
    _loopMode = b;
}

bool VideoEngine::isWorking() {
    QMutexLocker lock(&_startMutex);
    return _startCount > 0;
}

const QString VideoEngine::DAG::generateConcatenationOfAllReadersFileNames() const{
    QString ret;

    assert(_output);
    for (U32 i = 0; i < _inputs.size(); ++i) {
        assert(_inputs[i]);
        ret.append(_inputs[i]->getRandomFrameName(_output->currentFrame()));
    }
    return ret;
}
