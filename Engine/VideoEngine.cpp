//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#include "VideoEngine.h"

#ifndef __NATRON_WIN32__
#include <unistd.h> //Provides STDIN_FILENO
#endif
#include <iterator>
#include <cassert>

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/QSocketNotifier>

#include "Global/MemoryInfo.h"

#include "Engine/ViewerInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Hash64.h"
#include "Engine/Project.h"
#include "Engine/Lut.h"
#include "Engine/FrameEntry.h"
#include "Engine/MemoryFile.h"
#include "Engine/TimeLine.h"
#include "Engine/Timer.h"
#include "Engine/Log.h"
#include "Engine/EffectInstance.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/Node.h"


#define NATRON_FPS_REFRESH_RATE 10


using namespace Natron;
using std::make_pair;
using std::cout; using std::endl;


VideoEngine::VideoEngine(Natron::OutputEffectInstance* owner,QObject* parent)
    : QThread(parent)
    , _tree(owner)
    , _threadStarted(false)
    , _abortBeingProcessedMutex()
    , _abortBeingProcessed(false)
    , _abortedRequestedCondition()
    , _abortedRequestedMutex()
    , _abortRequested(0)
    , _mustQuitCondition()
    , _mustQuitMutex()
    , _mustQuit(false)
    , _loopModeMutex()
    , _loopMode(true)
    , _restart(true)
    , _startCondition()
    , _startMutex()
    , _startCount(0)
    , _workingMutex()
    , _working(false)
    , _timerMutex()
    , _timer(new Timer)
    , _timerFrameCount(0)
    , _lastRequestedRunArgs()
    , _currentRunArgs()
    , _startRenderFrameTime()
    , _timeline(owner->getNode()->getApp()->getTimeLine())
    , _getFrameRangeCond()
    , _getFrameRangeMutex()
    , _gettingFrameRange(false)
    , _firstFrame(0)
    , _lastFrame(0)
    , _doingARenderSingleThreaded(false)
{
    QObject::connect(this, SIGNAL(mustGetFrameRange()), this, SLOT(getFrameRange()));
}

VideoEngine::~VideoEngine() {
    _threadStarted = false;
}

void VideoEngine::quitEngineThread(){
    bool isThreadStarted = false;
    {
        QMutexLocker quitLocker(&_mustQuitMutex);
        isThreadStarted = _threadStarted;
    }
    if(isThreadStarted){
        {
            QMutexLocker locker(&_mustQuitMutex);
            _mustQuit = true;
        }
        abortRendering(true);

        {
            QMutexLocker locker(&_startMutex);
            ++_startCount;
            _startCondition.wakeAll();
        }
        
        {
            QMutexLocker locker(&_getFrameRangeMutex);
            _gettingFrameRange = false;
            _getFrameRangeCond.wakeAll();
        }
        
        {
            QMutexLocker locker(&_mustQuitMutex);
            while(_mustQuit){
                _mustQuitCondition.wait(&_mustQuitMutex);
            }
        }
        QMutexLocker quitLocker(&_mustQuitMutex);
        _threadStarted = false;
    } else {
        
        ///single threaded- no locking required
        _mustQuit = true;
    }
}

