/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "TrackerContext.h"

#include <set>
#include <sstream>

#include <QWaitCondition>
#include <QThread>
#include <QCoreApplication>
#include <QtConcurrentMap>

#include <libmv/autotrack/autotrack.h>
#include <libmv/autotrack/frame_accessor.h>
#include <libmv/image/array_nd.h>

#include <openMVG/robust_estimation/robust_estimator_Prosac.hpp>

#include <boost/bind.hpp>

#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/Curve.h"
#include "Engine/TLSHolder.h"
#include "Engine/Transform.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerSerialization.h"
#include "Engine/ViewerInstance.h"

#define kTrackBaseName "track"
#define TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE 8

/// Parameters definitions

//////// Global to all tracks
#define kTrackerParamTrackRed "trackRed"
#define kTrackerParamTrackRedLabel "Track Red"
#define kTrackerParamTrackRedHint "Enable tracking on the red channel"

#define kTrackerParamTrackGreen "trackGreen"
#define kTrackerParamTrackGreenLabel "Track Green"
#define kTrackerParamTrackGreenHint "Enable tracking on the green channel"

#define kTrackerParamTrackBlue "trackBlue"
#define kTrackerParamTrackBlueLabel "Track Blue"
#define kTrackerParamTrackBlueHint "Enable tracking on the blue channel"

#define kTrackerParamMaxError "maxError"
#define kTrackerParamMaxErrorLabel "Max. Error"
#define kTrackerParamMaxErrorHint "This is the minimum necessary error between the final tracked " \
"position of the patch on the destination image and the reference patch needed to declare tracking success." \
"The error is 1 minus the normalized cross-correlation score."

#define kTrackerParamMaximumIteration "maxIterations"
#define kTrackerParamMaximumIterationLabel "Maximum iterations"
#define kTrackerParamMaximumIterationHint "Maximum number of iterations the algorithm will run for the inner minimization " \
"before it gives up."

#define kTrackerParamBruteForcePreTrack "bruteForcePretTrack"
#define kTrackerParamBruteForcePreTrackLabel "Use brute-force pre-track"
#define kTrackerParamBruteForcePreTrackHint "Use a brute-force translation-only pre-track before refinement"

#define kTrackerParamNormalizeIntensities "normalizeIntensities"
#define kTrackerParamNormalizeIntensitiesLabel "Normalize Intensities"
#define kTrackerParamNormalizeIntensitiesHint "Normalize the image patches by their mean before doing the sum of squared" \
" error calculation. Slower."

#define kTrackerParamPreBlurSigma "preBlurSigma"
#define kTrackerParamPreBlurSigmaLabel "Pre-blur sigma"
#define kTrackerParamPreBlurSigmaHint "The size in pixels of the blur kernel used to both smooth the image and take the image derivative."

#define kTrackerParamExportDataSeparator "exportDataSection"
#define kTrackerParamExportDataSeparatorLabel "Export"

#define kTrackerParamExportDataChoice "exportDataOptions"
#define kTrackerParamExportDataChoiceLabel "Type"
#define kTrackerParamExportDataChoiceHint "Select the desired option to the Transform/CornerPin node that will be exported. " \
"Each export has a \"baked\" option which will copy the animation out of the tracks instead of linking them directly."

#define kTrackerParamExportDataChoiceCPThisFrame "CornerPin (Use current frame), linked"
#define kTrackerParamExportDataChoiceCPThisFrameHint "Warp the image according to the relative transform using the current frame as reference"

#define kTrackerParamExportDataChoiceCPRefFrame "CornerPin (Reference frame), linked"
#define kTrackerParamExportDataChoiceCPRefFrameHint "Warp the image according to the relative transform using the reference frame selected in " \
" the Transform tab as reference"

#define kTrackerParamExportDataChoiceCPThisFrameBaked "CornerPin (Use current frame), baked"

#define kTrackerParamExportDataChoiceCPRefFrameBaked "CornerPin (Reference frame), baked"

#define kTrackerParamExportDataChoiceTransformStabilize "Transform (Stabilize), linked"
#define kTrackerParamExportDataChoiceTransformStabilizeHint "Creates a Transform node that will stabilize the footage. The Transform is identity " \
"at the reference frame selected in the Transform tab"

#define kTrackerParamExportDataChoiceTransformMatchMove "Transform (Match-move), linked"
#define kTrackerParamExportDataChoiceTransformMatchMoveHint "Creates a Transform node that will match-move the footage. The Transform is identity " \
"at the reference frame selected in the Transform tab"

#define kTrackerParamExportDataChoiceTransformStabilizeBaked "Transform (Stabilize), baked"
#define kTrackerParamExportDataChoiceTransformMatchMoveBaked "Transform (Match-move), baked"


#define kTrackerParamExportButton "export"
#define kTrackerParamExportButtonLabel "Export"
#define kTrackerParamExportButtonHint "Creates a node referencing the tracked data according to the export type chosen on the left"

#define kCornerPinInvertParamName "invert"

#define kTrackerParamTransformType "transformType"
#define kTrackerParamTransformTypeLabel "Transform Type"
#define kTrackerParamTransformTypeHint "The type of transform in output of this node."

#define kTrackerParamTransformTypeNone "None"
#define kTrackerParamTransformTypeNoneHelp "No transformation applied in output to the image: this node is a pass-through. Set it to this mode when tracking to correclty see the input image on the viewer"

#define kTrackerParamTransformTypeStabilize "Stabilize"
#define kTrackerParamTransformTypeStabilizeHelp "Transforms the image so that the tracked points do not move"

#define kTrackerParamTransformTypeMatchMove "Match-Move"
#define kTrackerParamTransformTypeMatchMoveHelp "Transforms a different image so that it moves to match the tracked points"

#define kTrackerParamTransformTypeRemoveJitter "Remove Jitter"
#define kTrackerParamTransformTypeRemoveJitterHelp "Transforms the image so that the tracked points move smoothly with high frequencies removed"

enum TrackerTransformTypeEnum
{
    eTrackerTransformTypeNone,
    eTrackerTransformTypeStabilize,
    eTrackerTransformTypeMatchMove,
    eTrackerTransformTypeRemoveJitter,
    eTrackerTransformTypeAddJitter
};

#define kTrackerParamTransformTypeAddJitter "Add Jitter"
#define kTrackerParamTransformTypeAddJitterHelp "Transforms the image by the high frequencies of the animation of the tracks to increase the shake or apply it on another image"

#define kTrackerParamReferenceFrame "referenceFrame"
#define kTrackerParamReferenceFrameLabel "Reference frame"
#define kTrackerParamReferenceFrameHint "When exporting tracks to a CornerPin or Transform, this will be the frame number at which the transform will be an identity."

#define kTrackerParamSetReferenceFrame "setReferenceButton"
#define kTrackerParamSetReferenceFrameLabel "Set To Current Frame"
#define kTrackerParamSetReferenceFrameHint "Set the reference frame to the timeline's current frame"

#define kTrackerParamJitterPeriod "jitterPeriod"
#define kTrackerParamJitterPeriodLabel "Jitter Period"
#define kTrackerParamJitterPeriodHint "Number of frames to average together to remove high frequencies for the add/remove jitter transform type"

#define kTrackerParamSmooth "smooth"
#define kTrackerParamSmoothLabel "Smooth"
#define kTrackerParamSmoothHint "Smooth the translation/rotation/scale by averaging this number of frames together"

#define kTransformParamTranslate "translate"
#define kTransformParamRotate "rotate"
#define kTransformParamScale "scale"
#define kTransformParamUniform "uniform"
#define kTransformParamSkewX "skewX"
#define kTransformParamSkewY "skewY"
#define kTransformParamSkewOrder "skewOrder"
#define kTransformParamCenter "center"
#define kTransformParamInvert "invert"
#define kTransformParamFilter "filter"
#define kTransformParamClamp "clamp"
#define kTransformParamBlackOutside "black_outside"
#define kTransformParamMotionBlur "motionBlur"
#define kTransformParamShutter "shutter"
#define kTransformParamShutterOffset "shutterOffset"
#define kTransformParamCustomShutterOffset "shutterCustomOffset"

//////// Per-track parameters


#ifdef DEBUG
//#define TRACE_LIB_MV 1
#endif

NATRON_NAMESPACE_ENTER;

namespace  {
    
enum libmv_MarkerChannel {
    LIBMV_MARKER_CHANNEL_R = (1 << 0),
    LIBMV_MARKER_CHANNEL_G = (1 << 1),
    LIBMV_MARKER_CHANNEL_B = (1 << 2),
};

} // anon namespace

void
TrackerContext::getMotionModelsAndHelps(bool addPerspective,
                                        std::vector<std::string>* models,
                                        std::vector<std::string>* tooltips)
{
    models->push_back("Trans.");
    tooltips->push_back(kTrackerParamMotionModelTranslation);
    models->push_back("Trans.+Rot.");
    tooltips->push_back(kTrackerParamMotionModelTransRot);
    models->push_back("Trans.+Scale");
    tooltips->push_back(kTrackerParamMotionModelTransScale);
    models->push_back("Trans.+Rot.+Scale");
    tooltips->push_back(kTrackerParamMotionModelTransRotScale);
    models->push_back("Affine");
    tooltips->push_back(kTrackerParamMotionModelAffine);
    if (addPerspective) {
        models->push_back("Perspective");
        tooltips->push_back(kTrackerParamMotionModelPerspective);
    }
}



struct TrackMarkerAndOptions
{
    TrackMarkerPtr natronMarker;
    mv::Marker mvMarker;
    mv::TrackRegionOptions mvOptions;
};

class FrameAccessorImpl;

class TrackArgsLibMV
{
    int _start, _end;
    bool _isForward;
    boost::shared_ptr<TimeLine> _timeline;
    ViewerInstance* _viewer;
    boost::shared_ptr<mv::AutoTrack> _libmvAutotrack;
    boost::shared_ptr<FrameAccessorImpl> _fa;
    std::vector<boost::shared_ptr<TrackMarkerAndOptions> > _tracks;
    
    //Store the format size because LibMV internally has a top-down Y axis
    double _formatWidth,_formatHeight;
    mutable QMutex _autoTrackMutex;
    
public:
    
    TrackArgsLibMV()
    : _start(0)
    , _end(0)
    , _isForward(false)
    , _timeline()
    , _viewer(0)
    , _libmvAutotrack()
    , _fa()
    , _tracks()
    , _formatWidth(0)
    , _formatHeight(0)
    {
        
    }
    
    TrackArgsLibMV(int start,
                   int end,
                   bool isForward,
                   const boost::shared_ptr<TimeLine>& timeline,
                   ViewerInstance* viewer,
                   const boost::shared_ptr<mv::AutoTrack>& autoTrack,
                   const boost::shared_ptr<FrameAccessorImpl>& fa,
                   const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& tracks,
                   double formatWidth,
                   double formatHeight)
    : _start(start)
    , _end(end)
    , _isForward(isForward)
    , _timeline(timeline)
    , _viewer(viewer)
    , _libmvAutotrack(autoTrack)
    , _fa(fa)
    , _tracks(tracks)
    , _formatWidth(formatWidth)
    , _formatHeight(formatHeight)
    {
        
    }
    
    TrackArgsLibMV(const TrackArgsLibMV& other)
    {
        *this = other;
    }
    
    void operator=(const TrackArgsLibMV& other)
    {
        _start = other._start;
        _end = other._end;
        _isForward = other._isForward;
        _timeline = other._timeline;
        _viewer = other._viewer;
        _libmvAutotrack = other._libmvAutotrack;
        _fa = other._fa;
        _tracks = other._tracks;
        _formatWidth = other._formatWidth;
        _formatHeight = other._formatHeight;
    }
    
    double getFormatHeight() const
    {
        return _formatHeight;
    }
    
    double getFormatWidth() const
    {
        return _formatWidth;
    }
    
    QMutex* getAutoTrackMutex() const
    {
        return &_autoTrackMutex;
    }
    
    int getStart() const
    {
        return _start;
    }
    
    int getEnd() const
    {
        return _end;
    }
    
    bool getForward() const
    {
        return _isForward;
    }
    
    boost::shared_ptr<TimeLine> getTimeLine() const
    {
        return _timeline;
    }
    
    ViewerInstance* getViewer() const
    {
        return _viewer;
    }
    
    int getNumTracks() const
    {
        return (int)_tracks.size();
    }
    
    const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& getTracks() const
    {
        return _tracks;
    }
    
    boost::shared_ptr<mv::AutoTrack> getLibMVAutoTrack() const
    {
        return _libmvAutotrack;
    }
    
    void getEnabledChannels(bool* r, bool* g, bool* b) const;
    
    void getRedrawAreasNeeded(int time, std::list<RectD>* canonicalRects) const
    {
        for (std::vector<boost::shared_ptr<TrackMarkerAndOptions> >::const_iterator it = _tracks.begin(); it!=_tracks.end(); ++it) {
            boost::shared_ptr<KnobDouble> searchBtmLeft = (*it)->natronMarker->getSearchWindowBottomLeftKnob();
            boost::shared_ptr<KnobDouble> searchTopRight = (*it)->natronMarker->getSearchWindowTopRightKnob();
            boost::shared_ptr<KnobDouble> centerKnob = (*it)->natronMarker->getCenterKnob();
            boost::shared_ptr<KnobDouble> offsetKnob = (*it)->natronMarker->getOffsetKnob();
            
            Natron::Point offset,center,btmLeft,topRight;
            offset.x = offsetKnob->getValueAtTime(time, 0);
            offset.y = offsetKnob->getValueAtTime(time, 1);
            
            center.x = centerKnob->getValueAtTime(time, 0);
            center.y = centerKnob->getValueAtTime(time, 1);
            
            btmLeft.x = searchBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
            btmLeft.y = searchBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
            
            topRight.x = searchTopRight->getValueAtTime(time, 0) + center.x + offset.x;
            topRight.y = searchTopRight->getValueAtTime(time, 1) + center.y + offset.y;
            
            RectD rect;
            rect.x1 = btmLeft.x;
            rect.y1 = btmLeft.y;
            rect.x2 = topRight.x;
            rect.y2 = topRight.y;
            canonicalRects->push_back(rect);
        }
    }
};

void
TrackArgsV1::getRedrawAreasNeeded(int time, std::list<RectD>* canonicalRects) const
{
    for (std::vector<KnobButton*>::const_iterator it = _buttonInstances.begin(); it!=_buttonInstances.end(); ++it) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>((*it)->getHolder());
        assert(effect);
    
        boost::shared_ptr<KnobDouble> searchBtmLeft = boost::dynamic_pointer_cast<KnobDouble>(effect->getKnobByName("searchBoxBtmLeft"));
        boost::shared_ptr<KnobDouble> searchTopRight = boost::dynamic_pointer_cast<KnobDouble>(effect->getKnobByName("searchBoxTopRight"));
        boost::shared_ptr<KnobDouble> centerKnob = boost::dynamic_pointer_cast<KnobDouble>(effect->getKnobByName("center"));
        boost::shared_ptr<KnobDouble> offsetKnob = boost::dynamic_pointer_cast<KnobDouble>(effect->getKnobByName("offset"));
        assert(searchBtmLeft  && searchTopRight && centerKnob && offsetKnob);
        Natron::Point offset,center,btmLeft,topRight;
        offset.x = offsetKnob->getValueAtTime(time, 0);
        offset.y = offsetKnob->getValueAtTime(time, 1);
        
        center.x = centerKnob->getValueAtTime(time, 0);
        center.y = centerKnob->getValueAtTime(time, 1);
        
        btmLeft.x = searchBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
        btmLeft.y = searchBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
        
        topRight.x = searchTopRight->getValueAtTime(time, 0) + center.x + offset.x;
        topRight.y = searchTopRight->getValueAtTime(time, 1) + center.y + offset.y;
        
        RectD rect;
        rect.x1 = btmLeft.x;
        rect.y1 = btmLeft.y;
        rect.x2 = topRight.x;
        rect.y2 = topRight.y;
        canonicalRects->push_back(rect);
    }
}
/*
static void updateBbox(const Natron::Point& p, RectD* bbox)
{
    bbox->x1 = std::min(p.x, bbox->x1);
    bbox->x2 = std::max(p.x, bbox->x2);
    bbox->y1 = std::min(p.x, bbox->y1);
    bbox->y2 = std::max(p.x, bbox->y2);
}
*/
static double invertYCoordinate(double yIn, double formatHeight)
{
    return formatHeight - 1 - yIn;
}

