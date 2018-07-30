/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <cassert>
#include <cstring> // for std::memcpy
#include <cfloat> // DBL_MAX

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
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

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/Image.h"
#include "Engine/Log.h"
#include "Engine/Lut.h"
#include "Engine/MemoryFile.h"
#include "Engine/MemoryInfo.h" // printAsRAM
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
#include "Engine/UpdateViewerParams.h"
#include "Engine/ViewIdx.h"


#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif

#define NATRON_TIME_ELASPED_BEFORE_PROGRESS_REPORT 4. //!< do not display the progress report if estimated total time is less than this (in seconds)

NATRON_NAMESPACE_ENTER

using std::make_pair;
using boost::shared_ptr;

NATRON_NAMESPACE_ANONYMOUS_ENTER

struct MinMaxVal {
    MinMaxVal(double min_, double max_)
    : min(min_)
    , max(max_)
    {
    }
    MinMaxVal()
    : min(DBL_MAX)
    , max(-DBL_MAX)
    {
    }

    double min;
    double max;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

static void scaleToTexture8bits(const RectI& roi,
                                const RenderViewerArgs & args,
                                ViewerInstance* viewer,
                                const UpdateViewerParams::CachedTile& tile,
                                U32* output);
static void scaleToTexture32bits(const RectI& roi,
                                 const RenderViewerArgs & args,
                                 const UpdateViewerParams::CachedTile& tile,
                                 float *output);
static MinMaxVal findAutoContrastVminVmax(const ImagePtr inputImage,
                                                         DisplayChannelsEnum channels,
                                                         const RectI & rect);
static void renderFunctor(const RectI& roi,
                          const RenderViewerArgs & args,
                          ViewerInstance* viewer,
                          UpdateViewerParams::CachedTile tile);

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

EffectInstance*
ViewerInstance::BuildEffect(NodePtr n)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return new ViewerInstance(n);
}

ViewerInstance::ViewerInstance(NodePtr node)
    : OutputEffectInstance(node)
    , _imp( new ViewerInstancePrivate(this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);

    QObject::connect( this, SIGNAL(disconnectTextureRequest(int,bool)), this, SLOT(executeDisconnectTextureRequestOnMainThread(int,bool)) );
    QObject::connect( _imp.get(), SIGNAL(mustRedrawViewer()), this, SLOT(redrawViewer()) );
    QObject::connect( this, SIGNAL(s_callRedrawOnMainThread()), this, SLOT(redrawViewer()) );
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
    ViewerInstancePtr thisShared = boost::dynamic_pointer_cast<ViewerInstance>( shared_from_this() );

    return new ViewerRenderEngine(thisShared);
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
    _imp->forceRender[0] = _imp->forceRender[1] = true;
}

void
ViewerInstance::clearLastRenderedImage()
{
    EffectInstance::clearLastRenderedImage();

    if (_imp->uiContext) {
        _imp->uiContext->clearLastRenderedImage();
    }
    {
        QMutexLocker k(&_imp->lastRenderParamsMutex);
        _imp->lastRenderParams[0].reset();
        _imp->lastRenderParams[1].reset();
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
ViewerInstance::getNInputs() const
{
    return 10;
}

void
ViewerInstance::getFrameRange(double *first,
                              double *last)
{
    double inpFirst = 1, inpLast = 1;
    int activeInputs[2];

    getActiveInputs(activeInputs[0], activeInputs[1]);
    EffectInstancePtr n1 = getInput(activeInputs[0]);
    if (n1) {
        n1->getFrameRange_public(n1->getRenderHash(), &inpFirst, &inpLast);
    }
    *first = inpFirst;
    *last = inpLast;

    inpFirst = 1;
    inpLast = 1;

    EffectInstancePtr n2 = getInput(activeInputs[1]);
    if (n2) {
        n2->getFrameRange_public(n2->getRenderHash(), &inpFirst, &inpLast);
        if (inpFirst < *first) {
            *first = inpFirst;
        }

        if (inpLast > *last) {
            *last = inpLast;
        }
    }
}

void
ViewerInstance::executeDisconnectTextureRequestOnMainThread(int index,bool clearRoD)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->uiContext) {
        _imp->uiContext->disconnectInputTexture(index, clearRoD);
    }
}

static bool
isRotoPaintNodeInputRecursive(Node* node,
                              const NodePtr& rotoPaintNode)
{
    if ( node == rotoPaintNode.get() ) {
        return true;
    }
    int maxInputs = node->getNInputs();
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

static void
updateLastStrokeDataRecursively(Node* node,
                                const NodePtr& rotoPaintNode,
                                const RectD& lastStrokeBbox,
                                bool invalidate)
{
    if ( isRotoPaintNodeInputRecursive(node, rotoPaintNode) ) {
        if (invalidate) {
            node->invalidateLastPaintStrokeDataNoRotopaint();
        } else {
            node->setLastPaintStrokeDataNoRotopaint();
        }

        if ( node == rotoPaintNode.get() ) {
            return;
        }
        int maxInputs = node->getNInputs();
        for (int i = 0; i < maxInputs; ++i) {
            NodePtr input = node->getInput(i);
            if (input) {
                updateLastStrokeDataRecursively(input.get(), rotoPaintNode, lastStrokeBbox, invalidate);
            }
        }
    }
}

class ViewerParallelRenderArgsSetter
    : public ParallelRenderArgsSetter
{
    NodePtr rotoNode;
    NodePtr viewerNode;
    NodePtr viewerInputNode;

public:

    ViewerParallelRenderArgsSetter(double time,
                                   ViewIdx view,
                                   bool isRenderUserInteraction,
                                   bool isSequential,
                                   const AbortableRenderInfoPtr& abortInfo,
                                   const NodePtr& treeRoot,
                                   int textureIndex,
                                   const TimeLine* timeline,
                                   bool isAnalysis,
                                   const NodePtr& rotoPaintNode,
                                   const NodePtr& viewerInput,
                                   bool draftMode,
                                   const RenderStatsPtr& stats)
        : ParallelRenderArgsSetter(time, view, isRenderUserInteraction, isSequential, abortInfo, treeRoot, textureIndex, timeline, rotoPaintNode, isAnalysis, draftMode, stats)
        , rotoNode(rotoPaintNode)
        , viewerNode(treeRoot)
        , viewerInputNode()
    {
        ///There can be a case where the viewer input tree does not belong to the project, for example
        ///for the File Dialog preview.
        if ( viewerInput && !viewerInput->getGroup() ) {
            viewerInputNode = viewerInput;
            bool doNanHandling = appPTR->getCurrentSettings()->isNaNHandlingEnabled();
            U64 nodeHash = viewerInput->getHashValue();


            viewerInput->getEffectInstance()->setParallelRenderArgsTLS(time, view, isRenderUserInteraction, isSequential, nodeHash,  abortInfo, treeRoot, 1, NodeFrameRequestPtr(), _openGLContext.lock(), textureIndex, timeline, isAnalysis, false, NodesList(), viewerInput->getCurrentRenderThreadSafety(), viewerInput->getCurrentOpenGLRenderSupport(), doNanHandling, draftMode, stats);
        }
    }

    virtual ~ViewerParallelRenderArgsSetter()
    {
        if (rotoNode) {
            updateLastStrokeDataRecursively(viewerNode.get(), rotoNode, RectD(), true);
        }
        if (viewerInputNode) {
            viewerInputNode->getEffectInstance()->invalidateParallelRenderArgsTLS();
        }
    }
};

ViewerInstance::ViewerRenderRetCode
ViewerInstance::getViewerArgsAndRenderViewer(SequenceTime time,
                                             bool canAbort,
                                             ViewIdx view,
                                             U64 viewerHash,
                                             const NodePtr& rotoPaintNode,
                                             const RotoStrokeItemPtr& activeStroke,
                                             const RenderStatsPtr& stats,
                                             ViewerArgsPtr* argsA,
                                             ViewerArgsPtr* argsB)
{
    ///This is used only by the rotopaint while drawing. We must clear the action cache of the rotopaint node before calling
    ///getRoD or this will not work
    assert(rotoPaintNode);
    if ( !rotoPaintNode->getEffectInstance() ) {
        return eViewerRenderRetCodeFail;
    }
    rotoPaintNode->getEffectInstance()->clearActionsCache();

    ViewerRenderRetCode status[2] = {
        eViewerRenderRetCodeFail, eViewerRenderRetCodeFail
    };
    NodePtr thisNode = getNode();
    ViewerArgsPtr args[2];
    for (int i = 0; i < 2; ++i) {
        args[i] = boost::make_shared<ViewerArgs>();
        if ( (i == 1) && (_imp->uiContext->getCompositingOperator() == eViewerCompositingOperatorNone) ) {
            break;
        }

        AbortableRenderInfoPtr abortInfo = _imp->createNewRenderRequest(i, canAbort);


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
                                           abortInfo,
                                           thisNode,
                                           i,
                                           getTimeline().get(),
                                           false,
                                           rotoPaintNode,
                                           NodePtr(),
                                           false,
                                           stats);
        NodesList rotoPaintNodes;
        if (rotoPaintNode) {
            if (activeStroke) {
                EffectInstancePtr rotoLive = rotoPaintNode->getEffectInstance();
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
                std::list<std::pair<Point, double> > lastStrokePoints;

                //The stroke RoD so far
                RectD wholeStrokeRod;

                //The bbox of the lastStrokePoints
                RectD lastStrokeBbox;

                //The index in the stroke of the last point we have rendered and up to the new point we have rendered
                int lastAge, newAge;

                //get on the app object the last point index we have rendered on this stroke
                lastAge = getApp()->getStrokeLastIndex();

                //Get the active paint stroke so far and its multi-stroke index
                RotoStrokeItemPtr currentlyPaintedStroke;
                int currentlyPaintedStrokeMultiIndex;
                getApp()->getStrokeAndMultiStrokeIndex(&currentlyPaintedStroke, &currentlyPaintedStrokeMultiIndex);


                //If this crashes here that means the user could start a new stroke while this one is not done rendering.
                assert(currentlyPaintedStroke == activeStroke);

                //the multi-stroke index in case of a stroke containing multiple strokes from the user
                int strokeIndex;
                if ( activeStroke->getMostRecentStrokeChangesSinceAge(time, lastAge, currentlyPaintedStrokeMultiIndex, &lastStrokePoints, &lastStrokeBbox, &wholeStrokeRod, &newAge, &strokeIndex) ) {
                    getApp()->updateLastPaintStrokeData(newAge, lastStrokePoints, lastStrokeBbox, strokeIndex);
                    for (NodesList::iterator it = rotoPaintNodes.begin(); it != rotoPaintNodes.end(); ++it) {
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
            status[i] = getRenderViewerArgsAndCheckCache( time, false, view, i, viewerHash, rotoPaintNode, abortInfo, stats,  args[i].get() );
        }


        if (status[i] != eViewerRenderRetCodeRender) {
            /*
               Either failure, black or nothing, the texture is junk, remove it from the cache
             */
            if (args[i] && args[i]->params) {
                for (std::list<UpdateViewerParams::CachedTile>::iterator it = args[i]->params->tiles.begin(); it != args[i]->params->tiles.end(); ++it) {
                    if (it->cachedData) {
                        //it->cachedData->setAborted(true);
                        //appPTR->removeFromViewerCache(it->cachedData);
                        it->cachedData.reset();
                    }
                }
                args[i]->params->tiles.clear();
            }
        }


        if ( (status[i] == eViewerRenderRetCodeFail) || (status[i] == eViewerRenderRetCodeBlack) ) {
            disconnectTextureRequest(i, status[i] == eViewerRenderRetCodeFail);
        } else {
            assert(args[i] && args[i]->params);
            assert(args[i]->params->textureIndex == i);


            if ( !_imp->addOngoingRender(args[i]->params->textureIndex, abortInfo) ) {
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
                                                  ViewerCurrentFrameRequestSchedulerStartArgsPtr(),
                                                  stats,
                                                  *args[i]);
                args[i]->isRenderingFlag.reset();
            }

            if (args[i] && args[i]->params) {
                if ( (status[i] == eViewerRenderRetCodeFail) || (status[i] == eViewerRenderRetCodeBlack) ) {
                    _imp->checkAndUpdateDisplayAge( args[i]->params->textureIndex, abortInfo->getRenderAge() );
                }
                _imp->removeOngoingRender( args[i]->params->textureIndex, abortInfo->getRenderAge() );
            }


            if (status[i] == eViewerRenderRetCodeRedraw) {
                args[i].reset();
            }
        }
    }

    if ( (status[0] == eViewerRenderRetCodeFail) && (status[1] == eViewerRenderRetCodeFail) ) {
        return eViewerRenderRetCodeFail;
    }

    *argsA = args[0];
    *argsB = args[1];

    return eViewerRenderRetCodeRender;
} // ViewerInstance::getViewerArgsAndRenderViewer

ViewerInstance::ViewerRenderRetCode
ViewerInstance::renderViewer(ViewIdx view,
                             bool singleThreaded,
                             bool isSequentialRender,
                             U64 viewerHash,
                             bool canAbort,
                             const NodePtr& rotoPaintNode,
                             bool useTLS,
                             ViewerArgsPtr args[2],
                             const ViewerCurrentFrameRequestSchedulerStartArgsPtr& request,
                             const RenderStatsPtr& stats)
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
        if (args[i] && args[i]->params) {
            if ( (i == 1) && (_imp->uiContext->getCompositingOperator() == eViewerCompositingOperatorNone) ) {
                args[i]->params->tiles.clear();
                break;
            }
            assert(args[i]->params->textureIndex == i);

            ///We enable render stats just for the A input (i == 0) otherwise we would get crappy results

            if (!isSequentialRender) {
                if ( !_imp->addOngoingRender(args[i]->params->textureIndex, args[i]->params->abortInfo) ) {
                    /*
                       This may fail if another thread already pushed a more recent render in the render ages queue
                     */
                    ret[i] = eViewerRenderRetCodeRedraw;
                    args[i].reset();
                }
            }
            if (args[i]) {
                ret[i] = renderViewer_internal(view, singleThreaded, isSequentialRender, viewerHash, canAbort, rotoPaintNode, useTLS, request,
                                               i == 0 ? stats : RenderStatsPtr(),
                                               *args[i]);

                // Reset the rednering flag
                args[i]->isRenderingFlag.reset();
            }

            if (ret[i] != eViewerRenderRetCodeRender) {
                /*
                   Either failure, black or nothing, the texture is junk, remove it from the cache
                 */
                if (args[i] && args[i]->params) {
                    for (std::list<UpdateViewerParams::CachedTile>::iterator it = args[i]->params->tiles.begin(); it != args[i]->params->tiles.end(); ++it) {
                        if (it->cachedData) {
                            //it->cachedData->setAborted(true);
                            //appPTR->removeFromViewerCache(it->cachedData);
                            it->cachedData.reset();
                        }
                    }
                    args[i]->params->tiles.clear();
                }
            }

            if (!isSequentialRender && args[i] && args[i]->params) {
                if ( (ret[i] == eViewerRenderRetCodeFail) || (ret[i] == eViewerRenderRetCodeBlack) ) {
                    _imp->checkAndUpdateDisplayAge( args[i]->params->textureIndex, args[i]->params->abortInfo->getRenderAge() );
                }
                _imp->removeOngoingRender( args[i]->params->textureIndex, args[i]->params->abortInfo->getRenderAge() );
            }

            if (ret[i] == eViewerRenderRetCodeBlack) {
                disconnectTexture(args[i]->params->textureIndex, false);
            }

            if (ret[i] == eViewerRenderRetCodeFail) {
                args[i].reset();
            }
        }
    }


    if ( (ret[0] == eViewerRenderRetCodeFail) || (ret[1] == eViewerRenderRetCodeFail) ) {
        return eViewerRenderRetCodeFail;
    }

    return eViewerRenderRetCodeRender;
} // ViewerInstance::renderViewer