void VideoEngine::render(int frameCount,
                         bool seekTimeline,
                         bool refreshTree,
                         bool forward,
                         bool sameFrame,
                         bool forcePreview) {
    
    
    /*If the Tree was never built and we don't want to update the Tree, force an update
     so there's no null pointers hanging around*/
    if((!_tree.getOutput() || !_tree.wasTreeEverBuilt()) && !refreshTree) refreshTree = true;
    
    
    /*setting the run args that are used by the run function*/
    _lastRequestedRunArgs._sameFrame = sameFrame;
    _lastRequestedRunArgs._recursiveCall = false;
    _lastRequestedRunArgs._forward = forward;
    _lastRequestedRunArgs._refreshTree = refreshTree;
    _lastRequestedRunArgs._seekTimeline = seekTimeline;
    _lastRequestedRunArgs._frameRequestsCount = frameCount;
    _lastRequestedRunArgs._frameRequestIndex = 0;
    _lastRequestedRunArgs._forcePreview = forcePreview;
    std::string sequentialNode;
    if (_tree.getOutput()->getNode()->hasSequentialOnlyNodeUpstream(sequentialNode)) {
        ///The tree has a sequential node inside... due to the limitation of the beginSequenceRender/endSequenceRender
        ///actions in OpenFX we cannot render simultaneously multiple views. We just render the main view.
        ///A warning should be popped to the user prior to entering this code.
        _lastRequestedRunArgs._forceSequential = true;
    } else {
        _lastRequestedRunArgs._forceSequential = false;
    }

    
    if (appPTR->getCurrentSettings()->getNumberOfThreads() == -1) {
        runSameThread();
    } else {
        
        /*Starting or waking-up the thread*/
        QMutexLocker quitLocker(&_mustQuitMutex);
        if (!_threadStarted && !_mustQuit) {
            start(HighestPriority);
            _threadStarted = true;
        } else {
            QMutexLocker locker(&_startMutex);
            ++_startCount;
            _startCondition.wakeOne();
        }
    }
}

bool VideoEngine::startEngine(bool singleThreaded) {
    // don't allow "abort"s to be processed while starting engine by locking _abortBeingProcessedMutex
    QMutexLocker abortBeingProcessedLocker(&_abortBeingProcessedMutex);
    assert(!_abortBeingProcessed);

    {
        // let stopEngine run by unlocking abortBeingProcessedLocker()
        abortBeingProcessedLocker.unlock();
        QMutexLocker l(&_abortedRequestedMutex);
        if (_abortRequested > 0) {
            return false;
        }
        // make sure stopEngine is not running before releasing _abortedRequestedMutex
        abortBeingProcessedLocker.relock();
        assert(!_abortBeingProcessed);
    }
    _restart = false; /*we just called startEngine,we don't want to recall this function for the next frame in the sequence*/
    
    _currentRunArgs = _lastRequestedRunArgs;
    

    ///build the tree before getFrameRange!
    if(_currentRunArgs._refreshTree) {
        refreshTree();/*refresh the tree*/
    }
    
    if(!_tree.isOutputAViewer()){
        
        if (!singleThreaded && !appPTR->isBackground()) {
            {
                QMutexLocker l(&_abortedRequestedMutex);
                if (_abortRequested > 0) {
                    return false;
                }
            }
            QMutexLocker l(&_getFrameRangeMutex);
            _gettingFrameRange = true;
            emit mustGetFrameRange();
            while (_gettingFrameRange) {
                _getFrameRangeCond.wait(&_getFrameRangeMutex);
            }
        } else {
            getFrameRange();
        }
        
        Natron::OutputEffectInstance* output = dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput());
        output->setFirstFrame(_firstFrame);
        output->setLastFrame(_lastFrame);
        output->setDoingFullSequenceRender(true);
        
        if (_currentRunArgs._forceSequential) {
            if (_tree.beginSequentialRender(_firstFrame, _lastFrame, _tree.getOutput()->getApp()->getMainView()) == StatFailed) {
                return false;
            }
        }
    }

    {
        QMutexLocker workingLocker(&_workingMutex);
        _working = true;
    }

    
    if(!_currentRunArgs._sameFrame){
        emit engineStarted(_currentRunArgs._forward,_currentRunArgs._frameRequestsCount);
        _timer->playState = RUNNING; /*activating the timer*/

    }
    if(appPTR->isBackground()){
        appPTR->writeToOutputPipe(kRenderingStartedLong, kRenderingStartedShort);
    }
    return true;

}
bool VideoEngine::stopEngine() {
    
    bool wasAborted = false;
    bool mustQuit = false;
    {
        QMutexLocker locker(&_mustQuitMutex);
        mustQuit = _mustQuit;
    }
    /*reset the abort flag and wake up any thread waiting*/
    {
        // make sure startEngine is not running by locking _abortBeingProcessedMutex
        QMutexLocker abortBeingProcessedLocker(&_abortBeingProcessedMutex);
        _abortBeingProcessed = true; //_abortBeingProcessed is a dummy variable: it should be always false when stopeEngine is not running
        {
            QMutexLocker l(&_abortedRequestedMutex);
            if (_abortRequested > 0) {
                wasAborted = true;
            }
            _abortRequested = 0;
            
            /*Refresh preview for all nodes that have preview enabled & set the aborted flag to false.
             ONLY If we're not rendering the same frame (i.e: not panning & zooming) and the user is not scrubbing
             .*/
            if (!mustQuit) {
                bool shouldRefreshPreview = (_tree.getOutput()->getApp()->shouldRefreshPreview() && !_currentRunArgs._sameFrame)
                || _currentRunArgs._forcePreview;
                for (RenderTree::TreeIterator it = _tree.begin(); it != _tree.end(); ++it) {
                    bool previewEnabled = (*it)->isPreviewEnabled();
                    if (previewEnabled) {
                        if (_currentRunArgs._forcePreview) {
                            (*it)->computePreviewImage(_timeline->currentFrame());
                        } else {
                            if (shouldRefreshPreview) {
                                (*it)->refreshPreviewImage(_timeline->currentFrame());
                            }
                        }
                    }
                    (*it)->setAborted(false);
                }
            }
            
            _abortedRequestedCondition.wakeOne();
        }

        emit engineStopped(wasAborted ? 1 : 0);
        
        _currentRunArgs._frameRequestsCount = 0;
        _restart = true;
        _timer->playState = PAUSE;
        

        {
            QMutexLocker workingLocker(&_workingMutex);
            _working = false;
        }

        _abortBeingProcessed = false;

    }
    
    Natron::OutputEffectInstance* outputEffect = dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput());
    outputEffect->setDoingFullSequenceRender(false);
    
    if (!_tree.isOutputAViewer() && _currentRunArgs._forceSequential) {
        (void)_tree.endSequentialRender(_firstFrame, _lastFrame, _tree.getOutput()->getApp()->getMainView());
    }
    
    if (appPTR->isBackground()) {
        outputEffect->notifyRenderFinished();
    }


    {
        QMutexLocker locker(&_mustQuitMutex);
        if (_mustQuit) {
            _mustQuit = false;
            _mustQuitCondition.wakeAll();
            _threadStarted = false;
            return true;
        }
    }

    return false;
    
}

