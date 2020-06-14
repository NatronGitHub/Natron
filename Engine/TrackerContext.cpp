/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream> // stringstream

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)


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
#include "Engine/TrackerContextPrivate.h"
#include "Engine/TrackerSerialization.h"
#include "Engine/ViewerInstance.h"

#define NATRON_TRACKER_REPORT_PROGRESS_DELTA_MS 200

NATRON_NAMESPACE_ENTER


void
TrackerContext::getMotionModelsAndHelps(bool addPerspective,
                                            std::vector<ChoiceOption>* models)
{
    models->push_back(ChoiceOption("Trans.", "", tr(kTrackerParamMotionModelTranslation).toStdString()));
    models->push_back(ChoiceOption("Trans.+Rot.", "", tr(kTrackerParamMotionModelTransRot).toStdString()));
    models->push_back(ChoiceOption("Trans.+Scale", "", tr(kTrackerParamMotionModelTransScale).toStdString()));
    models->push_back(ChoiceOption("Trans.+Rot.+Scale", "", tr(kTrackerParamMotionModelTransRotScale).toStdString()));
    models->push_back(ChoiceOption("Affine", "", tr(kTrackerParamMotionModelAffine).toStdString()));
    if (addPerspective) {
        models->push_back(ChoiceOption("Perspective", "", tr(kTrackerParamMotionModelPerspective).toStdString()));
    }
}

