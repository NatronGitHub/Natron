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
#include <stdexcept>

#include <QLineF>
#include <QtDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Global/MemoryInfo.h"
#include "Engine/RotoContextPrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
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

#define kMergeOFXParamOperation "operation"
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

NATRON_NAMESPACE_ENTER;

////////////////////////////////////RotoContext////////////////////////////////////


RotoContext::RotoContext(const boost::shared_ptr<Node>& node)
    : _imp( new RotoContextPrivate(node) )
{
    QObject::connect(_imp->lifeTime.lock()->getSignalSlotHandler().get(), SIGNAL(valueChanged(int,int)), this, SLOT(onLifeTimeKnobValueChanged(int,int)));
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
    for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->globalMergeNodes.begin(); it != _imp->globalMergeNodes.end(); ++it) {
        (*it)->setWhileCreatingPaintStroke(b);
    }
}

boost::shared_ptr<Node>
RotoContext::getRotoPaintBottomMergeNode() const
{
    std::list<boost::shared_ptr<RotoDrawableItem> > items = getCurvesByRenderOrder();
    if (items.empty()) {
        return boost::shared_ptr<Node>();
    }
    
    if (isRotoPaintTreeConcatenatableInternal(items)) {
        QMutexLocker k(&_imp->rotoContextMutex);
        if (!_imp->globalMergeNodes.empty()) {
            return _imp->globalMergeNodes.front();
        }
    }
    
    const boost::shared_ptr<RotoDrawableItem>& firstStrokeItem = items.back();
    assert(firstStrokeItem);
    boost::shared_ptr<Node> bottomMerge = firstStrokeItem->getMergeNode();
    assert(bottomMerge);
    return bottomMerge;
}

void
RotoContext::getRotoPaintTreeNodes(std::list<boost::shared_ptr<Node> >* nodes) const
{
    std::list<boost::shared_ptr<RotoDrawableItem> > items = getCurvesByRenderOrder(false);
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::iterator it = items.begin(); it != items.end(); ++it) {
      
        boost::shared_ptr<Node> effectNode = (*it)->getEffectNode();
        boost::shared_ptr<Node> mergeNode = (*it)->getMergeNode();
        boost::shared_ptr<Node> timeOffsetNode = (*it)->getTimeOffsetNode();
        boost::shared_ptr<Node> frameHoldNode = (*it)->getFrameHoldNode();
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
    for (std::list<boost::shared_ptr<Node> >::const_iterator it = _imp->globalMergeNodes.begin(); it != _imp->globalMergeNodes.end(); ++it) {
        nodes->push_back(*it);
    }
}

///Must be done here because at the time of the constructor, the shared_ptr doesn't exist yet but
///addLayer() needs it to get a shared ptr to this
void
RotoContext::createBaseLayer()
{
    ////Add the base layer
    boost::shared_ptr<RotoLayer> base = addLayerInternal(false);
    
    deselect(base, RotoItem::eSelectionReasonOther);
}

RotoContext::~RotoContext()
{
}

boost::shared_ptr<RotoLayer>
RotoContext::getOrCreateBaseLayer()
{
    QMutexLocker k(&_imp->rotoContextMutex);
    if (_imp->layers.empty()) {
        k.unlock();
        addLayer();
        k.relock();
    }
    assert(!_imp->layers.empty());
    return _imp->layers.front();
}

boost::shared_ptr<RotoLayer>
RotoContext::addLayerInternal(bool declarePython)
{
    boost::shared_ptr<RotoContext> this_shared = shared_from_this();
    assert(this_shared);
    
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    boost::shared_ptr<RotoLayer> item;
    
    std::string name = generateUniqueName(kRotoLayerBaseName);
    int indexInLayer = -1;
    {
        
        boost::shared_ptr<RotoLayer> deepestLayer;
        boost::shared_ptr<RotoLayer> parentLayer;
        {
            QMutexLocker l(&_imp->rotoContextMutex);
            deepestLayer = _imp->findDeepestSelectedLayer();

            if (!deepestLayer) {
                ///find out if there's a base layer, if so add to the base layer,
                ///otherwise create the base layer
                for (std::list<boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
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
        
        item.reset( new RotoLayer(this_shared, name, boost::shared_ptr<RotoLayer>()) );
        if (parentLayer) {
            parentLayer->addItem(item,declarePython);
            indexInLayer = parentLayer->getItems().size() - 1;
        }
        
        QMutexLocker l(&_imp->rotoContextMutex);

        _imp->layers.push_back(item);
        
        _imp->lastInsertedItem = item;
    }
    
    Q_EMIT itemInserted(indexInLayer,RotoItem::eSelectionReasonOther);
    
    
    clearSelection(RotoItem::eSelectionReasonOther);
    select(item, RotoItem::eSelectionReasonOther);
    
    return item;

}

boost::shared_ptr<RotoLayer>
RotoContext::addLayer()
{
    return addLayerInternal(true);
} // addLayer

void
RotoContext::addLayer(const boost::shared_ptr<RotoLayer> & layer)
{
    std::list<boost::shared_ptr<RotoLayer> >::iterator it = std::find(_imp->layers.begin(), _imp->layers.end(), layer);

    if ( it == _imp->layers.end() ) {
        _imp->layers.push_back(layer);
    }
}

boost::shared_ptr<RotoItem>
RotoContext::getLastInsertedItem() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->lastInsertedItem;
}

#ifdef NATRON_ROTO_INVERTIBLE
boost::shared_ptr<KnobBool>
RotoContext::getInvertedKnob() const
{
    return _imp->inverted.lock();
}

#endif

boost::shared_ptr<KnobColor>
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

boost::shared_ptr<Node>
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
        if (getItemByName(name)) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);
    return name;
}

boost::shared_ptr<Bezier>
RotoContext::makeBezier(double x,
                        double y,
                        const std::string & baseName,
                        double time,
                        bool isOpenBezier)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    boost::shared_ptr<RotoLayer> parentLayer;
    boost::shared_ptr<RotoContext> this_shared = boost::dynamic_pointer_cast<RotoContext>(shared_from_this());
    assert(this_shared);
    std::string name = generateUniqueName(baseName);

    {

        QMutexLocker l(&_imp->rotoContextMutex);
        boost::shared_ptr<RotoLayer> deepestLayer = _imp->findDeepestSelectedLayer();


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
    boost::shared_ptr<Bezier> curve( new Bezier(this_shared, name, boost::shared_ptr<RotoLayer>(), isOpenBezier) );
    curve->createNodes();
    
    int indexInLayer = -1;
    if (parentLayer) {
        indexInLayer = 0;
        parentLayer->insertItem(curve,0);
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

boost::shared_ptr<RotoStrokeItem>
RotoContext::makeStroke(Natron::RotoStrokeType type,const std::string& baseName,bool clearSel)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    boost::shared_ptr<RotoLayer> parentLayer;
    boost::shared_ptr<RotoContext> this_shared = boost::dynamic_pointer_cast<RotoContext>(shared_from_this());
    assert(this_shared);
    std::string name = generateUniqueName(baseName);
    
    {
        
        QMutexLocker l(&_imp->rotoContextMutex);
        ++_imp->age; // increase age 
        
        boost::shared_ptr<RotoLayer> deepestLayer = _imp->findDeepestSelectedLayer();
        
        
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
    boost::shared_ptr<RotoStrokeItem> curve( new RotoStrokeItem(type,this_shared, name, boost::shared_ptr<RotoLayer>()) );
    int indexInLayer = -1;
    if (parentLayer) {
        indexInLayer = 0;
        parentLayer->insertItem(curve,0);
    }
    curve->createNodes();
    
    _imp->lastInsertedItem = curve;
    
    Q_EMIT itemInserted(indexInLayer,RotoItem::eSelectionReasonOther);
    
    if (clearSel) {
        clearSelection(RotoItem::eSelectionReasonOther);
        select(curve, RotoItem::eSelectionReasonOther);
    }
    return curve;
}

boost::shared_ptr<Bezier>
RotoContext::makeEllipse(double x,double y,double diameter,bool fromCenter, double time)
{
    double half = diameter / 2.;
    boost::shared_ptr<Bezier> curve = makeBezier(x , fromCenter ? y - half : y ,kRotoEllipseBaseName, time, false);
    if (fromCenter) {
        curve->addControlPoint(x + half,y, time);
        curve->addControlPoint(x,y + half, time);
        curve->addControlPoint(x - half,y, time);
    } else {
        curve->addControlPoint(x + diameter,y - diameter, time);
        curve->addControlPoint(x,y - diameter, time);
        curve->addControlPoint(x - diameter,y - diameter, time);
    }
    
    boost::shared_ptr<BezierCP> top = curve->getControlPointAtIndex(0);
    boost::shared_ptr<BezierCP> right = curve->getControlPointAtIndex(1);
    boost::shared_ptr<BezierCP> bottom = curve->getControlPointAtIndex(2);
    boost::shared_ptr<BezierCP> left = curve->getControlPointAtIndex(3);

    double topX,topY,rightX,rightY,btmX,btmY,leftX,leftY;
    top->getPositionAtTime(false,time, &topX, &topY);
    right->getPositionAtTime(false,time, &rightX, &rightY);
    bottom->getPositionAtTime(false,time, &btmX, &btmY);
    left->getPositionAtTime(false,time, &leftX, &leftY);
    
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

boost::shared_ptr<Bezier>
RotoContext::makeSquare(double x,double y,double initialSize,double time)
{
    boost::shared_ptr<Bezier> curve = makeBezier(x,y,kRotoRectangleBaseName,time, false);
    curve->addControlPoint(x + initialSize,y, time);
    curve->addControlPoint(x + initialSize,y - initialSize, time);
    curve->addControlPoint(x,y - initialSize, time);
    curve->setCurveFinished(true);
    
    return curve;

}

void
RotoContext::removeItemRecursively(const boost::shared_ptr<RotoItem>& item,
                                   RotoItem::SelectionReasonEnum reason)
{
    boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
    boost::shared_ptr<RotoItem> foundSelected;
    
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list< boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
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

        for (std::list<boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
            if (*it == isLayer) {
                _imp->layers.erase(it);
                break;
            }
        }
    }
    Q_EMIT itemRemoved(item,(int)reason);
}

void
RotoContext::removeItem(const boost::shared_ptr<RotoItem>& item,
                        RotoItem::SelectionReasonEnum reason)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    boost::shared_ptr<RotoLayer> layer = item->getParentLayer();
    if (layer) {
        layer->removeItem(item);
    }
    
    removeItemRecursively(item,reason);
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::addItem(const boost::shared_ptr<RotoLayer>& layer,
                     int indexInLayer,
                     const boost::shared_ptr<RotoItem> & item,
                     RotoItem::SelectionReasonEnum reason)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        if (layer) {
            layer->insertItem(item,indexInLayer);
        }
        
        QMutexLocker l(&_imp->rotoContextMutex);

        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
        if (isLayer) {
            std::list<boost::shared_ptr<RotoLayer> >::iterator foundLayer = std::find(_imp->layers.begin(), _imp->layers.end(), isLayer);
            if ( foundLayer == _imp->layers.end() ) {
                _imp->layers.push_back(isLayer);
            }
        }
        _imp->lastInsertedItem = item;
    }
    Q_EMIT itemInserted(indexInLayer,reason);
}


const std::list< boost::shared_ptr<RotoLayer> > &
RotoContext::getLayers() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->layers;
}

