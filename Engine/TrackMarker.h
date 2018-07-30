/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef Engine_TrackMarker_h
#define Engine_TrackMarker_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#endif

#include "Engine/EngineFwd.h"
#include "Engine/Knob.h"

#define kTrackerParamSearchWndBtmLeft "searchWndBtmLeft"
#define kTrackerParamSearchWndBtmLeftLabel "Search Window Bottom Left"
#define kTrackerParamSearchWndBtmLeftHint "The bottom left point of the search window, relative to the center point."

#define kTrackerParamSearchWndTopRight "searchWndTopRight"
#define kTrackerParamSearchWndTopRightLabel "Search Window Top Right"
#define kTrackerParamSearchWndTopRightHint "The top right point of the search window, relative to the center point."

#define kTrackerParamPatternTopLeft "patternTopLeft"
#define kTrackerParamPatternTopLeftLabel "Pattern Top Left"
#define kTrackerParamPatternTopLeftHint "The top left point of the quad defining the pattern to track"

#define kTrackerParamPatternTopRight "patternTopRight"
#define kTrackerParamPatternTopRightLabel "Pattern Top Right"
#define kTrackerParamPatternTopRightHint "The top right point of the quad defining the pattern to track"

#define kTrackerParamPatternBtmRight "patternBtmRight"
#define kTrackerParamPatternBtmRightLabel "Pattern Bottom Right"
#define kTrackerParamPatternBtmRightHint "The bottom right point of the quad defining the pattern to track"

#define kTrackerParamPatternBtmLeft "patternBtmLeft"
#define kTrackerParamPatternBtmLeftLabel "Pattern Bottom Left"
#define kTrackerParamPatternBtmLeftHint "The bottom left point of the quad defining the pattern to track"

#define kTrackerParamCenter "centerPoint"
#define kTrackerParamCenterLabel "Center"
#define kTrackerParamCenterHint "The point to track"

#define kTrackerParamOffset "offset"
#define kTrackerParamOffsetLabel "Offset"
#define kTrackerParamOffsetHint "The offset applied to the center point relative to the real tracked position"

#define kTrackerParamTrackWeight "trackWeight"
#define kTrackerParamTrackWeightLabel "Weight"
#define kTrackerParamTrackWeightHint "The weight determines the amount this marker contributes to the final solution"

#define kTrackerParamMotionModel "motionModel"
#define kTrackerParamMotionModelLabel "Motion model"
#define kTrackerParamMotionModelHint "The motion model to use for tracking."

#define kTrackerParamMotionModelTranslation "Search for markers that are only translated between frames."
#define kTrackerParamMotionModelTransRot "Search for markers that are translated and rotated between frames."
#define kTrackerParamMotionModelTransScale "Search for markers that are translated and scaled between frames."
#define kTrackerParamMotionModelTransRotScale "Search for markers that are translated, rotated and scaled between frames."
#define kTrackerParamMotionModelAffine "Search for markers that are affine transformed (t,r,k and skew) between frames."
#define kTrackerParamMotionModelPerspective "Search for markers that are perspectively deformed (homography) between frames."

#define kTrackerParamError "errorValue"
#define kTrackerParamErrorLabel "Error"
#define kTrackerParamErrorHint "The error obtained after tracking each frame. This 1 minus the corss-correlation score."

#define kTrackerParamEnabled "enabled"
#define kTrackerParamEnabledLabel "Enabled"
#define kTrackerParamEnabledHint "When checked, this track data will be used to generate the resulting Transform/CornerPin out of the tracker. You can animate this parameter to control the lifetime of the track."
//#define NATRON_TRACK_MARKER_USE_WEIGHT

NATRON_NAMESPACE_ENTER

struct TrackMarkerPrivate;
class TrackMarker
    : public NamedKnobHolder
    , public boost::enable_shared_from_this<TrackMarker>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

protected:
    struct MakeSharedEnabler;

    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    TrackMarker(const TrackerContextPtr& context);

public:
    static TrackMarkerPtr create(const TrackerContextPtr& context);
    
    virtual ~TrackMarker();

    void clone(const TrackMarker& other);

    void load(const TrackSerialization& serialization);

    void save(TrackSerialization* serialization) const;

    TrackerContextPtr getContext() const;

    bool setScriptName(const std::string& name);
    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    void setLabel(const std::string& label);
    std::string getLabel() const;
    KnobDoublePtr getSearchWindowBottomLeftKnob() const;
    KnobDoublePtr getSearchWindowTopRightKnob() const;
    KnobDoublePtr getPatternTopLeftKnob() const;
    KnobDoublePtr getPatternTopRightKnob() const;
    KnobDoublePtr getPatternBtmRightKnob() const;
    KnobDoublePtr getPatternBtmLeftKnob() const;