TrackerContext::TrackerContext(const NodePtr &node)
    : boost::enable_shared_from_this<TrackerContext>()
    , _imp( new TrackerContextPrivate(this, node) )
{
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct TrackerContext::MakeSharedEnabler: public TrackerContext
{
    MakeSharedEnabler(const NodePtr &node) : TrackerContext(node) {
    }
};


TrackerContextPtr
TrackerContext::create(const NodePtr &node)
{
    return boost::make_shared<TrackerContext::MakeSharedEnabler>(node);
}


TrackerContext::~TrackerContext()
{
}

void
TrackerContext::load(const TrackerContextSerialization& serialization)
{
    TrackerContextPtr thisShared = shared_from_this();
    QMutexLocker k(&_imp->trackerContextMutex);

    for (std::list<TrackSerialization>::const_iterator it = serialization._tracks.begin(); it != serialization._tracks.end(); ++it) {
        TrackMarkerPtr marker;
        if ( it->usePatternMatching() ) {
            marker = TrackMarkerPM::create(thisShared);
        } else {
            marker = TrackMarker::create(thisShared);
        }
        marker->initializeKnobsPublic();
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
    std::list<TrackMarkerPtr> markers;

    getSelectedMarkers(&markers);

    int minimum = INT_MIN;
    for (std::list<TrackMarkerPtr> ::iterator it = markers.begin(); it != markers.end(); ++it) {
        int t = (*it)->getPreviousKeyframe(time);
        if ( (t != INT_MIN) && (t > minimum) ) {
            minimum = t;
        }
    }
    if (minimum != INT_MIN) {
        getNode()->getApp()->setLastViewerUsingTimeline( NodePtr() );
        getNode()->getApp()->getTimeLine()->seekFrame(minimum, false,  NULL, eTimelineChangeReasonPlaybackSeek);
    }
}

void
TrackerContext::goToNextKeyFrame(int time)
{
    std::list<TrackMarkerPtr> markers;

    getSelectedMarkers(&markers);

    int maximum = INT_MAX;
    for (std::list<TrackMarkerPtr> ::iterator it = markers.begin(); it != markers.end(); ++it) {
        int t = (*it)->getNextKeyframe(time);
        if ( (t != INT_MAX) && (t < maximum) ) {
            maximum = t;
        }
    }
    if (maximum != INT_MAX) {
        getNode()->getApp()->setLastViewerUsingTimeline( NodePtr() );
        getNode()->getApp()->getTimeLine()->seekFrame(maximum, false,  NULL, eTimelineChangeReasonPlaybackSeek);
    }
}

TrackMarkerPtr
TrackerContext::getMarkerByName(const std::string & name) const
{
    QMutexLocker k(&_imp->trackerContextMutex);

    for (std::vector<TrackMarkerPtr>::const_iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
        if ( (*it)->getScriptName_mt_safe() == name ) {
            return *it;
        }
    }

    return TrackMarkerPtr();
}

void
TrackerContext::setFromPointsToInputRod()
{
    RectD inputRod = _imp->getInputRoDAtTime( getNode()->getApp()->getTimeLine()->currentFrame() );
    KnobDoublePtr fromPointsKnob[4];

    for (int i = 0; i < 4; ++i) {
        fromPointsKnob[i] = _imp->fromPoints[i].lock();
    }
    fromPointsKnob[0]->setValues(inputRod.x1, inputRod.y1, ViewSpec::all(), eValueChangedReasonPluginEdited, 0);
    fromPointsKnob[1]->setValues(inputRod.x2, inputRod.y1, ViewSpec::all(), eValueChangedReasonPluginEdited, 0);
    fromPointsKnob[2]->setValues(inputRod.x2, inputRod.y2, ViewSpec::all(), eValueChangedReasonPluginEdited, 0);
    fromPointsKnob[3]->setValues(inputRod.x1, inputRod.y2, ViewSpec::all(), eValueChangedReasonPluginEdited, 0);
}

void
TrackerContext::inputChanged(int inputNb)
{
    // If the cornerPin from points have never been computed, set them
    KnobBoolPtr fromPointsSetOnceKnob = _imp->cornerPinFromPointsSetOnceAutomatically.lock();

    if ( !fromPointsSetOnceKnob->getValue() ) {
        setFromPointsToInputRod();
        fromPointsSetOnceKnob->setValue(true);
    }
    s_onNodeInputChanged(inputNb);
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
        if ( getMarkerByName(name) ) {
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
    TrackMarkerPtr track;

#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    bool usePM = isTrackerPMEnabled();
    if (!usePM) {
        track = TrackMarker::create( shared_from_this() );
    } else {
        track = TrackMarkerPM::create( shared_from_this() );
    }
#else
    track = TrackMarker::create( shared_from_this() );
#endif

    int index;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        index  = _imp->markers.size();
        _imp->markers.push_back(track);
    }
    
    track->initializeKnobsPublic();
    std::string name = generateUniqueTrackName(kTrackBaseName);

    track->setScriptName(name);
    track->setLabel(name);
    track->resetCenter();

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
TrackerContext::getPrevMarker(const TrackMarkerPtr& marker,
                              bool loop) const
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
TrackerContext::getNextMarker(const TrackMarkerPtr& marker,
                              bool loop) const
{
    QMutexLocker k(&_imp->trackerContextMutex);

    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if (_imp->markers[i] == marker) {
            if ( i < (_imp->markers.size() - 1) ) {
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
TrackerContext::insertMarker(const TrackMarkerPtr& marker,
                             int index)
{
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        assert(index >= 0);
        if ( index >= (int)_imp->markers.size() ) {
            _imp->markers.push_back(marker);
        } else {
            std::vector<TrackMarkerPtr>::iterator it = _imp->markers.begin();
            std::advance(it, index);
            _imp->markers.insert(it, marker);
        }
    }
    declareItemAsPythonField(marker);
    Q_EMIT trackInserted(marker, index);
}

void
TrackerContext::removeMarker(const TrackMarkerPtr& marker)
{
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        for (std::vector<TrackMarkerPtr>::iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
            if (*it == marker) {
                _imp->markers.erase(it);
                break;
            }
        }
    }
    Q_EMIT trackRemoved(marker);

    removeItemAsPythonField(marker);
    beginEditSelection(TrackerContext::eTrackSelectionInternal);
    removeTrackFromSelection(marker, TrackerContext::eTrackSelectionInternal);
    endEditSelection(TrackerContext::eTrackSelectionInternal);
}

NodePtr
TrackerContext::getNode() const
{
    return _imp->node.lock();
}

KnobChoicePtr
TrackerContext::getCorrelationScoreTypeKnob() const
{
#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM

    return _imp->patternMatchingScore.lock();
#else

    return KnobChoicePtr();
#endif
}

KnobBoolPtr
TrackerContext::getEnabledKnob() const
{
    return _imp->activateTrack.lock();
}

KnobPagePtr
TrackerContext::getTrackingPageKnob() const
{
    return _imp->trackingPageKnob.lock();
}

KnobIntPtr
TrackerContext::getDefaultMarkerPatternWinSizeKnob() const
{
    return _imp->defaultPatternWinSize.lock();
}

KnobIntPtr
TrackerContext::getDefaultMarkerSearchWinSizeKnob() const
{
    return _imp->defaultSearchWinSize.lock();
}

KnobChoicePtr
TrackerContext::getDefaultMotionModelKnob() const
{
    return _imp->defaultMotionModel.lock();
}

bool
TrackerContext::isTrackerPMEnabled() const
{
#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM

    return _imp->usePatternMatching.lock()->getValue();
#else

    return false;
#endif
}

int
TrackerContext::getTimeLineFirstFrame() const
{
    NodePtr node = getNode();

    if (!node) {
        return -1;
    }
    double first, last;
    node->getApp()->getProject()->getFrameRange(&first, &last);

    return (int)first;
}

int
TrackerContext::getTimeLineLastFrame() const
{
    NodePtr node = getNode();

    if (!node) {
        return -1;
    }
    double first, last;
    node->getApp()->getProject()->getFrameRange(&first, &last);

    return (int)last;
}

void
TrackerContext::trackSelectedMarkers(int start,
                                     int end,
                                     int frameStep,
                                     OverlaySupport* viewer)
{
    std::list<TrackMarkerPtr> markers;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        for (std::list<TrackMarkerPtr>::iterator it = _imp->selectedMarkers.begin();
             it != _imp->selectedMarkers.end(); ++it) {
            double time = (*it)->getCurrentTime();
            if ( (*it)->isEnabled(time) ) {
                markers.push_back(*it);
            }
        }
    }

    trackMarkers(markers, start, end, frameStep, viewer);
}

bool
TrackerContext::isCurrentlyTracking() const
{
    return _imp->scheduler.isWorking();
}

void
TrackerContext::abortTracking()
{
    _imp->scheduler.abortThreadedTask();
}

void
TrackerContext::abortTracking_blocking()
{
    _imp->scheduler.abortThreadedTask();
    _imp->scheduler.waitForAbortToComplete_enforce_blocking();
}

void
TrackerContext::quitTrackerThread_non_blocking()
{
    _imp->scheduler.quitThread(true);
}

bool
TrackerContext::hasTrackerThreadQuit() const
{
    return !_imp->scheduler.isRunning();
}

void
TrackerContext::quitTrackerThread_blocking(bool allowRestart)
{
    _imp->scheduler.quitThread(allowRestart);
    _imp->scheduler.waitForThreadToQuit_enforce_blocking();
}

void
TrackerContext::beginEditSelection(TrackSelectionReason reason)
{
    QMutexLocker k(&_imp->trackerContextMutex);

    k.unlock();
    Q_EMIT selectionAboutToChange( (int)reason );
    k.relock();
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
TrackerContext::addTrackToSelection(const TrackMarkerPtr& mark,
                                    TrackSelectionReason reason)
{
    std::list<TrackMarkerPtr> marks;

    marks.push_back(mark);
    addTracksToSelection(marks, reason);
}

void
TrackerContext::addTracksToSelection(const std::list<TrackMarkerPtr>& marks,
                                     TrackSelectionReason reason)
{
    bool hasCalledBegin = false;
    {
        QMutexLocker k(&_imp->trackerContextMutex);

        if (!_imp->beginSelectionCounter) {
            k.unlock();
            Q_EMIT selectionAboutToChange( (int)reason );
            k.relock();
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }

        for (std::list<TrackMarkerPtr>::const_iterator it = marks.begin(); it != marks.end(); ++it) {
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
TrackerContext::removeTrackFromSelection(const TrackMarkerPtr& mark,
                                         TrackSelectionReason reason)
{
    std::list<TrackMarkerPtr> marks;

    marks.push_back(mark);
    removeTracksFromSelection(marks, reason);
}

void
TrackerContext::removeTracksFromSelection(const std::list<TrackMarkerPtr>& marks,
                                          TrackSelectionReason reason)
{
    bool hasCalledBegin = false;

    {
        QMutexLocker k(&_imp->trackerContextMutex);

        if (!_imp->beginSelectionCounter) {
            k.unlock();
            Q_EMIT selectionAboutToChange( (int)reason );
            k.relock();
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }

        for (std::list<TrackMarkerPtr>::const_iterator it = marks.begin(); it != marks.end(); ++it) {
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
    std::list<TrackMarkerPtr> markers;

    getSelectedMarkers(&markers);
    if ( markers.empty() ) {
        return;
    }
    removeTracksFromSelection(markers, reason);
}

void
TrackerContext::selectAll(TrackSelectionReason reason)
{
    beginEditSelection(reason);
    std::vector<TrackMarkerPtr> markers;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        markers = _imp->markers;
    }
    int time = getNode()->getApp()->getTimeLine()->currentFrame();
    for (std::vector<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        if ( (*it)->isEnabled(time) ) {
            addTrackToSelection(*it, reason);
        }
    }
    endEditSelection(reason);
}

void
TrackerContext::getAllMarkers(std::vector<TrackMarkerPtr>* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);

    *markers = _imp->markers;
}

void
TrackerContext::getAllEnabledMarkers(std::vector<TrackMarkerPtr>* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);

    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if ( _imp->markers[i]->isEnabled( _imp->markers[i]->getCurrentTime() ) ) {
            markers->push_back(_imp->markers[i]);
        }
    }
}

void
TrackerContext::getSelectedMarkers(std::list<TrackMarkerPtr>* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);

    *markers = _imp->selectedMarkers;
}

bool
TrackerContext::isMarkerSelected(const TrackMarkerPtr& marker) const
{
    QMutexLocker k(&_imp->trackerContextMutex);

    for (std::list<TrackMarkerPtr>::const_iterator it = _imp->selectedMarkers.begin(); it != _imp->selectedMarkers.end(); ++it) {
        if (*it == marker) {
            return true;
        }
    }

    return false;
}

void
TrackerContext::endSelection(TrackSelectionReason reason)
{
    assert( QThread::currentThread() == qApp->thread() );

    {
        QMutexLocker k(&_imp->trackerContextMutex);
        if (_imp->selectionRecursion > 0) {
            _imp->markersToSlave.clear();
            _imp->markersToUnslave.clear();

            return;
        }
        if ( _imp->markersToSlave.empty() && _imp->markersToUnslave.empty() ) {
            return;
        }
    }
    ++_imp->selectionRecursion;


    {
        QMutexLocker k(&_imp->trackerContextMutex);

        // Slave newly selected knobs
        bool selectionIsDirty = _imp->selectedMarkers.size() > 1;
        bool selectionEmpty = _imp->selectedMarkers.empty();


        _imp->linkMarkerKnobsToGuiKnobs(_imp->markersToUnslave, selectionIsDirty, false);
        _imp->markersToUnslave.clear();


        _imp->linkMarkerKnobsToGuiKnobs(_imp->markersToSlave, selectionIsDirty, true);
        _imp->markersToSlave.clear();


        for (std::list<KnobIWPtr>::iterator it = _imp->perTrackKnobs.begin(); it != _imp->perTrackKnobs.end(); ++it) {
            KnobIPtr k = it->lock();
            if (!k) {
                continue;
            }
            k->setAllDimensionsEnabled(!selectionEmpty);
            k->setDirty(selectionIsDirty);
        }
    } //  QMutexLocker k(&_imp->trackerContextMutex);
    Q_EMIT selectionChanged( (int)reason );

    --_imp->selectionRecursion;
}

#if 0
static
KnobDoublePtr
getCornerPinPoint(Node* node,
                  bool isFrom,
                  int index)
{
    assert(0 <= index && index < 4);
    QString name = isFrom ? QString::fromUtf8("from%1").arg(index + 1) : QString::fromUtf8("to%1").arg(index + 1);
    KnobIPtr knob = node->getKnobByName( name.toStdString() );
    assert(knob);
    KnobDoublePtr  ret = boost::dynamic_pointer_cast<KnobDouble>(knob);
    assert(ret);

    return ret;
}

#endif

static
KnobDoublePtr
getCornerPinPoint(Node* node,
                  bool isFrom,
                  int index)
{
    assert(0 <= index && index < 4);
    QString name = isFrom ? QString::fromUtf8("from%1").arg(index + 1) : QString::fromUtf8("to%1").arg(index + 1);
    KnobIPtr knob = node->getKnobByName( name.toStdString() );
    assert(knob);
    KnobDoublePtr  ret = boost::dynamic_pointer_cast<KnobDouble>(knob);
    assert(ret);

    return ret;
}

void
TrackerContext::exportTrackDataFromExportOptions()
{
    //bool transformLink = _imp->exportLink.lock()->getValue();
    KnobChoicePtr transformTypeKnob = _imp->transformType.lock();

    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    KnobChoicePtr motionTypeKnob = _imp->motionType.lock();
    if (!motionTypeKnob) {
        return;
    }
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum mt = (TrackerMotionTypeEnum)motionType_i;

    if (mt == eTrackerMotionTypeNone) {
        Dialogs::errorDialog( tr("Tracker Export").toStdString(), tr("Please select the export mode with the Motion Type parameter").toStdString() );

        return;
    }

    bool linked = _imp->exportLink.lock()->getValue();
    QString pluginID;
    switch (transformType) {
    case eTrackerTransformNodeCornerPin:
        pluginID = QString::fromUtf8(PLUGINID_OFX_CORNERPIN);
        break;
    case eTrackerTransformNodeTransform:
        pluginID = QString::fromUtf8(PLUGINID_OFX_TRANSFORM);
        break;
    }

    NodePtr thisNode = getNode();
    AppInstancePtr app = thisNode->getApp();
    CreateNodeArgs args( pluginID.toStdString(), thisNode->getGroup() );
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    NodePtr createdNode = app->createNode(args);
    if (!createdNode) {
        return;
    }

    // Move the new node
    double thisNodePos[2];
    double thisNodeSize[2];
    thisNode->getPosition(&thisNodePos[0], &thisNodePos[1]);
    thisNode->getSize(&thisNodeSize[0], &thisNodeSize[1]);
    createdNode->setPosition(thisNodePos[0] + thisNodeSize[0] * 2., thisNodePos[1]);

    int timeForFromPoints = getTransformReferenceFrame();


    switch (transformType) {
    case eTrackerTransformNodeCornerPin: {
        KnobDoublePtr cornerPinToPoints[4];
        KnobDoublePtr cornerPinFromPoints[4];


        for (unsigned int i = 0; i < 4; ++i) {
            cornerPinFromPoints[i] = getCornerPinPoint(createdNode.get(), true, i);
            assert(cornerPinFromPoints[i]);
            for (int j = 0; j < cornerPinFromPoints[i]->getDimension(); ++j) {
                cornerPinFromPoints[i]->setValue(_imp->fromPoints[i].lock()->getValueAtTime(timeForFromPoints, j), ViewSpec(0), j);
            }

            cornerPinToPoints[i] = getCornerPinPoint(createdNode.get(), false, i);
            assert(cornerPinToPoints[i]);
            if (!linked) {
                cornerPinToPoints[i]->cloneAndUpdateGui( _imp->toPoints[i].lock().get() );
            } else {
                bool ok = false;
                for (int d = 0; d < cornerPinToPoints[i]->getDimension(); ++d) {
                    ok = dynamic_cast<KnobI*>( cornerPinToPoints[i].get() )->slaveTo(d, _imp->toPoints[i].lock(), d);
                }
                (void)ok;
                assert(ok);
            }
        }
        {
            KnobIPtr knob = createdNode->getKnobByName(kCornerPinParamMatrix);
            if (knob) {
                KnobDouble* isType = dynamic_cast<KnobDouble*>( knob.get() );
                if (isType) {
                    isType->cloneAndUpdateGui( _imp->cornerPinMatrix.lock().get() );
                }
            }
        }
        break;
    }
    case eTrackerTransformNodeTransform: {
        KnobIPtr translateKnob = createdNode->getKnobByName(kTransformParamTranslate);
        if (translateKnob) {
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>( translateKnob.get() );
            if (isDbl) {
                if (!linked) {
                    isDbl->cloneAndUpdateGui( _imp->translate.lock().get() );
                } else {
                    dynamic_cast<KnobI*>(isDbl)->slaveTo(0, _imp->translate.lock(), 0);
                    dynamic_cast<KnobI*>(isDbl)->slaveTo(1, _imp->translate.lock(), 1);
                }
            }
        }

        KnobIPtr scaleKnob = createdNode->getKnobByName(kTransformParamScale);
        if (scaleKnob) {
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>( scaleKnob.get() );
            if (isDbl) {
                if (!linked) {
                    isDbl->cloneAndUpdateGui( _imp->scale.lock().get() );
                } else {
                    dynamic_cast<KnobI*>(isDbl)->slaveTo(0, _imp->scale.lock(), 0);
                    dynamic_cast<KnobI*>(isDbl)->slaveTo(1, _imp->scale.lock(), 1);
                }
            }
        }

        KnobIPtr rotateKnob = createdNode->getKnobByName(kTransformParamRotate);
        if (rotateKnob) {
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>( rotateKnob.get() );
            if (isDbl) {
                if (!linked) {
                    isDbl->cloneAndUpdateGui( _imp->rotate.lock().get() );
                } else {
                    dynamic_cast<KnobI*>(isDbl)->slaveTo(0, _imp->rotate.lock(), 0);
                }
            }
        }
        KnobIPtr centerKnob = createdNode->getKnobByName(kTransformParamCenter);
        if (centerKnob) {
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>( centerKnob.get() );
            if (isDbl) {
                isDbl->cloneAndUpdateGui( _imp->center.lock().get() );
            }
        }
        break;
    }
    } // switch

    KnobIPtr cpInvertKnob = createdNode->getKnobByName(kTransformParamInvert);
    if (cpInvertKnob) {
        KnobBool* isBool = dynamic_cast<KnobBool*>( cpInvertKnob.get() );
        if (isBool) {
            if (!linked) {
                isBool->cloneAndUpdateGui( _imp->invertTransform.lock().get() );
            } else {
                dynamic_cast<KnobI*>(isBool)->slaveTo(0, _imp->invertTransform.lock(), 0);
            }
        }
    }

    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamMotionBlur);
        if (knob) {
            KnobDouble* isType = dynamic_cast<KnobDouble*>( knob.get() );
            if (isType) {
                isType->cloneAndUpdateGui( _imp->motionBlur.lock().get() );
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamShutter);
        if (knob) {
            KnobDouble* isType = dynamic_cast<KnobDouble*>( knob.get() );
            if (isType) {
                isType->cloneAndUpdateGui( _imp->shutter.lock().get() );
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamShutterOffset);
        if (knob) {
            KnobChoice* isType = dynamic_cast<KnobChoice*>( knob.get() );
            if (isType) {
                isType->cloneAndUpdateGui( _imp->shutterOffset.lock().get() );
            }
        }
    }
    {
        KnobIPtr knob = createdNode->getKnobByName(kTransformParamCustomShutterOffset);
        if (knob) {
            KnobDouble* isType = dynamic_cast<KnobDouble*>( knob.get() );
            if (isType) {
                isType->cloneAndUpdateGui( _imp->customShutterOffset.lock().get() );
            }
        }
    }
} // TrackerContext::exportTrackDataFromExportOptions

void
TrackerContext::onMarkerEnabledChanged(int reason)
{
    TrackMarker* m = qobject_cast<TrackMarker*>( sender() );

    if (!m) {
        return;
    }

    Q_EMIT enabledChanged(m->shared_from_this(), reason);
}

void
TrackerContext::onKnobsLoaded()
{
    _imp->setSolverParamsEnabled(true);

    _imp->refreshVisibilityFromTransformType();
}

bool
TrackerContext::knobChanged(KnobI* k,
                            ValueChangedReasonEnum /*reason*/,
                            ViewSpec /*view*/,
                            double /*time*/,
                            bool /*originatedFromMainThread*/)
{
    bool ret = true;

    if ( k == _imp->exportButton.lock().get() ) {
        exportTrackDataFromExportOptions();
    } else if ( k == _imp->setCurrentFrameButton.lock().get() ) {
        KnobIntPtr refFrame = _imp->referenceFrame.lock();
        refFrame->setValue( _imp->node.lock()->getApp()->getTimeLine()->currentFrame() );
    } else if ( k == _imp->transformType.lock().get() ) {
        solveTransformParamsIfAutomatic();
        _imp->refreshVisibilityFromTransformType();
    } else if ( k == _imp->motionType.lock().get() ) {
        solveTransformParamsIfAutomatic();
        _imp->refreshVisibilityFromTransformType();
    } else if ( k == _imp->jitterPeriod.lock().get() ) {
        solveTransformParamsIfAutomatic();
    } else if ( k == _imp->smoothCornerPin.lock().get() ) {
        solveTransformParamsIfAutomatic();
    } else if ( k == _imp->smoothTransform.lock().get() ) {
        solveTransformParamsIfAutomatic();
    } else if ( k == _imp->referenceFrame.lock().get() ) {
        solveTransformParamsIfAutomatic();
    } else if ( k == _imp->robustModel.lock().get() ) {
        solveTransformParamsIfAutomatic();
    } else if ( k == _imp->generateTransformButton.lock().get() ) {
        solveTransformParams();
    } else if ( k == _imp->setFromPointsToInputRod.lock().get() ) {
        setFromPointsToInputRod();
        solveTransformParamsIfAutomatic();
    } else if ( k == _imp->autoGenerateTransform.lock().get() ) {
        solveTransformParams();
        _imp->refreshVisibilityFromTransformType();
    }
#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    else if ( k == _imp->usePatternMatching.lock().get() ) {
        _imp->refreshVisibilityFromTransformType();
    }
#endif
    else if ( k == _imp->disableTransform.lock().get() ) {
        _imp->refreshVisibilityFromTransformType();
    } else {
        ret = false;
    }

    return ret;
}

void
TrackerContext::removeItemAsPythonField(const TrackMarkerPtr& item)
{
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = "del " + nodeFullName + ".tracker." + item->getScriptName_mt_safe() + "\n";

    if ( !appPTR->isBackground() ) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
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
    std::string itemName = item->getScriptName_mt_safe();
    std::stringstream ss;

    ss << nodeFullName << ".tracker." << itemName << " = ";
    ss << nodeFullName << ".tracker.getTrackByName(\"" << itemName + "\")\n";


    const KnobsVec& knobs = item->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss << nodeFullName << ".tracker." << itemName << "." << (*it)->getName() << " = ";
        ss << nodeFullName << ".tracker." << itemName << ".getParam(\"" << (*it)->getName() << "\")\n";
    }
    std::string script = ss.str();
    if ( !appPTR->isBackground() ) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
}

void
TrackerContext::declarePythonFields()
{
    std::vector<TrackMarkerPtr> markers;

    getAllMarkers(&markers);
    for (std::vector<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        declareItemAsPythonField(*it);
    }
}

void
TrackerContext::resetTransformCenter()
{
    std::vector<TrackMarkerPtr> tracks;

    getAllEnabledMarkers(&tracks);

    double time = (double)getTransformReferenceFrame();
    Point center;
    if ( tracks.empty() ) {
        RectD rod = _imp->getInputRoDAtTime(time);
        center.x = (rod.x1 + rod.x2) / 2.;
        center.y = (rod.y1 + rod.y2) / 2.;
    } else {
        center.x = center.y = 0.;
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            KnobDoublePtr centerKnob = tracks[i]->getCenterKnob();
            center.x += centerKnob->getValueAtTime(time, 0);
            center.y += centerKnob->getValueAtTime(time, 1);
        }
        center.x /= tracks.size();
        center.y /= tracks.size();
    }

    KnobDoublePtr centerKnob = _imp->center.lock();
    centerKnob->resetToDefaultValue(0);
    centerKnob->resetToDefaultValue(1);
    centerKnob->setValues(center.x, center.y, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
}

void
TrackerContext::solveTransformParamsIfAutomatic()
{
    if ( _imp->isTransformAutoGenerationEnabled() ) {
        solveTransformParams();
    } else {
        _imp->setTransformOutOfDate(true);
    }
}

void
TrackerContext::solveTransformParams()
{
    _imp->setTransformOutOfDate(false);

    std::vector<TrackMarkerPtr> markers;

    getAllMarkers(&markers);
    if ( markers.empty() ) {
        return;
    }

    _imp->resetTransformParamsAnimation();

    KnobChoicePtr motionTypeKnob = _imp->motionType.lock();
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum type =  (TrackerMotionTypeEnum)motionType_i;
    double refTime = (double)getTransformReferenceFrame();
    int jitterPeriod = 0;
    bool jitterAdd = false;
    switch (type) {
    case eTrackerMotionTypeNone:

        return;
    case eTrackerMotionTypeMatchMove:
    case eTrackerMotionTypeStabilize:
        break;
    case eTrackerMotionTypeAddJitter:
    case eTrackerMotionTypeRemoveJitter: {
        jitterPeriod = _imp->jitterPeriod.lock()->getValue();
        jitterAdd = type == eTrackerMotionTypeAddJitter;
        break;
    }
    }

    _imp->setSolverParamsEnabled(false);

    std::set<double> keyframes;
    {
        for (std::size_t i = 0; i < markers.size(); ++i) {
            std::set<double> keys;
            markers[i]->getCenterKeyframes(&keys);
            for (std::set<double>::iterator it = keys.begin(); it != keys.end(); ++it) {
                keyframes.insert(*it);
            }
        }
    }
    KnobChoicePtr transformTypeKnob = _imp->transformType.lock();
    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    NodePtr node = getNode();
    node->getEffectInstance()->beginChanges();


    if (type == eTrackerMotionTypeStabilize) {
        _imp->invertTransform.lock()->setValue(true);
    } else {
        _imp->invertTransform.lock()->setValue(false);
    }

    KnobDoublePtr centerKnob = _imp->center.lock();

    // Set the center at the reference frame
    Point centerValue = {0, 0};
    int nSamplesAtRefTime = 0;
    for (std::size_t i = 0; i < markers.size(); ++i) {
        if ( !markers[i]->isEnabled(refTime) ) {
            continue;
        }
        KnobDoublePtr markerCenterKnob = markers[i]->getCenterKnob();

        centerValue.x += markerCenterKnob->getValueAtTime(refTime, 0);
        centerValue.y += markerCenterKnob->getValueAtTime(refTime, 1);
        ++nSamplesAtRefTime;
    }
    if (nSamplesAtRefTime) {
        centerValue.x /= nSamplesAtRefTime;
        centerValue.y /= nSamplesAtRefTime;
        centerKnob->setValues(centerValue.x, centerValue.y, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
    }

    bool robustModel;
    robustModel = _imp->robustModel.lock()->getValue();

    KnobDoublePtr maxFittingErrorKnob = _imp->fittingErrorWarnIfAbove.lock();
    const double maxFittingError = maxFittingErrorKnob->getValue();

    node->getApp()->progressStart( node, tr("Solving for transform parameters...").toStdString(), std::string() );

    _imp->lastSolveRequest.refTime = refTime;
    _imp->lastSolveRequest.jitterPeriod = jitterPeriod;
    _imp->lastSolveRequest.jitterAdd = jitterAdd;
    _imp->lastSolveRequest.allMarkers = markers;
    _imp->lastSolveRequest.keyframes = keyframes;
    _imp->lastSolveRequest.robustModel = robustModel;
    _imp->lastSolveRequest.maxFittingError = maxFittingError;

    switch (transformType) {
    case eTrackerTransformNodeTransform:
        _imp->computeTransformParamsFromTracks();
        break;
    case eTrackerTransformNodeCornerPin:
        _imp->computeCornerParamsFromTracks();
        break;
    }
} // TrackerContext::solveTransformParams

NodePtr
TrackerContext::getCurrentlySelectedTransformNode() const
{
    KnobChoicePtr transformTypeKnob = _imp->transformType.lock();

    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    switch (transformType) {
    case eTrackerTransformNodeTransform:

        return _imp->transformNode.lock();
    case eTrackerTransformNodeCornerPin:

        return _imp->cornerPinNode.lock();
    }

    return NodePtr();
}

void
TrackerContext::drawInternalNodesOverlay(double time,
                                         const RenderScale& renderScale,
                                         ViewIdx view,
                                         OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        thisNode->getEffectInstance()->drawOverlay_public(time, renderScale, view);
        //thisNode->drawHostOverlay(time, renderScale, view);
    }
}

bool
TrackerContext::onOverlayPenDownInternalNodes(double time,
                                              const RenderScale & renderScale,
                                              ViewIdx view,
                                              const QPointF & viewportPos,
                                              const QPointF & pos,
                                              double pressure,
                                              double timestamp,
                                              PenType pen,
                                              OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayPenDown_public(time, renderScale, view, viewportPos, pos, pressure, timestamp, pen) ) {
            return true;
        }
    }

    return false;
}

