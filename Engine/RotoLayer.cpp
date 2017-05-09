/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/KnobTypes.h"
#include "Engine/RotoPaint.h"
#include "Engine/Transform.h"
#include "Serialization/KnobTableItemSerialization.h"

NATRON_NAMESPACE_ENTER;


RotoLayer::RotoLayer(const KnobItemsTablePtr& model)
    : RotoItem(model)
{
}


RotoLayer::~RotoLayer()
{
}

RotoLayer::RotoLayer(const RotoLayerPtr& other, const FrameViewRenderKey& key)
: RotoItem(other, key)
{

}

bool
RotoLayer::isItemContainer() const
{
    return true;
}

std::string
RotoLayer::getBaseItemName() const
{
    return tr(kRotoLayerBaseName).toStdString();
}

std::string
RotoLayer::getSerializationClassName() const
{
    return kSerializationRotoGroupTag;
}

struct PlanarTrackLayerPrivate
{

    // Transform
    KnobDoubleWPtr translate;
    KnobDoubleWPtr rotate;
    KnobDoubleWPtr scale;
    KnobBoolWPtr scaleUniform;
    KnobDoubleWPtr skewX;
    KnobDoubleWPtr skewY;
    KnobChoiceWPtr skewOrder;
    KnobDoubleWPtr center;
    KnobDoubleWPtr extraMatrix;
    
    PlanarTrackLayerPrivate()
    {

    }
};

PlanarTrackLayer::PlanarTrackLayer(const KnobItemsTablePtr& model)
: RotoLayer(model)
, _imp(new PlanarTrackLayerPrivate())
{

}

PlanarTrackLayer::PlanarTrackLayer(const PlanarTrackLayerPtr& other, const FrameViewRenderKey& key)
: RotoLayer(other, key)
{

}

PlanarTrackLayer::~PlanarTrackLayer()
{

}

std::string
PlanarTrackLayer::getBaseItemName() const
{
    return kPlanarTrackLayerBaseName;
}

std::string
PlanarTrackLayer::getSerializationClassName() const
{
    return kSerializationRotoPlanarTrackGroupTag;
}

void
PlanarTrackLayer::initializeKnobs()
{
    // Transform
    _imp->translate = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemTranslateParam);
    _imp->rotate = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemRotateParam);
    _imp->scale = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemScaleParam);
    _imp->scaleUniform = createDuplicateOfTableKnob<KnobBool>(kRotoDrawableItemScaleUniformParam);
    _imp->skewX = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemSkewXParam);
    _imp->skewY = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemSkewYParam);
    _imp->skewOrder = createDuplicateOfTableKnob<KnobChoice>(kRotoDrawableItemSkewOrderParam);
    _imp->center = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemCenterParam);
    _imp->extraMatrix = createDuplicateOfTableKnob<KnobDouble>(kRotoDrawableItemExtraMatrixParam);

    RotoLayer::initializeKnobs();

} // initializeKnobs

void
PlanarTrackLayer::fetchRenderCloneKnobs()
{
    // Transform
    _imp->translate = getKnobByNameAndType<KnobDouble>(kRotoDrawableItemTranslateParam);
    _imp->rotate = getKnobByNameAndType<KnobDouble>(kRotoDrawableItemRotateParam);
    _imp->scale = getKnobByNameAndType<KnobDouble>(kRotoDrawableItemScaleParam);
    _imp->scaleUniform = getKnobByNameAndType<KnobBool>(kRotoDrawableItemScaleUniformParam);
    _imp->skewX = getKnobByNameAndType<KnobDouble>(kRotoDrawableItemSkewXParam);
    _imp->skewY = getKnobByNameAndType<KnobDouble>(kRotoDrawableItemSkewYParam);
    _imp->skewOrder = getKnobByNameAndType<KnobChoice>(kRotoDrawableItemSkewOrderParam);
    _imp->center = getKnobByNameAndType<KnobDouble>(kRotoDrawableItemCenterParam);
    _imp->extraMatrix = getKnobByNameAndType<KnobDouble>(kRotoDrawableItemExtraMatrixParam);

    RotoLayer::fetchRenderCloneKnobs();

} // fetchRenderCloneKnobs

