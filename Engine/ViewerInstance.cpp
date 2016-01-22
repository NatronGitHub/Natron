/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ViewerInstance.h"
#include "ViewerInstancePrivate.h"

#include <algorithm> // min, max
#include <stdexcept>

#include <boost/shared_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QtGlobal>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
#include <QtCore/QFutureWatcher>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QCoreApplication>
#include <QtCore/QThreadPool>
CLANG_DIAG_ON(deprecated)

#include "Global/MemoryInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/Image.h"
#include "Engine/ImageInfo.h"
#include "Engine/ImageInfo.h"
#include "Engine/Log.h"
#include "Engine/Lut.h"
#include "Engine/MemoryFile.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Timer.h"


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif

#define NATRON_TIME_ELASPED_BEFORE_PROGRESS_REPORT 0.4

NATRON_NAMESPACE_ENTER;

using std::make_pair;
using boost::shared_ptr;


static void scaleToTexture8bits(const RectI& roi,
                                const RenderViewerArgs & args,
                                ViewerInstance* viewer,
                                U32* output);
static void scaleToTexture32bits(const RectI& roi,
                                 const RenderViewerArgs & args,
                                 float *output);
static std::pair<double, double>
findAutoContrastVminVmax(boost::shared_ptr<const Image> inputImage,
                         DisplayChannelsEnum channels,
                         const RectI & rect);
static void renderFunctor(const RectI& roi,
                          const RenderViewerArgs & args,
                          ViewerInstance* viewer,
                          void *buffer);

/**
 *@brief Actually converting to ARGB... but it is called BGRA by
   the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

const Color::Lut*
ViewerInstance::lutFromColorspace(ViewerColorSpaceEnum cs)
{
    const Color::Lut* lut;

    switch (cs) {
    case eViewerColorSpaceSRGB:
        lut = Color::LutManager::sRGBLut();
        break;
    case eViewerColorSpaceRec709:
        lut = Color::LutManager::Rec709Lut();
        break;
    case eViewerColorSpaceLinear:
    default:
        lut = 0;
        break;
    }
    if (lut) {
        lut->validate();
    }

    return lut;
}


class ViewerRenderingStarted_RAII
{
    ViewerInstance* _node;
    bool _didEmit;
public:
    
    ViewerRenderingStarted_RAII(ViewerInstance* node)
    : _node(node)
    {
        _didEmit = node->getNode()->notifyRenderingStarted();
        if (_didEmit) {
            _node->s_viewerRenderingStarted();
        }
    }
    
    ~ViewerRenderingStarted_RAII()
    {
        if (_didEmit) {
            _node->getNode()->notifyRenderingEnded();
            _node->s_viewerRenderingEnded();
        }
    }
};


EffectInstance*
ViewerInstance::BuildEffect(boost::shared_ptr<Node> n)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return new ViewerInstance(n);
}

ViewerInstance::ViewerInstance(boost::shared_ptr<Node> node)
    : OutputEffectInstance(node)
      , _imp( new ViewerInstancePrivate(this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);
    
    QObject::connect( this,SIGNAL( disconnectTextureRequest(int) ),this,SLOT( executeDisconnectTextureRequestOnMainThread(int) ) );
    QObject::connect( _imp.get(),SIGNAL( mustRedrawViewer() ),this,SLOT( redrawViewer() ) );
    QObject::connect( this,SIGNAL( s_callRedrawOnMainThread() ), this, SLOT( redrawViewer() ) );
}

ViewerInstance::~ViewerInstance()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    
    // If _imp->updateViewerRunning is true, that means that the next updateViewer call was
    // not yet processed. Since we're in the main thread and it is processed in the main thread,
    // there is no way to wait for it (locking the mutex would cause a deadlock).
    // We don't care, after all.
    //{
    //    QMutexLocker locker(&_imp->updateViewerMutex);
    //    assert(!_imp->updateViewerRunning);
    //}
    if (_imp->uiContext) {
        _imp->uiContext->removeGUI();
    }
}

RenderEngine*
ViewerInstance::createRenderEngine()
{
    return new ViewerRenderEngine(this);
}

void
ViewerInstance::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_IMAGE);
}

OpenGLViewerI*
ViewerInstance::getUiContext() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->uiContext;
}

void
ViewerInstance::forceFullComputationOnNextFrame()
{
    // this is called by the GUI when the user presses the "Refresh" button.
    // It set the flag forceRender to true, meaning no cache will be used.

    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QMutexLocker forceRenderLocker(&_imp->forceRenderMutex);
    _imp->forceRender = true;
}

void
ViewerInstance::clearLastRenderedImage()
{
    EffectInstance::clearLastRenderedImage();
    if (_imp->uiContext) {
        _imp->uiContext->clearLastRenderedImage();
    }
    {
        QMutexLocker k(&_imp->lastRotoPaintTickParamsMutex);
        _imp->lastRotoPaintTickParams[0].reset();
        _imp->lastRotoPaintTickParams[1].reset();
    }

}

void
ViewerInstance::setUiContext(OpenGLViewerI* viewer)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    _imp->uiContext = viewer;
}

void
ViewerInstance::invalidateUiContext()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->uiContext = NULL;
}



int
ViewerInstance::getMaxInputCount() const
{
    // runs in the render thread or in the main thread
    
    //MT-safe
    return getNode()->getMaxInputCount();
}

void
ViewerInstance::getFrameRange(double *first,
                              double *last)
{

    double inpFirst = 1,inpLast = 1;
    int activeInputs[2];
    getActiveInputs(activeInputs[0], activeInputs[1]);
    EffectInstance* n1 = getInput(activeInputs[0]);
    if (n1) {
        n1->getFrameRange_public(n1->getRenderHash(),&inpFirst,&inpLast);
    }
    *first = inpFirst;
    *last = inpLast;

    inpFirst = 1;
    inpLast = 1;

    EffectInstance* n2 = getInput(activeInputs[1]);
    if (n2) {
        n2->getFrameRange_public(n2->getRenderHash(),&inpFirst,&inpLast);
        if (inpFirst < *first) {
            *first = inpFirst;
        }

        if (inpLast > *last) {
            *last = inpLast;
        }
    }
}

void
ViewerInstance::executeDisconnectTextureRequestOnMainThread(int index)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->uiContext) {
        _imp->uiContext->disconnectInputTexture(index);
    }
}


static bool isRotoPaintNodeInputRecursive(Node* node,const NodePtr& rotoPaintNode) {
    
    if (node == rotoPaintNode.get()) {
        return true;
    }
    int maxInputs = node->getMaxInputCount();
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr input = node->getInput(i);
        if (input) {
            if (input == rotoPaintNode) {
                return true;
            } else {
                bool ret = isRotoPaintNodeInputRecursive(input.get(), rotoPaintNode);
                if (ret) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void updateLastStrokeDataRecursively(Node* node,const NodePtr& rotoPaintNode, const RectD& lastStrokeBbox,
                                            bool invalidate)
{
    if (isRotoPaintNodeInputRecursive(node, rotoPaintNode)) {
        if (invalidate) {
            node->invalidateLastPaintStrokeDataNoRotopaint();
        } else {
            node->setLastPaintStrokeDataNoRotopaint();
        }
        
        if (node == rotoPaintNode.get()) {
            return;
        }
        int maxInputs = node->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            NodePtr input = node->getInput(i);
            if (input) {
                updateLastStrokeDataRecursively(input.get(), rotoPaintNode, lastStrokeBbox, invalidate);
            }
        }
    }
}


class ViewerParallelRenderArgsSetter : public ParallelRenderArgsSetter
{
    NodePtr rotoNode;
    NodePtr viewerNode;
    NodePtr viewerInputNode;
public:
    
    ViewerParallelRenderArgsSetter(double time,
                                   int view,
                                   bool isRenderUserInteraction,
                                   bool isSequential,
                                   bool canAbort,
                                   U64 renderAge,
                                   const boost::shared_ptr<Node>& treeRoot,
                                   const FrameRequestMap* request,
                                   int textureIndex,
                                   const TimeLine* timeline,
                                   bool isAnalysis,
                                   const NodePtr& rotoPaintNode,
                                   const NodePtr& viewerInput,
                                   bool draftMode,
                                   bool viewerProgressReportEnabled,
                                   const boost::shared_ptr<RenderStats>& stats)
    : ParallelRenderArgsSetter(time,view,isRenderUserInteraction,isSequential,canAbort,renderAge,treeRoot, request,textureIndex,timeline,rotoPaintNode, isAnalysis, draftMode, viewerProgressReportEnabled,stats)
    , rotoNode(rotoPaintNode)
    , viewerNode(treeRoot)
    , viewerInputNode()
    {
        
        ///There can be a case where the viewer input tree does not belong to the project, for example
        ///for the File Dialog preview.
        if (viewerInput && !viewerInput->getGroup()) {
            viewerInputNode = viewerInput;
            bool doNanHandling = appPTR->getCurrentSettings()->isNaNHandlingEnabled();
            
            U64 nodeHash = 0;
            bool hashSet = false;
            boost::shared_ptr<NodeFrameRequest> nodeRequest;
            if (request) {
                FrameRequestMap::const_iterator foundRequest = request->find(viewerInputNode);
                if (foundRequest != request->end()) {
                    nodeRequest = foundRequest->second;
                    nodeHash = nodeRequest->nodeHash;
                    hashSet = true;
                }
            }
            
            if (!hashSet) {
                nodeHash = viewerInput->getHashValue();
            }
            
            viewerInput->getLiveInstance()->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, canAbort, nodeHash,  renderAge, treeRoot, nodeRequest, textureIndex, timeline, isAnalysis, false, NodeList(), viewerInput->getCurrentRenderThreadSafety(), doNanHandling, draftMode, viewerProgressReportEnabled,stats);
        }
    }
    
    virtual ~ViewerParallelRenderArgsSetter()
    {
        if (rotoNode) {
            updateLastStrokeDataRecursively(viewerNode.get(), rotoNode, RectD(), true);
        }
        if (viewerInputNode) {
            viewerInputNode->getLiveInstance()->invalidateParallelRenderArgsTLS();
        }
    }
};



ViewerInstance::ViewerRenderRetCode
ViewerInstance::getViewerArgsAndRenderViewer(SequenceTime time,
                                             bool canAbort,
                                             int view,
                                             U64 viewerHash,
                                             const boost::shared_ptr<Node>& rotoPaintNode,
                                             const boost::shared_ptr<RotoStrokeItem>& activeStroke,
                                             const boost::shared_ptr<RenderStats>& stats,
                                             boost::shared_ptr<ViewerArgs>* argsA,
                                             boost::shared_ptr<ViewerArgs>* argsB)
{
    ///This is used only by the rotopaint while drawing. We must clear the action cache of the rotopaint node before calling
    ///getRoD or this will not work
    assert(rotoPaintNode);
    if (!rotoPaintNode->getLiveInstance()) {
        return eViewerRenderRetCodeFail;
    }
    rotoPaintNode->getLiveInstance()->clearActionsCache();
    
    ViewerRenderRetCode status[2] = {
        eViewerRenderRetCodeFail, eViewerRenderRetCodeFail
    };

    NodePtr thisNode = getNode();
    
    
    boost::shared_ptr<ViewerArgs> args[2];
    for (int i = 0; i < 2; ++i) {
        args[i].reset(new ViewerArgs);
        if ( (i == 1) && (_imp->uiContext->getCompositingOperator() == eViewerCompositingOperatorNone) ) {
            break;
        }
        
        U64 renderAge = _imp->getRenderAgeAndIncrement(i);

        
        /*FrameRequestMap request;
        RectI roi;
        {
            roi.x1 = args[i]->params->textureRect.x1;
            roi.y1 = args[i]->params->textureRect.y1;
            roi.x2 = args[i]->params->textureRect.x2;
            roi.y2 = args[i]->params->textureRect.y2;
        }
        status[i] = EffectInstance::computeRequestPass(time, view, args[i]->params->mipMapLevel, roi, thisNode, request);
        if (status[i] == eStatusFailed) {
            continue;
        }*/
        
       
        
        ViewerParallelRenderArgsSetter tls(time,
                                           view,
                                           true,
                                           false,
                                           canAbort,
                                           renderAge,
                                           thisNode,
                                           0,
                                           i,
                                           getTimeline().get(),
                                           false,
                                           rotoPaintNode,
                                           NodePtr(),
                                           false,
                                           false,
                                           stats);
        
        NodeList rotoPaintNodes;
        if (rotoPaintNode) {
            if (activeStroke) {
                EffectInstance* rotoLive = rotoPaintNode->getLiveInstance();
                assert(rotoLive);
                bool ok = rotoLive->getThreadLocalRotoPaintTreeNodes(&rotoPaintNodes);
                assert(ok);
                if (!ok) {
                    throw std::logic_error("ViewerParallelRenderArgsSetter(): getThreadLocalRotoPaintTreeNodes() failed");
                }
                
                /*
                 Take from the stroke all the points that were input by the user so far on the main thread and set them globally to the
                 application. These data are the ones that are going to be used by any Roto related tool. We ensure that they all access
                 the same data so we only access the real Roto datas now.
                 */
                
                //The last points input by the user
                std::list<std::pair<Point,double> > lastStrokePoints;
                
                //The stroke RoD so far
                RectD wholeStrokeRod;
                
                //The bbox of the lastStrokePoints
                RectD lastStrokeBbox;
                
                //The index in the stroke of the last point we have rendered and up to the new point we have rendered
                int lastAge,newAge;
                
                //get on the app object the last point index we have rendered on this stroke
                lastAge = getApp()->getStrokeLastIndex();
                
                //Get the active paint stroke so far and its multi-stroke index
                boost::shared_ptr<RotoStrokeItem> currentlyPaintedStroke;
                int currentlyPaintedStrokeMultiIndex;
                getApp()->getStrokeAndMultiStrokeIndex(&currentlyPaintedStroke, &currentlyPaintedStrokeMultiIndex);

                
                //If this crashes here that means the user could start a new stroke while this one is not done rendering.
                assert(currentlyPaintedStroke == activeStroke);
                
                //the multi-stroke index in case of a stroke containing multiple strokes from the user
                int strokeIndex;
                if (activeStroke->getMostRecentStrokeChangesSinceAge(time, lastAge, currentlyPaintedStrokeMultiIndex, &lastStrokePoints, &lastStrokeBbox, &wholeStrokeRod ,&newAge,&strokeIndex)) {
                

                    
                    getApp()->updateLastPaintStrokeData(newAge, lastStrokePoints, lastStrokeBbox, strokeIndex);
                    for (NodeList::iterator it = rotoPaintNodes.begin(); it!=rotoPaintNodes.end(); ++it) {
                        (*it)->prepareForNextPaintStrokeRender();
                        
                    }
                    updateLastStrokeDataRecursively(thisNode.get(), rotoPaintNode, lastStrokeBbox, false);
                } else {
                    ///The drawing is already up to date: all changes have been taken into account for this event
                    args[i].reset();
                    return eViewerRenderRetCodeRedraw;
                }
            }
        }
       
        
        if (args[i]) {
            status[i] = getRenderViewerArgsAndCheckCache(time, false, canAbort, view, i, viewerHash, rotoPaintNode, false, renderAge,stats,  args[i].get());
        }
        
    
        if (status[i] != eViewerRenderRetCodeRender) {
            /*
             Either failure, black or nothing, the texture is junk, remove it from the cache
             */
            if (args[i] && args[i]->params && args[i]->params->cachedFrame) {
                args[i]->params->cachedFrame->setAborted(true);
                appPTR->removeFromViewerCache(args[i]->params->cachedFrame);
                args[i]->params->cachedFrame.reset();
            }
            
        }

        
        if (status[i] == eViewerRenderRetCodeFail || status[i] == eViewerRenderRetCodeBlack) {
            disconnectTextureRequest(i);
        } else {
            assert(args[i] && args[i]->params);
            assert(args[i]->params->textureIndex == i);
            
            
            if (!_imp->addOngoingRender(args[i]->params->textureIndex, args[i]->params->renderAge)) {
                /*
                 This may fail if another thread already pushed a more recent render in the render ages queue
                 */
                status[i] = eViewerRenderRetCodeRedraw;
                args[i].reset();
            }
            
            if (args[i]) {
                status[i] = renderViewer_internal(view,
                                                  QThread::currentThread() == qApp->thread(), // singleThreaded
                                                  false, // isSequentialRender
                                                  viewerHash,
                                                  canAbort,
                                                  rotoPaintNode,
                                                  false, //useTLS
                                                  boost::shared_ptr<RequestedFrame>(),
                                                  stats,
                                                  *args[i]);
            }
            
            if (args[i] && args[i]->params) {
                if (status[i] == eViewerRenderRetCodeFail || status[i] == eViewerRenderRetCodeBlack) {
                    _imp->checkAndUpdateDisplayAge(args[i]->params->textureIndex,args[i]->params->renderAge);
                }
                _imp->removeOngoingRender(args[i]->params->textureIndex, args[i]->params->renderAge);
            }
            
            
            if (status[i] == eViewerRenderRetCodeRedraw) {
                args[i].reset();
            }
            
        }

        
     

        
    }
    
    if (status[0] == eViewerRenderRetCodeFail && status[1] == eViewerRenderRetCodeFail) {
        return eViewerRenderRetCodeFail;
    }

    *argsA = args[0];
    *argsB = args[1];
    return eViewerRenderRetCodeRender;
    
}