bool
TrackerContext::onOverlayPenMotionInternalNodes(double time,
                                                const RenderScale & renderScale,
                                                ViewIdx view,
                                                const QPointF & viewportPos,
                                                const QPointF & pos,
                                                double pressure,
                                                double timestamp,
                                                OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayPenMotion_public(time, renderScale, view, viewportPos, pos, pressure, timestamp) ) {
            return true;
        }
    }

    return false;
}

bool
TrackerContext::onOverlayPenUpInternalNodes(double time,
                                            const RenderScale & renderScale,
                                            ViewIdx view,
                                            const QPointF & viewportPos,
                                            const QPointF & pos,
                                            double pressure,
                                            double timestamp,
                                            OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayPenUp_public(time, renderScale, view, viewportPos, pos, pressure, timestamp) ) {
            return true;
        }
    }

    return false;
}

bool
TrackerContext::onOverlayKeyDownInternalNodes(double time,
                                              const RenderScale & renderScale,
                                              ViewIdx view,
                                              Key key,
                                              KeyboardModifiers modifiers,
                                              OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayKeyDown_public(time, renderScale, view, key, modifiers) ) {
            return true;
        }
    }

    return false;
}

bool
TrackerContext::onOverlayKeyUpInternalNodes(double time,
                                            const RenderScale & renderScale,
                                            ViewIdx view,
                                            Key key,
                                            KeyboardModifiers modifiers,
                                            OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayKeyUp_public(time, renderScale, view, key, modifiers) ) {
            return true;
        }
    }

    return false;
}

