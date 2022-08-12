/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#include "RotoContext.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cassert>
#include <stdexcept>
#include <cstring> // for std::memcpy, std::memset
#include <sstream> // stringstream

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <QtCore/QLineF>
#include <QtCore/QDebug>

//#define ROTO_RENDER_TRIANGLES_ONLY

#include "libtess.h"

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
#include "Engine/ImageParams.h"
#include "Engine/MemoryInfo.h" // printAsRAM
#include "Engine/NodeSerialization.h"
#include "Engine/Interpolation.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContextSerialization.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewIdx.h"

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

//This will enable correct evaluation of beziers
//#define ROTO_USE_MESH_PATTERN_ONLY

// The number of pressure levels is 256 on an old Wacom Graphire 4, and 512 on an entry-level Wacom Bamboo
// 512 should be OK, see:
// http://www.davidrevoy.com/article182/calibrating-wacom-stylus-pressure-on-krita
#define ROTO_PRESSURE_LEVELS 512

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER

////////////////////////////////////RotoContext////////////////////////////////////


RotoContext::RotoContext(const NodePtr& node)
    : _imp( new RotoContextPrivate(node) )
{
    QObject::connect( _imp->lifeTime.lock()->getSignalSlotHandler().get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onLifeTimeKnobValueChanged(ViewSpec,int,int)) );
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct RotoContext::MakeSharedEnabler: public RotoContext
{
    MakeSharedEnabler(const NodePtr& node) : RotoContext(node) {
    }
};


