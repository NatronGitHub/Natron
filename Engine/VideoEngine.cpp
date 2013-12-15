

//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#include "VideoEngine.h"

#include <unistd.h> //Provides STDIN_FILENO
#include <iterator>
#include <cassert>
#include <QtCore/QMutex>
#include <QtGui/QVector2D>
#include <QAction>
#include <QtCore/QThread>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QtCore/QSocketNotifier>

#include "Engine/ViewerInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Settings.h"
#include "Engine/Hash64.h"
#include "Engine/Project.h"
#include "Engine/Lut.h"
#include "Engine/FrameEntry.h"
#include "Engine/Row.h"
#include "Engine/MemoryFile.h"
#include "Engine/TimeLine.h"
#include "Engine/Timer.h"
#include "Engine/Log.h"

#include "Engine/EffectInstance.h"
#include "Writers/Writer.h"
#include "Readers/Reader.h"

#include "Global/AppManager.h"
#include "Global/MemoryInfo.h"


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
    , _treeVersionValid(false)
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
    , _processAborter(NULL)
{
}

VideoEngine::~VideoEngine() {
    if(_processAborter){
        delete _processAborter;
    }
}

void VideoEngine::quitEngineThread(){
    if(_threadStarted){
        abortRendering();
        {
            QMutexLocker locker(&_mustQuitMutex);
            _mustQuit = true;
        }
        {
            QMutexLocker locker(&_startMutex);
            ++_startCount;
            _startCondition.wakeAll();
        }
        if(_tree.isOutputAViewer())
            _tree.outputAsViewer()->wakeUpAnySleepingThread();
        {
            QMutexLocker locker(&_mustQuitMutex);
            while(_mustQuit){
                _mustQuitCondition.wait(&_mustQuitMutex);
            }
        }
    }
}