bool
TrackerContext::onOverlayKeyRepeatInternalNodes(double time,
                                                const RenderScale & renderScale,
                                                ViewIdx view,
                                                Key key,
                                                KeyboardModifiers modifiers,
                                                OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayKeyRepeat_public(time, renderScale, view, key, modifiers) ) {
            return true;
        }
    }

    return false;
}

bool
TrackerContext::onOverlayFocusGainedInternalNodes(double time,
                                                  const RenderScale & renderScale,
                                                  ViewIdx view,
                                                  OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayFocusGained_public(time, renderScale, view) ) {
            return true;
        }
    }

    return false;
}

bool
TrackerContext::onOverlayFocusLostInternalNodes(double time,
                                                const RenderScale & renderScale,
                                                ViewIdx view,
                                                OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayFocusLost_public(time, renderScale, view) ) {
            return true;
        }
    }

    return false;
}

void
TrackerContext::onSchedulerTrackingStarted(int frameStep)
{
    getNode()->getApp()->progressStart(getNode(), tr("Tracking...").toStdString(), "");
    Q_EMIT trackingStarted(frameStep);
}

void
TrackerContext::onSchedulerTrackingFinished()
{
    getNode()->getApp()->progressEnd( getNode() );
    Q_EMIT trackingFinished();
}