ViewerInstance::ViewerRenderRetCode
ViewerInstance::renderViewer(int view,
                             bool singleThreaded,
                             bool isSequentialRender,
                             U64 viewerHash,
                             bool canAbort,
                             const boost::shared_ptr<Node>& rotoPaintNode,
                             bool useTLS,
                             boost::shared_ptr<ViewerArgs> args[2],
                             const boost::shared_ptr<RequestedFrame>& request,
                             const boost::shared_ptr<RenderStats>& stats)
{
    if (!_imp->uiContext) {
        return eViewerRenderRetCodeFail;
    }

    /**
     * When entering this code, we already know that the textures are not cached for the A and B inputs
     * If the an input is not connected, it's args[i] will be NULL.
     * If either one of the renders fails, that means we should block playback and clear to black the viewer
     **/
    
    ViewerInstance::ViewerRenderRetCode ret[2] = {
        eViewerRenderRetCodeRedraw, eViewerRenderRetCodeRedraw
    };
    for (int i = 0; i < 2; ++i) {
        if ( (i == 1) && (_imp->uiContext->getCompositingOperator() == eViewerCompositingOperatorNone) ) {
            break;
        }
        if (args[i] && args[i]->params) {
            assert(args[i]->params->textureIndex == i);
            
            ///We enable render stats just for the A input (i == 0) otherwise we would get crappy results
            
            if (!isSequentialRender) {
                if (!_imp->addOngoingRender(args[i]->params->textureIndex, args[i]->params->renderAge)) {
                    /*
                     This may fail if another thread already pushed a more recent render in the render ages queue
                     */
                    ret[i] = eViewerRenderRetCodeRedraw;
                    args[i].reset();
                }
            }
            if (args[i]) {
                ret[i] = renderViewer_internal(view, singleThreaded, isSequentialRender, viewerHash, canAbort,rotoPaintNode, useTLS, request,
                                               i == 0 ? stats : boost::shared_ptr<RenderStats>(),
                                               *args[i]);
            }
            
            if (ret[i] != eViewerRenderRetCodeRender) {
                /*
                 Either failure, black or nothing, the texture is junk, remove it from the cache
                 */
                if (args[i] && args[i]->params && args[i]->params->cachedFrame) {
                    args[i]->params->cachedFrame->setAborted(true);
                    appPTR->removeFromViewerCache(args[i]->params->cachedFrame);
                    args[i]->params->cachedFrame.reset();
                }

            }
            
            if (!isSequentialRender && args[i] && args[i]->params) {
                if (ret[i] == eViewerRenderRetCodeFail || ret[i] == eViewerRenderRetCodeBlack) {
                    _imp->checkAndUpdateDisplayAge(args[i]->params->textureIndex,args[i]->params->renderAge);
                }
                _imp->removeOngoingRender(args[i]->params->textureIndex, args[i]->params->renderAge);
            }
            
            if (ret[i] == eViewerRenderRetCodeBlack) {
                disconnectTexture(args[i]->params->textureIndex);
            }
            
            if (ret[i] == eViewerRenderRetCodeFail || ret[i] == eViewerRenderRetCodeRedraw) {
                args[i].reset();
            }
        }
    }
    

    if ( (ret[0] == eViewerRenderRetCodeFail) || (ret[1] == eViewerRenderRetCodeFail) ) {
        return eViewerRenderRetCodeFail;
    }

    return eViewerRenderRetCodeRender;
}

