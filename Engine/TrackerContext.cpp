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

NATRON_NAMESPACE_ENTER;

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

void
TrackArgsV1::getRedrawAreasNeeded(int time,
                                  std::list<RectD>* canonicalRects) const
{
    for (std::vector<KnobButton*>::const_iterator it = _buttonInstances.begin(); it != _buttonInstances.end(); ++it) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>( (*it)->getHolder() );
        assert(effect);

        boost::shared_ptr<KnobDouble> searchBtmLeft = boost::dynamic_pointer_cast<KnobDouble>( effect->getKnobByName("searchBoxBtmLeft") );
        boost::shared_ptr<KnobDouble> searchTopRight = boost::dynamic_pointer_cast<KnobDouble>( effect->getKnobByName("searchBoxTopRight") );
        boost::shared_ptr<KnobDouble> centerKnob = boost::dynamic_pointer_cast<KnobDouble>( effect->getKnobByName("center") );
        boost::shared_ptr<KnobDouble> offsetKnob = boost::dynamic_pointer_cast<KnobDouble>( effect->getKnobByName("offset") );
        assert(searchBtmLeft  && searchTopRight && centerKnob && offsetKnob);
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

bool
TrackerContext::trackStepV1(int trackIndex,
                            const TrackArgsV1& args,
                            int time)
{
    assert( trackIndex >= 0 && trackIndex < (int)args.getInstances().size() );
    KnobButton* selectedInstance = args.getInstances()[trackIndex];
    selectedInstance->getHolder()->onKnobValueChanged_public(selectedInstance, eValueChangedReasonNatronInternalEdited, time, ViewIdx(0),
                                                             true);
    appPTR->getAppTLS()->cleanupTLSForThread();

    return true;
}

TrackerContext::TrackerContext(const boost::shared_ptr<Node> &node)
    : boost::enable_shared_from_this<TrackerContext>()
    , _imp( new TrackerContextPrivate(this, node) )
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
        TrackMarkerPtr marker( new TrackMarker(thisShared) );
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
        getNode()->getApp()->setLastViewerUsingTimeline( boost::shared_ptr<Node>() );
        getNode()->getApp()->getTimeLine()->seekFrame(minimum, false,  NULL, eTimelineChangeReasonPlaybackSeek);
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
        getNode()->getApp()->setLastViewerUsingTimeline( boost::shared_ptr<Node>() );
        getNode()->getApp()->getTimeLine()->seekFrame(maximum, false,  NULL, eTimelineChangeReasonPlaybackSeek);
    }
}