#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    KnobDoublePtr getWeightKnob() const;
#endif
    KnobDoublePtr getCenterKnob() const;
    KnobDoublePtr getOffsetKnob() const;
    KnobDoublePtr getErrorKnob() const;
    KnobChoicePtr getMotionModelKnob() const;
    KnobBoolPtr getEnabledKnob() const;

    int getReferenceFrame(int time, int frameStep) const;

    bool isUserKeyframe(int time) const;

    int getPreviousKeyframe(int time) const;

    int getNextKeyframe( int time) const;

    void getUserKeyframes(std::set<int>* keyframes) const;

    void getCenterKeyframes(std::set<double>* keyframes) const;

    bool isEnabled(double time) const;

    void setEnabledAtTime(double time, bool enabled);

    AnimationLevelEnum getEnabledNessAnimationLevel() const;

    void setEnabledFromGui(double time, bool enabled);

    void setMotionModelFromGui(int index);

    void resetCenter();

    void resetOffset();

    void resetTrack();

    void setKeyFrameOnCenterAndPatternAtTime(int time);

    void setUserKeyframe(int time);

    void removeUserKeyframe(int time);

    /*
       Controls animation of the center & offset not the pattern
     */
    void clearAnimation();
    void clearAnimationBeforeTime(int time);
    void clearAnimationAfterTime(int time);

    void removeAllUserKeyframes();

    std::pair<ImagePtr, RectI> getMarkerImage(int time, const RectI& roi) const;

    RectI getMarkerImageRoI(int time) const;

    virtual void onKnobSlaved(const KnobIPtr& slave, const KnobIPtr& master,
                              int dimension,
                              bool isSlave) OVERRIDE FINAL;

    void notifyTrackingStarted();
    void notifyTrackingEnded();

protected:

    virtual void initializeKnobs() OVERRIDE;

public Q_SLOTS:

    void onCenterKeyframeSet(double time, ViewSpec view, int dimension, int reason, bool added);
    void onCenterKeyframeRemoved(double time, ViewSpec view, int dimension, int reason);
    void onCenterMultipleKeysRemoved(const std::list<double>& times, ViewSpec view, int dimension, int reason);
    void onCenterKeyframeMoved(ViewSpec view, int dimension, double oldTime, double newTime);
    void onCenterKeyframesSet(const std::list<double>& keys, ViewSpec view, int dimension, int reason);
    void onCenterAnimationRemoved(ViewSpec view, int dimension);

    void onCenterKnobValueChanged(ViewSpec, int dimension, int reason);
    void onOffsetKnobValueChanged(ViewSpec, int dimension, int reason);
    void onErrorKnobValueChanged(ViewSpec, int dimension, int reason);
    void onWeightKnobValueChanged(ViewSpec, int dimension, int reason);
    void onMotionModelKnobValueChanged(ViewSpec, int dimension, int reason);

    /*void onPatternTopLeftKnobValueChanged(int dimension,int reason);
       void onPatternTopRightKnobValueChanged(int dimension,int reason);
       void onPatternBtmRightKnobValueChanged(int dimension,int reason);
       void onPatternBtmLeftKnobValueChanged(int dimension,int reason);*/
    void onSearchBtmLeftKnobValueChanged(ViewSpec, int dimension, int reason);
    void onSearchTopRightKnobValueChanged(ViewSpec, int dimension, int reason);

public Q_SLOTS:

    void onEnabledValueChanged(ViewSpec, int dimension, int reason);

Q_SIGNALS:

    void enabledChanged(int reason);

private:


    boost::scoped_ptr<TrackMarkerPrivate> _imp;
};


class TrackMarkerPM
    : public TrackMarker
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    NodePtr trackerNode;

    // These are knobs that live in the trackerPM node, we control them directly
    KnobButtonWPtr trackPrevButton, trackNextButton;
    KnobDoubleWPtr centerKnob, offsetKnob;
    KnobIntWPtr refFrameKnob;
    KnobChoiceWPtr scoreTypeKnob;
    KnobDoubleWPtr correlationScoreKnob;
    KnobDoubleWPtr patternBtmLeftKnob, patternTopRightKnob;
    KnobDoubleWPtr searchWindowBtmLeftKnob, searchWindowTopRightKnob;

private:
    struct MakeSharedEnabler;
    
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    TrackMarkerPM(const TrackerContextPtr& context);

public:
    static TrackMarkerPtr create(const TrackerContextPtr& context);

    virtual ~TrackMarkerPM();

    bool trackMarker(bool forward, int refFrame, int trackedFrame);

public Q_SLOTS:

    void onTrackerNodeInputChanged(int inputNb);

private:

    virtual void initializeKnobs() OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT


#endif // Engine_TrackMarker_h