boost::shared_ptr<Bezier>
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
    std::list<std::pair<boost::shared_ptr<Bezier>, std::pair<int,double> > > nearbyBeziers;
    for (std::list< boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        const RotoItems & items = (*it)->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            boost::shared_ptr<Bezier> b = boost::dynamic_pointer_cast<Bezier>(*it2);
            if (b && !b->isLockedRecursive()) {
                double param;
                int i = b->isPointOnCurve(x, y, acceptance, &param,feather);
                if (i != -1) {
                    nearbyBeziers.push_back(std::make_pair(b, std::make_pair(i, param)));
                }
            }
        }
    }
    
    std::list<std::pair<boost::shared_ptr<Bezier>, std::pair<int,double> > >::iterator firstNotSelected = nearbyBeziers.end();
    for (std::list<std::pair<boost::shared_ptr<Bezier>, std::pair<int,double> > >::iterator it = nearbyBeziers.begin(); it!=nearbyBeziers.end(); ++it) {
        bool foundSelected = false;
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it2 = _imp->selectedItems.begin(); it2 != _imp->selectedItems.end(); ++it2) {
            if (it2->get() == it->first.get()) {
                foundSelected = true;
                break;
            }
        }
        if (foundSelected) {
            *index = it->second.first;
            *t = it->second.second;
            return it->first;
        } else if (firstNotSelected == nearbyBeziers.end()) {
            firstNotSelected = it;
        }
    }
    if (firstNotSelected != nearbyBeziers.end()) {
        *index = firstNotSelected->second.first;
        *t = firstNotSelected->second.second;
        return firstNotSelected->first;
    }

    return boost::shared_ptr<Bezier>();
}

void
RotoContext::onLifeTimeKnobValueChanged(int /*dim*/, int reason)
{
    if ((ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited) {
        return;
    }
    int lifetime_i = _imp->lifeTime.lock()->getValue();
    _imp->activated.lock()->setSecret(lifetime_i != 3);
    boost::shared_ptr<KnobInt> frame = _imp->lifeTimeFrame.lock();
    frame->setSecret(lifetime_i == 3);
    if (lifetime_i != 3) {
        frame->setValue(getTimelineCurrentTime(), 0);
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
    } else if (shutterType_i == 1) {// start
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
RotoContext::getItemsRegionOfDefinition(const std::list<boost::shared_ptr<RotoItem> >& items, double time, int /*view*/, RectD* rod) const
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
    
    boost::shared_ptr<Node> activeRotoPaintNode;
    boost::shared_ptr<RotoStrokeItem> activeStroke;
    bool isDrawing;
    getNode()->getApp()->getActiveRotoDrawingStroke(&activeRotoPaintNode, &activeStroke,&isDrawing);
    if (!isDrawing) {
        activeStroke.reset();
    }
    
    QMutexLocker l(&_imp->rotoContextMutex);
    for (double t = startTime; t <= endTime; t+= mbFrameStep) {
        bool first = true;
        RectD bbox;
        for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it2->get());
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it2->get());
            if (isBezier && !isStroke) {
                if (isBezier->isActivated(time)  && isBezier->getControlPointsCount() > 1) {
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
                if (isStroke->isActivated(time)) {
                    if (isStroke == activeStroke.get()) {
                        strokeRod = isStroke->getMergeNode()->getPaintStrokeRoD_duringPainting();
                    } else {
                        strokeRod = isStroke->getBoundingBox(t);
                    }
                    if (first) {
                        first = false;
                        bbox= strokeRod;
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

}

void
RotoContext::getMaskRegionOfDefinition(double time,
                                       int view,
                                       RectD* rod) // rod is in canonical coordinates
const
{
    std::list<boost::shared_ptr<RotoItem> > allItems;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        
        for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
            RotoItems items = (*it)->getItems_mt_safe();
            for (RotoItems::iterator it2 = items.begin(); it2 != items.end(); ++it2) {
                allItems.push_back(*it2);
            }
        }
    }
    getItemsRegionOfDefinition(allItems, time, view, rod);
}

bool
RotoContext::isRotoPaintTreeConcatenatableInternal(const std::list<boost::shared_ptr<RotoDrawableItem> >& items)
{
    bool operatorSet = false;
    int comp_i;
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
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
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it->get());
        if (!isStroke) {
            assert(dynamic_cast<Bezier*>(it->get()));
        } else {
            if (isStroke->getBrushType() != Natron::eRotoStrokeTypeSolid) {
                return false;
            }
        }
        
    }
    return true;
}

bool
RotoContext::isRotoPaintTreeConcatenatable() const
{
    std::list<boost::shared_ptr<RotoDrawableItem> > items = getCurvesByRenderOrder();
    return isRotoPaintTreeConcatenatableInternal(items);
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
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        obj->_selectedItems.push_back( (*it)->getScriptName() );
    }
}

static void
linkItemsKnobsRecursively(RotoContext* ctx,
                          const boost::shared_ptr<RotoLayer> & layer)
{
    const RotoItems & items = layer->getItems();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);

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

    for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        it->lock()->setDefaultAllDimensionsEnabled(false);
    }

    assert(_imp->layers.size() == 1);

    boost::shared_ptr<RotoLayer> baseLayer = _imp->layers.front();

    baseLayer->load(obj._baseLayer);

    for (std::list<std::string>::const_iterator it = obj._selectedItems.begin(); it != obj._selectedItems.end(); ++it) {
        boost::shared_ptr<RotoItem> item = getItemByName(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
        if (isBezier) {
            select(isBezier,RotoItem::eSelectionReasonOther);
        } else if (isLayer) {
            linkItemsKnobsRecursively(this, isLayer);
        }
    }
    _imp->isCurrentlyLoading = false;
    refreshRotoPaintTree();
}

void
RotoContext::select(const boost::shared_ptr<RotoItem> & b,
                    RotoItem::SelectionReasonEnum reason)
{
    
    selectInternal(b);
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::select(const std::list<boost::shared_ptr<RotoDrawableItem> > & beziers,
                    RotoItem::SelectionReasonEnum reason)
{
    
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
        selectInternal(*it);
    }
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::select(const std::list<boost::shared_ptr<RotoItem> > & items,
                    RotoItem::SelectionReasonEnum reason)
{
    
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        selectInternal(*it);
    }
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const boost::shared_ptr<RotoItem> & b,
                      RotoItem::SelectionReasonEnum reason)
{
    
    deselectInternal(b);
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const std::list<boost::shared_ptr<Bezier> > & beziers,
                      RotoItem::SelectionReasonEnum reason)
{
    
    for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
        deselectInternal(*it);
    }
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const std::list<boost::shared_ptr<RotoItem> > & items,
                      RotoItem::SelectionReasonEnum reason)
{
    
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        deselectInternal(*it);
    }
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::clearAndSelectPreviousItem(const boost::shared_ptr<RotoItem>& item,RotoItem::SelectionReasonEnum reason)
{
    clearSelection(reason);
    assert(item);
    boost::shared_ptr<RotoItem> prev = item->getPreviousItemInLayer();
    if (prev) {
        select(prev,reason);
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
        boost::shared_ptr<RotoItem> item;
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
RotoContext::selectInternal(const boost::shared_ptr<RotoItem> & item)
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
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it->get());
            if (!isStroke && isBezier && !isBezier->isLockedRecursive()) {
                if (isBezier->isOpenBezier()) {
                    ++nbUnlockedStrokes;
                } else {
                    ++nbUnlockedBeziers;
                }
            } else if (isStroke) {
                ++nbUnlockedStrokes;
            }
            if (it->get() == item.get()) {
                foundItem = true;
            }
        }
    }
    ///the item is already selected, exit
    if (foundItem) {
        return;
    }
    
    
    boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);
    boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(item);
    RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>(item.get());
    boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);

    if (isDrawable) {
        if (!isStroke && isBezier && !isBezier->isLockedRecursive()) {
            if (isBezier->isOpenBezier()) {
                ++nbUnlockedStrokes;
            } else {
                ++nbUnlockedBeziers;
            }
            ++nbStrokeWithoutCloneFunctions;
            ++nbStrokeWithoutStrength;
        } else if (isStroke) {
            ++nbUnlockedStrokes;
            if (isStroke->getBrushType() != eRotoStrokeTypeBlur && isStroke->getBrushType() != eRotoStrokeTypeSharpen) {
                ++nbStrokeWithoutStrength;
            }
            if (isStroke->getBrushType() != eRotoStrokeTypeClone && isStroke->getBrushType() != eRotoStrokeTypeReveal) {
                ++nbStrokeWithoutCloneFunctions;
            }
        }
        
        const std::list<boost::shared_ptr<KnobI> >& drawableKnobs = isDrawable->getKnobs();
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = drawableKnobs.begin(); it != drawableKnobs.end(); ++it) {
            
            for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
                boost::shared_ptr<KnobI> thisKnob = it2->lock();
                if (thisKnob->getName() == (*it)->getName()) {
                    
                    //Clone current state
                    thisKnob->cloneAndUpdateGui(it->get());
                    
                    //Slave internal knobs of the bezier
                    assert((*it)->getDimension() == thisKnob->getDimension());
                    for (int i = 0; i < (*it)->getDimension(); ++i) {
                        (*it)->slaveTo(i, thisKnob, i);
                    }
                    
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,int,int,bool)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,int,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,double,double)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));

                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));

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
    if (nbUnlockedBeziers > 0 || nbUnlockedStrokes > 0) {
        
        boost::shared_ptr<KnobI> strengthKnob = _imp->brushEffectKnob.lock();
        boost::shared_ptr<KnobI> sourceTypeKnob = _imp->sourceTypeKnob.lock();
        boost::shared_ptr<KnobI> timeOffsetKnob = _imp->timeOffsetKnob.lock();
        boost::shared_ptr<KnobI> timeOffsetModeKnob = _imp->timeOffsetModeKnob.lock();
        for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
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
                    for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->cloneKnobs.begin(); it2!=_imp->cloneKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isCloneKnob = true;
                        }
                    }
                    mustDisable |= isCloneKnob;
                }
                if (nbUnlockedBeziers && !mustDisable) {
                    bool isStrokeKnob = false;
                    for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->strokeKnobs.begin(); it2!=_imp->strokeKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isStrokeKnob = true;
                        }
                    }
                    mustDisable |= isStrokeKnob;
                }
                if (nbUnlockedStrokes && !mustDisable) {
                    bool isBezierKnob = false;
                    for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->shapeKnobs.begin(); it2!=_imp->shapeKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isBezierKnob = true;
                        }
                    }
                    mustDisable |= isBezierKnob;
                }
                k->setAllDimensionsEnabled(!mustDisable);
                
            }
            if (nbUnlockedBeziers >= 2 || nbUnlockedStrokes >= 2) {
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
RotoContext::deselectInternal(boost::shared_ptr<RotoItem> b)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    int nbBeziersUnLockedBezier = 0;
    int nbStrokesUnlocked = 0;
    {
        QMutexLocker l(&_imp->rotoContextMutex);

        std::list<boost::shared_ptr<RotoItem> >::iterator foundSelected = std::find(_imp->selectedItems.begin(),_imp->selectedItems.end(),b);
        
        ///if the item is not selected, exit
        if ( foundSelected == _imp->selectedItems.end() ) {
            return;
        }
        
        _imp->selectedItems.erase(foundSelected);
        
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it->get());
            if (!isStroke && isBezier && !isBezier->isLockedRecursive()) {
                ++nbBeziersUnLockedBezier;
            } else if (isStroke) {
                ++nbStrokesUnlocked;
            }
        }
    }
    

    
    bool bezierDirty = nbBeziersUnLockedBezier > 1;
    bool strokeDirty = nbStrokesUnlocked > 1;
    boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(b);
    RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>(b.get());
    boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(b);
    boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(b);
    if (isDrawable) {
        ///first-off set the context knobs to the value of this bezier
        
        const std::list<boost::shared_ptr<KnobI> >& drawableKnobs = isDrawable->getKnobs();
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = drawableKnobs.begin(); it != drawableKnobs.end(); ++it) {
            
            for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
                boost::shared_ptr<KnobI> knob = it2->lock();
                if (knob->getName() == (*it)->getName()) {
                    
                    //Clone current state
                    knob->cloneAndUpdateGui(it->get());
                    
                    //Slave internal knobs of the bezier
                    assert((*it)->getDimension() == knob->getDimension());
                    for (int i = 0; i < (*it)->getDimension(); ++i) {
                        (*it)->unSlave(i,isBezier ? !bezierDirty : !strokeDirty);
                    }
                    
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,int,int,bool)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,int,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,double,double)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,int)),
                                        this, SLOT(onSelectedKnobCurveChanged()));
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
    
    if (nbBeziersUnLockedBezier == 0 || nbStrokesUnlocked == 0) {
        for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
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
RotoContext::resetTransformsCenter(bool doClone, bool doTransform)
{
    double time = getNode()->getApp()->getTimeLine()->currentFrame();
    RectD bbox;
    getItemsRegionOfDefinition(getSelectedItems(),time, 0, &bbox);
    if (doTransform) {
        boost::shared_ptr<KnobDouble> centerKnob = _imp->centerKnob.lock();
        centerKnob->beginChanges();
        dynamic_cast<KnobI*>(centerKnob.get())->removeAnimation(0);
        dynamic_cast<KnobI*>(centerKnob.get())->removeAnimation(1);
        centerKnob->setValues((bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2., eValueChangedReasonNatronInternalEdited);
        centerKnob->endChanges();
    }
    if (doClone) {
        boost::shared_ptr<KnobDouble> centerKnob = _imp->cloneCenterKnob.lock();
        centerKnob->beginChanges();
        dynamic_cast<KnobI*>(centerKnob.get())->removeAnimation(0);
        dynamic_cast<KnobI*>(centerKnob.get())->removeAnimation(1);
        centerKnob->setValues((bbox.x1 + bbox.x2) / 2., (bbox.y1 + bbox.y2) / 2., eValueChangedReasonNatronInternalEdited);
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
RotoContext::resetTransformInternal(const boost::shared_ptr<KnobDouble>& translate,
                                    const boost::shared_ptr<KnobDouble>& scale,
                                    const boost::shared_ptr<KnobDouble>& center,
                                    const boost::shared_ptr<KnobDouble>& rotate,
                                    const boost::shared_ptr<KnobDouble>& skewX,
                                    const boost::shared_ptr<KnobDouble>& skewY,
                                    const boost::shared_ptr<KnobBool>& scaleUniform,
                                    const boost::shared_ptr<KnobChoice>& skewOrder)
{
    std::list<KnobI*> knobs;
    knobs.push_back(translate.get());
    knobs.push_back(scale.get());
    knobs.push_back(center.get());
    knobs.push_back(rotate.get());
    knobs.push_back(skewX.get());
    knobs.push_back(skewY.get());
    knobs.push_back(scaleUniform.get());
    knobs.push_back(skewOrder.get());
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
    boost::shared_ptr<KnobDouble> translate = _imp->translateKnob.lock();
    boost::shared_ptr<KnobDouble> center = _imp->centerKnob.lock();
    boost::shared_ptr<KnobDouble> scale = _imp->scaleKnob.lock();
    boost::shared_ptr<KnobDouble> rotate = _imp->rotateKnob.lock();
    boost::shared_ptr<KnobBool> uniform = _imp->scaleUniformKnob.lock();
    boost::shared_ptr<KnobDouble> skewX = _imp->skewXKnob.lock();
    boost::shared_ptr<KnobDouble> skewY = _imp->skewYKnob.lock();
    boost::shared_ptr<KnobChoice> skewOrder = _imp->skewOrderKnob.lock();
    resetTransformInternal(translate, scale, center, rotate, skewX, skewY, uniform, skewOrder);

}

void
RotoContext::resetCloneTransform()
{
    boost::shared_ptr<KnobDouble> translate = _imp->cloneTranslateKnob.lock();
    boost::shared_ptr<KnobDouble> center = _imp->cloneCenterKnob.lock();
    boost::shared_ptr<KnobDouble> scale = _imp->cloneScaleKnob.lock();
    boost::shared_ptr<KnobDouble> rotate = _imp->cloneRotateKnob.lock();
    boost::shared_ptr<KnobBool> uniform = _imp->cloneUniformKnob.lock();
    boost::shared_ptr<KnobDouble> skewX = _imp->cloneSkewXKnob.lock();
    boost::shared_ptr<KnobDouble> skewY = _imp->cloneSkewYKnob.lock();
    boost::shared_ptr<KnobChoice> skewOrder = _imp->cloneSkewOrderKnob.lock();
    resetTransformInternal(translate, scale, center, rotate, skewX, skewY, uniform, skewOrder);
}

void
RotoContext::knobChanged(KnobI* k,
                 ValueChangedReasonEnum /*reason*/,
                 int /*view*/,
                 double /*time*/,
                 bool /*originatedFromMainThread*/)
{
    if (k == _imp->resetCenterKnob.lock().get()) {
        resetTransformCenter();
    } else if (k == _imp->resetCloneCenterKnob.lock().get()) {
        resetCloneTransformCenter();
    } else if (k == _imp->resetTransformKnob.lock().get()) {
        resetTransform();
    } else if (k == _imp->resetCloneTransformKnob.lock().get()) {
        resetCloneTransform();
    }
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    else if (k == _imp->motionBlurTypeKnob.lock().get()) {
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
}

boost::shared_ptr<RotoItem>
RotoContext::getLastItemLocked() const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->lastLockedItem;
}

static void
addOrRemoveKeyRecursively(const boost::shared_ptr<RotoLayer>& isLayer,
                          double time,
                          bool add,
                          bool removeAll)
{
    const RotoItems & items = isLayer->getItems();

    for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it2);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it2);
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
            addOrRemoveKeyRecursively(layer, time, add,removeAll);
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
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            isBezier->setKeyframe(time);
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer,time, true, false);
        }
    }
}