void
TrackerContext::onSchedulerTrackingProgress(double progress)
{
    if ( getNode()->getApp() && !getNode()->getApp()->progressUpdate(getNode(), progress) ) {
        _imp->scheduler.abortThreadedTask();
    }
}

//////////////////////// TrackScheduler

struct TrackArgsPrivate
{
    int start, end;
    int step;
    TimeLinePtr timeline;
    ViewerInstance* viewer;
    mv::AutoTrackPtr libmvAutotrack;
    TrackerFrameAccessorPtr fa;
    std::vector<TrackMarkerAndOptionsPtr> tracks;

    //Store the format size because LibMV internally has a top-down Y axis
    double formatWidth, formatHeight;
    mutable QMutex autoTrackMutex;
    
    bool autoKeyingOnEnabledParamEnabled;

    TrackArgsPrivate()
        : start(0)
        , end(0)
        , step(1)
        , timeline()
        , viewer(0)
        , libmvAutotrack()
        , fa()
        , tracks()
        , formatWidth(0)
        , formatHeight(0)
        , autoKeyingOnEnabledParamEnabled(false)
    {
    }
};

TrackArgs::TrackArgs()
    : _imp( new TrackArgsPrivate() )
{
}

TrackArgs::TrackArgs(int start,
                     int end,
                     int step,
                     const TimeLinePtr& timeline,
                     ViewerInstance* viewer,
                     const mv::AutoTrackPtr& autoTrack,
                     const TrackerFrameAccessorPtr& fa,
                     const std::vector<TrackMarkerAndOptionsPtr>& tracks,
                     double formatWidth,
                     double formatHeight,
                     bool autoKeyEnabled)
    : GenericThreadStartArgs()
    , _imp( new TrackArgsPrivate() )
{
    _imp->start = start;
    _imp->end = end;
    _imp->step = step;
    _imp->timeline = timeline;
    _imp->viewer = viewer;
    _imp->libmvAutotrack = autoTrack;
    _imp->fa = fa;
    _imp->tracks = tracks;
    _imp->formatWidth = formatWidth;
    _imp->formatHeight = formatHeight;
    _imp->autoKeyingOnEnabledParamEnabled = autoKeyEnabled;
}