void VideoEngine::run()
{
    
    for(;;){ // infinite loop
        {
            /*First-off, check if the node holding this engine got deleted
             in which case we must quit the engine.*/
            QMutexLocker locker(&_mustQuitMutex);
            if(_mustQuit) {
                _mustQuit = false;
                Natron::OutputEffectInstance* outputEffect = dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput());
                if(appPTR->isBackground()){
                    outputEffect->notifyRenderFinished();
                }
                _mustQuitCondition.wakeAll();
                return;
            }
        }
        
        /*If restart is on, start the engine. Restart is on for the 1st frame
         rendered of a sequence.*/
        if(_restart){
            if(!startEngine(false)){
                if(stopEngine())
                    return;
                
                /*pause the thread*/
                {
                    QMutexLocker locker(&_startMutex);
                    while(_startCount <= 0) {
                        _startCondition.wait(&_startMutex);
                    }
                    _startCount = 0;
                }
                
                continue;
            }
        }
        
        iterateKernel(false);
        
        if (stopEngine()) {
            return;
        } else {
            /*pause the thread*/
            {
                QMutexLocker locker(&_startMutex);
                while(_startCount <= 0) {
                    _startCondition.wait(&_startMutex);
                }
                _startCount = 0;
            }

        }
       
    } // end for(;;)
    
}