void
RotoContext::removeAnimationOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    double time = getTimelineCurrentTime();
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            isBezier->removeAnimation();
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer,time, false, true);
        }
    }
    if (!_imp->selectedItems.empty()) {
        evaluateChange();
    }
}

void
RotoContext::removeKeyframeOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            isBezier->removeKeyframe(time);
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer,time, false, false);
        }
    }
}

static void
findOutNearestKeyframeRecursively(const boost::shared_ptr<RotoLayer>& layer,
                                  bool previous,
                                  double time,
                                  int* nearest)
{
    const RotoItems & items = layer->getItems();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
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
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it);
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (isBezier) {
                int t = isBezier->getPreviousKeyframeTime(time);
                if ( (t != INT_MIN) && (t > minimum) ) {
                    minimum = t;
                }
            } else {
                assert(layer);
                if (layer) {
                    findOutNearestKeyframeRecursively(layer, true,time,&minimum);
                }
            }
        }
    }

    if (minimum != INT_MIN) {
        getNode()->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Node>());
        getNode()->getApp()->getTimeLine()->seekFrame(minimum, false,  NULL, Natron::eTimelineChangeReasonOtherSeek);
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
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
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
        getNode()->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Node>());
        getNode()->getApp()->getTimeLine()->seekFrame(maximum, false, NULL,Natron::eTimelineChangeReasonOtherSeek);
        
    }
}

static void
appendToSelectedCurvesRecursively(std::list< boost::shared_ptr<RotoDrawableItem> > * curves,
                                  const boost::shared_ptr<RotoLayer>& isLayer,
                                  double time,
                                  bool onlyActives,
                                  bool addStrokes)
{
    RotoItems items = isLayer->getItems_mt_safe();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<RotoDrawableItem> isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(isDrawable.get());
        if (isStroke && !addStrokes) {
            continue;
        }
        if (isDrawable) {
            if ( !onlyActives || isDrawable->isActivated(time) ) {
                curves->push_front(isDrawable);
            }
        } else if ( layer && layer->isGloballyActivated() ) {
            appendToSelectedCurvesRecursively(curves, layer, time, onlyActives ,addStrokes);
        }
    }
}

const std::list< boost::shared_ptr<RotoItem> > &
RotoContext::getSelectedItems() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->selectedItems;
}

std::list< boost::shared_ptr<RotoDrawableItem> >
RotoContext::getSelectedCurves() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    std::list< boost::shared_ptr<RotoDrawableItem> > drawables;
    double time = getTimelineCurrentTime();
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            assert(*it);
            boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
            boost::shared_ptr<RotoDrawableItem> isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);
            if (isDrawable) {
                drawables.push_back(isDrawable);
            } else {
                assert(isLayer);
                if (isLayer) {
                    appendToSelectedCurvesRecursively(&drawables, isLayer,time,false, true);
                }
            }
        }
    }
    return drawables;
}

std::list< boost::shared_ptr<RotoDrawableItem> >
RotoContext::getCurvesByRenderOrder(bool onlyActivated) const
{
    std::list< boost::shared_ptr<RotoDrawableItem> > ret;
    
    ///Note this might not be the timeline's current frame if this is a render thread.
    double time = getNode()->getLiveInstance()->getCurrentTime();
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
    std::list< boost::shared_ptr<RotoDrawableItem> > curves = getCurvesByRenderOrder();
    return (int)curves.size();
}

boost::shared_ptr<RotoLayer>
RotoContext::getLayerByName(const std::string & n) const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        if ( (*it)->getScriptName() == n ) {
            return *it;
        }
    }

    return boost::shared_ptr<RotoLayer>();
}

static void
findItemRecursively(const std::string & n,
                    const boost::shared_ptr<RotoLayer> & layer,
                    boost::shared_ptr<RotoItem>* ret)
{
    if (layer->getScriptName() == n) {
        *ret = boost::dynamic_pointer_cast<RotoItem>(layer);
    } else {
        const RotoItems & items = layer->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it2);
            if ( (*it2)->getScriptName() == n ) {
                *ret = *it2;

                return;
            } else if (isLayer) {
                findItemRecursively(n, isLayer, ret);
            }
        }
    }
}

boost::shared_ptr<RotoItem>
RotoContext::getItemByName(const std::string & n) const
{
    boost::shared_ptr<RotoItem> ret;
    QMutexLocker l(&_imp->rotoContextMutex);

    for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        findItemRecursively(n, *it, &ret);
    }

    return ret;
}

boost::shared_ptr<RotoLayer>
RotoContext::getDeepestSelectedLayer() const
{
    QMutexLocker l(&_imp->rotoContextMutex);
    return findDeepestSelectedLayer();
}

boost::shared_ptr<RotoLayer>
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
    _imp->node.lock()->getLiveInstance()->abortAnyEvaluation();
    getNode()->getLiveInstance()->evaluate_public(NULL, true,eValueChangedReasonUserEdited);
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
RotoContext::onItemLockedChanged(const boost::shared_ptr<RotoItem>& item, RotoItem::SelectionReasonEnum reason)
{
    assert(item);
    ///refresh knobs
    int nbBeziersUnLockedBezier = 0;

    {
        QMutexLocker l(&_imp->rotoContextMutex);

        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if ( isBezier && !isBezier->isLockedRecursive() ) {
                ++nbBeziersUnLockedBezier;
            }
        }
    }
    bool dirty = nbBeziersUnLockedBezier > 1;
    bool enabled = nbBeziersUnLockedBezier > 0;

    for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it!=_imp->knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->lock();
        if (!knob) {
            continue;
        }
        knob->setDirty(dirty);
        knob->setAllDimensionsEnabled(enabled);
    }
    _imp->lastLockedItem = item;
    Q_EMIT itemLockedChanged((int)reason);
}

void
RotoContext::onItemScriptNameChanged(const boost::shared_ptr<RotoItem>& item)
{
    Q_EMIT itemScriptNameChanged(item);
}

void
RotoContext::onItemLabelChanged(const boost::shared_ptr<RotoItem>& item)
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
    std::list< boost::shared_ptr<RotoDrawableItem> > splines = getCurvesByRenderOrder();

    for (std::list< boost::shared_ptr<RotoDrawableItem> > ::iterator it = splines.begin(); it != splines.end(); ++it) {
        std::set<double> splineKeys;
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (!isBezier) {
            continue;
        }
        isBezier->getKeyframeTimes(&splineKeys);
        for (std::set<double>::iterator it2 = splineKeys.begin(); it2 != splineKeys.end(); ++it2) {
            times->push_back(*it2);
        }
    }
}

