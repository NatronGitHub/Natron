/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef REMOVEPLANENODE_H
#define REMOVEPLANENODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/NoOpBase.h"

NATRON_NAMESPACE_ENTER

struct RemovePlaneNodePrivate;
class RemovePlaneNode : public NoOpBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:

    RemovePlaneNode(const NodePtr& n);

    RemovePlaneNode(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);

public:

    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RemovePlaneNode(node) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RemovePlaneNode(mainInstance, key) );
    }

    static PluginPtr createPlugin();

    virtual ~RemovePlaneNode();

    
private:

    virtual ActionRetCodeEnum getLayersProducedAndNeeded(TimeValue time,
                                                         ViewIdx view,
                                                         std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded,
                                                         std::list<ImagePlaneDesc>* layersProduced,
                                                         TimeValue* passThroughTime,
                                                         ViewIdx* passThroughView,
                                                         int* passThroughInputNb) OVERRIDE FINAL;

    virtual void onMetadataChanged(const NodeMetadata& metadata) OVERRIDE FINAL;

    virtual void fetchRenderCloneKnobs() OVERRIDE FINAL;

    virtual void initializeKnobs() OVERRIDE FINAL;

    boost::scoped_ptr<RemovePlaneNodePrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // REMOVEPLANENODE_H
