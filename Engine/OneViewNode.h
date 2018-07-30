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

#ifndef ENGINE_ONEVIEWNODE_H
#define ENGINE_ONEVIEWNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EffectInstance.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


struct OneViewNodePrivate;
class OneViewNode
    : public EffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    OneViewNode(const NodePtr& n);
    OneViewNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new OneViewNode(node) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new OneViewNode(mainInstance, key) );
    }

    static PluginPtr createPlugin();

    virtual ~OneViewNode();

private:

    virtual void fetchRenderCloneKnobs() OVERRIDE;

    virtual void onMetadataChanged(const NodeMetadata& metadata) OVERRIDE FINAL;

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual ActionRetCodeEnum getFramesNeeded(TimeValue time, ViewIdx view, FramesNeededMap* results) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                                         const RenderScale & scale,
                                         const RectI & roi,
                                         ViewIdx view,
                                         const ImagePlaneDesc& plane,
                                         TimeValue* inputTime,
                                         ViewIdx* inputView,
                                         int* inputNb,
                                         ImagePlaneDesc* inputPlane) OVERRIDE FINAL WARN_UNUSED_RETURN;
    boost::scoped_ptr<OneViewNodePrivate> _imp;
};


inline OneViewNodePtr
toOneViewNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<OneViewNode>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // ENGINE_ONEVIEWNODE_H