static bool
checkTreeCanRender_internal(Node* node,
                            std::list<Node*>& marked)
{
    if ( std::find(marked.begin(), marked.end(), node) != marked.end() ) {
        return true;
    }

    marked.push_back(node);

    // check that the nodes upstream have all their nonoptional inputs connected
    int maxInput = node->getNInputs();
    for (int i = 0; i < maxInput; ++i) {
        NodePtr input = node->getInput(i);
        bool optional = node->getEffectInstance()->isInputOptional(i);
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
static bool
checkTreeCanRender(Node* node)
{
    std::list<Node*> marked;
    bool ret = checkTreeCanRender_internal(node, marked);

    return ret;
}

static unsigned char*
getTexPixel(int x,
            int y,
            const TextureRect& bounds,
            std::size_t pixelDepth,
            unsigned char* bufStart)
{
    if ( ( x < bounds.x1 ) || ( x >= bounds.x2 ) || ( y < bounds.y1 ) || ( y >= bounds.y2 ) ) {
        return NULL;
    } else {
        int compDataSize = pixelDepth * 4;

        return (unsigned char*)(bufStart)
               + (qint64)( y - bounds.y1 ) * compDataSize * bounds.width()
               + (qint64)( x - bounds.x1 ) * compDataSize;
    }
}

static bool
copyAndSwap(const TextureRect& srcRect,
            const TextureRect& dstRect,
            std::size_t dstBytesCount,
            ImageBitDepthEnum bitdepth,
            unsigned char* srcBuf,
            unsigned char** dstBuf)
{
    // Ensure it has the correct size, resize it if needed
    if ( (srcRect.x1 == dstRect.x1) &&
         ( srcRect.y1 == dstRect.y1) &&
         ( srcRect.x2 == dstRect.x2) &&
         ( srcRect.y2 == dstRect.y2) ) {
        *dstBuf = srcBuf;

        return false;
    }

    // Use calloc so that newly allocated areas are already black and transparent
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

    std::size_t srcRowSize = srcRect.width() * 4 * pixelDepth;
    std::size_t dstRowSize = dstRect.width() * 4 * pixelDepth;

    for (int y = srcRect.y1; y < srcRect.y2;
         ++y, srcPixels += srcRowSize, dstPixels += dstRowSize) {
        std::memcpy(dstPixels, srcPixels, srcRowSize);
    }
    *dstBuf = tmpBuf;

    return true;
}

ViewerInstance::ViewerRenderRetCode
ViewerInstance::getRenderViewerArgsAndCheckCache_public(SequenceTime time,
                                                        bool isSequential,
                                                        ViewIdx view,
                                                        int textureIndex,
                                                        U64 viewerHash,
                                                        bool canAbort,
                                                        const NodePtr& rotoPaintNode,
                                                        const RenderStatsPtr& stats,
                                                        ViewerArgs* outArgs)
{
    AbortableRenderInfoPtr abortInfo = _imp->createNewRenderRequest(textureIndex, canAbort);
    ViewerRenderRetCode stat = getRenderViewerArgsAndCheckCache(time, isSequential, view, textureIndex, viewerHash, rotoPaintNode, abortInfo, stats, outArgs);

    if ( (stat == eViewerRenderRetCodeFail) || (stat == eViewerRenderRetCodeBlack) ) {
        _imp->checkAndUpdateDisplayAge( textureIndex, abortInfo->getRenderAge() );
    }

    return stat;
}

void
ViewerInstance::setupMinimalUpdateViewerParams(const SequenceTime time,
                                               const ViewIdx view,
                                               const int textureIndex,
                                               const AbortableRenderInfoPtr& abortInfo,
                                               const bool isSequential,
                                               ViewerArgs* outArgs)
{
    assert(_imp->uiContext);

    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        outArgs->mipmapLevelWithoutDraft = (unsigned int)_imp->viewerMipMapLevel;
    }

    assert(_imp->uiContext);

    // This is the current zoom factor (1. == 100%) currently set by the user in the viewport
    double zoomFactor = _imp->uiContext->getZoomFactor();

    // We render the image that is the nearest mipmap level higher in quality.
    // For instance, if we were to render at 48% zoom factor, we would render at 50% which is mipmapLevel=1
    // If on the other hand the zoom factor would be at 51%, then we would render at 100% which is mipmapLevel=0

    // Adjust the mipmap level (without taking draft into account yet) as the max of the closest mipmap level of the viewer zoom
    // and the requested user proxy mipmap level
    if (isFullFrameProcessingEnabled()) {
        outArgs->mipmapLevelWithoutDraft = 0;
    } else {
        int zoomMipMapLevel;
        {
            double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) );
            zoomMipMapLevel = std::log(closestPowerOf2) / M_LN2;
        }
        outArgs->mipmapLevelWithoutDraft = (unsigned int)std::max( (int)outArgs->mipmapLevelWithoutDraft, (int)zoomMipMapLevel );
    }
    outArgs->mipMapLevelWithDraft = outArgs->mipmapLevelWithoutDraft;

    outArgs->draftModeEnabled = getApp()->isDraftRenderEnabled();

    // If draft mode is enabled, compute the mipmap level according to the auto-proxy setting in the preferences
    if ( outArgs->draftModeEnabled && appPTR->getCurrentSettings()->isAutoProxyEnabled() ) {
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
        outArgs->mipMapLevelWithDraft = (unsigned int)std::max( (int)outArgs->mipmapLevelWithoutDraft, (int)autoProxyLevel );
    }


    // The hash of the node to render, we store it and make sure we never call getHash() again for the render of this frame
    outArgs->activeInputHash = outArgs->activeInputToRender->getHash();

    // Initialize the flag
    outArgs->mustComputeRoDAndLookupCache = true;

    // Check if the render was issued from the "Refresh" button, in which case we compute images from nodes at least once
    {
        QMutexLocker forceRenderLocker(&_imp->forceRenderMutex);
        outArgs->forceRender = _imp->forceRender[textureIndex];
        _imp->forceRender[textureIndex] = false;
    }

    // Did the user enabled the user roi from the viewer UI?
    outArgs->userRoIEnabled = _imp->uiContext->isUserRegionOfInterestEnabled();

    outArgs->params = boost::make_shared<UpdateViewerParams>();

    // The PAR of the input image
    outArgs->params->pixelAspectRatio = outArgs->activeInputToRender->getAspectRatio(-1);

    // Is it playback ?
    outArgs->params->isSequential = isSequential;

    // Used to identify this render when calling EffectInstance::Implementation::aborted
    outArgs->params->abortInfo = abortInfo;

    // Used to differentiate the 2 different textures when wipe is enabled
    outArgs->params->setUniqueID(textureIndex);

    // Used to determine how the viewer should handle alpha
    outArgs->params->srcPremult = outArgs->activeInputToRender->getPremult();

    // The user requested bitdepth of the textures
    outArgs->params->depth = _imp->uiContext->getBitDepth();

    // The frame number
    outArgs->params->time = time;

    // The view to render
    outArgs->params->view = view;

    // A input = 0, B input = 1
    outArgs->params->textureIndex = textureIndex;

    // These are the user settings from the viewer UI
    {
        QMutexLocker locker(&_imp->viewerParamsMutex);
        outArgs->autoContrast = _imp->viewerParamsAutoContrast;
        outArgs->channels = _imp->viewerParamsChannels[textureIndex];
        outArgs->params->gain = _imp->viewerParamsGain;
        outArgs->params->gamma = _imp->viewerParamsGamma;
        outArgs->params->lut = _imp->viewerParamsLut;
        outArgs->params->layer = _imp->viewerParamsLayer;
        outArgs->params->alphaLayer = _imp->viewerParamsAlphaLayer;
        outArgs->params->alphaChannelName = _imp->viewerParamsAlphaChannelName;
        outArgs->isDoingPartialUpdates = _imp->isDoingPartialUpdates;
    }

    // Fill the gamma LUT if it has never been filled yet
    bool gammaLookupEmpty;
    {
        QReadLocker k(&_imp->gammaLookupMutex);
        gammaLookupEmpty = _imp->gammaLookup.empty();
    }
    if (gammaLookupEmpty) {
        QWriteLocker k(&_imp->gammaLookupMutex);
        if ( _imp->gammaLookup.empty() ) {
            _imp->fillGammaLut(outArgs->params->gamma);
        }
    }

    // Flag that we are going to render
    outArgs->isRenderingFlag = boost::make_shared<RenderingFlagSetter>( getNode() );
} // ViewerInstance::setupMinimalUpdateViewerParams