static void convertLibMVRegionToRectI(const mv::Region& region, int formatHeight, RectI* roi)
{
    roi->x1 = region.min(0);
    roi->x2 = region.max(0);
    roi->y1 = invertYCoordinate(region.max(1),formatHeight);
    roi->y2 = invertYCoordinate(region.min(1),formatHeight);
}


/**
 * @brief Set keyframes on knobs from Marker data
 **/
static void setKnobKeyframesFromMarker(const mv::Marker& mvMarker,
                                       int formatHeight,
                                       const libmv::TrackRegionResult* result,
                                       const TrackMarkerPtr& natronMarker)
{
    int time = mvMarker.frame;
    boost::shared_ptr<KnobDouble> errorKnob = natronMarker->getErrorKnob();
    if (result) {
        errorKnob->setValueAtTime(time, 1. - result->correlation, ViewSpec::current(), 0);
    } else {
        errorKnob->setValueAtTime(time, 0., ViewSpec::current(), 0);
    }
    
    Natron::Point center;
    center.x = (double)mvMarker.center(0);
    center.y = (double)invertYCoordinate(mvMarker.center(1), formatHeight);
    
    boost::shared_ptr<KnobDouble> offsetKnob = natronMarker->getOffsetKnob();
    Natron::Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    // Set the center
    boost::shared_ptr<KnobDouble> centerKnob = natronMarker->getCenterKnob();
    centerKnob->setValuesAtTime(time, center.x, center.y, ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
    
    Natron::Point topLeftCorner,topRightCorner,btmRightCorner,btmLeftCorner;
    topLeftCorner.x = mvMarker.patch.coordinates(0,0) - offset.x - center.x;
    topLeftCorner.y = invertYCoordinate(mvMarker.patch.coordinates(0,1),formatHeight) - offset.y - center.y;
    
    topRightCorner.x = mvMarker.patch.coordinates(1,0) - offset.x - center.x;
    topRightCorner.y = invertYCoordinate(mvMarker.patch.coordinates(1,1),formatHeight) - offset.y - center.y;
    
    btmRightCorner.x = mvMarker.patch.coordinates(2,0) - offset.x - center.x;
    btmRightCorner.y = invertYCoordinate(mvMarker.patch.coordinates(2,1),formatHeight) - offset.y - center.y;
    
    btmLeftCorner.x = mvMarker.patch.coordinates(3,0) - offset.x - center.x;
    btmLeftCorner.y = invertYCoordinate(mvMarker.patch.coordinates(3,1),formatHeight) - offset.y - center.y;
    
    boost::shared_ptr<KnobDouble> pntTopLeftKnob = natronMarker->getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> pntTopRightKnob = natronMarker->getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> pntBtmLeftKnob = natronMarker->getPatternBtmLeftKnob();
    boost::shared_ptr<KnobDouble> pntBtmRightKnob = natronMarker->getPatternBtmRightKnob();
    
    // Set the pattern Quad
    pntTopLeftKnob->setValuesAtTime(time, topLeftCorner.x, topLeftCorner.y, ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
    pntTopRightKnob->setValuesAtTime(time, topRightCorner.x, topRightCorner.y, ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
    pntBtmLeftKnob->setValuesAtTime(time, btmLeftCorner.x, btmLeftCorner.y,ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
    pntBtmRightKnob->setValuesAtTime(time, btmRightCorner.x, btmRightCorner.y, ViewSpec::current(),Natron::eValueChangedReasonNatronInternalEdited);
}

#if 0
static void updateLibMvTrackMinimal(const TrackMarker& marker,
                                    int time,
                                    bool forward,
                                    int formatHeight,
                                    mv::Marker* mvMarker)
{
    boost::shared_ptr<KnobDouble> searchWindowBtmLeftKnob = marker.getSearchWindowBottomLeftKnob();
    boost::shared_ptr<KnobDouble> searchWindowTopRightKnob = marker.getSearchWindowTopRightKnob();
    boost::shared_ptr<KnobDouble> centerKnob = marker.getCenterKnob();
    boost::shared_ptr<KnobDouble> offsetKnob = marker.getOffsetKnob();
    mvMarker->reference_frame = marker.getReferenceFrame(time, forward);
    if (marker.isUserKeyframe(time)) {
        mvMarker->source = mv::Marker::MANUAL;
    } else {
        mvMarker->source = mv::Marker::TRACKED;
    }
    mvMarker->frame = time;
    
    Natron::Point nCenter;
    nCenter.x = centerKnob->getValueAtTime(time, 0);
    nCenter.y = centerKnob->getValueAtTime(time, 1);
    mvMarker->center(0) = nCenter.x;
    mvMarker->center(1) = invertYCoordinate(nCenter.y, formatHeight);
    
    Natron::Point searchWndBtmLeft,searchWndTopRight;
    searchWndBtmLeft.x = searchWindowBtmLeftKnob->getValueAtTime(mvMarker->reference_frame, 0);
    searchWndBtmLeft.y = searchWindowBtmLeftKnob->getValueAtTime(mvMarker->reference_frame, 1);
    
    searchWndTopRight.x = searchWindowTopRightKnob->getValueAtTime(mvMarker->reference_frame, 0);
    searchWndTopRight.y = searchWindowTopRightKnob->getValueAtTime(mvMarker->reference_frame, 1);
    
    Natron::Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    //See natronTrackerToLibMVTracker below for coordinates system
    mvMarker->search_region.min(0) = searchWndBtmLeft.x + mvMarker->center(0) + offset.x;
    mvMarker->search_region.min(1) = invertYCoordinate(searchWndTopRight.y + nCenter.x + offset.y,formatHeight);
    mvMarker->search_region.max(0) = searchWndTopRight.x + mvMarker->center(0) + offset.x;
    mvMarker->search_region.max(1) = invertYCoordinate(searchWndBtmLeft.y + nCenter.y + offset.y,formatHeight);
}
#endif

/// Converts a Natron track marker to the one used in LibMV. This is expensive: many calls to getValue are made
static void natronTrackerToLibMVTracker(bool useRefFrameForSearchWindow,
                                        bool trackChannels[3],
                                        const TrackMarker& marker,
                                        int trackIndex,
                                        int time,
                                        bool forward,
                                        double formatHeight,
                                        mv::Marker* mvMarker)
{
    boost::shared_ptr<KnobDouble> searchWindowBtmLeftKnob = marker.getSearchWindowBottomLeftKnob();
    boost::shared_ptr<KnobDouble> searchWindowTopRightKnob = marker.getSearchWindowTopRightKnob();
    boost::shared_ptr<KnobDouble> patternTopLeftKnob = marker.getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> patternTopRightKnob = marker.getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> patternBtmRightKnob = marker.getPatternBtmRightKnob();
    boost::shared_ptr<KnobDouble> patternBtmLeftKnob = marker.getPatternBtmLeftKnob();
    
#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    boost::shared_ptr<KnobDouble> weightKnob = marker.getWeightKnob();
#endif
    boost::shared_ptr<KnobDouble> centerKnob = marker.getCenterKnob();
    boost::shared_ptr<KnobDouble> offsetKnob = marker.getOffsetKnob();
    
    // We don't use the clip in Natron
    mvMarker->clip = 0;
    mvMarker->reference_clip = 0;
    
#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    mvMarker->weight = (float)weightKnob->getValue();
#else
    mvMarker->weight = 1.;
#endif
    
    mvMarker->frame = time;
    mvMarker->reference_frame = marker.getReferenceFrame(time, forward);
    if (marker.isUserKeyframe(time)) {
        mvMarker->source = mv::Marker::MANUAL;
    } else {
        mvMarker->source = mv::Marker::TRACKED;
    }
    Natron::Point nCenter;
    nCenter.x = centerKnob->getValueAtTime(time, 0);
    nCenter.y = centerKnob->getValueAtTime(time, 1);
    mvMarker->center(0) = nCenter.x;
    mvMarker->center(1) = invertYCoordinate(nCenter.y, formatHeight);
    mvMarker->model_type = mv::Marker::POINT;
    mvMarker->model_id = 0;
    mvMarker->track = trackIndex;
    
    mvMarker->disabled_channels =
    (trackChannels[0] ? LIBMV_MARKER_CHANNEL_R : 0) |
    (trackChannels[1] ? LIBMV_MARKER_CHANNEL_G : 0) |
    (trackChannels[2] ? LIBMV_MARKER_CHANNEL_B : 0);
    
    Natron::Point searchWndBtmLeft,searchWndTopRight;
    int searchWinTime = useRefFrameForSearchWindow ? mvMarker->reference_frame : time;
    searchWndBtmLeft.x = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, 0);
    searchWndBtmLeft.y = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, 1);
    
    searchWndTopRight.x = searchWindowTopRightKnob->getValueAtTime(searchWinTime, 0);
    searchWndTopRight.y = searchWindowTopRightKnob->getValueAtTime(searchWinTime, 1);
    
    Natron::Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    
    Natron::Point tl,tr,br,bl;
    tl.x = patternTopLeftKnob->getValueAtTime(time, 0);
    tl.y = patternTopLeftKnob->getValueAtTime(time, 1);
    
    tr.x = patternTopRightKnob->getValueAtTime(time, 0);
    tr.y = patternTopRightKnob->getValueAtTime(time, 1);
    
    br.x = patternBtmRightKnob->getValueAtTime(time, 0);
    br.y = patternBtmRightKnob->getValueAtTime(time, 1);
    
    bl.x = patternBtmLeftKnob->getValueAtTime(time, 0);
    bl.y = patternBtmLeftKnob->getValueAtTime(time, 1);
    
    /*RectD patternBbox;
    patternBbox.setupInfinity();
    updateBbox(tl, &patternBbox);
    updateBbox(tr, &patternBbox);
    updateBbox(br, &patternBbox);
    updateBbox(bl, &patternBbox);*/
    
    // The search-region is laid out as such:
    //
    //    +----------> x
    //    |
    //    |   (min.x, min.y)           (max.x, min.y)
    //    |        +-------------------------+
    //    |        |                         |
    //    |        |                         |
    //    |        |                         |
    //    |        +-------------------------+
    //    v   (min.x, max.y)           (max.x, max.y)
    //
    mvMarker->search_region.min(0) = searchWndBtmLeft.x + nCenter.x + offset.x;
    mvMarker->search_region.min(1) = invertYCoordinate(searchWndTopRight.y + nCenter.y + offset.y,formatHeight);
    mvMarker->search_region.max(0) = searchWndTopRight.x + nCenter.x + offset.x;
    mvMarker->search_region.max(1) = invertYCoordinate(searchWndBtmLeft.y + nCenter.y + offset.y,formatHeight);
    
    
    // The patch is a quad (4 points); generally in 2D or 3D (here 2D)
    //
    //    +----------> x
    //    |\.
    //    | \.
    //    |  z (z goes into screen)
    //    |
    //    |     r0----->r1
    //    |      ^       |
    //    |      |   .   |
    //    |      |       V
    //    |     r3<-----r2
    //    |              \.
    //    |               \.
    //    v                normal goes away (right handed).
    //    y
    //
    // Each row is one of the corners coordinates; either (x, y) or (x, y, z).
    // TrackMarker extracts the patch coordinates as such:
    /*
     Quad2Df offset_quad = marker.patch;
     Vec2f origin = marker.search_region.Rounded().min;
     offset_quad.coordinates.rowwise() -= origin.transpose();
     QuadToArrays(offset_quad, x, y);
     x[4] = marker.center.x() - origin(0);
     y[4] = marker.center.y() - origin(1);
     */
    // The patch coordinates should be canonical
    
    mvMarker->patch.coordinates(0,0) = tl.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(0,1) = invertYCoordinate(tl.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(1,0) = tr.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(1,1) = invertYCoordinate(tr.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(2,0) = br.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(2,1) = invertYCoordinate(br.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(3,0) = bl.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(3,1) = invertYCoordinate(bl.y + nCenter.y + offset.y,formatHeight);
}

/*
 * @brief This is the internal tracking function that makes use of LivMV to do 1 track step
 * @param trackingIndex This is the index of the Marker we should track in the args
 * @param args Multiple arguments global to the whole track, not just this step
 * @param time The search frame time, that is, the frame to track
 */
static bool trackStepLibMV(int trackIndex, const TrackArgsLibMV& args, int time)
{
    assert(trackIndex >= 0 && trackIndex < args.getNumTracks());
    
    const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& tracks = args.getTracks();
    const boost::shared_ptr<TrackMarkerAndOptions>& track = tracks[trackIndex];
    
    boost::shared_ptr<mv::AutoTrack> autoTrack = args.getLibMVAutoTrack();
    QMutex* autoTrackMutex = args.getAutoTrackMutex();
    
    bool enabledChans[3];
    args.getEnabledChannels(&enabledChans[0], &enabledChans[1], &enabledChans[2]);
    

    {
        //Add the tracked marker
        QMutexLocker k(autoTrackMutex);
        natronTrackerToLibMVTracker(true, enabledChans, *track->natronMarker, trackIndex, time, args.getForward(), args.getFormatHeight(), &track->mvMarker);
        autoTrack->AddMarker(track->mvMarker);
    }
    
    
    //The frame on the mvMarker should have been set accordingly
    assert(track->mvMarker.frame == time);

    if (track->mvMarker.source == mv::Marker::MANUAL) {
        // This is a user keyframe or the first frame, we do not track it
        assert(time == args.getStart() || track->natronMarker->isUserKeyframe(track->mvMarker.frame));
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "TrackStep:" << time << "is a keyframe";
#endif
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), 0, track->natronMarker);
        
    } else {
        
        // Set the reference rame
        track->mvMarker.reference_frame = track->natronMarker->getReferenceFrame(time, args.getForward());
        assert(track->mvMarker.reference_frame != track->mvMarker.frame);

        //Make sure the reference frame as the same search window as the tracked frame and exists in the AutoTrack
        {
            QMutexLocker k(autoTrackMutex);
            mv::Marker m;
            natronTrackerToLibMVTracker(false, enabledChans, *track->natronMarker, track->mvMarker.track, track->mvMarker.reference_frame, args.getForward(), args.getFormatHeight(), &m);
            autoTrack->AddMarker(m);
        }

        
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << ">>>> Tracking marker" << trackIndex << "at frame" << time <<
        "with reference frame" << track->mvMarker.reference_frame;
#endif
        
        // Do the actual tracking
        libmv::TrackRegionResult result;
        if (!autoTrack->TrackMarker(&track->mvMarker, &result, &track->mvOptions) || !result.is_usable()) {
#ifdef TRACE_LIB_MV
            qDebug() << QThread::currentThread() << "Tracking FAILED (" << (int)result.termination <<  ") for track" << trackIndex << "at frame" << time;
#endif
            return false;
        }
        
        //Ok the tracking has succeeded, now the marker is in this state:
        //the source is TRACKED, the search_window has been offset by the center delta,  the center has been offset
        
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "Tracking SUCCESS for track" << trackIndex << "at frame" << time;
#endif
        
        //Extract the marker to the knob keyframes
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), &result, track->natronMarker);
        
        //Add the marker to the autotrack
        /*{
            QMutexLocker k(autoTrackMutex);
            autoTrack->AddMarker(track->mvMarker);
        }*/
    } // if (track->mvMarker.source == mv::Marker::MANUAL) {
    
    appPTR->getAppTLS()->cleanupTLSForThread();
    
    //Refresh the marker for next iteration
    //int nextFrame = args.getForward() ? time + 1 : time - 1;
    //updateLibMvTrackMinimal(*track->natronMarker, nextFrame, args.getForward(), args.getFormatHeight(), &track->mvMarker);
    
    return true;
}

bool
TrackerContext::trackStepV1(int trackIndex, const TrackArgsV1& args, int time)
{
    assert(trackIndex >= 0 && trackIndex < (int)args.getInstances().size());
    KnobButton* selectedInstance = args.getInstances()[trackIndex];
    selectedInstance->getHolder()->onKnobValueChanged_public(selectedInstance,eValueChangedReasonNatronInternalEdited,time, ViewIdx(0),
                                                             true);
    appPTR->getAppTLS()->cleanupTLSForThread();
    return true;
}

struct TrackerContextPrivate
{
    
    TrackerContext* _publicInterface;
    boost::weak_ptr<Natron::Node> node;
    
    std::list<boost::weak_ptr<KnobI> > knobs,perTrackKnobs;
    boost::weak_ptr<KnobBool> enableTrackRed,enableTrackGreen,enableTrackBlue;
    boost::weak_ptr<KnobDouble> maxError;
    boost::weak_ptr<KnobInt> maxIterations;
    boost::weak_ptr<KnobBool> bruteForcePreTrack,useNormalizedIntensities;
    boost::weak_ptr<KnobDouble> preBlurSigma;
    
    boost::weak_ptr<KnobSeparator> exportDataSep;
    boost::weak_ptr<KnobChoice> exportChoice;
    boost::weak_ptr<KnobButton> exportButton;
    
    NodePtr internalTransformNode;
    
    boost::weak_ptr<KnobChoice> transformType;
    boost::weak_ptr<KnobInt> referenceFrame;
    boost::weak_ptr<KnobButton> setCurrentFrameButton;
    boost::weak_ptr<KnobInt> jitterPeriod;
    boost::weak_ptr<KnobInt> smoothTransform;
    boost::weak_ptr<KnobDouble> translate;
    boost::weak_ptr<KnobDouble> rotate;
    boost::weak_ptr<KnobDouble> scale;
    boost::weak_ptr<KnobBool> scaleUniform;
    boost::weak_ptr<KnobDouble> skewX;
    boost::weak_ptr<KnobDouble> skewY;
    boost::weak_ptr<KnobChoice> skewOrder;
    boost::weak_ptr<KnobDouble> center;
    boost::weak_ptr<KnobBool> invertTransform;
    boost::weak_ptr<KnobChoice> filter;
    boost::weak_ptr<KnobBool> clamp;
    boost::weak_ptr<KnobBool> blackOutside;
    boost::weak_ptr<KnobDouble> motionBlur;
    boost::weak_ptr<KnobDouble> shutter;
    boost::weak_ptr<KnobChoice> shutterOffset;
    boost::weak_ptr<KnobDouble> customShutterOffset;
    
    mutable QMutex trackerContextMutex;
    std::vector<TrackMarkerPtr > markers;
    std::list<TrackMarkerPtr > selectedMarkers,markersToSlave,markersToUnslave;
    int beginSelectionCounter;
    int selectionRecursion;
    
    TrackScheduler<TrackArgsLibMV> scheduler;
    

    
    TrackerContextPrivate(TrackerContext* publicInterface, const boost::shared_ptr<Natron::Node> &node)
    : _publicInterface(publicInterface)
    , node(node)
    , knobs()
    , perTrackKnobs()
    , enableTrackRed()
    , enableTrackGreen()
    , enableTrackBlue()
    , maxError()
    , maxIterations()
    , bruteForcePreTrack()
    , useNormalizedIntensities()
    , preBlurSigma()
    , exportDataSep()
    , exportChoice()
    , exportButton()
    , referenceFrame()
    , trackerContextMutex()
    , markers()
    , selectedMarkers()
    , markersToSlave()
    , markersToUnslave()
    , beginSelectionCounter(0)
    , selectionRecursion(0)
    , scheduler(_publicInterface ,node, trackStepLibMV)
    {
        EffectInstPtr effect = node->getEffectInstance();
        QObject::connect(&scheduler, SIGNAL(trackingStarted(int)), _publicInterface, SIGNAL(trackingStarted(int)));
        QObject::connect(&scheduler, SIGNAL(trackingFinished()), _publicInterface, SIGNAL(trackingFinished()));
        
        QString fixedNamePrefix = QString::fromUtf8(node->getScriptName_mt_safe().c_str());
        fixedNamePrefix.append(QLatin1Char('_'));
        fixedNamePrefix += QLatin1String("Transform");
        
        CreateNodeArgs args(QString::fromUtf8(PLUGINID_OFX_TRANSFORM), eCreateNodeReasonInternal, boost::shared_ptr<NodeCollection>());
        args.fixedName = fixedNamePrefix;
        args.createGui = false;
        args.addToProject = false;
        internalTransformNode = node->getApp()->createNode(args);

        boost::shared_ptr<KnobPage> settingsPage = AppManager::createKnob<KnobPage>(effect.get(), "Controls", 1 , false);
        boost::shared_ptr<KnobPage> transformPage = AppManager::createKnob<KnobPage>(effect.get(), "Transform", 1 , false);
        
        boost::shared_ptr<KnobBool> enableTrackRedKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackRedLabel, 1, false);
        enableTrackRedKnob->setName(kTrackerParamTrackRed);
        enableTrackRedKnob->setHintToolTip(kTrackerParamTrackRedHint);
        enableTrackRedKnob->setDefaultValue(true);
        enableTrackRedKnob->setAnimationEnabled(false);
        enableTrackRedKnob->setAddNewLine(false);
        enableTrackRedKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(enableTrackRedKnob);
        enableTrackRed = enableTrackRedKnob;
        knobs.push_back(enableTrackRedKnob);
        
        boost::shared_ptr<KnobBool> enableTrackGreenKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackGreenLabel, 1, false);
        enableTrackGreenKnob->setName(kTrackerParamTrackGreen);
        enableTrackGreenKnob->setHintToolTip(kTrackerParamTrackGreenHint);
        enableTrackGreenKnob->setDefaultValue(true);
        enableTrackGreenKnob->setAnimationEnabled(false);
        enableTrackGreenKnob->setAddNewLine(false);
        enableTrackGreenKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(enableTrackGreenKnob);
        enableTrackGreen = enableTrackGreenKnob;
        knobs.push_back(enableTrackGreenKnob);
        
        boost::shared_ptr<KnobBool> enableTrackBlueKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackBlueLabel, 1, false);
        enableTrackBlueKnob->setName(kTrackerParamTrackBlue);
        enableTrackBlueKnob->setHintToolTip(kTrackerParamTrackBlueHint);
        enableTrackBlueKnob->setDefaultValue(true);
        enableTrackBlueKnob->setAnimationEnabled(false);
        enableTrackBlueKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(enableTrackBlueKnob);
        enableTrackBlue = enableTrackBlueKnob;
        knobs.push_back(enableTrackBlueKnob);
        
        boost::shared_ptr<KnobDouble> maxErrorKnob = AppManager::createKnob<KnobDouble>(effect.get(), kTrackerParamMaxErrorLabel, 1, false);
        maxErrorKnob->setName(kTrackerParamMaxError);
        maxErrorKnob->setHintToolTip(kTrackerParamMaxErrorHint);
        maxErrorKnob->setAnimationEnabled(false);
        maxErrorKnob->setMinimum(0.);
        maxErrorKnob->setMaximum(1.);
        maxErrorKnob->setDefaultValue(0.2);
        maxErrorKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(maxErrorKnob);
        maxError = maxErrorKnob;
        knobs.push_back(maxErrorKnob);
        
        boost::shared_ptr<KnobInt> maxItKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamMaximumIterationLabel, 1, false);
        maxItKnob->setName(kTrackerParamMaximumIteration);
        maxItKnob->setHintToolTip(kTrackerParamMaximumIterationHint);
        maxItKnob->setAnimationEnabled(false);
        maxItKnob->setMinimum(0);
        maxItKnob->setMaximum(150);
        maxItKnob->setEvaluateOnChange(false);
        maxItKnob->setDefaultValue(50);
        settingsPage->addKnob(maxItKnob);
        maxIterations = maxItKnob;
        knobs.push_back(maxItKnob);
        
        boost::shared_ptr<KnobBool> usePretTrackBF = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamBruteForcePreTrackLabel, 1, false);
        usePretTrackBF->setName(kTrackerParamBruteForcePreTrack);
        usePretTrackBF->setHintToolTip(kTrackerParamBruteForcePreTrackHint);
        usePretTrackBF->setDefaultValue(true);
        usePretTrackBF->setAnimationEnabled(false);
        usePretTrackBF->setEvaluateOnChange(false);
        usePretTrackBF->setAddNewLine(false);
        settingsPage->addKnob(usePretTrackBF);
        bruteForcePreTrack = usePretTrackBF;
        knobs.push_back(usePretTrackBF);
        
        boost::shared_ptr<KnobBool> useNormalizedInt = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamNormalizeIntensitiesLabel, 1, false);
        useNormalizedInt->setName(kTrackerParamNormalizeIntensities);
        useNormalizedInt->setHintToolTip(kTrackerParamNormalizeIntensitiesHint);
        useNormalizedInt->setDefaultValue(false);
        useNormalizedInt->setAnimationEnabled(false);
        useNormalizedInt->setEvaluateOnChange(false);
        settingsPage->addKnob(useNormalizedInt);
        useNormalizedIntensities = useNormalizedInt;
        knobs.push_back(useNormalizedInt);

        boost::shared_ptr<KnobDouble> preBlurSigmaKnob = AppManager::createKnob<KnobDouble>(effect.get(), kTrackerParamPreBlurSigmaLabel, 1, false);
        preBlurSigmaKnob->setName(kTrackerParamPreBlurSigma);
        preBlurSigmaKnob->setHintToolTip(kTrackerParamPreBlurSigmaHint);
        preBlurSigmaKnob->setAnimationEnabled(false);
        preBlurSigmaKnob->setMinimum(0);
        preBlurSigmaKnob->setMaximum(10.);
        preBlurSigmaKnob->setDefaultValue(0.9);
        preBlurSigmaKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(preBlurSigmaKnob);
        preBlurSigma = preBlurSigmaKnob;
        knobs.push_back(preBlurSigmaKnob);
        
        
        boost::shared_ptr<KnobSeparator> exportDataSepKnob = AppManager::createKnob<KnobSeparator>(effect.get(), kTrackerParamExportDataSeparatorLabel, 1, false);
        exportDataSepKnob->setName(kTrackerParamExportDataSeparator);
        settingsPage->addKnob(exportDataSepKnob);
        exportDataSep = exportDataSepKnob;
        knobs.push_back(exportDataSepKnob);
        
        boost::shared_ptr<KnobChoice> exportDataKnob = AppManager::createKnob<KnobChoice>(effect.get(), kTrackerParamExportDataChoiceLabel, 1, false);
        exportDataKnob->setName(kTrackerParamExportDataChoice);
        exportDataKnob->setHintToolTip(kTrackerParamExportDataChoiceHint);
        {
            std::vector<std::string> entries, helps;
            entries.push_back(kTrackerParamExportDataChoiceCPThisFrame);
            helps.push_back(kTrackerParamExportDataChoiceCPThisFrameHint);
            
            entries.push_back(kTrackerParamExportDataChoiceCPRefFrame);
            helps.push_back(kTrackerParamExportDataChoiceCPRefFrameHint);
            
            entries.push_back(kTrackerParamExportDataChoiceCPThisFrameBaked);
            helps.push_back(kTrackerParamExportDataChoiceCPThisFrameHint);
            
            entries.push_back(kTrackerParamExportDataChoiceCPRefFrameBaked);
            helps.push_back(kTrackerParamExportDataChoiceCPRefFrameHint);
            
            entries.push_back(kTrackerParamExportDataChoiceTransformStabilize);
            helps.push_back(kTrackerParamExportDataChoiceTransformStabilizeHint);
            
            entries.push_back(kTrackerParamExportDataChoiceTransformMatchMove);
            helps.push_back(kTrackerParamExportDataChoiceTransformMatchMoveHint);

            entries.push_back(kTrackerParamExportDataChoiceTransformStabilizeBaked);
            helps.push_back(kTrackerParamExportDataChoiceTransformStabilizeHint);
            
            entries.push_back(kTrackerParamExportDataChoiceTransformMatchMoveBaked);
            helps.push_back(kTrackerParamExportDataChoiceTransformMatchMoveHint);
            exportDataKnob->populateChoices(entries,helps);
        }
        exportDataKnob->setAddNewLine(false);
        settingsPage->addKnob(exportDataKnob);
        exportChoice = exportDataKnob;
        knobs.push_back(exportDataKnob);
        
        boost::shared_ptr<KnobButton> exportButtonKnob = AppManager::createKnob<KnobButton>(effect.get(), kTrackerParamExportButtonLabel, 1);
        exportButtonKnob->setName(kTrackerParamExportButton);
        exportButtonKnob->setHintToolTip(kTrackerParamExportButtonHint);
        settingsPage->addKnob(exportButtonKnob);
        exportButton = exportButtonKnob;
        knobs.push_back(exportButtonKnob);
        
        boost::shared_ptr<KnobChoice> transformTypeKnob = AppManager::createKnob<KnobChoice>(effect.get(), kTrackerParamTransformTypeLabel, 1);
        transformTypeKnob->setName(kTrackerParamTransformType);
        transformTypeKnob->setHintToolTip(kTrackerParamTransformTypeHint);
        {
            std::vector<std::string> choices,helps;
            choices.push_back(kTrackerParamTransformTypeNone);
            helps.push_back(kTrackerParamTransformTypeNoneHelp);
            choices.push_back(kTrackerParamTransformTypeStabilize);
            helps.push_back(kTrackerParamTransformTypeStabilizeHelp);
            choices.push_back(kTrackerParamTransformTypeMatchMove);
            helps.push_back(kTrackerParamTransformTypeMatchMoveHelp);
            choices.push_back(kTrackerParamTransformTypeRemoveJitter);
            helps.push_back(kTrackerParamTransformTypeRemoveJitterHelp);
            choices.push_back(kTrackerParamTransformTypeAddJitter);
            helps.push_back(kTrackerParamTransformTypeAddJitterHelp);

            transformTypeKnob->populateChoices(choices,helps);
        }
        transformType = transformTypeKnob;
        transformPage->addKnob(transformTypeKnob);

        
        boost::shared_ptr<KnobInt> referenceFrameKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamReferenceFrameLabel, 1);
        referenceFrameKnob->setName(kTrackerParamReferenceFrame);
        referenceFrameKnob->setHintToolTip(kTrackerParamReferenceFrameHint);
        referenceFrameKnob->setAnimationEnabled(false);
        referenceFrameKnob->setDefaultValue(0);
        referenceFrameKnob->setAddNewLine(false);
        referenceFrameKnob->setEvaluateOnChange(false);
        transformPage->addKnob(referenceFrameKnob);
        referenceFrame = referenceFrameKnob;
        knobs.push_back(referenceFrameKnob);
        
        boost::shared_ptr<KnobButton> setCurrentFrameKnob = AppManager::createKnob<KnobButton>(effect.get(), kTrackerParamSetReferenceFrameLabel, 1);
        setCurrentFrameKnob->setName(kTrackerParamSetReferenceFrame);
        setCurrentFrameKnob->setHintToolTip(kTrackerParamSetReferenceFrameHint);
        transformPage->addKnob(setCurrentFrameKnob);
        setCurrentFrameButton = setCurrentFrameKnob;
        
        boost::shared_ptr<KnobInt>  jitterPeriodKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamJitterPeriodLabel, 1);
        jitterPeriodKnob->setName(kTrackerParamJitterPeriod);
        jitterPeriodKnob->setHintToolTip(kTrackerParamJitterPeriodHint);
        jitterPeriodKnob->setAnimationEnabled(false);
        jitterPeriodKnob->setDefaultValue(10);
        jitterPeriodKnob->setMinimum(0, 0);
        jitterPeriodKnob->setEvaluateOnChange(false);
        transformPage->addKnob(jitterPeriodKnob);
        jitterPeriod = jitterPeriodKnob;
        knobs.push_back(jitterPeriodKnob);
        
        boost::shared_ptr<KnobInt>  smoothTransformKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamSmoothLabel, 3);
        smoothTransformKnob->setName(kTrackerParamSmooth);
        smoothTransformKnob->setHintToolTip(kTrackerParamSmoothHint);
        smoothTransformKnob->setAnimationEnabled(false);
        smoothTransformKnob->disableSlider();
        smoothTransformKnob->setDimensionName(0, "t");
        smoothTransformKnob->setDimensionName(1, "r");
        smoothTransformKnob->setDimensionName(2, "s");
        for (int i = 0;i < 3; ++i) {
            smoothTransformKnob->setMinimum(0, i);
        }
        smoothTransformKnob->setEvaluateOnChange(false);
        transformPage->addKnob(smoothTransformKnob);
        smoothTransform = smoothTransformKnob;
        knobs.push_back(smoothTransformKnob);
        
        KnobPtr translateKnob = internalTransformNode->getKnobByName(kTransformParamTranslate);
        assert(translateKnob);
        KnobPtr translateK = translateKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, translateKnob->getName(), translateKnob->getLabel(), translateKnob->getHintToolTip(), false, false);
        translate = boost::dynamic_pointer_cast<KnobDouble>(translateK);
        
        KnobPtr rotateKnob = internalTransformNode->getKnobByName(kTransformParamRotate);
        assert(rotateKnob);
        KnobPtr rotateK = rotateKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, rotateKnob->getName(), rotateKnob->getLabel(), rotateKnob->getHintToolTip(), false, false);
        rotate = boost::dynamic_pointer_cast<KnobDouble>(rotateK);
        
        KnobPtr scaleKnob = internalTransformNode->getKnobByName(kTransformParamScale);
        assert(scaleKnob);
        KnobPtr scaleK = scaleKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, scaleKnob->getName(), scaleKnob->getLabel(), scaleKnob->getHintToolTip(), false, false);
        scaleK->setAddNewLine(false);
        scale = boost::dynamic_pointer_cast<KnobDouble>(scaleK);
        
        KnobPtr scaleUniformKnob = internalTransformNode->getKnobByName(kTransformParamUniform);
        assert(scaleUniformKnob);
        KnobPtr scaleUniK = scaleUniformKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, scaleUniformKnob->getName(), scaleUniformKnob->getLabel(), scaleKnob->getHintToolTip(), false, false);
        scaleUniform = boost::dynamic_pointer_cast<KnobBool>(scaleUniK);
        
        KnobPtr skewXKnob = internalTransformNode->getKnobByName(kTransformParamSkewX);
        assert(skewXKnob);
        KnobPtr skewXK = skewXKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, skewXKnob->getName(), skewXKnob->getLabel(), skewXKnob->getHintToolTip(), false, false);
        skewX = boost::dynamic_pointer_cast<KnobDouble>(skewXK);
        
        KnobPtr skewYKnob = internalTransformNode->getKnobByName(kTransformParamSkewY);
        assert(skewYKnob);
        KnobPtr skewYK = skewYKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, skewYKnob->getName(), skewYKnob->getLabel(), skewYKnob->getHintToolTip(), false, false);
        skewY = boost::dynamic_pointer_cast<KnobDouble>(skewYK);
        
        KnobPtr skewOrderKnob = internalTransformNode->getKnobByName(kTransformParamSkewOrder);
        assert(skewOrderKnob);
        KnobPtr skewOrderK = skewOrderKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, skewOrderKnob->getName(), skewOrderKnob->getLabel(), skewOrderKnob->getHintToolTip(), false, false);
        skewOrder = boost::dynamic_pointer_cast<KnobChoice>(skewOrderK);
        
        KnobPtr centerKnob = internalTransformNode->getKnobByName(kTransformParamCenter);
        assert(centerKnob);
        KnobPtr centerK = centerKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, centerKnob->getName(), centerKnob->getLabel(), centerKnob->getHintToolTip(), false, false);
        center = boost::dynamic_pointer_cast<KnobDouble>(centerK);
        
        KnobPtr invertKnob = internalTransformNode->getKnobByName(kTransformParamInvert);
        assert(invertKnob);
        KnobPtr invertK = invertKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, invertKnob->getName(), invertKnob->getLabel(), invertKnob->getHintToolTip(), false, false);
        invertTransform = boost::dynamic_pointer_cast<KnobBool>(invertK);

        
        KnobPtr filterKnob = internalTransformNode->getKnobByName(kTransformParamFilter);
        assert(filterKnob);
        KnobPtr filterK = filterKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, filterKnob->getName(), filterKnob->getLabel(), filterKnob->getHintToolTip(), false, false);
        filterK->setAddNewLine(false);
        filter = boost::dynamic_pointer_cast<KnobChoice>(filterK);
        
        KnobPtr clampKnob = internalTransformNode->getKnobByName(kTransformParamClamp);
        assert(clampKnob);
        KnobPtr clampK = clampKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, clampKnob->getName(), clampKnob->getLabel(), clampKnob->getHintToolTip(), false, false);
        clampK->setAddNewLine(false);
        clamp = boost::dynamic_pointer_cast<KnobBool>(clampK);
        
        KnobPtr boKnob = internalTransformNode->getKnobByName(kTransformParamBlackOutside);
        assert(boKnob);
        KnobPtr boK = boKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, boKnob->getName(), boKnob->getLabel(), boKnob->getHintToolTip(), false, false);
        blackOutside = boost::dynamic_pointer_cast<KnobBool>(boK);
        
        KnobPtr mbKnob = internalTransformNode->getKnobByName(kTransformParamMotionBlur);
        assert(mbKnob);
        KnobPtr mbK = mbKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, mbKnob->getName(), mbKnob->getLabel(), mbKnob->getHintToolTip(), false, false);
        motionBlur = boost::dynamic_pointer_cast<KnobDouble>(mbK);
        
        KnobPtr shutterKnob = internalTransformNode->getKnobByName(kTransformParamShutter);
        assert(shutterKnob);
        KnobPtr shutterK = shutterKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, shutterKnob->getName(), shutterKnob->getLabel(), shutterKnob->getHintToolTip(), false, false);
        shutter = boost::dynamic_pointer_cast<KnobDouble>(shutterK);
        
        KnobPtr shutterOffKnob = internalTransformNode->getKnobByName(kTransformParamShutterOffset);
        assert(shutterOffKnob);
        KnobPtr shutterOffK = shutterOffKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, shutterOffKnob->getName(), shutterOffKnob->getLabel(), shutterOffKnob->getHintToolTip(), false, false);
        shutterOffK->setAddNewLine(false);
        shutterOffset = boost::dynamic_pointer_cast<KnobChoice>(shutterOffK);
        
        KnobPtr customShutterKnob = internalTransformNode->getKnobByName(kTransformParamCustomShutterOffset);
        assert(customShutterKnob);
        KnobPtr customShutterK = customShutterKnob->createDuplicateOnNode(effect.get(), transformPage, boost::shared_ptr<KnobGroup>(), -1, true, customShutterKnob->getName(), customShutterKnob->getLabel(), customShutterKnob->getHintToolTip(), false, false);
        customShutterOffset = boost::dynamic_pointer_cast<KnobDouble>(customShutterK);
        
        node->addTransformInteract(translate.lock(), scale.lock(), scaleUniform.lock(), rotate.lock(), skewX.lock(), skewY.lock(), skewOrder.lock(), center.lock());
    }
    

    
    /// Make all calls to getValue() that are global to the tracker context in here
    void beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const;
    
    /// Make all calls to getValue() that are local to the track in here
    void endLibMVOptionsForTrack(const TrackMarker& marker,
                                  mv::TrackRegionOptions* options) const;
    
    void addToSelectionList(const TrackMarkerPtr& marker)
    {
        if (std::find(selectedMarkers.begin(), selectedMarkers.end(), marker) != selectedMarkers.end()) {
            return;
        }
        selectedMarkers.push_back(marker);
        markersToSlave.push_back(marker);
    }
    
    void removeFromSelectionList(const TrackMarkerPtr& marker)
    {
        std::list<TrackMarkerPtr >::iterator found = std::find(selectedMarkers.begin(), selectedMarkers.end(), marker);
        if (found == selectedMarkers.end()) {
            return;
        }
        selectedMarkers.erase(found);
        markersToUnslave.push_back(marker);
    }
    
    void incrementSelectionCounter()
    {
        ++beginSelectionCounter;
    }
    
    void decrementSelectionCounter()
    {
        if (beginSelectionCounter > 0) {
            --beginSelectionCounter;
        }
    }
    
    void linkMarkerKnobsToGuiKnobs(const std::list<TrackMarkerPtr >& markers,
                                   bool multipleTrackSelected,
                                   bool slave);
    
    void createCornerPinFromSelection(const std::list<TrackMarkerPtr > & selection,
                                      bool linked,
                                      bool useTransformRefFrame,
                                      bool invert);
    
    void createTransformFromSelection(const std::list<TrackMarkerPtr > & selection,
                                      bool linked,
                                      bool invert);
    
    
    
};