void VideoEngine::runSameThread() {
    
    if (_doingARenderSingleThreaded) {
        return;
    } else {
        _doingARenderSingleThreaded = true;
    }
    
    if (!startEngine(true)) {
        stopEngine();
    } else {
        QCoreApplication::processEvents();
        ///if single threaded: the user might have requested to exit and the engine might be deleted after the events process.

        if (_mustQuit) {
            _mustQuit = false;
            _doingARenderSingleThreaded = false;
            return;
        }
        iterateKernel(true);
        QCoreApplication::processEvents();
        ///if single threaded: the user might have requested to exit and the engine might be deleted after the events process.

        if (_mustQuit) {
            _mustQuit = false;
            _doingARenderSingleThreaded = false;
            return;
        }
        stopEngine();
    }
    
    _doingARenderSingleThreaded = false;
}

void VideoEngine::iterateKernel(bool singleThreaded) {
    for(;;){ // infinite loop
        
        {
            QMutexLocker locker(&_abortedRequestedMutex);
            if(_abortRequested > 0) {
                locker.unlock();
                return;
            }
        }
        
        Natron::OutputEffectInstance* output = dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput());
        assert(output);
        ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(output);
        
        /*update the tree inputs */
        _tree.refreshRenderInputs();
        
        
        if(viewer){
            if (singleThreaded || appPTR->isBackground()) {
                getFrameRange();
            } else {
                {
                    QMutexLocker l(&_abortedRequestedMutex);
                    if (_abortRequested > 0) {
                        return;
                    }
                }
                
                bool mustQuit;
                {
                    QMutexLocker l(&_mustQuitMutex);
                    mustQuit = _mustQuit;
                }
                
                QMutexLocker l(&_getFrameRangeMutex);
                _gettingFrameRange = true;
                emit mustGetFrameRange();
                while (_gettingFrameRange && !mustQuit) {
                    _getFrameRangeCond.wait(&_getFrameRangeMutex);
                }
                
            }
            
            //If the frame range is not locked, let the user define it.
            if (viewer->isFrameRangeLocked()) {
                _timeline->setFrameRange(_firstFrame, _lastFrame);
            }
        }
        
        int firstFrame,lastFrame;
        if (viewer) {
            firstFrame = _timeline->leftBound();
            lastFrame = _timeline->rightBound();
        } else {
            firstFrame = output->getFirstFrame();
            lastFrame = output->getLastFrame();
        }
        
        //////////////////////////////
        // Set the current frame
        //
        int currentFrame = 0;
        if (!_currentRunArgs._recursiveCall) {
            
            /*if writing on disk and not a recursive call, move back the timeline cursor to the start*/
            if (viewer) {
                currentFrame = _timeline->currentFrame();
            } else {
                output->setCurrentFrame(firstFrame);
                currentFrame = firstFrame;
            }
            
        } else if(!_currentRunArgs._sameFrame && _currentRunArgs._seekTimeline) {
            assert(_currentRunArgs._recursiveCall); // we're in the else part
            if (!viewer) {
                output->setCurrentFrame(output->getCurrentFrame()+1);
                currentFrame = output->getCurrentFrame();
                if(currentFrame > lastFrame){
                    return;
                }
            } else {
                // viewer
                assert(viewer);
                if (_currentRunArgs._forward) {
                    currentFrame = _timeline->currentFrame();
                    if (currentFrame < lastFrame) {
                        _timeline->incrementCurrentFrame(output);
                        ++currentFrame;
                    } else {
                        QMutexLocker loopModeLocker(&_loopModeMutex);
                        if (_loopMode) { // loop only for a viewer
                            currentFrame = firstFrame;
                            _timeline->seekFrame(currentFrame,output);
                        } else {
                            loopModeLocker.unlock();
                            return;
                        }
                    }
                    
                } else {
                    currentFrame = _timeline->currentFrame();
                    if (currentFrame > firstFrame) {
                        _timeline->decrementCurrentFrame(output);
                        --currentFrame;
                    } else {
                        QMutexLocker loopModeLocker(&_loopModeMutex);
                        if (_loopMode) { //loop only for a viewer
                            currentFrame = lastFrame;
                            _timeline->seekFrame(currentFrame,output);
                        } else {
                            loopModeLocker.unlock();
                            return;
                        }
                    }
                }
            }
        }
        
        ///////////////////////////////
        // Check whether we need to stop the engine or not for various reasons.
        //
        {
            QMutexLocker locker(&_abortedRequestedMutex);
            if(_abortRequested > 0 || // #1 aborted by the user
               
               (_tree.isOutputAViewer() // #2 the Tree contains only 1 frame and we rendered it
                &&  _currentRunArgs._recursiveCall
                &&  firstFrame == lastFrame
                && _currentRunArgs._frameRequestsCount == -1
                && _currentRunArgs._frameRequestIndex == 1)
               
               || _currentRunArgs._frameRequestsCount == 0// #3 the sequence ended and it was not an infinite run
               || (appPTR->getAppType() == AppManager::APP_BACKGROUND_AUTO_RUN && appPTR->hasAbortAnyProcessingBeenCalled()))
            {
                return;
            }
        }
        
        ///before rendering the frame, clear any persistent message that may be left
        _tree.clearPersistentMessages();
        
        ////////////////////////
        // Render currentFrame
        //
        // if the output is a writer, _tree.outputAsWriter() returns a valid pointer/
        Status stat;
        try {
            stat =  renderFrame(currentFrame,singleThreaded);
        } catch (const std::exception &e) {
            std::stringstream ss;
            ss << "Error while rendering" << " frame " << currentFrame << ": " << e.what();
            if (viewer) {
                //viewer->setPersistentMessage(Natron::ERROR_MESSAGE, ss.str());
                viewer->disconnectViewer();
            } else {
                std::cout << ss.str() << std::endl;
            }
            return;
            
        }
        
        if (stat == StatFailed) {
            return;
        }
        
        
        /*The frame has been rendered , we call engineLoop() which will reset all the flags,
         update viewers
         and appropriately increment counters for the next frame in the sequence.*/
        emit frameRendered(currentFrame);
        if(appPTR->isBackground()){
            QString frameStr = QString::number(currentFrame);
            appPTR->writeToOutputPipe(kFrameRenderedStringLong + frameStr,kFrameRenderedStringShort + frameStr);
        }
        
        if (singleThreaded) {
            QCoreApplication::processEvents();
            
            ///if single threaded: the user might have requested to exit and the engine might be deleted after the events process.
            if (_mustQuit) {
                return;
            }
        }
        
        if(_currentRunArgs._frameRequestIndex == 0 && _currentRunArgs._frameRequestsCount == 1 && !_currentRunArgs._sameFrame){
            _currentRunArgs._frameRequestsCount = 0;
        }else if(_currentRunArgs._frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
            --_currentRunArgs._frameRequestsCount;
        }
        ++_currentRunArgs._frameRequestIndex;//incrementing the frame counter
        
        _currentRunArgs._recursiveCall = true;
    } // end for(;;)
}

