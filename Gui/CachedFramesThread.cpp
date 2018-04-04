/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

#include "Engine/Cache.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/Node.h"
#include "Engine/ImageCacheKey.h"
#include "Engine/ImageCacheEntry.h"
#include "Engine/Timer.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"

NATRON_NAMESPACE_ENTER

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
    , mustQuitMutex()
    , mustQuitCond()
    , mustQuit(false)
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

    bool refreshCachedFramesInternal();
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

    if (!isRunning()) {
        return;
    }
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

bool
CachedFramesThread::Implementation::refreshCachedFramesInternal()
{
    ViewerCachedImagesMap framesDisplayed;
    viewer->getViewer()->getViewerProcessHashStored(&framesDisplayed);

    ViewerNodePtr internalNode = viewer->getInternalNode();
    if (!internalNode) {
        return false;
    }
    ViewerInstancePtr internalViewerProcessNode = internalNode->getViewerProcessNode(0);
    if (!internalViewerProcessNode) {
        return false;
    }
    // For all frames in the map:
    // 1) Check the hash is still valid at that frame
    // 2) Check if the cache still has a tile entry for this frame


    std::set<TimeValue> existingCachedFrames;
    std::set<TimeValue> updatedCachedFrames;
    {
        QMutexLocker k(&cachedFramesMutex);
        for (std::list<TimeValue>::const_iterator it = cachedFrames.begin(); it!=cachedFrames.end(); ++it) {
            existingCachedFrames.insert(*it);
        }
    }
    bool hasChanged = false;
    for (ViewerCachedImagesMap::const_iterator it = framesDisplayed.begin(); it != framesDisplayed.end(); ++it) {

        bool isValid = true;


        // Check if it is still cached
        {
            U64 hash = it->second->getHash();
            bool isCached = appPTR->getTileCache()->hasCacheEntryForHash(hash);
            if (!isCached) {
                isValid = false;

            }
        }


        if (isValid) {
            U64 nodeFrameViewHash;
            {
                HashableObject::ComputeHashArgs hashArgs;
                hashArgs.hashType = HashableObject::eComputeHashTypeTimeViewVariant;
                hashArgs.time = it->first.time;
                hashArgs.view = it->first.view;
                nodeFrameViewHash = internalViewerProcessNode->computeHash(hashArgs);
            }
            // Check if the node hash is the same
            U64 entryNodeHash = it->second->getNodeTimeVariantHashKey();
            if (nodeFrameViewHash != entryNodeHash) {
                isValid = false;
            }
        }
        
        if (!isValid) {
            viewer->getViewer()->removeViewerProcessHashAtTime(it->first.time, it->first.view);
        } else {
            updatedCachedFrames.insert(it->first.time);
            std::set<TimeValue>::iterator found = existingCachedFrames.find(it->first.time);
            if (found == existingCachedFrames.end()) {
                // We added a keyframe that didnt exist
                hasChanged = true;
            }
        }

    }

    if (!hasChanged) {
        // Check if we removed a keyframe that existed
        for (std::set<TimeValue>::iterator it = existingCachedFrames.begin(); it != existingCachedFrames.end(); ++it) {
            std::set<TimeValue>::iterator found = updatedCachedFrames.find(*it);
            if (found == updatedCachedFrames.end()) {
                // We removed a keyframe
                hasChanged = true;
                break;
            }
        }
    }

    if (hasChanged) {
        QMutexLocker k(&cachedFramesMutex);
        cachedFrames.clear();
        cachedFrames.insert(cachedFrames.end(),updatedCachedFrames.begin(), updatedCachedFrames.end());
        return true;
    }
    return false;

} // refreshCachedFramesInternal

void
CachedFramesThread::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    for (;;) {

        if (_imp->checkForExit()) {
            return;
        }

        {
            if (_imp->refreshCachedFramesInternal()) {
                Q_EMIT cachedFramesRefreshed();
            }
        }

        _imp->regulatingTimer.waitUntilNextFrameIsDue();
        
    } // infinite loop
} // run

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_CachedFramesThread.cpp"