TrackerContext::TrackerContext(const boost::shared_ptr<Natron::Node> &node)
: boost::enable_shared_from_this<TrackerContext>()
, _imp(new TrackerContextPrivate(this, node))
{
    
}

TrackerContext::~TrackerContext()
{
    
}

void
TrackerContext::load(const TrackerContextSerialization& serialization)
{
    
    boost::shared_ptr<TrackerContext> thisShared = shared_from_this();
    QMutexLocker k(&_imp->trackerContextMutex);

    for (std::list<TrackSerialization>::const_iterator it = serialization._tracks.begin(); it != serialization._tracks.end(); ++it) {
        TrackMarkerPtr marker(new TrackMarker(thisShared));
        marker->load(*it);
        _imp->markers.push_back(marker);
    }
}

void
TrackerContext::save(TrackerContextSerialization* serialization) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        TrackSerialization s;
        _imp->markers[i]->save(&s);
        serialization->_tracks.push_back(s);
    }
}

int
TrackerContext::getTransformReferenceFrame() const
{
    return _imp->referenceFrame.lock()->getValue();
}


void
TrackerContext::goToPreviousKeyFrame(int time)
{
    std::list<TrackMarkerPtr > markers;
    getSelectedMarkers(&markers);
    
    int minimum = INT_MIN;
    for (std::list<TrackMarkerPtr > ::iterator it = markers.begin(); it != markers.end(); ++it) {
        int t = (*it)->getPreviousKeyframe(time);
        if ( (t != INT_MIN) && (t > minimum) ) {
            minimum = t;
        }
    }
    if (minimum != INT_MIN) {
        getNode()->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());
        getNode()->getApp()->getTimeLine()->seekFrame(minimum, false,  NULL, Natron::eTimelineChangeReasonPlaybackSeek);
    }
}