RotoContextPtr
RotoContext::create(const NodePtr& node)
{
    return std::make_shared<RotoContext::MakeSharedEnabler>(node);
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

NodePtr
RotoContext::getRotoPaintBottomMergeNode() const
{
    std::list<RotoDrawableItemPtr> items = getCurvesByRenderOrder(false /*onlyActiveItems*/);

    if ( items.empty() ) {
        return NodePtr();
    }

    int bop;
    if ( isRotoPaintTreeConcatenatableInternal(items, &bop) ) {
        QMutexLocker k(&_imp->rotoContextMutex);
        if ( !_imp->globalMergeNodes.empty() ) {
            return _imp->globalMergeNodes.front();
        }
    }

    const RotoDrawableItemPtr& firstStrokeItem = items.back();
    assert(firstStrokeItem);
    NodePtr bottomMerge = firstStrokeItem->getMergeNode();
    assert(bottomMerge);

    return bottomMerge;
}

void
RotoContext::getRotoPaintTreeNodes(NodesList* nodes) const
{
    std::list<RotoDrawableItemPtr> items = getCurvesByRenderOrder(false);

    for (std::list<RotoDrawableItemPtr>::iterator it = items.begin(); it != items.end(); ++it) {
        NodePtr effectNode = (*it)->getEffectNode();
        NodePtr mergeNode = (*it)->getMergeNode();
        NodePtr timeOffsetNode = (*it)->getTimeOffsetNode();
        NodePtr frameHoldNode = (*it)->getFrameHoldNode();
        if (effectNode) {
            nodes->push_back(effectNode);
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
                for (std::list<RotoLayerPtr>::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
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
    std::list<RotoLayerPtr>::iterator it = std::find(_imp->layers.begin(), _imp->layers.end(), layer);

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
    RotoContextPtr this_shared = std::dynamic_pointer_cast<RotoContext>( shared_from_this() );
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
    RotoContextPtr this_shared = std::dynamic_pointer_cast<RotoContext>( shared_from_this() );
    assert(this_shared);
    std::string name = generateUniqueName(baseName);

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        ++_imp->age; // increase age

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
    RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(item);
    RotoItemPtr foundSelected;

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
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

        for (std::list<RotoLayerPtr>::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
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
        RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(item);
        if (isLayer) {
            std::list<RotoLayerPtr>::iterator foundLayer = std::find(_imp->layers.begin(), _imp->layers.end(), isLayer);
            if ( foundLayer == _imp->layers.end() ) {
                _imp->layers.push_back(isLayer);
            }
        }
        _imp->lastInsertedItem = item;
    }
    Q_EMIT itemInserted(indexInLayer, reason);
}

const std::list<RotoLayerPtr> &
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
    for (std::list<RotoLayerPtr>::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        const RotoItems & items = (*it)->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            BezierPtr b = std::dynamic_pointer_cast<Bezier>(*it2);
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
        for (std::list<RotoItemPtr>::iterator it2 = _imp->selectedItems.begin(); it2 != _imp->selectedItems.end(); ++it2) {
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
    _imp->activated.lock()->setSecret(lifetime_i != 4);
    KnobIntPtr frame = _imp->lifeTimeFrame.lock();
    frame->setSecret(lifetime_i == 4 || lifetime_i == 0);
    if (lifetime_i != 4) {
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
RotoContext::getItemsRegionOfDefinition(const std::list<RotoItemPtr>& items,
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
    bool isDrawing = false;
    getNode()->getApp()->getActiveRotoDrawingStroke(&activeRotoPaintNode, &activeStroke, &isDrawing);
    if (!isDrawing) {
        activeStroke.reset();
    }

    QMutexLocker l(&_imp->rotoContextMutex);
    for (double t = startTime; t <= endTime; t += mbFrameStep) {
        bool first = true;
        RectD bbox;
        for (std::list<RotoItemPtr>::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            Bezier* isBezier = dynamic_cast<Bezier*>( it2->get() );
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( it2->get() );
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
                    if ( isStroke == activeStroke.get() ) {
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

void
RotoContext::getMaskRegionOfDefinition(double time,
                                       ViewIdx view,
                                       RectD* rod) // rod is in canonical coordinates
const
{
    std::list<RotoItemPtr> allItems;
    {
        QMutexLocker l(&_imp->rotoContextMutex);

        for (std::list<RotoLayerPtr>::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
            RotoItems items = (*it)->getItems_mt_safe();
            for (RotoItems::iterator it2 = items.begin(); it2 != items.end(); ++it2) {
                allItems.push_back(*it2);
            }
        }
    }

    getItemsRegionOfDefinition(allItems, time, view, rod);
}

bool
RotoContext::isRotoPaintTreeConcatenatableInternal(const std::list<RotoDrawableItemPtr>& items,
                                                   int* blendingMode)
{
    bool operatorSet = false;
    int comp_i = -1;

    for (std::list<RotoDrawableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
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
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( it->get() );
        if (!isStroke) {
            assert( dynamic_cast<Bezier*>( it->get() ) );
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
    // Do not use only activated items when defining the shape of the RotoPaint tree otherwise we would have to adjust the tree at each frame.
    std::list<RotoDrawableItemPtr> items = getCurvesByRenderOrder(false /*onlyActivatedItems*/);
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
RotoContext::save(RotoContextSerialization* obj) const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    obj->_autoKeying = _imp->autoKeying;
    obj->_featherLink = _imp->featherLink;
    obj->_rippleEdit = _imp->rippleEdit;

    ///There must always be the base layer
    assert( !_imp->layers.empty() );

    ///Serializing this layer will recursively serialize everything
    _imp->layers.front()->save( dynamic_cast<RotoItemSerialization*>(&obj->_baseLayer) );

    ///the age of the context is not serialized as the images are wiped from the cache anyway

    ///Serialize the selection
    for (std::list<RotoItemPtr>::const_iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        obj->_selectedItems.push_back( (*it)->getScriptName() );
    }
}

static void
linkItemsKnobsRecursively(RotoContext* ctx,
                          const RotoLayerPtr & layer)
{
    const RotoItems & items = layer->getItems();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
        RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(*it);

        if (isBezier) {
            ctx->select(isBezier, RotoItem::eSelectionReasonOther);
        } else if (isLayer) {
            linkItemsKnobsRecursively(ctx, isLayer);
        }
    }
}

void
RotoContext::load(const RotoContextSerialization & obj)
{
    assert( QThread::currentThread() == qApp->thread() );
    ///no need to lock here, when this is called the main-thread is the only active thread

    _imp->isCurrentlyLoading = true;
    _imp->autoKeying = obj._autoKeying;
    _imp->featherLink = obj._featherLink;
    _imp->rippleEdit = obj._rippleEdit;

    for (std::list<KnobIWPtr>::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        it->lock()->setDefaultAllDimensionsEnabled(false);
    }

    assert(_imp->layers.size() == 1);

    RotoLayerPtr baseLayer = _imp->layers.front();

    baseLayer->load(obj._baseLayer);

    for (std::list<std::string>::const_iterator it = obj._selectedItems.begin(); it != obj._selectedItems.end(); ++it) {
        RotoItemPtr item = getItemByName(*it);
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(item);
        RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(item);
        if (isBezier) {
            select(isBezier, RotoItem::eSelectionReasonOther);
        } else if (isLayer) {
            linkItemsKnobsRecursively(this, isLayer);
        }
    }
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
RotoContext::selectFromLayer(const RotoItemPtr & b,
                    RotoItem::SelectionReasonEnum reason)
{
    selectInternal(b, false);
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::select(const std::list<RotoDrawableItemPtr> & beziers,
                    RotoItem::SelectionReasonEnum reason)
{
    for (std::list<RotoDrawableItemPtr>::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
        selectInternal(*it);
    }

    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::select(const std::list<RotoItemPtr> & items,
                    RotoItem::SelectionReasonEnum reason)
{
    for (std::list<RotoItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
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
RotoContext::deselect(const std::list<BezierPtr> & beziers,
                      RotoItem::SelectionReasonEnum reason)
{
    for (std::list<BezierPtr>::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
        deselectInternal(*it);
    }

    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const std::list<RotoItemPtr> & items,
                      RotoItem::SelectionReasonEnum reason)
{
    for (std::list<RotoItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
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
RotoContext::selectInternal(const RotoItemPtr & item, bool slaveKnobs)
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
        for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( it->get() );
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


    BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(item);
    RotoStrokeItemPtr isStroke = std::dynamic_pointer_cast<RotoStrokeItem>(item);
    RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>( item.get() );
    RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(item);

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

        const std::list<KnobIPtr>& drawableKnobs = isDrawable->getKnobs();
        for (std::list<KnobIPtr>::const_iterator it = drawableKnobs.begin(); it != drawableKnobs.end(); ++it) {
            for (std::list<KnobIWPtr>::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
                KnobIPtr thisKnob = it2->lock();
                if ( thisKnob->getName() == (*it)->getName() ) {
                    //Clone current state
                    thisKnob->cloneAndUpdateGui( it->get() );

                    //Slave internal knobs of the bezier
                    if (slaveKnobs) {
                        assert( (*it)->getDimension() == thisKnob->getDimension() );
                        for (int i = 0; i < (*it)->getDimension(); ++i) {
                            (*it)->slaveTo(i, thisKnob, i);
                        }
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
            selectInternal(*it, false);
        }
    }

    ///enable the knobs
    if ( (nbUnlockedBeziers > 0) || (nbUnlockedStrokes > 0) ) {
        KnobIPtr strengthKnob = _imp->brushEffectKnob.lock();
        KnobIPtr sourceTypeKnob = _imp->sourceTypeKnob.lock();
        KnobIPtr timeOffsetKnob = _imp->timeOffsetKnob.lock();
        KnobIPtr timeOffsetModeKnob = _imp->timeOffsetModeKnob.lock();
        for (std::list<KnobIWPtr>::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
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
                    for (std::list<KnobIWPtr>::iterator it2 = _imp->cloneKnobs.begin(); it2 != _imp->cloneKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isCloneKnob = true;
                        }
                    }
                    mustDisable |= isCloneKnob;
                }
                if (nbUnlockedBeziers && !mustDisable) {
                    bool isStrokeKnob = false;
                    for (std::list<KnobIWPtr>::iterator it2 = _imp->strokeKnobs.begin(); it2 != _imp->strokeKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isStrokeKnob = true;
                        }
                    }
                    mustDisable |= isStrokeKnob;
                }
                if (nbUnlockedStrokes && !mustDisable) {
                    bool isBezierKnob = false;
                    for (std::list<KnobIWPtr>::iterator it2 = _imp->shapeKnobs.begin(); it2 != _imp->shapeKnobs.end(); ++it2) {
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
        _imp->activated.lock()->setSecret(lifetime_i != 4);
        _imp->lifeTimeFrame.lock()->setSecret(lifetime_i == 4 || lifetime_i == 0);
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
        for (std::list<KnobIWPtr>::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            KnobIPtr k = it->lock();
            if ( k->getName() == knob->getName() ) {
                k->clone( knob.get() );
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
        std::list<RotoItemPtr>::iterator foundSelected = std::find(_imp->selectedItems.begin(), _imp->selectedItems.end(), b);

        ///if the item is not selected, exit
        if ( foundSelected == _imp->selectedItems.end() ) {
            return;
        }

        _imp->selectedItems.erase(foundSelected);

        for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( it->get() );
            if ( !isStroke && isBezier && !isBezier->isLockedRecursive() ) {
                ++nbBeziersUnLockedBezier;
            } else if (isStroke) {
                ++nbStrokesUnlocked;
            }
        }
    }
    bool bezierDirty = nbBeziersUnLockedBezier > 1;
    bool strokeDirty = nbStrokesUnlocked > 1;
    BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(b);
    RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>( b.get() );
    RotoStrokeItemPtr isStroke = std::dynamic_pointer_cast<RotoStrokeItem>(b);
    RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(b);
    if (isDrawable) {
        ///first-off set the context knobs to the value of this bezier

        const std::list<KnobIPtr>& drawableKnobs = isDrawable->getKnobs();
        for (std::list<KnobIPtr>::const_iterator it = drawableKnobs.begin(); it != drawableKnobs.end(); ++it) {
            for (std::list<KnobIWPtr>::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
                KnobIPtr knob = it2->lock();
                if ( knob->getName() == (*it)->getName() ) {
                    //Clone current state
                    knob->cloneAndUpdateGui( it->get() );

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
        for (std::list<KnobIWPtr>::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
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
        dynamic_cast<KnobI*>( centerKnob.get() )->removeAnimation(ViewSpec::all(), 0);
        dynamic_cast<KnobI*>( centerKnob.get() )->removeAnimation(ViewSpec::all(), 1);
        centerKnob->setValues( (bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2., ViewSpec::all(), eValueChangedReasonNatronInternalEdited );
        centerKnob->endChanges();
    }
    if (doClone) {
        KnobDoublePtr centerKnob = _imp->cloneCenterKnob.lock();
        centerKnob->beginChanges();
        dynamic_cast<KnobI*>( centerKnob.get() )->removeAnimation(ViewSpec::all(), 0);
        dynamic_cast<KnobI*>( centerKnob.get() )->removeAnimation(ViewSpec::all(), 1);
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
    std::list<KnobI*> knobs;

    knobs.push_back( translate.get() );
    knobs.push_back( scale.get() );
    knobs.push_back( center.get() );
    knobs.push_back( rotate.get() );
    knobs.push_back( skewX.get() );
    knobs.push_back( skewY.get() );
    knobs.push_back( scaleUniform.get() );
    knobs.push_back( skewOrder.get() );
    if (extraMatrix) {
        knobs.push_back( extraMatrix.get() );
    }
    bool wasEnabled = translate->isEnabled(0);
    for (std::list<KnobI*>::iterator it = knobs.begin(); it != knobs.end(); ++it) {
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
RotoContext::knobChanged(KnobI* k,
                         ValueChangedReasonEnum /*reason*/,
                         ViewSpec /*view*/,
                         double /*time*/,
                         bool /*originatedFromMainThread*/)
{
    bool ret = true;

    if ( k == _imp->resetCenterKnob.lock().get() ) {
        resetTransformCenter();
    } else if ( k == _imp->resetCloneCenterKnob.lock().get() ) {
        resetCloneTransformCenter();
    } else if ( k == _imp->resetTransformKnob.lock().get() ) {
        resetTransform();
    } else if ( k == _imp->resetCloneTransformKnob.lock().get() ) {
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
        RotoLayerPtr layer = std::dynamic_pointer_cast<RotoLayer>(*it2);
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it2);
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
    for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(*it);
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
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
    for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(*it);
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            isBezier->removeAnimation();
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer, time, false, true);
        }
    }
    if ( !_imp->selectedItems.empty() ) {
        evaluateChange();
    }
}

void
RotoContext::removeKeyframeOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(*it);
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
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
        RotoLayerPtr layer = std::dynamic_pointer_cast<RotoLayer>(*it);
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
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
        for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            RotoLayerPtr layer = std::dynamic_pointer_cast<RotoLayer>(*it);
            BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
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
        getNode()->getApp()->getTimeLine()->seekFrame(minimum, false,  NULL, eTimelineChangeReasonOtherSeek);
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
        for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(*it);
            BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
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
        getNode()->getApp()->getTimeLine()->seekFrame(maximum, false, NULL, eTimelineChangeReasonOtherSeek);
    }
}

static void
appendToSelectedCurvesRecursively(std::list<RotoDrawableItemPtr> * curves,
                                  const RotoLayerPtr& isLayer,
                                  double time,
                                  bool onlyActives,
                                  bool addStrokes)
{
    RotoItems items = isLayer->getItems_mt_safe();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        RotoLayerPtr layer = std::dynamic_pointer_cast<RotoLayer>(*it);
        RotoDrawableItemPtr isDrawable = std::dynamic_pointer_cast<RotoDrawableItem>(*it);
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( isDrawable.get() );
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

const std::list<RotoItemPtr> &
RotoContext::getSelectedItems() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->selectedItems;
}

std::list<RotoDrawableItemPtr>
RotoContext::getSelectedCurves() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    std::list<RotoDrawableItemPtr> drawables;
    double time = getTimelineCurrentTime();
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            assert(*it);
            RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(*it);
            RotoDrawableItemPtr isDrawable = std::dynamic_pointer_cast<RotoDrawableItem>(*it);
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

std::list<RotoDrawableItemPtr>
RotoContext::getCurvesByRenderOrder(bool onlyActivated) const
{
    std::list<RotoDrawableItemPtr> ret;
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
    std::list<RotoDrawableItemPtr> curves = getCurvesByRenderOrder();

    return (int)curves.size();
}

RotoLayerPtr
RotoContext::getLayerByName(const std::string & n) const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    for (std::list<RotoLayerPtr>::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
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
        *ret = std::dynamic_pointer_cast<RotoItem>(layer);
    } else {
        const RotoItems & items = layer->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(*it2);
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

    for (std::list<RotoLayerPtr>::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
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
RotoContext::incrementAge()
{
    _imp->incrementRotoAge();
}

void
RotoContext::evaluateChange()
{
    _imp->incrementRotoAge();
    getNode()->getEffectInstance()->incrHashAndEvaluate(true, false);
}

void
RotoContext::evaluateChange_noIncrement()
{
    //Used for the rotopaint to optimize the portion to render (render only the bbox of the last tick of the mouse move)
    std::list<ViewerInstance* > viewers;

    getNode()->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        (*it)->renderCurrentFrame(true);
    }
}

void
RotoContext::clearViewersLastRenderedStrokes()
{
    std::list<ViewerInstance* > viewers;

    getNode()->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        (*it)->clearLastRenderedImage();
    }
}

U64
RotoContext::getAge()
{
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->age;
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

        for (std::list<RotoItemPtr>::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
            if ( isBezier && !isBezier->isLockedRecursive() ) {
                ++nbBeziersUnLockedBezier;
            }
        }
    }
    bool dirty = nbBeziersUnLockedBezier > 1;
    bool enabled = nbBeziersUnLockedBezier > 0;

    for (std::list<KnobIWPtr>::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
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
RotoContext::onItemGloballyActivatedChanged(const RotoItemPtr& item)
{
    Q_EMIT itemGloballyActivatedChanged(item);
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

void
RotoContext::onItemKnobChanged()
{
    emitRefreshViewerOverlays();
    evaluateChange();
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
    std::list<RotoDrawableItemPtr> splines = getCurvesByRenderOrder();

    for (std::list<RotoDrawableItemPtr> ::iterator it = splines.begin(); it != splines.end(); ++it) {
        std::set<double> splineKeys;
        Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
        if (!isBezier) {
            continue;
        }
        isBezier->getKeyframeTimes(&splineKeys);
        for (std::set<double>::iterator it2 = splineKeys.begin(); it2 != splineKeys.end(); ++it2) {
            times->push_back(*it2);
        }
    }
}

static void
dequeueActionForLayer(RotoLayer* layer,
                      bool *evaluate)
{
    RotoItems items = layer->getItems_mt_safe();

    for (RotoItems::iterator it = items.begin(); it != items.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>( it->get() );
        RotoLayer* isLayer = dynamic_cast<RotoLayer*>( it->get() );
        if (isBezier) {
            *evaluate = *evaluate | isBezier->dequeueGuiActions();
        } else if (isLayer) {
            dequeueActionForLayer(isLayer, evaluate);
        }
    }
}

void
RotoContext::dequeueGuiActions()
{
    bool evaluate = false;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        if ( !_imp->layers.empty() ) {
            dequeueActionForLayer(_imp->layers.front().get(), &evaluate);
        }
    }

    if (evaluate) {
        evaluateChange();
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
    QMutexLocker k(&_imp->doingNeatRenderMutex);

    return _imp->doingNeatRender;
}

bool
RotoContext::mustDoNeatRender() const
{
    QMutexLocker k(&_imp->doingNeatRenderMutex);

    return _imp->mustDoNeatRender;
}

void
RotoContext::setIsDoingNeatRender(bool doing)
{
    QMutexLocker k(&_imp->doingNeatRenderMutex);

    if (doing && _imp->mustDoNeatRender) {
        assert(!_imp->doingNeatRender);
        _imp->doingNeatRender = true;
        _imp->mustDoNeatRender = false;
    } else if (_imp->doingNeatRender) {
        _imp->doingNeatRender = false;
        _imp->doingNeatRenderCond.wakeAll();
    }
}

void
RotoContext::evaluateNeatStrokeRender()
{
    {
        QMutexLocker k(&_imp->doingNeatRenderMutex);
        _imp->mustDoNeatRender = true;
    }
    evaluateChange();

    //EvaluateChange will call setIsDoingNeatRender(true);
    {
        QMutexLocker k(&_imp->doingNeatRenderMutex);
        while (_imp->doingNeatRender) {
            _imp->doingNeatRenderCond.wait(k.mutex());
        }
    }
}

CairoImageWrapper::~CairoImageWrapper()
{
    if (ctx) {
        cairo_destroy(ctx);
    }
    ////Free the buffer used by Cairo
    if (cairoImg) {
        cairo_surface_destroy(cairoImg);
    }
}

static void
adjustToPointToScale(unsigned int mipmapLevel,
                     double &x,
                     double &y)
{
    if (mipmapLevel != 0) {
        int pot = (1 << mipmapLevel);
        x /= pot;
        y /= pot;
    }
}

template <typename PIX, int maxValue, int dstNComps, int srcNComps, bool useOpacity, bool inverted>
static void
convertCairoImageToNatronImageForInverted_noColor(cairo_surface_t* cairoImg,
                                                  Image* image,
                                                  const RectI & pixelRod,
                                                  double shapeColor[3],
                                                  double opacity)
{
    unsigned char* cdata = cairo_image_surface_get_data(cairoImg);
    unsigned char* srcPix = cdata;
    int stride = cairo_image_surface_get_stride(cairoImg);
    Image::WriteAccess acc = image->getWriteRights();
    double r = useOpacity ? shapeColor[0] * opacity : shapeColor[0];
    double g = useOpacity ? shapeColor[1] * opacity : shapeColor[1];
    double b = useOpacity ? shapeColor[2] * opacity : shapeColor[2];
#ifdef DEBUG_NAN
    assert( !(boost::math::isnan)(r) ); // check for NaN
    assert( !(boost::math::isnan)(g) ); // check for NaN
    assert( !(boost::math::isnan)(b) ); // check for NaN
#endif
    int width = pixelRod.width();
    int srcNElements = width * srcNComps;

    for ( int y = 0; y < pixelRod.height(); ++y,
          srcPix += (stride - srcNElements) ) {
        PIX* dstPix = (PIX*)acc.pixelAt(pixelRod.x1, pixelRod.y1 + y);
        assert(dstPix);

        for (int x = 0; x < width; ++x,
             dstPix += dstNComps,
             srcPix += srcNComps) {
            float cairoPixel = !inverted ? ( (float)*srcPix * (maxValue / 255.f) ) : 1. - ( (float)*srcPix * (maxValue / 255.f) );
#         ifdef DEBUG_NAN
            assert( !(boost::math::isnan)(cairoPixel) ); // check for NaN
#         endif
            switch (dstNComps) {
            case 4:
                dstPix[0] = PIX(cairoPixel * r);
                dstPix[1] = PIX(cairoPixel * g);
                dstPix[2] = PIX(cairoPixel * b);
                dstPix[3] = useOpacity ? PIX(cairoPixel * opacity) : PIX(cairoPixel);
                break;
            case 1:
                dstPix[0] = useOpacity ? PIX(cairoPixel * opacity) : PIX(cairoPixel);
                break;
            case 3:
                dstPix[0] = PIX(cairoPixel * r);
                dstPix[1] = PIX(cairoPixel * g);
                dstPix[2] = PIX(cairoPixel * b);
                break;
            case 2:
                dstPix[0] = PIX(cairoPixel * r);
                dstPix[1] = PIX(cairoPixel * g);
                break;

            default:
                break;
            }
#         ifdef DEBUG_NAN
            for (int c = 0; c < dstNComps; ++c) {
                assert( !(boost::math::isnan)(dstPix[c]) ); // check for NaN
            }
#         endif
        }
    }
} // convertCairoImageToNatronImageForInverted_noColor

template <typename PIX, int maxValue, int dstNComps, int srcNComps, bool useOpacity>
static void
convertCairoImageToNatronImageForDstComponents_noColor(cairo_surface_t* cairoImg,
                                                       Image* image,
                                                       const RectI & pixelRod,
                                                       double shapeColor[3],
                                                       bool inverted,
                                                       double opacity)
{
    if (inverted) {
        convertCairoImageToNatronImageForInverted_noColor<PIX, maxValue, dstNComps, srcNComps, useOpacity, true>(cairoImg, image, pixelRod, shapeColor, opacity);
    } else {
        convertCairoImageToNatronImageForInverted_noColor<PIX, maxValue, dstNComps, srcNComps, useOpacity, false>(cairoImg, image, pixelRod, shapeColor, opacity);
    }
}

template <typename PIX, int maxValue, int dstNComps, int srcNComps>
static void
convertCairoImageToNatronImageForOpacity(cairo_surface_t* cairoImg,
                                         Image* image,
                                         const RectI & pixelRod,
                                         double shapeColor[3],
                                         double opacity,
                                         bool inverted,
                                         bool useOpacity)
{
    if (useOpacity) {
        convertCairoImageToNatronImageForDstComponents_noColor<PIX, maxValue, dstNComps, srcNComps, true>(cairoImg, image, pixelRod, shapeColor, inverted, opacity);
    } else {
        convertCairoImageToNatronImageForDstComponents_noColor<PIX, maxValue, dstNComps, srcNComps, false>(cairoImg, image, pixelRod, shapeColor, inverted, opacity);
    }
}

template <typename PIX, int maxValue, int dstNComps>
static void
convertCairoImageToNatronImageForSrcComponents_noColor(cairo_surface_t* cairoImg,
                                                       int srcNComps,
                                                       Image* image,
                                                       const RectI & pixelRod,
                                                       double shapeColor[3],
                                                       double opacity,
                                                       bool inverted,
                                                       bool useOpacity)
{
    if (srcNComps == 1) {
        convertCairoImageToNatronImageForOpacity<PIX, maxValue, dstNComps, 1>(cairoImg, image, pixelRod, shapeColor, opacity, inverted, useOpacity);
    } else if (srcNComps == 4) {
        convertCairoImageToNatronImageForOpacity<PIX, maxValue, dstNComps, 4>(cairoImg, image, pixelRod, shapeColor, opacity, inverted, useOpacity);
    } else {
        assert(false);
    }
}

template <typename PIX, int maxValue>
static void
convertCairoImageToNatronImage_noColor(cairo_surface_t* cairoImg,
                                       int srcNComps,
                                       Image* image,
                                       const RectI & pixelRod,
                                       double shapeColor[3],
                                       double opacity,
                                       bool inverted,
                                       bool useOpacity)
{
    int comps = (int)image->getComponentsCount();

    switch (comps) {
    case 1:
        convertCairoImageToNatronImageForSrcComponents_noColor<PIX, maxValue, 1>(cairoImg, srcNComps, image, pixelRod, shapeColor, opacity, inverted, useOpacity);
        break;
    case 2:
        convertCairoImageToNatronImageForSrcComponents_noColor<PIX, maxValue, 2>(cairoImg, srcNComps, image, pixelRod, shapeColor, opacity, inverted, useOpacity);
        break;
    case 3:
        convertCairoImageToNatronImageForSrcComponents_noColor<PIX, maxValue, 3>(cairoImg, srcNComps, image, pixelRod, shapeColor, opacity, inverted, useOpacity);
        break;
    case 4:
        convertCairoImageToNatronImageForSrcComponents_noColor<PIX, maxValue, 4>(cairoImg, srcNComps, image, pixelRod, shapeColor, opacity, inverted, useOpacity);
        break;
    default:
        break;
    }
}

#if 0
template <typename PIX, int maxValue, int srcNComps, int dstNComps>
static void
convertCairoImageToNatronImageForDstComponents(cairo_surface_t* cairoImg,
                                               Image* image,
                                               const RectI & pixelRod)
{
    unsigned char* cdata = cairo_image_surface_get_data(cairoImg);
    unsigned char* srcPix = cdata;
    int stride = cairo_image_surface_get_stride(cairoImg);
    int pixelSize = stride / pixelRod.width();
    Image::WriteAccess acc = image->getWriteRights();

    for (int y = 0; y < pixelRod.height(); ++y, srcPix += stride) {
        PIX* dstPix = (PIX*)acc.pixelAt(pixelRod.x1, pixelRod.y1 + y);
        assert(dstPix);

        for (int x = 0; x < pixelRod.width(); ++x) {
            switch (dstNComps) {
            case 4:
                assert(srcNComps == dstNComps);
                // cairo's format is ARGB (that is BGRA when interpreted as bytes)
                dstPix[x * dstNComps + 3] = PIX( (float)srcPix[x * pixelSize + 3] * (maxValue / 255.f) );
                dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] * (maxValue / 255.f) );
                dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] * (maxValue / 255.f) );
                dstPix[x * dstNComps + 2] = PIX( (float)srcPix[x * pixelSize + 0] * (maxValue / 255.f) );
                break;
            case 1:
                assert(srcNComps == dstNComps);
                dstPix[x] = PIX( (float)srcPix[x] * (1.f / 255) ) * maxValue;
                break;
            case 3:
                assert(srcNComps == dstNComps);
                dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] * (maxValue / 255.f) );
                dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] * (maxValue / 255.f) );
                dstPix[x * dstNComps + 2] = PIX( (float)srcPix[x * pixelSize + 0] * (maxValue / 255.f) );
                break;
            case 2:
                assert(srcNComps == 3);
                dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] * (maxValue / 255.f) );
                dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] * (maxValue / 255.f) );
                break;
            default:
                break;
            }
#         ifdef DEBUG_NAN
            for (int c = 0; c < dstNComps; ++c) {
                assert( !(boost::math::isnan)(dstPix[x * dstNComps + c]) ); // check for NaN
            }
#         endif
        }
    }
}

template <typename PIX, int maxValue, int srcNComps>
static void
convertCairoImageToNatronImageForSrcComponents(cairo_surface_t* cairoImg,
                                               Image* image,
                                               const RectI & pixelRod)
{
    int comps = (int)image->getComponentsCount();

    switch (comps) {
    case 1:
        convertCairoImageToNatronImageForDstComponents<PIX, maxValue, srcNComps, 1>(cairoImg, image, pixelRod);
        break;
    case 2:
        convertCairoImageToNatronImageForDstComponents<PIX, maxValue, srcNComps, 2>(cairoImg, image, pixelRod);
        break;
    case 3:
        convertCairoImageToNatronImageForDstComponents<PIX, maxValue, srcNComps, 3>(cairoImg, image, pixelRod);
        break;
    case 4:
        convertCairoImageToNatronImageForDstComponents<PIX, maxValue, srcNComps, 4>(cairoImg, image, pixelRod);
        break;
    default:
        break;
    }
}

template <typename PIX, int maxValue>
static void
convertCairoImageToNatronImage(cairo_surface_t* cairoImg,
                               Image* image,
                               const RectI & pixelRod,
                               int srcNComps)
{
    switch (srcNComps) {
    case 1:
        convertCairoImageToNatronImageForSrcComponents<PIX, maxValue, 1>(cairoImg, image, pixelRod);
        break;
    case 2:
        convertCairoImageToNatronImageForSrcComponents<PIX, maxValue, 2>(cairoImg, image, pixelRod);
        break;
    case 3:
        convertCairoImageToNatronImageForSrcComponents<PIX, maxValue, 3>(cairoImg, image, pixelRod);
        break;
    case 4:
        convertCairoImageToNatronImageForSrcComponents<PIX, maxValue, 4>(cairoImg, image, pixelRod);
        break;
    default:
        break;
    }
}

#endif // if 0


template <typename PIX, int maxValue, int srcNComps, int dstNComps>
static void
convertNatronImageToCairoImageForComponents(unsigned char* cairoImg,
                                            std::size_t stride,
                                            Image* image,
                                            const RectI& roi,
                                            const RectI& dstBounds,
                                            double shapeColor[3])
{
    unsigned char* dstPix = cairoImg;

    dstPix += ( (roi.y1 - dstBounds.y1) * stride + (roi.x1 - dstBounds.x1) );

    Image::ReadAccess acc = image->getReadRights();

    for (int y = 0; y < roi.height(); ++y, dstPix += stride) {
        const PIX* srcPix = (const PIX*)acc.pixelAt(roi.x1, roi.y1 + y);
        assert(srcPix);

        for (int x = 0; x < roi.width(); ++x) {
#         ifdef DEBUG_NAN
            for (int c = 0; c < srcNComps; ++c) {
                assert( !(boost::math::isnan)(srcPix[x * srcNComps + c]) ); // check for NaN
            }
#         endif
            if (dstNComps == 1) {
                dstPix[x] = (float)srcPix[x * srcNComps] * (255.f / maxValue);
            } else if (dstNComps == 4) {
                if (srcNComps == 4) {
                    //We are in the !buildUp case, do exactly the opposite that is done in convertNatronImageToCairoImageForComponents
                    dstPix[x * dstNComps + 0] = shapeColor[2] == 0 ? 0 : (float)(srcPix[x * srcNComps + 2] * (255.f / maxValue) ) / shapeColor[2];
                    dstPix[x * dstNComps + 1] = shapeColor[1] == 0 ? 0 : (float)(srcPix[x * srcNComps + 1] * (255.f / maxValue) ) / shapeColor[1];
                    dstPix[x * dstNComps + 2] = shapeColor[0] == 0 ? 0 : (float)(srcPix[x * srcNComps + 0] * (255.f / maxValue) ) / shapeColor[0];
                    dstPix[x * dstNComps + 3] = 255; //(float)srcPix[x * srcNComps + 3] * (255.f / maxValue);
                } else {
                    assert(srcNComps == 1);
                    float pix = (float)srcPix[x];
                    dstPix[x * dstNComps + 0] = pix * (255.f / maxValue);
                    dstPix[x * dstNComps + 1] = pix * (255.f / maxValue);
                    dstPix[x * dstNComps + 2] = pix * (255.f / maxValue);
                    dstPix[x * dstNComps + 3] = pix * (255.f / maxValue);
                }
            }
            // no need to check for NaN, dstPix is unsigned char
        }
    }
}

template <typename PIX, int maxValue, int srcComps>
static void
convertNatronImageToCairoImageForSrcComponents(unsigned char* cairoImg,
                                               int dstNComps,
                                               std::size_t stride,
                                               Image* image,
                                               const RectI& roi,
                                               const RectI& dstBounds,
                                               double shapeColor[3])
{
    if (dstNComps == 1) {
        convertNatronImageToCairoImageForComponents<PIX, maxValue, srcComps, 1>(cairoImg, stride, image, roi, dstBounds, shapeColor);
    } else if (dstNComps == 4) {
        convertNatronImageToCairoImageForComponents<PIX, maxValue, srcComps, 4>(cairoImg, stride, image, roi, dstBounds, shapeColor);
    } else {
        assert(false);
    }
}

template <typename PIX, int maxValue>
static void
convertNatronImageToCairoImage(unsigned char* cairoImg,
                               int dstNComps,
                               std::size_t stride,
                               Image* image,
                               const RectI& roi,
                               const RectI& dstBounds,
                               double shapeColor[3])
{
    int numComps = (int)image->getComponentsCount();

    switch (numComps) {
    case 1:
        convertNatronImageToCairoImageForSrcComponents<PIX, maxValue, 1>(cairoImg, dstNComps, stride, image, roi, dstBounds, shapeColor);
        break;
    case 2:
        convertNatronImageToCairoImageForSrcComponents<PIX, maxValue, 2>(cairoImg, dstNComps, stride, image, roi, dstBounds, shapeColor);
        break;
    case 3:
        convertNatronImageToCairoImageForSrcComponents<PIX, maxValue, 3>(cairoImg, dstNComps, stride, image, roi, dstBounds, shapeColor);
        break;
    case 4:
        convertNatronImageToCairoImageForSrcComponents<PIX, maxValue, 4>(cairoImg, dstNComps, stride, image, roi, dstBounds, shapeColor);
        break;
    default:
        break;
    }
}

double
RotoStrokeItem::renderSingleStroke(const RectD& pointsBbox,
                                   const std::list<std::pair<Point, double> >& points,
                                   unsigned int mipmapLevel,
                                   double par,
                                   const ImagePlaneDesc& components,
                                   ImageBitDepthEnum depth,
                                   double distToNext,
                                   ImagePtr *image)
{
    double time = getContext()->getTimelineCurrentTime();
    double shapeColor[3];

    getColor(time, shapeColor);

    ImagePtr source = *image;
    RectI pixelPointsBbox;
    pointsBbox.toPixelEnclosing(mipmapLevel, par, &pixelPointsBbox);


    NodePtr node = getContext()->getNode();
    ImageFieldingOrderEnum fielding = node->getEffectInstance()->getFieldingOrder();
    ImagePremultiplicationEnum premult = node->getEffectInstance()->getPremult();
    bool copyFromImage = false;
    bool mipMapLevelChanged = false;
    if (!source) {
        source.reset( new Image(components,
                                pointsBbox,
                                pixelPointsBbox,
                                mipmapLevel,
                                par,
                                depth,
                                premult,
                                fielding,
                                false) );
        *image = source;
    } else {
        if ( (*image)->getMipMapLevel() > mipmapLevel ) {
            mipMapLevelChanged = true;

            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds;
            otherRoD.toPixelEnclosing( (*image)->getMipMapLevel(), par, &oldBounds );
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            RectI mergeBounds;
            mergeRoD.toPixelEnclosing(mipmapLevel, par, &mergeBounds);


            //upscale the original image
            source.reset( new Image(components,
                                    mergeRoD,
                                    mergeBounds,
                                    mipmapLevel,
                                    par,
                                    depth,
                                    premult,
                                    fielding,
                                    false) );
            source->fillZero(pixelPointsBbox);
            (*image)->upscaleMipMap( oldBounds, (*image)->getMipMapLevel(), source->getMipMapLevel(), source.get() );
            *image = source;
        } else if ( (*image)->getMipMapLevel() < mipmapLevel ) {
            mipMapLevelChanged = true;

            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds;
            otherRoD.toPixelEnclosing( (*image)->getMipMapLevel(), par, &oldBounds );
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            RectI mergeBounds;
            mergeRoD.toPixelEnclosing(mipmapLevel, par, &mergeBounds);

            //downscale the original image
            source.reset( new Image(components,
                                    mergeRoD,
                                    mergeBounds,
                                    mipmapLevel,
                                    par,
                                    depth,
                                    premult,
                                    fielding,
                                    false) );
            source->fillZero(pixelPointsBbox);
            (*image)->downscaleMipMap( pointsBbox, oldBounds, (*image)->getMipMapLevel(), source->getMipMapLevel(), false, source.get() );
            *image = source;
        } else {
            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds = (*image)->getBounds();
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            source->setRoD(mergeRoD);
            source->ensureBounds(pixelPointsBbox, true);
        }
        copyFromImage = true;
    }

    bool doBuildUp = getBuildupKnob()->getValueAtTime(time);
    cairo_format_t cairoImgFormat;
    int srcNComps;
    //For the non build-up case, we use the LIGHTEN compositing operator, which only works on colors
    if ( !doBuildUp || (components.getNumComponents() > 1) ) {
        cairoImgFormat = CAIRO_FORMAT_ARGB32;
        srcNComps = 4;
    } else {
        cairoImgFormat = CAIRO_FORMAT_A8;
        srcNComps = 1;
    }


    ////Allocate the cairo temporary buffer
    CairoImageWrapper imgWrapper;
    double opacity = getOpacity(time);
    std::vector<unsigned char> buf;
    if (copyFromImage) {
        std::size_t stride = cairo_format_stride_for_width( cairoImgFormat, pixelPointsBbox.width() );
        std::size_t memSize = stride * pixelPointsBbox.height();
        buf.resize(memSize);
        std::memset(&buf.front(), 0, sizeof(unsigned char) * memSize);
        convertNatronImageToCairoImage<float, 1>(&buf.front(), srcNComps, stride, source.get(), pixelPointsBbox, pixelPointsBbox, shapeColor);
        imgWrapper.cairoImg = cairo_image_surface_create_for_data(&buf.front(), cairoImgFormat, pixelPointsBbox.width(), pixelPointsBbox.height(),
                                                                  stride);
    } else {
        imgWrapper.cairoImg = cairo_image_surface_create( cairoImgFormat, pixelPointsBbox.width(), pixelPointsBbox.height() );
        cairo_surface_set_device_offset(imgWrapper.cairoImg, -pixelPointsBbox.x1, -pixelPointsBbox.y1);
    }
    if (cairo_surface_status(imgWrapper.cairoImg) != CAIRO_STATUS_SUCCESS) {
        return 0;
    }
    cairo_surface_set_device_offset(imgWrapper.cairoImg, -pixelPointsBbox.x1, -pixelPointsBbox.y1);
    imgWrapper.ctx = cairo_create(imgWrapper.cairoImg);
    //cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD); // creates holes on self-overlapping shapes
    cairo_set_fill_rule(imgWrapper.ctx, CAIRO_FILL_RULE_WINDING);

    // these Roto shapes must be rendered WITHOUT antialias, or the junction between the inner
    // polygon and the feather zone will have artifacts. This is partly due to the fact that cairo
    // meshes are not antialiased.
    // Use a default feather distance of 1 pixel instead!
    // UPDATE: unfortunately, this produces less artifacts, but there are still some remaining (use opacity=0.5 to test)
    // maybe the inner polygon should be made of mesh patterns too?
    cairo_set_antialias(imgWrapper.ctx, CAIRO_ANTIALIAS_NONE);

    std::list<std::list<std::pair<Point, double> > > strokes;
    std::list<std::pair<Point, double> > toScalePoints;
    int pot = 1 << mipmapLevel;
    if (mipmapLevel == 0) {
        toScalePoints = points;
    } else {
        for (std::list<std::pair<Point, double> >::const_iterator it = points.begin(); it != points.end(); ++it) {
            std::pair<Point, double> p = *it;
            p.first.x /= pot;
            p.first.y /= pot;
            toScalePoints.push_back(p);
        }
    }
    strokes.push_back(toScalePoints);

    QMutexLocker k(&_imp->strokeDotPatternsMutex);
    std::vector<cairo_pattern_t*> dotPatterns = getPatternCache();
    if (mipMapLevelChanged) {
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            if (dotPatterns[i]) {
                cairo_pattern_destroy(dotPatterns[i]);
                dotPatterns[i] = 0;
            }
        }
        dotPatterns.clear();
    }
    if ( dotPatterns.empty() ) {
        dotPatterns.resize(ROTO_PRESSURE_LEVELS);
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            dotPatterns[i] = (cairo_pattern_t*)0;
        }
    }

    distToNext = RotoContextPrivate::renderStroke(imgWrapper.ctx, dotPatterns, strokes, distToNext, this, doBuildUp, opacity, time, mipmapLevel);

    updatePatternCache(dotPatterns);

    assert(cairo_surface_status(imgWrapper.cairoImg) == CAIRO_STATUS_SUCCESS);

    ///A call to cairo_surface_flush() is required before accessing the pixel data
    ///to ensure that all pending drawing operations are finished.
    cairo_surface_flush(imgWrapper.cairoImg);

    //Never use invert while drawing
    const bool inverted = false;
    convertCairoImageToNatronImage_noColor<float, 1>(imgWrapper.cairoImg, srcNComps, source.get(), pixelPointsBbox, shapeColor, 1., inverted, false);

    return distToNext;
} // RotoStrokeItem::renderSingleStroke

ImagePtr
RotoDrawableItem::renderMaskFromStroke(const ImagePlaneDesc& components,
                                       const double time,
                                       const ViewIdx view,
                                       const ImageBitDepthEnum depth,
                                       const unsigned int mipmapLevel,
                                       const RectD& rotoNodeSrcRod)
{
    NodePtr node = getContext()->getNode();
    ImagePtr image; // = stroke->getStrokeTimePreview();

    ///compute an enhanced hash different from the one of the merge node of the item in order to differentiate within the cache
    ///the output image of the node and the mask image.
    U64 rotoHash;
    {
        Hash64 hash;
        U64 mergeNodeHash = getMergeNode()->getEffectInstance()->getRenderHash();
        hash.append(mergeNodeHash);
        hash.computeHash();
        rotoHash = hash.value();
        assert(mergeNodeHash != rotoHash);
    }
    std::unique_ptr<ImageKey> key( new ImageKey(this,
                                                  rotoHash,
                                                  /*frameVaryingOrAnimated=*/ true,
                                                  time,
                                                  view,
                                                  /*pixelAspect=*/ 1.,
                                                  /*draftMode=*/ false,
                                                  /*fullScaleWithDownscaleInputs=*/ false) );

    {
        QReadLocker k(&_imp->cacheAccessMutex);
        node->getEffectInstance()->getImageFromCacheAndConvertIfNeeded(true, eStorageModeRAM, eStorageModeRAM, *key, mipmapLevel, NULL, NULL, RectI(), depth, components, EffectInstance::InputImagesMap(), RenderStatsPtr(), OSGLContextAttacherPtr(), &image);
    }

    if (image) {
        return image;
    }


    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    Bezier* isBezier = dynamic_cast<Bezier*>(this);
    double startTime = time, mbFrameStep = 1., endTime = time;
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    if (isBezier) {
        int mbType_i = getContext()->getMotionBlurTypeKnob()->getValue();
        bool applyPerShapeMotionBlur = mbType_i == 0;
        if (applyPerShapeMotionBlur) {
            isBezier->getMotionBlurSettings(time, &startTime, &endTime, &mbFrameStep);
        }
    }
#endif

#ifdef NATRON_ROTO_INVERTIBLE
    const bool inverted = isStroke->getInverted(time);
#else
    const bool inverted = false;
#endif

    RectD rotoBbox;
    std::list<std::list<std::pair<Point, double> > > strokes;

    if (isStroke) {
        isStroke->evaluateStroke(mipmapLevel, time, &strokes, &rotoBbox);
    } else if (isBezier) {
        bool bboxSet = false;
        for (double t = startTime; t <= endTime; t += mbFrameStep) {
            RectD subBbox = isBezier->getBoundingBox(t);
            if (!bboxSet) {
                rotoBbox = subBbox;
                bboxSet = true;
            } else {
                rotoBbox.merge(subBbox);
            }
        }
        if ( isBezier->isOpenBezier() ) {
            std::list<std::list<ParametricPoint> > decastelJauPolygon;
            isBezier->evaluateAtTime_DeCasteljau_autoNbPoints(false, time, mipmapLevel, &decastelJauPolygon, 0);
            std::list<std::pair<Point, double> > points;
            for (std::list<std::list<ParametricPoint> > ::iterator it = decastelJauPolygon.begin(); it != decastelJauPolygon.end(); ++it) {
                for (std::list<ParametricPoint>::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
                    Point p = {it2->x, it2->y};
                    points.push_back( std::make_pair(p, 1.) );
                }
            }
            if ( !points.empty() ) {
                strokes.push_back(points);
            }
        }
    }


    if (inverted) {
        // When inverted we at least need to have the RoD of the source of the Roto inverted
        rotoBbox.merge(rotoNodeSrcRod);
    }

    RectI pixelRod;
    rotoBbox.toPixelEnclosing(mipmapLevel, 1., &pixelRod);


    ImageParamsPtr params = Image::makeParams( rotoBbox,
                                                               pixelRod,
                                                               1., // par
                                                               mipmapLevel,
                                                               false,
                                                               components,
                                                               depth,
                                                               node->getEffectInstance()->getPremult(),
                                                               node->getEffectInstance()->getFieldingOrder() );
    /*
       At this point we take the cacheAccessMutex so that no other thread can retrieve this image from the cache while it has not been
       finished rendering. You might wonder why we do this differently here than in EffectInstance::renderRoI, this is because we do not use
       the trimap and notification system here in the rotocontext, which would be to much just for this small object, rather we just lock
       it once, which is fine.
     */
    QWriteLocker k(&_imp->cacheAccessMutex);
    appPTR->getImageOrCreate(*key, params, &image);
    if (!image) {
        std::stringstream ss;
        ss << "Failed to allocate an image of ";
        std::size_t size = params->getStorageInfo().bounds.area() * params->getStorageInfo().numComponents * params->getStorageInfo().dataTypeSize;
        ss << printAsRAM( size ).toStdString();
        Dialogs::errorDialog( tr("Out of memory").toStdString(), ss.str() );

        return image;
    }

    ///Does nothing if image is already alloc
    image->allocateMemory();


    image = renderMaskInternal(pixelRod, components, startTime, endTime, mbFrameStep, time, inverted, depth, mipmapLevel, strokes, image);

    return image;
} // RotoDrawableItem::renderMaskFromStroke

ImagePtr
RotoDrawableItem::renderMaskInternal(const RectI & roi,
                                     const ImagePlaneDesc& components,
                                     const double startTime,
                                     const double endTime,
                                     const double timeStep,
                                     const double time,
                                     const bool inverted,
                                     const ImageBitDepthEnum depth,
                                     const unsigned int mipmapLevel,
                                     const std::list<std::list<std::pair<Point, double> > >& strokes,
                                     const ImagePtr &image)
{
    Q_UNUSED(startTime);
    Q_UNUSED(endTime);
    Q_UNUSED(timeStep);
    NodePtr node = getContext()->getNode();
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    Bezier* isBezier = dynamic_cast<Bezier*>(this);
    cairo_format_t cairoImgFormat;
    int srcNComps;
    bool doBuildUp = true;

    if (isStroke) {
        //Motion-blur is not supported for strokes
        assert(startTime == endTime);

        doBuildUp = getBuildupKnob()->getValueAtTime(time);
        //For the non build-up case, we use the LIGHTEN compositing operator, which only works on colors
        if ( !doBuildUp || (components.getNumComponents() > 1) ) {
            cairoImgFormat = CAIRO_FORMAT_ARGB32;
            srcNComps = 4;
        } else {
            cairoImgFormat = CAIRO_FORMAT_A8;
            srcNComps = 1;
        }
    } else {
        cairoImgFormat = CAIRO_FORMAT_A8;
        srcNComps = 1;
    }


    double shapeColor[3];
    getColor(time, shapeColor);

    double opacity = getOpacity(time);

    ////Allocate the cairo temporary buffer
    CairoImageWrapper imgWrapper;

    imgWrapper.cairoImg = cairo_image_surface_create( cairoImgFormat, roi.width(), roi.height() );
    cairo_surface_set_device_offset(imgWrapper.cairoImg, -roi.x1, -roi.y1);
    if (cairo_surface_status(imgWrapper.cairoImg) != CAIRO_STATUS_SUCCESS) {
        return image;
    }
    imgWrapper.ctx = cairo_create(imgWrapper.cairoImg);
    //cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD); // creates holes on self-overlapping shapes
    cairo_set_fill_rule(imgWrapper.ctx, CAIRO_FILL_RULE_WINDING);

    // these Roto shapes must be rendered WITHOUT antialias, or the junction between the inner
    // polygon and the feather zone will have artifacts. This is partly due to the fact that cairo
    // meshes are not antialiased.
    // Use a default feather distance of 1 pixel instead!
    // UPDATE: unfortunately, this produces less artifacts, but there are still some remaining (use opacity=0.5 to test)
    // maybe the inner polygon should be made of mesh patterns too?
    cairo_set_antialias(imgWrapper.ctx, CAIRO_ANTIALIAS_NONE);


    assert(isStroke || isBezier);
    if ( isStroke || !isBezier || ( isBezier && isBezier->isOpenBezier() ) ) {
        std::vector<cairo_pattern_t*> dotPatterns(ROTO_PRESSURE_LEVELS);
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            dotPatterns[i] = (cairo_pattern_t*)0;
        }
        RotoContextPrivate::renderStroke(imgWrapper.ctx, dotPatterns, strokes, 0, this, doBuildUp, opacity, time, mipmapLevel);

        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            if (dotPatterns[i]) {
                cairo_pattern_destroy(dotPatterns[i]);
                dotPatterns[i] = 0;
            }
        }
    } else {
        RotoContextPrivate::renderBezier(imgWrapper.ctx, isBezier, opacity, time, startTime, endTime, timeStep, mipmapLevel);
    }

    bool useOpacityToConvert = (isBezier != 0);


    switch (depth) {
    case eImageBitDepthFloat:
        convertCairoImageToNatronImage_noColor<float, 1>(imgWrapper.cairoImg, srcNComps, image.get(), roi, shapeColor, opacity, inverted, useOpacityToConvert);
        break;
    case eImageBitDepthByte:
        convertCairoImageToNatronImage_noColor<unsigned char, 255>(imgWrapper.cairoImg, srcNComps,  image.get(), roi, shapeColor, opacity, inverted,  useOpacityToConvert);
        break;
    case eImageBitDepthShort:
        convertCairoImageToNatronImage_noColor<unsigned short, 65535>(imgWrapper.cairoImg, srcNComps, image.get(), roi, shapeColor, opacity, inverted, useOpacityToConvert);
        break;
    case eImageBitDepthHalf:
    case eImageBitDepthNone:
        assert(false);
        break;
    }


    assert(cairo_surface_status(imgWrapper.cairoImg) == CAIRO_STATUS_SUCCESS);

    ///A call to cairo_surface_flush() is required before accessing the pixel data
    ///to ensure that all pending drawing operations are finished.
    cairo_surface_flush(imgWrapper.cairoImg);

    return image;
} // RotoDrawableItem::renderMaskInternal

static inline
double
hardnessGaussLookup(double f)
{
    //2 hyperbolas + 1 parabola to approximate a gauss function
    if (f < -0.5) {
        f = -1. - f;

        return (2. * f * f);
    }

    if (f < 0.5) {
        return (1. - 2. * f * f);
    }
    f = 1. - f;

    return (2. * f * f);
}

void
RotoContextPrivate::renderDot(cairo_t* cr,
                              std::vector<cairo_pattern_t*>* dotPatterns,
                              const Point &center,
                              double internalDotRadius,
                              double externalDotRadius,
                              double pressure,
                              bool doBuildUp,
                              const std::vector<std::pair<double, double> >& opacityStops,
                              double opacity)
{
    if ( !opacityStops.empty() ) {
        cairo_pattern_t* pattern;
        // sometimes, Qt gives a pressure level > 1... so we clamp it
        int pressureInt = int(std::max( 0., std::min(pressure, 1.) ) * (ROTO_PRESSURE_LEVELS - 1) + 0.5);
        assert(pressureInt >= 0 && pressureInt < ROTO_PRESSURE_LEVELS);
        if (dotPatterns && (*dotPatterns)[pressureInt]) {
            pattern = (*dotPatterns)[pressureInt];
        } else {
            pattern = cairo_pattern_create_radial(0, 0, internalDotRadius, 0, 0, externalDotRadius);
            for (std::size_t i = 0; i < opacityStops.size(); ++i) {
                if (doBuildUp) {
                    cairo_pattern_add_color_stop_rgba(pattern, opacityStops[i].first, 1., 1., 1., opacityStops[i].second);
                } else {
                    cairo_pattern_add_color_stop_rgba(pattern, opacityStops[i].first, opacityStops[i].second, opacityStops[i].second, opacityStops[i].second, 1);
                }
            }
            //dotPatterns[pressureInt] = pattern;
        }
        cairo_translate(cr, center.x, center.y);
        cairo_set_source(cr, pattern);
        cairo_translate(cr, -center.x, -center.y);
    } else {
        if (doBuildUp) {
            cairo_set_source_rgba(cr, 1., 1., 1., opacity);
        } else {
            cairo_set_source_rgba(cr, opacity, opacity, opacity, 1.);
        }
    }
#ifdef DEBUG
    //Make sure the dot we are about to render falls inside the clip region, otherwise the bounds of the image are mis-calculated.
    cairo_surface_t* target = cairo_get_target(cr);
    int w = cairo_image_surface_get_width(target);
    int h = cairo_image_surface_get_height(target);
    double x1, y1;
    cairo_surface_get_device_offset(target, &x1, &y1);
    assert(std::floor(center.x - externalDotRadius) >= -x1 && std::floor(center.x + externalDotRadius) < -x1 + w &&
           std::floor(center.y - externalDotRadius) >= -y1 && std::floor(center.y + externalDotRadius) < -y1 + h);
#endif
    cairo_arc(cr, center.x, center.y, externalDotRadius, 0, M_PI * 2);
    cairo_fill(cr);
}

static void
getRenderDotParams(double alpha,
                   double brushSizePixel,
                   double brushHardness,
                   double brushSpacing,
                   double pressure,
                   bool pressureAffectsOpacity,
                   bool pressureAffectsSize,
                   bool pressureAffectsHardness,
                   double* internalDotRadius,
                   double* externalDotRadius,
                   double * spacing,
                   std::vector<std::pair<double, double> >* opacityStops)
{
    if (pressureAffectsSize) {
        brushSizePixel *= pressure;
    }
    if (pressureAffectsHardness) {
        brushHardness *= pressure;
    }
    if (pressureAffectsOpacity) {
        alpha *= pressure;
    }

    *internalDotRadius = std::max(brushSizePixel * brushHardness, 1.) / 2.;
    *externalDotRadius = std::max(brushSizePixel, 1.) / 2.;
    *spacing = *externalDotRadius * 2. * brushSpacing;


    opacityStops->clear();

    double exp = brushHardness != 1.0 ?  0.4 / (1.0 - brushHardness) : 0.;
    const int maxStops = 8;
    double incr = 1. / maxStops;

    if (brushHardness != 1.) {
        for (double d = 0; d <= 1.; d += incr) {
            double o = hardnessGaussLookup( std::pow(d, exp) );
            opacityStops->push_back( std::make_pair(d, o * alpha) );
        }
    }
}

double
RotoContextPrivate::renderStroke(cairo_t* cr,
                                 std::vector<cairo_pattern_t*>& dotPatterns,
                                 const std::list<std::list<std::pair<Point, double> > >& strokes,
                                 double distToNext,
                                 const RotoDrawableItem* stroke,
                                 bool doBuildup,
                                 double alpha,
                                 double time,
                                 unsigned int mipmapLevel)
{
    if ( strokes.empty() ) {
        return distToNext;
    }

    if ( !stroke || !stroke->isActivated(time) ) {
        return distToNext;
    }

    assert(dotPatterns.size() == ROTO_PRESSURE_LEVELS);

    KnobDoublePtr brushSizeKnob = stroke->getBrushSizeKnob();
    double brushSize = brushSizeKnob->getValueAtTime(time);
    KnobDoublePtr brushSpacingKnob = stroke->getBrushSpacingKnob();
    double brushSpacing = brushSpacingKnob->getValueAtTime(time);
    if (brushSpacing == 0.) {
        return distToNext;
    }


    brushSpacing = std::max(brushSpacing, 0.05);

    KnobDoublePtr brushHardnessKnob = stroke->getBrushHardnessKnob();
    double brushHardness = brushHardnessKnob->getValueAtTime(time);
    KnobDoublePtr visiblePortionKnob = stroke->getBrushVisiblePortionKnob();
    double writeOnStart = visiblePortionKnob->getValueAtTime(time, 0);
    double writeOnEnd = visiblePortionKnob->getValueAtTime(time, 1);
    if ( (writeOnEnd - writeOnStart) <= 0. ) {
        return distToNext;
    }

    KnobBoolPtr pressureOpacityKnob = stroke->getPressureOpacityKnob();
    KnobBoolPtr pressureSizeKnob = stroke->getPressureSizeKnob();
    KnobBoolPtr pressureHardnessKnob = stroke->getPressureHardnessKnob();
    bool pressureAffectsOpacity = pressureOpacityKnob->getValueAtTime(time);
    bool pressureAffectsSize = pressureSizeKnob->getValueAtTime(time);
    bool pressureAffectsHardness = pressureHardnessKnob->getValueAtTime(time);
    double brushSizePixel = brushSize;
    if (mipmapLevel != 0) {
        brushSizePixel = std::max( 1., brushSizePixel / (1 << mipmapLevel) );
    }
    cairo_set_operator(cr, doBuildup ? CAIRO_OPERATOR_OVER : CAIRO_OPERATOR_LIGHTEN);


    for (std::list<std::list<std::pair<Point, double> > >::const_iterator strokeIt = strokes.begin(); strokeIt != strokes.end(); ++strokeIt) {
        int firstPoint = (int)std::floor( (strokeIt->size() * writeOnStart) );
        int endPoint = (int)std::ceil( (strokeIt->size() * writeOnEnd) );
        assert( firstPoint >= 0 && firstPoint < (int)strokeIt->size() && endPoint > firstPoint && endPoint <= (int)strokeIt->size() );


        ///The visible portion of the paint's stroke with points adjusted to pixel coordinates
        std::list<std::pair<Point, double> > visiblePortion;
        std::list<std::pair<Point, double> >::const_iterator startingIt = strokeIt->begin();
        std::list<std::pair<Point, double> >::const_iterator endingIt = strokeIt->begin();
        std::advance(startingIt, firstPoint);
        std::advance(endingIt, endPoint);
        for (std::list<std::pair<Point, double> >::const_iterator it = startingIt; it != endingIt; ++it) {
            visiblePortion.push_back(*it);
        }
        if ( visiblePortion.empty() ) {
            return distToNext;
        }

        std::list<std::pair<Point, double> >::iterator it = visiblePortion.begin();

        if (visiblePortion.size() == 1) {
            double internalDotRadius, externalDotRadius, spacing;
            std::vector<std::pair<double, double> > opacityStops;
            getRenderDotParams(alpha, brushSizePixel, brushHardness, brushSpacing, it->second, pressureAffectsOpacity, pressureAffectsSize, pressureAffectsHardness, &internalDotRadius, &externalDotRadius, &spacing, &opacityStops);
            renderDot(cr, &dotPatterns, it->first, internalDotRadius, externalDotRadius, it->second, doBuildup, opacityStops, alpha);
            continue;
        }

        std::list<std::pair<Point, double> >::iterator next = it;
        ++next;

        while ( next != visiblePortion.end() ) {
            //Render for each point a dot. Spacing is a percentage of brushSize:
            //Spacing at 1 means no dot is overlapping another (so the spacing is in fact brushSize)
            //Spacing at 0 we do not render the stroke

            double dist = std::sqrt( (next->first.x - it->first.x) * (next->first.x - it->first.x) +  (next->first.y - it->first.y) * (next->first.y - it->first.y) );

            // while the next point can be drawn on this segment, draw a point and advance
            while (distToNext <= dist) {
                double a = dist == 0. ? 0. : distToNext / dist;
                Point center = {
                    it->first.x * (1 - a) + next->first.x * a,
                    it->first.y * (1 - a) + next->first.y * a
                };
                double pressure = it->second * (1 - a) + next->second * a;

                // draw the dot
                double internalDotRadius, externalDotRadius, spacing;
                std::vector<std::pair<double, double> > opacityStops;
                getRenderDotParams(alpha, brushSizePixel, brushHardness, brushSpacing, pressure, pressureAffectsOpacity, pressureAffectsSize, pressureAffectsHardness, &internalDotRadius, &externalDotRadius, &spacing, &opacityStops);
                renderDot(cr, &dotPatterns, center, internalDotRadius, externalDotRadius, pressure, doBuildup, opacityStops, alpha);

                distToNext += spacing;
            }

            // go to the next segment
            distToNext -= dist;
            ++next;
            ++it;
        }
    }


    return distToNext;
} // RotoContextPrivate::renderStroke

bool
RotoContext::allocateAndRenderSingleDotStroke(int brushSizePixel,
                                              double brushHardness,
                                              double alpha,
                                              CairoImageWrapper& wrapper)
{
    wrapper.cairoImg = cairo_image_surface_create(CAIRO_FORMAT_A8, brushSizePixel + 1, brushSizePixel + 1);
    cairo_surface_set_device_offset(wrapper.cairoImg, 0, 0);
    if (cairo_surface_status(wrapper.cairoImg) != CAIRO_STATUS_SUCCESS) {
        return false;
    }
    wrapper.ctx = cairo_create(wrapper.cairoImg);
    //cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD); // creates holes on self-overlapping shapes
    cairo_set_fill_rule(wrapper.ctx, CAIRO_FILL_RULE_WINDING);

    // these Roto shapes must be rendered WITHOUT antialias, or the junction between the inner
    // polygon and the feather zone will have artifacts. This is partly due to the fact that cairo
    // meshes are not antialiased.
    // Use a default feather distance of 1 pixel instead!
    // UPDATE: unfortunately, this produces less artifacts, but there are still some remaining (use opacity=0.5 to test)
    // maybe the inner polygon should be made of mesh patterns too?
    cairo_set_antialias(wrapper.ctx, CAIRO_ANTIALIAS_NONE);

    cairo_set_operator(wrapper.ctx, CAIRO_OPERATOR_OVER);

    double internalDotRadius, externalDotRadius, spacing;
    std::vector<std::pair<double, double> > opacityStops;
    Point p;
    p.x = brushSizePixel / 2.;
    p.y = brushSizePixel / 2.;

    const double pressure = 1.;
    const double brushspacing = 0.;

    getRenderDotParams(alpha, brushSizePixel, brushHardness, brushspacing, pressure, false, false, false, &internalDotRadius, &externalDotRadius, &spacing, &opacityStops);
    RotoContextPrivate::renderDot(wrapper.ctx, 0, p, internalDotRadius, externalDotRadius, pressure, true, opacityStops, alpha);

    return true;
}

void
RotoContextPrivate::renderBezier(cairo_t* cr,
                                 const Bezier* bezier,
                                 double opacity,
                                 double time,
                                 double startTime, double endTime, double mbFrameStep,
                                 unsigned int mipmapLevel)
{
    ///render the bezier only if finished (closed) and activated
    if ( !bezier->isCurveFinished() || !bezier->isActivated(time) || ( bezier->getControlPointsCount() <= 1 ) ) {
        return;
    }



    for (double t = startTime; t <= endTime; t+=mbFrameStep) {

        double fallOff = bezier->getFeatherFallOff(t);
        double featherDist = bezier->getFeatherDistance(t);
        double shapeColor[3];
        bezier->getColor(t, shapeColor);


        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

        cairo_new_path(cr);

        ////Define the feather edge pattern
        cairo_pattern_t* mesh = cairo_pattern_create_mesh();
        if (cairo_pattern_status(mesh) != CAIRO_STATUS_SUCCESS) {
            cairo_pattern_destroy(mesh);

            return;
        }

        ///Adjust the feather distance so it takes the mipmap level into account
        if (mipmapLevel != 0) {
            featherDist /= (1 << mipmapLevel);
        }

        Transform::Matrix3x3 transform;
        bezier->getTransformAtTime(t, &transform);


#ifdef ROTO_RENDER_TRIANGLES_ONLY
        std::list<RotoFeatherVertex> featherMesh;
        std::list<RotoTriangleFans> internalFans;
        std::list<RotoTriangles> internalTriangles;
        std::list<RotoTriangleStrips> internalStrips;
        computeTriangles(bezier, t, mipmapLevel, featherDist, &featherMesh, &internalFans, &internalTriangles, &internalStrips);
        renderFeather_cairo(featherMesh, shapeColor, fallOff, mesh);
        renderInternalShape_cairo(internalTriangles, internalFans, internalStrips, shapeColor, mesh);
        Q_UNUSED(opacity);
#else
        renderFeather(bezier, t, mipmapLevel, shapeColor, opacity, featherDist, fallOff, mesh);


        // strangely, the above-mentioned cairo bug doesn't affect this function
        BezierCPs cps = bezier->getControlPoints_mt_safe();
        renderInternalShape(t, mipmapLevel, shapeColor, opacity, transform, cr, mesh, cps);
        
#endif
        
        
        RotoContextPrivate::applyAndDestroyMask(cr, mesh);
    }
} // RotoContextPrivate::renderBezier

void
RotoContextPrivate::renderFeather(const Bezier* bezier,
                                  double time,
                                  unsigned int mipmapLevel,
                                  double shapeColor[3],
                                  double /*opacity*/,
                                  double featherDist,
                                  double fallOff,
                                  cairo_pattern_t* mesh)
{
    ///Note that we do not use the opacity when rendering the bezier, it is rendered with correct floating point opacity/color when converting
    ///to the Natron image.

    double fallOffInverse = 1. / fallOff;
    /*
     * We descretize the feather control points to obtain a polygon so that the feather distance will be of the same thickness around all the shape.
     * If we were to extend only the end points, the resulting bezier interpolation would create a feather with different thickness around the shape,
     * yielding an unwanted behaviour for the end user.
     */
    ///here is the polygon of the feather bezier
    ///This is used only if the feather distance is different of 0 and the feather points equal
    ///the control points in order to still be able to apply the feather distance.
    std::list<ParametricPoint> featherPolygon;
    std::list<ParametricPoint> bezierPolygon;
    RectD featherPolyBBox;

    featherPolyBBox.setupInfinity();

    bezier->evaluateFeatherPointsAtTime_DeCasteljau(false, time, mipmapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                    50,
#else
                                                    1,
#endif
                                                    true, &featherPolygon, &featherPolyBBox);
    bezier->evaluateAtTime_DeCasteljau(false, time, mipmapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                       50,
#else
                                       1,
#endif
                                       &bezierPolygon, NULL);

    bool clockWise = bezier->isFeatherPolygonClockwiseOriented(false, time);

    assert( !featherPolygon.empty() && !bezierPolygon.empty() );


    std::list<Point> featherContour;

    // prepare iterators
    std::list<ParametricPoint>::iterator next = featherPolygon.begin();
    ++next;  // can only be valid since we assert the list is not empty
    if ( next == featherPolygon.end() ) {
        next = featherPolygon.begin();
    }
    std::list<ParametricPoint>::iterator prev = featherPolygon.end();
    --prev; // can only be valid since we assert the list is not empty
    std::list<ParametricPoint>::iterator bezIT = bezierPolygon.begin();
    std::list<ParametricPoint>::iterator prevBez = bezierPolygon.end();
    --prevBez; // can only be valid since we assert the list is not empty

    // prepare p1
    double absFeatherDist = std::abs(featherDist);
    Point p1;
    p1.x = featherPolygon.begin()->x;
    p1.y = featherPolygon.begin()->y;
    double norm = sqrt( (next->x - prev->x) * (next->x - prev->x) + (next->y - prev->y) * (next->y - prev->y) );
    assert(norm != 0);
    double dx = (norm != 0) ? -( (next->y - prev->y) / norm ) : 0;
    double dy = (norm != 0) ? ( (next->x - prev->x) / norm ) : 1;

    if (!clockWise) {
        p1.x -= dx * absFeatherDist;
        p1.y -= dy * absFeatherDist;
    } else {
        p1.x += dx * absFeatherDist;
        p1.y += dy * absFeatherDist;
    }


    double innerOpacity = 1.;
    double outterOpacity = 0.;

    Point origin = p1;
    featherContour.push_back(p1);


    // increment for first iteration
    std::list<ParametricPoint>::iterator cur = featherPolygon.begin();
    // ++cur, ++prev, ++next, ++bezIT, ++prevBez
    // all should be valid, actually
    assert( cur != featherPolygon.end() &&
            prev != featherPolygon.end() &&
            next != featherPolygon.end() &&
            bezIT != bezierPolygon.end() &&
            prevBez != bezierPolygon.end() );
    if ( cur != featherPolygon.end() ) {
        ++cur;
    }
    if ( prev != featherPolygon.end() ) {
        ++prev;
    }
    if ( next != featherPolygon.end() ) {
        ++next;
    }
    if ( bezIT != bezierPolygon.end() ) {
        ++bezIT;
    }
    if ( prevBez != bezierPolygon.end() ) {
        ++prevBez;
    }

    for (;; ++cur) { // for each point in polygon
        if ( next == featherPolygon.end() ) {
            next = featherPolygon.begin();
        }
        if ( prev == featherPolygon.end() ) {
            prev = featherPolygon.begin();
        }
        if ( bezIT == bezierPolygon.end() ) {
            bezIT = bezierPolygon.begin();
        }
        if ( prevBez == bezierPolygon.end() ) {
            prevBez = bezierPolygon.begin();
        }
        bool mustStop = false;
        if ( cur == featherPolygon.end() ) {
            mustStop = true;
            cur = featherPolygon.begin();
        }

        ///skip it
        /*if ( (cur->x == prev->x) && (cur->y == prev->y) ) {
            continue;
        }*/

        Point p0, p0p1, p1p0, p2, p2p3, p3p2, p3;
        p0.x = prevBez->x;
        p0.y = prevBez->y;
        p3.x = bezIT->x;
        p3.y = bezIT->y;

        if (!mustStop) {
            norm = sqrt( (next->x - prev->x) * (next->x - prev->x) + (next->y - prev->y) * (next->y - prev->y) );
            if (norm == 0) {
                dx = 0;
                dy = 0;
            } else {
                dx = -( (next->y - prev->y) / norm );
                dy = ( (next->x - prev->x) / norm );
            }
            p2.x = cur->x;
            p2.y = cur->y;

            if (!clockWise) {
                p2.x -= dx * absFeatherDist;
                p2.y -= dy * absFeatherDist;
            } else {
                p2.x += dx * absFeatherDist;
                p2.y += dy * absFeatherDist;
            }
        } else {
            p2.x = origin.x;
            p2.y = origin.y;
        }
        featherContour.push_back(p2);

        ///linear interpolation
        p0p1.x = (p0.x * fallOff * 2. + fallOffInverse * p1.x) / (fallOff * 2. + fallOffInverse);
        p0p1.y = (p0.y * fallOff * 2. + fallOffInverse * p1.y) / (fallOff * 2. + fallOffInverse);
        p1p0.x = (p0.x * fallOff + 2. * fallOffInverse * p1.x) / (fallOff + 2. * fallOffInverse);
        p1p0.y = (p0.y * fallOff + 2. * fallOffInverse * p1.y) / (fallOff + 2. * fallOffInverse);


        p2p3.x = (p3.x * fallOff + 2. * fallOffInverse * p2.x) / (fallOff + 2. * fallOffInverse);
        p2p3.y = (p3.y * fallOff + 2. * fallOffInverse * p2.y) / (fallOff + 2. * fallOffInverse);
        p3p2.x = (p3.x * fallOff * 2. + fallOffInverse * p2.x) / (fallOff * 2. + fallOffInverse);
        p3p2.y = (p3.y * fallOff * 2. + fallOffInverse * p2.y) / (fallOff * 2. + fallOffInverse);


        ///move to the initial point
        cairo_mesh_pattern_begin_patch(mesh);
        cairo_mesh_pattern_move_to(mesh, p0.x, p0.y);
        cairo_mesh_pattern_curve_to(mesh, p0p1.x, p0p1.y, p1p0.x, p1p0.y, p1.x, p1.y);
        cairo_mesh_pattern_line_to(mesh, p2.x, p2.y);
        cairo_mesh_pattern_curve_to(mesh, p2p3.x, p2p3.y, p3p2.x, p3p2.y, p3.x, p3.y);
        cairo_mesh_pattern_line_to(mesh, p0.x, p0.y);
        ///Set the 4 corners color
        ///inner is full color

        // IMPORTANT NOTE:
        // The two sqrt below are due to a probable cairo bug.
        // To check whether the bug is present is a given cairo version,
        // make any shape with a very large feather and set
        // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
        // and approximately equal to 0.5.
        // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
        // older Cairo versions.
        cairo_mesh_pattern_set_corner_color_rgba( mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2], innerOpacity);
        ///outer is faded
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2], outterOpacity);
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2], outterOpacity);
        ///inner is full color
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2], innerOpacity);
        assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);

        cairo_mesh_pattern_end_patch(mesh);

        if (mustStop) {
            break;
        }

        //std::swap(innerOpacity, outterOpacity);

        p1 = p2;

        // increment for next iteration
        // ++prev, ++next, ++bezIT, ++prevBez
        if ( prev != featherPolygon.end() ) {
            ++prev;
        }
        if ( next != featherPolygon.end() ) {
            ++next;
        }
        if ( bezIT != bezierPolygon.end() ) {
            ++bezIT;
        }
        if ( prevBez != bezierPolygon.end() ) {
            ++prevBez;
        }
    }  // for each point in polygon
} // RotoContextPrivate::renderFeather

void
RotoContextPrivate::renderFeather_cairo(const std::list<RotoFeatherVertex>& vertices, double shapeColor[3], double fallOff, cairo_pattern_t * mesh)
{
    // Roto feather is rendered as triangles
    assert(vertices.size() >= 3 && vertices.size() % 3 == 0);

    double fallOffInverse = 1. / fallOff;
    
    std::list<RotoFeatherVertex>::const_iterator next = vertices.begin();
    ++next;
    std::list<RotoFeatherVertex>::const_iterator nextNext = next;
    ++nextNext;
    int index = 0;
    for (std::list<RotoFeatherVertex>::const_iterator it = vertices.begin(); it!=vertices.end(); index += 3) {


        cairo_mesh_pattern_begin_patch(mesh);

        // Only 3 of the 4 vertices are valid
        const RotoFeatherVertex* innerVertices[2] = {0, 0};
        const RotoFeatherVertex* outterVertices[2] = {0, 0};

        {
            int innerIndex = 0;
            int outterIndex = 0;
            if (it->isInner) {
                innerVertices[innerIndex] = &(*it);
                ++innerIndex;
            } else {
                outterVertices[outterIndex] = &(*it);
                ++outterIndex;
            }
            if (next->isInner) {
                assert(innerIndex <= 1);
                if (innerIndex <= 1) {
                    innerVertices[innerIndex] = &(*next);
                    ++innerIndex;
                }
            } else {
                assert(outterIndex <= 1);
                if (outterIndex <= 1) {
                    outterVertices[outterIndex] = &(*next);
                    ++outterIndex;
                }
            }
            if (nextNext->isInner) {
                assert(innerIndex <= 1);
                if (innerIndex <= 1) {
                    innerVertices[innerIndex] = &(*nextNext);
                    ++innerIndex;
                }
            } else {
                assert(outterIndex <= 1);
                if (outterIndex <= 1) {
                    outterVertices[outterIndex] = &(*nextNext);
                    ++outterIndex;
                }
            }
            assert((outterIndex == 1 && innerIndex == 2) || (innerIndex == 1 && outterIndex == 2));
        }
        // make a degenerated coons patch out of the triangle so that we can assign a color to each vertex to emulate simple gouraud shaded triangles
        Point p0, p0p1, p1, p1p0, p2, p2p3, p3p2, p3;
        p0.x = innerVertices[0]->x;
        p0.y = innerVertices[0]->y;
        p1.x = outterVertices[0]->x;
        p1.y = outterVertices[0]->y;
        if (outterVertices[1]) {
            p2.x = outterVertices[1]->x;
            p2.y = outterVertices[1]->y;
        } else {
            // Repeat p1 if only 1 outer vertex
            p2 = p1;
        }
        if (innerVertices[1]) {
            p3.x = innerVertices[1]->x;
            p3.y = innerVertices[1]->y;
        } else {
            // Repeat p0 if only 1 inner vertex
            p3 = p0;
        }


        ///linear interpolation
        p0p1.x = (p0.x * fallOff * 2. + fallOffInverse * p1.x) / (fallOff * 2. + fallOffInverse);
        p0p1.y = (p0.y * fallOff * 2. + fallOffInverse * p1.y) / (fallOff * 2. + fallOffInverse);
        p1p0.x = (p0.x * fallOff + 2. * fallOffInverse * p1.x) / (fallOff + 2. * fallOffInverse);
        p1p0.y = (p0.y * fallOff + 2. * fallOffInverse * p1.y) / (fallOff + 2. * fallOffInverse);


        p2p3.x = (p3.x * fallOff + 2. * fallOffInverse * p2.x) / (fallOff + 2. * fallOffInverse);
        p2p3.y = (p3.y * fallOff + 2. * fallOffInverse * p2.y) / (fallOff + 2. * fallOffInverse);
        p3p2.x = (p3.x * fallOff * 2. + fallOffInverse * p2.x) / (fallOff * 2. + fallOffInverse);
        p3p2.y = (p3.y * fallOff * 2. + fallOffInverse * p2.y) / (fallOff * 2. + fallOffInverse);


        // move to the initial point
        cairo_mesh_pattern_move_to(mesh, p0.x, p0.y);
        cairo_mesh_pattern_curve_to(mesh, p0p1.x, p0p1.y, p1p0.x, p1p0.y, p1.x, p1.y);
        cairo_mesh_pattern_line_to(mesh, p2.x, p2.y);
        cairo_mesh_pattern_curve_to(mesh, p2p3.x, p2p3.y, p3p2.x, p3p2.y, p3.x, p3.y);
        cairo_mesh_pattern_line_to(mesh, p0.x, p0.y);

        // Set the 4 corners color
        // inner is full color

        // IMPORTANT NOTE:
        // The two sqrt below are due to a probable cairo bug.
        // To check whether the bug is present is a given cairo version,
        // make any shape with a very large feather and set
        // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
        // and approximately equal to 0.5.
        // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
        // older Cairo versions.
        cairo_mesh_pattern_set_corner_color_rgba( mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2], 1.);
        // outer is faded
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2], 0.);
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2], 0.);
        // inner is full color
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2], 1.);
        assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);

        cairo_mesh_pattern_end_patch(mesh);


        assert(nextNext != vertices.end());
        ++nextNext;

        // check if we reached the end
        if (nextNext == vertices.end()) {
            break;
        }
        // advance a second time
        ++nextNext;
        if (nextNext == vertices.end()) {
            break;
        }
        // advance a 3rd time
        ++nextNext;
        if (nextNext == vertices.end()) {
            break;
        }

        assert(next != vertices.end());
        ++next;
        assert(next != vertices.end());
        ++next;
        assert(next != vertices.end());
        ++next;
        assert(next != vertices.end());

        assert(it != vertices.end());
        ++it;
        assert(it != vertices.end());
        ++it;
        assert(it != vertices.end());
        ++it;
        assert(it != vertices.end());
    } // for (std::list<RotoFeatherVertex>::const_iterator it = vertices.begin(); it!=vertices.end(); ) 
} // RotoContextPrivate::renderFeather_cairo