static bool checkTreeCanRender_internal(Node* node, std::list<Node*>& marked)
{
    if (std::find(marked.begin(), marked.end(), node) != marked.end()) {
        return true;
    }

    marked.push_back(node);

    // check that the nodes upstream have all their nonoptional inputs connected
    int maxInput = node->getMaxInputCount();
    for (int i = 0; i < maxInput; ++i) {
        NodePtr input = node->getInput(i);
        bool optional = node->getLiveInstance()->isInputOptional(i);
        if (optional) {
            continue;
        }
        if (!input) {
            return false;
        } else {
            bool ret = checkTreeCanRender_internal(input.get(), marked);
            if (!ret) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Returns false if the tree has unconnected mandatory inputs
 **/
static bool checkTreeCanRender(Node* node)
{
    std::list<Node*> marked;
    bool ret = checkTreeCanRender_internal(node, marked);
    return ret;
}

static unsigned char* getTexPixel(int x,int y,const TextureRect& bounds, std::size_t pixelDepth, unsigned char* bufStart)
{
    if ( ( x < bounds.x1 ) || ( x >= bounds.x2 ) || ( y < bounds.y1 ) || ( y >= bounds.y2 )) {
        return NULL;
    } else {
        int compDataSize = pixelDepth * 4;
        
        return (unsigned char*)(bufStart)
        + (qint64)( y - bounds.y1 ) * compDataSize * bounds.w
        + (qint64)( x - bounds.x1 ) * compDataSize;
    }

}

static bool copyAndSwap(const TextureRect& srcRect,
                        const TextureRect& dstRect,
                        std::size_t dstBytesCount,
                        ImageBitDepthEnum bitdepth,
                        unsigned char* srcBuf,
                        unsigned char** dstBuf)
{
    //Ensure it has the correct size, resize it if needed
    if (srcRect.x1 == dstRect.x1 &&
        srcRect.y1 == dstRect.y1 &&
        srcRect.x2 == dstRect.x2 &&
        srcRect.y2 == dstRect.y2) {
        *dstBuf = srcBuf;
        return false;
    }
    
    //Use calloc so that newly allocated areas are already black and transparant
    unsigned char* tmpBuf = (unsigned char*)calloc(dstBytesCount, 1);
    
    if (!tmpBuf) {
        *dstBuf = 0;
        return true;
    }
    
    std::size_t pixelDepth = getSizeOfForBitDepth(bitdepth);
    
    unsigned char* dstPixels = getTexPixel(srcRect.x1, srcRect.y1, dstRect, pixelDepth, tmpBuf);
    assert(dstPixels);
    const unsigned char* srcPixels = getTexPixel(srcRect.x1, srcRect.y1, srcRect, pixelDepth, srcBuf);
    assert(srcPixels);
    
    std::size_t srcRowSize = srcRect.w * 4 * pixelDepth;
    std::size_t dstRowSize = dstRect.w * 4 * pixelDepth;
    
    for (int y = srcRect.y1; y < srcRect.y2;
         ++y, srcPixels += srcRowSize, dstPixels += dstRowSize) {
        memcpy(dstPixels, srcPixels, srcRowSize);
    }
    *dstBuf = tmpBuf;
    return true;
}

ViewerInstance::ViewerRenderRetCode
ViewerInstance::getRenderViewerArgsAndCheckCache_public(SequenceTime time,
                                                           bool isSequential,
                                                           bool canAbort,
                                                           int view,
                                                           int textureIndex,
                                                           U64 viewerHash,
                                                           const boost::shared_ptr<Node>& rotoPaintNode,
                                                           bool useTLS,
                                                           const boost::shared_ptr<RenderStats>& stats,
                                                           ViewerArgs* outArgs)
{
    U64 renderAge = _imp->getRenderAgeAndIncrement(textureIndex);
    
   
    
    ViewerRenderRetCode stat = getRenderViewerArgsAndCheckCache(time, isSequential, canAbort, view, textureIndex, viewerHash, rotoPaintNode, useTLS, renderAge, stats, outArgs);
    
    if (stat == eViewerRenderRetCodeFail || stat == eViewerRenderRetCodeBlack) {
        _imp->checkAndUpdateDisplayAge(textureIndex,renderAge);
    }
    
    return stat;
}

ViewerInstance::ViewerRenderRetCode
ViewerInstance::getRenderViewerArgsAndCheckCache(SequenceTime time,
                                                 bool isSequential,
                                                 bool canAbort,
                                                 int view,
                                                 int textureIndex,
                                                 U64 viewerHash,
                                                 const boost::shared_ptr<Node>& rotoPaintNode,
                                                 bool useTLS,
                                                 U64 renderAge,
                                                 const boost::shared_ptr<RenderStats>& stats,
                                                 ViewerArgs* outArgs)
{
    
    
    if (textureIndex == 0) {
        QMutexLocker l(&_imp->activeInputsMutex);
        outArgs->activeInputIndex =  _imp->activeInputs[0];
    } else {
        QMutexLocker l(&_imp->activeInputsMutex);
        outArgs->activeInputIndex =  _imp->activeInputs[1];
    }
    
    
    EffectInstance* upstreamInput = getInput(outArgs->activeInputIndex);
    
    if (upstreamInput) {
        outArgs->activeInputToRender = upstreamInput->getNearestNonDisabled();
    }
    
    if (!upstreamInput || !outArgs->activeInputToRender || !checkTreeCanRender(outArgs->activeInputToRender->getNode().get())) {
        return eViewerRenderRetCodeFail;
    }
    
    {
        QMutexLocker forceRenderLocker(&_imp->forceRenderMutex);
        outArgs->forceRender = _imp->forceRender;
        _imp->forceRender = false;
    }
    
    ///instead of calling getRegionOfDefinition on the active input, check the image cache
    ///to see whether the result of getRegionOfDefinition is already present. A cache lookup
    ///might be much cheaper than a call to getRegionOfDefinition.
    ///
    ///Note that we can't yet use the texture cache because we would need the TextureRect identifyin
    ///the texture in order to retrieve from the cache, but to make the TextureRect we need the RoD!
    RenderScale scale(1.);
    int mipMapLevel;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        scale.x = Image::getScaleFromMipMapLevel(_imp->viewerMipMapLevel);
        scale.y = scale.x;
        mipMapLevel = _imp->viewerMipMapLevel;
    }
    
    assert(_imp->uiContext);
    double zoomFactor = _imp->uiContext->getZoomFactor();
    int zoomMipMapLevel;
    {
        double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2,-std::ceil(std::log(zoomFactor) / M_LN2) );
        zoomMipMapLevel = std::log(closestPowerOf2) / M_LN2;
    }
    
    mipMapLevel = std::max( (double)mipMapLevel, (double)zoomMipMapLevel );
    
    // realMipMapLevel is the mipmap level if the auto proxy is not applied
   // unsigned int realMipMapLevel = mipMapLevel;
    outArgs->draftModeEnabled = getApp()->isDraftRenderEnabled();
    
    //The original mipMapLevel without draft applied
    unsigned originalMipMapLevel = mipMapLevel;
    
    if (outArgs->draftModeEnabled && appPTR->getCurrentSettings()->isAutoProxyEnabled()) {
        unsigned int autoProxyLevel = appPTR->getCurrentSettings()->getAutoProxyMipMapLevel();
        if (zoomFactor > 1) {
            //Decrease draft mode at each inverse mipmaplevel level taken
            unsigned int invLevel = Image::getLevelFromScale(1. / zoomFactor);
            if (invLevel < autoProxyLevel) {
                autoProxyLevel -= invLevel;
            } else {
                autoProxyLevel = 0;
            }
        }
        mipMapLevel = std::max(mipMapLevel, (int)autoProxyLevel);
    }
    
    // If it's eSupportsMaybe and mipMapLevel!=0, don't forget to update
    // this after the first call to getRegionOfDefinition().
    RenderScale scaleOne(1.);
    EffectInstance::SupportsEnum supportsRS = outArgs->activeInputToRender->supportsRenderScaleMaybe();
    scale.x = scale.y = Image::getScaleFromMipMapLevel(originalMipMapLevel);
    
    
    
    //The hash of the node to render
    outArgs->activeInputHash = outArgs->activeInputToRender->getHash();
    
    //The RoD returned by the plug-in
    RectD rod;
    
    //The bounds of the image in pixel coords.
    
    //Is this image originated from a generator which is dependent of the size of the project ? If true we should then discard the image from the cache
    //when the project's format changes.
    bool isRodProjectFormat = false;
    
    
    const double par = outArgs->activeInputToRender->getPreferredAspectRatio();
    
        
    ///need to set TLS for getROD()
    boost::shared_ptr<ParallelRenderArgsSetter> frameArgs;
    if (useTLS) {
        frameArgs.reset(new ParallelRenderArgsSetter(time,
                                                     view,
                                                     !isSequential,  // is this render due to user interaction ?
                                                     isSequential, // is this sequential ?
                                                     canAbort,
                                                     renderAge,
                                                     getNode(),
                                                     0, // request
                                                     textureIndex,
                                                     getTimeline().get(),
                                                     NodePtr(),
                                                     false,
                                                     outArgs->draftModeEnabled,
                                                     false,
                                                     stats));
    }
    
    /**
     * @brief Start flagging that we're rendering for as long as the viewer is active.
     **/
    outArgs->isRenderingFlag.reset(new RenderingFlagSetter(getNode().get()));
    
    
    ///Get the RoD here to be able to figure out what is the RoI of the Viewer.
    StatusEnum stat = outArgs->activeInputToRender->getRegionOfDefinition_public(outArgs->activeInputHash,time,
                                                                                 supportsRS ==  eSupportsNo ? scaleOne : scale,
                                                                                 view, &rod, &isRodProjectFormat);
    if (stat == eStatusFailed) {
        return eViewerRenderRetCodeFail;
    }
    // update scale after the first call to getRegionOfDefinition
    if ( (supportsRS == eSupportsMaybe) && (mipMapLevel != 0) ) {
        supportsRS = (outArgs->activeInputToRender)->supportsRenderScaleMaybe();
        if (supportsRS == eSupportsNo) {
            scale.x = scale.y = 1.;
        }
    }
    
    isRodProjectFormat = ifInfiniteclipRectToProjectDefault(&rod);
    
    assert(_imp->uiContext);
    
    {
        QMutexLocker locker(&_imp->viewerParamsMutex);
        outArgs->autoContrast = _imp->viewerParamsAutoContrast;
        outArgs->channels = _imp->viewerParamsChannels[textureIndex];
    }
    outArgs->userRoIEnabled = _imp->uiContext->isUserRegionOfInterestEnabled();

    /*computing the RoI*/
    
    ////Texrect is the coordinates of the 4 corners of the texture in the bounds with the current zoom
    ////factor taken into account.
    RectI roi;
    if (outArgs->autoContrast || outArgs->userRoIEnabled) {
        roi = _imp->uiContext->getExactImageRectangleDisplayed(rod, par, originalMipMapLevel);
    } else {
        roi = _imp->uiContext->getImageRectangleDisplayedRoundedToTileSize(rod, par, originalMipMapLevel);
    }
    
    if (roi.isNull()) {
        return eViewerRenderRetCodeBlack;
    }
    
    
    /////////////////////////////////////
    // start UpdateViewerParams scope
    //
    outArgs->params.reset(new UpdateViewerParams);
    outArgs->params->isSequential = isSequential;
    outArgs->params->renderAge = renderAge;
    outArgs->params->setUniqueID(textureIndex);
    outArgs->params->srcPremult = outArgs->activeInputToRender->getOutputPremultiplication();
    ///Texture rect contains the pixel coordinates in the image to be rendered
    outArgs->params->textureRect.x1 = roi.x1;
    outArgs->params->textureRect.x2 = roi.x2;
    outArgs->params->textureRect.y1 = roi.y1;
    outArgs->params->textureRect.y2 = roi.y2;
    outArgs->params->textureRect.w = roi.width();
    outArgs->params->textureRect.h = roi.height();
    outArgs->params->textureRect.closestPo2 = 1 << originalMipMapLevel;
    outArgs->params->textureRect.par = par;
    
    outArgs->params->bytesCount = outArgs->params->textureRect.w * outArgs->params->textureRect.h * 4;
    assert(outArgs->params->bytesCount > 0);
    
    assert(_imp->uiContext);
    outArgs->params->depth = _imp->uiContext->getBitDepth();
    if (outArgs->params->depth == eImageBitDepthFloat) {
        outArgs->params->bytesCount *= sizeof(float);
    }
    
    outArgs->params->time = time;
    outArgs->params->rod = rod;
    outArgs->params->mipMapLevel = (unsigned int)originalMipMapLevel;
    outArgs->params->textureIndex = textureIndex;

    {
        QMutexLocker locker(&_imp->viewerParamsMutex);
        outArgs->params->gain = _imp->viewerParamsGain;
        outArgs->params->gamma = _imp->viewerParamsGamma;
        outArgs->params->lut = _imp->viewerParamsLut;
        outArgs->params->layer = _imp->viewerParamsLayer;
        outArgs->params->alphaLayer = _imp->viewerParamsAlphaLayer;
        outArgs->params->alphaChannelName = _imp->viewerParamsAlphaChannelName;
    }
    {
        QWriteLocker k(&_imp->gammaLookupMutex);
        if (_imp->gammaLookup.empty()) {
            _imp->fillGammaLut(1. / outArgs->params->gamma);
        }
    }
    std::string inputToRenderName = outArgs->activeInputToRender->getNode()->getScriptName_mt_safe();
    
    //When in draft mode first try to get a texture without draft and then try with draft
    int lookups = outArgs->draftModeEnabled ? 2 : 1;
    
    for (int lookup = 0; lookup < lookups; ++lookup) {
        outArgs->key.reset(new FrameKey(getNode().get(),
                                        time,
                                        viewerHash,
                                        outArgs->params->gain,
                                        outArgs->params->gamma,
                                        outArgs->params->lut,
                                        (int)outArgs->params->depth,
                                        outArgs->channels,
                                        view,
                                        outArgs->params->textureRect,
                                        scale,
                                        inputToRenderName,
                                        outArgs->params->layer,
                                        outArgs->params->alphaLayer.getLayerName() + outArgs->params->alphaChannelName,
                                        outArgs->params->depth == eImageBitDepthFloat && supportsGLSL(),
                                        lookup == 1));
        
        bool isCached = false;
        
        ///we never use the texture cache when the user RoI is enabled, otherwise we would have
        ///zillions of textures in the cache, each a few pixels different.
        assert(_imp->uiContext);
        boost::shared_ptr<FrameParams> cachedFrameParams;
        
        if (!outArgs->userRoIEnabled && !outArgs->autoContrast && !rotoPaintNode.get()) {
            isCached = AppManager::getTextureFromCache(*(outArgs->key), &outArgs->params->cachedFrame);
            
            if (!isCached && lookups == 2 && lookup == 0) {
                //Lookup didn't work in non draft mode, try with draft mode now, update the texture rectangle
                
                roi = outArgs->autoContrast ? _imp->uiContext->getExactImageRectangleDisplayed(rod, par, mipMapLevel) :
                _imp->uiContext->getImageRectangleDisplayedRoundedToTileSize(rod, par, mipMapLevel);
                outArgs->params->textureRect.x1 = roi.x1;
                outArgs->params->textureRect.x2 = roi.x2;
                outArgs->params->textureRect.y1 = roi.y1;
                outArgs->params->textureRect.y2 = roi.y2;
                outArgs->params->textureRect.w = roi.width();
                outArgs->params->textureRect.h = roi.height();
                outArgs->params->textureRect.closestPo2 = 1 << mipMapLevel;
                outArgs->params->mipMapLevel = mipMapLevel;
                outArgs->params->bytesCount = outArgs->params->textureRect.w * outArgs->params->textureRect.h * 4;
                scale.x = scale.y = Image::getScaleFromMipMapLevel(mipMapLevel);
                if (outArgs->params->depth == eImageBitDepthFloat) {
                    outArgs->params->bytesCount *= sizeof(float);
                }
                continue;
            }
            
            ///if we want to force a refresh, we by-pass the cache
            if (outArgs->forceRender && outArgs->params->cachedFrame) {
                appPTR->removeFromViewerCache(outArgs->params->cachedFrame);
                isCached = false;
                outArgs->params->cachedFrame.reset();
            }
            
            if (isCached) {
                assert(outArgs->params->cachedFrame);
                cachedFrameParams = outArgs->params->cachedFrame->getParams();
            }
            
        }
        
        if (!isCached) {
            if (stats  && stats->isInDepthProfilingEnabled()) {
                stats->addCacheInfosForNode(getNode(), true, false);
            }
        } else {
        
            /// make sure we have the lock on the texture because it may be in the cache already
            ///but not yet allocated.
            FrameEntryLocker entryLocker(_imp.get());
            if (!entryLocker.tryLock(outArgs->params->cachedFrame)) {
                ///Another thread is rendering it, just return it is not useful to keep this thread waiting.
                return eViewerRenderRetCodeRedraw;
            }
            
            if (outArgs->params->cachedFrame->getAborted()) {
                ///The thread rendering the frame entry might have been aborted and the entry removed from the cache
                ///but another thread might successfully have found it in the cache. This flag is to notify it the frame
                ///is invalid.
                return eViewerRenderRetCodeRedraw;
            }
            
            // how do you make sure cachedFrame->data() is not freed after this line?
            ///It is not freed as long as the cachedFrame shared_ptr has a used_count greater than 1.
            ///Since it is used during the whole function scope it is guaranteed not to be freed before
            ///The viewer is actually done with it.
            /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
            ///
            
            
            outArgs->params->ramBuffer = outArgs->params->cachedFrame->data();
            
            if (stats && stats->isInDepthProfilingEnabled()) {
                stats->addCacheInfosForNode(getNode(), false, false);
            }
            
        }
        break;
    } // for (int lookup = 0; lookup < lookups; ++lookup) {
    return eViewerRenderRetCodeRender;
}

//if render was aborted, remove the frame from the cache as it contains only garbage
#define abortCheck(input) if (input->aborted()) { \
                                return eViewerRenderRetCodeRedraw; \
                          }