Natron::Status VideoEngine::renderFrame(SequenceTime time,bool singleThreaded) {
    /*pre process frame*/
    
//    Status stat = _tree.preProcessFrame();
//    if (stat == StatFailed) {
//        return stat;
//        //don't throw an exception here, this is regular behaviour when a mandatory input is not connected.
//        // We don't want to popup a dialog everytime it occurs
//        //      throw std::runtime_error("PreProcessFrame failed, mandatory inputs are probably not connected.");
//    }
    bool isSequentialRender = _currentRunArgs._frameRequestsCount > 1 || _currentRunArgs._frameRequestsCount == -1 ||
    _currentRunArgs._forceSequential;
    Status stat = StatOK;
    /*get the time at which we started rendering the frame*/
    gettimeofday(&_startRenderFrameTime, 0);
    if (_tree.isOutputAViewer() && !_tree.isOutputAnOpenFXNode()) {
        ViewerInstance* viewer = _tree.outputAsViewer();
        stat = viewer->renderViewer(time,singleThreaded,isSequentialRender);
        
        if (!_currentRunArgs._sameFrame) {
            QMutexLocker timerLocker(&_timerMutex);
            _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
            if ((_timerFrameCount % NATRON_FPS_REFRESH_RATE) == 0 && _currentRunArgs._frameRequestsCount == -1) {
                emit fpsChanged(_timer->actualFrameRate(),_timer->getDesiredFrameRate()); // refreshing fps display on the GUI
                _timerFrameCount = 1; //reseting to 1
            } else {
                ++_timerFrameCount;
            }
            
        }
        
        if (stat == StatFailed) {
            viewer->disconnectViewer();
        }
    
    } else {
        RenderScale scale;
        scale.x = scale.y = 1.;
        RectI rod;
        bool isProjectFormat;
        
        int viewsCount = _tree.getOutput()->getApp()->getProject()->getProjectViewsCount();
        int mainView = 0;
        if (isSequentialRender) {
            mainView = _tree.getOutput()->getApp()->getMainView();
        }
        
        for (int i = 0; i < viewsCount;++i) {
            
            if (isSequentialRender && i != mainView) {
                ///@see the warning in EffectInstance::evaluate
                continue;
            }
            // Do not catch exceptions: if an exception occurs here it is probably fatal, since
            // it comes from Natron itself. All exceptions from plugins are already caught
            // by the HostSupport library.
            stat = _tree.getOutput()->getRegionOfDefinition_public(time,scale,i, &rod,&isProjectFormat);
            if (stat != StatFailed) {
                ImageComponents components;
                ImageBitDepth imageDepth;
                _tree.getOutput()->getPreferredDepthAndComponents(-1, &components, &imageDepth);
                (void)_tree.getOutput()->renderRoI(EffectInstance::RenderRoIArgs(time, //< the time at which to render
                                                                                 scale, //< the scale at which to render
                                                                                 0, //< the mipmap level (redundant with the scale)
                                                                                 i , //< the view to render
                                                                                 rod, //< the region of interest (in pixel coordinates)
                                                                                 isSequentialRender, // is this sequential
                                                                                 false,  // is this render due to user interaction ?
                                                                                 false,//< bypass cache ?
                                                                                 &rod, // < any precomputed rod ?
                                                                                 components,
                                                                                 imageDepth));
            } else {
                break;
            }
        }
    }
//
//    if (stat == StatFailed) {
//        throw std::runtime_error("Render failed");
//    }
    return stat;

}

