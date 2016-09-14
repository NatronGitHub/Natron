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

#include "RotoContext.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cassert>
#include <stdexcept>
#include <cstring> // for std::memcpy, std::memset

#include <ofxNatron.h>

#include <boost/scoped_ptr.hpp>

#include <QtCore/QLineF>
#include <QtCore/QDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#define ROTO_CAIRO_RENDER_TRIANGLES_ONLY



#include "Global/MemoryInfo.h"
#include "Engine/RotoContextPrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/OSGLContext.h"
#include "Engine/ImageParams.h"
#include "Engine/ImageLocker.h"
#include "Engine/Interpolation.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewIdx.h"

#include "Serialization/RotoLayerSerialization.h"
#include "Serialization/RotoContextSerialization.h"
#include "Serialization/NodeSerialization.h"


#define kMergeOFXParamOperation "operation"
#define kMergeOFXParamInvertMask "maskInvert"

#define kBlurCImgParamSize "size"
#define kTimeOffsetParamOffset "timeOffset"
#define kFrameHoldParamFirstFrame "firstFrame"

#define kTransformParamTranslate "translate"
#define kTransformParamRotate "rotate"
#define kTransformParamScale "scale"
#define kTransformParamUniform "uniform"
#define kTransformParamSkewX "skewX"
#define kTransformParamSkewY "skewY"
#define kTransformParamSkewOrder "skewOrder"
#define kTransformParamCenter "center"
#define kTransformParamFilter "filter"
#define kTransformParamResetCenter "resetCenter"
#define kTransformParamBlackOutside "black_outside"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER;

////////////////////////////////////RotoContext////////////////////////////////////


RotoContext::RotoContext(const NodePtr& node)
    : _imp( new RotoContextPrivate(node) )
{
    QObject::connect( _imp->lifeTime.lock()->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onLifeTimeKnobValueChanged(ViewSpec,int,int)) );
}

bool
RotoContext::isRotoPaint() const
{
    return _imp->isPaintNode;
}

void
RotoContext::setWhileCreatingPaintStrokeOnMergeNodes(bool b)
{
    getNode()->setWhileCreatingPaintStroke(b);
    QMutexLocker k(&_imp->rotoContextMutex);
    for (NodesList::iterator it = _imp->globalMergeNodes.begin(); it != _imp->globalMergeNodes.end(); ++it) {
        (*it)->setWhileCreatingPaintStroke(b);
    }
}


void
RotoContext::getRotoPaintTreeNodes(NodesList* nodes) const
{
    std::list<RotoDrawableItemPtr > items = getCurvesByRenderOrder(false);

    for (std::list<RotoDrawableItemPtr >::iterator it = items.begin(); it != items.end(); ++it) {
        NodePtr effectNode = (*it)->getEffectNode();
        NodePtr mergeNode = (*it)->getMergeNode();
        NodePtr timeOffsetNode = (*it)->getTimeOffsetNode();
        NodePtr frameHoldNode = (*it)->getFrameHoldNode();
        NodePtr maskNode = (*it)->getMaskNode();
        if (effectNode) {
            nodes->push_back(effectNode);
        }
        if (maskNode) {
            nodes->push_back(maskNode);
        }
        if (mergeNode) {
            nodes->push_back(mergeNode);
        }
        if (timeOffsetNode) {
            nodes->push_back(timeOffsetNode);
        }
        if (frameHoldNode) {
            nodes->push_back(frameHoldNode);
        }
    }

    QMutexLocker k(&_imp->rotoContextMutex);
    for (NodesList::const_iterator it = _imp->globalMergeNodes.begin(); it != _imp->globalMergeNodes.end(); ++it) {
        nodes->push_back(*it);
    }
}

///Must be done here because at the time of the constructor, the shared_ptr doesn't exist yet but
///addLayer() needs it to get a shared ptr to this
void
RotoContext::createBaseLayer()
{
    ////Add the base layer
    RotoLayerPtr base = addLayerInternal(false);

    deselect(base, RotoItem::eSelectionReasonOther);
}

RotoContext::~RotoContext()
{
}

RotoLayerPtr
RotoContext::getOrCreateBaseLayer()
{
    QMutexLocker k(&_imp->rotoContextMutex);

    if ( _imp->layers.empty() ) {
        k.unlock();
        addLayer();
        k.relock();
    }
    assert( !_imp->layers.empty() );

    return _imp->layers.front();
}

RotoLayerPtr
RotoContext::addLayerInternal(bool declarePython)
{
    RotoContextPtr this_shared = shared_from_this();

    assert(this_shared);

    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    RotoLayerPtr item;
    std::string name = generateUniqueName(kRotoLayerBaseName);
    int indexInLayer = -1;
    {
        RotoLayerPtr deepestLayer;
        RotoLayerPtr parentLayer;
        {
            QMutexLocker l(&_imp->rotoContextMutex);
            deepestLayer = _imp->findDeepestSelectedLayer();

            if (!deepestLayer) {
                ///find out if there's a base layer, if so add to the base layer,
                ///otherwise create the base layer
                for (std::list<RotoLayerPtr >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
                    int hierarchy = (*it)->getHierarchyLevel();
                    if (hierarchy == 0) {
                        parentLayer = *it;
                        break;
                    }
                }
            } else {
                parentLayer = deepestLayer;
            }
        }

        item.reset( new RotoLayer( this_shared, name, RotoLayerPtr() ) );
        item->initializeKnobsPublic();
        if (parentLayer) {
            parentLayer->addItem(item, declarePython);
            indexInLayer = parentLayer->getItems().size() - 1;
        }

        QMutexLocker l(&_imp->rotoContextMutex);

        _imp->layers.push_back(item);

        _imp->lastInsertedItem = item;
    }
    Q_EMIT itemInserted(indexInLayer, RotoItem::eSelectionReasonOther);


    clearSelection(RotoItem::eSelectionReasonOther);
    select(item, RotoItem::eSelectionReasonOther);

    return item;
} // RotoContext::addLayerInternal

RotoLayerPtr
RotoContext::addLayer()
{
    return addLayerInternal(true);
} // addLayer

void
RotoContext::addLayer(const RotoLayerPtr & layer)
{
    std::list<RotoLayerPtr >::iterator it = std::find(_imp->layers.begin(), _imp->layers.end(), layer);

    if ( it == _imp->layers.end() ) {
        _imp->layers.push_back(layer);
    }
}

RotoItemPtr
RotoContext::getLastInsertedItem() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->lastInsertedItem;
}

#ifdef NATRON_ROTO_INVERTIBLE
KnobBoolPtr
RotoContext::getInvertedKnob() const
{
    return _imp->inverted.lock();
}

#endif

KnobColorPtr
RotoContext::getColorKnob() const
{
    return _imp->colorKnob.lock();
}

void
RotoContext::setAutoKeyingEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->autoKeying = enabled;
}

bool
RotoContext::isAutoKeyingEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->autoKeying;
}

void
RotoContext::setFeatherLinkEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->featherLink = enabled;
}

bool
RotoContext::isFeatherLinkEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->featherLink;
}

void
RotoContext::setRippleEditEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->rippleEdit = enabled;
}

bool
RotoContext::isRippleEditEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->rippleEdit;
}

NodePtr
RotoContext::getNode() const
{
    return _imp->node.lock();
}

int
RotoContext::getTimelineCurrentTime() const
{
    return getNode()->getApp()->getTimeLine()->currentFrame();
}

std::string
RotoContext::generateUniqueName(const std::string& baseName)
{
    int no = 1;
    bool foundItem;
    std::string name;

    do {
        std::stringstream ss;
        ss << baseName;
        ss << no;
        name = ss.str();
        if ( getItemByName(name) ) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);

    return name;
}

