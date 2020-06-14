/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef ROTOSHAPERENDERNODE_H
#define ROTOSHAPERENDERNODE_H

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


#define kRotoShapeRenderNodeParamType "type"
#define kRotoShapeRenderNodeParamTypeLabel "Type"

#define kRotoShapeRenderNodeParamTypeSolid "Solid"
#define kRotoShapeRenderNodeParamTypeSmear "Smear"


class RotoShapeRenderNodePrivate;
class RotoShapeRenderNode
    : public EffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    RotoShapeRenderNode(NodePtr n);

    RotoShapeRenderNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);
public:

    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RotoShapeRenderNode(node) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RotoShapeRenderNode(mainInstance, key) );
    }


    static PluginPtr createPlugin();

    virtual ~RotoShapeRenderNode();

    static EffectInstance* BuildEffect(NodePtr n)
    {
        return new RotoShapeRenderNode(n);
    }


    virtual bool canCPUImplementationSupportOSMesa() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash)  OVERRIDE FINAL;

private:

    virtual KnobHolderPtr createRenderCopy(const FrameViewRenderKey& key) const OVERRIDE FINAL;

    virtual void fetchRenderCloneKnobs() OVERRIDE FINAL;

    virtual void initializeKnobs() OVERRIDE FINAL;

    virtual void purgeCaches() OVERRIDE FINAL;

    virtual ActionRetCodeEnum getLayersProducedAndNeeded(TimeValue time,
                                                         ViewIdx view,
                                                         std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                                         std::list<ImagePlaneDesc>* layersProduced,
                                                         TimeValue* passThroughTime,
                                                         ViewIdx* passThroughView,
                                                         int* passThroughInputNb) OVERRIDE FINAL;

    virtual ActionRetCodeEnum attachOpenGLContext(TimeValue time, ViewIdx view, const RenderScale& scale,  const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data) OVERRIDE FINAL;

    virtual ActionRetCodeEnum dettachOpenGLContext(const OSGLContextPtr& glContext, const EffectOpenGLContextDataPtr& data) OVERRIDE FINAL;

    virtual ActionRetCodeEnum getRegionOfDefinition(TimeValue time, const RenderScale & scale, ViewIdx view, RectD* rod) OVERRIDE WARN_UNUSED_RETURN;

    virtual ActionRetCodeEnum getTimeInvariantMetadata(NodeMetadata& metadata) OVERRIDE FINAL;

    virtual ActionRetCodeEnum getFramesNeeded(TimeValue time, ViewIdx view,  FramesNeededMap* results) OVERRIDE FINAL;

    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                                         const RenderScale & scale,
                                         const RectI & roi,
                                         ViewIdx view,
                                         const ImagePlaneDesc& plane,
                                         TimeValue* inputTime,
                                         ViewIdx* inputView,
                                         int* inputNb,
                                         ImagePlaneDesc* inputPlane) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual ActionRetCodeEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;

    boost::shared_ptr<RotoShapeRenderNodePrivate> _imp;

};

NATRON_NAMESPACE_EXIT

#endif // ROTOSHAPERENDERNODE_H