void
TrackerContext::goToNextKeyFrame(int time)
{
    std::list<TrackMarkerPtr > markers;
    getSelectedMarkers(&markers);
    
    int maximum = INT_MAX;
    for (std::list<TrackMarkerPtr > ::iterator it = markers.begin(); it != markers.end(); ++it) {
        int t = (*it)->getNextKeyframe(time);
        if ( (t != INT_MAX) && (t < maximum) ) {
            maximum = t;
        }
    }
    if (maximum != INT_MAX) {
        getNode()->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());
        getNode()->getApp()->getTimeLine()->seekFrame(maximum, false,  NULL, Natron::eTimelineChangeReasonPlaybackSeek);
    }
}

/*boost::shared_ptr<KnobDouble>
TrackerContext::getSearchWindowBottomLeftKnob() const
{
    return _imp->searchWindowBtmLeft.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getSearchWindowTopRightKnob() const
{
    return _imp->searchWindowTopRight.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getPatternTopLeftKnob() const
{
    return _imp->patternTopLeft.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getPatternTopRightKnob() const
{
    return _imp->patternTopRight.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getPatternBtmRightKnob() const
{
    return _imp->patternBtmRight.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getPatternBtmLeftKnob() const
{
    return _imp->patternBtmLeft.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getWeightKnob() const
{
    return _imp->weight.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getCenterKnob() const
{
    return _imp->center.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getOffsetKnob() const
{
    return _imp->offset.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getCorrelationKnob() const
{
    return _imp->correlation.lock();
}

boost::shared_ptr<KnobChoice>
TrackerContext::getMotionModelKnob() const
{
    return _imp->motionModel.lock();
}*/

TrackMarkerPtr
TrackerContext::getMarkerByName(const std::string & name) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::vector<TrackMarkerPtr >::const_iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
        if ((*it)->getScriptName_mt_safe() == name) {
            return *it;
        }
    }
    return TrackMarkerPtr();
}

std::string
TrackerContext::generateUniqueTrackName(const std::string& baseName)
{
    int no = 1;
    
    bool foundItem;
    std::string name;
    do {
        std::stringstream ss;
        ss << baseName;
        ss << no;
        name = ss.str();
        if (getMarkerByName(name)) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);
    return name;
}

TrackMarkerPtr
TrackerContext::createMarker()
{
    TrackMarkerPtr track(new TrackMarker(shared_from_this()));
    std::string name = generateUniqueTrackName(kTrackBaseName);
    track->setScriptName(name);
    track->setLabel(name);
    track->resetCenter();
    int index;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        index  = _imp->markers.size();
        _imp->markers.push_back(track);
    }
    declareItemAsPythonField(track);
    Q_EMIT trackInserted(track, index);
    return track;
    
}

int
TrackerContext::getMarkerIndex(const TrackMarkerPtr& marker) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if (_imp->markers[i] == marker) {
            return (int)i;
        }
    }
    return -1;
}

TrackMarkerPtr
TrackerContext::getPrevMarker(const TrackMarkerPtr& marker, bool loop) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if (_imp->markers[i] == marker) {
            if (i > 0) {
                return _imp->markers[i - 1];
            }
        }
    }
    return (_imp->markers.size() == 0 || !loop) ? TrackMarkerPtr() : _imp->markers[_imp->markers.size() - 1];
}

TrackMarkerPtr
TrackerContext::getNextMarker(const TrackMarkerPtr& marker, bool loop) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if (_imp->markers[i] == marker) {
            if (i < (_imp->markers.size() - 1)) {
                return _imp->markers[i + 1];
            } else if (!loop) {
                return TrackMarkerPtr();
            }
        }
    }
    return (_imp->markers.size() == 0 || !loop || _imp->markers[0] == marker) ? TrackMarkerPtr() : _imp->markers[0];
}

void
TrackerContext::appendMarker(const TrackMarkerPtr& marker)
{
    int index;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        index = _imp->markers.size();
        _imp->markers.push_back(marker);
    }
    declareItemAsPythonField(marker);
    Q_EMIT trackInserted(marker, index);
}

void
TrackerContext::insertMarker(const TrackMarkerPtr& marker, int index)
{
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        assert(index >= 0);
        if (index >= (int)_imp->markers.size()) {
            _imp->markers.push_back(marker);
        } else {
            std::vector<TrackMarkerPtr >::iterator it = _imp->markers.begin();
            std::advance(it, index);
            _imp->markers.insert(it, marker);
        }
    }
    declareItemAsPythonField(marker);
    Q_EMIT trackInserted(marker,index);
    
}

void
TrackerContext::removeMarker(const TrackMarkerPtr& marker)
{
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        for (std::vector<TrackMarkerPtr >::iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
            if (*it == marker) {
                _imp->markers.erase(it);
                break;
            }
        }
    }
    removeItemAsPythonField(marker);
    beginEditSelection();
    removeTrackFromSelection(marker, TrackerContext::eTrackSelectionInternal);
    endEditSelection(TrackerContext::eTrackSelectionInternal);
    Q_EMIT trackRemoved(marker);
}

boost::shared_ptr<Natron::Node>
TrackerContext::getNode() const
{
    return _imp->node.lock();
}

int
TrackerContext::getTimeLineFirstFrame() const
{
    boost::shared_ptr<Natron::Node> node = getNode();
    if (!node) {
        return -1;
    }
    double first,last;
    node->getApp()->getProject()->getFrameRange(&first, &last);
    return (int)first;
}