struct tessPolygonData
{
    std::list<RotoTriangleFans>* internalFans;
    std::list<RotoTriangles>* internalTriangles;
    std::list<RotoTriangleStrips>* internalStrips;

    std::unique_ptr<RotoTriangleFans> fanBeingEdited;
    std::unique_ptr<RotoTriangles> trianglesBeingEdited;
    std::unique_ptr<RotoTriangleStrips> stripsBeingEdited;

    std::list<std::shared_ptr<Point> > allocatedIntersections;
    unsigned int error;
};

static void tess_begin_primitive_callback(unsigned int which, void *polygonData)
{

    tessPolygonData* myData = (tessPolygonData*)polygonData;
    assert(myData);

    switch (which) {
        case LIBTESS_GL_TRIANGLE_STRIP:
            assert(!myData->stripsBeingEdited);
            // scoped_ptr
            myData->stripsBeingEdited.reset(new RotoTriangleStrips);
            break;
        case LIBTESS_GL_TRIANGLE_FAN:
            assert(!myData->fanBeingEdited);
            // scoped_ptr
            myData->fanBeingEdited.reset(new RotoTriangleFans);
            break;
        case LIBTESS_GL_TRIANGLES:
            assert(!myData->trianglesBeingEdited);
            // scoped_ptr
            myData->trianglesBeingEdited.reset(new RotoTriangles);
            break;
        default:
            assert(false);
            break;
    }
}

