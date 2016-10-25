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



#include "Gui/GuiFwd.h"

#include "Global/GlobalDefines.h"
#include "Engine/Curve.h"
#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;


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


struct KeyFrameWithString
{
    KeyFrame key;
    std::string string;
};

struct KeyFrameWithString_CompareLess
{
    bool operator()(const KeyFrameWithString& lhs, const KeyFrameWithString& rhs) const
    {
        return lhs.key.getTime() < rhs.key.getTime();
    }
};

typedef std::set<KeyFrameWithString, KeyFrameWithString_CompareLess> KeyFrameWithStringSet;

typedef std::map<AnimItemDimViewIndexID, KeyFrameWithStringSet, AnimItemDimViewIndexID_CompareLess> AnimItemDimViewKeyFramesMap;

class AnimItemBasePrivate;
class AnimItemBase : public boost::enable_shared_from_this<AnimItemBase>
{

public:

    AnimItemBase(const AnimationModulePtr& model);

    virtual ~AnimItemBase();

    /**
     * @brief Return the model containing this item
     **/
    AnimationModulePtr getModel() const;

    /**
     * @brief Return a pointer to the internal object
     **/
    virtual AnimatingObjectIPtr getInternalAnimItem() const = 0;

    /**
     * @brief Returns the root tree item for this item
     **/
    virtual QTreeWidgetItem * getRootItem() const = 0;

    /**
     * @brief Returns for the given dimension and view the associated tree item.
     **/
    virtual QTreeWidgetItem * getTreeItem(DimSpec dimension, ViewSetSpec view) const = 0;

    /**
     * @brief If the given item corresponds to one of this object tree items, returns to which dimension and view
     * it corresponds to.
     * @return True if it could be mapped to a dimension/view, false otherwise.
     **/
    virtual bool getTreeItemViewDimension(QTreeWidgetItem* item, DimSpec* dimension, ViewSetSpec* view, AnimatedItemTypeEnum* type) const = 0;

    virtual CurvePtr getCurve(DimIdx dimension, ViewIdx view) const = 0;

    /**
     * @brief Get keyframes associated to the item
     **/
    void getKeyframes(DimSpec dimension, ViewSetSpec view, KeyFrameWithStringSet *result) const;

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




/*class AnimKeyFramePrivate;
class AnimKeyFrame
{
public:

    AnimKeyFrame(const AnimItemBasePtr& item, const KeyFrame& kf, DimSpec dimension, ViewSetSpec view);

    AnimKeyFrame(const AnimKeyFrame &other);

    bool operator==(const AnimKeyFrame &other) const;

    AnimItemBasePtr getContext() const;

    const KeyFrame& getKeyFrame() const;

    const std::string& getStringValue() const;

    DimSpec getDimension() const;

    ViewSetSpec getView() const;

private:

    boost::scoped_ptr<AnimKeyFramePrivate> _imp;
};
struct SortIncreasingFunctor
{
    bool operator() (const AnimKeyFramePtr& lhs,
                     const AnimKeyFramePtr& rhs);
};

struct SortDecreasingFunctor
{
    bool operator() (const AnimKeyFramePtr& lhs,
                     const AnimKeyFramePtr& rhs);
};
*/

class KnobsHolderAnimBase
{
public:

    virtual const std::vector<KnobAnimPtr>& getKnobs() const = 0;

    virtual void refreshKnobsVisibility() = 0;

    virtual void refreshVisibility() = 0;

    
};


NATRON_NAMESPACE_EXIT;

#endif // NATRON_GUI_ANIMITEMBASE_H

