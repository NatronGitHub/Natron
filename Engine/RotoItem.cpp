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

#include "RotoItem.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cassert>
#include <stdexcept>

#include <QtCore/QLineF>
#include <QtCore/QDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Global/MemoryInfo.h"

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Node.h"
#include "Engine/RotoPaint.h"
#include "Engine/Interpolation.h"
#include "Engine/RenderStats.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"




#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER;


////////////////////////////////////RotoItem////////////////////////////////////


NATRON_NAMESPACE_ANONYMOUS_ENTER

class RotoMetaTypesRegistration
{
public:
    inline RotoMetaTypesRegistration()
    {
        qRegisterMetaType<RotoItem*>("RotoItem");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


static RotoMetaTypesRegistration registration;

struct RotoItemPrivate
{
    
    
    // This controls whether the item (and all its children if it is a layer)
    // should be visible/rendered or not at any time.
    // This is different from the "activated" knob for RotoDrawableItem's which in that
    // case allows to define a life-time
    KnobButtonWPtr activatedKnob;
    
    // A locked item should not be modifiable by the GUI
    KnobButtonWPtr lockedKnob;

    // Includes the current item in renders, ignoring layers without this switch set
    KnobButtonWPtr soloKnob;
    
    RotoItemPrivate()
    : activatedKnob()
    , lockedKnob()
    , soloKnob()
    {
    }
};

RotoItem::RotoItem(const KnobItemsTablePtr& model)
    : KnobTableItem(model)
    , _imp( new RotoItemPrivate() )
{
}

RotoItem::~RotoItem()
{
}



void
RotoItem::setGloballyActivated_recursive(bool a)
{
    {
        _imp->activatedKnob.lock()->setValue(a);
        RotoLayer* layer = dynamic_cast<RotoLayer*>(this);
        if (layer) {
            std::vector<KnobTableItemPtr> children = layer->getChildren();
            for (std::vector<KnobTableItemPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
                KnobHolderPtr item = *it;
                RotoItemPtr rotoItem = toRotoItem(item);
                rotoItem->setGloballyActivated_recursive(a);
            }
        }
    }
    invalidateCacheHashAndEvaluate(true, false);
}

void
RotoItem::initializeKnobs()
{
    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(shared_from_this(), tr(kParamRotoItemEnabledLabel));
        param->setName(kParamRotoItemEnabled);
        param->setHintToolTip(tr(kParamRotoItemEnabledHint));
        param->setIconLabel("Images/visible.png", true);
        param->setIconLabel("Images/unvisible.png", false);
        param->setCheckable(true);
        param->setDefaultValue(true);
        _imp->activatedKnob = param;
    }
    RotoPaintPtr effect = toRotoPaint(getModel()->getNode()->getEffectInstance());
    assert(effect);
    RotoPaint::RotoPaintTypeEnum type = effect->getRotoPaintNodeType();

    if (type == RotoPaint::eRotoPaintTypeRoto ||
        type == RotoPaint::eRotoPaintTypeRotoPaint) {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(shared_from_this(), tr(kParamRotoItemLockedLabel));
        param->setName(kParamRotoItemLocked);
        param->setHintToolTip(tr(kParamRotoItemLockedHint));
        param->setIconLabel("Images/locked.png", true);
        param->setIconLabel("Images/unlocked.png", false);
        param->setCheckable(true);
        param->setDefaultValue(true);
        _imp->lockedKnob = param;
    }

    if (type == RotoPaint::eRotoPaintTypeComp) {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(shared_from_this(), tr(kParamRotoItemSoloLabel));
        param->setName(kParamRotoItemSolo);
        param->setHintToolTip(tr(kParamRotoItemSoloHint));
        //param->setIconLabel("Images/soloOn.png", true);
        //param->setIconLabel("Images/soloOff.png", false);
        param->setCheckable(true);
        param->setDefaultValue(true);
        _imp->soloKnob = param;
    }

    addColumn(kKnobTableItemColumnLabel, DimIdx(0));
    addColumn(kParamRotoItemEnabled, DimIdx(0));
    if (type == RotoPaint::eRotoPaintTypeComp) {
        addColumn(kParamRotoItemSolo, DimIdx(0));
    }
    if (type == RotoPaint::eRotoPaintTypeRoto ||
        type == RotoPaint::eRotoPaintTypeRotoPaint) {
        addColumn(kParamRotoItemLocked, DimIdx(0));
    }

}

void
RotoItem::setGloballyActivated(bool a,
                               bool setChildren)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    if (setChildren) {
        setGloballyActivated_recursive(a);
    } else {
        _imp->activatedKnob.lock()->setValue(a);
    }
    invalidateCacheHashAndEvaluate(true, false);
}