void
ViewerInstance::getRegionsOfInterest(double /*time*/,
                                     const RenderScale & /*scale*/,
                                     const RectD & /*outputRoD*/,   //!< the RoD of the effect, in canonical coordinates
                                     const RectD & renderWindow,   //!< the region to be rendered in the output image, in Canonical Coordinates
                                     ViewIdx /*view*/,
                                     RoIMap* ret)
{
#pragma message WARN("2.2: fix this and only add RoI for thread local input")
    for (int i = 0; i < getNInputs(); ++i) {
        EffectInstancePtr input = getInput(i);
        if (input) {
            ret->insert( std::make_pair(input, renderWindow) );
        }
    }
}

ViewerInstance::ViewerRenderRetCode
ViewerInstance::getViewerRoIAndTexture(const RectD& rod,
                                       const U64 viewerHash,
                                       const bool useCache,
                                       const bool isDraftMode,
                                       const unsigned int mipmapLevel,
                                       const RenderStatsPtr& stats,
                                       ViewerArgs* outArgs)
{
    // Roi is the coordinates of the 4 corners of the texture in the bounds with the current zoom
    // factor taken into account.

    // When auto-contrast is enabled or user RoI, we compute exactly the iamge portion displayed in the rectangle and
    // do not round it to Viewer tiles.

    outArgs->params->tiles.clear();
    outArgs->params->nbCachedTile = 0;
    if (!useCache || outArgs->forceRender) {
        outArgs->params->roi = _imp->uiContext->getExactImageRectangleDisplayed(outArgs->params->textureIndex, rod, outArgs->params->pixelAspectRatio, mipmapLevel);
        outArgs->params->roiNotRoundedToTileSize = outArgs->params->roi;

        UpdateViewerParams::CachedTile tile;
        if (outArgs->isDoingPartialUpdates) {
            std::list<RectD> partialRects;
            QMutexLocker k(&_imp->viewerParamsMutex);
            partialRects = _imp->partialUpdateRects;
            for (std::list<RectD>::iterator it = partialRects.begin(); it != partialRects.end(); ++it) {
                RectI pixelRect;
                it->toPixelEnclosing(mipmapLevel, outArgs->params->pixelAspectRatio, &pixelRect);
                ///Intersect to the RoI
                if ( pixelRect.intersect(outArgs->params->roi, &pixelRect) ) {
                    tile.rect.set(pixelRect);
                    tile.rectRounded  = pixelRect;
                    tile.rect.closestPo2 = 1 << mipmapLevel;
                    tile.rect.par = outArgs->params->pixelAspectRatio;
                    tile.bytesCount = tile.rect.area() * 4;
                    assert(tile.bytesCount > 0);
                    if (outArgs->params->depth == eImageBitDepthFloat) {
                        tile.bytesCount *= sizeof(float);
                    }
                    outArgs->params->tiles.push_back(tile);
                }
            }
        } else {
            if (!outArgs->params->roi.isNull()) {
                tile.rect.set(outArgs->params->roi);
                tile.rectRounded = outArgs->params->roi;
                tile.rect.closestPo2 = 1 << mipmapLevel;
                tile.rect.par = outArgs->params->pixelAspectRatio;
                tile.bytesCount = tile.rect.area() * 4;
                assert(tile.bytesCount > 0);
                if (outArgs->params->depth == eImageBitDepthFloat) {
                    tile.bytesCount *= sizeof(float);
                }
                outArgs->params->tiles.push_back(tile);
            }
        }

    } else {
        std::vector<RectI> tiles, tilesRounded;
        outArgs->params->roi = _imp->uiContext->getImageRectangleDisplayedRoundedToTileSize(outArgs->params->textureIndex, rod, outArgs->params->pixelAspectRatio, mipmapLevel, &tiles, &tilesRounded, &outArgs->params->tileSize, &outArgs->params->roiNotRoundedToTileSize);

        assert(tiles.size() == tilesRounded.size());

        std::vector<RectI>::iterator itRounded = tilesRounded.begin();
        for (std::vector<RectI>::iterator it = tiles.begin(); it != tiles.end(); ++it, ++itRounded) {
            UpdateViewerParams::CachedTile tile;
            tile.rectRounded = *itRounded;
            tile.rect.set(*it);
            tile.rect.closestPo2 = 1 << mipmapLevel;
            tile.rect.par = outArgs->params->pixelAspectRatio;
            tile.bytesCount = outArgs->params->tileSize * outArgs->params->tileSize * 4; // RGBA
            assert( outArgs->params->roi.contains(tile.rect) );
            // If we are using floating point textures, multiply by size of float
            assert(tile.bytesCount > 0);
            if (outArgs->params->depth == eImageBitDepthFloat) {
                tile.bytesCount *= sizeof(float);
            }
            outArgs->params->tiles.push_back(tile);
        }
    }

    // If the RoI does not fall into the visible portion on the viewer, just clear the viewer to black
    if ( outArgs->params->roi.isNull() || (outArgs->params->tiles.size() == 0) ) {
        return eViewerRenderRetCodeBlack;
    }


    outArgs->params->rod = rod;
    outArgs->params->mipMapLevel = mipmapLevel;

    std::string inputToRenderName = outArgs->activeInputToRender->getNode()->getScriptName_mt_safe();


    // Texture rect contains the pixel coordinates in the image to be rendered

    if (useCache) {
        FrameEntryLocker entryLocker(_imp.get());
        for (std::list<UpdateViewerParams::CachedTile>::iterator it = outArgs->params->tiles.begin(); it != outArgs->params->tiles.end(); ++it) {
            FrameKey key(getNode().get(),
                         outArgs->params->time,
                         viewerHash,
                         outArgs->params->gain,
                         outArgs->params->gamma,
                         outArgs->params->lut,
                         (int)outArgs->params->depth,
                         outArgs->channels,
                         outArgs->params->view,
                         it->rect,
                         mipmapLevel,
                         inputToRenderName,
                         outArgs->params->layer,
                         outArgs->params->alphaLayer.getPlaneID() + outArgs->params->alphaChannelName,
                         outArgs->params->depth == eImageBitDepthFloat,
                         isDraftMode);
            std::list<FrameEntryPtr> entries;
            bool hasTextureCached = appPTR->getTexture(key, &entries);
            if ( stats  && stats->isInDepthProfilingEnabled() ) {
                if (hasTextureCached) {
                    stats->addCacheInfosForNode(getNode(), true, false);
                } else {
                    stats->addCacheInfosForNode(getNode(), false, false);
                }
            }

            // If we want to force a refresh, we remove from  the cache texture
            if (outArgs->forceRender && hasTextureCached) {
                for (std::list<FrameEntryPtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
                    appPTR->removeFromViewerCache(*it);
                }
            }
            // Find out if we have a corresponding tile in the cache
            FrameEntryPtr foundCachedEntry;
            for (std::list<FrameEntryPtr>::iterator it2 = entries.begin(); it2 != entries.end(); ++it2) {
                const TextureRect& entryRect = (*it2)->getKey().getTexRect();
                if (entryRect == it->rect) {
                    foundCachedEntry = *it2;
                    break;
                }
            }


            if (foundCachedEntry) {

                // If the tile is cached and we got it that means rendering is done
                entryLocker.lock(foundCachedEntry);

                /*if ( !entryLocker.tryLock(foundCachedEntry) ) {
                    // Another thread is rendering it, just return it is not useful to keep this thread waiting.
                    return eViewerRenderRetCodeRedraw;
                }*/

                /*if ( foundCachedEntry->getAborted() ) {
                    // The thread rendering the frame entry might have been aborted and the entry removed from the cache
                    // but another thread might successfully have found it in the cache. This flag is to notify it the frame

                    // is invalid.
                    return eViewerRenderRetCodeRedraw;
                }*/


                // The data will be valid as long as the cachedFrame shared pointer use_count is gt 1
                it->cachedData = foundCachedEntry;
                it->isCached = true;
                it->ramBuffer = foundCachedEntry->data();
                assert(it->ramBuffer);
                ++outArgs->params->nbCachedTile;
            }
        }
    }


    return eViewerRenderRetCodeRender;
} // ViewerInstance::getViewerRoIAndTexture

ViewerInstance::ViewerRenderRetCode
ViewerInstance::getRoDAndLookupCache(const bool useOnlyRoDCache,
                                     const U64 viewerHash,
                                     const NodePtr& rotoPaintNode,
                                     const RenderStatsPtr& stats,
                                     ViewerArgs* outArgs)
{
    // We never use the texture cache when the user RoI is enabled or while painting or when auto-contrast is on, otherwise we would have
    // zillions of textures in the cache, each a few pixels different.
    const bool useTextureCache = !outArgs->userRoIEnabled && !outArgs->autoContrast && !rotoPaintNode.get() && !outArgs->isDoingPartialUpdates;

    // If it's eSupportsMaybe and mipMapLevel!=0, don't forget to update
    // this after the first call to getRegionOfDefinition().
    const RenderScale scaleOne(1.);

    // This may be eSupportsMaybe
    EffectInstance::SupportsEnum supportsRS = outArgs->activeInputToRender->supportsRenderScaleMaybe();


    // When in draft mode first try to get a texture without draft and then try with draft
    const int nLookups = outArgs->draftModeEnabled ? 2 : 1;

    for (int lookup = 0; lookup < nLookups; ++lookup) {
        const unsigned mipMapLevel = lookup == 0 ? outArgs->mipmapLevelWithoutDraft : outArgs->mipMapLevelWithDraft;
        RenderScale scale;
        scale.x = scale.y = Image::getScaleFromMipMapLevel(mipMapLevel);


        RectD rod;
        // Get the RoD here to be able to figure out what is the RoI of the Viewer.
        // Note that we are in the main-thread (OpenGL) thread here to optimize the code path when all textures are cached
        // so we cannot afford computing the actual RoD. If it's not cached then we will actually compute the RoD in the render thread.
        StatusEnum stat;

        if (useOnlyRoDCache) {
            stat = outArgs->activeInputToRender->getRegionOfDefinitionFromCache(outArgs->activeInputHash,
                                                                                outArgs->params->time,
                                                                                scale,
                                                                                outArgs->params->view,
                                                                                &rod,
                                                                                0 /*isProjectFormat*/);
            if (stat == eStatusFailed) {
                // If was not cached, we cannot lookup the cache in the main-thread at this mipmapLevel
                continue;
            }
        } else {
            stat = outArgs->activeInputToRender->getRegionOfDefinition_public(outArgs->activeInputHash,
                                                                              outArgs->params->time,
                                                                              supportsRS ==  eSupportsNo ? scaleOne : scale,
                                                                              outArgs->params->view,
                                                                              &rod,
                                                                              0 /*isProjectFormat*/);
            if (stat == eStatusFailed) {
                // It really failed, just exit
                return eViewerRenderRetCodeFail;
            }
        }

        // update scale after the first call to getRegionOfDefinition
        if ( (supportsRS == eSupportsMaybe) && (mipMapLevel != 0) ) {
            supportsRS = (outArgs->activeInputToRender)->supportsRenderScaleMaybe();
        }

        outArgs->mustComputeRoDAndLookupCache = false;


        bool isRodProjectFormat = ifInfiniteclipRectToProjectDefault(&rod);
        Q_UNUSED(isRodProjectFormat);

        // Ok we go the RoD, we can actually compute the RoI and look-up the cache
        ViewerRenderRetCode retCode = getViewerRoIAndTexture(rod, viewerHash, useTextureCache, lookup == 1, mipMapLevel, stats, outArgs);
        if (retCode != eViewerRenderRetCodeRender) {
            return retCode;
        }

        assert(outArgs->params->tiles.size() > 0);


        // We found something in the cache, stop now
        if (outArgs->params->nbCachedTile > 0) {
            break;
        }
    } // for (int lookup = 0; lookup < nLookups; ++lookup)

    return eViewerRenderRetCodeRender;
} // ViewerInstance::getRoDAndLookupCache

