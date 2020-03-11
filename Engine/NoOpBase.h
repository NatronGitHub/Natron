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

#ifndef Engine_NoOpBase_h
#define Engine_NoOpBase_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief A NoOp is an effect that doesn't do anything. It is useful for scripting (adding custom parameters)
 * and it is also used to implement the "Dot" node.
 **/
class NoOpBase
    : public EffectInstance
{
public:

protected: // derives from EffectInstance, parent of Backdrop, Dot, groupInput, GroupOutput
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    NoOpBase(const NodePtr& n);

    NoOpBase(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);

private:

    /**
     * @brief A NoOp is always an identity on its input.
     **/
    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                                         const RenderScale & scale,
                                         const RectI & roi,
                                         ViewIdx view,
                                         const ImagePlaneDesc& plane,
                                         TimeValue* inputTime,
                                         ViewIdx* inputView,
                                         int* inputNb,
                                         ImagePlaneDesc* inputPlane) OVERRIDE FINAL WARN_UNUSED_RETURN;
};


inline NoOpBasePtr
toNoOpBase(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<NoOpBase>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // Engine_NoOpBase_h
