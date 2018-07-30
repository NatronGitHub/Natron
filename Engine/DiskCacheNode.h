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

#ifndef DISKCACHENODE_H
#define DISKCACHENODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct DiskCacheNodePrivate;

class DiskCacheNode
    : public EffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    DiskCacheNode(const NodePtr& node);

    DiskCacheNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new DiskCacheNode(node) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new DiskCacheNode(mainInstance, key) );
    }

    static PluginPtr createPlugin();

    virtual ~DiskCacheNode();

    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual ActionRetCodeEnum getFrameRange(double *first, double *last) OVERRIDE FINAL;


private:

    virtual void fetchRenderCloneKnobs() OVERRIDE FINAL;

    virtual ActionRetCodeEnum render(const RenderActionArgs& args) OVERRIDE FINAL;

    virtual bool knobChanged(const KnobIPtr&,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time) OVERRIDE FINAL;
    virtual bool shouldCacheOutput(bool isFrameVaryingOrAnimated, int visitsCount) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    boost::scoped_ptr<DiskCacheNodePrivate> _imp;
};


inline DiskCacheNodePtr
toDiskCacheNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<DiskCacheNode>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // DISKCACHENODE_H
