/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/RotoPaint.h"
#include "Engine/Interpolation.h"
#include "Engine/RenderStats.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoPaint.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"




#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER


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
    {
    }

};

RotoItem::RotoItem(const KnobItemsTablePtr& model)
    : KnobTableItem(model)
    , _imp( new RotoItemPrivate() )
{
}

RotoItem::RotoItem(const RotoItemPtr& other, const FrameViewRenderKey& key)
: KnobTableItem(other, key)
, _imp(new RotoItemPrivate())
{

}

RotoItem::~RotoItem()
{
}

bool
RotoItem::getTransformAtTimeInternal(TimeValue /*time*/, ViewIdx /*view*/, Transform::Matrix3x3* /*matrix*/) const
{
    return false;
}

void
RotoItem::getTransformAtTime(TimeValue time, ViewIdx view, Transform::Matrix3x3* matrix) const
{
    Transform::Matrix3x3 tmpMat;
    if (getTransformAtTimeInternal(time, view, &tmpMat)) {
        *matrix = tmpMat;
    } else {
        matrix->setIdentity();
    }
    // Get the transform recursively by concatenating transform matrices
    // on parent groups

    KnobHolderPtr parentHolder = getParent();
    RotoItemPtr parent = toRotoItem(parentHolder);
    while (parent) {
        Transform::Matrix3x3 tmpMat;
        if (parent->getTransformAtTimeInternal(time, view, &tmpMat)) {
            *matrix = Transform::matMul(*matrix, tmpMat);
        }
        parentHolder = parent->getParent();
        parent = toRotoItem(parentHolder);
    }

}

void
RotoItem::fetchRenderCloneKnobs()
{
    KnobTableItem::fetchRenderCloneKnobs();

    _imp->activatedKnob = getKnobByNameAndType<KnobButton>(kParamRotoItemEnabled);
    _imp->lockedKnob = getKnobByNameAndType<KnobButton>(kParamRotoItemLocked);
    _imp->soloKnob = getKnobByNameAndType<KnobButton>(kParamRotoItemSolo);
}

void
RotoItem::initializeKnobs()
{
    {
        KnobButtonPtr param = createKnob<KnobButton>(kParamRotoItemEnabled);
        param->setLabel(tr(kParamRotoItemEnabledLabel));
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
        KnobButtonPtr param = createKnob<KnobButton>(kParamRotoItemLocked);
        param->setLabel(tr(kParamRotoItemLockedLabel));
        param->setHintToolTip(tr(kParamRotoItemLockedHint));
        param->setIconLabel("Images/locked.png", true);
        param->setIconLabel("Images/unlocked.png", false);
        param->setCheckable(true);
        param->setDefaultValue(false);
        _imp->lockedKnob = param;
    }

    if (type == RotoPaint::eRotoPaintTypeComp) {
        KnobButtonPtr param = createKnob<KnobButton>(kParamRotoItemSolo);
        param->setLabel(tr(kParamRotoItemSoloLabel));
        param->setHintToolTip(tr(kParamRotoItemSoloHint));
        param->setIconLabel("Images/soloOn.png", true);
        param->setIconLabel("Images/soloOff.png", false);
        param->setCheckable(true);
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

    KnobTableItem::initializeKnobs();

}


bool
RotoItem::isGloballyActivated() const
{
    KnobButtonPtr knob = _imp->activatedKnob.lock();
    if (knob) {
        if (!knob->getValue()) {
            return false;
        }
    }
    RotoDrawableItemPtr isDrawable = boost::const_pointer_cast<RotoDrawableItem>(boost::dynamic_pointer_cast<const RotoDrawableItem>(shared_from_this()));
    RotoPaintPtr rotoEffect = toRotoPaint(getModel()->getNode()->getEffectInstance());
    if (isDrawable && rotoEffect) {
        if (!rotoEffect->isAmongstSoloItems(isDrawable)) {
            return false;
        }
    }
    return true;
}

static bool
isActivatedRecursive(const RotoItemPtr& item)
{
    if ( !item->isGloballyActivated() ) {
        return false;
    }
    RotoLayerPtr parent = toRotoLayer(item->getParent());
    if (parent) {
        return isActivatedRecursive(parent);
    }
    return true;
}

bool
RotoItem::isGloballyActivatedRecursive() const
{
    RotoItemPtr thisShared = boost::const_pointer_cast<RotoItem>(toRotoItem(shared_from_this()));
    return isActivatedRecursive(thisShared);
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

KnobButtonPtr
RotoItem::getSoloKnob() const
{
    return _imp->soloKnob.lock();
}


static
bool
isLocked_imp(const RotoLayerPtr& item)
{
    if ( item->getLockedKnob()->getValue() ) {
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
                             ValueChangedReasonEnum reason,
                             TimeValue /*time*/,
                             ViewSetSpec /*view*/)
{
    if (knob == _imp->lockedKnob.lock()) {

        const KnobsVec& knobs = getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            if (*it != knob) {
                (*it)->setEnabled(!_imp->lockedKnob.lock()->getValue());
            }
        }
        return true;
    } else if (knob == _imp->activatedKnob.lock()) {
        if (reason == eValueChangedReasonUserEdited) {
            std::vector<KnobTableItemPtr> children = getChildren();
            for (std::vector<KnobTableItemPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
                KnobHolderPtr item = *it;
                RotoItemPtr rotoItem = toRotoItem(item);
                rotoItem->getActivatedKnob()->setValue(getActivatedKnob()->getValue());
            }
        }
    } else if (knob == _imp->soloKnob.lock()) {
        bool isSolo = getSoloKnob()->getValue();
        RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(shared_from_this());
        if (isDrawable) {
            RotoPaintPtr rotoEffect = toRotoPaint(getModel()->getNode()->getEffectInstance());
            if (isSolo) {
                rotoEffect->addSoloItem(isDrawable);
            } else {
                rotoEffect->removeSoloItem(isDrawable);
            }
        }
        if (reason == eValueChangedReasonUserEdited) {
            std::vector<KnobTableItemPtr> children = getChildren();
            for (std::vector<KnobTableItemPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
                KnobHolderPtr item = *it;
                RotoItemPtr rotoItem = toRotoItem(item);
                rotoItem->getSoloKnob()->setValue(isSolo);
            }
        }
    }
    return false;
}


NATRON_NAMESPACE_EXIT
