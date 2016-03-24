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

#ifndef TRACKERCONTEXT_H
#define TRACKERCONTEXT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <set>
#include <list>

#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QThread>
#include <QMutex>

#include "Engine/EngineFwd.h"
#include "Engine/RectI.h"
#include "Engine/ThreadPool.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;



class TrackArgsV1
{
    int _start,_end;
    bool _forward;
    boost::shared_ptr<TimeLine> _timeline;
    ViewerInstance* _viewer;
    std::vector<KnobButton*> _buttonInstances;
    
public:
    
    TrackArgsV1()
    : _start(0)
    , _end(0)
    , _forward(false)
    , _timeline()
    , _viewer(0)
    , _buttonInstances()
    {
        
    }
    
    TrackArgsV1(const TrackArgsV1& other)
    {
        *this = other;
    }
    
    TrackArgsV1(int start,
                int end,
                bool forward,
                const boost::shared_ptr<TimeLine>& timeline,
                ViewerInstance* viewer,
                const std::vector<KnobButton*>& instances)
    : _start(start)
    , _end(end)
    , _forward(forward)
    , _timeline(timeline)
    , _viewer(viewer)
    , _buttonInstances(instances)
    {
        
    }
    
    ~TrackArgsV1() {}
    
    void operator=(const TrackArgsV1& other)
    {
        _start = other._start;
        _end = other._end;
        _forward = other._forward;
        _timeline = other._timeline;
        _viewer = other._viewer;
        _buttonInstances = other._buttonInstances;
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
        return _forward;
    }
    
    boost::shared_ptr<TimeLine> getTimeLine() const
    {
        return _timeline;
    }
    
    ViewerInstance* getViewer() const
    {
        return _viewer;
    }
    
    const std::vector<KnobButton*>& getInstances() const
    {
        return _buttonInstances;
    }
    
    int getNumTracks() const
    {
        return (int)_buttonInstances.size();
    }
    
    void getRedrawAreasNeeded(int time, std::list<RectD>* canonicalRects) const;
    
};

class TrackerParamsProvider
{
    mutable QMutex _trackParamsMutex;
    bool _centerTrack;
    bool _updateViewer;
public:
    
    TrackerParamsProvider()
    : _trackParamsMutex()
    , _centerTrack(false)
    , _updateViewer(false)
    {
        
    }
    
    void setCenterOnTrack(bool centerTrack)
    {
        QMutexLocker k(&_trackParamsMutex);
        _centerTrack = centerTrack;
    }
    
    void setUpdateViewer(bool updateViewer)
    {
        QMutexLocker k(&_trackParamsMutex);
        _updateViewer =  updateViewer;
    }
    
    bool getCenterOnTrack() const
    {
        QMutexLocker k(&_trackParamsMutex);
        return _centerTrack;
    }
    
    bool getUpdateViewer() const
    {
        QMutexLocker k(&_trackParamsMutex);
        return _updateViewer;
    }

};

struct TrackerContextPrivate;
class TrackerContext : public QObject, public boost::enable_shared_from_this<TrackerContext>, public TrackerParamsProvider
{
    Q_OBJECT
    
public:
    
    enum TrackSelectionReason
    {
        eTrackSelectionSettingsPanel,
        eTrackSelectionViewer,
        eTrackSelectionInternal,
    };
    
    enum TrackExportTypeEnum
    {
        eTrackExportTypeCornerPinThisFrame = 0,
        eTrackExportTypeCornerPinRefFrame,
        eTrackExportTypeCornerPinThisFrameBaked,
        eTrackExportTypeCornerPinRefFrameBaked,
        eTrackExportTypeTransformStabilize,
        eTrackExportTypeTransformMatchMove,
        eTrackExportTypeTransformStabilizeBaked,
        eTrackExportTypeTransformMatchMoveBaked
        
    };
    
    TrackerContext(const boost::shared_ptr<Natron::Node> &node);
    