BezierPtr
RotoContext::makeBezier(double x,
                        double y,
                        const std::string & baseName,
                        double time,
                        bool isOpenBezier)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    RotoLayerPtr parentLayer;
    RotoContextPtr this_shared = boost::dynamic_pointer_cast<RotoContext>( shared_from_this() );
    assert(this_shared);
    std::string name = generateUniqueName(baseName);

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        RotoLayerPtr deepestLayer = _imp->findDeepestSelectedLayer();


        if (!deepestLayer) {
            ///if there is no base layer, create one
            if ( _imp->layers.empty() ) {
                l.unlock();
                addLayer();
                l.relock();
            }
            parentLayer = _imp->layers.front();
        } else {
            parentLayer = deepestLayer;
        }
    }
    assert(parentLayer);
    BezierPtr curve( new Bezier(this_shared, name, RotoLayerPtr(), isOpenBezier) );
    curve->createNodes();

    int indexInLayer = -1;
    if (parentLayer) {
        indexInLayer = 0;
        parentLayer->insertItem(curve, 0);
    }
    _imp->lastInsertedItem = curve;

    Q_EMIT itemInserted(indexInLayer, RotoItem::eSelectionReasonOther);


    clearSelection(RotoItem::eSelectionReasonOther);
    select(curve, RotoItem::eSelectionReasonOther);

    if ( isAutoKeyingEnabled() ) {
        curve->setKeyframe( getTimelineCurrentTime() );
    }
    curve->addControlPoint(x, y, time);

    return curve;
} // makeBezier

RotoStrokeItemPtr
RotoContext::makeStroke(RotoStrokeType type,
                        const std::string& baseName,
                        bool clearSel)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    RotoLayerPtr parentLayer;
    RotoContextPtr this_shared = boost::dynamic_pointer_cast<RotoContext>( shared_from_this() );
    assert(this_shared);
    std::string name = generateUniqueName(baseName);

    {
        QMutexLocker l(&_imp->rotoContextMutex);

        RotoLayerPtr deepestLayer = _imp->findDeepestSelectedLayer();


        if (!deepestLayer) {
            ///if there is no base layer, create one
            if ( _imp->layers.empty() ) {
                l.unlock();
                addLayer();
                l.relock();
            }
            parentLayer = _imp->layers.front();
        } else {
            parentLayer = deepestLayer;
        }
    }
    assert(parentLayer);
    RotoStrokeItemPtr curve( new RotoStrokeItem( type, this_shared, name, RotoLayerPtr() ) );
    int indexInLayer = -1;
    if (parentLayer) {
        indexInLayer = 0;
        parentLayer->insertItem(curve, 0);
    }
    curve->createNodes();

    _imp->lastInsertedItem = curve;

    Q_EMIT itemInserted(indexInLayer, RotoItem::eSelectionReasonOther);

    if (clearSel) {
        clearSelection(RotoItem::eSelectionReasonOther);
        select(curve, RotoItem::eSelectionReasonOther);
    }

    return curve;
}

BezierPtr
RotoContext::makeEllipse(double x,
                         double y,
                         double diameter,
                         bool fromCenter,
                         double time)
{
    double half = diameter / 2.;
    BezierPtr curve = makeBezier(x, fromCenter ? y - half : y, kRotoEllipseBaseName, time, false);

    if (fromCenter) {
        curve->addControlPoint(x + half, y, time);
        curve->addControlPoint(x, y + half, time);
        curve->addControlPoint(x - half, y, time);
    } else {
        curve->addControlPoint(x + diameter, y - diameter, time);
        curve->addControlPoint(x, y - diameter, time);
        curve->addControlPoint(x - diameter, y - diameter, time);
    }

    BezierCPPtr top = curve->getControlPointAtIndex(0);
    BezierCPPtr right = curve->getControlPointAtIndex(1);
    BezierCPPtr bottom = curve->getControlPointAtIndex(2);
    BezierCPPtr left = curve->getControlPointAtIndex(3);
    double topX, topY, rightX, rightY, btmX, btmY, leftX, leftY;
    top->getPositionAtTime(false, time, ViewIdx(0), &topX, &topY);
    right->getPositionAtTime(false, time, ViewIdx(0), &rightX, &rightY);
    bottom->getPositionAtTime(false, time, ViewIdx(0), &btmX, &btmY);
    left->getPositionAtTime(false, time, ViewIdx(0), &leftX, &leftY);

    curve->setLeftBezierPoint(0, time,  (leftX + topX) / 2., topY);
    curve->setRightBezierPoint(0, time, (rightX + topX) / 2., topY);

    curve->setLeftBezierPoint(1, time,  rightX, (rightY + topY) / 2.);
    curve->setRightBezierPoint(1, time, rightX, (rightY + btmY) / 2.);

    curve->setLeftBezierPoint(2, time,  (rightX + btmX) / 2., btmY);
    curve->setRightBezierPoint(2, time, (leftX + btmX) / 2., btmY);

    curve->setLeftBezierPoint(3, time,   leftX, (btmY + leftY) / 2.);
    curve->setRightBezierPoint(3, time, leftX, (topY + leftY) / 2.);
    curve->setCurveFinished(true);

    return curve;
}

BezierPtr
RotoContext::makeSquare(double x,
                        double y,
                        double initialSize,
                        double time)
{
    BezierPtr curve = makeBezier(x, y, kRotoRectangleBaseName, time, false);

    curve->addControlPoint(x + initialSize, y, time);
    curve->addControlPoint(x + initialSize, y - initialSize, time);
    curve->addControlPoint(x, y - initialSize, time);
    curve->setCurveFinished(true);

    return curve;
}

void
RotoContext::removeItemRecursively(const RotoItemPtr& item,
                                   RotoItem::SelectionReasonEnum reason)
{
    RotoLayerPtr isLayer = toRotoLayer(item);
    RotoItemPtr foundSelected;

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list< RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            if (*it == item) {
                foundSelected = *it;
                break;
            }
        }
    }

    if (foundSelected) {
        deselectInternal(foundSelected);
    }

    if (isLayer) {
        const RotoItems & items = isLayer->getItems();
        for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
            removeItemRecursively(*it, reason);
        }
        QMutexLocker l(&_imp->rotoContextMutex);

        for (std::list<RotoLayerPtr >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
            if (*it == isLayer) {
                _imp->layers.erase(it);
                break;
            }
        }
    }
    Q_EMIT itemRemoved(item, (int)reason);
}

void
RotoContext::removeItem(const RotoItemPtr& item,
                        RotoItem::SelectionReasonEnum reason)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    RotoLayerPtr layer = item->getParentLayer();
    if (layer) {
        layer->removeItem(item);
    }

    removeItemRecursively(item, reason);

    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::addItem(const RotoLayerPtr& layer,
                     int indexInLayer,
                     const RotoItemPtr & item,
                     RotoItem::SelectionReasonEnum reason)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        if (layer) {
            layer->insertItem(item, indexInLayer);
        }

        QMutexLocker l(&_imp->rotoContextMutex);
        RotoLayerPtr isLayer = toRotoLayer(item);
        if (isLayer) {
            std::list<RotoLayerPtr >::iterator foundLayer = std::find(_imp->layers.begin(), _imp->layers.end(), isLayer);
            if ( foundLayer == _imp->layers.end() ) {
                _imp->layers.push_back(isLayer);
            }
        }
        _imp->lastInsertedItem = item;
    }
    Q_EMIT itemInserted(indexInLayer, reason);
}

const std::list< RotoLayerPtr > &
RotoContext::getLayers() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->layers;
}

BezierPtr
RotoContext::isNearbyBezier(double x,
                            double y,
                            double acceptance,
                            int* index,
                            double* t,
                            bool* feather) const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);
    std::list<std::pair<BezierPtr, std::pair<int, double> > > nearbyBeziers;
    for (std::list< RotoLayerPtr >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        const RotoItems & items = (*it)->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            BezierPtr b = toBezier(*it2);
            if ( b && !b->isLockedRecursive() ) {
                double param;
                int i = b->isPointOnCurve(x, y, acceptance, &param, feather);
                if (i != -1) {
                    nearbyBeziers.push_back( std::make_pair( b, std::make_pair(i, param) ) );
                }
            }
        }
    }

    std::list<std::pair<BezierPtr, std::pair<int, double> > >::iterator firstNotSelected = nearbyBeziers.end();
    for (std::list<std::pair<BezierPtr, std::pair<int, double> > >::iterator it = nearbyBeziers.begin(); it != nearbyBeziers.end(); ++it) {
        bool foundSelected = false;
        for (std::list<RotoItemPtr >::iterator it2 = _imp->selectedItems.begin(); it2 != _imp->selectedItems.end(); ++it2) {
            if ( it2->get() == it->first.get() ) {
                foundSelected = true;
                break;
            }
        }
        if (foundSelected) {
            *index = it->second.first;
            *t = it->second.second;

            return it->first;
        } else if ( firstNotSelected == nearbyBeziers.end() ) {
            firstNotSelected = it;
        }
    }
    if ( firstNotSelected != nearbyBeziers.end() ) {
        *index = firstNotSelected->second.first;
        *t = firstNotSelected->second.second;

        return firstNotSelected->first;
    }

    return BezierPtr();
}