ViewerInstance::ViewerRenderRetCode
ViewerInstance::getRenderViewerArgsAndCheckCache(SequenceTime time,
                                                 bool isSequential,
                                                 ViewIdx view,
                                                 int textureIndex,
                                                 U64 viewerHash,
                                                 const NodePtr& rotoPaintNode,
                                                 const AbortableRenderInfoPtr& abortInfo,
                                                 const RenderStatsPtr& stats,
                                                 ViewerArgs* outArgs)
{
    // Just redraw if the viewer is paused
    if ( isViewerPaused(textureIndex) ) {
        outArgs->params = boost::make_shared<UpdateViewerParams>();
        outArgs->params->isViewerPaused = true;
        outArgs->params->time = time;
        outArgs->params->setUniqueID(textureIndex);
        outArgs->params->textureIndex = textureIndex;
        outArgs->params->view = view;

        return eViewerRenderRetCodeRedraw;
    }

    int activeA, activeB;
    getActiveInputs(activeA, activeB);
    // Fetch the viewer indexes that we should render from the A or B input depending on the textureIndex parameter
    if (textureIndex == 0) {
        outArgs->activeInputIndex =  activeA;
    } else {
        if (_imp->uiContext->getCompositingOperator() == eViewerCompositingOperatorNone) {
            outArgs->activeInputIndex = -1;
        } else {
            outArgs->activeInputIndex =  activeB;
        }
    }


    // The active input providing the image is the first upstream non disabled node
    EffectInstancePtr upstreamInput = getInput(outArgs->activeInputIndex);
    outArgs->activeInputToRender.reset();
    if (upstreamInput) {
        outArgs->activeInputToRender = upstreamInput->getNearestNonDisabled();
    }

    // Before rendering we check that all mandatory inputs in the graph are connected else we fail
    if ( !outArgs->activeInputToRender || !checkTreeCanRender( outArgs->activeInputToRender->getNode().get() ) ) {
        return eViewerRenderRetCodeFail;
    }

    // Fetch the render parameters from the Viewer UI
    setupMinimalUpdateViewerParams(time, view, textureIndex, abortInfo, isSequential, outArgs);

    // Try to look-up the cache but do so only if we have a RoD valid in the cache because

    // we are on the main-thread here, it would be expensive to compute the RoD now.
    return getRoDAndLookupCache(true, viewerHash, rotoPaintNode, stats, outArgs);
}