ViewerInstance::ViewerRenderRetCode
ViewerInstance::renderViewer_internal(int view,
                                      bool singleThreaded,
                                      bool isSequentialRender,
                                      U64 /*viewerHash*/,
                                      bool canAbort,
                                      boost::shared_ptr<Node> rotoPaintNode,
                                      bool useTLS,
                                      const boost::shared_ptr<RequestedFrame>& request,
                                      const boost::shared_ptr<RenderStats>& stats,
                                      ViewerArgs& inArgs)
{
    //Do not call this if the texture is already cached.
    assert(!inArgs.params->ramBuffer);
    
    /*
     * There are 3 types of renders: 
     * 1) Playback: the canAbort flag is set to true and the isSequentialRender flag is set to true
     * 2) Single frame abortable render: the canAbort flag is set to true and the isSequentialRender flag is set to false. Basically
     * this kind of render is triggered by any parameter change, timeline scrubbing, curve positioning, etc... In this mode each image
     * rendered concurrently by the viewer is probably different than another one (different hash key or time) hence we want to abort
     * ongoing renders that are no longer corresponding to anything relevant to the actual state of the GUI. On the other hand, the user 
     * still want a smooth feedback, e.g: If the user scrubs the timeline, we want to give him/her a continuous feedback, even with a
     * latency so it looks like it is actually seeking, otherwise it would just refresh the image upon the mouseRelease event because all
     * other renders would be aborted. To enable this behaviour, we ensure that at least 1 render is always running, even if it does not
     * correspond to the GUI state anymore.
     * 3) Single frame non-abortable render: the canAbort flag is set to false and the isSequentialRender flag is set to false. 
     */
  
    
    RectI roi;
    roi.x1 = inArgs.params->textureRect.x1;
    roi.y1 = inArgs.params->textureRect.y1;
    roi.x2 = inArgs.params->textureRect.x2;
    roi.y2 = inArgs.params->textureRect.y2;
    
    
    ///Check that we were not aborted already
    if ( !isSequentialRender && (inArgs.activeInputToRender->getHash() != inArgs.activeInputHash ||
                                 inArgs.params->time != getTimeline()->currentFrame()) ) {
        return eViewerRenderRetCodeRedraw;
    }
    
    ///Notify the gui we're rendering.
    ViewerRenderingStarted_RAII renderingNotifier(this);
    
    ///Don't allow different threads to write the texture entry
    FrameEntryLocker entryLocker(_imp.get());
    

    ///Make sure the parallel render args are set on the thread and die when rendering is finished
    boost::shared_ptr<ViewerParallelRenderArgsSetter> frameArgs;
    bool tilingProgressReportPrefEnabled = false;
    if (useTLS) {
        tilingProgressReportPrefEnabled = appPTR->getCurrentSettings()->isInViewerProgressReportEnabled();
        
        RectD canonicalRoi;
        roi.toCanonical(inArgs.params->mipMapLevel, inArgs.activeInputToRender->getPreferredAspectRatio(), inArgs.params->rod, &canonicalRoi);
        
        FrameRequestMap request;
        StatusEnum stat = EffectInstance::computeRequestPass(inArgs.params->time, view, inArgs.params->mipMapLevel, canonicalRoi, getNode(), request);
        if (stat == eStatusFailed) {
            return eViewerRenderRetCodeFail;
        }
        
        frameArgs.reset(new ViewerParallelRenderArgsSetter(inArgs.params->time,
                                                           view,
                                                           !isSequentialRender,
                                                           isSequentialRender,
                                                           canAbort,
                                                           inArgs.params->renderAge,
                                                           getNode(),
                                                           &request,
                                                           inArgs.params->textureIndex,
                                                           getTimeline().get(),
                                                           false,
                                                           rotoPaintNode,
                                                           inArgs.activeInputToRender->getNode(),
                                                           inArgs.draftModeEnabled,
                                                           tilingProgressReportPrefEnabled,
                                                           stats));
    }

    
    ///If the user RoI is enabled, the odds that we find a texture containing exactly the same portion
    ///is very low, we better render again (and let the NodeCache do the work) rather than just
    ///overload the ViewerCache which may become slowe
    assert(_imp->uiContext);
    RectI lastPaintBboxPixel;
    if (inArgs.forceRender || inArgs.userRoIEnabled || inArgs.autoContrast || rotoPaintNode.get() != 0) {
        
        assert(!inArgs.params->cachedFrame);
        //if we are actively painting, re-use the last texture instead of re-drawing everything
        if (rotoPaintNode) {
            
            
            QMutexLocker k(&_imp->lastRotoPaintTickParamsMutex);
            if (_imp->lastRotoPaintTickParams[inArgs.params->textureIndex] && inArgs.params->mipMapLevel == _imp->lastRotoPaintTickParams[inArgs.params->textureIndex]->mipMapLevel && inArgs.params->textureRect.contains(_imp->lastRotoPaintTickParams[inArgs.params->textureIndex]->textureRect)) {
                
                //Overwrite the RoI to only the last portion rendered
                RectD lastPaintBbox = getApp()->getLastPaintStrokeBbox();
                const double par = inArgs.activeInputToRender->getPreferredAspectRatio();
                
                lastPaintBbox.toPixelEnclosing(inArgs.params->mipMapLevel, par, &lastPaintBboxPixel);

                
                assert(_imp->lastRotoPaintTickParams[inArgs.params->textureIndex]->ramBuffer);
                inArgs.params->ramBuffer =  0;
                bool mustFreeSource = copyAndSwap(_imp->lastRotoPaintTickParams[inArgs.params->textureIndex]->textureRect, inArgs.params->textureRect, inArgs.params->bytesCount, inArgs.params->depth,_imp->lastRotoPaintTickParams[inArgs.params->textureIndex]->ramBuffer, &inArgs.params->ramBuffer);
                if (mustFreeSource) {
                    _imp->lastRotoPaintTickParams[inArgs.params->textureIndex]->mustFreeRamBuffer = true;
                } else {
                    _imp->lastRotoPaintTickParams[inArgs.params->textureIndex]->mustFreeRamBuffer = false;
                }
                _imp->lastRotoPaintTickParams[inArgs.params->textureIndex].reset();
                if (!inArgs.params->ramBuffer) {
                    return eViewerRenderRetCodeFail;
                }
            } else {
                inArgs.params->mustFreeRamBuffer = true;
                inArgs.params->ramBuffer =  (unsigned char*)malloc(inArgs.params->bytesCount);
            }
            _imp->lastRotoPaintTickParams[inArgs.params->textureIndex] = inArgs.params;
        } else {
            
            {
                QMutexLocker k(&_imp->lastRotoPaintTickParamsMutex);
                _imp->lastRotoPaintTickParams[inArgs.params->textureIndex].reset();
            }
            
            inArgs.params->mustFreeRamBuffer = true;
            inArgs.params->ramBuffer =  (unsigned char*)malloc(inArgs.params->bytesCount);
        }
        
    } else {
        
        {
            QMutexLocker k(&_imp->lastRotoPaintTickParamsMutex);
            _imp->lastRotoPaintTickParams[inArgs.params->textureIndex].reset();
        }
        
        // For the viewer, we need the enclosing rectangle to avoid black borders.
        // Do this here to avoid infinity values.
        RectI bounds;
        inArgs.params->rod.toPixelEnclosing(inArgs.params->mipMapLevel, inArgs.params->textureRect.par, &bounds);
        
        
        boost::shared_ptr<FrameParams> cachedFrameParams =
        FrameEntry::makeParams(bounds,inArgs.key->getBitDepth(), inArgs.params->textureRect.w, inArgs.params->textureRect.h, ImagePtr());
        bool cached = AppManager::getTextureFromCacheOrCreate(*(inArgs.key), cachedFrameParams,
                                                                   &inArgs.params->cachedFrame);
        if (!inArgs.params->cachedFrame) {
            std::stringstream ss;
            ss << "Failed to allocate a texture of ";
            ss << printAsRAM( cachedFrameParams->getElementsCount() * sizeof(FrameEntry::data_t) ).toStdString();
            Dialogs::errorDialog( QObject::tr("Out of memory").toStdString(),ss.str() );

            return eViewerRenderRetCodeFail;
        }
        
        entryLocker.lock(inArgs.params->cachedFrame);
        
        ///The entry has already been locked by the cache
		if (!cached) {
			inArgs.params->cachedFrame->allocateMemory();
		}
        
    
        
        assert(inArgs.params->cachedFrame);
        // how do you make sure cachedFrame->data() is not freed after this line?
        ///It is not freed as long as the cachedFrame shared_ptr has a used_count greater than 1.
        ///Since it is used during the whole function scope it is guaranteed not to be freed before
        ///The viewer is actually done with it.
        /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
        inArgs.params->ramBuffer = inArgs.params->cachedFrame->data();
        
    }
    assert(inArgs.params->ramBuffer);
    
    //We are going to render a non cached frame and not in playback, clear persistent messages
    if (!inArgs.params->isSequential) {
        clearPersistentMessage(true);
    }
    
    std::list<ImageComponents> components;
    ImageBitDepthEnum imageDepth;
    inArgs.activeInputToRender->getPreferredDepthAndComponents(-1, &components, &imageDepth);
    assert(!components.empty());
    

    std::list<ImageComponents> requestedComponents;
    
    int alphaChannelIndex = -1;
    if (inArgs.channels != eDisplayChannelsA &&
        inArgs.channels != eDisplayChannelsMatte) {
        ///We fetch the Layer specified in the gui
        if (inArgs.params->layer.getNumComponents() > 0) {
            requestedComponents.push_back(inArgs.params->layer);
        }
    } else {
        ///We fetch the alpha layer
        if (!inArgs.params->alphaChannelName.empty()) {
            requestedComponents.push_back(inArgs.params->alphaLayer);
            const std::vector<std::string>& channels = inArgs.params->alphaLayer.getComponentsNames();
            for (std::size_t i = 0; i < channels.size(); ++i) {
                if (channels[i] == inArgs.params->alphaChannelName) {
                    alphaChannelIndex = i;
                    break;
                }
            }
            assert(alphaChannelIndex != -1);
        }
        if (inArgs.channels == eDisplayChannelsMatte) {
            //For the matte overlay also display the alpha mask on top of the red channel of the image
            if (inArgs.params->layer.getNumComponents() > 0 && inArgs.params->layer != inArgs.params->alphaLayer) {
                requestedComponents.push_back(inArgs.params->layer);
            }
        }
    }
    
    if (requestedComponents.empty()) {
        return eViewerRenderRetCodeBlack;
    }

    EffectInstance::NotifyInputNRenderingStarted_RAII inputNIsRendering_RAII(getNode().get(),inArgs.activeInputIndex);
    
    
    std::vector<RectI> splitRoi;
    if (tilingProgressReportPrefEnabled &&
        inArgs.params->cachedFrame &&
        !isSequentialRender &&
        canAbort &&
        inArgs.activeInputToRender->supportsTiles()) {
        /*
         Split the RoI in tiles and update viewer if rendering takes too much time.
         */
       splitRoi = roi.splitIntoSmallerRects(0);
    } else {
        /*
         Just render 1 tile
         */
        splitRoi.push_back(roi);
    }
    
    if (stats && stats->isInDepthProfilingEnabled()) {
        std::bitset<4> channelsRendered;
        switch (inArgs.channels) {
            case eDisplayChannelsMatte:
            case eDisplayChannelsRGB:
            case eDisplayChannelsY:
                channelsRendered[0] = channelsRendered[1] = channelsRendered[2] = true;
                channelsRendered[3] = false;
                break;
            case eDisplayChannelsR:
                channelsRendered[3] = channelsRendered[1] = channelsRendered[2] = false;
                channelsRendered[0] = true;
                break;
            case eDisplayChannelsG:
                channelsRendered[0] = channelsRendered[2] = channelsRendered[3] = false;
                channelsRendered[1] = true;
                break;
            case eDisplayChannelsB:
                channelsRendered[0] = channelsRendered[1] = channelsRendered[3] = false;
                channelsRendered[2] = true;
                break;
            case eDisplayChannelsA:
                channelsRendered[0] = channelsRendered[1] = channelsRendered[2] = false;
                channelsRendered[3] = true;
                break;
        }
        stats->setGlobalRenderInfosForNode(getNode(), inArgs.params->rod, inArgs.params->srcPremult, channelsRendered, true, true, inArgs.params->mipMapLevel);
    }
    
    /*
     Use a timer to enable progress report if the amount spent rendering exceeds some time
     */
    double totalRenderTime = 0.;
    TimeLapse timer;
    // List of the tiles that the progress did not report until now
    std::list<RectI> unreportedTiles;
    
    for (std::size_t rectIndex = 0; rectIndex < splitRoi.size(); ++rectIndex) {
        
        bool reportProgress = false;

        
        //AlphaImage will only be set when displaying the Matte overlay
        ImagePtr alphaImage,colorImage;

        // If an exception occurs here it is probably fatal, since
        // it comes from Natron itself. All exceptions from plugins are already caught
        // by the HostSupport library.
        // We catch it  and rethrow it just to notify the rendering is done.
        try {
            
            ImageList planes;
            EffectInstance::RenderRoIRetCode retCode =
            inArgs.activeInputToRender->renderRoI(EffectInstance::RenderRoIArgs(inArgs.params->time,
                                                                                inArgs.key->getScale(),
                                                                                inArgs.params->mipMapLevel,
                                                                                view,
                                                                                inArgs.forceRender,
                                                                                splitRoi[rectIndex],
                                                                                inArgs.params->rod,
                                                                                requestedComponents,
                                                                                imageDepth, false, this),&planes);
            //Either rendering failed or we have 2 planes (alpha mask and color image) or we have a single plane (color image)
            assert(planes.size() == 0 || planes.size() <= 2);
            if (!planes.empty() && retCode == EffectInstance::eRenderRoIRetCodeOk) {
                if (planes.size() == 2) {
                    ImageList::iterator firstPlane = planes.begin();
                    if ((*firstPlane)->getComponents() == inArgs.params->layer) {
                        colorImage = *firstPlane;
                        ++firstPlane;
                        alphaImage = *firstPlane;
                    } else {
                        assert((*firstPlane)->getComponents() == inArgs.params->alphaLayer);
                        alphaImage = *firstPlane;
                        ++firstPlane;
                        colorImage = *firstPlane;
                    }
                } else {
                    //only 1 plane, figure out if the alpha layer is the same as the color layer
                    if (inArgs.params->alphaLayer == inArgs.params->layer) {
                        if (inArgs.channels == eDisplayChannelsMatte) {
                            alphaImage = colorImage = planes.front();
                        } else {
                            colorImage = planes.front();
                        }
                    } else {
                        colorImage = planes.front();
                    }
                }
                assert(colorImage);
                inArgs.params->tiles.push_back(colorImage);
            }
            if (!colorImage) {
                if (retCode == EffectInstance::eRenderRoIRetCodeFailed) {
                    return eViewerRenderRetCodeFail;
                } else if (retCode == EffectInstance::eRenderRoIRetCodeOk) {
                    return eViewerRenderRetCodeBlack;
                } else {
                    /*
                     The render was not aborted but did not return an image, this may be the case
                     for example when an effect returns a NULL RoD at some point. Don't fail but
                     display a black image
                     */
                    return eViewerRenderRetCodeRedraw;
                }
            }
            
            if (!reportProgress && splitRoi.size() > 1) {
                double timeSpan = timer.getTimeElapsedReset();
                totalRenderTime += timeSpan;
                reportProgress = totalRenderTime > NATRON_TIME_ELASPED_BEFORE_PROGRESS_REPORT && !isCurrentlyUpdatingOpenGLViewer();
            }
            
            
        } catch (...) {
            ///If the plug-in was aborted, this is probably not a failure due to render but because of abortion.
            ///Don't forward the exception in that case.
            abortCheck(inArgs.activeInputToRender);
            throw;
        }
        
        
        ///We check that the render age is still OK and that no other renders were triggered, in which case we should not need to
        ///refresh the viewer.
        if (!_imp->checkAgeNoUpdate(inArgs.params->textureIndex,inArgs.params->renderAge)) {
            return eViewerRenderRetCodeRedraw;
        }
        
        abortCheck(inArgs.activeInputToRender);
 
        
        ViewerColorSpaceEnum srcColorSpace = getApp()->getDefaultColorSpaceForBitDepth( colorImage->getBitDepth() );
        
        assert((inArgs.channels != eDisplayChannelsMatte && alphaChannelIndex < (int)colorImage->getComponentsCount()) ||
               (inArgs.channels == eDisplayChannelsMatte && ((alphaImage && alphaChannelIndex < (int)alphaImage->getComponentsCount()) || !alphaImage)));
        
        //Make sure the viewer does not render something outside the bounds
        RectI viewerRenderRoI;
        splitRoi[rectIndex].intersect(colorImage->getBounds(), &viewerRenderRoI);
        
        //If we are painting, only render the portion needed
        if (!lastPaintBboxPixel.isNull()) {
            lastPaintBboxPixel.intersect(viewerRenderRoI, &viewerRenderRoI);
        }
        
        boost::shared_ptr<TimeLapse> viewerRenderTimeRecorder;
        if (stats) {
            viewerRenderTimeRecorder.reset(new TimeLapse());
        }
        
        if (singleThreaded) {
            if (inArgs.autoContrast) {
                double vmin, vmax;
                std::pair<double,double> vMinMax = findAutoContrastVminVmax(colorImage, inArgs.channels, viewerRenderRoI);
                vmin = vMinMax.first;
                vmax = vMinMax.second;
                
                ///if vmax - vmin is greater than 1 the gain will be really small and we won't see
                ///anything in the image
                if (vmin == vmax) {
                    vmin = vmax - 1.;
                }
                inArgs.params->gain = 1 / (vmax - vmin);
                inArgs.params->offset = -vmin / ( vmax - vmin);
            }
            
            const RenderViewerArgs args(colorImage,
                                        alphaImage,
                                        inArgs.params->textureRect,
                                        inArgs.channels,
                                        inArgs.params->srcPremult,
                                        inArgs.key->getBitDepth(),
                                        inArgs.params->gain,
                                        inArgs.params->gamma == 0. ? 0. : 1. / inArgs.params->gamma,
                                        inArgs.params->offset,
                                        lutFromColorspace(srcColorSpace),
                                        lutFromColorspace(inArgs.params->lut),
                                        alphaChannelIndex);
            
            QReadLocker k(&_imp->gammaLookupMutex);
            renderFunctor(viewerRenderRoI,
                          args,
                          this,
                          inArgs.params->ramBuffer);
        } else {
            
            bool runInCurrentThread = QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount();
            std::vector<RectI> splitRects;
            if (!runInCurrentThread && splitRoi.size() > 1) {
                runInCurrentThread = true;
            }
            if (!runInCurrentThread) {
                splitRects = viewerRenderRoI.splitIntoSmallerRects(appPTR->getHardwareIdealThreadCount());
            }
            
            ///if autoContrast is enabled, find out the vmin/vmax before rendering and mapping against new values
            if (inArgs.autoContrast) {
                
                double vmin = std::numeric_limits<double>::infinity();
                double vmax = -std::numeric_limits<double>::infinity();
                
                if (!runInCurrentThread) {
                    
                    QFuture<std::pair<double,double> > future = QtConcurrent::mapped( splitRects,
                                                                                     boost::bind(findAutoContrastVminVmax,
                                                                                                 colorImage,
                                                                                                 inArgs.channels,
                                                                                                 _1) );
                    future.waitForFinished();
                    
                    std::pair<double,double> vMinMax;
                    Q_FOREACH ( vMinMax, future.results() ) {
                        if (vMinMax.first < vmin) {
                            vmin = vMinMax.first;
                        }
                        if (vMinMax.second > vmax) {
                            vmax = vMinMax.second;
                        }
                    }
                } else { //!runInCurrentThread
                    std::pair<double,double> vMinMax = findAutoContrastVminVmax(colorImage, inArgs.channels, viewerRenderRoI);
                    vmin = vMinMax.first;
                    vmax = vMinMax.second;
                }
                
                if (vmax == vmin) {
                    vmin = vmax - 1.;
                }
                
                if (vmax <= 0) {
                    inArgs.params->gain = 0;
                    inArgs.params->offset = 0;
                } else {
                    inArgs.params->gain = 1 / (vmax - vmin);
                    inArgs.params->offset =  -vmin / (vmax - vmin);
                }
            }
            
            const RenderViewerArgs args(colorImage,
                                        alphaImage,
                                        inArgs.params->textureRect,
                                        inArgs.channels,
                                        inArgs.params->srcPremult,
                                        inArgs.key->getBitDepth(),
                                        inArgs.params->gain,
                                        inArgs.params->gamma == 0. ? 0. : 1. / inArgs.params->gamma,
                                        inArgs.params->offset,
                                        lutFromColorspace(srcColorSpace),
                                        lutFromColorspace(inArgs.params->lut),
                                        alphaChannelIndex);
            if (runInCurrentThread) {
                QReadLocker k(&_imp->gammaLookupMutex);
                renderFunctor(viewerRenderRoI,
                              args, this, inArgs.params->ramBuffer);
            } else {
                QReadLocker k(&_imp->gammaLookupMutex);
                QtConcurrent::map( splitRects,
                                  boost::bind(&renderFunctor,
                                              _1,
                                              args,
                                              this,
                                              inArgs.params->ramBuffer) ).waitForFinished();
            }
            
            if (splitRoi.size() > 1 && rectIndex < (splitRoi.size() -1)) {
                unreportedTiles.push_back(viewerRenderRoI);
                if (reportProgress) {
                    _imp->reportProgress(inArgs.params, unreportedTiles, stats, request);
                    unreportedTiles.clear();
                }
            }
        } // if (singleThreaded)
        if (inArgs.params->cachedFrame && colorImage) {
            inArgs.params->cachedFrame->addOriginalTile(colorImage);
        }
        
        if (stats && stats->isInDepthProfilingEnabled()) {
            stats->addRenderInfosForNode(getNode(), NodePtr(), colorImage->getComponents().getComponentsGlobalName(), viewerRenderRoI, viewerRenderTimeRecorder->getTimeSinceCreation());
        }
    } // for (std::vector<RectI>::iterator rect = splitRoi.begin(); rect != splitRoi.end(), ++rect) {
    
    return eViewerRenderRetCodeRender;
} // renderViewer_internal

