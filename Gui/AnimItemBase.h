/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_ANIMITEMBASE_H
#define NATRON_GUI_ANIMITEMBASE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>
#include <map>
#include <set>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif


#include <QtCore/QString>

#include "Gui/GuiFwd.h"

#include "Global/GlobalDefines.h"
#include "Engine/Curve.h"
#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER


/**
 * @brief  uniquely identifies a valid curve for an animation item
 **/
struct AnimItemDimViewIndexID
{
    AnimItemBasePtr item;
    ViewIdx view;
    DimIdx dim;

    AnimItemDimViewIndexID()
    : item()
    , view()
    , dim()
    {

    }

    AnimItemDimViewIndexID(const AnimItemBasePtr& item, ViewIdx view, DimIdx dim)
    : item(item)
    , view(view)
    , dim(dim)
    {

    }

};

struct AnimItemDimViewIndexID_CompareLess
{
    bool operator()(const AnimItemDimViewIndexID& lhs, const AnimItemDimViewIndexID& rhs) const
    {

        if (lhs.view < rhs.view) {
            return true;
        } else if (lhs.view > rhs.view) {
            return false;
        } else {
            if (lhs.dim < rhs.dim) {
                return true;
            } else if (lhs.dim > rhs.dim) {
                return false;
            } else {
                return lhs.item < rhs.item;
            }
        }

    }
};


struct AnimItemDimViewKeyFrame
{
    AnimItemDimViewIndexID id;
    KeyFrame key;
};


typedef std::map<AnimItemDimViewIndexID, KeyFrameSet, AnimItemDimViewIndexID_CompareLess> AnimItemDimViewKeyFramesMap;

class AnimItemBasePrivate;
class AnimItemBase : public boost::enable_shared_from_this<AnimItemBase>
{

public:

    AnimItemBase(const AnimationModuleBasePtr& model);

    virtual ~AnimItemBase();

    /**
     * @brief Return the model containing this item
     **/
    AnimationModuleBasePtr getModel() const;

    /**
     * @brief Return a pointer to the internal object
     **/
    virtual AnimatingObjectIPtr getInternalAnimItem() const = 0;

    /**
     * @brief Return the label of the curve in the given dimension/view
     **/
    virtual QString getViewDimensionLabel(DimIdx dimension, ViewIdx view) const = 0;

    /**
     * @brief Returns the root tree item for this item
     **/
    virtual QTreeWidgetItem * getRootItem() const = 0;

    /**
     * @brief Returns for the given dimension and view the associated tree item.
     **/
    virtual QTreeWidgetItem * getTreeItem(DimSpec dimension, ViewSetSpec view) const = 0;

    /**
     * @brief Returns the curve at the given dimension and view
     **/
    virtual CurvePtr getCurve(DimIdx dimension, ViewIdx view) const = 0;

    /**
     * @brief Returns the gui curve at the given dimension and view
     **/
    virtual CurveGuiPtr getCurveGui(DimIdx dimension, ViewIdx view) const = 0;

    /**
     * @brief Returns true if all dimensions are visible for the given view
     **/
    virtual bool getAllDimensionsVisible(ViewIdx view) const = 0;

    /**
     * @brief Returns true if the given dimension/view has an expression set
     **/
    virtual bool hasExpression(DimIdx dimension, ViewIdx view) const
    {
        Q_UNUSED(dimension);
        Q_UNUSED(view);
        return false;
    }

    /**
     * @brief Evaluates the curve at given dimension and parametric X and return the Y value
     * @param useExpressionIfAny If true and if the underlying curve is covered by an expression
     * this should return the value of the expression instead.
     * The default implementation doesn't know about expression and just returns the curve value
     **/
    virtual double evaluateCurve(bool useExpressionIfAny, double x, DimIdx dimension, ViewIdx view);

    enum GetKeyframesTypeEnum
    {
        // If passing DimSpec::all() or ViewSetSpec::all(), only keyframes that
        // are in all dimensions/views are considered for potential result.
        eGetKeyframesTypeOnlyIfAll,

        // All keyframes are merged into the resulting keyframes set.
        eGetKeyframesTypeMerged
    };

    /**
     * @brief Returns a set of all keyframes for given dimension/view
     * If dimension is all or view is all it will merge keyframes of all dimensions/views
     * respectively. In case all is passed to either dimension or view, the value of the
     * keyframes is irrelevant.
     **/
    void getKeyframes(DimSpec dimension, ViewSetSpec view, GetKeyframesTypeEnum type, KeyFrameSet *result) const;

    /**
     * @brief Same as getKeyframes above, except that each dimension/view pair has its own keyframe set 
     **/
    void getKeyframes(DimSpec dimension, ViewSetSpec view, AnimItemDimViewKeyFramesMap *result) const;

    /**
     * @brief Returns the views available in the item.
     **/
    virtual std::list<ViewIdx> getViewsList() const = 0;

    /**
     * @brief Returns the number of dimensions in the item
     **/
    virtual int getNDimensions() const = 0;

private:

    boost::scoped_ptr<AnimItemBasePrivate> _imp;
};



class KnobsHolderAnimBase
{
public:

    KnobsHolderAnimBase() {}
    
    virtual ~KnobsHolderAnimBase() {}

    KnobAnimPtr findKnobAnim(const KnobIPtr& knob) const;

    virtual const std::vector<KnobAnimPtr>& getKnobs() const = 0;

    virtual void refreshVisibility() = 0;
    
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_ANIMITEMBASE_H

