/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef Engine_JoinViewsNode_h
#define Engine_JoinViewsNode_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EffectInstance.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


class JoinViewsNode
    : public EffectInstance
{

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    JoinViewsNode(const NodePtr& node);

    JoinViewsNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new JoinViewsNode(node) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new JoinViewsNode(mainInstance, key) );
    }

    static PluginPtr createPlugin();

    virtual ~JoinViewsNode();

    virtual void initializeKnobs() OVERRIDE FINAL;


private:

    virtual void onMetadataChanged(const NodeMetadata& metadata) OVERRIDE FINAL;

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


inline JoinViewsNodePtr
toJoinViewsNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<JoinViewsNode>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // Engine_JoinViewsNode_h