void VideoEngine::render(int frameCount,
                         bool refreshTree,
                         bool fitFrameToViewer,
                         bool forward,
                         bool sameFrame) {
    
    
    if(_tree.getOutput()->getApp()->isBackground() && !_processAborter){
        _processAborter = new BackgroundProcessAborter(this);
        _processAborter->start();
    }
    
    /*If the Tree was never built and we don't want to update the Tree, force an update
     so there's no null pointers hanging around*/
    if(!_tree.getOutput() && !refreshTree) refreshTree = true;
    
    
    /*setting the run args that are used by the run function*/
    _lastRequestedRunArgs._sameFrame = sameFrame;
    _lastRequestedRunArgs._fitToViewer = fitFrameToViewer;
    _lastRequestedRunArgs._recursiveCall = false;
    _lastRequestedRunArgs._forward = forward;
    _lastRequestedRunArgs._refreshTree = refreshTree;
    _lastRequestedRunArgs._frameRequestsCount = frameCount;
    _lastRequestedRunArgs._frameRequestIndex = 0;

    
    
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

bool VideoEngine::startEngine() {
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
    
    int firstFrame,lastFrame;
    getFrameRange(&firstFrame, &lastFrame);
    if(_tree.isOutputAViewer()){
        _timeline->setFrameRange(firstFrame, lastFrame);
    }else{
        Natron::OutputEffectInstance* output = dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput());
        output->setFirstFrame(firstFrame);
        output->setLastFrame(lastFrame);
    }

    
    if(_currentRunArgs._refreshTree)
        _tree.refreshTree(firstFrame);/*refresh the tree*/
    
    
    ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(_tree.getOutput()); /*viewer might be NULL if the output is smthing else*/
    
    bool hasInput = false;
    for (RenderTree::TreeIterator it = _tree.begin() ; it != _tree.end() ; ++it) {
        if(it->first->isInputNode()){
            hasInput = true;
            break;
        }
    }

    if(!hasInput){
        if(viewer)
            viewer->disconnectViewer();
        return false;
    }
    
    {
        QMutexLocker workingLocker(&_workingMutex);
        _working = true;
    }

    /*beginRenderAction for all openFX nodes*/
    for (RenderTree::TreeIterator it = _tree.begin(); it!=_tree.end(); ++it) {
        OfxEffectInstance* n = dynamic_cast<OfxEffectInstance*>(it->second);
        if(n) {
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            assert(n->effectInstance());
            OfxStatus stat;
            stat = n->effectInstance()->beginRenderAction(_timeline->leftBound(),_timeline->rightBound(), //frame range
                                                          1, // frame step
                                                          true, //is interactive
                                                          renderScale); //scale
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            
        }
    }
    
    if(!_currentRunArgs._sameFrame){
        emit engineStarted(_currentRunArgs._forward);
        _timer->playState = RUNNING; /*activating the timer*/

    }
    return true;

}
bool VideoEngine::stopEngine() {
    /*reset the abort flag and wake up any thread waiting*/
    {
        // make sure startEngine is not running by locking _abortBeingProcessedMutex
        QMutexLocker abortBeingProcessedLocker(&_abortBeingProcessedMutex);
        _abortBeingProcessed = true; //_abortBeingProcessed is a dummy variable: it should be always false when stopeEngine is not running
        {
            QMutexLocker l(&_abortedRequestedMutex);
            _abortRequested = 0;
            
            /*refresh preview for all nodes that have preview enabled & set the aborted flag to false.
           .*/
            for (RenderTree::TreeIterator it = _tree.begin(); it != _tree.end(); ++it) {
                if(it->second->isPreviewEnabled()){
                    it->second->getNode()->refreshPreviewImage(_timeline->currentFrame());
                }
                it->second->setAborted(false);
            }
            
            _abortedRequestedCondition.wakeOne();
        }
        
        if(_tree.getOutput()->getApp()->isBackground()){
            std::cout << kRenderingFinishedString << std::endl;
        }

        emit engineStopped();
        
        _currentRunArgs._frameRequestsCount = 0;
        _restart = true;
        _timer->playState = PAUSE;
        

        {
            QMutexLocker workingLocker(&_workingMutex);
            _working = false;
        }

        _abortBeingProcessed = false;

    }
    
    
    /*endRenderAction for all openFX nodes*/
    for (RenderTree::TreeIterator it = _tree.begin(); it!=_tree.end(); ++it) {
        OfxEffectInstance* n = dynamic_cast<OfxEffectInstance*>(it->second);
        if(n){
            OfxPointD renderScale;
            renderScale.x = renderScale.y = 1.0;
            assert(n->effectInstance());
            OfxStatus stat;
            stat = n->effectInstance()->endRenderAction(_timeline->leftBound(),_timeline->rightBound(), 1, true, renderScale);
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        }
    }

    if(_processAborter){
        _processAborter->stopChecking();
    }

    {
        QMutexLocker locker(&_mustQuitMutex);
        if (_mustQuit) {
            _mustQuit = false;
            _mustQuitCondition.wakeAll();
            return true;
        }
    }
    if(_tree.getOutput()->getApp()->isBackground()){
        _tree.getOutput()->getApp()->notifyRenderFinished(dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput()));
        return true;
    }
    
    /*pause the thread if needed*/
    {
        QMutexLocker locker(&_startMutex);
        while(_startCount <= 0) {
            _startCondition.wait(&_startMutex);
        }
        _startCount = 0;
    }
    return false;
    
    
}
void VideoEngine::getFrameRange(int *firstFrame,int *lastFrame) const {
    if(_tree.getOutput()){
        _tree.getOutput()->getFrameRange(firstFrame, lastFrame);
        if(*firstFrame == INT_MIN){
            *firstFrame = _timeline->firstFrame();
        }
        if(*lastFrame == INT_MAX){
            *lastFrame = _timeline->lastFrame();
        }
    }else{
        *firstFrame = _timeline->firstFrame();
        *lastFrame = _timeline->lastFrame();
    }
}