TrackArgs::~TrackArgs()
{
}

TrackArgs::TrackArgs(const TrackArgs& other)
    : GenericThreadStartArgs()
    , _imp( new TrackArgsPrivate() )
{
    *this = other;
}

void
TrackArgs::operator=(const TrackArgs& other)
{
    _imp->start = other._imp->start;
    _imp->end = other._imp->end;
    _imp->step = other._imp->step;
    _imp->timeline = other._imp->timeline;
    _imp->viewer = other._imp->viewer;
    _imp->libmvAutotrack = other._imp->libmvAutotrack;
    _imp->fa = other._imp->fa;
    _imp->tracks = other._imp->tracks;
    _imp->formatWidth = other._imp->formatWidth;
    _imp->formatHeight = other._imp->formatHeight;
    _imp->autoKeyingOnEnabledParamEnabled = other._imp->autoKeyingOnEnabledParamEnabled;
}

bool
TrackArgs::isAutoKeyingEnabledParamEnabled() const
{
    return _imp->autoKeyingOnEnabledParamEnabled;
}

double
TrackArgs::getFormatHeight() const
{
    return _imp->formatHeight;
}

double
TrackArgs::getFormatWidth() const
{
    return _imp->formatWidth;
}

QMutex*
TrackArgs::getAutoTrackMutex() const
{
    return &_imp->autoTrackMutex;
}