void
RotoContext::onLifeTimeKnobValueChanged(ViewSpec view,
                                        int /*dim*/,
                                        int reason)
{
    if ( (ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited ) {
        return;
    }
    int lifetime_i = _imp->lifeTime.lock()->getValue();
    _imp->activated.lock()->setSecret(lifetime_i != 3);
    KnobIntPtr frame = _imp->lifeTimeFrame.lock();
    frame->setSecret(lifetime_i == 3);
    if (lifetime_i != 3) {
        frame->setValue(getTimelineCurrentTime(), view);
    }
}

void
RotoContext::onAutoKeyingChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->autoKeying = enabled;
}

void
RotoContext::onFeatherLinkChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->featherLink = enabled;
}

void
RotoContext::onRippleEditChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->rippleEdit = enabled;
}

void
RotoContext::getGlobalMotionBlurSettings(const double time,
                                         double* startTime,
                                         double* endTime,
                                         double* timeStep) const
{
    *startTime = time, *timeStep = 1., *endTime = time;
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    double motionBlurAmnt = _imp->globalMotionBlurKnob.lock()->getValueAtTime(time);
    if (motionBlurAmnt == 0) {
        return;
    }
    int nbSamples = std::floor(motionBlurAmnt * 10 + 0.5);
    double shutterInterval = _imp->globalMotionBlurKnob.lock()->getValueAtTime(time);
    if (shutterInterval == 0) {
        return;
    }
    int shutterType_i = _imp->globalMotionBlurKnob.lock()->getValueAtTime(time);
    if (nbSamples != 0) {
        *timeStep = shutterInterval / nbSamples;
    }
    if (shutterType_i == 0) { // centered
        *startTime = time - shutterInterval / 2.;
        *endTime = time + shutterInterval / 2.;
    } else if (shutterType_i == 1) { // start
        *startTime = time;
        *endTime = time + shutterInterval;
    } else if (shutterType_i == 2) { // end
        *startTime = time - shutterInterval;
        *endTime = time;
    } else if (shutterType_i == 3) { // custom
        *startTime = time + _imp->globalCustomOffsetKnob.lock()->getValueAtTime(time);
        *endTime = *startTime + shutterInterval;
    } else {
        assert(false);
    }


#endif
}

void
RotoContext::getItemsRegionOfDefinition(const std::list<RotoItemPtr >& items,
                                        double time,
                                        ViewIdx /*view*/,
                                        RectD* rod) const
{
    double startTime = time, mbFrameStep = 1., endTime = time;

#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    int mbType_i = getMotionBlurTypeKnob()->getValue();
    bool applyGlobalMotionBlur = mbType_i == 1;
    if (applyGlobalMotionBlur) {
        getGlobalMotionBlurSettings(time, &startTime, &endTime, &mbFrameStep);
    }
#endif


    bool rodSet = false;
    NodePtr activeRotoPaintNode;
    RotoStrokeItemPtr activeStroke;
    bool isDrawing;
    getNode()->getApp()->getActiveRotoDrawingStroke(&activeRotoPaintNode, &activeStroke, &isDrawing);
    if (!isDrawing) {
        activeStroke.reset();
    }

    QMutexLocker l(&_imp->rotoContextMutex);
    for (double t = startTime; t <= endTime; t += mbFrameStep) {
        bool first = true;
        RectD bbox;
        for (std::list<RotoItemPtr >::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            BezierPtr isBezier = toBezier(*it2);
            RotoStrokeItemPtr isStroke = toRotoStrokeItem(*it2);
            if (isBezier && !isStroke) {
                if ( isBezier->isActivated(time)  && (isBezier->getControlPointsCount() > 1) ) {
                    RectD splineRoD = isBezier->getBoundingBox(t);
                    if ( splineRoD.isNull() ) {
                        continue;
                    }

                    if (first) {
                        first = false;
                        bbox = splineRoD;
                    } else {
                        bbox.merge(splineRoD);
                    }
                }
            } else if (isStroke) {
                RectD strokeRod;
                if ( isStroke->isActivated(time) ) {
                    if ( isStroke == activeStroke ) {
                        strokeRod = isStroke->getMergeNode()->getPaintStrokeRoD_duringPainting();
                    } else {
                        strokeRod = isStroke->getBoundingBox(t);
                    }
                    if (first) {
                        first = false;
                        bbox = strokeRod;
                    } else {
                        bbox.merge(strokeRod);
                    }
                }
            }
        }
        if (!rodSet) {
            *rod = bbox;
            rodSet = true;
        } else {
            rod->merge(bbox);
        }
    }
} // RotoContext::getItemsRegionOfDefinition

bool
RotoContext::isRotoPaintTreeConcatenatableInternal(const std::list<RotoDrawableItemPtr >& items,
                                                   int* blendingMode)
{
    bool operatorSet = false;
    int comp_i = -1;

    for (std::list<RotoDrawableItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
        int op = (*it)->getCompositingOperator();
        if (!operatorSet) {
            operatorSet = true;
            comp_i = op;
        } else {
            if (op != comp_i) {
                //2 items have a different compositing operator
                return false;
            }
        }
        RotoStrokeItemPtr isStroke = toRotoStrokeItem(*it);
        if (!isStroke) {
            assert( toBezier(*it) );
        } else {
            if (isStroke->getBrushType() != eRotoStrokeTypeSolid) {
                return false;
            }
        }
    }
    if (operatorSet) {
        *blendingMode = comp_i;

        return true;
    }

    return false;
}

bool
RotoContext::isRotoPaintTreeConcatenatable() const
{
    std::list<RotoDrawableItemPtr > items = getCurvesByRenderOrder();
    int bop;

    return isRotoPaintTreeConcatenatableInternal(items, &bop);
}

bool
RotoContext::isEmpty() const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->layers.empty();
}

void
RotoContext::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::RotoContextSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::RotoContextSerialization*>(obj);
    if (!s) {
        return;
    }

    QMutexLocker l(&_imp->rotoContextMutex);

    ///There must always be the base layer
    assert( !_imp->layers.empty() );

    ///Serializing this layer will recursively serialize everything
    _imp->layers.front()->toSerialization(&s->_baseLayer);
}


void
RotoContext::resetToDefault()
{
    assert(_imp->layers.size() == 1);
    RotoLayerPtr baseLayer = _imp->layers.front();
    RotoItem::SelectionReasonEnum reason = RotoItem::eSelectionReasonOther;
    removeItemRecursively(baseLayer, reason);
    createBaseLayer();
    Q_EMIT selectionChanged( (int)reason );


}

void
RotoContext::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::RotoContextSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::RotoContextSerialization*>(&obj);
    if (!s) {
        return;
    }

    assert( QThread::currentThread() == qApp->thread() );
    ///no need to lock here, when this is called the main-thread is the only active thread
    
    _imp->isCurrentlyLoading = true;

    for (std::list<KnobIWPtr >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        it->lock()->setAllDimensionsEnabled(false);
    }

    assert(_imp->layers.size() == 1);

    RotoLayerPtr baseLayer = _imp->layers.front();

    baseLayer->fromSerialization(s->_baseLayer);

    _imp->isCurrentlyLoading = false;
    refreshRotoPaintTree();
}