int
TrackerContext::getTimeLineLastFrame() const
{
    boost::shared_ptr<Natron::Node> node = getNode();
    if (!node) {
        return -1;
    }
    double first,last;
    node->getApp()->getProject()->getFrameRange(&first, &last);
    return (int)last;
}

void
TrackerContextPrivate::beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const
{
    options->minimum_correlation = 1. - maxError.lock()->getValue();
    assert(options->minimum_correlation >= 0. && options->minimum_correlation <= 1.);
    options->max_iterations = maxIterations.lock()->getValue();
    options->use_brute_initialization = bruteForcePreTrack.lock()->getValue();
    options->use_normalized_intensities = useNormalizedIntensities.lock()->getValue();
    options->sigma = preBlurSigma.lock()->getValue();

}

void
TrackerContextPrivate::endLibMVOptionsForTrack(const TrackMarker& marker,
                              mv::TrackRegionOptions* options) const
{
    int mode_i = marker.getMotionModelKnob()->getValue();
    switch (mode_i) {
        case 0:
            options->mode = mv::TrackRegionOptions::TRANSLATION;
            break;
        case 1:
            options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION;
            break;
        case 2:
            options->mode = mv::TrackRegionOptions::TRANSLATION_SCALE;
            break;
        case 3:
            options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION_SCALE;
            break;
        case 4:
            options->mode = mv::TrackRegionOptions::AFFINE;
            break;
        case 5:
            options->mode = mv::TrackRegionOptions::HOMOGRAPHY;
            break;
        default:
            options->mode = mv::TrackRegionOptions::AFFINE;
            break;
    }
}

struct FrameAccessorCacheKey
{
    int frame;
    int mipMapLevel;
    mv::FrameAccessor::InputMode mode;
};

struct CacheKey_compare_less
{
    bool operator() (const FrameAccessorCacheKey & lhs,
                     const FrameAccessorCacheKey & rhs) const
    {
        if (lhs.frame < rhs.frame) {
            return true;
        } else if (lhs.frame > rhs.frame) {
            return false;
        } else {
            if (lhs.mipMapLevel < rhs.mipMapLevel) {
                return true;
            } else if (lhs.mipMapLevel > rhs.mipMapLevel) {
                return false;
            } else {
                if ((int)lhs.mode < (int)rhs.mode) {
                    return true;
                } else if ((int)lhs.mode > (int)rhs.mode) {
                    return false;
                } else {
                    return false;
                }
            }
                
        }
    }
};


class MvFloatImage : public libmv::Array3D<float>
{
    
public:
    
    MvFloatImage()
    : libmv::Array3D<float>()
    {
        
    }
    
    MvFloatImage(int height, int width)
    : libmv::Array3D<float>(height, width)
    {
        
    }
    
    MvFloatImage(float* data, int height, int width)
    : libmv::Array3D<float>(data, height, width)
    {
        
    }
    
    virtual ~MvFloatImage()
    {
        
    }
};

struct FrameAccessorCacheEntry
{
    boost::shared_ptr<MvFloatImage> image;
    
    // If null, this is the full image
    RectI bounds;
    
    unsigned int referenceCount;
};

class FrameAccessorImpl : public mv::FrameAccessor
{
    const TrackerContext* _context;
    boost::shared_ptr<Natron::Node> _trackerInput;
    typedef std::multimap<FrameAccessorCacheKey, FrameAccessorCacheEntry, CacheKey_compare_less > FrameAccessorCache;
    
    mutable QMutex _cacheMutex;
    FrameAccessorCache _cache;
    bool _enabledChannels[3];
    int _formatHeight;
    
public:
    
    FrameAccessorImpl(const TrackerContext* context,
                      bool enabledChannels[3],
                      int formatHeight)
    : _context(context)
    , _trackerInput()
    , _cacheMutex()
    , _cache()
    , _enabledChannels()
    , _formatHeight(formatHeight)
    {
        _trackerInput = context->getNode()->getInput(0);
        assert(_trackerInput);
        for (int i = 0; i < 3; ++i) {
            _enabledChannels[i] = enabledChannels[i];
        }
    }
    
    virtual ~FrameAccessorImpl()
    {
        
    }
    
    void getEnabledChannels(bool* r, bool* g, bool* b) const
    {
        *r = _enabledChannels[0];
        *g = _enabledChannels[1];
        *b = _enabledChannels[2];
    }
    
    // Get a possibly-filtered version of a frame of a video. Downscale will
    // cause the input image to get downscaled by 2^downscale for pyramid access.
    // Region is always in original-image coordinates, and describes the
    // requested area. The transform describes an (optional) transform to apply
    // to the image before it is returned.
    //
    // When done with an image, you must call ReleaseImage with the returned key.
    virtual mv::FrameAccessor::Key GetImage(int clip,
                                            int frame,
                                            mv::FrameAccessor::InputMode input_mode,
                                            int downscale,               // Downscale by 2^downscale.
                                            const mv::Region* region,        // Get full image if NULL.
                                            const mv::FrameAccessor::Transform* transform,  // May be NULL.
                                            mv::FloatImage* destination) OVERRIDE FINAL;
    
    // Releases an image from the frame accessor. Non-caching implementations may
    // free the image immediately; others may hold onto the image.
    virtual void ReleaseImage(Key) OVERRIDE FINAL;
    
    virtual bool GetClipDimensions(int clip, int* width, int* height) OVERRIDE FINAL;
    virtual int NumClips() OVERRIDE FINAL;
    virtual int NumFrames(int clip) OVERRIDE FINAL;
    
private:
    
    
};

void
TrackArgsLibMV::getEnabledChannels(bool* r, bool* g, bool* b) const
{
    _fa->getEnabledChannels(r,g,b);
}


template <bool doR, bool doG, bool doB>
void natronImageToLibMvFloatImageForChannels(const Natron::Image* source,
                                             const RectI& roi,
                                             MvFloatImage& mvImg)
{
    //mvImg is expected to have its bounds equal to roi
    
    Natron::Image::ReadAccess racc(source);
    
    unsigned int compsCount = source->getComponentsCount();
    assert(compsCount == 3);
    unsigned int srcRowElements = source->getRowElements();
    
    assert(source->getBounds().contains(roi));
    const float* src_pixels = (const float*)racc.pixelAt(roi.x1, roi.y2 - 1);
    assert(src_pixels);
    float* dst_pixels = mvImg.Data();
    assert(dst_pixels);
    //LibMV images have their origin in the top left hand corner
    
    // It's important to rescale the resultappropriately so that e.g. if only
    // blue is selected, it's not zeroed out.
    float scale = (doR ? 0.2126f : 0.0f) +
    (doG ? 0.7152f : 0.0f) +
    (doB ? 0.0722f : 0.0f);
    
    int h = roi.height();
    int w = roi.width();
    for (int y = 0; y < h ; ++y,
         src_pixels -= (srcRowElements + compsCount * w)) {
        
        for (int x = 0; x < w; ++x,
             src_pixels += compsCount,
             ++dst_pixels) {
            
            /// Apply luminance conversion while we copy the image
            /// This code is taken from DisableChannelsTransform::run in libmv/autotrack/autotrack.cc
            *dst_pixels = (0.2126f * (doR ? src_pixels[0] : 0.0f) +
                           0.7152f * (doG ? src_pixels[1] : 0.0f) +
                           0.0722f * (doB ? src_pixels[2] : 0.0f)) / scale;
        }
    }
}


void natronImageToLibMvFloatImage(bool enabledChannels[3],
                                  const Natron::Image* source,
                                  const RectI& roi,
                                  MvFloatImage& mvImg)
{
    
    if (enabledChannels[0]) {
        if (enabledChannels[1]) {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<true,true,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<true,true,false>(source,roi,mvImg);
            }
        } else {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<true,false,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<true,false,false>(source,roi,mvImg);
            }
        }
        
    } else {
        if (enabledChannels[1]) {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<false,true,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<false,true,false>(source,roi,mvImg);
            }
        } else {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<false,false,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<false,false,false>(source,roi,mvImg);
            }
        }
    }
}

/*
 * @brief This is called by LibMV to retrieve an image either for reference or as search frame.
 */
mv::FrameAccessor::Key
FrameAccessorImpl::GetImage(int /*clip*/,
                            int frame,
                            mv::FrameAccessor::InputMode input_mode,
                            int downscale,               // Downscale by 2^downscale.
                            const mv::Region* region,        // Get full image if NULL.
                            const mv::FrameAccessor::Transform* /*transform*/,  // May be NULL.
                            mv::FloatImage* destination)
{
    
    // Since libmv only uses MONO images for now we have only optimized for this case, remove and handle properly
    // other case(s) when they get integrated into libmv.
    assert(input_mode == mv::FrameAccessor::MONO);
    
    
    
    FrameAccessorCacheKey key;
    key.frame = frame;
    key.mipMapLevel = downscale;
    key.mode = input_mode;
    
    /*
     Check if a frame exists in the cache with matching key and bounds enclosing the given region
     */
    RectI roi;
    if (region) {
        convertLibMVRegionToRectI(*region,_formatHeight, &roi);

        QMutexLocker k(&_cacheMutex);
        std::pair<FrameAccessorCache::iterator,FrameAccessorCache::iterator> range = _cache.equal_range(key);
        for (FrameAccessorCache::iterator it = range.first; it != range.second; ++it) {
            if (roi.x1 >= it->second.bounds.x1 && roi.x2 <= it->second.bounds.x2 &&
                roi.y1 >= it->second.bounds.y1 && roi.y2 <= it->second.bounds.y2) {
                // LibMV is kinda dumb on this we must necessarily copy the data either via CopyFrom or the
                // assignment constructor
#ifdef TRACE_LIB_MV
                qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Found cached image at frame" << frame << "with RoI x1="
                << region->min(0) << "y1=" << region->max(1) << "x2=" << region->max(0) << "y2=" << region->min(1);
#endif
                destination->CopyFrom<float>(*it->second.image);
                ++it->second.referenceCount;
                return (mv::FrameAccessor::Key)it->second.image.get();
            }
            
        }
    }
    
    // Not in accessor cache, call renderRoI
    RenderScale scale;
    scale.y = scale.x = Image::getScaleFromMipMapLevel((unsigned int)downscale);
    
    
    RectD precomputedRoD;
    if (!region) {
        bool isProjectFormat;
        Natron::StatusEnum stat = _trackerInput->getEffectInstance()->getRegionOfDefinition_public(_trackerInput->getHashValue(), frame, scale, ViewIdx(0), &precomputedRoD, &isProjectFormat);
        if (stat == eStatusFailed) {
            return (mv::FrameAccessor::Key)0;
        }
        double par = _trackerInput->getEffectInstance()->getAspectRatio(-1);
        precomputedRoD.toPixelEnclosing((unsigned int)downscale, par, &roi);
    }
    
    std::list<ImageComponents> components;
    components.push_back(ImageComponents::getRGBComponents());
    
    NodePtr node = _context->getNode();
    
    AbortableRenderInfoPtr abortInfo(new AbortableRenderInfo(false, 0));
    ParallelRenderArgsSetter frameRenderArgs(frame,
                                             ViewIdx(0), //<  view 0 (left)
                                             true, //<isRenderUserInteraction
                                             false, //isSequential
                                             abortInfo, //abort info
                                             node, //  requester
                                             0, //texture index
                                             node->getApp()->getTimeLine().get(), //Timeline
                                             NodePtr(), // rotoPaintNode
                                             true, //isAnalysis
                                             false,//draftMode
                                             false, //viewer progress
                                             boost::shared_ptr<RenderStats>()); // Stats

    
    EffectInstance::RenderRoIArgs args(frame,
                                       scale,
                                       downscale,
                                       ViewIdx(0),
                                       false,
                                       roi,
                                       precomputedRoD,
                                       components,
                                       Natron::eImageBitDepthFloat,
                                       true,
                                       _context->getNode()->getEffectInstance().get());
    std::map<ImageComponents,ImagePtr> planes;
    EffectInstance::RenderRoIRetCode stat = _trackerInput->getEffectInstance()->renderRoI(args, &planes);
    if (stat != EffectInstance::eRenderRoIRetCodeOk || planes.empty()) {
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Failed to call renderRoI on input at frame" << frame << "with RoI x1="
        << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2;
#endif
        return (mv::FrameAccessor::Key)0;
    }
    
    assert(!planes.empty());
    const ImagePtr& sourceImage = planes.begin()->second;
    RectI sourceBounds = sourceImage->getBounds();
    RectI intersectedRoI;
    if (!roi.intersect(sourceBounds, &intersectedRoI)) {
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "RoI does not intersect the source image bounds (RoI x1="
        << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2 << ")";
#endif
        return (mv::FrameAccessor::Key)0;

    }
    
#ifdef TRACE_LIB_MV
    qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "renderRoi (frame" << frame << ") OK  (BOUNDS= x1="
    << sourceBounds.x1 << "y1=" << sourceBounds.y1 << "x2=" << sourceBounds.x2 << "y2=" << sourceBounds.y2 << ") (ROI = " << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2 << ")";
#endif
    
    /*
     Copy the Natron image to the LivMV float image
     */
    FrameAccessorCacheEntry entry;
    entry.image.reset(new MvFloatImage(intersectedRoI.height(), intersectedRoI.width()));
    entry.bounds = intersectedRoI;
    entry.referenceCount = 1;
    natronImageToLibMvFloatImage(_enabledChannels,
                                 sourceImage.get(),
                                 intersectedRoI,
                                 *entry.image);
    // we ignore the transform parameter and do it in natronImageToLibMvFloatImage instead
    
    // LibMV is kinda dumb on this we must necessarily copy the data...
    destination->CopyFrom<float>(*entry.image);
    
    //insert into the cache
    {
        QMutexLocker k(&_cacheMutex);
        _cache.insert(std::make_pair(key,entry));
    }
#ifdef TRACE_LIB_MV
    qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Rendered frame" << frame << "with RoI x1="
    << intersectedRoI.x1 << "y1=" << intersectedRoI.y1 << "x2=" << intersectedRoI.x2 << "y2=" << intersectedRoI.y2;
#endif
    return (mv::FrameAccessor::Key)entry.image.get();
}

void
FrameAccessorImpl::ReleaseImage(Key key)
{
    MvFloatImage* imgKey = (MvFloatImage*)key;
    QMutexLocker k(&_cacheMutex);
    for (FrameAccessorCache::iterator it = _cache.begin(); it != _cache.end(); ++it) {
        if (it->second.image.get() == imgKey) {
            --it->second.referenceCount;
            if (!it->second.referenceCount) {
                _cache.erase(it);
                return;
            }
        }
    }
}

// Not used in LibMV
bool
FrameAccessorImpl::GetClipDimensions(int /*clip*/, int* /*width*/, int* /*height*/)
{
    return false;
}

// Only used in AutoTrack::DetectAndTrack which is w.i.p in LibMV so don't bother implementing it
int
FrameAccessorImpl::NumClips()
{
    return 1;
}

// Only used in AutoTrack::DetectAndTrack which is w.i.p in LibMV so don't bother implementing it
int
FrameAccessorImpl::NumFrames(int /*clip*/)
{
    return 0;
}