int
TrackArgs::getStart() const
{
    return _imp->start;
}

int
TrackArgs::getEnd() const
{
    return _imp->end;
}

int
TrackArgs::getStep() const
{
    return _imp->step;
}

TimeLinePtr
TrackArgs::getTimeLine() const
{
    return _imp->timeline;
}

ViewerInstance*
TrackArgs::getViewer() const
{
    return _imp->viewer;
}

int
TrackArgs::getNumTracks() const
{
    return (int)_imp->tracks.size();
}

const std::vector<TrackMarkerAndOptionsPtr>&
TrackArgs::getTracks() const
{
    return _imp->tracks;
}

mv::AutoTrackPtr
TrackArgs::getLibMVAutoTrack() const
{
    return _imp->libmvAutotrack;
}

void
TrackArgs::getEnabledChannels(bool* r,
                              bool* g,
                              bool* b) const
{
    _imp->fa->getEnabledChannels(r, g, b);
}

void
TrackArgs::getRedrawAreasNeeded(int time,
                                std::list<RectD>* canonicalRects) const
{
    for (std::vector<TrackMarkerAndOptionsPtr>::const_iterator it = _imp->tracks.begin(); it != _imp->tracks.end(); ++it) {
        if ( !(*it)->natronMarker->isEnabled(time) ) {
            continue;
        }
        KnobDoublePtr searchBtmLeft = (*it)->natronMarker->getSearchWindowBottomLeftKnob();
        KnobDoublePtr searchTopRight = (*it)->natronMarker->getSearchWindowTopRightKnob();
        KnobDoublePtr centerKnob = (*it)->natronMarker->getCenterKnob();
        KnobDoublePtr offsetKnob = (*it)->natronMarker->getOffsetKnob();
        Point offset, center, btmLeft, topRight;
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

struct TrackSchedulerPrivate
{
    TrackerParamsProvider* paramsProvider;
    NodeWPtr node;

    TrackSchedulerPrivate(TrackerParamsProvider* paramsProvider,
                          const NodeWPtr& node)
        : paramsProvider(paramsProvider)
        , node(node)
    {
    }

    NodePtr getNode() const
    {
        return node.lock();
    }

    /*
     * @brief A pointer to a function that will be called concurrently for each Track marker to track.
     * @param index Identifies the track in args, which is supposed to hold the tracks vector.
     * @param time The time at which to track. The reference frame is held in the args and can be different for each track
     */
    static bool trackStepFunctor(int trackIndex, const TrackArgs& args, int time);
};

TrackScheduler::TrackScheduler(TrackerParamsProvider* paramsProvider,
                               const NodeWPtr& node)
    : GenericSchedulerThread()
    , _imp( new TrackSchedulerPrivate(paramsProvider, node) )
{
    QObject::connect( this, SIGNAL(renderCurrentFrameForViewer(ViewerInstance*)), this, SLOT(doRenderCurrentFrameForViewer(ViewerInstance*)) );

    setThreadName("TrackScheduler");
}

TrackScheduler::~TrackScheduler()
{
}

bool
TrackSchedulerPrivate::trackStepFunctor(int trackIndex,
                                        const TrackArgs& args,
                                        int time)
{
    assert( trackIndex >= 0 && trackIndex < args.getNumTracks() );
    const std::vector<TrackMarkerAndOptionsPtr>& tracks = args.getTracks();
    const TrackMarkerAndOptionsPtr& track = tracks[trackIndex];

    if ( !track->natronMarker->isEnabled(time) ) {
        return false;
    }

    TrackMarkerPM* isTrackerPM = dynamic_cast<TrackMarkerPM*>( track->natronMarker.get() );
    bool ret;
    if (isTrackerPM) {
        ret = TrackerContextPrivate::trackStepTrackerPM(isTrackerPM, args, time);
    } else {
        ret = TrackerContextPrivate::trackStepLibMV(trackIndex, args, time);
    }

    // Disable the marker since it failed to track
    if (!ret && args.isAutoKeyingEnabledParamEnabled()) {
        track->natronMarker->setEnabledAtTime(time, false);
    }

    appPTR->getAppTLS()->cleanupTLSForThread();

    return ret;
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

class IsTrackingFlagSetter_RAII
{
    Q_DECLARE_TR_FUNCTIONS(TrackScheduler)

private:
    ViewerInstance* _v;
    EffectInstancePtr _effect;
    TrackScheduler* _base;
    bool _reportProgress;
    bool _doPartialUpdates;

public:

    IsTrackingFlagSetter_RAII(const EffectInstancePtr& effect,
                              TrackScheduler* base,
                              int step,
                              bool reportProgress,
                              ViewerInstance* viewer,
                              bool doPartialUpdates)
        : _v(viewer)
        , _effect(effect)
        , _base(base)
        , _reportProgress(reportProgress)
        , _doPartialUpdates(doPartialUpdates)
    {
        if (_effect && _reportProgress) {
            _base->emit_trackingStarted(step);
        }

        if (viewer && doPartialUpdates) {
            viewer->setDoingPartialUpdates(doPartialUpdates);
        }
    }

    ~IsTrackingFlagSetter_RAII()
    {
        if (_v && _doPartialUpdates) {
            _v->setDoingPartialUpdates(false);
        }
        if (_effect && _reportProgress) {
            _base->emit_trackingFinished();
        }
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

GenericSchedulerThread::ThreadStateEnum
TrackScheduler::threadLoopOnce(const GenericThreadStartArgsPtr& inArgs)
{
    TrackArgsPtr args = boost::dynamic_pointer_cast<TrackArgs>(inArgs);

    assert(args);

    ThreadStateEnum state = eThreadStateActive;
    TimeLinePtr timeline = args->getTimeLine();
    ViewerInstance* viewer =  args->getViewer();
    int end = args->getEnd();
    int start = args->getStart();
    int cur = start;
    int frameStep = args->getStep();
    int framesCount = 0;
    if (frameStep != 0) {
        framesCount = frameStep > 0 ? (end - start) / frameStep : (start - end) / std::abs(frameStep);
    }


    const std::vector<TrackMarkerAndOptionsPtr>& tracks = args->getTracks();
    const int numTracks = (int)tracks.size();
    std::vector<int> trackIndexes( tracks.size() );
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        trackIndexes[i] = i;
        tracks[i]->natronMarker->notifyTrackingStarted();
        // unslave the enabled knob, since it is slaved to the gui but we may modify it
        KnobBoolPtr enabledKnob = tracks[i]->natronMarker->getEnabledKnob();
        enabledKnob->unSlave(0, false);
    }

    // Beyond TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE it becomes more expensive to render all partial rectangles
    // than just render the whole viewer RoI
    const bool doPartialUpdates = numTracks < TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE;
    int lastValidFrame = frameStep > 0 ? start - 1 : start + 1;
    bool reportProgress = numTracks > 1 || framesCount > 1;
    EffectInstancePtr effect = _imp->getNode()->getEffectInstance();
    timeval lastProgressUpdateTime;
    gettimeofday(&lastProgressUpdateTime, 0);

    bool allTrackFailed = false;
    {
        ///Use RAII style for setting the isDoingPartialUpdates flag so we're sure it gets removed
        IsTrackingFlagSetter_RAII __istrackingflag__(effect, this, frameStep, reportProgress, viewer, doPartialUpdates);

        if ( (frameStep == 0) || ( (frameStep > 0) && (start >= end) ) || ( (frameStep < 0) && (start <= end) ) ) {
            // Invalid range
            cur = end;
        }


        while (cur != end) {
            ///Launch parallel thread for each track using the global thread pool
            QFuture<bool> future = QtConcurrent::mapped( trackIndexes,
                                                         boost::bind(&TrackSchedulerPrivate::trackStepFunctor,
                                                                     _1,
                                                                     *args,
                                                                     cur) );
            future.waitForFinished();

            allTrackFailed = true;
            for (QFuture<bool>::const_iterator it = future.begin(); it != future.end(); ++it) {
                if ( (*it) ) {
                    allTrackFailed = false;
                    break;
                }
            }



            lastValidFrame = cur;



            // We don't have any successful track, stop
            if (allTrackFailed) {
                break;
            }



            cur += frameStep;

            double progress;
            if (frameStep > 0) {
                progress = (double)(cur - start) / framesCount;
            } else {
                progress = (double)(start - cur) / framesCount;
            }

            bool isUpdateViewerOnTrackingEnabled = _imp->paramsProvider->getUpdateViewer();
            bool isCenterViewerEnabled = _imp->paramsProvider->getCenterOnTrack();
            bool enoughTimePassedToReportProgress;
            {
                timeval now;
                gettimeofday(&now, 0);
                double dt =  now.tv_sec  - lastProgressUpdateTime.tv_sec +
                            (now.tv_usec - lastProgressUpdateTime.tv_usec) * 1e-6f;
                dt *= 1000; // switch to MS
                enoughTimePassedToReportProgress = dt > NATRON_TRACKER_REPORT_PROGRESS_DELTA_MS;
                if (enoughTimePassedToReportProgress) {
                    lastProgressUpdateTime = now;
                }
            }


            ///Ok all tracks are finished now for this frame, refresh viewer if needed
            if (isUpdateViewerOnTrackingEnabled && viewer) {
                //This will not refresh the viewer since when tracking, renderCurrentFrame()
                //is not called on viewers, see Gui::onTimeChanged
                timeline->seekFrame(cur, true, 0, eTimelineChangeReasonOtherSeek);

                if (enoughTimePassedToReportProgress) {
                    if (doPartialUpdates) {
                        std::list<RectD> updateRects;
                        args->getRedrawAreasNeeded(cur, &updateRects);
                        viewer->setPartialUpdateParams(updateRects, isCenterViewerEnabled);
                    } else {
                        viewer->clearPartialUpdateParams();
                    }
                    Q_EMIT renderCurrentFrameForViewer(viewer);
                }
            }

            if (enoughTimePassedToReportProgress && reportProgress && effect) {
                ///Notify we progressed of 1 frame
                Q_EMIT trackingProgress(progress);
            }

            // Check for abortion
            state = resolveState();
            if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
                break;
            }
        } // while (cur != end) {
    } // IsTrackingFlagSetter_RAII
    TrackerContext* isContext = dynamic_cast<TrackerContext*>(_imp->paramsProvider);
    if (isContext) {
        isContext->solveTransformParams();
    }

    appPTR->getAppTLS()->cleanupTLSForThread();


    KnobBoolPtr contextEnabledKnob;
    if (isContext) {
        contextEnabledKnob = isContext->getEnabledKnob();
    }
    // Re-slave the knobs to the gui
    if (contextEnabledKnob) {
        for (std::size_t i = 0; i < tracks.size(); ++i) {
            // unslave the enabled knob, since it is slaved to the gui but we may modify it
            KnobBoolPtr enabledKnob = tracks[i]->natronMarker->getEnabledKnob();

            tracks[i]->natronMarker->notifyTrackingEnded();
            contextEnabledKnob->blockListenersNotification();
            contextEnabledKnob->cloneAndUpdateGui( enabledKnob.get() );
            contextEnabledKnob->unblockListenersNotification();
            enabledKnob->slaveTo(0, contextEnabledKnob, 0);
        }
        contextEnabledKnob->setDirty(tracks.size() > 1);
    }


    //Now that tracking is done update viewer once to refresh the whole visible portion

    if ( _imp->paramsProvider && _imp->paramsProvider->getUpdateViewer() ) {
        //Refresh all viewers to the current frame
        timeline->seekFrame(lastValidFrame, true, 0, eTimelineChangeReasonOtherSeek);
    }

    return state;
} // >::threadLoopOnce

void
TrackScheduler::doRenderCurrentFrameForViewer(ViewerInstance* viewer)
{
    assert( QThread::currentThread() == qApp->thread() );
    viewer->renderCurrentFrame(true);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TrackerContext.cpp"