void
RotoContext::select(const RotoItemPtr & b,
                    RotoItem::SelectionReasonEnum reason)
{
    selectInternal(b);
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::select(const std::list<RotoDrawableItemPtr > & beziers,
                    RotoItem::SelectionReasonEnum reason)
{
    for (std::list<RotoDrawableItemPtr >::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
        selectInternal(*it);
    }

    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::select(const std::list<RotoItemPtr > & items,
                    RotoItem::SelectionReasonEnum reason)
{
    for (std::list<RotoItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
        selectInternal(*it);
    }

    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const RotoItemPtr & b,
                      RotoItem::SelectionReasonEnum reason)
{
    deselectInternal(b);

    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const std::list<BezierPtr > & beziers,
                      RotoItem::SelectionReasonEnum reason)
{
    for (std::list<BezierPtr >::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
        deselectInternal(*it);
    }

    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const std::list<RotoItemPtr > & items,
                      RotoItem::SelectionReasonEnum reason)
{
    for (std::list<RotoItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
        deselectInternal(*it);
    }

    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::clearAndSelectPreviousItem(const RotoItemPtr& item,
                                        RotoItem::SelectionReasonEnum reason)
{
    clearSelection(reason);
    assert(item);
    RotoItemPtr prev = item->getPreviousItemInLayer();
    if (prev) {
        select(prev, reason);
    }
}

void
RotoContext::clearSelection(RotoItem::SelectionReasonEnum reason)
{
    bool empty;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        empty = _imp->selectedItems.empty();
    }

    while (!empty) {
        RotoItemPtr item;
        {
            QMutexLocker l(&_imp->rotoContextMutex);
            item = _imp->selectedItems.front();
        }
        deselectInternal(item);

        QMutexLocker l(&_imp->rotoContextMutex);
        empty = _imp->selectedItems.empty();
    }
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::selectInternal(const RotoItemPtr & item)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    int nbUnlockedBeziers = 0;
    int nbUnlockedStrokes = 0;
    int nbStrokeWithoutStrength = 0;
    int nbStrokeWithoutCloneFunctions = 0;
    bool foundItem = false;

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            BezierPtr isBezier = toBezier(*it);
            RotoStrokeItemPtr isStroke = toRotoStrokeItem(*it);
            if ( !isStroke && isBezier && !isBezier->isLockedRecursive() ) {
                if ( isBezier->isOpenBezier() ) {
                    ++nbUnlockedStrokes;
                } else {
                    ++nbUnlockedBeziers;
                }
            } else if (isStroke) {
                ++nbUnlockedStrokes;
            }
            if ( it->get() == item.get() ) {
                foundItem = true;
            }
        }
    }
    ///the item is already selected, exit
    if (foundItem) {
        return;
    }


    BezierPtr isBezier = toBezier(item);
    RotoStrokeItemPtr isStroke = toRotoStrokeItem(item);
    RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(item);
    RotoLayerPtr isLayer = toRotoLayer(item);

    if (isDrawable) {
        if ( !isStroke && isBezier && !isBezier->isLockedRecursive() ) {
            if ( isBezier->isOpenBezier() ) {
                ++nbUnlockedStrokes;
            } else {
                ++nbUnlockedBeziers;
            }
            ++nbStrokeWithoutCloneFunctions;
            ++nbStrokeWithoutStrength;
        } else if (isStroke) {
            ++nbUnlockedStrokes;
            if ( (isStroke->getBrushType() != eRotoStrokeTypeBlur) && (isStroke->getBrushType() != eRotoStrokeTypeSharpen) ) {
                ++nbStrokeWithoutStrength;
            }
            if ( (isStroke->getBrushType() != eRotoStrokeTypeClone) && (isStroke->getBrushType() != eRotoStrokeTypeReveal) ) {
                ++nbStrokeWithoutCloneFunctions;
            }
        }

        const KnobsVec& drawableKnobs = isDrawable->getKnobs();
        for (KnobsVec::const_iterator it = drawableKnobs.begin(); it != drawableKnobs.end(); ++it) {
            for (std::list<KnobIWPtr >::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
                KnobIPtr thisKnob = it2->lock();
                if ( thisKnob->getName() == (*it)->getName() ) {
                    //Clone current state
                    thisKnob->cloneAndUpdateGui(*it);

                    //Slave internal knobs of the bezier
                    assert( (*it)->getDimension() == thisKnob->getDimension() );
                    for (int i = 0; i < (*it)->getDimension(); ++i) {
                        (*it)->slaveTo(i, thisKnob, i, true);
                    }

                    QObject::connect( (*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,ViewSpec,int,int,bool)),
                                      this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::connect( (*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                                      this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::connect( (*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec,int,double,double)),
                                      this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::connect( (*it)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewSpec,int)),
                                      this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::connect( (*it)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,ViewSpec,int)),
                                      this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::connect( (*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,ViewSpec,int)),
                                      this, SLOT(onSelectedKnobCurveChanged()) );

                    break;
                }
            }
        }
    } else if (isLayer) {
        const RotoItems & children = isLayer->getItems();
        for (RotoItems::const_iterator it = children.begin(); it != children.end(); ++it) {
            selectInternal(*it);
        }
    }

    ///enable the knobs
    if ( (nbUnlockedBeziers > 0) || (nbUnlockedStrokes > 0) ) {
        KnobIPtr strengthKnob = _imp->brushEffectKnob.lock();
        KnobIPtr sourceTypeKnob = _imp->sourceTypeKnob.lock();
        KnobIPtr timeOffsetKnob = _imp->timeOffsetKnob.lock();
        KnobIPtr timeOffsetModeKnob = _imp->timeOffsetModeKnob.lock();
        for (std::list<KnobIWPtr >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            KnobIPtr k = it->lock();
            if (!k) {
                continue;
            }

            if (k == strengthKnob) {
                if (nbStrokeWithoutStrength) {
                    k->setAllDimensionsEnabled(false);
                } else {
                    k->setAllDimensionsEnabled(true);
                }
            } else {
                bool mustDisable = false;
                if (nbStrokeWithoutCloneFunctions) {
                    bool isCloneKnob = false;
                    for (std::list<KnobIWPtr >::iterator it2 = _imp->cloneKnobs.begin(); it2 != _imp->cloneKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isCloneKnob = true;
                        }
                    }
                    mustDisable |= isCloneKnob;
                }
                if (nbUnlockedBeziers && !mustDisable) {
                    bool isStrokeKnob = false;
                    for (std::list<KnobIWPtr >::iterator it2 = _imp->strokeKnobs.begin(); it2 != _imp->strokeKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isStrokeKnob = true;
                        }
                    }
                    mustDisable |= isStrokeKnob;
                }
                if (nbUnlockedStrokes && !mustDisable) {
                    bool isBezierKnob = false;
                    for (std::list<KnobIWPtr >::iterator it2 = _imp->shapeKnobs.begin(); it2 != _imp->shapeKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isBezierKnob = true;
                        }
                    }
                    mustDisable |= isBezierKnob;
                }
                k->setAllDimensionsEnabled(!mustDisable);
            }
            if ( (nbUnlockedBeziers >= 2) || (nbUnlockedStrokes >= 2) ) {
                k->setDirty(true);
            }
        }

        //show activated/frame knob according to lifetime
        int lifetime_i = _imp->lifeTime.lock()->getValue();
        _imp->activated.lock()->setSecret(lifetime_i != 3);
        _imp->lifeTimeFrame.lock()->setSecret(lifetime_i == 3);
    }

    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->selectedItems.push_back(item);
} // selectInternal

void
RotoContext::onSelectedKnobCurveChanged()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    if (handler) {
        KnobIPtr knob = handler->getKnob();
        for (std::list<KnobIWPtr >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            KnobIPtr k = it->lock();
            if ( k->getName() == knob->getName() ) {
                k->clone(knob);
                break;
            }
        }
    }
}

