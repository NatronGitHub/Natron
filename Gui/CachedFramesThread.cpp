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

#include "CachedFramesThread.h"

#include <map>
#include <QMutex>
#include <QWaitCondition>

#include "Global/GlobalDefines.h"

#include "Engine/Cache.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/Node.h"
#include "Engine/Timer.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"

NATRON_NAMESPACE_ENTER;

struct CachedFramesThread::Implementation
{
    ViewerTab* viewer;

    QMutex cachedFramesMutex;
    std::list<TimeValue> cachedFrames;

    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;

    // A timer to avoid refreshing the cached frames too much
    Timer regulatingTimer;

    Implementation(ViewerTab* viewer)
    : viewer(viewer)
    , cachedFramesMutex()
    , cachedFrames()
    , regulatingTimer()
    {

    }

    bool checkForExit()
    {
        QMutexLocker l(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            mustQuitCond.wakeOne();
            return true;
        }
        return false;
    }

    void refreshCachedFramesInternal();
};

CachedFramesThread::CachedFramesThread(ViewerTab* viewer)
: QThread()
, _imp(new Implementation(viewer))
{
    // Maximum 5 fps for the refreshing of the cache line
    _imp->regulatingTimer.setDesiredFrameRate(5);

    TimeLineGui* timeline = viewer->getTimeLineGui();
    QObject::connect(this, SIGNAL(cachedFramesRefreshed()), timeline, SLOT(update()));
}

CachedFramesThread::~CachedFramesThread()
{

}


void
CachedFramesThread::quitThread()
{

    QMutexLocker k(&_imp->mustQuitMutex);
    _imp->mustQuit = true;

    // Push a fake request and wait til the thread is done.

    while (_imp->mustQuit) {
        _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
    }
    wait();
}

void
CachedFramesThread::getCachedFrames(std::list<TimeValue>* cachedFrames) const
{
    QMutexLocker k(&_imp->cachedFramesMutex);
    *cachedFrames = _imp->cachedFrames;
}

void
CachedFramesThread::Implementation::refreshCachedFramesInternal()
{
    std::map<TimeValue, ImageTileKeyPtr> framesDisplayed;
    viewer->getViewer()->getViewerProcessHashStored(&framesDisplayed);

    ViewerNodePtr internalNode = viewer->getInternalNode();
    if (!internalNode) {
        return;
    }
    ViewerInstancePtr internalViewerProcessNode = internalNode->getViewerProcessNode(0);

    // For all frames in the map:
    // 1) Check the hash is still valid at that frame
    // 2) Check if the cache still has a tile entry for this frame

    std::list<TimeValue> updatedCachedFrames;

    for (std::map<TimeValue, ImageTileKeyPtr>::const_iterator it = framesDisplayed.begin(); it != framesDisplayed.end(); ++it) {


        U64 hash = it->second->getHash();


        bool isValid = false;

        bool isCached = appPTR->getCache()->hasCacheEntryForHash(hash);
        if (!isCached) {
            isValid = false;

        }


        U64 nodeFrameViewHash;
        {
            HashableObject::ComputeHashArgs hashArgs;
            hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
            hashArgs.time = it->second->getTime();
            hashArgs.view = it->second->getView();
            nodeFrameViewHash = internalViewerProcessNode->computeHash(hashArgs);
        }
        if (nodeFrameViewHash != hash) {
            isValid = false;
        }

        if (!isValid) {
            viewer->getViewer()->removeViewerProcessHashAtTime(it->first);
        } else {
            updatedCachedFrames.push_back(it->first);
        }

    }

    {
        QMutexLocker k(&cachedFramesMutex);
        cachedFrames = updatedCachedFrames;
    }

} // refreshCachedFramesInternal

void
CachedFramesThread::run()
{
    for (;;) {

        if (_imp->checkForExit()) {
            return;
        }

        {
            _imp->refreshCachedFramesInternal();
            Q_EMIT cachedFramesRefreshed();
        }

        _imp->regulatingTimer.waitUntilNextFrameIsDue();
        
    } // infinite loop
} // run

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_CachedFramesThread.cpp"
