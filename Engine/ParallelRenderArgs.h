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

#ifndef PARALLELRENDERARGS_H
#define PARALLELRENDERARGS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <set>
#include <map>
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/GlobalDefines.h"

#include "Engine/RectD.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


//This controls how many frames a plug-in can pre-fetch (per view and per input)
//This is to avoid cases where the user would for example use the FrameBlend node with a huge amount of frames so that they
//do not all stick altogether in memory
#define NATRON_MAX_FRAMES_NEEDED_PRE_FETCHING 4

NATRON_NAMESPACE_ENTER;

typedef std::map<EffectInstPtr ,RectD> RoIMap; // RoIs are in canonical coordinates
typedef std::map<ViewIdx, std::vector<RangeD> > FrameRangesMap;
typedef std::map<int, FrameRangesMap> FramesNeededMap;

struct InputMatrix
{
    EffectInstPtr newInputEffect;
    boost::shared_ptr<Transform::Matrix3x3> cat;
    int newInputNbToFetchFrom;
};

typedef std::map<int,InputMatrix> InputMatrixMap;

struct NodeFrameRequest;

/**
 * @brief Thread-local arguments given to render a frame by the tree.
 * This is different than the RenderArgs because it is not local to a
 * renderRoI call but to the rendering of a whole frame.
 **/
struct ParallelRenderArgs
{
    // Developper note: the fields were reordered to optimize packing.
    // see http://www.catb.org/esr/structure-packing/

    ///The initial time requested to render.
    ///This may be different than the time held in RenderArgs
    ///which are local to a renderRoI call whilst this is local
    ///to a frame being rendered by the tree.
    double time;
    
    ///To check the current time on the timeline
    const TimeLine* timeline;

    ///The hash of the node at the time we started rendering
    U64 nodeHash;
    
    ///If set, contains data for all frame/view pair that are going to be computed
    ///for this frame/view pair with the overall RoI to avoid rendering several times with this node.
    boost::shared_ptr<NodeFrameRequest> request;

    ///The initial view requested to render.
    ///This may be different than the view held in RenderArgs
    ///which are local to a renderRoI call whilst this is local
    ///to a frame being rendered by the tree.
    ViewIdx view;


    ///A number identifying the current frame render to determine if we can really abort for abortable renders
    U64 renderAge;
    
    ///A pointer to the node that requested the current render.
    NodePtr treeRoot;

    ///List of the nodes in the rotopaint tree
    NodesList rotoPaintNodes;

    ///Various stats local to the render of a frame
    boost::shared_ptr<RenderStats> stats;

    ///The texture index of the viewer being rendered, only useful for abortable renders
    int textureIndex;

    ///Current thread safety: it might change in the case of the rotopaint: while drawing, the safety is instance safe,
    ///whereas afterwards we revert back to the plug-in thread safety
    RenderSafetyEnum currentThreadSafety;

    /// is this a render due to user interaction ? Generally this is true when rendering because
    /// of a user parameter tweek or timeline seek, or more generally by calling RenderEngine::renderCurrentFrame
    bool isRenderResponseToUserInteraction:1;

    /// Is this render sequential ? True for Viewer playback or a sequential writer such as WriteFFMPEG
    bool isSequentialRender:1;

    /// True if this frame can be aborted (false for preview and tracking)
    bool canAbort:1;

    ///Was the render started in the instanceChangedAction (knobChanged)
    bool isAnalysis:1;
    
    ///If true, the attached paint stroke is being drawn currently
    bool isDuringPaintStrokeCreation:1;

    ///When true, all NaNs will be converted to 1
    bool doNansHandling:1;
    
    ///When true, this is a hint for plug-ins that the render will be used for draft such as previewing while scrubbing the timeline
    bool draftMode:1;
    
    ///The support for tiles is local to a render and may change depending on GPU usage or other parameters
    bool tilesSupported:1;
    