void
RotoContext::deselectInternal(RotoItemPtr b)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    int nbBeziersUnLockedBezier = 0;
    int nbStrokesUnlocked = 0;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        std::list<RotoItemPtr >::iterator foundSelected = std::find(_imp->selectedItems.begin(), _imp->selectedItems.end(), b);

        ///if the item is not selected, exit
        if ( foundSelected == _imp->selectedItems.end() ) {
            return;
        }

        _imp->selectedItems.erase(foundSelected);

        for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            BezierPtr isBezier = toBezier(*it);
            RotoStrokeItemPtr isStroke = toRotoStrokeItem(*it);
            if ( !isStroke && isBezier && !isBezier->isLockedRecursive() ) {
                ++nbBeziersUnLockedBezier;
            } else if (isStroke) {
                ++nbStrokesUnlocked;
            }
        }
    }
    bool bezierDirty = nbBeziersUnLockedBezier > 1;
    bool strokeDirty = nbStrokesUnlocked > 1;
    BezierPtr isBezier = toBezier(b);
    RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(b);
    RotoStrokeItemPtr isStroke = toRotoStrokeItem(b);
    RotoLayerPtr isLayer = toRotoLayer(b);
    if (isDrawable) {
        ///first-off set the context knobs to the value of this bezier

        const KnobsVec& drawableKnobs = isDrawable->getKnobs();
        for (KnobsVec::const_iterator it = drawableKnobs.begin(); it != drawableKnobs.end(); ++it) {
            for (std::list<KnobIWPtr >::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
                KnobIPtr knob = it2->lock();
                if ( knob->getName() == (*it)->getName() ) {
                    //Clone current state
                    knob->cloneAndUpdateGui(*it);

                    //Slave internal knobs of the bezier
                    assert( (*it)->getDimension() == knob->getDimension() );
                    for (int i = 0; i < (*it)->getDimension(); ++i) {
                        (*it)->unSlave(i, isBezier ? !bezierDirty : !strokeDirty);
                    }

                    QObject::disconnect( (*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,ViewSpec,int,int,bool)),
                                         this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::disconnect( (*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                                         this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::disconnect( (*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(ViewSpec,int,double,double)),
                                         this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::disconnect( (*it)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(ViewSpec,int)),
                                         this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::disconnect( (*it)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,ViewSpec,int)),
                                         this, SLOT(onSelectedKnobCurveChanged()) );
                    QObject::disconnect( (*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,ViewSpec,int)),
                                         this, SLOT(onSelectedKnobCurveChanged()) );
                    break;
                }
            }
        }
    } else if (isLayer) {
        const RotoItems & children = isLayer->getItems();
        for (RotoItems::const_iterator it = children.begin(); it != children.end(); ++it) {
            deselectInternal(*it);
        }
    }


    ///if the selected beziers count reaches 0 notify the gui knobs so they appear not enabled

    if ( (nbBeziersUnLockedBezier == 0) || (nbStrokesUnlocked == 0) ) {
        for (std::list<KnobIWPtr >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            KnobIPtr k = it->lock();
            if (!k) {
                continue;
            }
            k->setAllDimensionsEnabled(false);
            if (!bezierDirty || !strokeDirty) {
                k->setDirty(false);
            }
        }
    }
} // deselectInternal

void
RotoContext::resetTransformsCenter(bool doClone,
                                   bool doTransform)
{
    double time = getNode()->getApp()->getTimeLine()->currentFrame();
    RectD bbox;

    getItemsRegionOfDefinition(getSelectedItems(), time, ViewIdx(0), &bbox);
    if (doTransform) {
        KnobDoublePtr centerKnob = _imp->centerKnob.lock();
        centerKnob->beginChanges();
        centerKnob->removeAnimation(ViewSpec::all(), 0);
        centerKnob->removeAnimation(ViewSpec::all(), 1);
        centerKnob->setValues( (bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2., ViewSpec::all(), eValueChangedReasonNatronInternalEdited );
        centerKnob->endChanges();
    }
    if (doClone) {
        KnobDoublePtr centerKnob = _imp->cloneCenterKnob.lock();
        centerKnob->beginChanges();
        centerKnob->removeAnimation(ViewSpec::all(), 0);
        centerKnob->removeAnimation(ViewSpec::all(), 1);
        centerKnob->setValues( (bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2., ViewSpec::all(), eValueChangedReasonNatronInternalEdited );
        centerKnob->endChanges();
    }
}

void
RotoContext::resetTransformCenter()
{
    resetTransformsCenter(false, true);
}

void
RotoContext::resetCloneTransformCenter()
{
    resetTransformsCenter(true, false);
}

void
RotoContext::resetTransformInternal(const KnobDoublePtr& translate,
                                    const KnobDoublePtr& scale,
                                    const KnobDoublePtr& center,
                                    const KnobDoublePtr& rotate,
                                    const KnobDoublePtr& skewX,
                                    const KnobDoublePtr& skewY,
                                    const KnobBoolPtr& scaleUniform,
                                    const KnobChoicePtr& skewOrder,
                                    const KnobDoublePtr& extraMatrix)
{
    std::list<KnobIPtr> knobs;

    knobs.push_back(translate);
    knobs.push_back(scale);
    knobs.push_back(center);
    knobs.push_back(rotate);
    knobs.push_back(skewX);
    knobs.push_back(skewY);
    knobs.push_back(scaleUniform);
    knobs.push_back(skewOrder);
    if (extraMatrix) {
        knobs.push_back(extraMatrix);
    }
    bool wasEnabled = translate->isEnabled(0);
    for (std::list<KnobIPtr>::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        for (int i = 0; i < (*it)->getDimension(); ++i) {
            (*it)->resetToDefaultValue(i);
        }
        (*it)->setAllDimensionsEnabled(wasEnabled);
    }
}

void
RotoContext::resetTransform()
{
    KnobDoublePtr translate = _imp->translateKnob.lock();
    KnobDoublePtr center = _imp->centerKnob.lock();
    KnobDoublePtr scale = _imp->scaleKnob.lock();
    KnobDoublePtr rotate = _imp->rotateKnob.lock();
    KnobBoolPtr uniform = _imp->scaleUniformKnob.lock();
    KnobDoublePtr skewX = _imp->skewXKnob.lock();
    KnobDoublePtr skewY = _imp->skewYKnob.lock();
    KnobChoicePtr skewOrder = _imp->skewOrderKnob.lock();
    KnobDoublePtr extraMatrix = _imp->extraMatrixKnob.lock();

    resetTransformInternal(translate, scale, center, rotate, skewX, skewY, uniform, skewOrder, extraMatrix);
}

void
RotoContext::resetCloneTransform()
{
    KnobDoublePtr translate = _imp->cloneTranslateKnob.lock();
    KnobDoublePtr center = _imp->cloneCenterKnob.lock();
    KnobDoublePtr scale = _imp->cloneScaleKnob.lock();
    KnobDoublePtr rotate = _imp->cloneRotateKnob.lock();
    KnobBoolPtr uniform = _imp->cloneUniformKnob.lock();
    KnobDoublePtr skewX = _imp->cloneSkewXKnob.lock();
    KnobDoublePtr skewY = _imp->cloneSkewYKnob.lock();
    KnobChoicePtr skewOrder = _imp->cloneSkewOrderKnob.lock();

    resetTransformInternal( translate, scale, center, rotate, skewX, skewY, uniform, skewOrder, KnobDoublePtr() );
}

bool
RotoContext::knobChanged(const KnobIPtr& k,
                         ValueChangedReasonEnum /*reason*/,
                         ViewSpec /*view*/,
                         double /*time*/,
                         bool /*originatedFromMainThread*/)
{
    bool ret = true;

    if ( k == _imp->resetCenterKnob.lock() ) {
        resetTransformCenter();
    } else if ( k == _imp->resetCloneCenterKnob.lock() ) {
        resetCloneTransformCenter();
    } else if ( k == _imp->resetTransformKnob.lock() ) {
        resetTransform();
    } else if ( k == _imp->resetCloneTransformKnob.lock() ) {
        resetCloneTransform();
    }
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    else if ( k == _imp->motionBlurTypeKnob.lock().get() ) {
        int mbType_i = getMotionBlurTypeKnob()->getValue();
        bool isPerShapeMB = mbType_i == 0;
        _imp->motionBlurKnob.lock()->setSecret(!isPerShapeMB);
        _imp->shutterKnob.lock()->setSecret(!isPerShapeMB);
        _imp->shutterTypeKnob.lock()->setSecret(!isPerShapeMB);
        _imp->customOffsetKnob.lock()->setSecret(!isPerShapeMB);

        _imp->globalMotionBlurKnob.lock()->setSecret(isPerShapeMB);
        _imp->globalShutterKnob.lock()->setSecret(isPerShapeMB);
        _imp->globalShutterTypeKnob.lock()->setSecret(isPerShapeMB);
        _imp->globalCustomOffsetKnob.lock()->setSecret(isPerShapeMB);
    }
#endif
    else {
        ret = false;
    }

    return ret;
}

RotoItemPtr
RotoContext::getLastItemLocked() const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->lastLockedItem;
}

static void
addOrRemoveKeyRecursively(const RotoLayerPtr& isLayer,
                          double time,
                          bool add,
                          bool removeAll)
{
    const RotoItems & items = isLayer->getItems();

    for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
        RotoLayerPtr layer = toRotoLayer(*it2);
        BezierPtr isBezier = toBezier(*it2);
        if (isBezier) {
            if (add) {
                isBezier->setKeyframe(time);
            } else {
                if (!removeAll) {
                    isBezier->removeKeyframe(time);
                } else {
                    isBezier->removeAnimation();
                }
            }
        } else if (layer) {
            addOrRemoveKeyRecursively(layer, time, add, removeAll);
        }
    }
}