    ~TrackerContext();
    
    void load(const TrackerContextSerialization& serialization);
    
    void save(TrackerContextSerialization* serialization) const;

    
    boost::shared_ptr<Natron::Node> getNode() const;
    
    TrackMarkerPtr createMarker();
    
    int getMarkerIndex(const TrackMarkerPtr& marker) const;
    
    TrackMarkerPtr getPrevMarker(const TrackMarkerPtr& marker, bool loop) const;
    TrackMarkerPtr getNextMarker(const TrackMarkerPtr& marker, bool loop) const;
    
    void appendMarker(const TrackMarkerPtr& marker);
    
    void insertMarker(const TrackMarkerPtr& marker, int index);
    
    void removeMarker(const TrackMarkerPtr& marker);
    
    TrackMarkerPtr getMarkerByName(const std::string & name) const;
    
    std::string generateUniqueTrackName(const std::string& baseName);
    
    int getTimeLineFirstFrame() const;
    int getTimeLineLastFrame() const;
    
    /**
     * @brief Tracks the selected markers over the range defined by [start,end[ (end pointing to the frame
     * after the last one, a la STL).
     **/
    void trackSelectedMarkers(int start, int end, bool forward, ViewerInstance* viewer);
    void trackMarkers(const std::list<TrackMarkerPtr >& marks,
                      int start,
                      int end,
                      bool forward,
                      ViewerInstance* viewer);
    
    void abortTracking();
    
    bool isCurrentlyTracking() const;
    
    void beginEditSelection();
    
    void endEditSelection(TrackSelectionReason reason);
    
    void addTracksToSelection(const std::list<TrackMarkerPtr >& marks, TrackSelectionReason reason);
    void addTrackToSelection(const TrackMarkerPtr& mark, TrackSelectionReason reason);
    
    void removeTracksFromSelection(const std::list<TrackMarkerPtr >& marks, TrackSelectionReason reason);
    void removeTrackFromSelection(const TrackMarkerPtr& mark, TrackSelectionReason reason);
    
    void clearSelection(TrackSelectionReason reason);
    
    void selectAll(TrackSelectionReason reason);
    
    void getAllMarkers(std::vector<TrackMarkerPtr >* markers) const;
    
    void getSelectedMarkers(std::list<TrackMarkerPtr >* markers) const;
    
    bool isMarkerSelected(const TrackMarkerPtr& marker) const;
    
    static void getMotionModelsAndHelps(bool addPerspective,
                                        std::vector<std::string>* models,
                                        std::vector<std::string>* tooltips);
    
    int getTransformReferenceFrame() const;
    
    void goToPreviousKeyFrame(int time);
    void goToNextKeyFrame(int time);
    
    
    void exportTrackDataFromExportOptions();
    
    void knobChanged(KnobI* k,
                     ValueChangedReasonEnum reason,
                     ViewSpec view,
                     double time,
                     bool originatedFromMainThread);
    
    static bool trackStepV1(int trackIndex, const TrackArgsV1& args, int time);

    void declarePythonFields();

    
    /*boost::shared_ptr<KnobDouble> getSearchWindowBottomLeftKnob() const;
    boost::shared_ptr<KnobDouble> getSearchWindowTopRightKnob() const;
    boost::shared_ptr<KnobDouble> getPatternTopLeftKnob() const;
    boost::shared_ptr<KnobDouble> getPatternTopRightKnob() const;
    boost::shared_ptr<KnobDouble> getPatternBtmRightKnob() const;
    boost::shared_ptr<KnobDouble> getPatternBtmLeftKnob() const;
    boost::shared_ptr<KnobDouble> getWeightKnob() const;
    boost::shared_ptr<KnobDouble> getCenterKnob() const;
    boost::shared_ptr<KnobDouble> getOffsetKnob() const;
    boost::shared_ptr<KnobDouble> getCorrelationKnob() const;
    boost::shared_ptr<KnobChoice> getMotionModelKnob() const;*/
    
