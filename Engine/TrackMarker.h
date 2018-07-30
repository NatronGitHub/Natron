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
#endif

#include "Engine/Knob.h"
#include "Engine/KnobItemsTable.h"

#include "Serialization/SerializationBase.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


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

#define kTrackerParamManualKeyframes "manualKeyframes"
#define kTrackerParamManualKeyframesLabel "Manual Keyframe(s)"
#define kTrackerParamManualKeyframesHint "Navigate throughout the keyframes created manually on the track"


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


struct TrackMarkerPrivate;
class TrackMarker
: public KnobTableItem
, public CurveChangesListener
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON


protected: // derives from KnobHolder
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    TrackMarker(const KnobItemsTablePtr& context);


public:
    static TrackMarkerPtr create(const KnobItemsTablePtr& model) WARN_UNUSED_RETURN
    {
        return TrackMarkerPtr( new TrackMarker(model) );
    }

    TrackMarkerPtr shared_from_this() {
        return boost::dynamic_pointer_cast<TrackMarker>(KnobHolder::shared_from_this());
    }

    TrackMarkerConstPtr shared_from_this() const {
        return boost::dynamic_pointer_cast<const TrackMarker>(KnobHolder::shared_from_this());
    }


    virtual ~TrackMarker();

    virtual bool isItemContainer() const OVERRIDE FINAL
    {
        return false;
    }


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

    int getReferenceFrame(TimeValue time, int frameStep) const;

    void getCenterKeyframes(std::set<double>* keyframes) const;

    bool isEnabled(TimeValue time) const;

    void setEnabledAtTime(TimeValue time, bool enabled);

    void setEnabledFromGui(TimeValue time, bool enabled);

    void setMotionModelFromGui(int index);

    void resetCenter();

    void resetOffset();

    void resetTrack();

    void setKeyFrameOnCenterAndPatternAtTime(TimeValue time);
    /*
       Controls animation of the center & offset not the pattern
     */
    void clearAnimation();
    void clearAnimationBeforeTime(TimeValue time);
    void clearAnimationAfterTime(TimeValue time);


    std::pair<ImagePtr, RectD> getMarkerImage(TimeValue time, const RectD& roi) ;

    RectD getMarkerImageRoI(TimeValue time) const;

    void notifyTrackingStarted();
    void notifyTrackingEnded();

    virtual std::string getBaseItemName() const OVERRIDE;

    virtual std::string getSerializationClassName() const OVERRIDE FINAL;

    KeyFrameSet getKeyFrames() const;

    void setKeyFrame(TimeValue time);

    void removeKeyFrame(TimeValue time);

    void removeAnimation();

    bool hasKeyFrameAtTime(TimeValue time) const;

    /// Overriden from CurveChangesListener
    virtual void onKeyFrameRemoved(const Curve* curve, const KeyFrame& key) OVERRIDE;
    virtual void onKeyFrameSet(const Curve* curve, const KeyFrame& key, bool added) OVERRIDE;
    virtual void onKeyFrameMoved(const Curve* curve, const KeyFrame& from, const KeyFrame& to) OVERRIDE;

Q_SIGNALS:

    void keyframeRemoved(TimeValue time);
    void keyframeAdded(TimeValue time);
    void keyframeMoved(TimeValue from, TimeValue to);
protected:

    virtual void initializeKnobs() OVERRIDE;



private:


    boost::scoped_ptr<TrackMarkerPrivate> _imp;
};

inline TrackMarkerPtr
toTrackMarker(const KnobHolderPtr& holder)
{
    return boost::dynamic_pointer_cast<TrackMarker>(holder);
}

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
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    TrackMarkerPM(const KnobItemsTablePtr& context);

public:
    static TrackMarkerPtr create(const KnobItemsTablePtr& context) WARN_UNUSED_RETURN
    {
        return TrackMarkerPtr( new TrackMarkerPM(context) );
    }

    virtual ~TrackMarkerPM();

    bool trackMarker(bool forward, int refFrame, int trackedFrame);

public Q_SLOTS:

    void onTrackerNodeInputChanged(int inputNb);

private:

    virtual void initializeKnobs() OVERRIDE FINAL;
};


inline TrackMarkerPMPtr
toTrackMarkerPM(const KnobHolderPtr& holder)
{
    return boost::dynamic_pointer_cast<TrackMarkerPM>(holder);
}


NATRON_NAMESPACE_EXIT

#endif // Engine_TrackMarker_h