void
RotoContext::setKeyframeOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        RotoLayerPtr isLayer = toRotoLayer(*it);
        BezierPtr isBezier = toBezier(*it);
        if (isBezier) {
            isBezier->setKeyframe(time);
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer, time, true, false);
        }
    }
}

void
RotoContext::removeAnimationOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        RotoLayerPtr isLayer = toRotoLayer(*it);
        BezierPtr isBezier = toBezier(*it);
        if (isBezier) {
            isBezier->removeAnimation();
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer, time, false, true);
        }
    }

}

void
RotoContext::removeKeyframeOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        RotoLayerPtr isLayer = toRotoLayer(*it);
        BezierPtr isBezier = toBezier(*it);
        if (isBezier) {
            isBezier->removeKeyframe(time);
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer, time, false, false);
        }
    }
}

static void
findOutNearestKeyframeRecursively(const RotoLayerPtr& layer,
                                  bool previous,
                                  double time,
                                  int* nearest)
{
    const RotoItems & items = layer->getItems();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        RotoLayerPtr layer = toRotoLayer(*it);
        BezierPtr isBezier = toBezier(*it);
        if (isBezier) {
            if (previous) {
                int t = isBezier->getPreviousKeyframeTime(time);
                if ( (t != INT_MIN) && (t > *nearest) ) {
                    *nearest = t;
                }
            } else if (layer) {
                int t = isBezier->getNextKeyframeTime(time);
                if ( (t != INT_MAX) && (t < *nearest) ) {
                    *nearest = t;
                }
            }
        } else {
            assert(layer);
            if (layer) {
                findOutNearestKeyframeRecursively(layer, previous, time, nearest);
            }
        }
    }
}

void
RotoContext::goToPreviousKeyframe()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    int minimum = INT_MIN;

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            RotoLayerPtr layer = toRotoLayer(*it);
            BezierPtr isBezier = toBezier(*it);
            if (isBezier) {
                int t = isBezier->getPreviousKeyframeTime(time);
                if ( (t != INT_MIN) && (t > minimum) ) {
                    minimum = t;
                }
            } else {
                assert(layer);
                if (layer) {
                    findOutNearestKeyframeRecursively(layer, true, time, &minimum);
                }
            }
        }
    }

    if (minimum != INT_MIN) {
        getNode()->getApp()->setLastViewerUsingTimeline( NodePtr() );
        getNode()->getApp()->getTimeLine()->seekFrame(minimum, false, OutputEffectInstancePtr(), eTimelineChangeReasonOtherSeek);
    }
}

void
RotoContext::goToNextKeyframe()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    int maximum = INT_MAX;

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            RotoLayerPtr isLayer = toRotoLayer(*it);
            BezierPtr isBezier = toBezier(*it);
            if (isBezier) {
                int t = isBezier->getNextKeyframeTime(time);
                if ( (t != INT_MAX) && (t < maximum) ) {
                    maximum = t;
                }
            } else {
                assert(isLayer);
                if (isLayer) {
                    findOutNearestKeyframeRecursively(isLayer, false, time, &maximum);
                }
            }
        }
    }
    if (maximum != INT_MAX) {
        getNode()->getApp()->setLastViewerUsingTimeline( NodePtr() );
        getNode()->getApp()->getTimeLine()->seekFrame(maximum, false, OutputEffectInstancePtr(), eTimelineChangeReasonOtherSeek);
    }
}

static void
appendToSelectedCurvesRecursively(std::list< RotoDrawableItemPtr > * curves,
                                  const RotoLayerPtr& isLayer,
                                  double time,
                                  bool onlyActives,
                                  bool addStrokes)
{
    RotoItems items = isLayer->getItems_mt_safe();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        RotoLayerPtr layer = toRotoLayer(*it);
        RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);
        RotoStrokeItemPtr isStroke = toRotoStrokeItem( isDrawable );
        if (isStroke && !addStrokes) {
            continue;
        }
        if (isDrawable) {
            if ( !onlyActives || isDrawable->isActivated(time) ) {
                curves->push_front(isDrawable);
            }
        } else if ( layer && layer->isGloballyActivated() ) {
            appendToSelectedCurvesRecursively(curves, layer, time, onlyActives, addStrokes);
        }
    }
}

const std::list< RotoItemPtr > &
RotoContext::getSelectedItems() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->selectedItems;
}

std::list< RotoDrawableItemPtr >
RotoContext::getSelectedCurves() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    std::list< RotoDrawableItemPtr > drawables;
    double time = getTimelineCurrentTime();
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            assert(*it);
            RotoLayerPtr isLayer = toRotoLayer(*it);
            RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);
            if (isDrawable) {
                drawables.push_back(isDrawable);
            } else {
                assert(isLayer);
                if (isLayer) {
                    appendToSelectedCurvesRecursively(&drawables, isLayer, time, false, true);
                }
            }
        }
    }

    return drawables;
}

std::list< RotoDrawableItemPtr >
RotoContext::getCurvesByRenderOrder(bool onlyActivated) const
{
    std::list< RotoDrawableItemPtr > ret;
    NodePtr node = getNode();

    if (!node) {
        return ret;
    }
    ///Note this might not be the timeline's current frame if this is a render thread.
    EffectInstancePtr effect = node->getEffectInstance();
    if (!effect) {
        return ret;
    }
    double time = effect->getCurrentTime();
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        if ( !_imp->layers.empty() ) {
            appendToSelectedCurvesRecursively(&ret, _imp->layers.front(), time, onlyActivated, true);
        }
    }

    return ret;
}

int
RotoContext::getNCurves() const
{
    std::list< RotoDrawableItemPtr > curves = getCurvesByRenderOrder();

    return (int)curves.size();
}

RotoLayerPtr
RotoContext::getLayerByName(const std::string & n) const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    for (std::list<RotoLayerPtr >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        if ( (*it)->getScriptName() == n ) {
            return *it;
        }
    }

    return RotoLayerPtr();
}

static void
findItemRecursively(const std::string & n,
                    const RotoLayerPtr & layer,
                    RotoItemPtr* ret)
{
    if (layer->getScriptName() == n) {
        *ret = boost::dynamic_pointer_cast<RotoItem>(layer);
    } else {
        const RotoItems & items = layer->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            RotoLayerPtr isLayer = toRotoLayer(*it2);
            if ( (*it2)->getScriptName() == n ) {
                *ret = *it2;

                return;
            } else if (isLayer) {
                findItemRecursively(n, isLayer, ret);
            }
        }
    }
}

RotoItemPtr
RotoContext::getItemByName(const std::string & n) const
{
    RotoItemPtr ret;
    QMutexLocker l(&_imp->rotoContextMutex);

    for (std::list<RotoLayerPtr >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        findItemRecursively(n, *it, &ret);
    }

    return ret;
}