static void dequeueActionForLayer(RotoLayer* layer, bool *evaluate)
{
    RotoItems items = layer->getItems_mt_safe();

    for (RotoItems::iterator it = items.begin(); it!=items.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
        if (isBezier) {
            *evaluate = *evaluate | isBezier->dequeueGuiActions();
        } else if (isLayer) {
            dequeueActionForLayer(isLayer,evaluate);
        }
        
    }
}

void
RotoContext::dequeueGuiActions()
{
    bool evaluate = false;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        if (!_imp->layers.empty()) {
            dequeueActionForLayer(_imp->layers.front().get(),&evaluate);
        }
    }
    if (evaluate) {
        evaluateChange();
    }
    
}

boost::shared_ptr<KnobChoice>
RotoContext::getMotionBlurTypeKnob() const
{
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    return _imp->motionBlurTypeKnob.lock();
#else
    return boost::shared_ptr<KnobChoice>();
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
    bool wasDoingNeatRender = _imp->doingNeatRender;
    if (doing && _imp->mustDoNeatRender) {
        _imp->doingNeatRender = true;
        _imp->mustDoNeatRender = false;
    } else {
        _imp->doingNeatRender = false;
    }
    if (!doing && wasDoingNeatRender) {
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
            _imp->doingNeatRenderCond.wait(&_imp->doingNeatRenderMutex);
        }
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

template <typename PIX,int maxValue, int dstNComps, int srcNComps, bool useOpacity>
static void
convertCairoImageToNatronImageForDstComponents_noColor(cairo_surface_t* cairoImg,
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
    
    for (int y = 0; y < pixelRod.height(); ++y, srcPix += stride) {
        PIX* dstPix = (PIX*)acc.pixelAt(pixelRod.x1, pixelRod.y1 + y);
        assert(dstPix);
        
        for (int x = 0; x < pixelRod.width(); ++x) {
            float cairoPixel = (float)srcPix[x * srcNComps] / 255.f;
            switch (dstNComps) {
                case 4:
                    dstPix[x * dstNComps + 0] = PIX(cairoPixel * r * maxValue);
                    assert(!boost::math::isnan(dstPix[x * dstNComps + 0]));
                    dstPix[x * dstNComps + 1] = PIX(cairoPixel * g * maxValue);
                    assert(!boost::math::isnan(dstPix[x * dstNComps + 1]));
                    dstPix[x * dstNComps + 2] = PIX(cairoPixel * b * maxValue);
                    assert(!boost::math::isnan(dstPix[x * dstNComps + 2]));
                    dstPix[x * dstNComps + 3] = useOpacity ? PIX(cairoPixel * opacity * maxValue) : PIX(cairoPixel * maxValue);
                    assert(!boost::math::isnan(dstPix[x * dstNComps + 3]));
                    break;
                case 1:
                    dstPix[x] = useOpacity ? PIX(cairoPixel * opacity * maxValue) : PIX(cairoPixel * maxValue);
                    assert(!boost::math::isnan(dstPix[x]));
                    break;
                case 3:
                    dstPix[x * dstNComps + 0] = PIX(cairoPixel * r * maxValue);
                    dstPix[x * dstNComps + 1] = PIX(cairoPixel * g * maxValue);
                    dstPix[x * dstNComps + 2] = PIX(cairoPixel * b * maxValue);
                    break;
                case 2:
                    dstPix[x * dstNComps + 0] = PIX(cairoPixel * r * maxValue);
                    dstPix[x * dstNComps + 1] = PIX(cairoPixel * g * maxValue);
                    break;

                default:
                    break;
            }
#         ifdef DEBUG
            for (int c = 0; c < dstNComps; ++c) {
                assert(dstPix[x * dstNComps + c] == dstPix[x * dstNComps + c]); // check for NaN
            }
#         endif
        }
    }

}

template <typename PIX,int maxValue, int dstNComps, int srcNComps>
static void
convertCairoImageToNatronImageForOpacity(cairo_surface_t* cairoImg,
                                         Image* image,
                                         const RectI & pixelRod,
                                         double shapeColor[3],
                                         double opacity,
                                         bool useOpacity)
{
    if (useOpacity) {
        convertCairoImageToNatronImageForDstComponents_noColor<PIX,maxValue,dstNComps, srcNComps, true>(cairoImg, image,pixelRod, shapeColor, opacity);
    } else {
        convertCairoImageToNatronImageForDstComponents_noColor<PIX,maxValue,dstNComps, srcNComps, false>(cairoImg, image,pixelRod, shapeColor, opacity);
    }

}


template <typename PIX,int maxValue, int dstNComps>
static void
convertCairoImageToNatronImageForSrcComponents_noColor(cairo_surface_t* cairoImg,
                                                       int srcNComps,
                                                       Image* image,
                                                       const RectI & pixelRod,
                                                       double shapeColor[3],
                                                       double opacity,
                                                       bool useOpacity)
{
    if (srcNComps == 1) {
        convertCairoImageToNatronImageForOpacity<PIX,maxValue,dstNComps, 1>(cairoImg, image,pixelRod, shapeColor, opacity, useOpacity);
    } else if (srcNComps == 4) {
        convertCairoImageToNatronImageForOpacity<PIX,maxValue,dstNComps, 4>(cairoImg, image,pixelRod, shapeColor, opacity, useOpacity);
    } else {
        assert(false);
    }
}

template <typename PIX,int maxValue>
static void
convertCairoImageToNatronImage_noColor(cairo_surface_t* cairoImg,
                                       int srcNComps,
                                       Image* image,
                                       const RectI & pixelRod,
                                       double shapeColor[3],
                                       double opacity,
                                       bool useOpacity)
{
    int comps = (int)image->getComponentsCount();
    switch (comps) {
        case 1:
            convertCairoImageToNatronImageForSrcComponents_noColor<PIX,maxValue,1>(cairoImg, srcNComps, image,pixelRod, shapeColor, opacity, useOpacity);
            break;
        case 2:
            convertCairoImageToNatronImageForSrcComponents_noColor<PIX,maxValue,2>(cairoImg, srcNComps, image,pixelRod, shapeColor, opacity, useOpacity);
            break;
        case 3:
            convertCairoImageToNatronImageForSrcComponents_noColor<PIX,maxValue,3>(cairoImg, srcNComps, image,pixelRod, shapeColor, opacity, useOpacity);
            break;
        case 4:
            convertCairoImageToNatronImageForSrcComponents_noColor<PIX,maxValue,4>(cairoImg, srcNComps, image,pixelRod, shapeColor, opacity, useOpacity);
            break;
        default:
            break;
    }
}

template <typename PIX,int maxValue, int srcNComps, int dstNComps>
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
                    dstPix[x * dstNComps + 3] = PIX( (float)srcPix[x * pixelSize + 3] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 2] = PIX( (float)srcPix[x * pixelSize + 0] / 255.f ) * maxValue;
                    break;
                case 1:
                    assert(srcNComps == dstNComps);
                    dstPix[x] = PIX( (float)srcPix[x] / 255.f ) * maxValue;
                    break;
                case 3:
                    assert(srcNComps == dstNComps);
                    dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 2] = PIX( (float)srcPix[x * pixelSize + 0] / 255.f ) * maxValue;
                    break;
                case 2:
                    assert(srcNComps == 3);
                    dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] / 255.f ) * maxValue;
                    break;
                default:
                    break;
            }
#         ifdef DEBUG
            for (int c = 0; c < dstNComps; ++c) {
                assert(dstPix[x * dstNComps + c] == dstPix[x * dstNComps + c]); // check for NaN
            }
#         endif
        }
    }
    
}

template <typename PIX,int maxValue, int srcNComps>
static void
convertCairoImageToNatronImageForSrcComponents(cairo_surface_t* cairoImg,
                                               Image* image,
                                               const RectI & pixelRod)
{
    

    int comps = (int)image->getComponentsCount();
    switch (comps) {
        case 1:
            convertCairoImageToNatronImageForDstComponents<PIX,maxValue, srcNComps, 1>(cairoImg,image,pixelRod);
            break;
        case 2:
            convertCairoImageToNatronImageForDstComponents<PIX,maxValue, srcNComps, 2>(cairoImg,image,pixelRod);
            break;
        case 3:
            convertCairoImageToNatronImageForDstComponents<PIX,maxValue, srcNComps, 3>(cairoImg,image,pixelRod);
            break;
        case 4:
            convertCairoImageToNatronImageForDstComponents<PIX,maxValue, srcNComps, 4>(cairoImg,image,pixelRod);
            break;
        default:
            break;
    }
}


template <typename PIX,int maxValue>
static void
convertCairoImageToNatronImage(cairo_surface_t* cairoImg,
                               Image* image,
                               const RectI & pixelRod,
                               int srcNComps)
{
    switch (srcNComps) {
        case 1:
            convertCairoImageToNatronImageForSrcComponents<PIX,maxValue,1>(cairoImg,image,pixelRod);
            break;
        case 2:
            convertCairoImageToNatronImageForSrcComponents<PIX,maxValue,2>(cairoImg,image,pixelRod);
            break;
        case 3:
            convertCairoImageToNatronImageForSrcComponents<PIX,maxValue,3>(cairoImg,image,pixelRod);
            break;
        case 4:
            convertCairoImageToNatronImageForSrcComponents<PIX,maxValue,4>(cairoImg,image,pixelRod);
            break;
        default:
            break;
    }
}

template <typename PIX,int maxValue, int srcNComps, int dstNComps>
static void
convertNatronImageToCairoImageForComponents(unsigned char* cairoImg,
                                            std::size_t stride,
                                            Image* image,
                                            const RectI& roi,
                                            const RectI& dstBounds,
                                            double shapeColor[3])
{
    unsigned char* dstPix = cairoImg;
    dstPix += ((roi.y1 - dstBounds.y1) * stride + (roi.x1 - dstBounds.x1));
    
    Image::ReadAccess acc = image->getReadRights();
    
    for (int y = 0; y < roi.height(); ++y, dstPix += stride) {
        
        const PIX* srcPix = (const PIX*)acc.pixelAt(roi.x1, roi.y1 + y);
        assert(srcPix);
        
        for (int x = 0; x < roi.width(); ++x) {
#         ifdef DEBUG
            for (int c = 0; c < srcNComps; ++c) {
                assert(srcPix[x * srcNComps + c] == srcPix[x * srcNComps + c]); // check for NaN
            }
#         endif
            if (dstNComps == 1) {
                dstPix[x] = (float)srcPix[x * srcNComps] / maxValue * 255.f;
            } else if (dstNComps == 4) {
                if (srcNComps == 4) {
                    //We are in the !buildUp case, do exactly the opposite that is done in convertNatronImageToCairoImageForComponents
                    dstPix[x * dstNComps + 0] = shapeColor[2] == 0 ? 0 : (float)(srcPix[x * srcNComps + 2] / maxValue) / shapeColor[2] * 255.f;
                    dstPix[x * dstNComps + 1] = shapeColor[1] == 0 ? 0 : (float)(srcPix[x * srcNComps + 1] / maxValue) / shapeColor[1] * 255.f;
                    dstPix[x * dstNComps + 2] = shapeColor[0] == 0 ? 0 : (float)(srcPix[x * srcNComps + 0] / maxValue) / shapeColor[0] * 255.f;
                    dstPix[x * dstNComps + 3] = 255;//(float)srcPix[x * srcNComps + 3] / maxValue * 255.f;
                } else {
                    assert(srcNComps == 1);
                    float pix = (float)srcPix[x];
                    dstPix[x * dstNComps + 0] = pix / maxValue * 255.f;
                    dstPix[x * dstNComps + 1] = pix / maxValue * 255.f;
                    dstPix[x * dstNComps + 2] = pix / maxValue * 255.f;
                    dstPix[x * dstNComps + 3] = pix / maxValue * 255.f;
                }
            }
            // no need to check for NaN, dstPix is unsigned char
        }
    }

}

template <typename PIX,int maxValue, int srcComps>
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
        convertNatronImageToCairoImageForComponents<PIX,maxValue, srcComps, 1>(cairoImg, stride, image, roi, dstBounds,shapeColor);
    } else if (dstNComps == 4) {
        convertNatronImageToCairoImageForComponents<PIX,maxValue, srcComps, 4>(cairoImg, stride, image, roi, dstBounds,shapeColor);
    } else {
        assert(false);
    }
}