ViewerInstance::ViewerRenderRetCode
ViewerInstance::renderViewer_internal(ViewIdx view,
                                      bool singleThreaded,
                                      bool isSequentialRender,
                                      U64 viewerHash,
                                      bool /*canAbort*/,
                                      const NodePtr& rotoPaintNode,
                                      bool useTLS,
                                      const ViewerCurrentFrameRequestSchedulerStartArgsPtr& request,
                                      const RenderStatsPtr& stats,
                                      ViewerArgs& inArgs)
{
    // We are in the render thread, we may not have computed the RoD and lookup the cache yet

    ViewerParallelRenderArgsSetterPtr frameArgs;

    if (useTLS) {
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
        frameArgs.reset( new ViewerParallelRenderArgsSetter(inArgs.params->time,
                                                            inArgs.params->view,
                                                            !isSequentialRender,
                                                            isSequentialRender,
                                                            inArgs.params->abortInfo,
                                                            getNode(),
                                                            inArgs.params->textureIndex,
                                                            getTimeline().get(),
                                                            false,
                                                            rotoPaintNode,
                                                            inArgs.activeInputToRender->getNode(),
                                                            inArgs.draftModeEnabled,
                                                            stats) );
#else
        frameArgs = boost::make_shared<ViewerParallelRenderArgsSetter>(inArgs.params->time,
                                                                       inArgs.params->view,
                                                                       !isSequentialRender,
                                                                       isSequentialRender,
                                                                       inArgs.params->abortInfo,
                                                                       getNode(),
                                                                       inArgs.params->textureIndex,
                                                                       getTimeline().get(),
                                                                       false,
                                                                       rotoPaintNode,
                                                                       inArgs.activeInputToRender->getNode(),
                                                                       inArgs.draftModeEnabled,
                                                                       stats);
#endif
    }


    if (inArgs.mustComputeRoDAndLookupCache) {
        // Since we will most likely need to compute the RoD now, we MUST setup the thread local
        // storage otherwise functions like EffectInstance::aborted() would not work.

        ViewerRenderRetCode retcode = getRoDAndLookupCache(false, viewerHash, rotoPaintNode, stats, &inArgs);
        if (retcode != eViewerRenderRetCodeRender) {
            return retcode;
        }


        if ( inArgs.params->nbCachedTile == (int)inArgs.params->tiles.size() ) {
            // Found a cached texture
            return eViewerRenderRetCodeRender;
        }
    }
    AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( QThread::currentThread() );
    if (isAbortable) {
        isAbortable->setAbortInfo( !isSequentialRender, inArgs.params->abortInfo, getNode()->getEffectInstance() );
    }


    assert( !inArgs.params->nbCachedTile || inArgs.params->nbCachedTile < (int)inArgs.params->tiles.size() );

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

    const bool useTextureCache = !inArgs.forceRender && !inArgs.userRoIEnabled && !inArgs.autoContrast && rotoPaintNode.get() == 0 && !inArgs.isDoingPartialUpdates;
    RectI roi = inArgs.params->roi;

    // We might already have some tiles cached, get their bounding box to see if it is less than the actual RoI
    if (useTextureCache) {
        RectI tilesBbox;
        bool tilesBboxSet = false;
        for (std::list<UpdateViewerParams::CachedTile>::const_iterator it = inArgs.params->tiles.begin(); it != inArgs.params->tiles.end(); ++it) {
            if (it->ramBuffer) {
                continue;
            }
            if (!tilesBboxSet) {
                tilesBboxSet = true;
                tilesBbox.x1 = it->rectRounded.x1;
                tilesBbox.x2 = it->rectRounded.x2;
                tilesBbox.y1 = it->rectRounded.y1;
                tilesBbox.y2 = it->rectRounded.y2;
            } else {
                tilesBbox.merge(it->rectRounded.x1, it->rectRounded.y1, it->rectRounded.x2, it->rectRounded.y2);
            }
        }
        if ( roi.contains(tilesBbox) ) {
            roi = tilesBbox;
        }
    }

    assert(inArgs.activeInputToRender);

    ///Check that we were not aborted already
    if ( !isSequentialRender && ( (inArgs.activeInputToRender->getHash() != inArgs.activeInputHash) ||
                                  ( inArgs.params->time != getTimeline()->currentFrame() ) ) ) {
        return eViewerRenderRetCodeRedraw;
    }

    ///Notify the gui we're rendering.
    EffectInstance::NotifyRenderingStarted_RAII renderingNotifier( getNode().get() );


    if (useTLS) {
        RectD canonicalRoi;
        roi.toCanonical(inArgs.params->mipMapLevel, inArgs.params->pixelAspectRatio, inArgs.params->rod, &canonicalRoi);

        FrameRequestMap requestPassData;
        StatusEnum stat = EffectInstance::computeRequestPass(inArgs.params->time, view, inArgs.params->mipMapLevel, canonicalRoi, getNode(), requestPassData);
        if (stat == eStatusFailed) {
            return eViewerRenderRetCodeFail;
        }


        frameArgs->updateNodesRequest(requestPassData);
    }

    const double par = inArgs.activeInputToRender->getAspectRatio(-1);

    // We are going to render a non cached frame and not in playback, clear persistent messages
    // (this code comes from https://github.com/NatronGitHub/Natron/commit/b0fcc946380571295b9230e9381218684474ca65)
    if (!inArgs.params->isSequential) {
        // Do not clear all previously set persistent message!
        // This also clears messages which do not come from the render action, such as messages set by InstanceChanged()!
        // Instead, the user can excplicitely clear the persistent messages using the refresh button, see ViewerTab::refresh(bool)
        //clearPersistentMessage(true);
    }

    ImagePlaneDesc components, pairedComponents;
    inArgs.activeInputToRender->getMetadataComponents(-1, &components, &pairedComponents);
    ImageBitDepthEnum imageDepth = inArgs.activeInputToRender->getBitDepth(-1);
    std::list<ImagePlaneDesc> requestedComponents;
    int alphaChannelIndex = -1;
    if ( (inArgs.channels != eDisplayChannelsA) &&
         ( inArgs.channels != eDisplayChannelsMatte) ) {
        ///We fetch the Layer specified in the gui
        if (inArgs.params->layer.getNumComponents() > 0) {
            requestedComponents.push_back(inArgs.params->layer);
        }
    } else {
        ///We fetch the alpha layer
        if ( !inArgs.params->alphaChannelName.empty() ) {
            requestedComponents.push_back(inArgs.params->alphaLayer);
            const std::vector<std::string>& channels = inArgs.params->alphaLayer.getChannels();
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
            if ( (inArgs.params->layer.getNumComponents() > 0) && (inArgs.params->layer != inArgs.params->alphaLayer) ) {
                requestedComponents.push_back(inArgs.params->layer);
            }
        }
    }

    if ( requestedComponents.empty() ) {
        return eViewerRenderRetCodeBlack;
    }

    EffectInstance::NotifyInputNRenderingStarted_RAII inputNIsRendering_RAII(getNode().get(), inArgs.activeInputIndex);
    std::vector<RectI> splitRoi;
    if (inArgs.isDoingPartialUpdates) {
        for (std::list<UpdateViewerParams::CachedTile>::iterator it = inArgs.params->tiles.begin(); it != inArgs.params->tiles.end(); ++it) {
            splitRoi.push_back(it->rect);
        }
    } else {
        /*
           Just render 1 tile
         */
        splitRoi.push_back(roi);
    }


    if ( stats && stats->isInDepthProfilingEnabled() ) {
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

#pragma message WARN("Implement Viewer so it accepts OpenGL Textures in input")
    BufferableObjectPtrList partialUpdateObjects;
    for (std::size_t rectIndex = 0; rectIndex < splitRoi.size(); ++rectIndex) {
        //AlphaImage will only be set when displaying the Matte overlay
        ImagePtr alphaImage, colorImage;

        // If an exception occurs here it is probably fatal, since
        // it comes from Natron itself. All exceptions from plugins are already caught
        // by the HostSupport library.
        // We catch it  and rethrow it just to notify the rendering is done.
        try {
            std::map<ImagePlaneDesc, ImagePtr> planes;
            EffectInstance::RenderRoIRetCode retCode;
            {
                boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs;
                renderArgs.reset( new EffectInstance::RenderRoIArgs(inArgs.params->time,
                                                                    Image::getScaleFromMipMapLevel(inArgs.params->mipMapLevel),
                                                                    inArgs.params->mipMapLevel,
                                                                    view,
                                                                    inArgs.forceRender,
                                                                    splitRoi[rectIndex],
                                                                    inArgs.params->rod,
                                                                    requestedComponents,
                                                                    imageDepth,
                                                                    false /*calledFromGetImage*/,
                                                                    this,
                                                                    eStorageModeRAM /*returnStorage*/,
                                                                    inArgs.params->time) );
                retCode = inArgs.activeInputToRender->renderRoI(*renderArgs, &planes);
            }
            //Either rendering failed or we have 2 planes (alpha mask and color image) or we have a single plane (color image)
            assert(planes.size() == 0 || planes.size() <= 2);
            if ( !planes.empty() && (retCode == EffectInstance::eRenderRoIRetCodeOk) ) {
                if (planes.size() == 2) {
                    std::map<ImagePlaneDesc, ImagePtr>::iterator foundColorLayer = planes.find(inArgs.params->layer);
                    if ( foundColorLayer != planes.end() ) {
                        colorImage = foundColorLayer->second;
                    }
                    std::map<ImagePlaneDesc, ImagePtr>::iterator foundAlphaLayer = planes.find(inArgs.params->alphaLayer);
                    if ( foundAlphaLayer != planes.end() ) {
                        alphaImage = foundAlphaLayer->second;
                    }
                } else {
                    //only 1 plane, figure out if the alpha layer is the same as the color layer
                    if (inArgs.params->alphaLayer == inArgs.params->layer) {
                        if (inArgs.channels == eDisplayChannelsMatte) {
                            alphaImage = colorImage = planes.begin()->second;
                        } else {
                            colorImage = planes.begin()->second;
                        }
                    } else {
                        colorImage = planes.begin()->second;
                    }
                }
                assert(colorImage);
                inArgs.params->colorImage = colorImage;
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
        } catch (...) {
            ///If the plug-in was aborted, this is probably not a failure due to render but because of abortion.
            ///Don't forward the exception in that case.
            if ( inArgs.activeInputToRender->aborted() ) {
                return eViewerRenderRetCodeRedraw;
            }
            throw;
        }


        // Don't uncomment: if the image was rendered so far, render it to the display texture so that the user get some feedback
        /*if ( !_imp->checkAgeNoUpdate( inArgs.params->textureIndex, inArgs.params->abortInfo->getRenderAge() ) ) {
            return eViewerRenderRetCodeRedraw;
        }

        if ( inArgs.activeInputToRender->aborted() ) {
            return eViewerRenderRetCodeRedraw;
        }*/

        const bool viewerRenderRoiOnly = !useTextureCache;
        ViewerColorSpaceEnum srcColorSpace = colorImage ? getApp()->getDefaultColorSpaceForBitDepth( colorImage->getBitDepth() ) : eViewerColorSpaceSRGB;

        if ( ( (inArgs.channels == eDisplayChannelsA) && ( !colorImage || (alphaChannelIndex < 0) || ( alphaChannelIndex >= (int)colorImage->getComponentsCount() ) ) ) ||
            ( ( inArgs.channels == eDisplayChannelsMatte) && ( !alphaImage || ( alphaChannelIndex < 0) || ( alphaChannelIndex >= (int)alphaImage->getComponentsCount() ) ) ) ) {
            return eViewerRenderRetCodeBlack;
        }


        assert( ( inArgs.channels != eDisplayChannelsMatte && ( !colorImage || alphaChannelIndex < (int)colorImage->getComponentsCount() ) ) ||
               ( inArgs.channels == eDisplayChannelsMatte && ( !alphaImage || alphaChannelIndex < (int)alphaImage->getComponentsCount() ) ) );


        //Make sure the viewer does not render something outside the bounds
        RectI viewerRenderRoI;
        if (colorImage) {
            splitRoi[rectIndex].intersect(colorImage->getBounds(), &viewerRenderRoI);
        }

        UpdateViewerParamsPtr updateParams;
        if (!inArgs.isDoingPartialUpdates) {
            updateParams = inArgs.params;
        } else {
            updateParams = boost::make_shared<UpdateViewerParams>(*inArgs.params);
            updateParams->mustFreeRamBuffer = true;
            updateParams->isPartialRect = true;
            UpdateViewerParams::CachedTile tile;
            tile.rect.set(viewerRenderRoI);
            tile.rectRounded = viewerRenderRoI;
            std::size_t pixelSize = 4;
            if (updateParams->depth == eImageBitDepthFloat) {
                pixelSize *= sizeof(float);
            }
            std::size_t dstRowSize = tile.rect.width() * pixelSize;
            tile.bytesCount = tile.rect.height() * dstRowSize;
            tile.ramBuffer =  (unsigned char*)malloc(tile.bytesCount);
            updateParams->tiles.clear();
            updateParams->tiles.push_back(tile);
        }

        // Allocate the texture on the CPU to apply the render viewer process
        FrameEntryLocker entryLocker( _imp.get() );
        RectI lastPaintBboxPixel;
        std::list<UpdateViewerParams::CachedTile> unCachedTiles;
        if (!useTextureCache) {
            assert(updateParams->tiles.size() == 1);

            UpdateViewerParams::CachedTile& tile = updateParams->tiles.front();


            // If we are tracking, only update the partial rectangles
            if (inArgs.isDoingPartialUpdates) {
                QMutexLocker k(&_imp->viewerParamsMutex);
                updateParams->recenterViewport = _imp->viewportCenterSet;
                updateParams->viewportCenter = _imp->viewportCenter;
                unCachedTiles.push_back(tile);
            }
            // If we are actively painting, re-use the last texture instead of re-drawing everything
            else if (rotoPaintNode) {
                /*
                   Check if we have a last valid texture and re-use the old texture only if the new texture is at least as big
                   as the old texture.
                 */
                QMutexLocker k(&_imp->lastRenderParamsMutex);
                assert(!_imp->lastRenderParams[updateParams->textureIndex] || _imp->lastRenderParams[updateParams->textureIndex]->tiles.size() == 1);


                bool canUseOldTex = _imp->lastRenderParams[updateParams->textureIndex] &&
                                    updateParams->mipMapLevel == _imp->lastRenderParams[updateParams->textureIndex]->mipMapLevel &&
                                    tile.rect.contains(_imp->lastRenderParams[updateParams->textureIndex]->tiles.front().rect);

                if (!canUseOldTex) {
                    //The old texture did not exist or was not usable, just make a new buffer
                    updateParams->mustFreeRamBuffer = true;
                    tile.ramBuffer =  (unsigned char*)malloc(tile.bytesCount);
                    unCachedTiles.push_back(tile);
                } else {
                    //Overwrite the RoI to only the last portion rendered
                    RectD lastPaintBbox = getApp()->getLastPaintStrokeBbox();


                    lastPaintBbox.toPixelEnclosing(updateParams->mipMapLevel, par, &lastPaintBboxPixel);


                    //The last buffer must be valid
                    const UpdateViewerParams::CachedTile& lastTile = _imp->lastRenderParams[updateParams->textureIndex]->tiles.front();
                    assert(lastTile.ramBuffer);
                    tile.ramBuffer =  0;

                    //If the old buffer size is the same as the requested texture size, this function does nothing
                    //otherwise it allocates a new buffer and copies the content of the old one.
                    bool mustFreeSource = copyAndSwap(lastTile.rect, tile.rect, tile.bytesCount, updateParams->depth, lastTile.ramBuffer, &tile.ramBuffer);

                    if (mustFreeSource) {
                        //A new buffer was allocated and copied the previous content, free the old texture
                        _imp->lastRenderParams[updateParams->textureIndex]->mustFreeRamBuffer = true;
                    } else {
                        //The buffer did not change its size, make sure to keep it
                        _imp->lastRenderParams[updateParams->textureIndex]->mustFreeRamBuffer = false;
                    }

                    //This will delete the previous buffer if mustFreeRamBuffer was set to true
                    _imp->lastRenderParams[updateParams->textureIndex].reset();

                    //Allocation may have failed
                    if (!tile.ramBuffer) {
                        return eViewerRenderRetCodeFail;
                    }

                    /*UpdateViewerParams::CachedTile tileCopy = tile;
                       //If we are painting, only render the portion needed
                       if ( !lastPaintBboxPixel.isNull() ) {
                        tileCopy.rect.intersect(lastPaintBboxPixel, &tileCopy.rect);
                        std::size_t pixelSize = 4;
                        if (updateParams->depth == eImageBitDepthFloat) {
                            pixelSize *= sizeof(float);
                        }
                        std::size_t dstRowSize = tileCopy.rect.width() * pixelSize;
                        tileCopy.bytesCount = tileCopy.rect.height() * dstRowSize;
                       }

                       unCachedTiles.push_back(tileCopy);*/
                    unCachedTiles.push_back(tile);
                }
                _imp->lastRenderParams[updateParams->textureIndex] = updateParams;
            }
            // Make a new buffer that will be freed when the params get destroyed
            else {
                assert(updateParams->tiles.size() == 1);

                updateParams->mustFreeRamBuffer = true;
                tile.ramBuffer =  (unsigned char*)malloc(tile.bytesCount);
                unCachedTiles.push_back(tile);
            }
        } else { // useTextureCache
            //Look up the cache for a texture or create one
            {
                QMutexLocker k(&_imp->lastRenderParamsMutex);
                _imp->lastRenderParams[updateParams->textureIndex].reset();
            }

            // For the viewer, we need the enclosing rectangle to avoid black borders.
            // Do this here to avoid infinity values.

            RectI bounds;
            updateParams->rod.toPixelEnclosing(updateParams->mipMapLevel, updateParams->pixelAspectRatio, &bounds);

            RectI tileBounds;
            tileBounds.x1 = tileBounds.y1 = 0;
            tileBounds.x2 = tileBounds.y2 = inArgs.params->tileSize;
            assert(!tileBounds.isNull());
            if (tileBounds.isNull()) {
                return eViewerRenderRetCodeRedraw;
            }
            std::string inputToRenderName = inArgs.activeInputToRender->getNode()->getScriptName_mt_safe();
            for (std::list<UpdateViewerParams::CachedTile>::iterator it = updateParams->tiles.begin(); it != updateParams->tiles.end(); ++it) {
                if (it->isCached) {
                    assert(it->ramBuffer);
                } else {
                    assert(!it->ramBuffer);


                    FrameKey key(getNode().get(),
                                 inArgs.params->time,
                                 viewerHash,
                                 inArgs.params->gain,
                                 inArgs.params->gamma,
                                 inArgs.params->lut,
                                 (int)inArgs.params->depth,
                                 inArgs.channels,
                                 inArgs.params->view,
                                 it->rect,
                                 inArgs.params->mipMapLevel,
                                 inputToRenderName,
                                 inArgs.params->layer,
                                 inArgs.params->alphaLayer.getPlaneID() + inArgs.params->alphaChannelName,
                                 inArgs.params->depth == eImageBitDepthFloat,
                                 inArgs.draftModeEnabled);



                    FrameParamsPtr cachedFrameParams( new FrameParams(bounds , key.getBitDepth(), tileBounds, ImagePtr() ) );
                    bool cached = appPTR->getTextureOrCreate(key, cachedFrameParams, &entryLocker, &it->cachedData);
                    if (!it->cachedData) {
                        std::size_t size = cachedFrameParams->getStorageInfo().numComponents * cachedFrameParams->getStorageInfo().dataTypeSize * cachedFrameParams->getStorageInfo().bounds.area();
                        QString s = tr("Failed to allocate a texture of %1.").arg( printAsRAM(size) );
                        Dialogs::errorDialog( tr("Out of memory").toStdString(), s.toStdString() );

                        return eViewerRenderRetCodeFail;
                    }

                    ///The entry has already been locked by the cache
                    if (!cached) {
                        it->cachedData->allocateMemory();
                    } else {
                        // If the tile is cached and we got it that means rendering is done
                        entryLocker.lock(it->cachedData);
                        it->ramBuffer = it->cachedData->data();
                        it->isCached = true;
                        continue;
                    }


                    assert(it->cachedData);
                    // how do you make sure cachedFrame->data() is not freed after this line?
                    ///It is not freed as long as the cachedFrame shared_ptr has a used_count greater than 1.
                    ///Since it is used during the whole function scope it is guaranteed not to be freed before
                    ///The viewer is actually done with it.
                    /// @see Cache::clearInMemoryPortion and Cache::clearDiskPortion and LRUHashTable::evict
                    it->ramBuffer = it->cachedData->data();
                    assert(it->ramBuffer);
                    unCachedTiles.push_back(*it);
                } // !it->isCached

                if (it->cachedData) {
                    it->cachedData->setInternalImage(colorImage);
                }
            }
        } // !useTextureCache

        /*
           If we are not using the texture cache, there is a unique tile which has the required texture size on the viewer, so we render the RoI instead
           of just the tile portion in the render function
         */


        //If we are painting, only render the portion needed
        if ( !lastPaintBboxPixel.isNull() ) {
            lastPaintBboxPixel.intersect(viewerRenderRoI, &viewerRenderRoI);
        }

        TimeLapsePtr viewerRenderTimeRecorder;
        if (stats) {
            viewerRenderTimeRecorder = boost::make_shared<TimeLapse>();
        }

        std::size_t tileRowElements = inArgs.params->tileSize;
        // Internally the buffer is interpreted as U32 when 8bit, so we do not multiply it by 4 for RGBA
        if (updateParams->depth == eImageBitDepthFloat) {
            tileRowElements *= 4;
        }

        if (singleThreaded) {
            if (inArgs.autoContrast && !inArgs.isDoingPartialUpdates) {
                double vmin, vmax;
                MinMaxVal vMinMax = findAutoContrastVminVmax(colorImage, inArgs.channels, viewerRenderRoI);
                vmin = vMinMax.min;
                vmax = vMinMax.max;

                ///if vmax - vmin is greater than 1 the gain will be really small and we won't see
                ///anything in the image
                if (vmin == vmax) {
                    vmin = vmax - 1.;
                }
                updateParams->gain = 1 / (vmax - vmin);
                updateParams->offset = -vmin / ( vmax - vmin);
            }

            const RenderViewerArgs args(colorImage,
                                        alphaImage,
                                        inArgs.channels,
                                        updateParams->srcPremult,
                                        updateParams->depth,
                                        updateParams->gain,
                                        updateParams->gamma,
                                        updateParams->offset,
                                        lutFromColorspace(srcColorSpace),
                                        lutFromColorspace(updateParams->lut),
                                        alphaChannelIndex,
                                        viewerRenderRoiOnly,
                                        tileRowElements);
            QReadLocker k(&_imp->gammaLookupMutex);
            for (std::list<UpdateViewerParams::CachedTile>::iterator it = unCachedTiles.begin(); it != unCachedTiles.end(); ++it) {
                renderFunctor(viewerRenderRoI,
                              args,
                              this,
                              *it);
            }
        } else {
            bool runInCurrentThread = QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount();
            if ( !runInCurrentThread && (splitRoi.size() > 1) ) {
                runInCurrentThread = true;
            }


            ///if autoContrast is enabled, find out the vmin/vmax before rendering and mapping against new values
            if (inArgs.autoContrast && !inArgs.isDoingPartialUpdates) {
                double vmin = std::numeric_limits<double>::infinity();
                double vmax = -std::numeric_limits<double>::infinity();

                if (runInCurrentThread) {
                    MinMaxVal vMinMax = findAutoContrastVminVmax(colorImage, inArgs.channels, viewerRenderRoI);
                    vmin = vMinMax.min;
                    vmax = vMinMax.max;
                } else {
                    std::vector<RectI> splitRects = viewerRenderRoI.splitIntoSmallerRects( appPTR->getMaxThreadCount() );
                    QFuture<MinMaxVal> future = QtConcurrent::mapped( splitRects,
                                                                                       boost::bind(findAutoContrastVminVmax,
                                                                                                   colorImage,
                                                                                                   inArgs.channels,
                                                                                                   _1) );
                    future.waitForFinished();
                    QList<MinMaxVal> results = future.results();
                    Q_FOREACH (const MinMaxVal &vMinMax, results) {
                        if (vMinMax.min < vmin) {
                            vmin = vMinMax.min;
                        }
                        if (vMinMax.max > vmax) {
                            vmax = vMinMax.max;
                        }
                    }
                } // runInCurrentThread

                if (vmax == vmin) {
                    vmin = vmax - 1.;
                }

                if (vmax <= 0) {
                    updateParams->gain = 0;
                    updateParams->offset = 0;
                } else {
                    updateParams->gain = 1 / (vmax - vmin);
                    updateParams->offset =  -vmin / (vmax - vmin);
                }
            }

            const RenderViewerArgs args(colorImage,
                                        alphaImage,
                                        inArgs.channels,
                                        updateParams->srcPremult,
                                        updateParams->depth,
                                        updateParams->gain,
                                        updateParams->gamma,
                                        updateParams->offset,
                                        lutFromColorspace(srcColorSpace),
                                        lutFromColorspace(updateParams->lut),
                                        alphaChannelIndex,
                                        viewerRenderRoiOnly,
                                        tileRowElements);

            if (runInCurrentThread) {
                QReadLocker k(&_imp->gammaLookupMutex);
                for (std::list<UpdateViewerParams::CachedTile>::iterator it = unCachedTiles.begin(); it != unCachedTiles.end(); ++it) {
                    renderFunctor(viewerRenderRoI,
                                  args, this, *it);
                }
            } else {
                QReadLocker k(&_imp->gammaLookupMutex);
                QtConcurrent::map( unCachedTiles,
                                   boost::bind(&renderFunctor,
                                               viewerRenderRoI,
                                               args,
                                               this,
                                               _1) ).waitForFinished();
            }

            if (inArgs.isDoingPartialUpdates) {
                partialUpdateObjects.push_back(updateParams);
            }
        } // if (singleThreaded)


        if ( colorImage && stats && stats->isInDepthProfilingEnabled() ) {
            stats->addRenderInfosForNode( getNode(), NodePtr(), colorImage->getComponents().getChannelsLabel(), viewerRenderRoI, viewerRenderTimeRecorder->getTimeSinceCreation() );
        }
    } // for (std::vector<RectI>::iterator rect = splitRoi.begin(); rect != splitRoi.end(), ++rect) {


    /*
       If we were rendering only partial rectangles, update them all at once
     */
    if ( !partialUpdateObjects.empty() ) {
        getRenderEngine()->notifyFrameProduced(partialUpdateObjects, stats, request);

        //If we reply eViewerRenderRetCodeRender, it will append another updateViewer for nothing
        return eViewerRenderRetCodeRedraw;
    }

    return eViewerRenderRetCodeRender;
} // renderViewer_internal

void
ViewerInstance::aboutToUpdateTextures()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->uiContext->clearPartialUpdateTextures();
}

bool
ViewerInstance::isViewerUIVisible() const
{
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->uiContext ? _imp->uiContext->isViewerUIVisible() : false;
}

void
ViewerInstance::updateViewer(UpdateViewerParamsPtr & frame)
{
    if (!frame) {
        return;
    }
    if (frame->isViewerPaused) {
        return;
    }
    if ( isViewerPaused(frame->textureIndex) ) {
        return;
    }
    _imp->updateViewer( boost::dynamic_pointer_cast<UpdateViewerParams>(frame) );
}

void
renderFunctor(const RectI& roi,
              const RenderViewerArgs & args,
              ViewerInstance* viewer,
              UpdateViewerParams::CachedTile tile)
{
    if ( (args.bitDepth == eImageBitDepthFloat) ) {
        // image is stored as linear, the OpenGL shader with do gamma/sRGB/Rec709 decompression, as well as gain and offset
        scaleToTexture32bits(roi, args, tile, (float*)tile.ramBuffer);
    } else {
        // texture is stored as sRGB/Rec709 compressed 8-bit RGBA
        scaleToTexture8bits(roi, args, viewer, tile, (U32*)tile.ramBuffer);
    }
}

inline
MinMaxVal
findAutoContrastVminVmax_generic(const ImagePtr inputImage,
                                 int nComps,
                                 DisplayChannelsEnum channels,
                                 const RectI & rect)
{
    double localVmin = std::numeric_limits<double>::infinity();
    double localVmax = -std::numeric_limits<double>::infinity();
    Image::ReadAccess acc = inputImage->getReadRights();

    for (int y = rect.bottom(); y < rect.top(); ++y) {
        const float* src_pixels = (const float*)acc.pixelAt(rect.left(), y);
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
                mini = std::min(std::min(r, g), b);
                maxi = std::max(std::max(r, g), b);
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

    return MinMaxVal(localVmin, localVmax);
} // findAutoContrastVminVmax_generic

template <int nComps>
MinMaxVal
findAutoContrastVminVmax_internal(const ImagePtr inputImage,
                                  DisplayChannelsEnum channels,
                                  const RectI & rect)
{
    return findAutoContrastVminVmax_generic(inputImage, nComps, channels, rect);
}

MinMaxVal
findAutoContrastVminVmax(const ImagePtr inputImage,
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

template <typename PIX, int maxValue, bool opaque, bool applyMatte, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bits_generic(const RectI& roi,
                            const RenderViewerArgs & args,
                            int nComps,
                            ViewerInstance* viewer,
                            const UpdateViewerParams::CachedTile& tile,
                            U32* tileBuffer)
{
    const size_t pixelSize = sizeof(PIX);
    const bool luminance = (args.channels == eDisplayChannelsY);
    Image::ReadAccess acc = Image::ReadAccess( args.inputImage.get() );
    const RectI srcImgBounds = args.inputImage->getBounds();

    if ( (args.renderOnlyRoI && !tile.rect.contains(roi)) || (!args.renderOnlyRoI && !roi.contains(tile.rect)) ) {
        return;
    }
    assert(tile.rect.x2 > tile.rect.x1);

    int dstRowElements;
    U32* dst_pixels;
    if (args.renderOnlyRoI) {
        dstRowElements = tile.rect.width();
        dst_pixels = tileBuffer + (roi.y1 - tile.rect.y1) * dstRowElements + (roi.x1 - tile.rect.x1);
    } else {
        dstRowElements = args.tileRowElements;
        dst_pixels = tileBuffer + (tile.rect.y1 - tile.rectRounded.y1) * args.tileRowElements + (tile.rect.x1 - tile.rectRounded.x1);
    }

    const int y1 = args.renderOnlyRoI ? roi.y1 : tile.rect.y1;
    const int y2 = args.renderOnlyRoI ? roi.y2 : tile.rect.y2;
    const int x1 = args.renderOnlyRoI ? roi.x1 : tile.rect.x1;
    const int x2 = args.renderOnlyRoI ? roi.x2 : tile.rect.x2;
    const PIX* src_pixels = (const PIX*)acc.pixelAt(x1, y1);
    const int srcRowElements = (int)args.inputImage->getRowElements();
    Image::ReadAccessPtr matteAcc;
    if (applyMatte) {
        matteAcc = boost::make_shared<Image::ReadAccess>( args.matteImage.get() );
    }

    for (int y = y1; y < y2;
         ++y,
         dst_pixels += dstRowElements) {
        // coverity[dont_call]
        int start = (int)( rand() % (x2 - x1) );


        for (int backward = 0; backward < 2; ++backward) {
            int index = backward ? start - 1 : start;

            assert( backward == 1 || ( index >= 0 && index < (x2 - x1) ) );

            unsigned error_r = 0x80;
            unsigned error_g = 0x80;
            unsigned error_b = 0x80;

            while (index < (x2 - x1) && index >= 0) {
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


                switch (pixelSize) {
                case sizeof(unsigned char):     //byte
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
                case sizeof(unsigned short):     //short
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
                case sizeof(float):     //float
                    if (args.srcColorSpace) {
                        r = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(r);
                        g = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(g);
                        b = args.srcColorSpace->fromColorSpaceFloatToLinearFloat(b);
                    }
                    break;
                default:
                    break;
                }

                r = r * args.gain + args.offset;
                g = g * args.gain + args.offset;
                b = b * args.gain + args.offset;
                if  (args.gamma <= 0) {
                    r = (r < 1.) ? 0. : (r == 1. ? 1. : std::numeric_limits<double>::infinity() );
                    g = (g < 1.) ? 0. : (g == 1. ? 1. : std::numeric_limits<double>::infinity() );
                    b = (b < 1.) ? 0. : (b == 1. ? 1. : std::numeric_limits<double>::infinity() );
                } else if (args.gamma != 1.) {
                    r = viewer->interpolateGammaLut(r);
                    g = viewer->interpolateGammaLut(g);
                    b = viewer->interpolateGammaLut(b);
                }


                if (luminance) {
                    r = 0.299 * r + 0.587 * g + 0.114 * b;
                    g = r;
                    b = r;
                }


                U8 uR, uG, uB;
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
                        const PIX* src_pixels = (const PIX*)matteAcc->pixelAt(x1 + index, y);
                        if (src_pixels) {
                            alphaMatteValue = (double)src_pixels[args.alphaChannelIndex];
                            switch (pixelSize) {
                            case sizeof(unsigned char):     //byte
                                alphaMatteValue = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                                break;
                            case sizeof(unsigned short):     //short
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
                    uR = Image::clampIfInt<U8>( (double)uR + matteA );
                }

                dst_pixels[index] = toBGRA(uR, uG, uB, uA);

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

template <typename PIX, int maxValue, int nComps, bool opaque, bool matteOverlay, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bits_internal(const RectI& roi,
                             const RenderViewerArgs & args,
                             ViewerInstance* viewer,
                             const UpdateViewerParams::CachedTile& tile,
                             U32* output)
{
    scaleToTexture8bits_generic<PIX, maxValue, opaque, matteOverlay, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, tile, output);
}

template <typename PIX, int maxValue, int nComps, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bitsForMatte(const RectI& roi,
                            const RenderViewerArgs & args,
                            ViewerInstance* viewer,
                            const UpdateViewerParams::CachedTile& tile,
                            U32* output)
{
    bool applyMate = args.matteImage.get() && args.alphaChannelIndex >= 0;

    if (applyMate) {
        scaleToTexture8bits_internal<PIX, maxValue, nComps, opaque, true, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
    } else {
        scaleToTexture8bits_internal<PIX, maxValue, nComps, opaque, false, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
    }
}

template <typename PIX, int maxValue, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture8bitsForDepthForComponents(const RectI& roi,
                                         const RenderViewerArgs & args,
                                         ViewerInstance* viewer,
                                         const UpdateViewerParams::CachedTile& tile,
                                         U32* output)
{
    int nComps = args.inputImage->getComponents().getNumComponents();

    switch (nComps) {
    case 4:
        scaleToTexture8bitsForMatte<PIX, maxValue, 4, opaque, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
        break;
    case 3:
        scaleToTexture8bitsForMatte<PIX, maxValue, 3, opaque, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
        break;
    case 2:
        scaleToTexture8bitsForMatte<PIX, maxValue, 2, opaque, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
        break;
    case 1:
        scaleToTexture8bitsForMatte<PIX, maxValue, 1, opaque, rOffset, gOffset, bOffset>(roi, args, viewer, tile, output);
        break;
    default:
        bool applyMate = args.matteImage.get() && args.alphaChannelIndex >= 0;
        if (applyMate) {
            scaleToTexture8bits_generic<PIX, maxValue, opaque, true, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, tile, output);
        } else {
            scaleToTexture8bits_generic<PIX, maxValue, opaque, false, rOffset, gOffset, bOffset>(roi, args, nComps, viewer, tile, output);
        }
        break;
    }
}

template <typename PIX, int maxValue, bool opaque>
void
scaleToTexture8bitsForPremult(const RectI& roi,
                              const RenderViewerArgs & args,
                              ViewerInstance* viewer,
                              const UpdateViewerParams::CachedTile& tile,
                              U32* output)
{
    switch (args.channels) {
    case eDisplayChannelsRGB:
    case eDisplayChannelsY:
    case eDisplayChannelsMatte:

        scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 1, 2>(roi, args, viewer, tile, output);
        break;
    case eDisplayChannelsG:
        scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, viewer, tile, output);
        break;
    case eDisplayChannelsB:
        scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, viewer, tile, output);
        break;
    case eDisplayChannelsA:
        switch (args.alphaChannelIndex) {
        case -1:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, viewer, tile, output);
            break;
        case 0:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, viewer, tile, output);
            break;
        case 1:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, viewer, tile, output);
            break;
        case 2:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, viewer, tile, output);
            break;
        case 3:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, viewer, tile, output);
            break;
        default:
            scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, viewer, tile, output);
        }

        break;
    case eDisplayChannelsR:
    default:
        scaleToTexture8bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, viewer, tile, output);

        break;
    }
}