void VideoEngine::run(){
    
    for(;;){ // infinite loop
        {
            /*First-off, check if the node holding this engine got deleted
             in which case we must quit the engine.*/
            QMutexLocker locker(&_mustQuitMutex);
            if(_mustQuit) {
                _mustQuit = false;
                _mustQuitCondition.wakeAll();
                return;
            }
        }
        
        /*If restart is on, start the engine. Restart is on for the 1st frame
         rendered of a sequence.*/
        if(_restart){
            if(!startEngine()){
                if(stopEngine())
                    return;
                continue;
            }
        }
        
        
        if (!_currentRunArgs._sameFrame && _currentRunArgs._frameRequestsCount == -1) {
            appPTR->clearNodeCache();
        }
        
        Natron::OutputEffectInstance* output = dynamic_cast<Natron::OutputEffectInstance*>(_tree.getOutput());
        assert(output);
        ViewerInstance* viewer = _tree.outputAsViewer();
        
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

        } else if(!_currentRunArgs._sameFrame) {
            assert(_currentRunArgs._recursiveCall); // we're in the else part
            if (!viewer) {
                output->setCurrentFrame(output->getCurrentFrame()+1);
                currentFrame = output->getCurrentFrame();
                if(currentFrame > lastFrame){
                    if(stopEngine()) {
                        return;
                    }
                    continue;
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
                            if (stopEngine()) {
                                return;
                            }
                            continue;
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
                            if (stopEngine()) {
                                return;
                            }
                            continue;
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
            if(_abortRequested || // #1 aborted by the user

                    (_tree.isOutputAViewer() // #2 the Tree contains only 1 frame and we rendered it
                     &&  _currentRunArgs._recursiveCall
                     &&  firstFrame == lastFrame
                     && _currentRunArgs._frameRequestsCount == -1
                     && _currentRunArgs._frameRequestIndex == 1)

                    || _currentRunArgs._frameRequestsCount == 0) // #3 the sequence ended and it was not an infinite run
            {
                locker.unlock();
                if(stopEngine())
                    return;
                continue;
            }
        }

        ////////////////////////
        // Render currentFrame
        //
        // TODO: everything inside the try..catch should be moved to a separate function

        Status stat;
        // if the output is a writer, _tree.outputAsWriter() returns a valid pointer/
        Writer* writer = _tree.outputAsWriter();
        bool continueOnError = (writer && writer->continueOnError());
        assert(!viewer || !writer); // output cannot be both a viewer and a writer

        try {
            /*update the tree hash */
            _tree.refreshKnobsAndHashAndClearPersistentMessage(currentFrame);

            /*pre process frame*/

            stat = _tree.preProcessFrame(currentFrame);
            if (stat == StatFailed) {
                if (viewer) {
                    viewer->disconnectViewer();
                }
                throw std::runtime_error("preProcessFrame failed");
            }

            /*get the time at which we started rendering the frame*/
            gettimeofday(&_startRenderFrameTime, 0);
            if (viewer && !_tree.isOutputAnOpenFXNode()) {
                assert(viewer);
                stat = viewer->renderViewer(currentFrame, _currentRunArgs._fitToViewer);

                if (!_currentRunArgs._sameFrame) {
                    QMutexLocker timerLocker(&_timerMutex);
                    _timer->waitUntilNextFrameIsDue(); // timer synchronizing with the requested fps
                    if ((_timerFrameCount % NATRON_FPS_REFRESH_RATE) == 0) {
                        emit fpsChanged(_timer->actualFrameRate()); // refreshing fps display on the GUI
                        _timerFrameCount = 1; //reseting to 1
                    } else {
                        ++_timerFrameCount;
                    }

                }



                if (stat == StatFailed) {
                    viewer->disconnectViewer();
                }
            } else if (writer && !_tree.isOutputAnOpenFXNode()) {
                stat = writer->renderWriter(currentFrame);
            } else {
                RenderScale scale;
                scale.x = scale.y = 1.;
                RectI rod;
                stat = _tree.getOutput()->getRegionOfDefinition(currentFrame, &rod);
                if(stat != StatFailed){
                    int viewsCount = _tree.getOutput()->getApp()->getCurrentProjectViewsCount();
                    for(int i = 0; i < viewsCount;++i){
                        // Do not catch exceptions: if an exception occurs here it is probably fatal, since
                        // it comes from Natron itself. All exceptions from plugins are already caught
                        // by the HostSupport library.
                        (void)_tree.getOutput()->renderRoI(currentFrame, scale,i ,rod);
                    }
                }

            }
        } catch (const std::exception &e) {
            stat = StatFailed;
            std::stringstream ss;
            ss << "Error while rendering" << " frame " << currentFrame << ": " << e.what();
            Natron::errorDialog("Error while rendering", ss.str());
        } catch (...) {
            stat = StatFailed;
            std::stringstream ss;
            ss << "Error while rendering" << " frame " << currentFrame;
            Natron::errorDialog("Error while rendering", ss.str());
        }
        if (!continueOnError && stat == StatFailed) {
            if(stopEngine())
                return;
            continue;
        }

        /*The frame has been rendered , we call engineLoop() which will reset all the flags,
         update viewers
         and appropriately increment counters for the next frame in the sequence.*/
        emit frameRendered(currentFrame);
        if(_tree.getOutput()->getApp()->isBackground()){
            std::cout << kFrameRenderedString << currentFrame << std::endl;
        }

        if(_currentRunArgs._frameRequestIndex == 0 && _currentRunArgs._frameRequestsCount == 1 && !_currentRunArgs._sameFrame){
            _currentRunArgs._frameRequestsCount = 0;
        }else if(_currentRunArgs._frameRequestsCount!=-1){ // if the frameRequestCount is defined (i.e: not indefinitely running)
            --_currentRunArgs._frameRequestsCount;
        }
        ++_currentRunArgs._frameRequestIndex;//incrementing the frame counter
        
        _currentRunArgs._fitToViewer = false;
        _currentRunArgs._recursiveCall = true;
    } // end for(;;)
    
}
void VideoEngine::onProgressUpdate(int /*i*/){
    // cout << "progress: index = " << i ;
    //    if(i < (int)_currentFrameInfos._rows.size()){
    //        //  cout <<" y = "<< _lastFrameInfos._rows[i] << endl;
    //        checkAndDisplayProgress(_currentFrameInfos._rows[i],i);
    //    }
}


void VideoEngine::abortRendering(){
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
            it->second->setAborted(true);
        }
        
        // _abortedCondition.wakeOne();
    }
}