template <typename PIX,int maxValue>
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
            convertNatronImageToCairoImageForSrcComponents<PIX,maxValue, 1>(cairoImg, dstNComps, stride, image, roi, dstBounds,shapeColor);
            break;
        case 2:
            convertNatronImageToCairoImageForSrcComponents<PIX,maxValue, 2>(cairoImg, dstNComps,stride, image, roi, dstBounds,shapeColor);
            break;
        case 3:
            convertNatronImageToCairoImageForSrcComponents<PIX,maxValue, 3>(cairoImg, dstNComps,stride, image, roi, dstBounds,shapeColor);
            break;
        case 4:
            convertNatronImageToCairoImageForSrcComponents<PIX,maxValue, 4>(cairoImg, dstNComps,stride, image, roi, dstBounds,shapeColor);
            break;
        default:
            break;
    }
}

double
RotoContext::renderSingleStroke(const boost::shared_ptr<RotoStrokeItem>& stroke,
                                const RectD& pointsBbox,
                                const std::list<std::pair<Point,double> >& points,
                                unsigned int mipmapLevel,
                                double par,
                                const ImageComponents& components,
                                ImageBitDepthEnum depth,
                                double distToNext,
                                boost::shared_ptr<Image> *image)
{
    
    double time = getTimelineCurrentTime();

    double shapeColor[3];
    stroke->getColor(time, shapeColor);
    
    boost::shared_ptr<Image> source = *image;
    RectI pixelPointsBbox;
    pointsBbox.toPixelEnclosing(mipmapLevel, par, &pixelPointsBbox);
    
    bool copyFromImage = false;
    bool mipMapLevelChanged = false;
    if (!source) {
        source.reset(new Image(components,
                                    pointsBbox,
                                    pixelPointsBbox,
                                    mipmapLevel,
                                    par,
                                    depth, false));
        *image = source;
    } else {
        
        if ((*image)->getMipMapLevel() > mipmapLevel) {
            
            mipMapLevelChanged = true;
            
            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds;
            otherRoD.toPixelEnclosing((*image)->getMipMapLevel(), par, &oldBounds);
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            RectI mergeBounds;
            mergeRoD.toPixelEnclosing(mipmapLevel, par, &mergeBounds);
            
            
            //upscale the original image
            source.reset(new Image(components,
                                        mergeRoD,
                                        mergeBounds,
                                        mipmapLevel,
                                        par,
                                        depth, false));
            source->fillZero(pixelPointsBbox);
            (*image)->upscaleMipMap(oldBounds, (*image)->getMipMapLevel(), source->getMipMapLevel(), source.get());
            *image = source;
        } else if ((*image)->getMipMapLevel() < mipmapLevel) {
            mipMapLevelChanged = true;
        
            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds;
            otherRoD.toPixelEnclosing((*image)->getMipMapLevel(), par, &oldBounds);
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            RectI mergeBounds;
            mergeRoD.toPixelEnclosing(mipmapLevel, par, &mergeBounds);
            
            //downscale the original image
            source.reset(new Image(components,
                                        mergeRoD,
                                        mergeBounds,
                                        mipmapLevel,
                                        par,
                                        depth, false));
            source->fillZero(pixelPointsBbox);
            (*image)->downscaleMipMap(pointsBbox, oldBounds, (*image)->getMipMapLevel(), source->getMipMapLevel(), false, source.get());
            *image = source;
        } else {
            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds = (*image)->getBounds();
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            source->setRoD(mergeRoD);
            source->ensureBounds(pixelPointsBbox,true);

        }
        copyFromImage = true;
    }

    bool doBuildUp = stroke->getBuildupKnob()->getValueAtTime(time);

    
    cairo_format_t cairoImgFormat;
    
    int srcNComps;
    //For the non build-up case, we use the LIGHTEN compositing operator, which only works on colors
    if (!doBuildUp || components.getNumComponents() > 1) {
        cairoImgFormat = CAIRO_FORMAT_ARGB32;
        srcNComps = 4;
    } else {
        cairoImgFormat = CAIRO_FORMAT_A8;
        srcNComps = 1;
    }
    
    
    ////Allocate the cairo temporary buffer
    cairo_surface_t* cairoImg;

    std::vector<unsigned char> buf;
    if (copyFromImage) {
        std::size_t stride = cairo_format_stride_for_width(cairoImgFormat, pixelPointsBbox.width());
        std::size_t memSize = stride * pixelPointsBbox.height();
        buf.resize(memSize);
        memset(&buf.front(), 0, sizeof(unsigned char) * memSize);
        convertNatronImageToCairoImage<float, 1>(&buf.front(), srcNComps, stride, source.get(), pixelPointsBbox, pixelPointsBbox, shapeColor);
        cairoImg = cairo_image_surface_create_for_data(&buf.front(), cairoImgFormat, pixelPointsBbox.width(), pixelPointsBbox.height(),
                                                       stride);
       
    } else {
        cairoImg = cairo_image_surface_create(cairoImgFormat, pixelPointsBbox.width(), pixelPointsBbox.height() );
        cairo_surface_set_device_offset(cairoImg, -pixelPointsBbox.x1, -pixelPointsBbox.y1);
    }
    if (cairo_surface_status(cairoImg) != CAIRO_STATUS_SUCCESS) {
        return 0;
    }
    cairo_surface_set_device_offset(cairoImg, -pixelPointsBbox.x1, -pixelPointsBbox.y1);
    cairo_t* cr = cairo_create(cairoImg);
    //cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD); // creates holes on self-overlapping shapes
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    
    // these Roto shapes must be rendered WITHOUT antialias, or the junction between the inner
    // polygon and the feather zone will have artifacts. This is partly due to the fact that cairo
    // meshes are not antialiased.
    // Use a default feather distance of 1 pixel instead!
    // UPDATE: unfortunately, this produces less artifacts, but there are still some remaining (use opacity=0.5 to test)
    // maybe the inner polygon should be made of mesh patterns too?
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    
    std::list<std::list<std::pair<Point,double> > > strokes;
    std::list<std::pair<Point,double> > toScalePoints;
    int pot = 1 << mipmapLevel;
    if (mipmapLevel == 0) {
        toScalePoints = points;
    } else {
        for (std::list<std::pair<Point,double> >::const_iterator it = points.begin(); it!=points.end(); ++it) {
            std::pair<Point,double> p = *it;
            p.first.x /= pot;
            p.first.y /= pot;
            toScalePoints.push_back(p);
        }
    }
    strokes.push_back(toScalePoints);
    
    std::vector<cairo_pattern_t*> dotPatterns = stroke->getPatternCache();
    if (mipMapLevelChanged) {
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            if (dotPatterns[i]) {
                cairo_pattern_destroy(dotPatterns[i]);
                dotPatterns[i] = 0;
            }
        }
        dotPatterns.clear();
    }
    if (dotPatterns.empty()) {
        dotPatterns.resize(ROTO_PRESSURE_LEVELS);
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            dotPatterns[i] = (cairo_pattern_t*)0;
        }
    }
    
    
    double opacity = stroke->getOpacity(time);
    distToNext = _imp->renderStroke(cr, dotPatterns, strokes, distToNext, stroke, doBuildUp, opacity, time, mipmapLevel);
    
    stroke->updatePatternCache(dotPatterns);
    
    assert(cairo_surface_status(cairoImg) == CAIRO_STATUS_SUCCESS);
    
    ///A call to cairo_surface_flush() is required before accessing the pixel data
    ///to ensure that all pending drawing operations are finished.
    cairo_surface_flush(cairoImg);
    
    
    
    convertCairoImageToNatronImage_noColor<float, 1>(cairoImg, srcNComps, source.get(), pixelPointsBbox, shapeColor, 1., false);

    
    cairo_destroy(cr);
    ////Free the buffer used by Cairo
    cairo_surface_destroy(cairoImg);
    
    return distToNext;
}


boost::shared_ptr<Image>
RotoContext::renderMaskFromStroke(const boost::shared_ptr<RotoDrawableItem>& stroke,
                                  const RectI& /*roi*/,
                                  const ImageComponents& components,
                                  const double time,
                                  const int view,
                                  const ImageBitDepthEnum depth,
                                  const unsigned int mipmapLevel)
{
    boost::shared_ptr<Node> node = getNode();
    
    
    
    ImagePtr image;// = stroke->getStrokeTimePreview();

    ///compute an enhanced hash different from the one of the merge node of the item in order to differentiate within the cache
    ///the output image of the node and the mask image.
    U64 rotoHash;
    {
        Hash64 hash;
        U64 mergeNodeHash = stroke->getMergeNode()->getLiveInstance()->getRenderHash();
        hash.append(mergeNodeHash);
        hash.computeHash();
        rotoHash = hash.value();
        assert(mergeNodeHash != rotoHash);
    }
    
    ImageKey key = Image::makeKey(stroke.get(),rotoHash, true ,time, view, false, false);
    
    {
        QMutexLocker k(&_imp->cacheAccessMutex);
        node->getLiveInstance()->getImageFromCacheAndConvertIfNeeded(true, false, key, mipmapLevel, NULL, NULL, depth, components, depth, components,EffectInstance::InputImagesMap(), boost::shared_ptr<RenderStats>(), &image);
    }
    if (image) {
        return image;
    }
    
    
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(stroke.get());
    Bezier* isBezier = dynamic_cast<Bezier*>(stroke.get());
    
    double startTime = time, mbFrameStep = 1., endTime = time;
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    if (isBezier) {
        int mbType_i = _imp->motionBlurTypeKnob.lock()->getValue();
        bool applyPerShapeMotionBlur = mbType_i == 0;
        if (applyPerShapeMotionBlur) {
            isBezier->getMotionBlurSettings(time, &startTime, &endTime, &mbFrameStep);
        }
    }
#endif
    
    RectD bbox;

    std::list<std::list<std::pair<Point,double> > > strokes;
 
    if (isStroke) {
        isStroke->evaluateStroke(mipmapLevel, time, &strokes, &bbox);
    } else {
        assert(isBezier);
        bool bboxSet = false;
        for (double t = startTime; t <= endTime; t += mbFrameStep) {
            RectD subBbox = isBezier->getBoundingBox(t);
            if (!bboxSet) {
                bbox = subBbox;
                bboxSet = true;
            } else {
                bbox.merge(subBbox);
            }
        }
        if (isBezier->isOpenBezier()) {
            std::list<Point> decastelJauPolygon;
            isBezier->evaluateAtTime_DeCasteljau_autoNbPoints(false, time, mipmapLevel, &decastelJauPolygon, 0);
            std::list<std::pair<Point,double> > points;
            for (std::list<Point> ::iterator it = decastelJauPolygon.begin(); it!=decastelJauPolygon.end(); ++it) {
                points.push_back(std::make_pair(*it, 1.));
            }
            strokes.push_back(points);
        }
        
    }
    
    RectI pixelRod;
    bbox.toPixelEnclosing(mipmapLevel, 1., &pixelRod);

    
    boost::shared_ptr<ImageParams> params = Image::makeParams( 0,
                                                                              bbox,
                                                                              pixelRod,
                                                                              1., // par
                                                                              mipmapLevel,
                                                                              false,
                                                                              components,
                                                                              depth,
                                                                              std::map<int,std::map<int, std::vector<RangeD> > >() );
    /*
     At this point we take the cacheAccessMutex so that no other thread can retrieve this image from the cache while it has not been 
     finished rendering. You might wonder why we do this differently here than in EffectInstance::renderRoI, this is because we do not use
     the trimap and notification system here in the rotocontext, which would be to much just for this small object, rather we just lock
     it once, which is fine.
     */
    QMutexLocker k(&_imp->cacheAccessMutex);
    
    natronGetImageFromCacheOrCreate(key, params, &image);
    if (!image) {
        std::stringstream ss;
        ss << "Failed to allocate an image of ";
        ss << printAsRAM( params->getElementsCount() * sizeof(Image::data_t) ).toStdString();
        natronErrorDialog( QObject::tr("Out of memory").toStdString(),ss.str() );
        
        return image;
    }
    
    ///Does nothing if image is already alloc
    image->allocateMemory();
    

    image = renderMaskInternal(stroke, pixelRod, components, startTime, endTime, mbFrameStep, time, depth, mipmapLevel, strokes, image);
    
    return image;
}