void
ViewerInstance::ViewerInstancePrivate::reportProgress(const boost::shared_ptr<UpdateViewerParams>& originalParams,
                                                      const std::list<RectI>& rectangles,
                                                      const boost::shared_ptr<RenderStats>& stats,
                                                      const boost::shared_ptr<RequestedFrame>& request)
{
    //update the viewer to report progress
    BufferableObjectList ret;
    for (std::list<RectI>::const_iterator it = rectangles.begin(); it!= rectangles.end(); ++it) {
        boost::shared_ptr<UpdateViewerParams> params(new UpdateViewerParams(*originalParams));
        params->roi = *it;
        params->updateOnlyRoi = true;
        std::size_t pixelSize = 4;
        if (params->depth == eImageBitDepthFloat) {
            pixelSize *= sizeof(float);
        }
        std::size_t dstRowSize = params->roi.width() * pixelSize;
        params->bytesCount = params->roi.height() * dstRowSize;
        
        
        ///Allocate a temporary buffer in which we copy the texture for the RoI
        unsigned char* tmpBuffer = (unsigned char*)malloc(params->bytesCount);
        params->mustFreeRamBuffer = true;
        
        unsigned char* dstPixels = tmpBuffer;
        for (int y = it->y1; y < it->y2; ++y, dstPixels += dstRowSize) {
            const unsigned char* srcPixels = params->cachedFrame->pixelAt(it->x1, y);
            memcpy(dstPixels, srcPixels, dstRowSize);
        }
        
        params->ramBuffer = tmpBuffer;
        ret.push_back(params);
    }
    instance->getRenderEngine()->notifyFrameProduced(ret, stats, request);
}

void
ViewerInstance::setCurrentlyUpdatingOpenGLViewer(bool updating)
{
    QMutexLocker k(&_imp->currentlyUpdatingOpenGLViewerMutex);
    _imp->currentlyUpdatingOpenGLViewer = updating;
}

bool
ViewerInstance::isCurrentlyUpdatingOpenGLViewer() const
{
    QMutexLocker k(&_imp->currentlyUpdatingOpenGLViewerMutex);
    return _imp->currentlyUpdatingOpenGLViewer;
}

void
ViewerInstance::updateViewer(boost::shared_ptr<UpdateViewerParams> & frame)
{
    _imp->updateViewer(boost::dynamic_pointer_cast<UpdateViewerParams>(frame));
}

void
renderFunctor(const RectI& roi,
              const RenderViewerArgs & args,
              ViewerInstance* viewer,
              void *buffer)
{
    assert(args.texRect.y1 <= roi.y1 && roi.y1 <= roi.y2 && roi.y2 <= args.texRect.y2);

    if ( (args.bitDepth == eImageBitDepthFloat) ) {
        // image is stored as linear, the OpenGL shader with do gamma/sRGB/Rec709 decompression, as well as gain and offset
        scaleToTexture32bits(roi, args, (float*)buffer);
    } else {
        // texture is stored as sRGB/Rec709 compressed 8-bit RGBA
        scaleToTexture8bits(roi, args,viewer, (U32*)buffer);
    }
}