void VideoEngine::refreshAndContinueRender(bool initViewer){
    //the changes will occur upon the next frame rendered. If the playback is running indefinately
    //we're sure that there will be a refresh. If the playback is for a determined amount of frame
    //we've to make sure the playback is not rendering the last frame, in which case we wouldn't see
    //the last changes.
    //The default case is if the playback is not running: just render the current frame

    bool isPlaybackRunning = isWorking() && (_currentRunArgs._frameRequestsCount == -1 ||
                                             (_currentRunArgs._frameRequestsCount > 1 && _currentRunArgs._frameRequestIndex < _currentRunArgs._frameRequestsCount - 1));
    if(!isPlaybackRunning){
        render(1,false,initViewer,_currentRunArgs._forward,true);
    }
}
void VideoEngine::updateTreeAndContinueRender(bool initViewer){
    //this is a bit more trickier than refreshAndContinueRender, we've to stop
    //the playback, and request a new render
    bool isPlaybackRunning = isWorking() && (_currentRunArgs._frameRequestsCount == -1 ||
                                             (_currentRunArgs._frameRequestsCount > 1 && _currentRunArgs._frameRequestIndex < _currentRunArgs._frameRequestsCount - 1));

    if(isPlaybackRunning){
        int count = _currentRunArgs._frameRequestsCount == - 1 ? -1 :
                                                                 _currentRunArgs._frameRequestsCount - _currentRunArgs._frameRequestIndex ;
        abortRendering();
        render(count,true,initViewer,_currentRunArgs._forward,false);
    }else{
        render(1,true,initViewer,_currentRunArgs._forward,true);
    }
}


RenderTree::RenderTree(EffectInstance *output):
    _output(output)
  ,_sorted()
  ,_isViewer(false)
  ,_isOutputOpenFXNode(false)
  ,_treeMutex(QMutex::Recursive) /*recursive lock*/
  ,_firstFrame(0)
  ,_lastFrame(0)
  ,_treeVersionValid(false)
  ,_renderOutputFormat()
  ,_projectViewsCount(1)
{
    assert(output);
}


void RenderTree::clearGraph(){
    for(TreeContainer::const_iterator it = _sorted.begin();it!=_sorted.end();++it) {
        (*it).second->setMarkedByTopologicalSort(false);
    }
    _sorted.clear();
}

void RenderTree::refreshTree(SequenceTime time){
    _isViewer = dynamic_cast<ViewerInstance*>(_output) != NULL;
    _isOutputOpenFXNode = _output->isOpenFX();
    
    
    /*unmark all nodes already present in the graph*/
    clearGraph();
    fillGraph(_output);

    std::vector<U64> inputsHash;
    for(TreeContainer::iterator it = _sorted.begin();it!=_sorted.end();++it) {
        it->second->setMarkedByTopologicalSort(false);
        it->second->updateInputs(this);
        U64 ret = 0;
        it->second->clone(time);
        ret = it->second->computeHash(time,inputsHash);
        inputsHash.push_back(ret);
    }
    
}