bool
RotoItem::isGloballyActivated() const
{
    KnobButtonPtr knob = _imp->activatedKnob.lock();
    return knob ? knob->getValue() : true;
}

static bool
isDeactivated_imp(const RotoLayerPtr& item)
{
    if ( !item->isGloballyActivated() ) {
        return true;
    } else {
        RotoLayerPtr parent = toRotoLayer(item->getParent());
        if (parent) {
            return isDeactivated_imp(parent);
        }
    }

    return false;
}

bool
RotoItem::isDeactivatedRecursive() const
{
    RotoLayerPtr parent;
    {
        if (!_imp->activatedKnob.lock()->getValue()) {
            return true;
        }
        parent = toRotoLayer(getParent());
    }

    if (parent) {
        return isDeactivated_imp(parent);
    }

    return false;
}

void
RotoItem::setLocked_recursive(bool locked)
{
    {

        _imp->lockedKnob.lock()->setValue(locked);
        RotoLayer* layer = dynamic_cast<RotoLayer*>(this);
        if (layer) {
            std::vector<KnobTableItemPtr> children = layer->getChildren();
            for (std::vector<KnobTableItemPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
                KnobHolderPtr item = *it;
                RotoItemPtr rotoItem = toRotoItem(item);
                rotoItem->setLocked_recursive(locked);
            }
        }
    }
}

void
RotoItem::setLocked(bool l,
                    bool lockChildren)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    if (!lockChildren) {
        _imp->lockedKnob.lock()->setValue(l);
    } else {
        setLocked_recursive(l);
    }
}

bool
RotoItem::getLocked() const
{
    return _imp->lockedKnob.lock()->getValue();
}

KnobButtonPtr
RotoItem::getLockedKnob() const
{
    return _imp->lockedKnob.lock();
}

KnobButtonPtr
RotoItem::getActivatedKnob() const
{
    return _imp->activatedKnob.lock();
}



static
bool
isLocked_imp(const RotoLayerPtr& item)
{
    if ( item->getLocked() ) {
        return true;
    } else {
        RotoLayerPtr parent = toRotoLayer(item->getParent());
        if (parent) {
            return isLocked_imp(parent);
        }
    }

    return false;
}

bool
RotoItem::isLockedRecursive() const
{
    KnobButtonPtr lockedKnob = _imp->lockedKnob.lock();
    if (!lockedKnob) {
        return false;
    }
    bool thisItemLocked = lockedKnob->getValue();
    if (thisItemLocked) {
        return true;
    }
    RotoLayerPtr parent = toRotoLayer(getParent());
    if (parent) {
        return isLocked_imp(parent);
    } else {
        return false;
    }
}

bool
RotoItem::onKnobValueChanged(const KnobIPtr& knob,
                             ValueChangedReasonEnum /*reason*/,
                             double /*time*/,
                             ViewSetSpec /*view*/,
                             bool /*originatedFromMainThread*/)
{
#pragma message WARN("check this")
    if (knob == _imp->lockedKnob.lock()) {
        KnobItemsTablePtr model = getModel();
        if (!model) {
            return false;
        }
        int nbBeziersUnLockedBezier = 0;

        {
            std::list<KnobTableItemPtr> selectedItems = getModel()->getSelectedItems();
            for (std::list<KnobTableItemPtr >::iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
                BezierPtr isBezier = toBezier(*it);
                if ( isBezier && !isBezier->isLockedRecursive() ) {
                    ++nbBeziersUnLockedBezier;
                }
            }
        }
        bool dirty = nbBeziersUnLockedBezier > 1;
        bool enabled = nbBeziersUnLockedBezier > 0;

        const KnobsVec& knobs = getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            knob->setKnobSelectedMultipleTimes(dirty);
            knob->setEnabled(enabled);
        }
        return true;
    }
    return false;
}


NATRON_NAMESPACE_EXIT;