template <typename PIX, int maxValue>
void
scaleToTexture8bitsForDepth(const RectI& roi,
                            const RenderViewerArgs & args,
                            ViewerInstance* viewer,
                            const UpdateViewerParams::CachedTile& tile,
                            U32* output)
{
    switch (args.srcPremult) {
    case eImagePremultiplicationOpaque:
        scaleToTexture8bitsForPremult<PIX, maxValue, true>(roi, args, viewer, tile, output);
        break;
    case eImagePremultiplicationPremultiplied:
    case eImagePremultiplicationUnPremultiplied:
    default:
        scaleToTexture8bitsForPremult<PIX, maxValue, false>(roi, args, viewer, tile, output);
        break;
    }
}

void
scaleToTexture8bits(const RectI& roi,
                    const RenderViewerArgs & args,
                    ViewerInstance* viewer,
                    const UpdateViewerParams::CachedTile& tile,
                    U32* output)
{
    assert(output);
    switch ( args.inputImage->getBitDepth() ) {
    case eImageBitDepthFloat:
        scaleToTexture8bitsForDepth<float, 1>(roi, args, viewer, tile, output);
        break;
    case eImageBitDepthByte:
        scaleToTexture8bitsForDepth<unsigned char, 255>(roi, args, viewer, tile, output);
        break;
    case eImageBitDepthShort:
        scaleToTexture8bitsForDepth<unsigned short, 65535>(roi, args, viewer, tile, output);
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
ViewerInstance::markAllOnGoingRendersAsAborted(bool keepOldestRender)
{
    //Do not abort the oldest render while scrubbing timeline or sliders so that the user gets some feedback
    bool keepOldest = getApp()->isDraftRenderEnabled() || isDoingPartialUpdates() || keepOldestRender;
    QMutexLocker k(&_imp->renderAgeMutex);

    for (int i = 0; i < 2; ++i) {
        if ( _imp->currentRenderAges[i].empty() ) {
            continue;
        }

        //Do not abort the oldest render, let it finish
        OnGoingRenders::iterator it = _imp->currentRenderAges[i].begin();
        if (keepOldest) {
            ++it;
        }

        for (; it != _imp->currentRenderAges[i].end(); ++it) {
            (*it)->setAborted();
        }
    }
}

template <typename PIX, int maxValue, bool opaque, bool applyMatte, int rOffset, int gOffset, int bOffset>
void
scaleToTexture32bitsGeneric(const RectI& roi,
                            const RenderViewerArgs & args,
                            int nComps,
                            const UpdateViewerParams::CachedTile& tile,
                            float *tileBuffer)
{
    const size_t pixelSize = sizeof(PIX);
    const bool luminance = (args.channels == eDisplayChannelsY);
    const int dstRowElements = args.renderOnlyRoI ? tile.rect.width() * 4 : args.tileRowElements;
    Image::ReadAccess acc = Image::ReadAccess( args.inputImage.get() );
    Image::ReadAccessPtr matteAcc;

    if (applyMatte) {
        matteAcc = boost::make_shared<Image::ReadAccess>( args.matteImage.get() );
    }

    assert( (args.renderOnlyRoI && roi.x1 >= tile.rect.x1 && roi.x2 <= tile.rect.x2 && roi.y1 >= tile.rect.y1 && roi.y2 <= tile.rect.y2) || (!args.renderOnlyRoI && tile.rect.x1 >= roi.x1 && tile.rect.x2 <= roi.x2 && tile.rect.y1 >= roi.y1 && tile.rect.y2 <= roi.y2) );
    assert(tile.rect.x2 > tile.rect.x1);

    float* dst_pixels;
    if (args.renderOnlyRoI) {
        dst_pixels = tileBuffer + (roi.y1 - tile.rect.y1) * dstRowElements + (roi.x1 - tile.rect.x1) * 4;
    } else {
        dst_pixels = tileBuffer + (tile.rect.y1 - tile.rectRounded.y1) * dstRowElements + (tile.rect.x1 - tile.rectRounded.x1) * 4;
    }

    const int y1 = args.renderOnlyRoI ? roi.y1 : tile.rect.y1;
    const int y2 = args.renderOnlyRoI ? roi.y2 : tile.rect.y2;
    const int x1 = args.renderOnlyRoI ? roi.x1 : tile.rect.x1;
    const int x2 = args.renderOnlyRoI ? roi.x2 : tile.rect.x2;
    const float* src_pixels = (const float*)acc.pixelAt(x1, y1);
    const int srcRowElements = (const int)args.inputImage->getRowElements();

    for (int y = y1; y < y2;
         ++y,
         dst_pixels += dstRowElements) {
        for (int x = 0; x < (x2 - x1);
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


            switch (pixelSize) {
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
                        switch (pixelSize) {
                        case sizeof(unsigned char):     //byte
                            alphaMatteValue = (double)Image::convertPixelDepth<unsigned char, float>( (unsigned char)r );
                            break;
                        case sizeof(unsigned short):     //short
                            alphaMatteValue = (double)Image::convertPixelDepth<unsigned short, float>( (unsigned short)r );
                            break;
                        default:
                            break;
                        }
                    }
                }
                r += alphaMatteValue * 0.5;
            }


            dst_pixels[x * 4] = r; // do not clamp! values may be more than 1 or less than 0
            dst_pixels[x * 4 + 1] = g;
            dst_pixels[x * 4 + 2] = b;
            dst_pixels[x * 4 + 3] = a;
        }
        if (src_pixels) {
            src_pixels += srcRowElements;
        }
    }
} // scaleToTexture32bitsGeneric

