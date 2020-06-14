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



// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TreeRenderQueueProvider.h"

#include <QMutex>

#include "Engine/AppManager.h"
#include "Engine/TreeRenderQueueManager.h"

NATRON_NAMESPACE_ENTER

struct TreeRenderQueueProvider::Implementation
{
    QMutex isWaitingForAnyRendersFinishedMutex;
    bool isWaitingForAnyRendersFinished;

    Implementation()
    : isWaitingForAnyRendersFinishedMutex()
    , isWaitingForAnyRendersFinished(false)
    {
        
    }
};

TreeRenderQueueProvider::TreeRenderQueueProvider()
: _imp(new Implementation())
{

}

TreeRenderQueueProvider::~TreeRenderQueueProvider()
{

}

void TreeRenderQueueProvider::launchRender(const TreeRenderPtr& render)
{
    return appPTR->getTasksQueueManager()->launchRender(render);
}

TreeRenderExecutionDataPtr
TreeRenderQueueProvider::launchSubRender(const EffectInstancePtr& treeRoot,
                                         TimeValue time,
                                         ViewIdx view,
                                         const RenderScale& proxyScale,
                                         unsigned int mipMapLevel,
                                         const ImagePlaneDesc* planeParam,
                                         const RectD* canonicalRoIParam,
                                         const TreeRenderPtr& render,
                                         int concatenationFlags,
                                         bool createTreeRenderIfUnrenderedImage)
{
   return appPTR->getTasksQueueManager()->launchSubRender(treeRoot, time, view, proxyScale, mipMapLevel, planeParam, canonicalRoIParam, render, concatenationFlags, createTreeRenderIfUnrenderedImage);
}

ActionRetCodeEnum
TreeRenderQueueProvider::waitForRenderFinished(const TreeRenderPtr& render)
{
    return appPTR->getTasksQueueManager()->waitForRenderFinished(render);
}

bool
TreeRenderQueueProvider::hasTreeRendersLaunched() const
{
    return appPTR->getTasksQueueManager()->hasTreeRendersLaunched(getThisTreeRenderQueueProviderShared());
}

bool
TreeRenderQueueProvider::hasTreeRendersFinished() const
{
    return appPTR->getTasksQueueManager()->hasTreeRendersFinished(getThisTreeRenderQueueProviderShared());
}

TreeRenderPtr
TreeRenderQueueProvider::waitForAnyTreeRenderFinished()
{
    return appPTR->getTasksQueueManager()->waitForAnyTreeRenderFinished(getThisTreeRenderQueueProviderShared());
}

bool
TreeRenderQueueProvider::isWaitingForAllTreeRenders() const
{
    QMutexLocker k(&_imp->isWaitingForAnyRendersFinishedMutex);
    return _imp->isWaitingForAnyRendersFinished;
}

void
TreeRenderQueueProvider::waitForAllTreeRenders()
{
    {
        QMutexLocker k(&_imp->isWaitingForAnyRendersFinishedMutex);
        _imp->isWaitingForAnyRendersFinished = true;
    }

    TreeRenderQueueManagerPtr mngr = appPTR->getTasksQueueManager();
    TreeRenderQueueProviderConstPtr thisShared = getThisTreeRenderQueueProviderShared();

    while (mngr->hasTreeRendersLaunched(thisShared)) {
        mngr->waitForAnyTreeRenderFinished(thisShared);
    }

    {
        QMutexLocker k(&_imp->isWaitingForAnyRendersFinishedMutex);
        _imp->isWaitingForAnyRendersFinished = false;
    }
}

void
TreeRenderQueueProvider::notifyNeedMoreRenders()
{
    // If we are in waitForAllTreeRenders(), do not start more renders because another thread is anyway trying
    // to make all render finish.
    {
        QMutexLocker k(&_imp->isWaitingForAnyRendersFinishedMutex);
        if (_imp->isWaitingForAnyRendersFinished) {
            return;
        }
    }
    requestMoreRenders();
}

void
TreeRenderQueueProvider::notifyTreeRenderFinished(const TreeRenderPtr& render)
{
    onTreeRenderFinished(render);
}

NATRON_NAMESPACE_EXIT