    void s_keyframeSetOnTrack(const TrackMarkerPtr& marker,int key) { Q_EMIT keyframeSetOnTrack(marker,key); }
    void s_keyframeRemovedOnTrack(const TrackMarkerPtr& marker,int key) { Q_EMIT keyframeRemovedOnTrack(marker,key); }
    void s_allKeyframesRemovedOnTrack(const TrackMarkerPtr& marker) { Q_EMIT allKeyframesRemovedOnTrack(marker); }
    
    void s_keyframeSetOnTrackCenter(const TrackMarkerPtr& marker,int key) { Q_EMIT keyframeSetOnTrackCenter(marker,key); }
    void s_keyframeRemovedOnTrackCenter(const TrackMarkerPtr& marker,int key) { Q_EMIT keyframeRemovedOnTrackCenter(marker,key); }
    void s_allKeyframesRemovedOnTrackCenter(const TrackMarkerPtr& marker) { Q_EMIT allKeyframesRemovedOnTrackCenter(marker); }
    void s_multipleKeyframesSetOnTrackCenter(const TrackMarkerPtr& marker, const std::list<double>& keys) { Q_EMIT multipleKeyframesSetOnTrackCenter(marker,keys); }
    
    void s_trackAboutToClone(const TrackMarkerPtr& marker) { Q_EMIT trackAboutToClone(marker); }
    void s_trackCloned(const TrackMarkerPtr& marker) { Q_EMIT trackCloned(marker); }
    
    void s_enabledChanged(TrackMarkerPtr marker,int reason) { Q_EMIT enabledChanged(marker, reason); }
    
    void s_centerKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT centerKnobValueChanged(marker,dimension,reason); }
    void s_offsetKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT offsetKnobValueChanged(marker,dimension,reason); }
    void s_errorKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT errorKnobValueChanged(marker,dimension,reason); }
    void s_weightKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT weightKnobValueChanged(marker,dimension,reason); }
    void s_motionModelKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT motionModelKnobValueChanged(marker,dimension,reason); }
    
   /* void s_patternTopLeftKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT patternTopLeftKnobValueChanged(marker,dimension,reason); }
    void s_patternTopRightKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT patternTopRightKnobValueChanged(marker,dimension, reason); }
    void s_patternBtmRightKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT patternBtmRightKnobValueChanged(marker,dimension, reason); }
    void s_patternBtmLeftKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT patternBtmLeftKnobValueChanged(marker,dimension, reason); }*/
    
    void s_searchBtmLeftKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT searchBtmLeftKnobValueChanged(marker,dimension,reason); }
    void s_searchTopRightKnobValueChanged(const TrackMarkerPtr& marker,int dimension,int reason) { Q_EMIT searchTopRightKnobValueChanged(marker, dimension, reason); }
    
    void s_onNodeInputChanged(int inputNb) { Q_EMIT onNodeInputChanged(inputNb); }
    
public Q_SLOTS:
    
    void onSelectedKnobCurveChanged();
    
    