template <typename PIX, int maxValue, int nComps, bool opaque, bool applyMatte, int rOffset, int gOffset, int bOffset>
void
scaleToTexture32bitsInternal(const RectI& roi,
                             const RenderViewerArgs & args,
                             const UpdateViewerParams::CachedTile& tile,
                             float *output)
{
    scaleToTexture32bitsGeneric<PIX, maxValue, opaque, applyMatte, rOffset, gOffset, bOffset>(roi, args, nComps, tile, output);
}

template <typename PIX, int maxValue, int nComps, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture32bitsForMatte(const RectI& roi,
                             const RenderViewerArgs & args,
                             const UpdateViewerParams::CachedTile& tile,
                             float *output)
{
    bool applyMatte = args.matteImage.get() && args.alphaChannelIndex >= 0;

    if (applyMatte) {
        scaleToTexture32bitsInternal<PIX, maxValue, nComps, opaque, true, rOffset, gOffset, bOffset>(roi, args, tile, output);
    } else {
        scaleToTexture32bitsInternal<PIX, maxValue, nComps, opaque, false, rOffset, gOffset, bOffset>(roi, args, tile, output);
    }
}

template <typename PIX, int maxValue, bool opaque, int rOffset, int gOffset, int bOffset>
void
scaleToTexture32bitsForDepthForComponents(const RectI& roi,
                                          const RenderViewerArgs & args,
                                          const UpdateViewerParams::CachedTile& tile,
                                          float *output)
{
    int nComps = args.inputImage->getComponents().getNumComponents();

    switch (nComps) {
    case 4:
        scaleToTexture32bitsForMatte<PIX, maxValue, 4, opaque, rOffset, gOffset, bOffset>(roi, args, tile, output);
        break;
    case 3:
        scaleToTexture32bitsForMatte<PIX, maxValue, 3, opaque, rOffset, gOffset, bOffset>(roi, args, tile, output);
        break;
    case 2:
        scaleToTexture32bitsForMatte<PIX, maxValue, 2, opaque, rOffset, gOffset, bOffset>(roi, args, tile, output);
        break;
    case 1:
        scaleToTexture32bitsForMatte<PIX, maxValue, 1, opaque, rOffset, gOffset, bOffset>(roi, args, tile, output);
        break;
    default:
        bool applyMatte = args.matteImage.get() && args.alphaChannelIndex >= 0;
        if (applyMatte) {
            scaleToTexture32bitsGeneric<PIX, maxValue, opaque, true, rOffset, gOffset, bOffset>(roi, args, nComps, tile, output);
        } else {
            scaleToTexture32bitsGeneric<PIX, maxValue, opaque, false, rOffset, gOffset, bOffset>(roi, args, nComps, tile, output);
        }
        break;
    }
}