static void tess_end_primitive_callback(void *polygonData)
{
    tessPolygonData* myData = (tessPolygonData*)polygonData;
    assert(myData);

    assert((myData->stripsBeingEdited && !myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && !myData->fanBeingEdited && myData->trianglesBeingEdited));

    if (myData->stripsBeingEdited) {
        myData->internalStrips->push_back(*myData->stripsBeingEdited);
        myData->stripsBeingEdited.reset();
    } else if (myData->fanBeingEdited) {
        myData->internalFans->push_back(*myData->fanBeingEdited);
        myData->fanBeingEdited.reset();
    } else if (myData->trianglesBeingEdited) {
        myData->internalTriangles->push_back(*myData->trianglesBeingEdited);
        myData->trianglesBeingEdited.reset();
    }

}

static void tess_vertex_callback(void* data /*per-vertex client data*/, void *polygonData)
{
    tessPolygonData* myData = (tessPolygonData*)polygonData;
    assert(myData);

    assert((myData->stripsBeingEdited && !myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && myData->fanBeingEdited && !myData->trianglesBeingEdited) ||
           (!myData->stripsBeingEdited && !myData->fanBeingEdited && myData->trianglesBeingEdited));

    Point* p = (Point*)data;
    assert(p);

    if (myData->stripsBeingEdited) {
        myData->stripsBeingEdited->vertices.push_back(*p);
    } else if (myData->fanBeingEdited) {
        myData->fanBeingEdited->vertices.push_back(*p);
    } else if (myData->trianglesBeingEdited) {
        myData->trianglesBeingEdited->vertices.push_back(*p);
    }

}