void VideoEngine::onProgressUpdate(int /*i*/){
    // cout << "progress: index = " << i ;
    //    if(i < (int)_currentFrameInfos._rows.size()){
    //        //  cout <<" y = "<< _lastFrameInfos._rows[i] << endl;
    //        checkAndDisplayProgress(_currentFrameInfos._rows[i],i);
    //    }
}


void VideoEngine::abortRendering(bool blocking) {
    {
        if(!isWorking()){
            return;
        }
    }
    {
        QMutexLocker locker(&_abortedRequestedMutex);
        ++_abortRequested;
        
        /*Note that we set the aborted flag in from output to inputs otherwise some aborted images
        might get rendered*/
        for (RenderTree::TreeReverseIterator it = _tree.rbegin(); it != _tree.rend(); ++it) {
            (*it)->setAborted(true);
        }
        if(_tree.isOutputAViewer() && QThread::currentThread() != this)
            _tree.outputAsViewer()->wakeUpAnySleepingThread();
        
        ///also wake up the run() thread if it is waiting for getFrameRange
        _gettingFrameRange = false;
        _getFrameRangeCond.wakeOne();
        
        if (QThread::currentThread() != this && isRunning()  && blocking) {
            while (_abortRequested > 0) {
                _abortedRequestedCondition.wait(&_abortedRequestedMutex);
            }
        }
        
    }
}