boost::shared_ptr<Image>
RotoContext::renderMaskInternal(const boost::shared_ptr<RotoDrawableItem>& stroke,
                                const RectI & roi,
                                const ImageComponents& components,
                                const double startTime,
                                const double endTime,
                                const double timeStep,
                                const double time,
                                const ImageBitDepthEnum depth,
                                const unsigned int mipmapLevel,
                                const std::list<std::list<std::pair<Point,double> > >& strokes,
                                const boost::shared_ptr<Image> &image)
{
 
    
    boost::shared_ptr<Node> node = getNode();
    
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(stroke.get());
    Bezier* isBezier = dynamic_cast<Bezier*>(stroke.get());
    cairo_format_t cairoImgFormat;
    
    int srcNComps;
    bool doBuildUp = true;
    
    if (isStroke) {
        //Motion-blur is not supported for strokes
        assert(startTime == endTime);
        
        doBuildUp = stroke->getBuildupKnob()->getValueAtTime(time);
        //For the non build-up case, we use the LIGHTEN compositing operator, which only works on colors
        if (!doBuildUp || components.getNumComponents() > 1) {
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
    

    ////Allocate the cairo temporary buffer
    cairo_surface_t* cairoImg = cairo_image_surface_create(cairoImgFormat, roi.width(), roi.height() );
    cairo_surface_set_device_offset(cairoImg, -roi.x1, -roi.y1);
    if (cairo_surface_status(cairoImg) != CAIRO_STATUS_SUCCESS) {
        return image;
    }
    cairo_t* cr = cairo_create(cairoImg);
    //cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD); // creates holes on self-overlapping shapes
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    
    // these Roto shapes must be rendered WITHOUT antialias, or the junction between the inner
    // polygon and the feather zone will have artifacts. This is partly due to the fact that cairo
    // meshes are not antialiased.
    // Use a default feather distance of 1 pixel instead!
    // UPDATE: unfortunately, this produces less artifacts, but there are still some remaining (use opacity=0.5 to test)
    // maybe the inner polygon should be made of mesh patterns too?
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    
    double shapeColor[3];
    stroke->getColor(time, shapeColor);

    double opacity = stroke->getOpacity(time);

    assert(isStroke || isBezier);
    if (isStroke || !isBezier || (isBezier && isBezier->isOpenBezier())) {
        std::vector<cairo_pattern_t*> dotPatterns(ROTO_PRESSURE_LEVELS);
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            dotPatterns[i] = (cairo_pattern_t*)0;
        }
        _imp->renderStroke(cr, dotPatterns, strokes, 0, stroke, doBuildUp, opacity, time, mipmapLevel);
        
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            if (dotPatterns[i]) {
                cairo_pattern_destroy(dotPatterns[i]);
                dotPatterns[i] = 0;
            }
        }
        
        
    } else {
        _imp->renderBezier(cr, isBezier, opacity, time, mipmapLevel);
    }
    
    bool useOpacityToConvert = (isBezier != 0);
    
    switch (depth) {
        case eImageBitDepthFloat:
            convertCairoImageToNatronImage_noColor<float, 1>(cairoImg, srcNComps, image.get(), roi, shapeColor, opacity, useOpacityToConvert);
            break;
        case eImageBitDepthByte:
            convertCairoImageToNatronImage_noColor<unsigned char, 255>(cairoImg, srcNComps,  image.get(), roi,shapeColor, opacity, useOpacityToConvert);
            break;
        case eImageBitDepthShort:
            convertCairoImageToNatronImage_noColor<unsigned short, 65535>(cairoImg, srcNComps, image.get(), roi,shapeColor, opacity, useOpacityToConvert);
            break;
        case eImageBitDepthHalf:
        case eImageBitDepthNone:
            assert(false);
            break;
    }

    
    assert(cairo_surface_status(cairoImg) == CAIRO_STATUS_SUCCESS);
    
    ///A call to cairo_surface_flush() is required before accessing the pixel data
    ///to ensure that all pending drawing operations are finished.
    cairo_surface_flush(cairoImg);
    
    
    
    cairo_destroy(cr);
    ////Free the buffer used by Cairo
    cairo_surface_destroy(cairoImg);

    
    return image;
}