void
TrackerContext::trackMarkers(const std::list<TrackMarkerPtr >& markers,
                             int start,
                             int end,
                             bool forward,
                             ViewerInstance* viewer)
{
    
    if (markers.empty()) {
        return;
    }
    
    /// The channels we are going to use for tracking
    bool enabledChannels[3];
    enabledChannels[0] = _imp->enableTrackRed.lock()->getValue();
    enabledChannels[1] = _imp->enableTrackGreen.lock()->getValue();
    enabledChannels[2] = _imp->enableTrackBlue.lock()->getValue();
    
    double formatWidth,formatHeight;
    Format f;
    getNode()->getApp()->getProject()->getProjectDefaultFormat(&f);
    formatWidth = f.width();
    formatHeight = f.height();
    
    /// The accessor and its cache is local to a track operation, it is wiped once the whole sequence track is finished.
    boost::shared_ptr<FrameAccessorImpl> accessor(new FrameAccessorImpl(this, enabledChannels, formatHeight));
    boost::shared_ptr<mv::AutoTrack> trackContext(new mv::AutoTrack(accessor.get()));
    
    
    
    std::vector<boost::shared_ptr<TrackMarkerAndOptions> > trackAndOptions;
    
    mv::TrackRegionOptions mvOptions;
    /*
     Get the global parameters for the LivMV track: pre-blur sigma, No iterations, normalized intensities, etc...
     */
    _imp->beginLibMVOptionsForTrack(&mvOptions);
    
    /*
     For the given markers, do the following:
     - Get the "User" keyframes that have been set and create a LibMV marker for each keyframe as well as for the "start" time
     - Set the "per-track" parameters that were given by the user, that is the mv::TrackRegionOptions
     - t->mvMarker will contain the marker that evolves throughout the tracking
     */
    int trackIndex = 0;
    for (std::list<TrackMarkerPtr >::const_iterator it = markers.begin(); it!= markers.end(); ++it, ++trackIndex) {
        boost::shared_ptr<TrackMarkerAndOptions> t(new TrackMarkerAndOptions);
        t->natronMarker = *it;
        
        std::set<int> userKeys;
        t->natronMarker->getUserKeyframes(&userKeys);
        
        // Add a libmv marker for all keyframes
        for (std::set<int>::iterator it2 = userKeys.begin(); it2 != userKeys.end(); ++it2) {
            
            if (*it2 == start) {
                //Will be added in the track step
                continue;
            } else {
                mv::Marker marker;
                natronTrackerToLibMVTracker(false, enabledChannels, *t->natronMarker, trackIndex, *it2, forward, formatHeight, &marker);
                assert(marker.source == mv::Marker::MANUAL);
                trackContext->AddMarker(marker);
            }
            
            
        }
        
        
        //For all already tracked frames which are not keyframes, add them to the AutoTrack too
        std::set<double> centerKeys;
        t->natronMarker->getCenterKeyframes(&centerKeys);
        for (std::set<double>::iterator it2 = centerKeys.begin(); it2 != centerKeys.end(); ++it2) {
            if (userKeys.find(*it2) != userKeys.end()) {
                continue;
            }
            if (*it2 == start) {
                //Will be added in the track step
                continue;
            } else {
                mv::Marker marker;
                natronTrackerToLibMVTracker(false, enabledChannels, *t->natronMarker, trackIndex, *it2, forward, formatHeight, &marker);
                assert(marker.source == mv::Marker::TRACKED);
                trackContext->AddMarker(marker);
            }
        }
        
        
        
        
        t->mvOptions = mvOptions;
        _imp->endLibMVOptionsForTrack(*t->natronMarker, &t->mvOptions);
        trackAndOptions.push_back(t);
    }
    
    
    /*
     Launch tracking in the scheduler thread.
     */
    TrackArgsLibMV args(start, end, forward, getNode()->getApp()->getTimeLine(), viewer, trackContext, accessor, trackAndOptions,formatWidth,formatHeight);
    _imp->scheduler.track(args);
}

void
TrackerContext::trackSelectedMarkers(int start, int end, bool forward, ViewerInstance* viewer)
{
    std::list<TrackMarkerPtr > markers;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        for (std::list<TrackMarkerPtr >::iterator it = _imp->selectedMarkers.begin();
             it != _imp->selectedMarkers.end(); ++it) {
            if ((*it)->isEnabled()) {
                markers.push_back(*it);
            }
        }
    }
    trackMarkers(markers,start,end,forward, viewer);
}

bool
TrackerContext::isCurrentlyTracking() const
{
    return _imp->scheduler.isWorking();
}

void
TrackerContext::abortTracking()
{
    _imp->scheduler.abortTracking();
}

void
TrackerContext::beginEditSelection()
{
    QMutexLocker k(&_imp->trackerContextMutex);
    _imp->incrementSelectionCounter();
}

void
TrackerContext::endEditSelection(TrackSelectionReason reason)
{
    bool doEnd = false;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        _imp->decrementSelectionCounter();
        
        if (_imp->beginSelectionCounter == 0) {
            doEnd = true;
            
        }
    }
    if (doEnd) {
        endSelection(reason);
    }
}

void
TrackerContext::addTrackToSelection(const TrackMarkerPtr& mark, TrackSelectionReason reason)
{
    std::list<TrackMarkerPtr > marks;
    marks.push_back(mark);
    addTracksToSelection(marks, reason);
}

void
TrackerContext::addTracksToSelection(const std::list<TrackMarkerPtr >& marks, TrackSelectionReason reason)
{
    bool hasCalledBegin = false;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        
        if (!_imp->beginSelectionCounter) {
            k.unlock();
            Q_EMIT selectionAboutToChange((int)reason);
            k.relock();
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }
        
        for (std::list<TrackMarkerPtr >::const_iterator it = marks.begin() ; it!=marks.end(); ++it) {
            _imp->addToSelectionList(*it);
        }
        
        if (hasCalledBegin) {
            _imp->decrementSelectionCounter();
        }
    }
    if (hasCalledBegin) {
        endSelection(reason);
        
    }
}

void
TrackerContext::removeTrackFromSelection(const TrackMarkerPtr& mark, TrackSelectionReason reason)
{
    std::list<TrackMarkerPtr > marks;
    marks.push_back(mark);
    removeTracksFromSelection(marks, reason);
}

void
TrackerContext::removeTracksFromSelection(const std::list<TrackMarkerPtr >& marks, TrackSelectionReason reason)
{
    bool hasCalledBegin = false;
    
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        
        if (!_imp->beginSelectionCounter) {
            k.unlock();
            Q_EMIT selectionAboutToChange((int)reason);
            k.relock();
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }
        
        for (std::list<TrackMarkerPtr >::const_iterator it = marks.begin() ; it!=marks.end(); ++it) {
            _imp->removeFromSelectionList(*it);
        }
        
        if (hasCalledBegin) {
            _imp->decrementSelectionCounter();
        }
    }
    if (hasCalledBegin) {
        endSelection(reason);
    }
}

void
TrackerContext::clearSelection(TrackSelectionReason reason)
{
    std::list<TrackMarkerPtr > markers;
    getSelectedMarkers(&markers);
    if (markers.empty()) {
        return;
    }
    removeTracksFromSelection(markers, reason);
}

void
TrackerContext::selectAll(TrackSelectionReason reason)
{
    beginEditSelection();
    std::vector<TrackMarkerPtr > markers;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        markers = _imp->markers;
    }
    for (std::vector<TrackMarkerPtr >::iterator it = markers.begin(); it!= markers.end(); ++it) {
        addTrackToSelection(*it, reason);
    }
    endEditSelection(reason);
    
}

void
TrackerContext::getAllMarkers(std::vector<TrackMarkerPtr >* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    *markers = _imp->markers;
}

void
TrackerContext::getAllEnabledMarkers(std::vector<TrackMarkerPtr >* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if (_imp->markers[i]->isEnabled()) {
            markers->push_back(_imp->markers[i]);
        }
    }
}

void
TrackerContext::getSelectedMarkers(std::list<TrackMarkerPtr >* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    *markers = _imp->selectedMarkers;
}

bool
TrackerContext::isMarkerSelected(const TrackMarkerPtr& marker) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::list<TrackMarkerPtr >::const_iterator it = _imp->selectedMarkers.begin(); it!=_imp->selectedMarkers.end(); ++it) {
        if (*it == marker) {
            return true;
        }
    }
    return false;
}

void
TrackerContextPrivate::linkMarkerKnobsToGuiKnobs(const std::list<TrackMarkerPtr >& markers,
                                                 bool multipleTrackSelected,
                                                 bool slave)
{
    std::list<TrackMarkerPtr >::const_iterator next = markers.begin();
    if (!markers.empty()) {
        ++next;
    }
    for (std::list<TrackMarkerPtr >::const_iterator it = markers.begin() ; it!= markers.end(); ++it) {
        const KnobsVec& trackKnobs = (*it)->getKnobs();
        for (KnobsVec::const_iterator it2 = trackKnobs.begin(); it2 != trackKnobs.end(); ++it2) {
            
            // Find the knob in the TrackerContext knobs
            boost::shared_ptr<KnobI> found;
            for (std::list<boost::weak_ptr<KnobI> >::iterator it3 = perTrackKnobs.begin(); it3 != perTrackKnobs.end(); ++it3) {
                boost::shared_ptr<KnobI> k = it3->lock();
                if (k->getName() == (*it2)->getName()) {
                    found = k;
                    break;
                }
            }
            
            if (!found) {
                continue;
            }
            
            //Clone current state only for the last marker
            if (next == markers.end()) {
                found->cloneAndUpdateGui(it2->get());
            }
            
            //Slave internal knobs
            assert((*it2)->getDimension() == found->getDimension());
            for (int i = 0; i < (*it2)->getDimension(); ++i) {
                if (slave) {
                    (*it2)->slaveTo(i, found, i);
                } else {
                    (*it2)->unSlave(i, !multipleTrackSelected);
                }
            }
            
            if (!slave) {
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,ViewSpec,int,bool)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec,int, double,double)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewSpec,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,ViewSpec,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,ViewSpec,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
            } else {
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,ViewSpec,int,bool)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec, int,double,double)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewSpec, int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,ViewSpec, int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,ViewSpec,int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
            }
            
        }
        if (next != markers.end()) {
            ++next;
        }
    } // for (std::list<TrackMarkerPtr >::const_iterator it = markers() ; it!=markers(); ++it)
}

void
TrackerContext::endSelection(TrackSelectionReason reason)
{
    assert(QThread::currentThread() == qApp->thread());
    
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        if (_imp->selectionRecursion > 0) {
            _imp->markersToSlave.clear();
            _imp->markersToUnslave.clear();
            return;
        }
        if (_imp->markersToSlave.empty() && _imp->markersToUnslave.empty()) {
            return;
        }

    }
    ++_imp->selectionRecursion;
    
    
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        
        // Slave newly selected knobs
        bool selectionIsDirty = _imp->selectedMarkers.size() > 1;
        bool selectionEmpty = _imp->selectedMarkers.empty();
        
        
        //_imp->linkMarkerKnobsToGuiKnobs(_imp->markersToUnslave, selectionIsDirty, false);
        _imp->markersToUnslave.clear();
        
        
        
        //_imp->linkMarkerKnobsToGuiKnobs(_imp->markersToSlave, selectionIsDirty, true);
        _imp->markersToSlave.clear();
        
        
        
        
        for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->perTrackKnobs.begin(); it != _imp->perTrackKnobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
            if (!k) {
                continue;
            }
            k->setAllDimensionsEnabled(!selectionEmpty);
            k->setDirty(selectionIsDirty);
        }
    } //  QMutexLocker k(&_imp->trackerContextMutex);
    Q_EMIT selectionChanged((int)reason);
    
    --_imp->selectionRecursion;
}


void
TrackerContext::exportTrackDataFromExportOptions()
{
    int exportType_i = _imp->exportChoice.lock()->getValue();
    TrackExportTypeEnum type = (TrackExportTypeEnum)exportType_i;
    
    std::list<TrackMarkerPtr > selection;
    getSelectedMarkers(&selection);
    
    // createCornerPinFromSelection(selection, linked, useTransformRefFrame, invert)
    switch (type) {
        case eTrackExportTypeCornerPinThisFrame:
            _imp->createCornerPinFromSelection(selection, true, false, false);
            break;
        case eTrackExportTypeCornerPinRefFrame:
            _imp->createCornerPinFromSelection(selection, true, true, false);
            break;
        case eTrackExportTypeCornerPinThisFrameBaked:
            _imp->createCornerPinFromSelection(selection, false, false, false);
            break;
        case eTrackExportTypeCornerPinRefFrameBaked:
            _imp->createCornerPinFromSelection(selection, false, true, false);
            break;
        case eTrackExportTypeTransformStabilize:
            _imp->createTransformFromSelection(selection, true, true);
            break;
        case eTrackExportTypeTransformMatchMove:
            _imp->createTransformFromSelection(selection, true, false);
            break;
        case eTrackExportTypeTransformStabilizeBaked:
            _imp->createTransformFromSelection(selection, false, true);
            break;
        case eTrackExportTypeTransformMatchMoveBaked:
            _imp->createTransformFromSelection(selection, false, false);
            break;
    }
}

static
boost::shared_ptr<KnobDouble>
getCornerPinPoint(Node* node,
                  bool isFrom,
                  int index)
{
    assert(0 <= index && index < 4);
    QString name = isFrom ? QString::fromUtf8("from%1").arg(index + 1) : QString::fromUtf8("to%1").arg(index + 1);
    boost::shared_ptr<KnobI> knob = node->getKnobByName( name.toStdString() );
    assert(knob);
    boost::shared_ptr<KnobDouble>  ret = boost::dynamic_pointer_cast<KnobDouble>(knob);
    assert(ret);
    return ret;
}



void
TrackerContextPrivate::createCornerPinFromSelection(const std::list<TrackMarkerPtr > & selection,
                                                    bool linked,
                                                    bool useTransformRefFrame,
                                                    bool invert)
{
    if (selection.size() > 4 || selection.empty()) {
        Dialogs::errorDialog(QObject::tr("Export").toStdString(),
                             QObject::tr("Export to corner pin needs between 1 and 4 selected tracks.").toStdString());
        
        return;
    }
    
    boost::shared_ptr<KnobDouble> centers[4];
    int i = 0;
    for (std::list<TrackMarkerPtr >::const_iterator it = selection.begin(); it != selection.end(); ++it, ++i) {
        centers[i] = (*it)->getCenterKnob();
        assert(centers[i]);
    }
    
    NodePtr thisNode = node.lock();
    
    AppInstance* app = thisNode->getApp();
    CreateNodeArgs args(QString::fromUtf8(PLUGINID_OFX_CORNERPIN), eCreateNodeReasonInternal, thisNode->getGroup());
    NodePtr cornerPin = app->createNode(args);
    if (!cornerPin) {
        return;
    }
    
    // Move the Corner Pin node
    double thisNodePos[2];
    double thisNodeSize[2];
    thisNode->getPosition(&thisNodePos[0], &thisNodePos[1]);
    thisNode->getSize(&thisNodeSize[0], &thisNodeSize[1]);
    
    cornerPin->setPosition(thisNodePos[0] + thisNodeSize[0] * 2., thisNodePos[1]);
    
    
    boost::shared_ptr<KnobDouble> toPoints[4];
    boost::shared_ptr<KnobDouble> fromPoints[4];
    
    int timeForFromPoints = useTransformRefFrame ? _publicInterface->getTransformReferenceFrame() : app->getTimeLine()->currentFrame();
    
    for (unsigned int i = 0; i < selection.size(); ++i) {
        fromPoints[i] = getCornerPinPoint(cornerPin.get(), true, i);
        assert(fromPoints[i] && centers[i]);
        for (int j = 0; j < fromPoints[i]->getDimension(); ++j) {
            fromPoints[i]->setValue(centers[i]->getValueAtTime(timeForFromPoints,j), ViewSpec(0), j);
        }
        
        toPoints[i] = getCornerPinPoint(cornerPin.get(), false, i);
        assert(toPoints[i]);
        if (!linked) {
            toPoints[i]->cloneAndUpdateGui(centers[i].get());
        } else {
            bool ok = false;
            for (int d = 0; d < toPoints[i]->getDimension() ; ++d) {
                ok = dynamic_cast<KnobI*>(toPoints[i].get())->slaveTo(d, centers[i], d);
            }
            (void)ok;
            assert(ok);
        }
    }
    
    ///Disable all non used points
    for (unsigned int i = selection.size(); i < 4; ++i) {
        QString enableName = QString::fromUtf8("enable%1").arg(i + 1);
        KnobPtr knob = cornerPin->getKnobByName( enableName.toStdString() );
        assert(knob);
        KnobBool* enableKnob = dynamic_cast<KnobBool*>( knob.get() );
        assert(enableKnob);
        enableKnob->setValue(false, ViewSpec(0), 0);
    }
    
    if (invert) {
        KnobPtr invertKnob = cornerPin->getKnobByName(kCornerPinInvertParamName);
        assert(invertKnob);
        KnobBool* isBool = dynamic_cast<KnobBool*>(invertKnob.get());
        assert(isBool);
        isBool->setValue(true, ViewSpec(0), 0);
    }
    
}