inline
std::pair<double, double>
findAutoContrastVminVmax_generic(boost::shared_ptr<const Image> inputImage,
                                 int nComps,
                                 DisplayChannelsEnum channels,
                                 const RectI & rect)
{
    double localVmin = std::numeric_limits<double>::infinity();
    double localVmax = -std::numeric_limits<double>::infinity();
    
    Image::ReadAccess acc = inputImage->getReadRights();
    
    for (int y = rect.bottom(); y < rect.top(); ++y) {
        const float* src_pixels = (const float*)acc.pixelAt(rect.left(),y);
        ///we fill the scan-line with all the pixels of the input image
        for (int x = rect.left(); x < rect.right(); ++x) {
            
            double r = 0.;
            double g = 0.;
            double b = 0.;
            double a = 0.;
            switch (nComps) {
                case 4:
                    r = src_pixels[0];
                    g = src_pixels[1];
                    b = src_pixels[2];
                    a = src_pixels[3];
                    break;
                case 3:
                    r = src_pixels[0];
                    g = src_pixels[1];
                    b = src_pixels[2];
                    a = 1.;
                    break;
                case 2:
                    r = src_pixels[0];
                    g = src_pixels[1];
                    b = 0.;
                    a = 1.;
                    break;
                case 1:
                    a = src_pixels[0];
                    r = g = b = 0.;
                    break;
                default:
                    r = g = b = a = 0.;
            }
            
            double mini, maxi;
            switch (channels) {
                case eDisplayChannelsRGB:
                    mini = std::min(std::min(r,g),b);
                    maxi = std::max(std::max(r,g),b);
                    break;
                case eDisplayChannelsY:
                    mini = r = 0.299 * r + 0.587 * g + 0.114 * b;
                    maxi = mini;
                    break;
                case eDisplayChannelsR:
                    mini = r;
                    maxi = mini;
                    break;
                case eDisplayChannelsG:
                    mini = g;
                    maxi = mini;
                    break;
                case eDisplayChannelsB:
                    mini = b;
                    maxi = mini;
                    break;
                case eDisplayChannelsA:
                    mini = a;
                    maxi = mini;
                    break;
                default:
                    mini = 0.;
                    maxi = 0.;
                    break;
            }
            if (mini < localVmin) {
                localVmin = mini;
            }
            if (maxi > localVmax) {
                localVmax = maxi;
            }
            
            src_pixels += nComps;
        }
    }
    
    return std::make_pair(localVmin, localVmax);
}

template <int nComps>
std::pair<double, double>
findAutoContrastVminVmax_internal(boost::shared_ptr<const Image> inputImage,
                                  DisplayChannelsEnum channels,
                                  const RectI & rect)
{
    return findAutoContrastVminVmax_generic(inputImage, nComps, channels, rect);
}

std::pair<double, double>
findAutoContrastVminVmax(boost::shared_ptr<const Image> inputImage,
                         DisplayChannelsEnum channels,
                         const RectI & rect)
{
    int nComps = inputImage->getComponents().getNumComponents();
    if (nComps == 4) {
        return findAutoContrastVminVmax_internal<4>(inputImage, channels, rect);
    } else if (nComps == 3) {
        return findAutoContrastVminVmax_internal<3>(inputImage, channels, rect);
    } else if (nComps == 1) {
        return findAutoContrastVminVmax_internal<1>(inputImage, channels, rect);
    } else {
        return findAutoContrastVminVmax_generic(inputImage, nComps, channels, rect);
    }
    
} // findAutoContrastVminVmax

template <typename PIX,int maxValue,bool opaque, bool applyMatte,int rOffset,int gOffset,int bOffset>
void
scaleToTexture8bits_generic(const RectI& roi,
                            const RenderViewerArgs & args,
                            int nComps,
                            ViewerInstance* viewer,
                            U32* output)
{
    size_t pixelSize = sizeof(PIX);
    
    const bool luminance = (args.channels == eDisplayChannelsY);
    
    Image::ReadAccess acc = Image::ReadAccess(args.inputImage.get());
    const RectI srcImgBounds = args.inputImage->getBounds();
    
    ///offset the output buffer at the starting point
    U32* dst_pixels = output + (roi.y1 - args.texRect.y1) * args.texRect.w + (roi.x1 - args.texRect.x1);

    ///Cannot be an empty rect
    assert(args.texRect.x2 > args.texRect.x1);
    
    const PIX* src_pixels = (const PIX*)acc.pixelAt(roi.x1, roi.y1);
    const int srcRowElements = (int)args.inputImage->getRowElements();
    
    boost::shared_ptr<Image::ReadAccess> matteAcc;
    if (applyMatte) {
        matteAcc.reset(new Image::ReadAccess(args.matteImage.get()));
    }
    
    for (int y = roi.y1; y < roi.y2;
         ++y,
         dst_pixels += args.texRect.w) {
        
        // coverity[dont_call]
        int start = (int)(rand() % (roi.x2 - roi.x1));
        

        for (int backward = 0; backward < 2; ++backward) {
            
            int index = backward ? start - 1 : start;
            
            assert( backward == 1 || ( index >= 0 && index < (args.texRect.x2 - args.texRect.x1) ) );

            unsigned error_r = 0x80;
            unsigned error_g = 0x80;
            unsigned error_b = 0x80;
            
            while (index < (roi.x2 - roi.x1) && index >= 0) {
                
                double r = 0.;
                double g = 0.;
                double b = 0.;
                int uA = 0;
                double a = 0;
                if (nComps >= 4) {
                    r = (src_pixels ? src_pixels[index * nComps + rOffset] : 0.);
                    g = (src_pixels ? src_pixels[index * nComps + gOffset] : 0.);
                    b = (src_pixels ? src_pixels[index * nComps + bOffset] : 0.);
                    if (opaque) {
                        a = 1;
                        uA = 255;
                    } else {
                        a = src_pixels ? src_pixels[index * nComps + 3] : 0;
                        uA = Color::floatToInt<256>(a);
                    }
                } else if (nComps == 3) {
                    // coverity[dead_error_line]
                    r = (src_pixels && rOffset < nComps) ? src_pixels[index * nComps + rOffset] : 0.;
                    // coverity[dead_error_line]
                    g = (src_pixels && gOffset < nComps) ? src_pixels[index * nComps + gOffset] : 0.;
                    // coverity[dead_error_line]
                    b = (src_pixels && bOffset < nComps) ? src_pixels[index * nComps + bOffset] : 0.;
                    a = (src_pixels ? 1 : 0);
                    uA = a * 255;
                } else if (nComps == 2) {
                    // coverity[dead_error_line]
                    r = (src_pixels && rOffset < nComps) ? src_pixels[index * nComps + rOffset] : 0.;
                    // coverity[dead_error_line]
                    g = (src_pixels && gOffset < nComps) ? src_pixels[index * nComps + gOffset] : 0.;
                    b = 0;
                    a = (src_pixels ? 1 : 0);
                    uA = a * 255;
                } else if (nComps == 1) {
                    // coverity[dead_error_line]
                    r = (src_pixels && rOffset < nComps) ? src_pixels[index * nComps + rOffset] : 0.;
                    g = b = r;
                    a = (src_pixels ? 1 : 0);
                    uA = a * 255;
                } else {
                    assert(false);
                }

                
                switch ( pixelSize ) {
                    case sizeof(unsigned char): //byte
                        if (args.srcColorSpace) {
                            r = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)r );
                            g = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)g );
                            b = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)b );
                        } else {
                            r = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                            g = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)g );
                            b = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)b );
                        }
                        break;
                    case sizeof(unsigned short): //short
                        if (args.srcColorSpace) {
                            r = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)r );
                            g = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)g );
                            b = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)b );
                        } else {
                            r = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)r );
                            g = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)g );
                            b = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)b );
                        }
                        break;
                    case sizeof(float): //float
                        if (args.srcColorSpace) {
                            r = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(r);
                            g = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(g);
                            b = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(b);
                        }
                        break;
                    default:
                        break;
                }
                
                //args.gamma is in fact 1. / gamma at this point
                if  (args.gamma == 0) {
                    r = 0;
                    g = 0.;
                    b = 0.;
                } else if (args.gamma == 1.) {
                    r = r * args.gain + args.offset;
                    g = g * args.gain + args.offset;
                    b = b * args.gain + args.offset;
                } else {
                    r = viewer->interpolateGammaLut(r * args.gain + args.offset);
                    g = viewer->interpolateGammaLut(g * args.gain + args.offset);
                    b = viewer->interpolateGammaLut(b * args.gain + args.offset);
                }
        
                
                if (luminance) {
                    r = 0.299 * r + 0.587 * g + 0.114 * b;
                    g = r;
                    b = r;
                }
            
                
                U8 uR,uG,uB;
                if (!args.colorSpace) {
                    uR = Color::floatToInt<256>(r);
                    uG = Color::floatToInt<256>(g);
                    uB = Color::floatToInt<256>(b);
                    
                } else {
                    error_r = (error_r & 0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(r);
                    error_g = (error_g & 0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(g);
                    error_b = (error_b & 0xff) + args.colorSpace->toColorSpaceUint8xxFromLinearFloatFast(b);
                    assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
                    uR = (U8)(error_r >> 8);
                    uG = (U8)(error_g >> 8);
                    uB = (U8)(error_b >> 8);
                }
                
                if (applyMatte) {
                    double alphaMatteValue = 0;
                    if (args.matteImage == args.inputImage) {
                        switch (args.alphaChannelIndex) {
                            case 0:
                                alphaMatteValue = r;
                                break;
                            case 1:
                                alphaMatteValue = g;
                                break;
                            case 2:
                                alphaMatteValue = b;
                                break;
                            case 3:
                                alphaMatteValue = a;
                                break;
                            default:
                                break;
                        }
                    } else {
                        const PIX* src_pixels = (const PIX*)matteAcc->pixelAt(roi.x1 + index, y);
                        if (src_pixels) {
                            alphaMatteValue = (double)src_pixels[args.alphaChannelIndex];
                            switch ( pixelSize ) {
                                case sizeof(unsigned char): //byte
                                    alphaMatteValue = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                                    break;
                                case sizeof(unsigned short): //short
                                    alphaMatteValue = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned short)r );
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    U8 matteA;
                    if (args.colorSpace) {
                        matteA = args.colorSpace->toColorSpaceUint8FromLinearFloatFast(alphaMatteValue) / 2;
                    } else {
                        matteA = Color::floatToInt<256>(alphaMatteValue) / 2;
                    }
                    uR = Image::clampIfInt<U8>((double)uR + matteA);
                    
                }
                
                dst_pixels[index] = toBGRA(uR,uG,uB,uA);
                
                if (backward) {
                    --index;
                } else {
                    ++index;
                }
                
            } // while (index < args.texRect.w && index >= 0) {
            
        } // for (int backward = 0; backward < 2; ++backward) {
        if (src_pixels) {
            src_pixels += srcRowElements;
        }
    } // for (int y = yRange.first; y < yRange.second;
} // scaleToTexture8bits_generic


template <typename PIX,int maxValue,int nComps,bool opaque,bool matteOverlay,int rOffset,int gOffset,int bOffset>
void
scaleToTexture8bits_internal(const RectI& roi,
                             const RenderViewerArgs & args,
                             ViewerInstance* viewer,
                             U32* output)
{
    scaleToTexture8bits_generic<PIX, maxValue, opaque, matteOverlay, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, output);
}


template <typename PIX,int maxValue,int nComps,bool opaque,int rOffset,int gOffset,int bOffset>
void
scaleToTexture8bitsForMatte(const RectI& roi,
                             const RenderViewerArgs & args,
                             ViewerInstance* viewer,
                             U32* output)
{
    bool applyMate = args.matteImage.get() && args.alphaChannelIndex >= 0;
    if (applyMate) {
        scaleToTexture8bits_internal<PIX, maxValue, nComps, opaque, true, rOffset, gOffset, bOffset>(roi, args, viewer, output);
    } else {
        scaleToTexture8bits_internal<PIX, maxValue, nComps, opaque, false, rOffset, gOffset, bOffset>(roi, args, viewer, output);
    }
    
}


template <typename PIX,int maxValue, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bitsForDepthForComponents(const RectI& roi,
                                         const RenderViewerArgs & args,
                                         ViewerInstance* viewer,
                                         U32* output)
{
    int nComps = args.inputImage->getComponents().getNumComponents();
    switch (nComps) {
        case 4:
            scaleToTexture8bitsForMatte<PIX,maxValue,4 , opaque, rOffset,gOffset,bOffset>(roi,args,viewer,output);
            break;
        case 3:
            scaleToTexture8bitsForMatte<PIX,maxValue,3, opaque, rOffset,gOffset,bOffset>(roi,args,viewer,output);
            break;
        case 2:
            scaleToTexture8bitsForMatte<PIX,maxValue,2, opaque, rOffset,gOffset,bOffset>(roi,args,viewer,output);
            break;
        case 1:
            scaleToTexture8bitsForMatte<PIX,maxValue,1, opaque, rOffset,gOffset,bOffset>(roi,args,viewer,output);
            break;
        default:
            bool applyMate = args.matteImage.get() && args.alphaChannelIndex >= 0;
            if (applyMate) {
                scaleToTexture8bits_generic<PIX, maxValue, opaque, true, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, output);
            } else {
                scaleToTexture8bits_generic<PIX, maxValue, opaque, false, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, output);
            }
            break;
    }
}