void VideoEngine::refreshAndContinueRender(bool forcePreview){
    //the changes will occur upon the next frame rendered. If the playback is running indefinately
    //we're sure that there will be a refresh. If the playback is for a determined amount of frame
    //we've to make sure the playback is not rendering the last frame, in which case we wouldn't see
    //the last changes.
    //The default case is if the playback is not running: just render the current frame

    bool isPlaybackRunning = isWorking() && (_currentRunArgs._frameRequestsCount == -1 ||
                                             (_currentRunArgs._frameRequestsCount > 1 && _currentRunArgs._frameRequestIndex < _currentRunArgs._frameRequestsCount - 1));
    if(!isPlaybackRunning) {
#pragma message WARN("We should abort here because this is called following a parameter change but doing so make some plugin error.")
        ///(typically S_Blur)
        
        //abortRendering(false);
        render(1,false,false,_currentRunArgs._forward,true,forcePreview);
    }
}
void VideoEngine::updateTreeAndContinueRender(){
    //this is a bit more trickier than refreshAndContinueRender, we've to stop
    //the playback, and request a new render
    bool isPlaybackRunning = isWorking() && (_currentRunArgs._frameRequestsCount == -1 ||
                                             (_currentRunArgs._frameRequestsCount > 1 && _currentRunArgs._frameRequestIndex < _currentRunArgs._frameRequestsCount - 1));

    if(isPlaybackRunning){
        int count = _currentRunArgs._frameRequestsCount == - 1 ? -1 :
                                                                 _currentRunArgs._frameRequestsCount - _currentRunArgs._frameRequestIndex ;
        abortRendering(true);
        render(count,true,true,_currentRunArgs._forward,false,false);
    }else{
        render(1,false,true,_currentRunArgs._forward,true,false);
    }
}


RenderTree::RenderTree(EffectInstance* output):
    _output(output)
  ,_sorted()
  ,_isViewer(false)
  ,_isOutputOpenFXNode(false)
  ,_wasEverBuilt(false)
  ,_firstFrame(0)
  ,_lastFrame(0)
{
    assert(output);
}


void RenderTree::clearGraph(){
    for(TreeContainer::const_iterator it = _sorted.begin();it!=_sorted.end();++it) {
        (*it)->clearPersistentMessage();
    }
    _sorted.clear();
}

void RenderTree::refreshTree(){
    _isViewer = dynamic_cast<ViewerInstance*>(_output) != NULL;
    _isOutputOpenFXNode = _output->isOpenFX();
    
    /*unmark all nodes already present in the graph*/
    clearGraph();
    std::vector<boost::shared_ptr<Natron::Node> > markedNodes;
    fillGraph(_output->getNode(),markedNodes);
    _wasEverBuilt = true;
}


void RenderTree::fillGraph(const boost::shared_ptr<Natron::Node>& node,std::vector<boost::shared_ptr<Natron::Node> >& markedNodes){
    
    
    /*call fillGraph recursivly on all the node's inputs*/
    node->updateRenderInputs();
    const std::vector<boost::shared_ptr<Node> >& inputs = node->getInputs_other_thread();
    for (U32 i = 0; i < inputs.size(); ++i) {
        if(inputs[i]){
            fillGraph(inputs[i],markedNodes);
        }
    }
    std::vector<boost::shared_ptr<Natron::Node> >::iterator foundNode = std::find(markedNodes.begin(), markedNodes.end(), node);
    if (foundNode == markedNodes.end()) {
        markedNodes.push_back(node);
        _sorted.push_back(node);
    }
}

void RenderTree::refreshRenderInputs()
{
    for(TreeContainer::iterator it = _sorted.begin();it!=_sorted.end();++it) {
        (*it)->updateRenderInputs();
    }
}


Natron::Status RenderTree::beginSequentialRender(SequenceTime first,SequenceTime last,int view)
{
    RenderScale s;
    s.x = s.y = 1.;
    for(TreeContainer::iterator it = _sorted.begin();it!=_sorted.end();++it) {
        if ((*it)->getLiveInstance()->beginSequenceRender_public(first, last, 1, false, s, true,false, view) == StatFailed) {
            return StatFailed;
        }
    }
    return StatOK;
}