static void tess_error_callback(unsigned int error, void *polygonData)
{
    tessPolygonData* myData = (tessPolygonData*)polygonData;
    assert(myData);
    myData->error = error;
}

static void tess_intersection_combine_callback(double coords[3], void */*data*/[4] /*4 original vertices*/, double /*w*/[4] /*weights*/, void **dataOut, void *polygonData)
{
    tessPolygonData* myData = (tessPolygonData*)polygonData;
    assert(myData);

    std::shared_ptr<Point> ret = std::make_shared<Point>();
    ret->x = coords[0];
    ret->y = coords[1];
    /*new->r = w[0]*d[0]->r + w[1]*d[1]->r + w[2]*d[2]->r + w[3]*d[3]->r;
    new->g = w[0]*d[0]->g + w[1]*d[1]->g + w[2]*d[2]->g + w[3]*d[3]->g;
    new->b = w[0]*d[0]->b + w[1]*d[1]->b + w[2]*d[2]->b + w[3]*d[3]->b;
    new->a = w[0]*d[0]->a + w[1]*d[1]->a + w[2]*d[2]->a + w[3]*d[3]->a;*/
    *dataOut = ret.get();
    myData->allocatedIntersections.push_back(ret);
}

void
RotoContextPrivate::computeTriangles(const Bezier * bezier, double time, unsigned int mipmapLevel, double featherDist,
                                     std::list<RotoFeatherVertex>* featherMesh,
                                     std::list<RotoTriangleFans>* internalFans,
                                     std::list<RotoTriangles>* internalTriangles,
                                     std::list<RotoTriangleStrips>* internalStrips)
{
    ///Note that we do not use the opacity when rendering the bezier, it is rendered with correct floating point opacity/color when converting
    ///to the Natron image.

    bool clockWise = bezier->isFeatherPolygonClockwiseOriented(false, time);

    const double absFeatherDist = std::abs(featherDist);

    std::list<std::list<ParametricPoint> > featherPolygon;
    std::list<std::list<ParametricPoint> > bezierPolygon;

    RectD featherPolyBBox;
    featherPolyBBox.setupInfinity();

#ifdef ROTO_BEZIER_EVAL_ITERATIVE
    int error = -1;
#else
    double error = 1;
#endif

    bezier->evaluateFeatherPointsAtTime_DeCasteljau(false, time, mipmapLevel,error, true, &featherPolygon, &featherPolyBBox);
    bezier->evaluateAtTime_DeCasteljau(false, time, mipmapLevel, error,&bezierPolygon, NULL);


    // First compute the mesh composed of triangles of the feather
    assert( !featherPolygon.empty() && !bezierPolygon.empty() && featherPolygon.size() == bezierPolygon.size());

    std::list<std::list<ParametricPoint> >::const_iterator fIt = featherPolygon.begin();
    for (std::list<std::list<ParametricPoint> > ::const_iterator it = bezierPolygon.begin(); it != bezierPolygon.end(); ++it, ++fIt) {

        // Iterate over each bezier segment.
        // There are the same number of bezier segments for the feather and the internal bezier. Each discretized segment is a contour (list of vertices)

        std::list<ParametricPoint>::const_iterator bSegmentIt = it->begin();
        std::list<ParametricPoint>::const_iterator fSegmentIt = fIt->begin();

        assert(!it->empty() && !fIt->empty());


        // prepare iterators to compute derivatives for feather distance
        std::list<ParametricPoint>::const_iterator fnext = fSegmentIt;
        ++fnext;  // can only be valid since we assert the list is not empty
        if ( fnext == fIt->end() ) {
            fnext = fIt->begin();
        }
        std::list<ParametricPoint>::const_iterator fprev = fIt->end();
        --fprev; // can only be valid since we assert the list is not empty


        // initialize the state with a segment between the first inner vertex and first outer vertex
        RotoFeatherVertex lastInnerVert,lastOutterVert;
        {
            lastInnerVert.x = bSegmentIt->x;
            lastInnerVert.y = bSegmentIt->y;
            lastInnerVert.isInner = true;
            featherMesh->push_back(lastInnerVert);
            ++bSegmentIt;
        }
        if ( fSegmentIt != fIt->end() ) {
            lastOutterVert.x = fSegmentIt->x;
            lastOutterVert.y = fSegmentIt->y;

            if (absFeatherDist) {
                double diffx = fnext->x - fprev->x;
                double diffy = fnext->y - fprev->y;
                double norm = std::sqrt( diffx * diffx + diffy * diffy );
                double dx = (norm != 0) ? -( diffy / norm ) : 0;
                double dy = (norm != 0) ? ( diffx / norm ) : 1;

                if (!clockWise) {
                    lastOutterVert.x -= dx * absFeatherDist;
                    lastOutterVert.y -= dy * absFeatherDist;
                } else {
                    lastOutterVert.x += dx * absFeatherDist;
                    lastOutterVert.y += dy * absFeatherDist;
                }
            }

            lastOutterVert.isInner = false;
            featherMesh->push_back(lastOutterVert);
            ++fSegmentIt;
        }

        if ( fprev != fIt->end() ) {
            ++fprev;
        }
        if ( fnext != fIt->end() ) {
            ++fnext;
        }



        for (;;) {

            if ( fnext == fIt->end() ) {
                fnext = fIt->begin();
            }
            if ( fprev == fIt->end() ) {
                fprev = fIt->begin();
            }

            double inner_t = (double)INT_MAX;
            double outter_t = (double)INT_MAX;
            bool gotOne = false;
            if (bSegmentIt != it->end()) {
                inner_t = bSegmentIt->t;
                gotOne = true;
            }
            if (fSegmentIt != fIt->end()) {
                outter_t = fSegmentIt->t;
                gotOne = true;
            }

            if (!gotOne) {
                break;
            }

            // Pick the point with the minimum t
            if (inner_t <= outter_t) {
                if ( bSegmentIt != it->end() ) {
                    lastInnerVert.x = bSegmentIt->x;
                    lastInnerVert.y = bSegmentIt->y;
                    lastInnerVert.isInner = true;
                    featherMesh->push_back(lastInnerVert);
                    ++bSegmentIt;
                }
            } else {
                if ( fSegmentIt != fIt->end() ) {
                    lastOutterVert.x = fSegmentIt->x;
                    lastOutterVert.y = fSegmentIt->y;

                    if (absFeatherDist) {
                        double diffx = fnext->x - fprev->x;
                        double diffy = fnext->y - fprev->y;
                        double norm = std::sqrt( diffx * diffx + diffy * diffy );
                        double dx = (norm != 0) ? -( diffy / norm ) : 0;
                        double dy = (norm != 0) ? ( diffx / norm ) : 1;

                        if (!clockWise) {
                            lastOutterVert.x -= dx * absFeatherDist;
                            lastOutterVert.y -= dy * absFeatherDist;
                        } else {
                            lastOutterVert.x += dx * absFeatherDist;
                            lastOutterVert.y += dy * absFeatherDist;
                        }
                    }
                    lastOutterVert.isInner = false;
                    featherMesh->push_back(lastOutterVert);
                    ++fSegmentIt;
                }


                if ( fprev != fIt->end() ) {
                    ++fprev;
                }
                if ( fnext != fIt->end() ) {
                    ++fnext;
                }
            }

            // Initialize the first segment of the next triangle if we did not reach the end
            if (fSegmentIt == fIt->end() && bSegmentIt == it->end()) {
                break;
            }
            featherMesh->push_back(lastOutterVert);
            featherMesh->push_back(lastInnerVert);


        } // for(;;)


    } // for all points in polygon

    // Now tessellate the internal bezier using glu
    tessPolygonData tessData;
    tessData.internalStrips = internalStrips;
    tessData.internalFans = internalFans;
    tessData.internalTriangles = internalTriangles;
    tessData.error = 0;
    libtess_GLUtesselator* tesselator = libtess_gluNewTess();

    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_BEGIN_DATA, (void (*)())tess_begin_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_VERTEX_DATA, (void (*)())tess_vertex_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_END_DATA, (void (*)())tess_end_primitive_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_ERROR_DATA, (void (*)())tess_error_callback);
    libtess_gluTessCallback(tesselator, LIBTESS_GLU_TESS_COMBINE_DATA, (void (*)())tess_intersection_combine_callback);

    // From README: if you know that all polygons lie in the x-y plane, call
    // gluTessNormal(tess, 0.0, 0.0, 1.0) before rendering any polygons.
    libtess_gluTessNormal(tesselator, 0, 0, 1);
    libtess_gluTessBeginPolygon(tesselator, (void*)&tessData);
    libtess_gluTessBeginContour(tesselator);

    for (std::list<std::list<ParametricPoint> >::const_iterator it = bezierPolygon.begin(); it != bezierPolygon.end(); ++it) {
        for (std::list<ParametricPoint>::const_iterator it2 = it->begin(); it2 != it->end(); ++it2) {
            double coords[3] = {it2->x, it2->y, 1.};
            libtess_gluTessVertex(tesselator, coords, (void*)&(*it2) /*per-vertex client data*/);
        }
    }


    libtess_gluTessEndContour(tesselator);
    libtess_gluTessEndPolygon(tesselator);
    libtess_gluDeleteTess(tesselator);

    // check for errors
    assert(tessData.error == 0);

} // RotoContextPrivate::computeFeatherTriangles

