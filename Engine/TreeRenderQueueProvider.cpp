/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "TreeRenderQueueProvider.h"

#include "Engine/AppManager.h"
#include "Engine/TreeRenderQueueManager.h"

NATRON_NAMESPACE_ENTER



void TreeRenderQueueProvider::launchRender(const TreeRenderPtr& render)
{
    appPTR->getTasksQueueManager()->launchRender(render);
}

TreeRenderExecutionDataPtr
TreeRenderQueueProvider::launchSubRender(const EffectInstancePtr& treeRoot,
                                         TimeValue time,
                                         ViewIdx view,
                                         const RenderScale& proxyScale,
                                         unsigned int mipMapLevel,
                                         const ImagePlaneDesc* planeParam,
                                         const RectD* canonicalRoIParam,
                                         const TreeRenderPtr& render)
{
   return appPTR->getTasksQueueManager()->launchSubRender(treeRoot, time, view, proxyScale, mipMapLevel, planeParam, canonicalRoIParam, render);
}

void
TreeRenderQueueProvider::waitForTreeRenderExecutionFinished(const TreeRenderExecutionDataPtr& execData)
{
    appPTR->getTasksQueueManager()->waitForTreeRenderExecutionFinished(execData);
}

void
TreeRenderQueueProvider::waitForRenderFinished(const TreeRenderPtr& render)
{
    appPTR->getTasksQueueManager()->waitForRenderFinished(render);
}

bool
TreeRenderQueueProvider::hasTreeRendersFinished() const
{
    appPTR->getTasksQueueManager()->hasTreeRendersFinished(getThisTreeRenderQueueProviderShared());
}

TreeRenderPtr
TreeRenderQueueProvider::waitForAnyTreeRenderFinished()
{
    appPTR->getTasksQueueManager()->waitForAnyTreeRenderFinished(getThisTreeRenderQueueProviderShared());
}


NATRON_NAMESPACE_EXIT
