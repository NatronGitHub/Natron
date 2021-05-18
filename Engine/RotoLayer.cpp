/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "RotoLayer.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cassert>
#include <stdexcept>

#include <QtCore/QLineF>
#include <QtCore/QDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierSerialization.h"
#include "Engine/BezierCP.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Interpolation.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContextPrivate.h"
#include "Engine/RotoLayerSerialization.h"
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

NATRON_NAMESPACE_ENTER

////////////////////////////////////Layer////////////////////////////////////

RotoLayer::RotoLayer(const RotoContextPtr& context,
                     const std::string & n,
                     const RotoLayerPtr& parent)
    : RotoItem(context, n, parent)
    , _imp( new RotoLayerPrivate() )
{
}

RotoLayer::RotoLayer(const RotoLayer & other)
    : RotoItem( other.getContext(), other.getScriptName(), other.getParentLayer() )
    , _imp( new RotoLayerPrivate() )
{
    clone(&other);
}

RotoLayer::~RotoLayer()
{
}

void
RotoLayer::clone(const RotoItem* other)
{
    RotoItem::clone(other);
    const RotoLayer* isOtherLayer = dynamic_cast<const RotoLayer*>(other);

    if (!isOtherLayer) {
        return;
    }
    RotoLayerPtr this_shared = boost::dynamic_pointer_cast<RotoLayer>( shared_from_this() );
    assert(this_shared);

    QMutexLocker l(&itemMutex);

    _imp->items.clear();
    for (std::list<RotoItemPtr>::const_iterator it = isOtherLayer->_imp->items.begin();
         it != isOtherLayer->_imp->items.end(); ++it) {
        RotoLayerPtr isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        RotoStrokeItemPtr isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(*it);
        if (isBezier) {
            BezierPtr copy( new Bezier(*isBezier, this_shared) );
            copy->createNodes();
            _imp->items.push_back(copy);
        } else if (isStroke) {
            RotoStrokeItemPtr copy( new RotoStrokeItem( isStroke->getBrushType(),
                                                                        isStroke->getContext(),
                                                                        isStroke->getScriptName() + "copy",
                                                                        RotoLayerPtr() ) );
            copy->createNodes();
            _imp->items.push_back(copy);
            copy->setParentLayer(this_shared);
        } else {
            assert(isLayer);
            if (isLayer) {
                RotoLayerPtr copy( new RotoLayer(*isLayer) );
                copy->setParentLayer(this_shared);
                _imp->items.push_back(copy);
                getContext()->addLayer(copy);
            }
        }
    }
}

void
RotoLayer::save(RotoItemSerialization *obj) const
{
    RotoLayerSerialization* s = dynamic_cast<RotoLayerSerialization*>(obj);

    assert(s);
    if (!s) {
        throw std::logic_error("RotoLayer::save");
    }
    RotoItems items;
    {
        QMutexLocker l(&itemMutex);
        items = _imp->items;
    }

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        RotoStrokeItemPtr isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(*it);
        RotoLayerPtr layer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        RotoItemSerializationPtr childSerialization;
        if (isBezier && !isStroke) {
            childSerialization = boost::make_shared<BezierSerialization>();
            isBezier->save( childSerialization.get() );
        } else if (isStroke) {
            childSerialization = boost::make_shared<RotoStrokeItemSerialization>();
            isStroke->save( childSerialization.get() );
        } else {
            assert(layer);
            if (layer) {
                childSerialization = boost::make_shared<RotoLayerSerialization>();
                layer->save( childSerialization.get() );
            }
        }
        assert(childSerialization);
        s->children.push_back(childSerialization);
    }


    RotoItem::save(obj);
}