void
RotoContextPrivate::renderInternalShape_cairo(const std::list<RotoTriangles>& triangles,
                                              const std::list<RotoTriangleFans>& fans,
                                              const std::list<RotoTriangleStrips>& strips,
                                              double shapeColor[3], cairo_pattern_t * mesh)
{
    for (std::list<RotoTriangles>::const_iterator it = triangles.begin(); it!=triangles.end(); ++it ) {

        assert(it->vertices.size() >= 3 && it->vertices.size() % 3 == 0);

        int c = 0;
        const Point* coonsPatchStart = 0;
        for (std::list<Point>::const_iterator it2 = it->vertices.begin(); it2!=it->vertices.end(); ++it2) {
            if (c == 0) {
                cairo_mesh_pattern_begin_patch(mesh);
                cairo_mesh_pattern_move_to(mesh, it2->x, it2->y);
                coonsPatchStart = &(*it2);
            } else {
                cairo_mesh_pattern_line_to(mesh, it2->x, it2->y);
            }
            if (c == 2) {
                assert(coonsPatchStart);
                // close coons patch by transforming the triangle into a degenerated coons patch
                cairo_mesh_pattern_line_to(mesh, coonsPatchStart->x, coonsPatchStart->y);
                // IMPORTANT NOTE:
                // The two sqrt below are due to a probable cairo bug.
                // To check whether the bug is present is a given cairo version,
                // make any shape with a very large feather and set
                // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
                // and approximately equal to 0.5.
                // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
                // older Cairo versions.
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2], 1);
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2], 1);
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2], 1);
                cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2], 1);
                assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);

                cairo_mesh_pattern_end_patch(mesh);
                c = 0;
            } else {
                ++c;
            }
        }
    }
    for (std::list<RotoTriangleFans>::const_iterator it = fans.begin(); it!=fans.end(); ++it ) {

        assert(it->vertices.size() >= 3);
        std::list<Point>::const_iterator cur = it->vertices.begin();
        const Point* fanStart = &(*(cur));
        ++cur;
        std::list<Point>::const_iterator next = cur;
        ++next;
        for (;next != it->vertices.end();) {
            cairo_mesh_pattern_begin_patch(mesh);
            cairo_mesh_pattern_move_to(mesh, fanStart->x, fanStart->y);
            cairo_mesh_pattern_line_to(mesh, cur->x, cur->y);
            cairo_mesh_pattern_line_to(mesh, next->x, next->y);
            cairo_mesh_pattern_line_to(mesh, fanStart->x, fanStart->y);
            // IMPORTANT NOTE:
            // The two sqrt below are due to a probable cairo bug.
            // To check whether the bug is present is a given cairo version,
            // make any shape with a very large feather and set
            // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
            // and approximately equal to 0.5.
            // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
            // older Cairo versions.
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2], 1);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2], 1);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2], 1);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2], 1);
            assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);

            cairo_mesh_pattern_end_patch(mesh);

            ++next;
            ++cur;
        }
    }
    for (std::list<RotoTriangleStrips>::const_iterator it = strips.begin(); it!=strips.end(); ++it ) {

        assert(it->vertices.size() >= 3);

        std::list<Point>::const_iterator cur = it->vertices.begin();
        const Point* prevPrev = &(*(cur));
        ++cur;
        const Point* prev = &(*(cur));
        ++cur;
        for (; cur != it->vertices.end(); ++cur) {
            cairo_mesh_pattern_begin_patch(mesh);
            cairo_mesh_pattern_move_to(mesh, prevPrev->x, prevPrev->y);
            cairo_mesh_pattern_line_to(mesh, prev->x, prev->y);
            cairo_mesh_pattern_line_to(mesh, cur->x, cur->y);
            cairo_mesh_pattern_line_to(mesh, prevPrev->x, prevPrev->y);

            // IMPORTANT NOTE:
            // The two sqrt below are due to a probable cairo bug.
            // To check whether the bug is present is a given cairo version,
            // make any shape with a very large feather and set
            // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
            // and approximately equal to 0.5.
            // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
            // older Cairo versions.
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2], 1);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2], 1);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2], 1);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2], 1);
            assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);

            cairo_mesh_pattern_end_patch(mesh);
            
            prevPrev = prev;
            prev = &(*(cur));
        }
    }
}