template <typename PIX, int maxValue, bool opaque>
void
scaleToTexture32bitsForPremultForComponents(const RectI& roi,
                                            const RenderViewerArgs & args,
                                            const UpdateViewerParams::CachedTile& tile,
                                            float *output)
{
    switch (args.channels) {
    case eDisplayChannelsRGB:
    case eDisplayChannelsY:
    case eDisplayChannelsMatte:
        scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 1, 2>(roi, args, tile, output);
        break;
    case eDisplayChannelsG:
        scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, tile, output);
        break;
    case eDisplayChannelsB:
        scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, tile, output);
        break;
    case eDisplayChannelsA:
        switch (args.alphaChannelIndex) {
        case -1:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, tile, output);
            break;
        case 0:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, tile, output);
            break;
        case 1:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 1, 1, 1>(roi, args, tile, output);
            break;
        case 2:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 2, 2, 2>(roi, args, tile, output);
            break;
        case 3:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, tile, output);
            break;
        default:
            scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 3, 3, 3>(roi, args, tile, output);
            break;
        }

        break;
    case eDisplayChannelsR:
    default:
        scaleToTexture32bitsForDepthForComponents<PIX, maxValue, opaque, 0, 0, 0>(roi, args, tile, output);
        break;
    }
}

template <typename PIX, int maxValue>
void
scaleToTexture32bitsForPremult(const RectI& roi,
                               const RenderViewerArgs & args,
                               const UpdateViewerParams::CachedTile& tile,
                               float *output)
{
    switch (args.srcPremult) {
    case eImagePremultiplicationOpaque:
        scaleToTexture32bitsForPremultForComponents<PIX, maxValue, true>(roi, args, tile, output);
        break;
    case eImagePremultiplicationPremultiplied:
    case eImagePremultiplicationUnPremultiplied:
    default:
        scaleToTexture32bitsForPremultForComponents<PIX, maxValue, false>(roi, args, tile, output);
        break;
    }
}

void
scaleToTexture32bits(const RectI& roi,
                     const RenderViewerArgs & args,
                     const UpdateViewerParams::CachedTile& tile,
                     float *output)
{
    assert(output);

    switch ( args.inputImage->getBitDepth() ) {
    case eImageBitDepthFloat:
        scaleToTexture32bitsForPremult<float, 1>(roi, args, tile, output);
        break;
    case eImageBitDepthByte:
        scaleToTexture32bitsForPremult<unsigned char, 255>(roi, args, tile, output);
        break;
    case eImageBitDepthShort:
        scaleToTexture32bitsForPremult<unsigned short, 65535>(roi, args, tile, output);
        break;
    case eImageBitDepthHalf:
        assert(false);
        break;
    case eImageBitDepthNone:
        break;
    }
} // scaleToTexture32bits

void
ViewerInstance::ViewerInstancePrivate::updateViewer(UpdateViewerParamsPtr params)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // QMutexLocker locker(&updateViewerMutex);
    // if (updateViewerRunning) {
    uiContext->makeOpenGLcontextCurrent();
    if (params->tiles.empty()) {
        return;
    }
    bool doUpdate = true;


    bool isImageUpToDate = checkAndUpdateDisplayAge( params->textureIndex, params->abortInfo->getRenderAge() );
    (void)isImageUpToDate;

    // Don't uncomment: if the image was rendered so far, render it to the display texture so that the user get some feedback
   /* if ( !params->isPartialRect && !params->isSequential && !isImageUpToDate) {
        doUpdate = false;
    }*/
    if (doUpdate) {
        /*RectI bounds;
           params->rod.toPixelEnclosing(params->mipMapLevel, params->pixelAspectRatio, &bounds);*/

        assert( (params->isPartialRect && params->tiles.size() == 1) || !params->isPartialRect );

        TexturePtr texture;
        bool isFirstTile = true;
        for (std::list<UpdateViewerParams::CachedTile>::iterator it = params->tiles.begin(); it != params->tiles.end(); ++it) {
            if (!it->ramBuffer) {
                continue;
            }

            // For cached tiles, some tiles might not have the standard tile, (i.e: the last tile column/row).
            // Since the internal buffer is rounded to the tile size anyway we want the glTexSubImage2D call to ensure
            // that we upload the full internal buffer
            TextureRect texRect;
            texRect.par = it->rect.par;
            texRect.closestPo2 = it->rect.closestPo2;
            texRect.set(it->rectRounded);
    
            assert(params->roi.contains(texRect));
            uiContext->transferBufferFromRAMtoGPU(it->ramBuffer, it->bytesCount, params->roi, params->roiNotRoundedToTileSize, texRect, params->textureIndex, params->isPartialRect, isFirstTile, &texture);
            isFirstTile = false;
        }


        NodePtr rotoPaintNode;
        RotoStrokeItemPtr curStroke;
        bool isDrawing = false;
        instance->getApp()->getActiveRotoDrawingStroke(&rotoPaintNode, &curStroke, &isDrawing);

        const UpdateViewerParams::CachedTile& firstTile = params->tiles.front();
        ImagePtr originalImage;
        originalImage = params->colorImage;
        if (firstTile.cachedData && !originalImage) {
            originalImage = firstTile.cachedData->getInternalImage();
        }
        ImageBitDepthEnum depth;
        if (originalImage) {
            depth = originalImage->getBitDepth();
        } else {
            assert(firstTile.cachedData);
            if (firstTile.cachedData) {
                depth = (ImageBitDepthEnum)firstTile.cachedData->getKey().getBitDepth();
            } else {
                depth = eImageBitDepthByte;
            }
        }

        uiContext->endTransferBufferFromRAMToGPU(params->textureIndex, texture, originalImage, params->time, params->rod,  params->pixelAspectRatio, depth, params->mipMapLevel, params->srcPremult, params->gain, params->gamma, params->offset, params->lut, params->recenterViewport, params->viewportCenter, params->isPartialRect);

        if (!isDrawing) {
            uiContext->updateColorPicker(params->textureIndex);
        }
    }

    //
    //        updateViewerRunning = false;
    //    }
    //    updateViewerCond.wakeOne();
} // ViewerInstance::ViewerInstancePrivate::updateViewer

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
        _imp->fillGammaLut(value);
    }
    assert(_imp->uiContext);
    if (changed) {
        if ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte)
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
        if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) )
             && !getApp()->getProject()->isLoadingProject() ) {
            renderCurrentFrame(true);
        } else {
            _imp->uiContext->redraw();
        }
    }
}

unsigned int
ViewerInstance::getViewerMipMapLevel() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerMipMapLevel;
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
    if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) )
         && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    } else {
        _imp->uiContext->redraw();
    }
}

void
ViewerInstance::setDisplayChannels(DisplayChannelsEnum channels,
                                   bool bothInputs)
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
ViewerInstance::setActiveLayer(const ImagePlaneDesc& layer,
                               bool doRender)
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
ViewerInstance::setAlphaChannel(const ImagePlaneDesc& layer,
                                const std::string& channelName,
                                bool doRender)
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
ViewerInstance::disconnectTexture(int index,bool clearRod)
{
    if (_imp->uiContext) {
        Q_EMIT disconnectTextureRequest(index, clearRod);
    }
}

void
ViewerInstance::redrawViewer()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!_imp->uiContext) {
        return;
    }
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
ViewerInstance::setFullFrameProcessingEnabled(bool fullFrame)
{
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->fullFrameProcessingEnabled == fullFrame) {
            return;
        }
        _imp->fullFrameProcessingEnabled = fullFrame;
    }
    if ( !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

bool
ViewerInstance::isFullFrameProcessingEnabled() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->fullFrameProcessingEnabled;
}

void
ViewerInstance::addAcceptedComponents(int /*inputNb*/,
                                      std::list<ImagePlaneDesc>* comps)
{
    ///Viewer only supports RGBA for now.
    comps->push_back( ImagePlaneDesc::getRGBAComponents() );
    comps->push_back( ImagePlaneDesc::getRGBComponents() );
    comps->push_back( ImagePlaneDesc::getAlphaComponents() );
}

ViewIdx
ViewerInstance::getViewerCurrentView() const
{
    return _imp->uiContext ? _imp->uiContext->getCurrentView() : ViewIdx(0);
}

void
ViewerInstance::setActivateInputChangeRequestedFromViewer(bool fromViewer)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->activateInputChangedFromViewer = fromViewer;
}

bool
ViewerInstance::isInputChangeRequestedFromViewer() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->activateInputChangedFromViewer;
}

void
ViewerInstance::setViewerPaused(bool paused,
                                bool allInputs)
{
    QMutexLocker k(&_imp->isViewerPausedMutex);

    _imp->isViewerPaused[0] = paused;
    if (allInputs) {
        _imp->isViewerPaused[1] = paused;
    }
}

bool
ViewerInstance::isViewerPaused(int texIndex) const
{
    QMutexLocker k(&_imp->isViewerPausedMutex);

    return _imp->isViewerPaused[texIndex];
}

void
ViewerInstance::onInputChanged(int /*inputNb*/)
{
    Q_EMIT clipPreferencesChanged();
}

void
ViewerInstance::onMetadataRefreshed(const NodeMetadata& /*metadata*/)
{
    Q_EMIT clipPreferencesChanged();
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
    NodePtr n = getNode();
    InspectorNode* isInspector = dynamic_cast<InspectorNode*>( n.get() );

    assert(isInspector);
    if (isInspector) {
        isInspector->getActiveInputs(a, b);
    }
}

void
ViewerInstance::setInputA(int inputNb)
{
    NodePtr n = getNode();
    InspectorNode* isInspector = dynamic_cast<InspectorNode*>( n.get() );

    assert(isInspector);
    if (isInspector) {
        isInspector->setInputA(inputNb);
    }

    Q_EMIT availableComponentsChanged();
}

void
ViewerInstance::setInputB(int inputNb)
{
    NodePtr n = getNode();
    InspectorNode* isInspector = dynamic_cast<InspectorNode*>( n.get() );

    assert(isInspector);
    if (isInspector) {
        isInspector->setInputB(inputNb);
    }

    Q_EMIT availableComponentsChanged();
}

int
ViewerInstance::getLastRenderedTime() const
{
    return _imp->uiContext ? _imp->uiContext->getCurrentlyDisplayedTime() : getApp()->getTimeLine()->currentFrame();
}

TimeLinePtr
ViewerInstance::getTimeline() const
{
    return _imp->uiContext ? _imp->uiContext->getTimeline() : getApp()->getTimeLine();
}

void
ViewerInstance::getTimelineBounds(int* first,
                                  int* last) const
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
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) );

    return std::log(closestPowerOf2) / M_LN2;
}

double
ViewerInstance::getCurrentTime() const
{
    return getFrameRenderArgsCurrentTime();
}

ViewIdx
ViewerInstance::getCurrentView() const
{
    return getFrameRenderArgsCurrentView();
}

bool
ViewerInstance::isLatestRender(int textureIndex,
                               U64 renderAge) const
{
    return _imp->isLatestRender(textureIndex, renderAge);
}

void
ViewerInstance::setPartialUpdateParams(const std::list<RectD>& rois,
                                       bool recenterViewer)
{
    double viewerCenterX = 0;
    double viewerCenterY = 0;

    if (recenterViewer) {
        RectD bbox;
        bool bboxSet = false;
        for (std::list<RectD>::const_iterator it = rois.begin(); it != rois.end(); ++it) {
            if (!bboxSet) {
                bboxSet = true;
                bbox = *it;
            } else {
                bbox.merge(*it);
            }
        }
        viewerCenterX = (bbox.x1 + bbox.x2) / 2.;
        viewerCenterY = (bbox.y1 + bbox.y2) / 2.;
    }
    QMutexLocker k(&_imp->viewerParamsMutex);
    _imp->partialUpdateRects = rois;
    _imp->viewportCenterSet = recenterViewer;
    _imp->viewportCenter.x = viewerCenterX;
    _imp->viewportCenter.y = viewerCenterY;
}

void
ViewerInstance::clearPartialUpdateParams()
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    _imp->partialUpdateRects.clear();
    _imp->viewportCenterSet = false;
}

void
ViewerInstance::setDoingPartialUpdates(bool doing)
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    _imp->isDoingPartialUpdates = doing;
}

bool
ViewerInstance::isDoingPartialUpdates() const
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    return _imp->isDoingPartialUpdates;
}

void
ViewerInstance::reportStats(int time,
                            ViewIdx view,
                            double wallTime,
                            const RenderStatsMap& stats)
{
    Q_EMIT renderStatsAvailable(time, view, wallTime, stats);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ViewerInstance.cpp"
#include "moc_ViewerInstancePrivate.cpp"