void
RotoLayer::load(const RotoItemSerialization &obj)
{
    const RotoLayerSerialization & s = dynamic_cast<const RotoLayerSerialization &>(obj);
    RotoLayerPtr this_layer = boost::dynamic_pointer_cast<RotoLayer>( shared_from_this() );

    assert(this_layer);
    RotoItem::load(obj);
    {
        for (std::list<RotoItemSerializationPtr>::const_iterator it = s.children.begin(); it != s.children.end(); ++it) {
            BezierSerializationPtr b = boost::dynamic_pointer_cast<BezierSerialization>(*it);
            RotoStrokeItemSerializationPtr s = boost::dynamic_pointer_cast<RotoStrokeItemSerialization>(*it);
            RotoLayerSerializationPtr l = boost::dynamic_pointer_cast<RotoLayerSerialization>(*it);
            if (b && !s) {
                BezierPtr bezier( new Bezier(getContext(), kRotoBezierBaseName, RotoLayerPtr(), false) );
                bezier->createNodes(false);
                bezier->load(*b);
                if ( !bezier->getParentLayer() ) {
                    bezier->setParentLayer(this_layer);
                }
                QMutexLocker l(&itemMutex);
                _imp->items.push_back(bezier);
            } else if (s) {
                RotoStrokeItemPtr stroke( new RotoStrokeItem( (RotoStrokeType)s->getType(), getContext(), kRotoPaintBrushBaseName, RotoLayerPtr() ) );
                stroke->createNodes(false);
                stroke->load(*s);
                if ( !stroke->getParentLayer() ) {
                    stroke->setParentLayer(this_layer);
                }


                QMutexLocker l(&itemMutex);
                _imp->items.push_back(stroke);
            } else if (l) {
                RotoLayerPtr layer( new RotoLayer(getContext(), kRotoLayerBaseName, this_layer) );
                _imp->items.push_back(layer);
                getContext()->addLayer(layer);
                layer->load(*l);
                if ( !layer->getParentLayer() ) {
                    layer->setParentLayer(this_layer);
                }
            }
            //Rotopaint tree nodes use the roto context age for their script-name, make sure they have a different one
            getContext()->incrementAge();
        }
    }
}

void
RotoLayer::addItem(const RotoItemPtr & item,
                   bool declareToPython )
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    RotoLayerPtr parentLayer = item->getParentLayer();
    if (parentLayer) {
        parentLayer->removeItem(item);
    }

    item->setParentLayer( boost::dynamic_pointer_cast<RotoLayer>( shared_from_this() ) );
    {
        QMutexLocker l(&itemMutex);
        _imp->items.push_back(item);
    }
    if (declareToPython) {
        getContext()->declareItemAsPythonField(item);
    }
    getContext()->refreshRotoPaintTree();
}

void
RotoLayer::insertItem(const RotoItemPtr & item,
                      int index)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(index >= 0);

    RotoLayerPtr parentLayer = item->getParentLayer();
    if ( parentLayer && (parentLayer.get() != this) ) {
        parentLayer->removeItem(item);
    }


    {
        QMutexLocker l(&itemMutex);
        if (parentLayer.get() != this) {
            item->setParentLayer( boost::dynamic_pointer_cast<RotoLayer>( shared_from_this() ) );
        } else {
            RotoItems::iterator found = std::find(_imp->items.begin(), _imp->items.end(), item);
            if ( found != _imp->items.end() ) {
                _imp->items.erase(found);
            }
        }
        RotoItems::iterator it = _imp->items.begin();
        if ( index >= (int)_imp->items.size() ) {
            it = _imp->items.end();
        } else {
            std::advance(it, index);
        }
        ///insert before the iterator
        _imp->items.insert(it, item);
    }
    getContext()->declareItemAsPythonField(item);
    getContext()->refreshRotoPaintTree();
}

void
RotoLayer::removeItem(const RotoItemPtr& item)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&itemMutex);
        for (RotoItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            if (*it == item) {
                l.unlock();
                getContext()->removeItemAsPythonField(item);
                l.relock();
                _imp->items.erase(it);
                break;
            }
        }
    }
    item->setParentLayer( RotoLayerPtr() );
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( item.get() );
    if (isStroke) {
        isStroke->disconnectNodes();
    }
    getContext()->refreshRotoPaintTree();
}

int
RotoLayer::getChildIndex(const RotoItemPtr & item) const
{
    QMutexLocker l(&itemMutex);
    int i = 0;

    for (RotoItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it, ++i) {
        if (*it == item) {
            return i;
        }
    }

    return -1;
}

const RotoItems &
RotoLayer::getItems() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->items;
}

RotoItems
RotoLayer::getItems_mt_safe() const
{
    QMutexLocker l(&itemMutex);

    return _imp->items;
}

NATRON_NAMESPACE_EXIT