Natron::Status RenderTree::endSequentialRender(SequenceTime first,SequenceTime last,int view)
{
    RenderScale s;
    s.x = s.y = 1.;
    for(TreeContainer::iterator it = _sorted.begin();it!=_sorted.end();++it) {
        if ((*it)->getLiveInstance()->endSequenceRender_public(first, last, 1, false, s, true,false, view) == StatFailed) {
            return StatFailed;
        }
    }
    return StatOK;
}


void RenderTree::clearPersistentMessages()
{
    for(TreeContainer::iterator it = _sorted.begin();it!=_sorted.end();++it) {
        (*it)->clearPersistentMessage();
    }
}

ViewerInstance* RenderTree::outputAsViewer() const {
    if(_output && _isViewer){
        return dynamic_cast<ViewerInstance*>(_output);
    }else{
        return NULL;
    }
}



void RenderTree::debug() const{
    cout << "Topological ordering of the Tree is..." << endl;
    for(RenderTree::TreeIterator it = begin(); it != end() ;++it) {
        cout << (*it)->getName() << endl;
    }
}

Natron::Status RenderTree::preProcessFrame(){
    /*Validating the Tree in topological order*/
    for (TreeIterator it = begin(); it != end(); ++it) {
        for (int i = 0; i < (*it)->maximumInputs(); ++i) {
            if (!(*it)->input_other_thread(i) && !(*it)->getLiveInstance()->isInputOptional(i)) {
                return StatFailed;
            }
        }
    }
    return Natron::StatOK;
}


bool VideoEngine::checkAndDisplayProgress(int /*y*/,int/* zoomedY*/){
    //    timeval now;
    //    gettimeofday(&now, 0);
    //    double t =  now.tv_sec  - _startRenderFrameTime.tv_sec +
    //    (now.tv_usec - _startRenderFrameTime.tv_usec) * 1e-6f;
    //    if(t >= 0.5){
    //        if(_tree.isOutputAViewer()){
    //            _tree.outputAsViewer()->getUiContext()->viewer->updateProgressOnViewer(_currentFrameInfos._textureRect, y,zoomedY);
    //        }else{
    //            emit progressChanged(floor(((double)y/(double)_currentFrameInfos._rows.size())*100));
    //            std::cout << kProgressChangedString << floor(((double)y/(double)_currentFrameInfos._rows.size())*100) << std::endl;
    //        }
    //        return true;
    //    }else{
    //        return false;
    //    }
    return false;
}

void VideoEngine::toggleLoopMode(bool b){
    _loopMode = b;
}


void VideoEngine::setDesiredFPS(double d){
    QMutexLocker timerLocker(&_timerMutex);
    _timer->setDesiredFrameRate(d);
}



bool VideoEngine::isWorking() const {
    QMutexLocker workingLocker(&_workingMutex);
    return _working;
}

bool VideoEngine::mustQuit() const{
    QMutexLocker locker(&_mustQuitMutex);
    return _mustQuit;
}

void VideoEngine::refreshTree(){
    ///get the knobs age before locking to prevent deadlock
    _tree.refreshTree();
}

void VideoEngine::getFrameRange() {
    QMutexLocker l(&_getFrameRangeMutex);
    
    if(_tree.getOutput()){
        _tree.getOutput()->getFrameRange_public(&_firstFrame, &_lastFrame);
        if(_firstFrame == INT_MIN){
            _firstFrame = _timeline->leftBound();
        }
        if(_lastFrame == INT_MAX){
            _lastFrame = _timeline->rightBound();
        }
    }else{
        _firstFrame = _timeline->leftBound();
        _lastFrame = _timeline->rightBound();
    }
    
    _gettingFrameRange = false;
    _getFrameRangeCond.wakeOne();

}