void
TrackerContextPrivate::createTransformFromSelection(const std::list<TrackMarkerPtr > & selection,
                                                    bool linked,
                                                    bool invert)
{
    NodePtr thisNode = node.lock();
    
    AppInstance* app = thisNode->getApp();
    CreateNodeArgs args(QString::fromUtf8(PLUGINID_OFX_TRANSFORM), eCreateNodeReasonInternal, thisNode->getGroup());
    NodePtr transformNode = app->createNode(args);
    if (!transformNode) {
        return;
    }

    // Move the Corner Pin node
    double thisNodePos[2];
    double thisNodeSize[2];
    thisNode->getPosition(&thisNodePos[0], &thisNodePos[1]);
    thisNode->getSize(&thisNodeSize[0], &thisNodeSize[1]);
    
    transformNode->setPosition(thisNodePos[0] + thisNodeSize[0] * 2., thisNodePos[1]);
    
#pragma message WARN("TODO")
}


void
TrackerContext::onSelectedKnobCurveChanged()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (handler) {
        boost::shared_ptr<KnobI> knob = handler->getKnob();
        for (std::list<boost::weak_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
            if (k->getName() == knob->getName()) {
                k->clone(knob.get());
                break;
            }
        }
    }
}

void
TrackerContext::knobChanged(KnobI* k,
                 ValueChangedReasonEnum /*reason*/,
                 ViewSpec /*view*/,
                 double /*time*/,
                 bool /*originatedFromMainThread*/)
{
    if (k == _imp->exportButton.lock().get()) {
        exportTrackDataFromExportOptions();
    } else if (k == _imp->setCurrentFrameButton.lock().get()) {
        boost::shared_ptr<KnobInt> refFrame = _imp->referenceFrame.lock();
        refFrame->setValue(_imp->node.lock()->getApp()->getTimeLine()->currentFrame());
    } else if (k == _imp->transformType.lock().get()) {
        
    }
}


void
TrackerContext::removeItemAsPythonField(const TrackMarkerPtr& item)
{
    
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = "del " + nodeFullName + ".tracker." + item->getScriptName_mt_safe() + "\n";
    if (!appPTR->isBackground()) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if (!NATRON_PYTHON_NAMESPACE::interpretPythonScript(script , &err, 0)) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
    
}

void
TrackerContext::declareItemAsPythonField(const TrackMarkerPtr& item)
{
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    
    std::string err;
    std::string script = (nodeFullName + ".tracker." + item->getScriptName_mt_safe() + " = " +
                          nodeFullName + ".tracker.getTrackByName(\"" + item->getScriptName_mt_safe() + "\")\n");
    if (!appPTR->isBackground()) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if(!NATRON_PYTHON_NAMESPACE::interpretPythonScript(script , &err, 0)) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
    
}

void
TrackerContext::declarePythonFields()
{
    std::vector<TrackMarkerPtr > markers;
    getAllMarkers(&markers);
    for (std::vector< TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        declareItemAsPythonField(*it);
    }
}

using namespace openMVG::robust;

static void throwProsacError(ProsacReturnCodeEnum c, int nMinSamples) {
    switch (c) {
        case openMVG::robust::eProsacReturnCodeFoundModel:
            break;
        case openMVG::robust::eProsacReturnCodeInliersIsMinSamples:
            break;
        case openMVG::robust::eProsacReturnCodeNoModelFound:
            throw std::runtime_error("Could not find a model for the given correspondences.");
            break;
        case openMVG::robust::eProsacReturnCodeNotEnoughPoints:
        {
            std::stringstream ss;
            ss << "This model requires a minimum of ";
            ss << nMinSamples;
            ss << " correspondences.";
            throw std::runtime_error(ss.str());
        }   break;
        case openMVG::robust::eProsacReturnCodeMaxIterationsFromProportionParamReached:
            throw std::runtime_error("Maximum iterations computed from outliers proportion reached");
            break;
        case openMVG::robust::eProsacReturnCodeMaxIterationsParamReached:
            throw std::runtime_error("Maximum solver iterations reached");
            break;
    }
}

template <typename MODELTYPE>
void runProsacForModel(const std::vector<Point>& x1,
                       const std::vector<Point>& x2,
                       int w1, int h1, int w2, int h2,
                       typename MODELTYPE::Model* foundModel)
{
    typedef ProsacKernelAdaptor<MODELTYPE> KernelType;
    
    assert(x1.size() == x2.size());
    openMVG::Mat M1(2, x1.size()),M2(2, x2.size());
    for (std::size_t i = 0; i < x1.size(); ++i) {
        M1(0, i) = x1[i].x;
        M1(1, i) = x1[i].y;
        
        M2(0, i) = x2[i].x;
        M2(1, i) = x2[i].y;
    }
    KernelType kernel(M1, w1, h1, M2, w2, h2);
    ProsacReturnCodeEnum ret = prosac(kernel, foundModel);
    throwProsacError(ret, KernelType::MinimumSamples());
}

void
TrackerContext::computeTranslationFromNPoints(const std::vector<Point>& x1,
                                              const std::vector<Point>& x2,
                                              int w1, int h1, int w2, int h2,
                                              Point* translation)
{
    openMVG::Vec2 model;
    runProsacForModel<openMVG::robust::Translation2DSolver>(x1, x2, w1, h1, w2, h2, &model);
    translation->x = model(0);
    translation->y = model(1);
}


void
TrackerContext::computeSimilarityFromNPoints(const std::vector<Point>& x1,
                                      const std::vector<Point>& x2,
                                      int w1, int h1, int w2, int h2,
                                      Point* translation,
                                      double* rotate,
                                      double* scale)
{
    openMVG::Vec4 model;
    runProsacForModel<openMVG::robust::Similarity2DSolver>(x1, x2, w1, h1, w2, h2, &model);
    openMVG::robust::Similarity2DSolver::rtsFromVec4(model, &translation->x, &translation->y, scale, rotate);

}


void
TrackerContext::computeHomographyFromNPoints(const std::vector<Point>& x1,
                                             const std::vector<Point>& x2,
                                             int w1, int h1, int w2, int h2,
                                             Transform::Matrix3x3* homog)
{
    openMVG::Mat3 model;
    runProsacForModel<openMVG::robust::Homography2DSolver>(x1, x2, w1, h1, w2, h2, &model);
    
    *homog = Transform::Matrix3x3(model(0,0),model(0,1),model(0,2),
                                  model(1,0),model(1,1),model(1,2),
                                  model(2,0),model(2,1),model(2,2));
}

void
TrackerContext::computeFundamentalFromNPoints(const std::vector<Point>& x1,
                                              const std::vector<Point>& x2,
                                              int w1, int h1, int w2, int h2,
                                              Transform::Matrix3x3* fundamental)
{
    openMVG::Mat3 model;
    runProsacForModel<openMVG::robust::FundamentalSolver>(x1, x2, w1, h1, w2, h2, &model);
    
    *fundamental = Transform::Matrix3x3(model(0,0),model(0,1),model(0,2),
                                  model(1,0),model(1,1),model(1,2),
                                  model(2,0),model(2,1),model(2,2));
}

RectD
TrackerContext::getInputRoDAtTime(double time) const
{
    NodePtr input = getNode()->getInput(0);
    bool useProjFormat = false;
    RectD ret;
    if (!input) {
        useProjFormat = true;
    } else {
        StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(input->getHashValue(), time, RenderScale(1.), ViewIdx(0), &ret, 0);
        if (stat == eStatusFailed) {
            useProjFormat = true;
        } else {
            return ret;
        }
    }
    if (useProjFormat) {
        Format f;
        getNode()->getApp()->getProject()->getProjectDefaultFormat(&f);
        ret.x1 = f.x1;
        ret.x2 = f.x2;
        ret.y1 = f.y1;
        ret.y2 = f.y2;
    }
    return ret;
}

void
TrackerContext::resetTransformCenter()
{
    std::vector<TrackMarkerPtr> tracks;
    getAllEnabledMarkers(&tracks);
    
    double time = (double)getTransformReferenceFrame();
    
    Point center;
    if (tracks.empty()) {
        RectD rod = getInputRoDAtTime(time);
        center.x = (rod.x1 + rod.x2) / 2.;
        center.y = (rod.y1 + rod.y2) / 2.;

    } else {
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            boost::shared_ptr<KnobDouble> centerKnob = tracks[i]->getCenterKnob();
            center.x += centerKnob->getValueAtTime(time, 0);
            center.y += centerKnob->getValueAtTime(time, 1);
            
        }
        center.x /= tracks.size();
        center.y /= tracks.size();
    }
    
    boost::shared_ptr<KnobDouble> centerKnob = _imp->center.lock();
    centerKnob->resetToDefaultValue(0);
    centerKnob->resetToDefaultValue(1);
    centerKnob->setValues(center.x, center.y, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
}

void
TrackerContext::resetTransformParams()
{
    boost::shared_ptr<KnobDouble> translate = _imp->translate.lock();
    translate->resetToDefaultValue(0);
    translate->resetToDefaultValue(1);
    
    boost::shared_ptr<KnobDouble> rotate = _imp->rotate.lock();
    rotate->resetToDefaultValue(0);
    
    boost::shared_ptr<KnobDouble> scale = _imp->scale.lock();
    scale->resetToDefaultValue(0);
    scale->resetToDefaultValue(1);
    
    boost::shared_ptr<KnobDouble> skewX = _imp->skewX.lock();
    skewX->resetToDefaultValue(0);
    
    boost::shared_ptr<KnobDouble> skewY = _imp->skewY.lock();
    skewY->resetToDefaultValue(0);
    
    resetTransformCenter();
}

struct PointWithError
{
    Point p1,p2;
    double error;
};

static bool PointWithErrorCompareLess(const PointWithError& lhs, const PointWithError& rhs)
{
    return lhs.error < rhs.error;
}

void
TrackerContext::extractSortedPointsFromMarkers(double refTime,
                                               double time,
                                               const std::vector<TrackMarkerPtr>& markers,
                                               int jitterPeriod,
                                               bool jitterAdd,
                                               std::vector<Point>* x1,
                                               std::vector<Point>* x2)
{
    assert(!markers.empty());

    std::vector<PointWithError> pointsWithErrors;
    
    bool useJitter = jitterPeriod > 1;
    int halfJitter = jitterPeriod / 2;
    // Prosac expects the points to be sorted by decreasing correlation score (increasing error)
    for (std::size_t i = 0; i < markers.size(); ++i) {
        boost::shared_ptr<KnobDouble> centerKnob = markers[i]->getCenterKnob();
        boost::shared_ptr<KnobDouble> errorKnob = markers[i]->getErrorKnob();
        
        if (centerKnob->getKeyFrameIndex(ViewSpec::current(), 0, time) < 0) {
            continue;
        }
        pointsWithErrors.resize(pointsWithErrors.size() + 1);
        
        if (!useJitter) {
            pointsWithErrors[i].p1.x = centerKnob->getValueAtTime(refTime, 0);
            pointsWithErrors[i].p1.y = centerKnob->getValueAtTime(refTime, 1);
            pointsWithErrors[i].p2.x = centerKnob->getValueAtTime(time, 0);
            pointsWithErrors[i].p2.y = centerKnob->getValueAtTime(time, 1);
        } else {
            // Average halfJitter frames before and after refTime and time together to smooth the center
            std::vector<Point> x1PointJitter,x2PointJitter;
            Point x1AtTime,x2AtTime;
            for (double t = refTime - halfJitter; t <= refTime + halfJitter; t += 1.) {
                Point p;
                p.x = centerKnob->getValueAtTime(t, 0);
                p.y = centerKnob->getValueAtTime(t, 1);
                if (t == refTime) {
                    x1AtTime = p;
                }
                x1PointJitter.push_back(p);
            }
            for (double t = time - halfJitter; t <= time + halfJitter; t += 1.) {
                Point p;
                p.x = centerKnob->getValueAtTime(t, 0);
                p.y = centerKnob->getValueAtTime(t, 1);
                if (t == time) {
                    x2AtTime = p;
                }
                x2PointJitter.push_back(p);
            }
            assert(x1PointJitter.size() == x2PointJitter.size());
            Point x1avg = {0,0},x2avg = {0,0};
            for (std::size_t i = 0; i < x1PointJitter.size(); ++i) {
                x1avg.x += x1PointJitter[i].x;
                x1avg.y += x1PointJitter[i].y;
                x2avg.x += x2PointJitter[i].x;
                x2avg.y += x2PointJitter[i].y;
            }
            if (!jitterAdd) {
                pointsWithErrors[i].p1.x = x1avg.x;
                pointsWithErrors[i].p1.y = x1avg.y;
                pointsWithErrors[i].p2.x = x2avg.x;
                pointsWithErrors[i].p2.y = x2avg.y;
            } else {
                Point highFreqX1,highFreqX2;
                highFreqX1.x = x1AtTime.x - x1avg.x;
                highFreqX1.y = x1AtTime.y - x1avg.y;
                
                highFreqX2.x = x2AtTime.x - x2avg.x;
                highFreqX2.y = x2AtTime.y - x2avg.y;
                
                pointsWithErrors[i].p1.x = x1AtTime.x + highFreqX1.x;
                pointsWithErrors[i].p1.y = x1AtTime.y + highFreqX1.y;
                pointsWithErrors[i].p2.x = x2AtTime.x + highFreqX2.x;
                pointsWithErrors[i].p2.y = x2AtTime.y + highFreqX2.y;
                
            }
        }
        
        pointsWithErrors[i].error = errorKnob->getValueAtTime(time, 0);
    }
    
    std::sort(pointsWithErrors.begin(), pointsWithErrors.end(), PointWithErrorCompareLess);
    
    x1->resize(pointsWithErrors.size());
    x2->resize(pointsWithErrors.size());
    int r = 0;
    for (int i = (int)pointsWithErrors.size() - 1; i >= 0; --i, ++r) {
        (*x1)[r] = pointsWithErrors[i].p1;
        (*x2)[r] = pointsWithErrors[i].p2;
    }

}

void
TrackerContext::computeTransformParamsFromTracksAtTime(double refTime,
                                                       double time,
                                                       int jitterPeriod,
                                                       bool jitterAdd,
                                                       const std::vector<TrackMarkerPtr>& markers,
                                                       TransformData* data)
{
    assert(!markers.empty());
    std::vector<Point> x1, x2;
    extractSortedPointsFromMarkers(refTime, time, markers, jitterPeriod, jitterAdd, &x1, &x2);
    
    RectD rodRef = getInputRoDAtTime(refTime);
    RectD rodTime = getInputRoDAtTime(time);
    
    int w1 = rodRef.width();
    int h1 = rodRef.height();
    
    int w2 = rodTime.width();
    int h2 = rodTime.height();
    
    if (x1.size() == 1) {
        data->hasRotationAndScale = false;
        computeTranslationFromNPoints(x1, x2, w1, h1, w2, h2, &data->translation);
    } else {
        data->hasRotationAndScale = true;
        computeSimilarityFromNPoints(x1, x2, w1, h1, w2, h2, &data->translation, &data->rotation, &data->scale);
    }
}

struct TransformDataWithTime
{
    TrackerContext::TransformData data;
    double time;
};