RotoLayerPtr
RotoContext::getDeepestSelectedLayer() const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    return findDeepestSelectedLayer();
}

RotoLayerPtr
RotoContext::findDeepestSelectedLayer() const
{
    QMutexLocker k(&_imp->rotoContextMutex);

    return _imp->findDeepestSelectedLayer();
}



void
RotoContext::clearViewersLastRenderedStrokes()
{
    std::list<ViewerInstancePtr> viewers;

    getNode()->hasViewersConnected(&viewers);
    for (std::list<ViewerInstancePtr>::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        (*it)->clearLastRenderedImage();
    }
}

void
RotoContext::onItemLockedChanged(const RotoItemPtr& item,
                                 RotoItem::SelectionReasonEnum reason)
{
    assert(item);
    ///refresh knobs
    int nbBeziersUnLockedBezier = 0;

    {
        QMutexLocker l(&_imp->rotoContextMutex);

        for (std::list<RotoItemPtr >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            BezierPtr isBezier = toBezier(*it);
            if ( isBezier && !isBezier->isLockedRecursive() ) {
                ++nbBeziersUnLockedBezier;
            }
        }
    }
    bool dirty = nbBeziersUnLockedBezier > 1;
    bool enabled = nbBeziersUnLockedBezier > 0;

    for (std::list<KnobIWPtr >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        KnobIPtr knob = it->lock();
        if (!knob) {
            continue;
        }
        knob->setDirty(dirty);
        knob->setAllDimensionsEnabled(enabled);
    }
    _imp->lastLockedItem = item;
    Q_EMIT itemLockedChanged( (int)reason );
}

void
RotoContext::onItemScriptNameChanged(const RotoItemPtr& item)
{
    Q_EMIT itemScriptNameChanged(item);
}

void
RotoContext::onItemLabelChanged(const RotoItemPtr& item)
{
    Q_EMIT itemLabelChanged(item);
}

std::string
RotoContext::getRotoNodeName() const
{
    return getNode()->getScriptName_mt_safe();
}

void
RotoContext::emitRefreshViewerOverlays()
{
    Q_EMIT refreshViewerOverlays();
}

void
RotoContext::getBeziersKeyframeTimes(std::list<double> *times) const
{
    std::list< RotoDrawableItemPtr > splines = getCurvesByRenderOrder();

    for (std::list< RotoDrawableItemPtr > ::iterator it = splines.begin(); it != splines.end(); ++it) {
        std::set<double> splineKeys;
        BezierPtr isBezier = toBezier(*it);
        if (!isBezier) {
            continue;
        }
        isBezier->getKeyframeTimes(&splineKeys);
        for (std::set<double>::iterator it2 = splineKeys.begin(); it2 != splineKeys.end(); ++it2) {
            times->push_back(*it2);
        }
    }
}


KnobChoicePtr
RotoContext::getMotionBlurTypeKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR

    return _imp->motionBlurTypeKnob.lock();
#else

    return KnobChoicePtr();
#endif
}

bool
RotoContext::isDoingNeatRender() const
{
    RotoPaint* rotoNode = dynamic_cast<RotoPaint*>(getNode()->getEffectInstance().get());
    if (!rotoNode) {
        return false;
    }
    return rotoNode->isDoingNeatRender();
}


void
RotoContext::changeItemScriptName(const std::string& oldFullyQualifiedName,
                                  const std::string& newFullyQUalifiedName)
{
    if  (oldFullyQualifiedName == newFullyQUalifiedName) {
        return;
    }
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string declStr = nodeFullName + ".roto." + newFullyQUalifiedName + " = " + nodeFullName + ".roto." + oldFullyQualifiedName + "\n";
    std::string delStr = "del " + nodeFullName + ".roto." + oldFullyQualifiedName + "\n";
    std::string script = declStr + delStr;

    if ( !appPTR->isBackground() ) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
}

void
RotoContext::removeItemAsPythonField(const RotoItemPtr& item)
{
    RotoStrokeItemPtr isStroke = toRotoStrokeItem( item );

    if (isStroke) {
        ///Strokes are unsupported in Python currently
        return;
    }
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = "del " + nodeFullName + ".roto." + item->getFullyQualifiedName() + "\n";
    if ( !appPTR->isBackground() ) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
}

static void setOperationKnob(const NodePtr& node, int blendingOperator)
{
    KnobIPtr mergeOperatorKnob = node->getKnobByName(kMergeOFXParamOperation);
    KnobChoicePtr mergeOp = toKnobChoice( mergeOperatorKnob );
    if (mergeOp) {
        mergeOp->setValue(blendingOperator);
    }
}

NodePtr
RotoContext::getOrCreateGlobalMergeNode(int blendingOperator, int *availableInputIndex)
{
    {
        QMutexLocker k(&_imp->rotoContextMutex);
        for (NodesList::iterator it = _imp->globalMergeNodes.begin(); it != _imp->globalMergeNodes.end(); ++it) {
            const std::vector<NodeWPtr > &inputs = (*it)->getInputs();

            // Merge node goes like this: B, A, Mask, A2, A3, A4 ...
            assert( inputs.size() >= 3 && (*it)->getEffectInstance()->isInputMask(2) );
            if ( !inputs[1].lock() ) {
                *availableInputIndex = 1;
                setOperationKnob(*it, blendingOperator);
                return *it;
            }

            //Leave the B empty to connect the next merge node
            for (std::size_t i = 3; i < inputs.size(); ++i) {
                if ( !inputs[i].lock() ) {
                    *availableInputIndex = (int)i;
                    setOperationKnob(*it, blendingOperator);
                    return *it;
                }
            }
        }
    }


    NodePtr node = getNode();
    RotoPaintPtr rotoPaintEffect = toRotoPaint(node->getEffectInstance());


    //We must create a new merge node
    QString fixedNamePrefix = QString::fromUtf8( node->getScriptName_mt_safe().c_str() );

    fixedNamePrefix.append( QLatin1Char('_') );
    fixedNamePrefix.append( QString::fromUtf8("globalMerge") );
    fixedNamePrefix.append( QLatin1Char('_') );


    CreateNodeArgs args( PLUGINID_OFX_MERGE,  rotoPaintEffect );
    args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    args.setProperty<bool>(kCreateNodeArgsPropVolatile, true);
    args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
    
    NodePtr mergeNode = node->getApp()->createNode(args);
    if (!mergeNode) {
        return mergeNode;
    }
    if ( getNode()->isDuringPaintStrokeCreation() ) {
        mergeNode->setWhileCreatingPaintStroke(true);
    }



    {
        // Link OpenGL enabled knob to the one on the Rotopaint so the user can control if GPU rendering is used in the roto internal node graph
        KnobChoicePtr glRenderKnob = mergeNode->getOpenGLEnabledKnob();
        if (glRenderKnob) {
            KnobChoicePtr rotoPaintGLRenderKnob = node->getOpenGLEnabledKnob();
            assert(rotoPaintGLRenderKnob);
            glRenderKnob->slaveTo(0, rotoPaintGLRenderKnob, 0);
        }
    }
    *availableInputIndex = 1;
    setOperationKnob(mergeNode, blendingOperator);

    {
        // Link the RGBA enabled checkbox of the Rotopaint to the merge output RGBA
        KnobBoolPtr rotoPaintRGBA[4];
        KnobBoolPtr mergeRGBA[4];
        rotoPaintEffect->getEnabledChannelKnobs(&rotoPaintRGBA[0], &rotoPaintRGBA[1], &rotoPaintRGBA[2], &rotoPaintRGBA[3]);
        mergeRGBA[0] = toKnobBool(mergeNode->getKnobByName(kMergeParamOutputChannelsR));
        mergeRGBA[1] = toKnobBool(mergeNode->getKnobByName(kMergeParamOutputChannelsG));
        mergeRGBA[2] = toKnobBool(mergeNode->getKnobByName(kMergeParamOutputChannelsB));
        mergeRGBA[3] = toKnobBool(mergeNode->getKnobByName(kMergeParamOutputChannelsA));
        for (int i = 0; i < 4; ++i) {
            mergeRGBA[i]->slaveTo(0, rotoPaintRGBA[i], 0);
        }

        // Link mix
        KnobIPtr rotoPaintMix = rotoPaintEffect->getKnobByName(kHostMixingKnobName);
        KnobIPtr mergeMix = mergeNode->getKnobByName(kMergeOFXParamMix);
        mergeMix->slaveTo(0, rotoPaintMix, 0);

    }


    QMutexLocker k(&_imp->rotoContextMutex);
    _imp->globalMergeNodes.push_back(mergeNode);

    return mergeNode;
} // RotoContext::getOrCreateGlobalMergeNode

