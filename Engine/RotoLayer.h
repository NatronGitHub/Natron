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

#ifndef Engine_RotoLayer_h
#define Engine_RotoLayer_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated-declarations)
#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"
#include "Engine/RotoItem.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @class A base class for all items made by the roto context
 **/

/**
 * @class A RotoLayer is a group of RotoItem. This allows the context to sort
 * and build hierarchies of layers.
 * Children items are rendered in reverse order of their ordering in the children list
 * i.e: the last item will be rendered first, etc...
 * Visually, in the GUI the top-most item of a layer corresponds to the first item in the children list
 **/
class RotoLayer
    : public RotoItem
{
public:

    RotoLayer(const KnobItemsTablePtr& model);

    RotoLayer(const RotoLayerPtr& other, const FrameViewRenderKey& key);

    virtual ~RotoLayer();

    virtual bool isItemContainer() const OVERRIDE FINAL;

    virtual std::string getBaseItemName() const OVERRIDE;

    virtual std::string getSerializationClassName() const OVERRIDE;
};

inline RotoLayerPtr
toRotoLayer(const KnobHolderPtr& item)
{
    return boost::dynamic_pointer_cast<RotoLayer>(item);
}

struct PlanarTrackLayerPrivate;
class PlanarTrackLayer : public RotoLayer
{
public:

    PlanarTrackLayer(const KnobItemsTablePtr& model);

    PlanarTrackLayer(const PlanarTrackLayerPtr& other, const FrameViewRenderKey& key);

    virtual ~PlanarTrackLayer();

    virtual std::string getBaseItemName() const OVERRIDE FINAL;

    virtual std::string getSerializationClassName() const OVERRIDE FINAL;

    virtual bool isRenderCloneNeeded() const OVERRIDE FINAL
    {
        return true;
    }

    virtual bool getTransformAtTimeInternal(TimeValue time, ViewIdx view, Transform::Matrix3x3* matrix) const OVERRIDE FINAL;

    void setExtraMatrix(bool setKeyframe, TimeValue time, ViewSetSpec view, const Transform::Matrix3x3& mat);

    void clearTransformAnimation();
    void clearTransformAnimationBeforeTime(TimeValue time);
    void clearTransformAnimationAfterTime(TimeValue time);
    void deleteTransformKeyframe(TimeValue time);

    void getTransformKeyframes(std::list<double>* keys) const;

    void getExtraMatrixAtTime(TimeValue time, ViewIdx view, Transform::Matrix3x3* mat) const;

    TimeValue getReferenceFrame() const;

    KnobDoublePtr getCornerPinPointKnob(int index) const;
    KnobDoublePtr getCornerPinPointOffsetKnob(int index) const;
    KnobChoicePtr getMotionModelKnob() const;

private:

    virtual void initializeKnobs() OVERRIDE FINAL;

    virtual void fetchRenderCloneKnobs() OVERRIDE FINAL;

    virtual KnobHolderPtr createRenderCopy(const FrameViewRenderKey& render) const OVERRIDE FINAL;

    boost::scoped_ptr<PlanarTrackLayerPrivate> _imp;

};


inline PlanarTrackLayerPtr
toPlanarTrackLayer(const KnobHolderPtr& item)
{
    return boost::dynamic_pointer_cast<PlanarTrackLayer>(item);
}


NATRON_NAMESPACE_EXIT

#endif // Engine_RotoLayer_h