TrackMarkerPtr
TrackerContext::getMarkerByName(const std::string & name) const
{
    QMutexLocker k(&_imp->trackerContextMutex);

    for (std::vector<TrackMarkerPtr >::const_iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
        if ( (*it)->getScriptName_mt_safe() == name ) {
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
    TrackMarkerPtr track( new TrackMarker( shared_from_this() ) );
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
            std::vector<TrackMarkerPtr >::iterator it = _imp->markers.begin();
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

boost::shared_ptr<Node>
TrackerContext::getNode() const
{
    return _imp->node.lock();
}

int
TrackerContext::getTimeLineFirstFrame() const
{
    boost::shared_ptr<Node> node = getNode();

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
    boost::shared_ptr<Node> node = getNode();

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
                                     ViewerInstance* viewer)
{
    std::list<TrackMarkerPtr > markers;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        for (std::list<TrackMarkerPtr >::iterator it = _imp->selectedMarkers.begin();
             it != _imp->selectedMarkers.end(); ++it) {
            if ( (*it)->isEnabled( (*it)->getCurrentTime() ) ) {
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
TrackerContext::addTrackToSelection(const TrackMarkerPtr& mark,
                                    TrackSelectionReason reason)
{
    std::list<TrackMarkerPtr > marks;

    marks.push_back(mark);
    addTracksToSelection(marks, reason);
}

void
TrackerContext::addTracksToSelection(const std::list<TrackMarkerPtr >& marks,
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

        for (std::list<TrackMarkerPtr >::const_iterator it = marks.begin(); it != marks.end(); ++it) {
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
    std::list<TrackMarkerPtr > marks;

    marks.push_back(mark);
    removeTracksFromSelection(marks, reason);
}

void
TrackerContext::removeTracksFromSelection(const std::list<TrackMarkerPtr >& marks,
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

        for (std::list<TrackMarkerPtr >::const_iterator it = marks.begin(); it != marks.end(); ++it) {
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
    if ( markers.empty() ) {
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
    int time = getNode()->getApp()->getTimeLine()->currentFrame();
    for (std::vector<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        if ((*it)->isEnabled(time)) {
            addTrackToSelection(*it, reason);
        }
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
        if ( _imp->markers[i]->isEnabled( _imp->markers[i]->getCurrentTime() ) ) {
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

    for (std::list<TrackMarkerPtr >::const_iterator it = _imp->selectedMarkers.begin(); it != _imp->selectedMarkers.end(); ++it) {
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


        for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->perTrackKnobs.begin(); it != _imp->perTrackKnobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
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

#endif

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
TrackerContext::exportTrackDataFromExportOptions()
{
    //bool transformLink = _imp->exportLink.lock()->getValue();
    boost::shared_ptr<KnobChoice> transformTypeKnob = _imp->transformType.lock();

    assert(transformTypeKnob);
    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    boost::shared_ptr<KnobChoice> motionTypeKnob = _imp->motionType.lock();
    if (!motionTypeKnob) {
        return;
    }
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum mt = (TrackerMotionTypeEnum)motionType_i;

    if (mt == eTrackerMotionTypeNone) {
        Dialogs::errorDialog( QObject::tr("Tracker Export").toStdString(), QObject::tr("Please select the export mode with the Motion Type parameter").toStdString() );

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
    AppInstance* app = thisNode->getApp();
    CreateNodeArgs args( pluginID, eCreateNodeReasonInternal, thisNode->getGroup() );
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
        boost::shared_ptr<KnobDouble> cornerPinToPoints[4];
        boost::shared_ptr<KnobDouble> cornerPinFromPoints[4];


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
            KnobPtr knob = createdNode->getKnobByName(kCornerPinParamMatrix);
            if (knob) {
                KnobDouble* isType = dynamic_cast<KnobDouble*>(knob.get());
                if (isType) {
                    isType->cloneAndUpdateGui(_imp->cornerPinMatrix.lock().get());
                }
            }
        }
    }   break;
    case eTrackerTransformNodeTransform: {
        KnobPtr translateKnob = createdNode->getKnobByName(kTransformParamTranslate);
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

        KnobPtr scaleKnob = createdNode->getKnobByName(kTransformParamScale);
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

        KnobPtr rotateKnob = createdNode->getKnobByName(kTransformParamRotate);
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
        KnobPtr centerKnob = createdNode->getKnobByName(kTransformParamCenter);
        if (centerKnob) {
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>(centerKnob.get());
            if (isDbl) {
                isDbl->cloneAndUpdateGui(_imp->center.lock().get());
               
            }
        }

    }   break;
    } // switch

    KnobPtr cpInvertKnob = createdNode->getKnobByName(kTransformParamInvert);
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
        KnobPtr knob = createdNode->getKnobByName(kTransformParamMotionBlur);
        if (knob) {
            KnobDouble* isType = dynamic_cast<KnobDouble*>(knob.get());
            if (isType) {
                isType->cloneAndUpdateGui(_imp->motionBlur.lock().get());
            }
        }
    }
    {
        KnobPtr knob = createdNode->getKnobByName(kTransformParamShutter);
        if (knob) {
            KnobDouble* isType = dynamic_cast<KnobDouble*>(knob.get());
            if (isType) {
                isType->cloneAndUpdateGui(_imp->shutter.lock().get());
            }
        }
    }
    {
        KnobPtr knob = createdNode->getKnobByName(kTransformParamShutterOffset);
        if (knob) {
            KnobChoice* isType = dynamic_cast<KnobChoice*>(knob.get());
            if (isType) {
                isType->cloneAndUpdateGui(_imp->shutterOffset.lock().get());
            }
        }
    }
    {
        KnobPtr knob = createdNode->getKnobByName(kTransformParamCustomShutterOffset);
        if (knob) {
            KnobDouble* isType = dynamic_cast<KnobDouble*>(knob.get());
            if (isType) {
                isType->cloneAndUpdateGui(_imp->customShutterOffset.lock().get());
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

void
TrackerContext::knobChanged(KnobI* k,
                            ValueChangedReasonEnum /*reason*/,
                            ViewSpec /*view*/,
                            double /*time*/,
                            bool /*originatedFromMainThread*/)
{
    if ( k == _imp->exportButton.lock().get() ) {
        exportTrackDataFromExportOptions();
    } else if ( k == _imp->setCurrentFrameButton.lock().get() ) {
        boost::shared_ptr<KnobInt> refFrame = _imp->referenceFrame.lock();
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
    } else if ( k == _imp->autoGenerateTransform.lock().get() ) {
        solveTransformParams();
       _imp->refreshVisibilityFromTransformType();
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
    std::string script = (nodeFullName + ".tracker." + item->getScriptName_mt_safe() + " = " +
                          nodeFullName + ".tracker.getTrackByName(\"" + item->getScriptName_mt_safe() + "\")\n");

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
    std::vector<TrackMarkerPtr > markers;

    getAllMarkers(&markers);
    for (std::vector< TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
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
TrackerContext::solveTransformParamsIfAutomatic()
{
    if (_imp->isTransformAutoGenerationEnabled()) {
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
    
    boost::shared_ptr<KnobChoice> motionTypeKnob = _imp->motionType.lock();
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
    boost::shared_ptr<KnobChoice> transformTypeKnob = _imp->transformType.lock();
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

    boost::shared_ptr<KnobDouble> centerKnob = _imp->center.lock();

    // Set the center at the reference frame
    Point centerValue = {0, 0};
    int nSamplesAtRefTime = 0;
    for (std::size_t i = 0; i < markers.size(); ++i) {
        if ( !markers[i]->isEnabled(refTime) ) {
            continue;
        }
        boost::shared_ptr<KnobDouble> markerCenterKnob = markers[i]->getCenterKnob();

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
    
    node->getApp()->progressStart(node, QObject::tr("Solving transform parameters...").toStdString(), std::string());

    _imp->lastSolveRequest.refTime = refTime;
    _imp->lastSolveRequest.jitterPeriod = jitterPeriod;
    _imp->lastSolveRequest.jitterAdd = jitterAdd;
    _imp->lastSolveRequest.allMarkers = markers;
    _imp->lastSolveRequest.keyframes = keyframes;
    _imp->lastSolveRequest.robustModel = robustModel;

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
    boost::shared_ptr<KnobChoice> transformTypeKnob = _imp->transformType.lock();

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
                                              OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayPenDown_public(time, renderScale, view, viewportPos, pos, pressure) ) {
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
                                                OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayPenMotion_public(time, renderScale, view, viewportPos, pos, pressure) ) {
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
                                            OverlaySupport* viewer)
{
    if ( _imp->transformPageKnob.lock()->getIsSecret() ) {
        return false;
    }
    NodePtr node = getCurrentlySelectedTransformNode();
    if (node) {
        NodePtr thisNode = getNode();
        thisNode->getEffectInstance()->setCurrentViewportForOverlays_public(viewer);
        if ( thisNode->getEffectInstance()->onOverlayPenUp_public(time, renderScale, view, viewportPos, pos, pressure) ) {
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


    TrackSchedulerPrivate(TrackerParamsProvider* paramsProvider,
                          const NodeWPtr& node)
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
    QObject::connect( this, SIGNAL(renderCurrentFrameForViewer(ViewerInstance*)), this, SLOT(doRenderCurrentFrameForViewer(ViewerInstance*)) );
}

template <class TrackArgsType>
TrackScheduler<TrackArgsType>::TrackScheduler(TrackerParamsProvider* paramsProvider,
                                              const NodeWPtr& node,
                                              TrackStepFunctor functor)
    : TrackSchedulerBase()
    , _imp( new TrackSchedulerPrivate(paramsProvider, node) )
    , argsMutex()
    , curArgs()
    , requestedArgs()
    , _functor(functor)
{
    setObjectName( QString::fromUtf8("TrackScheduler") );
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
    bool _doPartialUpdates;

public:

    IsTrackingFlagSetter_RAII(const EffectInstPtr& effect,
                              TrackSchedulerBase* base,
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
            _effect->getApp()->progressStart(_effect->getNode(), QObject::tr("Tracking...").toStdString(), "");
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
            _effect->getApp()->progressEnd( _effect->getNode() );
            _base->emit_trackingFinished();
        }
    }
};

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::run()
{
    for (;; ) {
        ///Check for exit of the thread
        if ( _imp->checkForExit() ) {
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
        int frameStep = curArgs.getStep();
        int framesCount = 0;
        if (frameStep != 0) {
            framesCount = frameStep > 0 ? (end - start) / frameStep : (start - end) / std::abs(frameStep);
        }
        const int numTracks = curArgs.getNumTracks();
        std::vector<int> trackIndexes;
        for (std::size_t i = 0; i < (std::size_t)numTracks; ++i) {
            trackIndexes.push_back(i);
        }

        // Beyond TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE it becomes more expensive to render all partial rectangles
        // than just render the whole viewer RoI
        const bool doPartialUpdates = numTracks < TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE;
        int lastValidFrame = frameStep > 0 ? start - 1 : start + 1;
        bool reportProgress = numTracks > 1 || framesCount > 1;
        EffectInstPtr effect = _imp->getNode()->getEffectInstance();
        {
            ///Use RAII style for setting the isDoingPartialUpdates flag so we're sure it gets removed
            IsTrackingFlagSetter_RAII __istrackingflag__(effect, this, frameStep, reportProgress, viewer, doPartialUpdates);


            if ( (frameStep == 0) || ( (frameStep > 0) && (start >= end) ) || ( (frameStep < 0) && (start <= end) ) ) {
                // Invalid range
                cur = end;
            }

            bool allTrackFailed = false;
            
            while (cur != end) {
                ///Launch parallel thread for each track using the global thread pool
                QFuture<bool> future = QtConcurrent::mapped( trackIndexes,
                                                             boost::bind(_functor,
                                                                         _1,
                                                                         curArgs,
                                                                         cur) );
                future.waitForFinished();

                allTrackFailed = true;
                for (QFuture<bool>::const_iterator it = future.begin(); it != future.end(); ++it) {
                    if ((*it)) {
                        allTrackFailed = false;
                        break;
                    }
                }
                
                // We don't have any successful track, stop
                if (allTrackFailed) {
                    break;
                }

                lastValidFrame = cur;

                cur += frameStep;

                double progress;
                if (frameStep > 0) {
                    progress = (double)(cur - start) / framesCount;
                } else {
                    progress = (double)(start - cur) / framesCount;
                }

                bool isUpdateViewerOnTrackingEnabled = _imp->paramsProvider->getUpdateViewer();
                bool isCenterViewerEnabled = _imp->paramsProvider->getCenterOnTrack();


                ///Ok all tracks are finished now for this frame, refresh viewer if needed
                if (isUpdateViewerOnTrackingEnabled && viewer) {
                    //This will not refresh the viewer since when tracking, renderCurrentFrame()
                    //is not called on viewers, see Gui::onTimeChanged
                    timeline->seekFrame(cur, true, 0, eTimelineChangeReasonOtherSeek);


                    if (doPartialUpdates) {
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
                    if ( !effect->getApp()->progressUpdate(effect->getNode(), progress) ) {
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
        if (!isContext) {
            isContext->solveTransformParams();
        }

        appPTR->getAppTLS()->cleanupTLSForThread();

        //Now that tracking is done update viewer once to refresh the whole visible portion

        if ( _imp->paramsProvider->getUpdateViewer() ) {
            //Refresh all viewers to the current frame
            timeline->seekFrame(lastValidFrame, true, 0, eTimelineChangeReasonOtherSeek);
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
} // >::run

void
TrackSchedulerBase::doRenderCurrentFrameForViewer(ViewerInstance* viewer)
{
    assert( QThread::currentThread() == qApp->thread() );
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
    if ( isRunning() ) {
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
    if ( !isRunning() || !isWorking() ) {
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
    if ( !isRunning() ) {
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
