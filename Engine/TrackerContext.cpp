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
    
    ProsacReturnCodeEnum ret;
    
    const std::size_t minSamples = (std::size_t)openMVG::robust::Similarity2DSolver::MinimumSamples();
    if (x1.size() > minSamples) {
        KernelType kernel(M1, w1, h1, M2, w2, h2);
        ret = prosac(kernel, foundModel);
    } else if (x1.size() == minSamples) {
        std::vector<typename MODELTYPE::Model> models;
        MODELTYPE::Solve(M1, M2, &models);
        if (!models.empty()) {
            *foundModel = models[0];
            ret = eProsacReturnCodeFoundModel;
        } else {
            ret = eProsacReturnCodeNoModelFound;
        }
    } else {
        ret = eProsacReturnCodeNotEnoughPoints;
    }
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
    *rotate = Transform::toDegrees(*rotate);

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
    
    if (refTime == time) {
        data->hasRotationAndScale = x1.size() > 1;
        data->translation.x = data->translation.y = data->rotation = 0;
        data->scale = 1.;
        return;
    }
    
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
    boost::shared_ptr<KnobDouble> centerKnob = _imp->center.lock();
    boost::shared_ptr<KnobDouble> scaleKnob = _imp->scale.lock();
    boost::shared_ptr<KnobDouble> rotationKnob = _imp->rotate.lock();
    
    translationKnob->resetToDefaultValue(0);
    translationKnob->resetToDefaultValue(1);
    
    centerKnob->resetToDefaultValue(0);
    centerKnob->resetToDefaultValue(1);
    
    scaleKnob->resetToDefaultValue(0);
    rotationKnob->resetToDefaultValue(0);
    
    // Set the center at the reference frame
    Point centerValue = {0,0};
    for (std::size_t i = 0; i < markers.size(); ++i) {
        
        boost::shared_ptr<KnobDouble> markerCenterKnob = markers[i]->getCenterKnob();
        
        centerValue.x += markerCenterKnob->getValueAtTime(refTime, 0);
        centerValue.y += markerCenterKnob->getValueAtTime(refTime, 1);
    }
    centerValue.x /= markers.size();
    centerValue.y /= markers.size();
    
    centerKnob->setValues(centerValue.x, centerValue.y, ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
    
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
            int nSamples = dataAtTime[i].data.hasRotationAndScale ? 1 : 0;
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
            if (nSamples) {
                avg /= nSamples;
            }
            
            rotationKnob->setValueAtTime(dataAtTime[i].time, avg, ViewSpec::all(), 0);
        } else {
            if (dataAtTime[i].data.hasRotationAndScale) {
                rotationKnob->setValueAtTime(dataAtTime[i].time, dataAtTime[i].data.rotation, ViewSpec::all(), 0);
            }
        }
        
        if (smoothSJitter > 1) {
            int halfJitter = smoothSJitter / 2;
            double avg = dataAtTime[i].data.scale;
            int nSamples = dataAtTime[i].data.hasRotationAndScale ? 1 : 0;
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
            if (nSamples) {
                avg /= nSamples;
                scaleKnob->setValueAtTime(dataAtTime[i].time, avg, ViewSpec::all(), 0);
            }
            
        } else {
            if (dataAtTime[i].data.hasRotationAndScale) {
                scaleKnob->setValueAtTime(dataAtTime[i].time, dataAtTime[i].data.scale, ViewSpec::all(), 0);
            }
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
