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

#include "AnimItemBase.h"

#include <stdexcept>

#include "Engine/AnimatingObjectI.h"
#include "Engine/Curve.h"
#include "Gui/KnobAnim.h"

NATRON_NAMESPACE_ENTER

class AnimItemBasePrivate
{
public:

    AnimItemBase* publicInterface;
    AnimationModuleBaseWPtr model;

    AnimItemBasePrivate(AnimItemBase* publicInterface, const AnimationModuleBasePtr& model)
    : publicInterface(publicInterface)
    , model(model)
    {

    }

    void addKeyFramesForDimView(DimIdx dimension, ViewIdx view, KeyFrameSet *result) const;
    
    
};



AnimItemBase::AnimItemBase(const AnimationModuleBasePtr& model)
: _imp(new AnimItemBasePrivate(this, model))
{

}

AnimItemBase::~AnimItemBase()
{

}

AnimationModuleBasePtr
AnimItemBase::getModel() const
{
    return _imp->model.lock();
}

void
AnimItemBasePrivate::addKeyFramesForDimView(DimIdx dimension, ViewIdx view, KeyFrameSet *result) const
{
    CurvePtr curve = publicInterface->getCurve(dimension, view);
    if (!curve) {
        return;
    }
    *result = curve->getKeyFrames_mt_safe();

}

void
AnimItemBase::getKeyframes(DimSpec dimension, ViewSetSpec viewSpec, GetKeyframesTypeEnum type, KeyFrameSet *result) const
{

    assert(viewSpec.isAll() || viewSpec.isViewIdx());
    std::list<DimensionViewPair> curvesToProcess;
    if (dimension.isAll()) {
        int nDims = getNDimensions();

        for (int i = 0; i < nDims; ++i) {
            if (viewSpec.isAll()) {
                std::list<ViewIdx> views = getViewsList();
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                    if (i > 0) {
                        bool allDimensionsVisible = getAllDimensionsVisible(*it);
                        if (!allDimensionsVisible) {
                            continue;
                        }
                    }

                    DimensionViewPair p = {DimIdx(i), *it};
                    curvesToProcess.push_back(p);
                }
            } else {
                assert(viewSpec.isViewIdx());
                if (i > 0) {
                    bool allDimensionsVisible = getAllDimensionsVisible(ViewIdx(viewSpec));
                    if (!allDimensionsVisible) {
                        continue;
                    }
                }


                DimensionViewPair p = {DimIdx(i), ViewIdx(viewSpec)};
                curvesToProcess.push_back(p);
            }
        }
    } else {
        if (viewSpec.isAll()) {
            std::list<ViewIdx> views = getViewsList();
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                DimensionViewPair p = {DimIdx(dimension), *it};
                curvesToProcess.push_back(p);
            }

        } else {
            assert(viewSpec.isViewIdx());
            DimensionViewPair p = {DimIdx(dimension), ViewIdx(viewSpec.value())};
            curvesToProcess.push_back(p);
        }
    }
    assert(!curvesToProcess.empty());
    if (curvesToProcess.empty()) {
        return;
    }

    if (type == eGetKeyframesTypeMerged) {
        for (std::list<DimensionViewPair>::const_iterator it = curvesToProcess.begin(); it!=curvesToProcess.end(); ++it) {
            _imp->addKeyFramesForDimView(it->dimension, it->view, result);
        }
    } else {
        // Get keyframes from first curve and then check if present in other curves
        const DimensionViewPair& firstDimView = curvesToProcess.front();

        KeyFrameSet tmpResult;
        _imp->addKeyFramesForDimView(firstDimView.dimension, firstDimView.view, &tmpResult);

        KeyFrame k;
        for (KeyFrameSet::const_iterator it2 = tmpResult.begin(); it2 != tmpResult.end(); ++it2) {

            bool hasKeyFrameForAllCurves = true;

            std::list<DimensionViewPair>::const_iterator it = curvesToProcess.begin();
            ++it;
            for (; it != curvesToProcess.end(); ++it) {
                CurvePtr curve = getCurve(it->dimension, it->view);
                if (!curve) {
                    return;
                }

                if (!curve->getKeyFrameWithTime(it2->getTime(), &k)) {
                    hasKeyFrameForAllCurves = false;
                    break;
                }
                
            }
            if (hasKeyFrameForAllCurves) {
                result->insert(*it2);
            }
        }

    }
} // getKeyframes

void
AnimItemBase::getKeyframes(DimSpec dimension, ViewSetSpec viewSpec, AnimItemDimViewKeyFramesMap *result) const
{

    assert(viewSpec.isAll() || viewSpec.isViewIdx());


    AnimItemBasePtr thisShared = boost::const_pointer_cast<AnimItemBase>(shared_from_this());
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        int nDims = getNDimensions();
        for (int i = 0; i < nDims; ++i) {
            if (viewSpec.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                    if (i > 0) {
                        bool allDimensionsVisible = getAllDimensionsVisible(*it);
                        if (!allDimensionsVisible) {
                            continue;
                        }
                    }
                    AnimItemDimViewIndexID id;
                    id.item = thisShared;
                    id.dim = DimIdx(i);
                    id.view = *it;
                    KeyFrameSet& keys = (*result)[id];
                    _imp->addKeyFramesForDimView(DimIdx(i), *it, &keys);
                }
            } else {
                assert(viewSpec.isViewIdx());
                if (i > 0) {
                    bool allDimensionsVisible = getAllDimensionsVisible(ViewIdx(viewSpec));
                    if (!allDimensionsVisible) {
                        continue;
                    }
                }
                AnimItemDimViewIndexID id;
                id.item = thisShared;
                id.dim = DimIdx(i);
                id.view = ViewIdx(viewSpec.value());
                KeyFrameSet& keys = (*result)[id];
                _imp->addKeyFramesForDimView(DimIdx(i), ViewIdx(viewSpec.value()), &keys);
            }
        }
    } else {
        if (viewSpec.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                AnimItemDimViewIndexID id;
                id.item = thisShared;
                id.dim = DimIdx(dimension.value());
                id.view = *it;
                KeyFrameSet& keys = (*result)[id];
                _imp->addKeyFramesForDimView(DimIdx(dimension.value()), *it, &keys);
            }

        } else {
            assert(viewSpec.isViewIdx());
            AnimItemDimViewIndexID id;
            id.item = thisShared;
            id.dim = DimIdx(dimension.value());
            id.view = ViewIdx(viewSpec.value());
            KeyFrameSet& keys = (*result)[id];

            _imp->addKeyFramesForDimView(DimIdx(dimension.value()), ViewIdx(viewSpec.value()), &keys);
        }
    }
} // getKeyframes

double
AnimItemBase::evaluateCurve(bool /*useExpressionIfAny*/, double x, DimIdx dimension, ViewIdx view)
{
    CurvePtr curve = getCurve(dimension, view);
    assert(curve);
    if (!curve) {
        throw std::runtime_error("Curve is null");
    }
    return curve->getValueAt(TimeValue(x), false /*doClamp*/).getValue();
   
}

KnobAnimPtr
KnobsHolderAnimBase::findKnobAnim(const KnobIPtr& knob) const
{
    const std::vector<KnobAnimPtr>& knobs = getKnobs();
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getInternalKnob() == knob) {
            return knobs[i];
        }
    }
    return KnobAnimPtr();
}


NATRON_NAMESPACE_EXIT