void
RotoContextPrivate::renderInternalShape(double time,
                                        unsigned int mipmapLevel,
                                        double /*shapeColor*/[3],
                                        double /*opacity*/,
                                        const Transform::Matrix3x3& transform,
                                        cairo_t* cr,
#ifdef ROTO_USE_MESH_PATTERN_ONLY
                                        cairo_pattern_t* mesh,
#else
                                        cairo_pattern_t* /*mesh*/,
#endif
                                        const BezierCPs & cps)
{
    assert( !cps.empty() );
#ifdef ROTO_USE_MESH_PATTERN_ONLY
    std::list<BezierCPs> coonPatches;
    bezulate(time, cps, &coonPatches);

    for (std::list<BezierCPs>::iterator it = coonPatches.begin(); it != coonPatches.end(); ++it) {
        std::list<BezierCPs> fixedPatch;
        CoonsRegularization::regularize(*it, time, &fixedPatch);
        for (std::list<BezierCPs>::iterator it2 = fixedPatch.begin(); it2 != fixedPatch.end(); ++it2) {
            std::size_t size = it2->size();
            assert(size <= 4 && size >= 2);

            BezierCPs::iterator patchIT = it2->begin();
            BezierCPPtr p0ptr, p1ptr, p2ptr, p3ptr;
            p0ptr = *patchIT;
            ++patchIT;
            if (size == 2) {
                p1ptr = p0ptr;
                p2ptr = *patchIT;
                p3ptr = p2ptr;
            } else if (size == 3) {
                p1ptr = *patchIT;
                p2ptr = *patchIT;
                ++patchIT;
                p3ptr = *patchIT;
            } else if (size == 4) {
                p1ptr = *patchIT;
                ++patchIT;
                p2ptr = *patchIT;
                ++patchIT;
                p3ptr = *patchIT;
            }
            assert(p0ptr && p1ptr && p2ptr && p3ptr);

            Point p0, p0p1, p1p0, p1, p1p2, p2p1, p2p3, p3p2, p2, p3, p3p0, p0p3;

            p0ptr->getLeftBezierPointAtTime(time, &p0p3.x, &p0p3.y);
            p0ptr->getPositionAtTime(time, &p0.x, &p0.y);
            p0ptr->getRightBezierPointAtTime(time, &p0p1.x, &p0p1.y);

            p1ptr->getLeftBezierPointAtTime(time, &p1p0.x, &p1p0.y);
            p1ptr->getPositionAtTime(time, &p1.x, &p1.y);
            p1ptr->getRightBezierPointAtTime(time, &p1p2.x, &p1p2.y);

            p2ptr->getLeftBezierPointAtTime(time, &p2p1.x, &p2p1.y);
            p2ptr->getPositionAtTime(time, &p2.x, &p2.y);
            p2ptr->getRightBezierPointAtTime(time, &p2p3.x, &p2p3.y);

            p3ptr->getLeftBezierPointAtTime(time, &p3p2.x, &p3p2.y);
            p3ptr->getPositionAtTime(time, &p3.x, &p3.y);
            p3ptr->getRightBezierPointAtTime(time, &p3p0.x, &p3p0.y);


            adjustToPointToScale(mipmapLevel, p0.x, p0.y);
            adjustToPointToScale(mipmapLevel, p0p1.x, p0p1.y);
            adjustToPointToScale(mipmapLevel, p1p0.x, p1p0.y);
            adjustToPointToScale(mipmapLevel, p1.x, p1.y);
            adjustToPointToScale(mipmapLevel, p1p2.x, p1p2.y);
            adjustToPointToScale(mipmapLevel, p2p1.x, p2p1.y);
            adjustToPointToScale(mipmapLevel, p2.x, p2.y);
            adjustToPointToScale(mipmapLevel, p2p3.x, p2p3.y);
            adjustToPointToScale(mipmapLevel, p3p2.x, p3p2.y);
            adjustToPointToScale(mipmapLevel, p3.x, p3.y);
            adjustToPointToScale(mipmapLevel, p3p0.x, p3p0.y);
            adjustToPointToScale(mipmapLevel, p0p3.x, p0p3.y);

            // Add a Coons patch such as:

            //         C1  Side 1   C2
            //        +---------------+
            //        |               |
            //        |  P1       P2  |
            //        |               |
            // Side 0 |               | Side 2
            //        |               |
            //        |               |
            //        |  P0       P3  |
            //        |               |
            //        +---------------+
            //        C0     Side 3   C3

            // In the above drawing, C0 is p0, P0 is p0p1, P1 is p1p0, C1 is p1 and so on...

            ///move to C0
            cairo_mesh_pattern_begin_patch(mesh);
            cairo_mesh_pattern_move_to(mesh, p0.x, p0.y);
            if (size == 4) {
                cairo_mesh_pattern_curve_to(mesh, p0p1.x, p0p1.y, p1p0.x, p1p0.y, p1.x, p1.y);
                cairo_mesh_pattern_curve_to(mesh, p1p2.x, p1p2.y, p2p1.x, p2p1.y, p2.x, p2.y);
                cairo_mesh_pattern_curve_to(mesh, p2p3.x, p2p3.y, p3p2.x, p3p2.y, p3.x, p3.y);
                cairo_mesh_pattern_curve_to(mesh, p3p0.x, p3p0.y, p0p3.x, p0p3.y, p0.x, p0.y);
            } else if (size == 3) {
                cairo_mesh_pattern_curve_to(mesh, p0p1.x, p0p1.y, p1p0.x, p1p0.y, p1.x, p1.y);
                cairo_mesh_pattern_line_to(mesh, p2.x, p2.y);
                cairo_mesh_pattern_curve_to(mesh, p2p3.x, p2p3.y, p3p2.x, p3p2.y, p3.x, p3.y);
                cairo_mesh_pattern_curve_to(mesh, p3p0.x, p3p0.y, p0p3.x, p0p3.y, p0.x, p0.y);
            } else {
                assert(size == 2);
                cairo_mesh_pattern_line_to(mesh, p1.x, p1.y);
                cairo_mesh_pattern_curve_to(mesh, p1p2.x, p1p2.y, p2p1.x, p2p1.y, p2.x, p2.y);
                cairo_mesh_pattern_line_to(mesh, p3.x, p3.y);
                cairo_mesh_pattern_curve_to(mesh, p3p0.x, p3p0.y, p0p3.x, p0p3.y, p0.x, p0.y);
            }
            ///Set the 4 corners color

            // IMPORTANT NOTE:
            // The two sqrt below are due to a probable cairo bug.
            // To check whether the bug is present is a given cairo version,
            // make any shape with a very large feather and set
            // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
            // and approximately equal to 0.5.
            // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
            // older Cairo versions.
            cairo_mesh_pattern_set_corner_color_rgba( mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2],
                                                      std::sqrt(opacity) );
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2],
                                                     opacity);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2],
                                                     opacity);
            cairo_mesh_pattern_set_corner_color_rgba( mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2],
                                                      std::sqrt(opacity) );
            assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);

            cairo_mesh_pattern_end_patch(mesh);
        }
    }
#else // ifdef ROTO_USE_MESH_PATTERN_ONLY

    cairo_set_source_rgba(cr, 1, 1, 1, 1);

    BezierCPs::const_iterator point = cps.begin();
    assert( point != cps.end() );
    if ( point == cps.end() ) {
        return;
    }
    BezierCPs::const_iterator nextPoint = point;
    if ( nextPoint != cps.end() ) {
        ++nextPoint;
    }


    Transform::Point3D initCp;
    (*point)->getPositionAtTime(false, time, ViewIdx(0), &initCp.x, &initCp.y);
    initCp.z = 1.;
    initCp = Transform::matApply(transform, initCp);

    adjustToPointToScale(mipmapLevel, initCp.x, initCp.y);

    cairo_move_to(cr, initCp.x, initCp.y);

    while ( point != cps.end() ) {
        if ( nextPoint == cps.end() ) {
            nextPoint = cps.begin();
        }

        Transform::Point3D right, nextLeft, next;
        (*point)->getRightBezierPointAtTime(false, time, ViewIdx(0), &right.x, &right.y);
        right.z = 1;
        (*nextPoint)->getLeftBezierPointAtTime(false, time, ViewIdx(0), &nextLeft.x, &nextLeft.y);
        nextLeft.z = 1;
        (*nextPoint)->getPositionAtTime(false, time, ViewIdx(0), &next.x, &next.y);
        next.z = 1;

        right = Transform::matApply(transform, right);
        nextLeft = Transform::matApply(transform, nextLeft);
        next = Transform::matApply(transform, next);

        adjustToPointToScale(mipmapLevel, right.x, right.y);
        adjustToPointToScale(mipmapLevel, next.x, next.y);
        adjustToPointToScale(mipmapLevel, nextLeft.x, nextLeft.y);
        cairo_curve_to(cr, right.x, right.y, nextLeft.x, nextLeft.y, next.x, next.y);

        // increment for next iteration
        ++point;
        if ( nextPoint != cps.end() ) {
            ++nextPoint;
        }
    } // while()
