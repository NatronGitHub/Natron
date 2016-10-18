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
    QObject::connect( _imp->lifeTime.lock()->getSignalSlotHandler().get(), SIGNAL(mustRefreshKnobGui(ViewIdx,DimIdx,ValueChangedReasonEnum)), this, SLOT(onLifeTimeKnobValueChanged(ViewIdx,DimIdx,ValueChangedReasonEnum)) );
}

bool
RotoContext::isRotoPaint() const
{
    return _imp->isPaintNode;



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
        it->lock()->setEnabled(false);
    }

    assert(_imp->layers.size() == 1);

    RotoLayerPtr baseLayer = _imp->layers.front();

    baseLayer->fromSerialization(s->_baseLayer);

    _imp->isCurrentlyLoading = false;
    refreshRotoPaintTree();
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
        knob->setEnabled(enabled);
    }
    _imp->lastLockedItem = item;
    Q_EMIT itemLockedChanged( (int)reason );
}




NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_RotoContext.cpp"