void RenderTree::fillGraph(EffectInstance *effect){
    
    /*call fillGraph recursivly on all the node's inputs*/
    const Node::InputMap& inputs = effect->getNode()->getInputs();
    for(Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it){
        if(it->second){
            /*if the node is an inspector*/
            const InspectorNode* insp = dynamic_cast<const InspectorNode*>(effect->getNode());
            if (insp && it->first != insp->activeInput()) {
                continue;
            }
            Natron::EffectInstance* inputEffect = it->second->findOrCreateLiveInstanceClone(this);
            fillGraph(inputEffect);
        }
    }
    if(!effect->isMarkedByTopologicalSort()){
        effect->setMarkedByTopologicalSort(true);
        _sorted.push_back(std::make_pair(effect->getNode(),(EffectInstance*)effect));
    }
}

U64 RenderTree::cloneKnobsAndcomputeTreeHash(SequenceTime time,EffectInstance* effect,const std::vector<U64>& inputsHashs){
    U64 ret = effect->hash().value();
    if(!effect->isHashValid()){
        effect->clone(time);
        ret = effect->computeHash(time,inputsHashs);
        //  std::cout << effect->getName() << ": " << ret << std::endl;
    }
    return ret;
}
void RenderTree::refreshKnobsAndHashAndClearPersistentMessage(SequenceTime time){
    _renderOutputFormat = _output->getApp()->getProjectFormat();
    _projectViewsCount = _output->getApp()->getCurrentProjectViewsCount();
    
    //    bool oldVersionValid = _treeVersionValid;
    //    U64 oldVersion = 0;
    //    if (oldVersionValid) {
    //        oldVersion = _output->hash().value();
    //    }
    
    /*Computing the hash of the tree in topological ordering.
     For each effect in the tree, the hash of its inputs is guaranteed to have
     been computed.*/
    std::vector<U64> inputsHash;
    for (TreeIterator it = _sorted.begin(); it!=_sorted.end(); ++it) {
        inputsHash.push_back(cloneKnobsAndcomputeTreeHash(time,it->second,inputsHash));
        (*it).second->clearPersistentMessage();
    }
    _treeVersionValid = true;
    
    //    /*If the hash changed we clear the playback cache.*/
    //    if((!oldVersionValid || (inputsHash.back() != oldVersion)) && !_output->getApp()->isBackground()){
    //        appPTR->clearPlaybackCache();
    //    }
    

}

Natron::EffectInstance* RenderTree::getEffectForNode(Natron::Node* node) const{
    for(TreeIterator it = _sorted.begin();it!=_sorted.end();++it){
        if (it->first == node) {
            return it->second;
        }
    }
    return NULL;
}

ViewerInstance* RenderTree::outputAsViewer() const {
    if(_output && _isViewer){
        return dynamic_cast<ViewerInstance*>(_output);
    }else{
        return NULL;
    }
}

Writer* RenderTree::outputAsWriter() const {
    if(_output && !_isViewer){
        return dynamic_cast<Writer*>(_output);
    }else{
        return NULL;
    }
}


void RenderTree::debug() const{
    cout << "Topological ordering of the Tree is..." << endl;
    for(RenderTree::TreeIterator it = begin(); it != end() ;++it) {
        cout << it->first->getName() << endl;
    }
}

Natron::Status RenderTree::preProcessFrame(SequenceTime time){
    /*Validating the Tree in topological order*/
    for (TreeIterator it = begin(); it != end(); ++it) {
        for (int i = 0; i < it->second->maximumInputs(); ++i) {
            if (!it->second->input(i) && !it->second->isInputOptional(i)) {
                return StatFailed;
            }
        }
        Natron::Status st = it->second->preProcessFrame(time);
        if(st == Natron::StatFailed){
            return st;
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

BackgroundProcessAborter::BackgroundProcessAborter(VideoEngine* engine)
    :
      _engine(engine)
{
    setTerminationEnabled();
}

void BackgroundProcessAborter::run(){
    for(;;){

        QTextStream qstdin(stdin);
        QString line = qstdin.readLine();
        Log::beginFunction("ProcessAborter","run");
        Log::print(QString("Process received message "+line).toStdString());
        Log::endFunction("ProcessAborter","run");
        if(line.contains(kAbortRenderingString)){
            _engine->abortRendering();
        }

        msleep(1000);
    }
}

void BackgroundProcessAborter::stopChecking(){
    terminate();
}

BackgroundProcessAborter::~BackgroundProcessAborter(){}