template <typename PIX,int maxValue,bool opaque>
void
scaleToTexture8bitsForPremult(const RectI& roi,
                             const RenderViewerArgs & args,
                             ViewerInstance* viewer,
                             U32* output)
{
    
    switch (args.channels) {
        case eDisplayChannelsRGB:
        case eDisplayChannelsY:
        case eDisplayChannelsMatte:
            
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 1, 2>(roi, args,viewer, output);
            break;
        case eDisplayChannelsG:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args,viewer, output);
            break;
        case eDisplayChannelsB:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args,viewer, output);
            break;
        case eDisplayChannelsA:
            switch (args.alphaChannelIndex) {
                case -1:
                    scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args,viewer, output);
                    break;
                case 0:
                    scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args,viewer, output);
                    break;
                case 1:
                    scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args,viewer, output);
                    break;
                case 2:
                    scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args,viewer, output);
                    break;
                case 3:
                    scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args,viewer, output);
                    break;
                default:
                    scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args,viewer, output);
            }
            
            break;
        case eDisplayChannelsR:
        default:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args,viewer, output);
            
            break;
    }
    
   
}


template <typename PIX,int maxValue>
void
scaleToTexture8bitsForDepth(const RectI& roi,
                            const RenderViewerArgs & args,
                            ViewerInstance* viewer,
                            U32* output)
{
    switch (args.srcPremult) {
        case eImagePremultiplicationOpaque:
            scaleToTexture8bitsForPremult<PIX, maxValue, true>(roi, args,viewer, output);
            break;
        case eImagePremultiplicationPremultiplied:
        case eImagePremultiplicationUnPremultiplied:
        default:
            scaleToTexture8bitsForPremult<PIX, maxValue, false>(roi, args,viewer, output);
            break;
            
    }
}

void
scaleToTexture8bits(const RectI& roi,
                    const RenderViewerArgs & args,
                    ViewerInstance* viewer,
                    U32* output)
{
    assert(output);
    switch ( args.inputImage->getBitDepth() ) {
        case eImageBitDepthFloat:
            scaleToTexture8bitsForDepth<float, 1>(roi, args,viewer, output);
            break;
        case eImageBitDepthByte:
            scaleToTexture8bitsForDepth<unsigned char, 255>(roi, args,viewer, output);
            break;
        case eImageBitDepthShort:
            scaleToTexture8bitsForDepth<unsigned short, 65535>(roi, args,viewer,output);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthNone:
            break;
    }
} // scaleToTexture8bits

float
ViewerInstance::interpolateGammaLut(float value)
{
    return _imp->lookupGammaLut(value);
}

void
ViewerInstance::markAllOnRendersAsAborted()
{
    QMutexLocker k(&_imp->renderAgeMutex);
    for (int i = 0; i < 2; ++i) {
        if (_imp->currentRenderAges[i].empty()) {
            continue;
        }
        
        //Do not abort the oldest render, let it finish
        OnGoingRenders::iterator it = _imp->currentRenderAges[i].begin();
        ++it;
        
        for (;it != _imp->currentRenderAges[i].end(); ++it) {
            it->second.aborted = true;
        }
    }
}

template <typename PIX,int maxValue,bool opaque, bool applyMatte, int rOffset,int gOffset,int bOffset>
void
scaleToTexture32bitsGeneric(const RectI& roi,
                            const RenderViewerArgs & args,
                            int nComps,
                            float *output)
{
    size_t pixelSize = sizeof(PIX);
    const bool luminance = (args.channels == eDisplayChannelsY);

    ///the width of the output buffer multiplied by the channels count
    const int dstRowElements = args.texRect.w * 4;

    
    Image::ReadAccess acc = Image::ReadAccess(args.inputImage.get());
    boost::shared_ptr<Image::ReadAccess> matteAcc;
    if (applyMatte) {
        matteAcc.reset(new Image::ReadAccess(args.matteImage.get()));
    }
    
    float* dst_pixels =  output + (roi.y1 - args.texRect.y1) * dstRowElements + (roi.x1 - args.texRect.x1) * 4;
    const float* src_pixels = (const float*)acc.pixelAt(roi.x1, roi.y1);

    assert(args.texRect.w == args.texRect.x2 - args.texRect.x1);
    
    const int srcRowElements = (const int)args.inputImage->getRowElements();
    
    for (int y = roi.y1; y < roi.y2;
         ++y,
         dst_pixels += dstRowElements) {

        for (int x = 0; x < roi.width();
             ++x) {
            
            double r = 0.;
            double g = 0.;
            double b = 0.;
            double a = 0.;
            
            if (nComps >= 4) {
                r = (src_pixels && rOffset < nComps) ? src_pixels[x * nComps + rOffset] : 0.;
                g = (src_pixels && gOffset < nComps) ? src_pixels[x * nComps + gOffset] : 0.;
                b = (src_pixels && bOffset < nComps) ? src_pixels[x * nComps + bOffset] : 0.;
                if (opaque) {
                    a = 1.;
                } else {
                    a = src_pixels ? src_pixels[x * nComps + 3] : 0.;
                }
            } else if (nComps == 3) {
                // coverity[dead_error_line]
                r = (src_pixels && rOffset < nComps) ? src_pixels[x * nComps + rOffset] : 0.;
                // coverity[dead_error_line]
                g = (src_pixels && gOffset < nComps) ? src_pixels[x * nComps + gOffset] : 0.;
                // coverity[dead_error_line]
                b = (src_pixels && bOffset < nComps) ? src_pixels[x * nComps + bOffset] : 0.;
                a = 1.;
            } else if (nComps == 2) {
                // coverity[dead_error_line]
                r = (src_pixels && rOffset < nComps) ? src_pixels[x * nComps + rOffset] : 0.;
                // coverity[dead_error_line]
                g = (src_pixels && gOffset < nComps) ? src_pixels[x * nComps + gOffset] : 0.;
                b = 0.;
                a = 1.;
            } else if (nComps == 1) {
                // coverity[dead_error_line]
                r = (src_pixels && rOffset < nComps) ? src_pixels[x * nComps + rOffset] : 0.;
                g = b = r;
                a = 1.;
            } else {
                assert(false);
            }
            
            
            switch ( pixelSize ) {
            case sizeof(unsigned char):
                if (args.srcColorSpace) {
                    r = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)r );
                    g = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)g );
                    b = args.srcColorSpace->fromColorSpaceUint8ToLinearFloatFast( (unsigned char)b );
                } else {
                    r = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                    g = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)g );
                    b = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)b );
                }
                break;
            case sizeof(unsigned short):
                if (args.srcColorSpace) {
                    r = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)r );
                    g = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)g );
                    b = args.srcColorSpace->fromColorSpaceUint16ToLinearFloatFast( (unsigned short)b );
                } else {
                    r = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)r );
                    g = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)g );
                    b = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned char)b );
                }
                break;
            case sizeof(float):
                if (args.srcColorSpace) {
                    r = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(r);
                    g = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(g);
                    b = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(b);
                }
                break;
            default:
                break;
            }


            if (luminance) {
                r = 0.299 * r + 0.587 * g + 0.114 * b;
                g = r;
                b = r;
            }
            
            if (applyMatte) {
                double alphaMatteValue = 0;
                if (args.matteImage == args.inputImage) {
                    switch (args.alphaChannelIndex) {
                        case 0:
                            alphaMatteValue = r;
                            break;
                        case 1:
                            alphaMatteValue = g;
                            break;
                        case 2:
                            alphaMatteValue = b;
                            break;
                        case 3:
                            alphaMatteValue = a;
                            break;
                        default:
                            break;
                    }
                } else {
                    const PIX* src_pixels = (const PIX*)matteAcc->pixelAt(x, y);
                    if (src_pixels) {
                        alphaMatteValue = (double)src_pixels[args.alphaChannelIndex];
                        switch ( pixelSize ) {
                            case sizeof(unsigned char): //byte
                                alphaMatteValue = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                                break;
                            case sizeof(unsigned short): //short
                                alphaMatteValue = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned short)r );
                                break;
                            default:
                                break;
                        }
                    }
                }
                r += alphaMatteValue * 0.5;
            }

            
            dst_pixels[x * 4] = Image::clamp(r, 0., 1.);
            dst_pixels[x * 4 + 1] = Image::clamp(g, 0., 1.);
            dst_pixels[x * 4 + 2] = Image::clamp(b, 0., 1.);
            dst_pixels[x * 4 + 3] = Image::clamp(a, 0., 1.);

        }
        if (src_pixels) {
            src_pixels += srcRowElements;
        }
    }
} // scaleToTexture32bitsGeneric

template <typename PIX,int maxValue,int nComps,bool opaque, bool applyMatte, int rOffset,int gOffset,int bOffset>
void
scaleToTexture32bitsInternal(const RectI& roi,
                             const RenderViewerArgs & args,
                             float *output) {
    scaleToTexture32bitsGeneric<PIX, maxValue, opaque, applyMatte, rOffset, gOffset, bOffset>(roi, args, nComps, output);
}

template <typename PIX,int maxValue,int nComps,bool opaque, int rOffset,int gOffset,int bOffset>
void
scaleToTexture32bitsForMatte(const RectI& roi,
                             const RenderViewerArgs & args,
                             float *output) {
    bool applyMatte = args.matteImage.get() && args.alphaChannelIndex >= 0;
    if (applyMatte) {
        scaleToTexture32bitsInternal<PIX, maxValue, nComps, opaque, true, rOffset, gOffset, bOffset>(roi, args, output);
    } else {
        scaleToTexture32bitsInternal<PIX, maxValue, nComps, opaque, false, rOffset, gOffset, bOffset>(roi, args, output);
    }
    
}

template <typename PIX,int maxValue,bool opaque,int rOffset,int gOffset,int bOffset>
void
scaleToTexture32bitsForDepthForComponents(const RectI& roi,
                             const RenderViewerArgs & args,
                             float *output)
{
    int  nComps = args.inputImage->getComponents().getNumComponents();
    switch (nComps) {
        case 4:
            scaleToTexture32bitsForMatte<PIX,maxValue,4, opaque, rOffset,gOffset,bOffset>(roi,args,output);
            break;
        case 3:
            scaleToTexture32bitsForMatte<PIX,maxValue,3, opaque, rOffset,gOffset,bOffset>(roi,args,output);
            break;
        case 2:
            scaleToTexture32bitsForMatte<PIX,maxValue,2, opaque, rOffset,gOffset,bOffset>(roi,args,output);
            break;
        case 1:
            scaleToTexture32bitsForMatte<PIX,maxValue,1, opaque, rOffset,gOffset,bOffset>(roi,args,output);
            break;
        default:
            bool applyMatte = args.matteImage.get() && args.alphaChannelIndex >= 0;
            if (applyMatte) {
                scaleToTexture32bitsGeneric<PIX,maxValue, opaque, true, rOffset,gOffset,bOffset>(roi,args,nComps,output);
            } else {
                scaleToTexture32bitsGeneric<PIX,maxValue, opaque, false, rOffset,gOffset,bOffset>(roi,args,nComps,output);
            }
            break;
    }
}

template <typename PIX,int maxValue,bool opaque>
void
scaleToTexture32bitsForPremultForComponents(const RectI& roi,
                             const RenderViewerArgs & args,
                             float *output)
{
    
 
    
    switch (args.channels) {
        case eDisplayChannelsRGB:
        case eDisplayChannelsY:
        case eDisplayChannelsMatte:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 1, 2>(roi, args, output);
            break;
        case eDisplayChannelsG:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, output);
            break;
        case eDisplayChannelsB:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, output);
            break;
        case eDisplayChannelsA:
            switch (args.alphaChannelIndex) {
                case -1:
                    scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, output);
                    break;
                case 0:
                    scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, output);
                    break;
                case 1:
                    scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, output);
                    break;
                case 2:
                    scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, output);
                    break;
                case 3:
                    scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, output);
                    break;
                default:
                    scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, output);
                    break;
            }
            
            break;
        case eDisplayChannelsR:
        default:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, output);
            break;
    }

}

template <typename PIX,int maxValue>
void
scaleToTexture32bitsForPremult(const RectI& roi,
                             const RenderViewerArgs & args,
                             float *output)
{
    switch (args.srcPremult) {
        case eImagePremultiplicationOpaque:
            scaleToTexture32bitsForPremultForComponents<PIX, maxValue, true>(roi, args, output);
            break;
        case eImagePremultiplicationPremultiplied:
        case eImagePremultiplicationUnPremultiplied:
        default:
            scaleToTexture32bitsForPremultForComponents<PIX, maxValue, false>(roi, args, output);
            break;
            
    }
    
    
}