bool
RotoContext::canConcatenatedRotoPaintTree() const
{
    std::list<RotoDrawableItemPtr > items = getCurvesByRenderOrder();
    int blendingOperator;
    return isRotoPaintTreeConcatenatableInternal(items, &blendingOperator);
}

bool
RotoContext::isAnimated() const
{
    std::list<RotoDrawableItemPtr> items = getCurvesByRenderOrder();
    for (std::list<RotoDrawableItemPtr>::iterator it = items.begin(); it!=items.end(); ++it) {

        const KnobsVec& knobs = (*it)->getKnobs();
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2!=knobs.end(); ++it2) {
            for (int i = 0; i < (*it2)->getDimension(); ++i) {
                if ((*it2)->isAnimated(i)) {
                    return true;
                }
            }
        }


        BezierPtr isBezier = toBezier(*it);
        if (isBezier) {
            int keys = isBezier->getKeyframesCount();
            if (keys > 1) {
                return true;
            }
        }
    }
    return false;
}

static void connectRotoPaintBottomTreeToItems(const RotoPaintPtr& rotoPaintEffect, const NodePtr& noOpNode, const NodePtr& premultNode, const NodePtr& treeOutputNode, const NodePtr& mergeNode)
{
    if (treeOutputNode->getInput(0) != noOpNode) {
        treeOutputNode->disconnectInput(0);
        treeOutputNode->connectInput(noOpNode, 0);
    }
    if (noOpNode->getInput(0) != premultNode) {
        noOpNode->disconnectInput(0);
        noOpNode->connectInput(premultNode, 0);
    }
    if (premultNode->getInput(0) != mergeNode) {
        premultNode->disconnectInput(0);
        premultNode->connectInput(mergeNode, 0);
    }
    // Connect the mask of the merge to the Mask input
    mergeNode->disconnectInput(2);
    mergeNode->connectInput(rotoPaintEffect->getInternalInputNode(ROTOPAINT_MASK_INPUT_INDEX), 2);

}

void
RotoContext::refreshRotoPaintTree()
{
    if (_imp->isCurrentlyLoading) {
        return;
    }

    std::list<RotoDrawableItemPtr > items = getCurvesByRenderOrder();

    // Check if the tree can be concatenated into a single merge node
    int blendingOperator;
    bool canConcatenate = isRotoPaintTreeConcatenatableInternal(items, &blendingOperator);
    NodePtr globalMerge;
    int globalMergeIndex = -1;
    NodesList mergeNodes;
    {
        QMutexLocker k(&_imp->rotoContextMutex);
        mergeNodes = _imp->globalMergeNodes;
    }

    // Ensure that all global merge nodes are disconnected
    for (NodesList::iterator it = mergeNodes.begin(); it != mergeNodes.end(); ++it) {
        int maxInputs = (*it)->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            (*it)->disconnectInput(i);
        }
    }
    globalMerge = getOrCreateGlobalMergeNode(blendingOperator, &globalMergeIndex);

    RotoPaintPtr rotoPaintEffect = toRotoPaint(getNode()->getEffectInstance());
    assert(rotoPaintEffect);

    if (canConcatenate) {
        NodePtr rotopaintNodeInput = rotoPaintEffect->getInternalInputNode(0);
        //Connect the rotopaint node input to the B input of the Merge
        if (rotopaintNodeInput) {
            globalMerge->connectInput(rotopaintNodeInput, 0);
        }
    } else {
        // Diconnect all inputs of the globalMerge
        int maxInputs = globalMerge->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            globalMerge->disconnectInput(i);
        }
    }

    // Refresh each item separately
    for (std::list<RotoDrawableItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
        (*it)->refreshNodesConnections(canConcatenate);

        if (canConcatenate) {

            // If we concatenate the tree, connect the global merge to the effect

            NodePtr effectNode = (*it)->getEffectNode();
            assert(effectNode);
            //qDebug() << "Connecting" << (*it)->getScriptName().c_str() << "to input" << globalMergeIndex <<
            //"(" << globalMerge->getInputLabel(globalMergeIndex).c_str() << ")" << "of" << globalMerge->getScriptName().c_str();
            globalMerge->connectInput(effectNode, globalMergeIndex);

            // Refresh for next node
            NodePtr nextMerge = getOrCreateGlobalMergeNode(blendingOperator, &globalMergeIndex);
            if (nextMerge != globalMerge) {
                assert( !nextMerge->getInput(0) );
                nextMerge->connectInput(globalMerge, 0);
                globalMerge = nextMerge;
            }
        }
    }

    // Default to noop node as bottom of the tree
    NodePtr premultNode = rotoPaintEffect->getPremultNode();
    NodePtr noOpNode = rotoPaintEffect->getMetadataFixerNode();
    NodePtr treeOutputNode = rotoPaintEffect->getOutputNode(false);
    if (!premultNode || !noOpNode || !treeOutputNode) {
        return;
    }

    if (canConcatenate) {
        connectRotoPaintBottomTreeToItems(rotoPaintEffect, noOpNode, premultNode, treeOutputNode, _imp->globalMergeNodes.front());
    } else {
        if (!items.empty()) {
            // Connect noop to the first item merge node
            connectRotoPaintBottomTreeToItems(rotoPaintEffect, noOpNode, premultNode, treeOutputNode, items.front()->getMergeNode());

        } else {
            NodePtr treeInputNode0 = rotoPaintEffect->getInternalInputNode(0);
            if (treeOutputNode->getInput(0) != treeInputNode0) {
                treeOutputNode->disconnectInput(0);
                treeOutputNode->connectInput(treeInputNode0, 0);
            }
        }
    }

    {
        // Make sure the premult node has its RGB checkbox checked
        premultNode->getEffectInstance()->beginChanges();
        KnobBoolPtr process[3];
        process[0] = toKnobBool(premultNode->getKnobByName(kNatronOfxParamProcessR));
        process[1] = toKnobBool(premultNode->getKnobByName(kNatronOfxParamProcessG));
        process[2] = toKnobBool(premultNode->getKnobByName(kNatronOfxParamProcessB));
        for (int i = 0; i < 3; ++i) {
            assert(process[i]);
            process[i]->setValue(true);
        }
        premultNode->getEffectInstance()->endChanges();

    }


} // RotoContext::refreshRotoPaintTree

void
RotoContext::onRotoPaintInputChanged(const NodePtr& node)
{
    if (node) {
    }
    refreshRotoPaintTree();
}

void
RotoContext::declareItemAsPythonField(const RotoItemPtr& item)
{
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    RotoStrokeItemPtr isStroke = toRotoStrokeItem( item );

    if (isStroke) {
        ///Strokes are unsupported in Python currently
        return;
    }
    RotoLayerPtr isLayer = toRotoLayer(item);
    std::string err;
    std::string script = (nodeFullName + ".roto." + item->getFullyQualifiedName() + " = " +
                          nodeFullName + ".roto.getItemByName(\"" + item->getScriptName() + "\")\n");
    if ( !appPTR->isBackground() ) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        getNode()->getApp()->appendToScriptEditor(err);
    }

    if (isLayer) {
        const RotoItems& items = isLayer->getItems();
        for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
            declareItemAsPythonField(*it);
        }
    }
}

void
RotoContext::declarePythonFields()
{
    for (std::list< RotoLayerPtr >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        declareItemAsPythonField(*it);
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_RotoContext.cpp"