Q_SIGNALS:
    
    void keyframeSetOnTrack(TrackMarkerPtr marker, int);
    void keyframeRemovedOnTrack(TrackMarkerPtr marker, int);
    void allKeyframesRemovedOnTrack(TrackMarkerPtr marker);
    
    void keyframeSetOnTrackCenter(TrackMarkerPtr marker, int);
    void keyframeRemovedOnTrackCenter(TrackMarkerPtr marker, int);
    void allKeyframesRemovedOnTrackCenter(TrackMarkerPtr marker);
    void multipleKeyframesSetOnTrackCenter(TrackMarkerPtr marker, std::list<double>);
    
    void trackAboutToClone(TrackMarkerPtr marker);
    void trackCloned(TrackMarkerPtr marker);
    
    //reason is of type TrackSelectionReason
    void selectionChanged(int reason);
    void selectionAboutToChange(int reason);
    
    void trackInserted(TrackMarkerPtr marker,int index);
    void trackRemoved(TrackMarkerPtr marker);
    
    void enabledChanged(TrackMarkerPtr marker,int reason);
    
    void centerKnobValueChanged(TrackMarkerPtr marker,int,int);
    void offsetKnobValueChanged(TrackMarkerPtr marker,int,int);
    void errorKnobValueChanged(TrackMarkerPtr marker,int,int);
    void weightKnobValueChanged(TrackMarkerPtr marker,int,int);
    void motionModelKnobValueChanged(TrackMarkerPtr marker,int,int);
    
    /*void patternTopLeftKnobValueChanged(TrackMarkerPtr marker,int,int);
    void patternTopRightKnobValueChanged(TrackMarkerPtr marker,int,int);
    void patternBtmRightKnobValueChanged(TrackMarkerPtr marker,int,int);
    void patternBtmLeftKnobValueChanged(TrackMarkerPtr marker,int,int);*/
    
    void searchBtmLeftKnobValueChanged(TrackMarkerPtr marker,int,int);
    void searchTopRightKnobValueChanged(TrackMarkerPtr marker,int,int);
    
    void trackingStarted(int);
    void trackingFinished();
    
    void onNodeInputChanged(int inputNb);
    


    
private:
    
    void removeItemAsPythonField(const TrackMarkerPtr& item);
    
    void declareItemAsPythonField(const TrackMarkerPtr& item);
    
    void endSelection(TrackSelectionReason reason);
    
    boost::scoped_ptr<TrackerContextPrivate> _imp;
};

class TrackSchedulerBase
: public QThread
#ifdef QT_CUSTOM_THREADPOOL
, public AbortableThread
#endif
{
    Q_OBJECT
    
public:
    
    TrackSchedulerBase();
    
    virtual ~TrackSchedulerBase() {}
    
    void emit_trackingStarted(int step)
    {
        Q_EMIT trackingStarted(step);
    }
    
    void emit_trackingFinished()
    {
        Q_EMIT trackingFinished();
    }
    
private Q_SLOTS:

    void doRenderCurrentFrameForViewer(ViewerInstance* viewer);
  
    
Q_SIGNALS:
    
    void trackingStarted(int step);
    
    void trackingFinished();
    

    void renderCurrentFrameForViewer(ViewerInstance* viewer);
};


struct TrackSchedulerPrivate;
template <class TrackArgsType>
class TrackScheduler : public TrackSchedulerBase
{
    
public:
    
    /*
     * @brief A pointer to a function that will be called concurrently for each Track marker to track.
     * @param index Identifies the track in args, which is supposed to hold the tracks vector.
     * @param time The time at which to track. The reference frame is held in the args and can be different for each track
     */
    typedef bool (*TrackStepFunctor)(int trackIndex, const TrackArgsType& args, int time);
    
    TrackScheduler(TrackerParamsProvider* paramsProvider, const NodeWPtr& node, TrackStepFunctor functor);
    
    virtual ~TrackScheduler();
    
    /**
     * @brief Track the selectedInstances, calling the instance change action on each button (either the previous or
     * next button) in a separate thread.
     * @param start the first frame to track, if forward is true then start < end otherwise start > end
     * @param end the next frame after the last frame to track (a la STL iterators), if forward is true then end > start
     **/
    void track(const TrackArgsType& args);
    
    void abortTracking();
    
    void quitThread();
    
    bool isWorking() const;
    
private:
    
    virtual void run() OVERRIDE FINAL;
    
    boost::scoped_ptr<TrackSchedulerPrivate> _imp;
    
    QMutex argsMutex;
    TrackArgsType curArgs,requestedArgs;
    TrackStepFunctor _functor;
};

NATRON_NAMESPACE_EXIT;

#endif // TRACKERCONTEXT_H