void
TrackerContext::computeTransformParamsFromTracks(const std::vector<TrackMarkerPtr>& markers)
{
    if (markers.empty()) {
        return;
    }
    int transformType_i = _imp->transformType.lock()->getValue();
    TrackerTransformTypeEnum type =  (TrackerTransformTypeEnum)transformType_i;
    
    double refTime = (double)getTransformReferenceFrame();

    int jitterPeriod = 0;
    bool jitterAdd = false;
    switch (type) {
        case eTrackerTransformTypeNone:
        case eTrackerTransformTypeMatchMove:
        case eTrackerTransformTypeStabilize:
            break;
        case eTrackerTransformTypeAddJitter:
        case eTrackerTransformTypeRemoveJitter:
        {
            jitterPeriod = _imp->jitterPeriod.lock()->getValue();
            jitterAdd = type == eTrackerTransformTypeAddJitter;
        } break;
            
    }
    
    std::set<double> keyframes;
    {
        for (std::size_t i = 0; i < markers.size(); ++i) {
            std::set<double> keys;
            markers[i]->getCenterKeyframes(&keys);
            for (std::set<double>::iterator it = keys.begin(); it!= keys.end(); ++it) {
                keyframes.insert(*it);
            }
            
        }
    }

    boost::shared_ptr<KnobInt> smoothKnob = _imp->smoothTransform.lock();
    int smoothTJitter,smoothRJitter, smoothSJitter;
    smoothTJitter = smoothKnob->getValue(0);
    smoothRJitter = smoothKnob->getValue(1);
    smoothSJitter = smoothKnob->getValue(2);
    
    
    
    std::vector<TransformDataWithTime> dataAtTime;
    for (std::set<double>::iterator it = keyframes.begin(); it!=keyframes.end(); ++it) {
        TransformDataWithTime t;
        t.time = *it;
        try {
            computeTransformParamsFromTracksAtTime(refTime, t.time, jitterPeriod, jitterAdd, markers, &t.data);
        } catch (const std::exception& e) {
            qDebug() << e.what();
            continue;
        }
        dataAtTime.push_back(t);
    }
    
    NodePtr node = getNode();
    node->getEffectInstance()->beginChanges();
    
    boost::shared_ptr<KnobDouble> translationKnob = _imp->translate.lock();
    boost::shared_ptr<KnobDouble> scaleKnob = _imp->scale.lock();
    boost::shared_ptr<KnobDouble> rotationKnob = _imp->rotate.lock();
    
    translationKnob->removeAnimation(ViewSpec::all(),0);
    translationKnob->removeAnimation(ViewSpec::all(),1);
    
    scaleKnob->removeAnimation(ViewSpec::all(),0);
    rotationKnob->removeAnimation(ViewSpec::all(),0);
    
    for (std::size_t i = 0; i < dataAtTime.size(); ++i) {
        if (smoothTJitter > 1) {
            int halfJitter = smoothTJitter / 2;
            Point avgT = {0,0};
            
            int nSamples = 0;
            for (int t = std::max(0, (int)i - halfJitter); t < (i + halfJitter) && t < (int)dataAtTime.size(); ++t, ++nSamples) {
                avgT.x += dataAtTime[t].data.translation.x;
                avgT.y += dataAtTime[t].data.translation.y;
            }
            avgT.x /= nSamples;
            avgT.y /= nSamples;
            
            translationKnob->setValueAtTime(dataAtTime[i].time, avgT.x, ViewSpec::all(), 0);
            translationKnob->setValueAtTime(dataAtTime[i].time, avgT.y, ViewSpec::all(), 1);
        } else {
            translationKnob->setValueAtTime(dataAtTime[i].time, dataAtTime[i].data.translation.x, ViewSpec::all(), 0);
            translationKnob->setValueAtTime(dataAtTime[i].time, dataAtTime[i].data.translation.y, ViewSpec::all(), 1);
        }
        
        if (smoothRJitter > 1) {
            int halfJitter = smoothRJitter / 2;
            double avg = dataAtTime[i].data.rotation;
            int nSamples = 1;
            int offset = 1;
            while (nSamples < halfJitter) {
                bool canMoveForward = i + offset < dataAtTime.size();
                if (canMoveForward) {
                    if (dataAtTime[i + offset].data.hasRotationAndScale) {
                        avg += dataAtTime[i + offset].data.rotation;
                        ++nSamples;
                    }
                }
                bool canMoveBackward = (int)i - offset >= 0;
                if (canMoveForward) {
                    if (dataAtTime[i - offset].data.hasRotationAndScale) {
                        avg += dataAtTime[i - offset].data.rotation;
                        ++nSamples;
                    }
                }
                if (canMoveForward || canMoveBackward) {
                    ++nSamples;
                } else if (!canMoveBackward && !canMoveForward) {
                    break;
                }
            }
        
            avg /= nSamples;
            
            rotationKnob->setValueAtTime(dataAtTime[i].time, avg, ViewSpec::all(), 0);
        } else {
            rotationKnob->setValueAtTime(dataAtTime[i].time, dataAtTime[i].data.rotation, ViewSpec::all(), 0);
        }
        
        if (smoothSJitter > 1) {
            int halfJitter = smoothSJitter / 2;
            double avg = dataAtTime[i].data.scale;
            int nSamples = 1;
            int offset = 1;
            while (nSamples < halfJitter) {
                bool canMoveForward = i + offset < dataAtTime.size();
                if (canMoveForward) {
                    if (dataAtTime[i + offset].data.hasRotationAndScale) {
                        avg += dataAtTime[i + offset].data.scale;
                        ++nSamples;
                    }
                }
                bool canMoveBackward = (int)i - offset >= 0;
                if (canMoveForward) {
                    if (dataAtTime[i - offset].data.hasRotationAndScale) {
                        avg += dataAtTime[i - offset].data.scale;
                        ++nSamples;
                    }
                }
                if (canMoveForward || canMoveBackward) {
                    ++nSamples;
                } else if (!canMoveBackward && !canMoveForward) {
                    break;
                }
            }
            avg /= nSamples;
            
            scaleKnob->setValueAtTime(dataAtTime[i].time, avg, ViewSpec::all(), 0);
        } else {
            scaleKnob->setValueAtTime(dataAtTime[i].time, dataAtTime[i].data.scale, ViewSpec::all(), 0);
        }
    } // for (std::size_t i = 0; i < dataAtTime.size(); ++i)
    
    node->getEffectInstance()->endChanges();
}

void
TrackerContext::computeTransformParamsFromEnabledTracks()
{
    std::vector<TrackMarkerPtr> tracks;
    getAllEnabledMarkers(&tracks);
    computeTransformParamsFromTracks(tracks);
}

//////////////////////// TrackScheduler

struct TrackSchedulerPrivate
{
    TrackerParamsProvider* paramsProvider;
    NodeWPtr node;
    mutable QMutex mustQuitMutex;
    bool mustQuit;
    QWaitCondition mustQuitCond;
    
    mutable QMutex abortRequestedMutex;
    int abortRequested;
    QWaitCondition abortRequestedCond;
    
    QMutex startRequesstMutex;
    int startRequests;
    QWaitCondition startRequestsCond;
    
    mutable QMutex isWorkingMutex;
    bool isWorking;
    
    
    TrackSchedulerPrivate(TrackerParamsProvider* paramsProvider, const NodeWPtr& node)
    : paramsProvider(paramsProvider)
    , node(node)
    , mustQuitMutex()
    , mustQuit(false)
    , mustQuitCond()
    , abortRequestedMutex()
    , abortRequested(0)
    , abortRequestedCond()
    , startRequesstMutex()
    , startRequests(0)
    , startRequestsCond()
    , isWorkingMutex()
    , isWorking(false)
    {
        
    }
    
    NodePtr getNode() const
    {
        return node.lock();
    }
    
    bool checkForExit()
    {
        QMutexLocker k(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            mustQuitCond.wakeAll();
            return true;
        }
        return false;
    }
    
};

TrackSchedulerBase::TrackSchedulerBase()
: QThread()
{
    QObject::connect(this, SIGNAL(renderCurrentFrameForViewer(ViewerInstance*)), this, SLOT(doRenderCurrentFrameForViewer(ViewerInstance*)));
}

template <class TrackArgsType>
TrackScheduler<TrackArgsType>::TrackScheduler(TrackerParamsProvider* paramsProvider, const NodeWPtr& node, TrackStepFunctor functor)
: TrackSchedulerBase()
, _imp(new TrackSchedulerPrivate(paramsProvider, node))
, argsMutex()
, curArgs()
, requestedArgs()
, _functor(functor)
{
    setObjectName(QString::fromUtf8("TrackScheduler"));
}

template <class TrackArgsType>
TrackScheduler<TrackArgsType>::~TrackScheduler()
{
    
}

template <class TrackArgsType>
bool
TrackScheduler<TrackArgsType>::isWorking() const
{
    QMutexLocker k(&_imp->isWorkingMutex);
    return _imp->isWorking;
}

class IsTrackingFlagSetter_RAII
{
    ViewerInstance* _v;
    EffectInstPtr _effect;
    TrackSchedulerBase* _base;
    bool _reportProgress;
    
public:
    
    IsTrackingFlagSetter_RAII(const EffectInstPtr& effect, TrackSchedulerBase* base, int step, bool reportProgress, ViewerInstance* viewer)
    : _v(viewer)
    , _effect(effect)
    , _base(base)
    , _reportProgress(reportProgress)
    {
        if (_effect && _reportProgress) {
            _effect->getApp()->progressStart(_effect->getNode(), QObject::tr("Tracking...").toStdString(), "");
            _base->emit_trackingStarted(step);
        }

        if (viewer) {
            viewer->setDoingPartialUpdates(true);
        }
    }
    
    ~IsTrackingFlagSetter_RAII()
    {
        if (_v) {
            _v->setDoingPartialUpdates(false);
        }
        if (_effect && _reportProgress) {
            _effect->getApp()->progressEnd(_effect->getNode());
            _base->emit_trackingFinished();
        }

    }
};

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::run()
{
    for (;;) {
        
        ///Check for exit of the thread
        if (_imp->checkForExit()) {
            return;
        }
        
        ///Flag that we're working
        {
            QMutexLocker k(&_imp->isWorkingMutex);
            _imp->isWorking = true;
        }
        
        ///Copy the requested args to the args used for processing
        {
            QMutexLocker k(&argsMutex);
            curArgs = requestedArgs;
        }
        
        
        boost::shared_ptr<TimeLine> timeline = curArgs.getTimeLine();
        
        ViewerInstance* viewer =  curArgs.getViewer();
        
        
        int end = curArgs.getEnd();
        int start = curArgs.getStart();
        int cur = start;
        bool isForward = curArgs.getForward();
        int framesCount = isForward ? (end - start) : (start - end);
        int numTracks = curArgs.getNumTracks();
        
        
        std::vector<int> trackIndexes;
        for (std::size_t i = 0; i < (std::size_t)numTracks; ++i) {
            trackIndexes.push_back(i);
        }
        
        int lastValidFrame = isForward ? start - 1 : start + 1;
        bool reportProgress = numTracks > 1 || framesCount > 1;

        EffectInstPtr effect = _imp->getNode()->getEffectInstance();
        {
            ///Use RAII style for setting the isDoingPartialUpdates flag so we're sure it gets removed
            IsTrackingFlagSetter_RAII __istrackingflag__(effect, this, isForward ? 1 : -1, reportProgress, viewer);

            
            if ((isForward && start >= end) || (!isForward && start <= end)) {
                // Invalid range
                cur = end;
            }
            
            while (cur != end) {
                
                
                
                ///Launch parallel thread for each track using the global thread pool
                QFuture<bool> future = QtConcurrent::mapped(trackIndexes,
                                                            boost::bind(_functor,
                                                                        _1,
                                                                        curArgs,
                                                                        cur));
                future.waitForFinished();
                
                bool failure = false;
                for (QFuture<bool>::const_iterator it = future.begin(); it != future.end(); ++it) {
                    if (!(*it)) {
                        failure = true;
                        break;
                    }
                }
                if (failure) {
                    break;
                }
                
                lastValidFrame = cur;
                
                
                double progress;
                if (isForward) {
                    ++cur;
                    progress = (double)(cur - start) / framesCount;
                } else {
                    --cur;
                    progress = (double)(start - cur) / framesCount;
                }
                
                bool isUpdateViewerOnTrackingEnabled = _imp->paramsProvider->getUpdateViewer();
                bool isCenterViewerEnabled = _imp->paramsProvider->getCenterOnTrack();

                
                ///Ok all tracks are finished now for this frame, refresh viewer if needed
                if (isUpdateViewerOnTrackingEnabled && viewer) {
                    
                    //This will not refresh the viewer since when tracking, renderCurrentFrame()
                    //is not called on viewers, see Gui::onTimeChanged
                    timeline->seekFrame(cur, true, 0, Natron::eTimelineChangeReasonOtherSeek);
                    
                    ///Beyond TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE it becomes more expensive to render all partial rectangles
                    ///than just render the whole viewer RoI
                    if (numTracks < TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE) {
                        std::list<RectD> updateRects;
                        curArgs.getRedrawAreasNeeded(cur, &updateRects);
                        viewer->setPartialUpdateParams(updateRects, isCenterViewerEnabled);
                    } else {
                        viewer->clearPartialUpdateParams();
                    }
                    Q_EMIT renderCurrentFrameForViewer(viewer);
                }
                
                if (reportProgress && effect) {
                    ///Notify we progressed of 1 frame
                    if (!effect->getApp()->progressUpdate(effect->getNode(), progress)) {
                        QMutexLocker k(&_imp->abortRequestedMutex);
                        ++_imp->abortRequested;
                    }
                }
                
                ///Check for abortion
                {
                    QMutexLocker k(&_imp->abortRequestedMutex);
                    if (_imp->abortRequested > 0) {
                        _imp->abortRequested = 0;
                        _imp->abortRequestedCond.wakeAll();
                        break;
                    }
                }
                
            } // while (cur != end) {
            
        
        } // IsTrackingFlagSetter_RAII
        
        TrackerContext* isContext = dynamic_cast<TrackerContext*>(_imp->paramsProvider);
        if (isContext) {
            isContext->computeTransformParamsFromEnabledTracks();
        }
        
        appPTR->getAppTLS()->cleanupTLSForThread();
        
        //Now that tracking is done update viewer once to refresh the whole visible portion
        
        if (_imp->paramsProvider->getUpdateViewer()) {
            //Refresh all viewers to the current frame
            timeline->seekFrame(lastValidFrame, true, 0, Natron::eTimelineChangeReasonOtherSeek);
        }
        
        ///Flag that we're no longer working
        {
            QMutexLocker k(&_imp->isWorkingMutex);
            _imp->isWorking = false;
        }
        
        ///Make sure we really reset the abort flag
        {
            QMutexLocker k(&_imp->abortRequestedMutex);
            if (_imp->abortRequested > 0) {
                _imp->abortRequested = 0;
                _imp->abortRequestedCond.wakeAll();
                
            }
        }
        
        ///Sleep or restart if we've got requests in the queue
        {
            QMutexLocker k(&_imp->startRequesstMutex);
            while (_imp->startRequests <= 0) {
                _imp->startRequestsCond.wait(&_imp->startRequesstMutex);
            }
            _imp->startRequests = 0;
        }
        
    } // for (;;) {
}

void
TrackSchedulerBase::doRenderCurrentFrameForViewer(ViewerInstance* viewer)
{
    assert(QThread::currentThread() == qApp->thread());
    viewer->renderCurrentFrame(true);
}

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::track(const TrackArgsType& args)
{
    
    {
        QMutexLocker k(&argsMutex);
        requestedArgs = args;
    }
    if (isRunning()) {
        QMutexLocker k(&_imp->startRequesstMutex);
        ++_imp->startRequests;
        _imp->startRequestsCond.wakeAll();
    } else {
        start();
    }
}


template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::abortTracking()
{
    if (!isRunning() || !isWorking()) {
        return;
    }
    
    
    {
        QMutexLocker k(&_imp->abortRequestedMutex);
        ++_imp->abortRequested;
        while (_imp->abortRequested) {
            _imp->abortRequestedCond.wait(&_imp->abortRequestedMutex);
        }
        
    }
    
}

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::quitThread()
{
    if (!isRunning()) {
        return;
    }
    
    abortTracking();
    
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
        
        {
            QMutexLocker k(&_imp->startRequesstMutex);
            ++_imp->startRequests;
            _imp->startRequestsCond.wakeAll();
        }
        
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
        
    }
    
    
    wait();
    
}

///Explicit template instantiation for TrackScheduler
template class TrackScheduler<TrackArgsV1>;
template class TrackScheduler<TrackArgsLibMV>;

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_TrackerContext.cpp"
