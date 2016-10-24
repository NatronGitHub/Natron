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

#include "AnimItemBase.h"

#include "Engine/AnimatingObjectI.h"
#include "Engine/Curve.h"
#include "Engine/StringAnimationManager.h"

NATRON_NAMESPACE_ENTER;

class AnimItemBasePrivate
{
public:

    AnimItemBase* publicInterface;
    AnimationModuleWPtr model;

    AnimItemBasePrivate(AnimItemBase* publicInterface, const AnimationModulePtr& model)
    : publicInterface(publicInterface)
    , model(model)
    {

    }

    void addKeyFramesForDimView(DimIdx dimension, ViewIdx view, KeyFrameWithStringSet *result) const;
    
    
};



AnimItemBase::AnimItemBase(const AnimationModulePtr& model)
: _imp(new AnimItemBasePrivate(this, model))
{

}

AnimItemBase::~AnimItemBase()
{

}

AnimationModulePtr
AnimItemBase::getModel() const
{
    return _imp->model.lock();
}

void
AnimItemBasePrivate::addKeyFramesForDimView(DimIdx dimension, ViewIdx view, KeyFrameWithStringSet *result) const
{
    CurvePtr curve = publicInterface->getCurve(dimension, view);
    if (!curve) {
        return;
    }
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    AnimatingObjectIPtr animObject = publicInterface->getInternalAnimItem();
    StringAnimationManagerPtr stringAnim = animObject->getStringAnimation();
    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        KeyFrameWithString k;
        if (stringAnim) {
            stringAnim->stringFromInterpolatedIndex(it->getValue(), view, &k.string);
        }
        k.key = *it;
        result->insert(k);
    }
}

void
AnimItemBase::getKeyframes(DimSpec dimension, ViewSetSpec viewSpec, KeyFrameWithStringSet *result) const
{

    assert(viewSpec.isAll() || viewSpec.isViewIdx());



    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        int nDims = getNDimensions();
        for (int i = 0; i < nDims; ++i) {
            if (viewSpec.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                    _imp->addKeyFramesForDimView(DimIdx(i), *it, result);
                }
            } else {
                assert(viewSpec.isViewIdx());
                _imp->addKeyFramesForDimView(DimIdx(i), ViewIdx(viewSpec.value()), result);
            }
        }
    } else {
        if (viewSpec.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                _imp->addKeyFramesForDimView(DimIdx(dimension.value()), *it, result);
            }

        } else {
            assert(viewSpec.isViewIdx());
            _imp->addKeyFramesForDimView(DimIdx(dimension.value()), ViewIdx(viewSpec.value()), result);
        }
    }
}

#if 0
class AnimKeyFramePrivate
{

public:
    AnimItemBaseWPtr item;
    KeyFrame key;

    DimSpec dimension;
    ViewSetSpec view;

    // For animating string params we need to also store the string
    std::string animatingStringValue;

    AnimKeyFramePrivate(const AnimItemBasePtr& item,
                        const KeyFrame& kf,
                        DimSpec dimension,
                        ViewSetSpec view)
    : item(item)
    , key(kf)
    , dimension(dimension)
    , view(view)
    , animatingStringValue()
    {

    }

    AnimKeyFramePrivate(const AnimKeyFramePrivate& other)
    : item(other.item)
    , key(other.key)
    , dimension(other.dimension)
    , view(other.view)
    , animatingStringValue(other.animatingStringValue)
    {

    }
};

AnimKeyFrame::AnimKeyFrame(const AnimItemBasePtr& item, const KeyFrame& kf, DimSpec dimension, ViewSetSpec view)
: _imp(new AnimKeyFramePrivate(item, kf, dimension, view))
{
}

AnimKeyFrame::AnimKeyFrame(const AnimKeyFrame &other)
: _imp(new AnimKeyFramePrivate(*other._imp))
{
}

AnimItemBasePtr
AnimKeyFrame::getContext() const
{
    return _imp->item.lock();
}

const KeyFrame&
AnimKeyFrame::getKeyFrame() const
{
    return _imp->key;
}

const std::string&
AnimKeyFrame::getStringValue() const
{
    return _imp->animatingStringValue;
}


DimSpec
AnimKeyFrame::getDimension() const
{
    return _imp->dimension;
}

ViewSetSpec
AnimKeyFrame::getView() const
{
    return _imp->view;
}

bool
AnimKeyFrame::operator==(const AnimKeyFrame &other) const
{
    AnimItemBasePtr item = _imp->item.lock();
    AnimItemBasePtr otherItem = other._imp->item.lock();

    if ( !item || item != otherItem || item->getRootItem() != otherItem->getRootItem() ) {
        return false;
    }

    if ( _imp->key.getTime() != other._imp->key.getTime() ) {
        return false;
    }

    if (_imp->view != other._imp->view) {
        return false;
    }

    if (_imp->dimension != other._imp->dimension) {
        return false;
    }

    return true;
}


bool SortIncreasingFunctor::operator() (const AnimKeyFramePtr& lhs,
                                        const AnimKeyFramePtr& rhs)
{
    AnimItemBasePtr leftKnobDs = lhs->getContext();
    AnimItemBasePtr rightKnobDs = rhs->getContext();

    if ( leftKnobDs.get() < rightKnobDs.get() ) {
        return true;
    } else if ( leftKnobDs.get() > rightKnobDs.get() ) {
        return false;
    } else {
        if (lhs->getView() < rhs->getView()) {
            return true;
        } else if (lhs->getView() > rhs->getView()) {
            return false;
        } else {
            if (lhs->getDimension() < rhs->getDimension()) {
                return true;
            } else if (lhs->getDimension() > rhs->getDimension()) {
                return false;
            } else {
                return lhs->getKeyFrame().getTime() < rhs->getKeyFrame().getTime();
            }
        }
    }
}

bool SortDecreasingFunctor::operator() (const AnimKeyFramePtr& lhs,
                                        const AnimKeyFramePtr& rhs)
{
    AnimItemBasePtr leftKnobDs = lhs->getContext();
    AnimItemBasePtr rightKnobDs = rhs->getContext();

    assert(leftKnobDs && rightKnobDs);
    if ( leftKnobDs.get() > rightKnobDs.get() ) {
        return true;
    } else if ( leftKnobDs.get() < rightKnobDs.get() ) {
        return false;
    } else {
        if (lhs->getView() > rhs->getView()) {
            return true;
        } else if (lhs->getView() < rhs->getView()) {
            return false;
        } else {
            if (lhs->getDimension() > rhs->getDimension()) {
                return true;
            } else if (lhs->getDimension() < rhs->getDimension()) {
                return false;
            } else {
                return lhs->getKeyFrame().getTime() > rhs->getKeyFrame().getTime();
            }
        }
    }
}
#endif

NATRON_NAMESPACE_EXIT;