//    if (cairo_get_antialias(cr) != CAIRO_ANTIALIAS_NONE ) {
//        cairo_fill_preserve(cr);
//        // These line properties make for a nicer looking polygon mesh
//        cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
//        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
//        // Comment out the following call to cairo_set_line width
//        // since the hard-coded width value of 1.0 is not appropriate
//        // for fills of small areas. Instead, use the line width that
//        // has already been set by the user via the above call of
//        // poly_line which in turn calls set_current_context which in
//        // turn calls cairo_set_line_width for the user-specified
//        // width.
//        cairo_set_line_width(cr, 1.0);
//        cairo_stroke(cr);
//    } else {
    cairo_fill(cr);
//    }
#endif // ifdef ROTO_USE_MESH_PATTERN_ONLY
} // RotoContextPrivate::renderInternalShape

struct qpointf_compare_less
{
    bool operator() (const QPointF& lhs,
                     const QPointF& rhs) const
    {
        if (std::abs( lhs.x() - rhs.x() ) < 1e-6) {
            if (std::abs( lhs.y() - rhs.y() ) < 1e-6) {
                return false;
            } else if ( lhs.y() < rhs.y() ) {
                return true;
            } else {
                return false;
            }
        } else if ( lhs.x() < rhs.x() ) {
            return true;
        } else {
            return false;
        }
    }
};

static bool
pointInPolygon(const Point & p,
               const std::list<Point> & polygon,
               const RectD & featherPolyBBox,
               Bezier::FillRuleEnum rule)
{
    ///first check if the point lies inside the bounding box
    if ( (p.x < featherPolyBBox.x1) || (p.x >= featherPolyBBox.x2) || (p.y < featherPolyBBox.y1) || (p.y >= featherPolyBBox.y2)
         || polygon.empty() ) {
        return false;
    }

    int winding_number = 0;
    std::list<Point>::const_iterator last_pt = polygon.begin();
    std::list<Point>::const_iterator last_start = last_pt;
    std::list<Point>::const_iterator cur = last_pt;
    ++cur;
    for (; cur != polygon.end(); ++cur, ++last_pt) {
        Bezier::point_line_intersection(*last_pt, *cur, p, &winding_number);
    }

    // implicitly close last subpath
    if (last_pt != last_start) {
        Bezier::point_line_intersection(*last_pt, *last_start, p, &winding_number);
    }

    return rule == Bezier::eFillRuleWinding
           ? (winding_number != 0)
           : ( (winding_number % 2) != 0 );
}

//From http://www.math.ualberta.ca/~bowman/publications/cad10.pdf
void
RotoContextPrivate::bezulate(double time,
                             const BezierCPs& cps,
                             std::list<BezierCPs>* patches)
{
    BezierCPs simpleClosedCurve = cps;

    while (simpleClosedCurve.size() > 4) {
        bool found = false;
        for (int n = 3; n >= 2; --n) {
            assert( (int)simpleClosedCurve.size() > n );

            //next points at point i + n
            BezierCPs::iterator next = simpleClosedCurve.begin();
            std::advance(next, n);
            std::list<Point> polygon;
            RectD bbox;
            bbox.setupInfinity();
            for (BezierCPs::iterator it = simpleClosedCurve.begin(); it != simpleClosedCurve.end(); ++it) {
                Point p;
                (*it)->getPositionAtTime(false, time, ViewIdx(0), &p.x, &p.y);
                polygon.push_back(p);
                if (p.x < bbox.x1) {
                    bbox.x1 = p.x;
                }
                if (p.x > bbox.x2) {
                    bbox.x2 = p.x;
                }
                if (p.y < bbox.y1) {
                    bbox.y1 = p.y;
                }
                if (p.y > bbox.y2) {
                    bbox.y2 = p.y;
                }
            }


            for (BezierCPs::iterator it = simpleClosedCurve.begin(); it != simpleClosedCurve.end(); ++it) {
                bool nextIsPassedEnd = false;
                if ( next == simpleClosedCurve.end() ) {
                    next = simpleClosedCurve.begin();
                    nextIsPassedEnd = true;
                }

                //mid-point of the line segment between points i and i + n
                Point nextPoint, curPoint;
                (*it)->getPositionAtTime(false, time, ViewIdx(0), &curPoint.x, &curPoint.y);
                (*next)->getPositionAtTime(false, time, ViewIdx(0), &nextPoint.x, &nextPoint.y);

                /*
                 * Compute the number of intersections between the current line segment [it,next] and all other line segments
                 * If the number of intersections is different of 2, ignore this segment.
                 */
                QLineF line( QPointF(curPoint.x, curPoint.y), QPointF(nextPoint.x, nextPoint.y) );
                std::set<QPointF, qpointf_compare_less> intersections;
                std::list<Point>::const_iterator last_pt = polygon.begin();
                std::list<Point>::const_iterator cur = last_pt;
                ++cur;
                QPointF intersectionPoint;
                for (; cur != polygon.end(); ++cur, ++last_pt) {
                    QLineF polygonSegment( QPointF(last_pt->x, last_pt->y), QPointF(cur->x, cur->y) );
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                    QLineF::IntersectionType intersectType = line.intersects(polygonSegment, &intersectionPoint);
#else
                    QLineF::IntersectType intersectType = line.intersect(polygonSegment, &intersectionPoint);
#endif
                    if (intersectType == QLineF::BoundedIntersection) {
                        intersections.insert(intersectionPoint);
                    }
                    if (intersections.size() > 2) {
                        break;
                    }
                }

                if (intersections.size() != 2) {
                    continue;
                }

                /*
                 * Check if the midpoint of the line segment [it,next] lies inside the simple closed curve (polygon), otherwise
                 * ignore it.
                 */
                Point midPoint;
                midPoint.x = (nextPoint.x + curPoint.x) / 2.;
                midPoint.y = (nextPoint.y + curPoint.y) / 2.;
                bool isInside = pointInPolygon(midPoint, polygon, bbox, Bezier::eFillRuleWinding);

                if (isInside) {
                    //Make the sub closed curve composed of the path from points i to i + n
                    BezierCPs subCurve;
                    subCurve.push_back(*it);
                    BezierCPs::iterator pointIt = it;
                    for (int i = 0; i < n - 1; ++i) {
                        ++pointIt;
                        if ( pointIt == simpleClosedCurve.end() ) {
                            pointIt = simpleClosedCurve.begin();
                        }
                        subCurve.push_back(*pointIt);
                    }
                    subCurve.push_back(*next);

                    // Ensure that all interior angles are less than 180 degrees.


                    patches->push_back(subCurve);

                    //Remove i + 1 to i + n
                    BezierCPs::iterator eraseStart = it;
                    ++eraseStart;
                    bool eraseStartIsPassedEnd = false;
                    if ( eraseStart == simpleClosedCurve.end() ) {
                        eraseStart = simpleClosedCurve.begin();
                        eraseStartIsPassedEnd = true;
                    }
                    //"it" is  invalidated after the next instructions but we leave the loop anyway
                    assert( !simpleClosedCurve.empty() );
                    if ( (!nextIsPassedEnd && !eraseStartIsPassedEnd) || (nextIsPassedEnd && eraseStartIsPassedEnd) ) {
                        simpleClosedCurve.erase(eraseStart, next);
                    } else {
                        simpleClosedCurve.erase( eraseStart, simpleClosedCurve.end() );
                        if ( !simpleClosedCurve.empty() ) {
                            simpleClosedCurve.erase(simpleClosedCurve.begin(), next);
                        }
                    }
                    found = true;
                    break;
                }

                // increment for next iteration
                if ( next != simpleClosedCurve.end() ) {
                    ++next;
                }
            } // for(it)
            if (found) {
                break;
            }
        } // for(n)

        if (!found) {
            BezierCPs subdivisedCurve;
            //Subdivise the curve at the midpoint of each segment
            BezierCPs::iterator next = simpleClosedCurve.begin();
            if ( next != simpleClosedCurve.end() ) {
                ++next;
            }
            for (BezierCPs::iterator it = simpleClosedCurve.begin(); it != simpleClosedCurve.end(); ++it) {
                if ( next == simpleClosedCurve.end() ) {
                    next = simpleClosedCurve.begin();
                }
                Point p0, p1, p2, p3, p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3, dest;
                (*it)->getPositionAtTime(false, time, ViewIdx(0), &p0.x, &p0.y);
                (*it)->getRightBezierPointAtTime(false, time, ViewIdx(0), &p1.x, &p1.y);
                (*next)->getLeftBezierPointAtTime(false, time, ViewIdx(0), &p2.x, &p2.y);
                (*next)->getPositionAtTime(false, time, ViewIdx(0), &p3.x, &p3.y);
                Bezier::bezierFullPoint(p0, p1, p2, p3, 0.5, &p0p1, &p1p2, &p2p3, &p0p1_p1p2, &p1p2_p2p3, &dest);
                BezierCPPtr controlPoint = std::make_shared<BezierCP>();
                controlPoint->setStaticPosition(false, dest.x, dest.y);
                controlPoint->setLeftBezierStaticPosition(false, p0p1_p1p2.x, p0p1_p1p2.y);
                controlPoint->setRightBezierStaticPosition(false, p1p2_p2p3.x, p1p2_p2p3.y);
                subdivisedCurve.push_back(*it);
                subdivisedCurve.push_back(controlPoint);

                // increment for next iteration
                if ( next != simpleClosedCurve.end() ) {
                    ++next;
                }
            } // for()
            simpleClosedCurve = subdivisedCurve;
        }
    }
    if ( !simpleClosedCurve.empty() ) {
        assert(simpleClosedCurve.size() >= 2);
        patches->push_back(simpleClosedCurve);
    }
} // RotoContextPrivate::bezulate

void
RotoContextPrivate::applyAndDestroyMask(cairo_t* cr,
                                        cairo_pattern_t* mesh)
{
    assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
    cairo_set_source(cr, mesh);

    ///paint with the feather with the pattern as a mask
    cairo_mask(cr, mesh);

    cairo_pattern_destroy(mesh);
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
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( item.get() );

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

NodePtr
RotoContext::getOrCreateGlobalMergeNode(int *availableInputIndex)
{
    {
        QMutexLocker k(&_imp->rotoContextMutex);
        for (NodesList::iterator it = _imp->globalMergeNodes.begin(); it != _imp->globalMergeNodes.end(); ++it) {
            const std::vector<NodeWPtr> &inputs = (*it)->getInputs();

            //Merge node goes like this: B, A, Mask, A2, A3, A4 ...
            assert( inputs.size() >= 3 && (*it)->getEffectInstance()->isInputMask(2) );
            if ( !inputs[1].lock() ) {
                *availableInputIndex = 1;

                return *it;
            }

            //Leave the B empty to connect the next merge node
            for (std::size_t i = 3; i < inputs.size(); ++i) {
                if ( !inputs[i].lock() ) {
                    *availableInputIndex = (int)i;

                    return *it;
                }
            }
        }
    }

    NodePtr node = getNode();
    //We must create a new merge node
    QString fixedNamePrefix = QString::fromUtf8( node->getScriptName_mt_safe().c_str() );

    fixedNamePrefix.append( QLatin1Char('_') );
    fixedNamePrefix.append( QString::fromUtf8("globalMerge") );
    fixedNamePrefix.append( QLatin1Char('_') );


    CreateNodeArgs args( PLUGINID_OFX_MERGE,  NodeCollectionPtr() );
    args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
    args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
    args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, fixedNamePrefix.toStdString());
    
    NodePtr mergeNode = node->getApp()->createNode(args);
    if (!mergeNode) {
        return mergeNode;
    }
    mergeNode->setUseAlpha0ToConvertFromRGBToRGBA(true);
    if ( getNode()->isDuringPaintStrokeCreation() ) {
        mergeNode->setWhileCreatingPaintStroke(true);
    }
    *availableInputIndex = 1;

    QMutexLocker k(&_imp->rotoContextMutex);
    _imp->globalMergeNodes.push_back(mergeNode);

    return mergeNode;
} // RotoContext::getOrCreateGlobalMergeNode

void
RotoContext::refreshRotoPaintTree()
{
    if (_imp->isCurrentlyLoading) {
        return;
    }

    // Do not use only activated items when defining the shape of the RotoPaint tree otherwise we would have to adjust the tree at each frame.
    std::list<RotoDrawableItemPtr> items = getCurvesByRenderOrder(false /*onlyActivatedItems*/);
    int blendingOperator;
    bool canConcatenate = isRotoPaintTreeConcatenatableInternal(items, &blendingOperator);
    NodePtr globalMerge;
    int globalMergeIndex = -1;
    NodesList mergeNodes;
    {
        QMutexLocker k(&_imp->rotoContextMutex);
        mergeNodes = _imp->globalMergeNodes;
    }
    //ensure that all global merge nodes are disconnected
    for (NodesList::iterator it = mergeNodes.begin(); it != mergeNodes.end(); ++it) {
        int maxInputs = (*it)->getNInputs();
        for (int i = 0; i < maxInputs; ++i) {
            (*it)->disconnectInput(i);
        }
    }
    if (canConcatenate) {
        globalMerge = getOrCreateGlobalMergeNode(&globalMergeIndex);
    }
    if (globalMerge) {
        NodePtr rotopaintNodeInput = getNode()->getInput(0);
        //Connect the rotopaint node input to the B input of the Merge
        if (rotopaintNodeInput) {
            globalMerge->connectInput(rotopaintNodeInput, 0);
        }
    }

    for (std::list<RotoDrawableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        (*it)->refreshNodesConnections();

        if (globalMerge) {
            {
                KnobIPtr mergeOperatorKnob = globalMerge->getKnobByName(kMergeOFXParamOperation);
                KnobChoice* mergeOp = dynamic_cast<KnobChoice*>( mergeOperatorKnob.get() );
                if (mergeOp) {
                    mergeOp->setValue(blendingOperator);
                }
            }

            NodePtr effectNode = (*it)->getEffectNode();
            assert(effectNode);
            //qDebug() << "Connecting" << (*it)->getScriptName().c_str() << "to input" << globalMergeIndex <<
            //"(" << globalMerge->getInputLabel(globalMergeIndex).c_str() << ")" << "of" << globalMerge->getScriptName().c_str();
            globalMerge->connectInput(effectNode, globalMergeIndex);

            ///Refresh for next node
            NodePtr nextMerge = getOrCreateGlobalMergeNode(&globalMergeIndex);
            if (nextMerge != globalMerge) {
                assert( !nextMerge->getInput(0) );
                nextMerge->connectInput(globalMerge, 0);
                globalMerge = nextMerge;
            }
        }
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
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( item.get() );

    if (isStroke) {
        ///Strokes are unsupported in Python currently
        return;
    }
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>( item.get() );
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
    for (std::list<RotoLayerPtr>::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        declareItemAsPythonField(*it);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_RotoContext.cpp"