void
scaleToTexture32bits(const RectI& roi,
                     const RenderViewerArgs & args,
                     float *output)
{
    assert(output);

    switch ( args.inputImage->getBitDepth() ) {
        case eImageBitDepthFloat:
            scaleToTexture32bitsForPremult<float, 1>(roi, args, output);
            break;
        case eImageBitDepthByte:
            scaleToTexture32bitsForPremult<unsigned char, 255>(roi, args, output);
            break;
        case eImageBitDepthShort:
            scaleToTexture32bitsForPremult<unsigned short, 65535>(roi, args, output);
            break;
        case eImageBitDepthHalf:
            assert(false);
            break;
        case eImageBitDepthNone:
            break;
    }
} // scaleToTexture32bits


void
ViewerInstance::ViewerInstancePrivate::updateViewer(boost::shared_ptr<UpdateViewerParams> params)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // QMutexLocker locker(&updateViewerMutex);
    // if (updateViewerRunning) {
    uiContext->makeOpenGLcontextCurrent();
    
    // how do you make sure params->ramBuffer is not freed during this operation?
    /// It is not freed as long as the cachedFrame shared_ptr in renderViewer has a used_count greater than 1.
    /// i.e. until renderViewer exits.
    /// Since updateViewer() is in the scope of cachedFrame, and renderViewer waits for the completion
    /// of updateViewer(), it is guaranteed not to be freed before the viewer is actually done with it.
    /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
    
    assert(params->ramBuffer);
    
    bool doUpdate = true;
    if (!params->updateOnlyRoi && !params->isSequential && !checkAndUpdateDisplayAge(params->textureIndex,params->renderAge)) {
        doUpdate = false;
    }
    if (doUpdate) {
        
        ImageList tiles;
        if (params->cachedFrame) {
            if (!params->tiles.empty()) {
                tiles = params->tiles;
            } else {
                params->cachedFrame->getOriginalTiles(&tiles);
            }
        } else {
            assert(!params->tiles.empty());
            tiles = params->tiles;
        }

        ImageBitDepthEnum depth;
        if (!tiles.empty()) {
            depth = tiles.front()->getBitDepth();
        } else {
            depth = (ImageBitDepthEnum)params->cachedFrame->getKey().getBitDepth();
        }
        
        uiContext->transferBufferFromRAMtoGPU(params->ramBuffer,
                                              tiles,
                                              depth,
                                              params->time,
                                              params->rod,
                                              params->bytesCount,
                                              params->textureRect,
                                              params->gain,
                                              params->gamma,
                                              params->offset,
                                              params->lut,
                                              updateViewerPboIndex,
                                              params->mipMapLevel,
                                              params->srcPremult,
                                              params->textureIndex,
                                              params->roi,
                                              params->updateOnlyRoi);
        updateViewerPboIndex = (updateViewerPboIndex + 1) % 2;
        
        boost::shared_ptr<Node> rotoPaintNode;
        boost::shared_ptr<RotoStrokeItem> curStroke;
        bool isDrawing;
        instance->getApp()->getActiveRotoDrawingStroke(&rotoPaintNode, &curStroke,&isDrawing);
        
        if (!isDrawing) {
            uiContext->updateColorPicker(params->textureIndex);
        }
    }
    
    //
    //        updateViewerRunning = false;
    //    }
    //    updateViewerCond.wakeOne();
}

bool
ViewerInstance::isInputOptional(int n) const
{
    //activeInput() is MT-safe
    int activeInputs[2];

    getActiveInputs(activeInputs[0], activeInputs[1]);
    
    if ( (n == 0) && (activeInputs[0] == -1) && (activeInputs[1] == -1) ) {
        return false;
    }

    return n != activeInputs[0] && n != activeInputs[1];
}

void
ViewerInstance::onGammaChanged(double value)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        
        if (_imp->viewerParamsGamma != value) {
            _imp->viewerParamsGamma = value;
            changed = true;
        }
    }
    if (changed) {
        QWriteLocker k(&_imp->gammaLookupMutex);
        _imp->fillGammaLut(1. / value);
    }
    assert(_imp->uiContext);
    if (changed) {
        if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) || !_imp->uiContext->supportsGLSL() )
            && !getApp()->getProject()->isLoadingProject() ) {
            renderCurrentFrame(true);
        } else {
            _imp->uiContext->redraw();
        }
    }
}

double
ViewerInstance::getGamma() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);
    return _imp->viewerParamsGamma;
}

void
ViewerInstance::onGainChanged(double exp)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsGain != exp) {
            _imp->viewerParamsGain = exp;
            changed = true;
        }
    }
    if (changed) {
        assert(_imp->uiContext);
        if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) || !_imp->uiContext->supportsGLSL() )
            && !getApp()->getProject()->isLoadingProject() ) {
            renderCurrentFrame(true);
        } else {
            _imp->uiContext->redraw();
        }
    }
}

void
ViewerInstance::onMipMapLevelChanged(int level)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerMipMapLevel == (unsigned int)level) {
            return;
        }
        _imp->viewerMipMapLevel = level;
    }
    if (!getApp()->getProject()->isLoadingProject()) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::onAutoContrastChanged(bool autoContrast,
                                      bool refresh)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsAutoContrast != autoContrast) {
            _imp->viewerParamsAutoContrast = autoContrast;
            changed = true;
        }
    }
    if ( changed && refresh && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

bool
ViewerInstance::isAutoContrastEnabled() const
{
    // MT-safe
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsAutoContrast;
}

void
ViewerInstance::onColorSpaceChanged(ViewerColorSpaceEnum colorspace)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsLut == colorspace) {
            return;
        }
        _imp->viewerParamsLut = colorspace;
    }
    assert(_imp->uiContext);
    if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) || !_imp->uiContext->supportsGLSL() )
        && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    } else {
        _imp->uiContext->redraw();
    }
}

void
ViewerInstance::setDisplayChannels(DisplayChannelsEnum channels, bool bothInputs)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (!bothInputs) {
            if (_imp->viewerParamsChannels[0] != channels) {
                _imp->viewerParamsChannels[0] = channels;
                changed = true;
            }
        } else {
            if (_imp->viewerParamsChannels[0] != channels) {
                _imp->viewerParamsChannels[0] = channels;
                changed = true;
            }
            if (_imp->viewerParamsChannels[1] != channels) {
                _imp->viewerParamsChannels[1] = channels;
                changed = true;
            }
        }
    }
    if ( changed && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::setActiveLayer(const ImageComponents& layer, bool doRender)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsLayer != layer) {
            _imp->viewerParamsLayer = layer;
            changed = true;
        }
    }
    if ( doRender && changed && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::setAlphaChannel(const ImageComponents& layer, const std::string& channelName, bool doRender)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsAlphaLayer != layer) {
            _imp->viewerParamsAlphaLayer = layer;
            changed = true;
        }
        if (_imp->viewerParamsAlphaChannelName != channelName) {
            _imp->viewerParamsAlphaChannelName = channelName;
            changed = true;
        }
    }
    if ( changed && doRender && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::disconnectViewer()
{
    // always running in the render thread
    if (_imp->uiContext) {
        Q_EMIT viewerDisconnected();
    }
}

void
ViewerInstance::disconnectTexture(int index)
{
    if (_imp->uiContext) {
        Q_EMIT disconnectTextureRequest(index);
    }
}


bool
ViewerInstance::supportsGLSL() const
{

    ///This is a short-cut, this is primarily used when the user switch the
    /// texture mode in the preferences menu. If the hardware doesn't support GLSL
    /// it returns false, true otherwise. @see Settings::onKnobValueChanged
    assert(_imp->uiContext);

    return _imp->uiContext->supportsGLSL();
}

void
ViewerInstance::redrawViewer()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(_imp->uiContext);
    _imp->uiContext->redraw();
}

void
ViewerInstance::redrawViewerNow()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(_imp->uiContext);
    _imp->uiContext->redrawNow();
}

int
ViewerInstance::getLutType() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsLut;
}

double
ViewerInstance::getGain() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsGain;
}

int
ViewerInstance::getMipMapLevel() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerMipMapLevel;
}


DisplayChannelsEnum
ViewerInstance::getChannels(int texIndex) const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsChannels[texIndex];
}

void
ViewerInstance::addAcceptedComponents(int /*inputNb*/,
                                      std::list<ImageComponents>* comps)
{
    ///Viewer only supports RGBA for now.
    comps->push_back(ImageComponents::getRGBAComponents());
    comps->push_back(ImageComponents::getRGBComponents());
    comps->push_back(ImageComponents::getAlphaComponents());
}

int
ViewerInstance::getViewerCurrentView() const
{
    return _imp->uiContext ? _imp->uiContext->getCurrentView() : 0;
}

void
ViewerInstance::setActivateInputChangeRequestedFromViewer(bool fromViewer)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->activateInputChangedFromViewer = fromViewer;
}

bool
ViewerInstance::isInputChangeRequestedFromViewer() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->activateInputChangedFromViewer;
}

void
ViewerInstance::refreshActiveInputs(int inputNbChanged)
{
    assert( QThread::currentThread() == qApp->thread() );
    NodePtr inputNode = getNode()->getRealInput(inputNbChanged);
    {
        QMutexLocker l(&_imp->activeInputsMutex);
        if (!inputNode) {
            ///check if the input was one of the active ones if so set to -1
            if (_imp->activeInputs[0] == inputNbChanged) {
                _imp->activeInputs[0] = -1;
            } else if (_imp->activeInputs[1] == inputNbChanged) {
                _imp->activeInputs[1] = -1;
            }
        } else {
            if (_imp->activeInputs[0] != -1 && _imp->activateInputChangedFromViewer) {
                ViewerCompositingOperatorEnum op = _imp->uiContext->getCompositingOperator();
                if (op == eViewerCompositingOperatorNone) {
                    _imp->uiContext->setCompositingOperator(eViewerCompositingOperatorWipe);
                }
                _imp->activeInputs[1] = inputNbChanged;
                
            } else {
                _imp->activeInputs[0] = inputNbChanged;
            }
        }
    }
    Q_EMIT activeInputsChanged();
    Q_EMIT refreshOptionalState();
}

void
ViewerInstance::onInputChanged(int /*inputNb*/)
{
  
    Q_EMIT clipPreferencesChanged();
}

bool
ViewerInstance::refreshClipPreferences(double /*time*/,
                                       const RenderScale & /*scale*/,
                                       ValueChangedReasonEnum /*reason*/,
                                       bool /*forceGetClipPrefAction*/)
{
    Q_EMIT clipPreferencesChanged();
    return false;
}

void
ViewerInstance::onChannelsSelectorRefreshed()
{
    Q_EMIT availableComponentsChanged();
}

void
ViewerInstance::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthByte);
}

void
ViewerInstance::getActiveInputs(int & a,
                                int &b) const
{
    QMutexLocker l(&_imp->activeInputsMutex);

    a = _imp->activeInputs[0];
    b = _imp->activeInputs[1];
}

void
ViewerInstance::setInputA(int inputNb)
{
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&_imp->activeInputsMutex);
        _imp->activeInputs[0] = inputNb;
    }
    Q_EMIT refreshOptionalState();
}

void
ViewerInstance::setInputB(int inputNb)
{
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&_imp->activeInputsMutex);
        _imp->activeInputs[1] = inputNb;
    }
    Q_EMIT refreshOptionalState();
}

int
ViewerInstance::getLastRenderedTime() const
{
    return _imp->uiContext ? _imp->uiContext->getCurrentlyDisplayedTime() : getApp()->getTimeLine()->currentFrame();
}

boost::shared_ptr<TimeLine>
ViewerInstance::getTimeline() const
{
    return _imp->uiContext ? _imp->uiContext->getTimeline() : getApp()->getTimeLine();
}

void
ViewerInstance::getTimelineBounds(int* first,int* last) const
{
    if (_imp->uiContext) {
        _imp->uiContext->getViewerFrameRange(first, last);
    } else {
        *first = 0;
        *last = 0;
    }
}


int
ViewerInstance::getMipMapLevelFromZoomFactor() const
{
    double zoomFactor = _imp->uiContext->getZoomFactor();
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2,-std::ceil(std::log(zoomFactor) / M_LN2) );
    return std::log(closestPowerOf2) / M_LN2;
}

double
ViewerInstance::getCurrentTime() const
{
    return getFrameRenderArgsCurrentTime();
}

int
ViewerInstance::getCurrentView() const
{
    return getFrameRenderArgsCurrentView();
}

bool
ViewerInstance::isRenderAbortable(int textureIndex, U64 renderAge) const
{
    return _imp->isRenderAbortable(textureIndex, renderAge);
}

void
ViewerInstance::reportStats(int time, int view, double wallTime, const RenderStatsMap& stats)
{
    Q_EMIT renderStatsAvailable(time, view, wallTime, stats);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ViewerInstance.cpp"
#include "moc_ViewerInstancePrivate.cpp"