static inline
double hardnessGaussLookup(double f)
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
                              std::vector<cairo_pattern_t*>& dotPatterns,
                              const Point &center,
                              double internalDotRadius,
                              double externalDotRadius,
                              double pressure,
                              bool doBuildUp,
                              const std::vector<std::pair<double, double> >& opacityStops,
                              double opacity)
{
    
    if (!opacityStops.empty()) {
        cairo_pattern_t* pattern;
        // sometimes, Qt gives a pressure level > 1... so we clamp it
        int pressureInt = int(std::max(0., std::min(pressure, 1.)) * (ROTO_PRESSURE_LEVELS-1) + 0.5);
        assert(pressureInt >= 0 && pressureInt < ROTO_PRESSURE_LEVELS);
        if (dotPatterns[pressureInt]) {
            pattern = dotPatterns[pressureInt];
        } else {
            pattern = cairo_pattern_create_radial(0, 0, internalDotRadius, 0, 0, externalDotRadius);
            for (std::size_t i = 0; i < opacityStops.size(); ++i) {
                if (doBuildUp) {
                    cairo_pattern_add_color_stop_rgba(pattern, opacityStops[i].first, 1., 1., 1.,opacityStops[i].second);
                } else {
                    cairo_pattern_add_color_stop_rgba(pattern, opacityStops[i].first, opacityStops[i].second, opacityStops[i].second, opacityStops[i].second,1);
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
    double x1,y1;
    cairo_surface_get_device_offset(target, &x1, &y1);
    assert(std::floor(center.x - externalDotRadius) >= -x1 && std::floor(center.x + externalDotRadius) < -x1 + w &&
           std::floor(center.y - externalDotRadius) >= -y1 && std::floor(center.y + externalDotRadius) < -y1 + h);
#endif
    cairo_arc(cr, center.x, center.y, externalDotRadius, 0, M_PI * 2);
    cairo_fill(cr);
}

static void getRenderDotParams(double alpha, double brushSizePixel, double brushHardness, double brushSpacing, double pressure, bool pressureAffectsOpacity, bool pressureAffectsSize, bool pressureAffectsHardness, double* internalDotRadius, double* externalDotRadius, double * spacing, std::vector<std::pair<double,double> >* opacityStops)
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
    
    *internalDotRadius = std::max(brushSizePixel * brushHardness,1.) / 2.;
    *externalDotRadius = std::max(brushSizePixel, 1.) / 2.;
    *spacing = *externalDotRadius * 2. * brushSpacing;
    
    
    opacityStops->clear();
    
    double exp = brushHardness != 1.0 ?  0.4 / (1.0 - brushHardness) : 0.;
    const int maxStops = 8;
    double incr = 1. / maxStops;
    
    if (brushHardness != 1.) {
        for (double d = 0; d <= 1.; d += incr) {
            double o = hardnessGaussLookup(std::pow(d, exp));
            opacityStops->push_back(std::make_pair(d, o * alpha));
        }
    }
    
}

double
RotoContextPrivate::renderStroke(cairo_t* cr,
                                 std::vector<cairo_pattern_t*>& dotPatterns,
                                 const std::list<std::list<std::pair<Point,double> > >& strokes,
                                 double distToNext,
                                 const boost::shared_ptr<RotoDrawableItem>&  stroke,
                                 bool doBuildup,
                                 double alpha,
                                 double time,
                                 unsigned int mipmapLevel)
{
    if (strokes.empty()) {
        return distToNext;
    }
    
    if (!stroke->isActivated(time)) {
        return distToNext;
    }
    
    assert(dotPatterns.size() == ROTO_PRESSURE_LEVELS);
    
    boost::shared_ptr<KnobDouble> brushSizeKnob = stroke->getBrushSizeKnob();
    double brushSize = brushSizeKnob->getValueAtTime(time);
    boost::shared_ptr<KnobDouble> brushSpacingKnob = stroke->getBrushSpacingKnob();
    double brushSpacing = brushSpacingKnob->getValueAtTime(time);
    if (brushSpacing == 0.) {
        return distToNext;
    }
    
    brushSpacing = std::max(brushSpacing, 0.05);
    
    boost::shared_ptr<KnobDouble> brushHardnessKnob = stroke->getBrushHardnessKnob();
    double brushHardness = brushHardnessKnob->getValueAtTime(time);
    boost::shared_ptr<KnobDouble> visiblePortionKnob = stroke->getBrushVisiblePortionKnob();
    double writeOnStart = visiblePortionKnob->getValueAtTime(time, 0);
    double writeOnEnd = visiblePortionKnob->getValueAtTime(time, 1);
    if ((writeOnEnd - writeOnStart) <= 0.) {
        return distToNext;
    }
    
    boost::shared_ptr<KnobBool> pressureOpacityKnob = stroke->getPressureOpacityKnob();
    boost::shared_ptr<KnobBool> pressureSizeKnob = stroke->getPressureSizeKnob();
    boost::shared_ptr<KnobBool> pressureHardnessKnob = stroke->getPressureHardnessKnob();
    
    bool pressureAffectsOpacity = pressureOpacityKnob->getValueAtTime(time);
    bool pressureAffectsSize = pressureSizeKnob->getValueAtTime(time);
    bool pressureAffectsHardness = pressureHardnessKnob->getValueAtTime(time);
    
    double brushSizePixel = brushSize;
    if (mipmapLevel != 0) {
        brushSizePixel = std::max(1.,brushSizePixel / (1 << mipmapLevel));
    }
    cairo_set_operator(cr,doBuildup ? CAIRO_OPERATOR_OVER : CAIRO_OPERATOR_LIGHTEN);

    
    for (std::list<std::list<std::pair<Point,double> > >::const_iterator strokeIt = strokes.begin() ;strokeIt != strokes.end() ;++strokeIt) {
        int firstPoint = (int)std::floor((strokeIt->size() * writeOnStart));
        int endPoint = (int)std::ceil((strokeIt->size() * writeOnEnd));
        assert(firstPoint >= 0 && firstPoint < (int)strokeIt->size() && endPoint > firstPoint && endPoint <= (int)strokeIt->size());
        
        
        ///The visible portion of the paint's stroke with points adjusted to pixel coordinates
        std::list<std::pair<Point,double> > visiblePortion;
        std::list<std::pair<Point,double> >::const_iterator startingIt = strokeIt->begin();
        std::list<std::pair<Point,double> >::const_iterator endingIt = strokeIt->begin();
        std::advance(startingIt, firstPoint);
        std::advance(endingIt, endPoint);
        for (std::list<std::pair<Point,double> >::const_iterator it = startingIt; it!=endingIt; ++it) {
            visiblePortion.push_back(*it);
        }
        if (visiblePortion.empty()) {
            return distToNext;
        }
        
        std::list<std::pair<Point,double> >::iterator it = visiblePortion.begin();
        
        if (visiblePortion.size() == 1) {
            double internalDotRadius, externalDotRadius, spacing;
            std::vector<std::pair<double,double> > opacityStops;
            getRenderDotParams(alpha, brushSizePixel, brushHardness, brushSpacing, it->second, pressureAffectsOpacity, pressureAffectsSize, pressureAffectsHardness, &internalDotRadius, &externalDotRadius, &spacing, &opacityStops);
            renderDot(cr, dotPatterns, it->first, internalDotRadius, externalDotRadius, it->second, doBuildup, opacityStops, alpha);
            continue;
        }
        
        std::list<std::pair<Point,double> >::iterator next = it;
        ++next;
        
        while (next!=visiblePortion.end()) {
            //Render for each point a dot. Spacing is a percentage of brushSize:
            //Spacing at 1 means no dot is overlapping another (so the spacing is in fact brushSize)
            //Spacing at 0 we do not render the stroke
            
            double dist = std::sqrt((next->first.x - it->first.x) * (next->first.x - it->first.x) +  (next->first.y - it->first.y) * (next->first.y - it->first.y));
            
            // while the next point can be drawn on this segment, draw a point and advance
            while (distToNext <= dist) {
                double a = dist == 0. ? 0. : distToNext/dist;
                Point center = {
                    it->first.x * (1 - a) + next->first.x * a,
                    it->first.y * (1 - a) + next->first.y * a
                };
                double pressure = it->second * (1 - a) + next->second * a;
                
                // draw the dot
                double internalDotRadius, externalDotRadius, spacing;
                std::vector<std::pair<double,double> > opacityStops;
                getRenderDotParams(alpha, brushSizePixel, brushHardness, brushSpacing, pressure, pressureAffectsOpacity, pressureAffectsSize, pressureAffectsHardness, &internalDotRadius, &externalDotRadius, &spacing, &opacityStops);
                renderDot(cr, dotPatterns, center, internalDotRadius, externalDotRadius, pressure, doBuildup, opacityStops, alpha);
                
                distToNext += spacing;
            }
            
            // go to the next segment
            distToNext -= dist;
            ++next;
            ++it;
        }
    }
    
    
    return distToNext;
}



void
RotoContextPrivate::renderBezier(cairo_t* cr,const Bezier* bezier,double opacity, double time, unsigned int mipmapLevel)
{
    ///render the bezier only if finished (closed) and activated
    if ( !bezier->isCurveFinished() || !bezier->isActivated(time) || ( bezier->getControlPointsCount() <= 1 ) ) {
        return;
    }
    
    
    double fallOff = bezier->getFeatherFallOff(time);
    double featherDist = bezier->getFeatherDistance(time);
#ifdef NATRON_ROTO_INVERTIBLE
    bool inverted = (*it2)->getInverted(time);
#else
    const bool inverted = false;
#endif
    double shapeColor[3];
    bezier->getColor(time, shapeColor);
    
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    BezierCPs cps = bezier->getControlPoints_mt_safe();
    
    BezierCPs fps = bezier->getFeatherPoints_mt_safe();
    
    assert( cps.size() == fps.size() );
    
    if ( cps.empty() ) {
        return;
    }
    
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
    bezier->getTransformAtTime(time, &transform);
    
    
    renderFeather(bezier, time, mipmapLevel, inverted, shapeColor, opacity, featherDist, fallOff, mesh);
    
    
    if (!inverted) {
        // strangely, the above-mentioned cairo bug doesn't affect this function
        renderInternalShape(time, mipmapLevel, shapeColor, opacity, transform, cr, mesh, cps);
#ifdef NATRON_ROTO_INVERTIBLE
    } else {
#pragma message WARN("doesn't work! the image should be infinite for this to work!")
        // Doesn't work! the image should be infinite for this to work!
        // Or at least it should contain the Union of the source RoDs.
        // Here, it only contains the boinding box of the Bezier.
        // If there's a transform after the roto node, a black border will appear.
        // The only solution would be to have a color parameter which specifies how on image is outside of its RoD.
        // Unfortunately, the OFX definition is: "it is black and transparent"
        
        ///If inverted, draw an inverted rectangle on all the image first
        // with a hole consisting of the feather polygon
        
        double xOffset, yOffset;
        cairo_surface_get_device_offset(cairoImg, &xOffset, &yOffset);
        int width = cairo_image_surface_get_width(cairoImg);
        int height = cairo_image_surface_get_height(cairoImg);
        
        cairo_move_to(cr, -xOffset, -yOffset);
        cairo_line_to(cr, -xOffset + width, -yOffset);
        cairo_line_to(cr, -xOffset + width, -yOffset + height);
        cairo_line_to(cr, -xOffset, -yOffset + height);
        cairo_line_to(cr, -xOffset, -yOffset);
        // strangely, the above-mentioned cairo bug doesn't affect this function
#pragma message WARN("WRONG! should use the outer feather contour, *displaced* by featherDistance, not fps")
        renderInternalShape(time, mipmapLevel, cr, fps);
#endif
    }
    
    applyAndDestroyMask(cr, mesh);

}

void
RotoContextPrivate::renderFeather(const Bezier* bezier,double time, unsigned int mipmapLevel, bool inverted, double shapeColor[3], double /*opacity*/, double featherDist, double fallOff, cairo_pattern_t* mesh)
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
    std::list<Point> featherPolygon;
    std::list<Point> bezierPolygon;
    RectD featherPolyBBox;
    featherPolyBBox.setupInfinity();
    
    bezier->evaluateFeatherPointsAtTime_DeCasteljau(false, time, mipmapLevel, 50, true, &featherPolygon, &featherPolyBBox);
    bezier->evaluateAtTime_DeCasteljau(false, time, mipmapLevel, 50, &bezierPolygon, NULL);
    
    bool clockWise = bezier->isFeatherPolygonClockwiseOriented(false,time);
    
    assert( !featherPolygon.empty() && !bezierPolygon.empty());


    std::list<Point> featherContour;

    // prepare iterators
    std::list<Point>::iterator next = featherPolygon.begin();
    ++next;  // can only be valid since we assert the list is not empty
    if (next == featherPolygon.end()) {
        next = featherPolygon.begin();
    }
    std::list<Point>::iterator prev = featherPolygon.end();
    --prev; // can only be valid since we assert the list is not empty
    std::list<Point>::iterator bezIT = bezierPolygon.begin();
    std::list<Point>::iterator prevBez = bezierPolygon.end();
    --prevBez; // can only be valid since we assert the list is not empty

    // prepare p1
    double absFeatherDist = std::abs(featherDist);
    Point p1 = *featherPolygon.begin();
    double norm = sqrt( (next->x - prev->x) * (next->x - prev->x) + (next->y - prev->y) * (next->y - prev->y) );
    assert(norm != 0);
    double dx = -( (next->y - prev->y) / norm );
    double dy = ( (next->x - prev->x) / norm );

    if (!clockWise) {
        p1.x -= dx * absFeatherDist;
        p1.y -= dy * absFeatherDist;
    } else {
        p1.x += dx * absFeatherDist;
        p1.y += dy * absFeatherDist;
    }
    
    Point origin = p1;
    featherContour.push_back(p1);


    // increment for first iteration
    std::list<Point>::iterator cur = featherPolygon.begin();
    // ++cur, ++prev, ++next, ++bezIT, ++prevBez
    // all should be valid, actually
    assert(cur != featherPolygon.end() &&
           prev != featherPolygon.end() &&
           next != featherPolygon.end() &&
           bezIT != bezierPolygon.end() &&
           prevBez != bezierPolygon.end());
    if (cur != featherPolygon.end()) {
        ++cur;
    }
    if (prev != featherPolygon.end()) {
        ++prev;
    }
    if (next != featherPolygon.end()) {
        ++next;
    }
    if (bezIT != bezierPolygon.end()) {
        ++bezIT;
    }
    if (prevBez != bezierPolygon.end()) {
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
        if ( (cur->x == prev->x) && (cur->y == prev->y) ) {
            continue;
        }
        
        Point p0, p0p1, p1p0, p2, p2p3, p3p2, p3;
        p0.x = prevBez->x;
        p0.y = prevBez->y;
        p3.x = bezIT->x;
        p3.y = bezIT->y;
        
        if (!mustStop) {
            norm = sqrt( (next->x - prev->x) * (next->x - prev->x) + (next->y - prev->y) * (next->y - prev->y) );
            assert(norm != 0);
            dx = -( (next->y - prev->y) / norm );
            dy = ( (next->x - prev->x) / norm );
            p2 = *cur;

            if (!clockWise) {
                p2.x -= dx * absFeatherDist;
                p2.y -= dy * absFeatherDist;
            } else {
                p2.x += dx * absFeatherDist;
                p2.y += dy * absFeatherDist;
            }
        } else {
            p2 = origin;
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
        // To check wether the bug is present is a given cairo version,
        // make any shape with a very large feather and set
        // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
        // and approximately equal to 0.5.
        // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
        // older Cairo versions.
        cairo_mesh_pattern_set_corner_color_rgba( mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2],
                                                 std::sqrt(inverted ? 0. : 1.) );
        ///outter is faded
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2],
                                                 inverted ? 1. : 0.);
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2],
                                                 inverted ? 1. : 0.);
        ///inner is full color
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2],
                                                 std::sqrt(inverted ? 0. : 1.));
        assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
        
        cairo_mesh_pattern_end_patch(mesh);
        
        if (mustStop) {
            break;
        }
        
        p1 = p2;

        // increment for next iteration
        // ++prev, ++next, ++bezIT, ++prevBez
        if (prev != featherPolygon.end()) {
            ++prev;
        }
        if (next != featherPolygon.end()) {
            ++next;
        }
        if (bezIT != bezierPolygon.end()) {
            ++bezIT;
        }
        if (prevBez != bezierPolygon.end()) {
            ++prevBez;
        }

    }  // for each point in polygon

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
    assert(!cps.empty());
#ifdef ROTO_USE_MESH_PATTERN_ONLY
    std::list<BezierCPs> coonPatches;
    bezulate(time, cps, &coonPatches);
    
    for (std::list<BezierCPs>::iterator it = coonPatches.begin(); it != coonPatches.end(); ++it) {
        
        std::list<BezierCPs> fixedPatch;
        Natron::regularize(*it, time, &fixedPatch);
        for (std::list<BezierCPs>::iterator it2 = fixedPatch.begin(); it2 != fixedPatch.end(); ++it2) {
            
            
            
            std::size_t size = it2->size();
            assert(size <= 4 && size >= 2);
            
            BezierCPs::iterator patchIT = it2->begin();
            boost::shared_ptr<BezierCP> p0ptr,p1ptr,p2ptr,p3ptr;
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
            
            Point p0,p0p1,p1p0,p1,p1p2,p2p1,p2p3,p3p2,p2,p3,p3p0,p0p3;
            
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
            
            
            /*
             Add a Coons patch such as:
             
             C1  Side 1   C2
             +---------------+
             |               |
             |  P1       P2  |
             |               |
             Side 0 |               | Side 2
             |               |
             |               |
             |  P0       P3  |
             |               |
             +---------------+
             C0     Side 3   C3
             
             In the above drawing, C0 is p0, P0 is p0p1, P1 is p1p0, C1 is p1 and so on...
             */
            
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
            // To check wether the bug is present is a given cairo version,
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
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2],
                                                     std::sqrt(opacity));
            assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
            
            cairo_mesh_pattern_end_patch(mesh);
            
        }
    }