    ///True when the preference in Natron is set and the renderRequester is a Viewer
    bool viewerProgressReportEnabled:1;

    
    ParallelRenderArgs()
    : time(0)
    , timeline(0)
    , nodeHash(0)
    , request()
    , view(0)
    , renderAge(0)
    , treeRoot()
    , rotoPaintNodes()
    , stats()
    , textureIndex(0)
    , currentThreadSafety(eRenderSafetyInstanceSafe)
    , isRenderResponseToUserInteraction(false)
    , isSequentialRender(false)
    , canAbort(false)
    , isAnalysis(false)
    , isDuringPaintStrokeCreation(false)
    , doNansHandling(true)
    , draftMode(false)
    , tilesSupported(false)
    , viewerProgressReportEnabled(false)
    {
        
    }
};




struct FrameViewPair {
    double time;
    ViewIdx view;
};

struct FrameViewRequestGlobalData
{
    ///The transforms associated to each input branch, set on first request
    boost::shared_ptr<InputMatrixMap> transforms;
    boost::shared_ptr<std::map<int, EffectInstPtr> > reroutesMap;
    
    ///The required frame/views in input, set on first request
    FramesNeededMap frameViewsNeeded;
    
    ///Set when the first request is made, set on first request
    RectD rod;
    bool isProjectFormat;

    //Identity data, set on first request
    bool isIdentity;
    int identityInputNb;
    double inputIdentityTime;
};

struct FrameViewRequestFinalData
{
    RectD finalRoi;
};

struct FrameViewPerRequestData
{
    RoIMap inputsRoi;
};

struct FrameViewRequest
{
    ///All different requests led by different branchs in the tree
    std::list<std::pair<RectD,FrameViewPerRequestData> > requests;
    
    ///Final datas that are computed once the whole tree has been cycled through
    FrameViewRequestFinalData finalData;
    
    ///Global datas for this frame/view set upon first request
    FrameViewRequestGlobalData globalData;
    
};

struct FrameView_compare_less
{
    bool operator() (const FrameViewPair & lhs,
                     const FrameViewPair & rhs) const
    {
        if (lhs.time < rhs.time) {
            return true;
        } else if (lhs.time > rhs.time) {
            return false;
        } else {
            if (lhs.view < rhs.view) {
                return true;
            } else if (lhs.view > rhs.view) {
                return true;
            } else {
                return false;
            }
        }
    }
};

typedef std::map<FrameViewPair,FrameViewRequest,FrameView_compare_less> NodeFrameViewRequestData;

struct NodeFrameRequest
{
    NodeFrameViewRequestData frames;
    
    ///Set on first request
    U64 nodeHash;
    RenderScale mappedScale;
    
    bool getFrameViewCanonicalRoI(double time, ViewIdx view, RectD* roi) const;
    
    const FrameViewRequest* getFrameViewRequest(double time, ViewIdx view) const;

};

typedef std::map<NodePtr,boost::shared_ptr<NodeFrameRequest> > FrameRequestMap;


class ParallelRenderArgsSetter
{
    boost::shared_ptr<std::map<NodePtr,boost::shared_ptr<ParallelRenderArgs> > > argsMap;
    NodesList nodes;
    
public:
    
    /**
     * @brief Set the TLS for rendering a frame on the tree upstream of treeRoot (including it) and all nodes that 
     * can be reached through expressions.
     * We do this because TLS is needed to know the correct frame, view at which the frame is evaluated (i.e rendered)
     * even in nodes that do not belong in the tree. The reason why is because the nodes in the tree may have parameters
     * relying on other nodes that do not belong in the tree through expressions.
     **/
    ParallelRenderArgsSetter(double time,
                             ViewIdx view,
                             bool isRenderUserInteraction,
                             bool isSequential,
                             bool canAbort,
                             U64 renderAge,
                             const NodePtr& treeRoot,
                             const FrameRequestMap* request,
                             int textureIndex,
                             const TimeLine* timeline,
                             const NodePtr& activeRotoPaintNode,
                             bool isAnalysis,
                             bool draftMode,
                             bool viewerProgressReportEnabled,
                             const boost::shared_ptr<RenderStats>& stats);
    
    ParallelRenderArgsSetter(const boost::shared_ptr<std::map<NodePtr, boost::shared_ptr<ParallelRenderArgs> > >& args);
    
    virtual ~ParallelRenderArgsSetter();
};

NATRON_NAMESPACE_EXIT;

#endif // PARALLELRENDERARGS_H