bool
PlanarTrackLayer::getTransformAtTimeInternal(TimeValue time, ViewIdx view, Transform::Matrix3x3* matrix) const
{
    KnobDoublePtr translate = _imp->translate.lock();
    if (!translate) {
        return false;
    }
    KnobDoublePtr rotate = _imp->rotate.lock();
    KnobBoolPtr scaleUniform = _imp->scaleUniform.lock();
    KnobDoublePtr scale = _imp->scale.lock();
    KnobDoublePtr skewXKnob = _imp->skewX.lock();
    KnobDoublePtr skewYKnob = _imp->skewY.lock();
    KnobDoublePtr centerKnob = _imp->center.lock();
    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    KnobChoicePtr skewOrder = _imp->skewOrder.lock();

    double tx = translate->getValueAtTime(time, DimIdx(0), view);
    double ty = translate->getValueAtTime(time, DimIdx(1), view);
    double sx = scale->getValueAtTime(time, DimIdx(0), view);
    double sy = scaleUniform->getValueAtTime(time) ? sx : scale->getValueAtTime(time, DimIdx(1), view);
    double skewX = skewXKnob->getValueAtTime(time, DimIdx(0), view);
    double skewY = skewYKnob->getValueAtTime(time, DimIdx(0), view);
    double rot = rotate->getValueAtTime(time, DimIdx(0), view);

    rot = Transform::toRadians(rot);
    double centerX = centerKnob->getValueAtTime(time, DimIdx(0), view);
    double centerY = centerKnob->getValueAtTime(time, DimIdx(1), view);
    bool skewOrderYX = skewOrder->getValueAtTime(time) == 1;
    *matrix = Transform::matTransformCanonical(tx, ty, sx, sy, skewX, skewY, skewOrderYX, rot, centerX, centerY);

    Transform::Matrix3x3 extraMat;
    extraMat.a = extraMatrix->getValueAtTime(time, DimIdx(0), view); extraMat.b = extraMatrix->getValueAtTime(time, DimIdx(1), view); extraMat.c = extraMatrix->getValueAtTime(time, DimIdx(2), view);
    extraMat.d = extraMatrix->getValueAtTime(time, DimIdx(3), view); extraMat.e = extraMatrix->getValueAtTime(time, DimIdx(4), view); extraMat.f = extraMatrix->getValueAtTime(time, DimIdx(5), view);
    extraMat.g = extraMatrix->getValueAtTime(time, DimIdx(6), view); extraMat.h = extraMatrix->getValueAtTime(time, DimIdx(7), view); extraMat.i = extraMatrix->getValueAtTime(time, DimIdx(8), view);
    *matrix = Transform::matMul(*matrix, extraMat);
    return true;
}

KnobHolderPtr
PlanarTrackLayer::createRenderCopy(const FrameViewRenderKey& render) const
{
    PlanarTrackLayerPtr mainInstance = toPlanarTrackLayer(getMainInstance());
    if (!mainInstance) {
        mainInstance = toPlanarTrackLayer(boost::const_pointer_cast<KnobHolder>(shared_from_this()));
    }
    PlanarTrackLayerPtr ret(new PlanarTrackLayer(mainInstance, render));
    return ret;
} // createRenderCopy


void
PlanarTrackLayer::setExtraMatrix(bool setKeyframe,
                                 TimeValue time,
                                 ViewSetSpec view,
                                 const Transform::Matrix3x3& mat)
{
    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    if (!extraMatrix) {
        return;
    }
    ScopedChanges_RAII changes(extraMatrix.get());
    if (setKeyframe) {
        std::vector<double> matValues(9);
        memcpy(&matValues[0], &mat.a, 9 * sizeof(double));
        extraMatrix->setValueAtTime(time, mat.a, view, DimIdx(0)); extraMatrix->setValueAtTime(time, mat.b, view, DimIdx(1)); extraMatrix->setValueAtTime(time, mat.c, view, DimIdx(2));
        extraMatrix->setValueAtTime(time, mat.d, view, DimIdx(3)); extraMatrix->setValueAtTime(time, mat.e, view, DimIdx(4)); extraMatrix->setValueAtTime(time, mat.f, view, DimIdx(5));
        extraMatrix->setValueAtTime(time, mat.g, view, DimIdx(6)); extraMatrix->setValueAtTime(time, mat.h, view, DimIdx(7)); extraMatrix->setValueAtTime(time, mat.i, view, DimIdx(8));
    } else {
        extraMatrix->setValue(mat.a, view, DimIdx(0)); extraMatrix->setValue(mat.b, view, DimIdx(1)); extraMatrix->setValue(mat.c, view, DimIdx(2));
        extraMatrix->setValue(mat.d, view, DimIdx(3)); extraMatrix->setValue(mat.e, view, DimIdx(4)); extraMatrix->setValue(mat.f, view, DimIdx(5));
        extraMatrix->setValue(mat.g, view, DimIdx(6)); extraMatrix->setValue(mat.h, view, DimIdx(7)); extraMatrix->setValue(mat.i, view, DimIdx(8));
    }
}

void
PlanarTrackLayer::clearTransformAnimation()
{

    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    extraMatrix->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    if (!extraMatrix->hasAnimation()) {
        Transform::Matrix3x3 identity;
        identity.setIdentity();
        setExtraMatrix(false, TimeValue(0.), ViewIdx(0), identity);
    }
}

void
PlanarTrackLayer::clearTransformAnimationBeforeTime(TimeValue time)
{
    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    extraMatrix->deleteValuesBeforeTime(std::set<double>(), time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    if (!extraMatrix->hasAnimation()) {
        Transform::Matrix3x3 identity;
        identity.setIdentity();
        setExtraMatrix(false, TimeValue(0.), ViewIdx(0), identity);
    }

}

void
PlanarTrackLayer::deleteTransformKeyframe(TimeValue time)
{
    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    extraMatrix->deleteValueAtTime(time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    if (!extraMatrix->hasAnimation()) {
        Transform::Matrix3x3 identity;
        identity.setIdentity();
        setExtraMatrix(false, TimeValue(0.), ViewIdx(0), identity);
    }

}

void
PlanarTrackLayer::clearTransformAnimationAfterTime(TimeValue time)
{
    KnobDoublePtr extraMatrix = _imp->extraMatrix.lock();
    extraMatrix->deleteValuesAfterTime(std::set<double>(), time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    if (!extraMatrix->hasAnimation()) {
        Transform::Matrix3x3 identity;
        identity.setIdentity();
        setExtraMatrix(false, TimeValue(0.), ViewIdx(0), identity);
    }

}

NATRON_NAMESPACE_EXIT;