#else
    
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    
    BezierCPs::const_iterator point = cps.begin();
    assert(point != cps.end());
    if (point == cps.end()) {
        return;
    }
    BezierCPs::const_iterator nextPoint = point;
    if (nextPoint != cps.end()) {
        ++nextPoint;
    }

    
    Transform::Point3D initCp;
    (*point)->getPositionAtTime(false,time, &initCp.x,&initCp.y);
    initCp.z = 1.;
    initCp = Transform::matApply(transform, initCp);
    
    adjustToPointToScale(mipmapLevel,initCp.x,initCp.y);

    cairo_move_to(cr, initCp.x,initCp.y);

    while ( point != cps.end() ) {
        if ( nextPoint == cps.end() ) {
            nextPoint = cps.begin();
        }
        
        Transform::Point3D right,nextLeft,next;
        (*point)->getRightBezierPointAtTime(false,time, &right.x, &right.y);
        right.z = 1;
        (*nextPoint)->getLeftBezierPointAtTime(false,time, &nextLeft.x, &nextLeft.y);
        nextLeft.z = 1;
        (*nextPoint)->getPositionAtTime(false,time, &next.x, &next.y);
        next.z = 1;

        right = Transform::matApply(transform, right);
        nextLeft = Transform::matApply(transform, nextLeft);
        next = Transform::matApply(transform, next);
        
        adjustToPointToScale(mipmapLevel,right.x,right.y);
        adjustToPointToScale(mipmapLevel,next.x,next.y);
        adjustToPointToScale(mipmapLevel,nextLeft.x,nextLeft.y);
        cairo_curve_to(cr, right.x, right.y, nextLeft.x, nextLeft.y, next.x, next.y);

        // increment for next iteration
        ++point;
        if (nextPoint != cps.end()) {
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
#endif
}

struct qpointf_compare_less
{
    bool operator() (const QPointF& lhs,const QPointF& rhs) const
    {
        if (std::abs(lhs.x() - rhs.x()) < 1e-6) {
            if (std::abs(lhs.y() - rhs.y()) < 1e-6) {
                return false;
            } else if (lhs.y() < rhs.y()) {
                return true;
            } else {
                return false;
            }
        } else if (lhs.x() < rhs.x()) {
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
RotoContextPrivate::bezulate(double time, const BezierCPs& cps,std::list<BezierCPs>* patches)
{
    BezierCPs simpleClosedCurve = cps;
    
    while (simpleClosedCurve.size() > 4) {
        
        bool found = false;
        for (int n = 3; n >= 2; --n) {
            
            assert((int)simpleClosedCurve.size() > n);
            
            //next points at point i + n
            BezierCPs::iterator next = simpleClosedCurve.begin();
            std::advance(next, n);
            
            std::list<Point> polygon;
            RectD bbox;
            bbox.setupInfinity();
            for (BezierCPs::iterator it = simpleClosedCurve.begin(); it != simpleClosedCurve.end(); ++it) {
                Point p;
                (*it)->getPositionAtTime(false,time, &p.x, &p.y);
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
                if (next == simpleClosedCurve.end()) {
                    next = simpleClosedCurve.begin();
                    nextIsPassedEnd = true;
                }
                
                //mid-point of the line segment between points i and i + n
                Point nextPoint,curPoint;
                (*it)->getPositionAtTime(false,time, &curPoint.x, &curPoint.y);
                (*next)->getPositionAtTime(false,time, &nextPoint.x, &nextPoint.y);
                
                /*
                 * Compute the number of intersections between the current line segment [it,next] and all other line segments
                 * If the number of intersections is different of 2, ignore this segment.
                 */
                QLineF line(QPointF(curPoint.x,curPoint.y),QPointF(nextPoint.x,nextPoint.y));
                std::set<QPointF,qpointf_compare_less> intersections;
                std::list<Point>::const_iterator last_pt = polygon.begin();
                std::list<Point>::const_iterator cur = last_pt;
                ++cur;
                QPointF intersectionPoint;
                for (; cur != polygon.end(); ++cur, ++last_pt) {
                    QLineF polygonSegment(QPointF(last_pt->x,last_pt->y),QPointF(cur->x,cur->y));
                    if (line.intersect(polygonSegment, &intersectionPoint) == QLineF::BoundedIntersection) {
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
                bool isInside = pointInPolygon(midPoint,polygon,bbox,Bezier::eFillRuleWinding);

                if (isInside) {
                    
                    //Make the sub closed curve composed of the path from points i to i + n
                    BezierCPs subCurve;
                    subCurve.push_back(*it);
                    BezierCPs::iterator pointIt = it;
                    for (int i = 0; i < n - 1; ++i) {
                        ++pointIt;
                        if (pointIt == simpleClosedCurve.end()) {
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
                    if (eraseStart == simpleClosedCurve.end()) {
                        eraseStart = simpleClosedCurve.begin();
                        eraseStartIsPassedEnd = true;
                    }
                    //"it" is  invalidated after the next instructions but we leave the loop anyway
                    assert(!simpleClosedCurve.empty());
                    if ((!nextIsPassedEnd && !eraseStartIsPassedEnd) || (nextIsPassedEnd && eraseStartIsPassedEnd)) {
                        simpleClosedCurve.erase(eraseStart,next);
                    } else {
                        simpleClosedCurve.erase(eraseStart,simpleClosedCurve.end());
                        if (!simpleClosedCurve.empty()) {
                            simpleClosedCurve.erase(simpleClosedCurve.begin(),next);
                        }
                    }
                    found = true;
                    break;
                }

                // increment for next iteration
                if (next != simpleClosedCurve.end()) {
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
            if (next != simpleClosedCurve.end()) {
                ++next;
            }
            for (BezierCPs::iterator it = simpleClosedCurve.begin(); it != simpleClosedCurve.end(); ++it) {
                
                if (next == simpleClosedCurve.end()) {
                    next = simpleClosedCurve.begin();
                }
                Point p0,p1,p2,p3,p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3,dest;
                (*it)->getPositionAtTime(false,time, &p0.x, &p0.y);
                (*it)->getRightBezierPointAtTime(false,time, &p1.x, &p1.y);
                (*next)->getLeftBezierPointAtTime(false,time, &p2.x, &p2.y);
                (*next)->getPositionAtTime(false,time, &p3.x, &p3.y);
                Bezier::bezierFullPoint(p0, p1, p2, p3, 0.5, &p0p1, &p1p2, &p2p3, &p0p1_p1p2, &p1p2_p2p3, &dest);
                boost::shared_ptr<BezierCP> controlPoint(new BezierCP);
                controlPoint->setStaticPosition(false,dest.x, dest.y);
                controlPoint->setLeftBezierStaticPosition(false,p0p1_p1p2.x, p0p1_p1p2.y);
                controlPoint->setRightBezierStaticPosition(false,p1p2_p2p3.x, p1p2_p2p3.y);
                subdivisedCurve.push_back(*it);
                subdivisedCurve.push_back(controlPoint);

                // increment for next iteration
                if (next != simpleClosedCurve.end()) {
                    ++next;
                }
            } // for()
            simpleClosedCurve = subdivisedCurve;
        }
    }
    if (!simpleClosedCurve.empty()) {
        assert(simpleClosedCurve.size() >= 2);
        patches->push_back(simpleClosedCurve);
    }
}

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
RotoContext::changeItemScriptName(const std::string& oldFullyQualifiedName,const std::string& newFullyQUalifiedName)
{
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    
    std::string declStr = nodeFullName + ".roto." + newFullyQUalifiedName + " = " + nodeFullName + ".roto." + oldFullyQualifiedName + "\n";
    std::string delStr = "del " + nodeFullName + ".roto." + oldFullyQualifiedName + "\n";
    std::string script = declStr + delStr;
    if (!appPTR->isBackground()) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if (!interpretPythonScript(script , &err, 0)) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
}

void
RotoContext::removeItemAsPythonField(const boost::shared_ptr<RotoItem>& item)
{
    
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(item.get());
    if (isStroke) {
        ///Strokes are unsupported in Python currently
        return;
    }
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = "del " + nodeFullName + ".roto." + item->getFullyQualifiedName() + "\n";
    if (!appPTR->isBackground()) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if (!interpretPythonScript(script , &err, 0)) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
    
}

boost::shared_ptr<Node>
RotoContext::getOrCreateGlobalMergeNode(int *availableInputIndex)
{
    
    
    {
        QMutexLocker k(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<Node> >::iterator it = _imp->globalMergeNodes.begin(); it!=_imp->globalMergeNodes.end(); ++it) {
            const std::vector<boost::shared_ptr<Node> > &inputs = (*it)->getInputs();
            
            //Merge node goes like this: B, A, Mask, A2, A3, A4 ...
            assert(inputs.size() >= 3 && (*it)->getLiveInstance()->isInputMask(2));
            if (!inputs[1]) {
                *availableInputIndex = 1;
                return *it;
            }
            
            //Leave the B empty to connect the next merge node
            for (std::size_t i = 3; i < inputs.size(); ++i) {
                if (!inputs[i]) {
                    *availableInputIndex = (int)i;
                    return *it;
                }
            }
        }
    }
    
    boost::shared_ptr<Node>  node = getNode();
    //We must create a new merge node
    QString fixedNamePrefix(node->getScriptName_mt_safe().c_str());
    fixedNamePrefix.append('_');
    fixedNamePrefix.append("globalMerge");
    fixedNamePrefix.append('_');
    CreateNodeArgs args(PLUGINID_OFX_MERGE, "",
                        -1,-1,
                        false,
                        INT_MIN,
                        INT_MIN,
                        false,
                        false,
                        false,
                        fixedNamePrefix,
                        CreateNodeArgs::DefaultValuesList(),
                        boost::shared_ptr<NodeCollection>());
    args.createGui = false;
    
    boost::shared_ptr<Node> mergeNode = node->getApp()->createNode(args);
    if (!mergeNode) {
        return mergeNode;
    }
    mergeNode->setUseAlpha0ToConvertFromRGBToRGBA(true);
    if (getNode()->isDuringPaintStrokeCreation()) {
        mergeNode->setWhileCreatingPaintStroke(true);
    }
    *availableInputIndex = 1;
    
    QMutexLocker k(&_imp->rotoContextMutex);
    _imp->globalMergeNodes.push_back(mergeNode);
    return mergeNode;
}

void
RotoContext::refreshRotoPaintTree()
{
    if (_imp->isCurrentlyLoading) {
        return;
    }
    
    bool canConcatenate = isRotoPaintTreeConcatenatable();
    
    boost::shared_ptr<Node> globalMerge;
    int globalMergeIndex = -1;
    
    
    std::list<boost::shared_ptr<Node> > mergeNodes;
    {
        QMutexLocker k(&_imp->rotoContextMutex);
        mergeNodes = _imp->globalMergeNodes;
    }
    //ensure that all global merge nodes are disconnected
    for (std::list<boost::shared_ptr<Node> >::iterator it = mergeNodes.begin(); it!=mergeNodes.end(); ++it) {
        int maxInputs = (*it)->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            (*it)->disconnectInput(i);
        }
    }
    if (canConcatenate) {
        globalMerge = getOrCreateGlobalMergeNode(&globalMergeIndex);
    }
    if (globalMerge) {
        boost::shared_ptr<Node> rotopaintNodeInput = getNode()->getInput(0);
        //Connect the rotopaint node input to the B input of the Merge
        if (rotopaintNodeInput) {
            globalMerge->connectInput(rotopaintNodeInput, 0);
        }
    }
    
    std::list<boost::shared_ptr<RotoDrawableItem> > items = getCurvesByRenderOrder();
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::const_iterator it = items.begin(); it!=items.end(); ++it) {
        (*it)->refreshNodesConnections();
        
        if (globalMerge) {
            boost::shared_ptr<Node> effectNode = (*it)->getEffectNode();
            assert(effectNode);
            //qDebug() << "Connecting" << (*it)->getScriptName().c_str() << "to input" << globalMergeIndex <<
            //"(" << globalMerge->getInputLabel(globalMergeIndex).c_str() << ")" << "of" << globalMerge->getScriptName().c_str();
            globalMerge->connectInput(effectNode, globalMergeIndex);
            
            ///Refresh for next node
            boost::shared_ptr<Node> nextMerge = getOrCreateGlobalMergeNode(&globalMergeIndex);
            if (nextMerge != globalMerge) {
                assert(!nextMerge->getInput(0));
                nextMerge->connectInput(globalMerge, 0);
                globalMerge = nextMerge;
            }
        }
        
    }
}

void
RotoContext::onRotoPaintInputChanged(const boost::shared_ptr<Node>& node)
{
    
    if (node) {
        
    }
    refreshRotoPaintTree();
}

void
RotoContext::declareItemAsPythonField(const boost::shared_ptr<RotoItem>& item)
{
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = getNode()->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;

    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(item.get());
    if (isStroke) {
        ///Strokes are unsupported in Python currently
        return;
    }
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>(item.get());
    
    std::string err;
    std::string script = (nodeFullName + ".roto." + item->getFullyQualifiedName() + " = " +
                          nodeFullName + ".roto.getItemByName(\"" + item->getScriptName() + "\")\n");
    if (!appPTR->isBackground()) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if(!interpretPythonScript(script , &err, 0)) {
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
    for (std::list< boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        declareItemAsPythonField(*it);
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_RotoContext.cpp"
